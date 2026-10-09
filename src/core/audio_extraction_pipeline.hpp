/**
 * @file audio_extraction_pipeline.h
 * @brief Professional Audio Extraction Pipeline for DAB/DAB+ streams
 * 
 * Implements real-time audio extraction and conversion from ETI streams with
 * support for multiple DAB/DAB+ audio formats, professional quality processing,
 * and multi-service concurrent extraction.
 * 
 * @author Network/Stream Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 */

#ifndef AUDIO_EXTRACTION_PIPELINE_H
#define AUDIO_EXTRACTION_PIPELINE_H

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QAudioFormat>
#include <QAudioDevice>
#include <QFile>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <vector>
#include <map>
#include <string>

#include "eti_types.hpp"

namespace eti {

/**
 * @brief Audio format enumeration for extraction output
 */
enum class AudioOutputFormat {
    PCM_16_MONO,        // 16-bit PCM mono
    PCM_16_STEREO,      // 16-bit PCM stereo
    PCM_24_MONO,        // 24-bit PCM mono
    PCM_24_STEREO,      // 24-bit PCM stereo
    PCM_32_FLOAT,       // 32-bit float PCM
    WAV_16,             // WAV format 16-bit
    WAV_24,             // WAV format 24-bit
    FLAC_16,            // FLAC compressed 16-bit
    FLAC_24,            // FLAC compressed 24-bit
    MP3_128,            // MP3 128 kbps
    MP3_192,            // MP3 192 kbps
    MP3_320,            // MP3 320 kbps
    AAC_128,            // AAC 128 kbps
    AAC_192,            // AAC 192 kbps
    AAC_320             // AAC 320 kbps
};

/**
 * @brief DAB audio mode enumeration
 */
enum class DabAudioMode {
    DAB_MPEG1_L2,       // DAB MPEG-1 Layer 2
    DAB_PLUS_AAC,       // DAB+ AAC
    DMB_MPEG4_AAC,      // DMB MPEG-4 AAC
    UNKNOWN
};

/**
 * @brief Audio extraction configuration
 */
struct AudioExtractionConfig {
    AudioOutputFormat output_format = AudioOutputFormat::WAV_16;
    uint32_t sample_rate = 48000;              // Output sample rate
    uint16_t channels = 2;                     // Output channels (1=mono, 2=stereo)
    uint16_t bit_depth = 16;                   // Output bit depth
    bool enable_real_time = true;              // Real-time processing
    bool enable_buffering = true;              // Enable output buffering
    size_t buffer_size_ms = 100;               // Buffer size in milliseconds
    
    // Quality settings
    bool enable_error_concealment = true;     // Enable audio error concealment
    bool enable_dynamic_range = false;        // Enable dynamic range compression
    double gain_adjustment = 1.0;             // Audio gain adjustment (0.0-2.0)
    bool enable_noise_reduction = false;      // Enable noise reduction
    
    // Format-specific settings
    int mp3_quality = 2;                      // MP3 quality (0-9, 0=best)
    int flac_compression = 5;                 // FLAC compression (0-8, 8=best)
    int aac_bitrate = 128;                    // AAC bitrate in kbps
    
    // Metadata embedding
    bool embed_metadata = true;               // Embed service metadata
    bool embed_timestamp = true;              // Embed extraction timestamp
    bool embed_ensemble_info = true;          // Embed ensemble information
};

/**
 * @brief Audio service information
 */
struct AudioServiceInfo {
    uint16_t service_id;                      // Service identifier
    uint8_t sub_channel_id;                   // Sub-channel identifier
    std::string service_label;                // Service label/name
    std::string component_label;              // Component label
    DabAudioMode audio_mode;                  // Audio encoding mode
    uint16_t bitrate;                         // Bitrate in kbps
    uint16_t sample_rate;                     // Original sample rate
    uint8_t channels;                         // Number of channels
    uint8_t protection_level;                 // Error protection level
    bool is_stereo;                          // Stereo flag
    bool has_program_type;                   // Program type available
    uint8_t program_type;                    // Program type code
    
    // Quality metrics
    double signal_quality = 0.0;             // Signal quality (0.0-1.0)
    double error_rate = 0.0;                 // Bit error rate
    size_t frames_processed = 0;             // Total frames processed
    size_t frames_with_errors = 0;           // Frames with errors
    
    std::chrono::system_clock::time_point last_update; // Last update time
};

/**
 * @brief Extracted audio data container
 */
struct AudioData {
    std::vector<int16_t> samples;             // Audio samples (16-bit)
    std::vector<float> samples_float;         // Audio samples (float)
    uint32_t sample_rate;                     // Sample rate
    uint16_t channels;                        // Number of channels
    uint16_t bit_depth;                       // Bit depth
    size_t frame_count;                       // Number of audio frames
    std::chrono::system_clock::time_point timestamp; // Extraction timestamp
    
    // Metadata
    AudioServiceInfo service_info;            // Associated service information
    bool is_valid = false;                   // Data validity flag
    bool has_errors = false;                 // Error flag
    double peak_level = 0.0;                 // Peak audio level
    double rms_level = 0.0;                  // RMS audio level
    
    // Quality metrics
    double snr = 0.0;                        // Signal-to-noise ratio
    double thd = 0.0;                        // Total harmonic distortion
    
    size_t size_bytes() const {
        return samples.size() * sizeof(int16_t);
    }
    
    std::chrono::milliseconds duration() const {
        if (sample_rate == 0) return std::chrono::milliseconds{0};
        return std::chrono::milliseconds((samples.size() / channels * 1000) / sample_rate);
    }
};

/**
 * @brief Audio extraction worker thread for concurrent processing
 */
class AudioExtractionWorker : public QObject {
    Q_OBJECT

public:
    explicit AudioExtractionWorker(uint16_t service_id, 
                                  const AudioExtractionConfig& config,
                                  QObject* parent = nullptr);
    ~AudioExtractionWorker();

public slots:
    void start_extraction();
    void stop_extraction();
    void pause_extraction();
    void resume_extraction();
    void process_frame(const eti::EtiFrame& frame);
    void update_config(const eti::AudioExtractionConfig& config);
    void flush_buffers();

signals:
    void extraction_started(uint16_t service_id);
    void extraction_stopped(uint16_t service_id);
    void extraction_paused(uint16_t service_id);
    void extraction_resumed(uint16_t service_id);
    void audio_data_ready(uint16_t service_id, const eti::AudioData& audio_data);
    void service_info_updated(uint16_t service_id, const eti::AudioServiceInfo& info);
    void error_occurred(uint16_t service_id, const QString& error);
    void quality_metrics_updated(uint16_t service_id, double snr, double thd);

private slots:
    void process_audio_queue();

private:
    void initialize_decoder();
    void cleanup_decoder();
    bool decode_dab_audio(const std::vector<uint8_t>& audio_data, AudioData& output);
    bool decode_dabplus_audio(const std::vector<uint8_t>& aac_data, AudioData& output);
    bool decode_mpeg2_audio(const std::vector<uint8_t>& mpeg_data, AudioData& output);
    bool decode_audio_data(const std::vector<uint8_t>& audio_data, AudioData& output);
    
    std::vector<uint8_t> extract_service_audio(const EtiFrame& frame);
    AudioServiceInfo extract_service_info(const EtiFrame& frame);
    void update_service_info(const EtiFrame& frame);
    
    void apply_gain_adjustment(AudioData& audio_data);
    void apply_noise_reduction(AudioData& audio_data);
    void apply_error_concealment(AudioData& audio_data);
    void calculate_audio_levels(AudioData& audio_data);
    void calculate_quality_metrics(AudioData& audio_data);
    
    bool convert_to_output_format(const AudioData& input, AudioData& output);
    void embed_metadata(AudioData& audio_data);
    
    uint16_t m_service_id;
    AudioExtractionConfig m_config;
    AudioServiceInfo m_service_info;
    
    std::atomic<bool> m_extracting{false};
    std::atomic<bool> m_paused{false};
    
    QQueue<EtiFrame> m_frame_queue;
    QMutex m_queue_mutex;
    std::unique_ptr<QTimer> m_process_timer;
    
    // Audio processing components
    void* m_faad_decoder = nullptr;           // FAAD2 AAC decoder
    void* m_mpeg_decoder = nullptr;           // MPEG audio decoder
    
    // Buffer management
    std::vector<uint8_t> m_audio_buffer;
    size_t m_buffer_position = 0;
    
    // Performance tracking
    std::chrono::steady_clock::time_point m_last_frame_time;
    std::vector<double> m_processing_times;
    static constexpr size_t PERF_HISTORY_SIZE = 100;
};

/**
 * @brief Professional Audio Extraction Pipeline
 * 
 * Multi-threaded, high-performance audio extraction system for DAB/DAB+ streams:
 * - Concurrent multi-service audio extraction
 * - Real-time DAB/DAB+ decoding with FAAD2 integration
 * - Professional quality audio processing and format conversion
 * - Metadata extraction and embedding
 * - Audio level monitoring and quality metrics
 * - Multiple output format support (PCM, WAV, FLAC, MP3, AAC)
 */
class AudioExtractionPipeline : public QObject {
    Q_OBJECT

public:
    explicit AudioExtractionPipeline(QObject* parent = nullptr);
    ~AudioExtractionPipeline();

    // Service management
    bool add_service(uint16_t service_id, const AudioExtractionConfig& config = AudioExtractionConfig{});
    bool remove_service(uint16_t service_id);
    void remove_all_services();
    QList<uint16_t> get_active_services() const;
    bool is_service_active(uint16_t service_id) const;
    
    // Extraction control
    void start_extraction();
    void stop_extraction();
    void pause_extraction();
    void resume_extraction();
    void start_service_extraction(uint16_t service_id);
    void stop_service_extraction(uint16_t service_id);
    void pause_service_extraction(uint16_t service_id);
    void resume_service_extraction(uint16_t service_id);
    
    // Frame processing
    void process_frame(const EtiFrame& frame);
    void process_frame_batch(const std::vector<EtiFrame>& frames);
    
    // Configuration management
    void set_service_config(uint16_t service_id, const AudioExtractionConfig& config);
    AudioExtractionConfig get_service_config(uint16_t service_id) const;
    void set_default_config(const AudioExtractionConfig& config);
    AudioExtractionConfig get_default_config() const;
    
    // Service information
    AudioServiceInfo get_service_info(uint16_t service_id) const;
    QList<AudioServiceInfo> get_all_service_info() const;
    void update_service_info_from_ensemble(const Ensemble& ensemble);
    
    // Audio data access
    bool has_audio_data(uint16_t service_id) const;
    AudioData get_latest_audio_data(uint16_t service_id) const;
    std::vector<AudioData> get_audio_data_batch(uint16_t service_id, size_t max_count = 10) const;
    void clear_audio_data(uint16_t service_id);
    
    // Performance monitoring
    double get_extraction_rate(uint16_t service_id) const;
    std::chrono::microseconds get_processing_latency(uint16_t service_id) const;
    size_t get_frames_processed(uint16_t service_id) const;
    size_t get_frames_with_errors(uint16_t service_id) const;
    double get_error_rate(uint16_t service_id) const;
    
    // Audio level monitoring
    double get_peak_level(uint16_t service_id) const;
    double get_rms_level(uint16_t service_id) const;
    std::vector<double> get_level_history(uint16_t service_id, size_t samples = 100) const;
    void enable_level_monitoring(bool enabled);
    
    // Quality metrics
    double get_signal_quality(uint16_t service_id) const;
    double get_snr(uint16_t service_id) const;
    double get_thd(uint16_t service_id) const;
    void enable_quality_monitoring(bool enabled);
    
    // Format support
    QList<AudioOutputFormat> get_supported_formats() const;
    QString get_format_description(AudioOutputFormat format) const;
    QString get_format_extension(AudioOutputFormat format) const;
    bool is_format_lossy(AudioOutputFormat format) const;
    
    // Error handling
    QStringList get_service_errors(uint16_t service_id) const;
    void clear_service_errors(uint16_t service_id);
    size_t get_error_count(uint16_t service_id) const;
    
    // Advanced features
    void enable_real_time_processing(bool enabled);
    void set_processing_priority(int priority); // Thread priority
    void enable_metadata_embedding(bool enabled);
    void set_buffer_size(size_t buffer_size_ms);
    
    // Audio device integration
    QList<QAudioDevice> get_available_audio_devices() const;
    bool set_audio_device(uint16_t service_id, const QAudioDevice& device);
    void enable_audio_playback(uint16_t service_id, bool enabled);
    
    // File output
    bool start_file_output(uint16_t service_id, const QString& filename, 
                          AudioOutputFormat format = AudioOutputFormat::WAV_16);
    void stop_file_output(uint16_t service_id);
    bool is_file_output_active(uint16_t service_id) const;
    
    // Callback registration
    using AudioDataCallback = std::function<void(uint16_t, const AudioData&)>;
    using ServiceInfoCallback = std::function<void(uint16_t, const AudioServiceInfo&)>;
    using ErrorCallback = std::function<void(uint16_t, const QString&)>;
    using QualityCallback = std::function<void(uint16_t, double, double)>;
    
    void set_audio_data_callback(AudioDataCallback callback);
    void set_service_info_callback(ServiceInfoCallback callback);
    void set_error_callback(ErrorCallback callback);
    void set_quality_callback(QualityCallback callback);

signals:
    void service_extraction_started(uint16_t service_id);
    void service_extraction_stopped(uint16_t service_id);
    void service_extraction_paused(uint16_t service_id);
    void service_extraction_resumed(uint16_t service_id);
    void audio_data_ready(uint16_t service_id, const eti::AudioData& audio_data);
    void service_info_updated(uint16_t service_id, const eti::AudioServiceInfo& info);
    void extraction_error_occurred(uint16_t service_id, const QString& error);
    void quality_metrics_updated(uint16_t service_id, double snr, double thd);
    void audio_levels_updated(uint16_t service_id, double peak, double rms);
    void file_output_started(uint16_t service_id, const QString& filename);
    void file_output_stopped(uint16_t service_id, const QString& filename);

private slots:
    void handle_worker_extraction_started(uint16_t service_id);
    void handle_worker_extraction_stopped(uint16_t service_id);
    void handle_worker_audio_data(uint16_t service_id, const eti::AudioData& audio_data);
    void handle_worker_service_info(uint16_t service_id, const eti::AudioServiceInfo& info);
    void handle_worker_error(uint16_t service_id, const QString& error);
    void handle_worker_quality_metrics(uint16_t service_id, double snr, double thd);

private:
    void initialize_service_worker(uint16_t service_id);
    void cleanup_service_worker(uint16_t service_id);
    void validate_config(const AudioExtractionConfig& config);
    
    // Service management
    mutable QMutex m_services_mutex;
    QMap<uint16_t, AudioExtractionConfig> m_service_configs;
    QMap<uint16_t, AudioServiceInfo> m_service_info;
    QMap<uint16_t, std::unique_ptr<QThread>> m_worker_threads;
    QMap<uint16_t, std::unique_ptr<AudioExtractionWorker>> m_workers;
    
    // Configuration
    AudioExtractionConfig m_default_config;
    std::atomic<bool> m_extracting{false};
    std::atomic<bool> m_paused{false};
    std::atomic<bool> m_level_monitoring{true};
    std::atomic<bool> m_quality_monitoring{true};
    std::atomic<bool> m_real_time_processing{true};
    std::atomic<int> m_processing_priority{0};
    
    // Audio data storage
    QMap<uint16_t, QQueue<AudioData>> m_audio_data_queues;
    QMutex m_audio_data_mutex;
    
    // Level monitoring
    QMap<uint16_t, std::vector<double>> m_peak_level_history;
    QMap<uint16_t, std::vector<double>> m_rms_level_history;
    QMutex m_levels_mutex;
    
    // Error tracking
    QMap<uint16_t, QStringList> m_service_errors;
    QMutex m_errors_mutex;
    
    // File output
    QMap<uint16_t, QString> m_file_output_paths;
    QMap<uint16_t, std::unique_ptr<QFile>> m_output_files;
    QMutex m_file_output_mutex;
    
    // Callbacks
    AudioDataCallback m_audio_data_callback;
    ServiceInfoCallback m_service_info_callback;
    ErrorCallback m_error_callback;
    QualityCallback m_quality_callback;
    QMutex m_callbacks_mutex;
    
    static constexpr size_t MAX_CONCURRENT_SERVICES = 64;
    static constexpr size_t MAX_AUDIO_DATA_QUEUE_SIZE = 100;
    static constexpr size_t MAX_LEVEL_HISTORY_SIZE = 1000;
    static constexpr size_t MAX_ERROR_HISTORY = 50;
};

/**
 * @brief Factory for creating pre-configured audio extraction pipelines
 */
class AudioExtractionPipelineFactory {
public:
    /**
     * @brief Create pipeline for broadcast monitoring applications
     */
    static std::unique_ptr<AudioExtractionPipeline> create_monitoring_pipeline();

    /**
     * @brief Create pipeline for high-quality audio archival
     */
    static std::unique_ptr<AudioExtractionPipeline> create_archival_pipeline();

    /**
     * @brief Create pipeline for real-time audio streaming
     */
    static std::unique_ptr<AudioExtractionPipeline> create_streaming_pipeline();

    /**
     * @brief Create pipeline for audio analysis and measurement
     */
    static std::unique_ptr<AudioExtractionPipeline> create_analysis_pipeline();
};

/**
 * @brief Utility functions for audio processing
 */
namespace AudioUtils {
    
    /**
     * @brief Convert between audio sample formats
     */
    std::vector<int16_t> float_to_int16(const std::vector<float>& samples);
    std::vector<float> int16_to_float(const std::vector<int16_t>& samples);
    
    /**
     * @brief Calculate audio level metrics
     */
    double calculate_peak_level(const std::vector<int16_t>& samples);
    double calculate_rms_level(const std::vector<int16_t>& samples);
    double calculate_peak_level_dbfs(const std::vector<int16_t>& samples);
    double calculate_rms_level_dbfs(const std::vector<int16_t>& samples);
    
    /**
     * @brief Audio quality analysis
     */
    double calculate_snr(const std::vector<int16_t>& signal, const std::vector<int16_t>& noise);
    double calculate_thd(const std::vector<int16_t>& samples, uint32_t sample_rate);
    
    /**
     * @brief Sample rate conversion
     */
    std::vector<int16_t> resample_audio(const std::vector<int16_t>& input, 
                                       uint32_t input_rate, uint32_t output_rate);
    
    /**
     * @brief Audio format conversion
     */
    bool write_wav_file(const QString& filename, const AudioData& audio_data);
    bool write_flac_file(const QString& filename, const AudioData& audio_data, int compression_level = 5);
    bool write_mp3_file(const QString& filename, const AudioData& audio_data, int quality = 2);
    
    /**
     * @brief Metadata utilities
     */
    QJsonObject extract_audio_metadata(const AudioData& audio_data);
    bool embed_metadata_in_file(const QString& filename, const QJsonObject& metadata);
}

} // namespace eti

#endif // AUDIO_EXTRACTION_PIPELINE_H