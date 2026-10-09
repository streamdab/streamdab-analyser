#include "etsi_compliance_engine.hpp"
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include <algorithm>
#include <numeric>

namespace etsi {

ETSIComplianceEngine::ETSIComplianceEngine(QObject* parent)
    : QObject(parent)
    , m_strict_mode(false)
    , m_min_validation_level(ComplianceLevel::WARNING)
    , m_last_frame_counter(0)
{
}

ValidationResult ETSIComplianceEngine::validateETIFrame(const ETIFrame& frame)
{
    // Validate ETI sync bytes (ETSI EN 300 799 Section 5.3.1)
    auto sync_result = validateETISync(frame);
    if (sync_result.level == ComplianceLevel::CRITICAL) {
        emit criticalErrorDetected(sync_result);
        return sync_result;
    }
    
    // Validate frame counter sequence (ETSI EN 300 799 Section 5.3.2)
    auto fc_result = validateFrameCounter(frame, m_last_frame_counter + 1);
    if (fc_result.level == ComplianceLevel::CRITICAL) {
        emit criticalErrorDetected(fc_result);
        return fc_result;
    }
    
    // Update frame counter for next validation
    m_last_frame_counter = frame.frame_counter;
    
    // Validate FIC section (ETSI EN 300 799 Section 5.4)
    auto fic_result = validateFICSection(frame);
    if (fic_result.level == ComplianceLevel::CRITICAL) {
        emit criticalErrorDetected(fic_result);
        return fic_result;
    }
    
    // Validate MSC section (ETSI EN 300 799 Section 5.5)
    auto msc_result = validateMSCSection(frame);
    if (msc_result.level == ComplianceLevel::CRITICAL) {
        emit criticalErrorDetected(msc_result);
        return msc_result;
    }
    
    // If all validations pass
    ValidationResult pass_result(ValidationErrorType::ETI_SYNC_ERROR, ComplianceLevel::PASS,
                               QString("ETI Frame %1 passes ETSI EN 300 799 validation").arg(frame.frame_number),
                               "ETSI EN 300 799", frame.frame_number, 0);
    
    emit validationComplete(pass_result);
    return pass_result;
}

ValidationResult ETSIComplianceEngine::validateETISync(const ETIFrame& frame)
{
    // ETSI EN 300 799 Section 5.1.1 - ETI synchronization patterns
    // ETI-NI (Non-Interleaved): 0x49 0x93 0x1E 0x03
    // ETI-LI (Linear Interleaved): 0xFF 0xF8 0xC5 0x49 (Pattern A), 0xFF 0x07 0x3A 0xB6 (Pattern B)

    // ETI-NI patterns (various byte order interpretations)
    bool is_eti_ni = (frame.sync_bytes == 0xFF1F491F || frame.sync_bytes == 0x491F1FFF ||
                      frame.sync_bytes == 0xFF1FC4FF || frame.sync_bytes == 0xC4FF1FFF ||
                      frame.sync_bytes == 0x491FC4FF || frame.sync_bytes == 0xC4FF491F ||
                      frame.sync_bytes == 0x49931E03);  // Direct ETI-NI pattern

    // ETI-LI patterns (from real ETI file analysis)
    bool is_eti_li = (frame.sync_bytes == 0xFFF8C549 || frame.sync_bytes == 0x49C5F8FF ||  // Pattern A
                      frame.sync_bytes == 0xFF073AB6 || frame.sync_bytes == 0xB63A07FF);    // Pattern B

    if (!is_eti_ni && !is_eti_li) {
        return ValidationResult(
            ValidationErrorType::ETI_SYNC_ERROR,
            ComplianceLevel::CRITICAL,
            QString("Invalid ETI sync bytes: 0x%1 (expected ETI-NI or ETI-LI pattern)")
                .arg(frame.sync_bytes, 8, 16, QChar('0')).toUpper(),
            "ETSI EN 300 799 Section 5.1.1",
            frame.frame_number,
            0
        );
    }

    return ValidationResult(
        ValidationErrorType::ETI_SYNC_ERROR,
        ComplianceLevel::PASS,
        QString("ETI sync bytes valid (%1)").arg(is_eti_ni ? "ETI-NI" : "ETI-LI"),
        "ETSI EN 300 799 Section 5.1.1",
        frame.frame_number,
        0
    );
}

ValidationResult ETSIComplianceEngine::validateFrameCounter(const ETIFrame& frame, uint32_t expected_fc)
{
    // Frame counter should increment sequentially (with wraparound at 16777215)
    const uint32_t MAX_FRAME_COUNTER = 0xFFFFFF; // 24-bit counter
    
    uint32_t expected_with_wrap = expected_fc % (MAX_FRAME_COUNTER + 1);
    
    if (frame.frame_counter != expected_with_wrap && m_last_frame_counter != 0) {
        ComplianceLevel level = (abs(static_cast<int>(frame.frame_counter) - static_cast<int>(expected_with_wrap)) > 10) 
                               ? ComplianceLevel::CRITICAL : ComplianceLevel::WARNING;
        
        return ValidationResult(
            ValidationErrorType::FRAME_COUNTER_ERROR,
            level,
            QString("Frame counter discontinuity: got %1, expected %2")
                .arg(frame.frame_counter).arg(expected_with_wrap),
            "ETSI EN 300 799 Section 5.3.2",
            frame.frame_number,
            4
        );
    }
    
    return ValidationResult(
        ValidationErrorType::FRAME_COUNTER_ERROR,
        ComplianceLevel::PASS,
        QString("Frame counter valid: %1").arg(frame.frame_counter),
        "ETSI EN 300 799 Section 5.3.2",
        frame.frame_number,
        4
    );
}

ValidationResult ETSIComplianceEngine::validateFICSection(const ETIFrame& frame)
{
    // FIC section validation (ETSI EN 300 799 Section 5.4)
    if (frame.fic_data.empty()) {
        return ValidationResult(
            ValidationErrorType::FIC_CRC_ERROR,
            ComplianceLevel::WARNING,
            "FIC section is empty",
            "ETSI EN 300 799 Section 5.4",
            frame.frame_number,
            32
        );
    }
    
    // Validate FIC CRC
    if (!validateFICCRC(frame.fic_data, frame.fic_crc)) {
        return ValidationResult(
            ValidationErrorType::FIC_CRC_ERROR,
            ComplianceLevel::CRITICAL,
            QString("FIC CRC validation failed: calculated CRC differs from frame CRC 0x%1")
                .arg(frame.fic_crc, 4, 16, QChar('0')).toUpper(),
            "ETSI EN 300 799 Section 5.4.3",
            frame.frame_number,
            32
        );
    }
    
    return ValidationResult(
        ValidationErrorType::FIC_CRC_ERROR,
        ComplianceLevel::PASS,
        QString("FIC section valid (%1 bytes)").arg(frame.fic_data.size()),
        "ETSI EN 300 799 Section 5.4",
        frame.frame_number,
        32
    );
}

ValidationResult ETSIComplianceEngine::validateMSCSection(const ETIFrame& frame)
{
    // MSC section validation (ETSI EN 300 799 Section 5.5)
    if (frame.msc_data.empty()) {
        return ValidationResult(
            ValidationErrorType::MSC_CRC_ERROR,
            ComplianceLevel::WARNING,
            "MSC section is empty",
            "ETSI EN 300 799 Section 5.5",
            frame.frame_number,
            128
        );
    }
    
    // Validate MSC CRC
    if (!validateMSCCRC(frame.msc_data, frame.msc_crc)) {
        return ValidationResult(
            ValidationErrorType::MSC_CRC_ERROR,
            ComplianceLevel::CRITICAL,
            QString("MSC CRC validation failed: calculated CRC differs from frame CRC 0x%1")
                .arg(frame.msc_crc, 4, 16, QChar('0')).toUpper(),
            "ETSI EN 300 799 Section 5.5.2",
            frame.frame_number,
            128
        );
    }
    
    return ValidationResult(
        ValidationErrorType::MSC_CRC_ERROR,
        ComplianceLevel::PASS,
        QString("MSC section valid (%1 bytes)").arg(frame.msc_data.size()),
        "ETSI EN 300 799 Section 5.5",
        frame.frame_number,
        128
    );
}

bool ETSIComplianceEngine::validateFICCRC(const std::vector<uint8_t>& fic_data, uint16_t crc)
{
    // Calculate CRC-16 for FIC data
    uint16_t calculated_crc = calculateCRC16(fic_data);
    return calculated_crc == crc;
}

bool ETSIComplianceEngine::validateMSCCRC(const std::vector<uint8_t>& msc_data, uint16_t crc)
{
    // Calculate CRC-16 for MSC data
    uint16_t calculated_crc = calculateCRC16(msc_data);
    return calculated_crc == crc;
}

uint16_t ETSIComplianceEngine::calculateCRC16(const std::vector<uint8_t>& data)
{
    // CRC-16-CCITT polynomial: 0x1021
    const uint16_t CRC_POLY = 0x1021;
    uint16_t crc = 0xFFFF;
    
    for (uint8_t byte : data) {
        crc ^= (static_cast<uint16_t>(byte) << 8);
        for (int i = 0; i < 8; i++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ CRC_POLY;
            } else {
                crc <<= 1;
            }
        }
    }
    
    return crc;
}

ComplianceReport ETSIComplianceEngine::analyzeETIStream(const std::vector<ETIFrame>& frames)
{
    ComplianceReport report;
    report.stream_name = QString("ETI Stream Analysis");
    report.analysis_time = QDateTime::currentDateTime();
    report.total_frames_analyzed = static_cast<uint32_t>(frames.size());
    
    // Reset frame counter for stream analysis
    m_last_frame_counter = 0;
    
    for (const auto& frame : frames) {
        ValidationResult result = validateETIFrame(frame);
        report.results.push_back(result);
        updateComplianceStatistics(report, result);
    }
    
    // Calculate final compliance score
    report.compliance_score = calculateComplianceScore(report);
    
    emit complianceReportReady(report);
    return report;
}

void ETSIComplianceEngine::updateComplianceStatistics(ComplianceReport& report, const ValidationResult& result)
{
    switch (result.level) {
        case ComplianceLevel::CRITICAL:
            report.critical_errors++;
            break;
        case ComplianceLevel::WARNING:
            report.warnings++;
            break;
        case ComplianceLevel::INFO:
            report.info_notices++;
            break;
        case ComplianceLevel::PASS:
            report.passed_validations++;
            break;
    }
}

double ETSIComplianceEngine::calculateComplianceScore(const ComplianceReport& report)
{
    if (report.total_frames_analyzed == 0) return 0.0;
    
    // Scoring: Critical errors = -10 points, Warnings = -2 points, Pass = +1 point
    double score = (report.passed_validations * 1.0) - (report.critical_errors * 10.0) - (report.warnings * 2.0);
    double max_score = report.total_frames_analyzed * 1.0;
    
    double percentage = (score / max_score) * 100.0;
    return std::max(0.0, std::min(100.0, percentage));
}

QString ETSIComplianceEngine::generateTextReport(const ComplianceReport& report)
{
    QString text_report;
    QTextStream stream(&text_report);
    
    stream << "ETSI EN 300 799 Compliance Report\n";
    stream << "==================================\n\n";
    stream << "Stream: " << report.stream_name << "\n";
    stream << "Analysis Time: " << report.analysis_time.toString("yyyy-MM-dd hh:mm:ss") << "\n";
    stream << "Total Frames: " << report.total_frames_analyzed << "\n";
    stream << "Compliance Score: " << QString::number(report.compliance_score, 'f', 2) << "%\n\n";
    
    stream << "Summary:\n";
    stream << "  Critical Errors: " << report.critical_errors << "\n";
    stream << "  Warnings: " << report.warnings << "\n";
    stream << "  Info Notices: " << report.info_notices << "\n";
    stream << "  Passed Validations: " << report.passed_validations << "\n\n";
    
    if (!report.results.empty()) {
        stream << "Detailed Results:\n";
        stream << "-----------------\n";
        for (const auto& result : report.results) {
            if (result.level != ComplianceLevel::PASS || m_strict_mode) {
                stream << QString("[Frame %1] %2: %3 (%4)\n")
                          .arg(result.frame_number)
                          .arg(result.level == ComplianceLevel::CRITICAL ? "CRITICAL" :
                               result.level == ComplianceLevel::WARNING ? "WARNING" : "INFO")
                          .arg(result.description)
                          .arg(result.etsi_reference);
            }
        }
    }
    
    return text_report;
}

QString ETSIComplianceEngine::generateHTMLReport(const ComplianceReport& report)
{
    QString html_report;
    QTextStream stream(&html_report);
    
    stream << "<!DOCTYPE html>\n<html><head>\n";
    stream << "<title>ETSI EN 300 799 Compliance Report</title>\n";
    stream << "<style>\n";
    stream << "body { font-family: Arial, sans-serif; margin: 20px; }\n";
    stream << ".critical { color: #d32f2f; font-weight: bold; }\n";
    stream << ".warning { color: #f57c00; }\n";
    stream << ".info { color: #1976d2; }\n";
    stream << ".pass { color: #388e3c; }\n";
    stream << "table { border-collapse: collapse; width: 100%; }\n";
    stream << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
    stream << "th { background-color: #f2f2f2; }\n";
    stream << "</style>\n</head><body>\n";
    
    stream << "<h1>ETSI EN 300 799 Compliance Report</h1>\n";
    stream << "<h2>Summary</h2>\n";
    stream << "<p><strong>Stream:</strong> " << report.stream_name << "</p>\n";
    stream << "<p><strong>Analysis Time:</strong> " << report.analysis_time.toString("yyyy-MM-dd hh:mm:ss") << "</p>\n";
    stream << "<p><strong>Total Frames:</strong> " << report.total_frames_analyzed << "</p>\n";
    stream << "<p><strong>Compliance Score:</strong> " << QString::number(report.compliance_score, 'f', 2) << "%</p>\n";
    
    stream << "<table>\n<tr><th>Type</th><th>Count</th></tr>\n";
    stream << "<tr><td class='critical'>Critical Errors</td><td>" << report.critical_errors << "</td></tr>\n";
    stream << "<tr><td class='warning'>Warnings</td><td>" << report.warnings << "</td></tr>\n";
    stream << "<tr><td class='info'>Info Notices</td><td>" << report.info_notices << "</td></tr>\n";
    stream << "<tr><td class='pass'>Passed Validations</td><td>" << report.passed_validations << "</td></tr>\n";
    stream << "</table>\n";
    
    stream << "</body></html>\n";
    
    return html_report;
}

bool ETSIComplianceEngine::saveReportToFile(const ComplianceReport& report, const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&file);
    if (filename.endsWith(".html")) {
        out << generateHTMLReport(report);
    } else {
        out << generateTextReport(report);
    }
    
    return true;
}

void ETSIComplianceEngine::setStrictMode(bool strict)
{
    m_strict_mode = strict;
}

void ETSIComplianceEngine::setValidationLevel(ComplianceLevel min_level)
{
    m_min_validation_level = min_level;
}

} // namespace etsi