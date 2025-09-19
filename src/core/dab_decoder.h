#pragma once

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QHash>
#include <memory>
#include "eti_types.h"

// Forward declaration for ETI types

/**
 * @class DabDecoder
 * @brief Professional DAB/DAB+ decoder following ETSI EN 300 401 standard
 * 
 * Decodes Digital Audio Broadcasting signals from ETI frames according to
 * ETSI specifications. Supports both DAB (MPEG-1 Layer 2) and DAB+ (HE-AAC)
 * audio encoding formats.
 * 
 * Features:
 * - ETSI EN 300 401 compliant DAB decoding
 * - DAB+ (HE-AAC) support with error correction
 * - Service component extraction and routing
 * - Real-time decoding with <50ms latency
 * - Professional broadcast quality validation
 */
class DabDecoder : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Audio codec types supported by DAB decoder
     */
    enum class AudioCodec {
        Unknown = 0,
        DAB_MP2 = 1,    // MPEG-1 Layer 2 (traditional DAB)
        DAB_PLUS = 2    // HE-AAC (DAB+)
    };
    Q_ENUM(AudioCodec)

    /**
     * @brief Decoder processing status
     */
    enum class DecoderStatus {
        Idle = 0,
        Processing = 1,
        Error = 2,
        Synchronized = 3
    };
    Q_ENUM(DecoderStatus)

    /**
     * @brief Service component information
     */
    struct ServiceComponent {
        quint16 serviceId;
        quint8 subChannelId;
        AudioCodec codec;
        quint16 bitrate;
        QString label;
        bool isProtected;
        
        ServiceComponent() : serviceId(0), subChannelId(0), codec(AudioCodec::Unknown), 
                           bitrate(0), isProtected(false) {}
    };

    explicit DabDecoder(QObject *parent = nullptr);
    virtual ~DabDecoder();

    // Core decoding operations
    bool initialize();
    void shutdown();
    bool isInitialized() const { return m_initialized; }

    // Frame processing
    bool processEtiFrame(const eti::EtiFrameData& frame);
    bool processSubChannel(quint8 subChannelId, const QByteArray& data);

    // Service management
    QList<ServiceComponent> getAvailableServices() const;
    ServiceComponent getServiceInfo(quint16 serviceId) const;
    bool selectService(quint16 serviceId);
    quint16 getCurrentServiceId() const { return m_currentServiceId; }

    // Audio output
    QByteArray getDecodedAudio();
    bool hasDecodedAudio() const;
    void clearAudioBuffer();

    // Status and statistics
    DecoderStatus getStatus() const { return m_status; }
    quint32 getProcessedFrameCount() const { return m_frameCount; }
    double getErrorRate() const { return m_errorRate; }
    quint32 getBitrate() const;

    // Configuration
    void setMaxBufferSize(quint32 maxSize) { m_maxBufferSize = maxSize; }
    quint32 getMaxBufferSize() const { return m_maxBufferSize; }

signals:
    /**
     * @brief Emitted when new services are discovered
     */
    void servicesDiscovered(const QList<ServiceComponent>& services);

    /**
     * @brief Emitted when audio data is decoded and ready
     */
    void audioDataReady(const QByteArray& audioData, quint32 sampleRate);

    /**
     * @brief Emitted when decoder status changes
     */
    void statusChanged(DecoderStatus status);

    /**
     * @brief Emitted on decoding errors
     */
    void decodingError(const QString& error);

    /**
     * @brief Emitted when service selection changes
     */
    void serviceSelected(quint16 serviceId, const ServiceComponent& info);

private slots:
    void handleFrameProcessed();
    void handleDecodingError(const QString& error);

private:
    // Internal processing methods
    bool parseServiceInformation(const eti::EtiFrameData& frame);
    bool decodeAudioData(const QByteArray& data, AudioCodec codec);
    bool synchronizeToStream(const eti::EtiFrameData& frame);
    void updateErrorStatistics(bool hasError);
    void resetDecoder();

    // DAB/DAB+ specific decoding
    bool decodeDabMp2(const QByteArray& data);
    bool decodeDabPlus(const QByteArray& data);
    
    // ETI frame parsing helpers
    QByteArray extractSubchannelData(const eti::EtiFrameData& frame, quint8 subChannelId);

    // Service component management
    void addServiceComponent(const ServiceComponent& component);
    void removeServiceComponent(quint16 serviceId);
    void updateServiceComponent(const ServiceComponent& component);

    // Member variables
    bool m_initialized;
    DecoderStatus m_status;
    quint16 m_currentServiceId;
    quint32 m_frameCount;
    double m_errorRate;
    quint32 m_errorCount;
    quint32 m_maxBufferSize;

    // Service management
    QHash<quint16, ServiceComponent> m_services;
    QList<ServiceComponent> m_discoveredServices;

    // Audio processing
    QByteArray m_audioBuffer;
    quint32 m_currentSampleRate;
    QByteArray m_tempBuffer;

    // Synchronization and buffering
    bool m_synchronized;
    quint8 m_syncPattern[4];
    qint64 m_lastFrameTime;

    // Statistics
    struct Statistics {
        quint32 totalFrames;
        quint32 errorFrames;
        quint32 syncLoss;
        qint64 processingTime;
        double averageLatency;
    } m_stats;

    // Constants
    static constexpr quint32 DEFAULT_BUFFER_SIZE = 8192;
    static constexpr quint32 MAX_AUDIO_BUFFER_SIZE = 65536;
    static constexpr double MAX_ERROR_RATE = 0.1; // 10% maximum error rate
    static constexpr quint32 SYNC_PATTERN = 0xFF1F; // DAB sync pattern
};