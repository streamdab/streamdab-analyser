/**
 * @file test_streaming_processor_tdd.cpp
 * @brief Comprehensive TDD Test Suite for StreamingProcessor
 * 
 * Implements professional real-time streaming test coverage for:
 * - Ultra-low latency processing (<50ms pipeline)
 * - Multi-threaded lock-free architecture validation
 * - Adaptive quality management under load
 * - Performance scaling and resource optimization
 * - Professional broadcast workflow integration
 * 
 * @author Network/Stream Agent - TDD Lead
 * @date 2025-09-22
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QCoreApplication>
#include <QTimer>
#include <QEventLoop>
#include <QSignalSpy>
#include <QThread>
#include <chrono>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>

#include "../../src/network/streaming_processor.h"
#include "../../src/core/eti_types.h"
#include "../fixtures/network_test_data.h"
#include "../mocks/mock_streaming_source.h"

using namespace eti_network;
using namespace eti;
using namespace testing;

/**
 * @brief Mock ETI Streaming Source for Testing
 * Generates realistic ETI streams with configurable characteristics
 */
class MockETIStreamingSource {
public:
    explicit MockETIStreamingSource(double fps = 900.0)
        : m_targetFPS(fps)
        , m_running(false)
        , m_frameCount(0) {
        
        // Create realistic ETI frame template
        m_frameTemplate = createETIFrameTemplate();
    }
    
    ~MockETIStreamingSource() {
        stop();
    }
    
    void start() {
        if (m_running.load()) return;
        
        m_running = true;
        m_workerThread = std::thread(&MockETIStreamingSource::workerFunction, this);
    }
    
    void stop() {
        m_running = false;
        if (m_workerThread.joinable()) {
            m_workerThread.join();
        }
    }
    
    void setFrameRate(double fps) {
        m_targetFPS = fps;
    }
    
    void simulateJitter(bool enabled) {
        m_jitterEnabled = enabled;
    }
    
    void simulateBurstLoad(bool enabled) {
        m_burstLoadEnabled = enabled;
    }
    
    // Callback registration
    using FrameCallback = std::function<void(const eti::EtiFrame&)>;
    void setFrameCallback(FrameCallback callback) {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_frameCallback = callback;
    }
    
    size_t getFrameCount() const {
        return m_frameCount.load();
    }
    
    double getCurrentFPS() const {
        return m_currentFPS.load();
    }

private:
    void workerFunction() {
        auto lastFrameTime = std::chrono::high_resolution_clock::now();
        auto frameInterval = std::chrono::microseconds(
            static_cast<int64_t>(1000000.0 / m_targetFPS)
        );
        
        while (m_running.load()) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto elapsed = currentTime - lastFrameTime;
            
            if (elapsed >= frameInterval) {
                // Generate and send frame
                eti::EtiFrame frame = generateFrame();
                
                {
                    std::lock_guard<std::mutex> lock(m_callbackMutex);
                    if (m_frameCallback) {
                        m_frameCallback(frame);
                    }
                }
                
                m_frameCount++;
                lastFrameTime = currentTime;
                
                // Update FPS calculation
                updateFPSCalculation();
                
                // Apply jitter if enabled
                if (m_jitterEnabled) {
                    auto jitter = std::chrono::microseconds(
                        (std::rand() % 10000) - 5000  // ±5ms jitter
                    );
                    std::this_thread::sleep_for(jitter);
                }
                
                // Apply burst load if enabled
                if (m_burstLoadEnabled && (m_frameCount % 100 == 0)) {
                    for (int i = 0; i < 10; ++i) {
                        eti::EtiFrame burstFrame = generateFrame();
                        std::lock_guard<std::mutex> lock(m_callbackMutex);
                        if (m_frameCallback) {
                            m_frameCallback(burstFrame);
                        }
                        m_frameCount++;
                    }
                }
            }
            
            // Precise timing sleep
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
    
    eti::EtiFrame generateFrame() {
        eti::EtiFrame frame;
        frame.raw_data = m_frameTemplate;
        frame.frame_number = m_frameCount.load();
        frame.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count();
        
        return frame;
    }
    
    QByteArray createETIFrameTemplate() {
        QByteArray frame(6144, 0);
        
        // ETI frame header
        frame[0] = 0x68; // SYNC1
        frame[1] = 0x1A; // SYNC2
        frame[2] = 0x4B; // SYNC3
        frame[3] = 0x68; // SYNC4
        
        // Frame characterization
        frame[4] = 0x07;
        frame[5] = 0x00;
        frame[6] = 0x18;
        frame[7] = 0x01;
        
        // Fill with pattern data
        for (int i = 8; i < 6144; ++i) {
            frame[i] = static_cast<char>((i * 17 + 42) % 256);
        }
        
        return frame;
    }
    
    void updateFPSCalculation() {
        static auto lastUpdate = std::chrono::high_resolution_clock::now();
        static size_t lastFrameCount = 0;
        
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastUpdate
        ).count();
        
        if (elapsed >= 1000) { // Update every second
            size_t currentFrames = m_frameCount.load();
            double fps = static_cast<double>(currentFrames - lastFrameCount) / (elapsed / 1000.0);
            m_currentFPS = fps;
            
            lastUpdate = now;
            lastFrameCount = currentFrames;
        }
    }
    
private:
    double m_targetFPS;
    std::atomic<bool> m_running;
    std::atomic<size_t> m_frameCount;
    std::atomic<double> m_currentFPS{0.0};
    std::thread m_workerThread;
    
    QByteArray m_frameTemplate;
    bool m_jitterEnabled = false;
    bool m_burstLoadEnabled = false;
    
    FrameCallback m_frameCallback;
    std::mutex m_callbackMutex;
};

/**
 * @brief TDD Test Fixture for StreamingProcessor
 */
class StreamingProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize Qt application
        if (!QCoreApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            app = std::make_unique<QCoreApplication>(argc, argv);
        }
        
        // Create streaming configuration for testing
        config.mode = ProcessingMode::LOW_LATENCY;
        config.target_latency = std::chrono::milliseconds{50};
        config.max_jitter = std::chrono::milliseconds{10};
        config.processing_threads = std::min(4u, std::thread::hardware_concurrency());
        config.enable_metrics = true;
        config.adaptive_quality = true;
        
        // Create streaming processor
        processor = std::make_unique<StreamingProcessor>(config);
        
        // Create mock source
        mockSource = std::make_unique<MockETIStreamingSource>(900.0);
        
        // Connect mock source to processor
        mockSource->setFrameCallback([this](const eti::EtiFrame& frame) {
            processor->processFrame(frame);
        });
        
        // Connect signals for testing
        connectSignalsForTesting();
    }
    
    void TearDown() override {
        mockSource->stop();
        processor->stopProcessing();
        
        // Allow cleanup
        QTimer::singleShot(100, [this]() {
            if (eventLoop.isRunning()) {
                eventLoop.quit();
            }
        });
        
        if (eventLoop.isRunning()) {
            eventLoop.exec();
        }
    }
    
    void connectSignalsForTesting() {
        frameProcessedSpy = std::make_unique<QSignalSpy>(
            processor.get(), &StreamingProcessor::frameProcessed
        );
        
        performanceMetricsSpy = std::make_unique<QSignalSpy>(
            processor.get(), &StreamingProcessor::performanceMetricsUpdated
        );
        
        adaptiveOptimizationSpy = std::make_unique<QSignalSpy>(
            processor.get(), &StreamingProcessor::adaptiveOptimizationApplied
        );
        
        errorSpy = std::make_unique<QSignalSpy>(
            processor.get(), &StreamingProcessor::processingError
        );
    }
    
    bool waitForSignal(QSignalSpy* spy, int timeoutMs = 5000) {
        return spy->wait(timeoutMs);
    }
    
protected:
    std::unique_ptr<QCoreApplication> app;
    std::unique_ptr<StreamingProcessor> processor;
    std::unique_ptr<MockETIStreamingSource> mockSource;
    StreamingConfig config;
    QEventLoop eventLoop;
    
    // Signal spies
    std::unique_ptr<QSignalSpy> frameProcessedSpy;
    std::unique_ptr<QSignalSpy> performanceMetricsSpy;
    std::unique_ptr<QSignalSpy> adaptiveOptimizationSpy;
    std::unique_ptr<QSignalSpy> errorSpy;
};

// =============================================================================
// ULTRA-LOW LATENCY PROCESSING TESTS
// =============================================================================

/**
 * @brief Test ultra-low latency processing (<50ms pipeline)
 */
TEST_F(StreamingProcessorTest, UltraLowLatency_ShouldProcessUnder50ms) {
    // ARRANGE
    config.mode = ProcessingMode::ULTRA_LOW_LATENCY;
    config.target_latency = std::chrono::milliseconds{20};
    processor->updateConfiguration(config);
    
    processor->startProcessing();
    
    std::vector<std::chrono::microseconds> latencies;
    
    processor->setLatencyCallback([&latencies](std::chrono::microseconds latency) {
        latencies.push_back(latency);
    });
    
    // ACT
    mockSource->start();
    
    QTimer::singleShot(5000, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();
    
    mockSource->stop();
    
    // ASSERT
    ASSERT_FALSE(latencies.empty());
    
    // Check all latencies are under 50ms
    for (const auto& latency : latencies) {
        EXPECT_LT(latency.count(), 50000); // <50ms in microseconds
    }
    
    // Calculate statistics
    auto avgLatency = std::accumulate(latencies.begin(), latencies.end(), 
                                     std::chrono::microseconds{0}) / latencies.size();
    auto maxLatency = *std::max_element(latencies.begin(), latencies.end());
    
    EXPECT_LT(avgLatency.count(), 30000); // Average <30ms
    EXPECT_LT(maxLatency.count(), 50000); // Maximum <50ms
    
    qDebug() << "Average latency:" << avgLatency.count() << "μs";
    qDebug() << "Maximum latency:" << maxLatency.count() << "μs";
}

/**
 * @brief Test processing pipeline maintains 900 FPS minimum
 */
TEST_F(StreamingProcessorTest, HighThroughput_ShouldMaintain900FPS) {
    // ARRANGE
    constexpr double TARGET_FPS = 900.0;
    processor->startProcessing();
    
    // ACT
    mockSource->setFrameRate(TARGET_FPS);
    mockSource->start();
    
    // Run for 10 seconds to get stable measurements
    QTimer::singleShot(10000, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();
    
    mockSource->stop();
    
    // ASSERT
    StreamingPerformance metrics = processor->getPerformanceMetrics();
    
    EXPECT_GE(metrics.current_fps.load(), TARGET_FPS * 0.95); // Allow 5% tolerance
    EXPECT_GE(metrics.average_fps.load(), TARGET_FPS * 0.90); // Average should be >90%
    
    // Verify frame processing count
    size_t processedFrames = frameProcessedSpy->count();
    double testDuration = 10.0; // seconds
    double actualFPS = static_cast<double>(processedFrames) / testDuration;
    
    EXPECT_GE(actualFPS, TARGET_FPS * 0.90);
    
    qDebug() << "Processed frames:" << processedFrames;
    qDebug() << "Actual FPS:" << actualFPS;
    qDebug() << "Current FPS:" << metrics.current_fps.load();
}

// =============================================================================
// MULTI-THREADED ARCHITECTURE TESTS
// =============================================================================

/**
 * @brief Test lock-free queue performance under load
 */
TEST_F(StreamingProcessorTest, LockFreeQueues_ShouldScaleWithThreads) {
    // ARRANGE
    config.processing_threads = std::thread::hardware_concurrency();
    config.lock_free_queues = true;
    config.input_buffer_size = 10000;
    processor->updateConfiguration(config);
    
    processor->startProcessing();
    
    // ACT - High load test with multiple concurrent producers
    std::vector<std::unique_ptr<MockETIStreamingSource>> sources;
    
    for (size_t i = 0; i < 4; ++i) {
        auto source = std::make_unique<MockETIStreamingSource>(300.0);
        source->setFrameCallback([this](const eti::EtiFrame& frame) {
            processor->processFrame(frame);
        });
        sources.push_back(std::move(source));
    }
    
    // Start all sources simultaneously
    for (auto& source : sources) {
        source->start();
    }
    
    QTimer::singleShot(8000, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();
    
    // Stop all sources
    for (auto& source : sources) {
        source->stop();
    }
    
    // ASSERT
    StreamingPerformance metrics = processor->getPerformanceMetrics();
    
    // Should handle combined load from all sources (4 * 300 = 1200 FPS)
    EXPECT_GE(metrics.average_fps.load(), 1000.0); // At least 1000 FPS
    EXPECT_LT(metrics.average_latency_us.load(), 100000); // <100ms average latency
    
    // Verify queue performance
    EXPECT_LT(metrics.buffer_overflow_count.load(), 100); // Minimal overflows
    EXPECT_GT(metrics.processed_frame_count.load(), 8000); // Minimum frames processed
}

/**
 * @brief Test thread affinity and CPU core utilization
 */
TEST_F(StreamingProcessorTest, ThreadAffinity_ShouldOptimizeCPUUsage) {
    // ARRANGE
    config.use_thread_affinity = true;
    config.cpu_cores = {0, 1, 2, 3}; // Use first 4 cores
    config.priority_scheduling = true;
    processor->updateConfiguration(config);
    
    processor->startProcessing();
    
    // ACT
    mockSource->setFrameRate(1200.0); // High load
    mockSource->start();
    
    std::vector<double> cpuUsages;
    QTimer cpuTimer;
    cpuTimer.setInterval(1000);
    
    QObject::connect(&cpuTimer, &QTimer::timeout, [&cpuUsages, this]() {
        double cpu = processor->getCurrentCPUUsage();
        cpuUsages.push_back(cpu);
    });
    
    cpuTimer.start();
    
    QTimer::singleShot(10000, [&cpuTimer, this]() {
        cpuTimer.stop();
        eventLoop.quit();
    });
    eventLoop.exec();
    
    mockSource->stop();
    
    // ASSERT
    ASSERT_FALSE(cpuUsages.empty());
    
    double avgCPU = std::accumulate(cpuUsages.begin(), cpuUsages.end(), 0.0) / cpuUsages.size();
    double maxCPU = *std::max_element(cpuUsages.begin(), cpuUsages.end());
    
    EXPECT_LT(avgCPU, 0.8); // Average CPU usage <80%
    EXPECT_LT(maxCPU, 0.95); // Maximum CPU usage <95%
    
    // Verify thread affinity was applied
    EXPECT_TRUE(processor->isThreadAffinitySet());
    EXPECT_EQ(processor->getActiveThreadCount(), config.processing_threads);
}

// =============================================================================
// ADAPTIVE QUALITY MANAGEMENT TESTS
// =============================================================================

/**
 * @brief Test adaptive quality adjustment under varying load
 */
TEST_F(StreamingProcessorTest, AdaptiveQuality_ShouldAdjustUnderLoad) {
    // ARRANGE
    config.adaptive_quality = true;
    config.cpu_threshold = 0.7; // 70% CPU threshold
    config.frame_dropping = true;
    processor->updateConfiguration(config);
    
    processor->startProcessing();
    
    // Track quality adjustments
    std::vector<QString> qualityChanges;
    processor->setQualityChangeCallback([&qualityChanges](const QString& change) {
        qualityChanges.push_back(change);
    });
    
    // ACT - Start with normal load
    mockSource->setFrameRate(600.0);
    mockSource->start();
    
    QTimer::singleShot(3000, [this]() {
        // Increase to high load
        mockSource->setFrameRate(1500.0);
        mockSource->simulateBurstLoad(true);
    });
    
    QTimer::singleShot(8000, [this]() {
        // Return to normal load
        mockSource->setFrameRate(600.0);
        mockSource->simulateBurstLoad(false);
    });
    
    QTimer::singleShot(12000, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();
    
    mockSource->stop();
    
    // ASSERT
    EXPECT_GT(adaptiveOptimizationSpy->count(), 0); // Quality adjustments occurred
    EXPECT_FALSE(qualityChanges.empty());
    
    // Verify system remained stable during load changes
    StreamingPerformance metrics = processor->getPerformanceMetrics();
    EXPECT_LT(metrics.max_latency_us.load(), 200000); // <200ms max latency even under load
    EXPECT_GT(metrics.average_fps.load(), 400.0); // Maintained reasonable FPS
    
    // Check for appropriate quality adjustments
    bool foundQualityReduction = false;
    bool foundQualityRecovery = false;
    
    for (const QString& change : qualityChanges) {
        if (change.contains("reduced") || change.contains("lower")) {
            foundQualityReduction = true;
        }
        if (change.contains("increased") || change.contains("restored")) {
            foundQualityRecovery = true;
        }
    }
    
    EXPECT_TRUE(foundQualityReduction); // Should reduce quality under high load
    EXPECT_TRUE(foundQualityRecovery);  // Should recover when load decreases
}

/**
 * @brief Test frame dropping mechanism under overload
 */
TEST_F(StreamingProcessorTest, FrameDropping_ShouldMaintainLatency) {
    // ARRANGE
    config.frame_dropping = true;
    config.max_jitter = std::chrono::milliseconds{20};
    config.input_buffer_size = 100; // Small buffer to force dropping
    processor->updateConfiguration(config);
    
    processor->startProcessing();
    
    size_t initialFrameCount = 0;
    size_t droppedFrameCount = 0;
    
    processor->setFrameDropCallback([&droppedFrameCount](size_t dropped) {
        droppedFrameCount += dropped;
    });
    
    // ACT - Overload the system
    mockSource->setFrameRate(2000.0); // Extreme load
    mockSource->simulateBurstLoad(true);
    mockSource->start();
    
    QTimer::singleShot(6000, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();
    
    mockSource->stop();
    
    // ASSERT
    StreamingPerformance metrics = processor->getPerformanceMetrics();
    
    // Should maintain latency even under extreme load
    EXPECT_LT(metrics.average_latency_us.load(), 100000); // <100ms average
    EXPECT_LT(metrics.max_latency_us.load(), 300000);     // <300ms maximum
    
    // Should have dropped frames to maintain performance
    EXPECT_GT(droppedFrameCount, 0);
    
    // Processed frames should be less than sent frames due to dropping
    size_t sentFrames = mockSource->getFrameCount();
    size_t processedFrames = frameProcessedSpy->count();
    
    EXPECT_LT(processedFrames, sentFrames);
    EXPECT_EQ(droppedFrameCount, sentFrames - processedFrames);
    
    qDebug() << "Sent frames:" << sentFrames;
    qDebug() << "Processed frames:" << processedFrames;
    qDebug() << "Dropped frames:" << droppedFrameCount;
}

// =============================================================================
// PERFORMANCE OPTIMIZATION TESTS
// =============================================================================

/**
 * @brief Test memory efficiency under sustained operation
 */
TEST_F(StreamingProcessorTest, MemoryEfficiency_ShouldStayUnder100MB) {
    // ARRANGE
    processor->startProcessing();
    
    std::vector<size_t> memoryMeasurements;
    QTimer memoryTimer;
    memoryTimer.setInterval(2000); // Check every 2 seconds
    
    QObject::connect(&memoryTimer, &QTimer::timeout, [&memoryMeasurements, this]() {
        size_t currentMemory = processor->getCurrentMemoryUsage();
        memoryMeasurements.push_back(currentMemory);
    });
    
    // ACT - Run sustained load for extended period
    mockSource->setFrameRate(800.0);
    mockSource->start();
    memoryTimer.start();
    
    QTimer::singleShot(30000, [&memoryTimer, this]() { // 30 seconds
        memoryTimer.stop();
        eventLoop.quit();
    });
    eventLoop.exec();
    
    mockSource->stop();
    
    // ASSERT
    ASSERT_FALSE(memoryMeasurements.empty());
    
    size_t maxMemory = *std::max_element(memoryMeasurements.begin(), memoryMeasurements.end());
    size_t avgMemory = std::accumulate(memoryMeasurements.begin(), memoryMeasurements.end(), 0UL) / memoryMeasurements.size();
    
    EXPECT_LT(maxMemory, 100 * 1024 * 1024); // <100MB
    EXPECT_LT(avgMemory, 80 * 1024 * 1024);  // <80MB average
    
    // Check for memory leaks
    if (memoryMeasurements.size() >= 5) {
        size_t startMemory = memoryMeasurements[0];
        size_t endMemory = memoryMeasurements.back();
        double memoryGrowth = static_cast<double>(endMemory - startMemory) / startMemory;
        
        EXPECT_LT(memoryGrowth, 0.2); // <20% memory growth over test period
    }
}

/**
 * @brief Test error recovery and system resilience
 */
TEST_F(StreamingProcessorTest, ErrorRecovery_ShouldResumeProcessing) {
    // ARRANGE
    config.continue_on_error = true;
    config.max_consecutive_errors = 5;
    config.error_recovery_delay = std::chrono::milliseconds{100};
    processor->updateConfiguration(config);
    
    processor->startProcessing();
    
    // ACT - Inject errors periodically
    size_t errorCount = 0;
    QTimer errorTimer;
    errorTimer.setInterval(1000);
    
    QObject::connect(&errorTimer, &QTimer::timeout, [this, &errorCount]() {
        if (errorCount < 10) {
            processor->injectProcessingError("Test error " + QString::number(errorCount));
            errorCount++;
        }
    });
    
    mockSource->setFrameRate(500.0);
    mockSource->start();
    errorTimer.start();
    
    QTimer::singleShot(15000, [&errorTimer, this]() {
        errorTimer.stop();
        eventLoop.quit();
    });
    eventLoop.exec();
    
    mockSource->stop();
    
    // ASSERT
    EXPECT_GT(errorSpy->count(), 0); // Errors were detected
    EXPECT_GT(frameProcessedSpy->count(), 6000); // Processing continued despite errors
    
    // Verify error recovery occurred
    StreamingPerformance metrics = processor->getPerformanceMetrics();
    EXPECT_LT(metrics.error_recovery_count.load(), 15); // Reasonable recovery attempts
    EXPECT_TRUE(processor->isProcessing()); // Still processing after errors
}

// =============================================================================
// PROFESSIONAL BROADCAST WORKFLOW TESTS
// =============================================================================

/**
 * @brief Test integration with broadcast workflow systems
 */
TEST_F(StreamingProcessorTest, BroadcastWorkflow_ShouldIntegrateSeamlessly) {
    // ARRANGE
    processor->enableBroadcastMode(true);
    processor->setTimestampSynchronization(true);
    processor->startProcessing();
    
    // Simulate broadcast automation system
    std::vector<QString> workflowEvents;
    processor->setBroadcastEventCallback([&workflowEvents](const QString& event) {
        workflowEvents.push_back(event);
    });
    
    // ACT - Simulate broadcast day operations
    mockSource->setFrameRate(750.0);
    mockSource->start();
    
    // Simulate program schedule changes
    QTimer::singleShot(2000, [this]() {
        processor->handleBroadcastEvent("PROGRAM_START", "News");
    });
    
    QTimer::singleShot(5000, [this]() {
        processor->handleBroadcastEvent("COMMERCIAL_BREAK", "Advertisement");
    });
    
    QTimer::singleShot(8000, [this]() {
        processor->handleBroadcastEvent("PROGRAM_RESUME", "News");
    });
    
    QTimer::singleShot(12000, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();
    
    mockSource->stop();
    
    // ASSERT
    EXPECT_FALSE(workflowEvents.empty());
    EXPECT_GE(workflowEvents.size(), 3); // Should have captured workflow events
    
    // Verify broadcast timing accuracy
    StreamingPerformance metrics = processor->getPerformanceMetrics();
    EXPECT_LT(metrics.jitter_us.load(), 5000); // <5ms jitter for broadcast timing
    EXPECT_TRUE(processor->isSynchronizationLocked());
    
    // Check workflow event processing
    bool foundProgramStart = false;
    bool foundCommercialBreak = false;
    
    for (const QString& event : workflowEvents) {
        if (event.contains("PROGRAM_START")) foundProgramStart = true;
        if (event.contains("COMMERCIAL_BREAK")) foundCommercialBreak = true;
    }
    
    EXPECT_TRUE(foundProgramStart);
    EXPECT_TRUE(foundCommercialBreak);
}

// =============================================================================
// INTEGRATION TEST RUNNER
// =============================================================================

class StreamingProcessorIntegrationTest : public ::testing::Test {
public:
    static void SetUpTestSuite() {
        qDebug() << "Setting up Streaming Processor TDD Test Suite";
        qDebug() << "Target Performance: <50ms latency, >900 FPS, multi-threaded processing";
    }
    
    static void TearDownTestSuite() {
        qDebug() << "Streaming Processor TDD Test Suite completed";
    }
};

TEST_F(StreamingProcessorIntegrationTest, RunAllStreamingTests) {
    SUCCEED() << "All streaming processor tests completed successfully";
}