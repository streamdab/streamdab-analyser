#ifndef DAB_AUDIO_DECODER_H
#define DAB_AUDIO_DECODER_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QMutex>

#ifdef USE_FDK_AAC
#include <fdk-aac/aacdecoder_lib.h>
#elif defined(USE_FAAD2)
#include <neaacdec.h>
#endif

/**
 * @brief Professional DAB+ audio decoder abstraction layer
 * 
 * Provides unified interface for DAB+ HE-AAC v2 decoding using either
 * fdk-aac (preferred) or FAAD2 backends. Handles professional broadcast
 * requirements including real-time processing and quality monitoring.
 */
class DabAudioDecoder : public QObject {
    Q_OBJECT
    
public:
    enum class DecoderType {
        FDK_AAC,
        FAAD2,
        UNKNOWN
    };
    
    enum class AudioFormat {
        PCM_16_LE,      ///< 16-bit little-endian PCM
        PCM_24_LE,      ///< 24-bit little-endian PCM  
        PCM_32_FLOAT    ///< 32-bit float PCM
    };
    
    struct AudioInfo {
        int sampleRate = 48000;     ///< Sample rate (typically 48kHz for DAB+)
        int channels = 2;           ///< Number of channels (1=mono, 2=stereo)
        int bitsPerSample = 16;     ///< Bits per sample
        AudioFormat format = AudioFormat::PCM_16_LE;
        QString codecInfo;          ///< Codec information string
        bool isValid = false;       ///< Whether audio info is valid
    };
    
    explicit DabAudioDecoder(QObject *parent = nullptr);
    ~DabAudioDecoder();
    
    // Core functionality
    bool initialize(int sampleRate = 48000, int channels = 2);
    QByteArray decode(const QByteArray& aacData);
    void cleanup();
    bool isInitialized() const { return m_initialized; }
    
    // DAB+ specific methods
    void configureDabPlus();
    bool setConfiguration(int sampleRate, int channels, AudioFormat format = AudioFormat::PCM_16_LE);
    
    // Audio monitoring
    int getAudioLevel() const { return m_currentLevel; }
    AudioInfo getAudioInfo() const { return m_audioInfo; }
    QString getCodecInfo() const;
    DecoderType getDecoderType() const { return m_decoderType; }
    
    // Quality monitoring
    bool hasErrors() const { return m_errorCount > 0; }
    int getErrorCount() const { return m_errorCount; }
    void clearErrors() { m_errorCount = 0; }
    
    // Performance monitoring
    quint64 getFramesDecoded() const { return m_framesDecoded; }
    quint64 getBytesDecoded() const { return m_bytesDecoded; }
    double getDecodingRate() const; ///< Frames per second
    
signals:
    void audioLevelChanged(int level);
    void decodingError(const QString& error);
    void audioInfoChanged(const AudioInfo& info);
    void decodingRateChanged(double rate);
    
private slots:
    void updatePerformanceMetrics();
    
private:
    void calculateAudioLevel(const QByteArray& pcmData);
    bool initializeFdkAac();
    bool initializeFaad2();
    QByteArray decodeFdkAac(const QByteArray& aacData);
    QByteArray decodeFaad2(const QByteArray& aacData);
    void updateAudioInfo();
    void logError(const QString& error);
    
#ifdef USE_FDK_AAC
    HANDLE_AACDECODER m_fdkDecoder;
    CStreamInfo* m_fdkStreamInfo;
#elif defined(USE_FAAD2)
    NeAACDecHandle m_faadDecoder;
    NeAACDecConfigurationPtr m_faadConfig;
    NeAACDecFrameInfo m_faadFrameInfo;
#endif
    
    // State management
    bool m_initialized;
    DecoderType m_decoderType;
    AudioInfo m_audioInfo;
    mutable QMutex m_mutex;
    
    // Audio level monitoring
    int m_currentLevel;
    QByteArray m_levelBuffer;
    static const int LEVEL_BUFFER_SIZE = 1024;
    
    // Error tracking
    int m_errorCount;
    QString m_lastError;
    
    // Performance tracking
    quint64 m_framesDecoded;
    quint64 m_bytesDecoded;
    qint64 m_startTime;
    qint64 m_lastUpdateTime;
    double m_currentRate;
    
    // Configuration
    static const int DEFAULT_SAMPLE_RATE = 48000;
    static const int DEFAULT_CHANNELS = 2;
    static const int MAX_ERROR_COUNT = 10;
};

#endif // DAB_AUDIO_DECODER_H