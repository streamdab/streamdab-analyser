/**
 * @file eti_processing_cache.cpp
 * @brief ETI Processing Cache implementation
 * 
 * @author Build Manager Agent
 * @date 2025
 * @copyright StreamDAB Analyser Project
 */

#include "eti_processing_cache.hpp"
#include <algorithm>

namespace eti::modern {

ETIProcessingCache::ETIProcessingCache() {
    m_cache.reserve(m_max_cache_size);
}

ETIProcessingCache::ETIProcessingCache(size_t cache_size_mb) {
    // Convert MB to number of cache entries (estimate ~1KB per entry)
    m_max_cache_size = cache_size_mb * 1024;
    m_cache.reserve(m_max_cache_size);
}

ETIProcessingCache::~ETIProcessingCache() {
    clearCache();
}

void ETIProcessingCache::cacheFrame(uint32_t frame_number, const eti::EtiFrame& frame) {
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    
    // Check if we need to evict old entries
    if (m_cache.size() >= m_max_cache_size) {
        evictOldestEntries();
    }
    
    auto entry = std::make_unique<CacheEntry>();
    entry->cached_frame = frame;
    entry->cache_time = std::chrono::steady_clock::now();
    entry->access_count = 0;
    entry->is_valid = true;
    
    m_cache[frame_number] = std::move(entry);
}

std::shared_ptr<eti::EtiFrame> ETIProcessingCache::getCachedFrame(uint32_t frame_number) {
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    
    m_total_requests++;
    
    auto it = m_cache.find(frame_number);
    if (it != m_cache.end() && it->second->is_valid) {
        it->second->access_count++;
        m_cache_hits++;
        
        return std::make_shared<eti::EtiFrame>(it->second->cached_frame);
    }
    
    m_cache_misses++;
    return nullptr;
}

bool ETIProcessingCache::isFrameCached(uint32_t frame_number) const {
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    
    auto it = m_cache.find(frame_number);
    return (it != m_cache.end() && it->second->is_valid);
}

void ETIProcessingCache::clearCache() {
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    m_cache.clear();
}

ETIProcessingCache::CacheStats ETIProcessingCache::getStatistics() const {
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    
    CacheStats stats;
    stats.total_requests = m_total_requests;
    stats.cache_hits = m_cache_hits;
    stats.cache_misses = m_cache_misses;
    stats.cached_frames = m_cache.size();
    
    if (m_total_requests > 0) {
        stats.hit_ratio = (double(m_cache_hits) / double(m_total_requests)) * 100.0;
    }
    
    return stats;
}

void ETIProcessingCache::setMaxCacheSize(size_t max_size) {
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    m_max_cache_size = max_size;
    
    // Evict entries if current size exceeds new limit
    if (m_cache.size() > m_max_cache_size) {
        evictOldestEntries();
    }
}

std::shared_ptr<eti::EtiFrame> ETIProcessingCache::lookup(std::span<const uint8_t, 6144> frame_data) {
    // Calculate a simple hash from frame data for lookup
    uint32_t frame_hash = 0;
    for (size_t i = 0; i < std::min(frame_data.size(), size_t(32)); ++i) {
        frame_hash = (frame_hash * 31) + frame_data[i];
    }
    
    return getCachedFrame(frame_hash);
}

void ETIProcessingCache::store(std::span<const uint8_t, 6144> frame_data, const eti::modern::ETIParseResult& result) {
    // Calculate hash and store the frame
    uint32_t frame_hash = 0;
    for (size_t i = 0; i < std::min(frame_data.size(), size_t(32)); ++i) {
        frame_hash = (frame_hash * 31) + frame_data[i];
    }
    
    // For now, we'll store a basic frame - proper ETIParseResult integration will be added later
    eti::EtiFrame basic_frame;
    std::copy(frame_data.begin(), frame_data.end(), basic_frame.frame_data.begin());
    
    cacheFrame(frame_hash, basic_frame);
}

void ETIProcessingCache::clearStatistics() {
    std::lock_guard<std::mutex> lock(m_cache_mutex);
    m_total_requests = 0;
    m_cache_hits = 0;
    m_cache_misses = 0;
}

void ETIProcessingCache::evictOldestEntries() {
    // Remove entries with lowest access count and oldest cache time
    while (m_cache.size() >= m_max_cache_size && !m_cache.empty()) {
        auto oldest_it = std::min_element(m_cache.begin(), m_cache.end(),
            [](const auto& a, const auto& b) {
                if (a.second->access_count != b.second->access_count) {
                    return a.second->access_count < b.second->access_count;
                }
                return a.second->cache_time < b.second->cache_time;
            });
        
        if (oldest_it != m_cache.end()) {
            m_cache.erase(oldest_it);
        } else {
            break;
        }
    }
}

} // namespace eti::modern