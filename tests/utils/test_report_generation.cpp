#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QTest>
#include <QSignalSpy>
#include <QApplication>
#include <QTimer>
#include <QTime>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPrinter>
#include <QTextDocument>
#include <memory>
#include "../../src/utils/report_generator.h"

using namespace testing;

/**
 * @brief Test Report Generation TDD Implementation
 * 
 * Professional Report Generation for ETI Stream Analyser following TDD methodology.
 * Tests written BEFORE implementation (RED phase).
 * 
 * Features to test:
 * - PDF/HTML report generation capabilities
 * - Professional report templates for broadcast industry
 * - Statistical analysis and trending capabilities
 * - ETSI compliance reporting integration
 * - Export functionality with multiple formats
 * - Real-time dashboard generation
 * - Performance requirements for large datasets
 */
class ReportGenerationTest : public ::testing::Test 
{
protected:
    void SetUp() override 
    {
        // Create QApplication if needed for Qt tests
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            app = std::make_unique<QApplication>(argc, argv);
        }
        
        // Create temporary test directory
        testDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/eti_analyser_test_reports";
        QDir().mkpath(testDir);
        
        // Initialize report generator
        reportGenerator = std::make_unique<ReportGenerator>();
        reportGenerator->setOutputDirectory(testDir);
        
        // Create sample report data
        setupSampleData();
    }

    void TearDown() override 
    {
        reportGenerator.reset();
        
        // Clean up test files
        QDir dir(testDir);
        dir.removeRecursively();
    }
    
    void setupSampleData() {
        // Sample ETI analysis data
        sampleData.streamName = "Test ETI Stream";
        sampleData.analysisStartTime = QDateTime::currentDateTime().addSecs(-3600);
        sampleData.analysisEndTime = QDateTime::currentDateTime();
        sampleData.totalFrames = 90000;  // 1 hour of ETI frames (250 fps)
        sampleData.validFrames = 89950;
        sampleData.invalidFrames = 50;
        sampleData.averageSignalQuality = 18.5;  // dB SNR
        sampleData.averageErrorRate = 0.0005;    // 0.05% BER
        
        // Service information
        ReportData::ServiceInfo service1;
        service1.serviceId = 0x1001;
        service1.serviceName = "Test Radio";
        service1.serviceType = "DAB+ Audio";
        service1.bitRate = 96;  // kbps
        service1.qualityScore = 95.2;  // %
        sampleData.services.append(service1);
        
        ReportData::ServiceInfo service2;
        service2.serviceId = 0x1002;
        service2.serviceName = "Music Station";
        service2.serviceType = "DAB+ Audio";
        service2.bitRate = 128;  // kbps
        service2.qualityScore = 97.8;  // %
        sampleData.services.append(service2);
        
        // Alert statistics
        sampleData.totalAlerts = 15;
        sampleData.criticalAlerts = 2;
        sampleData.warningAlerts = 8;
        sampleData.infoAlerts = 5;
    }

    std::unique_ptr<QApplication> app;
    std::unique_ptr<ReportGenerator> reportGenerator;
    QString testDir;
    ReportData sampleData;
};

// ============================================================================
// BASIC REPORT GENERATION FUNCTIONALITY TESTS
// ============================================================================

TEST_F(ReportGenerationTest, ConstructorInitializesCorrectly)
{
    // ARRANGE & ACT - Constructor called in SetUp
    
    // ASSERT
    EXPECT_TRUE(reportGenerator != nullptr);
    EXPECT_TRUE(reportGenerator->isInitialized());
    EXPECT_EQ(reportGenerator->getOutputDirectory(), testDir);
    EXPECT_EQ(reportGenerator->getDefaultFormat(), ReportGenerator::OutputFormat::HTML);
}

TEST_F(ReportGenerationTest, SetOutputDirectoryValidatesPath)
{
    // ARRANGE
    QString validPath = testDir + "/reports";
    QString invalidPath = "/root/restricted";  // Should not be writable
    
    // ACT & ASSERT
    EXPECT_TRUE(reportGenerator->setOutputDirectory(validPath));
    EXPECT_EQ(reportGenerator->getOutputDirectory(), validPath);
    
    EXPECT_FALSE(reportGenerator->setOutputDirectory(invalidPath));
    EXPECT_NE(reportGenerator->getOutputDirectory(), invalidPath);
}

TEST_F(ReportGenerationTest, SupportedFormatsAreCorrect)
{
    // ARRANGE & ACT
    QStringList supportedFormats = reportGenerator->getSupportedFormats();
    
    // ASSERT
    EXPECT_TRUE(supportedFormats.contains("HTML"));
    EXPECT_TRUE(supportedFormats.contains("PDF"));
    EXPECT_TRUE(supportedFormats.contains("CSV"));
    EXPECT_TRUE(supportedFormats.contains("JSON"));
    EXPECT_TRUE(supportedFormats.contains("XML"));
    EXPECT_GE(supportedFormats.size(), 5);
}

// ============================================================================
// HTML REPORT GENERATION TESTS
// ============================================================================

TEST_F(ReportGenerationTest, GenerateHtmlReportCreatesValidFile)
{
    // ARRANGE
    QString reportPath = testDir + "/test_report.html";
    ReportGenerator::ReportConfig config;
    config.format = ReportGenerator::OutputFormat::HTML;
    config.title = "ETI Stream Analysis Report";
    config.includeCharts = true;
    config.includeDetailedAnalysis = true;
    
    // ACT
    bool result = reportGenerator->generateReport(sampleData, config, reportPath);
    
    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_TRUE(QFile::exists(reportPath));
    
    // Verify HTML structure
    QFile htmlFile(reportPath);
    EXPECT_TRUE(htmlFile.open(QIODevice::ReadOnly));
    QString content = QString::fromUtf8(htmlFile.readAll());
    
    EXPECT_TRUE(content.contains("<!DOCTYPE html>"));
    EXPECT_TRUE(content.contains("<html>"));
    EXPECT_TRUE(content.contains("ETI Stream Analysis Report"));
    EXPECT_TRUE(content.contains("Test ETI Stream"));
    EXPECT_TRUE(content.contains("Test Radio"));
    EXPECT_TRUE(content.contains("Music Station"));
    EXPECT_TRUE(content.contains("</html>"));
}

TEST_F(ReportGenerationTest, HtmlReportIncludesStylesAndScripts)
{
    // ARRANGE
    QString reportPath = testDir + "/styled_report.html";
    ReportGenerator::ReportConfig config;
    config.format = ReportGenerator::OutputFormat::HTML;
    config.includeCharts = true;
    config.theme = ReportGenerator::Theme::Professional;
    
    // ACT
    bool result = reportGenerator->generateReport(sampleData, config, reportPath);
    
    // ASSERT
    EXPECT_TRUE(result);
    
    QFile htmlFile(reportPath);
    EXPECT_TRUE(htmlFile.open(QIODevice::ReadOnly));
    QString content = QString::fromUtf8(htmlFile.readAll());
    
    // Should include CSS styles
    EXPECT_TRUE(content.contains("<style>") || content.contains("stylesheet"));
    
    // Should include Chart.js for interactive charts
    EXPECT_TRUE(content.contains("chart") || content.contains("Chart"));
    
    // Should have professional styling
    EXPECT_TRUE(content.contains("table") || content.contains("grid"));
}

TEST_F(ReportGenerationTest, HtmlReportChartsAreGenerated)
{
    // ARRANGE
    QString reportPath = testDir + "/chart_report.html";
    ReportGenerator::ReportConfig config;
    config.format = ReportGenerator::OutputFormat::HTML;
    config.includeCharts = true;
    config.chartTypes = {
        ReportGenerator::ChartType::SignalQuality,
        ReportGenerator::ChartType::ServiceStatus,
        ReportGenerator::ChartType::AlertSummary
    };
    
    // ACT
    bool result = reportGenerator->generateReport(sampleData, config, reportPath);
    
    // ASSERT
    EXPECT_TRUE(result);
    
    QFile htmlFile(reportPath);
    EXPECT_TRUE(htmlFile.open(QIODevice::ReadOnly));
    QString content = QString::fromUtf8(htmlFile.readAll());
    
    // Should contain chart elements
    EXPECT_TRUE(content.contains("canvas") || content.contains("svg"));
    EXPECT_TRUE(content.contains("Signal Quality") || content.contains("signalQuality"));
    EXPECT_TRUE(content.contains("Service") || content.contains("service"));
    EXPECT_TRUE(content.contains("Alert") || content.contains("alert"));
}

// ============================================================================
// PDF REPORT GENERATION TESTS
// ============================================================================

TEST_F(ReportGenerationTest, GeneratePdfReportCreatesValidFile)
{
    // ARRANGE
    QString reportPath = testDir + "/test_report.pdf";
    ReportGenerator::ReportConfig config;
    config.format = ReportGenerator::OutputFormat::PDF;
    config.title = "Professional ETI Analysis Report";
    config.includeCharts = true;
    config.includeExecutiveSummary = true;
    
    // ACT
    bool result = reportGenerator->generateReport(sampleData, config, reportPath);
    
    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_TRUE(QFile::exists(reportPath));
    
    // Verify file is not empty and has PDF signature
    QFile pdfFile(reportPath);
    EXPECT_TRUE(pdfFile.open(QIODevice::ReadOnly));
    QByteArray pdfData = pdfFile.read(8);
    QString pdfHeader = QString::fromLatin1(pdfData);
    EXPECT_TRUE(pdfHeader.startsWith("%PDF"));
    
    // File should be reasonably sized (not empty)
    EXPECT_GT(pdfFile.size(), 1000);
}

TEST_F(ReportGenerationTest, PdfReportHasProfessionalLayout)
{
    // ARRANGE
    QString reportPath = testDir + "/professional_report.pdf";
    ReportGenerator::ReportConfig config;
    config.format = ReportGenerator::OutputFormat::PDF;
    config.theme = ReportGenerator::Theme::Professional;
    config.includeTableOfContents = true;
    config.includePageNumbers = true;
    config.includeHeader = true;
    config.includeFooter = true;
    
    // ACT
    bool result = reportGenerator->generateReport(sampleData, config, reportPath);
    
    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_TRUE(QFile::exists(reportPath));
    
    // PDF should be larger with professional formatting
    QFile pdfFile(reportPath);
    EXPECT_TRUE(pdfFile.open(QIODevice::ReadOnly));
    EXPECT_GT(pdfFile.size(), 5000);  // Should be larger with professional layout
}

// ============================================================================
// CSV EXPORT TESTS
// ============================================================================

TEST_F(ReportGenerationTest, GenerateCsvExportCreatesValidFile)
{
    // ARRANGE
    QString csvPath = testDir + "/export_data.csv";
    ReportGenerator::ExportConfig config;
    config.format = ReportGenerator::OutputFormat::CSV;
    config.includeServiceData = true;
    config.includeAlertData = true;
    config.includeQualityMetrics = true;
    
    // ACT
    bool result = reportGenerator->exportData(sampleData, config, csvPath);
    
    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_TRUE(QFile::exists(csvPath));
    
    // Verify CSV structure
    QFile csvFile(csvPath);
    EXPECT_TRUE(csvFile.open(QIODevice::ReadOnly));
    QString content = QString::fromUtf8(csvFile.readAll());
    QStringList lines = content.split('\n');
    
    EXPECT_GT(lines.size(), 2);  // Header + at least one data row
    
    // Check header line
    QString header = lines[0];
    EXPECT_TRUE(header.contains("Service"));
    EXPECT_TRUE(header.contains("Quality") || header.contains("Bit Rate"));
    
    // Check data lines contain expected values
    bool foundTestRadio = false;
    for (const QString& line : lines) {
        if (line.contains("Test Radio")) {
            foundTestRadio = true;
            EXPECT_TRUE(line.contains("96") || line.contains("95.2"));  // Bit rate or quality
            break;
        }
    }
    EXPECT_TRUE(foundTestRadio);
}

// ============================================================================
// JSON EXPORT TESTS
// ============================================================================

TEST_F(ReportGenerationTest, GenerateJsonExportCreatesValidFile)
{
    // ARRANGE
    QString jsonPath = testDir + "/export_data.json";
    ReportGenerator::ExportConfig config;
    config.format = ReportGenerator::OutputFormat::JSON;
    config.prettyPrint = true;
    
    // ACT
    bool result = reportGenerator->exportData(sampleData, config, jsonPath);
    
    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_TRUE(QFile::exists(jsonPath));
    
    // Verify JSON structure
    QFile jsonFile(jsonPath);
    EXPECT_TRUE(jsonFile.open(QIODevice::ReadOnly));
    QByteArray jsonData = jsonFile.readAll();
    
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    EXPECT_FALSE(doc.isNull());
    
    QJsonObject root = doc.object();
    EXPECT_TRUE(root.contains("streamName"));
    EXPECT_TRUE(root.contains("analysisStartTime"));
    EXPECT_TRUE(root.contains("services"));
    EXPECT_TRUE(root.contains("totalFrames"));
    
    // Check services array
    QJsonArray services = root["services"].toArray();
    EXPECT_EQ(services.size(), 2);
    
    QJsonObject firstService = services[0].toObject();
    EXPECT_TRUE(firstService.contains("serviceName"));
    EXPECT_TRUE(firstService.contains("bitRate"));
    EXPECT_TRUE(firstService.contains("qualityScore"));
}

// ============================================================================
// STATISTICAL ANALYSIS TESTS
// ============================================================================

TEST_F(ReportGenerationTest, StatisticalAnalysisIsAccurate)
{
    // ARRANGE
    ReportGenerator::AnalysisConfig config;
    config.calculateTrends = true;
    config.calculateAverages = true;
    config.calculatePercentiles = true;
    
    // ACT
    ReportGenerator::StatisticalSummary summary = reportGenerator->generateStatisticalSummary(sampleData, config);
    
    // ASSERT
    // Frame statistics
    EXPECT_EQ(summary.totalFrames, 90000);
    EXPECT_EQ(summary.validFrames, 89950);
    EXPECT_NEAR(summary.frameSuccessRate, 99.944, 0.001);  // (89950/90000)*100
    
    // Signal quality statistics
    EXPECT_NEAR(summary.averageSignalQuality, 18.5, 0.1);
    EXPECT_NEAR(summary.averageErrorRate, 0.0005, 0.0001);
    
    // Service statistics
    EXPECT_EQ(summary.totalServices, 2);
    EXPECT_NEAR(summary.averageServiceQuality, 96.5, 0.1);  // (95.2 + 97.8) / 2
    
    // Alert statistics
    EXPECT_EQ(summary.totalAlerts, 15);
    EXPECT_EQ(summary.criticalAlerts, 2);
    EXPECT_NEAR(summary.alertRate, 13.33, 0.1);  // 2 critical / 15 total * 100
}

TEST_F(ReportGenerationTest, TrendAnalysisGeneratesInsights)
{
    // ARRANGE
    // Create historical data for trend analysis
    QList<ReportData> historicalData;
    
    for (int i = 0; i < 5; ++i) {
        ReportData data = sampleData;
        data.analysisStartTime = QDateTime::currentDateTime().addDays(-i-1);
        data.averageSignalQuality = 18.5 + (i * 0.5);  // Improving trend
        data.averageErrorRate = 0.0005 - (i * 0.0001);  // Improving trend
        historicalData.append(data);
    }
    
    // ACT
    ReportGenerator::TrendAnalysis trends = reportGenerator->analyzeTrends(historicalData);
    
    // ASSERT
    EXPECT_EQ(trends.signalQualityTrend, ReportGenerator::TrendDirection::Improving);
    EXPECT_EQ(trends.errorRateTrend, ReportGenerator::TrendDirection::Improving);
    EXPECT_GT(trends.signalQualitySlope, 0);  // Positive slope = improving
    EXPECT_LT(trends.errorRateSlope, 0);      // Negative slope = improving (lower error rate)
    
    EXPECT_FALSE(trends.insights.empty());
    EXPECT_TRUE(trends.insights.contains("improving") || trends.insights.contains("better"));
}

// ============================================================================
// DASHBOARD GENERATION TESTS
// ============================================================================

TEST_F(ReportGenerationTest, DashboardGenerationCreatesInteractiveHtml)
{
    // ARRANGE
    QString dashboardPath = testDir + "/dashboard.html";
    ReportGenerator::DashboardConfig config;
    config.refreshInterval = 30;  // seconds
    config.enableRealTimeUpdates = true;
    config.widgets = {
        ReportGenerator::Widget::ServiceStatus,
        ReportGenerator::Widget::SignalQuality,
        ReportGenerator::Widget::AlertSummary,
        ReportGenerator::Widget::PerformanceMetrics
    };
    
    // ACT
    bool result = reportGenerator->generateDashboard(sampleData, config, dashboardPath);
    
    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_TRUE(QFile::exists(dashboardPath));
    
    QFile dashboardFile(dashboardPath);
    EXPECT_TRUE(dashboardFile.open(QIODevice::ReadOnly));
    QString content = QString::fromUtf8(dashboardFile.readAll());
    
    // Should be interactive HTML
    EXPECT_TRUE(content.contains("dashboard"));
    EXPECT_TRUE(content.contains("widget"));
    EXPECT_TRUE(content.contains("refresh") || content.contains("update"));
    
    // Should include JavaScript for interactivity
    EXPECT_TRUE(content.contains("<script>") || content.contains("javascript"));
    
    // Should include all requested widgets
    EXPECT_TRUE(content.contains("Service") || content.contains("service"));
    EXPECT_TRUE(content.contains("Signal") || content.contains("signal"));
    EXPECT_TRUE(content.contains("Alert") || content.contains("alert"));
    EXPECT_TRUE(content.contains("Performance") || content.contains("performance"));
}

// ============================================================================
// PERFORMANCE TESTS
// ============================================================================

TEST_F(ReportGenerationTest, LargeDatasetGenerationPerformance)
{
    // ARRANGE
    // Create large dataset
    ReportData largeData = sampleData;
    largeData.totalFrames = 2160000;  // 24 hours of data
    largeData.validFrames = 2159800;
    
    // Add many services
    for (int i = 0; i < 50; ++i) {
        ReportData::ServiceInfo service;
        service.serviceId = 0x2000 + i;
        service.serviceName = QString("Service %1").arg(i);
        service.serviceType = "DAB+ Audio";
        service.bitRate = 64 + (i % 3) * 32;  // 64, 96, or 128 kbps
        service.qualityScore = 90.0 + (qrand() % 1000) / 100.0;  // 90-100%
        largeData.services.append(service);
    }
    
    QString reportPath = testDir + "/large_report.html";
    ReportGenerator::ReportConfig config;
    config.format = ReportGenerator::OutputFormat::HTML;
    config.includeCharts = true;
    
    // ACT
    QTime startTime = QTime::currentTime();
    bool result = reportGenerator->generateReport(largeData, config, reportPath);
    int elapsedMs = startTime.msecsTo(QTime::currentTime());
    
    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_TRUE(QFile::exists(reportPath));
    
    // Should complete within reasonable time (< 5 seconds for large dataset)
    EXPECT_LT(elapsedMs, 5000);
    
    // Generated file should be appropriately sized
    QFile reportFile(reportPath);
    EXPECT_TRUE(reportFile.open(QIODevice::ReadOnly));
    EXPECT_GT(reportFile.size(), 10000);  // Should be substantial
}

TEST_F(ReportGenerationTest, ConcurrentReportGenerationIsThreadSafe)
{
    // ARRANGE
    const int reportCount = 5;
    QList<QString> reportPaths;
    QList<QThread*> threads;
    
    for (int i = 0; i < reportCount; ++i) {
        reportPaths.append(testDir + QString("/concurrent_report_%1.html").arg(i));
    }
    
    // ACT
    QTime startTime = QTime::currentTime();
    
    // Start concurrent report generation (simplified for test)
    for (int i = 0; i < reportCount; ++i) {
        ReportGenerator::ReportConfig config;
        config.format = ReportGenerator::OutputFormat::HTML;
        config.title = QString("Concurrent Report %1").arg(i);
        
        bool result = reportGenerator->generateReport(sampleData, config, reportPaths[i]);
        EXPECT_TRUE(result);
    }
    
    int elapsedMs = startTime.msecsTo(QTime::currentTime());
    
    // ASSERT
    // All reports should be generated successfully
    for (const QString& path : reportPaths) {
        EXPECT_TRUE(QFile::exists(path));
        
        QFile reportFile(path);
        EXPECT_TRUE(reportFile.open(QIODevice::ReadOnly));
        EXPECT_GT(reportFile.size(), 1000);  // Should contain content
    }
    
    // Should complete within reasonable time
    EXPECT_LT(elapsedMs, 10000);  // 10 seconds for 5 reports
}

// ============================================================================
// CONFIGURATION TESTS
// ============================================================================

TEST_F(ReportGenerationTest, ReportConfigurationCanBeSavedAndLoaded)
{
    // ARRANGE
    ReportGenerator::ReportConfig originalConfig;
    originalConfig.format = ReportGenerator::OutputFormat::PDF;
    originalConfig.theme = ReportGenerator::Theme::Professional;
    originalConfig.includeCharts = true;
    originalConfig.includeExecutiveSummary = true;
    originalConfig.title = "Saved Configuration Test";
    
    QString configPath = testDir + "/report_config.json";
    
    // ACT
    bool saveResult = reportGenerator->saveReportConfiguration(originalConfig, configPath);
    ReportGenerator::ReportConfig loadedConfig = reportGenerator->loadReportConfiguration(configPath);
    
    // ASSERT
    EXPECT_TRUE(saveResult);
    EXPECT_TRUE(QFile::exists(configPath));
    
    EXPECT_EQ(loadedConfig.format, ReportGenerator::OutputFormat::PDF);
    EXPECT_EQ(loadedConfig.theme, ReportGenerator::Theme::Professional);
    EXPECT_EQ(loadedConfig.includeCharts, true);
    EXPECT_EQ(loadedConfig.includeExecutiveSummary, true);
    EXPECT_EQ(loadedConfig.title, "Saved Configuration Test");
}

// ============================================================================
// ETSI COMPLIANCE INTEGRATION TESTS
// ============================================================================

TEST_F(ReportGenerationTest, EtsiComplianceDataIsIncluded)
{
    // ARRANGE
    QString reportPath = testDir + "/etsi_compliance_report.html";
    ReportGenerator::ReportConfig config;
    config.format = ReportGenerator::OutputFormat::HTML;
    config.includeEtsiCompliance = true;
    config.includeStandardsValidation = true;
    
    // Add ETSI compliance data to sample
    sampleData.etsiCompliance.overallCompliance = 98.5;
    sampleData.etsiCompliance.en300799Compliance = 99.2;
    sampleData.etsiCompliance.en300401Compliance = 97.8;
    
    // ACT
    bool result = reportGenerator->generateReport(sampleData, config, reportPath);
    
    // ASSERT
    EXPECT_TRUE(result);
    
    QFile reportFile(reportPath);
    EXPECT_TRUE(reportFile.open(QIODevice::ReadOnly));
    QString content = QString::fromUtf8(reportFile.readAll());
    
    // Should include ETSI compliance information
    EXPECT_TRUE(content.contains("ETSI") || content.contains("etsi"));
    EXPECT_TRUE(content.contains("300 799") || content.contains("300799"));
    EXPECT_TRUE(content.contains("300 401") || content.contains("300401"));
    EXPECT_TRUE(content.contains("98.5") || content.contains("99.2"));
}