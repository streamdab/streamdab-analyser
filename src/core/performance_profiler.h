/**
 * @file performance_profiler.h
 * @brief Performance profiling and monitoring for ETI processing
 * 
 * Advanced performance monitoring to track and validate the >30% speed
 * improvement and >40% memory reduction targets for Phase 2 migration.
 */

#pragma once

#include <chrono>
#include <atomic>
#include <vector>
#include <string>
#include <memory>
#include <thread>

namespace eti::modern {

/**
 * @brief Detailed performance metrics
 */
struct DetailedPerformanceMetrics {
    // Processing metrics
    std::atomic<uint64_t> frames_processed{0};
    std::atomic<uint64_t> total_processing_time_ns{0};
    std::atomic<uint64_t> peak_processing_time_ns{0};
    std::atomic<uint64_t> min_processing_time_ns{UINT64_MAX};
    
    // Memory metrics
    std::atomic<size_t> current_memory_bytes{0};
    std::atomic<size_t> peak_memory_bytes{0};
    std::atomic<size_t> allocated_memory_bytes{0};
    std::atomic<size_t> deallocated_memory_bytes{0};
    
    // CPU metrics
    std::atomic<double> cpu_usage_percent{0.0};
    std::atomic<uint64_t> cpu_cycles_used{0};
    std::atomic<double> cpu_efficiency{0.0};
    
    // Threading metrics
    std::atomic<uint32_t> active_threads{0};
    std::atomic<uint64_t> thread_context_switches{0};
    std::atomic<uint64_t> thread_contention_events{0};
    
    // Cache metrics
    std::atomic<uint64_t> cache_hits{0};
    std::atomic<uint64_t> cache_misses{0};
    std::atomic<uint64_t> cache_evictions{0};
    
    // Target validation
    std::atomic<bool> speed_target_met{false};
    std::atomic<bool> memory_target_met{false};
    std::atomic<double> speed_improvement_percent{0.0};
    std::atomic<double> memory_reduction_percent{0.0};
};

/**
 * @brief Performance benchmark result
 */
struct BenchmarkResult {
    double average_fps;
    double peak_fps;
    double min_fps;
    double average_memory_mb;
    double peak_memory_mb;
    double cpu_efficiency;
    double cache_hit_rate;
    
    // Comparison with targets
    bool meets_speed_target;
    bool meets_memory_target;
    double speed_improvement_percent;
    double memory_reduction_percent;
    
    std::vector<std::string> performance_notes;
    std::vector<std::string> optimization_recommendations;
};

/**
 * @brief Real-time performance profiler for ETI processing
 * 
 * Comprehensive performance monitoring and analysis to validate
 * Phase 2 migration performance targets and identify optimization
 * opportunities.
 */
class PerformanceProfiler {
public:
    PerformanceProfiler();
    ~PerformanceProfiler();

    /**
     * @brief Start performance monitoring
     */
    void start();

    /**
     * @brief Stop performance monitoring
     */
    void stop();

    /**
     * @brief Reset all performance counters
     */
    void reset();

    /**
     * @brief Record frame processing event
     * @param processing_time_ns Processing time in nanoseconds
     */
    void recordFrameProcessing(uint64_t processing_time_ns);

    /**
     * @brief Record memory allocation event
     * @param bytes Number of bytes allocated
     */
    void recordMemoryAllocation(size_t bytes);

    /**
     * @brief Record memory deallocation event
     * @param bytes Number of bytes deallocated
     */
    void recordMemoryDeallocation(size_t bytes);

    /**
     * @brief Record cache event
     * @param hit True for cache hit, false for miss
     */
    void recordCacheEvent(bool hit);

    /**
     * @brief Record cache eviction
     */
    void recordCacheEviction();

    /**
     * @brief Set performance targets for validation
     * @param target_fps Target frames per second
     * @param baseline_fps Baseline performance for comparison
     * @param memory_limit_mb Memory usage limit in MB
     * @param baseline_memory_mb Baseline memory usage for comparison
     */
    void setTargets(double target_fps, double baseline_fps, 
                   double memory_limit_mb, double baseline_memory_mb);

    /**
     * @brief Set target FPS
     */
    void setTargetFPS(double fps) { target_fps_ = fps; }

    /**
     * @brief Set memory limit
     */
    void setMemoryLimit(size_t bytes) { memory_limit_bytes_ = bytes; }

    /**
     * @brief Get current performance metrics
     */
    DetailedPerformanceMetrics getMetrics() const { return metrics_; }

    /**
     * @brief Get current memory usage in bytes
     */
    size_t getCurrentMemoryUsage() const { return metrics_.current_memory_bytes.load(); }

    /**
     * @brief Get current CPU efficiency (0.0 - 1.0)
     */
    double getCPUEfficiency() const { return metrics_.cpu_efficiency.load(); }

    /**
     * @brief Get current frames per second
     */
    double getCurrentFPS() const;

    /**
     * @brief Check if performance targets are being met
     */
    bool areTargetsMet() const;

    /**
     * @brief Get detailed benchmark results
     */
    BenchmarkResult getBenchmarkResults() const;

    /**
     * @brief Generate performance report
     */
    std::string generatePerformanceReport() const;

    /**
     * @brief Get optimization recommendations
     */
    std::vector<std::string> getOptimizationRecommendations() const;

private:
    DetailedPerformanceMetrics metrics_;
    
    // Profiling control
    std::atomic<bool> profiling_active_{false};
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point last_update_;
    
    // Targets and baselines
    double target_fps_{1000.0};
    double baseline_fps_{670.0}; // ETISnoop baseline
    size_t memory_limit_bytes_{40 * 1024 * 1024}; // 40MB target
    size_t baseline_memory_bytes_{100 * 1024 * 1024}; // 100MB baseline
    
    // Rolling averages for FPS calculation
    std::vector<std::chrono::steady_clock::time_point> recent_frame_times_;
    mutable std::mutex frame_times_mutex_;
    
    // CPU monitoring
    std::unique_ptr<std::thread> cpu_monitor_thread_;
    std::atomic<bool> cpu_monitoring_active_{false};
    
    // Internal methods
    void updateCPUMetrics();
    void updateMemoryMetrics();
    void updateTargetValidation();
    void cpuMonitorLoop();
    double calculateCurrentFPS() const;
    double calculateAverageFPS() const;
    void cleanupOldFrameTimes();
};

} // namespace eti::modern