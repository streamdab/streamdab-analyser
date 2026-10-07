#include "professional_export_manager.h"
#include "../core/eti_processor.hpp"
// NOTE: ServiceBrowser / PerformanceDashboard are only held as opaque
// pointers (forward-declared in the header); their .cpp files are not part
// of the shipping GUI target, so including their headers here would invite
// AUTOMOC to emit moc_*.cpp files whose slot references cannot link.
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
#include <QLocale>
#include <QStringConverter>
#include <QSettings>

// ---------------------------------------------------------------------------
// Real-analysis record injection (no synthetic content in the app path).
//
// Callers that own the live analysis (DABAnalyserWindow) inject the current
// rows via ExportConfiguration::customSettings:
//   customSettings["records"] = QVariantList of QVariantMap (one per row)
//   customSettings["columns"] = QVariantList of { "key", "header" }
// The writers prefer these records and only fall back to the historical
// sample rows when a programmatic caller supplies no dataset, so the GUI
// never exports fabricated data.
// ---------------------------------------------------------------------------
namespace {

QVariantList exportRecords(const ExportConfiguration& c)
{
    return c.customSettings.value(QStringLiteral("records")).toList();
}

QVariantList exportColumns(const ExportConfiguration& c)
{
    return c.customSettings.value(QStringLiteral("columns")).toList();
}

bool exportHasRealData(const ExportConfiguration& c)
{
    return !exportRecords(c).isEmpty() && !exportColumns(c).isEmpty();
}

QString exportHeaderOf(const QVariant& column)
{
    return column.toMap().value(QStringLiteral("header")).toString();
}

QString exportKeyOf(const QVariant& column)
{
    return column.toMap().value(QStringLiteral("key")).toString();
}

QString exportValueOf(const QVariant& record, const QVariant& column)
{
    return record.toMap().value(exportKeyOf(column)).toString();
}

// CSV/RFC-4180 field escaping: quote fields containing comma, quote, CR or LF
// and double embedded quotes. Decoded labels (Thai/UTF-8, commas, quotes) must
// not corrupt the table.
QString escapeCsv(const QString& value)
{
    if (value.contains(QLatin1Char(',')) || value.contains(QLatin1Char('"'))
        || value.contains(QLatin1Char('\n')) || value.contains(QLatin1Char('\r'))) {
        QString escaped = value;
        escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
        return QLatin1Char('"') + escaped + QLatin1Char('"');
    }
    return value;
}

// Minimal HTML text escaping for table headers/cells.
QString escapeHtml(const QString& value)
{
    QString escaped = value;
    escaped.replace(QLatin1Char('&'), QStringLiteral("&amp;"));
    escaped.replace(QLatin1Char('<'), QStringLiteral("&lt;"));
    escaped.replace(QLatin1Char('>'), QStringLiteral("&gt;"));
    escaped.replace(QLatin1Char('"'), QStringLiteral("&quot;"));
    return escaped;
}

// Double-quoted YAML scalar escaping.
QString escapeYaml(const QString& value)
{
    QString escaped = value;
    escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    escaped.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    escaped.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
    escaped.replace(QLatin1Char('\r'), QStringLiteral("\\r"));
    return escaped;
}

// Canonical file extension for a format (with the leading dot).
QString exportExtensionFor(ExportFormat format)
{
    switch (format) {
        case ExportFormat::CSV:    return QStringLiteral(".csv");
        case ExportFormat::XML:    return QStringLiteral(".xml");
        case ExportFormat::JSON:   return QStringLiteral(".json");
        case ExportFormat::HTML:   return QStringLiteral(".html");
        case ExportFormat::PDF:    return QStringLiteral(".pdf");
        case ExportFormat::Excel:  return QStringLiteral(".xlsx");
        case ExportFormat::YAML:   return QStringLiteral(".yaml");
        case ExportFormat::Binary: return QStringLiteral(".bin");
        case ExportFormat::Text:   return QStringLiteral(".txt");
    }
    return QStringLiteral(".txt");
}

// Qt6 removed Qt::DefaultLocaleShortDate; use the locale's short format.
QString shortDateTime(const QDateTime& dt)
{
    return QLocale().toString(dt, QLocale::ShortFormat);
}

} // namespace

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
    stream.setEncoding(QStringConverter::Utf8);
    
    if (exportHasRealData(m_config)) {
        const QVariantList columns = exportColumns(m_config);
        const QVariantList records = exportRecords(m_config);
        QStringList headers;
        for (const QVariant& column : columns) {
            headers << escapeCsv(exportHeaderOf(column));
        }
        stream << headers.join(QLatin1Char(',')) << "\n";
        for (int i = 0; i < records.size() && !m_cancelled; ++i) {
            QStringList cells;
            for (const QVariant& column : columns) {
                cells << escapeCsv(exportValueOf(records.at(i), column));
            }
            stream << cells.join(QLatin1Char(',')) << "\n";
            if (i % 10 == 0) {
                emit progressUpdated(50 + (i * 40) / records.size(),
                                     tr("Writing data row %1...").arg(i));
            }
        }
        emit progressUpdated(90, tr("Finalizing CSV file..."));
        return;
    }

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
    
    if (exportHasRealData(m_config)) {
        const QVariantList columns = exportColumns(m_config);
        for (const QVariant& rec : exportRecords(m_config)) {
            xml.writeStartElement("Record");
            const QVariantMap map = rec.toMap();
            for (const QVariant& col : columns) {
                const QString key = exportKeyOf(col);
                xml.writeTextElement(key, map.value(key).toString());
            }
            xml.writeEndElement(); // Record
        }
        xml.writeEndElement(); // Data
        xml.writeEndElement(); // ETI_Analysis_Report
        xml.writeEndDocument();
        emit progressUpdated(90, tr("Finalizing XML file..."));
        return;
    }
    
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
    
    if (exportHasRealData(m_config)) {
        QJsonArray dataArray;
        for (const QVariant& rec : exportRecords(m_config)) {
            dataArray.append(QJsonObject::fromVariantMap(rec.toMap()));
        }
        root["data"] = dataArray;
        QFile file(m_config.fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            throw std::runtime_error("Cannot open file for writing");
        }
        QJsonDocument doc(root);
        file.write(doc.toJson());
        emit progressUpdated(90, tr("Finalizing JSON file..."));
        return;
    }
    
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
    stream.setEncoding(QStringConverter::Utf8);
    
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
    stream << "<p>Generated: " << shortDateTime(QDateTime::currentDateTime()) << "</p>\n";
    stream << "<p>Time Range: " << shortDateTime(m_config.startTime) 
           << " to " << shortDateTime(m_config.endTime) << "</p>\n";
    stream << "</div>\n";
    
    emit progressUpdated(40, tr("Writing HTML content..."));
    
    // Content section
    stream << "<div class=\"content\">\n";
    stream << "<h2>Analysis Results</h2>\n";
    stream << "<table>\n";
    
    if (exportHasRealData(m_config)) {
        const QVariantList columns = exportColumns(m_config);
        const QVariantList records = exportRecords(m_config);
        stream << "<tr>";
        for (const QVariant& col : columns) {
            stream << "<th>" << escapeHtml(exportHeaderOf(col)) << "</th>";
        }
        stream << "</tr>\n";
        for (const QVariant& rec : records) {
            stream << "<tr>";
            for (const QVariant& col : columns) {
                stream << "<td>" << escapeHtml(exportValueOf(rec, col)) << "</td>";
            }
            stream << "</tr>\n";
        }
        stream << "</table>\n";
        stream << "</div>\n";
        stream << "</body>\n</html>\n";
        emit progressUpdated(90, tr("Finalizing HTML file..."));
        return;
    }
    
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
        stream << "<td>" << shortDateTime(timestamp) << "</td>\n";
        
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
    stream.setEncoding(QStringConverter::Utf8);
    
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
    
    if (exportHasRealData(m_config)) {
        const QVariantList columns = exportColumns(m_config);
        const QVariantList records = exportRecords(m_config);
        stream << "  data:\n";
        for (int i = 0; i < records.size() && !m_cancelled; ++i) {
            stream << "    - record_" << i << ":\n";
            const QVariantMap map = records.at(i).toMap();
            for (const QVariant& col : columns) {
                const QString key = exportKeyOf(col);
                stream << "        " << key << ": \""
                       << escapeYaml(map.value(key).toString()) << "\"\n";
            }
        }
        emit progressUpdated(90, tr("Finalizing YAML file..."));
        return;
    }
    
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
    stream.setEncoding(QStringConverter::Utf8);
    
    // Text header
    stream << "================================================================================\n";
    stream << "                        ETI STREAM ANALYSIS REPORT\n";
    stream << "================================================================================\n\n";
    
    stream << "Generated: " << shortDateTime(QDateTime::currentDateTime()) << "\n";
    stream << "Time Range: " << shortDateTime(m_config.startTime)
           << " to " << shortDateTime(m_config.endTime) << "\n";
    stream << "Template: " << static_cast<int>(m_config.template_) << "\n\n";
    
    emit progressUpdated(40, tr("Writing text data..."));
    
    if (exportHasRealData(m_config)) {
        const QVariantList columns = exportColumns(m_config);
        const QVariantList records = exportRecords(m_config);
        stream << "ANALYSIS RESULTS:\n";
        stream << "--------------------------------------------------------------------------------\n";
        for (int i = 0; i < records.size() && !m_cancelled; ++i) {
            stream << QString("Record %1:\n").arg(i + 1);
            for (const QVariant& col : columns) {
                stream << QString("  %1: %2\n")
                              .arg(exportHeaderOf(col),
                                   exportValueOf(records.at(i), col));
            }
            stream << "\n";
        }
        stream << "================================================================================\n";
        stream << "                              END OF REPORT\n";
        stream << "================================================================================\n";
        emit progressUpdated(90, tr("Finalizing text file..."));
        return;
    }
    
    stream << "ANALYSIS RESULTS:\n";
    stream << "--------------------------------------------------------------------------------\n";
    
    for (int i = 0; i < 30 && !m_cancelled; ++i) {
        QDateTime timestamp = m_config.startTime.addSecs(i * 200);
        
        stream << QString("Record %1:\n").arg(i + 1);
        stream << QString("  Timestamp: %1\n").arg(shortDateTime(timestamp));
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

// ExportConfigurationDialog Implementation
//
// The legacy generation shipped this class declared in the header but with no
// definitions (it was never compiled). Adopting the export manager therefore
// requires a working, self-contained dialog. It is deliberately compact and
// fully functional: format / scope / template / output target / options.
ExportConfigurationDialog::ExportConfigurationDialog(QWidget* parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_configLayout(nullptr)
    , m_formatGroup(nullptr)
    , m_formatCombo(nullptr)
    , m_formatDescription(nullptr)
    , m_scopeGroup(nullptr)
    , m_scopeCombo(nullptr)
    , m_scopeDescription(nullptr)
    , m_templateGroup(nullptr)
    , m_templateCombo(nullptr)
    , m_templateDescription(nullptr)
    , m_outputGroup(nullptr)
    , m_fileNameEdit(nullptr)
    , m_outputDirEdit(nullptr)
    , m_browseButton(nullptr)
    , m_timeRangeGroup(nullptr)
    , m_optionsGroup(nullptr)
    , m_includeMetadataCheck(nullptr)
    , m_includeChartsCheck(nullptr)
    , m_includeRawDataCheck(nullptr)
    , m_compressOutputCheck(nullptr)
    , m_compressionCombo(nullptr)
    , m_fieldsGroup(nullptr)
    , m_includedFieldsEdit(nullptr)
    , m_excludedFieldsEdit(nullptr)
    , m_buttonLayout(nullptr)
    , m_previewButton(nullptr)
    , m_resetButton(nullptr)
    , m_loadPresetButton(nullptr)
    , m_savePresetButton(nullptr)
    , m_exportButton(nullptr)
    , m_cancelButton(nullptr)
{
    setWindowTitle(tr("Export Analysis"));
    setupUI();
    setConfiguration(m_config);
}

void ExportConfigurationDialog::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);

    m_configLayout = new QGridLayout();
    int row = 0;

    m_formatGroup = new QGroupBox(tr("Format"), this);
    {
        QHBoxLayout* l = new QHBoxLayout(m_formatGroup);
        m_formatCombo = new QComboBox(m_formatGroup);
        m_formatCombo->setObjectName(QStringLiteral("exportFormatCombo"));
        m_formatCombo->addItem(tr("CSV"), static_cast<int>(ExportFormat::CSV));
        m_formatCombo->addItem(tr("JSON"), static_cast<int>(ExportFormat::JSON));
        m_formatCombo->addItem(tr("XML"), static_cast<int>(ExportFormat::XML));
        m_formatCombo->addItem(tr("HTML"), static_cast<int>(ExportFormat::HTML));
        m_formatCombo->addItem(tr("YAML"), static_cast<int>(ExportFormat::YAML));
        m_formatCombo->addItem(tr("Text"), static_cast<int>(ExportFormat::Text));
        l->addWidget(m_formatCombo, 1);
        m_formatDescription = new QLabel(m_formatGroup);
        m_formatDescription->setWordWrap(true);
        l->addWidget(m_formatDescription, 2);
    }
    m_configLayout->addWidget(m_formatGroup, row++, 0, 1, 2);

    m_scopeGroup = new QGroupBox(tr("Scope"), this);
    {
        QHBoxLayout* l = new QHBoxLayout(m_scopeGroup);
        m_scopeCombo = new QComboBox(m_scopeGroup);
        m_scopeCombo->addItem(tr("Current View"), static_cast<int>(ExportScope::CurrentView));
        m_scopeCombo->addItem(tr("All Data"), static_cast<int>(ExportScope::AllData));
        m_scopeCombo->addItem(tr("Compliance Results"), static_cast<int>(ExportScope::ComplianceResults));
        m_scopeCombo->addItem(tr("Performance Metrics"), static_cast<int>(ExportScope::PerformanceMetrics));
        m_scopeCombo->addItem(tr("Service Details"), static_cast<int>(ExportScope::ServiceDetails));
        l->addWidget(m_scopeCombo, 1);
        m_scopeDescription = new QLabel(m_scopeGroup);
        m_scopeDescription->setWordWrap(true);
        l->addWidget(m_scopeDescription, 2);
    }
    m_configLayout->addWidget(m_scopeGroup, row++, 0, 1, 2);

    m_templateGroup = new QGroupBox(tr("Template"), this);
    {
        QHBoxLayout* l = new QHBoxLayout(m_templateGroup);
        m_templateCombo = new QComboBox(m_templateGroup);
        m_templateCombo->addItem(tr("Broadcast Analysis"), static_cast<int>(ExportTemplate::BroadcastAnalysis));
        m_templateCombo->addItem(tr("Compliance Report"), static_cast<int>(ExportTemplate::ComplianceReport));
        m_templateCombo->addItem(tr("Performance Report"), static_cast<int>(ExportTemplate::PerformanceReport));
        m_templateCombo->addItem(tr("Service Inventory"), static_cast<int>(ExportTemplate::ServiceInventory));
        m_templateCombo->addItem(tr("Error Summary"), static_cast<int>(ExportTemplate::ErrorSummary));
        m_templateCombo->addItem(tr("Technical Specification"), static_cast<int>(ExportTemplate::TechnicalSpecification));
        l->addWidget(m_templateCombo, 1);
        m_templateDescription = new QLabel(m_templateGroup);
        m_templateDescription->setWordWrap(true);
        l->addWidget(m_templateDescription, 2);
    }
    m_configLayout->addWidget(m_templateGroup, row++, 0, 1, 2);

    m_outputGroup = new QGroupBox(tr("Output"), this);
    {
        QGridLayout* l = new QGridLayout(m_outputGroup);
        m_fileNameEdit = new QLineEdit(m_outputGroup);
        m_fileNameEdit->setObjectName(QStringLiteral("exportFileNameEdit"));
        m_outputDirEdit = new QLineEdit(m_outputGroup);
        m_browseButton = new QPushButton(tr("Browse..."), m_outputGroup);
        l->addWidget(new QLabel(tr("Directory:"), m_outputGroup), 0, 0);
        l->addWidget(m_outputDirEdit, 0, 1);
        l->addWidget(m_browseButton, 0, 2);
        l->addWidget(new QLabel(tr("File name:"), m_outputGroup), 1, 0);
        l->addWidget(m_fileNameEdit, 1, 1, 1, 2);
        connect(m_browseButton, &QPushButton::clicked,
                this, &ExportConfigurationDialog::onBrowseOutputDirectory);
    }
    m_configLayout->addWidget(m_outputGroup, row++, 0, 1, 2);

    m_optionsGroup = new QGroupBox(tr("Options"), this);
    {
        QGridLayout* l = new QGridLayout(m_optionsGroup);
        m_includeMetadataCheck = new QCheckBox(tr("Include metadata"), m_optionsGroup);
        m_includeChartsCheck = new QCheckBox(tr("Include charts"), m_optionsGroup);
        m_includeRawDataCheck = new QCheckBox(tr("Include raw data"), m_optionsGroup);
        m_compressOutputCheck = new QCheckBox(tr("Compress output"), m_optionsGroup);
        m_compressionCombo = new QComboBox(m_optionsGroup);
        m_compressionCombo->addItem(tr("Normal"), QStringLiteral("normal"));
        m_compressionCombo->addItem(tr("Maximum"), QStringLiteral("maximum"));
        l->addWidget(m_includeMetadataCheck, 0, 0);
        l->addWidget(m_includeChartsCheck, 0, 1);
        l->addWidget(m_includeRawDataCheck, 1, 0);
        l->addWidget(m_compressOutputCheck, 1, 1);
        l->addWidget(m_compressionCombo, 1, 2);
    }
    m_configLayout->addWidget(m_optionsGroup, row++, 0, 1, 2);

    m_mainLayout->addLayout(m_configLayout);
    m_mainLayout->addStretch();

    m_buttonLayout = new QHBoxLayout();
    m_previewButton = new QPushButton(tr("Preview"), this);
    m_resetButton = new QPushButton(tr("Reset"), this);
    m_loadPresetButton = new QPushButton(tr("Load Preset"), this);
    m_savePresetButton = new QPushButton(tr("Save Preset"), this);
    m_exportButton = new QPushButton(tr("Export"), this);
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_exportButton->setDefault(true);
    m_buttonLayout->addWidget(m_previewButton);
    m_buttonLayout->addWidget(m_resetButton);
    m_buttonLayout->addWidget(m_loadPresetButton);
    m_buttonLayout->addWidget(m_savePresetButton);
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_exportButton);
    m_buttonLayout->addWidget(m_cancelButton);
    m_mainLayout->addLayout(m_buttonLayout);

    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ExportConfigurationDialog::onFormatChanged);
    connect(m_scopeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ExportConfigurationDialog::onScopeChanged);
    connect(m_templateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ExportConfigurationDialog::onTemplateChanged);
    connect(m_previewButton, &QPushButton::clicked,
            this, &ExportConfigurationDialog::onPreviewExport);
    connect(m_resetButton, &QPushButton::clicked,
            this, &ExportConfigurationDialog::onResetToDefaults);
    connect(m_loadPresetButton, &QPushButton::clicked,
            this, &ExportConfigurationDialog::onLoadPreset);
    connect(m_savePresetButton, &QPushButton::clicked,
            this, &ExportConfigurationDialog::onSavePreset);
    connect(m_exportButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

ExportConfiguration ExportConfigurationDialog::getConfiguration() const
{
    ExportConfiguration config = m_config;
    if (m_formatCombo) {
        config.format = static_cast<ExportFormat>(m_formatCombo->currentData().toInt());
    }
    if (m_scopeCombo) {
        config.scope = static_cast<ExportScope>(m_scopeCombo->currentData().toInt());
    }
    if (m_templateCombo) {
        config.template_ = static_cast<ExportTemplate>(m_templateCombo->currentData().toInt());
    }
    if (m_fileNameEdit) config.fileName = m_fileNameEdit->text();
    if (m_outputDirEdit) config.outputDirectory = m_outputDirEdit->text();
    if (m_includeMetadataCheck) config.includeMetadata = m_includeMetadataCheck->isChecked();
    if (m_includeChartsCheck) config.includeCharts = m_includeChartsCheck->isChecked();
    if (m_includeRawDataCheck) config.includeRawData = m_includeRawDataCheck->isChecked();
    if (m_compressOutputCheck) config.compressOutput = m_compressOutputCheck->isChecked();
    if (m_compressionCombo) config.compressionLevel = m_compressionCombo->currentData().toString();
    return config;
}

void ExportConfigurationDialog::setConfiguration(const ExportConfiguration& config)
{
    m_config = config;
    if (m_formatCombo) {
        const int i = m_formatCombo->findData(static_cast<int>(config.format));
        m_formatCombo->setCurrentIndex(i >= 0 ? i : 0);
    }
    if (m_scopeCombo) {
        const int i = m_scopeCombo->findData(static_cast<int>(config.scope));
        m_scopeCombo->setCurrentIndex(i >= 0 ? i : 0);
    }
    if (m_templateCombo) {
        const int i = m_templateCombo->findData(static_cast<int>(config.template_));
        m_templateCombo->setCurrentIndex(i >= 0 ? i : 0);
    }
    if (m_fileNameEdit) m_fileNameEdit->setText(config.fileName);
    if (m_outputDirEdit) m_outputDirEdit->setText(config.outputDirectory);
    if (m_includeMetadataCheck) m_includeMetadataCheck->setChecked(config.includeMetadata);
    if (m_includeChartsCheck) m_includeChartsCheck->setChecked(config.includeCharts);
    if (m_includeRawDataCheck) m_includeRawDataCheck->setChecked(config.includeRawData);
    if (m_compressOutputCheck) m_compressOutputCheck->setChecked(config.compressOutput);
    if (m_compressionCombo) {
        const int i = m_compressionCombo->findData(config.compressionLevel);
        m_compressionCombo->setCurrentIndex(i >= 0 ? i : 0);
    }
    updateUIForFormat();
    updateUIForScope();
    validateConfiguration();
}

void ExportConfigurationDialog::onFormatChanged()
{
    updateUIForFormat();

    // Keep the output file extension in sync with the selected format, so
    // switching to JSON/XML/HTML/YAML does not keep a .csv name.
    if (m_formatCombo && m_fileNameEdit) {
        QString name = m_fileNameEdit->text();
        if (!name.isEmpty()) {
            static const QStringList knownExtensions = {
                QStringLiteral(".csv"), QStringLiteral(".json"), QStringLiteral(".xml"),
                QStringLiteral(".html"), QStringLiteral(".yaml"), QStringLiteral(".yml"),
                QStringLiteral(".txt"), QStringLiteral(".pdf"), QStringLiteral(".xlsx"),
                QStringLiteral(".bin")};
            for (const QString& ext : knownExtensions) {
                if (name.endsWith(ext, Qt::CaseInsensitive)) {
                    name.chop(ext.size());
                    break;
                }
            }
            const ExportFormat format =
                static_cast<ExportFormat>(m_formatCombo->currentData().toInt());
            name += exportExtensionFor(format);
            m_fileNameEdit->setText(name);
        }
    }

    validateConfiguration();
}

void ExportConfigurationDialog::onScopeChanged()
{
    updateUIForScope();
}

void ExportConfigurationDialog::onTemplateChanged()
{
    if (m_templateDescription) {
        m_templateDescription->setText(tr("Report template selection."));
    }
}

void ExportConfigurationDialog::onBrowseOutputDirectory()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Select Output Directory"),
        m_outputDirEdit ? m_outputDirEdit->text() : QString());
    if (!dir.isEmpty() && m_outputDirEdit) {
        m_outputDirEdit->setText(dir);
        validateConfiguration();
    }
}

void ExportConfigurationDialog::onPreviewExport()
{
    const ExportConfiguration c = getConfiguration();
    QMessageBox::information(
        this, tr("Export Preview"),
        tr("Format: %1\nScope: %2\nOutput: %3")
            .arg(m_formatCombo ? m_formatCombo->currentText() : QString(),
                 m_scopeCombo ? m_scopeCombo->currentText() : QString(),
                 c.fileName));
}

void ExportConfigurationDialog::onResetToDefaults()
{
    setConfiguration(ExportConfiguration());
}

void ExportConfigurationDialog::onLoadPreset()
{
    QSettings settings(QStringLiteral("StreamDAB-Analyser"), QStringLiteral("DABAnalyser"));
    settings.beginGroup(QStringLiteral("export/preset"));
    ExportConfiguration config = getConfiguration();
    config.format = static_cast<ExportFormat>(
        settings.value(QStringLiteral("format"), static_cast<int>(config.format)).toInt());
    config.scope = static_cast<ExportScope>(
        settings.value(QStringLiteral("scope"), static_cast<int>(config.scope)).toInt());
    config.template_ = static_cast<ExportTemplate>(
        settings.value(QStringLiteral("template"), static_cast<int>(config.template_)).toInt());
    const QString dir = settings.value(QStringLiteral("outputDirectory"), config.outputDirectory).toString();
    settings.endGroup();
    config.outputDirectory = dir;
    setConfiguration(config);
}

void ExportConfigurationDialog::onSavePreset()
{
    const ExportConfiguration config = getConfiguration();
    QSettings settings(QStringLiteral("StreamDAB-Analyser"), QStringLiteral("DABAnalyser"));
    settings.beginGroup(QStringLiteral("export/preset"));
    settings.setValue(QStringLiteral("format"), static_cast<int>(config.format));
    settings.setValue(QStringLiteral("scope"), static_cast<int>(config.scope));
    settings.setValue(QStringLiteral("template"), static_cast<int>(config.template_));
    settings.setValue(QStringLiteral("outputDirectory"), config.outputDirectory);
    settings.endGroup();
    settings.sync();
}

void ExportConfigurationDialog::updateUIForFormat()
{
    const ExportFormat format = m_formatCombo
        ? static_cast<ExportFormat>(m_formatCombo->currentData().toInt())
        : ExportFormat::CSV;
    const bool htmlLike = format == ExportFormat::HTML;
    if (m_includeChartsCheck) {
        m_includeChartsCheck->setEnabled(htmlLike);
    }
    if (m_formatDescription) {
        switch (format) {
            case ExportFormat::CSV:
                m_formatDescription->setText(tr("Comma-separated values."));
                break;
            case ExportFormat::JSON:
                m_formatDescription->setText(tr("JSON for API integration."));
                break;
            case ExportFormat::XML:
                m_formatDescription->setText(tr("XML with ETI schema."));
                break;
            case ExportFormat::HTML:
                m_formatDescription->setText(tr("HTML report with styling."));
                break;
            case ExportFormat::YAML:
                m_formatDescription->setText(tr("YAML configuration format."));
                break;
            case ExportFormat::Text:
                m_formatDescription->setText(tr("Plain text summary."));
                break;
            default:
                m_formatDescription->setText(tr("Unsupported format."));
                break;
        }
    }
}

void ExportConfigurationDialog::updateUIForScope()
{
    const ExportScope scope = m_scopeCombo
        ? static_cast<ExportScope>(m_scopeCombo->currentData().toInt())
        : ExportScope::CurrentView;
    if (m_scopeDescription) {
        switch (scope) {
            case ExportScope::CurrentView:
                m_scopeDescription->setText(tr("Export the currently visible data."));
                break;
            case ExportScope::AllData:
                m_scopeDescription->setText(tr("Export the complete analysis."));
                break;
            case ExportScope::ComplianceResults:
                m_scopeDescription->setText(tr("Export ETSI compliance results."));
                break;
            case ExportScope::PerformanceMetrics:
                m_scopeDescription->setText(tr("Export performance metrics."));
                break;
            case ExportScope::ServiceDetails:
                m_scopeDescription->setText(tr("Export decoded service details."));
                break;
            default:
                m_scopeDescription->setText(tr("Filtered / selected data."));
                break;
        }
    }
}

void ExportConfigurationDialog::validateConfiguration()
{
    const bool ok = m_outputDirEdit && !m_outputDirEdit->text().trimmed().isEmpty()
                    && m_fileNameEdit && !m_fileNameEdit->text().trimmed().isEmpty();
    if (m_exportButton) {
        m_exportButton->setEnabled(ok);
    }
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