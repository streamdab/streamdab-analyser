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
struct PerformanceMetrics {
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
class PerformanceProfiler {
public:
    PerformanceProfiler();
    ~PerformanceProfiler();

    /**
     * @brief Start timing measurement for operation
     * @param operation_name Name of the operation being timed
     * @return Timer handle for ending measurement
     */
    uint64_t startTiming(const std::string& operation_name);

    /**
     * @brief End timing measurement
     * @param timer_handle Handle returned from startTiming
     */
    void endTiming(uint64_t timer_handle);

    /**
     * @brief Record operation error
     * @param operation_name Name of the operation that failed
     */
    void recordError(const std::string& operation_name);

    /**
     * @brief Get performance metrics for operation
     * @param operation_name Name of the operation
     * @return Performance metrics or nullptr if not found
     */
    const PerformanceMetrics* getMetrics(const std::string& operation_name) const;

    /**
     * @brief Get all performance metrics
     * @return Map of operation names to metrics
     */
    std::unordered_map<std::string, PerformanceMetrics> getAllMetrics() const;

    /**
     * @brief Reset all performance statistics
     */
    void resetStatistics();

    /**
     * @brief Get current processing rate (operations per second)
     * @param operation_name Name of the operation
     * @return Operations per second
     */
    double getProcessingRate(const std::string& operation_name) const;

    /**
     * @brief Enable or disable profiling
     * @param enabled true to enable profiling
     */
    void setEnabled(bool enabled);

    /**
     * @brief Check if profiling is enabled
     * @return true if profiling is enabled
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * @brief Set target FPS for performance monitoring
     * @param target_fps Target frames per second
     */
    void setTargetFPS(double target_fps);

    /**
     * @brief Set memory limit for performance monitoring
     * @param memory_limit_bytes Memory limit in bytes
     */
    void setMemoryLimit(size_t memory_limit_bytes);

    /**
     * @brief Get current memory usage
     * @return Current memory usage in bytes
     */
    size_t getCurrentMemoryUsage() const;

    /**
     * @brief Get current CPU efficiency
     * @return CPU efficiency percentage (0-100)
     */
    double getCPUEfficiency() const;

private:
    struct TimerEntry {
        std::string operation_name;
        std::chrono::high_resolution_clock::time_point start_time;
        bool is_active{false};
    };

    mutable std::mutex m_metrics_mutex;
    std::unordered_map<std::string, PerformanceMetrics> m_metrics;
    std::unordered_map<uint64_t, TimerEntry> m_active_timers;
    
    std::atomic<uint64_t> m_next_timer_id{1};
    std::atomic<bool> m_enabled{true};
    std::chrono::high_resolution_clock::time_point m_start_time;

    void updateMetrics(const std::string& operation_name, 
                      std::chrono::nanoseconds duration);
};

/**
 * @brief RAII-style timer for automatic profiling
 */
class ScopedTimer {
public:
    ScopedTimer(PerformanceProfiler& profiler, const std::string& operation_name)
        : m_profiler(profiler)
        , m_timer_id(profiler.startTiming(operation_name)) {}

    ~ScopedTimer() {
        m_profiler.endTiming(m_timer_id);
    }

    // Non-copyable, non-movable
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
    ScopedTimer(ScopedTimer&&) = delete;
    ScopedTimer& operator=(ScopedTimer&&) = delete;

private:
    PerformanceProfiler& m_profiler;
    uint64_t m_timer_id;
};

} // namespace eti::modern

// Convenience macro for scoped timing
#define PROFILE_SCOPE(profiler, name) \
    eti::modern::ScopedTimer _timer(profiler, name)
