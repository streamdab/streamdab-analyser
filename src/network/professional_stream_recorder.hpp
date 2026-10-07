/**
 * @file professional_stream_recorder.hpp
 * @brief Production-Grade Stream Recording with Metadata Embedding
 * 
 * Professional broadcast-grade recording system implementing:
 * - Time-based automatic segmentation with sub-second precision
 * - Comprehensive metadata embedding (ETSI compliance, quality metrics)
 * - Multiple format support (ETI-NI, ETI-LI, EDI, compressed archives)
 * - Automatic file rotation with configurable retention policies
 * - Zero-loss recording with overflow protection and recovery
 * - Professional broadcast workflow integration
 * 
 * @author Network/Stream Agent - Phase 3 Production Platform
 * @date 2025-09-28
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 */

#ifndef PROFESSIONAL_STREAM_RECORDER_HPP
#define PROFESSIONAL_STREAM_RECORDER_HPP

#include <QObject>
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QMutex>
#include <QThread>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QFileSystemWatcher>
#include <memory>
#include <atomic>
#include <chrono>
#include <vector>
#include <queue>
#include <unordered_map>
#include <functional>
#include <condition_variable>

#include "../core/eti_types.hpp"

namespace eti_network {

/**
 * @brief Professional recording format with metadata support
 */
enum class ProfessionalRecordingFormat {
    ETI_NI_NATIVE,              // Native ETI-NI format (6144 bytes/frame)
    ETI_LI_LINEAR,              // Linear ETI format for processing
    EDI_ENCAPSULATED,           // Encapsulated DAB Interface (RFC 6936)
    RAW_STREAM_BINARY,          // Unprocessed stream data
    COMPRESSED_ETI_ARCHIVE,     // ZIP compressed with embedded metadata
    PROFESSIONAL_BROADCAST,     // Broadcast-standard format with full metadata
    TIMESTAMPED_SEGMENTS        // Time-segmented with precision timestamps
};

/**
 * @brief Comprehensive metadata structure for professional recording
 */
struct RecordingMetadata {
    // Stream identification
    QString stream_source;                      // Source URL/multicast address
    QString ensemble_label;                     // DAB ensemble name
    QString provider_name;                      // Service provider
    quint32 ensemble_id = 0;                    // Ensemble identifier
    
    // Timing information
    QDateTime recording_start;                  // Recording start timestamp
    QDateTime recording_end;                    // Recording end timestamp
    std::chrono::microseconds frame_duration{24000}; // Frame duration (24ms default)
    std::chrono::nanoseconds timestamp_precision{1000}; // Timestamp precision
    
    // Technical parameters
    double sample_rate = 48000.0;               // Audio sample rate
    size_t bit_depth = 16;                      // Audio bit depth
    size_t channels = 2;                        // Audio channels
    QString audio_codec = "DAB+";               // Audio codec used
    
    // Quality metrics
    double average_signal_quality = 1.0;       // Average signal quality (0.0-1.0)
    double minimum_signal_quality = 1.0;       // Minimum signal quality
    double packet_loss_rate = 0.0;             // Packet loss rate (0.0-1.0)
    double average_jitter_ms = 0.0;             // Average jitter in milliseconds
    double maximum_jitter_ms = 0.0;             // Maximum jitter in milliseconds
    
    // ETSI compliance
    QStringList etsi_compliance_flags;          // ETSI compliance status
    QStringList detected_errors;               // Detected ETSI errors
    QStringList warning_messages;              // Warning messages
    QString etsi_standard_version = "EN 300 799 V1.3.1"; // ETSI standard version
    
    // Service information
    struct ServiceInfo {
        quint32 service_id = 0;                 // Service ID (SID)
        QString service_label;                  // Service label
        QString service_type;                   // Service type (audio/data)
        quint8 subchannel_id = 0;              // Subchannel ID
        size_t bitrate_kbps = 0;               // Service bitrate
        QString protection_level;               // Protection level
        bool is_dab_plus = false;              // DAB+ flag
    };
    QList<ServiceInfo> services;               // Discovered services
    
    // File information
    QString filename;                          // Recording filename
    qint64 file_size_bytes = 0;               // File size in bytes
    size_t total_frames = 0;                  // Total frames recorded
    size_t valid_frames = 0;                  // Valid frames count
    size_t corrupted_frames = 0;              // Corrupted frames count
    QString checksum_md5;                     // MD5 checksum
    QString checksum_sha256;                  // SHA-256 checksum
    
    // Performance metrics
    double average_recording_fps = 0.0;        // Average recording FPS
    double peak_recording_fps = 0.0;           // Peak recording FPS
    std::chrono::microseconds average_latency{0}; // Average recording latency
    std::chrono::microseconds maximum_latency{0}; // Maximum recording latency
    size_t buffer_overflows = 0;              // Buffer overflow count
    size_t recording_errors = 0;              // Recording error count
    
    // Custom fields
    QJsonObject custom_metadata;              // Custom metadata fields
    QString recording_notes;                  // User notes
    QString operator_name;                    // Operator name
    QString equipment_info;                   // Equipment information
    
    // Serialization
    QJsonObject to_json() const;
    void from_json(const QJsonObject& json);
    QString to_xml() const;
    void from_xml(const QString& xml);
};

/**
 * @brief Professional segmentation configuration with sub-second precision
 */
struct ProfessionalSegmentationConfig {
    // Time-based segmentation
    std::chrono::seconds segment_duration{3600};        // 1 hour default
    std::chrono::milliseconds segment_precision{100};   // 100ms precision
    bool enable_frame_boundary_alignment = true;        // Align to frame boundaries
    
    // Size-based segmentation
    size_t max_segment_size_mb = 2048;                  // 2GB segment limit
    size_t max_segment_frames = 150000;                 // ~1 hour at 50fps
    bool prioritize_time_over_size = true;              // Time priority
    
    // File management
    QString filename_template = "eti_%Y%m%d_%H%M%S_%f"; // Filename pattern
    QString segment_extension = "eti";                   // Segment file extension
    QString metadata_extension = "meta";                 // Metadata file extension
    bool embed_metadata_in_file = true;                 // Embed metadata
    bool create_separate_metadata = true;               // Separate metadata files
    
    // Archive management
    bool enable_automatic_compression = false;          // Auto-compress segments
    int compression_level = 6;                          // Compression level (1-9)
    std::chrono::hours retention_period{168};          // 7 days retention
    bool enable_automatic_cleanup = true;              // Auto-cleanup old files
    double max_disk_usage_percent = 90.0;              // Disk usage threshold
    
    // Quality assurance
    bool enable_integrity_checking = true;             // File integrity checks
    bool enable_redundant_storage = false;             // Dual-path storage
    QString backup_storage_path;                        // Backup storage location
    bool verify_checksums_on_close = true;             // Verify on segment close
    
    // Professional features
    bool enable_broadcast_logging = true;              // Broadcast-style logging
    bool include_service_metadata = true;              // Include service info
    bool include_quality_metrics = true;               // Include quality data
    bool include_etsi_compliance = true;               // Include ETSI status
    QString operator_signature;                        // Digital signature
};

/**
 * @brief High-performance recording buffer with metadata tracking
 */
class ProfessionalRecordingBuffer {
public:
    explicit ProfessionalRecordingBuffer(size_t capacity = 10000);
    ~ProfessionalRecordingBuffer() = default;
    
    // Buffer operations
    bool push_frame(const eti::EtiFrame& frame, const RecordingMetadata& metadata);
    bool pop_frame(eti::EtiFrame& frame, RecordingMetadata& metadata);
    void clear();
    
    // Buffer management
    size_t size() const;
    size_t capacity() const;
    double utilization() const;
    bool is_full() const;
    bool is_empty() const;
    
    // Overflow protection
    void enable_overflow_protection(bool enabled);
    void set_overflow_threshold(double threshold); // 0.0-1.0
    size_t get_overflow_count() const;
    size_t get_dropped_frames() const;
    
    // Performance metrics
    std::chrono::microseconds get_average_latency() const;
    std::chrono::microseconds get_maximum_latency() const;
    double get_throughput_fps() const;
    
    // Memory management
    void optimize_memory_usage();
    size_t get_memory_usage_bytes() const;

private:
    struct BufferEntry {
        eti::EtiFrame frame;
        RecordingMetadata metadata;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::queue<BufferEntry> m_buffer;
    size_t m_capacity;
    
    // Overflow protection
    std::atomic<bool> m_overflow_protection{true};
    std::atomic<double> m_overflow_threshold{0.9};
    std::atomic<size_t> m_overflow_count{0};
    std::atomic<size_t> m_dropped_frames{0};
    
    // Performance tracking
    std::vector<std::chrono::microseconds> m_latency_history;
    std::chrono::steady_clock::time_point m_last_operation_time;
    std::atomic<double> m_throughput_fps{0.0};
    
    void handle_buffer_overflow();
    void update_performance_metrics();
    static constexpr size_t LATENCY_HISTORY_SIZE = 1000;
};

/**
 * @brief Professional recording writer thread with metadata embedding
 */
class ProfessionalRecordingWriter : public QThread {
    Q_OBJECT
    
public:
    explicit ProfessionalRecordingWriter(const QString& output_path,
                                       ProfessionalRecordingFormat format,
                                       const ProfessionalSegmentationConfig& config,
                                       QObject* parent = nullptr);
    ~ProfessionalRecordingWriter() override;
    
    // Writing operations
    void write_frame(const eti::EtiFrame& frame, const RecordingMetadata& metadata);
    void write_frame_batch(const std::vector<std::pair<eti::EtiFrame, RecordingMetadata>>& frames);
    void flush_buffers();
    void force_segment_rotation();
    
    // Configuration
    void update_config(const ProfessionalSegmentationConfig& config);
    ProfessionalSegmentationConfig get_config() const;
    void set_output_path(const QString& path);
    QString get_current_segment_path() const;
    
    // Status information
    size_t get_frames_written() const;
    size_t get_bytes_written() const;
    size_t get_segments_created() const;
    QStringList get_completed_segments() const;
    RecordingMetadata get_current_metadata() const;
    
    // Performance metrics
    double get_writing_fps() const;
    std::chrono::microseconds get_writing_latency() const;
    double get_disk_usage_mb_per_hour() const;
    size_t get_compression_ratio() const;

signals:
    void segment_started(const QString& segment_path);
    void segment_completed(const QString& segment_path, const RecordingMetadata& metadata);
    void metadata_embedded(const QString& segment_path, const QJsonObject& metadata);
    void compression_completed(const QString& original_path, const QString& compressed_path);
    void writing_error(const QString& error, int severity);
    void disk_space_warning(const QString& path, double usage_percent);
    void integrity_check_completed(const QString& segment_path, bool passed);

protected:
    void run() override;

private slots:
    void handle_segment_timer();
    void handle_cleanup_timer();
    void check_disk_space();

private:
    void initialize_writer();
    void finalize_writer();
    void create_new_segment();
    void close_current_segment();
    void write_frame_to_segment(const eti::EtiFrame& frame, const RecordingMetadata& metadata);
    void embed_metadata_in_segment();
    void create_metadata_file(const QString& segment_path, const RecordingMetadata& metadata);
    void compress_segment(const QString& segment_path);
    void verify_segment_integrity(const QString& segment_path);
    void cleanup_old_segments();
    QString generate_segment_filename() const;
    void calculate_checksums(const QString& file_path, QString& md5, QString& sha256);
    
    QString m_output_path;
    ProfessionalRecordingFormat m_format;
    ProfessionalSegmentationConfig m_config;
    
    std::unique_ptr<QFile> m_current_segment;
    std::unique_ptr<ProfessionalRecordingBuffer> m_buffer;
    std::unique_ptr<QTimer> m_segment_timer;
    std::unique_ptr<QTimer> m_cleanup_timer;
    
    RecordingMetadata m_current_metadata;
    std::atomic<size_t> m_frames_written{0};
    std::atomic<size_t> m_bytes_written{0};
    std::atomic<size_t> m_segments_created{0};
    
    QStringList m_completed_segments;
    mutable std::mutex m_segments_mutex;
    
    // Performance tracking
    std::chrono::steady_clock::time_point m_segment_start_time;
    std::chrono::steady_clock::time_point m_writing_start_time;
    std::vector<double> m_fps_history;
    std::vector<std::chrono::microseconds> m_latency_history;
    
    std::atomic<bool> m_running{false};
    mutable std::mutex m_config_mutex;
    
    static constexpr size_t PERFORMANCE_HISTORY_SIZE = 100;
    static constexpr std::chrono::milliseconds DISK_CHECK_INTERVAL{30000}; // 30 seconds
};

/**
 * @brief Production-Grade Professional Stream Recorder
 * 
 * Enterprise-class recording system implementing:
 * - Zero-loss recording with overflow protection
 * - Comprehensive metadata embedding with ETSI compliance
 * - Time-based segmentation with sub-second precision
 * - Multiple format support with professional broadcast standards
 * - Automatic file rotation and retention management
 * - Real-time performance monitoring and optimization
 */
class ProfessionalStreamRecorder : public QObject {
    Q_OBJECT
    
public:
    explicit ProfessionalStreamRecorder(QObject* parent = nullptr);
    ~ProfessionalStreamRecorder() override;
    
    // Recording session management
    QString start_recording_session(const QString& output_directory,
                                   ProfessionalRecordingFormat format = ProfessionalRecordingFormat::ETI_NI_NATIVE,
                                   const ProfessionalSegmentationConfig& config = ProfessionalSegmentationConfig{});
    bool stop_recording_session(const QString& session_id);
    bool pause_recording_session(const QString& session_id);
    bool resume_recording_session(const QString& session_id);
    void stop_all_sessions();
    
    // Frame recording with metadata
    void record_frame(const QString& session_id, const eti::EtiFrame& frame, 
                     const RecordingMetadata& metadata = RecordingMetadata{});
    void record_frame_with_quality(const QString& session_id, const eti::EtiFrame& frame,
                                  const StreamQualityMetrics& quality_metrics);
    void record_frame_batch(const QString& session_id, 
                           const std::vector<std::pair<eti::EtiFrame, RecordingMetadata>>& frames);
    
    // Session information
    QStringList get_active_sessions() const;
    QStringList get_all_sessions() const;
    RecordingMetadata get_session_metadata(const QString& session_id) const;
    bool is_session_active(const QString& session_id) const;
    bool is_session_paused(const QString& session_id) const;
    
    // Configuration management
    void set_default_config(const ProfessionalSegmentationConfig& config);
    ProfessionalSegmentationConfig get_default_config() const;
    void update_session_config(const QString& session_id, const ProfessionalSegmentationConfig& config);
    ProfessionalSegmentationConfig get_session_config(const QString& session_id) const;
    
    // Format support
    QList<ProfessionalRecordingFormat> get_supported_formats() const;
    QString get_format_description(ProfessionalRecordingFormat format) const;
    QString get_format_extension(ProfessionalRecordingFormat format) const;
    bool supports_metadata_embedding(ProfessionalRecordingFormat format) const;
    
    // Performance monitoring
    double get_recording_fps(const QString& session_id) const;
    std::chrono::microseconds get_recording_latency(const QString& session_id) const;
    size_t get_total_frames_recorded(const QString& session_id) const;
    size_t get_total_bytes_recorded(const QString& session_id) const;
    size_t get_segments_created(const QString& session_id) const;
    
    // File management
    QStringList get_session_segments(const QString& session_id) const;
    qint64 get_session_total_size(const QString& session_id) const;
    void force_segment_rotation(const QString& session_id);
    void compress_session_segments(const QString& session_id);
    void cleanup_session_files(const QString& session_id, std::chrono::hours retention_hours);
    
    // Quality assurance
    bool verify_session_integrity(const QString& session_id) const;
    QStringList get_session_integrity_report(const QString& session_id) const;
    void enable_redundant_recording(const QString& session_id, const QString& backup_path);
    void enable_realtime_verification(const QString& session_id, bool enabled);
    
    // Metadata management
    void update_session_metadata(const QString& session_id, const RecordingMetadata& metadata);
    void add_custom_metadata(const QString& session_id, const QString& key, const QVariant& value);
    QJsonObject export_session_metadata(const QString& session_id) const;
    void import_session_metadata(const QString& session_id, const QJsonObject& metadata);
    
    // Professional features
    void set_operator_signature(const QString& signature);
    void enable_broadcast_logging(bool enabled);
    void set_equipment_info(const QString& equipment_info);
    void add_recording_notes(const QString& session_id, const QString& notes);
    
    // Disk space management
    double get_disk_usage_percent(const QString& directory) const;
    qint64 get_available_disk_space(const QString& directory) const;
    void set_disk_usage_threshold(double threshold_percent);
    void enable_automatic_cleanup(bool enabled);
    
    // Advanced configuration
    void enable_compression_optimization(bool enabled);
    void set_compression_level(int level); // 1-9
    void enable_integrity_checking(bool enabled);
    void set_buffer_optimization_mode(const QString& mode); // "latency", "quality", "balanced"
    
    // Callback registration
    using SessionCallback = std::function<void(const QString&, const RecordingMetadata&)>;
    using SegmentCallback = std::function<void(const QString&, const QString&, const RecordingMetadata&)>;
    using ErrorCallback = std::function<void(const QString&, const QString&, int)>;
    using QualityCallback = std::function<void(const QString&, const RecordingMetadata&)>;
    
    void set_session_callback(SessionCallback callback);
    void set_segment_callback(SegmentCallback callback);
    void set_error_callback(ErrorCallback callback);
    void set_quality_callback(QualityCallback callback);

signals:
    void recording_session_started(const QString& session_id, const RecordingMetadata& metadata);
    void recording_session_stopped(const QString& session_id, const RecordingMetadata& final_metadata);
    void recording_session_paused(const QString& session_id);
    void recording_session_resumed(const QString& session_id);
    
    void segment_started(const QString& session_id, const QString& segment_path);
    void segment_completed(const QString& session_id, const QString& segment_path, 
                          const RecordingMetadata& metadata);
    void metadata_embedded(const QString& session_id, const QString& segment_path);
    void compression_completed(const QString& session_id, const QString& original_path, 
                             const QString& compressed_path);
    
    void recording_error(const QString& session_id, const QString& error, int severity);
    void disk_space_warning(const QString& session_id, const QString& directory, double usage_percent);
    void integrity_check_completed(const QString& session_id, const QString& segment_path, bool passed);
    void quality_metrics_updated(const QString& session_id, const RecordingMetadata& metadata);

private slots:
    void handle_writer_segment_started(const QString& segment_path);
    void handle_writer_segment_completed(const QString& segment_path, const RecordingMetadata& metadata);
    void handle_writer_error(const QString& error, int severity);
    void handle_writer_disk_warning(const QString& path, double usage_percent);
    void perform_maintenance_tasks();

private:
    // Session management
    struct RecordingSession {
        QString session_id;
        QString output_directory;
        ProfessionalRecordingFormat format;
        ProfessionalSegmentationConfig config;
        std::unique_ptr<ProfessionalRecordingWriter> writer;
        RecordingMetadata metadata;
        bool is_active = false;
        bool is_paused = false;
        std::chrono::steady_clock::time_point start_time;
        std::chrono::steady_clock::time_point pause_time;
        std::chrono::microseconds total_pause_duration{0};
    };
    
    mutable std::mutex m_sessions_mutex;
    std::unordered_map<std::string, std::unique_ptr<RecordingSession>> m_sessions;
    std::atomic<size_t> m_next_session_id{1};
    
    // Global configuration
    ProfessionalSegmentationConfig m_default_config;
    QString m_operator_signature;
    QString m_equipment_info;
    std::atomic<bool> m_broadcast_logging{true};
    std::atomic<bool> m_automatic_cleanup{true};
    std::atomic<double> m_disk_threshold{90.0};
    
    // Maintenance
    std::unique_ptr<QTimer> m_maintenance_timer;
    std::unique_ptr<QFileSystemWatcher> m_disk_watcher;
    
    // Callbacks
    SessionCallback m_session_callback;
    SegmentCallback m_segment_callback;
    ErrorCallback m_error_callback;
    QualityCallback m_quality_callback;
    mutable std::mutex m_callback_mutex;
    
    // Helper methods
    QString generate_session_id();
    void initialize_session_writer(RecordingSession* session);
    void finalize_session(const QString& session_id);
    void validate_recording_config(const ProfessionalSegmentationConfig& config);
    void update_session_metadata_from_quality(RecordingSession* session, 
                                             const StreamQualityMetrics& quality);
    RecordingMetadata create_metadata_from_quality(const StreamQualityMetrics& quality);
    void perform_disk_cleanup();
    void check_all_disk_usage();
    
    static constexpr std::chrono::milliseconds MAINTENANCE_INTERVAL{60000}; // 1 minute
    static constexpr size_t MAX_CONCURRENT_SESSIONS = 8;
};

/**
 * @brief Factory for creating professional stream recorders
 */
class ProfessionalStreamRecorderFactory {
public:
    /**
     * @brief Create recorder optimized for broadcast operations
     */
    static std::unique_ptr<ProfessionalStreamRecorder> create_broadcast_recorder();
    
    /**
     * @brief Create recorder optimized for archival storage
     */
    static std::unique_ptr<ProfessionalStreamRecorder> create_archival_recorder();
    
    /**
     * @brief Create recorder optimized for monitoring applications
     */
    static std::unique_ptr<ProfessionalStreamRecorder> create_monitoring_recorder();
    
    /**
     * @brief Create recorder optimized for compliance logging
     */
    static std::unique_ptr<ProfessionalStreamRecorder> create_compliance_recorder();
    
    /**
     * @brief Create custom recorder with specific configuration
     */
    static std::unique_ptr<ProfessionalStreamRecorder> create_custom_recorder(
        const ProfessionalSegmentationConfig& config);
};

} // namespace eti_network

#endif // PROFESSIONAL_STREAM_RECORDER_HPP