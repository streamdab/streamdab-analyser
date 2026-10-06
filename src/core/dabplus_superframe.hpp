/**
 * @file dabplus_superframe.hpp
 * @brief Shared DAB+ transport layer: 5x24 ms CIF superframe Assembly with
 *        FireCode sync, Access-Unit table decoding, per-AU CRC and DSE/PAD
 *        location (ETSI TS 102 563 §4–§7, ETSI EN 300 401 §7.4).
 *
 * This module holds ONLY the transport layer that the DLS+/MOT GUI parser
 * (src/main.cpp) and the T34 audio decoder (dabplus_audio_decoder.cpp) used to
 * implement twice with slight divergence. Everything above the transport layer
 * (F-PAD/X-PAD CI decode, DLS/DL+ assembly, MOT data-group reassembly, ASC
 * construction and libfaad2 decode) stays with the callers.
 *
 * What lives here:
 *   1. 5 x 24 ms CIF chunk accumulation with sliding FireCode sync.
 *   2. AU count / start-offset decoding from the superframe header nibbles.
 *   3. Per-AU CRC-16/CCITT-FALSE validation (the two trailing AU bytes).
 *   4. DSE (Data Stream Element) detection and the PAD offset/length at the
 *      start of an AU payload.
 *
 * What deliberately does NOT live here:
 *   - FireCode is NOT eti::crc16ccitt_false() from crc16.hpp: the FireCode is
 *     poly 0x782F / init 0x0000 (TS 102 563 §6.2) whereas the FIB/AU CRC is
 *     poly 0x1021 / init 0xFFFF. The two are not interchangeable, so the
 *     FireCode variant is kept here with firecodeCrc().
 *
 * Thread-safety: SuperframeAssembler is not thread-safe; one instance per
 * sub-channel, owned and serialized by the caller.
 *
 * @author C++ Qt Developer Agent (T38)
 */

#pragma once

#include <QByteArray>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace streamdab::dabplus {

/// F-PAD length in bytes (ETSI EN 300 401 §7.4); the minimum valid PAD length.
inline constexpr int kFpadLen = 2;

/**
 * @brief FireCode CRC-16 (poly 0x782F, init 0x0000), MSB-first.
 *
 * ETSI TS 102 563 §6.2 / ODR-Dab etisnoop firecode.c. This protects the 9
 * bytes that follow the CRC in a DAB+ superframe header. It is intentionally
 * distinct from eti::crc16ccitt_false() (poly 0x1021, init 0xFFFF).
 *
 * @param data Buffer start (may be nullptr when len == 0)
 * @param len  Number of bytes
 * @return     Raw FireCode CRC-16 value
 */
uint16_t firecodeCrc(const uint8_t* data, std::size_t len);

/**
 * @brief DAB+ superframe format (the bit field in superframe byte 2).
 *
 * Field names follow the reference decoders (DABlin `SuperframeFormat`).
 */
struct SuperframeFormat {
    bool dacRate = false;        ///< byte2 & 0x40 (0 = 32 kHz core, 1 = 48 kHz core)
    bool sbr = false;            ///< byte2 & 0x20 (Spectral Band Replication)
    bool aacChannelMode = false; ///< byte2 & 0x10 (stereo core)
    bool ps = false;             ///< byte2 & 0x08 (Parametric Stereo; implies SBR)
    int numAus = 0;              ///< Access Units per 120 ms superframe
    int bitrateKbps = 0;         ///< superframeLength/120*8 (TS 102 563 §4)
};

/**
 * @brief Transport view of one Access Unit inside a superframe.
 *
 * `offset`/`size` locate the AU (payload + 2 trailing CRC bytes) inside
 * SuperframeResult::data. `padOffset`/`padLen` locate the DSE-carried PAD
 * region inside the AU payload (i.e. relative to `offset`; the payload length
 * is `size - 2`). `padOffset < 0` means "no usable DSE/PAD".
 */
struct AccessUnit {
    int offset = 0;     ///< byte offset of the AU in the superframe
    int size = 0;       ///< AU size in bytes (payload + 2 trailing CRC bytes)
    bool crcOk = false; ///< AU CRC-16/CCITT-FALSE validated
    int padOffset = -1; ///< PAD byte offset within the AU payload (-1 = no DSE)
    int padLen = 0;     ///< announced PAD length in bytes (0 when padOffset < 0)
};

/// FireCode sync state of the candidate superframe window.
enum class SuperframeSync {
    Unlocked, ///< FireCode/plausibility failed — not a superframe boundary
    Locked,   ///< candidate is a valid DAB+ superframe boundary
};

/// Outcome of evaluating one candidate superframe window.
struct SuperframeResult {
    /// A full candidate window of valid size was evaluated (size % 120 == 0).
    bool examined = false;
    /// FireCode/plausibility result. A locked window consumes the full
    /// superframe; an unlocked one consumes a single chunk (sliding sync).
    SuperframeSync sync = SuperframeSync::Unlocked;
    /// superframe size / 120 (bitrate kbps = value*8); AU data ends at
    /// value*110. Named "sub-channel index" historically.
    int subChannelIndex = 0;
    /// Raw superframe byte 2 (the audio format nibble byte).
    uint8_t audioParams = 0;
    /// Decoded superframe format (numAus is only meaningful when Locked).
    SuperframeFormat format;
    /// AU start offsets decoded and strictly increasing.
    bool offsetsValid = false;
    /// AUs with size >= 2 (in transmission order). Populated when offsetsValid.
    std::vector<AccessUnit> aus;
    /// The candidate superframe bytes; `aus[].offset` indexes into this.
    QByteArray data;
};

/**
 * @brief Stateful per-sub-channel 5 x 24 ms CIF accumulator.
 *
 * feedChunk() appends one ETI frame's sub-channel slice. Once 5 chunks have
 * accumulated it evaluates the leading window. On a FireCode failure exactly
 * ONE chunk is dropped (sliding sync) so the real boundary is re-acquired
 * quickly; on success the whole superframe is consumed.
 */
class SuperframeAssembler {
public:
    /// Clear the accumulator (chunk length and buffer).
    void reset();

    /**
     * @brief Feed one CIF chunk for this sub-channel.
     * @return The result of the (at most one) candidate window evaluated by
     *         this call; `examined == false` when fewer than 5 chunks are held
     *         OR the held window is not a multiple of 120 bytes (chunk length
     *         not a multiple of 24 ms).
     */
    SuperframeResult processChunk(const QByteArray& chunk);

    /**
     * @brief Stateless evaluation of an already-assembled candidate superframe.
     *
     * Used by callers/tests that hold a whole superframe (e.g. the GUI
     * synthetic-superframe hook). Does not accumulate or consume any state.
     */
    static SuperframeResult processSuperframe(const QByteArray& superframe);

    /// Current per-chunk length (0 before the first chunk).
    int chunkLength() const { return m_chunkLen; }

private:
    QByteArray m_buffer;
    int m_chunkLen = 0;
};

} // namespace streamdab::dabplus
