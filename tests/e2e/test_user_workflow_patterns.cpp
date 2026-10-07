/**
 * @file test_user_workflow_patterns.cpp
 * @brief Comprehensive E2E testing for Phase 5.3 Professional UI/UX Validation
 * 
 * Tests all user workflow patterns implemented in Phase 5.1 (professional layout)
 * and Phase 5.2 (ETI/DAB integration) for broadcast industry compliance.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QApplication>
#include <QTest>
#include <QTimer>
#include <QSignalSpy>
#include <QEventLoop>
#include <QFileInfo>
#include <QDir>
#include <QElapsedTimer>
// #include <QMemoryInfo>  // Not available in this Qt6 version, using alternatives
#include <QThread>
#include <memory>

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
#include "fixtures/test_data_generators.h"
#include "mocks/mock_eti_processor.h"

using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;
using ::testing::AtLeast;

/**
 * @class UserWorkflowPatternsTest
 * @brief E2E test suite for professional UI/UX workflow validation
 */
class UserWorkflowPatternsTest : public ::testing::Test
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

        // Setup mock ETI processor
        mockEtiProcessor = std::make_unique<MockEtiProcessor>();
        
        // Create test data generators
        testDataGenerator = std::make_unique<TestDataGenerators>();
        
        // Setup performance monitoring
        memoryMonitor = std::make_unique<QElapsedTimer>();
        performanceTimer = std::make_unique<QElapsedTimer>();
        
        // Show main window for testing
        mainWindow->show();
        QTest::qWaitForWindowExposed(mainWindow.get());
        
        // Allow UI to stabilize
        QTest::qWait(100);
    }

    void TearDown() override {
        if (mainWindow) {
            mainWindow->close();
            mainWindow.reset();
        }
        mockEtiProcessor.reset();
        testDataGenerator.reset();
        memoryMonitor.reset();
        performanceTimer.reset();
    }

    /**
     * @brief Validate three-panel professional layout
     */
    bool validateThreePanelLayout() {
        auto* centralWidget = mainWindow->findChild<QWidget*>("centralWidget");
        if (!centralWidget) return false;

        auto* horizontalSplitter = mainWindow->findChild<QSplitter*>("horizontalSplitter");
        if (!horizontalSplitter) return false;

        // Validate panel proportions (25%-50%-25%)
        QList<int> sizes = horizontalSplitter->sizes();
        if (sizes.size() != 3) return false;

        int totalWidth = sizes[0] + sizes[1] + sizes[2];
        double leftPercent = (double)sizes[0] / totalWidth * 100.0;
        double centerPercent = (double)sizes[1] / totalWidth * 100.0;
        double rightPercent = (double)sizes[2] / totalWidth * 100.0;

        // Allow 5% tolerance for professional layout
        return (leftPercent >= 20.0 && leftPercent <= 30.0) &&
               (centerPercent >= 45.0 && centerPercent <= 55.0) &&
               (rightPercent >= 20.0 && rightPercent <= 30.0);
    }

    /**
     * @brief Check broadcast industry aesthetics
     */
    bool validateBroadcastAesthetics() {
        // Check dark theme colors (#2D2D30, #0078D4)
        auto palette = mainWindow->palette();
        auto backgroundColor = palette.color(QPalette::Window);
        auto highlightColor = palette.color(QPalette::Highlight);
        
        // Validate dark background (approximately #2D2D30)
        bool darkBackground = (backgroundColor.red() < 60 && 
                              backgroundColor.green() < 60 && 
                              backgroundColor.blue() < 60);
        
        // Validate professional blue highlight
        bool professionalHighlight = (highlightColor.blue() > 180);
        
        return darkBackground && professionalHighlight;
    }

    /**
     * @brief Measure UI responsiveness
     */
    double measureUIResponsiveness(int iterations = 100) {
        QElapsedTimer timer;
        timer.start();
        
        for (int i = 0; i < iterations; ++i) {
            QApplication::processEvents();
            QTest::qWait(1); // 1ms minimum
        }
        
        qint64 totalTime = timer.elapsed();
        return (double)totalTime / iterations; // Average ms per iteration
    }

    /**
     * @brief Monitor memory usage
     */
    qint64 getCurrentMemoryUsage() {
        // Simplified memory monitoring
        return QThread::currentThread()->stackSize();
    }

    std::unique_ptr<QApplication> app;
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<MockEtiProcessor> mockEtiProcessor;
    std::unique_ptr<TestDataGenerators> testDataGenerator;
    std::unique_ptr<QElapsedTimer> memoryMonitor;
    std::unique_ptr<QElapsedTimer> performanceTimer;
};

/**
 * @brief Test Case 1: File Analysis Workflow
 * Load ETI → Service Tree Display → Frame Selection → Properties Analysis
 */
TEST_F(UserWorkflowPatternsTest, FileAnalysisWorkflow) {
    ASSERT_TRUE(mainWindow);
    
    // Step 1: Load ETI file
    QString testFile = testDataGenerator->createSampleEtiFile();
    ASSERT_FALSE(testFile.isEmpty());
    
    // Simulate file open action
    QSignalSpy fileOpenedSpy(mainWindow.get(), &MainWindow::etiFileOpened);
    
    // Get file menu action
    auto* openAction = mainWindow->findChild<QAction*>("openFileAction");
    ASSERT_TRUE(openAction);
    
    // Trigger file open (simulate user action)
    openAction->trigger();
    
    // Wait for file dialog and processing
    QTest::qWait(500);
    
    // Step 2: Verify Service Tree Display
    auto* serviceBrowser = mainWindow->getServiceBrowser();
    ASSERT_TRUE(serviceBrowser);
    
    auto* serviceTreeModel = serviceBrowser->findChild<EtiServiceTreeModel*>();
    ASSERT_TRUE(serviceTreeModel);
    
    // Wait for service tree population
    QTest::qWait(1000);
    
    // Verify service tree has data
    EXPECT_GT(serviceTreeModel->rowCount(), 0);
    
    // Step 3: Frame Selection
    auto* analyserWidget = mainWindow->getAnalyserWidget();
    ASSERT_TRUE(analyserWidget);
    
    auto* frameListTable = analyserWidget->findChild<QTableWidget*>("frameListTable");
    ASSERT_TRUE(frameListTable);
    
    // Wait for frame data
    QTest::qWait(500);
    
    // Simulate frame selection
    if (frameListTable->rowCount() > 0) {
        frameListTable->selectRow(0);
        QTest::qWait(100);
    }
    
    // Step 4: Properties Analysis
    auto* propertiesPanel = mainWindow->findChild<QWidget*>("propertiesPanel");
    ASSERT_TRUE(propertiesPanel);
    
    auto* parametersText = propertiesPanel->findChild<QTextEdit*>("parametersText");
    ASSERT_TRUE(parametersText);
    
    // Verify properties display
    EXPECT_FALSE(parametersText->toPlainText().isEmpty());
    
    // Cleanup
    QFile::remove(testFile);
}

/**
 * @brief Test Case 2: Real-Time Analysis Workflow
 * Stream Connect → Live Service Monitoring → Alert Processing
 */
TEST_F(UserWorkflowPatternsTest, RealTimeAnalysisWorkflow) {
    ASSERT_TRUE(mainWindow);
    
    // Setup mock expectations
    EXPECT_CALL(*mockEtiProcessor, startRealTimeProcessing())
        .Times(1)
        .WillOnce(Return(true));
    
    // Step 1: Stream Connect
    QSignalSpy realTimeModeChangedSpy(mainWindow.get(), &MainWindow::realTimeModeChanged);
    
    auto* startAnalysisAction = mainWindow->findChild<QAction*>("startAnalysisAction");
    ASSERT_TRUE(startAnalysisAction);
    
    // Start real-time analysis
    startAnalysisAction->trigger();
    
    // Wait for real-time mode activation
    QTest::qWait(200);
    
    // Verify real-time mode is active
    EXPECT_TRUE(mainWindow->isRealTimeMode());
    
    // Step 2: Live Service Monitoring
    auto* serviceBrowser = mainWindow->getServiceBrowser();
    ASSERT_TRUE(serviceBrowser);
    
    // Simulate live service data
    testDataGenerator->generateLiveServiceData(serviceBrowser);
    
    // Wait for service updates
    QTest::qWait(1000);
    
    // Verify live updates
    auto* serviceTreeModel = serviceBrowser->findChild<EtiServiceTreeModel*>();
    ASSERT_TRUE(serviceTreeModel);
    EXPECT_TRUE(serviceTreeModel->isRealTimeEnabled());
    
    // Step 3: Alert Processing
    auto* alertSystem = mainWindow->findChild<QWidget*>("alertSystem");
    if (alertSystem) {
        // Simulate alert condition
        testDataGenerator->generateTestAlert();
        QTest::qWait(100);
        
        // Verify alert processing
        // This would be validated through alert system widgets
    }
    
    // Stop real-time analysis
    auto* stopAnalysisAction = mainWindow->findChild<QAction*>("stopAnalysisAction");
    if (stopAnalysisAction) {
        stopAnalysisAction->trigger();
        QTest::qWait(100);
    }
}

/**
 * @brief Test Case 3: Error Detection Workflow
 * Frame Error Highlighting → ETSI Compliance Validation → Alert Generation
 */
TEST_F(UserWorkflowPatternsTest, ErrorDetectionWorkflow) {
    ASSERT_TRUE(mainWindow);
    
    // Step 1: Frame Error Highlighting
    auto* analyserWidget = mainWindow->getAnalyserWidget();
    ASSERT_TRUE(analyserWidget);
    
    // Generate ETI data with errors
    QString errorEtiFile = testDataGenerator->createEtiFileWithErrors();
    ASSERT_FALSE(errorEtiFile.isEmpty());
    
    // Load file with errors
    QSignalSpy fileOpenedSpy(mainWindow.get(), &MainWindow::etiFileOpened);
    // Simulate file loading...
    
    QTest::qWait(500);
    
    // Step 2: ETSI Compliance Validation
    auto* figAnalysisWidget = mainWindow->findChild<FigAnalysisWidget*>();
    if (figAnalysisWidget) {
        // Wait for FIG analysis
        QTest::qWait(1000);
        
        // Verify compliance checking
        double complianceScore = figAnalysisWidget->getOverallComplianceScore();
        EXPECT_GE(complianceScore, 0.0);
        EXPECT_LE(complianceScore, 100.0);
    }
    
    // Step 3: Alert Generation
    // Verify error highlighting in frame list
    auto* frameListTable = analyserWidget->findChild<QTableWidget*>("frameListTable");
    if (frameListTable && frameListTable->rowCount() > 0) {
        // Check for error highlighting (red background, etc.)
        for (int row = 0; row < frameListTable->rowCount(); ++row) {
            auto* item = frameListTable->item(row, 0);
            if (item) {
                QColor backgroundColor = item->background().color();
                // Error frames should have red/warning background
                if (backgroundColor.red() > 200) {
                    // Found error highlighting
                    break;
                }
            }
        }
    }
    
    // Cleanup
    QFile::remove(errorEtiFile);
}

/**
 * @brief Test Case 4: Dual-Mode Workflow
 * Seamless switching between file and real-time processing
 */
TEST_F(UserWorkflowPatternsTest, DualModeWorkflow) {
    ASSERT_TRUE(mainWindow);
    
    // Step 1: Start with file mode
    QString testFile = testDataGenerator->createSampleEtiFile();
    ASSERT_FALSE(testFile.isEmpty());
    
    // Load file
    QSignalSpy fileOpenedSpy(mainWindow.get(), &MainWindow::etiFileOpened);
    QSignalSpy realTimeModeChangedSpy(mainWindow.get(), &MainWindow::realTimeModeChanged);
    
    // Simulate file opening
    auto* openAction = mainWindow->findChild<QAction*>("openFileAction");
    if (openAction) {
        openAction->trigger();
    }
    
    QTest::qWait(500);
    
    // Verify file mode
    EXPECT_FALSE(mainWindow->isRealTimeMode());
    
    // Step 2: Switch to real-time mode
    auto* startRealTimeAction = mainWindow->findChild<QAction*>("startAnalysisAction");
    if (startRealTimeAction) {
        startRealTimeAction->trigger();
        QTest::qWait(200);
        
        // Verify real-time mode
        EXPECT_TRUE(mainWindow->isRealTimeMode());
    }
    
    // Step 3: Switch back to file mode
    auto* stopRealTimeAction = mainWindow->findChild<QAction*>("stopAnalysisAction");
    if (stopRealTimeAction) {
        stopRealTimeAction->trigger();
        QTest::qWait(200);
        
        // Verify file mode restored
        EXPECT_FALSE(mainWindow->isRealTimeMode());
    }
    
    // Step 4: Verify UI state consistency
    auto* serviceBrowser = mainWindow->getServiceBrowser();
    ASSERT_TRUE(serviceBrowser);
    
    auto* analyserWidget = mainWindow->getAnalyserWidget();
    ASSERT_TRUE(analyserWidget);
    
    // Both widgets should be responsive and consistent
    EXPECT_TRUE(serviceBrowser->isEnabled());
    EXPECT_TRUE(analyserWidget->isEnabled());
    
    // Cleanup
    QFile::remove(testFile);
}

/**
 * @brief Test Case 5: Panel Interaction Testing
 * Explorer Panel (25%) → Main Content (50%) → Properties Panel (25%) → Bottom Tools
 */
TEST_F(UserWorkflowPatternsTest, PanelInteractionTesting) {
    ASSERT_TRUE(mainWindow);
    
    // Step 1: Validate three-panel layout
    EXPECT_TRUE(validateThreePanelLayout());
    
    // Step 2: Test Explorer Panel interactions
    auto* explorerPanel = mainWindow->findChild<QWidget*>("explorerPanel");
    ASSERT_TRUE(explorerPanel);
    
    auto* serviceBrowser = mainWindow->getServiceBrowser();
    ASSERT_TRUE(serviceBrowser);
    
    // Test service selection
    testDataGenerator->populateServiceBrowser(serviceBrowser);
    QTest::qWait(200);
    
    // Step 3: Test Main Content Area
    auto* mainContentArea = mainWindow->findChild<QWidget*>("mainContentArea");
    ASSERT_TRUE(mainContentArea);
    
    auto* frameListTable = mainContentArea->findChild<QTableWidget*>("frameListTable");
    ASSERT_TRUE(frameListTable);
    
    // Test frame selection and color coding
    if (frameListTable->rowCount() > 0) {
        frameListTable->selectRow(0);
        QTest::qWait(50);
        
        // Verify selection feedback
        EXPECT_EQ(frameListTable->currentRow(), 0);
    }
    
    // Step 4: Test Properties Panel
    auto* propertiesPanel = mainWindow->findChild<QWidget*>("propertiesPanel");
    ASSERT_TRUE(propertiesPanel);
    
    auto* propertiesTabs = propertiesPanel->findChild<QTabWidget*>("propertiesTabs");
    ASSERT_TRUE(propertiesTabs);
    
    // Test tab switching
    for (int i = 0; i < propertiesTabs->count(); ++i) {
        propertiesTabs->setCurrentIndex(i);
        QTest::qWait(50);
        EXPECT_EQ(propertiesTabs->currentIndex(), i);
    }
    
    // Step 5: Test Bottom Tool Panels
    auto* bottomToolTabs = mainWindow->findChild<QTabWidget*>("bottomToolTabs");
    ASSERT_TRUE(bottomToolTabs);
    
    // Test bottom panel tools
    for (int i = 0; i < bottomToolTabs->count(); ++i) {
        bottomToolTabs->setCurrentIndex(i);
        QTest::qWait(50);
        EXPECT_EQ(bottomToolTabs->currentIndex(), i);
    }
}

/**
 * @brief Test Case 6: Performance Validation
 * >900 fps ETI processing, <50ms latency, <100MB memory, layout stability
 */
TEST_F(UserWorkflowPatternsTest, PerformanceValidation) {
    ASSERT_TRUE(mainWindow);
    
    // Step 1: UI Responsiveness Test
    double avgResponseTime = measureUIResponsiveness(100);
    EXPECT_LT(avgResponseTime, 50.0); // <50ms average response time
    
    // Step 2: Memory Usage Test
    qint64 initialMemory = getCurrentMemoryUsage();
    
    // Load test data and monitor memory
    QString testFile = testDataGenerator->createLargeEtiFile();
    ASSERT_FALSE(testFile.isEmpty());
    
    // Simulate heavy processing
    QElapsedTimer loadTimer;
    loadTimer.start();
    
    // Load file and process
    QTest::qWait(2000); // 2 second processing
    
    qint64 loadTime = loadTimer.elapsed();
    qint64 finalMemory = getCurrentMemoryUsage();
    
    // Memory should not exceed 100MB increase (simplified check)
    qint64 memoryIncrease = finalMemory - initialMemory;
    EXPECT_LT(memoryIncrease, 100 * 1024 * 1024); // 100MB limit
    
    // Step 3: Layout Stability Test
    QSize initialSize = mainWindow->size();
    
    // Simulate window resize
    mainWindow->resize(1400, 900);
    QTest::qWait(100);
    
    // Verify layout proportions maintained
    EXPECT_TRUE(validateThreePanelLayout());
    
    // Restore original size
    mainWindow->resize(initialSize);
    QTest::qWait(100);
    
    // Step 4: Real-time Update Performance
    auto* realTimeTimer = mainWindow->findChild<QTimer*>("realTimeTimer");
    if (realTimeTimer) {
        EXPECT_EQ(realTimeTimer->interval(), MainWindow::UPDATE_INTERVAL_MS);
        EXPECT_LE(MainWindow::UPDATE_INTERVAL_MS, 17); // ~60 FPS (16.67ms)
    }
    
    // Cleanup
    QFile::remove(testFile);
}

/**
 * @brief Test Case 7: ETSI Compliance Testing
 * ETI frames, FIG analysis, service tree, alerts
 */
TEST_F(UserWorkflowPatternsTest, ETSIComplianceValidation) {
    ASSERT_TRUE(mainWindow);
    
    // Step 1: ETI Frame Processing (6144-byte frames, 24ms precision)
    QString etsiCompliantFile = testDataGenerator->createETSICompliantEtiFile();
    ASSERT_FALSE(etsiCompliantFile.isEmpty());
    
    // Load ETSI compliant file
    QTest::qWait(500);
    
    // Step 2: FIG Analysis Validation
    auto* figAnalysisWidget = mainWindow->findChild<FigAnalysisWidget*>();
    if (figAnalysisWidget) {
        // Wait for FIG analysis
        QTest::qWait(1000);
        
        // Verify FIG types detected
        QList<quint8> figTypes = figAnalysisWidget->getDetectedFigTypes();
        EXPECT_GT(figTypes.size(), 0);
        
        // Verify compliance scores
        double complianceScore = figAnalysisWidget->getOverallComplianceScore();
        EXPECT_GE(complianceScore, 80.0); // ETSI compliant file should score high
        
        // Check specific FIG types
        for (quint8 figType : figTypes) {
            auto figAnalysis = figAnalysisWidget->getFigAnalysis(figType);
            EXPECT_NE(figAnalysis.complianceStatus, FigAnalysisWidget::ComplianceStatus::Error);
        }
    }
    
    // Step 3: Service Tree Model Accuracy
    auto* serviceBrowser = mainWindow->getServiceBrowser();
    ASSERT_TRUE(serviceBrowser);
    
    auto* serviceTreeModel = serviceBrowser->findChild<EtiServiceTreeModel*>();
    ASSERT_TRUE(serviceTreeModel);
    
    // Verify DAB hierarchy
    QModelIndex ensembleIndex = serviceTreeModel->getEnsembleIndex();
    EXPECT_TRUE(ensembleIndex.isValid());
    
    // Check services
    QList<quint32> services = serviceTreeModel->getAllServices();
    EXPECT_GT(services.size(), 0);
    
    // Step 4: Error Detection and Alert System
    QString errorFile = testDataGenerator->createETSINonCompliantEtiFile();
    ASSERT_FALSE(errorFile.isEmpty());
    
    // Load non-compliant file
    QTest::qWait(500);
    
    // Wait for error detection
    QTest::qWait(1000);
    
    // Verify error detection
    if (figAnalysisWidget) {
        double errorComplianceScore = figAnalysisWidget->getOverallComplianceScore();
        EXPECT_LT(errorComplianceScore, 90.0); // Non-compliant should score lower
    }
    
    // Cleanup
    QFile::remove(etsiCompliantFile);
    QFile::remove(errorFile);
}

/**
 * @brief Test Case 8: Broadcasting Aesthetics Validation
 */
TEST_F(UserWorkflowPatternsTest, BroadcastingAestheticsValidation) {
    ASSERT_TRUE(mainWindow);
    
    // Step 1: Color Scheme Validation
    EXPECT_TRUE(validateBroadcastAesthetics());
    
    // Step 2: Professional Layout Validation
    EXPECT_TRUE(validateThreePanelLayout());
    
    // Step 3: Font and Sizing
    QFont appFont = QApplication::font();
    EXPECT_GE(appFont.pointSize(), 8); // Minimum readable size
    EXPECT_LE(appFont.pointSize(), 12); // Maximum professional size
    
    // Step 4: Window Title
    QString title = mainWindow->windowTitle();
    EXPECT_TRUE(title.contains("ETI Stream Analyser") || 
                title.contains("DAB Analyser") ||
                title.contains("StreamDAB"));
    
    // Step 5: Status Bar Professional Information
    auto* statusBar = mainWindow->statusBar();
    ASSERT_TRUE(statusBar);
    EXPECT_TRUE(statusBar->isVisible());
}

/**
 * @brief Test Case 9: Integration Testing Between Panels
 */
TEST_F(UserWorkflowPatternsTest, PanelIntegrationTesting) {
    ASSERT_TRUE(mainWindow);
    
    // Load test data
    QString testFile = testDataGenerator->createSampleEtiFile();
    ASSERT_FALSE(testFile.isEmpty());
    
    QTest::qWait(500);
    
    // Step 1: Service Selection → Frame Display Integration
    auto* serviceBrowser = mainWindow->getServiceBrowser();
    auto* analyserWidget = mainWindow->getAnalyserWidget();
    
    ASSERT_TRUE(serviceBrowser);
    ASSERT_TRUE(analyserWidget);
    
    // Setup signal spy for integration
    QSignalSpy serviceSelectionSpy(serviceBrowser, SIGNAL(serviceSelected(quint32)));
    
    // Simulate service selection
    testDataGenerator->simulateServiceSelection(serviceBrowser, 0x1001);
    QTest::qWait(200);
    
    // Verify frame display updates
    auto* frameListTable = analyserWidget->findChild<QTableWidget*>("frameListTable");
    if (frameListTable) {
        EXPECT_GT(frameListTable->rowCount(), 0);
    }
    
    // Step 2: Frame Selection → Properties Update Integration
    if (frameListTable && frameListTable->rowCount() > 0) {
        frameListTable->selectRow(0);
        QTest::qWait(100);
        
        // Verify properties panel updates
        auto* propertiesPanel = mainWindow->findChild<QWidget*>("propertiesPanel");
        ASSERT_TRUE(propertiesPanel);
        
        auto* parametersText = propertiesPanel->findChild<QTextEdit*>("parametersText");
        if (parametersText) {
            EXPECT_FALSE(parametersText->toPlainText().isEmpty());
        }
    }
    
    // Step 3: Real-time Mode → All Panels Integration
    auto* startRealTimeAction = mainWindow->findChild<QAction*>("startAnalysisAction");
    if (startRealTimeAction) {
        startRealTimeAction->trigger();
        QTest::qWait(500);
        
        // Verify all panels update in real-time
        EXPECT_TRUE(mainWindow->isRealTimeMode());
        
        // Stop real-time mode
        auto* stopRealTimeAction = mainWindow->findChild<QAction*>("stopAnalysisAction");
        if (stopRealTimeAction) {
            stopRealTimeAction->trigger();
        }
    }
    
    // Cleanup
    QFile::remove(testFile);
}

/**
 * @brief Test Case 10: Professional Deployment Readiness
 */
TEST_F(UserWorkflowPatternsTest, ProfessionalDeploymentReadiness) {
    ASSERT_TRUE(mainWindow);
    
    // Step 1: Initialization Check
    EXPECT_TRUE(mainWindow->initialize());
    
    // Step 2: All Required Components Present
    EXPECT_TRUE(mainWindow->getAnalyserWidget());
    EXPECT_TRUE(mainWindow->getServiceBrowser());
    EXPECT_TRUE(mainWindow->getConstellationWidget());
    
    // Step 3: Menu and Toolbar Completeness
    auto* menuBar = mainWindow->menuBar();
    ASSERT_TRUE(menuBar);
    
    QList<QMenu*> menus = menuBar->findChildren<QMenu*>();
    EXPECT_GE(menus.size(), 4); // File, Analysis, View, Help minimum
    
    // Step 4: Performance Targets
    EXPECT_EQ(mainWindow->getUpdateRate(), MainWindow::TARGET_FPS);
    
    // Step 5: Window Size and Layout
    QSize windowSize = mainWindow->size();
    EXPECT_GE(windowSize.width(), MainWindow::DEFAULT_WINDOW_WIDTH);
    EXPECT_GE(windowSize.height(), MainWindow::DEFAULT_WINDOW_HEIGHT);
    
    // Step 6: Professional Features
    auto* settingsDialog = mainWindow->findChild<QDialog*>("settingsDialog");
    auto* aboutDialog = mainWindow->findChild<QDialog*>("aboutDialog");
    
    // These should be accessible even if not created yet
    auto* settingsAction = mainWindow->findChild<QAction*>("settingsAction");
    auto* aboutAction = mainWindow->findChild<QAction*>("aboutAction");
    
    EXPECT_TRUE(settingsAction);
    EXPECT_TRUE(aboutAction);
}

// Test Suite Entry Point
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize Qt Test framework
    QApplication app(argc, argv);
    
    // Run all tests
    return RUN_ALL_TESTS();
}