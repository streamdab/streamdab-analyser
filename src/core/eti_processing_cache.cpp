/**
 * @file eti_processing_cache.cpp
 * @brief ETI Processing Cache Implementation
 */

#include "eti_processing_cache.h"
#include "utils/logger.h"
#include <algorithm>
#include <openssl/sha.h>
#include <cstring>

namespace eti::modern {

ETIProcessingCache::ETIProcessingCache(size_t max_memory_mb)
    : max_memory_bytes_(max_memory_mb * 1024 * 1024)
{
    // Reserve space for expected cache entries
    cache_map_.reserve(1000);
    lru_list_.reserve(1000);
    
    Logger::instance().log(Logger::Info, "ETIProcessingCache", 
                          QString("Processing cache initialized: %1 MB limit")
                          .arg(max_memory_mb));
}

ETIProcessingCache::~ETIProcessingCache() {
    std::unique_lock<std::shared_mutex> lock(cache_mutex_);
    
    Logger::instance().log(Logger::Info, "ETIProcessingCache", 
                          QString("Cache destroyed - Stats: %1 hits, %2 misses, %.1f%% hit rate")
                          .arg(stats_.hits.load())
                          .arg(stats_.misses.load())
                          .arg(stats_.get_hit_rate() * 100.0));
}

std::optional<ETIParseResult> ETIProcessingCache::lookup(
    std::span<const uint8_t, ETI_FRAME_SIZE> frame_data) {
    
    stats_.total_requests.fetch_add(1);
    
    FrameHash hash = calculateFrameHash(frame_data);
    
    std::shared_lock<std::shared_mutex> lock(cache_mutex_);
    
    auto it = cache_map_.find(hash);
    if (it != cache_map_.end()) {
        // Cache hit
        auto& entry = it->second;
        entry->last_access = std::chrono::steady_clock::now();
        entry->access_count++;
        
        stats_.hits.fetch_add(1);
        
        // Update LRU without holding write lock
        lock.unlock();
        updateLRU(hash);
        
        // Return copy of cached result
        ETIParseResult result = entry->result; // Copy constructor
        result.performance.cache_hits++;
        
        Logger::instance().log(Logger::Debug, "ETIProcessingCache", 
                              QString("Cache hit for frame hash (access count: %1)")
                              .arg(entry->access_count));
        
        return result;
    }
    
    // Cache miss
    stats_.misses.fetch_add(1);
    
    Logger::instance().log(Logger::Debug, "ETIProcessingCache", "Cache miss");
    return std::nullopt;
}

void ETIProcessingCache::store(std::span<const uint8_t, ETI_FRAME_SIZE> frame_data, 
                              const ETIParseResult& result) {
    
    FrameHash hash = calculateFrameHash(frame_data);
    size_t entry_size = estimateEntrySize(result);
    
    std::unique_lock<std::shared_mutex> lock(cache_mutex_);
    
    // Check if we need to evict entries to make space
    while (stats_.memory_usage.load() + entry_size > max_memory_bytes_ && !cache_map_.empty()) {
        evictLRUEntries(1);
    }
    
    // Check if entry already exists (avoid duplicates)
    auto it = cache_map_.find(hash);
    if (it != cache_map_.end()) {
        // Update existing entry
        it->second->result = result;
        it->second->last_access = std::chrono::steady_clock::now();
        it->second->access_count++;
        updateLRU(hash);
        return;
    }
    
    // Create new entry
    auto entry = std::make_unique<CacheEntry>(ETIParseResult(result));
    
    cache_map_[hash] = std::move(entry);
    
    // Add to LRU list
    lru_list_.push_back({hash, std::chrono::steady_clock::now()});
    
    // Update statistics
    stats_.current_size.fetch_add(1);
    stats_.memory_usage.fetch_add(entry_size);
    
    Logger::instance().log(Logger::Debug, "ETIProcessingCache", 
                          QString("Cached new entry - Size: %1, Memory: %2 KB")
                          .arg(stats_.current_size.load())
                          .arg(stats_.memory_usage.load() / 1024));
}

void ETIProcessingCache::clearStatistics() {
    stats_.hits.store(0);
    stats_.misses.store(0);
    stats_.evictions.store(0);
    stats_.total_requests.store(0);
    // Don't reset current_size and memory_usage as they reflect actual state
}

void ETIProcessingCache::clear() {
    std::unique_lock<std::shared_mutex> lock(cache_mutex_);
    
    cache_map_.clear();
    lru_list_.clear();
    
    stats_.current_size.store(0);
    stats_.memory_usage.store(0);
    
    Logger::instance().log(Logger::Info, "ETIProcessingCache", "Cache cleared");
}

void ETIProcessingCache::optimize() {
    std::unique_lock<std::shared_mutex> lock(cache_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    const auto cleanup_threshold = std::chrono::minutes(10);
    
    // Remove entries that haven't been accessed recently
    size_t removed = 0;
    auto it = cache_map_.begin();
    while (it != cache_map_.end()) {
        if ((now - it->second->last_access) > cleanup_threshold && 
            it->second->access_count <= 1) {
            
            size_t entry_size = estimateEntrySize(it->second->result);
            stats_.memory_usage.fetch_sub(entry_size);
            stats_.current_size.fetch_sub(1);
            stats_.evictions.fetch_add(1);
            
            // Remove from LRU list
            auto lru_it = std::find_if(lru_list_.begin(), lru_list_.end(),
                [&](const LRUNode& node) { return node.key == it->first; });
            if (lru_it != lru_list_.end()) {
                lru_list_.erase(lru_it);
            }
            
            it = cache_map_.erase(it);
            removed++;
        } else {
            ++it;
        }
    }
    
    // Sort LRU list for better performance
    std::sort(lru_list_.begin(), lru_list_.end());
    
    Logger::instance().log(Logger::Debug, "ETIProcessingCache", 
                          QString("Cache optimized: removed %1 old entries").arg(removed));
}

void ETIProcessingCache::setMaxMemorySize(size_t max_memory_mb) {
    const_cast<size_t&>(max_memory_bytes_) = max_memory_mb * 1024 * 1024;
    
    // Trigger cleanup if current usage exceeds new limit
    if (stats_.memory_usage.load() > max_memory_bytes_) {
        optimize();
    }
}

ETIProcessingCache::FrameHash ETIProcessingCache::calculateFrameHash(
    std::span<const uint8_t, ETI_FRAME_SIZE> data) const {
    
    FrameHash hash;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data.data(), data.size());
    SHA256_Final(hash.data(), &ctx);
    
    return hash;
}

void ETIProcessingCache::evictLRUEntries(size_t count) {
    if (lru_list_.empty()) return;
    
    // Sort LRU list by access time (oldest first)
    std::sort(lru_list_.begin(), lru_list_.end());
    
    size_t evicted = 0;
    for (auto it = lru_list_.begin(); 
         it != lru_list_.end() && evicted < count; ) {
        
        auto cache_it = cache_map_.find(it->key);
        if (cache_it != cache_map_.end()) {
            size_t entry_size = estimateEntrySize(cache_it->second->result);
            stats_.memory_usage.fetch_sub(entry_size);
            stats_.current_size.fetch_sub(1);
            stats_.evictions.fetch_add(1);
            
            cache_map_.erase(cache_it);
            evicted++;
        }
        
        it = lru_list_.erase(it);
    }
    
    Logger::instance().log(Logger::Debug, "ETIProcessingCache", 
                          QString("Evicted %1 LRU entries").arg(evicted));
}

void ETIProcessingCache::updateLRU(const FrameHash& key) {
    std::unique_lock<std::shared_mutex> lock(cache_mutex_);
    
    auto it = std::find_if(lru_list_.begin(), lru_list_.end(),
        [&key](const LRUNode& node) { return node.key == key; });
    
    if (it != lru_list_.end()) {
        it->last_access = std::chrono::steady_clock::now();
    }
}

size_t ETIProcessingCache::estimateEntrySize(const ETIParseResult& result) const {
    size_t size = sizeof(ETIParseResult);
    size += sizeof(eti::EtiFrame);
    size += result.validation.errors.size() * 50; // Estimate string sizes
    size += result.validation.warnings.size() * 50;
    return size;
}

void ETIProcessingCache::cleanup() {
    // This method is called periodically to maintain cache health
    auto now = std::chrono::steady_clock::now();
    const auto stale_threshold = std::chrono::hours(1);
    
    size_t removed = 0;
    auto it = cache_map_.begin();
    while (it != cache_map_.end()) {
        if ((now - it->second->creation_time) > stale_threshold && 
            it->second->access_count <= 2) {
            
            size_t entry_size = estimateEntrySize(it->second->result);
            stats_.memory_usage.fetch_sub(entry_size);
            stats_.current_size.fetch_sub(1);
            
            it = cache_map_.erase(it);
            removed++;
        } else {
            ++it;
        }
    }
    
    if (removed > 0) {
        Logger::instance().log(Logger::Debug, "ETIProcessingCache", 
                              QString("Cleanup: removed %1 stale entries").arg(removed));
    }
}

} // namespace eti::modern