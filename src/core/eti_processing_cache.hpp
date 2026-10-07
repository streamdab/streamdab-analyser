/**
 * @file eti_processing_cache.h
 * @brief ETI Processing Cache for optimized frame analysis
 * 
 * High-performance caching system for ETI frame analysis results to improve
 * processing speed and reduce redundant computations in the Modern ETI Core Engine.
 * 
 * @author Build Manager Agent
 * @date 2025
 * @copyright StreamDAB Analyser Project
 */

#pragma once

#include "eti_types.hpp"
#include <unordered_map>
#include <memory>
#include <mutex>
#include <chrono>
#include <span>

namespace eti::modern {
    struct ETIParseResult;
}

namespace eti::modern {

/**
 * @brief Cache entry for ETI processing results
 */
struct CacheEntry {
    eti::EtiFrame cached_frame;
    std::chrono::steady_clock::time_point cache_time;
    size_t access_count{0};
    bool is_valid{true};
};

/**
 * @brief High-performance cache for ETI processing results
 * 
 * Provides LRU-based caching of ETI frame analysis results to eliminate
 * redundant processing during real-time operations.
 */
class ETIProcessingCache {
public:
    ETIProcessingCache();
    explicit ETIProcessingCache(size_t cache_size_mb);
    ~ETIProcessingCache();

    /**
     * @brief Cache processed ETI frame
     * @param frame_number Frame number identifier
     * @param frame Processed ETI frame data
     */
    void cacheFrame(uint32_t frame_number, const eti::EtiFrame& frame);

    /**
     * @brief Retrieve cached ETI frame
     * @param frame_number Frame number identifier
     * @return Cached frame data or nullptr if not found
     */
    std::shared_ptr<eti::EtiFrame> getCachedFrame(uint32_t frame_number);

    /**
     * @brief Check if frame is cached
     * @param frame_number Frame number identifier
     * @return true if frame is cached and valid
     */
    bool isFrameCached(uint32_t frame_number) const;

    /**
     * @brief Clear all cached frames
     */
    void clearCache();

    /**
     * @brief Get cache statistics
     * @return Cache performance statistics
     */
    struct CacheStats {
        size_t total_requests{0};
        size_t cache_hits{0};
        size_t cache_misses{0};
        double hit_ratio{0.0};
        size_t cached_frames{0};
    };
    
    CacheStats getStatistics() const;

    /**
     * @brief Set maximum cache size
     * @param max_size Maximum number of cached frames
     */
    void setMaxCacheSize(size_t max_size);

    /**
     * @brief Lookup frame data in cache
     * @param frame_data Raw frame data for lookup
     * @return Cached result or nullptr if not found
     */
    std::shared_ptr<eti::EtiFrame> lookup(std::span<const uint8_t, 6144> frame_data);

    /**
     * @brief Store frame analysis result in cache
     * @param frame_data Raw frame data as key
     * @param result Analysis result to store
     */
    void store(std::span<const uint8_t, 6144> frame_data, const eti::modern::ETIParseResult& result);

    /**
     * @brief Clear cache statistics
     */
    void clearStatistics();

private:
    std::unordered_map<uint32_t, std::unique_ptr<CacheEntry>> m_cache;
    mutable std::mutex m_cache_mutex;
    
    size_t m_max_cache_size{1000};
    mutable size_t m_total_requests{0};
    mutable size_t m_cache_hits{0};
    mutable size_t m_cache_misses{0};

    void evictOldestEntries();
};

} // namespace eti::modern