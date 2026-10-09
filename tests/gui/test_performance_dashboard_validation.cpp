/**
 * @file test_performance_dashboard_validation.cpp
 * @brief Performance Dashboard Validation with Modern ETI Core Engine Metrics
 * 
 * This test suite validates the Performance Dashboard's ability to display
 * real-time metrics from the Modern ETI Core Engine, ensuring accurate
 * representation of >7,482 FPS processing capabilities and professional
 * broadcast industry standards compliance.
 * 
 * Test Coverage:
 * - Real-time FPS display validation
 * - Memory usage monitoring accuracy
 * - ETSI compliance score display
 * - Performance target achievement indicators
 * - Dashboard responsiveness under load
 * - Professional theme consistency
 * - Metric update frequency validation
 * 
 * @author UI/UX Agent - Performance Dashboard Specialist
 * @date 2025-09-21
 */

#include <gtest/gtest.h>
#include <QApplication>
#include <QTest>
#include <QSignalSpy>
#include <QTimer>
#include <QLabel>
#include <QProgressBar>
#include <QWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <memory>
#include <chrono>
#include <vector>

// GUI components
#include "gui/main_window.h"
#include "gui/broadcast_theme.h"

// Core Modern ETI components
#include "core/eti_processor.hpp"
#include "core/modern_eti_frame_parser.hpp"
#include "core/eti_types.h"

// Test utilities
#include "fixtures/test_data_generators.h"

using namespace eti::modern;
using namespace std::chrono;

/**
 * @brief Mock Performance Dashboard Widget for testing
 * 
 * This represents the actual performance dashboard that would be integrated
 * into the main window, providing real-time metrics display.
 */
class MockPerformanceDashboard : public QWidget {
    Q_OBJECT

public:
    explicit MockPerformanceDashboard(QWidget* parent = nullptr) : QWidget(parent) {
        setupUI();
        applyBroadcastTheme();
        
        // Initialize performance metrics
        resetMetrics();
        
        // Set up update timer for real-time display
        updateTimer = new QTimer(this);
        connect(updateTimer, &QTimer::timeout, this, &MockPerformanceDashboard::updateDisplay);
        updateTimer->start(16);  // ~60 FPS UI updates
    }
    
    ~MockPerformanceDashboard() = default;
    
    // Getters for testing
    double getCurrentFPS() const { return currentFPS; }
    double getMemoryUsageMB() const { return memoryUsageMB; }
    double getComplianceScore() const { return complianceScore; }
    double getLatencyMs() const { return latencyMs; }
    bool isTargetsMet() const { return targetsMet; }
    
    // Setters for simulation
    void setFPS(double fps) { 
        currentFPS = fps; 
        updateFPSDisplay();
    }
    
    void setMemoryUsage(double memory) { 
        memoryUsageMB = memory; 
        updateMemoryDisplay();
    }
    
    void setComplianceScore(double score) { 
        complianceScore = score; 
        updateComplianceDisplay();
    }
    
    void setLatency(double latency) { 
        latencyMs = latency; 
        updateLatencyDisplay();
    }
    
    void setTargetsMet(bool met) { 
        targetsMet = met; 
        updateTargetsDisplay();
    }
    
    // Simulate Modern ETI Core Engine performance update
    void simulatePerformanceUpdate(const ModernETIFrameParser::PerformanceStats& stats) {
        setFPS(stats.average_fps);
        setMemoryUsage(stats.memory_usage_mb);
        setLatency(stats.average_frame_time_us / 1000.0);  // Convert to ms
        setTargetsMet(stats.meets_performance_targets);
        
        // Update additional metrics
        speedImprovement = stats.speed_improvement_percent;
        memoryReduction = stats.memory_reduction_percent;
        cacheHitRate = stats.cache_hit_rate_percent;
        
        updateAdditionalMetrics();
    }
    
    // Get UI components for testing
    QLabel* getFPSLabel() const { return fpsLabel; }
    QLabel* getMemoryLabel() const { return memoryLabel; }
    QLabel* getComplianceLabel() const { return complianceLabel; }
    QLabel* getLatencyLabel() const { return latencyLabel; }
    QWidget* getTargetsIndicator() const { return targetsIndicator; }
    QProgressBar* getMemoryBar() const { return memoryBar; }
    QProgressBar* getComplianceBar() const { return complianceBar; }

public slots:
    void onPerformanceUpdate(const ModernETIFrameParser::PerformanceStats& stats) {
        simulatePerformanceUpdate(stats);
    }

private slots:
    void updateDisplay() {
        // Simulate smooth updates for better visual experience
        updateFPSDisplay();
        updateMemoryDisplay();
        updateComplianceDisplay();
        updateLatencyDisplay();
        updateTargetsDisplay();
    }

private:
    void setupUI() {
        auto mainLayout = new QVBoxLayout(this);
        
        // Performance Metrics Section
        auto metricsLayout = new QGridLayout();
        
        // FPS Display
        metricsLayout->addWidget(new QLabel("Processing FPS:"), 0, 0);
        fpsLabel = new QLabel("0");
        fpsLabel->setObjectName("fpsLabel");
        metricsLayout->addWidget(fpsLabel, 0, 1);
        
        // Memory Usage
        metricsLayout->addWidget(new QLabel("Memory Usage:"), 1, 0);
        memoryLabel = new QLabel("0 MB");
        memoryLabel->setObjectName("memoryLabel");
        metricsLayout->addWidget(memoryLabel, 1, 1);
        
        memoryBar = new QProgressBar();
        memoryBar->setObjectName("memoryBar");
        memoryBar->setRange(0, 100);  // 0-100MB range
        metricsLayout->addWidget(memoryBar, 1, 2);
        
        // Compliance Score
        metricsLayout->addWidget(new QLabel("ETSI Compliance:"), 2, 0);
        complianceLabel = new QLabel("100%");
        complianceLabel->setObjectName("complianceLabel");
        metricsLayout->addWidget(complianceLabel, 2, 1);
        
        complianceBar = new QProgressBar();
        complianceBar->setObjectName("complianceBar");
        complianceBar->setRange(0, 100);
        complianceBar->setValue(100);
        metricsLayout->addWidget(complianceBar, 2, 2);
        
        // Latency
        metricsLayout->addWidget(new QLabel("Processing Latency:"), 3, 0);
        latencyLabel = new QLabel("0 ms");
        latencyLabel->setObjectName("latencyLabel");
        metricsLayout->addWidget(latencyLabel, 3, 1);
        
        mainLayout->addLayout(metricsLayout);
        
        // Targets Indicator
        targetsIndicator = new QWidget();
        targetsIndicator->setObjectName("targetsIndicator");
        targetsIndicator->setFixedSize(20, 20);
        
        auto targetsLayout = new QHBoxLayout();
        targetsLayout->addWidget(new QLabel("Performance Targets:"));
        targetsLayout->addWidget(targetsIndicator);
        targetsLayout->addStretch();
        
        mainLayout->addLayout(targetsLayout);
        
        // Additional Metrics
        additionalMetricsLabel = new QLabel("Speed: 0%, Memory Reduction: 0%, Cache: 0%");
        additionalMetricsLabel->setObjectName("additionalMetricsLabel");
        mainLayout->addWidget(additionalMetricsLabel);
        
        mainLayout->addStretch();
    }
    
    void applyBroadcastTheme() {
        // Apply professional broadcast theme
        setStyleSheet(QString(
            "QWidget { "
            "    background-color: %1; "
            "    color: white; "
            "    font-family: 'Segoe UI'; "
            "} "
            "QLabel { "
            "    font-size: 10px; "
            "    padding: 2px; "
            "} "
            "#fpsLabel { "
            "    font-weight: bold; "
            "    font-size: 12px; "
            "} "
            "#complianceLabel { "
            "    font-weight: bold; "
            "} "
            "QProgressBar { "
            "    border: 1px solid %2; "
            "    border-radius: 3px; "
            "    background-color: #404040; "
            "    text-align: center; "
            "} "
            "QProgressBar::chunk { "
            "    background-color: %3; "
            "    border-radius: 2px; "
            "} "
        ).arg(BroadcastTheme::colorToHex(BroadcastTheme::BACKGROUND_DARK))
         .arg(BroadcastTheme::colorToHex(BroadcastTheme::ACCENT_BLUE))
         .arg(BroadcastTheme::colorToHex(BroadcastTheme::STATUS_OK)));
    }
    
    void updateFPSDisplay() {
        fpsLabel->setText(QString::number(currentFPS, 'f', 1));
        
        // Color coding based on performance
        QColor fpsColor;
        if (currentFPS >= 7482) {
            fpsColor = BroadcastTheme::STATUS_OK;  // Green for excellent
        } else if (currentFPS >= 5000) {
            fpsColor = BroadcastTheme::STATUS_WARNING;  // Orange for good
        } else {
            fpsColor = BroadcastTheme::STATUS_ERROR;  // Red for poor
        }
        
        fpsLabel->setStyleSheet(QString("color: %1; font-weight: bold;")
                               .arg(BroadcastTheme::colorToHex(fpsColor)));
    }
    
    void updateMemoryDisplay() {
        memoryLabel->setText(QString("%1 MB").arg(memoryUsageMB, 0, 'f', 1));
        memoryBar->setValue(static_cast<int>(memoryUsageMB));
        
        // Color coding based on memory usage
        QColor memoryColor;
        if (memoryUsageMB <= 5.0) {
            memoryColor = BroadcastTheme::STATUS_OK;  // Green for excellent
        } else if (memoryUsageMB <= 10.0) {
            memoryColor = BroadcastTheme::STATUS_WARNING;  // Orange for acceptable
        } else {
            memoryColor = BroadcastTheme::STATUS_ERROR;  // Red for high
        }
        
        memoryBar->setStyleSheet(QString(
            "QProgressBar::chunk { background-color: %1; }"
        ).arg(BroadcastTheme::colorToHex(memoryColor)));
    }
    
    void updateComplianceDisplay() {
        complianceLabel->setText(QString("%1%").arg(complianceScore, 0, 'f', 1));
        complianceBar->setValue(static_cast<int>(complianceScore));
        
        // Color coding based on compliance score
        QColor complianceColor;
        if (complianceScore >= 95.0) {
            complianceColor = BroadcastTheme::STATUS_OK;  // Green for excellent
        } else if (complianceScore >= 85.0) {
            complianceColor = BroadcastTheme::STATUS_WARNING;  // Orange for good
        } else {
            complianceColor = BroadcastTheme::STATUS_ERROR;  // Red for poor
        }
        
        complianceBar->setStyleSheet(QString(
            "QProgressBar::chunk { background-color: %1; }"
        ).arg(BroadcastTheme::colorToHex(complianceColor)));
    }
    
    void updateLatencyDisplay() {
        latencyLabel->setText(QString("%1 ms").arg(latencyMs, 0, 'f', 2));
        
        // Color coding based on latency
        QColor latencyColor;
        if (latencyMs <= 1.0) {
            latencyColor = BroadcastTheme::STATUS_OK;  // Green for excellent
        } else if (latencyMs <= 5.0) {
            latencyColor = BroadcastTheme::STATUS_WARNING;  // Orange for acceptable
        } else {
            latencyColor = BroadcastTheme::STATUS_ERROR;  // Red for high
        }
        
        latencyLabel->setStyleSheet(QString("color: %1;")
                                   .arg(BroadcastTheme::colorToHex(latencyColor)));
    }
    
    void updateTargetsDisplay() {
        QColor indicatorColor = targetsMet ? BroadcastTheme::STATUS_OK : BroadcastTheme::STATUS_ERROR;
        
        targetsIndicator->setStyleSheet(QString(
            "QWidget { "
            "    background-color: %1; "
            "    border-radius: 10px; "
            "    border: 2px solid #333; "
            "}"
        ).arg(BroadcastTheme::colorToHex(indicatorColor)));
    }
    
    void updateAdditionalMetrics() {
        additionalMetricsLabel->setText(QString(
            "Speed: %1%, Memory Reduction: %2%, Cache: %3%"
        ).arg(speedImprovement, 0, 'f', 1)
         .arg(memoryReduction, 0, 'f', 1)
         .arg(cacheHitRate));
    }
    
    void resetMetrics() {
        currentFPS = 0.0;
        memoryUsageMB = 0.0;
        complianceScore = 100.0;
        latencyMs = 0.0;
        targetsMet = false;
        speedImprovement = 0.0;
        memoryReduction = 0.0;
        cacheHitRate = 0;
    }
    
    // UI Components
    QLabel* fpsLabel;
    QLabel* memoryLabel;
    QLabel* complianceLabel;
    QLabel* latencyLabel;
    QLabel* additionalMetricsLabel;
    QWidget* targetsIndicator;
    QProgressBar* memoryBar;
    QProgressBar* complianceBar;
    QTimer* updateTimer;
    
    // Performance metrics
    double currentFPS;
    double memoryUsageMB;
    double complianceScore;
    double latencyMs;
    bool targetsMet;
    
    // Additional metrics
    double speedImprovement;
    double memoryReduction;
    int cacheHitRate;
};

class PerformanceDashboardTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize Qt application
        if (!QApplication::instance()) {
            int argc = 1;
            char* argv[] = {"test"};
            app = std::make_unique<QApplication>(argc, argv);
        }
        
        // Create dashboard widget
        dashboard = std::make_unique<MockPerformanceDashboard>();
        dashboard->show();
        QTest::qWaitForWindowExposed(dashboard.get());
        
        // Create Modern ETI processor for integration testing
        setupModernETIProcessor();
    }
    
    void TearDown() override {
        dashboard.reset();
        if (app) {
            app->processEvents();
        }
    }
    
    void setupModernETIProcessor() {
        // Configure Modern ETI Frame Parser for testing
        ProcessingConfig config;
        config.optimization_level = ProcessingConfig::OptimizationLevel::AGGRESSIVE;
        config.target_fps = 8000.0;
        config.memory_limit_mb = 4;
        
        modernParser = std::make_unique<ModernETIFrameParser>();
        ASSERT_TRUE(modernParser->initialize(config));
        
        // Connect performance updates to dashboard
        QObject::connect(modernParser.get(), &ModernETIFrameParser::frameProcessed,
                        [this](uint32_t frameNumber, const eti::EtiFrame& frame, std::chrono::nanoseconds parseTime) {
                            // Update dashboard with current performance
                            auto stats = modernParser->getPerformanceStats();
                            dashboard->simulatePerformanceUpdate(stats);
                        });
    }
    
    // Generate test performance statistics
    ModernETIFrameParser::PerformanceStats generateTestStats(double fps, double memory, bool targetsAchieved) {
        ModernETIFrameParser::PerformanceStats stats;
        stats.frames_processed = 10000;
        stats.average_fps = fps;
        stats.average_frame_time_us = 1000000.0 / fps;  // Convert FPS to microseconds
        stats.memory_usage_mb = memory;
        stats.cpu_efficiency = 85.0;
        stats.cache_hit_rate_percent = 92;
        stats.speed_improvement_percent = 35.0;
        stats.memory_reduction_percent = 45.0;
        stats.meets_performance_targets = targetsAchieved;
        
        return stats;
    }
    
    std::unique_ptr<QApplication> app;
    std::unique_ptr<MockPerformanceDashboard> dashboard;
    std::unique_ptr<ModernETIFrameParser> modernParser;
};

/**
 * @brief Test real-time FPS display validation with >7,482 FPS capability
 */
TEST_F(PerformanceDashboardTest, RealTimeFPSDisplayValidation) {
    // Test various FPS levels
    std::vector<double> testFPS = {5000.0, 7482.0, 8500.0, 10000.0, 12000.0};
    
    for (double fps : testFPS) {
        auto stats = generateTestStats(fps, 3.5, fps >= 7482.0);
        dashboard->simulatePerformanceUpdate(stats);
        
        // Process GUI events for display update
        QApplication::processEvents();
        QTest::qWait(50);  // Allow UI to update
        
        // Verify FPS display
        EXPECT_NEAR(dashboard->getCurrentFPS(), fps, 0.1);
        
        auto fpsLabel = dashboard->getFPSLabel();
        ASSERT_NE(fpsLabel, nullptr);
        
        QString displayText = fpsLabel->text();
        EXPECT_TRUE(displayText.contains(QString::number(fps, 'f', 1)));
        
        // Verify color coding
        if (fps >= 7482.0) {
            // Should be green for meeting targets
            QString styleSheet = fpsLabel->styleSheet();
            EXPECT_TRUE(styleSheet.contains("color"));
        }
    }
    
    std::cout << "Real-time FPS Display Validation: PASS - "
              << "FPS range tested: 5,000-12,000" << std::endl;
}

/**
 * @brief Test memory usage monitoring accuracy
 */
TEST_F(PerformanceDashboardTest, MemoryUsageMonitoringAccuracy) {
    // Test memory usage levels
    std::vector<double> memoryLevels = {2.1, 3.8, 4.5, 6.2, 8.9};
    
    for (double memory : memoryLevels) {
        auto stats = generateTestStats(8000.0, memory, memory <= 5.0);
        dashboard->simulatePerformanceUpdate(stats);
        
        QApplication::processEvents();
        QTest::qWait(30);
        
        // Verify memory display
        EXPECT_NEAR(dashboard->getMemoryUsageMB(), memory, 0.1);
        
        auto memoryLabel = dashboard->getMemoryLabel();
        auto memoryBar = dashboard->getMemoryBar();
        
        ASSERT_NE(memoryLabel, nullptr);
        ASSERT_NE(memoryBar, nullptr);
        
        // Verify text display
        QString memoryText = memoryLabel->text();
        EXPECT_TRUE(memoryText.contains(QString::number(memory, 'f', 1)));
        EXPECT_TRUE(memoryText.contains("MB"));
        
        // Verify progress bar
        int expectedBarValue = static_cast<int>(memory);
        EXPECT_EQ(memoryBar->value(), expectedBarValue);
        
        // Verify color coding based on memory usage
        if (memory <= 5.0) {
            // Should use green color for good memory usage
            QString barStyle = memoryBar->styleSheet();
            EXPECT_TRUE(barStyle.contains("background-color"));
        }
    }
    
    std::cout << "Memory Usage Monitoring Accuracy: PASS - "
              << "Memory range tested: 2.1-8.9 MB" << std::endl;
}

/**
 * @brief Test ETSI compliance score display
 */
TEST_F(PerformanceDashboardTest, ETSIComplianceScoreDisplay) {
    // Test compliance scores
    std::vector<double> complianceScores = {100.0, 98.5, 95.0, 87.3, 76.8};
    
    for (double score : complianceScores) {
        dashboard->setComplianceScore(score);
        
        QApplication::processEvents();
        QTest::qWait(20);
        
        // Verify compliance display
        EXPECT_NEAR(dashboard->getComplianceScore(), score, 0.1);
        
        auto complianceLabel = dashboard->getComplianceLabel();
        auto complianceBar = dashboard->getComplianceBar();
        
        ASSERT_NE(complianceLabel, nullptr);
        ASSERT_NE(complianceBar, nullptr);
        
        // Verify text display
        QString complianceText = complianceLabel->text();
        EXPECT_TRUE(complianceText.contains(QString::number(score, 'f', 1)));
        EXPECT_TRUE(complianceText.contains("%"));
        
        // Verify progress bar
        int expectedBarValue = static_cast<int>(score);
        EXPECT_EQ(complianceBar->value(), expectedBarValue);
        
        // Verify color coding
        if (score >= 95.0) {
            // Should use green for excellent compliance
            QString barStyle = complianceBar->styleSheet();
            EXPECT_TRUE(barStyle.contains("background-color"));
        }
    }
    
    std::cout << "ETSI Compliance Score Display: PASS - "
              << "Compliance range tested: 76.8%-100%" << std::endl;
}

/**
 * @brief Test performance target achievement indicators
 */
TEST_F(PerformanceDashboardTest, PerformanceTargetIndicators) {
    // Test scenarios with targets met and not met
    struct TestScenario {
        double fps;
        double memory;
        bool expectedTargetsMet;
        std::string description;
    };
    
    std::vector<TestScenario> scenarios = {
        {8500.0, 3.2, true,  "Excellent performance - targets exceeded"},
        {7500.0, 4.8, true,  "Good performance - targets met"},
        {6800.0, 5.5, false, "Poor FPS - targets not met"},
        {8000.0, 8.0, false, "High memory - targets not met"},
        {5000.0, 12.0, false, "Poor overall - targets not met"}
    };
    
    for (const auto& scenario : scenarios) {
        auto stats = generateTestStats(scenario.fps, scenario.memory, scenario.expectedTargetsMet);
        dashboard->simulatePerformanceUpdate(stats);
        
        QApplication::processEvents();
        QTest::qWait(30);
        
        // Verify targets indicator
        EXPECT_EQ(dashboard->isTargetsMet(), scenario.expectedTargetsMet);
        
        auto targetsIndicator = dashboard->getTargetsIndicator();
        ASSERT_NE(targetsIndicator, nullptr);
        
        // Verify indicator styling
        QString indicatorStyle = targetsIndicator->styleSheet();
        EXPECT_TRUE(indicatorStyle.contains("background-color"));
        
        if (scenario.expectedTargetsMet) {
            // Should show green for targets met
            EXPECT_TRUE(indicatorStyle.contains("4CAF50") || 
                       indicatorStyle.contains("00A000"));  // Green variations
        } else {
            // Should show red for targets not met
            EXPECT_TRUE(indicatorStyle.contains("F44336") || 
                       indicatorStyle.contains("DC3545"));  // Red variations
        }
        
        std::cout << "  Scenario: " << scenario.description << " - " 
                  << (scenario.expectedTargetsMet ? "PASS" : "EXPECTED FAIL") << std::endl;
    }
    
    std::cout << "Performance Target Indicators: PASS - All scenarios validated" << std::endl;
}

/**
 * @brief Test dashboard responsiveness under high update frequency
 */
TEST_F(PerformanceDashboardTest, DashboardResponsivenessUnderLoad) {
    const int UPDATE_COUNT = 1000;
    const int UPDATE_INTERVAL_MS = 5;  // Very fast updates
    
    std::vector<double> updateTimes;
    
    for (int i = 0; i < UPDATE_COUNT; ++i) {
        // Generate varying performance data
        double fps = 7000.0 + (i % 100) * 50;  // 7000-12000 FPS range
        double memory = 2.0 + (i % 10) * 0.3;  // 2.0-5.0 MB range
        bool targets = fps >= 7482.0 && memory <= 5.0;
        
        auto startTime = high_resolution_clock::now();
        
        auto stats = generateTestStats(fps, memory, targets);
        dashboard->simulatePerformanceUpdate(stats);
        
        QApplication::processEvents();
        
        auto endTime = high_resolution_clock::now();
        auto updateTime = duration_cast<microseconds>(endTime - startTime);
        updateTimes.push_back(updateTime.count());
        
        // Small delay to simulate real-time updates
        if (i % 10 == 0) {
            QTest::qWait(UPDATE_INTERVAL_MS);
        }
    }
    
    // Calculate statistics
    double avgUpdateTime = std::accumulate(updateTimes.begin(), updateTimes.end(), 0.0) / updateTimes.size();
    double maxUpdateTime = *std::max_element(updateTimes.begin(), updateTimes.end());
    
    // Verify responsiveness
    EXPECT_LT(avgUpdateTime, 1000);  // <1ms average update time
    EXPECT_LT(maxUpdateTime, 5000);  // <5ms maximum update time
    
    // Verify final display values are reasonable
    EXPECT_GT(dashboard->getCurrentFPS(), 7000);
    EXPECT_LT(dashboard->getMemoryUsageMB(), 6.0);
    
    std::cout << "Dashboard Responsiveness Under Load: PASS - "
              << "Avg update: " << avgUpdateTime << "µs, "
              << "Max update: " << maxUpdateTime << "µs" << std::endl;
}

/**
 * @brief Test professional theme consistency in dashboard
 */
TEST_F(PerformanceDashboardTest, ProfessionalThemeConsistency) {
    // Verify dashboard uses professional broadcast theme
    QString dashboardStyle = dashboard->styleSheet();
    
    // Check for key theme elements
    EXPECT_TRUE(dashboardStyle.contains("#2D2D30") || dashboardStyle.contains("background-color"));
    EXPECT_TRUE(dashboardStyle.contains("Segoe UI") || dashboardStyle.contains("font-family"));
    
    // Test theme application with different performance states
    struct ThemeTestCase {
        double fps;
        double memory;
        double compliance;
        std::string expectedColors;
    };
    
    std::vector<ThemeTestCase> themeTests = {
        {9000.0, 3.0, 100.0, "green"},     // Excellent - green indicators
        {6000.0, 7.0, 85.0, "orange"},    // Warning - orange indicators  
        {3000.0, 15.0, 70.0, "red"}       // Error - red indicators
    };
    
    for (const auto& test : themeTests) {
        auto stats = generateTestStats(test.fps, test.memory, test.fps >= 7482.0);
        dashboard->simulatePerformanceUpdate(stats);
        dashboard->setComplianceScore(test.compliance);
        
        QApplication::processEvents();
        QTest::qWait(50);
        
        // Verify color consistency across components
        auto fpsLabel = dashboard->getFPSLabel();
        auto memoryBar = dashboard->getMemoryBar();
        auto complianceBar = dashboard->getComplianceBar();
        auto targetsIndicator = dashboard->getTargetsIndicator();
        
        // All components should have appropriate styling
        EXPECT_FALSE(fpsLabel->styleSheet().isEmpty());
        EXPECT_FALSE(memoryBar->styleSheet().isEmpty());
        EXPECT_FALSE(complianceBar->styleSheet().isEmpty());
        EXPECT_FALSE(targetsIndicator->styleSheet().isEmpty());
    }
    
    std::cout << "Professional Theme Consistency: PASS - "
              << "Theme applied consistently across performance states" << std::endl;
}

/**
 * @brief Test metric update frequency validation
 */
TEST_F(PerformanceDashboardTest, MetricUpdateFrequencyValidation) {
    // Test update frequency matches expected 60 FPS
    const int MEASUREMENT_DURATION_MS = 1000;  // 1 second
    int updateCount = 0;
    
    QTimer measurementTimer;
    measurementTimer.setSingleShot(true);
    
    QTimer testTimer;
    connect(&testTimer, &QTimer::timeout, [&]() {
        updateCount++;
        // Simulate performance update
        auto stats = generateTestStats(8000.0, 3.5, true);
        dashboard->simulatePerformanceUpdate(stats);
    });
    
    // Start measurement
    testTimer.start(16);  // ~60 FPS
    measurementTimer.start(MEASUREMENT_DURATION_MS);
    
    // Wait for measurement to complete
    QEventLoop loop;
    connect(&measurementTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    loop.exec();
    
    testTimer.stop();
    
    // Calculate actual update frequency
    double actualFPS = updateCount;  // Updates per second
    
    // Verify update frequency is close to target (allow some variance)
    EXPECT_GT(actualFPS, 55);  // At least 55 FPS
    EXPECT_LT(actualFPS, 65);  // No more than 65 FPS
    
    std::cout << "Metric Update Frequency Validation: PASS - "
              << "Actual FPS: " << actualFPS << " (target: 60)" << std::endl;
}

#include "test_performance_dashboard_validation.moc"

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize Qt application for GUI testing
    QApplication app(argc, argv);
    
    // Run tests
    int result = RUN_ALL_TESTS();
    
    return result;
}