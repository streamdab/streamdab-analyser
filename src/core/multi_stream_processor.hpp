/**
 * @file multi_stream_processor.h
 * @brief Professional Multi-Stream Processor for concurrent ETI stream handling
 * 
 * Implements high-performance concurrent processing of multiple ETI streams with
 * load balancing, failover capabilities, and professional broadcast workflow
 * integration. Supports up to 32 concurrent streams with real-time processing.
 * 
 * @author Network/Stream Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyzer Team
 */

#ifndef MULTI_STREAM_PROCESSOR_H
#define MULTI_STREAM_PROCESSOR_H

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QThreadPool>
#include <QMutex>
#include <QReadWriteLock>
#include <QQueue>
#include <QNetworkInterface>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <vector>
#include <map>
#include <string>
#include <unordered_set>

#include "eti_types.hpp"
#include "network_stream_receiver.hpp"
#include "stream_recording_manager.hpp"
#include "audio_extraction_pipeline.hpp"

namespace eti {

/**
 * @brief Stream source type enumeration
 */
enum class StreamSourceType {
    MULTICAST_UDP,          // UDP multicast stream
    UNICAST_UDP,            // UDP unicast stream
    TCP_STREAM,             // TCP stream
    FILE_PLAYBACK,          // File-based playback
    HARDWARE_INTERFACE,     // Hardware ETI interface
    PIPE_INPUT             // Named pipe input
};

/**
 * @brief Stream processing mode enumeration
 */
enum class StreamProcessingMode {
    REAL_TIME,              // Real-time processing with minimal latency
    BATCH,                  // Batch processing for maximum throughput
    ANALYSIS,               // Analysis mode with detailed metrics
    MONITORING,             // Monitoring mode with alerts
    RECORDING,              // Recording mode with archival
    RELAY                   // Relay mode for stream distribution
};

/**
 * @brief Stream configuration for multi-stream processing
 */
struct StreamConfig {
    QString stream_id;                          // Unique stream identifier
    QString description;                        // Human-readable description
    StreamSourceType source_type;               // Source type
    StreamProcessingMode processing_mode;       // Processing mode
    
    // Network configuration
    QString multicast_address;                  // Multicast address (if applicable)
    quint16 port;                              // Network port
    QString interface_name;                     // Network interface
    
    // File configuration
    QString file_path;                         // File path (for file playback)
    bool loop_playback = false;                // Loop file playback
    
    // Processing configuration
    bool enable_recording = false;             // Enable stream recording
    bool enable_audio_extraction = false;      // Enable audio extraction
    bool enable_analysis = true;               // Enable stream analysis
    bool enable_monitoring = true;             // Enable quality monitoring
    
    // Performance configuration
    uint32_t priority = 50;                    // Processing priority (0-100)
    size_t buffer_size = 1000;                // Buffer size in frames
    std::chrono::milliseconds timeout{5000};   // Processing timeout
    
    // Quality thresholds
    double min_frame_rate = 200.0;             // Minimum acceptable frame rate
    double max_frame_loss = 0.01;              // Maximum acceptable frame loss
    std::chrono::milliseconds max_latency{100}; // Maximum acceptable latency
    
    // Failover configuration
    bool enable_failover = false;              // Enable automatic failover
    QStringList backup_addresses;              // Backup stream addresses
    std::chrono::milliseconds failover_delay{1000}; // Failover delay
    
    // Output configuration
    QString output_directory;                  // Output directory for recordings
    RecordingFormat recording_format = RecordingFormat::ETI_NI;
    AudioOutputFormat audio_format = AudioOutputFormat::WAV_16;
};

/**
 * @brief Stream statistics and status information
 */
struct StreamStatus {
    QString stream_id;                         // Stream identifier
    bool is_active = false;                   // Stream active status
    bool is_connected = false;                // Connection status
    bool is_recording = false;                // Recording status
    bool is_processing = false;               // Processing status
    
    std::chrono::system_clock::time_point start_time; // Stream start time
    std::chrono::system_clock::time_point last_frame_time; // Last frame received
    
    // Performance metrics
    std::atomic<size_t> frames_received{0};    // Total frames received
    std::atomic<size_t> frames_processed{0};   // Total frames processed
    std::atomic<size_t> frames_dropped{0};     // Total frames dropped
    std::atomic<size_t> bytes_received{0};     // Total bytes received
    std::atomic<size_t> processing_errors{0};  // Processing errors
    
    std::atomic<double> current_frame_rate{0.0}; // Current frame rate
    std::atomic<double> average_frame_rate{0.0}; // Average frame rate
    std::atomic<double> frame_loss_rate{0.0};   // Frame loss rate
    std::atomic<int64_t> average_latency_us{0}; // Average latency in microseconds
    std::atomic<int64_t> max_latency_us{0};     // Maximum latency
    
    // Quality metrics
    std::atomic<double> signal_quality{0.0};   // Signal quality (0.0-1.0)
    std::atomic<double> ensemble_confidence{0.0}; // Ensemble decode confidence
    std::atomic<size_t> services_detected{0};   // Number of services detected
    
    // Resource usage
    std::atomic<double> cpu_usage{0.0};        // CPU usage percentage
    std::atomic<size_t> memory_usage{0};       // Memory usage in bytes
    std::atomic<double> network_bandwidth{0.0}; // Network bandwidth usage
    
    // Error information
    QStringList recent_errors;                 // Recent error messages
    std::chrono::system_clock::time_point last_error_time; // Last error timestamp
    
    void reset() {
        frames_received = 0;
        frames_processed = 0;
        frames_dropped = 0;
        bytes_received = 0;
        processing_errors = 0;
        current_frame_rate = 0.0;
        average_frame_rate = 0.0;
        frame_loss_rate = 0.0;
        average_latency_us = 0;
        max_latency_us = 0;
        signal_quality = 0.0;
        ensemble_confidence = 0.0;
        services_detected = 0;
        cpu_usage = 0.0;
        memory_usage = 0;
        network_bandwidth = 0.0;
        recent_errors.clear();
        start_time = std::chrono::system_clock::now();
        last_frame_time = start_time;
    }
};

/**
 * @brief Stream processing task for thread pool execution
 */
class StreamProcessingTask : public QRunnable {
public:
    StreamProcessingTask(const QString& stream_id, const EtiFrame& frame,
                        const StreamConfig& config);
    
    void run() override;
    
    void set_result_callback(std::function<void(const QString&, bool, const QString&)> callback);

private:
    QString m_stream_id;
    EtiFrame m_frame;
    StreamConfig m_config;
    std::function<void(const QString&, bool, const QString&)> m_result_callback;
};

/**
 * @brief Stream worker for individual stream management
 */
class StreamWorker : public QObject {
    Q_OBJECT

public:
    explicit StreamWorker(const StreamConfig& config, QObject* parent = nullptr);
    ~StreamWorker();

public slots:
    void start_stream();
    void stop_stream();
    void pause_stream();
    void resume_stream();
    void update_config(const eti::StreamConfig& config);
    void handle_frame_received(const eti::EtiFrame& frame);
    void update_statistics();

signals:
    void stream_started(const QString& stream_id);
    void stream_stopped(const QString& stream_id);
    void stream_paused(const QString& stream_id);
    void stream_resumed(const QString& stream_id);
    void frame_processed(const QString& stream_id, const eti::EtiFrame& frame);
    void error_occurred(const QString& stream_id, const QString& error);
    void statistics_updated(const QString& stream_id, const eti::StreamStatus& status);
    void quality_threshold_violated(const QString& stream_id, const QString& violation);
    void failover_triggered(const QString& stream_id, const QString& backup_address);

private slots:
    void handle_receiver_frame(const eti::EtiFrame& frame);
    void handle_receiver_error(const QString& error);
    void handle_receiver_statistics(const eti::NetworkStreamStatistics& stats);
    void attempt_failover();

private:
    void initialize_components();
    void cleanup_components();
    void setup_receiver();
    void setup_recording();
    void setup_audio_extraction();
    void process_frame_internal(const EtiFrame& frame);
    void update_performance_metrics(const EtiFrame& frame);
    void check_quality_thresholds();
    bool validate_frame_quality(const EtiFrame& frame);
    
    StreamConfig m_config;
    StreamStatus m_status;
    
    std::unique_ptr<NetworkStreamReceiver> m_receiver;
    std::unique_ptr<StreamRecordingManager> m_recording_manager;
    std::unique_ptr<AudioExtractionPipeline> m_audio_pipeline;
    
    std::unique_ptr<QTimer> m_statistics_timer;
    std::unique_ptr<QTimer> m_failover_timer;
    
    QMutex m_status_mutex;
    QQueue<EtiFrame> m_frame_queue;
    QMutex m_queue_mutex;
    
    std::atomic<bool> m_active{false};
    std::atomic<bool> m_paused{false};
    std::atomic<bool> m_failover_in_progress{false};
    std::atomic<size_t> m_current_backup_index{0};
    
    // Performance tracking
    std::chrono::steady_clock::time_point m_last_frame_time;
    std::vector<double> m_frame_rate_history;
    std::vector<std::chrono::microseconds> m_latency_history;
    static constexpr size_t PERF_HISTORY_SIZE = 100;
};

/**
 * @brief Professional Multi-Stream Processor
 * 
 * High-performance, scalable processor for concurrent ETI streams with:
 * - Concurrent processing of up to 32 streams
 * - Load balancing across multiple CPU cores
 * - Automatic failover and redundancy
 * - Real-time performance monitoring
 * - Professional broadcast workflow integration
 * - Resource-aware stream management
 * - Quality threshold enforcement
 */
class MultiStreamProcessor : public QObject {
    Q_OBJECT

public:
    explicit MultiStreamProcessor(QObject* parent = nullptr);
    ~MultiStreamProcessor();

    // Stream management
    bool add_stream(const StreamConfig& config);
    bool remove_stream(const QString& stream_id);
    void remove_all_streams();
    bool update_stream_config(const QString& stream_id, const StreamConfig& config);
    
    // Stream control
    bool start_stream(const QString& stream_id);
    bool stop_stream(const QString& stream_id);
    bool pause_stream(const QString& stream_id);
    bool resume_stream(const QString& stream_id);
    void start_all_streams();
    void stop_all_streams();
    void pause_all_streams();
    void resume_all_streams();
    
    // Stream information
    QStringList get_active_streams() const;
    QStringList get_all_streams() const;
    StreamConfig get_stream_config(const QString& stream_id) const;
    StreamStatus get_stream_status(const QString& stream_id) const;
    QList<StreamStatus> get_all_stream_status() const;
    bool is_stream_active(const QString& stream_id) const;
    bool is_stream_connected(const QString& stream_id) const;
    
    // Performance monitoring
    size_t get_total_streams() const;
    size_t get_active_stream_count() const;
    double get_total_frame_rate() const;
    double get_average_frame_rate() const;
    size_t get_total_frames_processed() const;
    size_t get_total_frames_dropped() const;
    double get_overall_frame_loss_rate() const;
    
    // Resource management
    void set_max_concurrent_streams(size_t max_streams);
    size_t get_max_concurrent_streams() const;
    void set_thread_pool_size(size_t thread_count);
    size_t get_thread_pool_size() const;
    double get_cpu_usage() const;
    size_t get_memory_usage() const;
    
    // Quality management
    void set_global_quality_thresholds(double min_frame_rate, double max_frame_loss,
                                      std::chrono::milliseconds max_latency);
    void enable_automatic_quality_control(bool enabled);
    bool is_automatic_quality_control_enabled() const;
    
    // Load balancing
    void enable_load_balancing(bool enabled);
    bool is_load_balancing_enabled() const;
    void set_load_balancing_strategy(const QString& strategy); // "round_robin", "cpu_based", "priority"
    QString get_load_balancing_strategy() const;
    
    // Failover management
    void enable_global_failover(bool enabled);
    bool is_global_failover_enabled() const;
    void set_failover_timeout(std::chrono::milliseconds timeout);
    std::chrono::milliseconds get_failover_timeout() const;
    
    // Network interface management
    QStringList get_available_network_interfaces() const;
    void set_preferred_network_interface(const QString& interface_name);
    QString get_preferred_network_interface() const;
    void enable_interface_monitoring(bool enabled);
    
    // Stream discovery
    QStringList discover_multicast_streams() const;
    QStringList discover_multicast_streams_on_interface(const QString& interface_name) const;
    void start_stream_discovery();
    void stop_stream_discovery();
    
    // Batch operations
    bool start_streams_by_pattern(const QString& pattern);
    bool stop_streams_by_pattern(const QString& pattern);
    void apply_config_to_all_streams(const StreamConfig& template_config);
    
    // Statistics and reporting
    QJsonObject get_comprehensive_statistics() const;
    QStringList generate_status_report() const;
    void export_statistics_to_file(const QString& filename) const;
    void reset_all_statistics();
    
    // Error handling
    QStringList get_all_errors() const;
    QStringList get_stream_errors(const QString& stream_id) const;
    void clear_all_errors();
    void clear_stream_errors(const QString& stream_id);
    size_t get_total_error_count() const;
    
    // Advanced features
    void enable_stream_redundancy(const QString& primary_stream_id, 
                                 const QStringList& backup_stream_ids);
    void disable_stream_redundancy(const QString& primary_stream_id);
    void set_processing_priority(const QString& stream_id, uint32_t priority);
    
    // Callback registration
    using FrameCallback = std::function<void(const QString&, const EtiFrame&)>;
    using StatusCallback = std::function<void(const QString&, const StreamStatus&)>;
    using ErrorCallback = std::function<void(const QString&, const QString&)>;
    using QualityCallback = std::function<void(const QString&, const QString&)>;
    
    void set_frame_callback(FrameCallback callback);
    void set_status_callback(StatusCallback callback);
    void set_error_callback(ErrorCallback callback);
    void set_quality_callback(QualityCallback callback);

signals:
    void stream_added(const QString& stream_id);
    void stream_removed(const QString& stream_id);
    void stream_started(const QString& stream_id);
    void stream_stopped(const QString& stream_id);
    void stream_paused(const QString& stream_id);
    void stream_resumed(const QString& stream_id);
    void frame_processed(const QString& stream_id, const eti::EtiFrame& frame);
    void stream_error_occurred(const QString& stream_id, const QString& error);
    void stream_statistics_updated(const QString& stream_id, const eti::StreamStatus& status);
    void quality_threshold_violated(const QString& stream_id, const QString& violation);
    void failover_triggered(const QString& stream_id, const QString& backup_address);
    void load_balancing_adjusted();
    void resource_threshold_exceeded(const QString& resource, double usage);
    void stream_discovered(const QString& multicast_address, quint16 port);

private slots:
    void handle_worker_stream_started(const QString& stream_id);
    void handle_worker_stream_stopped(const QString& stream_id);
    void handle_worker_frame_processed(const QString& stream_id, const eti::EtiFrame& frame);
    void handle_worker_error(const QString& stream_id, const QString& error);
    void handle_worker_statistics(const QString& stream_id, const eti::StreamStatus& status);
    void handle_worker_quality_violation(const QString& stream_id, const QString& violation);
    void handle_worker_failover(const QString& stream_id, const QString& backup_address);
    void update_global_statistics();
    void check_resource_usage();
    void perform_load_balancing();

private:
    void initialize_thread_pool();
    void cleanup_thread_pool();
    void initialize_stream_worker(const QString& stream_id);
    void cleanup_stream_worker(const QString& stream_id);
    void validate_stream_config(const StreamConfig& config);
    QString generate_unique_stream_id(const QString& base_id) const;
    
    // Resource management
    void monitor_cpu_usage();
    void monitor_memory_usage();
    void adjust_stream_priorities();
    bool can_add_stream() const;
    
    // Load balancing
    QString select_optimal_thread_for_stream(const QString& stream_id) const;
    void redistribute_stream_load();
    
    // Stream management
    mutable QReadWriteLock m_streams_lock;
    QMap<QString, StreamConfig> m_stream_configs;
    QMap<QString, StreamStatus> m_stream_status;
    QMap<QString, std::unique_ptr<QThread>> m_worker_threads;
    QMap<QString, std::unique_ptr<StreamWorker>> m_workers;
    
    // Thread pool for frame processing
    std::unique_ptr<QThreadPool> m_thread_pool;
    std::atomic<size_t> m_thread_pool_size{4};
    
    // Configuration
    std::atomic<size_t> m_max_concurrent_streams{32};
    std::atomic<bool> m_load_balancing_enabled{true};
    std::atomic<bool> m_quality_control_enabled{true};
    std::atomic<bool> m_global_failover_enabled{true};
    QString m_load_balancing_strategy = "cpu_based";
    QString m_preferred_interface;
    
    // Global quality thresholds
    std::atomic<double> m_global_min_frame_rate{200.0};
    std::atomic<double> m_global_max_frame_loss{0.01};
    std::atomic<int64_t> m_global_max_latency_us{100000}; // 100ms
    
    // Global statistics
    std::atomic<size_t> m_total_frames_processed{0};
    std::atomic<size_t> m_total_frames_dropped{0};
    std::atomic<double> m_total_frame_rate{0.0};
    std::atomic<double> m_cpu_usage{0.0};
    std::atomic<size_t> m_memory_usage{0};
    
    // Timers
    std::unique_ptr<QTimer> m_statistics_timer;
    std::unique_ptr<QTimer> m_resource_monitor_timer;
    std::unique_ptr<QTimer> m_load_balancing_timer;
    
    // Error tracking
    QMap<QString, QStringList> m_stream_errors;
    QMutex m_errors_mutex;
    
    // Redundancy management
    QMap<QString, QStringList> m_redundancy_groups; // primary -> backups
    QMutex m_redundancy_mutex;
    
    // Callbacks
    FrameCallback m_frame_callback;
    StatusCallback m_status_callback;
    ErrorCallback m_error_callback;
    QualityCallback m_quality_callback;
    QMutex m_callbacks_mutex;
    
    static constexpr size_t MAX_CONCURRENT_STREAMS = 64;
    static constexpr size_t MAX_ERROR_HISTORY = 100;
    static constexpr size_t DEFAULT_THREAD_POOL_SIZE = 8;
};

/**
 * @brief Factory for creating pre-configured multi-stream processors
 */
class MultiStreamProcessorFactory {
public:
    /**
     * @brief Create processor for broadcast monitoring with multiple streams
     */
    static std::unique_ptr<MultiStreamProcessor> create_monitoring_processor(
        size_t max_streams = 16);

    /**
     * @brief Create processor for high-availability broadcasting with redundancy
     */
    static std::unique_ptr<MultiStreamProcessor> create_redundant_processor(
        size_t max_streams = 32);

    /**
     * @brief Create processor for stream analysis and quality assessment
     */
    static std::unique_ptr<MultiStreamProcessor> create_analysis_processor(
        size_t max_streams = 8);

    /**
     * @brief Create processor for stream relay and distribution
     */
    static std::unique_ptr<MultiStreamProcessor> create_relay_processor(
        size_t max_streams = 64);
};

/**
 * @brief Utility functions for multi-stream operations
 */
namespace MultiStreamUtils {
    
    /**
     * @brief Stream discovery utilities
     */
    QStringList scan_multicast_range(const QString& base_address, quint16 start_port, 
                                    quint16 end_port, std::chrono::milliseconds timeout = std::chrono::milliseconds{1000});
    
    /**
     * @brief Network interface utilities
     */
    QStringList get_multicast_capable_interfaces();
    bool is_interface_suitable_for_streaming(const QString& interface_name);
    double get_interface_bandwidth_usage(const QString& interface_name);
    
    /**
     * @brief Performance analysis
     */
    struct StreamPerformanceReport {
        QString stream_id;
        double average_frame_rate;
        double frame_loss_rate;
        std::chrono::microseconds average_latency;
        size_t total_frames;
        size_t error_count;
        double uptime_percentage;
        QDateTime report_time;
    };
    
    StreamPerformanceReport generate_performance_report(const StreamStatus& status);
    QList<StreamPerformanceReport> compare_stream_performance(const QList<StreamStatus>& streams);
    
    /**
     * @brief Configuration utilities
     */
    StreamConfig create_config_from_url(const QString& url);
    QList<StreamConfig> load_configs_from_file(const QString& filename);
    bool save_configs_to_file(const QList<StreamConfig>& configs, const QString& filename);
    
    /**
     * @brief Load balancing utilities
     */
    QMap<QString, double> calculate_stream_load_distribution(const QList<StreamStatus>& streams);
    QStringList suggest_load_balancing_adjustments(const QMap<QString, double>& load_distribution);
}

} // namespace eti

#endif // MULTI_STREAM_PROCESSOR_H