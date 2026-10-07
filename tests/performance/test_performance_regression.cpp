/**
 * @file test_performance_regression.cpp
 * @brief Performance Regression Tests for Phase 2 Migration
 * 
 * Comprehensive performance validation for the new integrated C++20 ETI engine
 * to ensure >30% speed improvement and >40% memory reduction targets are met.
 * 
 * Test Categories:
 * - Baseline performance measurement (legacy wrapper)
 * - Integrated engine performance validation
 * - Memory usage regression testing
 * - Real-time processing benchmarks
 * - Component-level performance validation
 * 
 * Success Criteria:
 * - >30% speed improvement over legacy wrapper
 * - >40% memory reduction compared to wrapper
 * - >900 FPS real-time processing capability
 * - <50ms latency for real-time mode
 * - Consistent performance across test runs
 */

#include <QtTest/QtTest>
#include <QApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QDir>
#include <chrono>
#include <memory>
#include <vector>
#include <algorithm>
#include <numeric>
#include <fstream>

#include "core/eti_processing_engine.hpp"
#include "core/modern_eti_frame_parser.hpp"
#include "core/enhanced_fig_analyser.hpp"
// REMOVED: No external dependencies - using internal implementations only
#include "utils/logger.h"

class TestPerformanceRegression : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Baseline Performance Measurement
    void testInternalEngineBaseline();
    void testInternalEngineMemoryBaseline();
    void testInternalEngineRealTimeBaseline();

    // Real Implementation Performance Testing
    void testRealETIProcessingPerformance();
    void testRealFIGParsingPerformance();
    void testRealServiceDiscoveryPerformance();
    void testRealETSIComplianceValidation();

    // Integrated Engine Performance Validation
    void testIntegratedEnginePerformance();
    void testIntegratedEngineMemoryUsage();
    void testIntegratedEngineRealTimeProcessing();

    // Regression Testing
    void testSpeedImprovementRegression();
    void testMemoryReductionRegression();
    void testRealTimeFPSRegression();
    void testLatencyRegression();

    // Component-Level Performance
    void testModernFrameParserPerformance();
    void testEnhancedFIGAnalyserPerformance();
    void testETIProcessingCachePerformance();
    void testPerformanceProfilerOverhead();

    // Statistical Validation
    void testPerformanceConsistency();
    void testPerformanceDistribution();
    void testPerformanceOutliers();

private:
    QApplication* app;
    QString testETIFilePath;
    
    // Test configurations
    static constexpr int PERFORMANCE_TEST_RUNS = 10;
    static constexpr int REALTIME_FRAME_COUNT = 1000;
    static constexpr double SPEED_IMPROVEMENT_TARGET = 30.0; // 30%
    static constexpr double MEMORY_REDUCTION_TARGET = 40.0;  // 40%
    static constexpr double FPS_TARGET = 900.0;              // 900 FPS
    static constexpr double LATENCY_TARGET = 50.0;           // 50ms
    
    // Performance tracking
    struct PerformanceMetrics {
        std::vector<double> processing_times_ms;
        std::vector<size_t> memory_usage_bytes;
        std::vector<double> fps_measurements;
        std::vector<double> latency_measurements_ms;
        
        double mean_processing_time = 0.0;
        double std_dev_processing_time = 0.0;
        double mean_memory_usage = 0.0;
        double mean_fps = 0.0;
        double mean_latency = 0.0;
        
        void calculateStatistics();
        QString generateReport() const;
    };
    
    PerformanceMetrics baselineMetrics;
    PerformanceMetrics optimizedMetrics;
    
    // Test engines - INTERNAL IMPLEMENTATIONS ONLY
    std::unique_ptr<eti::processing::UnifiedETIProcessingEngine> testEngine;
    std::unique_ptr<eti::processing::UnifiedETIProcessingEngine> baselineEngine;
    
    // Helper methods
    bool initializeTestEngine(eti::processing::EngineConfiguration::ProcessingMode mode);
    PerformanceMetrics measureEnginePerformance(eti::processing::EngineConfiguration::ProcessingMode mode);
    size_t getCurrentMemoryUsage();
    double measureProcessingTime(eti::processing::EngineConfiguration::ProcessingMode mode);
    double measureRealTimeFPS(eti::processing::EngineConfiguration::ProcessingMode mode);
    double measureProcessingLatency(eti::processing::EngineConfiguration::ProcessingMode mode);
    QByteArray createTestETIFrame(uint32_t frameNumber = 0);
    bool validatePerformanceImprovement(const PerformanceMetrics& baseline, const PerformanceMetrics& optimized);
    void savePerformanceReport(const QString& filename);
};

void TestPerformanceRegression::PerformanceMetrics::calculateStatistics()
{
    if (!processing_times_ms.empty()) {
        mean_processing_time = std::accumulate(processing_times_ms.begin(), processing_times_ms.end(), 0.0) / processing_times_ms.size();
        
        double variance = 0.0;
        for (double time : processing_times_ms) {
            variance += (time - mean_processing_time) * (time - mean_processing_time);
        }
        std_dev_processing_time = std::sqrt(variance / processing_times_ms.size());
    }
    
    if (!memory_usage_bytes.empty()) {
        mean_memory_usage = std::accumulate(memory_usage_bytes.begin(), memory_usage_bytes.end(), 0.0) / memory_usage_bytes.size();
    }
    
    if (!fps_measurements.empty()) {
        mean_fps = std::accumulate(fps_measurements.begin(), fps_measurements.end(), 0.0) / fps_measurements.size();
    }
    
    if (!latency_measurements_ms.empty()) {
        mean_latency = std::accumulate(latency_measurements_ms.begin(), latency_measurements_ms.end(), 0.0) / latency_measurements_ms.size();
    }
}

QString TestPerformanceRegression::PerformanceMetrics::generateReport() const
{
    QString report;
    report += QString("Performance Metrics Report\n");
    report += QString("=========================\n");
    report += QString("Mean Processing Time: %1ms (±%2ms)\n").arg(mean_processing_time, 0, 'f', 2).arg(std_dev_processing_time, 0, 'f', 2);
    report += QString("Mean Memory Usage: %1MB\n").arg(mean_memory_usage / (1024.0 * 1024.0), 0, 'f', 2);
    report += QString("Mean FPS: %1\n").arg(mean_fps, 0, 'f', 1);
    report += QString("Mean Latency: %1ms\n").arg(mean_latency, 0, 'f', 2);
    report += QString("Test Runs: %1\n").arg(processing_times_ms.size());
    return report;
}

void TestPerformanceRegression::initTestCase()
{
    // Initialize application
    int argc = 1;
    const char* argv[] = {"test_performance_regression"};
    app = new QApplication(argc, const_cast<char**>(argv));
    
    // Set up logging
    Logger::instance().setLogLevel(Logger::Info);
    Logger::instance().log(Logger::Info, "PerfTest", "Starting Performance Regression Tests");
    
    // Verify test ETI file exists
    testETIFilePath = QString(ETI_TEST_FILES_DIR) + "/bkk_20062022_141637.eti";
    QFileInfo fileInfo(testETIFilePath);
    QVERIFY2(fileInfo.exists(), QString("Test ETI file not found: %1").arg(testETIFilePath).toLocal8Bit());
    QVERIFY2(fileInfo.size() > 0, "Test ETI file is empty");
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("Using test ETI file: %1 (%2 bytes)")
                          .arg(testETIFilePath).arg(fileInfo.size()));
}

void TestPerformanceRegression::cleanupTestCase()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Performance Regression Tests completed");
    
    // Save final performance report
    savePerformanceReport("performance_regression_results.txt");
    
    if (app && app != QApplication::instance()) {
        delete app;
        app = nullptr;
    }
}

void TestPerformanceRegression::init()
{
    // Clean state for each test
    testEngine.reset();
    baselineEngine.reset();
}

void TestPerformanceRegression::cleanup()
{
    // Cleanup after each test
    testEngine.reset();
    baselineEngine.reset();
}

void TestPerformanceRegression::testInternalEngineBaseline()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Measuring Internal Engine Baseline Performance");
    
    baselineMetrics = measureEnginePerformance(eti::processing::EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE);
    baselineMetrics.calculateStatistics();
    
    QVERIFY(baselineMetrics.mean_processing_time > 0.0);
    QVERIFY(baselineMetrics.mean_memory_usage > 0.0);
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("Internal baseline: %1ms processing, %2MB memory")
                          .arg(baselineMetrics.mean_processing_time, 0, 'f', 2)
                          .arg(baselineMetrics.mean_memory_usage / (1024.0 * 1024.0), 0, 'f', 2));
}

void TestPerformanceRegression::testInternalEngineMemoryBaseline()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Measuring Legacy Wrapper Memory Baseline");
    
    QVERIFY(initializeTestEngine(eti::processing::EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE));
    
    size_t initialMemory = getCurrentMemoryUsage();
    
    // Process test file
    QVERIFY(testEngine->processFile(testETIFilePath));
    
    size_t finalMemory = getCurrentMemoryUsage();
    baselineMetrics.memory_usage_bytes.push_back(finalMemory - initialMemory);
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("Legacy memory baseline: %1MB")
                          .arg((finalMemory - initialMemory) / (1024.0 * 1024.0), 0, 'f', 2));
}

void TestPerformanceRegression::testInternalEngineRealTimeBaseline()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Measuring Legacy Wrapper Real-time Baseline");
    
    double fps = measureRealTimeFPS(eti::processing::EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE);
    baselineMetrics.fps_measurements.push_back(fps);
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("Legacy real-time baseline: %1 FPS").arg(fps, 0, 'f', 1));
}

void TestPerformanceRegression::testIntegratedEnginePerformance()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Measuring Integrated Engine Performance");
    
    optimizedMetrics = measureEnginePerformance(eti::processing::EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE);
    optimizedMetrics.calculateStatistics();
    
    QVERIFY(optimizedMetrics.mean_processing_time > 0.0);
    QVERIFY(optimizedMetrics.mean_memory_usage > 0.0);
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("Integrated performance: %1ms processing, %2MB memory")
                          .arg(optimizedMetrics.mean_processing_time, 0, 'f', 2)
                          .arg(optimizedMetrics.mean_memory_usage / (1024.0 * 1024.0), 0, 'f', 2));
}

void TestPerformanceRegression::testIntegratedEngineMemoryUsage()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Measuring Integrated Engine Memory Usage");
    
    QVERIFY(initializeTestEngine(eti::processing::EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE));
    
    size_t initialMemory = getCurrentMemoryUsage();
    
    // Process test file
    QVERIFY(testEngine->processFile(testETIFilePath));
    
    size_t finalMemory = getCurrentMemoryUsage();
    optimizedMetrics.memory_usage_bytes.push_back(finalMemory - initialMemory);
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("Integrated memory usage: %1MB")
                          .arg((finalMemory - initialMemory) / (1024.0 * 1024.0), 0, 'f', 2));
}

void TestPerformanceRegression::testIntegratedEngineRealTimeProcessing()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Measuring Integrated Engine Real-time Processing");
    
    double fps = measureRealTimeFPS(eti::processing::EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE);
    optimizedMetrics.fps_measurements.push_back(fps);
    
    // Validate FPS target
    QVERIFY2(fps >= FPS_TARGET, 
             QString("Integrated FPS %1 below %2 FPS target").arg(fps).arg(FPS_TARGET).toLocal8Bit());
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("Integrated real-time performance: %1 FPS").arg(fps, 0, 'f', 1));
}

void TestPerformanceRegression::testSpeedImprovementRegression()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Testing Speed Improvement Regression");
    
    // Ensure we have baseline measurements
    if (baselineMetrics.processing_times_ms.empty()) {
        testLegacyWrapperBaseline();
    }
    if (optimizedMetrics.processing_times_ms.empty()) {
        testIntegratedEnginePerformance();
    }
    
    // Calculate speed improvement
    double improvement = ((baselineMetrics.mean_processing_time - optimizedMetrics.mean_processing_time) 
                         / baselineMetrics.mean_processing_time) * 100.0;
    
    // Validate improvement target
    QVERIFY2(improvement >= SPEED_IMPROVEMENT_TARGET,
             QString("Speed improvement %1% below %2% target")
             .arg(improvement, 0, 'f', 1).arg(SPEED_IMPROVEMENT_TARGET).toLocal8Bit());
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("Speed improvement validated: %1%").arg(improvement, 0, 'f', 1));
}

void TestPerformanceRegression::testMemoryReductionRegression()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Testing Memory Reduction Regression");
    
    // Ensure we have baseline measurements
    if (baselineMetrics.memory_usage_bytes.empty()) {
        testLegacyWrapperMemoryBaseline();
    }
    if (optimizedMetrics.memory_usage_bytes.empty()) {
        testIntegratedEngineMemoryUsage();
    }
    
    // Calculate memory reduction
    double reduction = ((baselineMetrics.mean_memory_usage - optimizedMetrics.mean_memory_usage) 
                       / baselineMetrics.mean_memory_usage) * 100.0;
    
    // Validate reduction target
    QVERIFY2(reduction >= MEMORY_REDUCTION_TARGET,
             QString("Memory reduction %1% below %2% target")
             .arg(reduction, 0, 'f', 1).arg(MEMORY_REDUCTION_TARGET).toLocal8Bit());
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("Memory reduction validated: %1%").arg(reduction, 0, 'f', 1));
}

void TestPerformanceRegression::testRealTimeFPSRegression()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Testing Real-time FPS Regression");
    
    // Measure multiple runs for statistical significance
    std::vector<double> fpsResults;
    for (int i = 0; i < 5; ++i) {
        double fps = measureRealTimeFPS(eti::processing::EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE);
        fpsResults.push_back(fps);
    }
    
    // Calculate mean FPS
    double meanFPS = std::accumulate(fpsResults.begin(), fpsResults.end(), 0.0) / fpsResults.size();
    
    // Validate FPS target
    QVERIFY2(meanFPS >= FPS_TARGET,
             QString("Mean FPS %1 below %2 FPS target").arg(meanFPS).arg(FPS_TARGET).toLocal8Bit());
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("Real-time FPS validated: %1 FPS").arg(meanFPS, 0, 'f', 1));
}

void TestPerformanceRegression::testLatencyRegression()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Testing Latency Regression");
    
    double latency = measureProcessingLatency(eti::processing::EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE);
    
    // Validate latency target
    QVERIFY2(latency <= LATENCY_TARGET,
             QString("Processing latency %1ms above %2ms target").arg(latency).arg(LATENCY_TARGET).toLocal8Bit());
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("Processing latency validated: %1ms").arg(latency, 0, 'f', 2));
}

void TestPerformanceRegression::testModernFrameParserPerformance()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Testing Modern Frame Parser Performance");
    
    QVERIFY(initializeTestEngine(eti::processing::EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE));
    testEngine->enableModernFrameParser(true);
    
    // Benchmark frame parsing
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < REALTIME_FRAME_COUNT; ++i) {
        QByteArray frameData = createTestETIFrame(i);
        auto result = testEngine->processFrame(frameData);
        QVERIFY(result.success);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    double fps = (REALTIME_FRAME_COUNT * 1000000.0) / duration.count();
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("Modern Frame Parser: %1 FPS").arg(fps, 0, 'f', 1));
}

// Real Implementation Performance Testing
void TestPerformanceRegression::testRealETIProcessingPerformance()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Testing Real ETI Processing Performance (7,482+ FPS target)");
    
    // Create real ETI processor for testing
    std::unique_ptr<EtiProcessor> processor = std::make_unique<EtiProcessor>();
    QVERIFY(processor->initialize());
    
    // Benchmark real ETI frame processing
    auto startTime = std::chrono::high_resolution_clock::now();
    int successfulFrames = 0;
    
    for (int i = 0; i < 10000; ++i) {
        QByteArray testFrame = createTestETIFrame(i);
        if (processor->processEtiFrame(testFrame)) {
            successfulFrames++;
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    double actualFPS = (successfulFrames * 1000000.0) / duration.count();
    
    // Validate >7482 FPS target
    QVERIFY2(actualFPS >= 7482.0, 
             QString("Real ETI processing FPS %1 below 7,482 FPS target").arg(actualFPS).toLocal8Bit());
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("Real ETI processing performance: %1 FPS (target: 7,482+)")
                          .arg(actualFPS, 0, 'f', 1));
}

void TestPerformanceRegression::testRealFIGParsingPerformance()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Testing Real FIG Parsing Performance");
    
    // Create real ETI processor for testing
    std::unique_ptr<EtiProcessor> processor = std::make_unique<EtiProcessor>();
    QVERIFY(processor->initialize());
    
    // Create test frames with FIC data
    std::vector<QByteArray> testFrames;
    for (int i = 0; i < 1000; ++i) {
        QByteArray frame = createTestETIFrame(i);
        // Add FIC flag to LIDATA field
        frame[6] |= 0x08; // Set FICF bit
        testFrames.push_back(frame);
    }
    
    // Benchmark FIG parsing
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (const auto& frame : testFrames) {
        processor->processEtiFrame(frame);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    double framesPerSecond = (testFrames.size() * 1000.0) / duration.count();
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("Real FIG parsing performance: %1 frames/second")
                          .arg(framesPerSecond, 0, 'f', 1));
    
    // Verify performance is reasonable
    QVERIFY2(framesPerSecond > 100.0, "FIG parsing performance too low");
}

void TestPerformanceRegression::testRealServiceDiscoveryPerformance()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Testing Real Service Discovery Performance");
    
    // Create real ETI processor for testing
    std::unique_ptr<EtiProcessor> processor = std::make_unique<EtiProcessor>();
    QVERIFY(processor->initialize());
    
    // Monitor service discovery signals
    QSignalSpy serviceSpy(processor.get(), &EtiProcessor::serviceDiscovered);
    QSignalSpy ensembleSpy(processor.get(), &EtiProcessor::ensembleDiscovered);
    
    // Process test file for service discovery
    auto startTime = std::chrono::high_resolution_clock::now();
    
    bool processed = processor->processFile(testETIFilePath);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    if (processed) {
        // Wait for signals
        serviceSpy.wait(1000);
        ensembleSpy.wait(1000);
        
        Logger::instance().log(Logger::Info, "PerfTest", 
                              QString("Service discovery completed in %1ms: %2 services, %3 ensembles")
                              .arg(duration.count())
                              .arg(serviceSpy.count())
                              .arg(ensembleSpy.count()));
        
        // Verify performance is reasonable
        QVERIFY2(duration.count() < 5000, "Service discovery taking too long");
    } else {
        Logger::instance().log(Logger::Warning, "PerfTest", "Service discovery test file processing failed");
    }
}

void TestPerformanceRegression::testRealETSIComplianceValidation()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Testing Real ETSI Compliance Validation Performance");
    
    // Create real ETI processor for testing
    std::unique_ptr<EtiProcessor> processor = std::make_unique<EtiProcessor>();
    QVERIFY(processor->initialize());
    
    // Create test frames with various compliance issues
    std::vector<QByteArray> testFrames;
    
    // Valid frame
    testFrames.push_back(createTestETIFrame(0));
    
    // Invalid sync pattern
    QByteArray invalidSync = createTestETIFrame(1);
    invalidSync[0] = 0xFF; // Corrupt sync
    testFrames.push_back(invalidSync);
    
    // Invalid frame count
    QByteArray invalidFC = createTestETIFrame(2);
    invalidFC[4] = 250; // FC > 249
    testFrames.push_back(invalidFC);
    
    // Benchmark compliance validation
    auto startTime = std::chrono::high_resolution_clock::now();
    
    int validFrames = 0;
    for (const auto& frame : testFrames) {
        if (processor->processEtiFrame(frame)) {
            validFrames++;
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    double complianceChecksPerSecond = (testFrames.size() * 1000000.0) / duration.count();
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("ETSI compliance validation: %1 checks/second, %2/%3 valid frames")
                          .arg(complianceChecksPerSecond, 0, 'f', 1)
                          .arg(validFrames)
                          .arg(testFrames.size()));
    
    // Verify compliance checking detected issues
    QVERIFY2(validFrames < static_cast<int>(testFrames.size()), "Compliance validation should reject invalid frames");
}

void TestPerformanceRegression::testEnhancedFIGAnalyserPerformance()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Testing Enhanced FIG Analyser Performance");
    
    QVERIFY(initializeTestEngine(eti::processing::EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE));
    testEngine->enableEnhancedFIGAnalyser(true);
    
    // Benchmark FIG analysis
    double processingTime = measureProcessingTime(eti::processing::EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE);
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("Enhanced FIG Analyser: %1ms processing time").arg(processingTime, 0, 'f', 2));
}

void TestPerformanceRegression::testETIProcessingCachePerformance()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Testing ETI Processing Cache Performance");
    
    QVERIFY(initializeTestEngine(eti::processing::EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE));
    
    auto config = testEngine->getConfiguration();
    config.performance_config.enable_caching = true;
    QVERIFY(testEngine->updateConfiguration(config));
    
    // Measure cache effectiveness
    double uncachedTime = measureProcessingTime(eti::processing::EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE);
    
    // Process same data again (should be cached)
    double cachedTime = measureProcessingTime(eti::processing::EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE);
    
    // Cache should improve performance
    QVERIFY2(cachedTime <= uncachedTime, "Cache did not improve performance");
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("Cache performance: %1ms uncached, %2ms cached")
                          .arg(uncachedTime, 0, 'f', 2).arg(cachedTime, 0, 'f', 2));
}

void TestPerformanceRegression::testPerformanceProfilerOverhead()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Testing Performance Profiler Overhead");
    
    // Measure without profiling
    QVERIFY(initializeTestEngine(eti::processing::EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE));
    auto config = testEngine->getConfiguration();
    config.enable_migration_metrics = false;
    QVERIFY(testEngine->updateConfiguration(config));
    
    double timeWithoutProfiling = measureProcessingTime(eti::processing::EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE);
    
    // Measure with profiling
    config.enable_migration_metrics = true;
    QVERIFY(testEngine->updateConfiguration(config));
    
    double timeWithProfiling = measureProcessingTime(eti::processing::EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE);
    
    // Profiling overhead should be minimal (<5%)
    double overhead = ((timeWithProfiling - timeWithoutProfiling) / timeWithoutProfiling) * 100.0;
    QVERIFY2(overhead < 5.0, QString("Profiling overhead %1% exceeds 5% limit").arg(overhead).toLocal8Bit());
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("Profiling overhead: %1%").arg(overhead, 0, 'f', 2));
}

void TestPerformanceRegression::testPerformanceConsistency()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Testing Performance Consistency");
    
    std::vector<double> times;
    for (int i = 0; i < PERFORMANCE_TEST_RUNS; ++i) {
        double time = measureProcessingTime(eti::processing::EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE);
        times.push_back(time);
    }
    
    // Calculate coefficient of variation
    double mean = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
    double variance = 0.0;
    for (double time : times) {
        variance += (time - mean) * (time - mean);
    }
    double stdDev = std::sqrt(variance / times.size());
    double cv = (stdDev / mean) * 100.0;
    
    // Coefficient of variation should be <10% for consistency
    QVERIFY2(cv < 10.0, QString("Performance CV %1% exceeds 10% consistency limit").arg(cv).toLocal8Bit());
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("Performance consistency: CV = %1%").arg(cv, 0, 'f', 2));
}

void TestPerformanceRegression::testPerformanceDistribution()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Testing Performance Distribution");
    
    // Ensure we have enough measurements
    if (optimizedMetrics.processing_times_ms.size() < PERFORMANCE_TEST_RUNS) {
        for (int i = optimizedMetrics.processing_times_ms.size(); i < PERFORMANCE_TEST_RUNS; ++i) {
            double time = measureProcessingTime(eti::processing::EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE);
            optimizedMetrics.processing_times_ms.push_back(time);
        }
        optimizedMetrics.calculateStatistics();
    }
    
    // Check for normal distribution characteristics
    std::sort(optimizedMetrics.processing_times_ms.begin(), optimizedMetrics.processing_times_ms.end());
    
    double median = optimizedMetrics.processing_times_ms[optimizedMetrics.processing_times_ms.size() / 2];
    double q1 = optimizedMetrics.processing_times_ms[optimizedMetrics.processing_times_ms.size() / 4];
    double q3 = optimizedMetrics.processing_times_ms[3 * optimizedMetrics.processing_times_ms.size() / 4];
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("Performance distribution: Q1=%1ms, Median=%2ms, Q3=%3ms")
                          .arg(q1, 0, 'f', 2).arg(median, 0, 'f', 2).arg(q3, 0, 'f', 2));
}

void TestPerformanceRegression::testPerformanceOutliers()
{
    Logger::instance().log(Logger::Info, "PerfTest", "Testing Performance Outliers");
    
    // Use existing measurements or create new ones
    if (optimizedMetrics.processing_times_ms.size() < PERFORMANCE_TEST_RUNS) {
        testPerformanceDistribution(); // This will ensure we have enough measurements
    }
    
    // Detect outliers using IQR method
    std::vector<double> times = optimizedMetrics.processing_times_ms;
    std::sort(times.begin(), times.end());
    
    double q1 = times[times.size() / 4];
    double q3 = times[3 * times.size() / 4];
    double iqr = q3 - q1;
    double lowerBound = q1 - 1.5 * iqr;
    double upperBound = q3 + 1.5 * iqr;
    
    int outlierCount = 0;
    for (double time : times) {
        if (time < lowerBound || time > upperBound) {
            outlierCount++;
        }
    }
    
    // Less than 10% outliers is acceptable
    double outlierPercent = (outlierCount * 100.0) / times.size();
    QVERIFY2(outlierPercent < 10.0, 
             QString("Outlier percentage %1% exceeds 10% limit").arg(outlierPercent).toLocal8Bit());
    
    Logger::instance().log(Logger::Info, "PerfTest", 
                          QString("Performance outliers: %1% (%2 of %3 measurements)")
                          .arg(outlierPercent, 0, 'f', 1).arg(outlierCount).arg(times.size()));
}

// Helper method implementations
bool TestPerformanceRegression::initializeTestEngine(eti::processing::EngineConfiguration::ProcessingMode mode)
{
    eti::processing::EngineConfiguration config;
    config.processing_mode = mode;
    config.components.enable_performance_comparison = true;
    config.enable_migration_metrics = true;
    
    testEngine = eti::processing::createConfiguredEngine(config);
    if (!testEngine) {
        return false;
    }
    
    return testEngine->initialize(config);
}

TestPerformanceRegression::PerformanceMetrics 
TestPerformanceRegression::measureEnginePerformance(eti::processing::EngineConfiguration::ProcessingMode mode)
{
    PerformanceMetrics metrics;
    
    for (int run = 0; run < PERFORMANCE_TEST_RUNS; ++run) {
        QVERIFY(initializeTestEngine(mode));
        
        size_t initialMemory = getCurrentMemoryUsage();
        auto startTime = std::chrono::high_resolution_clock::now();
        
        QVERIFY(testEngine->processFile(testETIFilePath));
        
        auto endTime = std::chrono::high_resolution_clock::now();
        size_t finalMemory = getCurrentMemoryUsage();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        double processingTime = duration.count() / 1000.0; // Convert to milliseconds
        
        metrics.processing_times_ms.push_back(processingTime);
        metrics.memory_usage_bytes.push_back(finalMemory - initialMemory);
        
        testEngine.reset(); // Clean up for next run
    }
    
    return metrics;
}

size_t TestPerformanceRegression::getCurrentMemoryUsage()
{
    // On Linux, read from /proc/self/status
    std::ifstream statusFile("/proc/self/status");
    if (!statusFile.is_open()) {
        return 0;
    }
    
    std::string line;
    while (std::getline(statusFile, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            std::istringstream iss(line);
            std::string label, value, unit;
            iss >> label >> value >> unit;
            
            size_t memory = std::stoul(value);
            if (unit == "kB") {
                memory *= 1024; // Convert to bytes
            }
            return memory;
        }
    }
    
    return 0;
}

double TestPerformanceRegression::measureProcessingTime(eti::processing::EngineConfiguration::ProcessingMode mode)
{
    QVERIFY(initializeTestEngine(mode));
    
    auto startTime = std::chrono::high_resolution_clock::now();
    QVERIFY(testEngine->processFile(testETIFilePath));
    auto endTime = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    return duration.count() / 1000.0; // Convert to milliseconds
}

double TestPerformanceRegression::measureRealTimeFPS(eti::processing::EngineConfiguration::ProcessingMode mode)
{
    QVERIFY(initializeTestEngine(mode));
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < REALTIME_FRAME_COUNT; ++i) {
        QByteArray frameData = createTestETIFrame(i);
        auto result = testEngine->processFrame(frameData);
        QVERIFY(result.success);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    return (REALTIME_FRAME_COUNT * 1000000.0) / duration.count();
}

double TestPerformanceRegression::measureProcessingLatency(eti::processing::EngineConfiguration::ProcessingMode mode)
{
    QVERIFY(initializeTestEngine(mode));
    
    QByteArray frameData = createTestETIFrame();
    
    auto startTime = std::chrono::high_resolution_clock::now();
    auto result = testEngine->processFrame(frameData);
    auto endTime = std::chrono::high_resolution_clock::now();
    
    QVERIFY(result.success);
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    return duration.count() / 1000.0; // Convert to milliseconds
}

QByteArray TestPerformanceRegression::createTestETIFrame(uint32_t frameNumber)
{
    QByteArray frame(6144, 0);
    
    // ETI sync word
    frame[0] = 0x49;
    frame[1] = 0x6F;
    frame[2] = 0x52;
    frame[3] = 0x85;
    
    // Frame number
    frame[4] = (frameNumber >> 24) & 0xFF;
    frame[5] = (frameNumber >> 16) & 0xFF;
    frame[6] = (frameNumber >> 8) & 0xFF;
    frame[7] = frameNumber & 0xFF;
    
    // Fill with test pattern
    for (int i = 8; i < 6144; ++i) {
        frame[i] = (i + frameNumber) & 0xFF;
    }
    
    return frame;
}

bool TestPerformanceRegression::validatePerformanceImprovement(const PerformanceMetrics& baseline, 
                                                              const PerformanceMetrics& optimized)
{
    double speedImprovement = ((baseline.mean_processing_time - optimized.mean_processing_time) 
                              / baseline.mean_processing_time) * 100.0;
    
    double memoryReduction = ((baseline.mean_memory_usage - optimized.mean_memory_usage) 
                             / baseline.mean_memory_usage) * 100.0;
    
    bool speedOk = speedImprovement >= SPEED_IMPROVEMENT_TARGET;
    bool memoryOk = memoryReduction >= MEMORY_REDUCTION_TARGET;
    bool fpsOk = optimized.mean_fps >= FPS_TARGET;
    
    return speedOk && memoryOk && fpsOk;
}

void TestPerformanceRegression::savePerformanceReport(const QString& filename)
{
    QFile reportFile(filename);
    if (reportFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&reportFile);
        
        stream << "Performance Regression Test Results\n";
        stream << "===================================\n\n";
        
        stream << "Legacy Wrapper Baseline:\n";
        stream << baselineMetrics.generateReport() << "\n";
        
        stream << "Integrated Engine Performance:\n";
        stream << optimizedMetrics.generateReport() << "\n";
        
        if (!baselineMetrics.processing_times_ms.empty() && !optimizedMetrics.processing_times_ms.empty()) {
            double speedImprovement = ((baselineMetrics.mean_processing_time - optimizedMetrics.mean_processing_time) 
                                      / baselineMetrics.mean_processing_time) * 100.0;
            
            double memoryReduction = ((baselineMetrics.mean_memory_usage - optimizedMetrics.mean_memory_usage) 
                                     / baselineMetrics.mean_memory_usage) * 100.0;
            
            stream << "Performance Improvements:\n";
            stream << QString("Speed Improvement: %1% (Target: %2%)\n")
                      .arg(speedImprovement, 0, 'f', 1).arg(SPEED_IMPROVEMENT_TARGET);
            stream << QString("Memory Reduction: %1% (Target: %2%)\n")
                      .arg(memoryReduction, 0, 'f', 1).arg(MEMORY_REDUCTION_TARGET);
            
            bool targetsMetAnalysis = speedImprovement >= SPEED_IMPROVEMENT_TARGET && 
                                     memoryReduction >= MEMORY_REDUCTION_TARGET;
            stream << QString("Targets Met: %1\n").arg(targetsMetAnalysis ? "YES" : "NO");
        }
        
        reportFile.close();
        
        Logger::instance().log(Logger::Info, "PerfTest", 
                              QString("Performance report saved to: %1").arg(filename));
    }
}

QTEST_MAIN(TestPerformanceRegression)
#include "test_performance_regression.moc"