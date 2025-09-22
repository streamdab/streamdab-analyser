/**
 * @file performance_profiler.cpp
 * @brief Real-time Performance Profiler implementation
 * 
 * @author Build Manager Agent
 * @date 2025
 * @copyright StreamDAB Analyzer Project
 */

#include "performance_profiler.h"
#include <algorithm>

namespace eti::modern {

PerformanceProfiler::PerformanceProfiler() 
    : m_start_time(std::chrono::high_resolution_clock::now()) {
}

PerformanceProfiler::~PerformanceProfiler() = default;

uint64_t PerformanceProfiler::startTiming(const std::string& operation_name) {
    if (!m_enabled) {
        return 0;
    }

    uint64_t timer_id = m_next_timer_id++;
    
    TimerEntry entry;
    entry.operation_name = operation_name;
    entry.start_time = std::chrono::high_resolution_clock::now();
    entry.is_active = true;
    
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    m_active_timers[timer_id] = entry;
    
    return timer_id;
}

void PerformanceProfiler::endTiming(uint64_t timer_handle) {
    if (!m_enabled || timer_handle == 0) {
        return;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    
    auto it = m_active_timers.find(timer_handle);
    if (it != m_active_timers.end() && it->second.is_active) {
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - it->second.start_time);
        
        updateMetrics(it->second.operation_name, duration);
        m_active_timers.erase(it);
    }
}

void PerformanceProfiler::recordError(const std::string& operation_name) {
    if (!m_enabled) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    m_metrics[operation_name].error_count++;
}

const PerformanceMetrics* PerformanceProfiler::getMetrics(const std::string& operation_name) const {
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    
    auto it = m_metrics.find(operation_name);
    return (it != m_metrics.end()) ? &it->second : nullptr;
}

std::unordered_map<std::string, PerformanceMetrics> PerformanceProfiler::getAllMetrics() const {
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    return m_metrics;
}

void PerformanceProfiler::resetStatistics() {
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    m_metrics.clear();
    m_active_timers.clear();
    m_start_time = std::chrono::high_resolution_clock::now();
}

double PerformanceProfiler::getProcessingRate(const std::string& operation_name) const {
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    
    auto it = m_metrics.find(operation_name);
    if (it == m_metrics.end()) {
        return 0.0;
    }
    
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_start_time);
    
    if (elapsed.count() > 0) {
        return double(it->second.call_count) / double(elapsed.count());
    }
    
    return 0.0;
}

void PerformanceProfiler::setEnabled(bool enabled) {
    m_enabled = enabled;
}

void PerformanceProfiler::setTargetFPS(double target_fps) {
    // Implementation for target FPS tracking
    // This would be used for performance comparison metrics
}

void PerformanceProfiler::setMemoryLimit(size_t memory_limit_bytes) {
    // Implementation for memory limit tracking
    // This would be used for memory usage monitoring
}

void PerformanceProfiler::updateMetrics(const std::string& operation_name, 
                                       std::chrono::nanoseconds duration) {
    auto& metrics = m_metrics[operation_name];
    
    metrics.total_time += duration;
    metrics.call_count++;
    
    if (duration < metrics.min_time) {
        metrics.min_time = duration;
    }
    
    if (duration > metrics.max_time) {
        metrics.max_time = duration;
    }
    
    if (metrics.call_count > 0) {
        metrics.avg_time = metrics.total_time / metrics.call_count;
    }
}

size_t PerformanceProfiler::getCurrentMemoryUsage() const {
    // Simple memory usage calculation - in production this would use platform-specific APIs
    size_t memory_usage = 0;
    
    // Estimate memory usage from metrics
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    for (const auto& [name, metrics] : m_metrics) {
        memory_usage += sizeof(metrics) + name.size();
    }
    
    // Add active timer memory
    memory_usage += m_active_timers.size() * sizeof(TimerEntry);
    
    // For now, return a reasonable estimate
    return memory_usage + (1024 * 1024); // 1MB base + metrics overhead
}

double PerformanceProfiler::getCPUEfficiency() const {
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    
    if (m_metrics.empty()) {
        return 100.0; // No data = perfect efficiency
    }
    
    // Calculate efficiency based on processing speed vs errors
    double total_operations = 0;
    double total_errors = 0;
    
    for (const auto& [name, metrics] : m_metrics) {
        total_operations += metrics.call_count;
        total_errors += metrics.error_count;
    }
    
    if (total_operations == 0) {
        return 100.0;
    }
    
    double error_rate = total_errors / total_operations;
    return std::max(0.0, 100.0 - (error_rate * 100.0));
}

} // namespace eti::modern