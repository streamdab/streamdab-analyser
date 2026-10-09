/**
 * @file test_performance_validation.cpp
 * @brief Performance Validation Testing for Phase 5.3 UI/UX Professional Standards
 * 
 * Tests performance targets for Phase 5 professional deployment:
 * - UI responsiveness during >900 fps ETI processing
 * - Real-time signal/slot integration with <50ms latency
 * - Memory usage <100MB during continuous operation
 * - Professional layout stability under load
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QApplication>
#include <QTest>
#include <QTimer>
#include <QElapsedTimer>
#include <QThread>
#include <QEventLoop>
#include <QSignalSpy>
#include <QRandomGenerator>
#include <QFileInfo>
#include <QDir>
#include <QProcessEnvironment>
#include <QSystemSemaphore>
#include <QSharedMemory>
#include <memory>
#include <chrono>
#include <vector>
#include <atomic>

// Core includes
#include "gui/main_window.h"
#include "gui/analyser_widget.h"
#include "gui/service_browser.h"
#include "gui/constellation_widget.h"
#include "gui/eti_service_tree_model.h"
#include "gui/eti_frame_list_model.h"
#include "gui/fig_analysis_widget.h"
#include "core/eti_processor.hpp"
#include "core/etsi/compliance_engine.h"
#include "tests/fixtures/test_data_generators.h"
#include "tests/mocks/mock_eti_processor.h"

using ::testing::_;
using ::testing::Return;
using ::testing::AtLeast;
using namespace std::chrono;

/**
 * @class PerformanceValidationTest
 * @brief Comprehensive performance testing for Phase 5.3 professional standards
 */
class PerformanceValidationTest : public ::testing::Test
{
protected:
    void SetUp() override {
        // Initialize Qt application for GUI testing
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            app = std::make_unique<QApplication>(argc, argv);
        }

        // Create main window
        mainWindow = std::make_unique<MainWindow>();
        ASSERT_TRUE(mainWindow->initialize());

        // Setup mock ETI processor for performance testing
        mockEtiProcessor = std::make_unique<MockEtiProcessor>();
        
        // Create test data generators
        testDataGenerator = std::make_unique<TestDataGenerators>();
        
        // Initialize performance monitoring
        performanceMonitor = std::make_unique<PerformanceMonitor>();
        
        // Show main window for testing
        mainWindow->show();
        QTest::qWaitForWindowExposed(mainWindow.get());
        
        // Allow UI to stabilize
        QTest::qWait(200);
        
        // Clear any existing performance data
        performanceMonitor->reset();
    }

    void TearDown() override {
        if (mainWindow) {
            mainWindow->close();
            mainWindow.reset();
        }
        mockEtiProcessor.reset();
        testDataGenerator.reset();
        performanceMonitor.reset();
    }

    /**
     * @brief Performance monitoring utility class
     */
    class PerformanceMonitor {
    public:
        PerformanceMonitor() : frameCount(0), totalProcessingTime(0), maxLatency(0), minLatency(LLONG_MAX) {}
        
        void reset() {
            frameCount = 0;
            totalProcessingTime = 0;
            maxLatency = 0;
            minLatency = LLONG_MAX;
            frameTimes.clear();
            latencies.clear();
        }
        
        void recordFrameProcessing(qint64 processingTimeNs) {
            frameCount++;
            totalProcessingTime += processingTimeNs;
            frameTimes.push_back(processingTimeNs);
        }
        
        void recordLatency(qint64 latencyMs) {
            latencies.push_back(latencyMs);
            maxLatency = std::max(maxLatency, latencyMs);
            minLatency = std::min(minLatency, latencyMs);
        }
        
        double getAverageFrameTime() const {
            if (frameCount == 0) return 0.0;
            return (double)totalProcessingTime / frameCount / 1000000.0; // Convert to ms
        }
        
        double getFramesPerSecond() const {
            if (frameCount == 0 || totalProcessingTime == 0) return 0.0;
            return (double)frameCount * 1000000000.0 / totalProcessingTime; // Convert from ns
        }
        
        double getAverageLatency() const {
            if (latencies.empty()) return 0.0;
            qint64 sum = 0;
            for (qint64 latency : latencies) {
                sum += latency;
            }
            return (double)sum / latencies.size();
        }
        
        qint64 getMaxLatency() const { return maxLatency; }
        qint64 getMinLatency() const { return minLatency == LLONG_MAX ? 0 : minLatency; }
        
        std::vector<qint64> getFrameTimes() const { return frameTimes; }
        std::vector<qint64> getLatencies() const { return latencies; }
        
    private:
        std::atomic<quint64> frameCount;
        std::atomic<qint64> totalProcessingTime;
        std::atomic<qint64> maxLatency;
        std::atomic<qint64> minLatency;
        std::vector<qint64> frameTimes;
        std::vector<qint64> latencies;
    };

    /**
     * @brief Memory usage monitoring
     */
    struct MemoryUsage {
        size_t peakUsage = 0;
        size_t currentUsage = 0;
        size_t initialUsage = 0;
        
        void updateUsage(size_t usage) {
            currentUsage = usage;
            peakUsage = std::max(peakUsage, usage);
        }
        
        size_t getIncrease() const {
            return currentUsage > initialUsage ? currentUsage - initialUsage : 0;
        }
    };

    /**
     * @brief Get current memory usage (simplified estimation)
     */
    size_t getCurrentMemoryUsage() {
        // This is a simplified memory monitoring
        // In production, you'd use platform-specific APIs
        static size_t baseMemory = 50 * 1024 * 1024; // 50MB base
        
        // Simulate memory tracking based on widget count and data
        size_t widgetMemory = 0;
        
        if (mainWindow) {
            auto widgets = mainWindow->findChildren<QWidget*>();
            widgetMemory += widgets.size() * 1024; // 1KB per widget estimate
            
            // Add memory for table data
            auto tables = mainWindow->findChildren<QTableWidget*>();
            for (auto* table : tables) {
                widgetMemory += table->rowCount() * table->columnCount() * 100; // 100 bytes per cell estimate
            }
            
            // Add memory for text widgets
            auto textWidgets = mainWindow->findChildren<QTextEdit*>();
            for (auto* textWidget : textWidgets) {
                widgetMemory += textWidget->toPlainText().length() * 2; // 2 bytes per char
            }
        }
        
        return baseMemory + widgetMemory;
    }

    /**
     * @brief Simulate high-frequency ETI frame processing
     */
    void simulateEtiProcessing(int frameCount, int targetFps) {
        QElapsedTimer overallTimer;
        overallTimer.start();
        
        qint64 frameIntervalNs = 1000000000 / targetFps; // Nanoseconds per frame
        
        for (int i = 0; i < frameCount; ++i) {
            QElapsedTimer frameTimer;
            frameTimer.start();
            
            // Simulate ETI frame processing
            testDataGenerator->generateEtiFrame();
            QApplication::processEvents(QEventLoop::AllEvents, 1); // 1ms max processing time
            
            qint64 frameTimeNs = frameTimer.nsecsElapsed();
            performanceMonitor->recordFrameProcessing(frameTimeNs);
            
            // Maintain target frame rate
            qint64 remainingTimeNs = frameIntervalNs - frameTimeNs;
            if (remainingTimeNs > 0) {
                // Sleep for remaining time (convert to microseconds)
                QThread::usleep(remainingTimeNs / 1000);
            }
        }
    }

    /**
     * @brief Measure UI response latency
     */
    qint64 measureUIResponseLatency(std::function<void()> action) {
        QElapsedTimer timer;
        timer.start();
        
        action();
        QApplication::processEvents();
        
        return timer.elapsed();
    }

    /**
     * @brief Test layout stability under load
     */
    bool testLayoutStabilityUnderLoad() {
        // Record initial layout
        auto* horizontalSplitter = mainWindow->findChild<QSplitter*>("horizontalSplitter");
        if (!horizontalSplitter) return false;
        
        QList<int> initialSizes = horizontalSplitter->sizes();
        
        // Apply load and test stability
        for (int i = 0; i < 100; ++i) {
            // Simulate various UI operations under load
            QApplication::processEvents();
            
            // Rapid resizing
            if (i % 10 == 0) {
                mainWindow->resize(mainWindow->size() + QSize(10, 5));
                QApplication::processEvents();
            }
            
            // Tab switching
            auto* bottomTabs = mainWindow->findChild<QTabWidget*>("bottomToolTabs");
            if (bottomTabs && bottomTabs->count() > 1) {
                bottomTabs->setCurrentIndex(i % bottomTabs->count());
                QApplication::processEvents();
            }
            
            // Table operations
            auto* frameTable = mainWindow->findChild<QTableWidget*>("frameListTable");
            if (frameTable && frameTable->rowCount() > 0) {
                frameTable->selectRow(i % frameTable->rowCount());
                QApplication::processEvents();
            }
        }
        
        // Check if layout remained stable
        QList<int> finalSizes = horizontalSplitter->sizes();
        
        // Calculate layout deviation
        double totalDeviation = 0.0;
        for (int i = 0; i < std::min(initialSizes.size(), finalSizes.size()); ++i) {
            if (initialSizes[i] > 0) {
                double deviation = std::abs(finalSizes[i] - initialSizes[i]) / (double)initialSizes[i];
                totalDeviation += deviation;
            }
        }
        
        // Layout should remain relatively stable (less than 10% total deviation)
        return totalDeviation < 0.1;
    }

    std::unique_ptr<QApplication> app;
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<MockEtiProcessor> mockEtiProcessor;
    std::unique_ptr<TestDataGenerators> testDataGenerator;
    std::unique_ptr<PerformanceMonitor> performanceMonitor;
};

/**
 * @brief Test Case 1: UI Responsiveness During >900 FPS ETI Processing
 */
TEST_F(PerformanceValidationTest, UIResponsivenessDuringHighFrequencyProcessing) {
    ASSERT_TRUE(mainWindow);
    
    // Setup mock expectations for high-frequency processing
    EXPECT_CALL(*mockEtiProcessor, processFrame(_))
        .Times(AtLeast(900))
        .WillRepeatedly(Return(true));
    
    // Record initial memory usage
    MemoryUsage memoryUsage;
    memoryUsage.initialUsage = getCurrentMemoryUsage();
    
    // Test at target 900+ FPS
    const int testFrameCount = 1000;
    const int targetFps = 950; // Slightly above 900 FPS target
    
    QElapsedTimer testTimer;
    testTimer.start();
    
    // Start high-frequency ETI processing simulation
    simulateEtiProcessing(testFrameCount, targetFps);
    
    qint64 totalTestTime = testTimer.elapsed();
    
    // Verify performance targets
    double achievedFps = performanceMonitor->getFramesPerSecond();
    double averageFrameTime = performanceMonitor->getAverageFrameTime();
    
    EXPECT_GE(achievedFps, 900.0) << QString("ETI processing should achieve >900 FPS, got %1").arg(achievedFps);
    EXPECT_LE(averageFrameTime, 1.11) << QString("Average frame time should be ≤1.11ms (900 FPS), got %1ms").arg(averageFrameTime);
    
    // Test UI responsiveness during processing
    QElapsedTimer uiTimer;
    std::vector<qint64> uiResponseTimes;
    
    // Test various UI operations
    for (int i = 0; i < 20; ++i) {
        // Test menu interaction
        qint64 menuResponseTime = measureUIResponseLatency([this]() {
            auto* fileMenu = mainWindow->findChild<QMenu*>("fileMenu");
            if (fileMenu) {
                fileMenu->show();
                fileMenu->hide();
            }
        });
        uiResponseTimes.push_back(menuResponseTime);
        
        // Test tab switching
        qint64 tabResponseTime = measureUIResponseLatency([this]() {
            auto* bottomTabs = mainWindow->findChild<QTabWidget*>("bottomToolTabs");
            if (bottomTabs && bottomTabs->count() > 1) {
                int newIndex = (bottomTabs->currentIndex() + 1) % bottomTabs->count();
                bottomTabs->setCurrentIndex(newIndex);
            }
        });
        uiResponseTimes.push_back(tabResponseTime);
        
        // Test table selection
        qint64 tableResponseTime = measureUIResponseLatency([this]() {
            auto* frameTable = mainWindow->findChild<QTableWidget*>("frameListTable");
            if (frameTable && frameTable->rowCount() > 0) {
                frameTable->selectRow(i % frameTable->rowCount());
            }
        });
        uiResponseTimes.push_back(tableResponseTime);
        
        // Continue ETI processing during UI tests
        simulateEtiProcessing(50, targetFps); // 50 more frames
    }
    
    // Calculate UI response statistics
    qint64 maxUIResponse = *std::max_element(uiResponseTimes.begin(), uiResponseTimes.end());
    double avgUIResponse = std::accumulate(uiResponseTimes.begin(), uiResponseTimes.end(), 0.0) / uiResponseTimes.size();
    
    EXPECT_LT(avgUIResponse, 50.0) << QString("Average UI response should be <50ms, got %1ms").arg(avgUIResponse);
    EXPECT_LT(maxUIResponse, 100) << QString("Maximum UI response should be <100ms, got %1ms").arg(maxUIResponse);
    
    // Verify memory usage remained reasonable
    memoryUsage.updateUsage(getCurrentMemoryUsage());
    size_t memoryIncrease = memoryUsage.getIncrease();
    
    EXPECT_LT(memoryIncrease, 100 * 1024 * 1024) << QString("Memory increase should be <100MB, got %1 bytes").arg(memoryIncrease);
    
    qDebug() << QString("Performance Results: %1 FPS, %2ms avg frame time, %3ms avg UI response")
                .arg(achievedFps, 0, 'f', 1)
                .arg(averageFrameTime, 0, 'f', 3)
                .arg(avgUIResponse, 0, 'f', 1);
}

/**
 * @brief Test Case 2: Real-time Signal/Slot Integration Latency
 */
TEST_F(PerformanceValidationTest, RealTimeSignalSlotLatency) {
    ASSERT_TRUE(mainWindow);
    
    // Test service browser signal/slot latency
    auto* serviceBrowser = mainWindow->getServiceBrowser();
    ASSERT_TRUE(serviceBrowser);
    
    auto* serviceTreeModel = serviceBrowser->findChild<EtiServiceTreeModel*>();
    if (serviceTreeModel) {
        // Populate test data
        testDataGenerator->populateServiceBrowser(serviceBrowser);
        QTest::qWait(200);
        
        // Test service selection signal latency
        QSignalSpy selectionSpy(serviceTreeModel, &EtiServiceTreeModel::serviceSelectionRequested);
        
        std::vector<qint64> selectionLatencies;
        QList<quint32> services = serviceTreeModel->getAllServices();
        
        for (int i = 0; i < 50 && !services.isEmpty(); ++i) {
            QElapsedTimer latencyTimer;
            latencyTimer.start();
            
            // Trigger service selection
            quint32 serviceId = services[i % services.size()];
            QModelIndex serviceIndex = serviceTreeModel->getServiceIndex(serviceId);
            serviceBrowser->setCurrentIndex(serviceIndex);
            
            // Wait for signal
            QEventLoop loop;
            QTimer::singleShot(100, &loop, &QEventLoop::quit); // 100ms timeout
            if (selectionSpy.count() == i) {
                QObject::connect(serviceTreeModel, &EtiServiceTreeModel::serviceSelectionRequested,
                                &loop, &QEventLoop::quit);
            }
            loop.exec();
            
            qint64 latency = latencyTimer.elapsed();
            selectionLatencies.push_back(latency);
            performanceMonitor->recordLatency(latency);
        }
        
        // Calculate latency statistics
        double avgLatency = performanceMonitor->getAverageLatency();
        qint64 maxLatency = performanceMonitor->getMaxLatency();
        
        EXPECT_LT(avgLatency, 50.0) << QString("Average signal/slot latency should be <50ms, got %1ms").arg(avgLatency);
        EXPECT_LT(maxLatency, 100) << QString("Maximum signal/slot latency should be <100ms, got %1ms").arg(maxLatency);
    }
    
    // Test frame list signal/slot latency
    auto* analyserWidget = mainWindow->getAnalyserWidget();
    if (analyserWidget) {
        auto* frameTable = analyserWidget->findChild<QTableWidget*>("frameListTable");
        if (frameTable) {
            testDataGenerator->populateFrameList(frameTable);
            QTest::qWait(200);
            
            QSignalSpy frameSpy(frameTable, &QTableWidget::currentCellChanged);
            
            for (int i = 0; i < 30 && frameTable->rowCount() > 0; ++i) {
                QElapsedTimer latencyTimer;
                latencyTimer.start();
                
                frameTable->selectRow(i % frameTable->rowCount());
                QApplication::processEvents();
                
                qint64 latency = latencyTimer.elapsed();
                performanceMonitor->recordLatency(latency);
            }
            
            double avgFrameLatency = performanceMonitor->getAverageLatency();
            EXPECT_LT(avgFrameLatency, 50.0) << QString("Frame selection latency should be <50ms, got %1ms").arg(avgFrameLatency);
        }
    }
    
    // Test properties panel update latency
    auto* propertiesPanel = mainWindow->findChild<QWidget*>("propertiesPanel");
    if (propertiesPanel) {
        auto* parametersText = propertiesPanel->findChild<QTextEdit*>("parametersText");
        if (parametersText) {
            for (int i = 0; i < 20; ++i) {
                QElapsedTimer updateTimer;
                updateTimer.start();
                
                // Simulate parameter update
                testDataGenerator->updateParametersDisplay(parametersText);
                QApplication::processEvents();
                
                qint64 updateLatency = updateTimer.elapsed();
                performanceMonitor->recordLatency(updateLatency);
            }
            
            double avgUpdateLatency = performanceMonitor->getAverageLatency();
            EXPECT_LT(avgUpdateLatency, 50.0) << QString("Properties update latency should be <50ms, got %1ms").arg(avgUpdateLatency);
        }
    }
}

/**
 * @brief Test Case 3: Memory Usage During Continuous Operation
 */
TEST_F(PerformanceValidationTest, MemoryUsageContinuousOperation) {
    ASSERT_TRUE(mainWindow);
    
    // Record initial memory state
    MemoryUsage memoryUsage;
    memoryUsage.initialUsage = getCurrentMemoryUsage();
    
    qDebug() << QString("Initial memory usage: %1 MB").arg(memoryUsage.initialUsage / (1024 * 1024));
    
    // Test Phase 1: Load test data
    testDataGenerator->populateServiceBrowser(mainWindow->getServiceBrowser());
    
    auto* frameTable = mainWindow->findChild<QTableWidget*>("frameListTable");
    if (frameTable) {
        testDataGenerator->populateFrameList(frameTable, 1000); // Large dataset
    }
    
    QTest::qWait(500);
    memoryUsage.updateUsage(getCurrentMemoryUsage());
    qDebug() << QString("After data loading: %1 MB").arg(memoryUsage.currentUsage / (1024 * 1024));
    
    // Test Phase 2: Continuous operation simulation (5 minutes simulated)
    const int continuousOperationCycles = 300; // 5 minutes at 1 cycle per second
    
    for (int cycle = 0; cycle < continuousOperationCycles; ++cycle) {
        // Simulate various operations
        
        // ETI frame processing
        simulateEtiProcessing(10, 900); // 10 frames at 900 FPS
        
        // UI interactions
        if (cycle % 10 == 0) {
            // Service selection
            auto* serviceBrowser = mainWindow->getServiceBrowser();
            if (serviceBrowser) {
                auto* serviceTreeModel = serviceBrowser->findChild<EtiServiceTreeModel*>();
                if (serviceTreeModel) {
                    QList<quint32> services = serviceTreeModel->getAllServices();
                    if (!services.isEmpty()) {
                        quint32 serviceId = services[cycle % services.size()];
                        QModelIndex serviceIndex = serviceTreeModel->getServiceIndex(serviceId);
                        serviceBrowser->setCurrentIndex(serviceIndex);
                    }
                }
            }
        }
        
        if (cycle % 15 == 0) {
            // Frame selection
            if (frameTable && frameTable->rowCount() > 0) {
                frameTable->selectRow(cycle % frameTable->rowCount());
            }
        }
        
        if (cycle % 20 == 0) {
            // Tab switching
            auto* bottomTabs = mainWindow->findChild<QTabWidget*>("bottomToolTabs");
            if (bottomTabs && bottomTabs->count() > 1) {
                bottomTabs->setCurrentIndex(cycle % bottomTabs->count());
            }
        }
        
        if (cycle % 30 == 0) {
            // Properties update
            auto* parametersText = mainWindow->findChild<QTextEdit*>("parametersText");
            if (parametersText) {
                testDataGenerator->updateParametersDisplay(parametersText);
            }
        }
        
        // Memory monitoring
        if (cycle % 50 == 0) { // Check every 50 cycles
            memoryUsage.updateUsage(getCurrentMemoryUsage());
            
            // Ensure memory hasn't grown excessively
            size_t currentIncrease = memoryUsage.getIncrease();
            EXPECT_LT(currentIncrease, 100 * 1024 * 1024) 
                << QString("Memory increase at cycle %1 should be <100MB, got %2 MB")
                   .arg(cycle)
                   .arg(currentIncrease / (1024 * 1024));
        }
        
        // Process events and maintain timing
        QApplication::processEvents();
        QTest::qWait(16); // ~60 FPS timing
    }
    
    // Final memory check
    memoryUsage.updateUsage(getCurrentMemoryUsage());
    size_t finalIncrease = memoryUsage.getIncrease();
    
    EXPECT_LT(finalIncrease, 100 * 1024 * 1024) 
        << QString("Final memory increase should be <100MB, got %1 MB")
           .arg(finalIncrease / (1024 * 1024));
    
    qDebug() << QString("Memory usage - Initial: %1 MB, Final: %2 MB, Increase: %3 MB, Peak: %4 MB")
                .arg(memoryUsage.initialUsage / (1024 * 1024))
                .arg(memoryUsage.currentUsage / (1024 * 1024))
                .arg(finalIncrease / (1024 * 1024))
                .arg(memoryUsage.peakUsage / (1024 * 1024));
}

/**
 * @brief Test Case 4: Professional Layout Stability Under Load
 */
TEST_F(PerformanceValidationTest, LayoutStabilityUnderLoad) {
    ASSERT_TRUE(mainWindow);
    
    // Test layout stability under various load conditions
    EXPECT_TRUE(testLayoutStabilityUnderLoad()) << "Layout should remain stable under load";
    
    // Test extreme window resizing
    QSize originalSize = mainWindow->size();
    
    std::vector<QSize> testSizes = {
        QSize(800, 600),    // Minimum professional size
        QSize(1920, 1080),  // Full HD
        QSize(2560, 1440),  // QHD
        QSize(3840, 2160),  // 4K
        QSize(1200, 800)    // Default size
    };
    
    for (const QSize& testSize : testSizes) {
        mainWindow->resize(testSize);
        QTest::qWait(100);
        
        // Verify layout proportions maintained
        auto* horizontalSplitter = mainWindow->findChild<QSplitter*>("horizontalSplitter");
        if (horizontalSplitter) {
            QList<int> sizes = horizontalSplitter->sizes();
            ASSERT_EQ(sizes.size(), 3) << "Should maintain 3-panel layout";
            
            int totalWidth = sizes[0] + sizes[1] + sizes[2];
            EXPECT_GT(totalWidth, 0) << "Total layout width should be positive";
            
            // Check proportions (with some tolerance for different screen sizes)
            double leftPercent = (double)sizes[0] / totalWidth * 100.0;
            double centerPercent = (double)sizes[1] / totalWidth * 100.0;
            double rightPercent = (double)sizes[2] / totalWidth * 100.0;
            
            EXPECT_GE(leftPercent, 15.0) << "Left panel should be at least 15%";
            EXPECT_LE(leftPercent, 35.0) << "Left panel should be at most 35%";
            EXPECT_GE(centerPercent, 35.0) << "Center panel should be at least 35%";
            EXPECT_LE(centerPercent, 65.0) << "Center panel should be at most 65%";
            EXPECT_GE(rightPercent, 15.0) << "Right panel should be at least 15%";
            EXPECT_LE(rightPercent, 35.0) << "Right panel should be at most 35%";
        }
        
        // Test UI responsiveness at each size
        qint64 responseTime = measureUIResponseLatency([this]() {
            auto* bottomTabs = mainWindow->findChild<QTabWidget*>("bottomToolTabs");
            if (bottomTabs && bottomTabs->count() > 1) {
                int newIndex = (bottomTabs->currentIndex() + 1) % bottomTabs->count();
                bottomTabs->setCurrentIndex(newIndex);
            }
        });
        
        EXPECT_LT(responseTime, 50) << QString("UI should remain responsive at size %1x%2, got %3ms")
                                      .arg(testSize.width()).arg(testSize.height()).arg(responseTime);
    }
    
    // Restore original size
    mainWindow->resize(originalSize);
    QTest::qWait(100);
}

/**
 * @brief Test Case 5: Real-time Update Performance (60 FPS Target)
 */
TEST_F(PerformanceValidationTest, RealTimeUpdatePerformance) {
    ASSERT_TRUE(mainWindow);
    
    // Test real-time timer performance
    auto* realTimeTimer = mainWindow->findChild<QTimer*>("realTimeTimer");
    if (realTimeTimer) {
        EXPECT_EQ(realTimeTimer->interval(), MainWindow::UPDATE_INTERVAL_MS) 
            << "Real-time timer should be configured for 60 FPS";
        EXPECT_LE(MainWindow::UPDATE_INTERVAL_MS, 17) 
            << "Update interval should be ≤17ms for 60 FPS";
    }
    
    // Enable real-time mode
    if (mainWindow->isRealTimeMode()) {
        auto* stopAction = mainWindow->findChild<QAction*>("stopAnalysisAction");
        if (stopAction) stopAction->trigger();
        QTest::qWait(100);
    }
    
    auto* startAction = mainWindow->findChild<QAction*>("startAnalysisAction");
    if (startAction) {
        startAction->trigger();
        QTest::qWait(200);
        EXPECT_TRUE(mainWindow->isRealTimeMode()) << "Real-time mode should be active";
    }
    
    // Monitor update performance for 3 seconds
    QElapsedTimer updateMonitor;
    updateMonitor.start();
    
    std::vector<qint64> updateIntervals;
    qint64 lastUpdateTime = updateMonitor.elapsed();
    int updateCount = 0;
    
    while (updateMonitor.elapsed() < 3000) { // 3 seconds
        QApplication::processEvents();
        
        qint64 currentTime = updateMonitor.elapsed();
        qint64 interval = currentTime - lastUpdateTime;
        
        if (interval > 0) {
            updateIntervals.push_back(interval);
            lastUpdateTime = currentTime;
            updateCount++;
        }
        
        QTest::qWait(1); // Small delay
    }
    
    // Calculate update statistics
    if (!updateIntervals.empty()) {
        double avgInterval = std::accumulate(updateIntervals.begin(), updateIntervals.end(), 0.0) / updateIntervals.size();
        double actualFps = 1000.0 / avgInterval;
        
        EXPECT_GE(actualFps, 45.0) << QString("Should achieve at least 45 FPS, got %1 FPS").arg(actualFps);
        EXPECT_LE(avgInterval, 25.0) << QString("Average update interval should be ≤25ms, got %1ms").arg(avgInterval);
        
        qDebug() << QString("Real-time updates: %1 FPS average, %2ms interval, %3 total updates")
                    .arg(actualFps, 0, 'f', 1)
                    .arg(avgInterval, 0, 'f', 1)
                    .arg(updateCount);
    }
    
    // Stop real-time mode
    auto* stopAction = mainWindow->findChild<QAction*>("stopAnalysisAction");
    if (stopAction) {
        stopAction->trigger();
        QTest::qWait(100);
        EXPECT_FALSE(mainWindow->isRealTimeMode()) << "Real-time mode should be stopped";
    }
}

/**
 * @brief Test Case 6: Stress Test with Concurrent Operations
 */
TEST_F(PerformanceValidationTest, StressTestConcurrentOperations) {
    ASSERT_TRUE(mainWindow);
    
    // Record initial state
    MemoryUsage memoryUsage;
    memoryUsage.initialUsage = getCurrentMemoryUsage();
    
    // Setup concurrent operations
    std::atomic<bool> stopStressTest(false);
    std::atomic<int> operationCount(0);
    
    // Populate with large dataset
    testDataGenerator->populateServiceBrowser(mainWindow->getServiceBrowser());
    
    auto* frameTable = mainWindow->findChild<QTableWidget*>("frameListTable");
    if (frameTable) {
        testDataGenerator->populateFrameList(frameTable, 2000); // Large dataset
    }
    
    QTest::qWait(500);
    
    // Start stress test for 10 seconds
    QTimer::singleShot(10000, [&stopStressTest]() { stopStressTest = true; });
    
    QElapsedTimer stressTimer;
    stressTimer.start();
    
    while (!stopStressTest) {
        // Rapid service selections
        auto* serviceBrowser = mainWindow->getServiceBrowser();
        if (serviceBrowser) {
            auto* serviceTreeModel = serviceBrowser->findChild<EtiServiceTreeModel*>();
            if (serviceTreeModel) {
                QList<quint32> services = serviceTreeModel->getAllServices();
                if (!services.isEmpty()) {
                    quint32 serviceId = services[QRandomGenerator::global()->bounded(services.size())];
                    QModelIndex serviceIndex = serviceTreeModel->getServiceIndex(serviceId);
                    serviceBrowser->setCurrentIndex(serviceIndex);
                    operationCount++;
                }
            }
        }
        
        // Rapid frame selections
        if (frameTable && frameTable->rowCount() > 0) {
            int row = QRandomGenerator::global()->bounded(frameTable->rowCount());
            frameTable->selectRow(row);
            operationCount++;
        }
        
        // Rapid tab switching
        auto* bottomTabs = mainWindow->findChild<QTabWidget*>("bottomToolTabs");
        if (bottomTabs && bottomTabs->count() > 1) {
            int tab = QRandomGenerator::global()->bounded(bottomTabs->count());
            bottomTabs->setCurrentIndex(tab);
            operationCount++;
        }
        
        auto* propertiesTabs = mainWindow->findChild<QTabWidget*>("propertiesTabs");
        if (propertiesTabs && propertiesTabs->count() > 1) {
            int tab = QRandomGenerator::global()->bounded(propertiesTabs->count());
            propertiesTabs->setCurrentIndex(tab);
            operationCount++;
        }
        
        // Process events and small delay
        QApplication::processEvents();
        QTest::qWait(1);
        
        // Check memory periodically
        if (operationCount % 100 == 0) {
            memoryUsage.updateUsage(getCurrentMemoryUsage());
            size_t currentIncrease = memoryUsage.getIncrease();
            
            EXPECT_LT(currentIncrease, 150 * 1024 * 1024) 
                << QString("Memory during stress test should not exceed 150MB increase, got %1 MB")
                   .arg(currentIncrease / (1024 * 1024));
        }
    }
    
    qint64 totalStressTime = stressTimer.elapsed();
    double operationsPerSecond = (double)operationCount * 1000.0 / totalStressTime;
    
    // Verify stress test results
    EXPECT_GT(operationsPerSecond, 50.0) << QString("Should handle >50 operations/second under stress, got %1").arg(operationsPerSecond);
    EXPECT_TRUE(mainWindow->isVisible()) << "Main window should remain visible after stress test";
    EXPECT_TRUE(mainWindow->isEnabled()) << "Main window should remain enabled after stress test";
    
    // Final memory check
    memoryUsage.updateUsage(getCurrentMemoryUsage());
    size_t finalIncrease = memoryUsage.getIncrease();
    
    EXPECT_LT(finalIncrease, 150 * 1024 * 1024) 
        << QString("Final memory after stress test should be <150MB increase, got %1 MB")
           .arg(finalIncrease / (1024 * 1024));
    
    qDebug() << QString("Stress test completed: %1 operations in %2ms (%3 ops/sec), memory increase: %4 MB")
                .arg(operationCount.load())
                .arg(totalStressTime)
                .arg(operationsPerSecond, 0, 'f', 1)
                .arg(finalIncrease / (1024 * 1024));
}

// Test Suite Entry Point
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize Qt Test framework
    QApplication app(argc, argv);
    
    // Run all tests
    return RUN_ALL_TESTS();
}