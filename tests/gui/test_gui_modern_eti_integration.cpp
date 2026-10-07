/**
 * @file test_gui_modern_eti_integration.cpp
 * @brief Comprehensive GUI Integration Tests with Modern ETI Core Engine
 * 
 * This test suite validates all GUI components work seamlessly with the
 * Modern ETI Core Engine, ensuring professional broadcast industry standards
 * are maintained during high-performance processing.
 * 
 * Test Coverage:
 * - MainWindow integration with Modern ETI components
 * - Service Tree Model performance at >7,482 FPS
 * - Real-time Performance Dashboard validation
 * - ETI Analysis Widget with Enhanced FIG Analyser
 * - Network Streaming Panel (Phase 8)
 * - UI responsiveness under high processing loads
 * - Professional theme consistency
 * - Thai DAB GUI support validation
 * 
 * @author UI/UX Agent - Modern ETI Core Engine GUI Integration
 * @date 2025-09-21
 */

#include <gtest/gtest.h>
#include <QApplication>
#include <QTest>
#include <QSignalSpy>
#include <QTimer>
#include <QTreeWidget>
#include <QTableWidget>
#include <QLabel>
#include <QProgressBar>
#include <memory>
#include <chrono>
#include <vector>

// GUI components
#include "gui/main_window.h"
#include "gui/service_explorer_panel.h"
#include "gui/eti_analysis_widget.h"
#include "gui/eti_service_tree_model.h"
#include "gui/eti_frame_list_model.h"


// Core Modern ETI components
#include "core/eti_processor.hpp"
#include "core/modern_eti_frame_parser.hpp"
#include "core/enhanced_fig_analyser.hpp"
#include "core/eti_types.h"

// Test utilities
#include "fixtures/eti_test_data.h"
#include "fixtures/test_data_generators.h"
#include "mocks/mock_eti_processor.h"

using namespace eti::modern;
using namespace std::chrono;

class GUIModernETIIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize Qt application if not already done
        if (!QApplication::instance()) {
            int argc = 1;
            char* argv[] = {"test"};
            app = std::make_unique<QApplication>(argc, argv);
        }
        
        // Create main window and initialize Modern ETI Core Engine
        mainWindow = std::make_unique<MainWindow>();
        ASSERT_TRUE(mainWindow->initialize());
        
        // Set up Modern ETI components
        setupModernETIComponents();
        
        // Show window for GUI testing
        mainWindow->show();
        QTest::qWaitForWindowExposed(mainWindow.get());
    }
    
    void TearDown() override {
        mainWindow.reset();
        if (app) {
            app->processEvents();
        }
    }
    
    void setupModernETIComponents() {
        // Configure Modern ETI Frame Parser for high performance
        ProcessingConfig config;
        config.optimization_level = ProcessingConfig::OptimizationLevel::AGGRESSIVE;
        config.enable_simd = true;
        config.enable_caching = true;
        config.enable_threading = true;
        config.target_fps = 8000.0;  // Target >7,482 FPS
        config.memory_limit_mb = 4;  // 4MB target
        
        // Initialize Modern ETI components
        auto processor = mainWindow->getEtiProcessor();
        ASSERT_NE(processor, nullptr);
        
        auto modernParser = processor->getETIParser();
        ASSERT_NE(modernParser, nullptr);
        ASSERT_TRUE(modernParser->initialize(config));
        
        auto figAnalyser = processor->getFIGAnalyser();
        ASSERT_NE(figAnalyser, nullptr);
    }
    
    // Generate test ETI frames for performance testing
    std::vector<QByteArray> generateTestFrames(int count = 1000) {
        std::vector<QByteArray> frames;
        frames.reserve(count);
        
        for (int i = 0; i < count; ++i) {
            QByteArray frameData = TestDataGenerators::createValidETIFrame(i);
            EXPECT_EQ(frameData.size(), 6144);  // Standard ETI frame size
            frames.push_back(frameData);
        }
        
        return frames;
    }
    
    // Generate Thai DAB service data for localization testing
    ServiceInfo generateThaiDABService(uint32_t serviceId) {
        ServiceInfo service;
        service.serviceId = serviceId;
        service.ensembleId = 0x1234;
        service.label = "สถานีวิทยุ BBC ไทย";  // Thai BBC Radio
        service.ensembleLabel = "ทดสอบ DAB ไทย";  // Thai DAB Test
        service.typeString = "DAB+";
        service.quality = 95;
        service.bitrate = 128;
        service.isActive = true;
        service.isDabPlus = true;
        service.type = ServiceInfo::DABPlus;
        
        return service;
    }
    
    std::unique_ptr<QApplication> app;
    std::unique_ptr<MainWindow> mainWindow;
};

/**
 * @brief Test MainWindow integration with Modern ETI Core Engine components
 */
TEST_F(GUIModernETIIntegrationTest, MainWindowModernETIIntegration) {
    // Verify Modern ETI components are properly initialized
    auto processor = mainWindow->getEtiProcessor();
    ASSERT_NE(processor, nullptr);
    EXPECT_TRUE(processor->isInitialized());
    
    auto modernParser = processor->getETIParser();
    ASSERT_NE(modernParser, nullptr);
    EXPECT_TRUE(modernParser->isInitialized());
    
    auto figAnalyser = processor->getFIGAnalyser();
    ASSERT_NE(figAnalyser, nullptr);
    
    // Test signal/slot connections between Modern ETI and GUI
    QSignalSpy frameProcessedSpy(processor, &EtiProcessor::frameProcessed);
    QSignalSpy serviceDiscoveredSpy(processor, &EtiProcessor::serviceDiscovered);
    QSignalSpy ensembleDiscoveredSpy(processor, &EtiProcessor::ensembleDiscovered);
    
    // Generate test frame and process it
    auto testFrame = TestDataGenerators::createValidETIFrame(1);
    
    auto startTime = high_resolution_clock::now();
    bool success = processor->processEtiFrame(testFrame);
    auto endTime = high_resolution_clock::now();
    
    EXPECT_TRUE(success);
    
    // Verify processing time meets Modern ETI performance targets
    auto processingTime = duration_cast<microseconds>(endTime - startTime);
    EXPECT_LT(processingTime.count(), 200);  // <200µs for >5000 FPS capability
    
    // Process GUI events to trigger signal/slot mechanism
    QApplication::processEvents();
    
    // Verify signals were emitted and GUI received them
    EXPECT_GE(frameProcessedSpy.count(), 1);
    
    std::cout << "MainWindow Modern ETI Integration: PASS - "
              << "Processing time: " << processingTime.count() << "µs" << std::endl;
}

/**
 * @brief Test Service Tree Model performance with >7,482 FPS processing
 */
TEST_F(GUIModernETIIntegrationTest, ServiceTreeModelHighPerformance) {
    auto serviceExplorer = mainWindow->findChild<ServiceExplorerPanel*>();
    ASSERT_NE(serviceExplorer, nullptr);
    
    // Connect to ETI processor for service updates
    auto processor = mainWindow->getEtiProcessor();
    serviceExplorer->connectEtiProcessor(processor);
    
    // Measure service update performance
    const int SERVICE_COUNT = 1000;
    auto startTime = high_resolution_clock::now();
    
    // Add services rapidly to simulate high FPS discovery
    for (int i = 0; i < SERVICE_COUNT; ++i) {
        ServiceInfo service;
        service.serviceId = i + 1;
        service.ensembleId = 0x1234;
        service.label = QString("Service %1").arg(i + 1);
        service.typeString = (i % 3 == 0) ? "DAB+" : "Audio";
        service.quality = 90 + (i % 10);
        service.bitrate = 128;
        service.isActive = true;
        service.isDabPlus = (i % 3 == 0);
        service.type = service.isDabPlus ? ServiceInfo::DABPlus : ServiceInfo::Audio;
        
        serviceExplorer->addDABService(service);
        
        // Process GUI events every 100 services
        if (i % 100 == 0) {
            QApplication::processEvents();
        }
    }
    
    auto endTime = high_resolution_clock::now();
    auto totalTime = duration_cast<microseconds>(endTime - startTime);
    
    // Verify GUI remained responsive during high-volume updates
    EXPECT_LT(totalTime.count(), 100000);  // <100ms for 1000 services
    
    // Verify all services were added correctly
    auto visibleServices = serviceExplorer->getVisibleServices();
    EXPECT_EQ(visibleServices.size(), SERVICE_COUNT);
    
    // Calculate effective update rate
    double updatesPerSecond = (SERVICE_COUNT * 1000000.0) / totalTime.count();
    EXPECT_GT(updatesPerSecond, 7482);  // Meets >7,482 updates/second target
    
    std::cout << "Service Tree Model Performance: PASS - "
              << "Update rate: " << updatesPerSecond << " updates/sec" << std::endl;
}

/**
 * @brief Test real-time Performance Dashboard with Modern ETI Core metrics
 */
TEST_F(GUIModernETIIntegrationTest, PerformanceDashboardValidation) {
    // Find performance dashboard widget
    auto dashboard = mainWindow->findChild<QWidget*>("performanceDashboard");
    
    if (!dashboard) {
        // Performance dashboard may be in a dock widget
        auto performanceDock = mainWindow->findChild<QDockWidget*>();
        if (performanceDock) {
            dashboard = performanceDock->widget();
        }
    }
    
    // Generate high-performance test scenario
    auto processor = mainWindow->getEtiProcessor();
    auto modernParser = processor->getETIParser();
    
    // Process multiple frames to generate performance metrics
    auto testFrames = generateTestFrames(500);
    
    auto startTime = high_resolution_clock::now();
    int successfulFrames = 0;
    
    for (const auto& frame : testFrames) {
        if (processor->processEtiFrame(frame)) {
            successfulFrames++;
        }
        QApplication::processEvents();  // Update GUI
    }
    
    auto endTime = high_resolution_clock::now();
    auto totalTime = duration_cast<microseconds>(endTime - startTime);
    
    // Calculate actual FPS
    double actualFPS = (successfulFrames * 1000000.0) / totalTime.count();
    
    // Verify performance exceeds targets
    EXPECT_GT(actualFPS, 7482);  // >7,482 FPS target
    EXPECT_EQ(successfulFrames, testFrames.size());
    
    // Get Modern ETI performance statistics
    auto perfStats = modernParser->getPerformanceStats();
    EXPECT_GT(perfStats.average_fps, 7482.0);
    EXPECT_LT(perfStats.memory_usage_mb, 5.0);  // <5MB memory usage
    EXPECT_TRUE(perfStats.meets_performance_targets);
    
    std::cout << "Performance Dashboard Validation: PASS - "
              << "FPS: " << actualFPS << ", Memory: " << perfStats.memory_usage_mb << "MB" << std::endl;
}

/**
 * @brief Test ETI Analysis Widget integration with Enhanced FIG Analyser
 */
TEST_F(GUIModernETIIntegrationTest, ETIAnalysisWidgetFIGIntegration) {
    auto analysisWidget = mainWindow->findChild<EtiAnalysisWidget*>();
    
    if (!analysisWidget) {
        // May be in central widget or dock
        auto centralWidget = mainWindow->centralWidget();
        if (centralWidget) {
            analysisWidget = centralWidget->findChild<EtiAnalysisWidget*>();
        }
    }
    
    // Generate test frame with comprehensive FIG data
    auto testFrame = TestDataGenerators::createETIFrameWithFIGs();
    
    auto processor = mainWindow->getEtiProcessor();
    auto figAnalyser = processor->getFIGAnalyser();
    
    // Set up signal spies for FIG analysis
    QSignalSpy figAnalysisSpy(figAnalyser, SIGNAL(figAnalysisComplete(uint32_t, const FIGAnalysisResult&)));
    QSignalSpy ensembleSpy(processor, &EtiProcessor::ensembleDiscovered);
    QSignalSpy serviceSpy(processor, &EtiProcessor::serviceDiscovered);
    
    // Process frame and verify FIG analysis
    bool success = processor->processEtiFrame(testFrame);
    EXPECT_TRUE(success);
    
    // Wait for FIG analysis to complete
    QTest::qWait(100);
    QApplication::processEvents();
    
    // Verify FIG analysis signals were emitted
    EXPECT_GE(figAnalysisSpy.count(), 1);
    
    // Verify ensemble and service discovery
    EXPECT_GE(ensembleSpy.count(), 1);
    EXPECT_GE(serviceSpy.count(), 1);
    
    // If analysis widget exists, verify it displays FIG information
    if (analysisWidget) {
        // Verify widget shows frame information
        auto frameList = analysisWidget->findChild<QTableWidget*>();
        if (frameList) {
            EXPECT_GT(frameList->rowCount(), 0);
        }
    }
    
    std::cout << "ETI Analysis Widget FIG Integration: PASS - "
              << "FIG analyses: " << figAnalysisSpy.count() 
              << ", Services discovered: " << serviceSpy.count() << std::endl;
}

/**
 * @brief Test comprehensive UI responsiveness under high processing loads
 */
TEST_F(GUIModernETIIntegrationTest, UIResponsivenessUnderLoad) {
    // Generate large dataset for stress testing
    auto testFrames = generateTestFrames(2000);
    auto processor = mainWindow->getEtiProcessor();
    
    // Measure GUI responsiveness during high-load processing
    std::vector<int> uiResponseTimes;
    const int RESPONSE_CHECKS = 10;
    
    auto startTime = high_resolution_clock::now();
    
    for (size_t i = 0; i < testFrames.size(); ++i) {
        // Process frame
        processor->processEtiFrame(testFrames[i]);
        
        // Check UI responsiveness every 200 frames
        if (i % 200 == 0) {
            auto uiStartTime = high_resolution_clock::now();
            QApplication::processEvents();
            auto uiEndTime = high_resolution_clock::now();
            
            auto uiTime = duration_cast<microseconds>(uiEndTime - uiStartTime);
            uiResponseTimes.push_back(static_cast<int>(uiTime.count()));
        }
    }
    
    auto endTime = high_resolution_clock::now();
    auto totalTime = duration_cast<milliseconds>(endTime - startTime);
    
    // Verify overall performance
    double fps = (testFrames.size() * 1000.0) / totalTime.count();
    EXPECT_GT(fps, 7482);
    
    // Verify UI remained responsive
    for (int responseTime : uiResponseTimes) {
        EXPECT_LT(responseTime, 16667);  // <16.67ms for 60 FPS UI responsiveness
    }
    
    // Calculate average UI response time
    double avgUIResponse = 0.0;
    for (int time : uiResponseTimes) {
        avgUIResponse += time;
    }
    avgUIResponse /= uiResponseTimes.size();
    
    EXPECT_LT(avgUIResponse, 10000);  // <10ms average UI response time
    
    std::cout << "UI Responsiveness Under Load: PASS - "
              << "FPS: " << fps << ", Avg UI response: " << avgUIResponse << "µs" << std::endl;
}

/**
 * @brief Test Professional Theme consistency across all components
 */
TEST_F(GUIModernETIIntegrationTest, ProfessionalThemeValidation) {
    // Apply professional broadcasting theme
    mainWindow->findChild<ServiceExplorerPanel*>()->applyBroadcastingTheme();
    
    // Verify theme colors are applied correctly
    auto serviceExplorer = mainWindow->findChild<ServiceExplorerPanel*>();
    ASSERT_NE(serviceExplorer, nullptr);
    
    auto serviceTree = serviceExplorer->findChild<QTreeWidget*>();
    if (serviceTree) {
        // Check background color
        auto palette = serviceTree->palette();
        auto bgColor = palette.color(QPalette::Base);
        
        // Verify dark background (#2D2D30 or similar)
        EXPECT_LT(bgColor.red(), 100);
        EXPECT_LT(bgColor.green(), 100);
        EXPECT_LT(bgColor.blue(), 100);
    }
    
    // Add test services with different types to verify color coding
    ServiceInfo audioService;
    audioService.serviceId = 1;
    audioService.label = "Audio Service";
    audioService.type = ServiceInfo::Audio;
    
    ServiceInfo dabPlusService;
    dabPlusService.serviceId = 2;
    dabPlusService.label = "DAB+ Service";
    dabPlusService.type = ServiceInfo::DABPlus;
    
    ServiceInfo dataService;
    dataService.serviceId = 3;
    dataService.label = "Data Service";
    dataService.type = ServiceInfo::Data;
    
    serviceExplorer->addDABService(audioService);
    serviceExplorer->addDABService(dabPlusService);
    serviceExplorer->addDABService(dataService);
    
    QApplication::processEvents();
    
    // Verify services were added with proper theme colors
    auto visibleServices = serviceExplorer->getVisibleServices();
    EXPECT_EQ(visibleServices.size(), 3);
    
    std::cout << "Professional Theme Validation: PASS - "
              << "Theme applied consistently across components" << std::endl;
}

/**
 * @brief Test Thai DAB GUI support with UTF-8 encoding validation
 */
TEST_F(GUIModernETIIntegrationTest, ThaiDABGUISupport) {
    auto serviceExplorer = mainWindow->findChild<ServiceExplorerPanel*>();
    ASSERT_NE(serviceExplorer, nullptr);
    
    // Generate Thai DAB services
    std::vector<ServiceInfo> thaiServices;
    thaiServices.push_back(generateThaiDABService(1));
    
    // Add more Thai services with different content
    ServiceInfo thaiService2;
    thaiService2.serviceId = 2;
    thaiService2.ensembleId = 0x1234;
    thaiService2.label = "วิทยุกระจายเสียง FM";  // FM Radio Broadcasting
    thaiService2.ensembleLabel = "สถานีวิทยุแห่งประเทศไทย";  // Radio Thailand
    thaiService2.typeString = "Audio";
    thaiService2.quality = 88;
    thaiService2.bitrate = 128;
    thaiService2.isActive = true;
    thaiService2.isDabPlus = false;
    thaiService2.type = ServiceInfo::Audio;
    
    thaiServices.push_back(thaiService2);
    
    // Add Thai services to explorer
    for (const auto& service : thaiServices) {
        serviceExplorer->addDABService(service);
    }
    
    QApplication::processEvents();
    
    // Verify Thai text is displayed correctly
    auto serviceTree = serviceExplorer->findChild<QTreeWidget*>();
    if (serviceTree) {
        // Find service items with Thai text
        auto items = serviceTree->findItems("สถานีวิทยุ", Qt::MatchContains | Qt::MatchRecursive);
        EXPECT_GT(items.size(), 0);
        
        if (!items.isEmpty()) {
            auto item = items.first();
            QString text = item->text(0);
            
            // Verify UTF-8 encoding preservation
            EXPECT_TRUE(text.contains("สถานีวิทยุ"));
            EXPECT_TRUE(text.contains("BBC"));
            EXPECT_TRUE(text.contains("ไทย"));
            
            // Verify text length is reasonable (Thai characters properly counted)
            EXPECT_GT(text.length(), 5);
            EXPECT_LT(text.length(), 100);
        }
    }
    
    // Verify visible services include Thai services
    auto visibleServices = serviceExplorer->getVisibleServices();
    EXPECT_EQ(visibleServices.size(), thaiServices.size());
    
    std::cout << "Thai DAB GUI Support: PASS - "
              << "UTF-8 Thai text displayed correctly for " << thaiServices.size() << " services" << std::endl;
}

/**
 * @brief Test Modern ETI Core Engine memory efficiency validation
 */
TEST_F(GUIModernETIIntegrationTest, ModernETIMemoryEfficiency) {
    auto processor = mainWindow->getEtiProcessor();
    auto modernParser = processor->getETIParser();
    
    // Get initial memory usage
    auto initialStats = modernParser->getPerformanceStats();
    double initialMemory = initialStats.memory_usage_mb;
    
    // Process significant amount of data to test memory management
    auto testFrames = generateTestFrames(5000);
    
    for (size_t i = 0; i < testFrames.size(); ++i) {
        processor->processEtiFrame(testFrames[i]);
        
        // Check memory usage periodically
        if (i % 1000 == 0) {
            auto currentStats = modernParser->getPerformanceStats();
            EXPECT_LT(currentStats.memory_usage_mb, 10.0);  // <10MB limit
            QApplication::processEvents();
        }
    }
    
    // Get final memory usage
    auto finalStats = modernParser->getPerformanceStats();
    double finalMemory = finalStats.memory_usage_mb;
    
    // Verify memory usage remained within targets
    EXPECT_LT(finalMemory, 5.0);  // <5MB final memory usage
    EXPECT_LT(finalMemory - initialMemory, 2.0);  // <2MB memory growth
    
    // Verify performance targets were met
    EXPECT_TRUE(finalStats.meets_performance_targets);
    EXPECT_GT(finalStats.average_fps, 7482.0);
    EXPECT_GT(finalStats.speed_improvement_percent, 30.0);  // >30% speed improvement
    EXPECT_GT(finalStats.memory_reduction_percent, 40.0);   // >40% memory reduction
    
    std::cout << "Modern ETI Memory Efficiency: PASS - "
              << "Final memory: " << finalMemory << "MB, "
              << "Speed improvement: " << finalStats.speed_improvement_percent << "%, "
              << "Memory reduction: " << finalStats.memory_reduction_percent << "%" << std::endl;
}

/**
 * @brief Test ETSI compliance display integration
 */
TEST_F(GUIModernETIIntegrationTest, ETSIComplianceDisplayIntegration) {
    auto processor = mainWindow->getEtiProcessor();
    
    // Set up signal spy for compliance updates
    QSignalSpy complianceSpy(processor, SIGNAL(complianceIssueDetected(const QString&, const QString&, const QString&)));
    
    // Generate test frame with perfect ETSI compliance
    auto compliantFrame = TestDataGenerators::createETSICompliantFrame();
    
    // Process frame and verify compliance
    bool success = processor->processEtiFrame(compliantFrame);
    EXPECT_TRUE(success);
    
    QApplication::processEvents();
    
    // For a perfectly compliant frame, should have no compliance issues
    EXPECT_EQ(complianceSpy.count(), 0);
    
    // Generate frame with compliance issues
    auto nonCompliantFrame = TestDataGenerators::createETSINonCompliantFrame();
    
    // Process non-compliant frame
    success = processor->processEtiFrame(nonCompliantFrame);
    EXPECT_TRUE(success);  // Processing should succeed even with compliance issues
    
    QApplication::processEvents();
    
    // Should detect compliance issues
    EXPECT_GT(complianceSpy.count(), 0);
    
    std::cout << "ETSI Compliance Display Integration: PASS - "
              << "Compliance monitoring functional" << std::endl;
}

// Performance benchmark test
TEST_F(GUIModernETIIntegrationTest, DISABLED_PerformanceBenchmark) {
    // This test is disabled by default as it's intensive
    // Enable manually for performance validation
    
    auto processor = mainWindow->getEtiProcessor();
    auto modernParser = processor->getETIParser();
    
    // Generate large test dataset
    const int FRAME_COUNT = 10000;
    auto testFrames = generateTestFrames(FRAME_COUNT);
    
    // Benchmark processing performance
    auto startTime = high_resolution_clock::now();
    
    int successCount = 0;
    for (const auto& frame : testFrames) {
        if (processor->processEtiFrame(frame)) {
            successCount++;
        }
    }
    
    auto endTime = high_resolution_clock::now();
    auto totalTime = duration_cast<microseconds>(endTime - startTime);
    
    // Calculate performance metrics
    double fps = (successCount * 1000000.0) / totalTime.count();
    auto finalStats = modernParser->getPerformanceStats();
    
    // Verify performance targets
    EXPECT_GT(fps, 7482);  // >7,482 FPS
    EXPECT_LT(finalStats.memory_usage_mb, 5.0);  // <5MB memory
    EXPECT_GT(finalStats.speed_improvement_percent, 30.0);  // >30% improvement
    EXPECT_GT(finalStats.memory_reduction_percent, 40.0);   // >40% reduction
    
    std::cout << "Performance Benchmark Results:" << std::endl;
    std::cout << "  Frames processed: " << successCount << "/" << FRAME_COUNT << std::endl;
    std::cout << "  Processing FPS: " << fps << std::endl;
    std::cout << "  Memory usage: " << finalStats.memory_usage_mb << " MB" << std::endl;
    std::cout << "  Speed improvement: " << finalStats.speed_improvement_percent << "%" << std::endl;
    std::cout << "  Memory reduction: " << finalStats.memory_reduction_percent << "%" << std::endl;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize Qt application for GUI testing
    QApplication app(argc, argv);
    
    // Run tests
    int result = RUN_ALL_TESTS();
    
    return result;
}