/**
 * @file performance_profiler.h
 * @brief Real-time Performance Profiler for Modern ETI Core Engine
 * 
 * Comprehensive performance monitoring and profiling system for ETI processing
 * operations, providing detailed metrics and optimization insights.
 * 
 * @author Build Manager Agent
 * @date 2025
 * @copyright StreamDAB Analyser Project
 */

#pragma once

#include <chrono>
#include <atomic>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>

namespace eti::modern {

// Performance monitoring constants
constexpr double TARGET_MEMORY_LIMIT_MB = 100.0;
constexpr double TARGET_CPU_LIMIT_PERCENT = 80.0;
constexpr double TARGET_ERROR_RATE = 0.05;          // 5% maximum error rate
constexpr double TARGET_LATENCY_LIMIT_MS = 17.0;    // 17ms maximum latency

/**
 * @brief Performance metrics data structure
 */
struct performance_metrics {
    std::chrono::nanoseconds total_time{0};
    std::chrono::nanoseconds min_time{std::chrono::nanoseconds::max()};
    std::chrono::nanoseconds max_time{0};
    std::chrono::nanoseconds avg_time{0};
    uint64_t call_count{0};
    uint64_t error_count{0};
};

/**
 * @brief Performance report structure
 */
struct PerformanceReport {
    std::chrono::high_resolution_clock::time_point timestamp;
    size_t total_operations{0};
    size_t memory_usage_bytes{0};
    double memory_usage_mb{0.0};
    double cpu_usage_percent{0.0};
    double overall_fps{0.0};
    double error_rate_percent{0.0};
    double peak_throughput_fps{0.0};
    std::string peak_throughput_operation;
    std::chrono::nanoseconds worst_latency{0};
    std::string worst_latency_operation;
    uint64_t total_calls{0};
    uint64_t total_errors{0};
    double overall_error_rate{0.0};
    double performance_score{0.0};
    std::unordered_map<std::string, performance_metrics> operation_metrics;
};

/**
 * @brief High-performance profiler for ETI operations
 * 
 * Provides low-overhead performance monitoring for critical ETI processing
 * paths with nanosecond precision timing and comprehensive statistics.
 */
class performance_profiler {
public:
    performance_profiler();
    ~performance_profiler();

    /**
     * @brief Start timing measurement for operation
     * @param operation_name Name of the operation being timed
     * @return Timer handle for ending measurement
     */
    uint64_t start_timing(const std::string& operation_name);

    /**
     * @brief End timing measurement
     * @param timer_handle Handle returned from startTiming
     */
    void end_timing(uint64_t timer_handle);

    /**
     * @brief Record operation error
     * @param operation_name Name of the operation that failed
     */
    void record_error(const std::string& operation_name);

    /**
     * @brief Get performance metrics for operation
     * @param operation_name Name of the operation
     * @return Performance metrics or nullptr if not found
     */
    const performance_metrics* get_metrics(const std::string& operation_name) const;

    /**
     * @brief Get all performance metrics
     * @return Map of operation names to metrics
     */
    std::unordered_map<std::string, performance_metrics> get_all_metrics() const;

    /**
     * @brief Reset all performance statistics
     */
    void reset_statistics();

    /**
     * @brief Get current processing rate (operations per second)
     * @param operation_name Name of the operation
     * @return Operations per second
     */
    double get_processing_rate(const std::string& operation_name) const;

    /**
     * @brief Enable or disable profiling
     * @param enabled true to enable profiling
     */
    void set_enabled(bool enabled);

    /**
     * @brief Check if profiling is enabled
     * @return true if profiling is enabled
     */
    bool is_enabled() const { return enabled_; }

    /**
     * @brief Set target FPS for performance monitoring
     * @param target_fps Target frames per second
     */
    void set_target_fps(double target_fps);

    /**
     * @brief Set memory limit for performance monitoring
     * @param memory_limit_bytes Memory limit in bytes
     */
    void set_memory_limit(size_t memory_limit_bytes);

    /**
     * @brief Get current memory usage
     * @return Current memory usage in bytes
     */
    size_t get_current_memory_usage() const;

    /**
     * @brief Get current CPU efficiency
     * @return CPU efficiency percentage (0-100)
     */
    double get_cpu_efficiency() const;

    /**
     * @brief Generate comprehensive performance report
     * @return Complete performance report
     */
    PerformanceReport generate_performance_report() const;

    /**
     * @brief Calculate overall performance score
     * @param report Performance report to analyze
     * @return Performance score (0-10)
     */
    double calculate_performance_score(const PerformanceReport& report) const;

private:
    struct timer_entry {
        std::string operation_name;
        std::chrono::high_resolution_clock::time_point start_time;
        bool is_active{false};
    };

    mutable std::mutex metrics_mutex_;
    std::unordered_map<std::string, performance_metrics> metrics_;
    std::unordered_map<uint64_t, timer_entry> active_timers_;
    
    std::atomic<uint64_t> next_timer_id_{1};
    std::atomic<bool> enabled_{true};
    std::chrono::high_resolution_clock::time_point start_time_;

    // Performance monitoring parameters
    double target_fps_{0.0};
    size_t memory_limit_bytes_{0};

    void update_metrics(const std::string& operation_name, 
                       std::chrono::nanoseconds duration);

    /**
     * @brief Check performance thresholds and emit warnings
     */
    void check_performance_thresholds(const std::string& operation_name,
                                     const performance_metrics& metrics);

    /**
     * @brief Log performance summary to console
     */
    void log_performance_summary() const;

    /**
     * @brief Get current memory usage in bytes
     * @return Memory usage in bytes
     */
    size_t get_memory_usage_bytes() const;

    /**
     * @brief Get current CPU usage percentage
     * @return CPU usage percentage (0-100)
     */
    double get_cpu_usage_percent() const;

    /**
     * @brief Check if performance targets are met
     * @return true if all performance targets are met
     */
    bool meets_performance_targets() const;
};

/**
 * @brief RAII-style timer for automatic profiling
 */
class scoped_timer {
public:
    scoped_timer(performance_profiler& profiler, const std::string& operation_name)
        : profiler_(profiler)
        , timer_id_(profiler.start_timing(operation_name)) {}

    ~scoped_timer() {
        profiler_.end_timing(timer_id_);
    }

    // Non-copyable, non-movable
    scoped_timer(const scoped_timer&) = delete;
    scoped_timer& operator=(const scoped_timer&) = delete;
    scoped_timer(scoped_timer&&) = delete;
    scoped_timer& operator=(scoped_timer&&) = delete;

private:
    performance_profiler& profiler_;
    uint64_t timer_id_;
};

} // namespace eti::modern

// Convenience macro for scoped timing
#define PROFILE_SCOPE(profiler, name) \
    eti::modern::scoped_timer _timer(profiler, name)
