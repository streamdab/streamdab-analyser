/**
 * @file eti_processing_cache.h
 * @brief High-performance caching system for ETI processing results
 * 
 * Optimized caching to reduce redundant ETI frame processing and improve
 * performance. Part of Phase 2 migration architecture.
 */

#pragma once

#include "eti_types.h"
#include "modern_eti_frame_parser.h"
#include <unordered_map>
#include <chrono>
#include <optional>
#include <shared_mutex>
#include <span>

namespace eti::modern {

/**
 * @brief Cache entry for processed ETI frames
 */
struct CacheEntry {
    ETIParseResult result;
    std::chrono::steady_clock::time_point creation_time;
    std::chrono::steady_clock::time_point last_access;
    uint32_t access_count{1};
    
    CacheEntry(ETIParseResult&& res) 
        : result(std::move(res))
        , creation_time(std::chrono::steady_clock::now())
        , last_access(creation_time)
    {}
};

/**
 * @brief Cache statistics for monitoring performance
 */
struct CacheStatistics {
    std::atomic<uint64_t> hits{0};
    std::atomic<uint64_t> misses{0};
    std::atomic<uint64_t> evictions{0};
    std::atomic<uint64_t> total_requests{0};
    std::atomic<size_t> current_size{0};
    std::atomic<size_t> memory_usage{0};
    
    double get_hit_rate() const {
        uint64_t total = total_requests.load();
        return total > 0 ? (static_cast<double>(hits.load()) / total) : 0.0;
    }
};

/**
 * @brief ETI Processing Cache with LRU eviction
 * 
 * High-performance cache for ETI frame processing results with
 * automatic memory management and performance optimization.
 */
class ETIProcessingCache {
public:
    explicit ETIProcessingCache(size_t max_memory_mb = 64);
    ~ETIProcessingCache();

    /**
     * @brief Lookup cached result for ETI frame
     * @param frame_data Raw ETI frame data
     * @return Cached result if found, nullopt otherwise
     */
    std::optional<ETIParseResult> lookup(std::span<const uint8_t, ETI_FRAME_SIZE> frame_data);

    /**
     * @brief Store processing result in cache
     * @param frame_data Raw ETI frame data (used as key)
     * @param result Processing result to cache
     */
    void store(std::span<const uint8_t, ETI_FRAME_SIZE> frame_data, const ETIParseResult& result);

    /**
     * @brief Get current cache statistics
     */
    CacheStatistics getStatistics() const { return stats_; }

    /**
     * @brief Clear all cache statistics
     */
    void clearStatistics();

    /**
     * @brief Clear all cached entries
     */
    void clear();

    /**
     * @brief Optimize cache for current usage patterns
     */
    void optimize();

    /**
     * @brief Get current memory usage in bytes
     */
    size_t getCurrentMemoryUsage() const { return stats_.memory_usage.load(); }

    /**
     * @brief Set maximum cache size
     */
    void setMaxMemorySize(size_t max_memory_mb);

private:
    using FrameHash = std::array<uint8_t, 32>; // SHA-256 hash
    using CacheMap = std::unordered_map<FrameHash, std::unique_ptr<CacheEntry>>;

    mutable std::shared_mutex cache_mutex_;
    CacheMap cache_map_;
    const size_t max_memory_bytes_;
    mutable CacheStatistics stats_;
    
    // LRU tracking
    struct LRUNode {
        FrameHash key;
        std::chrono::steady_clock::time_point last_access;
        
        bool operator<(const LRUNode& other) const {
            return last_access < other.last_access;
        }
    };
    
    std::vector<LRUNode> lru_list_;
    
    // Cache management
    FrameHash calculateFrameHash(std::span<const uint8_t, ETI_FRAME_SIZE> data) const;
    void evictLRUEntries(size_t target_size);
    void updateLRU(const FrameHash& key);
    size_t estimateEntrySize(const ETIParseResult& result) const;
    void cleanup();
};

} // namespace eti::modern