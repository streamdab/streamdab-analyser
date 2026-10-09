/**
 * @file stream_recording_manager.h
 * @brief Professional Stream Recording Manager with time-based segmentation
 * 
 * Implements professional broadcast-grade ETI stream recording with configurable
 * time-based segmentation, multiple format support, and automated archive management.
 * Supports ETI-NI, ETI-LI, EDI, and compressed formats for long-term storage.
 * 
 * @author Network/Stream Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 */

#ifndef STREAM_RECORDING_MANAGER_H
#define STREAM_RECORDING_MANAGER_H

#include <QObject>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QMutex>
#include <QThread>
#include <QQueue>
#include <QDateTime>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <vector>
#include <string>
#include <unordered_map>
#include <queue>
#include <condition_variable>

#include "eti_types.hpp"

namespace eti {
Q_NAMESPACE

/**
 * @brief Recording format enumeration for different output types
 */
enum class RecordingFormat {
    ETI_NI,                 // Native ETI-NI format (6144 bytes/frame)
    ETI_LI,                 // Linear ETI format for direct processing
    EDI,                    // Encapsulated DAB Interface for IP distribution
    RAW_BINARY,             // Unprocessed stream data
    COMPRESSED_ETI,         // ZIP compressed ETI with metadata
    COMPRESSED_ARCHIVE      // TAR/ZIP archive with metadata for long-term storage
};

/**
 * @brief Time-based segmentation configuration
 */
struct SegmentationConfig {
    std::chrono::minutes segment_duration{60};     // Segment duration in minutes
    std::chrono::hours daily_rotation{24};         // Daily file rotation (0 = disabled)
    bool enable_compression = false;               // Compress completed segments
    bool preserve_timestamps = true;               // Preserve original timestamps
    size_t max_segment_size_mb = 1024;            // Maximum segment size in MB
    size_t max_archive_segments = 100;            // Maximum segments per archive
    
    // Automatic cleanup configuration
    std::chrono::hours retention_period{168};     // 7 days default retention
    bool auto_cleanup = true;                     // Enable automatic cleanup
    double max_disk_usage_percent = 85.0;         // Maximum disk usage percentage
    
    // Naming convention
    QString filename_pattern = "eti_%Y%m%d_%H%M%S"; // strftime-style pattern
    QString segment_suffix = "seg";                // Segment file suffix
    QString archive_suffix = "archive";           // Archive file suffix
};

/**
 * @brief Recording session statistics and metadata
 */
struct RecordingSession {
    QString session_id;                           // Unique session identifier
    QDateTime start_time;                         // Recording start time
    QDateTime end_time;                           // Recording end time (if completed)
    RecordingFormat format;                       // Recording format
    SegmentationConfig segmentation;              // Segmentation configuration
    
    std::atomic<size_t> frames_recorded{0};       // Total frames recorded
    std::atomic<size_t> segments_created{0};      // Number of segments created
    std::atomic<size_t> bytes_written{0};         // Total bytes written
    std::atomic<size_t> errors_encountered{0};    // Recording errors
    
    QString output_directory;                     // Base output directory
    QStringList segment_files;                    // List of segment files
    QStringList archive_files;                    // List of archive files
    
    bool is_active = false;                       // Recording session active
    bool is_paused = false;                       // Recording session paused
    
    // Custom copy constructor to handle atomic members
    RecordingSession() = default;
    RecordingSession(const RecordingSession& other) 
        : session_id(other.session_id)
        , start_time(other.start_time)
        , end_time(other.end_time)
        , format(other.format)
        , segmentation(other.segmentation)
        , frames_recorded(other.frames_recorded.load())
        , segments_created(other.segments_created.load())
        , bytes_written(other.bytes_written.load())
        , errors_encountered(other.errors_encountered.load())
        , output_directory(other.output_directory)
        , segment_files(other.segment_files)
        , archive_files(other.archive_files)
        , is_active(other.is_active)
        , is_paused(other.is_paused)
    {}
    
    // Custom assignment operator to handle atomic members
    RecordingSession& operator=(const RecordingSession& other) {
        if (this != &other) {
            session_id = other.session_id;
            start_time = other.start_time;
            end_time = other.end_time;
            format = other.format;
            segmentation = other.segmentation;
            frames_recorded = other.frames_recorded.load();
            segments_created = other.segments_created.load();
            bytes_written = other.bytes_written.load();
            errors_encountered = other.errors_encountered.load();
            output_directory = other.output_directory;
            segment_files = other.segment_files;
            archive_files = other.archive_files;
            is_active = other.is_active;
            is_paused = other.is_paused;
        }
        return *this;
    }
    
    // Performance metrics
    double average_frame_rate = 0.0;              // Average recording frame rate
    double peak_frame_rate = 0.0;                 // Peak recording frame rate
    std::chrono::microseconds average_latency{0}; // Average recording latency
    std::chrono::microseconds max_latency{0};     // Maximum recording latency
};

/**
 * @brief Recording worker thread for high-performance file I/O
 */
class RecordingWorker : public QObject {
    Q_OBJECT

public:
    explicit RecordingWorker(const RecordingSession& session, QObject* parent = nullptr);
    ~RecordingWorker();

public slots:
    void start_recording();
    void stop_recording();
    void pause_recording();
    void resume_recording();
    void record_frame(const eti::EtiFrame& frame);
    void flush_buffers();
    void rotate_segment();
    void cleanup_old_files();

signals:
    void recording_started(const QString& session_id);
    void recording_stopped(const QString& session_id);
    void recording_paused(const QString& session_id);
    void recording_resumed(const QString& session_id);
    void segment_completed(const QString& session_id, const QString& filename);
    void archive_created(const QString& session_id, const QString& filename);
    void error_occurred(const QString& session_id, const QString& error);
    void statistics_updated(const eti::RecordingSession& session);
    void disk_space_warning(const QString& directory, double usage_percent);

private slots:
    void handle_segment_timer();
    void handle_rotation_timer();
    void handle_cleanup_timer();

private:
    void initialize_recording();
    void finalize_recording();
    void create_new_segment();
    void close_current_segment();
    void write_frame_to_file(const EtiFrame& frame);
    void write_metadata_header();
    void compress_segment(const QString& segment_path);
    void create_archive();
    void check_disk_space();
    void update_statistics();
    QString generate_segment_filename() const;
    QString generate_archive_filename() const;
    
    RecordingSession m_session;
    std::unique_ptr<QFile> m_current_file;
    std::unique_ptr<QTimer> m_segment_timer;
    std::unique_ptr<QTimer> m_rotation_timer;
    std::unique_ptr<QTimer> m_cleanup_timer;
    
    QMutex m_file_mutex;
    QQueue<EtiFrame> m_frame_queue;
    QMutex m_queue_mutex;
    
    std::atomic<bool> m_recording{false};
    std::atomic<bool> m_paused{false};
    std::atomic<size_t> m_current_segment_frames{0};
    std::atomic<size_t> m_current_segment_bytes{0};
    
    QDateTime m_current_segment_start;
    QString m_current_segment_path;
    
    // Performance tracking
    std::chrono::steady_clock::time_point m_last_frame_time;
    std::vector<double> m_frame_rate_history;
    std::vector<std::chrono::microseconds> m_latency_history;
    static constexpr size_t PERF_HISTORY_SIZE = 100;
};

/**
 * @brief Professional Stream Recording Manager
 * 
 * Thread-safe, high-performance recording manager for ETI streams with:
 * - Time-based automatic segmentation (minutes/hours/days)
 * - Multiple format support (ETI-NI, ETI-LI, EDI, compressed)
 * - Automatic archive creation and compression
 * - Intelligent disk space management
 * - Professional broadcast workflow integration
 * - Real-time performance monitoring
 */
class StreamRecordingManager : public QObject {
    Q_OBJECT

public:
    explicit StreamRecordingManager(QObject* parent = nullptr);
    ~StreamRecordingManager();

    // Recording session management
    QString start_recording_session(const QString& output_directory,
                                   RecordingFormat format = RecordingFormat::ETI_NI,
                                   const SegmentationConfig& config = SegmentationConfig{});
    bool stop_recording_session(const QString& session_id);
    bool pause_recording_session(const QString& session_id);
    bool resume_recording_session(const QString& session_id);
    
    // Frame recording
    void record_frame(const QString& session_id, const EtiFrame& frame);
    void record_frame_batch(const QString& session_id, const std::vector<EtiFrame>& frames);
    
    // Session information
    QStringList get_active_sessions() const;
    RecordingSession get_session_info(const QString& session_id) const;
    QList<RecordingSession> get_all_sessions() const;
    bool is_session_active(const QString& session_id) const;
    bool is_session_paused(const QString& session_id) const;
    
    // Configuration management
    void set_default_segmentation_config(const SegmentationConfig& config);
    SegmentationConfig get_default_segmentation_config() const;
    void update_session_config(const QString& session_id, const SegmentationConfig& config);
    
    // Format support
    QList<RecordingFormat> get_supported_formats() const;
    QString get_format_description(RecordingFormat format) const;
    QString get_format_extension(RecordingFormat format) const;
    bool is_format_compressed(RecordingFormat format) const;
    
    // Directory and file management
    bool set_output_directory(const QString& session_id, const QString& directory);
    QString get_output_directory(const QString& session_id) const;
    QStringList get_session_files(const QString& session_id) const;
    QStringList get_session_archives(const QString& session_id) const;
    qint64 get_session_total_size(const QString& session_id) const;
    
    // Segment management
    void force_segment_rotation(const QString& session_id);
    void force_archive_creation(const QString& session_id);
    bool compress_session_segments(const QString& session_id);
    bool cleanup_session_files(const QString& session_id, std::chrono::hours retention_hours = std::chrono::hours{168});
    
    // Performance monitoring
    double get_recording_frame_rate(const QString& session_id) const;
    std::chrono::microseconds get_recording_latency(const QString& session_id) const;
    size_t get_total_frames_recorded(const QString& session_id) const;
    size_t get_total_bytes_recorded(const QString& session_id) const;
    
    // Disk space management
    double get_disk_usage_percent(const QString& directory) const;
    qint64 get_available_disk_space(const QString& directory) const;
    void enable_automatic_cleanup(bool enabled);
    void set_disk_usage_threshold(double threshold_percent);
    
    // Error handling and monitoring
    QStringList get_session_errors(const QString& session_id) const;
    void clear_session_errors(const QString& session_id);
    size_t get_error_count(const QString& session_id) const;
    
    // Advanced features
    void enable_realtime_compression(bool enabled);
    void set_compression_level(int level); // 1-9, 9 = maximum compression
    void enable_integrity_checking(bool enabled);
    void set_buffer_size(size_t buffer_frames);
    
    // Callback registration
    using SessionCallback = std::function<void(const QString&, const RecordingSession&)>;
    using ErrorCallback = std::function<void(const QString&, const QString&)>;
    using FileCallback = std::function<void(const QString&, const QString&)>;
    
    void set_session_callback(SessionCallback callback);
    void set_error_callback(ErrorCallback callback);
    void set_file_callback(FileCallback callback);

signals:
    void recording_session_started(const QString& session_id);
    void recording_session_stopped(const QString& session_id);
    void recording_session_paused(const QString& session_id);
    void recording_session_resumed(const QString& session_id);
    void segment_completed(const QString& session_id, const QString& filename);
    void archive_created(const QString& session_id, const QString& filename);
    void recording_error_occurred(const QString& session_id, const QString& error);
    void session_statistics_updated(const QString& session_id, const eti::RecordingSession& session);
    void disk_space_warning(const QString& session_id, const QString& directory, double usage_percent);
    void automatic_cleanup_performed(const QString& session_id, const QStringList& deleted_files);

private slots:
    void handle_worker_recording_started(const QString& session_id);
    void handle_worker_recording_stopped(const QString& session_id);
    void handle_worker_segment_completed(const QString& session_id, const QString& filename);
    void handle_worker_archive_created(const QString& session_id, const QString& filename);
    void handle_worker_error(const QString& session_id, const QString& error);
    void handle_worker_statistics(const eti::RecordingSession& session);
    void handle_worker_disk_warning(const QString& directory, double usage_percent);

private:
    void initialize_session_worker(const QString& session_id);
    void cleanup_session_worker(const QString& session_id);
    QString generate_session_id() const;
    void validate_output_directory(const QString& directory);
    void validate_segmentation_config(const SegmentationConfig& config);
    
    // Compression worker methods
    void compression_worker_loop();
    void enqueue_for_compression(const QString& file_path);
    void compress_file(const QString& file_path);
    void create_output_directory(const QString& directory);
    
    // Session management
    mutable QMutex m_sessions_mutex;
    QMap<QString, RecordingSession> m_sessions;
    std::unordered_map<std::string, std::unique_ptr<QThread>> m_worker_threads;
    std::unordered_map<std::string, std::unique_ptr<RecordingWorker>> m_workers;
    
    // Configuration
    SegmentationConfig m_default_config;
    std::atomic<bool> m_realtime_compression{false};
    std::atomic<int> m_compression_level{6};
    
    // Compression worker thread management
    std::atomic<bool> m_compression_worker_active{false};
    std::condition_variable m_compression_queue_condition;
    std::mutex m_compression_queue_mutex;
    std::queue<QString> m_compression_queue;
    std::atomic<bool> m_integrity_checking{true};
    std::atomic<size_t> m_buffer_size{1000};
    std::atomic<bool> m_automatic_cleanup{true};
    std::atomic<double> m_disk_threshold{85.0};
    
    // Error tracking
    QMap<QString, QStringList> m_session_errors;
    QMutex m_errors_mutex;
    
    // Callbacks
    SessionCallback m_session_callback;
    ErrorCallback m_error_callback;
    FileCallback m_file_callback;
    QMutex m_callbacks_mutex;
    
    static constexpr size_t MAX_CONCURRENT_SESSIONS = 32;
    static constexpr size_t MAX_ERROR_HISTORY = 100;
};

/**
 * @brief Factory for creating pre-configured recording managers
 */
class StreamRecordingManagerFactory {
public:
    /**
     * @brief Create recording manager for standard broadcast operations
     */
    static std::unique_ptr<StreamRecordingManager> create_broadcast_recorder(
        const QString& base_directory);

    /**
     * @brief Create recording manager for continuous archival recording
     */
    static std::unique_ptr<StreamRecordingManager> create_archival_recorder(
        const QString& base_directory,
        std::chrono::hours retention_period = std::chrono::hours{168});

    /**
     * @brief Create recording manager for high-frequency short-term recording
     */
    static std::unique_ptr<StreamRecordingManager> create_monitoring_recorder(
        const QString& base_directory,
        std::chrono::minutes segment_duration = std::chrono::minutes{5});

    /**
     * @brief Create recording manager for compressed long-term storage
     */
    static std::unique_ptr<StreamRecordingManager> create_storage_recorder(
        const QString& base_directory,
        int compression_level = 9);
};

/**
 * @brief Utility functions for recording format conversion
 */
namespace RecordingUtils {
    
    /**
     * @brief Convert ETI frame to different recording formats
     */
    std::vector<uint8_t> convert_frame_to_format(const EtiFrame& frame, RecordingFormat format);
    
    /**
     * @brief Extract metadata from recorded ETI stream
     */
    QJsonObject extract_stream_metadata(const QString& recording_file);
    
    /**
     * @brief Validate recording file integrity
     */
    bool validate_recording_integrity(const QString& recording_file);
    
    /**
     * @brief Calculate recording file statistics
     */
    struct RecordingFileStats {
        size_t total_frames;
        size_t valid_frames;
        size_t corrupted_frames;
        QDateTime start_time;
        QDateTime end_time;
        double average_frame_rate;
        std::chrono::microseconds total_duration;
    };
    
    RecordingFileStats analyze_recording_file(const QString& recording_file);
    
    /**
     * @brief Compress recording file with metadata preservation
     */
    bool compress_recording_file(const QString& input_file, const QString& output_file, 
                                int compression_level = 6);
    
    /**
     * @brief Decompress recording file and validate integrity
     */
    bool decompress_recording_file(const QString& compressed_file, const QString& output_file);
}

} // namespace eti

#endif // STREAM_RECORDING_MANAGER_H