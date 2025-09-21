/**
 * @file performance_profiler.cpp
 * @brief Performance Profiler Implementation
 */

#include "performance_profiler.h"
#include "utils/logger.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <sys/resource.h>
#include <fstream>

namespace eti::modern {

PerformanceProfiler::PerformanceProfiler() {
    recent_frame_times_.reserve(1000); // Track last 1000 frames
    
    Logger::instance().log(Logger::Info, "PerformanceProfiler", 
                          "Performance profiler initialized");
}

PerformanceProfiler::~PerformanceProfiler() {
    stop();
    
    Logger::instance().log(Logger::Info, "PerformanceProfiler", 
                          "Performance profiler destroyed");
}

void PerformanceProfiler::start() {
    if (profiling_active_.load()) {
        return; // Already started
    }
    
    reset();
    profiling_active_.store(true);
    start_time_ = std::chrono::steady_clock::now();
    last_update_ = start_time_;
    
    // Start CPU monitoring thread
    cpu_monitoring_active_.store(true);
    cpu_monitor_thread_ = std::make_unique<std::thread>(&PerformanceProfiler::cpuMonitorLoop, this);
    
    Logger::instance().log(Logger::Info, "PerformanceProfiler", 
                          "Performance profiling started");
}

void PerformanceProfiler::stop() {
    if (!profiling_active_.load()) {
        return; // Already stopped
    }
    
    profiling_active_.store(false);
    
    // Stop CPU monitoring
    cpu_monitoring_active_.store(false);
    if (cpu_monitor_thread_ && cpu_monitor_thread_->joinable()) {
        cpu_monitor_thread_->join();
    }
    cpu_monitor_thread_.reset();
    
    // Generate final report
    auto report = generatePerformanceReport();
    Logger::instance().log(Logger::Info, "PerformanceProfiler", 
                          QString("Performance profiling stopped\n%1").arg(QString::fromStdString(report)));
}

void PerformanceProfiler::reset() {
    metrics_ = DetailedPerformanceMetrics{};
    
    std::lock_guard<std::mutex> lock(frame_times_mutex_);
    recent_frame_times_.clear();
    
    start_time_ = std::chrono::steady_clock::now();
    last_update_ = start_time_;
    
    Logger::instance().log(Logger::Debug, "PerformanceProfiler", 
                          "Performance metrics reset");
}

void PerformanceProfiler::recordFrameProcessing(uint64_t processing_time_ns) {
    if (!profiling_active_.load()) return;
    
    auto now = std::chrono::steady_clock::now();
    
    // Update frame processing metrics
    metrics_.frames_processed.fetch_add(1);
    metrics_.total_processing_time_ns.fetch_add(processing_time_ns);
    
    // Update peak and min processing times
    uint64_t current_peak = metrics_.peak_processing_time_ns.load();
    while (processing_time_ns > current_peak && 
           !metrics_.peak_processing_time_ns.compare_exchange_weak(current_peak, processing_time_ns)) {
        // Retry if another thread updated peak
    }
    
    uint64_t current_min = metrics_.min_processing_time_ns.load();
    while (processing_time_ns < current_min && 
           !metrics_.min_processing_time_ns.compare_exchange_weak(current_min, processing_time_ns)) {
        // Retry if another thread updated min
    }
    
    // Update frame times for FPS calculation
    {
        std::lock_guard<std::mutex> lock(frame_times_mutex_);
        recent_frame_times_.push_back(now);
        cleanupOldFrameTimes();
    }
    
    // Update target validation
    updateTargetValidation();
}

void PerformanceProfiler::recordMemoryAllocation(size_t bytes) {
    if (!profiling_active_.load()) return;
    
    metrics_.allocated_memory_bytes.fetch_add(bytes);
    
    size_t new_current = metrics_.current_memory_bytes.fetch_add(bytes) + bytes;
    
    // Update peak memory usage
    size_t current_peak = metrics_.peak_memory_bytes.load();
    while (new_current > current_peak && 
           !metrics_.peak_memory_bytes.compare_exchange_weak(current_peak, new_current)) {
        // Retry if another thread updated peak
    }
    
    updateMemoryMetrics();
}

void PerformanceProfiler::recordMemoryDeallocation(size_t bytes) {
    if (!profiling_active_.load()) return;
    
    metrics_.deallocated_memory_bytes.fetch_add(bytes);
    
    size_t current = metrics_.current_memory_bytes.load();
    if (current >= bytes) {
        metrics_.current_memory_bytes.fetch_sub(bytes);
    }
    
    updateMemoryMetrics();
}

void PerformanceProfiler::recordCacheEvent(bool hit) {
    if (!profiling_active_.load()) return;
    
    if (hit) {
        metrics_.cache_hits.fetch_add(1);
    } else {
        metrics_.cache_misses.fetch_add(1);
    }
}

void PerformanceProfiler::recordCacheEviction() {
    if (!profiling_active_.load()) return;
    
    metrics_.cache_evictions.fetch_add(1);
}

void PerformanceProfiler::setTargets(double target_fps, double baseline_fps, 
                                    double memory_limit_mb, double baseline_memory_mb) {
    target_fps_ = target_fps;
    baseline_fps_ = baseline_fps;
    memory_limit_bytes_ = static_cast<size_t>(memory_limit_mb * 1024 * 1024);
    baseline_memory_bytes_ = static_cast<size_t>(baseline_memory_mb * 1024 * 1024);
    
    Logger::instance().log(Logger::Info, "PerformanceProfiler", 
                          QString("Performance targets set - FPS: %1 (baseline: %2), Memory: %3 MB (baseline: %4 MB)")
                          .arg(target_fps, 0, 'f', 1)
                          .arg(baseline_fps, 0, 'f', 1)
                          .arg(memory_limit_mb, 0, 'f', 1)
                          .arg(baseline_memory_mb, 0, 'f', 1));
}

double PerformanceProfiler::getCurrentFPS() const {
    return calculateCurrentFPS();
}

bool PerformanceProfiler::areTargetsMet() const {
    return metrics_.speed_target_met.load() && metrics_.memory_target_met.load();
}

BenchmarkResult PerformanceProfiler::getBenchmarkResults() const {
    BenchmarkResult result;
    
    // FPS metrics
    result.average_fps = calculateAverageFPS();
    result.peak_fps = calculateCurrentFPS(); // Simplified for now
    result.min_fps = result.average_fps * 0.8; // Estimate
    
    // Memory metrics
    result.average_memory_mb = static_cast<double>(metrics_.current_memory_bytes.load()) / (1024.0 * 1024.0);
    result.peak_memory_mb = static_cast<double>(metrics_.peak_memory_bytes.load()) / (1024.0 * 1024.0);
    
    // CPU and cache metrics
    result.cpu_efficiency = metrics_.cpu_efficiency.load();
    
    uint64_t hits = metrics_.cache_hits.load();
    uint64_t misses = metrics_.cache_misses.load();
    result.cache_hit_rate = (hits + misses > 0) ? 
        (static_cast<double>(hits) / (hits + misses)) : 0.0;
    
    // Target validation
    result.speed_improvement_percent = metrics_.speed_improvement_percent.load();
    result.memory_reduction_percent = metrics_.memory_reduction_percent.load();
    result.meets_speed_target = metrics_.speed_target_met.load();
    result.meets_memory_target = metrics_.memory_target_met.load();
    
    // Performance notes
    if (result.meets_speed_target) {
        result.performance_notes.push_back("Speed target achieved");
    } else {
        result.performance_notes.push_back("Speed target not met");
    }
    
    if (result.meets_memory_target) {
        result.performance_notes.push_back("Memory target achieved");
    } else {
        result.performance_notes.push_back("Memory target not met");
    }
    
    // Optimization recommendations
    result.optimization_recommendations = getOptimizationRecommendations();
    
    return result;
}

std::string PerformanceProfiler::generatePerformanceReport() const {
    std::ostringstream report;
    
    auto benchmark = getBenchmarkResults();
    
    report << "=== ETI Processing Performance Report ===\n";
    report << std::fixed << std::setprecision(2);
    
    // Processing performance
    report << "\nProcessing Performance:\n";
    report << "  Frames Processed: " << metrics_.frames_processed.load() << "\n";
    report << "  Average FPS: " << benchmark.average_fps << "\n";
    report << "  Target FPS: " << target_fps_ << "\n";
    report << "  Speed Improvement: " << benchmark.speed_improvement_percent << "%\n";
    report << "  Speed Target Met: " << (benchmark.meets_speed_target ? "YES" : "NO") << "\n";
    
    // Memory performance
    report << "\nMemory Performance:\n";
    report << "  Current Usage: " << benchmark.average_memory_mb << " MB\n";
    report << "  Peak Usage: " << benchmark.peak_memory_mb << " MB\n";
    report << "  Memory Limit: " << (memory_limit_bytes_ / (1024.0 * 1024.0)) << " MB\n";
    report << "  Memory Reduction: " << benchmark.memory_reduction_percent << "%\n";
    report << "  Memory Target Met: " << (benchmark.meets_memory_target ? "YES" : "NO") << "\n";
    
    // Cache performance
    report << "\nCache Performance:\n";
    report << "  Cache Hit Rate: " << (benchmark.cache_hit_rate * 100.0) << "%\n";
    report << "  Cache Hits: " << metrics_.cache_hits.load() << "\n";
    report << "  Cache Misses: " << metrics_.cache_misses.load() << "\n";
    report << "  Cache Evictions: " << metrics_.cache_evictions.load() << "\n";
    
    // CPU performance
    report << "\nCPU Performance:\n";
    report << "  CPU Efficiency: " << (benchmark.cpu_efficiency * 100.0) << "%\n";
    report << "  CPU Usage: " << metrics_.cpu_usage_percent.load() << "%\n";
    
    // Overall assessment
    report << "\nOverall Assessment:\n";
    if (benchmark.meets_speed_target && benchmark.meets_memory_target) {
        report << "  Status: ALL TARGETS MET - Phase 2 migration successful\n";
    } else {
        report << "  Status: TARGETS NOT MET - optimization needed\n";
    }
    
    // Recommendations
    if (!benchmark.optimization_recommendations.empty()) {
        report << "\nOptimization Recommendations:\n";
        for (const auto& rec : benchmark.optimization_recommendations) {
            report << "  - " << rec << "\n";
        }
    }
    
    return report.str();
}

std::vector<std::string> PerformanceProfiler::getOptimizationRecommendations() const {
    std::vector<std::string> recommendations;
    
    auto current_fps = calculateAverageFPS();
    auto memory_mb = static_cast<double>(metrics_.current_memory_bytes.load()) / (1024.0 * 1024.0);
    auto cache_hit_rate = static_cast<double>(metrics_.cache_hits.load()) / 
                         std::max(1UL, metrics_.cache_hits.load() + metrics_.cache_misses.load());
    
    // Speed optimizations
    if (current_fps < target_fps_) {
        recommendations.push_back("Enable SIMD optimizations for frame parsing");
        recommendations.push_back("Increase thread pool size for batch processing");
        recommendations.push_back("Optimize FIG parsing algorithms");
    }
    
    // Memory optimizations
    if (memory_mb > (memory_limit_bytes_ / (1024.0 * 1024.0))) {
        recommendations.push_back("Reduce cache size to lower memory usage");
        recommendations.push_back("Enable memory pooling for better allocation patterns");
        recommendations.push_back("Implement more aggressive cache eviction policies");
    }
    
    // Cache optimizations
    if (cache_hit_rate < 0.8) {
        recommendations.push_back("Increase cache size for better hit rates");
        recommendations.push_back("Implement smarter cache key generation");
        recommendations.push_back("Use LRU cache replacement strategy");
    }
    
    // CPU optimizations
    if (metrics_.cpu_efficiency.load() < 0.7) {
        recommendations.push_back("Reduce thread contention in critical sections");
        recommendations.push_back("Optimize hot paths in frame processing");
        recommendations.push_back("Consider lock-free data structures");
    }
    
    return recommendations;
}

// Private methods

void PerformanceProfiler::updateCPUMetrics() {
    // Simple CPU usage calculation based on processing time
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - last_update_);
    
    if (elapsed.count() > 0) {
        uint64_t processing_time = metrics_.total_processing_time_ns.load();
        double efficiency = static_cast<double>(processing_time) / elapsed.count();
        metrics_.cpu_efficiency.store(std::min(1.0, efficiency));
    }
    
    last_update_ = now;
}

void PerformanceProfiler::updateMemoryMetrics() {
    // Update memory target validation
    size_t current_memory = metrics_.current_memory_bytes.load();
    double reduction_percent = 0.0;
    
    if (baseline_memory_bytes_ > 0 && current_memory < baseline_memory_bytes_) {
        reduction_percent = ((baseline_memory_bytes_ - current_memory) * 100.0) / baseline_memory_bytes_;
    }
    
    metrics_.memory_reduction_percent.store(reduction_percent);
    metrics_.memory_target_met.store(reduction_percent >= 40.0);
}

void PerformanceProfiler::updateTargetValidation() {
    // Update speed improvement calculation
    double current_fps = calculateAverageFPS();
    double improvement_percent = 0.0;
    
    if (baseline_fps_ > 0 && current_fps > baseline_fps_) {
        improvement_percent = ((current_fps - baseline_fps_) * 100.0) / baseline_fps_;
    }
    
    metrics_.speed_improvement_percent.store(improvement_percent);
    metrics_.speed_target_met.store(improvement_percent >= 30.0);
    
    updateMemoryMetrics();
}

void PerformanceProfiler::cpuMonitorLoop() {
    while (cpu_monitoring_active_.load()) {
        updateCPUMetrics();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

double PerformanceProfiler::calculateCurrentFPS() const {
    std::lock_guard<std::mutex> lock(frame_times_mutex_);
    
    if (recent_frame_times_.size() < 2) {
        return 0.0;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto window_start = now - std::chrono::seconds(1); // 1-second window
    
    auto start_it = std::lower_bound(recent_frame_times_.begin(), recent_frame_times_.end(), window_start);
    size_t frames_in_window = std::distance(start_it, recent_frame_times_.end());
    
    return static_cast<double>(frames_in_window);
}

double PerformanceProfiler::calculateAverageFPS() const {
    if (!profiling_active_.load()) return 0.0;
    
    uint64_t frames = metrics_.frames_processed.load();
    if (frames == 0) return 0.0;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    
    if (elapsed.count() > 0) {
        return static_cast<double>(frames) / elapsed.count();
    }
    
    return 0.0;
}

void PerformanceProfiler::cleanupOldFrameTimes() {
    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - std::chrono::seconds(10); // Keep last 10 seconds
    
    auto it = std::lower_bound(recent_frame_times_.begin(), recent_frame_times_.end(), cutoff);
    recent_frame_times_.erase(recent_frame_times_.begin(), it);
    
    // Limit size to prevent excessive memory usage
    if (recent_frame_times_.size() > 1000) {
        recent_frame_times_.erase(recent_frame_times_.begin(), 
                                 recent_frame_times_.begin() + (recent_frame_times_.size() - 1000));
    }
}

} // namespace eti::modern