/**
 * @file performance_profiler.h
 * @brief Real-time Performance Profiler for Modern ETI Core Engine
 * 
 * Comprehensive performance monitoring and profiling system for ETI processing
 * operations, providing detailed metrics and optimization insights.
 * 
 * @author Build Manager Agent
 * @date 2025
 * @copyright StreamDAB Analyzer Project
 */

#pragma once

#include <chrono>
#include <atomic>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>

namespace eti::modern {

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

    void update_metrics(const std::string& operation_name, 
                       std::chrono::nanoseconds duration);
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
