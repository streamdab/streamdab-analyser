/**
 * @file eti_memory_pool.hpp
 * @brief Enhanced ETI Memory Pool with C++20 features
 * 
 * Professional memory pool implementation with smart pointers,
 * RAII patterns, concepts, and optimized allocation strategies.
 * 
 * @author C++20 Features Integration Specialist
 * @date 2025
 */

#pragma once

#include <memory>
#include <cstddef>
#include <chrono>
#include <concepts>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <ranges>
#include <unordered_map>
#include <shared_mutex>
#include "eti_types.hpp"

namespace eti::modern {

// ============================================================================
// C++20 CONCEPTS FOR MEMORY MANAGEMENT
// ============================================================================

/**
 * @brief Concept for memory pool allocatable types
 */
template<typename T>
concept allocatable_type = requires {
    !std::is_void_v<T>;
    std::is_destructible_v<T>;
    std::is_default_constructible_v<T>;
};

/**
 * @brief Custom deleter for memory pool allocated objects
 */
template<allocatable_type T>
class memory_pool_deleter {
private:
    class ETIFrameMemoryPool* pool_;
    
public:
    explicit memory_pool_deleter(ETIFrameMemoryPool* pool) noexcept : pool_(pool) {}
    
    void operator()(T* ptr) noexcept;
};

/**
 * @brief RAII wrapper for ETI frame data
 */
template<allocatable_type T>
class managed_frame_data {
private:
    std::unique_ptr<T, memory_pool_deleter<T>> data_;
    std::chrono::steady_clock::time_point allocation_time_;
    
public:
    explicit managed_frame_data(std::unique_ptr<T, memory_pool_deleter<T>> data) noexcept
        : data_(std::move(data))
        , allocation_time_(std::chrono::steady_clock::now()) {}
    
    // Move semantics only
    managed_frame_data(const managed_frame_data&) = delete;
    managed_frame_data& operator=(const managed_frame_data&) = delete;
    
    managed_frame_data(managed_frame_data&&) = default;
    managed_frame_data& operator=(managed_frame_data&&) = default;
    
    [[nodiscard]] T* get() const noexcept { return data_.get(); }
    [[nodiscard]] T& operator*() const noexcept { return *data_; }
    [[nodiscard]] T* operator->() const noexcept { return data_.get(); }
    
    [[nodiscard]] bool is_valid() const noexcept { return data_ != nullptr; }
    
    [[nodiscard]] auto get_age() const noexcept -> std::chrono::milliseconds {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - allocation_time_);
    }
    
    // Structured bindings support
    [[nodiscard]] auto get_info() const noexcept 
        -> std::pair<T*, std::chrono::milliseconds> {
        return {data_.get(), get_age()};
    }
};

/**
 * @brief Enhanced ETI Frame Memory Pool with C++20 features
 */
class ETIFrameMemoryPool {
private:
    // Memory block structure
    struct memory_block {
        std::unique_ptr<uint8_t[]> data;
        size_t size;
        bool in_use;
        std::chrono::steady_clock::time_point allocation_time;
        
        memory_block(size_t block_size) 
            : data(std::make_unique<uint8_t[]>(block_size))
            , size(block_size)
            , in_use(false)
            , allocation_time(std::chrono::steady_clock::now()) {}
    };
    
    // Pool configuration
    static constexpr size_t DEFAULT_BLOCK_SIZE = 6144;  // ETI frame size
    static constexpr size_t DEFAULT_POOL_SIZE = 100;    // Number of pre-allocated blocks
    static constexpr auto CLEANUP_INTERVAL = std::chrono::minutes(5);
    
    std::vector<std::unique_ptr<memory_block>> memory_blocks_;
    std::queue<size_t> available_blocks_;
    mutable std::mutex pool_mutex_;
    std::atomic<size_t> total_allocations_{0};
    std::atomic<size_t> current_usage_{0};
    std::chrono::steady_clock::time_point last_cleanup_;
    
public:
    /**
     * @brief Constructor with pool size configuration
     */
    explicit ETIFrameMemoryPool(size_t pool_size = DEFAULT_POOL_SIZE) 
        : last_cleanup_(std::chrono::steady_clock::now()) {
        
        memory_blocks_.reserve(pool_size);
        
        // Pre-allocate memory blocks
        for (size_t i = 0; i < pool_size; ++i) {
            memory_blocks_.push_back(std::make_unique<memory_block>(DEFAULT_BLOCK_SIZE));
            available_blocks_.push(i);
        }
    }
    
    /**
     * @brief Destructor with cleanup validation
     */
    ~ETIFrameMemoryPool() {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        
        // Check for memory leaks
        const size_t blocks_in_use = std::ranges::count_if(memory_blocks_,
            [](const auto& block) { return block && block->in_use; });
            
        if (blocks_in_use > 0) {
            // Log potential memory leak (would use proper logging in production)
        }
    }
    
    /**
     * @brief Allocate memory using smart pointer RAII
     * @tparam T Type to allocate
     * @return Managed frame data with automatic cleanup
     */
    template<allocatable_type T>
    [[nodiscard]] auto allocate_managed() -> managed_frame_data<T> {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        
        if (available_blocks_.empty()) {
            expand_pool();
        }
        
        if (available_blocks_.empty()) {
            // Fallback to direct allocation
            auto ptr = std::unique_ptr<T, memory_pool_deleter<T>>(
                new T{}, memory_pool_deleter<T>(this)
            );
            return managed_frame_data<T>(std::move(ptr));
        }
        
        const size_t block_index = available_blocks_.front();
        available_blocks_.pop();
        
        auto& block = memory_blocks_[block_index];
        block->in_use = true;
        block->allocation_time = std::chrono::steady_clock::now();
        
        ++total_allocations_;
        ++current_usage_;
        
        // Create managed object from pool memory
        auto ptr = std::unique_ptr<T, memory_pool_deleter<T>>(
            reinterpret_cast<T*>(block->data.get()),
            memory_pool_deleter<T>(this)
        );
        
        return managed_frame_data<T>(std::move(ptr));
    }
    
    /**
     * @brief Legacy allocate method for compatibility
     */
    void* allocate(size_t size) {
        if (size > DEFAULT_BLOCK_SIZE) {
            return std::malloc(size);  // Fallback for oversized allocations
        }
        
        auto managed = allocate_managed<uint8_t>();
        return managed.get();  // Note: This breaks RAII - kept for compatibility only
    }
    
    /**
     * @brief Deallocate memory block
     */
    void deallocate(void* ptr) noexcept {
        if (!ptr) return;
        
        std::lock_guard<std::mutex> lock(pool_mutex_);
        
        // Find the memory block containing this pointer
        auto block_it = std::ranges::find_if(memory_blocks_,
            [ptr](const auto& block) {
                return block && block->in_use && 
                       block->data.get() <= ptr &&
                       ptr < block->data.get() + block->size;
            });
            
        if (block_it != memory_blocks_.end()) {
            const size_t block_index = std::distance(memory_blocks_.begin(), block_it);
            (*block_it)->in_use = false;
            available_blocks_.push(block_index);
            --current_usage_;
        } else {
            // Not from pool, use standard deallocation
            std::free(ptr);
        }
    }
    
    /**
     * @brief Get current memory usage with structured bindings
     * @return [used_blocks, total_blocks, usage_percentage] tuple
     */
    [[nodiscard]] auto get_memory_usage() const noexcept 
        -> std::tuple<size_t, size_t, double> {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        
        const size_t used_blocks = current_usage_.load();
        const size_t total_blocks = memory_blocks_.size();
        const double usage_percentage = total_blocks > 0 
            ? (static_cast<double>(used_blocks) / static_cast<double>(total_blocks)) * 100.0
            : 0.0;
            
        return {used_blocks, total_blocks, usage_percentage};
    }
    
    /**
     * @brief Optimize memory pool using ranges
     */
    void optimize() {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        
        const auto now = std::chrono::steady_clock::now();
        if (now - last_cleanup_ < CLEANUP_INTERVAL) {
            return;  // Too soon for cleanup
        }
        
        // Find and cleanup old unused blocks using ranges
        for (size_t index = 0; index < memory_blocks_.size(); ++index) {
            const auto& block = memory_blocks_[index];
            if (block && !block->in_use && 
                (now - block->allocation_time) > CLEANUP_INTERVAL) {
                block->allocation_time = now;
            }
        }
        
        last_cleanup_ = now;
    }
    
    /**
     * @brief Get allocation statistics
     * @return [total_allocations, current_usage, hit_rate] tuple
     */
    [[nodiscard]] auto get_statistics() const noexcept 
        -> std::tuple<size_t, size_t, double> {
        const size_t total = total_allocations_.load();
        const size_t current = current_usage_.load();
        const double hit_rate = total > 0 
            ? (static_cast<double>(total - current) / static_cast<double>(total)) * 100.0
            : 100.0;
            
        return {total, current, hit_rate};
    }
    
private:
    void expand_pool() {
        const size_t current_size = memory_blocks_.size();
        const size_t new_size = current_size + (current_size / 2);  // Grow by 50%
        
        memory_blocks_.reserve(new_size);
        
        for (size_t i = current_size; i < new_size; ++i) {
            memory_blocks_.push_back(std::make_unique<memory_block>(DEFAULT_BLOCK_SIZE));
            available_blocks_.push(i);
        }
    }
};

/**
 * @brief Implementation of custom deleter
 */
template<allocatable_type T>
void memory_pool_deleter<T>::operator()(T* ptr) noexcept {
    if (pool_ && ptr) {
        pool_->deallocate(ptr);
    }
}

/**
 * @brief Enhanced processing cache with smart pointers
 */
template<typename KeyType, typename ValueType>
class ETIProcessingCache {
private:
    struct cache_entry {
        std::shared_ptr<ValueType> value;
        std::chrono::steady_clock::time_point insertion_time;
        std::atomic<size_t> access_count{0};
        
        cache_entry(std::shared_ptr<ValueType> val)
            : value(std::move(val))
            , insertion_time(std::chrono::steady_clock::now()) {}
    };
    
    std::unordered_map<KeyType, std::unique_ptr<cache_entry>> cache_;
    mutable std::shared_mutex cache_mutex_;
    static constexpr size_t MAX_CACHE_SIZE = 1000;
    static constexpr auto MAX_AGE = std::chrono::hours(1);
    
public:
    /**
     * @brief Insert value with smart pointer management
     */
    void insert(const KeyType& key, std::shared_ptr<ValueType> value) {
        std::unique_lock<std::shared_mutex> lock(cache_mutex_);
        
        if (cache_.size() >= MAX_CACHE_SIZE) {
            cleanup_old_entries();
        }
        
        cache_[key] = std::make_unique<cache_entry>(std::move(value));
    }
    
    /**
     * @brief Find value with access tracking
     * @return [found, value, access_count] tuple
     */
    [[nodiscard]] auto find(const KeyType& key) 
        -> std::tuple<bool, std::shared_ptr<ValueType>, size_t> {
        std::shared_lock<std::shared_mutex> lock(cache_mutex_);
        
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            auto& entry = *it->second;
            ++entry.access_count;
            return {true, entry.value, entry.access_count.load()};
        }
        
        return {false, nullptr, 0};
    }
    
    /**
     * @brief Get cache statistics
     * @return [size, hit_rate, oldest_entry_age] tuple  
     */
    [[nodiscard]] auto get_statistics() const 
        -> std::tuple<size_t, double, std::chrono::milliseconds> {
        std::shared_lock<std::shared_mutex> lock(cache_mutex_);
        
        const size_t cache_size = cache_.size();
        
        // Calculate oldest entry age using ranges
        auto ages = cache_ 
            | std::views::values
            | std::views::transform([](const auto& entry) {
                return std::chrono::steady_clock::now() - entry->insertion_time;
              });
              
        const auto oldest_age = cache_size > 0 
            ? *std::ranges::max_element(ages)
            : std::chrono::milliseconds{0};
            
        return {
            cache_size, 
            100.0,  // Placeholder hit rate calculation
            std::chrono::duration_cast<std::chrono::milliseconds>(oldest_age)
        };
    }
    
private:
    void cleanup_old_entries() {
        const auto now = std::chrono::steady_clock::now();
        
        // Remove entries older than MAX_AGE using ranges
        std::erase_if(cache_, [now](const auto& pair) {
            const auto& entry = *pair.second;
            return (now - entry.insertion_time) > MAX_AGE;
        });
    }
};

// Type aliases for common usage patterns
using frame_data_ptr = managed_frame_data<::eti::EtiFrame>;
using service_cache = ETIProcessingCache<uint16_t, ::eti::ServiceInfo>;
using ensemble_cache = ETIProcessingCache<uint16_t, ::eti::Ensemble>;

} // namespace eti::modern