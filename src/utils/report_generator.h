#pragma once

#include <QObject>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantHash>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <memory>

/**
 * @class ReportGenerator
 * @brief Professional Report Generation System for ETI Stream Analyser
 * 
 * Comprehensive report generation with PDF, HTML, CSV, JSON, and XML export
 * capabilities for broadcast industry standards and ETSI compliance reporting.
 * 
 * Features:
 * - Multiple output formats (HTML, PDF, CSV, JSON, XML)
 * - Professional templates with broadcast industry styling
 * - Statistical analysis and trending capabilities
 * - Interactive dashboards with real-time updates
 * - ETSI compliance reporting integration
 * - Performance-optimized for large datasets
 * - Thread-safe operation for concurrent generation
 * 
 * TDD Implementation:
 * - All features implemented using Test-Driven Development
 * - Comprehensive test coverage for reliability
 * - Performance-validated for production use
 */
class ReportGenerator : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Output format enumeration
     */
    enum class OutputFormat {
        HTML = 0,       // Interactive HTML reports
        PDF = 1,        // Professional PDF documents
        CSV = 2,        // Comma-separated values for data analysis
        JSON = 3,       // JSON format for API integration
        XML = 4         // XML format for structured data exchange
    };
    Q_ENUM(OutputFormat)

    /**
     * @brief Report theme options
     */
    enum class Theme {
        Professional = 0,   // Professional broadcast industry theme
        Modern = 1,         // Modern clean theme
        Classic = 2,        // Classic report theme
        Dark = 3           // Dark theme for monitoring environments
    };
    Q_ENUM(Theme)

    /**
     * @brief Chart type enumeration
     */
    enum class ChartType {
        SignalQuality = 0,      // Signal quality over time
        ServiceStatus = 1,      // Service availability status
        AlertSummary = 2,       // Alert distribution and trends
        PerformanceMetrics = 3, // Performance metrics visualization
        ErrorRates = 4,         // Error rate trends
        AudioQuality = 5        // Audio quality measurements
    };
    Q_ENUM(ChartType)

    /**
     * @brief Dashboard widget types
     */
    enum class Widget {
        ServiceStatus = 0,      // Current service status
        SignalQuality = 1,      // Real-time signal quality
        AlertSummary = 2,       // Active alerts summary
        PerformanceMetrics = 3, // System performance metrics
        AudioLevels = 4,        // Audio level monitoring
        ComplianceStatus = 5    // ETSI compliance status
    };
    Q_ENUM(Widget)

    /**
     * @brief Trend direction enumeration
     */
    enum class TrendDirection {
        Improving = 0,      // Positive trend
        Stable = 1,         // No significant change
        Declining = 2,      // Negative trend
        Volatile = 3        // High variability
    };
    Q_ENUM(TrendDirection)

    /**
     * @brief Report data structure
     */
    struct ReportData {
        // Stream information
        QString streamName;
        QDateTime analysisStartTime;
        QDateTime analysisEndTime;
        
        // Frame statistics
        quint64 totalFrames = 0;
        quint64 validFrames = 0;
        quint64 invalidFrames = 0;
        
        // Quality metrics
        double averageSignalQuality = 0.0;  // dB SNR
        double averageErrorRate = 0.0;      // BER
        
        // Service information
        struct ServiceInfo {
            quint16 serviceId = 0;
            QString serviceName;
            QString serviceType;
            quint32 bitRate = 0;           // kbps
            double qualityScore = 0.0;     // %
        };
        QList<ServiceInfo> services;
        
        // Alert statistics
        quint32 totalAlerts = 0;
        quint32 criticalAlerts = 0;
        quint32 warningAlerts = 0;
        quint32 infoAlerts = 0;
        
        // ETSI compliance data
        struct EtsiCompliance {
            double overallCompliance = 0.0;   // %
            double en300799Compliance = 0.0;  // ETI standard compliance
            double en300401Compliance = 0.0;  // DAB standard compliance
        } etsiCompliance;
    };

    /**
     * @brief Report configuration structure
     */
    struct ReportConfig {
        OutputFormat format = OutputFormat::HTML;
        Theme theme = Theme::Professional;
        QString title = "ETI Stream Analysis Report";
        
        // Content options
        bool includeCharts = true;
        bool includeDetailedAnalysis = true;
        bool includeExecutiveSummary = true;
        bool includeTableOfContents = false;
        bool includePageNumbers = true;
        bool includeHeader = true;
        bool includeFooter = true;
        bool includeEtsiCompliance = true;
        bool includeStandardsValidation = true;
        
        // Chart configuration
        QList<ChartType> chartTypes;
        
        ReportConfig() {
            chartTypes = {ChartType::SignalQuality, ChartType::ServiceStatus, ChartType::AlertSummary};
        }
    };

    /**
     * @brief Export configuration structure
     */
    struct ExportConfig {
        OutputFormat format = OutputFormat::CSV;
        bool includeServiceData = true;
        bool includeAlertData = true;
        bool includeQualityMetrics = true;
        bool prettyPrint = false;  // For JSON/XML formats
    };

    /**
     * @brief Dashboard configuration structure
     */
    struct DashboardConfig {
        quint32 refreshInterval = 30;      // seconds
        bool enableRealTimeUpdates = true;
        QList<Widget> widgets;
        
        DashboardConfig() {
            widgets = {Widget::ServiceStatus, Widget::SignalQuality, Widget::AlertSummary};
        }
    };

    /**
     * @brief Statistical summary structure
     */
    struct StatisticalSummary {
        // Frame statistics
        quint64 totalFrames = 0;
        quint64 validFrames = 0;
        double frameSuccessRate = 0.0;     // %
        
        // Quality statistics
        double averageSignalQuality = 0.0;
        double averageErrorRate = 0.0;
        
        // Service statistics
        quint32 totalServices = 0;
        double averageServiceQuality = 0.0;
        
        // Alert statistics
        quint32 totalAlerts = 0;
        quint32 criticalAlerts = 0;
        double alertRate = 0.0;            // %
    };

    /**
     * @brief Analysis configuration structure
     */
    struct AnalysisConfig {
        bool calculateTrends = true;
        bool calculateAverages = true;
        bool calculatePercentiles = true;
    };

    /**
     * @brief Trend analysis structure
     */
    struct TrendAnalysis {
        TrendDirection signalQualityTrend = TrendDirection::Stable;
        TrendDirection errorRateTrend = TrendDirection::Stable;
        double signalQualitySlope = 0.0;
        double errorRateSlope = 0.0;
        QString insights;
    };

    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit ReportGenerator(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~ReportGenerator();

    /**
     * @brief Initialize the report generator
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * @brief Check if report generator is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return m_initialized; }

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Set output directory for generated reports
     * @param directory Directory path
     * @return true if directory is valid and writable
     */
    bool setOutputDirectory(const QString& directory);

    /**
     * @brief Get current output directory
     * @return Output directory path
     */
    QString getOutputDirectory() const { return m_outputDirectory; }

    /**
     * @brief Set default output format
     * @param format Default format for reports
     */
    void setDefaultFormat(OutputFormat format) { m_defaultFormat = format; }

    /**
     * @brief Get default output format
     * @return Default format
     */
    OutputFormat getDefaultFormat() const { return m_defaultFormat; }

    /**
     * @brief Get list of supported output formats
     * @return List of format names
     */
    QStringList getSupportedFormats() const;

    // ========================================================================
    // REPORT GENERATION
    // ========================================================================

    /**
     * @brief Generate report with specified configuration
     * @param data Report data
     * @param config Report configuration
     * @param outputPath Output file path
     * @return true if generation successful
     */
    bool generateReport(const ReportData& data, const ReportConfig& config, const QString& outputPath);

    /**
     * @brief Export data in specified format
     * @param data Report data
     * @param config Export configuration
     * @param outputPath Output file path
     * @return true if export successful
     */
    bool exportData(const ReportData& data, const ExportConfig& config, const QString& outputPath);

    /**
     * @brief Generate interactive dashboard
     * @param data Report data
     * @param config Dashboard configuration
     * @param outputPath Output file path
     * @return true if generation successful
     */
    bool generateDashboard(const ReportData& data, const DashboardConfig& config, const QString& outputPath);

    // ========================================================================
    // STATISTICAL ANALYSIS
    // ========================================================================

    /**
     * @brief Generate statistical summary from report data
     * @param data Report data
     * @param config Analysis configuration
     * @return Statistical summary
     */
    StatisticalSummary generateStatisticalSummary(const ReportData& data, const AnalysisConfig& config);

    /**
     * @brief Analyze trends from historical data
     * @param historicalData List of historical report data
     * @return Trend analysis results
     */
    TrendAnalysis analyzeTrends(const QList<ReportData>& historicalData);

    // ========================================================================
    // CONFIGURATION PERSISTENCE
    // ========================================================================

    /**
     * @brief Save report configuration to file
     * @param config Configuration to save
     * @param filePath Configuration file path
     * @return true if saved successfully
     */
    bool saveReportConfiguration(const ReportConfig& config, const QString& filePath);

    /**
     * @brief Load report configuration from file
     * @param filePath Configuration file path
     * @return Loaded configuration
     */
    ReportConfig loadReportConfiguration(const QString& filePath);

signals:
    /**
     * @brief Emitted when report generation starts
     * @param reportType Type of report being generated
     */
    void reportGenerationStarted(const QString& reportType);

    /**
     * @brief Emitted when report generation completes
     * @param outputPath Path to generated report
     */
    void reportGenerationCompleted(const QString& outputPath);

    /**
     * @brief Emitted when report generation fails
     * @param errorMessage Error description
     */
    void reportGenerationFailed(const QString& errorMessage);

    /**
     * @brief Emitted to indicate generation progress
     * @param percentage Progress percentage (0-100)
     */
    void generationProgress(int percentage);

private:
    /**
     * @brief Generate HTML report
     * @param data Report data
     * @param config Report configuration
     * @param outputPath Output file path
     * @return true if successful
     */
    bool generateHtmlReport(const ReportData& data, const ReportConfig& config, const QString& outputPath);

    /**
     * @brief Generate PDF report
     * @param data Report data
     * @param config Report configuration
     * @param outputPath Output file path
     * @return true if successful
     */
    bool generatePdfReport(const ReportData& data, const ReportConfig& config, const QString& outputPath);

    /**
     * @brief Generate CSV export
     * @param data Report data
     * @param config Export configuration
     * @param outputPath Output file path
     * @return true if successful
     */
    bool generateCsvExport(const ReportData& data, const ExportConfig& config, const QString& outputPath);

    /**
     * @brief Generate JSON export
     * @param data Report data
     * @param config Export configuration
     * @param outputPath Output file path
     * @return true if successful
     */
    bool generateJsonExport(const ReportData& data, const ExportConfig& config, const QString& outputPath);

    /**
     * @brief Generate XML export
     * @param data Report data
     * @param config Export configuration
     * @param outputPath Output file path
     * @return true if successful
     */
    bool generateXmlExport(const ReportData& data, const ExportConfig& config, const QString& outputPath);

    // HTML generation helpers
    QString generateHtmlHeader(const ReportConfig& config) const;
    QString generateHtmlFooter(const ReportConfig& config) const;
    QString generateHtmlStyles(Theme theme) const;
    QString generateHtmlExecutiveSummary(const ReportData& data) const;
    QString generateHtmlServiceSection(const ReportData& data) const;
    QString generateHtmlQualitySection(const ReportData& data) const;
    QString generateHtmlAlertSection(const ReportData& data) const;
    QString generateHtmlChart(ChartType chartType, const ReportData& data) const;

    // PDF generation helpers
    QString generatePdfContent(const ReportData& data, const ReportConfig& config) const;

    // Widget generation helpers for dashboard
    QString generateServiceStatusWidget(const ReportData& data) const;
    QString generateSignalQualityWidget(const ReportData& data) const;
    QString generateAlertSummaryWidget(const ReportData& data) const;
    QString generatePerformanceMetricsWidget(const ReportData& data) const;
    QString generateAudioLevelsWidget(const ReportData& data) const;
    QString generateComplianceStatusWidget(const ReportData& data) const;

    // Data conversion helpers
    QJsonObject reportDataToJson(const ReportData& data) const;
    QString formatDuration(const QDateTime& start, const QDateTime& end) const;
    QString formatPercentage(double percentage, int decimals = 2) const;
    QString formatNumber(double number, int decimals = 2) const;

    // Validation helpers
    bool validateOutputPath(const QString& path) const;
    bool validateReportData(const ReportData& data) const;

    // Member variables
    bool m_initialized;
    QString m_outputDirectory;
    OutputFormat m_defaultFormat;

    // Constants
    static constexpr int CHART_WIDTH = 800;
    static constexpr int CHART_HEIGHT = 400;
    static constexpr int MAX_SERVICES_IN_CHART = 20;
};

// Register metatypes for Qt signals
Q_DECLARE_METATYPE(ReportGenerator::OutputFormat)
Q_DECLARE_METATYPE(ReportGenerator::Theme)
Q_DECLARE_METATYPE(ReportGenerator::ChartType)
Q_DECLARE_METATYPE(ReportGenerator::ReportData)
Q_DECLARE_METATYPE(ReportGenerator::StatisticalSummary)