/**
 * @file test_performance_impact_validation.cpp
 * @brief Performance Impact Validation Tests for 100% ETSI Compliance
 * 
 * Comprehensive test suite for validating that 100% ETSI compliance optimizations
 * maintain acceptable performance impact and system efficiency.
 * 
 * @author TDD Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 * @version 1.0
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <future>
#include <algorithm>
#include <numeric>

// Core system headers
#include "../../src/core/etsi/compliance_engine.h"
#include "../../src/core/etsi/alert_system.h"
#include "../../src/core/etsi/realtime_monitor.h"
#include "../../src/core/eti_processor.h"

// Performance monitoring
#include "../../src/core/performance/performance_monitor.h"
#include "../../src/core/performance/resource_monitor.h"
#include "../../src/core/performance/benchmark_runner.h"

// Test fixtures
#include "../fixtures/eti_streams/optimized_compliance_frames.h"

using namespace etsi::compliance;
using namespace etsi::alerts;
using namespace etsi::realtime;
using namespace performance;

using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;

/**
 * @brief Performance benchmark result
 */
struct BenchmarkResult {
    std::string test_name;
    size_t iterations;
    std::chrono::microseconds total_time;
    std::chrono::microseconds average_time;
    std::chrono::microseconds min_time;
    std::chrono::microseconds max_time;
    double frames_per_second;
    double cpu_usage_percent;
    double memory_usage_mb;
    double compliance_percentage;
    bool performance_acceptable;
};

/**
 * @brief Resource usage snapshot
 */
struct ResourceSnapshot {
    std::chrono::high_resolution_clock::time_point timestamp;
    double cpu_usage_percent;
    double memory_usage_mb;
    double memory_peak_mb;
    size_t thread_count;
    double disk_io_mbps;
    double network_io_mbps;
};

/**
 * @brief Test fixture for performance impact validation
 */
class PerformanceImpactValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Configure optimized compliance engine
        EtsiComplianceEngine::Config compliance_config;
        compliance_config.target_level = ComplianceLevel::BROADCAST_QUALITY;
        compliance_config.enable_optimized_penalty_calculation = true;
        compliance_config.enable_operational_tolerance = true;
        compliance_config.enable_recovery_bonus_system = true;
        compliance_config.enable_fast_validation = true;
        compliance_config.validation_timeout = std::chrono::milliseconds(10);
        
        optimized_compliance_engine_ = std::make_unique<EtsiComplianceEngine>(compliance_config);
        ASSERT_TRUE(optimized_compliance_engine_->initialize());
        
        // Configure standard compliance engine for comparison
        EtsiComplianceEngine::Config standard_config;
        standard_config.target_level = ComplianceLevel::BROADCAST_QUALITY;
        standard_config.enable_optimized_penalty_calculation = false;
        standard_config.enable_operational_tolerance = false;
        standard_config.enable_recovery_bonus_system = false;
        standard_config.enable_fast_validation = false;
        
        standard_compliance_engine_ = std::make_unique<EtsiComplianceEngine>(standard_config);
        ASSERT_TRUE(standard_compliance_engine_->initialize());
        
        // Configure performance monitor
        PerformanceMonitorConfig perf_config;
        perf_config.enable_real_time_monitoring = true;
        perf_config.sampling_interval_ms = 100;
        perf_config.enable_detailed_profiling = true;
        
        performance_monitor_ = std::make_unique<PerformanceMonitor>(perf_config);
        ASSERT_TRUE(performance_monitor_->initialize());
        
        // Configure resource monitor
        ResourceMonitorConfig resource_config;
        resource_config.monitor_cpu = true;
        resource_config.monitor_memory = true;
        resource_config.monitor_disk_io = true;
        resource_config.monitor_network_io = true;
        resource_config.sampling_rate_hz = 10;
        
        resource_monitor_ = std::make_unique<ResourceMonitor>(resource_config);
        ASSERT_TRUE(resource_monitor_->initialize());
        
        // Configure benchmark runner
        BenchmarkRunnerConfig benchmark_config;
        benchmark_config.warmup_iterations = 10;
        benchmark_config.measurement_iterations = 100;
        benchmark_config.enable_statistical_analysis = true;
        benchmark_config.target_fps = 7000.0; // Target performance
        
        benchmark_runner_ = std::make_unique<BenchmarkRunner>(benchmark_config);
        
        // Generate test data
        test_frames_ = fixtures::eti::CreateOptimizedComplianceFrames(1000);
        
        test_start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    void TearDown() override {
        auto test_end_time = std::chrono::high_resolution_clock::now();
        auto test_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            test_end_time - test_start_time_);
        
        std::cout << "Performance impact test completed in " << test_duration.count() << " ms" << std::endl;
        
        // Print final performance summary
        print_performance_summary();
    }
    
    /**
     * @brief Benchmark compliance engine performance
     */
    BenchmarkResult BenchmarkComplianceEngine(EtsiComplianceEngine* engine, 
                                             const std::string& test_name,
                                             size_t iterations = 100) {
        BenchmarkResult result;
        result.test_name = test_name;
        result.iterations = iterations;
        
        std::vector<std::chrono::microseconds> timing_results;
        timing_results.reserve(iterations);
        
        // Start resource monitoring
        resource_monitor_->start();
        auto start_resources = resource_monitor_->get_current_snapshot();
        
        // Warmup
        for (size_t i = 0; i < 10; ++i) {
            engine->validate_eti_frame(test_frames_[i % test_frames_.size()]);
        }
        
        // Benchmark measurement
        auto benchmark_start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < iterations; ++i) {
            auto frame_start = std::chrono::high_resolution_clock::now();
            
            auto validation_result = engine->validate_eti_frame(test_frames_[i % test_frames_.size()]);
            result.compliance_percentage = validation_result.get_overall_compliance();
            
            auto frame_end = std::chrono::high_resolution_clock::now();
            auto frame_time = std::chrono::duration_cast<std::chrono::microseconds>(frame_end - frame_start);
            timing_results.push_back(frame_time);
        }
        
        auto benchmark_end = std::chrono::high_resolution_clock::now();
        
        // Get final resource snapshot
        auto end_resources = resource_monitor_->get_current_snapshot();
        resource_monitor_->stop();
        
        // Calculate statistics
        result.total_time = std::chrono::duration_cast<std::chrono::microseconds>(benchmark_end - benchmark_start);
        result.average_time = result.total_time / iterations;
        result.min_time = *std::min_element(timing_results.begin(), timing_results.end());
        result.max_time = *std::max_element(timing_results.begin(), timing_results.end());
        
        result.frames_per_second = 1000000.0 / result.average_time.count(); // Convert μs to FPS
        result.cpu_usage_percent = end_resources.cpu_usage_percent - start_resources.cpu_usage_percent;
        result.memory_usage_mb = end_resources.memory_peak_mb;
        
        // Performance acceptance criteria
        result.performance_acceptable = (result.frames_per_second >= 5000.0) && // ≥5000 FPS
                                      (result.cpu_usage_percent <= 80.0) &&   // ≤80% CPU
                                      (result.memory_usage_mb <= 100.0);      // ≤100MB memory
        
        return result;
    }
    
    /**
     * @brief Stress test with sustained load
     */
    BenchmarkResult StressTestSustainedLoad(EtsiComplianceEngine* engine,
                                          const std::string& test_name,
                                          std::chrono::seconds duration) {
        BenchmarkResult result;
        result.test_name = test_name + " (Sustained)";
        
        std::vector<std::chrono::microseconds> timing_results;
        std::vector<ResourceSnapshot> resource_snapshots;
        
        resource_monitor_->start();
        
        auto test_start = std::chrono::high_resolution_clock::now();
        auto test_end = test_start + duration;
        size_t frame_count = 0;
        
        while (std::chrono::high_resolution_clock::now() < test_end) {
            auto frame_start = std::chrono::high_resolution_clock::now();
            
            auto validation_result = engine->validate_eti_frame(test_frames_[frame_count % test_frames_.size()]);
            
            auto frame_end = std::chrono::high_resolution_clock::now();
            auto frame_time = std::chrono::duration_cast<std::chrono::microseconds>(frame_end - frame_start);
            timing_results.push_back(frame_time);
            
            // Capture resource snapshot every 100 frames
            if (frame_count % 100 == 0) {
                resource_snapshots.push_back(resource_monitor_->get_current_snapshot());
            }
            
            frame_count++;
            
            // Brief pause to prevent overwhelming system
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        auto actual_end = std::chrono::high_resolution_clock::now();
        resource_monitor_->stop();
        
        // Calculate sustained performance statistics
        result.iterations = frame_count;
        result.total_time = std::chrono::duration_cast<std::chrono::microseconds>(actual_end - test_start);
        result.average_time = result.total_time / frame_count;
        result.frames_per_second = static_cast<double>(frame_count) / (result.total_time.count() / 1000000.0);
        
        // Calculate resource usage statistics
        if (!resource_snapshots.empty()) {
            double total_cpu = 0.0, max_memory = 0.0;
            for (const auto& snapshot : resource_snapshots) {
                total_cpu += snapshot.cpu_usage_percent;
                max_memory = std::max(max_memory, snapshot.memory_usage_mb);
            }
            result.cpu_usage_percent = total_cpu / resource_snapshots.size();
            result.memory_usage_mb = max_memory;
        }
        
        // Sustained performance acceptance criteria
        result.performance_acceptable = (result.frames_per_second >= 1000.0) && // ≥1000 FPS sustained
                                      (result.cpu_usage_percent <= 85.0) &&   // ≤85% CPU sustained
                                      (result.memory_usage_mb <= 150.0);      // ≤150MB peak memory
        
        return result;
    }
    
    void print_performance_summary() {
        std::cout << "\nPerformance Impact Summary:" << std::endl;
        std::cout << "==========================" << std::endl;
        
        for (const auto& result : benchmark_results_) {
            std::cout << "Test: " << result.test_name << std::endl;
            std::cout << "  FPS: " << result.frames_per_second << std::endl;
            std::cout << "  CPU: " << result.cpu_usage_percent << "%" << std::endl;
            std::cout << "  Memory: " << result.memory_usage_mb << " MB" << std::endl;
            std::cout << "  Compliance: " << result.compliance_percentage << "%" << std::endl;
            std::cout << "  Status: " << (result.performance_acceptable ? "PASS" : "FAIL") << std::endl;
        }
    }
    
    std::unique_ptr<EtsiComplianceEngine> optimized_compliance_engine_;
    std::unique_ptr<EtsiComplianceEngine> standard_compliance_engine_;
    std::unique_ptr<PerformanceMonitor> performance_monitor_;
    std::unique_ptr<ResourceMonitor> resource_monitor_;
    std::unique_ptr<BenchmarkRunner> benchmark_runner_;
    std::vector<EtiFrame> test_frames_;
    std::vector<BenchmarkResult> benchmark_results_;
    std::chrono::high_resolution_clock::time_point test_start_time_;
};

/**
 * @brief Test optimized vs standard compliance engine performance
 */
TEST_F(PerformanceImpactValidationTest, testOptimizedVsStandardPerformance) {
    // Benchmark standard compliance engine
    auto standard_result = BenchmarkComplianceEngine(standard_compliance_engine_.get(), 
                                                    "Standard Compliance Engine", 200);
    benchmark_results_.push_back(standard_result);
    
    // Benchmark optimized compliance engine
    auto optimized_result = BenchmarkComplianceEngine(optimized_compliance_engine_.get(),
                                                     "Optimized Compliance Engine", 200);
    benchmark_results_.push_back(optimized_result);
    
    // VALIDATE: Optimized engine should perform comparably or better
    EXPECT_GE(optimized_result.frames_per_second, standard_result.frames_per_second * 0.9)
        << "Optimized engine should maintain at least 90% of standard performance";
    
    // VALIDATE: Both should meet performance targets
    EXPECT_TRUE(standard_result.performance_acceptable) << "Standard engine should meet performance criteria";
    EXPECT_TRUE(optimized_result.performance_acceptable) << "Optimized engine should meet performance criteria";
    
    // VALIDATE: Optimized engine should achieve better compliance
    EXPECT_GE(optimized_result.compliance_percentage, standard_result.compliance_percentage)
        << "Optimized engine should achieve equal or better compliance";
    
    // VALIDATE: Memory usage should be reasonable
    EXPECT_LE(optimized_result.memory_usage_mb, standard_result.memory_usage_mb * 1.2)
        << "Optimized engine should not use more than 20% additional memory";
    
    std::cout << "\nOptimized vs Standard Performance Comparison:" << std::endl;
    std::cout << "Standard Engine:" << std::endl;
    std::cout << "  FPS: " << standard_result.frames_per_second << std::endl;
    std::cout << "  CPU: " << standard_result.cpu_usage_percent << "%" << std::endl;
    std::cout << "  Memory: " << standard_result.memory_usage_mb << " MB" << std::endl;
    std::cout << "  Compliance: " << standard_result.compliance_percentage << "%" << std::endl;
    
    std::cout << "Optimized Engine:" << std::endl;
    std::cout << "  FPS: " << optimized_result.frames_per_second << std::endl;
    std::cout << "  CPU: " << optimized_result.cpu_usage_percent << "%" << std::endl;
    std::cout << "  Memory: " << optimized_result.memory_usage_mb << " MB" << std::endl;
    std::cout << "  Compliance: " << optimized_result.compliance_percentage << "%" << std::endl;
    
    double performance_ratio = optimized_result.frames_per_second / standard_result.frames_per_second;
    double compliance_improvement = optimized_result.compliance_percentage - standard_result.compliance_percentage;
    
    std::cout << "Performance ratio: " << performance_ratio << "x" << std::endl;
    std::cout << "Compliance improvement: " << compliance_improvement << "%" << std::endl;
}

/**
 * @brief Test sustained load performance
 */
TEST_F(PerformanceImpactValidationTest, testSustainedLoadPerformance) {
    // Test sustained performance over 30 seconds
    auto sustained_result = StressTestSustainedLoad(optimized_compliance_engine_.get(),
                                                   "Optimized Engine Sustained Load",
                                                   std::chrono::seconds(30));
    benchmark_results_.push_back(sustained_result);
    
    // VALIDATE: Sustained performance should meet criteria
    EXPECT_TRUE(sustained_result.performance_acceptable) 
        << "Sustained load should meet performance criteria";
    
    // VALIDATE: Should maintain high frame rate
    EXPECT_GE(sustained_result.frames_per_second, 1000.0) 
        << "Should maintain ≥1000 FPS under sustained load";
    
    // VALIDATE: CPU usage should be reasonable
    EXPECT_LE(sustained_result.cpu_usage_percent, 85.0) 
        << "CPU usage should stay ≤85% under sustained load";
    
    // VALIDATE: Memory usage should be stable
    EXPECT_LE(sustained_result.memory_usage_mb, 150.0) 
        << "Memory usage should stay ≤150MB under sustained load";
    
    std::cout << "\nSustained Load Performance (30 seconds):" << std::endl;
    std::cout << "  Total frames processed: " << sustained_result.iterations << std::endl;
    std::cout << "  Average FPS: " << sustained_result.frames_per_second << std::endl;
    std::cout << "  Average CPU: " << sustained_result.cpu_usage_percent << "%" << std::endl;
    std::cout << "  Peak memory: " << sustained_result.memory_usage_mb << " MB" << std::endl;
}

/**
 * @brief Test concurrent processing performance
 */
TEST_F(PerformanceImpactValidationTest, testConcurrentProcessingPerformance) {
    const int num_threads = 4;
    const int frames_per_thread = 100;
    
    std::vector<std::future<BenchmarkResult>> future_results;
    std::atomic<int> completed_threads(0);
    
    auto concurrent_start = std::chrono::high_resolution_clock::now();
    
    // Launch concurrent processing threads
    for (int thread_id = 0; thread_id < num_threads; ++thread_id) {
        auto future = std::async(std::launch::async, [this, thread_id, frames_per_thread, &completed_threads]() {
            std::string thread_name = "Concurrent Thread " + std::to_string(thread_id);
            
            auto result = BenchmarkComplianceEngine(optimized_compliance_engine_.get(),
                                                   thread_name, frames_per_thread);
            
            completed_threads.fetch_add(1);
            return result;
        });
        
        future_results.push_back(std::move(future));
    }
    
    // Wait for all threads to complete
    std::vector<BenchmarkResult> concurrent_results;
    for (auto& future : future_results) {
        concurrent_results.push_back(future.get());
    }
    
    auto concurrent_end = std::chrono::high_resolution_clock::now();
    auto total_concurrent_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        concurrent_end - concurrent_start);
    
    // Calculate aggregate statistics
    double total_fps = 0.0;
    double max_cpu = 0.0;
    double max_memory = 0.0;
    double avg_compliance = 0.0;
    
    for (const auto& result : concurrent_results) {
        total_fps += result.frames_per_second;
        max_cpu = std::max(max_cpu, result.cpu_usage_percent);
        max_memory = std::max(max_memory, result.memory_usage_mb);
        avg_compliance += result.compliance_percentage;
        
        benchmark_results_.push_back(result);
    }
    
    avg_compliance /= concurrent_results.size();
    double aggregate_fps = (num_threads * frames_per_thread) / (total_concurrent_time.count() / 1000.0);
    
    // VALIDATE: Concurrent processing should scale reasonably
    EXPECT_GE(aggregate_fps, 2000.0) << "Aggregate concurrent FPS should be ≥2000";
    EXPECT_LE(max_cpu, 95.0) << "Peak CPU usage should stay ≤95% during concurrent processing";
    EXPECT_LE(max_memory, 200.0) << "Peak memory usage should stay ≤200MB during concurrent processing";
    EXPECT_GE(avg_compliance, 99.0) << "Average compliance should stay ≥99% during concurrent processing";
    
    std::cout << "\nConcurrent Processing Performance (" << num_threads << " threads):" << std::endl;
    std::cout << "  Total frames: " << (num_threads * frames_per_thread) << std::endl;
    std::cout << "  Total time: " << total_concurrent_time.count() << " ms" << std::endl;
    std::cout << "  Aggregate FPS: " << aggregate_fps << std::endl;
    std::cout << "  Peak CPU: " << max_cpu << "%" << std::endl;
    std::cout << "  Peak memory: " << max_memory << " MB" << std::endl;
    std::cout << "  Average compliance: " << avg_compliance << "%" << std::endl;
}

/**
 * @brief Test memory efficiency under various loads
 */
TEST_F(PerformanceImpactValidationTest, testMemoryEfficiencyUnderLoad) {
    std::vector<size_t> load_sizes = {100, 500, 1000, 2000};
    
    for (size_t load_size : load_sizes) {
        std::string test_name = "Memory Test " + std::to_string(load_size) + " frames";
        
        // Monitor memory before test
        auto memory_before = resource_monitor_->get_memory_usage();
        
        // Process specified number of frames
        for (size_t i = 0; i < load_size; ++i) {
            optimized_compliance_engine_->validate_eti_frame(test_frames_[i % test_frames_.size()]);
            
            // Force garbage collection every 100 frames
            if (i % 100 == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        
        // Monitor memory after test
        auto memory_after = resource_monitor_->get_memory_usage();
        auto memory_delta = memory_after - memory_before;
        
        // VALIDATE: Memory growth should be linear and bounded
        double memory_per_frame = memory_delta / load_size;
        EXPECT_LT(memory_per_frame, 0.1) << "Memory usage per frame should be <0.1 MB";
        
        // VALIDATE: Total memory should stay reasonable
        EXPECT_LT(memory_after, 500.0) << "Total memory usage should stay <500 MB";
        
        std::cout << "Memory efficiency for " << load_size << " frames:" << std::endl;
        std::cout << "  Memory before: " << memory_before << " MB" << std::endl;
        std::cout << "  Memory after: " << memory_after << " MB" << std::endl;
        std::cout << "  Memory delta: " << memory_delta << " MB" << std::endl;
        std::cout << "  Memory per frame: " << (memory_per_frame * 1024) << " KB" << std::endl;
    }
}

/**
 * @brief Test latency distribution and consistency
 */
TEST_F(PerformanceImpactValidationTest, testLatencyDistributionAndConsistency) {
    const size_t num_measurements = 1000;
    std::vector<std::chrono::microseconds> latencies;
    latencies.reserve(num_measurements);
    
    // Measure individual frame processing latencies
    for (size_t i = 0; i < num_measurements; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        optimized_compliance_engine_->validate_eti_frame(test_frames_[i % test_frames_.size()]);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        latencies.push_back(latency);
    }
    
    // Calculate statistical measures
    std::sort(latencies.begin(), latencies.end());
    
    auto min_latency = latencies.front();
    auto max_latency = latencies.back();
    auto median_latency = latencies[latencies.size() / 2];
    auto p95_latency = latencies[static_cast<size_t>(latencies.size() * 0.95)];
    auto p99_latency = latencies[static_cast<size_t>(latencies.size() * 0.99)];
    
    auto total_latency = std::accumulate(latencies.begin(), latencies.end(), 
                                       std::chrono::microseconds(0));
    auto avg_latency = total_latency / latencies.size();
    
    // Calculate standard deviation
    double variance = 0.0;
    for (const auto& latency : latencies) {
        double diff = latency.count() - avg_latency.count();
        variance += diff * diff;
    }
    variance /= latencies.size();
    double std_dev = std::sqrt(variance);
    
    // VALIDATE: Latency requirements
    EXPECT_LT(avg_latency.count(), 500) << "Average latency should be <500 μs";
    EXPECT_LT(p95_latency.count(), 1000) << "95th percentile latency should be <1 ms";
    EXPECT_LT(p99_latency.count(), 2000) << "99th percentile latency should be <2 ms";
    EXPECT_LT(max_latency.count(), 5000) << "Maximum latency should be <5 ms";
    
    // VALIDATE: Consistency requirements
    double coefficient_of_variation = std_dev / avg_latency.count();
    EXPECT_LT(coefficient_of_variation, 0.5) << "Latency should be reasonably consistent (CV < 0.5)";
    
    std::cout << "\nLatency Distribution Analysis (" << num_measurements << " samples):" << std::endl;
    std::cout << "  Min: " << min_latency.count() << " μs" << std::endl;
    std::cout << "  Average: " << avg_latency.count() << " μs" << std::endl;
    std::cout << "  Median: " << median_latency.count() << " μs" << std::endl;
    std::cout << "  95th percentile: " << p95_latency.count() << " μs" << std::endl;
    std::cout << "  99th percentile: " << p99_latency.count() << " μs" << std::endl;
    std::cout << "  Max: " << max_latency.count() << " μs" << std::endl;
    std::cout << "  Standard deviation: " << std_dev << " μs" << std::endl;
    std::cout << "  Coefficient of variation: " << coefficient_of_variation << std::endl;
}

/**
 * @brief Test performance regression detection
 */
TEST_F(PerformanceImpactValidationTest, testPerformanceRegressionDetection) {
    // Baseline performance measurement
    auto baseline_result = BenchmarkComplianceEngine(optimized_compliance_engine_.get(),
                                                    "Baseline Performance", 200);
    
    // Simulate potential performance regression scenarios
    std::vector<std::pair<std::string, std::function<void()>>> regression_scenarios = {
        {"Memory pressure", [this]() {
            // Allocate some memory to simulate pressure
            std::vector<std::vector<uint8_t>> memory_pressure;
            for (int i = 0; i < 100; ++i) {
                memory_pressure.emplace_back(1024 * 1024); // 1MB each
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }},
        {"CPU competition", [this]() {
            // Start some CPU-intensive background work
            std::atomic<bool> stop_work(false);
            auto cpu_work = std::async(std::launch::async, [&stop_work]() {
                volatile int work = 0;
                while (!stop_work.load()) {
                    for (int i = 0; i < 10000; ++i) {
                        work += i * i;
                    }
                }
            });
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            stop_work.store(true);
            cpu_work.wait();
        }},
        {"I/O competition", [this]() {
            // Simulate some disk I/O load
            std::ofstream temp_file("/tmp/test_io_load.tmp");
            for (int i = 0; i < 1000; ++i) {
                temp_file << "This is test data for I/O load simulation " << i << std::endl;
            }
            temp_file.close();
            std::remove("/tmp/test_io_load.tmp");
        }}
    };
    
    for (const auto& scenario : regression_scenarios) {
        std::cout << "\nTesting performance under: " << scenario.first << std::endl;
        
        // Apply regression scenario
        scenario.second();
        
        // Measure performance under stress
        auto stress_result = BenchmarkComplianceEngine(optimized_compliance_engine_.get(),
                                                      "Performance under " + scenario.first, 100);
        
        // Calculate performance degradation
        double fps_ratio = stress_result.frames_per_second / baseline_result.frames_per_second;
        double memory_ratio = stress_result.memory_usage_mb / baseline_result.memory_usage_mb;
        
        // VALIDATE: Performance should degrade gracefully
        EXPECT_GE(fps_ratio, 0.8) << "FPS should not degrade more than 20% under " << scenario.first;
        EXPECT_LE(memory_ratio, 1.5) << "Memory usage should not increase more than 50% under " << scenario.first;
        EXPECT_TRUE(stress_result.performance_acceptable) << "Should still meet performance criteria under " << scenario.first;
        
        std::cout << "  Baseline FPS: " << baseline_result.frames_per_second << std::endl;
        std::cout << "  Stress FPS: " << stress_result.frames_per_second << " (ratio: " << fps_ratio << ")" << std::endl;
        std::cout << "  Baseline Memory: " << baseline_result.memory_usage_mb << " MB" << std::endl;
        std::cout << "  Stress Memory: " << stress_result.memory_usage_mb << " MB (ratio: " << memory_ratio << ")" << std::endl;
        
        benchmark_results_.push_back(stress_result);
    }
}

/**
 * @brief Test scalability with increasing complexity
 */
TEST_F(PerformanceImpactValidationTest, testScalabilityWithIncreasingComplexity) {
    // Test with different frame complexities
    std::vector<std::pair<std::string, std::function<EtiFrame()>>> complexity_levels = {
        {"Minimal frame", []() { return fixtures::eti::CreateMinimalValidFrame(); }},
        {"Standard frame", []() { return fixtures::eti::CreatePerfectComplianceFrame(); }},
        {"Dense FIG frame", []() { return fixtures::eti::CreateDenseFIGFrame(); }},
        {"Max capacity frame", []() { return fixtures::eti::CreateMaxCapacityFrame(); }}
    };
    
    for (const auto& complexity : complexity_levels) {
        std::cout << "\nTesting scalability with: " << complexity.first << std::endl;
        
        // Create test frames of specified complexity
        std::vector<EtiFrame> complex_frames;
        for (int i = 0; i < 100; ++i) {
            auto frame = complexity.second();
            frame.frame_number = i;
            complex_frames.push_back(frame);
        }
        
        // Benchmark performance with this complexity
        std::vector<std::chrono::microseconds> processing_times;
        
        for (const auto& frame : complex_frames) {
            auto start = std::chrono::high_resolution_clock::now();
            auto result = optimized_compliance_engine_->validate_eti_frame(frame);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            processing_times.push_back(processing_time);
        }
        
        // Calculate performance metrics
        auto avg_time = std::accumulate(processing_times.begin(), processing_times.end(),
                                      std::chrono::microseconds(0)) / processing_times.size();
        auto max_time = *std::max_element(processing_times.begin(), processing_times.end());
        double fps = 1000000.0 / avg_time.count();
        
        // VALIDATE: Performance should scale reasonably with complexity
        EXPECT_GE(fps, 1000.0) << "Should maintain ≥1000 FPS even with " << complexity.first;
        EXPECT_LT(max_time.count(), 5000) << "Maximum processing time should be <5ms for " << complexity.first;
        
        std::cout << "  Average processing time: " << avg_time.count() << " μs" << std::endl;
        std::cout << "  Maximum processing time: " << max_time.count() << " μs" << std::endl;
        std::cout << "  Frames per second: " << fps << std::endl;
    }
}

/**
 * @brief Main test execution
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "\n⚡ PERFORMANCE IMPACT VALIDATION TESTS" << std::endl;
    std::cout << "=====================================" << std::endl;
    std::cout << "Testing performance impact of 100% ETSI compliance optimizations..." << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    int result = RUN_ALL_TESTS();
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "\n📊 PERFORMANCE IMPACT TEST SUMMARY" << std::endl;
    std::cout << "===================================" << std::endl;
    std::cout << "Total execution time: " << total_time.count() << " ms" << std::endl;
    
    if (result == 0) {
        std::cout << "\n✅ ALL PERFORMANCE TESTS PASSED - OPTIMIZATION IMPACT ACCEPTABLE" << std::endl;
        std::cout << "🚀 HIGH PERFORMANCE MAINTAINED WITH 100% COMPLIANCE" << std::endl;
    } else {
        std::cout << "\n❌ PERFORMANCE IMPACT TESTS FAILED - OPTIMIZATION REQUIRES TUNING" << std::endl;
    }
    
    return result;
}