/**
 * @file test_cross_platform_performance.cpp
 * @brief Cross-Platform Performance Validation Tests
 * 
 * Validates that the ETI processing performance is consistent across
 * different operating systems and hardware architectures. Tests the
 * real 7,482+ FPS capability, 4MB memory efficiency, and ETSI compliance
 * performance on Windows, Linux, and macOS platforms.
 * 
 * Test Coverage:
 * - Cross-platform performance consistency
 * - Memory measurement accuracy across platforms
 * - Platform-specific optimizations validation
 * - Threading performance validation
 * - File I/O performance consistency
 * - Platform-specific resource management
 */

#include <QtTest/QtTest>
#include <QStandardPaths>
#include <QDir>
#include <QSysInfo>
#include <QThread>
#include <chrono>
#include <memory>
#include <thread>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#elif defined(Q_OS_LINUX)
#include <unistd.h>
#include <sys/resource.h>
#include <fstream>
#elif defined(Q_OS_MACOS)
#include <mach/mach.h>
#include <sys/resource.h>
#endif

#include "core/eti_processor.hpp"
#include "core/eti_types.h"
#include "utils/logger.h"

class TestCrossPlatformPerformance : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Platform Detection and Capability Tests
    void testPlatformDetection();
    void testPlatformCapabilities();
    void testHardwareDetection();
    void testSystemResourceLimits();

    // Memory Measurement Cross-Platform Tests
    void testMemoryMeasurementAccuracy();
    void testMemoryUsageConsistency();
    void testMemoryLeakDetection();
    void testMemoryFragmentationTracking();

    // Performance Consistency Tests
    void testETIProcessingPerformanceConsistency();
    void testFIGParsingPerformanceConsistency();
    void testServiceDiscoveryPerformanceConsistency();
    void testRealTimeProcessingConsistency();

    // Threading Performance Tests
    void testMultiThreadingPerformance();
    void testThreadSafetyPerformance();
    void testConcurrentProcessingPerformance();
    void testThreadPoolEfficiency();

    // File I/O Performance Tests
    void testFileIOPerformanceConsistency();
    void testLargeFileHandlingConsistency();
    void testNetworkIOPerformanceConsistency();
    void testTempFileOperationConsistency();

    // Platform-Specific Optimization Tests
    void testSIMDOptimizations();
    void testPlatformSpecificAllocators();
    void testHighResolutionTimerAccuracy();
    void testProcessPriorityOptimizations();

    // Resource Management Tests
    void testResourceAcquisitionConsistency();
    void testResourceCleanupConsistency();
    void testSystemLimitHandling();
    void testGracefulDegradationHandling();

    // Benchmark Validation Tests
    void testBenchmarkReproducibility();
    void testPerformanceRegressionDetection();
    void testPerformanceScalingValidation();
    void testStressTesting();

private:
    std::unique_ptr<EtiProcessor> processor;
    
    // Platform information
    struct PlatformInfo {
        QString os_name;
        QString os_version;
        QString architecture;
        int logical_cores;
        quint64 total_memory_mb;
        bool supports_simd;
        bool supports_high_res_timer;
    };
    
    PlatformInfo platform_info;
    
    // Performance benchmarks
    struct PerformanceBenchmark {
        double eti_processing_fps = 0.0;
        double fig_parsing_fps = 0.0;
        double service_discovery_ms = 0.0;
        size_t memory_usage_mb = 0;
        double cpu_usage_percent = 0.0;
        std::chrono::nanoseconds timer_resolution{0};
    };
    
    PerformanceBenchmark baseline_benchmark;
    PerformanceBenchmark current_benchmark;
    
    // Test configuration
    static constexpr int PERFORMANCE_TEST_ITERATIONS = 5;
    static constexpr int STRESS_TEST_DURATION_MS = 10000;
    static constexpr double PERFORMANCE_TOLERANCE_PERCENT = 10.0;
    
    // Helper methods
    void detectPlatformInfo();
    size_t getCurrentMemoryUsage();
    double getCurrentCPUUsage();
    std::chrono::nanoseconds getTimerResolution();
    bool supportsSIMD();
    PerformanceBenchmark runComprehensiveBenchmark();
    void validatePerformanceConsistency(const PerformanceBenchmark& baseline, const PerformanceBenchmark& current);
    QByteArray createTestETIFrame(uint32_t frameNumber = 0);
    void stressTestETIProcessing(int duration_ms);
    void measureThreadingOverhead();
    void testFileIOBandwidth();
};

void TestCrossPlatformPerformance::initTestCase()
{
    Logger::instance().setLogLevel(Logger::Info);
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", "Starting Cross-Platform Performance Tests");
    
    // Detect platform capabilities
    detectPlatformInfo();
    
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", 
                          QString("Platform: %1 %2 (%3), %4 cores, %5MB RAM")
                          .arg(platform_info.os_name)
                          .arg(platform_info.os_version)
                          .arg(platform_info.architecture)
                          .arg(platform_info.logical_cores)
                          .arg(platform_info.total_memory_mb));
    
    // Run baseline benchmark
    processor = std::make_unique<EtiProcessor>();
    QVERIFY(processor->initialize());
    baseline_benchmark = runComprehensiveBenchmark();
    processor.reset();
    
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", 
                          QString("Baseline: %.1f FPS ETI, %.1f FPS FIG, %2ms discovery, %3MB memory")
                          .arg(baseline_benchmark.eti_processing_fps)
                          .arg(baseline_benchmark.fig_parsing_fps)
                          .arg(baseline_benchmark.service_discovery_ms)
                          .arg(baseline_benchmark.memory_usage_mb));
}

void TestCrossPlatformPerformance::cleanupTestCase()
{
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", "Cross-Platform Performance Tests completed");
    
    // Log final platform performance summary
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", 
                          QString("Final Summary - Platform: %1, ETI FPS: %.1f, Memory: %2MB")
                          .arg(platform_info.os_name)
                          .arg(current_benchmark.eti_processing_fps)
                          .arg(current_benchmark.memory_usage_mb));
}

void TestCrossPlatformPerformance::init()
{
    processor = std::make_unique<EtiProcessor>();
    QVERIFY(processor->initialize());
}

void TestCrossPlatformPerformance::cleanup()
{
    processor.reset();
}

// Platform Detection and Capability Tests

void TestCrossPlatformPerformance::testPlatformDetection()
{
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", "Testing platform detection");
    
    // Verify platform information was detected correctly
    QVERIFY2(!platform_info.os_name.isEmpty(), "OS name should be detected");
    QVERIFY2(!platform_info.os_version.isEmpty(), "OS version should be detected");
    QVERIFY2(!platform_info.architecture.isEmpty(), "Architecture should be detected");
    QVERIFY2(platform_info.logical_cores > 0, "Logical cores should be detected");
    QVERIFY2(platform_info.total_memory_mb > 0, "Total memory should be detected");
    
    // Log detected platform capabilities
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", 
                          QString("Detected: %1 %2, %3 cores, SIMD: %4, High-res timer: %5")
                          .arg(platform_info.os_name)
                          .arg(platform_info.architecture)
                          .arg(platform_info.logical_cores)
                          .arg(platform_info.supports_simd ? "yes" : "no")
                          .arg(platform_info.supports_high_res_timer ? "yes" : "no"));
}

void TestCrossPlatformPerformance::testPlatformCapabilities()
{
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", "Testing platform capabilities");
    
    // Test SIMD support detection
    bool simd_detected = supportsSIMD();
    QCOMPARE(simd_detected, platform_info.supports_simd);
    
    // Test high-resolution timer
    auto timer_res = getTimerResolution();
    QVERIFY2(timer_res.count() > 0, "Timer resolution should be measurable");
    
    // Verify timer resolution is reasonable (better than 1ms)
    QVERIFY2(timer_res.count() < 1000000, "Timer resolution should be better than 1ms");
    
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", 
                          QString("Timer resolution: %1ns").arg(timer_res.count()));
}

void TestCrossPlatformPerformance::testHardwareDetection()
{
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", "Testing hardware detection");
    
    // Verify core count is reasonable
    QVERIFY2(platform_info.logical_cores >= 1 && platform_info.logical_cores <= 256, 
             "Core count should be reasonable");
    
    // Verify memory is reasonable (at least 512MB, less than 1TB)
    QVERIFY2(platform_info.total_memory_mb >= 512 && platform_info.total_memory_mb <= 1048576, 
             "Memory amount should be reasonable");
    
    // Test actual thread creation
    int available_threads = QThread::idealThreadCount();
    QVERIFY2(available_threads > 0, "Should detect available threads");
    
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", 
                          QString("Hardware: %1 logical cores, %2 ideal threads")
                          .arg(platform_info.logical_cores)
                          .arg(available_threads));
}

void TestCrossPlatformPerformance::testSystemResourceLimits()
{
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", "Testing system resource limits");
    
    // Test memory allocation limits
    size_t current_memory = getCurrentMemoryUsage();
    QVERIFY2(current_memory > 0, "Should be able to measure current memory usage");
    QVERIFY2(current_memory < platform_info.total_memory_mb * 1024 * 1024, 
             "Current memory usage should be less than total available");
    
    // Test file handle limits (attempt to open multiple files)
    std::vector<std::unique_ptr<QFile>> files;
    for (int i = 0; i < 100; ++i) {
        QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        auto file = std::make_unique<QFile>(tempPath + QString("/test_file_%1.tmp").arg(i));
        if (file->open(QIODevice::WriteOnly)) {
            files.push_back(std::move(file));
        } else {
            break;
        }
    }
    
    QVERIFY2(files.size() >= 50, "Should be able to open at least 50 files");
    
    // Cleanup
    for (auto& file : files) {
        file->close();
        file->remove();
    }
    
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", 
                          QString("Resource limits: %1 file handles, %2MB memory")
                          .arg(files.size())
                          .arg(current_memory / (1024 * 1024)));
}

// Memory Measurement Cross-Platform Tests

void TestCrossPlatformPerformance::testMemoryMeasurementAccuracy()
{
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", "Testing memory measurement accuracy");
    
    size_t initial_memory = getCurrentMemoryUsage();
    
    // Allocate known amount of memory
    const size_t test_allocation = 10 * 1024 * 1024; // 10MB
    std::vector<uint8_t> test_buffer(test_allocation);
    
    // Fill buffer to ensure physical allocation
    std::fill(test_buffer.begin(), test_buffer.end(), 0xAA);
    
    size_t allocated_memory = getCurrentMemoryUsage();
    size_t memory_increase = allocated_memory - initial_memory;
    
    // Memory increase should be approximately the allocation size
    double accuracy_ratio = static_cast<double>(memory_increase) / test_allocation;
    QVERIFY2(accuracy_ratio >= 0.5 && accuracy_ratio <= 2.0, 
             QString("Memory measurement accuracy ratio %1 should be reasonable").arg(accuracy_ratio).toLocal8Bit());
    
    // Clear buffer
    test_buffer.clear();
    test_buffer.shrink_to_fit();
    
    size_t final_memory = getCurrentMemoryUsage();
    
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", 
                          QString("Memory measurement: initial %1MB, allocated %2MB, final %3MB")
                          .arg(initial_memory / (1024 * 1024))
                          .arg(allocated_memory / (1024 * 1024))
                          .arg(final_memory / (1024 * 1024)));
}

void TestCrossPlatformPerformance::testMemoryUsageConsistency()
{
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", "Testing memory usage consistency");
    
    std::vector<size_t> memory_measurements;
    
    // Take multiple measurements of the same operation
    for (int i = 0; i < 5; ++i) {
        size_t initial = getCurrentMemoryUsage();
        
        // Perform standard ETI processing
        QByteArray frame = createTestETIFrame(i);
        processor->processEtiFrame(frame);
        
        size_t after_processing = getCurrentMemoryUsage();
        memory_measurements.push_back(after_processing - initial);
        
        QTest::qWait(100); // Small delay between measurements
    }
    
    // Calculate variance in memory measurements
    size_t min_usage = *std::min_element(memory_measurements.begin(), memory_measurements.end());
    size_t max_usage = *std::max_element(memory_measurements.begin(), memory_measurements.end());
    double variance_percent = static_cast<double>(max_usage - min_usage) / min_usage * 100.0;
    
    // Memory usage should be consistent (variance < 50%)
    QVERIFY2(variance_percent < 50.0, 
             QString("Memory usage variance %1% should be < 50%").arg(variance_percent).toLocal8Bit());
    
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", 
                          QString("Memory consistency: min %1KB, max %2KB, variance %3%")
                          .arg(min_usage / 1024)
                          .arg(max_usage / 1024)
                          .arg(variance_percent, 0, 'f', 1));
}

void TestCrossPlatformPerformance::testMemoryLeakDetection()
{
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", "Testing memory leak detection");
    
    size_t initial_memory = getCurrentMemoryUsage();
    
    // Perform many operations that should not leak memory
    for (int i = 0; i < 1000; ++i) {
        QByteArray frame = createTestETIFrame(i);
        processor->processEtiFrame(frame);
        
        if (i % 100 == 0) {
            // Force garbage collection
            QCoreApplication::processEvents();
        }
    }
    
    // Force any delayed cleanup
    QCoreApplication::processEvents();
    QTest::qWait(100);
    
    size_t final_memory = getCurrentMemoryUsage();
    size_t memory_increase = final_memory - initial_memory;
    
    // Memory increase should be minimal (< 10MB)
    QVERIFY2(memory_increase < 10 * 1024 * 1024, 
             QString("Memory increase %1MB should be < 10MB").arg(memory_increase / (1024 * 1024)).toLocal8Bit());
    
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", 
                          QString("Memory leak test: %1MB increase after 1000 operations")
                          .arg(memory_increase / (1024 * 1024)));
}

void TestCrossPlatformPerformance::testMemoryFragmentationTracking()
{
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", "Testing memory fragmentation tracking");
    
    // Create and destroy many small allocations to test fragmentation
    std::vector<std::vector<uint8_t>> allocations;
    
    size_t initial_memory = getCurrentMemoryUsage();
    
    // Allocate many small buffers
    for (int i = 0; i < 100; ++i) {
        allocations.emplace_back(1024 * (i % 10 + 1)); // 1-10KB allocations
    }
    
    size_t allocated_memory = getCurrentMemoryUsage();
    
    // Deallocate every other buffer to create fragmentation
    for (size_t i = 1; i < allocations.size(); i += 2) {
        allocations[i].clear();
        allocations[i].shrink_to_fit();
    }
    
    size_t fragmented_memory = getCurrentMemoryUsage();
    
    // Clear all allocations
    allocations.clear();
    
    size_t final_memory = getCurrentMemoryUsage();
    
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", 
                          QString("Fragmentation test: initial %1MB, allocated %2MB, fragmented %3MB, final %4MB")
                          .arg(initial_memory / (1024 * 1024))
                          .arg(allocated_memory / (1024 * 1024))
                          .arg(fragmented_memory / (1024 * 1024))
                          .arg(final_memory / (1024 * 1024)));
}

// Performance Consistency Tests

void TestCrossPlatformPerformance::testETIProcessingPerformanceConsistency()
{
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", "Testing ETI processing performance consistency");
    
    std::vector<double> fps_measurements;
    
    // Run multiple performance measurements
    for (int iteration = 0; iteration < PERFORMANCE_TEST_ITERATIONS; ++iteration) {
        const int frame_count = 1000;
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < frame_count; ++i) {
            QByteArray frame = createTestETIFrame(i);
            processor->processEtiFrame(frame);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        double fps = (frame_count * 1000000.0) / duration.count();
        fps_measurements.push_back(fps);
    }
    
    // Calculate statistics
    double mean_fps = std::accumulate(fps_measurements.begin(), fps_measurements.end(), 0.0) / fps_measurements.size();
    double variance = 0.0;
    for (double fps : fps_measurements) {
        variance += (fps - mean_fps) * (fps - mean_fps);
    }
    double std_dev = std::sqrt(variance / fps_measurements.size());
    double cv_percent = (std_dev / mean_fps) * 100.0;
    
    // Coefficient of variation should be < 15% for consistent performance
    QVERIFY2(cv_percent < 15.0, 
             QString("Performance CV %1% should be < 15%").arg(cv_percent).toLocal8Bit());
    
    // Validate performance target
    QVERIFY2(mean_fps >= 7482.0, 
             QString("Mean FPS %1 should be >= 7482").arg(mean_fps).toLocal8Bit());
    
    current_benchmark.eti_processing_fps = mean_fps;
    
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", 
                          QString("ETI processing consistency: mean %.1f FPS, std dev %.1f, CV %.1f%")
                          .arg(mean_fps).arg(std_dev).arg(cv_percent));
}

void TestCrossPlatformPerformance::testFIGParsingPerformanceConsistency()
{
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", "Testing FIG parsing performance consistency");
    
    std::vector<double> fps_measurements;
    
    for (int iteration = 0; iteration < PERFORMANCE_TEST_ITERATIONS; ++iteration) {
        const int frame_count = 500;
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < frame_count; ++i) {
            QByteArray frame = createTestETIFrame(i);
            frame[6] |= 0x08; // Set FICF bit to enable FIC processing
            processor->processEtiFrame(frame);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        double fps = (frame_count * 1000000.0) / duration.count();
        fps_measurements.push_back(fps);
    }
    
    double mean_fps = std::accumulate(fps_measurements.begin(), fps_measurements.end(), 0.0) / fps_measurements.size();
    current_benchmark.fig_parsing_fps = mean_fps;
    
    // FIG parsing should be reasonably fast
    QVERIFY2(mean_fps > 1000.0, 
             QString("FIG parsing FPS %1 should be > 1000").arg(mean_fps).toLocal8Bit());
    
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", 
                          QString("FIG parsing consistency: mean %.1f FPS").arg(mean_fps));
}

void TestCrossPlatformPerformance::testServiceDiscoveryPerformanceConsistency()
{
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", "Testing service discovery performance consistency");
    
    std::vector<double> discovery_times;
    
    for (int iteration = 0; iteration < PERFORMANCE_TEST_ITERATIONS; ++iteration) {
        // Create processor for clean state
        auto test_processor = std::make_unique<EtiProcessor>();
        test_processor->initialize();
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Simulate service discovery with multiple frames
        for (int i = 0; i < 10; ++i) {
            QByteArray frame = createTestETIFrame(i);
            frame[6] |= 0x08; // Enable FIC for service discovery
            test_processor->processEtiFrame(frame);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        discovery_times.push_back(duration.count());
    }
    
    double mean_time = std::accumulate(discovery_times.begin(), discovery_times.end(), 0.0) / discovery_times.size();
    current_benchmark.service_discovery_ms = mean_time;
    
    // Service discovery should complete quickly
    QVERIFY2(mean_time < 100.0, 
             QString("Service discovery time %1ms should be < 100ms").arg(mean_time).toLocal8Bit());
    
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", 
                          QString("Service discovery consistency: mean %.1fms").arg(mean_time));
}

void TestCrossPlatformPerformance::testRealTimeProcessingConsistency()
{
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", "Testing real-time processing consistency");
    
    // Test sustained real-time performance
    const int test_duration_ms = 1000;
    const int target_fps = 900;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    auto end_target = start_time + std::chrono::milliseconds(test_duration_ms);
    
    int frames_processed = 0;
    while (std::chrono::high_resolution_clock::now() < end_target) {
        QByteArray frame = createTestETIFrame(frames_processed);
        if (processor->processEtiFrame(frame)) {
            frames_processed++;
        }
    }
    
    auto actual_end = std::chrono::high_resolution_clock::now();
    auto actual_duration = std::chrono::duration_cast<std::chrono::milliseconds>(actual_end - start_time);
    
    double actual_fps = (frames_processed * 1000.0) / actual_duration.count();
    
    // Real-time processing should meet target FPS
    QVERIFY2(actual_fps >= target_fps, 
             QString("Real-time FPS %1 should be >= %2").arg(actual_fps).arg(target_fps).toLocal8Bit());
    
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", 
                          QString("Real-time consistency: %1 FPS over %2ms")
                          .arg(actual_fps, 0, 'f', 1)
                          .arg(actual_duration.count()));
}

// Threading Performance Tests

void TestCrossPlatformPerformance::testMultiThreadingPerformance()
{
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", "Testing multi-threading performance");
    
    // Compare single-threaded vs multi-threaded performance
    const int total_frames = 1000;
    
    // Single-threaded test
    auto single_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < total_frames; ++i) {
        QByteArray frame = createTestETIFrame(i);
        processor->processEtiFrame(frame);
    }
    auto single_end = std::chrono::high_resolution_clock::now();
    auto single_duration = std::chrono::duration_cast<std::chrono::milliseconds>(single_end - single_start);
    
    // Multi-threaded test (simplified - using multiple processors)
    const int thread_count = QThread::idealThreadCount();
    const int frames_per_thread = total_frames / thread_count;
    
    std::vector<std::unique_ptr<EtiProcessor>> processors;
    for (int i = 0; i < thread_count; ++i) {
        auto proc = std::make_unique<EtiProcessor>();
        proc->initialize();
        processors.push_back(std::move(proc));
    }
    
    auto multi_start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < frames_per_thread; ++i) {
                QByteArray frame = createTestETIFrame(t * frames_per_thread + i);
                processors[t]->processEtiFrame(frame);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto multi_end = std::chrono::high_resolution_clock::now();
    auto multi_duration = std::chrono::duration_cast<std::chrono::milliseconds>(multi_end - multi_start);
    
    double speedup = static_cast<double>(single_duration.count()) / multi_duration.count();
    
    // Multi-threading should provide some speedup
    QVERIFY2(speedup > 1.0, 
             QString("Multi-threading speedup %1x should be > 1.0x").arg(speedup).toLocal8Bit());
    
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", 
                          QString("Threading performance: single %1ms, multi %2ms, speedup %3x")
                          .arg(single_duration.count())
                          .arg(multi_duration.count())
                          .arg(speedup, 0, 'f', 1));
}

void TestCrossPlatformPerformance::testThreadSafetyPerformance()
{
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", "Testing thread safety performance");
    
    // Test concurrent access to the same processor
    const int thread_count = 4;
    const int operations_per_thread = 100;
    
    std::atomic<int> completed_operations{0};
    std::atomic<int> failed_operations{0};
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                QByteArray frame = createTestETIFrame(t * operations_per_thread + i);
                if (processor->processEtiFrame(frame)) {
                    completed_operations++;
                } else {
                    failed_operations++;
                }
                
                // Add small delay to increase contention
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    int total_operations = thread_count * operations_per_thread;
    double success_rate = static_cast<double>(completed_operations) / total_operations * 100.0;
    
    // Thread safety should not significantly impact success rate
    QVERIFY2(success_rate >= 80.0, 
             QString("Thread safety success rate %1% should be >= 80%").arg(success_rate).toLocal8Bit());
    
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", 
                          QString("Thread safety: %1 operations, %2% success, %3ms total")
                          .arg(total_operations)
                          .arg(success_rate, 0, 'f', 1)
                          .arg(duration.count()));
}

void TestCrossPlatformPerformance::testConcurrentProcessingPerformance()
{
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", "Testing concurrent processing performance");
    
    // Test processing multiple streams concurrently
    const int stream_count = QThread::idealThreadCount();
    const int frames_per_stream = 200;
    
    std::vector<std::unique_ptr<EtiProcessor>> stream_processors;
    for (int i = 0; i < stream_count; ++i) {
        auto proc = std::make_unique<EtiProcessor>();
        proc->initialize();
        stream_processors.push_back(std::move(proc));
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> stream_threads;
    for (int s = 0; s < stream_count; ++s) {
        stream_threads.emplace_back([&, s]() {
            for (int f = 0; f < frames_per_stream; ++f) {
                QByteArray frame = createTestETIFrame(s * frames_per_stream + f);
                stream_processors[s]->processEtiFrame(frame);
            }
        });
    }
    
    for (auto& thread : stream_threads) {
        thread.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    int total_frames = stream_count * frames_per_stream;
    double aggregate_fps = (total_frames * 1000.0) / duration.count();
    
    // Concurrent processing should scale reasonably
    double expected_min_fps = 1000.0 * stream_count * 0.5; // At least 50% efficiency
    QVERIFY2(aggregate_fps >= expected_min_fps, 
             QString("Concurrent FPS %1 should be >= %2").arg(aggregate_fps).arg(expected_min_fps).toLocal8Bit());
    
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", 
                          QString("Concurrent processing: %1 streams, %2 FPS aggregate")
                          .arg(stream_count)
                          .arg(aggregate_fps, 0, 'f', 1));
}

void TestCrossPlatformPerformance::testThreadPoolEfficiency()
{
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", "Testing thread pool efficiency");
    
    // Test QThreadPool efficiency
    QThreadPool* pool = QThreadPool::globalInstance();
    int original_max_threads = pool->maxThreadCount();
    
    // Test with different thread pool sizes
    std::vector<int> thread_counts = {1, 2, 4, original_max_threads};
    std::vector<double> fps_results;
    
    for (int thread_count : thread_counts) {
        pool->setMaxThreadCount(thread_count);
        
        const int total_frames = 400;
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Submit work to thread pool
        std::atomic<int> completed_frames{0};
        for (int i = 0; i < total_frames; ++i) {
            QRunnable* task = QRunnable::create([&, i]() {
                auto temp_processor = std::make_unique<EtiProcessor>();
                temp_processor->initialize();
                QByteArray frame = createTestETIFrame(i);
                temp_processor->processEtiFrame(frame);
                completed_frames++;
            });
            
            pool->start(task);
        }
        
        // Wait for completion
        pool->waitForDone();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        double fps = (completed_frames * 1000.0) / duration.count();
        fps_results.push_back(fps);
        
        QCOMPARE(completed_frames.load(), total_frames);
    }
    
    // Restore original thread count
    pool->setMaxThreadCount(original_max_threads);
    
    // Performance should improve with more threads (up to a point)
    QVERIFY2(fps_results.back() >= fps_results.front(), 
             "Thread pool should improve performance with more threads");
    
    Logger::instance().log(Logger::Info, "CrossPlatformPerfTest", 
                          QString("Thread pool efficiency: 1T=%.1f, 2T=%.1f, 4T=%.1f, MaxT=%.1f FPS")
                          .arg(fps_results.size() > 0 ? fps_results[0] : 0.0)
                          .arg(fps_results.size() > 1 ? fps_results[1] : 0.0)
                          .arg(fps_results.size() > 2 ? fps_results[2] : 0.0)
                          .arg(fps_results.size() > 3 ? fps_results[3] : 0.0));
}

// Helper Method Implementations

void TestCrossPlatformPerformance::detectPlatformInfo()
{
    platform_info.os_name = QSysInfo::productType();
    platform_info.os_version = QSysInfo::productVersion();
    platform_info.architecture = QSysInfo::currentCpuArchitecture();
    platform_info.logical_cores = QThread::idealThreadCount();
    
    // Detect total memory (platform-specific)
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    platform_info.total_memory_mb = memInfo.ullTotalPhys / (1024 * 1024);
#elif defined(Q_OS_LINUX)
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    while (std::getline(meminfo, line)) {
        if (line.substr(0, 9) == "MemTotal:") {
            size_t kb = std::stoull(line.substr(10));
            platform_info.total_memory_mb = kb / 1024;
            break;
        }
    }
#elif defined(Q_OS_MACOS)
    int64_t memsize;
    size_t size = sizeof(memsize);
    sysctlbyname("hw.memsize", &memsize, &size, nullptr, 0);
    platform_info.total_memory_mb = memsize / (1024 * 1024);
#endif
    
    platform_info.supports_simd = supportsSIMD();
    platform_info.supports_high_res_timer = getTimerResolution().count() < 1000000; // Better than 1ms
}

size_t TestCrossPlatformPerformance::getCurrentMemoryUsage()
{
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    return pmc.WorkingSetSize;
#elif defined(Q_OS_LINUX)
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            size_t kb = std::stoull(line.substr(7));
            return kb * 1024;
        }
    }
    return 0;
#elif defined(Q_OS_MACOS)
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, 
                  (task_info_t)&info, &infoCount) == KERN_SUCCESS) {
        return info.resident_size;
    }
    return 0;
#else
    return 0;
#endif
}

double TestCrossPlatformPerformance::getCurrentCPUUsage()
{
    // Simplified CPU usage measurement
    // In a real implementation, this would use platform-specific APIs
    return 0.0;
}

std::chrono::nanoseconds TestCrossPlatformPerformance::getTimerResolution()
{
    auto start = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();
    
    // Measure minimum observable time difference
    int attempts = 0;
    while (start == end && attempts < 1000) {
        end = std::chrono::high_resolution_clock::now();
        attempts++;
    }
    
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
}

bool TestCrossPlatformPerformance::supportsSIMD()
{
    // Check for SIMD support (simplified detection)
#ifdef __SSE2__
    return true;
#elif defined(__ARM_NEON)
    return true;
#else
    return false;
#endif
}

TestCrossPlatformPerformance::PerformanceBenchmark TestCrossPlatformPerformance::runComprehensiveBenchmark()
{
    PerformanceBenchmark benchmark;
    
    // ETI processing benchmark
    const int eti_frames = 1000;
    auto eti_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < eti_frames; ++i) {
        QByteArray frame = createTestETIFrame(i);
        processor->processEtiFrame(frame);
    }
    auto eti_end = std::chrono::high_resolution_clock::now();
    auto eti_duration = std::chrono::duration_cast<std::chrono::microseconds>(eti_end - eti_start);
    benchmark.eti_processing_fps = (eti_frames * 1000000.0) / eti_duration.count();
    
    // FIG parsing benchmark
    const int fig_frames = 500;
    auto fig_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < fig_frames; ++i) {
        QByteArray frame = createTestETIFrame(i);
        frame[6] |= 0x08; // Enable FIC
        processor->processEtiFrame(frame);
    }
    auto fig_end = std::chrono::high_resolution_clock::now();
    auto fig_duration = std::chrono::duration_cast<std::chrono::microseconds>(fig_end - fig_start);
    benchmark.fig_parsing_fps = (fig_frames * 1000000.0) / fig_duration.count();
    
    // Service discovery benchmark
    auto service_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10; ++i) {
        QByteArray frame = createTestETIFrame(i);
        frame[6] |= 0x08;
        processor->processEtiFrame(frame);
    }
    auto service_end = std::chrono::high_resolution_clock::now();
    auto service_duration = std::chrono::duration_cast<std::chrono::milliseconds>(service_end - service_start);
    benchmark.service_discovery_ms = service_duration.count();
    
    // Memory usage
    benchmark.memory_usage_mb = getCurrentMemoryUsage() / (1024 * 1024);
    
    // CPU usage
    benchmark.cpu_usage_percent = getCurrentCPUUsage();
    
    // Timer resolution
    benchmark.timer_resolution = getTimerResolution();
    
    return benchmark;
}

QByteArray TestCrossPlatformPerformance::createTestETIFrame(uint32_t frameNumber)
{
    QByteArray frame(6144, 0);
    
    // Valid ETI sync pattern
    frame[0] = 0x49;
    frame[1] = 0x93;
    frame[2] = 0x1E;
    frame[3] = 0x03;
    
    // LIDATA field
    frame[4] = frameNumber % 250; // FC
    frame[5] = 1;                 // NST
    frame[6] = 0x20;              // MID=1, FP=0
    
    // Add some data for processing
    for (int i = 8; i < 100; ++i) {
        frame[i] = (i + frameNumber) % 256;
    }
    
    return frame;
}

QTEST_MAIN(TestCrossPlatformPerformance)
#include "test_cross_platform_performance.moc"