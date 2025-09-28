#pragma once

#include <QObject>
#include <QByteArray>
#include <QDateTime>
#include <QMap>
#include <memory>
#include <chrono>
#include "audio_monitoring_widget.h"

// Forward declarations
namespace eti { namespace audio { class audio_decoder; } }
class DabAudioDecoder;
namespace eti { class AudioExtractionPipeline; }

/**
 * @brief Audio codec type enumeration for decoder selection
 */
enum class AudioCodecType {
    Unknown = 0,
    DAB_MPEG1_Layer2 = 1,       ///< Traditional DAB MPEG-1 Layer 2
    DABPlus_AAC = 2,            ///< DAB+ AAC-LC
    DABPlus_HE_AAC_v1 = 3,      ///< DAB+ HE-AAC v1 with SBR
    DABPlus_HE_AAC_v2 = 4       ///< DAB+ HE-AAC v2 with SBR+PS
};

/**
 * @brief Audio quality level enumeration
 */
enum class AudioQualityLevel {
    Unknown = 0,
    Critical = 1,      ///< Severe quality issues
    Poor = 2,          ///< Below acceptable quality
    Fair = 3,          ///< Acceptable quality with minor issues
    Good = 4,          ///< Professional broadcast quality
    Excellent = 5      ///< Optimal quality
};

/**
 * @brief Comprehensive audio quality assessment structure
 */
struct AudioQualityAssessment {
    uint32_t service_id = 0;
    AudioQualityLevel overall_quality = AudioQualityLevel::Unknown;
    double peak_level_db = -60.0;        ///< Peak level in dBFS
    double rms_level_db = -60.0;         ///< RMS level in dBFS
    double signal_quality_percent = 0.0; ///< Overall signal quality (0-100%)
    double bit_error_rate = 0.0;         ///< Estimated bit error rate
    bool clipping_detected = false;      ///< Audio clipping detected
    bool silence_detected = false;       ///< Extended silence detected
    QDateTime last_update;               ///< Last assessment update time
    
    // Additional metrics
    uint32_t bitrate_kbps = 0;          ///< Audio bitrate
    AudioCodecType codec_type = AudioCodecType::Unknown; ///< Detected codec
    QString codec_info;                  ///< Detailed codec information
};

/**
 * @brief Complete GUI integration for audio decoder system
 * 
 * This class provides a comprehensive integration layer between the audio
 * decoding backend (audio_decoder, DabAudioDecoder, AudioExtractionPipeline)
 * and the GUI frontend (AudioMonitoringWidget) for professional DAB/DAB+
 * audio processing with real-time monitoring and quality assessment.
 * 
 * Features:
 * - Multi-codec audio decoding (MPEG-1 Layer 2, AAC, HE-AAC v1/v2)
 * - Real-time audio level monitoring and quality assessment
 * - Professional GUI integration with signal/slot architecture
 * - Performance monitoring and latency measurement
 * - Error handling and recovery mechanisms
 * - Thread-safe operation for real-time processing
 * - Memory-efficient audio processing with zero-copy optimization
 * - ETSI standards compliance validation
 * - Professional broadcast industry workflow support
 */
class AudioDecoderIntegration : public QObject {
    Q_OBJECT

public:
    explicit AudioDecoderIntegration(QObject* parent = nullptr);
    ~AudioDecoderIntegration();

    /**
     * @brief Initialize the audio decoder integration system
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * @brief Set the audio monitoring widget for GUI integration
     * @param widget Audio monitoring widget instance
     */
    void setMonitoringWidget(AudioMonitoringWidget* widget);

    /**
     * @brief Decode audio frame with appropriate codec
     * @param service_id DAB service identifier
     * @param encoded_data Encoded audio frame data
     * @param codec_type Audio codec type for decoder selection
     * @return true if decoding successful
     */
    bool decodeAudioFrame(uint32_t service_id, 
                         const QByteArray& encoded_data,
                         AudioCodecType codec_type);

    /**
     * @brief Start real-time audio processing for service
     * @param service_id Service identifier
     * @return true if real-time processing started successfully
     */
    bool startRealTimeProcessing(uint32_t service_id);

    /**
     * @brief Stop real-time audio processing
     */
    void stopRealTimeProcessing();

    /**
     * @brief Check if real-time processing is active
     * @return true if processing is active
     */
    bool isRealTimeProcessingActive() const;

    /**
     * @brief Get current service ID being processed
     * @return Service ID (0 if none)
     */
    uint32_t getCurrentServiceId() const;

    /**
     * @brief Get audio processing rate in frames per second
     * @return Processing rate (FPS)
     */
    double getProcessingRate() const;

    /**
     * @brief Get average processing latency
     * @return Average latency in microseconds
     */
    std::chrono::microseconds getAverageLatency() const;

    /**
     * @brief Get comprehensive audio quality assessment for service
     * @param service_id Service identifier
     * @return Audio quality assessment structure
     */
    AudioQualityAssessment getAudioQuality(uint32_t service_id) const;

    /**
     * @brief Get total frames processed since initialization
     * @return Total frame count
     */
    uint64_t getTotalFramesProcessed() const;

    /**
     * @brief Get total bytes processed since initialization
     * @return Total byte count
     */
    uint64_t getTotalBytesProcessed() const;

    /**
     * @brief Check if integration is initialized
     * @return true if initialized
     */
    bool isInitialized() const;

public slots:
    /**
     * @brief Select service for audio decoding
     * @param service_id Service identifier
     */
    void selectServiceForDecoding(uint32_t service_id);

    /**
     * @brief Reset processing statistics
     */
    void resetProcessingStatistics();

    /**
     * @brief Set audio gain adjustment
     * @param gain Gain multiplier (0.1-2.0)
     */
    void setAudioGain(double gain);

    /**
     * @brief Enable/disable error concealment
     * @param enabled Error concealment state
     */
    void setErrorConcealmentEnabled(bool enabled);

    /**
     * @brief Set processing buffer size
     * @param buffer_size_ms Buffer size in milliseconds
     */
    void setProcessingBufferSize(uint32_t buffer_size_ms);

signals:
    /**
     * @brief Emitted when initialization completes
     * @param success true if initialization successful
     */
    void initialization_completed(bool success);

    /**
     * @brief Emitted when audio frame is successfully decoded
     * @param service_id Service identifier
     * @param pcm_data Decoded PCM audio data
     */
    void audio_frame_decoded(uint32_t service_id, const QByteArray& pcm_data);

    /**
     * @brief Emitted when audio levels are updated
     * @param levels Map of service ID (as string) to audio level (0-100)
     */
    void audio_levels_updated(const QMap<QString, int>& levels);

    /**
     * @brief Emitted when audio quality changes for a service
     * @param service_id Service identifier
     * @param quality Audio quality level
     */
    void audio_quality_changed(uint32_t service_id, AudioMonitoringWidget::AudioQuality quality);

    /**
     * @brief Emitted when real-time processing starts
     * @param service_id Service identifier
     */
    void real_time_processing_started(uint32_t service_id);

    /**
     * @brief Emitted when real-time processing stops
     * @param service_id Service identifier
     */
    void real_time_processing_stopped(uint32_t service_id);

    /**
     * @brief Emitted when service selection changes
     * @param service_id New selected service identifier
     */
    void service_selection_changed(uint32_t service_id);

    /**
     * @brief Emitted when performance metrics are updated
     * @param processing_rate_fps Processing rate in frames per second
     * @param average_latency_us Average latency in microseconds
     */
    void performance_metrics_updated(double processing_rate_fps, std::chrono::microseconds average_latency_us);

    /**
     * @brief Emitted when decoding error occurs
     * @param service_id Service identifier
     * @param error_message Error description
     */
    void decoding_error_occurred(uint32_t service_id, const QString& error_message);

    /**
     * @brief Emitted when audio clipping is detected
     * @param service_id Service identifier
     * @param peak_level_db Peak level that caused clipping
     */
    void audio_clipping_detected(uint32_t service_id, double peak_level_db);

    /**
     * @brief Emitted when extended silence is detected
     * @param service_id Service identifier
     * @param duration_ms Silence duration in milliseconds
     */
    void audio_silence_detected(uint32_t service_id, uint32_t duration_ms);

private:
    /**
     * @brief Update audio quality metrics for service
     * @param service_id Service identifier
     * @param pcm_data Decoded PCM data
     * @param codec_type Audio codec type
     */
    void updateAudioQualityMetrics(uint32_t service_id, 
                                  const QByteArray& pcm_data,
                                  AudioCodecType codec_type);

    /**
     * @brief Update monitoring widget with audio data
     * @param service_id Service identifier
     * @param pcm_data PCM audio data
     */
    void updateMonitoringWidget(uint32_t service_id, const QByteArray& pcm_data);

    /**
     * @brief Setup default configurations
     */
    void setupDefaultConfigurations();

    /**
     * @brief Setup signal connections
     */
    void setupSignalConnections();

    /**
     * @brief Cleanup resources
     */
    void cleanup();

private:
    class Private;
    std::unique_ptr<Private> d_ptr;
};

Q_DECLARE_METATYPE(AudioCodecType)
Q_DECLARE_METATYPE(AudioQualityLevel)
Q_DECLARE_METATYPE(AudioQualityAssessment)