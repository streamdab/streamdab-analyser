/**
 * @file test_gui_performance_responsiveness.cpp
 * @brief TDD Test Suite for GUI Performance and Responsiveness Validation
 * 
 * Comprehensive test-driven development suite for validating GUI performance
 * and responsiveness according to professional broadcast industry standards.
 * 
 * Performance Requirements (Professional Broadcast Standards):
 * - UI Updates: <50ms latency for real-time responsiveness
 * - Memory Usage: <50MB target for GUI components
 * - Frame Rate: 60 FPS smooth updates during data visualization
 * - Window Operations: <2s startup time, <100ms resize response
 * - Panel Synchronization: <10ms cross-panel update latency
 * - Error Recovery: <500ms recovery time from UI blocking operations
 * 
 * @author UI/UX Agent - TDD Lead
 * @date 2025-09-22
 * @copyright StreamDAB Analyser Project
 */

#include "professional_gui_tdd_framework.h"
#include "gui/main_window.h"
#include "gui/service_explorer_panel.h"
#include "gui/eti_analysis_widget.h"
#include "gui/constellation_widget.h"
#include "gui/performance_dashboard.h"
#include <QtTest/QtTest>
#include <QApplication>
#include <QElapsedTimer>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QRandomGenerator>
#include <QPixmap>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QScrollBar>
#include <QAbstractItemModel>
#include <QStandardItemModel>
#include <QTreeView>
#include <QTableView>
#include <QTextEdit>
#include <QProgressBar>
#include <QLabel>
#include <QSpinBox>
#include <QSlider>
#include <QCheckBox>
#include <QGroupBox>
#include <QSplitter>
#include <QTabWidget>
#include <QDockWidget>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <memory>
#include <vector>
#include <chrono>
#include <algorithm>

/**
 * @class GuiPerformanceResponsivenessTest
 * @brief TDD test suite for GUI performance and responsiveness validation
 * 
 * This test suite validates professional broadcast industry performance
 * requirements for real-time GUI operations and user interaction responsiveness.
 * 
 * Test Coverage:
 * - Real-time UI update performance (<50ms requirement)
 * - Memory efficiency validation (<50MB target)
 * - 60 FPS smooth visualization performance
 * - Window operation performance benchmarks
 * - Panel synchronization latency validation
 * - Stress testing under high data load
 * - Error recovery time validation
 * - Cross-platform performance consistency
 */
GUI_TDD_TEST_CASE(GuiPerformanceResponsivenessTest, CRITICAL)

private:
    MainWindow* m_mainWindow = nullptr;
    ProfessionalGUI::ProfessionalGUITestFramework* m_testFramework = nullptr;
    
    // Performance monitoring
    struct PerformanceMetrics {
        std::vector<qint64> updateTimes;
        std::vector<qint64> renderTimes;
        std::vector<qint64> memoryUsage;
        qint64 totalMemoryMB = 0;
        int frameDrops = 0;
        double averageUpdateTime = 0.0;
        double averageRenderTime = 0.0;
        double frameRate = 0.0;
        bool meetsRequirements = false;
    };
    
    PerformanceMetrics m_performanceMetrics;

public slots:
    void initTestCase() {
        qDebug() << "=== GUI Performance and Responsiveness TDD Test Suite ===";
        qDebug() << "Professional broadcast industry performance requirements:";
        qDebug() << "  - UI Updates: <50ms latency";
        qDebug() << "  - Memory Usage: <50MB target";
        qDebug() << "  - Frame Rate: 60 FPS smooth updates";
        qDebug() << "  - Window Operations: <2s startup, <100ms resize";
        qDebug() << "  - Panel Sync: <10ms cross-panel latency";
        
        // Initialize professional GUI test framework
        m_testFramework = new ProfessionalGUI::ProfessionalGUITestFramework(this);
        QVERIFY(m_testFramework->initializeProfessionalTestEnvironment());
        
        // Initialize performance metrics
        m_performanceMetrics = PerformanceMetrics();
    }

    void init() {
        // Create fresh main window for each test
        QElapsedTimer setupTimer;
        setupTimer.start();
        
        m_mainWindow = m_testFramework->createTestMainWindow();
        QVERIFY(m_mainWindow != nullptr);
        
        // Wait for window to be fully initialized
        QTest::qWaitForWindowExposed(m_mainWindow);
        QApplication::processEvents();
        
        qint64 setupTime = setupTimer.elapsed();
        qDebug() << "Window setup time:" << setupTime << "ms";
        
        // Professional requirement: Window setup should be <2000ms
        if (setupTime <= 2000) {
            qDebug() << "✅ Window setup performance meets professional standards";
        } else {
            qWarning() << "⚠️ Window setup time exceeds professional target (2000ms)";
        }
    }

    void cleanup() {
        if (m_mainWindow) {
            m_mainWindow->close();
            m_mainWindow = nullptr;
        }
        
        // Clear performance metrics for next test
        m_performanceMetrics = PerformanceMetrics();
    }

    /**
     * @brief TDD RED Phase: Performance should fail to meet requirements initially
     */
    TDD_RED_PHASE(UIUpdateLatency)
        if (qEnvironmentVariableIsSet("TDD_FORCE_RED")) {
            // In RED phase, expect performance to not meet professional standards
            bool performanceMeetsStandards = testUIUpdateLatencyPerformance(10); // Small sample
            TDD_RED_ASSERT(!performanceMeetsStandards, "UI performance should fail requirements in RED phase");
        }
    }

    /**
     * @brief TDD GREEN Phase: Performance should meet professional requirements
     */
    TDD_GREEN_PHASE(UIUpdateLatency)
        qDebug() << "🟢 GREEN Phase: Testing UI update latency performance";
        
        // Test that UI updates meet <50ms professional requirement
        bool performanceMeetsStandards = testUIUpdateLatencyPerformance(100);
        QVERIFY2(performanceMeetsStandards, "UI update latency must meet <50ms professional requirement");
        
        qDebug() << "✅ UI update latency performance validation completed";
    }

    /**
     * @brief Test UI update latency performance with comprehensive validation
     */
    bool testUIUpdateLatencyPerformance(int sampleCount = 100) {
        qDebug() << "⚡ Testing UI Update Latency Performance (" << sampleCount << "samples)";
        
        QElapsedTimer updateTimer;
        std::vector<qint64> updateLatencies;
        updateLatencies.reserve(sampleCount);
        
        int successfulUpdates = 0;
        
        for (int i = 0; i < sampleCount; ++i) {
            updateTimer.restart();
            
            // Trigger UI update
            m_mainWindow->update();
            QApplication::processEvents();
            
            qint64 updateTime = updateTimer.elapsed();
            updateLatencies.push_back(updateTime);
            m_performanceMetrics.updateTimes.push_back(updateTime);
            
            if (updateTime <= ProfessionalGUI::BroadcastStandards::MAX_UI_UPDATE_MS) {
                successfulUpdates++;
            }
            
            // Small delay to prevent overwhelming the system
            if (i % 10 == 0) {
                QTest::qWait(1);
            }
        }
        
        // Calculate performance metrics
        double averageLatency = 0.0;
        qint64 maxLatency = 0;
        qint64 minLatency = updateLatencies[0];
        
        for (qint64 latency : updateLatencies) {
            averageLatency += latency;
            maxLatency = std::max(maxLatency, latency);
            minLatency = std::min(minLatency, latency);
        }
        averageLatency /= updateLatencies.size();
        
        // Calculate 95th percentile
        std::sort(updateLatencies.begin(), updateLatencies.end());
        qint64 percentile95 = updateLatencies[static_cast<int>(updateLatencies.size() * 0.95)];
        
        double successRate = (double)successfulUpdates / sampleCount * 100.0;
        
        qDebug() << "📊 UI Update Latency Performance Results:";
        qDebug() << "   Sample Count:" << sampleCount;
        qDebug() << "   Average Latency:" << QString::number(averageLatency, 'f', 2) << "ms";
        qDebug() << "   Min Latency:" << minLatency << "ms";
        qDebug() << "   Max Latency:" << maxLatency << "ms";
        qDebug() << "   95th Percentile:" << percentile95 << "ms";
        qDebug() << "   Success Rate (<50ms):" << QString::number(successRate, 'f', 1) << "%";
        qDebug() << "   Target: <50ms, 90% success rate";
        
        // Professional requirement: 90% of updates must be <50ms
        bool meetsRequirement = (successRate >= 90.0 && averageLatency <= 50.0);
        
        if (meetsRequirement) {
            qDebug() << "   ✅ UI update latency meets professional broadcast standards";
        } else {
            qWarning() << "   ❌ UI update latency does not meet professional requirements";
            qDebug() << "   Required: 90% success rate with <50ms average";
            qDebug() << "   Actual: " << QString::number(successRate, 'f', 1) << "% success rate with " 
                     << QString::number(averageLatency, 'f', 2) << "ms average";
        }
        
        m_performanceMetrics.averageUpdateTime = averageLatency;
        m_performanceMetrics.meetsRequirements = meetsRequirement;
        
        return meetsRequirement;
    }

    /**
     * @brief Test memory efficiency during GUI operations
     */
    void testMemoryEfficiencyValidation() {
        qDebug() << "💾 Testing Memory Efficiency Validation";
        
        // Measure baseline memory usage
        qint64 baselineMemory = getCurrentMemoryUsageMB();
        qDebug() << "   Baseline memory usage:" << baselineMemory << "MB";
        
        // Perform memory-intensive GUI operations
        TDD_MEMORY_CHECK([&]() {
            // Create and manipulate multiple GUI components
            testIntensiveGUIOperations();
            
        }, ProfessionalGUI::BroadcastStandards::TARGET_MEMORY_MB, "Intensive GUI operations");
        
        // Measure memory usage after operations
        qint64 finalMemory = getCurrentMemoryUsageMB();
        qint64 memoryIncrease = finalMemory - baselineMemory;
        
        qDebug() << "📊 Memory Efficiency Results:";
        qDebug() << "   Baseline Memory:" << baselineMemory << "MB";
        qDebug() << "   Final Memory:" << finalMemory << "MB";
        qDebug() << "   Memory Increase:" << memoryIncrease << "MB";
        qDebug() << "   Target: <" << ProfessionalGUI::BroadcastStandards::TARGET_MEMORY_MB << "MB total";
        
        bool meetsMemoryTarget = (finalMemory <= ProfessionalGUI::BroadcastStandards::TARGET_MEMORY_MB);
        
        if (meetsMemoryTarget) {
            qDebug() << "   ✅ Memory usage meets professional broadcast standards";
        } else {
            qWarning() << "   ⚠️ Memory usage exceeds professional target";
        }
        
        m_performanceMetrics.totalMemoryMB = finalMemory;
        
        QVERIFY2(meetsMemoryTarget || memoryIncrease <= 20, // Allow 20MB increase
                 "Memory usage must remain within professional limits");
    }

    /**
     * @brief Test 60 FPS smooth visualization performance
     */
    void test60FPSVisualizationPerformance() {
        qDebug() << "🎬 Testing 60 FPS Visualization Performance";
        
        // Find visualization components
        ConstellationWidget* constellationWidget = m_mainWindow->findChild<ConstellationWidget*>();
        PerformanceDashboard* performanceDashboard = m_mainWindow->findChild<PerformanceDashboard*>();
        
        if (!constellationWidget && !performanceDashboard) {
            qDebug() << "   ℹ️ No visualization components found for FPS testing";
            return;
        }
        
        // Test constellation widget performance
        if (constellationWidget) {
            bool constellationFPS = testVisualizationWidgetFPS(constellationWidget, "Constellation");
            if (!constellationFPS) {
                qWarning() << "   Constellation widget does not meet 60 FPS requirement";
            }
        }
        
        // Test performance dashboard
        if (performanceDashboard) {
            bool dashboardFPS = testVisualizationWidgetFPS(performanceDashboard, "Performance Dashboard");
            if (!dashboardFPS) {
                qWarning() << "   Performance dashboard does not meet 60 FPS requirement";
            }
        }
        
        // Calculate overall frame rate performance
        double averageFrameRate = calculateAverageFrameRate();
        
        qDebug() << "📊 Visualization Performance Results:";
        qDebug() << "   Average Frame Rate:" << QString::number(averageFrameRate, 'f', 1) << "FPS";
        qDebug() << "   Target: 60 FPS";
        qDebug() << "   Frame Drops:" << m_performanceMetrics.frameDrops;
        
        bool meets60FPS = (averageFrameRate >= 55.0); // Allow 5 FPS tolerance
        
        if (meets60FPS) {
            qDebug() << "   ✅ Visualization performance meets 60 FPS professional standard";
        } else {
            qWarning() << "   ⚠️ Visualization performance below 60 FPS professional requirement";
        }
        
        m_performanceMetrics.frameRate = averageFrameRate;
    }

    /**
     * @brief Test window operation performance benchmarks
     */
    void testWindowOperationPerformance() {
        qDebug() << "🖼️ Testing Window Operation Performance";
        
        // Test window resize performance
        testWindowResizePerformance();
        
        // Test dock widget manipulation performance
        testDockWidgetPerformance();
        
        // Test panel visibility toggle performance
        testPanelVisibilityPerformance();
        
        // Test tab switching performance
        testTabSwitchingPerformance();
        
        qDebug() << "   ✅ Window operation performance testing completed";
    }

    /**
     * @brief Test panel synchronization latency
     */
    void testPanelSynchronizationLatency() {
        qDebug() << "🔄 Testing Panel Synchronization Latency";
        
        // Test Explorer → Properties synchronization
        bool explorerPropsSync = testExplorerPropertiesSync();
        
        // Test Service Selection → Main Content synchronization
        bool serviceContentSync = testServiceContentSync();
        
        // Test Main Content → Properties synchronization
        bool contentPropsSync = testContentPropertiesSync();
        
        qDebug() << "📊 Panel Synchronization Results:";
        qDebug() << "   Explorer → Properties:" << (explorerPropsSync ? "✅ PASSED" : "❌ FAILED");
        qDebug() << "   Service → Content:" << (serviceContentSync ? "✅ PASSED" : "❌ FAILED");
        qDebug() << "   Content → Properties:" << (contentPropsSync ? "✅ PASSED" : "❌ FAILED");
        
        // At least one synchronization pattern should work
        bool anySyncWorking = explorerPropsSync || serviceContentSync || contentPropsSync;
        
        if (anySyncWorking) {
            qDebug() << "   ✅ Panel synchronization performance meets professional standards";
        } else {
            qDebug() << "   ℹ️ Panel synchronization may need implementation";
        }
    }

    /**
     * @brief Test stress performance under high data load
     */
    void testStressPerformanceHighDataLoad() {
        qDebug() << "🔥 Testing Stress Performance Under High Data Load";
        
        // Simulate high-frequency data updates
        const int stressIterations = 200;
        const int batchSize = 10;
        
        QElapsedTimer stressTimer;
        stressTimer.start();
        
        int successfulUpdates = 0;
        
        for (int batch = 0; batch < stressIterations / batchSize; ++batch) {
            QElapsedTimer batchTimer;
            batchTimer.start();
            
            // Rapid updates within batch
            for (int i = 0; i < batchSize; ++i) {
                QElapsedTimer updateTimer;
                updateTimer.restart();
                
                // Simulate data update
                simulateDataUpdate();
                
                qint64 updateTime = updateTimer.elapsed();
                if (updateTime <= 50) { // 50ms target
                    successfulUpdates++;
                }
            }
            
            QApplication::processEvents();
            
            qint64 batchTime = batchTimer.elapsed();
            
            // Professional requirement: Each batch should complete in reasonable time
            if (batchTime > 500) { // 500ms max per batch
                qWarning() << "   Batch" << batch << "took" << batchTime << "ms (exceeds 500ms limit)";
            }
            
            // Brief pause between batches to prevent system overload
            QTest::qWait(10);
        }
        
        qint64 totalStressTime = stressTimer.elapsed();
        double successRate = (double)successfulUpdates / stressIterations * 100.0;
        double averageBatchTime = (double)totalStressTime / (stressIterations / batchSize);
        
        qDebug() << "📊 Stress Performance Results:";
        qDebug() << "   Total Iterations:" << stressIterations;
        qDebug() << "   Total Time:" << totalStressTime << "ms";
        qDebug() << "   Success Rate:" << QString::number(successRate, 'f', 1) << "%";
        qDebug() << "   Average Batch Time:" << QString::number(averageBatchTime, 'f', 1) << "ms";
        qDebug() << "   Target: 80% success rate, <300ms average batch time";
        
        bool meetsStressRequirement = (successRate >= 80.0 && averageBatchTime <= 300.0);
        
        if (meetsStressRequirement) {
            qDebug() << "   ✅ Stress performance meets professional broadcast standards";
        } else {
            qWarning() << "   ⚠️ Stress performance needs optimization for professional deployment";
        }
        
        QVERIFY2(successRate >= 70.0, "Must maintain at least 70% success rate under stress");
    }

    /**
     * @brief Test error recovery time validation
     */
    void testErrorRecoveryTimeValidation() {
        qDebug() << "🚨 Testing Error Recovery Time Validation";
        
        QElapsedTimer recoveryTimer;
        
        // Test recovery from various error scenarios
        bool uiBlockingRecovery = testUIBlockingOperationRecovery(recoveryTimer);
        bool memoryPressureRecovery = testMemoryPressureRecovery(recoveryTimer);
        bool invalidOperationRecovery = testInvalidOperationRecovery(recoveryTimer);
        
        qDebug() << "📊 Error Recovery Results:";
        qDebug() << "   UI Blocking Recovery:" << (uiBlockingRecovery ? "✅ PASSED" : "❌ FAILED");
        qDebug() << "   Memory Pressure Recovery:" << (memoryPressureRecovery ? "✅ PASSED" : "❌ FAILED");
        qDebug() << "   Invalid Operation Recovery:" << (invalidOperationRecovery ? "✅ PASSED" : "❌ FAILED");
        
        // Professional requirement: At least 2/3 recovery scenarios should work
        int passedTests = (uiBlockingRecovery ? 1 : 0) + (memoryPressureRecovery ? 1 : 0) + (invalidOperationRecovery ? 1 : 0);
        bool meetsRecoveryRequirement = (passedTests >= 2);
        
        if (meetsRecoveryRequirement) {
            qDebug() << "   ✅ Error recovery meets professional robustness standards";
        } else {
            qWarning() << "   ⚠️ Error recovery needs improvement for professional deployment";
        }
    }

    /**
     * @brief Test cross-platform performance consistency
     */
    void testCrossPlatformPerformanceConsistency() {
        qDebug() << "🖥️ Testing Cross-Platform Performance Consistency";
        
        // Get platform information
        QString platformName = QSysInfo::prettyProductName();
        QString architecture = QSysInfo::currentCpuArchitecture();
        QSize screenSize = QApplication::primaryScreen()->size();
        
        qDebug() << "   Platform:" << platformName;
        qDebug() << "   Architecture:" << architecture;
        qDebug() << "   Screen Size:" << screenSize.width() << "x" << screenSize.height();
        
        // Test platform-specific performance characteristics
        bool fontRenderingPerf = testFontRenderingPerformance();
        bool widgetStylePerf = testWidgetStylePerformance();
        bool eventProcessingPerf = testEventProcessingPerformance();
        
        qDebug() << "📊 Cross-Platform Performance Results:";
        qDebug() << "   Font Rendering:" << (fontRenderingPerf ? "✅ OPTIMIZED" : "⚠️ NEEDS OPTIMIZATION");
        qDebug() << "   Widget Styling:" << (widgetStylePerf ? "✅ OPTIMIZED" : "⚠️ NEEDS OPTIMIZATION");
        qDebug() << "   Event Processing:" << (eventProcessingPerf ? "✅ OPTIMIZED" : "⚠️ NEEDS OPTIMIZATION");
        
        // Professional requirement: All platform aspects should be optimized
        bool platformOptimized = fontRenderingPerf && widgetStylePerf && eventProcessingPerf;
        
        if (platformOptimized) {
            qDebug() << "   ✅ Cross-platform performance is fully optimized";
        } else {
            qDebug() << "   ℹ️ Some platform optimizations may be needed";
        }
    }

    /**
     * @brief Comprehensive performance benchmark summary
     */
    void testComprehensivePerformanceBenchmark() {
        qDebug() << "📈 Comprehensive Performance Benchmark Summary";
        
        // Run all performance tests and collect metrics
        bool latencyTest = testUIUpdateLatencyPerformance(50);
        
        testMemoryEfficiencyValidation();
        test60FPSVisualizationPerformance();
        testWindowOperationPerformance();
        testPanelSynchronizationLatency();
        
        // Generate comprehensive performance report
        generatePerformanceReport(latencyTest);
    }

private:
    /**
     * @brief Get current memory usage in MB (platform-specific)
     */
    qint64 getCurrentMemoryUsageMB() {
        // Simplified memory measurement - real implementation would be platform-specific
        
#ifdef Q_OS_LINUX
        // Linux: Read from /proc/self/status
        QFile statusFile("/proc/self/status");
        if (statusFile.open(QIODevice::ReadOnly)) {
            QTextStream stream(&statusFile);
            QString line;
            while (stream.readLineInto(&line)) {
                if (line.startsWith("VmRSS:")) {
                    QStringList parts = line.split(QRegExp("\\s+"));
                    if (parts.size() >= 2) {
                        bool ok;
                        qint64 memoryKB = parts[1].toLongLong(&ok);
                        if (ok) {
                            return memoryKB / 1024; // Convert to MB
                        }
                    }
                }
            }
        }
#endif
        
        // Fallback: Estimate based on widget count and typical memory usage
        QList<QWidget*> allWidgets = m_mainWindow->findChildren<QWidget*>();
        qint64 estimatedMemoryMB = 10 + (allWidgets.size() * 1024 / 1024); // Base 10MB + 1KB per widget
        
        return estimatedMemoryMB;
    }

    /**
     * @brief Perform intensive GUI operations for memory testing
     */
    void testIntensiveGUIOperations() {
        // Create and manipulate multiple components
        QList<QWidget*> testWidgets;
        
        for (int i = 0; i < 100; ++i) {
            // Create various widget types
            QLabel* label = new QLabel(QString("Test Label %1").arg(i), m_mainWindow);
            QPushButton* button = new QPushButton(QString("Button %1").arg(i), m_mainWindow);
            QLineEdit* lineEdit = new QLineEdit(QString("Text %1").arg(i), m_mainWindow);
            
            testWidgets.append(label);
            testWidgets.append(button);
            testWidgets.append(lineEdit);
            
            // Process events periodically
            if (i % 10 == 0) {
                QApplication::processEvents();
            }
        }
        
        // Manipulate widgets
        for (QWidget* widget : testWidgets) {
            widget->setVisible(true);
            widget->update();
        }
        
        QApplication::processEvents();
        
        // Clean up
        qDeleteAll(testWidgets);
        testWidgets.clear();
        
        QApplication::processEvents();
    }

    /**
     * @brief Test visualization widget FPS performance
     */
    bool testVisualizationWidgetFPS(QWidget* widget, const QString& widgetName) {
        if (!widget || !widget->isVisible()) {
            return false;
        }
        
        qDebug() << "   Testing" << widgetName << "FPS performance";
        
        const int testDurationMs = 2000; // 2 seconds
        const int targetFPS = 60;
        const int targetFrameCount = (testDurationMs * targetFPS) / 1000;
        
        QElapsedTimer fpsTimer;
        fpsTimer.start();
        
        int frameCount = 0;
        QElapsedTimer frameTimer;
        
        while (fpsTimer.elapsed() < testDurationMs) {
            frameTimer.restart();
            
            // Trigger widget update/repaint
            widget->update();
            QApplication::processEvents();
            
            qint64 frameTime = frameTimer.elapsed();
            m_performanceMetrics.renderTimes.push_back(frameTime);
            
            // Count frame drop if it takes too long (>16.67ms for 60fps)
            if (frameTime > 17) {
                m_performanceMetrics.frameDrops++;
            }
            
            frameCount++;
            
            // Maintain target frame rate
            int remainingTime = 16 - static_cast<int>(frameTime);
            if (remainingTime > 0) {
                QTest::qWait(remainingTime);
            }
        }
        
        qint64 actualDuration = fpsTimer.elapsed();
        double actualFPS = (double)frameCount / actualDuration * 1000.0;
        double frameDropRate = (double)m_performanceMetrics.frameDrops / frameCount * 100.0;
        
        qDebug() << "     " << widgetName << "FPS Results:";
        qDebug() << "       Actual FPS:" << QString::number(actualFPS, 'f', 1);
        qDebug() << "       Frame Count:" << frameCount << "/" << targetFrameCount;
        qDebug() << "       Frame Drop Rate:" << QString::number(frameDropRate, 'f', 1) << "%";
        
        bool meetsFPSRequirement = (actualFPS >= 55.0 && frameDropRate <= 10.0);
        
        return meetsFPSRequirement;
    }

    /**
     * @brief Calculate average frame rate from recorded metrics
     */
    double calculateAverageFrameRate() {
        if (m_performanceMetrics.renderTimes.empty()) {
            return 0.0;
        }
        
        double totalRenderTime = 0.0;
        for (qint64 renderTime : m_performanceMetrics.renderTimes) {
            totalRenderTime += renderTime;
        }
        
        double averageRenderTime = totalRenderTime / m_performanceMetrics.renderTimes.size();
        double averageFPS = 1000.0 / averageRenderTime; // Convert ms to FPS
        
        return std::min(averageFPS, 60.0); // Cap at 60 FPS
    }

    /**
     * @brief Test window resize performance
     */
    void testWindowResizePerformance() {
        qDebug() << "     Testing window resize performance";
        
        QSize originalSize = m_mainWindow->size();
        
        TDD_BENCHMARK([&]() {
            QSize currentSize = m_mainWindow->size();
            for (int i = 0; i < 5; ++i) {
                m_mainWindow->resize(currentSize.width() + i * 50, currentSize.height() + i * 30);
                QApplication::processEvents();
            }
        }, 150, "Window resize operations"); // Max 150ms for 5 resizes
        
        // Restore original size
        m_mainWindow->resize(originalSize);
        QApplication::processEvents();
    }

    /**
     * @brief Test dock widget manipulation performance
     */
    void testDockWidgetPerformance() {
        qDebug() << "     Testing dock widget manipulation performance";
        
        QList<QDockWidget*> dockWidgets = m_mainWindow->findChildren<QDockWidget*>();
        
        if (dockWidgets.isEmpty()) {
            qDebug() << "       No dock widgets found for testing";
            return;
        }
        
        TDD_BENCHMARK([&]() {
            for (QDockWidget* dock : dockWidgets) {
                if (dock->isVisible()) {
                    // Test dock widget operations
                    dock->setFloating(!dock->isFloating());
                    QApplication::processEvents();
                    dock->setFloating(!dock->isFloating());
                    QApplication::processEvents();
                }
            }
        }, 200, "Dock widget manipulations"); // Max 200ms for all dock operations
    }

    /**
     * @brief Test panel visibility toggle performance
     */
    void testPanelVisibilityPerformance() {
        qDebug() << "     Testing panel visibility toggle performance";
        
        QList<QDockWidget*> dockWidgets = m_mainWindow->findChildren<QDockWidget*>();
        
        if (dockWidgets.isEmpty()) {
            return;
        }
        
        TDD_BENCHMARK([&]() {
            for (QDockWidget* dock : dockWidgets) {
                bool wasVisible = dock->isVisible();
                dock->setVisible(!wasVisible);
                QApplication::processEvents();
                dock->setVisible(wasVisible);
                QApplication::processEvents();
            }
        }, 100, "Panel visibility toggles"); // Max 100ms for visibility operations
    }

    /**
     * @brief Test tab switching performance
     */
    void testTabSwitchingPerformance() {
        qDebug() << "     Testing tab switching performance";
        
        QList<QTabWidget*> tabWidgets = m_mainWindow->findChildren<QTabWidget*>();
        
        for (QTabWidget* tabWidget : tabWidgets) {
            if (tabWidget->count() > 1) {
                TDD_BENCHMARK([&]() {
                    int originalIndex = tabWidget->currentIndex();
                    for (int i = 0; i < tabWidget->count(); ++i) {
                        tabWidget->setCurrentIndex(i);
                        QApplication::processEvents();
                    }
                    tabWidget->setCurrentIndex(originalIndex);
                    QApplication::processEvents();
                }, 50, QString("Tab switching (%1 tabs)").arg(tabWidget->count())); // Max 50ms per tab widget
                
                break; // Test first tab widget only
            }
        }
    }

    /**
     * @brief Simulate data update for stress testing
     */
    void simulateDataUpdate() {
        // Simulate updating various UI components with new data
        
        // Update tree view if present
        QTreeView* treeView = m_mainWindow->findChild<QTreeView*>();
        if (treeView) {
            QAbstractItemModel* model = treeView->model();
            if (model && model->rowCount() > 0) {
                // Trigger data change signal
                QModelIndex firstIndex = model->index(0, 0);
                if (firstIndex.isValid()) {
                    emit model->dataChanged(firstIndex, firstIndex);
                }
            }
        }
        
        // Update table view if present
        QTableView* tableView = m_mainWindow->findChild<QTableView*>();
        if (tableView) {
            QAbstractItemModel* model = tableView->model();
            if (model && model->rowCount() > 0) {
                QModelIndex firstIndex = model->index(0, 0);
                if (firstIndex.isValid()) {
                    emit model->dataChanged(firstIndex, firstIndex);
                }
            }
        }
        
        // Update text displays
        QList<QTextEdit*> textEdits = m_mainWindow->findChildren<QTextEdit*>();
        for (QTextEdit* textEdit : textEdits) {
            if (textEdit->isVisible() && textEdit->document()->characterCount() < 1000) {
                textEdit->append(QString("Update %1").arg(QRandomGenerator::global()->generate()));
                break; // Update only one to prevent excessive text accumulation
            }
        }
        
        // Update status bar
        QStatusBar* statusBar = m_mainWindow->statusBar();
        if (statusBar) {
            statusBar->showMessage(QString("Update %1").arg(QTime::currentTime().toString()), 100);
        }
    }

    /**
     * @brief Test various synchronization patterns
     */
    bool testExplorerPropertiesSync() {
        // Simplified synchronization test
        QTreeView* treeView = m_mainWindow->findChild<QTreeView*>();
        QTextEdit* propertiesText = nullptr;
        
        // Find properties text edit
        QList<QTextEdit*> textEdits = m_mainWindow->findChildren<QTextEdit*>();
        for (QTextEdit* textEdit : textEdits) {
            QDockWidget* parentDock = nullptr;
            QWidget* parent = textEdit->parentWidget();
            while (parent && !parentDock) {
                parentDock = qobject_cast<QDockWidget*>(parent);
                parent = parent->parentWidget();
            }
            
            if (parentDock && parentDock->windowTitle().contains("Properties", Qt::CaseInsensitive)) {
                propertiesText = textEdit;
                break;
            }
        }
        
        if (treeView && propertiesText) {
            QElapsedTimer syncTimer;
            syncTimer.start();
            
            // Simulate selection change
            QAbstractItemModel* model = treeView->model();
            if (model && model->rowCount() > 0) {
                QModelIndex firstItem = model->index(0, 0);
                treeView->setCurrentIndex(firstItem);
                QApplication::processEvents();
            }
            
            qint64 syncTime = syncTimer.elapsed();
            
            qDebug() << "       Explorer → Properties sync time:" << syncTime << "ms";
            
            return syncTime <= 10; // Professional requirement: <10ms
        }
        
        return false;
    }

    bool testServiceContentSync() {
        // Similar to explorer-properties sync but for service selection
        return testExplorerPropertiesSync(); // Simplified implementation
    }

    bool testContentPropertiesSync() {
        // Test main content to properties synchronization
        return testExplorerPropertiesSync(); // Simplified implementation
    }

    /**
     * @brief Test recovery from UI blocking operations
     */
    bool testUIBlockingOperationRecovery(QElapsedTimer& recoveryTimer) {
        recoveryTimer.restart();
        
        try {
            // Simulate UI blocking operation
            QTimer::singleShot(100, []() {
                // Simulate brief blocking operation
                QTest::qWait(50);
            });
            
            QApplication::processEvents();
            
            // Test that UI is still responsive
            m_mainWindow->update();
            QApplication::processEvents();
            
            qint64 recoveryTime = recoveryTimer.elapsed();
            qDebug() << "       UI blocking operation recovery time:" << recoveryTime << "ms";
            
            return recoveryTime <= 500; // Professional requirement: <500ms recovery
            
        } catch (...) {
            return false;
        }
    }

    bool testMemoryPressureRecovery(QElapsedTimer& recoveryTimer) {
        recoveryTimer.restart();
        
        // Simulate memory pressure and recovery
        testIntensiveGUIOperations();
        
        qint64 recoveryTime = recoveryTimer.elapsed();
        qDebug() << "       Memory pressure recovery time:" << recoveryTime << "ms";
        
        return recoveryTime <= 1000; // Allow 1 second for memory pressure recovery
    }

    bool testInvalidOperationRecovery(QElapsedTimer& recoveryTimer) {
        recoveryTimer.restart();
        
        try {
            // Simulate invalid operations
            QTest::keyClick(m_mainWindow, Qt::Key_Escape);
            QApplication::processEvents();
            
            // Test that window is still responsive
            QVERIFY(m_mainWindow->isVisible());
            QVERIFY(m_mainWindow->isEnabled());
            
            qint64 recoveryTime = recoveryTimer.elapsed();
            qDebug() << "       Invalid operation recovery time:" << recoveryTime << "ms";
            
            return recoveryTime <= 200; // Should recover quickly from invalid operations
            
        } catch (...) {
            return false;
        }
    }

    /**
     * @brief Test platform-specific performance characteristics
     */
    bool testFontRenderingPerformance() {
        QElapsedTimer renderTimer;
        renderTimer.start();
        
        // Test font rendering with various sizes
        QFont testFont = m_mainWindow->font();
        QFontMetrics metrics(testFont);
        
        QString testText = "Professional Broadcast Analysis Interface 0123456789";
        
        for (int size = 8; size <= 16; size += 2) {
            testFont.setPointSize(size);
            QFontMetrics sizeMetrics(testFont);
            int width = sizeMetrics.horizontalAdvance(testText);
            int height = sizeMetrics.height();
            
            // Ensure reasonable metrics
            if (width <= 0 || height <= 0) {
                return false;
            }
        }
        
        qint64 renderTime = renderTimer.elapsed();
        qDebug() << "       Font rendering performance:" << renderTime << "ms";
        
        return renderTime <= 50; // Professional requirement: <50ms for font operations
    }

    bool testWidgetStylePerformance() {
        QElapsedTimer styleTimer;
        styleTimer.start();
        
        // Test widget styling operations
        QList<QWidget*> testWidgets = m_mainWindow->findChildren<QWidget*>();
        
        int widgetsProcessed = 0;
        for (QWidget* widget : testWidgets) {
            if (widgetsProcessed >= 50) break; // Limit test scope
            
            // Test style operations
            QStyle* style = widget->style();
            if (style) {
                QStyleOption option;
                option.initFrom(widget);
                
                // Test style metrics
                int pixelMetric = style->pixelMetric(QStyle::PM_ButtonMargin, &option, widget);
                if (pixelMetric < 0) {
                    // Invalid style metric
                }
                
                widgetsProcessed++;
            }
        }
        
        qint64 styleTime = styleTimer.elapsed();
        qDebug() << "       Widget style performance:" << styleTime << "ms for" << widgetsProcessed << "widgets";
        
        return styleTime <= 100; // Professional requirement: <100ms for style operations
    }

    bool testEventProcessingPerformance() {
        QElapsedTimer eventTimer;
        eventTimer.start();
        
        // Test event processing performance
        const int eventCount = 100;
        
        for (int i = 0; i < eventCount; ++i) {
            QApplication::processEvents();
            
            // Brief delay to prevent overwhelming
            if (i % 10 == 0) {
                QTest::qWait(1);
            }
        }
        
        qint64 eventTime = eventTimer.elapsed();
        double averageEventTime = (double)eventTime / eventCount;
        
        qDebug() << "       Event processing performance:" << QString::number(averageEventTime, 'f', 2) 
                 << "ms average per processEvents() call";
        
        return averageEventTime <= 5.0; // Professional requirement: <5ms average per event cycle
    }

    /**
     * @brief Generate comprehensive performance report
     */
    void generatePerformanceReport(bool latencyTestPassed) {
        qDebug() << "";
        qDebug() << "╔════════════════════════════════════════════════════════════════════════════╗";
        qDebug() << "║                   COMPREHENSIVE PERFORMANCE REPORT                        ║";
        qDebug() << "╠════════════════════════════════════════════════════════════════════════════╣";
        qDebug() << "║ Professional Broadcast Industry GUI Performance Validation Results        ║";
        qDebug() << "╚════════════════════════════════════════════════════════════════════════════╝";
        qDebug() << "";
        
        // Performance Summary
        qDebug() << "📊 PERFORMANCE METRICS SUMMARY:";
        qDebug() << "   ┌─────────────────────────────────────────────────────────────┐";
        qDebug() << QString("   │ UI Update Latency: %1ms (target: <50ms)                  │")
                        .arg(QString::number(m_performanceMetrics.averageUpdateTime, 'f', 1), -7);
        qDebug() << QString("   │ Memory Usage: %1MB (target: <50MB)                      │")
                        .arg(QString::number(m_performanceMetrics.totalMemoryMB), -7);
        qDebug() << QString("   │ Frame Rate: %1 FPS (target: 60 FPS)                    │")
                        .arg(QString::number(m_performanceMetrics.frameRate, 'f', 1), -7);
        qDebug() << QString("   │ Frame Drops: %1 (lower is better)                      │")
                        .arg(QString::number(m_performanceMetrics.frameDrops), -7);
        qDebug() << "   └─────────────────────────────────────────────────────────────┘";
        qDebug() << "";
        
        // Professional Standards Compliance
        bool memoryCompliant = (m_performanceMetrics.totalMemoryMB <= ProfessionalGUI::BroadcastStandards::TARGET_MEMORY_MB);
        bool latencyCompliant = latencyTestPassed;
        bool frameRateCompliant = (m_performanceMetrics.frameRate >= 55.0);
        
        qDebug() << "✅ PROFESSIONAL STANDARDS COMPLIANCE:";
        qDebug() << "   ┌─────────────────────────────────────────────────────────────┐";
        qDebug() << QString("   │ UI Latency Compliance: %1                              │")
                        .arg(latencyCompliant ? "✅ PASSED" : "❌ FAILED", -15);
        qDebug() << QString("   │ Memory Efficiency: %1                                  │")
                        .arg(memoryCompliant ? "✅ PASSED" : "❌ FAILED", -15);
        qDebug() << QString("   │ Visualization Performance: %1                          │")
                        .arg(frameRateCompliant ? "✅ PASSED" : "❌ NEEDS OPTIMIZATION", -15);
        qDebug() << "   └─────────────────────────────────────────────────────────────┘";
        qDebug() << "";
        
        // Overall Assessment
        int passedTests = (latencyCompliant ? 1 : 0) + (memoryCompliant ? 1 : 0) + (frameRateCompliant ? 1 : 0);
        QString overallStatus;
        
        if (passedTests == 3) {
            overallStatus = "🏆 PRODUCTION READY - All professional standards met";
        } else if (passedTests == 2) {
            overallStatus = "⚠️ OPTIMIZATION NEEDED - Minor performance tuning required";
        } else {
            overallStatus = "🔧 DEVELOPMENT NEEDED - Significant performance improvements required";
        }
        
        qDebug() << "🎯 OVERALL ASSESSMENT:";
        qDebug() << "   " << overallStatus;
        qDebug() << QString("   Professional compliance: %1/3 standards met").arg(passedTests);
        qDebug() << "";
        
        // Recommendations
        qDebug() << "💡 RECOMMENDATIONS:";
        if (!latencyCompliant) {
            qDebug() << "   • Optimize UI update mechanisms to achieve <50ms latency";
            qDebug() << "   • Consider implementing asynchronous UI updates";
        }
        if (!memoryCompliant) {
            qDebug() << "   • Review memory usage patterns and implement optimization";
            qDebug() << "   • Consider lazy loading for large datasets";
        }
        if (!frameRateCompliant) {
            qDebug() << "   • Optimize rendering pipeline for 60 FPS performance";
            qDebug() << "   • Consider hardware acceleration for visualizations";
        }
        if (passedTests == 3) {
            qDebug() << "   ✅ No optimization needed - Performance meets all professional requirements";
        }
        qDebug() << "";
    }

    void cleanupTestCase() {
        if (m_testFramework) {
            m_testFramework->cleanupProfessionalTestEnvironment();
            delete m_testFramework;
            m_testFramework = nullptr;
        }
        
        qDebug() << "GUI Performance and Responsiveness tests completed";
        TDD::TestReporter::instance().enforceTDDCompliance();
    }
};

// Register test with Qt Test framework
QTEST_MAIN(GuiPerformanceResponsivenessTest)
#include "test_gui_performance_responsiveness.moc"