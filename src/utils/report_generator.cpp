#include "report_generator.h"
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QTextDocument>
#include <QTextStream>
#include <QPrinter>
#include <QPainter>
#include <QUrl>
#include <QMutexLocker>
#include <QMutex>
#include <algorithm>
#include <cmath>

/**
 * @brief Professional Report Generator Implementation
 * 
 * Comprehensive report generation system with TDD-validated functionality
 * for broadcast industry standards and professional documentation.
 */

ReportGenerator::ReportGenerator(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_defaultFormat(OutputFormat::HTML)
{
    // Set default output directory
    m_outputDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/reports";
    
    // Initialize the system
    initialize();
}

ReportGenerator::~ReportGenerator()
{
}

bool ReportGenerator::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    // Create output directory if it doesn't exist
    QDir outputDir;
    if (!outputDir.mkpath(m_outputDirectory)) {
        qWarning() << "ReportGenerator: Failed to create output directory:" << m_outputDirectory;
        return false;
    }
    
    // Verify directory is writable
    QFileInfo dirInfo(m_outputDirectory);
    if (!dirInfo.isWritable()) {
        qWarning() << "ReportGenerator: Output directory is not writable:" << m_outputDirectory;
        return false;
    }
    
    m_initialized = true;
    qDebug() << "ReportGenerator: Initialized successfully, output directory:" << m_outputDirectory;
    return true;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

bool ReportGenerator::setOutputDirectory(const QString& directory)
{
    QDir dir(directory);
    if (!dir.exists() && !dir.mkpath(directory)) {
        qWarning() << "ReportGenerator: Cannot create directory:" << directory;
        return false;
    }
    
    QFileInfo dirInfo(directory);
    if (!dirInfo.isWritable()) {
        qWarning() << "ReportGenerator: Directory is not writable:" << directory;
        return false;
    }
    
    m_outputDirectory = directory;
    return true;
}

QStringList ReportGenerator::getSupportedFormats() const
{
    return {"HTML", "PDF", "CSV", "JSON", "XML"};
}

// ============================================================================
// REPORT GENERATION
// ============================================================================

bool ReportGenerator::generateReport(const ReportData& data, const ReportConfig& config, const QString& outputPath)
{
    if (!m_initialized) {
        qWarning() << "ReportGenerator: Not initialized";
        return false;
    }
    
    if (!validateReportData(data)) {
        qWarning() << "ReportGenerator: Invalid report data";
        return false;
    }
    
    if (!validateOutputPath(outputPath)) {
        qWarning() << "ReportGenerator: Invalid output path:" << outputPath;
        return false;
    }
    
    emit reportGenerationStarted(config.title);
    emit generationProgress(0);
    
    bool success = false;
    
    try {
        switch (config.format) {
            case OutputFormat::HTML:
                success = generateHtmlReport(data, config, outputPath);
                break;
            case OutputFormat::PDF:
                success = generatePdfReport(data, config, outputPath);
                break;
            case OutputFormat::CSV: {
                ExportConfig exportConfig;
                exportConfig.format = OutputFormat::CSV;
                success = generateCsvExport(data, exportConfig, outputPath);
                break;
            }
            case OutputFormat::JSON: {
                ExportConfig exportConfig;
                exportConfig.format = OutputFormat::JSON;
                success = generateJsonExport(data, exportConfig, outputPath);
                break;
            }
            case OutputFormat::XML: {
                ExportConfig exportConfig;
                exportConfig.format = OutputFormat::XML;
                success = generateXmlExport(data, exportConfig, outputPath);
                break;
            }
            default:
                qWarning() << "ReportGenerator: Unsupported format";
                success = false;
                break;
        }
        
        emit generationProgress(100);
        
        if (success) {
            emit reportGenerationCompleted(outputPath);
        } else {
            emit reportGenerationFailed("Report generation failed");
        }
        
    } catch (const std::exception& e) {
        qWarning() << "ReportGenerator: Exception during generation:" << e.what();
        emit reportGenerationFailed(QString("Exception: %1").arg(e.what()));
        success = false;
    }
    
    return success;
}

bool ReportGenerator::exportData(const ReportData& data, const ExportConfig& config, const QString& outputPath)
{
    if (!m_initialized) {
        return false;
    }
    
    switch (config.format) {
        case OutputFormat::CSV:
            return generateCsvExport(data, config, outputPath);
        case OutputFormat::JSON:
            return generateJsonExport(data, config, outputPath);
        case OutputFormat::XML:
            return generateXmlExport(data, config, outputPath);
        default:
            return false;
    }
}

bool ReportGenerator::generateDashboard(const ReportData& data, const DashboardConfig& config, const QString& outputPath)
{
    if (!m_initialized) {
        return false;
    }
    
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream stream(&file);
    // Note: setCodec is no longer needed in Qt6 - UTF-8 is default
    
    // Generate dashboard HTML
    stream << "<!DOCTYPE html>\n<html>\n<head>\n";
    stream << "<title>ETI Stream Analyser Dashboard</title>\n";
    stream << "<meta charset=\"UTF-8\">\n";
    stream << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    
    // Include Chart.js for interactive charts
    stream << "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>\n";
    
    // Dashboard styles
    stream << "<style>\n";
    stream << "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f5f5f5; }\n";
    stream << ".dashboard { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; }\n";
    stream << ".widget { background: white; border-radius: 8px; padding: 20px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
    stream << ".widget h3 { margin-top: 0; color: #333; border-bottom: 2px solid #007acc; padding-bottom: 10px; }\n";
    stream << ".metric { display: flex; justify-content: space-between; margin: 10px 0; }\n";
    stream << ".metric-value { font-weight: bold; color: #007acc; }\n";
    stream << ".status-good { color: #28a745; }\n";
    stream << ".status-warning { color: #ffc107; }\n";
    stream << ".status-error { color: #dc3545; }\n";
    stream << "</style>\n";
    stream << "</head>\n<body>\n";
    
    stream << "<h1>ETI Stream Analyser - Real-time Dashboard</h1>\n";
    stream << "<p>Stream: " << data.streamName.toHtmlEscaped() << " | ";
    stream << "Last Updated: <span id=\"lastUpdate\">" << QDateTime::currentDateTime().toString() << "</span></p>\n";
    
    stream << "<div class=\"dashboard\">\n";
    
    // Generate widgets based on configuration
    for (Widget widget : config.widgets) {
        switch (widget) {
            case Widget::ServiceStatus:
                stream << generateServiceStatusWidget(data);
                break;
            case Widget::SignalQuality:
                stream << generateSignalQualityWidget(data);
                break;
            case Widget::AlertSummary:
                stream << generateAlertSummaryWidget(data);
                break;
            case Widget::PerformanceMetrics:
                stream << generatePerformanceMetricsWidget(data);
                break;
            case Widget::AudioLevels:
                stream << generateAudioLevelsWidget(data);
                break;
            case Widget::ComplianceStatus:
                stream << generateComplianceStatusWidget(data);
                break;
        }
    }
    
    stream << "</div>\n";
    
    // Auto-refresh script if enabled
    if (config.enableRealTimeUpdates) {
        stream << "<script>\n";
        stream << "setInterval(function() {\n";
        stream << "  document.getElementById('lastUpdate').textContent = new Date().toLocaleString();\n";
        stream << "  // Add AJAX refresh logic here\n";
        stream << "}, " << (config.refreshInterval * 1000) << ");\n";
        stream << "</script>\n";
    }
    
    stream << "</body>\n</html>\n";
    
    return true;
}

// ============================================================================
// STATISTICAL ANALYSIS
// ============================================================================

ReportGenerator::StatisticalSummary ReportGenerator::generateStatisticalSummary(const ReportData& data, const AnalysisConfig& config)
{
    Q_UNUSED(config)
    StatisticalSummary summary;
    
    // Frame statistics
    summary.totalFrames = data.totalFrames;
    summary.validFrames = data.validFrames;
    if (data.totalFrames > 0) {
        summary.frameSuccessRate = (static_cast<double>(data.validFrames) / data.totalFrames) * 100.0;
    }
    
    // Quality statistics
    summary.averageSignalQuality = data.averageSignalQuality;
    summary.averageErrorRate = data.averageErrorRate;
    
    // Service statistics
    summary.totalServices = data.services.size();
    if (!data.services.isEmpty()) {
        double totalQuality = 0.0;
        for (const ReportData::ServiceInfo& service : data.services) {
            totalQuality += service.qualityScore;
        }
        summary.averageServiceQuality = totalQuality / data.services.size();
    }
    
    // Alert statistics
    summary.totalAlerts = data.totalAlerts;
    summary.criticalAlerts = data.criticalAlerts;
    if (data.totalAlerts > 0) {
        summary.alertRate = (static_cast<double>(data.criticalAlerts) / data.totalAlerts) * 100.0;
    }
    
    return summary;
}

ReportGenerator::TrendAnalysis ReportGenerator::analyzeTrends(const QList<ReportData>& historicalData)
{
    TrendAnalysis trends;
    
    if (historicalData.size() < 2) {
        return trends; // Need at least 2 data points for trend analysis
    }
    
    // Calculate signal quality trend
    QList<double> signalQualities;
    QList<double> errorRates;
    
    for (const ReportData& data : historicalData) {
        signalQualities.append(data.averageSignalQuality);
        errorRates.append(data.averageErrorRate);
    }
    
    // Simple linear regression for trend analysis
    auto calculateSlope = [](const QList<double>& values) -> double {
        if (values.size() < 2) return 0.0;
        
        double n = values.size();
        double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumX2 = 0.0;
        
        for (int i = 0; i < values.size(); ++i) {
            sumX += i;
            sumY += values[i];
            sumXY += i * values[i];
            sumX2 += i * i;
        }
        
        return (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
    };
    
    trends.signalQualitySlope = calculateSlope(signalQualities);
    trends.errorRateSlope = calculateSlope(errorRates);
    
    // Determine trend directions
    const double threshold = 0.1; // Minimum slope to consider significant
    
    if (trends.signalQualitySlope > threshold) {
        trends.signalQualityTrend = TrendDirection::Improving;
    } else if (trends.signalQualitySlope < -threshold) {
        trends.signalQualityTrend = TrendDirection::Declining;
    } else {
        trends.signalQualityTrend = TrendDirection::Stable;
    }
    
    if (trends.errorRateSlope > threshold) {
        trends.errorRateTrend = TrendDirection::Declining; // Higher error rate = declining
    } else if (trends.errorRateSlope < -threshold) {
        trends.errorRateTrend = TrendDirection::Improving; // Lower error rate = improving
    } else {
        trends.errorRateTrend = TrendDirection::Stable;
    }
    
    // Generate insights
    QStringList insights;
    if (trends.signalQualityTrend == TrendDirection::Improving) {
        insights << "Signal quality is improving over time";
    } else if (trends.signalQualityTrend == TrendDirection::Declining) {
        insights << "Signal quality shows declining trend - investigation recommended";
    }
    
    if (trends.errorRateTrend == TrendDirection::Improving) {
        insights << "Error rates are decreasing - system performance improving";
    } else if (trends.errorRateTrend == TrendDirection::Declining) {
        insights << "Error rates are increasing - attention required";
    }
    
    trends.insights = insights.join(". ");
    
    return trends;
}

// ============================================================================
// CONFIGURATION PERSISTENCE
// ============================================================================

bool ReportGenerator::saveReportConfiguration(const ReportConfig& config, const QString& filePath)
{
    QJsonObject configObj;
    configObj["format"] = static_cast<int>(config.format);
    configObj["theme"] = static_cast<int>(config.theme);
    configObj["title"] = config.title;
    configObj["includeCharts"] = config.includeCharts;
    configObj["includeDetailedAnalysis"] = config.includeDetailedAnalysis;
    configObj["includeExecutiveSummary"] = config.includeExecutiveSummary;
    configObj["includeTableOfContents"] = config.includeTableOfContents;
    configObj["includePageNumbers"] = config.includePageNumbers;
    configObj["includeHeader"] = config.includeHeader;
    configObj["includeFooter"] = config.includeFooter;
    configObj["includeEtsiCompliance"] = config.includeEtsiCompliance;
    configObj["includeStandardsValidation"] = config.includeStandardsValidation;
    
    QJsonArray chartTypesArray;
    for (ChartType chartType : config.chartTypes) {
        chartTypesArray.append(static_cast<int>(chartType));
    }
    configObj["chartTypes"] = chartTypesArray;
    
    QJsonDocument doc(configObj);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(doc.toJson());
    return true;
}

ReportGenerator::ReportConfig ReportGenerator::loadReportConfiguration(const QString& filePath)
{
    ReportConfig config; // Default configuration
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return config;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) {
        return config;
    }
    
    QJsonObject configObj = doc.object();
    
    if (configObj.contains("format")) {
        config.format = static_cast<OutputFormat>(configObj["format"].toInt());
    }
    if (configObj.contains("theme")) {
        config.theme = static_cast<Theme>(configObj["theme"].toInt());
    }
    if (configObj.contains("title")) {
        config.title = configObj["title"].toString();
    }
    if (configObj.contains("includeCharts")) {
        config.includeCharts = configObj["includeCharts"].toBool();
    }
    if (configObj.contains("includeDetailedAnalysis")) {
        config.includeDetailedAnalysis = configObj["includeDetailedAnalysis"].toBool();
    }
    if (configObj.contains("includeExecutiveSummary")) {
        config.includeExecutiveSummary = configObj["includeExecutiveSummary"].toBool();
    }
    if (configObj.contains("includeTableOfContents")) {
        config.includeTableOfContents = configObj["includeTableOfContents"].toBool();
    }
    if (configObj.contains("includePageNumbers")) {
        config.includePageNumbers = configObj["includePageNumbers"].toBool();
    }
    if (configObj.contains("includeHeader")) {
        config.includeHeader = configObj["includeHeader"].toBool();
    }
    if (configObj.contains("includeFooter")) {
        config.includeFooter = configObj["includeFooter"].toBool();
    }
    if (configObj.contains("includeEtsiCompliance")) {
        config.includeEtsiCompliance = configObj["includeEtsiCompliance"].toBool();
    }
    if (configObj.contains("includeStandardsValidation")) {
        config.includeStandardsValidation = configObj["includeStandardsValidation"].toBool();
    }
    
    if (configObj.contains("chartTypes")) {
        config.chartTypes.clear();
        QJsonArray chartTypesArray = configObj["chartTypes"].toArray();
        for (const QJsonValue& value : chartTypesArray) {
            config.chartTypes.append(static_cast<ChartType>(value.toInt()));
        }
    }
    
    return config;
}

// ============================================================================
// PRIVATE IMPLEMENTATION
// ============================================================================

bool ReportGenerator::generateHtmlReport(const ReportData& data, const ReportConfig& config, const QString& outputPath)
{
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream stream(&file);
    // Note: setCodec is no longer needed in Qt6 - UTF-8 is default
    
    emit generationProgress(10);
    
    // Generate HTML content
    stream << generateHtmlHeader(config);
    
    emit generationProgress(20);
    
    if (config.includeExecutiveSummary) {
        stream << generateHtmlExecutiveSummary(data);
    }
    
    emit generationProgress(40);
    
    stream << generateHtmlServiceSection(data);
    
    emit generationProgress(60);
    
    stream << generateHtmlQualitySection(data);
    
    emit generationProgress(80);
    
    stream << generateHtmlAlertSection(data);
    
    if (config.includeCharts) {
        for (ChartType chartType : config.chartTypes) {
            stream << generateHtmlChart(chartType, data);
        }
    }
    
    emit generationProgress(90);
    
    stream << generateHtmlFooter(config);
    
    return true;
}

bool ReportGenerator::generatePdfReport(const ReportData& data, const ReportConfig& config, const QString& outputPath)
{
    // Generate HTML first, then convert to PDF
    QString htmlContent = generatePdfContent(data, config);
    
    QTextDocument document;
    document.setHtml(htmlContent);
    
    QPrinter printer;
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(outputPath);
    printer.setPageSize(QPageSize::A4);
    printer.setPageMargins(QMarginsF(20, 20, 20, 20), QPageLayout::Millimeter);
    
    document.print(&printer);
    
    return QFile::exists(outputPath);
}

bool ReportGenerator::generateCsvExport(const ReportData& data, const ExportConfig& config, const QString& outputPath)
{
    Q_UNUSED(config)
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream stream(&file);
    // Note: setCodec is no longer needed in Qt6 - UTF-8 is default
    
    // CSV header
    QStringList headers;
    headers << "Service ID" << "Service Name" << "Service Type" << "Bit Rate (kbps)" << "Quality Score (%)";
    stream << headers.join(",") << "\n";
    
    // Service data
    for (const ReportData::ServiceInfo& service : data.services) {
        QStringList values;
        values << QString::number(service.serviceId);
        values << QString("\"%1\"").arg(service.serviceName);
        values << QString("\"%1\"").arg(service.serviceType);
        values << QString::number(service.bitRate);
        values << QString::number(service.qualityScore, 'f', 2);
        stream << values.join(",") << "\n";
    }
    
    return true;
}

bool ReportGenerator::generateJsonExport(const ReportData& data, const ExportConfig& config, const QString& outputPath)
{
    QJsonObject jsonData = reportDataToJson(data);
    
    QJsonDocument doc(jsonData);
    
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    if (config.prettyPrint) {
        file.write(doc.toJson());
    } else {
        file.write(doc.toJson(QJsonDocument::Compact));
    }
    
    return true;
}

bool ReportGenerator::generateXmlExport(const ReportData& data, const ExportConfig& config, const QString& outputPath)
{
    Q_UNUSED(config)
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream stream(&file);
    // Note: setCodec is no longer needed in Qt6 - UTF-8 is default
    
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    stream << "<eti_report>\n";
    stream << "  <stream_info>\n";
    stream << "    <name>" << data.streamName.toHtmlEscaped() << "</name>\n";
    stream << "    <start_time>" << data.analysisStartTime.toString(Qt::ISODate) << "</start_time>\n";
    stream << "    <end_time>" << data.analysisEndTime.toString(Qt::ISODate) << "</end_time>\n";
    stream << "  </stream_info>\n";
    
    stream << "  <frame_statistics>\n";
    stream << "    <total_frames>" << data.totalFrames << "</total_frames>\n";
    stream << "    <valid_frames>" << data.validFrames << "</valid_frames>\n";
    stream << "    <invalid_frames>" << data.invalidFrames << "</invalid_frames>\n";
    stream << "  </frame_statistics>\n";
    
    stream << "  <services>\n";
    for (const ReportData::ServiceInfo& service : data.services) {
        stream << "    <service>\n";
        stream << "      <id>" << service.serviceId << "</id>\n";
        stream << "      <name>" << service.serviceName.toHtmlEscaped() << "</name>\n";
        stream << "      <type>" << service.serviceType.toHtmlEscaped() << "</type>\n";
        stream << "      <bit_rate>" << service.bitRate << "</bit_rate>\n";
        stream << "      <quality_score>" << service.qualityScore << "</quality_score>\n";
        stream << "    </service>\n";
    }
    stream << "  </services>\n";
    
    stream << "</eti_report>\n";
    
    return true;
}

// ============================================================================
// HTML GENERATION HELPERS
// ============================================================================

QString ReportGenerator::generateHtmlHeader(const ReportConfig& config) const
{
    QString html = "<!DOCTYPE html>\n<html>\n<head>\n";
    html += "<meta charset=\"UTF-8\">\n";
    html += "<title>" + config.title.toHtmlEscaped() + "</title>\n";
    html += generateHtmlStyles(config.theme);
    html += "</head>\n<body>\n";
    
    if (config.includeHeader) {
        html += "<header>\n";
        html += "<h1>" + config.title.toHtmlEscaped() + "</h1>\n";
        html += "<p>Generated: " + QDateTime::currentDateTime().toString() + "</p>\n";
        html += "</header>\n";
    }
    
    return html;
}

QString ReportGenerator::generateHtmlFooter(const ReportConfig& config) const
{
    QString html;
    
    if (config.includeFooter) {
        html += "<footer>\n";
        html += "<p>Generated by ETI Stream Analyser</p>\n";
        html += "<p>Report generated on " + QDateTime::currentDateTime().toString() + "</p>\n";
        html += "</footer>\n";
    }
    
    html += "</body>\n</html>\n";
    return html;
}

QString ReportGenerator::generateHtmlStyles(Theme theme) const
{
    QString styles = "<style>\n";
    styles += "body { font-family: Arial, sans-serif; margin: 40px; line-height: 1.6; }\n";
    styles += "h1, h2, h3 { color: #333; }\n";
    styles += "table { border-collapse: collapse; width: 100%; margin: 20px 0; }\n";
    styles += "th, td { border: 1px solid #ddd; padding: 12px; text-align: left; }\n";
    styles += "th { background-color: #f2f2f2; font-weight: bold; }\n";
    styles += "tr:nth-child(even) { background-color: #f9f9f9; }\n";
    styles += ".metric { margin: 10px 0; }\n";
    styles += ".metric-label { font-weight: bold; }\n";
    styles += ".metric-value { color: #007acc; }\n";
    styles += ".section { margin: 30px 0; }\n";
    
    switch (theme) {
        case Theme::Professional:
            styles += "body { background-color: #fafafa; }\n";
            styles += "h1 { color: #1e3a8a; border-bottom: 3px solid #007acc; }\n";
            break;
        case Theme::Dark:
            styles += "body { background-color: #2d3748; color: #e2e8f0; }\n";
            styles += "table { background-color: #4a5568; }\n";
            break;
        default:
            break;
    }
    
    styles += "</style>\n";
    return styles;
}

QString ReportGenerator::generateHtmlExecutiveSummary(const ReportData& data) const
{
    QString html = "<div class=\"section\">\n";
    html += "<h2>Executive Summary</h2>\n";
    html += "<p>Stream: <strong>" + data.streamName.toHtmlEscaped() + "</strong></p>\n";
    html += "<p>Analysis Period: " + formatDuration(data.analysisStartTime, data.analysisEndTime) + "</p>\n";
    
    if (data.totalFrames > 0) {
        double successRate = (static_cast<double>(data.validFrames) / data.totalFrames) * 100.0;
        html += "<p>Frame Success Rate: <strong>" + formatPercentage(successRate) + "</strong></p>\n";
    }
    
    html += "<p>Average Signal Quality: <strong>" + formatNumber(data.averageSignalQuality) + " dB SNR</strong></p>\n";
    html += "<p>Average Error Rate: <strong>" + formatNumber(data.averageErrorRate * 100, 4) + "%</strong></p>\n";
    html += "</div>\n";
    
    return html;
}

QString ReportGenerator::generateHtmlServiceSection(const ReportData& data) const
{
    QString html = "<div class=\"section\">\n";
    html += "<h2>Service Information</h2>\n";
    html += "<table>\n";
    html += "<tr><th>Service ID</th><th>Name</th><th>Type</th><th>Bit Rate (kbps)</th><th>Quality Score (%)</th></tr>\n";
    
    for (const ReportData::ServiceInfo& service : data.services) {
        html += "<tr>";
        html += "<td>" + QString::number(service.serviceId, 16).toUpper() + "</td>";
        html += "<td>" + service.serviceName.toHtmlEscaped() + "</td>";
        html += "<td>" + service.serviceType.toHtmlEscaped() + "</td>";
        html += "<td>" + QString::number(service.bitRate) + "</td>";
        html += "<td>" + formatNumber(service.qualityScore) + "</td>";
        html += "</tr>\n";
    }
    
    html += "</table>\n";
    html += "</div>\n";
    
    return html;
}

QString ReportGenerator::generateHtmlQualitySection(const ReportData& data) const
{
    QString html = "<div class=\"section\">\n";
    html += "<h2>Quality Metrics</h2>\n";
    html += "<div class=\"metric\">Signal Quality: <span class=\"metric-value\">" + formatNumber(data.averageSignalQuality) + " dB SNR</span></div>\n";
    html += "<div class=\"metric\">Error Rate: <span class=\"metric-value\">" + formatNumber(data.averageErrorRate * 100, 4) + "%</span></div>\n";
    html += "</div>\n";
    
    return html;
}

QString ReportGenerator::generateHtmlAlertSection(const ReportData& data) const
{
    QString html = "<div class=\"section\">\n";
    html += "<h2>Alert Summary</h2>\n";
    html += "<div class=\"metric\">Total Alerts: <span class=\"metric-value\">" + QString::number(data.totalAlerts) + "</span></div>\n";
    html += "<div class=\"metric\">Critical Alerts: <span class=\"metric-value\">" + QString::number(data.criticalAlerts) + "</span></div>\n";
    html += "<div class=\"metric\">Warning Alerts: <span class=\"metric-value\">" + QString::number(data.warningAlerts) + "</span></div>\n";
    html += "<div class=\"metric\">Info Alerts: <span class=\"metric-value\">" + QString::number(data.infoAlerts) + "</span></div>\n";
    html += "</div>\n";
    
    return html;
}

QString ReportGenerator::generateHtmlChart(ChartType chartType, const ReportData& data) const
{
    Q_UNUSED(data)
    QString html = "<div class=\"section\">\n";
    
    switch (chartType) {
        case ChartType::SignalQuality:
            html += "<h3>Signal Quality Chart</h3>\n";
            html += "<canvas id=\"signalQualityChart\" width=\"800\" height=\"400\"></canvas>\n";
            break;
        case ChartType::ServiceStatus:
            html += "<h3>Service Status Chart</h3>\n";
            html += "<canvas id=\"serviceStatusChart\" width=\"800\" height=\"400\"></canvas>\n";
            break;
        case ChartType::AlertSummary:
            html += "<h3>Alert Summary Chart</h3>\n";
            html += "<canvas id=\"alertSummaryChart\" width=\"800\" height=\"400\"></canvas>\n";
            break;
        default:
            html += "<p>Chart placeholder for " + QString::number(static_cast<int>(chartType)) + "</p>\n";
            break;
    }
    
    html += "</div>\n";
    return html;
}

// ============================================================================
// DASHBOARD WIDGET GENERATORS
// ============================================================================

QString ReportGenerator::generateServiceStatusWidget(const ReportData& data) const
{
    QString html = "<div class=\"widget\">\n";
    html += "<h3>Service Status</h3>\n";
    html += "<div class=\"metric\">Total Services: <span class=\"metric-value\">" + QString::number(data.services.size()) + "</span></div>\n";
    
    for (const ReportData::ServiceInfo& service : data.services) {
        QString statusClass = service.qualityScore > 95 ? "status-good" : 
                             service.qualityScore > 85 ? "status-warning" : "status-error";
        html += "<div class=\"metric\">";
        html += service.serviceName.toHtmlEscaped() + ": ";
        html += "<span class=\"metric-value " + statusClass + "\">" + formatNumber(service.qualityScore) + "%</span>";
        html += "</div>\n";
    }
    
    html += "</div>\n";
    return html;
}

QString ReportGenerator::generateSignalQualityWidget(const ReportData& data) const
{
    QString html = "<div class=\"widget\">\n";
    html += "<h3>Signal Quality</h3>\n";
    html += "<div class=\"metric\">SNR: <span class=\"metric-value\">" + formatNumber(data.averageSignalQuality) + " dB</span></div>\n";
    html += "<div class=\"metric\">BER: <span class=\"metric-value\">" + formatNumber(data.averageErrorRate * 100, 4) + "%</span></div>\n";
    html += "</div>\n";
    return html;
}

QString ReportGenerator::generateAlertSummaryWidget(const ReportData& data) const
{
    QString html = "<div class=\"widget\">\n";
    html += "<h3>Alert Summary</h3>\n";
    html += "<div class=\"metric\">Critical: <span class=\"metric-value status-error\">" + QString::number(data.criticalAlerts) + "</span></div>\n";
    html += "<div class=\"metric\">Warning: <span class=\"metric-value status-warning\">" + QString::number(data.warningAlerts) + "</span></div>\n";
    html += "<div class=\"metric\">Info: <span class=\"metric-value status-good\">" + QString::number(data.infoAlerts) + "</span></div>\n";
    html += "</div>\n";
    return html;
}

QString ReportGenerator::generatePerformanceMetricsWidget(const ReportData& data) const
{
    QString html = "<div class=\"widget\">\n";
    html += "<h3>Performance Metrics</h3>\n";
    
    if (data.totalFrames > 0) {
        double successRate = (static_cast<double>(data.validFrames) / data.totalFrames) * 100.0;
        html += "<div class=\"metric\">Frame Success Rate: <span class=\"metric-value\">" + formatPercentage(successRate) + "</span></div>\n";
    }
    
    html += "<div class=\"metric\">Total Frames: <span class=\"metric-value\">" + QString::number(data.totalFrames) + "</span></div>\n";
    html += "<div class=\"metric\">Valid Frames: <span class=\"metric-value\">" + QString::number(data.validFrames) + "</span></div>\n";
    html += "</div>\n";
    return html;
}

QString ReportGenerator::generateAudioLevelsWidget(const ReportData& data) const
{
    Q_UNUSED(data)
    QString html = "<div class=\"widget\">\n";
    html += "<h3>Audio Levels</h3>\n";
    html += "<p>Audio level monitoring widget</p>\n";
    html += "</div>\n";
    return html;
}

QString ReportGenerator::generateComplianceStatusWidget(const ReportData& data) const
{
    QString html = "<div class=\"widget\">\n";
    html += "<h3>ETSI Compliance</h3>\n";
    html += "<div class=\"metric\">Overall: <span class=\"metric-value\">" + formatPercentage(data.etsiCompliance.overallCompliance) + "</span></div>\n";
    html += "<div class=\"metric\">EN 300 799: <span class=\"metric-value\">" + formatPercentage(data.etsiCompliance.en300799Compliance) + "</span></div>\n";
    html += "<div class=\"metric\">EN 300 401: <span class=\"metric-value\">" + formatPercentage(data.etsiCompliance.en300401Compliance) + "</span></div>\n";
    html += "</div>\n";
    return html;
}

// ============================================================================
// UTILITY METHODS
// ============================================================================

QString ReportGenerator::generatePdfContent(const ReportData& data, const ReportConfig& config) const
{
    QString html = generateHtmlHeader(config);
    
    if (config.includeExecutiveSummary) {
        html += generateHtmlExecutiveSummary(data);
    }
    
    html += generateHtmlServiceSection(data);
    html += generateHtmlQualitySection(data);
    html += generateHtmlAlertSection(data);
    
    html += generateHtmlFooter(config);
    return html;
}

QJsonObject ReportGenerator::reportDataToJson(const ReportData& data) const
{
    QJsonObject json;
    
    json["streamName"] = data.streamName;
    json["analysisStartTime"] = data.analysisStartTime.toString(Qt::ISODate);
    json["analysisEndTime"] = data.analysisEndTime.toString(Qt::ISODate);
    json["totalFrames"] = static_cast<qint64>(data.totalFrames);
    json["validFrames"] = static_cast<qint64>(data.validFrames);
    json["invalidFrames"] = static_cast<qint64>(data.invalidFrames);
    json["averageSignalQuality"] = data.averageSignalQuality;
    json["averageErrorRate"] = data.averageErrorRate;
    
    QJsonArray servicesArray;
    for (const ReportData::ServiceInfo& service : data.services) {
        QJsonObject serviceObj;
        serviceObj["serviceId"] = service.serviceId;
        serviceObj["serviceName"] = service.serviceName;
        serviceObj["serviceType"] = service.serviceType;
        serviceObj["bitRate"] = static_cast<qint64>(service.bitRate);
        serviceObj["qualityScore"] = service.qualityScore;
        servicesArray.append(serviceObj);
    }
    json["services"] = servicesArray;
    
    json["totalAlerts"] = static_cast<qint64>(data.totalAlerts);
    json["criticalAlerts"] = static_cast<qint64>(data.criticalAlerts);
    json["warningAlerts"] = static_cast<qint64>(data.warningAlerts);
    json["infoAlerts"] = static_cast<qint64>(data.infoAlerts);
    
    QJsonObject etsiCompliance;
    etsiCompliance["overallCompliance"] = data.etsiCompliance.overallCompliance;
    etsiCompliance["en300799Compliance"] = data.etsiCompliance.en300799Compliance;
    etsiCompliance["en300401Compliance"] = data.etsiCompliance.en300401Compliance;
    json["etsiCompliance"] = etsiCompliance;
    
    return json;
}

QString ReportGenerator::formatDuration(const QDateTime& start, const QDateTime& end) const
{
    qint64 seconds = start.secsTo(end);
    qint64 hours = seconds / 3600;
    qint64 minutes = (seconds % 3600) / 60;
    seconds = seconds % 60;
    
    return QString("%1h %2m %3s").arg(hours).arg(minutes).arg(seconds);
}

QString ReportGenerator::formatPercentage(double percentage, int decimals) const
{
    return QString::number(percentage, 'f', decimals) + "%";
}

QString ReportGenerator::formatNumber(double number, int decimals) const
{
    return QString::number(number, 'f', decimals);
}

bool ReportGenerator::validateOutputPath(const QString& path) const
{
    QFileInfo fileInfo(path);
    QDir dir = fileInfo.dir();
    
    return dir.exists() && dir.isReadable();
}

bool ReportGenerator::validateReportData(const ReportData& data) const
{
    // Basic validation
    if (data.streamName.isEmpty()) {
        return false;
    }
    
    if (!data.analysisStartTime.isValid() || !data.analysisEndTime.isValid()) {
        return false;
    }
    
    if (data.analysisStartTime >= data.analysisEndTime) {
        return false;
    }
    
    return true;
}