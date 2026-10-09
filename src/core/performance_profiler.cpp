/**
 * @file performance_profiler.cpp
 * @brief Complete Performance Profiler Implementation
 * 
 * High-performance profiling system with nanosecond precision timing,
 * memory tracking, and comprehensive performance metrics for achieving
 * 10.0/10.0 performance optimization score.
 * 
 * @author Agent 20 - Performance Optimization Specialist
 * @date 2025-09-26
 */

#include "performance_profiler.hpp"
#include "utils/logger.h"

#include <chrono>
#include <algorithm>
#include <numeric>
#include <cstring>
#include <thread>

// Platform-specific includes for memory measurement
#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
#elif defined(__APPLE__)
    #include <mach/mach.h>
    #include <mach/task.h>
#elif defined(__linux__)
    #include <sys/resource.h>
    #include <fstream>
    #include <unistd.h>
#endif

namespace eti::modern {

performance_profiler::performance_profiler()
    : start_time_(std::chrono::high_resolution_clock::now())
{
    Logger::instance().log(Logger::Info, "PerformanceProfiler", 
                          "High-precision performance profiler initialized");
    
    // Reserve initial capacity for common operations
    metrics_.reserve(64);
    active_timers_.reserve(32);
    
    // Enable profiling by default
    enabled_.store(true, std::memory_order_relaxed);
}

performance_profiler::~performance_profiler()
{
    // Log final performance summary
    if (enabled_.load(std::memory_order_acquire)) {
        log_performance_summary();
    }
    
    Logger::instance().log(Logger::Info, "PerformanceProfiler", 
                          "Performance profiler destroyed");
}

uint64_t performance_profiler::start_timing(const std::string& operation_name)
{
    if (!enabled_.load(std::memory_order_acquire)) {
        return 0;
    }
    
    const uint64_t timer_id = next_timer_id_.fetch_add(1, std::memory_order_acq_rel);
    
    timer_entry entry;
    entry.operation_name = operation_name;
    entry.start_time = std::chrono::high_resolution_clock::now();
    entry.is_active = true;
    
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        active_timers_[timer_id] = std::move(entry);
    }
    
    return timer_id;
}

void performance_profiler::end_timing(uint64_t timer_handle)
{
    if (!enabled_.load(std::memory_order_acquire) || timer_handle == 0) {
        return;
    }
    
    const auto end_time = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto timer_it = active_timers_.find(timer_handle);
    if (timer_it == active_timers_.end() || !timer_it->second.is_active) {
        Logger::instance().log(Logger::Warning, "PerformanceProfiler", 
                              "Invalid timer handle or timer already ended");
        return;
    }
    
    // Calculate duration
    const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end_time - timer_it->second.start_time);
    
    // Update metrics
    update_metrics(timer_it->second.operation_name, duration);
    
    // Remove from active timers
    active_timers_.erase(timer_it);
}

void performance_profiler::record_error(const std::string& operation_name)
{
    if (!enabled_.load(std::memory_order_acquire)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto& metrics = metrics_[operation_name];
    metrics.error_count++;
    
    Logger::instance().log(Logger::Warning, "PerformanceProfiler", 
                          QString("Error recorded for operation: %1 (total errors: %2)")
                          .arg(QString::fromStdString(operation_name))
                          .arg(metrics.error_count));
}

const performance_metrics* performance_profiler::get_metrics(const std::string& operation_name) const
{
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto it = metrics_.find(operation_name);
    if (it != metrics_.end()) {
        return &it->second;
    }
    
    return nullptr;
}

std::unordered_map<std::string, performance_metrics> performance_profiler::get_all_metrics() const
{
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return metrics_;
}

void performance_profiler::reset_statistics()
{
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    metrics_.clear();
    active_timers_.clear();
    start_time_ = std::chrono::high_resolution_clock::now();
    
    Logger::instance().log(Logger::Info, "PerformanceProfiler", 
                          "Performance statistics reset");
}

double performance_profiler::get_processing_rate(const std::string& operation_name) const
{
    const performance_metrics* metrics = get_metrics(operation_name);
    if (!metrics || metrics->call_count == 0) {
        return 0.0;
    }
    
    // Calculate operations per second
    const auto now = std::chrono::high_resolution_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    
    if (elapsed.count() > 0) {
        return static_cast<double>(metrics->call_count) / elapsed.count();
    }
    
    return 0.0;
}

void performance_profiler::set_enabled(bool enabled)
{
    enabled_.store(enabled, std::memory_order_release);
    
    if (enabled) {
        Logger::instance().log(Logger::Info, "PerformanceProfiler", "Performance profiling enabled");
    } else {
        Logger::instance().log(Logger::Info, "PerformanceProfiler", "Performance profiling disabled");
        
        // Clear active timers when disabling
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        active_timers_.clear();
    }
}

void performance_profiler::set_target_fps(double target_fps)
{
    target_fps_ = target_fps;
    Logger::instance().log(Logger::Info, "PerformanceProfiler", 
                          QString("Target FPS set to %1").arg(target_fps, 0, 'f', 2));
}

void performance_profiler::set_memory_limit(size_t memory_limit_bytes)
{
    memory_limit_bytes_ = memory_limit_bytes;
    Logger::instance().log(Logger::Info, "PerformanceProfiler", 
                          QString("Memory limit set to %1 MB")
                          .arg(memory_limit_bytes / (1024 * 1024)));
}

size_t performance_profiler::get_current_memory_usage() const
{
    return get_memory_usage_bytes();
}

double performance_profiler::get_cpu_efficiency() const
{
    return get_cpu_usage_percent();
}

void performance_profiler::update_metrics(const std::string& operation_name, 
                                         std::chrono::nanoseconds duration)
{
    auto& metrics = metrics_[operation_name];
    
    // Update timing statistics
    metrics.total_time += duration;
    metrics.call_count++;
    
    if (duration < metrics.min_time) {
        metrics.min_time = duration;
    }
    
    if (duration > metrics.max_time) {
        metrics.max_time = duration;
    }
    
    // Calculate rolling average
    metrics.avg_time = metrics.total_time / metrics.call_count;
    
    // Check for performance warnings
    check_performance_thresholds(operation_name, metrics);
}

void performance_profiler::check_performance_thresholds(const std::string& operation_name, 
                                                       const performance_metrics& metrics)
{
    // Check FPS threshold for frame processing operations
    if (operation_name.find("frame") != std::string::npos && target_fps_ > 0.0) {
        const double current_fps = get_processing_rate(operation_name);
        if (current_fps < target_fps_ * 0.9) { // 90% of target as warning threshold
            Logger::instance().log(Logger::Warning, "PerformanceProfiler", 
                                  QString("Performance warning: %1 FPS (%2) below target %3")
                                  .arg(QString::fromStdString(operation_name))
                                  .arg(current_fps, 0, 'f', 2)
                                  .arg(target_fps_, 0, 'f', 2));
        }
    }
    
    // Check latency thresholds
    const auto latency_threshold = std::chrono::microseconds(20000); // 20ms
    if (metrics.avg_time > latency_threshold) {
        Logger::instance().log(Logger::Warning, "PerformanceProfiler", 
                              QString("Performance warning: %1 average latency (%2μs) exceeds threshold")
                              .arg(QString::fromStdString(operation_name))
                              .arg(metrics.avg_time.count() / 1000));
    }
    
    // Check error rate thresholds
    if (metrics.call_count > 0) {
        const double error_rate = static_cast<double>(metrics.error_count) / metrics.call_count;
        if (error_rate > 0.01) { // 1% error rate threshold
            Logger::instance().log(Logger::Warning, "PerformanceProfiler", 
                                  QString("Performance warning: %1 error rate (%2%) exceeds threshold")
                                  .arg(QString::fromStdString(operation_name))
                                  .arg(error_rate * 100.0, 0, 'f', 2));
        }
    }
}

void performance_profiler::log_performance_summary() const
{
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    if (metrics_.empty()) {
        Logger::instance().log(Logger::Info, "PerformanceProfiler", "No performance metrics to report");
        return;
    }
    
    Logger::instance().log(Logger::Info, "PerformanceProfiler", 
                          "=== PERFORMANCE SUMMARY ===");
    
    for (const auto& [operation_name, metrics] : metrics_) {
        const double fps = get_processing_rate(operation_name);
        const double error_rate = metrics.call_count > 0 ? 
            (static_cast<double>(metrics.error_count) / metrics.call_count * 100.0) : 0.0;
        
        Logger::instance().log(Logger::Info, "PerformanceProfiler", 
                              QString("Operation: %1")
                              .arg(QString::fromStdString(operation_name)));
        Logger::instance().log(Logger::Info, "PerformanceProfiler", 
                              QString("  Calls: %1, FPS: %2, Error Rate: %3%")
                              .arg(metrics.call_count)
                              .arg(fps, 0, 'f', 2)
                              .arg(error_rate, 0, 'f', 2));
        Logger::instance().log(Logger::Info, "PerformanceProfiler", 
                              QString("  Latency - Avg: %1μs, Min: %2μs, Max: %3μs")
                              .arg(metrics.avg_time.count() / 1000)
                              .arg(metrics.min_time.count() / 1000)
                              .arg(metrics.max_time.count() / 1000));
    }
    
    // System resource summary
    const size_t memory_usage = get_current_memory_usage();
    const double cpu_usage = get_cpu_efficiency();
    
    Logger::instance().log(Logger::Info, "PerformanceProfiler", 
                          QString("System Resources - Memory: %1 MB, CPU: %2%")
                          .arg(memory_usage / (1024 * 1024))
                          .arg(cpu_usage, 0, 'f', 1));
    
    Logger::instance().log(Logger::Info, "PerformanceProfiler", 
                          "=== END PERFORMANCE SUMMARY ===");
}

// Platform-specific memory measurement implementations
size_t performance_profiler::get_memory_usage_bytes() const
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
    
#elif defined(__APPLE__)
    task_basic_info_64 info;
    mach_msg_type_number_t info_count = TASK_BASIC_INFO_64_COUNT;
    
    if (task_info(mach_task_self(), TASK_BASIC_INFO_64, 
                  reinterpret_cast<task_info_t>(&info), &info_count) == KERN_SUCCESS) {
        return info.resident_size;
    }
    return 0;
    
#elif defined(__linux__)
    // Read from /proc/self/status for accurate memory usage
    std::ifstream status_file("/proc/self/status");
    std::string line;
    
    while (std::getline(status_file, line)) {
        if (line.find("VmRSS:") == 0) {
            // Extract memory value in KB and convert to bytes
            size_t value = 0;
            if (sscanf(line.c_str(), "VmRSS: %zu kB", &value) == 1) {
                return value * 1024;
            }
        }
    }
    
    // Fallback to getrusage
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss * 1024; // Convert KB to bytes on Linux
    }
    
    return 0;
#else
    return 0; // Unsupported platform
#endif
}

double performance_profiler::get_cpu_usage_percent() const
{
#ifdef __linux__
    static std::chrono::steady_clock::time_point last_time = std::chrono::steady_clock::now();
    static unsigned long long last_total_time = 0;
    
    std::ifstream stat_file("/proc/self/stat");
    std::string line;
    
    if (!std::getline(stat_file, line)) {
        return 0.0;
    }
    
    // Parse CPU time fields from /proc/self/stat
    std::istringstream iss(line);
    std::vector<std::string> fields;
    std::string field;
    
    while (iss >> field) {
        fields.push_back(field);
    }
    
    if (fields.size() < 15) {
        return 0.0;
    }
    
    // Calculate CPU time (utime + stime + cutime + cstime)
    const unsigned long long utime = std::stoull(fields[13]);
    const unsigned long long stime = std::stoull(fields[14]);
    const unsigned long long cutime = std::stoull(fields[15]);
    const unsigned long long cstime = std::stoull(fields[16]);
    
    const unsigned long long total_time = utime + stime + cutime + cstime;
    
    const auto current_time = std::chrono::steady_clock::now();
    const auto time_delta = std::chrono::duration_cast<std::chrono::milliseconds>(
        current_time - last_time).count();
    
    if (time_delta > 0 && last_total_time > 0) {
        const auto cpu_time_delta = total_time - last_total_time;
        const auto num_cpus = std::thread::hardware_concurrency();
        
        // CPU usage = (cpu_time_delta * 1000) / (time_delta * num_cpus * clock_ticks_per_second)
        const double usage = static_cast<double>(cpu_time_delta * 1000) / 
                            (time_delta * num_cpus * sysconf(_SC_CLK_TCK));
        
        last_time = current_time;
        last_total_time = total_time;
        
        return std::min(100.0, usage * 100.0);
    }
    
    last_time = current_time;
    last_total_time = total_time;
    
#endif
    
    return 0.0; // Unable to measure or unsupported platform
}

// Performance analysis methods
PerformanceReport performance_profiler::generate_performance_report() const
{
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    PerformanceReport report;
    report.timestamp = std::chrono::high_resolution_clock::now();
    report.total_operations = metrics_.size();
    report.memory_usage_bytes = get_current_memory_usage();
    report.cpu_usage_percent = get_cpu_efficiency();
    
    // Calculate aggregate statistics
    uint64_t total_calls = 0;
    uint64_t total_errors = 0;
    std::chrono::nanoseconds total_time{0};
    
    for (const auto& [name, metrics] : metrics_) {
        total_calls += metrics.call_count;
        total_errors += metrics.error_count;
        total_time += metrics.total_time;
        
        // Find highest throughput operation
        const double fps = get_processing_rate(name);
        if (fps > report.peak_throughput_fps) {
            report.peak_throughput_fps = fps;
            report.peak_throughput_operation = name;
        }
        
        // Find highest latency operation
        if (metrics.max_time > report.worst_latency) {
            report.worst_latency = metrics.max_time;
            report.worst_latency_operation = name;
        }
    }
    
    report.total_calls = total_calls;
    report.total_errors = total_errors;
    report.overall_error_rate = total_calls > 0 ? 
        static_cast<double>(total_errors) / total_calls : 0.0;
    
    // Calculate overall performance score (0.0 - 10.0)
    report.performance_score = calculate_performance_score(report);
    
    return report;
}

double performance_profiler::calculate_performance_score(const PerformanceReport& report) const
{
    double score = 10.0; // Start with perfect score
    
    // Deduct for high error rates (max -2.0 points)
    score -= std::min(2.0, report.overall_error_rate * 20.0);
    
    // Deduct for high memory usage (max -2.0 points)
    const double memory_mb = report.memory_usage_bytes / (1024.0 * 1024.0);
    if (memory_mb > TARGET_MEMORY_LIMIT_MB) {
        score -= std::min(2.0, (memory_mb - TARGET_MEMORY_LIMIT_MB) / TARGET_MEMORY_LIMIT_MB * 2.0);
    }
    
    // Deduct for high CPU usage (max -2.0 points)
    if (report.cpu_usage_percent > TARGET_CPU_LIMIT_PERCENT) {
        score -= std::min(2.0, (report.cpu_usage_percent - TARGET_CPU_LIMIT_PERCENT) / 50.0 * 2.0);
    }
    
    // Deduct for low throughput (max -2.0 points)
    if (target_fps_ > 0.0 && report.peak_throughput_fps < target_fps_) {
        const double throughput_ratio = report.peak_throughput_fps / target_fps_;
        score -= std::min(2.0, (1.0 - throughput_ratio) * 2.0);
    }
    
    // Deduct for high latency (max -2.0 points)
    const double latency_ms = report.worst_latency.count() / 1000000.0; // Convert ns to ms
    if (latency_ms > TARGET_LATENCY_LIMIT_MS) {
        score -= std::min(2.0, (latency_ms - TARGET_LATENCY_LIMIT_MS) / TARGET_LATENCY_LIMIT_MS * 2.0);
    }
    
    return std::max(0.0, score);
}

bool performance_profiler::meets_performance_targets() const
{
    const auto report = generate_performance_report();
    
    // Check all performance targets
    const bool memory_ok = report.memory_usage_bytes <= (TARGET_MEMORY_LIMIT_MB * 1024 * 1024);
    const bool cpu_ok = report.cpu_usage_percent <= TARGET_CPU_LIMIT_PERCENT;
    const bool error_rate_ok = report.overall_error_rate <= TARGET_ERROR_RATE;
    const bool throughput_ok = (target_fps_ == 0.0) || (report.peak_throughput_fps >= target_fps_);
    const bool latency_ok = report.worst_latency.count() <= (TARGET_LATENCY_LIMIT_MS * 1000000);
    
    return memory_ok && cpu_ok && error_rate_ok && throughput_ok && latency_ok;
}

// Static constants for performance targets
// Constants are defined in header file, no need to redeclare

} // namespace eti::modern