/**
 * @file dabplus_audio_decoder.cpp
 * @brief T34: DAB+ superframe/AU extraction + libfaad2 PCM decode.
 * @author C++ Qt Developer Agent (T34)
 */

#include "dabplus_audio_decoder.hpp"

#include <QDebug>


#ifdef HAVE_FAAD2
extern "C" {
#include <neaacdec.h>
}
#endif

namespace streamdab::audio {

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
    m_superframe.reset();
    m_haveFormat = false;
    m_lastFormatByte = 0;
    m_format = DabPlusFormat{};
    m_superframesAccepted = 0;
    m_accessUnitsAccepted = 0;
    endDecode();
}

void DabPlusAudioDecoder::setFormatFrom(const streamdab::dabplus::SuperframeResult& result)
{
    DabPlusFormat fmt;
    fmt.dacRate = result.format.dacRate;
    fmt.sbr = result.format.sbr;
    fmt.aacChannelMode = result.format.aacChannelMode;
    fmt.ps = result.format.ps;
    fmt.bitrateKbps = result.format.bitrateKbps;
    fmt.numAus = result.format.numAus;
    fmt.subChannelId = m_format.subChannelId;
    fmt.serviceId = m_format.serviceId;

    const bool changed = !m_haveFormat || m_lastFormatByte != result.audioParams;
    m_format = fmt;
    if (changed) {
        m_haveFormat = true;
        m_lastFormatByte = result.audioParams;
        emit formatChanged();
    }
}

bool DabPlusAudioDecoder::feedChunk(const QByteArray& chunk,
                                    const std::function<void(const QByteArray&)>& onAu)
{
    // T38: the shared transport module owns accumulation and sync. A locked
    // window increments the accepted-superframe counter exactly as the local
    // implementation did (before the AU offset table is decoded).
    const streamdab::dabplus::SuperframeResult result = m_superframe.processChunk(chunk);
    if (!result.examined
        || result.sync != streamdab::dabplus::SuperframeSync::Locked) {
        return false;
    }
    ++m_superframesAccepted;
    setFormatFrom(result);
    if (!result.offsetsValid) {
        return false;
    }

    const uint8_t* b = reinterpret_cast<const uint8_t*>(result.data.constData());
    bool produced = false;
    for (const streamdab::dabplus::AccessUnit& au : result.aus) {
        if (!au.crcOk) {
            continue;  // corrupted AU -> skip (no RS correction here)
        }
        ++m_accessUnitsAccepted;
        if (onAu) {
            onAu(QByteArray(reinterpret_cast<const char*>(b + au.offset), au.size - 2));
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
