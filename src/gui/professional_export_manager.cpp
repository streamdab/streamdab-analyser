#include "professional_export_manager.h"
#include "enhanced_broadcast_theme.h"
#include "../core/eti_processor.hpp"
#include "service_browser.h"
#include "performance_dashboard.h"
#include <QApplication>
#include <QMessageBox>
#include <QClipboard>
#include <QMimeData>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QXmlStreamWriter>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

// ExportWorker Implementation
ExportWorker::ExportWorker(const ExportConfiguration& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_cancelled(false)
{
}

void ExportWorker::startExport()
{
    QMutexLocker locker(&m_mutex);
    m_cancelled = false;
    
    try {
        emit progressUpdated(0, tr("Starting export..."));
        
        switch (m_config.format) {
            case ExportFormat::CSV:
                exportToCSV();
                break;
            case ExportFormat::XML:
                exportToXML();
                break;
            case ExportFormat::JSON:
                exportToJSON();
                break;
            case ExportFormat::HTML:
                exportToHTML();
                break;
            case ExportFormat::PDF:
                exportToPDF();
                break;
            case ExportFormat::Excel:
                exportToExcel();
                break;
            case ExportFormat::YAML:
                exportToYAML();
                break;
            case ExportFormat::Binary:
                exportToBinary();
                break;
            case ExportFormat::Text:
                exportToText();
                break;
        }
        
        if (!m_cancelled) {
            emit progressUpdated(100, tr("Export completed"));
            emit exportCompleted(true, m_config.fileName);
        }
    } catch (const std::exception& e) {
        emit exportError(tr("Export failed: %1").arg(e.what()));
        emit exportCompleted(false, m_config.fileName);
    }
}

void ExportWorker::cancelExport()
{
    QMutexLocker locker(&m_mutex);
    m_cancelled = true;
}

void ExportWorker::exportToCSV()
{
    emit progressUpdated(20, tr("Preparing CSV data..."));
    
    QFile file(m_config.fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw std::runtime_error("Cannot open file for writing");
    }
    
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    
    // Write CSV header based on template
    switch (m_config.template_) {
        case ExportTemplate::BroadcastAnalysis:
            stream << "Timestamp,Frame Number,Service ID,Service Name,Bitrate,Quality,Error Count\n";
            break;
        case ExportTemplate::ComplianceReport:
            stream << "Timestamp,Violation Type,Severity,Frame Number,Description,Standard Reference\n";
            break;
        case ExportTemplate::PerformanceReport:
            stream << "Timestamp,FPS,Memory MB,CPU %,Latency MS,Frame Count\n";
            break;
        case ExportTemplate::ServiceInventory:
            stream << "Service ID,Service Name,Service Type,Bitrate,Protection Level,Language\n";
            break;
        default:
            stream << "Timestamp,Parameter,Value,Unit,Quality\n";
            break;
    }
    
    emit progressUpdated(50, tr("Writing CSV data..."));
    
    // Write sample data (in production, this would fetch real data)
    for (int i = 0; i < 100 && !m_cancelled; ++i) {
        QDateTime timestamp = m_config.startTime.addSecs(i * 60);
        
        switch (m_config.template_) {
            case ExportTemplate::BroadcastAnalysis:
                stream << QString("%1,%2,0x%3,Service %4,%5,95.5,0\n")
                          .arg(timestamp.toString(Qt::ISODate))
                          .arg(i * 24)
                          .arg(0x1000 + i, 0, 16)
                          .arg(i + 1)
                          .arg(128 + (i % 64));
                break;
            case ExportTemplate::ComplianceReport:
                if (i % 10 == 0) { // Occasional violations
                    stream << QString("%1,FIG Type Error,Warning,%2,Invalid FIG type,EN 300 799 Section 5.2\n")
                              .arg(timestamp.toString(Qt::ISODate))
                              .arg(i * 24);
                }
                break;
            case ExportTemplate::PerformanceReport:
                stream << QString("%1,%2,%3,%4,%5,%6\n")
                          .arg(timestamp.toString(Qt::ISODate))
                          .arg(900 + (i % 100))
                          .arg(80 + (i % 20))
                          .arg(25 + (i % 50))
                          .arg(15 + (i % 10))
                          .arg(i * 24);
                break;
            default:
                stream << QString("%1,Parameter_%2,%3,Unit_%4,Good\n")
                          .arg(timestamp.toString(Qt::ISODate))
                          .arg(i)
                          .arg(100 + i)
                          .arg(i % 5);
                break;
        }
        
        if (i % 10 == 0) {
            emit progressUpdated(50 + (i * 40) / 100, tr("Writing data row %1...").arg(i));
        }
    }
    
    emit progressUpdated(90, tr("Finalizing CSV file..."));
}

void ExportWorker::exportToXML()
{
    emit progressUpdated(20, tr("Preparing XML data..."));
    
    QFile file(m_config.fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        throw std::runtime_error("Cannot open file for writing");
    }
    
    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    
    xml.writeStartElement("ETI_Analysis_Report");
    xml.writeAttribute("version", "1.0");
    xml.writeAttribute("generated", QDateTime::currentDateTime().toString(Qt::ISODate));
    xml.writeAttribute("template", QString::number(static_cast<int>(m_config.template_)));
    
    // Metadata section
    if (m_config.includeMetadata) {
        xml.writeStartElement("Metadata");
        xml.writeTextElement("ExportTime", QDateTime::currentDateTime().toString(Qt::ISODate));
        xml.writeTextElement("TimeRange", QString("%1 to %2")
                            .arg(m_config.startTime.toString(Qt::ISODate))
                            .arg(m_config.endTime.toString(Qt::ISODate)));
        xml.writeTextElement("Application", QCoreApplication::applicationName());
        xml.writeTextElement("Version", QCoreApplication::applicationVersion());
        xml.writeEndElement(); // Metadata
    }
    
    emit progressUpdated(40, tr("Writing XML data..."));
    
    // Data section
    xml.writeStartElement("Data");
    
    for (int i = 0; i < 50 && !m_cancelled; ++i) {
        xml.writeStartElement("Record");
        xml.writeAttribute("id", QString::number(i));
        
        QDateTime timestamp = m_config.startTime.addSecs(i * 120);
        xml.writeTextElement("Timestamp", timestamp.toString(Qt::ISODate));
        xml.writeTextElement("FrameNumber", QString::number(i * 24));
        xml.writeTextElement("ServiceID", QString("0x%1").arg(0x1000 + i, 0, 16));
        xml.writeTextElement("Quality", QString::number(95.0 + (i % 5)));
        
        xml.writeEndElement(); // Record
        
        if (i % 5 == 0) {
            emit progressUpdated(40 + (i * 40) / 50, tr("Writing XML record %1...").arg(i));
        }
    }
    
    xml.writeEndElement(); // Data
    xml.writeEndElement(); // ETI_Analysis_Report
    xml.writeEndDocument();
    
    emit progressUpdated(90, tr("Finalizing XML file..."));
}

void ExportWorker::exportToJSON()
{
    emit progressUpdated(20, tr("Preparing JSON data..."));
    
    QJsonObject root;
    root["version"] = "1.0";
    root["generated"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["template"] = static_cast<int>(m_config.template_);
    
    // Metadata
    if (m_config.includeMetadata) {
        QJsonObject metadata;
        metadata["exportTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        metadata["timeRange"] = QString("%1 to %2")
                               .arg(m_config.startTime.toString(Qt::ISODate))
                               .arg(m_config.endTime.toString(Qt::ISODate));
        metadata["application"] = QCoreApplication::applicationName();
        metadata["version"] = QCoreApplication::applicationVersion();
        root["metadata"] = metadata;
    }
    
    emit progressUpdated(40, tr("Writing JSON data..."));
    
    // Data array
    QJsonArray dataArray;
    for (int i = 0; i < 100 && !m_cancelled; ++i) {
        QJsonObject record;
        QDateTime timestamp = m_config.startTime.addSecs(i * 60);
        
        record["timestamp"] = timestamp.toString(Qt::ISODate);
        record["frameNumber"] = i * 24;
        record["serviceId"] = QString("0x%1").arg(0x1000 + i, 0, 16);
        record["quality"] = 95.0 + (i % 5);
        record["bitrate"] = 128 + (i % 64);
        
        dataArray.append(record);
        
        if (i % 10 == 0) {
            emit progressUpdated(40 + (i * 40) / 100, tr("Writing JSON record %1...").arg(i));
        }
    }
    
    root["data"] = dataArray;
    
    // Write to file
    QFile file(m_config.fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        throw std::runtime_error("Cannot open file for writing");
    }
    
    QJsonDocument doc(root);
    file.write(doc.toJson());
    
    emit progressUpdated(90, tr("Finalizing JSON file..."));
}

void ExportWorker::exportToHTML()
{
    emit progressUpdated(20, tr("Preparing HTML report..."));
    
    QFile file(m_config.fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw std::runtime_error("Cannot open file for writing");
    }
    
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    
    // HTML header with professional styling
    stream << "<!DOCTYPE html>\n";
    stream << "<html>\n<head>\n";
    stream << "<meta charset=\"UTF-8\">\n";
    stream << "<title>ETI Stream Analysis Report</title>\n";
    stream << "<style>\n";
    stream << "body { font-family: 'Segoe UI', Arial, sans-serif; margin: 40px; background: #f5f5f5; }\n";
    stream << ".header { background: #2D2D30; color: white; padding: 20px; border-radius: 8px; }\n";
    stream << ".content { background: white; padding: 20px; margin-top: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
    stream << "table { width: 100%; border-collapse: collapse; margin-top: 20px; }\n";
    stream << "th, td { border: 1px solid #ddd; padding: 12px; text-align: left; }\n";
    stream << "th { background-color: #0078D4; color: white; }\n";
    stream << "tr:nth-child(even) { background-color: #f9f9f9; }\n";
    stream << ".quality-excellent { color: #28A745; font-weight: bold; }\n";
    stream << ".quality-good { color: #6BBF59; }\n";
    stream << ".quality-fair { color: #FFC107; }\n";
    stream << ".quality-poor { color: #FD7E14; }\n";
    stream << ".quality-critical { color: #DC3545; font-weight: bold; }\n";
    stream << "</style>\n";
    stream << "</head>\n<body>\n";
    
    // Header section
    stream << "<div class=\"header\">\n";
    stream << "<h1>ETI Stream Analysis Report</h1>\n";
    stream << "<p>Generated: " << QDateTime::currentDateTime().toString(Qt::DefaultLocaleShortDate) << "</p>\n";
    stream << "<p>Time Range: " << m_config.startTime.toString(Qt::DefaultLocaleShortDate) 
           << " to " << m_config.endTime.toString(Qt::DefaultLocaleShortDate) << "</p>\n";
    stream << "</div>\n";
    
    emit progressUpdated(40, tr("Writing HTML content..."));
    
    // Content section
    stream << "<div class=\"content\">\n";
    stream << "<h2>Analysis Results</h2>\n";
    stream << "<table>\n";
    
    // Table header based on template
    switch (m_config.template_) {
        case ExportTemplate::BroadcastAnalysis:
            stream << "<tr><th>Timestamp</th><th>Frame</th><th>Service ID</th><th>Service Name</th><th>Bitrate</th><th>Quality</th></tr>\n";
            break;
        case ExportTemplate::ComplianceReport:
            stream << "<tr><th>Timestamp</th><th>Violation Type</th><th>Severity</th><th>Frame</th><th>Description</th></tr>\n";
            break;
        default:
            stream << "<tr><th>Timestamp</th><th>Parameter</th><th>Value</th><th>Quality</th></tr>\n";
            break;
    }
    
    // Table data
    for (int i = 0; i < 50 && !m_cancelled; ++i) {
        QDateTime timestamp = m_config.startTime.addSecs(i * 120);
        double quality = 95.0 + (i % 5);
        QString qualityClass = quality > 95 ? "quality-excellent" : 
                              quality > 80 ? "quality-good" : "quality-fair";
        
        stream << "<tr>\n";
        stream << "<td>" << timestamp.toString(Qt::DefaultLocaleShortDate) << "</td>\n";
        
        switch (m_config.template_) {
            case ExportTemplate::BroadcastAnalysis:
                stream << "<td>" << (i * 24) << "</td>\n";
                stream << "<td>0x" << QString::number(0x1000 + i, 16).toUpper() << "</td>\n";
                stream << "<td>Service " << (i + 1) << "</td>\n";
                stream << "<td>" << (128 + (i % 64)) << " kbps</td>\n";
                stream << "<td class=\"" << qualityClass << "\">" << QString::number(quality, 'f', 1) << "%</td>\n";
                break;
            default:
                stream << "<td>Parameter " << i << "</td>\n";
                stream << "<td>" << (100 + i) << "</td>\n";
                stream << "<td class=\"" << qualityClass << "\">" << QString::number(quality, 'f', 1) << "%</td>\n";
                break;
        }
        
        stream << "</tr>\n";
        
        if (i % 5 == 0) {
            emit progressUpdated(40 + (i * 40) / 50, tr("Writing HTML row %1...").arg(i));
        }
    }
    
    stream << "</table>\n";
    stream << "</div>\n";
    stream << "</body>\n</html>\n";
    
    emit progressUpdated(90, tr("Finalizing HTML file..."));
}

void ExportWorker::exportToPDF()
{
    // PDF export would require QPrinter or external library
    emit progressUpdated(50, tr("PDF export not yet implemented"));
    emit exportError(tr("PDF export feature is not yet implemented"));
}

void ExportWorker::exportToExcel()
{
    // Excel export would require external library like QXlsx
    emit progressUpdated(50, tr("Excel export not yet implemented"));
    emit exportError(tr("Excel export feature is not yet implemented"));
}

void ExportWorker::exportToYAML()
{
    emit progressUpdated(20, tr("Preparing YAML data..."));
    
    QFile file(m_config.fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw std::runtime_error("Cannot open file for writing");
    }
    
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    
    // YAML header
    stream << "---\n";
    stream << "eti_analysis_report:\n";
    stream << "  version: \"1.0\"\n";
    stream << "  generated: \"" << QDateTime::currentDateTime().toString(Qt::ISODate) << "\"\n";
    stream << "  template: " << static_cast<int>(m_config.template_) << "\n";
    
    if (m_config.includeMetadata) {
        stream << "  metadata:\n";
        stream << "    export_time: \"" << QDateTime::currentDateTime().toString(Qt::ISODate) << "\"\n";
        stream << "    time_range:\n";
        stream << "      start: \"" << m_config.startTime.toString(Qt::ISODate) << "\"\n";
        stream << "      end: \"" << m_config.endTime.toString(Qt::ISODate) << "\"\n";
        stream << "    application: \"" << QCoreApplication::applicationName() << "\"\n";
    }
    
    emit progressUpdated(40, tr("Writing YAML data..."));
    
    stream << "  data:\n";
    for (int i = 0; i < 20 && !m_cancelled; ++i) {
        QDateTime timestamp = m_config.startTime.addSecs(i * 300);
        
        stream << "    - record_" << i << ":\n";
        stream << "        timestamp: \"" << timestamp.toString(Qt::ISODate) << "\"\n";
        stream << "        frame_number: " << (i * 24) << "\n";
        stream << "        service_id: \"0x" << QString::number(0x1000 + i, 16) << "\"\n";
        stream << "        quality: " << (95.0 + (i % 5)) << "\n";
        
        if (i % 2 == 0) {
            emit progressUpdated(40 + (i * 40) / 20, tr("Writing YAML record %1...").arg(i));
        }
    }
    
    emit progressUpdated(90, tr("Finalizing YAML file..."));
}

void ExportWorker::exportToBinary()
{
    emit progressUpdated(50, tr("Binary export not yet implemented"));
    emit exportError(tr("Binary export feature is not yet implemented"));
}

void ExportWorker::exportToText()
{
    emit progressUpdated(20, tr("Preparing text report..."));
    
    QFile file(m_config.fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw std::runtime_error("Cannot open file for writing");
    }
    
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    
    // Text header
    stream << "================================================================================\n";
    stream << "                        ETI STREAM ANALYSIS REPORT\n";
    stream << "================================================================================\n\n";
    
    stream << "Generated: " << QDateTime::currentDateTime().toString(Qt::DefaultLocaleShortDate) << "\n";
    stream << "Time Range: " << m_config.startTime.toString(Qt::DefaultLocaleShortDate)
           << " to " << m_config.endTime.toString(Qt::DefaultLocaleShortDate) << "\n";
    stream << "Template: " << static_cast<int>(m_config.template_) << "\n\n";
    
    emit progressUpdated(40, tr("Writing text data..."));
    
    stream << "ANALYSIS RESULTS:\n";
    stream << "--------------------------------------------------------------------------------\n";
    
    for (int i = 0; i < 30 && !m_cancelled; ++i) {
        QDateTime timestamp = m_config.startTime.addSecs(i * 200);
        
        stream << QString("Record %1:\n").arg(i + 1);
        stream << QString("  Timestamp: %1\n").arg(timestamp.toString(Qt::DefaultLocaleShortDate));
        stream << QString("  Frame Number: %1\n").arg(i * 24);
        stream << QString("  Service ID: 0x%1\n").arg(0x1000 + i, 0, 16);
        stream << QString("  Quality: %1%\n").arg(95.0 + (i % 5), 0, 'f', 1);
        stream << "\n";
        
        if (i % 3 == 0) {
            emit progressUpdated(40 + (i * 40) / 30, tr("Writing text record %1...").arg(i));
        }
    }
    
    stream << "================================================================================\n";
    stream << "                              END OF REPORT\n";
    stream << "================================================================================\n";
    
    emit progressUpdated(90, tr("Finalizing text file..."));
}

// ProfessionalExportManager Implementation
ProfessionalExportManager::ProfessionalExportManager(QObject* parent)
    : QObject(parent)
    , m_etiProcessor(nullptr)
    , m_serviceBrowser(nullptr)
    , m_dashboard(nullptr)
    , m_exportWorker(nullptr)
    , m_exportThread(nullptr)
    , m_exportRunning(false)
    , m_progressDialog(nullptr)
    , m_initialized(false)
{
    m_defaultOutputDirectory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/ETI_Exports";
    QDir().mkpath(m_defaultOutputDirectory);
}

ProfessionalExportManager::~ProfessionalExportManager()
{
    if (m_exportRunning) {
        cancelExport();
    }
    
    cleanupExportWorker();
    
    if (m_progressDialog) {
        delete m_progressDialog;
    }
}

void ProfessionalExportManager::initialize(EtiProcessor* etiProcessor, 
                                          ServiceBrowser* serviceBrowser,
                                          PerformanceDashboard* dashboard)
{
    m_etiProcessor = etiProcessor;
    m_serviceBrowser = serviceBrowser;
    m_dashboard = dashboard;
    m_initialized = true;
    
    qDebug() << "Professional Export Manager initialized";
}

bool ProfessionalExportManager::showExportDialog(const ExportConfiguration& config)
{
    if (!m_initialized) {
        QMessageBox::warning(nullptr, tr("Export Manager"), 
                           tr("Export manager is not initialized"));
        return false;
    }
    
    ExportConfigurationDialog dialog;
    dialog.setConfiguration(config);
    
    // Apply broadcast theme
    BROADCAST_THEME.applyToWidget(&dialog);
    
    if (dialog.exec() == QDialog::Accepted) {
        ExportConfiguration exportConfig = dialog.getConfiguration();
        return startExport(exportConfig);
    }
    
    return false;
}

bool ProfessionalExportManager::startExport(const ExportConfiguration& config)
{
    if (m_exportRunning) {
        QMessageBox::information(nullptr, tr("Export Manager"), 
                                tr("Export is already running"));
        return false;
    }
    
    if (!validateConfiguration(config)) {
        QMessageBox::warning(nullptr, tr("Export Manager"), 
                           tr("Invalid export configuration"));
        return false;
    }
    
    m_lastConfiguration = config;
    
    // Setup progress dialog
    if (!m_progressDialog) {
        m_progressDialog = new QProgressDialog();
        m_progressDialog->setWindowTitle(tr("Exporting Data"));
        m_progressDialog->setModal(true);
        m_progressDialog->setMinimumDuration(0);
        BROADCAST_THEME.applyToWidget(m_progressDialog);
    }
    
    m_progressDialog->setLabelText(tr("Preparing export..."));
    m_progressDialog->setRange(0, 100);
    m_progressDialog->setValue(0);
    m_progressDialog->show();
    
    // Setup export worker
    setupExportWorker(config);
    
    emit exportStarted(config.fileName);
    return true;
}

bool ProfessionalExportManager::quickExport(ExportFormat format, const QString& fileName)
{
    ExportConfiguration config = createDefaultConfiguration(format);
    
    if (!fileName.isEmpty()) {
        config.fileName = fileName;
    } else {
        config.fileName = generateFileName(format, config.template_);
    }
    
    return startExport(config);
}

bool ProfessionalExportManager::exportCurrentView(ExportFormat format)
{
    ExportConfiguration config = createDefaultConfiguration(format);
    config.scope = ExportScope::CurrentView;
    config.fileName = generateFileName(format, ExportTemplate::BroadcastAnalysis);
    
    return startExport(config);
}

bool ProfessionalExportManager::exportSelectedItems(ExportFormat format)
{
    ExportConfiguration config = createDefaultConfiguration(format);
    config.scope = ExportScope::SelectedItems;
    config.fileName = generateFileName(format, ExportTemplate::BroadcastAnalysis);
    
    return startExport(config);
}

bool ProfessionalExportManager::generateComplianceReport(ExportTemplate template_, const QString& fileName)
{
    ExportConfiguration config = createDefaultConfiguration(ExportFormat::HTML);
    config.template_ = template_;
    config.scope = ExportScope::ComplianceResults;
    
    if (!fileName.isEmpty()) {
        config.fileName = fileName;
    } else {
        config.fileName = generateFileName(ExportFormat::HTML, template_);
    }
    
    return startExport(config);
}

bool ProfessionalExportManager::generatePerformanceReport(ExportTemplate template_, const QString& fileName)
{
    ExportConfiguration config = createDefaultConfiguration(ExportFormat::HTML);
    config.template_ = template_;
    config.scope = ExportScope::PerformanceMetrics;
    
    if (!fileName.isEmpty()) {
        config.fileName = fileName;
    } else {
        config.fileName = generateFileName(ExportFormat::HTML, template_);
    }
    
    return startExport(config);
}

void ProfessionalExportManager::cancelExport()
{
    if (m_exportRunning && m_exportWorker) {
        m_exportWorker->cancelExport();
    }
    
    if (m_progressDialog) {
        m_progressDialog->hide();
    }
}

QStringList ProfessionalExportManager::getSupportedFormats()
{
    return QStringList() << "CSV" << "XML" << "JSON" << "HTML" << "PDF" 
                         << "Excel" << "YAML" << "Binary" << "Text";
}

QStringList ProfessionalExportManager::getAvailableTemplates()
{
    return QStringList() << "Broadcast Analysis" << "Compliance Report" 
                         << "Performance Report" << "Service Inventory"
                         << "Error Summary" << "Technical Specification" 
                         << "Custom Format";
}

QString ProfessionalExportManager::getDefaultOutputDirectory() const
{
    return m_defaultOutputDirectory;
}

void ProfessionalExportManager::setDefaultOutputDirectory(const QString& directory)
{
    m_defaultOutputDirectory = directory;
    QDir().mkpath(directory);
}

void ProfessionalExportManager::showQuickExportMenu()
{
    // Quick export menu would be implemented here
    exportCurrentView(ExportFormat::CSV);
}

void ProfessionalExportManager::exportToClipboard(ExportFormat format)
{
    // Create temporary file for clipboard export
    QString tempFileName = QDir::tempPath() + "/eti_clipboard_export" + getFileExtension(format);
    
    ExportConfiguration config = createDefaultConfiguration(format);
    config.fileName = tempFileName;
    config.scope = ExportScope::CurrentView;
    
    // For clipboard, we'll do a synchronous export
    ExportWorker worker(config);
    worker.startExport();
    
    // Read the file and put it on clipboard
    QFile file(tempFileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(file.readAll());
        
        QMessageBox::information(nullptr, tr("Export Manager"), 
                                tr("Data exported to clipboard"));
    }
    
    // Clean up temp file
    QFile::remove(tempFileName);
}

void ProfessionalExportManager::onWorkerProgress(int percentage, const QString& status)
{
    if (m_progressDialog) {
        m_progressDialog->setValue(percentage);
        m_progressDialog->setLabelText(status);
    }
    
    emit exportProgress(percentage, status);
}

void ProfessionalExportManager::onWorkerCompleted(bool success, const QString& fileName)
{
    m_exportRunning = false;
    
    if (m_progressDialog) {
        m_progressDialog->hide();
    }
    
    cleanupExportWorker();
    
    if (success) {
        QMessageBox::information(nullptr, tr("Export Completed"), 
                                tr("Data exported successfully to:\n%1").arg(fileName));
        
        // Option to open the file
        int ret = QMessageBox::question(nullptr, tr("Export Completed"),
                                       tr("Would you like to open the exported file?"),
                                       QMessageBox::Yes | QMessageBox::No);
        
        if (ret == QMessageBox::Yes) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
        }
    }
    
    emit exportCompleted(success, fileName);
}

void ProfessionalExportManager::onWorkerError(const QString& error)
{
    m_exportRunning = false;
    
    if (m_progressDialog) {
        m_progressDialog->hide();
    }
    
    cleanupExportWorker();
    
    QMessageBox::critical(nullptr, tr("Export Error"), 
                         tr("Export failed:\n%1").arg(error));
    
    emit exportError(error);
}

ExportConfiguration ProfessionalExportManager::createDefaultConfiguration(ExportFormat format) const
{
    ExportConfiguration config;
    config.format = format;
    config.outputDirectory = m_defaultOutputDirectory;
    config.fileName = generateFileName(format, config.template_);
    
    return config;
}

QString ProfessionalExportManager::generateFileName(ExportFormat format, ExportTemplate template_) const
{
    QString baseName;
    
    switch (template_) {
        case ExportTemplate::BroadcastAnalysis:
            baseName = "broadcast_analysis";
            break;
        case ExportTemplate::ComplianceReport:
            baseName = "compliance_report";
            break;
        case ExportTemplate::PerformanceReport:
            baseName = "performance_report";
            break;
        case ExportTemplate::ServiceInventory:
            baseName = "service_inventory";
            break;
        case ExportTemplate::ErrorSummary:
            baseName = "error_summary";
            break;
        default:
            baseName = "eti_export";
            break;
    }
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString extension = getFileExtension(format);
    
    return QString("%1/%2_%3%4")
           .arg(m_defaultOutputDirectory)
           .arg(baseName)
           .arg(timestamp)
           .arg(extension);
}

QString ProfessionalExportManager::getFileExtension(ExportFormat format) const
{
    switch (format) {
        case ExportFormat::CSV: return ".csv";
        case ExportFormat::XML: return ".xml";
        case ExportFormat::JSON: return ".json";
        case ExportFormat::HTML: return ".html";
        case ExportFormat::PDF: return ".pdf";
        case ExportFormat::Excel: return ".xlsx";
        case ExportFormat::YAML: return ".yaml";
        case ExportFormat::Binary: return ".bin";
        case ExportFormat::Text: return ".txt";
        default: return ".txt";
    }
}

bool ProfessionalExportManager::validateConfiguration(const ExportConfiguration& config) const
{
    if (config.fileName.isEmpty()) {
        return false;
    }
    
    if (config.outputDirectory.isEmpty()) {
        return false;
    }
    
    // Ensure output directory exists
    QDir dir(config.outputDirectory);
    if (!dir.exists()) {
        if (!dir.mkpath(config.outputDirectory)) {
            return false;
        }
    }
    
    return true;
}

void ProfessionalExportManager::setupExportWorker(const ExportConfiguration& config)
{
    cleanupExportWorker();
    
    m_exportThread = new QThread(this);
    m_exportWorker = new ExportWorker(config);
    m_exportWorker->moveToThread(m_exportThread);
    
    // Connect worker signals
    connect(m_exportWorker, &ExportWorker::progressUpdated, 
            this, &ProfessionalExportManager::onWorkerProgress);
    connect(m_exportWorker, &ExportWorker::exportCompleted, 
            this, &ProfessionalExportManager::onWorkerCompleted);
    connect(m_exportWorker, &ExportWorker::exportError, 
            this, &ProfessionalExportManager::onWorkerError);
    
    // Connect thread signals
    connect(m_exportThread, &QThread::started, m_exportWorker, &ExportWorker::startExport);
    connect(m_exportWorker, &ExportWorker::exportCompleted, m_exportThread, &QThread::quit);
    connect(m_exportWorker, &ExportWorker::exportError, m_exportThread, &QThread::quit);
    connect(m_exportThread, &QThread::finished, m_exportWorker, &ExportWorker::deleteLater);
    
    // Connect progress dialog cancel
    if (m_progressDialog) {
        connect(m_progressDialog, &QProgressDialog::canceled, 
                m_exportWorker, &ExportWorker::cancelExport);
    }
    
    m_exportRunning = true;
    m_exportThread->start();
}

void ProfessionalExportManager::cleanupExportWorker()
{
    if (m_exportThread) {
        if (m_exportThread->isRunning()) {
            m_exportThread->quit();
            m_exportThread->wait(5000); // Wait up to 5 seconds
        }
        
        m_exportThread->deleteLater();
        m_exportThread = nullptr;
    }
    
    m_exportWorker = nullptr; // Will be deleted by thread finished signal
    m_exportRunning = false;
}

#include "professional_export_manager.moc"