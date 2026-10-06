/**
 * @file dabplus_audio_decoder.cpp
 * @brief T34: DAB+ superframe/AU extraction + libfaad2 PCM decode.
 * @author C++ Qt Developer Agent (T34)
 */

#include "dabplus_audio_decoder.hpp"
#include "crc16.hpp"

#include <QDebug>

#include <algorithm>
#include <cstring>

#ifdef HAVE_FAAD2
extern "C" {
#include <neaacdec.h>
}
#endif

namespace streamdab::audio {

namespace {

// FireCode CRC-16 (poly 0x782F, init 0x0000), bit order per ETSI TS 102 563
// §6.2 / ODR-Dab etisnoop (firecode.c). Identical to the DLS+ parser's helper.
uint16_t firecodeCrc(const uint8_t* buf, size_t size)
{
    uint16_t crc = 0x0000;
    constexpr uint16_t genPoly = 0x782F;
    for (size_t len = 0; len < size; ++len) {
        for (int i = 0x80; i != 0; i >>= 1) {
            if (crc & 0x8000) {
                crc = static_cast<uint16_t>((crc << 1) ^ genPoly);
            } else {
                crc = static_cast<uint16_t>(crc << 1);
            }
            if (buf[len] & i) {
                crc ^= genPoly;
            }
        }
    }
    return crc;
}

} // namespace

DabPlusAudioDecoder::DabPlusAudioDecoder(QObject* parent)
    : QObject(parent)
{
}

DabPlusAudioDecoder::~DabPlusAudioDecoder()
{
    endDecode();
}

bool DabPlusAudioDecoder::faadAvailable()
{
#ifdef HAVE_FAAD2
    return true;
#else
    return false;
#endif
}

void DabPlusAudioDecoder::setError(const QString& error)
{
    m_lastError = error;
}

void DabPlusAudioDecoder::reset()
{
    m_superframeBuffer.clear();
    m_chunkLen = 0;
    m_haveFormat = false;
    m_lastFormatByte = 0;
    m_format = DabPlusFormat{};
    m_superframesAccepted = 0;
    m_accessUnitsAccepted = 0;
    endDecode();
}

void DabPlusAudioDecoder::setFormatFromByte(uint8_t audioParams, int bitrateKbps)
{
    DabPlusFormat fmt;
    fmt.dacRate = (audioParams & 0x40) != 0;
    fmt.sbr = (audioParams & 0x20) != 0;
    fmt.aacChannelMode = (audioParams & 0x10) != 0;
    fmt.ps = (audioParams & 0x08) != 0;
    fmt.bitrateKbps = bitrateKbps;
    // numAus follows the format bits (DABlin SuperframeFilter::CheckSync):
    // dac_rate ? (sbr ? 3 : 6) : (sbr ? 2 : 4)
    fmt.numAus = fmt.dacRate ? (fmt.sbr ? 3 : 6) : (fmt.sbr ? 2 : 4);
    fmt.subChannelId = m_format.subChannelId;
    fmt.serviceId = m_format.serviceId;

    const bool changed = !m_haveFormat || m_lastFormatByte != audioParams;
    m_format = fmt;
    if (changed) {
        m_haveFormat = true;
        m_lastFormatByte = audioParams;
        emit formatChanged();
    }
}

bool DabPlusAudioDecoder::feedChunk(const QByteArray& chunk,
                                    const std::function<void(const QByteArray&)>& onAu)
{
    if (chunk.isEmpty()) {
        return false;
    }
    if (m_chunkLen == 0) {
        m_chunkLen = chunk.size();
    }
    if (chunk.size() != m_chunkLen) {
        m_superframeBuffer.clear();
        m_chunkLen = chunk.size();
    }
    m_superframeBuffer.append(chunk);
    const int sfLen = 5 * m_chunkLen;
    if (m_superframeBuffer.size() < sfLen) {
        return false;
    }
    const QByteArray sf = m_superframeBuffer.left(sfLen);
    bool synced = false;
    const bool produced = processSuperframe(sf, onAu, &synced);
    m_superframeBuffer.remove(0, synced ? sfLen : m_chunkLen);
    return produced;
}

bool DabPlusAudioDecoder::processSuperframe(
    const QByteArray& sf,
    const std::function<void(const QByteArray&)>& onAu,
    bool* syncedOut)
{
    if (syncedOut) {
        *syncedOut = false;
    }
    if (sf.size() < 120 || (sf.size() % 120) != 0) {
        return false;
    }
    const int subchIndex = sf.size() / 120;
    const uint8_t* b = reinterpret_cast<const uint8_t*>(sf.constData());

    // ---- Superframe header / FireCode (ETSI TS 102 563 §4) ----
    const uint16_t fireStored = static_cast<uint16_t>((b[0] << 8) | b[1]);
    if (fireStored != firecodeCrc(b + 2, 9)) {
        return false;  // not synced
    }
    // Plausibility check (etisnoop): the three bytes after the FireCode must
    // not all be zero (avoids sync in zero padding).
    if (b[3] == 0x00 && (b[4] & 0xF0) == 0x00) {
        return false;
    }
    if (syncedOut) {
        *syncedOut = true;
    }
    ++m_superframesAccepted;

    const uint8_t audioParams = b[2];
    setFormatFromByte(audioParams, sf.size() / 120 * 8);

    const bool dacRate = (audioParams & 0x40) != 0;
    const bool sbrFlag = (audioParams & 0x20) != 0;
    int numAus = 0;
    int firstAuStart = 0;
    if (!dacRate && sbrFlag) {
        numAus = 2;
        firstAuStart = 5;
    } else if (dacRate && sbrFlag) {
        numAus = 3;
        firstAuStart = 6;
    } else if (!dacRate && !sbrFlag) {
        numAus = 4;
        firstAuStart = 8;
    } else {
        numAus = 6;
        firstAuStart = 11;
    }

    // AU start offsets: n-1 values of 3 nibbles each (etisnoop).
    std::vector<int> auStart(static_cast<size_t>(numAus) + 1, 0);
    auStart[0] = firstAuStart;
    auStart[numAus] = subchIndex * 110;  // end of AU data (before RS parity)

    std::vector<uint8_t> nibbles;
    nibbles.reserve(static_cast<size_t>((numAus - 1) * 3));
    for (int i = 0; i < (numAus - 1) * 3; ++i) {
        const int byteIndex = 3 + i / 2;
        if (byteIndex >= sf.size()) {
            return false;
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
            return false;
        }
    }

    // ---- Per-AU: AU CRC (ETSI TS 102 563 §5) ----
    bool produced = false;
    for (int au = 0; au < numAus; ++au) {
        const int auLen = auStart[au + 1] - auStart[au];
        if (auLen < 2) {
            continue;
        }
        const uint8_t* auData = b + auStart[au];
        const int payloadLen = auLen - 2;
        const uint16_t auCrcStored =
            static_cast<uint16_t>((auData[payloadLen] << 8) | auData[payloadLen + 1]);
        const uint16_t auCrcCalc =
            static_cast<uint16_t>(~eti::crc16ccitt_false(auData, static_cast<size_t>(payloadLen)));
        if (auCrcStored != auCrcCalc) {
            continue;  // corrupted AU -> skip (no RS correction here)
        }
        ++m_accessUnitsAccepted;
        if (onAu) {
            onAu(QByteArray(reinterpret_cast<const char*>(auData), payloadLen));
        }
        produced = true;
    }
    return produced;
}

bool DabPlusAudioDecoder::beginDecode(const DabPlusFormat& fmt)
{
    endDecode();
    m_lastError.clear();

#ifndef HAVE_FAAD2
    Q_UNUSED(fmt);
    setError(QStringLiteral("libfaad2 not available in this build"));
    return false;
#else
    if (!fmt.valid()) {
        setError(QStringLiteral("no DAB+ superframe format yet"));
        return false;
    }

    // Build the AudioSpecificConfig (DABlin/AACDecoder, TS 102 563 §4.1):
    //   AOT 2 (AAC-LC), core SR index, core channel config, GASpecificConfig
    //   with 960 transform (0b100); optional explicit SBR (AOT 5) + PS.
    uint8_t asc[7];
    size_t ascLen = 0;
    asc[ascLen++] = static_cast<uint8_t>((0b00010 << 3) | (fmt.coreSrIndex() >> 1));
    asc[ascLen++] = static_cast<uint8_t>(((fmt.coreSrIndex() & 0x01) << 7)
                                         | (fmt.coreChConfig() << 3) | 0b100);
    if (fmt.sbr) {
        asc[ascLen++] = 0x56;
        asc[ascLen++] = 0xE5;
        asc[ascLen++] = static_cast<uint8_t>(0x80 | (fmt.extSrIndex() << 3));
        if (fmt.ps) {
            asc[ascLen - 1] = static_cast<uint8_t>(asc[ascLen - 1] | 0x05);
            asc[ascLen++] = 0x48;
            asc[ascLen++] = 0x80;
        }
    }

    m_faadHandle = NeAACDecOpen();
    if (!m_faadHandle) {
        setError(QStringLiteral("NeAACDecOpen failed"));
        return false;
    }
    NeAACDecConfigurationPtr config = NeAACDecGetCurrentConfiguration(m_faadHandle);
    if (!config) {
        setError(QStringLiteral("NeAACDecGetCurrentConfiguration failed"));
        endDecode();
        return false;
    }
    config->outputFormat = FAAD_FMT_16BIT;
    config->dontUpSampleImplicitSBR = 0;
    if (NeAACDecSetConfiguration(m_faadHandle, config) != 1) {
        setError(QStringLiteral("NeAACDecSetConfiguration failed"));
        endDecode();
        return false;
    }

    unsigned long sr = 0;
    unsigned char ch = 0;
    const long initResult = NeAACDecInit2(m_faadHandle, asc,
                                         static_cast<unsigned long>(ascLen), &sr, &ch);
    if (initResult != 0) {
        setError(QString("NeAACDecInit2 failed: %1")
                     .arg(QString::fromLatin1(NeAACDecGetErrorMessage(
                         static_cast<int>(-initResult)))));
        endDecode();
        return false;
    }

    m_outputSampleRate = sr;
    m_outputChannels = ch;
    m_decoderReady = true;
    m_format = fmt;
    return true;
#endif
}

bool DabPlusAudioDecoder::decodeAu(const QByteArray& au, QByteArray& pcmOut)
{
    if (!m_decoderReady) {
        setError(QStringLiteral("decoder not initialized"));
        return false;
    }
#ifdef HAVE_FAAD2
    if (au.isEmpty()) {
        return false;
    }
    void* output = NeAACDecDecode(
        m_faadHandle, &m_frameInfo,
        reinterpret_cast<unsigned char*>(const_cast<char*>(au.constData())),
        static_cast<unsigned long>(au.size()));
    if (m_frameInfo.bytesconsumed != static_cast<unsigned long>(au.size())) {
        // The AU should be consumed exactly; a mismatch means the frame was
        // truncated/misaligned. Error concealment may still produce output, so
        // warn and keep it rather than dropping audio, but do not report it as
        // a clean success below.
        qWarning() << "[T34] FAAD consumed" << m_frameInfo.bytesconsumed
                   << "of" << au.size() << "AU bytes";
    }
    if (m_frameInfo.error != 0) {
        setError(QString("FAAD decode error: %1")
                     .arg(QString::fromLatin1(
                         NeAACDecGetErrorMessage(static_cast<int>(m_frameInfo.error)))));
        // Error concealment may still yield a frame; fall through and use it.
    }
    if (output && m_frameInfo.samples > 0) {
        pcmOut.append(static_cast<const char*>(output),
                      static_cast<int>(m_frameInfo.samples) * 2);
        m_lastError.clear();  // a successful frame clears any stale error
        return true;
    }
    return false;
#else
    Q_UNUSED(au);
    Q_UNUSED(pcmOut);
    return false;
#endif
}

void DabPlusAudioDecoder::endDecode()
{
#ifdef HAVE_FAAD2
    if (m_faadHandle) {
        NeAACDecClose(m_faadHandle);
        m_faadHandle = nullptr;
    }
#endif
    m_decoderReady = false;
    m_outputSampleRate = 0;
    m_outputChannels = 0;
}

} // namespace streamdab::audio
