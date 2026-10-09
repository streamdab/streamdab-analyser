#include "dab_audio_decoder.hpp"
#include "../utils/logger.h"
#include <QTimer>
#include <QDateTime>
#include <QtMath>
#include <QDebug>

DabAudioDecoder::DabAudioDecoder(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_decoderType(DecoderType::UNKNOWN)
    , m_currentLevel(0)
    , m_errorCount(0)
    , m_framesDecoded(0)
    , m_bytesDecoded(0)
    , m_startTime(0)
    , m_lastUpdateTime(0)
    , m_currentRate(0.0)
{
#ifdef USE_FDK_AAC
    m_decoderType = DecoderType::FDK_AAC;
    m_fdkDecoder = nullptr;
    m_fdkStreamInfo = nullptr;
#elif defined(USE_FAAD2)
    m_decoderType = DecoderType::FAAD2;
    m_faadDecoder = nullptr;
    m_faadConfig = nullptr;
#endif
    
    // Setup performance monitoring timer
    QTimer *perfTimer = new QTimer(this);
    connect(perfTimer, &QTimer::timeout, this, &DabAudioDecoder::updatePerformanceMetrics);
    perfTimer->start(1000); // Update every second
    
    Logger::instance().logInfo(QString("Initialized with %1 backend")
                 .arg(m_decoderType == DecoderType::FDK_AAC ? "fdk-aac" : "FAAD2"), "DabAudioDecoder");
}

DabAudioDecoder::~DabAudioDecoder()
{
    cleanup();
}

bool DabAudioDecoder::initialize(int sampleRate, int channels)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_initialized) {
        cleanup();
    }
    
    m_audioInfo.sampleRate = sampleRate;
    m_audioInfo.channels = channels;
    m_audioInfo.bitsPerSample = 16; // Default to 16-bit
    m_audioInfo.format = AudioFormat::PCM_16_LE;
    
    bool success = false;
    
#ifdef USE_FDK_AAC
    success = initializeFdkAac();
#elif defined(USE_FAAD2)
    success = initializeFaad2();
#else
    logError("No audio decoder backend compiled");
    return false;
#endif
    
    if (success) {
        m_initialized = true;
        m_startTime = QDateTime::currentMSecsSinceEpoch();
        m_lastUpdateTime = m_startTime;
        configureDabPlus();
        updateAudioInfo();
        
        Logger::instance().logInfo(QString("Successfully initialized %1 decoder for %2Hz %3ch")
                     .arg(getCodecInfo())
                     .arg(sampleRate)
                     .arg(channels), "DabAudioDecoder");
        
        emit audioInfoChanged(m_audioInfo);
    } else {
        logError("Failed to initialize audio decoder");
    }
    
    return success;
}

#ifdef USE_FDK_AAC
bool DabAudioDecoder::initializeFdkAac()
{
    // Initialize FDK-AAC decoder
    m_fdkDecoder = aacDecoder_Open(TT_MP4_ADTS, 1);
    if (!m_fdkDecoder) {
        logError("Failed to open FDK-AAC decoder");
        return false;
    }
    
    // Configure for DAB+ (HE-AAC v2)
    // Note: Some FDK-AAC parameter constants may vary by version
    // Using basic configuration for maximum compatibility
    
    // The decoder is ready for use without specific parameter setting
    // in most FDK-AAC versions for basic HE-AAC v2 decoding
    
    return true;
}

QByteArray DabAudioDecoder::decodeFdkAac(const QByteArray& aacData)
{
    if (!m_fdkDecoder || aacData.isEmpty()) {
        return QByteArray();
    }
    
    UCHAR* inputBuffer[1];
    UINT inputBufferSize[1];
    UINT inputBytesValid[1];
    
    inputBuffer[0] = (UCHAR*)aacData.constData();
    inputBufferSize[0] = aacData.size();
    inputBytesValid[0] = aacData.size();
    
    // Fill decoder with input data
    AAC_DECODER_ERROR err = aacDecoder_Fill(m_fdkDecoder, inputBuffer, inputBufferSize, inputBytesValid);
    if (err != AAC_DEC_OK) {
        logError(QString("FDK-AAC fill error: %1").arg(err));
        return QByteArray();
    }
    
    // Decode frame
    const int outputBufferSize = 8192 * 2 * sizeof(INT_PCM); // Max frame size * channels * sample size
    QByteArray outputBuffer(outputBufferSize, 0);
    
    err = aacDecoder_DecodeFrame(m_fdkDecoder, (INT_PCM*)outputBuffer.data(), outputBufferSize / sizeof(INT_PCM), 0);
    if (err != AAC_DEC_OK) {
        if (err != AAC_DEC_NOT_ENOUGH_BITS) { // NOT_ENOUGH_BITS is common and not an error
            logError(QString("FDK-AAC decode error: %1").arg(err));
        }
        return QByteArray();
    }
    
    // Get stream info to determine actual output size
    m_fdkStreamInfo = aacDecoder_GetStreamInfo(m_fdkDecoder);
    if (!m_fdkStreamInfo) {
        logError("Failed to get FDK-AAC stream info");
        return QByteArray();
    }
    
    int samplesPerFrame = m_fdkStreamInfo->frameSize;
    int channels = m_fdkStreamInfo->numChannels;
    int actualSize = samplesPerFrame * channels * sizeof(INT_PCM);
    
    outputBuffer.resize(actualSize);
    
    // Update audio info if changed
    if (m_audioInfo.sampleRate != m_fdkStreamInfo->sampleRate ||
        m_audioInfo.channels != channels) {
        m_audioInfo.sampleRate = m_fdkStreamInfo->sampleRate;
        m_audioInfo.channels = channels;
        updateAudioInfo();
        emit audioInfoChanged(m_audioInfo);
    }
    
    return outputBuffer;
}
#endif

#ifdef USE_FAAD2
bool DabAudioDecoder::initializeFaad2()
{
    // Initialize FAAD2 decoder
    m_faadDecoder = NeAACDecOpen();
    if (!m_faadDecoder) {
        logError("Failed to open FAAD2 decoder");
        return false;
    }
    
    // Get default configuration
    m_faadConfig = NeAACDecGetCurrentConfiguration(m_faadDecoder);
    if (!m_faadConfig) {
        logError("Failed to get FAAD2 configuration");
        NeAACDecClose(m_faadDecoder);
        m_faadDecoder = nullptr;
        return false;
    }
    
    // Configure for DAB+ (optimized settings)
    m_faadConfig->outputFormat = FAAD_FMT_16BIT;     // 16-bit output
    m_faadConfig->downMatrix = 0;                     // No downmixing
    m_faadConfig->useOldADTSFormat = 0;              // Use new ADTS format
    m_faadConfig->dontUpSampleImplicitSBR = 0;       // Allow upsampling for SBR
    
    // Set configuration
    if (NeAACDecSetConfiguration(m_faadDecoder, m_faadConfig) != 1) {
        logError("Failed to set FAAD2 configuration");
        NeAACDecClose(m_faadDecoder);
        m_faadDecoder = nullptr;
        return false;
    }
    
    return true;
}

QByteArray DabAudioDecoder::decodeFaad2(const QByteArray& aacData)
{
    if (!m_faadDecoder || aacData.isEmpty()) {
        return QByteArray();
    }
    
    // Decode frame
    void* outputBuffer = NeAACDecDecode(m_faadDecoder, &m_faadFrameInfo,
                                       (unsigned char*)aacData.constData(), aacData.size());
    
    if (m_faadFrameInfo.error != 0) {
        QString errorMsg = QString("FAAD2 decode error: %1").arg(NeAACDecGetErrorMessage(m_faadFrameInfo.error));
        if (m_faadFrameInfo.error != 21) { // Ignore "Channel coupling not yet implemented" warning
            logError(errorMsg);
        }
        return QByteArray();
    }
    
    if (!outputBuffer || m_faadFrameInfo.samples == 0) {
        return QByteArray();
    }
    
    // Calculate output size
    int outputSize = m_faadFrameInfo.samples * sizeof(int16_t);
    QByteArray result((const char*)outputBuffer, outputSize);
    
    // Update audio info if changed
    if (m_audioInfo.sampleRate != (int)m_faadFrameInfo.samplerate ||
        m_audioInfo.channels != m_faadFrameInfo.channels) {
        m_audioInfo.sampleRate = m_faadFrameInfo.samplerate;
        m_audioInfo.channels = m_faadFrameInfo.channels;
        updateAudioInfo();
        emit audioInfoChanged(m_audioInfo);
    }
    
    return result;
}
#endif

QByteArray DabAudioDecoder::decode(const QByteArray& aacData)
{
    if (!m_initialized || aacData.isEmpty()) {
        return QByteArray();
    }
    
    QByteArray result;
    
#ifdef USE_FDK_AAC
    result = decodeFdkAac(aacData);
#elif defined(USE_FAAD2)
    result = decodeFaad2(aacData);
#endif
    
    if (!result.isEmpty()) {
        m_framesDecoded++;
        m_bytesDecoded += result.size();
        calculateAudioLevel(result);
    } else {
        m_errorCount++;
        if (m_errorCount > MAX_ERROR_COUNT) {
            logError(QString("Too many decode errors (%1), decoder may be unstable").arg(m_errorCount));
        }
    }
    
    return result;
}

void DabAudioDecoder::configureDabPlus()
{
    // DAB+ specific configuration
    m_audioInfo.codecInfo = QString("DAB+ HE-AAC v2 (%1)").arg(getCodecInfo());
    
    // Set typical DAB+ parameters
    if (m_audioInfo.sampleRate == 0) {
        m_audioInfo.sampleRate = 48000; // DAB+ standard
    }
    if (m_audioInfo.channels == 0) {
        m_audioInfo.channels = 2; // Stereo default
    }
    
    m_audioInfo.isValid = true;
    
    Logger::instance().logInfo(QString("Configured for DAB+ HE-AAC v2: %1Hz %2ch")
                 .arg(m_audioInfo.sampleRate)
                 .arg(m_audioInfo.channels), "DabAudioDecoder");
}

bool DabAudioDecoder::setConfiguration(int sampleRate, int channels, AudioFormat format)
{
    QMutexLocker locker(&m_mutex);
    
    m_audioInfo.sampleRate = sampleRate;
    m_audioInfo.channels = channels;
    m_audioInfo.format = format;
    
    switch (format) {
    case AudioFormat::PCM_16_LE:
        m_audioInfo.bitsPerSample = 16;
        break;
    case AudioFormat::PCM_24_LE:
        m_audioInfo.bitsPerSample = 24;
        break;
    case AudioFormat::PCM_32_FLOAT:
        m_audioInfo.bitsPerSample = 32;
        break;
    }
    
    updateAudioInfo();
    return true;
}

void DabAudioDecoder::calculateAudioLevel(const QByteArray& pcmData)
{
    if (pcmData.isEmpty()) {
        return;
    }
    
    // Calculate RMS level for 16-bit PCM data
    const int16_t* samples = reinterpret_cast<const int16_t*>(pcmData.constData());
    int sampleCount = pcmData.size() / sizeof(int16_t);
    
    if (sampleCount == 0) {
        return;
    }
    
    qint64 sum = 0;
    for (int i = 0; i < sampleCount; i++) {
        sum += samples[i] * samples[i];
    }
    
    double rms = qSqrt(static_cast<double>(sum) / sampleCount);
    int level = qBound(0, static_cast<int>((rms / 32767.0) * 100), 100);
    
    if (level != m_currentLevel) {
        m_currentLevel = level;
        emit audioLevelChanged(level);
    }
}

QString DabAudioDecoder::getCodecInfo() const
{
#ifdef USE_FDK_AAC
    return QString("FDK-AAC %1").arg(AACDECODER_LIB_VL0);
#elif defined(USE_FAAD2)
    return QString("FAAD2 %1").arg(FAAD2_VERSION);
#else
    return "Unknown";
#endif
}

void DabAudioDecoder::updateAudioInfo()
{
    m_audioInfo.codecInfo = QString("DAB+ HE-AAC v2 (%1)").arg(getCodecInfo());
    m_audioInfo.isValid = (m_audioInfo.sampleRate > 0 && m_audioInfo.channels > 0);
}

void DabAudioDecoder::updatePerformanceMetrics()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 elapsed = currentTime - m_lastUpdateTime;
    
    if (elapsed > 0 && m_framesDecoded > 0) {
        // Calculate frames per second over the last interval
        double totalElapsed = (currentTime - m_startTime) / 1000.0;
        m_currentRate = totalElapsed > 0 ? m_framesDecoded / totalElapsed : 0.0;
        emit decodingRateChanged(m_currentRate);
    }
    
    m_lastUpdateTime = currentTime;
}

double DabAudioDecoder::getDecodingRate() const
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    double totalElapsed = (currentTime - m_startTime) / 1000.0;
    return totalElapsed > 0 ? m_framesDecoded / totalElapsed : 0.0;
}

void DabAudioDecoder::cleanup()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_initialized) {
#ifdef USE_FDK_AAC
        if (m_fdkDecoder) {
            aacDecoder_Close(m_fdkDecoder);
            m_fdkDecoder = nullptr;
        }
        m_fdkStreamInfo = nullptr;
#elif defined(USE_FAAD2)
        if (m_faadDecoder) {
            NeAACDecClose(m_faadDecoder);
            m_faadDecoder = nullptr;
        }
        m_faadConfig = nullptr;
#endif
        
        m_initialized = false;
        Logger::instance().logInfo("Audio decoder cleanup completed", "DabAudioDecoder");
    }
    
    // Reset counters
    m_framesDecoded = 0;
    m_bytesDecoded = 0;
    m_errorCount = 0;
    m_currentLevel = 0;
    m_currentRate = 0.0;
}

void DabAudioDecoder::logError(const QString& error)
{
    m_lastError = error;
    Logger::instance().logError(error, "DabAudioDecoder");
    emit decodingError(error);
}