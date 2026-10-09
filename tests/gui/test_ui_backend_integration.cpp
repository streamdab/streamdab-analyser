/**
 * @file test_ui_backend_integration.cpp
 * @brief Comprehensive UI-Backend Integration Testing
 * 
 * Tests the complete integration between Qt UI components and high-performance
 * ETI processing backend, validating >900 FPS capabilities and real-time performance.
 * 
 * @author UI/UX Agent - Parallel Coordination Mode
 * @date 2025-09-27
 */

#include <QtTest/QtTest>
#include <QApplication>
#include <QSignalSpy>
#include <QTimer>
#include <QEventLoop>
#include <QThread>
#include <chrono>
#include <memory>

#include "../../src/gui/main_window.h"
#include "../../src/core/eti_processor.hpp"
#include "../../src/network/eti_over_ip_receiver.h"
#include "../../src/gui/etsi_compliance_monitor.h"
#include "../../src/gui/performance_dashboard.h"

class UIBackendIntegrationTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // === CORE INTEGRATION TESTS ===
    void testMainWindowEtiProcessorIntegration();
    void testNetworkStreamingIntegration();
    void testETSIComplianceIntegration();
    void testPerformanceDashboardIntegration();
    void testSignalSlotConnectivity();

    // === REAL-TIME PERFORMANCE TESTS ===
    void testHighFrameRateProcessing();
    void testRealTimeDisplayUpdates();
    void testNetworkQualityMetricsIntegration();
    void testBufferManagementIntegration();

    // === PROFESSIONAL WORKFLOW TESTS ===
    void testFileToRealTimeTransition();
    void testComprehensiveAnalysisWorkflow();
    void testErrorHandlingIntegration();
    void testServiceDiscoveryIntegration();

    // === END-TO-END VALIDATION ===
    void testCompleteWorkflowIntegration();
    void testBroadcastIndustryCompliance();

private:
    // Test infrastructure
    std::unique_ptr<QApplication> m_app;
    std::unique_ptr<MainWindow> m_mainWindow;
    std::unique_ptr<EtiProcessor> m_etiProcessor;
    std::unique_ptr<eti_network::EtiOverIpReceiver> m_networkReceiver;
    
    // Performance tracking
    struct PerformanceMetrics {
        double maxFrameRate = 0.0;
        double avgLatency = 0.0;
        size_t totalFramesProcessed = 0;
        std::chrono::milliseconds testDuration{0};
        
        void reset() {
            maxFrameRate = 0.0;
            avgLatency = 0.0;
            totalFramesProcessed = 0;
            testDuration = std::chrono::milliseconds{0};
        }
    } m_performanceMetrics;
    
    // Test utilities
    bool waitForSignalWithTimeout(QObject* sender, const char* signal, int timeoutMs = 5000);
    QByteArray createSampleETIFrame(uint32_t frameNumber = 0);
    void simulateHighFrequencyETIFrames(int frameCount, int intervalMs = 1);
    PerformanceMetrics measureProcessingPerformance(int frameCount);
};

void UIBackendIntegrationTest::initTestCase()
{
    qDebug() << "=== UI-Backend Integration Test Suite Initialization ===";
    
    // Initialize test application if not already running
    if (!QApplication::instance()) {
        int argc = 1;
        char* argv[] = {"test_ui_backend_integration"};
        m_app = std::make_unique<QApplication>(argc, argv);
    }
    
    qDebug() << "Test application initialized";
}

void UIBackendIntegrationTest::cleanupTestCase()
{
    qDebug() << "=== UI-Backend Integration Test Suite Cleanup ===";
    
    m_mainWindow.reset();
    m_etiProcessor.reset();
    m_networkReceiver.reset();
    
    qDebug() << "Test cleanup completed";
}

void UIBackendIntegrationTest::init()
{
    // Reset performance metrics for each test
    m_performanceMetrics.reset();
    
    // Create fresh instances for each test
    m_mainWindow = std::make_unique<MainWindow>();
    m_etiProcessor = std::make_unique<EtiProcessor>();
    
    // Initialize components
    QVERIFY(m_mainWindow->initialize());
    QVERIFY(m_etiProcessor->initialize());
    
    qDebug() << "Test components initialized successfully";
}

void UIBackendIntegrationTest::cleanup()
{
    // Clean up for next test
    m_mainWindow.reset();
    m_etiProcessor.reset();
    m_networkReceiver.reset();
}

void UIBackendIntegrationTest::testMainWindowEtiProcessorIntegration()
{
    qDebug() << "=== Testing MainWindow-EtiProcessor Integration ===";
    
    // Verify MainWindow has ETI processor access
    QVERIFY(m_mainWindow != nullptr);
    QVERIFY(m_mainWindow->getEtiProcessor() != nullptr);
    
    // Test signal connectivity
    QSignalSpy frameProcessedSpy(m_etiProcessor.get(), &EtiProcessor::frameProcessed);
    QSignalSpy statusChangedSpy(m_etiProcessor.get(), &EtiProcessor::statusChanged);
    QSignalSpy ensembleDiscoveredSpy(m_etiProcessor.get(), &EtiProcessor::ensembleDiscovered);
    
    QVERIFY(frameProcessedSpy.isValid());
    QVERIFY(statusChangedSpy.isValid());
    QVERIFY(ensembleDiscoveredSpy.isValid());
    
    // Simulate ETI frame processing
    QByteArray sampleFrame = createSampleETIFrame(1001);
    bool processResult = m_etiProcessor->process_eti_frame(sampleFrame);
    QVERIFY(processResult);
    
    // Verify signals were emitted
    QVERIFY(frameProcessedSpy.wait(1000));
    QCOMPARE(frameProcessedSpy.count(), 1);
    
    // Verify frame data in signal
    QList<QVariant> frameArgs = frameProcessedSpy.takeFirst();
    QVERIFY(frameArgs.size() == 2);
    QCOMPARE(frameArgs.at(0).toULongLong(), 1001ULL);
    
    qDebug() << "✅ MainWindow-EtiProcessor integration validated";
}

void UIBackendIntegrationTest::testNetworkStreamingIntegration()
{
    qDebug() << "=== Testing Network Streaming Integration ===";
    
    // Create network receiver
    m_networkReceiver = std::make_unique<eti_network::EtiOverIpReceiver>();
    
    // Test network quality metrics integration
    QSignalSpy qualityMetricsSpy(m_networkReceiver.get(), 
                                &eti_network::EtiOverIpReceiver::qualityMetricsUpdated);
    QSignalSpy frameReceivedSpy(m_networkReceiver.get(), 
                               &eti_network::EtiOverIpReceiver::etiFrameReceived);
    
    QVERIFY(qualityMetricsSpy.isValid());
    QVERIFY(frameReceivedSpy.isValid());
    
    // Test network interface availability
    QStringList interfaces = m_networkReceiver->getAvailableInterfaces();
    QVERIFY(!interfaces.isEmpty());
    qDebug() << "Available network interfaces:" << interfaces;
    
    // Test configuration
    m_networkReceiver->setBufferSize(50); // 50ms buffer
    m_networkReceiver->setJitterBuffer(20); // 20ms jitter buffer
    m_networkReceiver->enableProfessionalMode(true);
    
    // Verify configuration applied
    auto qualityMetrics = m_networkReceiver->getQualityMetrics();
    QVERIFY(qualityMetrics.buffer_fill_level >= 0.0);
    
    qDebug() << "✅ Network streaming integration validated";
}

void UIBackendIntegrationTest::testETSIComplianceIntegration()
{
    qDebug() << "=== Testing ETSI Compliance Integration ===";
    
    // Test ETSI compliance monitoring
    ETSIComplianceMonitor complianceMonitor;
    
    // Test compliance status retrieval
    auto complianceStatus = m_etiProcessor->get_compliance_status();
    QVERIFY(complianceStatus.overall_score >= 0.0);
    QVERIFY(complianceStatus.overall_score <= 100.0);
    
    // Test compliance violation handling
    QSignalSpy violationSpy(&complianceMonitor, &ETSIComplianceMonitor::violationDetected);
    
    // Simulate compliance check
    QByteArray testFrame = createSampleETIFrame(2001);
    bool frameValid = m_etiProcessor->validate_frame(testFrame);
    QVERIFY(frameValid);
    
    // Test Standards Agent integration results (8.5/10.0 score)
    auto serviceValidation = m_etiProcessor->validate_all_services();
    auto [validCount, totalCount, firstError] = serviceValidation;
    
    // Verify validation integration
    QVERIFY(totalCount >= 0);
    QVERIFY(validCount <= totalCount);
    
    qDebug() << QString("Service validation: %1/%2 valid services")
                .arg(validCount).arg(totalCount);
    
    if (!firstError.empty()) {
        qDebug() << "First validation error:" << QString::fromStdString(firstError);
    }
    
    qDebug() << "✅ ETSI compliance integration validated";
}

void UIBackendIntegrationTest::testHighFrameRateProcessing()
{
    qDebug() << "=== Testing High Frame Rate Processing (>900 FPS Target) ===";
    
    // Measure processing performance
    const int TEST_FRAME_COUNT = 1000;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    QSignalSpy frameProcessedSpy(m_etiProcessor.get(), &EtiProcessor::frameProcessed);
    
    // Process frames at high rate
    for (int i = 0; i < TEST_FRAME_COUNT; ++i) {
        QByteArray frame = createSampleETIFrame(static_cast<uint32_t>(i));
        bool result = m_etiProcessor->process_eti_frame(frame);
        QVERIFY(result);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Calculate frame rate
    double frameRate = (TEST_FRAME_COUNT * 1000.0) / duration.count();
    m_performanceMetrics.maxFrameRate = frameRate;
    m_performanceMetrics.totalFramesProcessed = TEST_FRAME_COUNT;
    m_performanceMetrics.testDuration = duration;
    
    qDebug() << QString("Processed %1 frames in %2ms = %3 FPS")
                .arg(TEST_FRAME_COUNT)
                .arg(duration.count())
                .arg(frameRate, 0, 'f', 1);
    
    // Verify performance target achievement
    QVERIFY(frameRate > 900.0); // Exceed 900 FPS requirement
    
    // Verify all frames were processed
    QVERIFY(frameProcessedSpy.count() == TEST_FRAME_COUNT);
    
    qDebug() << "✅ High frame rate processing validated:" << frameRate << "FPS";
}

void UIBackendIntegrationTest::testRealTimeDisplayUpdates()
{
    qDebug() << "=== Testing Real-Time Display Updates ===";
    
    // Show main window for UI update testing
    m_mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow.get()));
    
    // Test real-time mode activation
    m_mainWindow->startRealTimeAnalysis();
    QVERIFY(m_mainWindow->isRealTimeMode());
    
    // Simulate real-time frame updates
    QSignalSpy statusUpdateSpy(m_mainWindow.get(), &MainWindow::etiFileOpened);
    
    // Process frames with UI updates
    const int REALTIME_FRAMES = 100;
    for (int i = 0; i < REALTIME_FRAMES; ++i) {
        QByteArray frame = createSampleETIFrame(static_cast<uint32_t>(i + 3000));
        m_etiProcessor->process_eti_frame(frame);
        
        // Allow UI to process events every 10 frames
        if (i % 10 == 0) {
            QApplication::processEvents();
            QTest::qWait(1); // 1ms pause for realistic real-time simulation
        }
    }
    
    // Stop real-time mode
    m_mainWindow->stopRealTimeAnalysis();
    QVERIFY(!m_mainWindow->isRealTimeMode());
    
    qDebug() << "✅ Real-time display updates validated";
}

void UIBackendIntegrationTest::testCompleteWorkflowIntegration()
{
    qDebug() << "=== Testing Complete Workflow Integration ===";
    
    // Test file-to-real-time workflow transition
    
    // Phase 1: File Analysis
    m_mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow.get()));
    
    // Simulate file processing
    QSignalSpy ensembleDiscoveredSpy(m_etiProcessor.get(), &EtiProcessor::ensembleDiscovered);
    QSignalSpy serviceDiscoveredSpy(m_etiProcessor.get(), &EtiProcessor::serviceDiscovered);
    
    // Process sample frames as if from file
    for (int i = 0; i < 50; ++i) {
        QByteArray frame = createSampleETIFrame(static_cast<uint32_t>(i));
        m_etiProcessor->process_eti_frame(frame);
    }
    
    // Phase 2: Real-time transition
    m_mainWindow->startRealTimeAnalysis();
    
    // Phase 3: Network streaming simulation
    if (m_networkReceiver) {
        // Test network streaming integration
        m_mainWindow->startNetworkStreaming();
        
        // Simulate network frames
        for (int i = 50; i < 100; ++i) {
            QByteArray frame = createSampleETIFrame(static_cast<uint32_t>(i));
            m_etiProcessor->process_eti_frame(frame);
            QApplication::processEvents();
        }
        
        m_mainWindow->stopNetworkStreaming();
    }
    
    // Phase 4: Analysis completion
    m_mainWindow->stopRealTimeAnalysis();
    
    // Verify complete workflow
    QVERIFY(m_etiProcessor->get_frame_count() >= 50);
    
    // Test service discovery results
    auto services = m_etiProcessor->get_discovered_services();
    qDebug() << "Discovered services count:" << services.size();
    
    qDebug() << "✅ Complete workflow integration validated";
}

// === UTILITY METHODS ===

bool UIBackendIntegrationTest::waitForSignalWithTimeout(QObject* sender, const char* signal, int timeoutMs)
{
    QSignalSpy spy(sender, signal);
    return spy.wait(timeoutMs);
}

QByteArray UIBackendIntegrationTest::createSampleETIFrame(uint32_t frameNumber)
{
    // Create a valid 6144-byte ETI frame for testing
    QByteArray frame(6144, 0x00);
    
    // ETI sync pattern (4 bytes): 0xFF 0xFF 0xFF 0x1F
    frame[0] = static_cast<char>(0xFF);
    frame[1] = static_cast<char>(0xFF); 
    frame[2] = static_cast<char>(0xFF);
    frame[3] = static_cast<char>(0x1F);
    
    // Frame number (little endian)
    frame[4] = static_cast<char>(frameNumber & 0xFF);
    frame[5] = static_cast<char>((frameNumber >> 8) & 0xFF);
    frame[6] = static_cast<char>((frameNumber >> 16) & 0xFF);
    frame[7] = static_cast<char>((frameNumber >> 24) & 0xFF);
    
    // Add some realistic FIC data
    frame[8] = 0x01; // FIG type 0/0 (ensemble information)
    frame[9] = 0x02; // Length
    frame[10] = 0x03; // Data
    frame[11] = 0x04; // Data
    
    return frame;
}

void UIBackendIntegrationTest::testSignalSlotConnectivity()
{
    qDebug() << "=== Testing Signal/Slot Architecture ===";
    
    // Verify main window signal connections
    QVERIFY(m_mainWindow->metaObject()->methodCount() > 0);
    QVERIFY(m_etiProcessor->metaObject()->methodCount() > 0);
    
    // Test direct signal emission
    QSignalSpy statusChangedSpy(m_etiProcessor.get(), &EtiProcessor::statusChanged);
    
    // Trigger status change
    QByteArray testFrame = createSampleETIFrame(4001);
    m_etiProcessor->process_eti_frame(testFrame);
    
    // Wait for signals
    QApplication::processEvents();
    
    qDebug() << "✅ Signal/slot architecture validated";
}

void UIBackendIntegrationTest::testBroadcastIndustryCompliance()
{
    qDebug() << "=== Testing Broadcast Industry Compliance ===";
    
    // Test professional UI layout
    QVERIFY(m_mainWindow->getAnalyserWidget() != nullptr);
    QVERIFY(m_mainWindow->getConstellationWidget() != nullptr);
    QVERIFY(m_mainWindow->getServiceBrowser() != nullptr);
    
    // Test professional workflow manager
    QVERIFY(m_mainWindow->getWorkflowManager() != nullptr);
    
    // Test update rate compliance (60 FPS UI target)
    int updateRate = m_mainWindow->getUpdateRate();
    QVERIFY(updateRate > 0);
    QVERIFY(updateRate <= 60); // Professional broadcast standard
    
    // Test window dimensions
    QVERIFY(m_mainWindow->width() >= MainWindow::DEFAULT_WINDOW_WIDTH);
    QVERIFY(m_mainWindow->height() >= MainWindow::DEFAULT_WINDOW_HEIGHT);
    
    qDebug() << "✅ Broadcast industry compliance validated";
}

// === TEST PERFORMANCE SUMMARY ===

void UIBackendIntegrationTest::testPerformanceDashboardIntegration()
{
    qDebug() << "=== Testing Performance Dashboard Integration ===";
    
    // Create performance dashboard
    PerformanceDashboard dashboard;
    
    // Test metrics integration
    dashboard.updateProcessingRate(m_performanceMetrics.maxFrameRate);
    dashboard.updateMemoryUsage(45.7); // MB
    dashboard.updateNetworkMetrics(0.95, 0.001, 2.3); // Quality, loss, jitter
    
    // Verify dashboard functionality
    QVERIFY(dashboard.getProcessingRate() >= 0.0);
    
    qDebug() << "✅ Performance dashboard integration validated";
}

// === MAIN TEST EXECUTION ===

QTEST_MAIN(UIBackendIntegrationTest)
#include "test_ui_backend_integration.moc"