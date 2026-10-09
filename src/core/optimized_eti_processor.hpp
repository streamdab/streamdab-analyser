/**
 * @file optimized_eti_processor.hpp
 * @brief High-Performance Optimized ETI Processor with SIMD Instructions
 * 
 * Advanced ETI frame processing engine optimized for >1200 FPS throughput
 * with SIMD vectorization, zero-copy operations, and memory pool optimization.
 * Target: Achieve 10.0/10.0 performance score with validated benchmarks.
 * 
 * @author Agent 20 - Performance Optimization Specialist
 * @date 2025-09-26
 */

#pragma once

#include <QObject>
#include <QByteArray>
#include <immintrin.h>  // AVX2/SIMD instructions
#include <memory>
#include <atomic>
#include <chrono>
#include <vector>
#include <array>
#include <thread>
#include <condition_variable>
#include <mutex>
#include "eti_types.hpp"
#include "performance_profiler.hpp"

namespace eti::performance {

/**
 * @brief SIMD-optimized sync pattern detection
 */
class SimdSyncDetector {
public:
    SimdSyncDetector();
    
    /**
     * @brief Detect ETI sync pattern using AVX2 instructions
     * @param data Input data buffer (must be 32-byte aligned)
     * @param length Buffer length in bytes
     * @return Offset of sync pattern or -1 if not found
     */
    int32_t detect_sync_pattern_avx2(const uint8_t* __restrict__ data, size_t length) const;
    
    /**
     * @brief Vectorized CRC-32 calculation using SIMD
     * @param data Input data (32-byte aligned)
     * @param length Data length in bytes
     * @return CRC-32 checksum
     */
    uint32_t calculate_crc32_simd(const uint8_t* __restrict__ data, size_t length) const;
    
private:
    alignas(32) uint8_t sync_pattern_[32];  // 32-byte aligned sync pattern
    alignas(32) uint32_t crc32_table_[256]; // SIMD-optimized CRC table
};

/**
 * @brief Lock-free ring buffer for zero-copy frame processing
 */
template<size_t BufferSize = 8192>
class LockFreeFrameBuffer {
public:
    static_assert((BufferSize & (BufferSize - 1)) == 0, "BufferSize must be power of 2");
    
    struct FrameEntry {
        alignas(32) uint8_t data[eti::ETI_FRAME_SIZE];
        std::atomic<bool> ready{false};
        std::chrono::high_resolution_clock::time_point timestamp;
    };
    
    LockFreeFrameBuffer();
    
    /**
     * @brief Acquire frame buffer for writing (zero-copy)
     * @return Pointer to frame entry or nullptr if buffer full
     */
    FrameEntry* acquire_write_buffer();
    
    /**
     * @brief Release write buffer and mark ready
     * @param entry Frame entry to release
     */
    void release_write_buffer(FrameEntry* entry);
    
    /**
     * @brief Acquire frame buffer for reading
     * @return Pointer to ready frame or nullptr if empty
     */
    FrameEntry* acquire_read_buffer();
    
    /**
     * @brief Release read buffer
     * @param entry Frame entry to release
     */
    void release_read_buffer(FrameEntry* entry);
    
    /**
     * @brief Get current buffer utilization
     * @return Utilization percentage (0-100)
     */
    double get_utilization() const;
    
private:
    alignas(64) std::array<FrameEntry, BufferSize> buffer_;
    alignas(64) std::atomic<size_t> write_index_{0};
    alignas(64) std::atomic<size_t> read_index_{0};
    
    static constexpr size_t INDEX_MASK = BufferSize - 1;
};

/**
 * @brief Memory pool for ETI frame allocation
 */
class EtiFrameMemoryPool {
public:
    explicit EtiFrameMemoryPool(size_t pool_size = 1024);
    ~EtiFrameMemoryPool();
    
    /**
     * @brief Allocate aligned frame buffer
     * @return 32-byte aligned frame buffer or nullptr if pool exhausted
     */
    uint8_t* allocate_frame();
    
    /**
     * @brief Deallocate frame buffer
     * @param buffer Previously allocated buffer
     */
    void deallocate_frame(uint8_t* buffer);
    
    /**
     * @brief Get pool statistics
     */
    struct PoolStats {
        size_t total_blocks;
        size_t used_blocks;
        size_t peak_usage;
        double utilization_percent;
    };
    
    PoolStats get_statistics() const;
    
private:
    struct alignas(32) FrameBlock {
        uint8_t data[eti::ETI_FRAME_SIZE];
        FrameBlock* next;
        bool in_use;
    };
    
    mutable std::mutex pool_mutex_;
    std::vector<std::unique_ptr<FrameBlock[]>> memory_blocks_;
    FrameBlock* free_list_;
    std::atomic<size_t> used_count_{0};
    std::atomic<size_t> peak_usage_{0};
    const size_t pool_size_;
};

/**
 * @brief High-performance ETI processor with SIMD optimization
 */
class OptimizedEtiProcessor : public QObject {
    Q_OBJECT
    
public:
    explicit OptimizedEtiProcessor(QObject* parent = nullptr);
    ~OptimizedEtiProcessor();
    
    /**
     * @brief Initialize optimized processor
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Process ETI frame with SIMD optimization
     * @param frame_data Raw ETI frame data (6144 bytes)
     * @return true if frame processed successfully
     * 
     * Performance Target: >1200 FPS sustained throughput
     */
    bool process_eti_frame_optimized(const QByteArray& frame_data);
    
    /**
     * @brief Batch process multiple frames for maximum throughput
     * @param frames Vector of ETI frames
     * @return Number of successfully processed frames
     * 
     * Performance Target: >1500 FPS batch processing
     */
    size_t process_batch_optimized(const std::vector<QByteArray>& frames);
    
    /**
     * @brief Enable real-time processing mode
     * @param enabled Enable/disable real-time mode
     * 
     * Real-time mode optimizations:
     * - Lock-free ring buffers
     * - Thread affinity optimization
     * - SIMD prefetching
     */
    void set_real_time_mode(bool enabled);
    
    /**
     * @brief Get current processing performance
     */
    struct PerformanceMetrics {
        double current_fps;
        double peak_fps;
        double average_fps;
        std::chrono::microseconds average_latency;
        std::chrono::microseconds peak_latency;
        size_t memory_usage_bytes;
        double cpu_efficiency_percent;
        size_t total_frames_processed;
        size_t successful_frames;
        double success_rate_percent;
    };
    
    PerformanceMetrics get_performance_metrics() const;
    
    /**
     * @brief Reset performance counters
     */
    void reset_performance_counters();
    
    /**
     * @brief Configure SIMD optimization level
     */
    enum class SimdLevel {
        DISABLED,
        SSE2,
        AVX,
        AVX2,
        AVX512
    };
    
    void set_simd_level(SimdLevel level);
    SimdLevel get_optimal_simd_level() const;
    
signals:
    /**
     * @brief Emitted when frame processing completes
     * @param frame_number Frame sequence number
     * @param processing_time Time taken to process frame (nanoseconds)
     */
    void frame_processed_optimized(uint64_t frame_number, std::chrono::nanoseconds processing_time);
    
    /**
     * @brief Emitted when performance target is achieved
     * @param current_fps Current processing rate
     * @param target_fps Target processing rate
     */
    void performance_target_achieved(double current_fps, double target_fps);
    
    /**
     * @brief Emitted when performance warning occurs
     * @param warning_message Performance warning description
     * @param current_metric Current performance metric value
     * @param threshold_metric Threshold that was exceeded
     */
    void performance_warning(const QString& warning_message, double current_metric, double threshold_metric);

private slots:
    void handle_buffer_overflow();
    void handle_performance_monitoring();

private:
    // SIMD optimization components
    std::unique_ptr<SimdSyncDetector> simd_detector_;
    SimdLevel current_simd_level_;
    
    // Memory management
    std::unique_ptr<EtiFrameMemoryPool> memory_pool_;
    std::unique_ptr<LockFreeFrameBuffer<8192>> frame_buffer_;
    
    // Performance monitoring
    std::unique_ptr<eti::modern::performance_profiler> profiler_;
    mutable std::mutex performance_mutex_;
    
    // Processing threads
    std::vector<std::thread> processing_threads_;
    std::atomic<bool> processing_active_{false};
    std::atomic<bool> real_time_mode_{false};
    
    // Performance counters
    std::atomic<uint64_t> frames_processed_{0};
    std::atomic<uint64_t> successful_frames_{0};
    std::atomic<uint64_t> failed_frames_{0};
    
    // Timing statistics
    std::chrono::high_resolution_clock::time_point start_time_;
    std::atomic<double> current_fps_{0.0};
    std::atomic<double> peak_fps_{0.0};
    std::atomic<uint64_t> total_processing_time_ns_{0};
    
    // Target performance thresholds
    static constexpr double TARGET_FPS_MINIMUM = 1200.0;
    static constexpr double TARGET_FPS_OPTIMAL = 1500.0;
    static constexpr size_t TARGET_MEMORY_MB = 50;
    static constexpr std::chrono::microseconds TARGET_LATENCY{10000}; // 10ms
    
    // Core processing methods
    bool process_frame_internal(const uint8_t* __restrict__ frame_data);
    bool validate_frame_simd(const uint8_t* __restrict__ frame_data);
    bool parse_fic_optimized(const uint8_t* __restrict__ fic_data, size_t fic_length);
    bool parse_msc_optimized(const uint8_t* __restrict__ msc_data, size_t msc_length);
    
    // Performance optimization methods
    void optimize_thread_affinity();
    void configure_memory_prefetch();
    void update_performance_statistics();
    void monitor_system_resources();
    
    // SIMD-specific processing methods
    bool process_frame_avx2(const uint8_t* __restrict__ frame_data);
    bool process_frame_sse2(const uint8_t* __restrict__ frame_data);
    bool process_frame_scalar(const uint8_t* __restrict__ frame_data);
    
    // Memory management helpers
    void initialize_memory_pools();
    void cleanup_memory_resources();
    size_t calculate_memory_usage() const;
    
    // CPU feature detection
    bool detect_avx2_support() const;
    bool detect_avx512_support() const;
    void log_cpu_features() const;
};

// Template implementations for LockFreeFrameBuffer
template<size_t BufferSize>
LockFreeFrameBuffer<BufferSize>::LockFreeFrameBuffer() {
    // Initialize all frame entries
    for (auto& entry : buffer_) {
        entry.ready.store(false, std::memory_order_relaxed);
    }
}

template<size_t BufferSize>
typename LockFreeFrameBuffer<BufferSize>::FrameEntry* 
LockFreeFrameBuffer<BufferSize>::acquire_write_buffer() {
    const size_t current_write = write_index_.load(std::memory_order_acquire);
    const size_t next_write = (current_write + 1) & INDEX_MASK;
    
    // Check if buffer would be full
    if (next_write == read_index_.load(std::memory_order_acquire)) {
        return nullptr; // Buffer full
    }
    
    FrameEntry* entry = &buffer_[current_write];
    if (entry->ready.load(std::memory_order_acquire)) {
        return nullptr; // Entry not yet consumed
    }
    
    // Try to advance write index
    if (write_index_.compare_exchange_weak(const_cast<size_t&>(current_write), next_write, 
                                          std::memory_order_acq_rel)) {
        entry->timestamp = std::chrono::high_resolution_clock::now();
        return entry;
    }
    
    return nullptr; // Another thread advanced the index
}

template<size_t BufferSize>
void LockFreeFrameBuffer<BufferSize>::release_write_buffer(FrameEntry* entry) {
    entry->ready.store(true, std::memory_order_release);
}

template<size_t BufferSize>
typename LockFreeFrameBuffer<BufferSize>::FrameEntry* 
LockFreeFrameBuffer<BufferSize>::acquire_read_buffer() {
    const size_t current_read = read_index_.load(std::memory_order_acquire);
    
    if (current_read == write_index_.load(std::memory_order_acquire)) {
        return nullptr; // Buffer empty
    }
    
    FrameEntry* entry = &buffer_[current_read];
    if (!entry->ready.load(std::memory_order_acquire)) {
        return nullptr; // Entry not ready
    }
    
    return entry;
}

template<size_t BufferSize>
void LockFreeFrameBuffer<BufferSize>::release_read_buffer(FrameEntry* entry) {
    entry->ready.store(false, std::memory_order_release);
    
    // Advance read index
    const size_t current_read = read_index_.load(std::memory_order_acquire);
    const size_t next_read = (current_read + 1) & INDEX_MASK;
    read_index_.store(next_read, std::memory_order_release);
}

template<size_t BufferSize>
double LockFreeFrameBuffer<BufferSize>::get_utilization() const {
    const size_t write_pos = write_index_.load(std::memory_order_acquire);
    const size_t read_pos = read_index_.load(std::memory_order_acquire);
    
    size_t used_entries;
    if (write_pos >= read_pos) {
        used_entries = write_pos - read_pos;
    } else {
        used_entries = BufferSize - read_pos + write_pos;
    }
    
    return (static_cast<double>(used_entries) / BufferSize) * 100.0;
}

} // namespace eti::performance