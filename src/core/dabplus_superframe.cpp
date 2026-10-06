/**
 * @file dabplus_superframe.cpp
 * @brief Shared DAB+ transport layer (T38): superframe / AU / PAD extraction.
 * @author C++ Qt Developer Agent (T38)
 */

#include "dabplus_superframe.hpp"
#include "crc16.hpp"

#include <cstdint>

namespace streamdab::dabplus {

namespace {

constexpr int kBytesPerCif = 120;  // bytes per 24 ms CIF in the 120 ms superframe

/**
 * @brief Detect the DSE-carried PAD region at the start of an AU payload.
 *
 * A PAD-bearing AU starts with a Data Stream Element (element id 4, top 3 bits
 * of the first byte). The second byte is the PAD length; the special value 255
 * adds the third byte. `padOffset`/`padLen` are set only when the announced
 * PAD fits the payload and is at least one F-PAD long.
 *
 * @param payload     AU payload (AU minus its 2 trailing CRC bytes)
 * @param payloadLen  payload length in bytes
 * @param padOffset   [out] PAD offset within the payload, or -1
 * @param padLen      [out] announced PAD length, or 0
 */
void detectPad(const uint8_t* payload, int payloadLen, int& padOffset, int& padLen)
{
    padOffset = -1;
    padLen = 0;
    if (payloadLen < 3 || (payload[0] >> 5) != 4) {
        return;  // no DSE (PAD) present
    }
    int start = 2;
    int len = payload[1];
    if (len == 255) {
        len += payload[2];
        ++start;
    }
    if (len < kFpadLen || payloadLen < start + len) {
        return;  // malformed or truncated PAD
    }
    padOffset = start;
    padLen = len;
}

} // namespace

uint16_t firecodeCrc(const uint8_t* data, std::size_t len)
{
    uint16_t crc = 0x0000;
    constexpr uint16_t genPoly = 0x782F;
    for (std::size_t i = 0; i < len; ++i) {
        for (int bit = 0x80; bit != 0; bit >>= 1) {
            if (crc & 0x8000) {
                crc = static_cast<uint16_t>((crc << 1) ^ genPoly);
            } else {
                crc = static_cast<uint16_t>(crc << 1);
            }
            if (data[i] & bit) {
                crc ^= genPoly;
            }
        }
    }
    return crc;
}

void SuperframeAssembler::reset()
{
    m_buffer.clear();
    m_chunkLen = 0;
}

SuperframeResult SuperframeAssembler::processChunk(const QByteArray& chunk)
{
    SuperframeResult result;
    if (chunk.isEmpty()) {
        return result;
    }
    if (m_chunkLen == 0) {
        m_chunkLen = chunk.size();
    }
    if (chunk.size() != m_chunkLen) {
        m_buffer.clear();
        m_chunkLen = chunk.size();
    }
    m_buffer.append(chunk);
    const int sfLen = 5 * m_chunkLen;
    if (m_buffer.size() < sfLen) {
        return result;
    }
    result = processSuperframe(m_buffer.left(sfLen));
    m_buffer.remove(0, result.sync == SuperframeSync::Locked ? sfLen : m_chunkLen);
    return result;
}

SuperframeResult SuperframeAssembler::processSuperframe(const QByteArray& sf)
{
    SuperframeResult result;
    if (sf.size() < kBytesPerCif || (sf.size() % kBytesPerCif) != 0) {
        return result;
    }
    result.examined = true;
    result.subChannelIndex = sf.size() / kBytesPerCif;
    result.data = sf;

    const uint8_t* b = reinterpret_cast<const uint8_t*>(sf.constData());
    const uint8_t audioParams = b[2];
    result.audioParams = audioParams;

    SuperframeFormat fmt;
    fmt.dacRate = (audioParams & 0x40) != 0;
    fmt.sbr = (audioParams & 0x20) != 0;
    fmt.aacChannelMode = (audioParams & 0x10) != 0;
    fmt.ps = (audioParams & 0x08) != 0;
    fmt.bitrateKbps = result.subChannelIndex * 8;

    // ---- Superframe header / FireCode (ETSI TS 102 563 §4) ----
    const uint16_t fireStored = static_cast<uint16_t>((b[0] << 8) | b[1]);
    if (fireStored != firecodeCrc(b + 2, 9)) {
        return result;  // not synced
    }
    // Plausibility check (etisnoop): the bytes after the FireCode must not be
    // zero padding, otherwise a run of zeroes could fake a boundary.
    if (b[3] == 0x00 && (b[4] & 0xF0) == 0x00) {
        return result;
    }
    result.sync = SuperframeSync::Locked;

    // ---- AU count / first-AU offset from the format bits (TS 102 563 §4) ----
    int numAus = 0;
    int firstAuStart = 0;
    if (!fmt.dacRate && fmt.sbr) {
        numAus = 2;
        firstAuStart = 5;
    } else if (fmt.dacRate && fmt.sbr) {
        numAus = 3;
        firstAuStart = 6;
    } else if (!fmt.dacRate && !fmt.sbr) {
        numAus = 4;
        firstAuStart = 8;
    } else {
        numAus = 6;
        firstAuStart = 11;
    }
    fmt.numAus = numAus;
    result.format = fmt;

    // AU start offsets: n-1 values of 3 nibbles each (etisnoop).
    std::vector<int> auStart(static_cast<std::size_t>(numAus) + 1, 0);
    auStart[0] = firstAuStart;
    auStart[numAus] = result.subChannelIndex * 110;  // end of AU data (before RS parity)

    std::vector<uint8_t> nibbles;
    nibbles.reserve(static_cast<std::size_t>((numAus - 1) * 3));
    for (int i = 0; i < (numAus - 1) * 3; ++i) {
        const int byteIndex = 3 + i / 2;
        if (byteIndex >= sf.size()) {
            return result;
        }
        nibbles.push_back((i % 2 == 0) ? static_cast<uint8_t>(b[byteIndex] >> 4)
                                       : static_cast<uint8_t>(b[byteIndex] & 0x0F));
    }
    int nib = 0;
    for (int au = 1; au < numAus; ++au) {
        auStart[au] = (nibbles[nib] << 8) | (nibbles[nib + 1] << 4) | nibbles[nib + 2];
        nib += 3;
    }
    for (int au = 0; au < numAus; ++au) {
        if (auStart[au] >= auStart[au + 1]) {
            return result;
        }
    }
    result.offsetsValid = true;

    // ---- Per-AU: AU CRC + DSE/PAD location (TS 102 563 §5 / §7) ----
    result.aus.reserve(static_cast<std::size_t>(numAus));
    for (int au = 0; au < numAus; ++au) {
        const int auLen = auStart[au + 1] - auStart[au];
        if (auLen < 2) {
            continue;
        }
        const uint8_t* auData = b + auStart[au];
        const int payloadLen = auLen - 2;

        AccessUnit unit;
        unit.offset = auStart[au];
        unit.size = auLen;
        const uint16_t auCrcStored =
            static_cast<uint16_t>((auData[payloadLen] << 8) | auData[payloadLen + 1]);
        const uint16_t auCrcCalc = static_cast<uint16_t>(
            ~eti::crc16ccitt_false(auData, static_cast<std::size_t>(payloadLen)));
        unit.crcOk = (auCrcStored == auCrcCalc);
        detectPad(auData, payloadLen, unit.padOffset, unit.padLen);
        result.aus.push_back(unit);
    }
    return result;
}

} // namespace streamdab::dabplus
