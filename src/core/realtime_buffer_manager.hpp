/**
 * @file realtime_buffer_manager.h
 * @brief Professional Real-Time Buffer Manager with ring buffer optimization
 * 
 * Implements high-performance, lock-free ring buffer management for real-time
 * ETI stream processing with minimal latency, overflow/underflow detection,
 * and professional broadcast-grade reliability.
 * 
 * @author Network/Stream Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 */

#ifndef REALTIME_BUFFER_MANAGER_H
#define REALTIME_BUFFER_MANAGER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include <QAtomicInt>
#include <QAtomicPointer>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <vector>
#include <array>
#include <cstring>

#include "eti_types.hpp"

namespace eti {

/**
 * @brief Buffer management strategy enumeration
 */
enum class BufferStrategy {
    RING_BUFFER,            // Lock-free ring buffer (default)
    DOUBLE_BUFFER,          // Double buffering for producer/consumer
    TRIPLE_BUFFER,          // Triple buffering for low latency
    ADAPTIVE_BUFFER,        // Adaptive buffer size based on load
    PRIORITY_BUFFER,        // Priority-based buffer management
    STREAMING_BUFFER        // Optimized for continuous streaming
};

/**
 * @brief Buffer overflow handling strategy
 */
enum class OverflowStrategy {
    DROP_OLDEST,            // Drop oldest frames when buffer full
    DROP_NEWEST,            // Drop newest frames when buffer full
    EXPAND_BUFFER,          // Dynamically expand buffer size
    BLOCK_PRODUCER,         // Block producer until space available
    CALLBACK_NOTIFY,        // Notify via callback and continue
    EMERGENCY_FLUSH         // Emergency flush to recording/disk
};

/**
 * @brief Buffer underflow handling strategy
 */
enum class UnderflowStrategy {
    WAIT_FOR_DATA,          // Wait for data to become available
    RETURN_INVALID,         // Return invalid frame indicator
    REPEAT_LAST_FRAME,      // Repeat the last valid frame
    INTERPOLATE_FRAME,      // Interpolate between frames
    CALLBACK_NOTIFY,        // Notify via callback
    EMERGENCY_FILL          // Fill with emergency backup data
};

/**
 * @brief Real-time buffer configuration
 */
struct BufferConfig {
    BufferStrategy strategy = BufferStrategy::RING_BUFFER;
    OverflowStrategy overflow_strategy = OverflowStrategy::DROP_OLDEST;
    UnderflowStrategy underflow_strategy = UnderflowStrategy::WAIT_FOR_DATA;
    
    size_t initial_capacity = 1000;        // Initial buffer capacity in frames
    size_t max_capacity = 10000;           // Maximum buffer capacity
    size_t min_capacity = 100;             // Minimum buffer capacity
    
    bool auto_resize = true;               // Enable automatic buffer resizing
    double resize_threshold_high = 0.85;   // Resize up when 85% full
    double resize_threshold_low = 0.25;    // Resize down when 25% full
    double resize_factor = 1.5;            // Resize factor for expansion
    
    // Real-time constraints
    std::chrono::microseconds max_latency{100000}; // 100ms maximum latency
    std::chrono::microseconds target_latency{50000}; // 50ms target latency
    bool strict_timing = true;             // Enforce strict timing constraints
    
    // Performance tuning
    size_t batch_size = 10;                // Batch processing size
    bool enable_prefetch = true;           // Enable cache prefetching
    bool enable_memory_alignment = true;   // Enable memory alignment optimization
    size_t alignment_bytes = 64;           // Cache line alignment (64 bytes)
    
    // Monitoring and debugging
    bool enable_statistics = true;         // Enable performance statistics
    bool enable_overflow_detection = true; // Enable overflow detection
    bool enable_underflow_detection = true; // Enable underflow detection
    std::chrono::milliseconds stats_interval{1000}; // Statistics update interval
};

/**
 * @brief Buffer status and performance metrics
 */
struct BufferStatus {
    size_t capacity = 0;                   // Current buffer capacity
    size_t used_slots = 0;                 // Currently used buffer slots
    size_t free_slots = 0;                 // Available buffer slots
    double utilization = 0.0;              // Buffer utilization percentage
    
    // Performance metrics
    std::atomic<size_t> frames_written{0}; // Total frames written
    std::atomic<size_t> frames_read{0};    // Total frames read
    std::atomic<size_t> frames_dropped{0}; // Frames dropped due to overflow
    std::atomic<size_t> underrun_events{0}; // Buffer underrun events
    std::atomic<size_t> overrun_events{0}; // Buffer overrun events
    
    // Timing metrics
    std::atomic<int64_t> write_latency_us{0}; // Average write latency
    std::atomic<int64_t> read_latency_us{0};  // Average read latency
    std::atomic<int64_t> max_write_latency_us{0}; // Maximum write latency
    std::atomic<int64_t> max_read_latency_us{0};  // Maximum read latency
    
    // Real-time performance
    std::atomic<double> throughput_mbps{0.0}; // Throughput in MB/s
    std::atomic<double> frame_rate{0.0};      // Current frame rate
    std::atomic<bool> real_time_guarantee{true}; // Real-time guarantee status
    
    // Quality metrics
    std::atomic<double> jitter_us{0.0};       // Timing jitter in microseconds
    std::atomic<double> packet_loss_rate{0.0}; // Packet loss rate
    std::atomic<size_t> consecutive_drops{0};  // Consecutive frame drops
    
    std::chrono::steady_clock::time_point last_update; // Last statistics update
    
    void reset() {
        frames_written = 0;
        frames_read = 0;
        frames_dropped = 0;
        underrun_events = 0;
        overrun_events = 0;
        write_latency_us = 0;
        read_latency_us = 0;
        max_write_latency_us = 0;
        max_read_latency_us = 0;
        throughput_mbps = 0.0;
        frame_rate = 0.0;
        real_time_guarantee = true;
        jitter_us = 0.0;
        packet_loss_rate = 0.0;
        consecutive_drops = 0;
        last_update = std::chrono::steady_clock::now();
    }
};

/**
 * @brief Lock-free ring buffer slot for ETI frames
 */
struct alignas(64) BufferSlot {
    std::atomic<bool> occupied{false};     // Slot occupation flag
    std::atomic<bool> valid{false};        // Data validity flag
    std::atomic<uint32_t> sequence{0};     // Sequence number for ordering
    std::chrono::steady_clock::time_point timestamp; // Frame timestamp
    std::unique_ptr<EtiFrame> frame;       // ETI frame data (heap allocated)
    
    // Padding to cache line boundary
    static constexpr size_t required_size = sizeof(std::atomic<bool>) * 2 + sizeof(std::atomic<uint32_t>) + 
                                           sizeof(std::chrono::steady_clock::time_point) + sizeof(std::unique_ptr<EtiFrame>);
    static constexpr size_t padding_size = (required_size <= 64) ? (64 - required_size) : 0;
    char padding[padding_size > 0 ? padding_size : 1];  // Ensure at least 1 byte
};

/**
 * @brief High-performance lock-free ring buffer for ETI frames
 */
class LockFreeRingBuffer {
public:
    explicit LockFreeRingBuffer(size_t capacity);
    ~LockFreeRingBuffer();

    // Core operations
    bool try_write(const EtiFrame& frame);
    bool try_read(EtiFrame& frame);
    bool try_write_batch(const std::vector<EtiFrame>& frames, size_t& written_count);
    bool try_read_batch(std::vector<EtiFrame>& frames, size_t max_count);
    
    // Non-blocking operations
    bool write_with_timeout(const EtiFrame& frame, std::chrono::microseconds timeout);
    bool read_with_timeout(EtiFrame& frame, std::chrono::microseconds timeout);
    
    // Status and metrics
    size_t capacity() const { return m_capacity; }
    size_t size() const;
    size_t available() const { return capacity() - size(); }
    bool empty() const { return size() == 0; }
    bool full() const { return size() >= capacity(); }
    double utilization() const { return static_cast<double>(size()) / capacity(); }
    
    // Advanced operations
    bool peek(EtiFrame& frame) const;  // Peek without consuming
    bool skip(size_t count = 1);       // Skip frames
    void clear();                      // Clear all frames
    void reset();                      // Reset buffer to initial state
    
    // Resize operations (not lock-free, use with care)
    bool resize(size_t new_capacity);
    
    // Statistics
    size_t get_write_count() const { return m_write_count.load(); }
    size_t get_read_count() const { return m_read_count.load(); }
    size_t get_drop_count() const { return m_drop_count.load(); }

private:
    void advance_write_index();
    void advance_read_index();
    bool is_valid_index(size_t index) const;
    
    size_t m_capacity;
    std::unique_ptr<BufferSlot[]> m_buffer;
    
    // Lock-free indices
    std::atomic<size_t> m_write_index{0};
    std::atomic<size_t> m_read_index{0};
    std::atomic<size_t> m_size{0};
    
    // Statistics
    std::atomic<size_t> m_write_count{0};
    std::atomic<size_t> m_read_count{0};
    std::atomic<size_t> m_drop_count{0};
    std::atomic<uint32_t> m_sequence{0};
    
    // Cache line padding
    alignas(64) char m_padding[64];
};

/**
 * @brief Adaptive buffer that adjusts size based on load
 */
class AdaptiveBuffer {
public:
    explicit AdaptiveBuffer(const BufferConfig& config);
    ~AdaptiveBuffer();

    // Core operations
    bool write_frame(const EtiFrame& frame);
    bool read_frame(EtiFrame& frame);
    bool write_batch(const std::vector<EtiFrame>& frames);
    bool read_batch(std::vector<EtiFrame>& frames, size_t max_count);
    
    // Configuration
    void update_config(const BufferConfig& config);
    BufferConfig get_config() const;
    
    // Status
    BufferStatus get_status() const;
    bool is_healthy() const;
    
    // Adaptive behavior
    void analyze_performance();
    void adjust_buffer_size();
    void optimize_for_latency();
    void optimize_for_throughput();

private:
    BufferConfig m_config;
    std::unique_ptr<LockFreeRingBuffer> m_primary_buffer;
    std::unique_ptr<LockFreeRingBuffer> m_overflow_buffer;
    
    mutable QMutex m_config_mutex;
    BufferStatus m_status;
    
    // Performance tracking
    std::vector<double> m_latency_history;
    std::vector<double> m_throughput_history;
    std::vector<double> m_utilization_history;
    
    std::chrono::steady_clock::time_point m_last_resize;
    std::chrono::steady_clock::time_point m_last_analysis;
    
    static constexpr size_t HISTORY_SIZE = 100;
};

/**
 * @brief Professional Real-Time Buffer Manager
 * 
 * High-performance buffer management system for ETI streams with:
 * - Lock-free ring buffer implementation for minimal latency
 * - Adaptive buffer sizing based on stream characteristics
 * - Professional overflow/underflow handling strategies
 * - Real-time performance monitoring and guarantees
 * - Cache-optimized memory layout for maximum throughput
 * - Broadcast-grade reliability and error recovery
 */
class RealTimeBufferManager : public QObject {
    Q_OBJECT

public:
    explicit RealTimeBufferManager(QObject* parent = nullptr);
    explicit RealTimeBufferManager(const BufferConfig& config, QObject* parent = nullptr);
    ~RealTimeBufferManager();

    // Configuration management
    void set_config(const BufferConfig& config);
    BufferConfig get_config() const;
    void update_buffer_strategy(BufferStrategy strategy);
    void update_overflow_strategy(OverflowStrategy strategy);
    void update_underflow_strategy(UnderflowStrategy strategy);
    
    // Buffer operations
    bool write_frame(const EtiFrame& frame);
    bool read_frame(EtiFrame& frame);
    bool write_frame_batch(const std::vector<EtiFrame>& frames);
    bool read_frame_batch(std::vector<EtiFrame>& frames, size_t max_count = 10);
    
    // Non-blocking operations with timeout
    bool write_frame_timeout(const EtiFrame& frame, std::chrono::microseconds timeout);
    bool read_frame_timeout(EtiFrame& frame, std::chrono::microseconds timeout);
    
    // Priority operations (for priority-based strategy)
    bool write_priority_frame(const EtiFrame& frame, uint32_t priority);
    bool read_priority_frame(EtiFrame& frame, uint32_t min_priority);
    
    // Buffer status and monitoring
    BufferStatus get_buffer_status() const;
    bool is_buffer_healthy() const;
    double get_buffer_utilization() const;
    size_t get_buffer_capacity() const;
    size_t get_buffer_size() const;
    size_t get_available_space() const;
    
    // Real-time performance
    std::chrono::microseconds get_current_latency() const;
    std::chrono::microseconds get_average_latency() const;
    std::chrono::microseconds get_maximum_latency() const;
    double get_current_frame_rate() const;
    double get_average_frame_rate() const;
    double get_throughput_mbps() const;
    bool is_real_time_guarantee_met() const;
    
    // Quality metrics
    double get_jitter() const;
    double get_packet_loss_rate() const;
    size_t get_consecutive_drops() const;
    size_t get_total_frames_dropped() const;
    size_t get_underrun_count() const;
    size_t get_overrun_count() const;
    
    // Buffer management
    void flush_buffer();
    void clear_buffer();
    void reset_statistics();
    bool resize_buffer(size_t new_capacity);
    void enable_auto_resize(bool enabled);
    void set_resize_thresholds(double high_threshold, double low_threshold);
    
    // Emergency operations
    void emergency_flush_to_disk(const QString& filename);
    void emergency_load_from_disk(const QString& filename);
    bool enable_emergency_backup(const QString& backup_directory);
    void disable_emergency_backup();
    
    // Advanced features
    void enable_memory_prefetch(bool enabled);
    void set_batch_processing_size(size_t batch_size);
    void enable_cache_optimization(bool enabled);
    void set_thread_affinity(int cpu_core);
    
    // Performance tuning
    void optimize_for_low_latency();
    void optimize_for_high_throughput();
    void optimize_for_memory_efficiency();
    void auto_optimize_performance();
    
    // Monitoring and analysis
    void enable_performance_monitoring(bool enabled);
    void set_monitoring_interval(std::chrono::milliseconds interval);
    QJsonObject get_detailed_statistics() const;
    QStringList get_performance_recommendations() const;
    
    // Callback registration
    using OverflowCallback = std::function<void(const BufferStatus&)>;
    using UnderflowCallback = std::function<void(const BufferStatus&)>;
    using LatencyCallback = std::function<void(std::chrono::microseconds)>;
    using PerformanceCallback = std::function<void(const BufferStatus&)>;
    
    void set_overflow_callback(OverflowCallback callback);
    void set_underflow_callback(UnderflowCallback callback);
    void set_latency_callback(LatencyCallback callback);
    void set_performance_callback(PerformanceCallback callback);

signals:
    void buffer_overflow_detected(const eti::BufferStatus& status);
    void buffer_underflow_detected(const eti::BufferStatus& status);
    void latency_threshold_exceeded(std::chrono::microseconds latency);
    void real_time_guarantee_violated();
    void buffer_resize_occurred(size_t old_capacity, size_t new_capacity);
    void performance_degradation_detected(const QString& issue);
    void emergency_backup_triggered(const QString& reason);
    void statistics_updated(const eti::BufferStatus& status);

private slots:
    void update_statistics();
    void analyze_performance();
    void check_emergency_conditions();
    void perform_maintenance();

private:
    void initialize_buffer();
    void cleanup_buffer();
    void validate_config(const BufferConfig& config);
    void setup_monitoring();
    void cleanup_monitoring();
    
    // Buffer strategy implementations
    std::unique_ptr<LockFreeRingBuffer> create_ring_buffer(size_t capacity);
    std::unique_ptr<AdaptiveBuffer> create_adaptive_buffer();
    
    // Performance optimization
    void apply_memory_optimizations();
    void apply_cache_optimizations();
    void apply_thread_optimizations();
    
    // Emergency handling
    void handle_overflow_emergency();
    void handle_underflow_emergency();
    void handle_latency_emergency();
    
    // Statistics calculation
    void calculate_latency_statistics();
    void calculate_throughput_statistics();
    void calculate_quality_metrics();
    void update_real_time_guarantee();
    
    BufferConfig m_config;
    mutable QMutex m_config_mutex;
    
    // Buffer implementations
    std::unique_ptr<LockFreeRingBuffer> m_ring_buffer;
    std::unique_ptr<AdaptiveBuffer> m_adaptive_buffer;
    
    // Status and monitoring
    BufferStatus m_status;
    mutable QMutex m_status_mutex;
    
    // Performance tracking
    std::unique_ptr<QTimer> m_statistics_timer;
    std::unique_ptr<QTimer> m_performance_timer;
    std::unique_ptr<QTimer> m_maintenance_timer;
    
    // Performance history
    std::vector<std::chrono::microseconds> m_latency_history;
    std::vector<double> m_throughput_history;
    std::vector<double> m_frame_rate_history;
    std::vector<double> m_jitter_history;
    
    // Emergency backup
    QString m_backup_directory;
    std::atomic<bool> m_emergency_backup_enabled{false};
    std::atomic<size_t> m_emergency_backup_count{0};
    
    // Callbacks
    OverflowCallback m_overflow_callback;
    UnderflowCallback m_underflow_callback;
    LatencyCallback m_latency_callback;
    PerformanceCallback m_performance_callback;
    QMutex m_callbacks_mutex;
    
    // Performance optimization flags
    std::atomic<bool> m_performance_monitoring_enabled{true};
    std::atomic<bool> m_memory_prefetch_enabled{true};
    std::atomic<bool> m_cache_optimization_enabled{true};
    std::atomic<int> m_thread_affinity{-1};
    
    static constexpr size_t DEFAULT_CAPACITY = 1000;
    static constexpr size_t MAX_CAPACITY = 100000;
    static constexpr size_t HISTORY_SIZE = 1000;
    static constexpr double DEFAULT_RESIZE_FACTOR = 1.5;
};

/**
 * @brief Factory for creating pre-configured buffer managers
 */
class RealTimeBufferManagerFactory {
public:
    /**
     * @brief Create buffer manager optimized for low latency applications
     */
    static std::unique_ptr<RealTimeBufferManager> create_low_latency_buffer(
        size_t capacity = 500);

    /**
     * @brief Create buffer manager optimized for high throughput
     */
    static std::unique_ptr<RealTimeBufferManager> create_high_throughput_buffer(
        size_t capacity = 5000);

    /**
     * @brief Create buffer manager for broadcast monitoring
     */
    static std::unique_ptr<RealTimeBufferManager> create_monitoring_buffer(
        size_t capacity = 2000);

    /**
     * @brief Create buffer manager for stream recording
     */
    static std::unique_ptr<RealTimeBufferManager> create_recording_buffer(
        size_t capacity = 10000);

    /**
     * @brief Create adaptive buffer manager that adjusts to load
     */
    static std::unique_ptr<RealTimeBufferManager> create_adaptive_buffer(
        size_t initial_capacity = 1000);
};

/**
 * @brief Utility functions for buffer performance analysis
 */
namespace BufferUtils {
    
    /**
     * @brief Buffer performance analysis
     */
    struct PerformanceReport {
        double average_latency_us;
        double maximum_latency_us;
        double jitter_us;
        double throughput_mbps;
        double frame_rate;
        double utilization;
        double packet_loss_rate;
        size_t total_overruns;
        size_t total_underruns;
        bool real_time_guarantee;
        QDateTime report_time;
    };
    
    PerformanceReport analyze_buffer_performance(const BufferStatus& status);
    QStringList generate_performance_recommendations(const PerformanceReport& report);
    
    /**
     * @brief Buffer sizing utilities
     */
    size_t calculate_optimal_buffer_size(double frame_rate, 
                                        std::chrono::microseconds target_latency,
                                        double safety_factor = 1.5);
    
    size_t calculate_minimum_buffer_size(double frame_rate,
                                        std::chrono::microseconds max_jitter);
    
    /**
     * @brief Memory optimization utilities
     */
    bool is_memory_aligned(void* ptr, size_t alignment);
    void* allocate_aligned_memory(size_t size, size_t alignment);
    void free_aligned_memory(void* ptr);
    
    /**
     * @brief Performance testing utilities
     */
    struct BufferBenchmarkResult {
        std::chrono::microseconds write_latency;
        std::chrono::microseconds read_latency;
        double throughput_mbps;
        size_t operations_per_second;
        bool passed_real_time_test;
    };
    
    BufferBenchmarkResult benchmark_buffer_performance(RealTimeBufferManager& buffer,
                                                       size_t test_frames = 10000);
}

} // namespace eti

#endif // REALTIME_BUFFER_MANAGER_H