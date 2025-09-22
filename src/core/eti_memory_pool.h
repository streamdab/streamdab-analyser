/**
 * @file eti_memory_pool.h
 * @brief High-performance memory pool for ETI frame processing
 * 
 * Optimized memory management for zero-copy ETI processing with
 * memory usage reduction targets. Part of Phase 2 migration
 * to integrated C++20 architecture.
 */

#pragma once

#include "eti_types.h"
#include <memory>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>

namespace eti::modern {

/**
 * @brief Memory pool statistics for monitoring
 */
struct MemoryPoolStats {
    size_t total_allocations{0};
    size_t total_deallocations{0};
    size_t active_allocations{0};
    size_t pool_hits{0};
    size_t pool_misses{0};
    size_t peak_memory_usage{0};
    size_t current_memory_usage{0};
    
    double get_hit_rate() const {
        return (pool_hits + pool_misses > 0) ? (static_cast<double>(pool_hits) / (pool_hits + pool_misses)) : 0.0;
    }
};

/**
 * @brief High-performance memory pool for ETI frames
 * 
 * Provides pre-allocated memory blocks optimized for ETI frame processing
 * with automatic memory management and performance monitoring.
 */
class ETIFrameMemoryPool {
public:
    explicit ETIFrameMemoryPool(size_t max_memory_bytes = 64 * 1024 * 1024);
    ~ETIFrameMemoryPool();

    /**
     * @brief Allocate ETI frame buffer from pool
     * @return Pointer to ETI frame buffer or nullptr if allocation failed
     */
    uint8_t* allocate();

    /**
     * @brief Return ETI frame buffer to pool
     * @param buffer Buffer to deallocate
     */
    void deallocate(uint8_t* buffer);

    /**
     * @brief Get current memory usage statistics
     */
    MemoryPoolStats getStats() const { return stats_; }

    /**
     * @brief Get current memory usage in bytes
     */
    size_t getCurrentMemoryUsage() const { 
        std::lock_guard<std::mutex> lock(pool_mutex_);
        return stats_.current_memory_usage; 
    }

    /**
     * @brief Check if pool has available buffers
     */
    bool hasAvailableBuffers() const;

    /**
     * @brief Optimize pool for current usage patterns
     */
    void optimizePool();

    /**
     * @brief Clear all unused buffers to reduce memory usage
     */
    void compact();

private:
    struct MemoryBlock {
        std::unique_ptr<uint8_t[]> data;
        bool in_use;
        std::chrono::steady_clock::time_point last_used;
        
        MemoryBlock() : in_use(false), last_used(std::chrono::steady_clock::now()) {}
    };

    mutable std::mutex pool_mutex_;
    std::vector<MemoryBlock> memory_blocks_;
    std::vector<size_t> free_indices_;
    const size_t max_memory_bytes_;
    const size_t block_size_;
    mutable MemoryPoolStats stats_;
    
    // Pool management
    bool expandPool();
    void shrinkPool();
    size_t findFreeBlock();
};

} // namespace eti::modern