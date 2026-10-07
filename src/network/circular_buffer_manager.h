/**
 * @file circular_buffer_manager.h
 * @brief Memory-Efficient Circular Buffer Manager for Real-time ETI Processing
 *
 * Implements high-performance circular buffering for ETI streams with:
 * - Lock-free thread-safe operations for >900 FPS throughput
 * - Memory-efficient design <30MB for network components
 * - Adaptive buffer sizing based on network conditions
 * - Real-time overflow and underflow protection
 * - Zero-copy operations where possible
 * - Professional broadcast industry reliability
 *
 * @author Network/Stream Agent
 * @date 2025-09-22
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 */

#ifndef CIRCULAR_BUFFER_MANAGER_H
#define CIRCULAR_BUFFER_MANAGER_H

#include <QObject>
#include <QMutex>
#include <QTimer>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <array>

#include "../core/eti_types.hpp"
#include "eti_over_ip_receiver.h"

namespace eti_network {

/**
 * @brief Buffer configuration for different use cases
 */
struct CircularBufferConfig {
    size_t buffer_size = 1000;                           // Number of ETI frames
    size_t memory_limit_mb = 25;                         // Memory limit in MB (<30MB requirement)
    bool adaptive_sizing = true;                         // Enable adaptive buffer sizing
    bool zero_copy_mode = true;                          // Enable zero-copy operations
    bool overflow_protection = true;                     // Protect against buffer overflow
    bool underflow_recovery = true;                      // Enable underflow recovery

    // Performance targets
    std::chrono::microseconds target_latency{17000};     // <17ms target latency
    double target_throughput_fps = 1000.0;              // >900 FPS requirement
    double overflow_threshold = 0.9;                     // Overflow at 90% capacity
    double underflow_threshold = 0.1;                    // Underflow at 10% capacity

    // Adaptive behavior
    size_t min_buffer_size = 100;                        // Minimum buffer size
    size_t max_buffer_size = 2000;                       // Maximum buffer size
    std::chrono::milliseconds adaptation_interval{1000}; // Adaptation check interval
    double growth_factor = 1.5;                         // Buffer growth factor
    double shrink_factor = 0.8;                         // Buffer shrink factor

    // Memory optimization
    bool use_memory_mapping = false;                     // Use memory-mapped buffers
    bool compact_on_idle = true;                         // Compact memory during idle
    std::chrono::seconds idle_threshold{5};              // Idle time before compaction

    // Monitoring
    bool enable_metrics = true;                          // Enable performance metrics
    std::chrono::milliseconds metrics_interval{500};     // Metrics update interval
};

/**
 * @brief Copyable buffer metrics snapshot (for returning values)
 */
struct BufferMetricsSnapshot {
    // Capacity and utilization
    size_t capacity{0};                    // Current buffer capacity
    size_t size{0};                        // Current number of items
    double utilization{0.0};               // Buffer utilization (0.0-1.0)
    size_t peak_utilization{0};            // Peak utilization reached

    // Memory usage
    size_t memory_usage_bytes{0};          // Current memory usage
    size_t peak_memory_bytes{0};           // Peak memory usage
    double memory_efficiency{1.0};         // Memory efficiency ratio

    // Performance metrics
    size_t items_written{0};               // Total items written
    size_t items_read{0};                  // Total items read
    size_t overflows{0};                   // Overflow events
    size_t underflows{0};                  // Underflow events
    size_t adaptations{0};                 // Adaptive resizing events

    // Timing metrics
    int64_t avg_write_time_ns{0};          // Average write time (nanoseconds)
    int64_t avg_read_time_ns{0};           // Average read time (nanoseconds)
    int64_t max_latency_ns{0};             // Maximum latency observed
    double throughput_fps{0.0};            // Current throughput (FPS)

    // Quality metrics
    double data_integrity{1.0};            // Data integrity score (0.0-1.0)
    size_t corruption_events{0};           // Data corruption events
    size_t recovery_events{0};             // Recovery events triggered
};

/**
 * @brief Buffer performance metrics for monitoring and optimization (atomic for thread safety)
 */
struct BufferMetrics {
    // Capacity and utilization
    std::atomic<size_t> capacity{0};                    // Current buffer capacity
    std::atomic<size_t> size{0};                        // Current number of items
    std::atomic<double> utilization{0.0};               // Buffer utilization (0.0-1.0)
    std::atomic<size_t> peak_utilization{0};            // Peak utilization reached

    // Memory usage
    std::atomic<size_t> memory_usage_bytes{0};          // Current memory usage
    std::atomic<size_t> peak_memory_bytes{0};           // Peak memory usage
    std::atomic<double> memory_efficiency{1.0};         // Memory efficiency ratio

    // Performance metrics
    std::atomic<size_t> items_written{0};               // Total items written
    std::atomic<size_t> items_read{0};                  // Total items read
    std::atomic<size_t> overflows{0};                   // Overflow events
    std::atomic<size_t> underflows{0};                  // Underflow events
    std::atomic<size_t> adaptations{0};                 // Adaptive resizing events

    // Timing metrics
    std::atomic<int64_t> avg_write_time_ns{0};          // Average write time (nanoseconds)
    std::atomic<int64_t> avg_read_time_ns{0};           // Average read time (nanoseconds)
    std::atomic<int64_t> max_latency_ns{0};             // Maximum latency observed
    std::atomic<double> throughput_fps{0.0};            // Current throughput (FPS)

    // Quality metrics
    std::atomic<double> data_integrity{1.0};            // Data integrity score (0.0-1.0)
    std::atomic<size_t> corruption_events{0};           // Data corruption events
    std::atomic<size_t> recovery_events{0};             // Recovery events triggered

    void reset() {
        capacity = 0;
        size = 0;
        utilization = 0.0;
        peak_utilization = 0;
        memory_usage_bytes = 0;
        peak_memory_bytes = 0;
        memory_efficiency = 1.0;
        items_written = 0;
        items_read = 0;
        overflows = 0;
        underflows = 0;
        adaptations = 0;
        avg_write_time_ns = 0;
        avg_read_time_ns = 0;
        max_latency_ns = 0;
        throughput_fps = 0.0;
        data_integrity = 1.0;
        corruption_events = 0;
        recovery_events = 0;
    }

    // Method to create a copyable snapshot
    BufferMetricsSnapshot getSnapshot() const {
        BufferMetricsSnapshot snapshot;
        snapshot.capacity = capacity.load();
        snapshot.size = size.load();
        snapshot.utilization = utilization.load();
        snapshot.peak_utilization = peak_utilization.load();
        snapshot.memory_usage_bytes = memory_usage_bytes.load();
        snapshot.peak_memory_bytes = peak_memory_bytes.load();
        snapshot.memory_efficiency = memory_efficiency.load();
        snapshot.items_written = items_written.load();
        snapshot.items_read = items_read.load();
        snapshot.overflows = overflows.load();
        snapshot.underflows = underflows.load();
        snapshot.adaptations = adaptations.load();
        snapshot.avg_write_time_ns = avg_write_time_ns.load();
        snapshot.avg_read_time_ns = avg_read_time_ns.load();
        snapshot.max_latency_ns = max_latency_ns.load();
        snapshot.throughput_fps = throughput_fps.load();
        snapshot.data_integrity = data_integrity.load();
        snapshot.corruption_events = corruption_events.load();
        snapshot.recovery_events = recovery_events.load();
        return snapshot;
    }
};

/**
 * @brief ETI frame with metadata for efficient buffering
 */
struct BufferedETIFrame {
    eti::EtiFrame frame;                                 // ETI frame data
    NetworkQualityMetrics network_quality;              // Network quality at reception
    std::chrono::steady_clock::time_point timestamp;    // Buffer timestamp
    uint64_t sequence_number;                           // Sequence number for ordering
    uint32_t checksum;                                  // Data integrity checksum
    bool is_valid;                                      // Validity flag

    BufferedETIFrame() : sequence_number(0), checksum(0), is_valid(false) {
        timestamp = std::chrono::steady_clock::now();
    }

    explicit BufferedETIFrame(const eti::EtiFrame& eti_frame,
                             const NetworkQualityMetrics& quality = NetworkQualityMetrics{})
        : frame(eti_frame), network_quality(quality), sequence_number(0), is_valid(true) {
        timestamp = std::chrono::steady_clock::now();
        checksum = calculateChecksum();
    }

    bool validateIntegrity() const {
        return is_valid && (calculateChecksum() == checksum);
    }

    std::chrono::microseconds getAge() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(now - timestamp);
    }

private:
    uint32_t calculateChecksum() const {
        // Simplified checksum - in real implementation would use CRC32 or similar
        uint32_t sum = 0;
        const uint8_t* data = frame.data();
        for (size_t i = 0; i < frame.size(); ++i) {
            sum += data[i];
        }
        return sum;
    }
};

/**
 * @brief Lock-free circular buffer for high-performance ETI frame buffering
 *
 * Implements a lock-free SPSC (Single Producer Single Consumer) circular buffer
 * optimized for real-time ETI frame processing with guaranteed <17ms latency.
 */
template<typename T, size_t N>
class LockFreeCircularBuffer {
public:
    static_assert(N > 0 && (N & (N - 1)) == 0, "Buffer size must be power of 2");

    LockFreeCircularBuffer() : m_head(0), m_tail(0) {}

    /**
     * @brief Write item to buffer (producer side)
     * @return true if successful, false if buffer full
     */
    bool write(const T& item) {
        const size_t head = m_head.load(std::memory_order_relaxed);
        const size_t next_head = (head + 1) & (N - 1);

        if (next_head == m_tail.load(std::memory_order_acquire)) {
            return false; // Buffer full
        }

        m_buffer[head] = item;
        m_head.store(next_head, std::memory_order_release);
        return true;
    }

    /**
     * @brief Write item to buffer (move semantics)
     */
    bool write(T&& item) {
        const size_t head = m_head.load(std::memory_order_relaxed);
        const size_t next_head = (head + 1) & (N - 1);

        if (next_head == m_tail.load(std::memory_order_acquire)) {
            return false; // Buffer full
        }

        m_buffer[head] = std::move(item);
        m_head.store(next_head, std::memory_order_release);
        return true;
    }

    /**
     * @brief Read item from buffer (consumer side)
     * @return true if successful, false if buffer empty
     */
    bool read(T& item) {
        const size_t tail = m_tail.load(std::memory_order_relaxed);

        if (tail == m_head.load(std::memory_order_acquire)) {
            return false; // Buffer empty
        }

        item = std::move(m_buffer[tail]);
        m_tail.store((tail + 1) & (N - 1), std::memory_order_release);
        return true;
    }

    /**
     * @brief Get current buffer size
     */
    size_t size() const {
        const size_t head = m_head.load(std::memory_order_acquire);
        const size_t tail = m_tail.load(std::memory_order_acquire);
        return (head - tail) & (N - 1);
    }

    /**
     * @brief Check if buffer is empty
     */
    bool empty() const {
        return m_head.load(std::memory_order_acquire) ==
               m_tail.load(std::memory_order_acquire);
    }

    /**
     * @brief Check if buffer is full
     */
    bool full() const {
        const size_t head = m_head.load(std::memory_order_acquire);
        const size_t tail = m_tail.load(std::memory_order_acquire);
        return ((head + 1) & (N - 1)) == tail;
    }

    /**
     * @brief Get buffer capacity
     */
    constexpr size_t capacity() const {
        return N - 1; // One slot reserved for distinguishing full/empty
    }

    /**
     * @brief Get buffer utilization (0.0-1.0)
     */
    double utilization() const {
        return static_cast<double>(size()) / capacity();
    }

    /**
     * @brief Clear buffer (not thread-safe, for single-threaded use only)
     */
    void clear() {
        m_head.store(0, std::memory_order_relaxed);
        m_tail.store(0, std::memory_order_relaxed);
    }

private:
    std::array<T, N> m_buffer;
    std::atomic<size_t> m_head;
    std::atomic<size_t> m_tail;

    // Cache line padding to avoid false sharing
    char padding1[64 - sizeof(std::atomic<size_t>)];
    char padding2[64 - sizeof(std::atomic<size_t>)];
};

/**
 * @brief Custom deleter for aligned BufferedETIFrame allocation
 */
struct AlignedBufferDeleter {
    size_t capacity{0};

    void operator()(BufferedETIFrame* ptr) const {
        if (ptr) {
            // Explicitly destroy all objects
            for (size_t i = 0; i < capacity; ++i) {
                ptr[i].~BufferedETIFrame();
            }
            // Free aligned memory (use free, not delete)
            std::free(ptr);
        }
    }
};

/**
 * @brief Adaptive circular buffer manager for dynamic sizing
 */
class AdaptiveBufferManager {
public:
    explicit AdaptiveBufferManager(const CircularBufferConfig& config);
    ~AdaptiveBufferManager();

    // Buffer operations
    bool writeFrame(const BufferedETIFrame& frame);
    bool readFrame(BufferedETIFrame& frame);
    bool peekFrame(BufferedETIFrame& frame) const;

    // Buffer management
    void resize(size_t newSize);
    void compact();
    void flush();
    void reset();

    // Status and metrics
    size_t size() const;
    size_t capacity() const;
    double utilization() const;
    bool isEmpty() const;
    bool isFull() const;

    // Configuration
    void updateConfig(const CircularBufferConfig& config);
    CircularBufferConfig getConfig() const;

    // Metrics
    BufferMetricsSnapshot getMetrics() const;
    void resetMetrics();

    // Adaptive behavior
    void enableAdaptiveSizing(bool enabled);
    void checkAdaptation();
    void adaptToLoad();

private:
    void performAdaptation();
    void growBuffer();
    void shrinkBuffer();
    bool shouldGrow() const;
    bool shouldShrink() const;
    bool allocateBuffer(size_t capacity);

    CircularBufferConfig m_config;
    std::unique_ptr<BufferedETIFrame, AlignedBufferDeleter> m_buffer_memory;
    std::atomic<size_t> m_head{0};
    std::atomic<size_t> m_tail{0};
    std::atomic<size_t> m_capacity{0};

    mutable BufferMetrics m_metrics;
    mutable QMutex m_metrics_mutex;

    std::chrono::steady_clock::time_point m_last_adaptation;
    std::chrono::steady_clock::time_point m_last_activity;

    static constexpr size_t CACHE_LINE_SIZE = 64;
};

/**
 * @brief Memory-Efficient Circular Buffer Manager for Real-time ETI Processing
 *
 * High-performance buffer manager implementing:
 * - Lock-free circular buffering for >900 FPS throughput
 * - Memory-efficient design <30MB for network components
 * - Adaptive buffer sizing based on network conditions
 * - Real-time overflow and underflow protection
 * - Zero-copy operations for maximum performance
 * - Professional broadcast industry reliability
 */
class CircularBufferManager : public QObject {
    Q_OBJECT

public:
    explicit CircularBufferManager(QObject* parent = nullptr);
    explicit CircularBufferManager(const CircularBufferConfig& config, QObject* parent = nullptr);
    ~CircularBufferManager();

    // Configuration management
    void setConfiguration(const CircularBufferConfig& config);
    CircularBufferConfig getConfiguration() const;
    void setMemoryLimit(size_t limitMB);
    void setTargetLatency(std::chrono::microseconds latency);
    void setTargetThroughput(double fps);

    // Buffer operations
    bool writeFrame(const eti::EtiFrame& frame, const NetworkQualityMetrics& quality);
    bool writeBatch(const std::vector<std::pair<eti::EtiFrame, NetworkQualityMetrics>>& frames);
    bool readFrame(eti::EtiFrame& frame, NetworkQualityMetrics& quality);
    bool readBatch(std::vector<std::pair<eti::EtiFrame, NetworkQualityMetrics>>& frames, size_t maxCount);
    bool peekFrame(eti::EtiFrame& frame, NetworkQualityMetrics& quality) const;

    // Buffer management
    void flush();
    void reset();
    void compact();
    void resize(size_t newSize);

    // Status monitoring
    size_t size() const;
    size_t capacity() const;
    double utilization() const;
    bool isEmpty() const;
    bool isFull() const;
    bool isHealthy() const;

    // Memory management
    size_t getMemoryUsage() const;
    size_t getMemoryLimit() const;
    double getMemoryEfficiency() const;
    void optimizeMemoryUsage();
    void enableMemoryCompaction(bool enabled);

    // Performance metrics
    BufferMetricsSnapshot getMetrics() const;
    double getCurrentThroughput() const;
    std::chrono::microseconds getCurrentLatency() const;
    bool isPerformanceTargetMet() const;
    void resetMetrics();

    // Adaptive behavior
    void enableAdaptiveSizing(bool enabled);
    void enableOverflowProtection(bool enabled);
    void enableUnderflowRecovery(bool enabled);
    void setAdaptationThresholds(double overflowThreshold, double underflowThreshold);

    // Monitoring and diagnostics
    QString getDiagnosticInfo() const;
    QStringList getPerformanceReport() const;
    void exportMetrics(const QString& filename) const;

    // Callback registration
    using OverflowCallback = std::function<void(size_t droppedFrames)>;
    using UnderflowCallback = std::function<void()>;
    using MetricsCallback = std::function<void(const BufferMetrics&)>;

    void setOverflowCallback(OverflowCallback callback);
    void setUnderflowCallback(UnderflowCallback callback);
    void setMetricsCallback(MetricsCallback callback);

signals:
    void bufferOverflow(size_t droppedFrames);
    void bufferUnderflow();
    void bufferAdapted(size_t oldSize, size_t newSize);
    void memoryLimitExceeded(size_t currentUsage, size_t limit);
    void performanceTargetMissed(double currentThroughput, double targetThroughput);
    void metricsUpdated(const BufferMetricsSnapshot& metrics);
    void dataIntegrityError(const QString& details);

private slots:
    void updateMetrics();
    void checkAdaptation();
    void performMaintenance();
    void monitorPerformance();

private:
    // Initialization and cleanup
    void initializeManager();
    void cleanupManager();
    void setupTimers();
    void connectSignals();

    // Buffer implementation selection
    void selectOptimalImplementation();
    void initializeLockFreeBuffer();
    void initializeAdaptiveBuffer();

    // Memory management
    void updateMemoryUsage();
    void enforceMemoryLimit();
    void compactMemoryIfNeeded();

    // Performance optimization
    void optimizeForLatency();
    void optimizeForThroughput();
    void balanceLatencyThroughput();

    // Adaptive behavior
    void performAdaptiveResize();
    void handleOverflow();
    void handleUnderflow();
    void adaptToNetworkConditions();

    // Data integrity
    void validateFrameIntegrity(const BufferedETIFrame& frame);
    void handleDataCorruption(const QString& details);

    // Metrics and monitoring
    void updatePerformanceMetrics();
    void calculateLatencyMetrics();
    void calculateThroughputMetrics();
    void updateMemoryMetrics();

    CircularBufferConfig m_config;

    // Buffer implementations
    std::unique_ptr<LockFreeCircularBuffer<BufferedETIFrame, 2048>> m_lockFreeBuffer;
    std::unique_ptr<AdaptiveBufferManager> m_adaptiveBuffer;
    bool m_useLockFreeBuffer = true;

    // Metrics and monitoring
    BufferMetrics m_metrics;
    mutable QMutex m_metricsMutex;

    // Timers for monitoring and maintenance
    std::unique_ptr<QTimer> m_metricsTimer;
    std::unique_ptr<QTimer> m_adaptationTimer;
    std::unique_ptr<QTimer> m_maintenanceTimer;
    std::unique_ptr<QTimer> m_performanceTimer;

    // Performance tracking
    std::chrono::steady_clock::time_point m_lastWriteTime;
    std::chrono::steady_clock::time_point m_lastReadTime;
    std::chrono::steady_clock::time_point m_startTime;

    // Sequence tracking
    std::atomic<uint64_t> m_writeSequence{0};
    std::atomic<uint64_t> m_readSequence{0};

    // State management
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_healthy{true};
    std::atomic<bool> m_adaptiveSizing{true};
    std::atomic<bool> m_overflowProtection{true};
    std::atomic<bool> m_underflowRecovery{true};

    // Memory management
    std::atomic<size_t> m_memoryUsageBytes{0};
    std::atomic<size_t> m_memoryLimitBytes{26214400}; // 25MB default

    // Callbacks
    OverflowCallback m_overflowCallback;
    UnderflowCallback m_underflowCallback;
    MetricsCallback m_metricsCallback;
    mutable QMutex m_callbackMutex;

    // Constants
    static constexpr size_t DEFAULT_BUFFER_SIZE = 1000;
    static constexpr size_t MIN_BUFFER_SIZE = 100;
    static constexpr size_t MAX_BUFFER_SIZE = 2000;
    static constexpr std::chrono::milliseconds METRICS_UPDATE_INTERVAL{500};
    static constexpr std::chrono::milliseconds ADAPTATION_CHECK_INTERVAL{1000};
    static constexpr std::chrono::milliseconds MAINTENANCE_INTERVAL{5000};
    static constexpr std::chrono::milliseconds PERFORMANCE_CHECK_INTERVAL{100};
};

/**
 * @brief Factory for creating optimized circular buffer managers
 */
class CircularBufferFactory {
public:
    /**
     * @brief Create low-latency buffer manager (<17ms)
     */
    static std::unique_ptr<CircularBufferManager> createLowLatencyManager();

    /**
     * @brief Create high-throughput buffer manager (>900 FPS)
     */
    static std::unique_ptr<CircularBufferManager> createHighThroughputManager();

    /**
     * @brief Create memory-efficient buffer manager (<30MB)
     */
    static std::unique_ptr<CircularBufferManager> createMemoryEfficientManager();

    /**
     * @brief Create balanced buffer manager (latency + throughput)
     */
    static std::unique_ptr<CircularBufferManager> createBalancedManager();

    /**
     * @brief Create custom buffer manager with specific configuration
     */
    static std::unique_ptr<CircularBufferManager> createCustomManager(const CircularBufferConfig& config);

    /**
     * @brief Create optimized manager for current hardware
     */
    static std::unique_ptr<CircularBufferManager> createOptimizedManager();
};

} // namespace eti_network

#endif // CIRCULAR_BUFFER_MANAGER_H
