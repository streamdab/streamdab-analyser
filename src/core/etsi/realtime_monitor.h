/**
 * @file realtime_monitor.h
 * @brief Real-time ETSI Standards Compliance Monitoring Framework
 * 
 * High-performance real-time monitoring framework for ETSI standards compliance
 * with <100ms latency, live violation detection, multi-stream processing,
 * and professional broadcast monitoring capabilities.
 * 
 * Features:
 * - Real-time ETSI compliance validation with <100ms latency
 * - Multi-stream concurrent monitoring and processing
 * - Live violation detection with immediate alerting
 * - Performance monitoring with resource optimization
 * - Professional quality assurance for broadcast operations
 * - Cross-platform high-performance implementation
 * - Memory-efficient ring buffer management
 * - Thread-safe concurrent processing architecture
 * 
 * Performance Targets:
 * - Processing Latency: <100ms per ETI frame
 * - Throughput: >1000 frames/second sustained
 * - Memory Usage: <512MB for 8 concurrent streams
 * - CPU Usage: <30% on modern multi-core systems
 * - Reliability: 99.99% uptime for 24/7 operations
 * 
 * @author Standards Compliance Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 * @version 1.0
 */

#ifndef ETSI_REALTIME_MONITOR_H
#define ETSI_REALTIME_MONITOR_H

#include "compliance_engine.h"
#include "alert_system.h"
#include "compliance_logger.h"
#include "broadcast_standards.h"
#include "../eti_types.hpp"
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <array>

namespace etsi {
namespace realtime {

/**
 * @brief Real-time monitoring mode configurations
 */
enum class MonitoringMode : uint8_t {
    LIVE_STREAM = 1,        // Live ETI stream monitoring
    FILE_PLAYBACK = 2,      // File-based playback monitoring
    NETWORK_CAPTURE = 3,    // Network ETI capture monitoring
    MULTI_STREAM = 4,       // Multiple concurrent streams
    PERFORMANCE_TEST = 5,   // Performance benchmarking mode
    DEBUG_MODE = 6          // Debug monitoring with detailed logging
};

/**
 * @brief Stream source configuration
 */
struct StreamSource {
    uint32_t stream_id;
    std::string source_name;
    std::string source_url;         // Network URL or file path
    std::string source_type;        // "udp", "tcp", "file", "pipe"
    uint16_t port = 0;             // Network port if applicable
    bool enable_monitoring = true;
    bool enable_recording = false;
    std::string recording_path;
    
    // Performance settings
    size_t buffer_size = 8192;      // Buffer size in frames
    std::chrono::milliseconds timeout{5000}; // Connection timeout
    uint32_t max_retries = 3;       // Connection retry attempts
    
    StreamSource(uint32_t id, const std::string& name, const std::string& url)
        : stream_id(id), source_name(name), source_url(url) {}
};

/**
 * @brief Real-time performance metrics
 */
struct RealtimePerformanceMetrics {
    // Processing performance
    std::chrono::microseconds avg_processing_latency{0};
    std::chrono::microseconds max_processing_latency{0};
    std::chrono::microseconds min_processing_latency{std::chrono::microseconds::max()};
    
    // Throughput metrics
    uint32_t frames_processed_per_second = 0;
    uint32_t total_frames_processed = 0;
    uint32_t frames_dropped = 0;
    uint32_t frames_with_errors = 0;
    
    // Resource usage
    double cpu_usage_percentage = 0.0;
    double memory_usage_mb = 0.0;
    uint32_t active_threads = 0;
    uint32_t queue_depth = 0;
    
    // Compliance metrics
    uint32_t compliance_violations_detected = 0;
    uint32_t alerts_generated = 0;
    double overall_compliance_rate = 0.0;
    
    // Network metrics (if applicable)
    uint64_t bytes_received = 0;
    uint32_t packet_loss_count = 0;
    double jitter_milliseconds = 0.0;
    
    // Timing metrics
    std::chrono::system_clock::time_point measurement_start;
    std::chrono::system_clock::time_point last_update;
    std::chrono::seconds uptime{0};
    
    RealtimePerformanceMetrics() {
        measurement_start = std::chrono::system_clock::now();
        last_update = measurement_start;
    }
    
    void update_processing_latency(std::chrono::microseconds latency) {
        if (latency > max_processing_latency) max_processing_latency = latency;
        if (latency < min_processing_latency) min_processing_latency = latency;
        
        // Update running average
        static uint32_t sample_count = 0;
        static uint64_t total_latency = 0;
        
        total_latency += latency.count();
        sample_count++;
        avg_processing_latency = std::chrono::microseconds{total_latency / sample_count};
    }
    
    void reset() {
        *this = RealtimePerformanceMetrics{};
    }
};

/**
 * @brief Ring buffer for high-performance ETI frame processing
 */
template<typename T, size_t Size>
class RingBuffer {
public:
    RingBuffer() : head_(0), tail_(0), size_(0) {}
    
    bool push(const T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (size_ == Size) {
            return false; // Buffer full
        }
        
        buffer_[head_] = item;
        head_ = (head_ + 1) % Size;
        size_++;
        return true;
    }
    
    bool pop(T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (size_ == 0) {
            return false; // Buffer empty
        }
        
        item = buffer_[tail_];
        tail_ = (tail_ + 1) % Size;
        size_--;
        return true;
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_;
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_ == 0;
    }
    
    bool full() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_ == Size;
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        head_ = tail_ = size_ = 0;
    }

private:
    std::array<T, Size> buffer_;
    size_t head_;
    size_t tail_;
    size_t size_;
    mutable std::mutex mutex_;
};

/**
 * @brief ETI frame with processing metadata
 */
struct ProcessingFrame {
    EtiFrame frame;
    uint32_t stream_id;
    std::chrono::high_resolution_clock::time_point arrival_time;
    std::chrono::high_resolution_clock::time_point processing_start;
    uint32_t sequence_number;
    bool requires_priority_processing = false;
    
    ProcessingFrame() = default;
    ProcessingFrame(const EtiFrame& f, uint32_t sid) 
        : frame(f), stream_id(sid), arrival_time(std::chrono::high_resolution_clock::now()),
          sequence_number(0) {}
};

/**
 * @brief Stream monitoring state
 */
struct StreamMonitoringState {
    uint32_t stream_id;
    std::atomic<bool> active{false};
    std::atomic<bool> connected{false};
    std::atomic<uint32_t> frames_processed{0};
    std::atomic<uint32_t> errors_detected{0};
    
    std::chrono::system_clock::time_point last_frame_time;
    std::chrono::system_clock::time_point connection_start_time;
    RealtimePerformanceMetrics performance_metrics;
    
    // Stream-specific compliance tracking
    compliance::ComplianceResult latest_compliance_result;
    std::atomic<double> current_compliance_rate{0.0};
    
    mutable std::mutex state_mutex;
    
    StreamMonitoringState(uint32_t id) 
        : stream_id(id), last_frame_time(std::chrono::system_clock::now()),
          connection_start_time(std::chrono::system_clock::now()) {}
};

// Forward declarations
class StreamProcessor;
class NetworkReceiver;
class FileReader;
class PerformanceMonitor;

/**
 * @brief Real-time ETSI Standards Compliance Monitor
 * 
 * High-performance real-time monitoring system for ETSI compliance validation
 * with multi-stream support, <100ms latency, and professional broadcast
 * monitoring capabilities.
 */
class EtsiRealtimeMonitor {
public:
    /**
     * @brief Monitor configuration
     */
    struct Config {
        MonitoringMode mode = MonitoringMode::LIVE_STREAM;
        
        // Performance settings
        uint32_t max_concurrent_streams = 8;
        uint32_t processing_threads = 4;
        size_t frame_buffer_size = 8192;        // Frames per stream
        std::chrono::microseconds max_processing_latency{100000}; // 100ms
        
        // Quality settings
        compliance::ComplianceLevel compliance_level = compliance::ComplianceLevel::BROADCAST_QUALITY;
        bool enable_audio_monitoring = true;
        bool enable_performance_monitoring = true;
        bool enable_detailed_logging = false;
        
        // Alert settings
        bool enable_real_time_alerts = true;
        alerts::AlertSeverity min_alert_severity = alerts::AlertSeverity::WARNING;
        std::chrono::milliseconds alert_debounce_time{1000}; // 1 second
        
        // Resource limits
        double max_cpu_usage = 80.0;           // Percentage
        double max_memory_usage_mb = 512.0;    // MB
        uint32_t max_processing_queue_depth = 1000;
        
        // Network settings (for network streams)
        std::chrono::seconds network_timeout{30};
        uint32_t network_retry_attempts = 3;
        size_t network_buffer_size = 65536;   // Bytes
        
        Config() = default;
    };
    
    explicit EtsiRealtimeMonitor(const Config& config = Config{});
    ~EtsiRealtimeMonitor();
    
    // Disable copy/move for thread safety
    EtsiRealtimeMonitor(const EtsiRealtimeMonitor&) = delete;
    EtsiRealtimeMonitor& operator=(const EtsiRealtimeMonitor&) = delete;
    EtsiRealtimeMonitor(EtsiRealtimeMonitor&&) = delete;
    EtsiRealtimeMonitor& operator=(EtsiRealtimeMonitor&&) = delete;
    
    /**
     * @brief Initialize the real-time monitor
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Shutdown the real-time monitor
     */
    void shutdown();
    
    /**
     * @brief Add stream source for monitoring
     * @param source Stream source configuration
     * @return true if stream added successfully
     */
    bool add_stream_source(const StreamSource& source);
    
    /**
     * @brief Remove stream source from monitoring
     * @param stream_id Stream ID to remove
     * @return true if stream removed successfully
     */
    bool remove_stream_source(uint32_t stream_id);
    
    /**
     * @brief Start monitoring all configured streams
     * @return true if monitoring started successfully
     */
    bool start_monitoring();
    
    /**
     * @brief Stop monitoring all streams
     * @return true if monitoring stopped successfully
     */
    bool stop_monitoring();
    
    /**
     * @brief Start monitoring specific stream
     * @param stream_id Stream ID to start monitoring
     * @return true if stream monitoring started
     */
    bool start_stream_monitoring(uint32_t stream_id);
    
    /**
     * @brief Stop monitoring specific stream
     * @param stream_id Stream ID to stop monitoring
     * @return true if stream monitoring stopped
     */
    bool stop_stream_monitoring(uint32_t stream_id);
    
    /**
     * @brief Process single ETI frame (for external frame sources)
     * @param frame ETI frame to process
     * @param stream_id Associated stream ID
     * @return Processing result
     */
    compliance::ComplianceResult process_frame(const EtiFrame& frame, uint32_t stream_id);
    
    /**
     * @brief Get current monitoring status
     * @return true if actively monitoring
     */
    bool is_monitoring() const { return monitoring_active_.load(); }
    
    /**
     * @brief Get active stream count
     * @return Number of active streams
     */
    uint32_t get_active_stream_count() const;
    
    /**
     * @brief Get stream monitoring state
     * @param stream_id Stream ID to query
     * @return Stream monitoring state (nullptr if not found)
     */
    std::shared_ptr<StreamMonitoringState> get_stream_state(uint32_t stream_id) const;
    
    /**
     * @brief Get all stream states
     * @return Vector of all stream monitoring states
     */
    std::vector<std::shared_ptr<StreamMonitoringState>> get_all_stream_states() const;
    
    /**
     * @brief Get current performance metrics
     * @return Real-time performance metrics
     */
    RealtimePerformanceMetrics get_performance_metrics() const;
    
    /**
     * @brief Get stream-specific performance metrics
     * @param stream_id Stream ID to query
     * @return Stream performance metrics
     */
    RealtimePerformanceMetrics get_stream_performance_metrics(uint32_t stream_id) const;
    
    /**
     * @brief Reset performance metrics
     */
    void reset_performance_metrics();
    
    /**
     * @brief Update monitoring configuration
     * @param config New configuration
     */
    void update_configuration(const Config& config);
    
    /**
     * @brief Set compliance engine for validation
     * @param engine Compliance engine instance
     */
    void set_compliance_engine(std::shared_ptr<compliance::EtsiComplianceEngine> engine);
    
    /**
     * @brief Set alert system for notifications
     * @param alert_system Alert system instance
     */
    void set_alert_system(std::shared_ptr<alerts::EtsiAlertSystem> alert_system);
    
    /**
     * @brief Set logger for audit trail
     * @param logger Compliance logger instance
     */
    void set_logger(std::shared_ptr<logging::EtsiComplianceLogger> logger);
    
    /**
     * @brief Set broadcast standards framework
     * @param framework Broadcast standards framework instance
     */
    void set_broadcast_framework(std::shared_ptr<broadcast::BroadcastStandardsFramework> framework);
    
    /**
     * @brief Register frame processing callback
     * @param callback Function to call on frame processing completion
     */
    using FrameProcessingCallback = std::function<void(const compliance::ComplianceResult&, uint32_t)>;
    void register_frame_callback(FrameProcessingCallback callback);
    
    /**
     * @brief Register performance monitoring callback
     * @param callback Function to call on performance updates
     */
    using PerformanceCallback = std::function<void(const RealtimePerformanceMetrics&)>;
    void register_performance_callback(PerformanceCallback callback);
    
    /**
     * @brief Register stream status callback
     * @param callback Function to call on stream status changes
     */
    using StreamStatusCallback = std::function<void(uint32_t, bool)>; // stream_id, connected
    void register_stream_status_callback(StreamStatusCallback callback);
    
    /**
     * @brief Unregister all callbacks
     */
    void unregister_callbacks();
    
    /**
     * @brief Get monitor statistics
     */
    struct MonitorStatistics {
        std::chrono::system_clock::time_point start_time;
        std::chrono::seconds uptime{0};
        uint64_t total_frames_processed = 0;
        uint64_t total_violations_detected = 0;
        uint64_t total_alerts_generated = 0;
        uint32_t max_concurrent_streams_processed = 0;
        std::chrono::microseconds best_processing_time{std::chrono::microseconds::max()};
        std::chrono::microseconds worst_processing_time{0};
        double average_compliance_rate = 0.0;
    };
    
    MonitorStatistics get_statistics() const;
    
    /**
     * @brief Reset monitor statistics
     */
    void reset_statistics();
    
    /**
     * @brief Enable/disable debug mode
     * @param enabled true to enable debug mode
     */
    void set_debug_mode(bool enabled);
    
    /**
     * @brief Get current monitoring mode
     * @return Current monitoring mode
     */
    MonitoringMode get_monitoring_mode() const { return config_.mode; }

private:
    Config config_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> monitoring_active_{false};
    std::atomic<bool> shutdown_requested_{false};
    
    // Core components
    std::shared_ptr<compliance::EtsiComplianceEngine> compliance_engine_;
    std::shared_ptr<alerts::EtsiAlertSystem> alert_system_;
    std::shared_ptr<logging::EtsiComplianceLogger> logger_;
    std::shared_ptr<broadcast::BroadcastStandardsFramework> broadcast_framework_;
    
    // Stream management
    std::map<uint32_t, StreamSource> stream_sources_;
    std::map<uint32_t, std::shared_ptr<StreamMonitoringState>> stream_states_;
    mutable std::mutex streams_mutex_;
    
    // Processing infrastructure
    std::vector<std::unique_ptr<std::thread>> processing_threads_;
    std::unique_ptr<std::thread> performance_monitor_thread_;
    
    // High-performance frame processing
    RingBuffer<ProcessingFrame, 8192> frame_queue_;
    std::condition_variable processing_condition_;
    std::mutex processing_mutex_;
    
    // Performance tracking
    mutable std::mutex metrics_mutex_;
    RealtimePerformanceMetrics global_metrics_;
    MonitorStatistics statistics_;
    
    // Callback handling
    std::mutex callback_mutex_;
    FrameProcessingCallback frame_callback_;
    PerformanceCallback performance_callback_;
    StreamStatusCallback stream_status_callback_;
    
    // Stream processing components
    std::map<uint32_t, std::unique_ptr<StreamProcessor>> stream_processors_;
    std::map<uint32_t, std::unique_ptr<NetworkReceiver>> network_receivers_;
    std::map<uint32_t, std::unique_ptr<FileReader>> file_readers_;
    
    // Internal methods
    void initialize_processing_threads();
    void processing_thread_loop();
    void performance_monitoring_loop();
    void stream_monitoring_loop(uint32_t stream_id);
    
    bool create_stream_processor(const StreamSource& source);
    bool destroy_stream_processor(uint32_t stream_id);
    
    void process_frame_internal(const ProcessingFrame& processing_frame);
    void update_stream_performance(uint32_t stream_id, std::chrono::microseconds processing_time);
    void update_global_performance();
    
    void notify_frame_processed(const compliance::ComplianceResult& result, uint32_t stream_id);
    void notify_performance_update(const RealtimePerformanceMetrics& metrics);
    void notify_stream_status_change(uint32_t stream_id, bool connected);
    
    // Utility methods
    bool validate_configuration(const Config& config) const;
    void log_performance_warning(const std::string& warning);
    void handle_processing_error(uint32_t stream_id, const std::string& error);
    
    static std::string get_monitoring_mode_name(MonitoringMode mode);
};

/**
 * @brief Stream processor for individual ETI streams
 */
class StreamProcessor {
public:
    explicit StreamProcessor(uint32_t stream_id, const StreamSource& source);
    ~StreamProcessor();
    
    bool initialize();
    void shutdown();
    
    bool start_processing();
    bool stop_processing();
    
    bool is_processing() const { return processing_active_.load(); }
    
    void process_frame(const EtiFrame& frame);
    
    std::shared_ptr<StreamMonitoringState> get_state() const { return state_; }
    
private:
    uint32_t stream_id_;
    StreamSource source_;
    std::shared_ptr<StreamMonitoringState> state_;
    std::atomic<bool> processing_active_{false};
    
    std::unique_ptr<std::thread> processing_thread_;
    
    void processing_loop();
};

/**
 * @brief Utility functions for real-time monitoring
 */
namespace utils {
    /**
     * @brief Calculate processing latency statistics
     * @param latencies Vector of processing latencies
     * @return Statistics (min, max, average, percentiles)
     */
    struct LatencyStats {
        std::chrono::microseconds min;
        std::chrono::microseconds max;
        std::chrono::microseconds average;
        std::chrono::microseconds p50;
        std::chrono::microseconds p95;
        std::chrono::microseconds p99;
    };
    LatencyStats calculate_latency_statistics(const std::vector<std::chrono::microseconds>& latencies);
    
    /**
     * @brief Format performance metrics for display
     * @param metrics Performance metrics to format
     * @return Human-readable metrics string
     */
    std::string format_performance_metrics(const RealtimePerformanceMetrics& metrics);
    
    /**
     * @brief Check if performance meets real-time requirements
     * @param metrics Performance metrics to check
     * @return true if meets real-time requirements
     */
    bool meets_realtime_requirements(const RealtimePerformanceMetrics& metrics);
    
    /**
     * @brief Optimize processing configuration for performance
     * @param current_config Current configuration
     * @param performance_metrics Current performance metrics
     * @return Optimized configuration
     */
    EtsiRealtimeMonitor::Config optimize_configuration(
        const EtsiRealtimeMonitor::Config& current_config,
        const RealtimePerformanceMetrics& performance_metrics
    );
    
    /**
     * @brief Generate real-time monitoring dashboard data
     * @param monitor Real-time monitor instance
     * @return JSON dashboard data
     */
    std::string generate_dashboard_data(const EtsiRealtimeMonitor& monitor);
    
    /**
     * @brief Calculate system resource usage
     * @return Resource usage metrics
     */
    struct ResourceUsage {
        double cpu_percentage;
        double memory_mb;
        double network_bandwidth_mbps;
        uint32_t thread_count;
    };
    ResourceUsage get_system_resource_usage();
}

} // namespace realtime
} // namespace etsi

#endif // ETSI_REALTIME_MONITOR_H