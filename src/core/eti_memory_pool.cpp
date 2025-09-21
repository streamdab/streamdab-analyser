/**
 * @file eti_memory_pool.cpp
 * @brief ETI Frame Memory Pool Implementation
 */

#include "eti_memory_pool.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>

namespace eti::modern {

ETIFrameMemoryPool::ETIFrameMemoryPool(size_t max_memory_bytes)
    : max_memory_bytes_(max_memory_bytes)
    , block_size_(eti::ETI_FRAME_SIZE)
{
    // Pre-allocate initial pool
    size_t initial_blocks = std::min(static_cast<size_t>(100), max_memory_bytes_ / block_size_);
    memory_blocks_.reserve(initial_blocks * 2); // Reserve space for growth
    free_indices_.reserve(initial_blocks * 2);
    
    // Create initial memory blocks
    for (size_t i = 0; i < initial_blocks; ++i) {
        if (!expandPool()) {
            break;
        }
    }
    
    Logger::instance().log(Logger::Info, "ETIFrameMemoryPool", 
                          QString("Memory pool initialized: %1 blocks, %2 KB total")
                          .arg(memory_blocks_.size())
                          .arg((memory_blocks_.size() * block_size_) / 1024));
}

ETIFrameMemoryPool::~ETIFrameMemoryPool() {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    size_t active_blocks = 0;
    for (const auto& block : memory_blocks_) {
        if (block.in_use) {
            active_blocks++;
        }
    }
    
    if (active_blocks > 0) {
        Logger::instance().log(Logger::Warning, "ETIFrameMemoryPool", 
                              QString("Memory pool destroyed with %1 active allocations")
                              .arg(active_blocks));
    }
    
    Logger::instance().log(Logger::Info, "ETIFrameMemoryPool", 
                          QString("Memory pool destroyed - Stats: %1 allocs, %2 deallocs, %.1f%% hit rate")
                          .arg(stats_.total_allocations.load())
                          .arg(stats_.total_deallocations.load())
                          .arg(stats_.get_hit_rate() * 100.0));
}

uint8_t* ETIFrameMemoryPool::allocate() {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    size_t block_index = findFreeBlock();
    
    if (block_index == SIZE_MAX) {
        // No free blocks, try to expand pool
        if (!expandPool()) {
            stats_.pool_misses.fetch_add(1);
            Logger::instance().log(Logger::Warning, "ETIFrameMemoryPool", 
                                  "Pool allocation failed - maximum memory reached");
            return nullptr;
        }
        block_index = memory_blocks_.size() - 1;
    }
    
    // Mark block as in use
    memory_blocks_[block_index].in_use = true;
    memory_blocks_[block_index].last_used = std::chrono::steady_clock::now();
    
    // Remove from free list
    auto it = std::find(free_indices_.begin(), free_indices_.end(), block_index);
    if (it != free_indices_.end()) {
        free_indices_.erase(it);
    }
    
    stats_.total_allocations.fetch_add(1);
    stats_.active_allocations.fetch_add(1);
    stats_.pool_hits.fetch_add(1);
    
    size_t current_usage = stats_.active_allocations.load() * block_size_;
    stats_.current_memory_usage.store(current_usage);
    
    // Update peak usage
    size_t peak = stats_.peak_memory_usage.load();
    while (current_usage > peak && 
           !stats_.peak_memory_usage.compare_exchange_weak(peak, current_usage)) {
        // Retry if another thread updated peak
    }
    
    return memory_blocks_[block_index].data.get();
}

void ETIFrameMemoryPool::deallocate(uint8_t* buffer) {
    if (!buffer) return;
    
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    // Find the block that owns this buffer
    for (size_t i = 0; i < memory_blocks_.size(); ++i) {
        if (memory_blocks_[i].data.get() == buffer) {
            if (memory_blocks_[i].in_use) {
                memory_blocks_[i].in_use = false;
                memory_blocks_[i].last_used = std::chrono::steady_clock::now();
                free_indices_.push_back(i);
                
                stats_.total_deallocations.fetch_add(1);
                stats_.active_allocations.fetch_sub(1);
                stats_.current_memory_usage.store(stats_.active_allocations.load() * block_size_);
                
                return;
            } else {
                Logger::instance().log(Logger::Warning, "ETIFrameMemoryPool", 
                                      "Attempted to deallocate already free buffer");
                return;
            }
        }
    }
    
    Logger::instance().log(Logger::Error, "ETIFrameMemoryPool", 
                          "Attempted to deallocate unknown buffer");
}

bool ETIFrameMemoryPool::hasAvailableBuffers() const {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    return !free_indices_.empty() || 
           (memory_blocks_.size() * block_size_ < max_memory_bytes_);
}

void ETIFrameMemoryPool::optimizePool() {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    const auto cleanup_threshold = std::chrono::minutes(5);
    
    // Remove old unused blocks
    auto it = std::remove_if(free_indices_.begin(), free_indices_.end(),
        [this, now, cleanup_threshold](size_t index) {
            if (index < memory_blocks_.size()) {
                auto& block = memory_blocks_[index];
                return !block.in_use && 
                       (now - block.last_used) > cleanup_threshold;
            }
            return false;
        });
    
    if (it != free_indices_.end()) {
        size_t removed_count = std::distance(it, free_indices_.end());
        free_indices_.erase(it, free_indices_.end());
        
        Logger::instance().log(Logger::Debug, "ETIFrameMemoryPool", 
                              QString("Pool optimization: removed %1 old blocks")
                              .arg(removed_count));
    }
}

void ETIFrameMemoryPool::compact() {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    // Remove all unused blocks beyond minimum pool size
    const size_t min_pool_size = 10;
    
    if (memory_blocks_.size() > min_pool_size) {
        // Sort free indices in descending order for safe removal
        std::sort(free_indices_.rbegin(), free_indices_.rend());
        
        size_t removed = 0;
        for (auto it = free_indices_.begin(); 
             it != free_indices_.end() && memory_blocks_.size() > min_pool_size; ) {
            size_t index = *it;
            if (index < memory_blocks_.size() && !memory_blocks_[index].in_use) {
                memory_blocks_.erase(memory_blocks_.begin() + index);
                it = free_indices_.erase(it);
                removed++;
                
                // Adjust remaining indices
                for (auto& idx : free_indices_) {
                    if (idx > index) {
                        idx--;
                    }
                }
            } else {
                ++it;
            }
        }
        
        Logger::instance().log(Logger::Info, "ETIFrameMemoryPool", 
                              QString("Pool compacted: removed %1 blocks, %2 remaining")
                              .arg(removed).arg(memory_blocks_.size()));
    }
}

bool ETIFrameMemoryPool::expandPool() {
    // Check if we can allocate more memory
    size_t current_size = memory_blocks_.size() * block_size_;
    if (current_size + block_size_ > max_memory_bytes_) {
        return false;
    }
    
    try {
        MemoryBlock new_block;
        new_block.data = std::make_unique<uint8_t[]>(block_size_);
        new_block.in_use = false;
        new_block.last_used = std::chrono::steady_clock::now();
        
        size_t new_index = memory_blocks_.size();
        memory_blocks_.push_back(std::move(new_block));
        free_indices_.push_back(new_index);
        
        return true;
    }
    catch (const std::bad_alloc&) {
        Logger::instance().log(Logger::Error, "ETIFrameMemoryPool", 
                              "Failed to expand pool - out of memory");
        return false;
    }
}

void ETIFrameMemoryPool::shrinkPool() {
    // Remove unused blocks from the end
    while (!memory_blocks_.empty() && !memory_blocks_.back().in_use) {
        size_t last_index = memory_blocks_.size() - 1;
        
        // Remove from free indices
        auto it = std::find(free_indices_.begin(), free_indices_.end(), last_index);
        if (it != free_indices_.end()) {
            free_indices_.erase(it);
        }
        
        memory_blocks_.pop_back();
    }
}

size_t ETIFrameMemoryPool::findFreeBlock() {
    if (!free_indices_.empty()) {
        return free_indices_.front();
    }
    
    // Linear search as fallback
    for (size_t i = 0; i < memory_blocks_.size(); ++i) {
        if (!memory_blocks_[i].in_use) {
            return i;
        }
    }
    
    return SIZE_MAX; // No free block found
}

} // namespace eti::modern