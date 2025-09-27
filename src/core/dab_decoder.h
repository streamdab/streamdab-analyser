#pragma once

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QHash>
#include <memory>
#include "eti_types.hpp"

// Forward declaration for ETI types

/**
 * @class dab_decoder
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
class dab_decoder : public QObject
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

    explicit dab_decoder(QObject *parent = nullptr);
    virtual ~dab_decoder();

    // Core decoding operations
    bool initialize();
    void shutdown();
    bool is_initialized() const { return initialized_; }

    // Frame processing
    bool process_eti_frame(const eti::EtiFrameData& frame);
    bool process_sub_channel(quint8 subChannelId, const QByteArray& data);
    
    // CamelCase versions for existing implementation compatibility
    bool processEtiFrame(const eti::EtiFrameData& frame);
    bool processSubChannel(quint8 subChannelId, const QByteArray& data);

    // Service management
    QList<ServiceComponent> get_available_services() const;
    ServiceComponent get_service_info(quint16 serviceId) const;
    bool select_service(quint16 serviceId);
    
    // CamelCase versions for existing implementation compatibility
    QList<ServiceComponent> getAvailableServices() const;
    ServiceComponent getServiceInfo(quint16 serviceId) const;
    bool selectService(quint16 serviceId);
    quint16 get_current_service_id() const { return current_service_id_; }

    // Audio output
    QByteArray get_decoded_audio();
    bool has_decoded_audio() const;
    void clear_audio_buffer();
    
    // CamelCase versions for existing implementation compatibility
    QByteArray getDecodedAudio();
    bool hasDecodedAudio() const;
    void clearAudioBuffer();

    // Status and statistics
    DecoderStatus get_status() const { return status_; }
    quint32 get_processed_frame_count() const { return frame_count_; }
    double get_error_rate() const { return error_rate_; }
    quint32 get_bitrate() const;
    
    // CamelCase versions for existing implementation compatibility  
    quint32 getBitrate() const;

    // Configuration
    void set_max_buffer_size(quint32 maxSize) { max_buffer_size_ = maxSize; }
    quint32 get_max_buffer_size() const { return max_buffer_size_; }

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
    void handle_frame_processed();
    void handle_decoding_error(const QString& error);

private:
    // Internal processing methods
    bool parse_service_information(const eti::EtiFrameData& frame);
    
    // CamelCase versions for existing implementation compatibility
    bool parseServiceInformation(const eti::EtiFrameData& frame);
    bool decode_audio_data(const QByteArray& data, AudioCodec codec);
    bool synchronize_to_stream(const eti::EtiFrameData& frame);
    void update_error_statistics(bool hasError);
    void reset_decoder();
    
    // CamelCase versions for existing implementation compatibility
    bool synchronizeToStream(const eti::EtiFrameData& frame);
    void updateErrorStatistics(bool hasError);

    // DAB/DAB+ specific decoding
    bool decode_dab_mp2(const QByteArray& data);
    bool decode_dab_plus(const QByteArray& data);
    
    // CamelCase versions for existing implementation compatibility
    bool decodeDabMp2(const QByteArray& data);
    bool decodeDabPlus(const QByteArray& data);
    
    // ETI frame parsing helpers
    QByteArray extract_subchannel_data(const eti::EtiFrameData& frame, quint8 subChannelId);
    
    // CamelCase versions for existing implementation compatibility
    QByteArray extractSubchannelData(const eti::EtiFrameData& frame, quint8 subChannelId);

    // Service component management
    void add_service_component(const ServiceComponent& component);
    void remove_service_component(quint16 serviceId);
    void update_service_component(const ServiceComponent& component);
    
    // CamelCase versions for existing implementation compatibility
    void addServiceComponent(const ServiceComponent& component);
    void removeServiceComponent(quint16 serviceId);
    void updateServiceComponent(const ServiceComponent& component);

    // Member variables
    bool initialized_;
    DecoderStatus status_;
    quint16 current_service_id_;
    quint32 frame_count_;
    double error_rate_;
    quint32 error_count_;
    quint32 max_buffer_size_;

    // Service management
    QHash<quint16, ServiceComponent> services_;
    QList<ServiceComponent> discovered_services_;

    // Audio processing
    QByteArray audio_buffer_;
    quint32 current_sample_rate_;
    QByteArray temp_buffer_;

    // Synchronization and buffering
    bool synchronized_;
    quint8 sync_pattern_[4];
    qint64 last_frame_time_;

    // Statistics
    struct Statistics {
        quint32 totalFrames;
        quint32 errorFrames;
        quint32 syncLoss;
        qint64 processingTime;
        double averageLatency;
    } stats_;

    // Constants
    static constexpr quint32 DEFAULT_BUFFER_SIZE = 8192;
    static constexpr quint32 MAX_AUDIO_BUFFER_SIZE = 65536;
    static constexpr double MAX_ERROR_RATE = 0.1; // 10% maximum error rate
    static constexpr quint32 SYNC_PATTERN = 0xFF1F; // DAB sync pattern
};