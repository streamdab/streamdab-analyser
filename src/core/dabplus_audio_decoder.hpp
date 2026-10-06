/**
 * @file dabplus_audio_decoder.hpp
 * @brief DAB+ (ETSI TS 102 563) audio superframe/Access-Unit extraction and
 *        HE-AAC decoding to PCM (T34).
 *
 * This module is the audio counterpart of the existing DLS+/MOT feed path used
 * by the GUI: it reuses the same MSC sub-channel slices the ETI processor
 * already extracts (ProcessedFrame::sub_channels) and applies the identical,
 * spec-correct DAB+ superframe split (FireCode -> 5-CIF 120 ms superframe ->
 * AU start offsets -> AU CRC) implemented once in the shared transport module
 * `dabplus_superframe.{hpp,cpp}` (T38), but only the audio payload is kept.
 * Each CRC-valid Access Unit (raw AAC frame, including its embedded Data Stream
 * Element/PAD) can then be decoded sequentially with libfaad2 (the
 * AudioSpecificConfig construction follows ODR-Dab DABlin's dabplus_decoder,
 * cross-checked against ETSI TS 102 563).
 *
 * Design notes
 *  - Extraction and decoding are separate entry points so a caller can retain
 *    the (small) encoded AU stream during a full ETI load and decode it later,
 *    on demand, without keeping PCM for every service in memory.
 *  - The decoder is NOT thread-safe; the owner must serialize access. The
 *    playback controller keeps it on the GUI thread (decode is far faster than
 *    real time) and hands only the finished PCM buffer to the audio sink.
 *  - When libfaad2 is unavailable at build time the extraction side still
 *    works; decoding reports an explicit error instead of fabricating audio.
 *
 * @author C++ Qt Developer Agent (T34)
 */

#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "dabplus_superframe.hpp"  // T38: shared superframe/AU/PAD transport layer

#ifdef HAVE_FAAD2
extern "C" {
#include <neaacdec.h>
}
#else
struct NeAACDecStruct;
typedef NeAACDecStruct* NeAACDecHandle;
struct NeAACDecFrameInfo;
#endif

namespace streamdab::audio {

/**
 * @brief DAB+ audio superframe format (ETSI TS 102 563 §4).
 *
 * Field names follow the reference decoders (DABlin `SuperframeFormat`):
 * the bitmap lives in superframe byte 2.
 */
struct DabPlusFormat {
    bool dacRate = false;         ///< sf[2] & 0x40 (0 = 32 kHz core, 1 = 48 kHz core)
    bool sbr = false;             ///< sf[2] & 0x20 (Spectral Band Replication)
    bool aacChannelMode = false;  ///< sf[2] & 0x10 (stereo core)
    bool ps = false;              ///< sf[2] & 0x08 (Parametric Stereo, implies SBR)
    int numAus = 0;               ///< Access Units per 120 ms superframe
    int subChannelId = -1;        ///< ETI sub-channel id (extraction context)
    quint32 serviceId = 0;        ///< owning service id (set by the owner)
    int bitrateKbps = 0;          ///< superframe_len/120*8 (TS 102 563 §4)

    /// Core sampling-frequency index per ISO/IEC 14496-3 (24/48/16/32 kHz).
    int coreSrIndex() const { return dacRate ? (sbr ? 6 : 3) : (sbr ? 8 : 5); }
    /// Extension sampling-frequency index (48/32 kHz).
    int extSrIndex() const { return dacRate ? 3 : 5; }
    /// channelConfiguration for the AAC AudioSpecificConfig (1 = mono, 2 = stereo).
    int coreChConfig() const { return aacChannelMode ? 2 : 1; }

    /// Human-readable codec profile.
    QString profile() const
    {
        if (sbr) {
            return ps ? QStringLiteral("HE-AAC v2") : QStringLiteral("HE-AAC");
        }
        return QStringLiteral("AAC-LC");
    }

    /// Display sample rate of the CORE stream (kHz); the decoder reports the
    /// true output rate, which SBR doubles.
    int coreSampleRateKHz() const { return dacRate ? 48 : 32; }

    bool valid() const { return numAus > 0; }
};

/**
 * @brief DAB+ audio superframe/AU extractor + libfaad2 HE-AAC decoder.
 */
class DabPlusAudioDecoder : public QObject {
    Q_OBJECT

public:
    explicit DabPlusAudioDecoder(QObject* parent = nullptr);
    ~DabPlusAudioDecoder() override;

    /// Clear the superframe assembler and the FAAD decoder (keeps no state).
    void reset();

    /**
     * @brief Feed one CIF chunk (one ETI frame's worth of this sub-channel).
     *
     * Accumulates 5 CIFs into a 120 ms superframe using a sliding sync: a
     * candidate window that does not pass the FireCode drops ONE chunk and
     * retries, so the parser locks onto the real superframe boundary quickly.
     * For every CRC-valid Access Unit `onAu` is invoked with the raw payload
     * (the AU minus its trailing 2 CRC bytes, including any Data Stream
     * Element/PAD). Emits formatChanged() when the announced format changes.
     *
     * @param chunk   MSC bytes for this sub-channel in this ETI frame.
     * @param onAu    Callback receiving each CRC-valid AU (may be empty).
     * @return true if at least one AU was emitted.
     */
    bool feedChunk(const QByteArray& chunk,
                   const std::function<void(const QByteArray&)>& onAu);

    /// Number of superframes whose FireCode validated (extraction evidence).
    quint64 superframesAccepted() const { return m_superframesAccepted; }
    /// Number of Access Units whose AU CRC validated.
    quint64 accessUnitsAccepted() const { return m_accessUnitsAccepted; }

    /// The most recently announced superframe format.
    DabPlusFormat format() const { return m_format; }

    /// Initialize libfaad2 from `fmt` (builds the DAB+ AudioSpecificConfig).
    /// @return false when libfaad2 is unavailable or the config is rejected.
    bool beginDecode(const DabPlusFormat& fmt);

    /// Decode one Access Unit (in transmission order). Appends interleaved
    /// signed 16-bit little-endian PCM to `pcmOut`.
    /// @return true when the decoder produced at least one sample.
    bool decodeAu(const QByteArray& au, QByteArray& pcmOut);

    /// Release the libfaad2 decoder.
    void endDecode();

    bool isDecoderReady() const { return m_decoderReady; }
    int outputSampleRate() const { return static_cast<int>(m_outputSampleRate); }
    int outputChannels() const { return static_cast<int>(m_outputChannels); }
    QString lastError() const { return m_lastError; }

    /// True when this build has libfaad2 compiled in.
    static bool faadAvailable();

signals:
    /// Emitted (same thread) when the announced superframe format changes.
    void formatChanged();

private:
    void setFormatFrom(const streamdab::dabplus::SuperframeResult& result);
    void setError(const QString& error);

    // Shared 5-CIF superframe accumulation + FireCode/AU/PAD transport (T38).
    streamdab::dabplus::SuperframeAssembler m_superframe;

    DabPlusFormat m_format;
    uint8_t m_lastFormatByte = 0;
    bool m_haveFormat = false;

    quint64 m_superframesAccepted = 0;
    quint64 m_accessUnitsAccepted = 0;

    // FAAD decoder state.
#ifdef HAVE_FAAD2
    NeAACDecHandle m_faadHandle = nullptr;
    NeAACDecFrameInfo m_frameInfo{};
#endif
    bool m_decoderReady = false;
    unsigned long m_outputSampleRate = 0;
    unsigned char m_outputChannels = 0;
    QString m_lastError;
};

} // namespace streamdab::audio

Q_DECLARE_METATYPE(streamdab::audio::DabPlusFormat)
