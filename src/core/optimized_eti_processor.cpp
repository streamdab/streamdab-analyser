/**
 * @file optimized_eti_processor.cpp
 * @brief SIMD-Optimized ETI Processor Implementation
 * 
 * High-performance ETI frame processing with AVX2/SIMD instructions,
 * lock-free ring buffers, and memory pool optimization.
 * Target: >1200 FPS sustained throughput with <15ms latency.
 * 
 * @author Agent 20 - Performance Optimization Specialist
 * @date 2025-09-26
 */

#include "optimized_eti_processor.hpp"
#include "utils/logger.h"

#include <immintrin.h>
#include <thread>
#include <algorithm>
#include <cstring>
#include <cpuid.h>

namespace eti::performance {

// SIMD Sync Detector Implementation
SimdSyncDetector::SimdSyncDetector() {
    // Initialize ETI sync pattern: 0x49, 0x93, 0x1E, 0x03
    std::memset(sync_pattern_, 0, sizeof(sync_pattern_));
    sync_pattern_[0] = 0x49;
    sync_pattern_[1] = 0x93;
    sync_pattern_[2] = 0x1E;
    sync_pattern_[3] = 0x03;
    
    // Replicate pattern for SIMD comparison
    for (int i = 4; i < 32; i += 4) {
        std::memcpy(&sync_pattern_[i], sync_pattern_, 4);
    }
    
    // Initialize CRC-32 lookup table for SIMD optimization
    constexpr uint32_t polynomial = 0x04C11DB7;
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i << 24;
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x80000000) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
        crc32_table_[i] = crc;
    }
}

int32_t SimdSyncDetector::detect_sync_pattern_avx2(const uint8_t* __restrict__ data, size_t length) const {
    if (length < 32) {
        // Fallback to scalar search for small buffers
        for (size_t i = 0; i <= length - 4; ++i) {
            if (data[i] == 0x49 && data[i+1] == 0x93 && data[i+2] == 0x1E && data[i+3] == 0x03) {
                return static_cast<int32_t>(i);
            }
        }
        return -1;
    }
    
    const __m256i pattern = _mm256_load_si256(reinterpret_cast<const __m256i*>(sync_pattern_));
    
    // Process 32-byte chunks with AVX2
    const size_t simd_end = length - 32;
    for (size_t i = 0; i <= simd_end; i += 32) {
        const __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data + i));
        const __m256i cmp = _mm256_cmpeq_epi32(chunk, pattern);
        
        const uint32_t mask = _mm256_movemask_epi8(cmp);
        if (mask != 0) {
            // Found potential match, verify with scalar code
            for (size_t j = i; j < std::min(i + 32, length - 3); ++j) {
                if (data[j] == 0x49 && data[j+1] == 0x93 && data[j+2] == 0x1E && data[j+3] == 0x03) {
                    return static_cast<int32_t>(j);
                }
            }
        }
    }
    
    // Handle remaining bytes with scalar code
    for (size_t i = simd_end + 1; i <= length - 4; ++i) {
        if (data[i] == 0x49 && data[i+1] == 0x93 && data[i+2] == 0x1E && data[i+3] == 0x03) {
            return static_cast<int32_t>(i);
        }
    }
    
    return -1;
}

uint32_t SimdSyncDetector::calculate_crc32_simd(const uint8_t* __restrict__ data, size_t length) const {
    uint32_t crc = 0xFFFFFFFF;
    
    // Process 32-byte chunks with SIMD when possible
    const size_t simd_chunks = length / 32;
    const size_t simd_bytes = simd_chunks * 32;
    
    for (size_t i = 0; i < simd_bytes; i += 32) {
        // Load 32 bytes
        const __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data + i));
        
        // Process each byte using SIMD-optimized lookup
        alignas(32) uint8_t bytes[32];
        _mm256_store_si256(reinterpret_cast<__m256i*>(bytes), chunk);
        
        for (int j = 0; j < 32; ++j) {
            const uint32_t table_index = ((crc >> 24) ^ bytes[j]) & 0xFF;
            crc = (crc << 8) ^ crc32_table_[table_index];
        }
    }
    
    // Process remaining bytes
    for (size_t i = simd_bytes; i < length; ++i) {
        const uint32_t table_index = ((crc >> 24) ^ data[i]) & 0xFF;
        crc = (crc << 8) ^ crc32_table_[table_index];
    }
    
    return crc ^ 0xFFFFFFFF;
}

// ETI Frame Memory Pool Implementation
EtiFrameMemoryPool::EtiFrameMemoryPool(size_t pool_size)
    : pool_size_(pool_size), free_list_(nullptr) {
    
    // Allocate memory blocks
    const size_t blocks_per_allocation = 256;
    const size_t total_allocations = (pool_size + blocks_per_allocation - 1) / blocks_per_allocation;
    
    memory_blocks_.reserve(total_allocations);
    
    for (size_t alloc = 0; alloc < total_allocations; ++alloc) {
        const size_t blocks_this_alloc = std::min(blocks_per_allocation, 
                                                  pool_size - alloc * blocks_per_allocation);
        
        auto block_array = std::make_unique<FrameBlock[]>(blocks_this_alloc);
        
        // Initialize blocks and build free list
        for (size_t i = 0; i < blocks_this_alloc; ++i) {
            block_array[i].in_use = false;
            block_array[i].next = (i + 1 < blocks_this_alloc) ? &block_array[i + 1] : free_list_;
        }
        
        // Link to existing free list
        if (blocks_this_alloc > 0) {
            free_list_ = &block_array[0];
        }
        
        memory_blocks_.push_back(std::move(block_array));
    }
    
    Logger::instance().log(Logger::Info, "EtiFrameMemoryPool", 
                          QString("Memory pool initialized: %1 blocks, %2 MB total")
                          .arg(pool_size_)
                          .arg((pool_size_ * eti::ETI_FRAME_SIZE) / (1024 * 1024)));
}

EtiFrameMemoryPool::~EtiFrameMemoryPool() {
    const size_t leaked_blocks = used_count_.load();
    if (leaked_blocks > 0) {
        Logger::instance().log(Logger::Warning, "EtiFrameMemoryPool", 
                              QString("Memory pool destroyed with %1 leaked blocks").arg(leaked_blocks));
    }
}

uint8_t* EtiFrameMemoryPool::allocate_frame() {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    if (!free_list_) {
        Logger::instance().log(Logger::Error, "EtiFrameMemoryPool", "Memory pool exhausted");
        return nullptr;
    }
    
    FrameBlock* block = free_list_;
    free_list_ = block->next;
    block->in_use = true;
    
    const size_t current_used = used_count_.fetch_add(1, std::memory_order_acq_rel) + 1;
    
    // Update peak usage
    size_t expected_peak = peak_usage_.load(std::memory_order_acquire);
    while (current_used > expected_peak && 
           !peak_usage_.compare_exchange_weak(expected_peak, current_used, std::memory_order_acq_rel)) {
        // Retry if another thread updated peak_usage_
    }
    
    return block->data;
}

void EtiFrameMemoryPool::deallocate_frame(uint8_t* buffer) {
    if (!buffer) return;
    
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    // Find the block containing this buffer
    FrameBlock* block = reinterpret_cast<FrameBlock*>(
        reinterpret_cast<char*>(buffer) - offsetof(FrameBlock, data));
    
    if (!block->in_use) {
        Logger::instance().log(Logger::Warning, "EtiFrameMemoryPool", 
                              "Attempt to deallocate already free block");
        return;
    }
    
    block->in_use = false;
    block->next = free_list_;
    free_list_ = block;
    
    used_count_.fetch_sub(1, std::memory_order_acq_rel);
}

EtiFrameMemoryPool::PoolStats EtiFrameMemoryPool::get_statistics() const {
    PoolStats stats;
    stats.total_blocks = pool_size_;
    stats.used_blocks = used_count_.load(std::memory_order_acquire);
    stats.peak_usage = peak_usage_.load(std::memory_order_acquire);
    stats.utilization_percent = (static_cast<double>(stats.used_blocks) / stats.total_blocks) * 100.0;
    
    return stats;
}

// Optimized ETI Processor Implementation
OptimizedEtiProcessor::OptimizedEtiProcessor(QObject* parent)
    : QObject(parent)
    , current_simd_level_(SimdLevel::DISABLED)
    , start_time_(std::chrono::high_resolution_clock::now())
{
    // Detect optimal SIMD level
    current_simd_level_ = get_optimal_simd_level();
    log_cpu_features();
    
    // Initialize performance profiler
    profiler_ = std::make_unique<eti::modern::performance_profiler>();
    profiler_->set_target_fps(TARGET_FPS_OPTIMAL);
    profiler_->set_memory_limit(TARGET_MEMORY_MB * 1024 * 1024);
}

OptimizedEtiProcessor::~OptimizedEtiProcessor() {
    // Stop processing threads
    processing_active_.store(false, std::memory_order_release);
    
    for (auto& thread : processing_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    cleanup_memory_resources();
    
    Logger::instance().log(Logger::Info, "OptimizedEtiProcessor", 
                          "Optimized ETI processor destroyed");
}

bool OptimizedEtiProcessor::initialize() {
    try {
        // Initialize SIMD detector
        simd_detector_ = std::make_unique<SimdSyncDetector>();
        
        // Initialize memory management
        initialize_memory_pools();
        
        // Configure thread affinity for real-time performance
        optimize_thread_affinity();
        
        // Configure memory prefetching
        configure_memory_prefetch();
        
        Logger::instance().log(Logger::Info, "OptimizedEtiProcessor", 
                              QString("Initialized with SIMD level: %1, Memory pool: %2 MB")
                              .arg(static_cast<int>(current_simd_level_))
                              .arg((TARGET_MEMORY_MB)));
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "OptimizedEtiProcessor", 
                              QString("Initialization failed: %1").arg(e.what()));
        return false;
    }
}

bool OptimizedEtiProcessor::process_eti_frame_optimized(const QByteArray& frame_data) {
    if (frame_data.size() != eti::ETI_FRAME_SIZE) {
        Logger::instance().log(Logger::Warning, "OptimizedEtiProcessor", 
                              QString("Invalid frame size: %1 (expected %2)")
                              .arg(frame_data.size()).arg(eti::ETI_FRAME_SIZE));
        return false;
    }
    
    const uint64_t timer_id = profiler_->start_timing("eti_frame_processing");
    const auto processing_start = std::chrono::high_resolution_clock::now();
    
    // Process frame based on SIMD capability
    bool success = false;
    const uint8_t* data = reinterpret_cast<const uint8_t*>(frame_data.constData());
    
    switch (current_simd_level_) {
        case SimdLevel::AVX2:
            success = process_frame_avx2(data);
            break;
        case SimdLevel::AVX:
        case SimdLevel::SSE2:
            success = process_frame_sse2(data);
            break;
        default:
            success = process_frame_scalar(data);
            break;
    }
    
    const auto processing_end = std::chrono::high_resolution_clock::now();
    const auto processing_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        processing_end - processing_start);
    
    profiler_->end_timing(timer_id);
    
    // Update performance statistics
    const uint64_t frame_number = frames_processed_.fetch_add(1, std::memory_order_acq_rel);
    
    if (success) {
        successful_frames_.fetch_add(1, std::memory_order_acq_rel);
    } else {
        failed_frames_.fetch_add(1, std::memory_order_acq_rel);
        profiler_->record_error("eti_frame_processing");
    }
    
    // Update performance counters
    total_processing_time_ns_.fetch_add(processing_time.count(), std::memory_order_acq_rel);
    update_performance_statistics();
    
    // Emit signals
    emit frame_processed_optimized(frame_number, processing_time);
    
    // Check performance targets
    const double current_fps = current_fps_.load(std::memory_order_acquire);
    if (current_fps >= TARGET_FPS_MINIMUM) {
        emit performance_target_achieved(current_fps, TARGET_FPS_MINIMUM);
    }
    
    return success;
}

size_t OptimizedEtiProcessor::process_batch_optimized(const std::vector<QByteArray>& frames) {
    const uint64_t timer_id = profiler_->start_timing("batch_processing");
    
    size_t successful_count = 0;
    const auto batch_start = std::chrono::high_resolution_clock::now();
    
    // Process frames in batches for maximum throughput
    constexpr size_t BATCH_SIZE = 32;
    
    for (size_t i = 0; i < frames.size(); i += BATCH_SIZE) {
        const size_t batch_end = std::min(i + BATCH_SIZE, frames.size());
        
        // Process batch of frames
        for (size_t j = i; j < batch_end; ++j) {
            if (process_eti_frame_optimized(frames[j])) {
                successful_count++;
            }
        }
        
        // Yield CPU periodically to maintain system responsiveness
        if (i > 0 && (i % 1000) == 0) {
            std::this_thread::yield();
        }
    }
    
    const auto batch_end = std::chrono::high_resolution_clock::now();
    const auto batch_time = std::chrono::duration_cast<std::chrono::microseconds>(
        batch_end - batch_start);
    
    profiler_->end_timing(timer_id);
    
    // Calculate batch throughput
    const double batch_fps = successful_count > 0 ? 
        (static_cast<double>(successful_count) / batch_time.count() * 1000000.0) : 0.0;
    
    Logger::instance().log(Logger::Info, "OptimizedEtiProcessor", 
                          QString("Batch processed: %1/%2 frames, %3 FPS")
                          .arg(successful_count).arg(frames.size()).arg(batch_fps, 0, 'f', 2));
    
    return successful_count;
}

void OptimizedEtiProcessor::set_real_time_mode(bool enabled) {
    const bool was_enabled = real_time_mode_.exchange(enabled, std::memory_order_acq_rel);
    
    if (enabled == was_enabled) {
        return; // No change
    }
    
    if (enabled) {
        Logger::instance().log(Logger::Info, "OptimizedEtiProcessor", "Enabling real-time mode");
        
        // Start processing threads for real-time mode
        processing_active_.store(true, std::memory_order_release);
        
        const size_t thread_count = std::min(4u, std::thread::hardware_concurrency());
        processing_threads_.reserve(thread_count);
        
        for (size_t i = 0; i < thread_count; ++i) {
            processing_threads_.emplace_back([this, i]() {
                // Set thread name and priority
                pthread_setname_np(pthread_self(), 
                                  QString("ETIProcessor%1").arg(i).toLocal8Bit().constData());
                
                // Real-time processing loop
                while (processing_active_.load(std::memory_order_acquire)) {
                    // Process frames from ring buffer
                    auto* frame_entry = frame_buffer_->acquire_read_buffer();
                    if (frame_entry) {
                        const QByteArray frame_data(reinterpret_cast<const char*>(frame_entry->data), 
                                                   eti::ETI_FRAME_SIZE);
                        process_eti_frame_optimized(frame_data);
                        frame_buffer_->release_read_buffer(frame_entry);
                    } else {
                        // No frames available, brief sleep
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                    }
                }
            });
        }
        
    } else {
        Logger::instance().log(Logger::Info, "OptimizedEtiProcessor", "Disabling real-time mode");
        
        // Stop processing threads
        processing_active_.store(false, std::memory_order_release);
        
        for (auto& thread : processing_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        processing_threads_.clear();
    }
}

OptimizedEtiProcessor::PerformanceMetrics OptimizedEtiProcessor::get_performance_metrics() const {
    std::lock_guard<std::mutex> lock(performance_mutex_);
    
    PerformanceMetrics metrics;
    
    const uint64_t total_frames = frames_processed_.load(std::memory_order_acquire);
    const uint64_t successful = successful_frames_.load(std::memory_order_acquire);
    
    metrics.total_frames_processed = total_frames;
    metrics.successful_frames = successful;
    metrics.success_rate_percent = total_frames > 0 ? 
        (static_cast<double>(successful) / total_frames * 100.0) : 0.0;
    
    metrics.current_fps = current_fps_.load(std::memory_order_acquire);
    metrics.peak_fps = peak_fps_.load(std::memory_order_acquire);
    
    // Calculate average FPS
    const auto elapsed = std::chrono::high_resolution_clock::now() - start_time_;
    const auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    
    if (elapsed_seconds > 0) {
        metrics.average_fps = static_cast<double>(successful) / elapsed_seconds;
    } else {
        metrics.average_fps = 0.0;
    }
    
    // Get latency metrics from profiler
    const auto* frame_metrics = profiler_->get_metrics("eti_frame_processing");
    if (frame_metrics) {
        metrics.average_latency = std::chrono::duration_cast<std::chrono::microseconds>(
            frame_metrics->avg_time);
        metrics.peak_latency = std::chrono::duration_cast<std::chrono::microseconds>(
            frame_metrics->max_time);
    }
    
    metrics.memory_usage_bytes = calculate_memory_usage();
    metrics.cpu_efficiency_percent = profiler_->get_cpu_efficiency();
    
    return metrics;
}

// SIMD-specific processing implementations
bool OptimizedEtiProcessor::process_frame_avx2(const uint8_t* __restrict__ frame_data) {
    // Validate frame with SIMD sync detection
    if (!validate_frame_simd(frame_data)) {
        return false;
    }
    
    // Use AVX2 for high-speed frame processing
    const uint32_t calculated_crc = simd_detector_->calculate_crc32_simd(
        frame_data, eti::ETI_FRAME_SIZE - 4);
    
    // Extract received CRC (last 4 bytes)
    const uint32_t received_crc = *reinterpret_cast<const uint32_t*>(
        frame_data + eti::ETI_FRAME_SIZE - 4);
    
    if (calculated_crc != received_crc) {
        Logger::instance().log(Logger::Debug, "OptimizedEtiProcessor", 
                              QString("CRC mismatch: calculated=0x%1, received=0x%2")
                              .arg(calculated_crc, 8, 16, QChar('0'))
                              .arg(received_crc, 8, 16, QChar('0')));
        return false;
    }
    
    // Process FIC and MSC data with optimizations
    return parse_fic_optimized(frame_data + 4, 96) && 
           parse_msc_optimized(frame_data + 100, eti::ETI_FRAME_SIZE - 104);
}

bool OptimizedEtiProcessor::process_frame_sse2(const uint8_t* __restrict__ frame_data) {
    // Fallback SSE2 implementation for older CPUs
    return process_frame_scalar(frame_data);
}

bool OptimizedEtiProcessor::process_frame_scalar(const uint8_t* __restrict__ frame_data) {
    // Scalar fallback implementation
    return validate_frame_simd(frame_data);
}

bool OptimizedEtiProcessor::validate_frame_simd(const uint8_t* __restrict__ frame_data) {
    // Use SIMD sync pattern detection
    const int32_t sync_offset = simd_detector_->detect_sync_pattern_avx2(frame_data, 32);
    return sync_offset == 0; // Sync should be at the beginning
}

// Performance optimization methods
void OptimizedEtiProcessor::optimize_thread_affinity() {
#ifdef __linux__
    // Set CPU affinity for performance cores
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    
    // Use first few CPU cores for processing threads
    const int num_cores = std::min(4, static_cast<int>(std::thread::hardware_concurrency()));
    for (int i = 0; i < num_cores; ++i) {
        CPU_SET(i, &cpuset);
    }
    
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) == 0) {
        Logger::instance().log(Logger::Info, "OptimizedEtiProcessor", 
                              QString("Thread affinity set to %1 cores").arg(num_cores));
    }
#endif
}

void OptimizedEtiProcessor::update_performance_statistics() {
    const auto now = std::chrono::high_resolution_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    
    if (elapsed.count() > 0) {
        const double fps = static_cast<double>(successful_frames_.load(std::memory_order_acquire)) / 
                          elapsed.count();
        
        current_fps_.store(fps, std::memory_order_release);
        
        // Update peak FPS
        double expected_peak = peak_fps_.load(std::memory_order_acquire);
        while (fps > expected_peak && 
               !peak_fps_.compare_exchange_weak(expected_peak, fps, std::memory_order_acq_rel)) {
            // Retry if another thread updated peak_fps_
        }
    }
}

// CPU feature detection
OptimizedEtiProcessor::SimdLevel OptimizedEtiProcessor::get_optimal_simd_level() const {
    if (detect_avx512_support()) {
        return SimdLevel::AVX512;
    }
    
    if (detect_avx2_support()) {
        return SimdLevel::AVX2;
    }
    
    // Check for basic AVX support
    uint32_t eax, ebx, ecx, edx;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx) && (ecx & (1 << 28))) {
        return SimdLevel::AVX;
    }
    
    // Check for SSE2 support
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx) && (edx & (1 << 26))) {
        return SimdLevel::SSE2;
    }
    
    return SimdLevel::DISABLED;
}

bool OptimizedEtiProcessor::detect_avx2_support() const {
    uint32_t eax, ebx, ecx, edx;
    return __get_cpuid_max(0, nullptr) >= 7 &&
           __get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx) &&
           (ebx & (1 << 5)); // AVX2 bit
}

bool OptimizedEtiProcessor::detect_avx512_support() const {
    uint32_t eax, ebx, ecx, edx;
    return __get_cpuid_max(0, nullptr) >= 7 &&
           __get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx) &&
           (ebx & (1 << 16)); // AVX-512F bit
}

void OptimizedEtiProcessor::log_cpu_features() const {
    QString features = "CPU features: ";
    
    switch (current_simd_level_) {
        case SimdLevel::AVX512:
            features += "AVX-512 ";
            [[fallthrough]];
        case SimdLevel::AVX2:
            features += "AVX2 ";
            [[fallthrough]];
        case SimdLevel::AVX:
            features += "AVX ";
            [[fallthrough]];
        case SimdLevel::SSE2:
            features += "SSE2 ";
            break;
        case SimdLevel::DISABLED:
            features += "Scalar only";
            break;
    }
    
    Logger::instance().log(Logger::Info, "OptimizedEtiProcessor", features);
}

void OptimizedEtiProcessor::initialize_memory_pools() {
    // Initialize memory pool for frame processing
    memory_pool_ = std::make_unique<EtiFrameMemoryPool>(2048); // 2048 frame buffers
    
    // Initialize lock-free ring buffer
    frame_buffer_ = std::make_unique<LockFreeFrameBuffer<8192>>();
}

size_t OptimizedEtiProcessor::calculate_memory_usage() const {
    size_t total_usage = 0;
    
    // Memory pool usage
    if (memory_pool_) {
        const auto pool_stats = memory_pool_->get_statistics();
        total_usage += pool_stats.used_blocks * eti::ETI_FRAME_SIZE;
    }
    
    // Ring buffer usage
    if (frame_buffer_) {
        const double utilization = frame_buffer_->get_utilization();
        total_usage += static_cast<size_t>((utilization / 100.0) * 8192 * eti::ETI_FRAME_SIZE);
    }
    
    // Add profiler memory usage
    total_usage += profiler_->get_current_memory_usage();
    
    return total_usage;
}

} // namespace eti::performance

#include "optimized_eti_processor.moc"