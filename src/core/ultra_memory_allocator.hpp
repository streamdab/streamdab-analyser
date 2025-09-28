/**
 * @file ultra_memory_allocator.hpp
 * @brief Ultra-Performance Memory Allocator for ETI Processing
 * 
 * Custom memory allocators optimized for ETI frame structures with:
 * - NUMA-aware allocation
 * - Huge page support
 * - Lock-free allocation pools
 * - Cache-aligned structures
 * - Memory prefetching
 * 
 * Target: <25MB memory footprint with 75% reduction from baseline
 * 
 * @author Performance Optimization Agent
 * @date 2025
 */

#pragma once

#include <memory>
#include <atomic>
#include <array>
#include <cstddef>
#include <cstdint>
#include <new>
#include <sys/mman.h>
#include <numa.h>
#include <immintrin.h>

namespace eti::memory {

/**
 * @brief Memory allocation constants
 */
constexpr size_t CACHE_LINE_SIZE = 64;
constexpr size_t HUGE_PAGE_SIZE = 2 * 1024 * 1024;  // 2MB huge pages
constexpr size_t ETI_FRAME_ALIGNED_SIZE = ((6144 + CACHE_LINE_SIZE - 1) / CACHE_LINE_SIZE) * CACHE_LINE_SIZE;
constexpr size_t MAX_MEMORY_POOLS = 16;
constexpr size_t POOL_SIZE_SMALL = 4096;   // Small objects
constexpr size_t POOL_SIZE_MEDIUM = 1024;  // ETI frames
constexpr size_t POOL_SIZE_LARGE = 256;    // Large objects

/**
 * @brief Memory allocation size classes
 */
enum class allocation_size_class : uint8_t {
    SMALL = 0,    // <= 256 bytes
    MEDIUM = 1,   // <= 6144 bytes (ETI frames)
    LARGE = 2,    // > 6144 bytes
    HUGE = 3      // > 64KB
};

/**
 * @brief NUMA-aware memory statistics
 */
struct numa_memory_stats {
    std::array<size_t, 8> allocations_per_node{};
    std::array<size_t, 8> bytes_per_node{};
    std::array<double, 8> utilization_per_node{};
    size_t total_allocations{0};
    size_t total_bytes{0};
    double overall_utilization{0.0};
    int preferred_node{-1};
};

/**
 * @brief Lock-free memory block with intrusive free list
 */
template<size_t BlockSize>
struct alignas(CACHE_LINE_SIZE) memory_block {
    union {
        alignas(CACHE_LINE_SIZE) uint8_t data[BlockSize];
        memory_block* next_free;
    };
    
    std::atomic<bool> is_allocated{false};
    uint32_t allocation_id{0};
    uint16_t numa_node{0};
    uint16_t cpu_core{0};
    
    static_assert(BlockSize >= sizeof(memory_block*), "Block size too small for free list");
};

/**
 * @brief Cache-optimized free list for lock-free allocation
 */
template<typename BlockType>
class alignas(CACHE_LINE_SIZE) lock_free_free_list {
private:
    alignas(CACHE_LINE_SIZE) std::atomic<BlockType*> head_{nullptr};
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> size_{0};
    
public:
    void push(BlockType* block) noexcept {
        block->next_free = head_.load(std::memory_order_relaxed);
        
        while (!head_.compare_exchange_weak(
            block->next_free, block, 
            std::memory_order_release, 
            std::memory_order_relaxed)) {
            // Retry until successful
        }
        
        size_.fetch_add(1, std::memory_order_relaxed);
    }
    
    BlockType* pop() noexcept {
        BlockType* head = head_.load(std::memory_order_acquire);
        
        while (head && !head_.compare_exchange_weak(
            head, head->next_free,
            std::memory_order_release,
            std::memory_order_relaxed)) {
            // Retry until successful
        }
        
        if (head) {
            size_.fetch_sub(1, std::memory_order_relaxed);
        }
        
        return head;
    }
    
    size_t size() const noexcept {
        return size_.load(std::memory_order_relaxed);
    }
    
    bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) == nullptr;
    }
};

/**
 * @brief NUMA-aware memory pool for specific size class
 */
template<size_t BlockSize, size_t PoolSize>
class numa_memory_pool {
private:
    using block_type = memory_block<BlockSize>;
    
    // Memory storage
    alignas(HUGE_PAGE_SIZE) std::array<block_type, PoolSize> memory_blocks_;
    
    // Free list per NUMA node
    std::array<lock_free_free_list<block_type>, 8> free_lists_;
    
    // Pool metadata
    std::atomic<size_t> total_allocations_{0};
    std::atomic<size_t> current_allocations_{0};
    std::atomic<uint32_t> allocation_counter_{0};
    int numa_node_{-1};
    
public:
    numa_memory_pool() {
        // Determine NUMA node
        numa_node_ = numa_node_of_cpu(sched_getcpu());
        
        // Initialize all blocks in free lists
        initialize_memory_blocks();
        
        // Advise kernel about access patterns
        madvise(memory_blocks_.data(), sizeof(memory_blocks_), MADV_SEQUENTIAL);
        
        // Try to use huge pages for better performance
        if (madvise(memory_blocks_.data(), sizeof(memory_blocks_), MADV_HUGEPAGE) != 0) {
            // Huge pages not available, continue with normal pages
        }
    }
    
    ~numa_memory_pool() {
        // Cleanup - advise kernel we're done
        madvise(memory_blocks_.data(), sizeof(memory_blocks_), MADV_DONTNEED);
    }
    
    /**
     * @brief Allocate memory block from preferred NUMA node
     */
    void* allocate(int preferred_numa_node = -1) noexcept {
        const int target_node = (preferred_numa_node >= 0) ? preferred_numa_node : numa_node_;
        const int actual_node = std::min(target_node, 7);
        
        // Try preferred NUMA node first
        block_type* block = free_lists_[actual_node].pop();
        
        // If preferred node is empty, try other nodes
        if (!block) {
            for (int node = 0; node < 8; ++node) {
                if (node != actual_node) {
                    block = free_lists_[node].pop();
                    if (block) break;
                }
            }
        }
        
        if (block) {
            block->is_allocated.store(true, std::memory_order_release);
            block->allocation_id = allocation_counter_.fetch_add(1, std::memory_order_relaxed);
            block->numa_node = actual_node;
            block->cpu_core = sched_getcpu();
            
            total_allocations_.fetch_add(1, std::memory_order_relaxed);
            current_allocations_.fetch_add(1, std::memory_order_relaxed);
            
            // Prefetch for write access
            _mm_prefetch(block->data, _MM_HINT_T0);
            
            return block->data;
        }
        
        return nullptr;  // Pool exhausted
    }
    
    /**
     * @brief Deallocate memory block
     */
    void deallocate(void* ptr) noexcept {
        if (!ptr) return;
        
        // Find the block containing this pointer
        block_type* block = reinterpret_cast<block_type*>(
            reinterpret_cast<uintptr_t>(ptr) & ~(sizeof(block_type) - 1));
        
        // Validate block belongs to this pool
        if (block < memory_blocks_.data() || 
            block >= memory_blocks_.data() + PoolSize) {
            return;  // Not from this pool
        }
        
        if (block->is_allocated.load(std::memory_order_acquire)) {
            block->is_allocated.store(false, std::memory_order_release);
            current_allocations_.fetch_sub(1, std::memory_order_relaxed);
            
            // Return to appropriate NUMA node free list
            const int numa_node = std::min(static_cast<int>(block->numa_node), 7);
            free_lists_[numa_node].push(block);
        }
    }
    
    /**
     * @brief Get pool statistics
     */
    struct pool_stats {
        size_t total_blocks{PoolSize};
        size_t allocated_blocks{0};
        size_t free_blocks{0};
        double utilization_percent{0.0};
        size_t total_allocations{0};
        int numa_node{-1};
        std::array<size_t, 8> free_per_node{};
    };
    
    pool_stats get_statistics() const noexcept {
        pool_stats stats;
        stats.allocated_blocks = current_allocations_.load(std::memory_order_relaxed);
        stats.total_allocations = total_allocations_.load(std::memory_order_relaxed);
        stats.numa_node = numa_node_;
        
        size_t total_free = 0;
        for (int i = 0; i < 8; ++i) {
            stats.free_per_node[i] = free_lists_[i].size();
            total_free += stats.free_per_node[i];
        }
        
        stats.free_blocks = total_free;
        stats.utilization_percent = PoolSize > 0 ? 
            (static_cast<double>(stats.allocated_blocks) / PoolSize) * 100.0 : 0.0;
        
        return stats;
    }

private:
    void initialize_memory_blocks() {
        // Distribute blocks across NUMA nodes based on CPU topology
        const int num_numa_nodes = std::min(numa_num_configured_nodes(), 8);
        const size_t blocks_per_node = PoolSize / num_numa_nodes;
        
        for (size_t i = 0; i < PoolSize; ++i) {
            const int target_node = (i / blocks_per_node) % num_numa_nodes;
            memory_blocks_[i].numa_node = target_node;
            free_lists_[target_node].push(&memory_blocks_[i]);
        }
    }
};

/**
 * @brief Ultra-performance memory allocator manager
 */
class UltraMemoryAllocator {
private:
    // Memory pools for different size classes
    numa_memory_pool<256, POOL_SIZE_SMALL> small_pool_;
    numa_memory_pool<ETI_FRAME_ALIGNED_SIZE, POOL_SIZE_MEDIUM> eti_frame_pool_;
    numa_memory_pool<65536, POOL_SIZE_LARGE> large_pool_;
    
    // Allocation tracking
    std::atomic<size_t> total_memory_allocated_{0};
    std::atomic<size_t> peak_memory_usage_{0};
    std::atomic<double> allocation_efficiency_{100.0};
    
    // Performance targets
    static constexpr size_t TARGET_MEMORY_LIMIT = 25 * 1024 * 1024;  // 25MB
    static constexpr double TARGET_EFFICIENCY = 95.0;  // 95%
    
public:
    UltraMemoryAllocator() {
        // Initialize NUMA if available
        if (numa_available() >= 0) {
            numa_set_localalloc();
        }
        
        // Lock allocator in memory for deterministic performance
        if (mlock(this, sizeof(*this)) != 0) {
            // Non-critical if fails
        }
    }
    
    ~UltraMemoryAllocator() {
        munlock(this, sizeof(*this));
    }
    
    /**
     * @brief Allocate memory from appropriate pool
     */
    void* allocate(size_t size, int numa_node = -1) {
        const allocation_size_class size_class = classify_allocation_size(size);
        void* ptr = nullptr;
        
        switch (size_class) {
            case allocation_size_class::SMALL:
                ptr = small_pool_.allocate(numa_node);
                break;
            case allocation_size_class::MEDIUM:
                ptr = eti_frame_pool_.allocate(numa_node);
                break;
            case allocation_size_class::LARGE:
                ptr = large_pool_.allocate(numa_node);
                break;
            case allocation_size_class::HUGE:
                ptr = allocate_huge_page(size);
                break;
        }
        
        if (ptr) {
            const size_t actual_size = get_allocation_size(size_class);
            total_memory_allocated_.fetch_add(actual_size, std::memory_order_relaxed);
            
            // Update peak usage
            const size_t current_usage = total_memory_allocated_.load(std::memory_order_relaxed);
            size_t expected_peak = peak_memory_usage_.load(std::memory_order_relaxed);
            while (current_usage > expected_peak && 
                   !peak_memory_usage_.compare_exchange_weak(expected_peak, current_usage)) {
                // Retry until successful
            }
        }
        
        return ptr;
    }
    
    /**
     * @brief Deallocate memory
     */
    void deallocate(void* ptr, size_t size) {
        if (!ptr) return;
        
        const allocation_size_class size_class = classify_allocation_size(size);
        
        switch (size_class) {
            case allocation_size_class::SMALL:
                small_pool_.deallocate(ptr);
                break;
            case allocation_size_class::MEDIUM:
                eti_frame_pool_.deallocate(ptr);
                break;
            case allocation_size_class::LARGE:
                large_pool_.deallocate(ptr);
                break;
            case allocation_size_class::HUGE:
                deallocate_huge_page(ptr, size);
                break;
        }
        
        const size_t actual_size = get_allocation_size(size_class);
        total_memory_allocated_.fetch_sub(actual_size, std::memory_order_relaxed);
    }
    
    /**
     * @brief Allocate ETI frame with optimal alignment
     */
    void* allocate_eti_frame(int numa_node = -1) {
        return eti_frame_pool_.allocate(numa_node);
    }
    
    /**
     * @brief Deallocate ETI frame
     */
    void deallocate_eti_frame(void* ptr) {
        eti_frame_pool_.deallocate(ptr);
    }
    
    /**
     * @brief Get comprehensive memory statistics
     */
    struct memory_statistics {
        size_t current_usage_bytes;
        size_t peak_usage_bytes;
        double efficiency_percent;
        bool meets_memory_target;
        numa_memory_stats numa_stats;
        
        struct {
            size_t total_blocks;
            size_t allocated_blocks;
            double utilization_percent;
        } small_pool, eti_frame_pool, large_pool;
    };
    
    memory_statistics get_statistics() const {
        memory_statistics stats;
        
        stats.current_usage_bytes = total_memory_allocated_.load(std::memory_order_relaxed);
        stats.peak_usage_bytes = peak_memory_usage_.load(std::memory_order_relaxed);
        stats.efficiency_percent = allocation_efficiency_.load(std::memory_order_relaxed);
        stats.meets_memory_target = stats.current_usage_bytes <= TARGET_MEMORY_LIMIT;
        
        // Get pool statistics
        const auto small_stats = small_pool_.get_statistics();
        stats.small_pool.total_blocks = small_stats.total_blocks;
        stats.small_pool.allocated_blocks = small_stats.allocated_blocks;
        stats.small_pool.utilization_percent = small_stats.utilization_percent;
        
        const auto eti_stats = eti_frame_pool_.get_statistics();
        stats.eti_frame_pool.total_blocks = eti_stats.total_blocks;
        stats.eti_frame_pool.allocated_blocks = eti_stats.allocated_blocks;
        stats.eti_frame_pool.utilization_percent = eti_stats.utilization_percent;
        
        const auto large_stats = large_pool_.get_statistics();
        stats.large_pool.total_blocks = large_stats.total_blocks;
        stats.large_pool.allocated_blocks = large_stats.allocated_blocks;
        stats.large_pool.utilization_percent = large_stats.utilization_percent;
        
        return stats;
    }
    
    /**
     * @brief Calculate memory performance score (0-10)
     */
    double calculate_memory_score() const {
        const auto stats = get_statistics();
        
        // Memory usage score (0-4 points)
        const double usage_score = stats.meets_memory_target ? 4.0 : 
            (static_cast<double>(TARGET_MEMORY_LIMIT) / stats.current_usage_bytes) * 4.0;
        
        // Efficiency score (0-3 points)
        const double efficiency_score = (stats.efficiency_percent / 100.0) * 3.0;
        
        // Utilization score (0-3 points)
        const double avg_utilization = (stats.small_pool.utilization_percent + 
                                       stats.eti_frame_pool.utilization_percent + 
                                       stats.large_pool.utilization_percent) / 3.0;
        const double utilization_score = (avg_utilization / 100.0) * 3.0;
        
        return std::min(10.0, usage_score + efficiency_score + utilization_score);
    }
    
    /**
     * @brief Check if memory targets are met
     */
    bool meets_performance_targets() const {
        const auto stats = get_statistics();
        return stats.meets_memory_target && stats.efficiency_percent >= TARGET_EFFICIENCY;
    }

private:
    allocation_size_class classify_allocation_size(size_t size) const {
        if (size <= 256) return allocation_size_class::SMALL;
        if (size <= ETI_FRAME_ALIGNED_SIZE) return allocation_size_class::MEDIUM;
        if (size <= 65536) return allocation_size_class::LARGE;
        return allocation_size_class::HUGE;
    }
    
    size_t get_allocation_size(allocation_size_class size_class) const {
        switch (size_class) {
            case allocation_size_class::SMALL: return 256;
            case allocation_size_class::MEDIUM: return ETI_FRAME_ALIGNED_SIZE;
            case allocation_size_class::LARGE: return 65536;
            default: return 0;  // Huge pages calculated separately
        }
    }
    
    void* allocate_huge_page(size_t size) {
        const size_t aligned_size = ((size + HUGE_PAGE_SIZE - 1) / HUGE_PAGE_SIZE) * HUGE_PAGE_SIZE;
        void* ptr = mmap(nullptr, aligned_size, PROT_READ | PROT_WRITE, 
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
        return (ptr == MAP_FAILED) ? nullptr : ptr;
    }
    
    void deallocate_huge_page(void* ptr, size_t size) {
        const size_t aligned_size = ((size + HUGE_PAGE_SIZE - 1) / HUGE_PAGE_SIZE) * HUGE_PAGE_SIZE;
        munmap(ptr, aligned_size);
    }
};

/**
 * @brief Global ultra-performance allocator instance
 */
UltraMemoryAllocator& get_ultra_allocator();

/**
 * @brief STL-compatible allocator using ultra-performance allocator
 */
template<typename T>
class ultra_stl_allocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    template<typename U>
    struct rebind {
        using other = ultra_stl_allocator<U>;
    };
    
    ultra_stl_allocator() noexcept = default;
    
    template<typename U>
    ultra_stl_allocator(const ultra_stl_allocator<U>&) noexcept {}
    
    pointer allocate(size_type n, const void* = nullptr) {
        return static_cast<pointer>(get_ultra_allocator().allocate(n * sizeof(T)));
    }
    
    void deallocate(pointer p, size_type n) {
        get_ultra_allocator().deallocate(p, n * sizeof(T));
    }
    
    template<typename U>
    bool operator==(const ultra_stl_allocator<U>&) const noexcept { return true; }
    
    template<typename U>
    bool operator!=(const ultra_stl_allocator<U>&) const noexcept { return false; }
};

} // namespace eti::memory