/**
 * @file yaml_output_generator.cpp
 * @brief Implementation of YAML Output Generator
 *
 * @author Agent 35 - YAML Output Generator
 * @date 2025-11-05
 */

#include "yaml_output_generator.hpp"
#include <QTextStream>
#include <QFile>
#include <QDebug>

namespace cli {

// ============================================================================
// Constructor
// ============================================================================

YAMLOutputGenerator::YAMLOutputGenerator() {
    m_data.timestamp = QDateTime::currentDateTimeUtc();
}

// ============================================================================
// Data Setters
// ============================================================================

void YAMLOutputGenerator::setInputFile(const QString& filename) {
    m_data.input_file = filename;
}

void YAMLOutputGenerator::setProcessingTime(qint64 ms) {
    m_data.analysis_duration_ms = ms;
}

void YAMLOutputGenerator::setEnsembleInfo(const eti::Ensemble& ensemble) {
    m_data.ensemble = ensemble;
}

void YAMLOutputGenerator::addService(const eti::DabService& service) {
    m_data.services.append(service);
}

void YAMLOutputGenerator::addSubchannel(const eti::fig::Fig01SubchannelInfo& subchannel) {
    m_data.subchannels.append(subchannel);
}

void YAMLOutputGenerator::setFIGStatistics(const QMap<QString, int>& fig_counts) {
    m_data.fig_counts = fig_counts;
}

void YAMLOutputGenerator::setETSICompliance(bool compliant, const QStringList& warnings) {
    m_data.etsi_compliant = compliant;
    m_data.etsi_warnings = warnings;
}

void YAMLOutputGenerator::setPerformanceMetrics(qint64 processing_time, double fps) {
    m_data.processing_time_ms = processing_time;
    m_data.frames_per_second = fps;
}

void YAMLOutputGenerator::setETIStreamInfo(int total_frames, int valid_frames,
                                           int error_frames, int crc_errors) {
    m_data.eti_stream.total_frames = total_frames;
    m_data.eti_stream.valid_frames = valid_frames;
    m_data.eti_stream.error_frames = error_frames;
    m_data.eti_stream.crc_errors = crc_errors;

    // Calculate duration in seconds
    if (total_frames > 0) {
        m_data.eti_stream.duration_seconds = total_frames / 250.0;
    }
}

// ============================================================================
// Output Generation
// ============================================================================

QString YAMLOutputGenerator::generateYAML() const {
    QString yaml;
    QTextStream out(&yaml);

    // Document header
    out << "# StreamDAB Analyzer CLI Output\n";
    out << "version: \"" << m_data.version << "\"\n";
    out << "generator: \"" << m_data.generator << "\"\n";
    out << "timestamp: \"" << m_data.timestamp.toString(Qt::ISODate) << "\"\n";
    out << "input_file: \"" << escapeYAMLString(m_data.input_file) << "\"\n";
    out << "analysis_duration_ms: " << m_data.analysis_duration_ms << "\n";
    out << "\n";

    // ETI stream section
    out << formatETIStream();
    out << "\n";

    // Ensemble section
    out << formatEnsemble();
    out << "\n";

    // Services section
    out << formatServices();
    out << "\n";

    // Subchannels section
    out << formatSubchannels();
    out << "\n";

    // FIG statistics section
    out << formatFIGStatistics();
    out << "\n";

    // ETSI compliance section
    out << formatETSICompliance();
    out << "\n";

    // Performance section
    out << formatPerformance();

    return yaml;
}

bool YAMLOutputGenerator::writeToFile(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << filename;
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    QString yaml = generateYAML();
    bool success = writeToStream(stream, yaml);

    file.close();
    return success;
}

bool YAMLOutputGenerator::writeToStdout() {
    QTextStream stream(stdout);
    stream.setEncoding(QStringConverter::Utf8);

    QString yaml = generateYAML();
    return writeToStream(stream, yaml);
}

// ============================================================================
// YAML Formatting Methods
// ============================================================================

QString YAMLOutputGenerator::formatETIStream() const {
    QString section;
    QTextStream out(&section);

    out << "eti_stream:\n";
    out << "  format: \"" << m_data.eti_stream.format << "\"\n";
    out << "  frame_size: " << m_data.eti_stream.frame_size << "\n";
    out << "  total_frames: " << m_data.eti_stream.total_frames << "\n";
    out << "  valid_frames: " << m_data.eti_stream.valid_frames << "\n";
    out << "  error_frames: " << m_data.eti_stream.error_frames << "\n";
    out << "  frame_rate: " << QString::number(m_data.eti_stream.frame_rate, 'f', 1) << "\n";
    out << "  duration_seconds: " << QString::number(m_data.eti_stream.duration_seconds, 'f', 1) << "\n";
    out << "  crc_errors: " << m_data.eti_stream.crc_errors << "\n";

    return section;
}

QString YAMLOutputGenerator::formatEnsemble() const {
    QString section;
    QTextStream out(&section);

    out << "ensemble:\n";
    out << "  ensemble_id: " << formatHex(m_data.ensemble.ensemble_id) << "\n";
    out << "  ensemble_label: \"" << escapeYAMLString(QString::fromStdString(m_data.ensemble.label)) << "\"\n";
    out << "  country_id: " << formatHex(static_cast<quint16>(m_data.ensemble.country_id)) << "\n";

    // Map country_id to country name (basic mapping)
    QString country_name = "Unknown";
    if (m_data.ensemble.country_id == 0x0E && m_data.ensemble.extended_country_code == 0xE1) {
        country_name = "Thailand";
    } else if (m_data.ensemble.country_id == 0x0D) {
        country_name = "France";
    } else if (m_data.ensemble.country_id == 0x09) {
        country_name = "Germany";
    } else if (m_data.ensemble.country_id == 0x0C) {
        country_name = "United Kingdom";
    }

    out << "  country_name: \"" << country_name << "\"\n";
    out << "  extended_country_code: " << formatHex(static_cast<quint16>(m_data.ensemble.extended_country_code)) << "\n";
    out << "  alarm_flag: " << (m_data.ensemble.alarm_flag ? "true" : "false") << "\n";

    return section;
}

QString YAMLOutputGenerator::formatServices() const {
    QString section;
    QTextStream out(&section);

    out << "services:\n";

    if (m_data.services.isEmpty()) {
        out << "  []\n";
        return section;
    }

    for (const auto& service : m_data.services) {
        out << "  - service_id: " << formatHex(static_cast<quint32>(service.service_id)) << "\n";
        out << "    service_label: \"" << escapeYAMLString(QString::fromStdString(service.label)) << "\"\n";
        out << "    service_type: \"" << getServiceType(service) << "\"\n";
        out << "    is_programme_service: " << (service.is_programme ? "true" : "false") << "\n";

        // Components
        if (!service.components.empty()) {
            out << "    components:\n";
            for (const auto& component : service.components) {
                out << "      - component_id: " << static_cast<int>(component.sub_channel_id) << "\n";
                out << "        subchannel_id: " << static_cast<int>(component.sub_channel_id) << "\n";
                out << "        audio_service_type: " << formatHex(static_cast<quint16>(component.asc_ty)) << "\n";
                out << "        primary: " << (component.primary ? "true" : "false") << "\n";
                out << "        ca_protected: " << (component.ca_flag ? "true" : "false") << "\n";
            }
        } else {
            out << "    components: []\n";
        }
    }

    return section;
}

QString YAMLOutputGenerator::formatSubchannels() const {
    QString section;
    QTextStream out(&section);

    out << "subchannels:\n";

    if (m_data.subchannels.isEmpty()) {
        out << "  []\n";
        return section;
    }

    for (const auto& subchannel : m_data.subchannels) {
        out << "  - subchannel_id: " << static_cast<int>(subchannel.subchannel_id) << "\n";
        out << "    start_address: " << formatHex(subchannel.start_address) << "\n";

        // Calculate size from table index or use subchannel_size for long form
        int size = subchannel.short_form ? (subchannel.table_index * 8) : subchannel.subchannel_size;
        out << "    size: " << size << "\n";

        out << "    protection_form: \"" << formatProtectionLevel(subchannel) << "\"\n";
        out << "    protection_level: " << static_cast<int>(subchannel.get_protection_level()) << "\n";

        // Estimate bitrate (basic calculation)
        int bitrate = size * 8;  // Simplified bitrate calculation
        out << "    bitrate: " << bitrate << "\n";
    }

    return section;
}

QString YAMLOutputGenerator::formatFIGStatistics() const {
    QString section;
    QTextStream out(&section);

    out << "fig_statistics:\n";

    // Calculate total FIGs
    int total_figs = 0;
    for (auto it = m_data.fig_counts.constBegin(); it != m_data.fig_counts.constEnd(); ++it) {
        total_figs += it.value();
    }

    out << "  total_figs_processed: " << total_figs << "\n";

    // FIG type counts
    out << "  fig_type_counts:\n";

    if (m_data.fig_counts.isEmpty()) {
        out << "    {}\n";
    } else {
        // Sort by FIG type for consistent output
        QStringList keys = m_data.fig_counts.keys();
        keys.sort();

        for (const QString& key : keys) {
            out << "    \"" << key << "\": " << m_data.fig_counts[key] << "\n";
        }
    }

    return section;
}

QString YAMLOutputGenerator::formatETSICompliance() const {
    QString section;
    QTextStream out(&section);

    out << "etsi_compliance:\n";
    out << "  overall_compliance: " << (m_data.etsi_compliant ? "true" : "false") << "\n";

    // Warnings
    out << "  warnings:\n";
    if (m_data.etsi_warnings.isEmpty()) {
        out << "    []\n";
    } else {
        for (const QString& warning : m_data.etsi_warnings) {
            out << "    - \"" << escapeYAMLString(warning) << "\"\n";
        }
    }

    return section;
}

QString YAMLOutputGenerator::formatPerformance() const {
    QString section;
    QTextStream out(&section);

    out << "performance:\n";
    out << "  total_processing_time_ms: " << m_data.processing_time_ms << "\n";
    out << "  frames_per_second: " << QString::number(m_data.frames_per_second, 'f', 1) << "\n";

    return section;
}

// ============================================================================
// Helper Methods
// ============================================================================

QString YAMLOutputGenerator::escapeYAMLString(const QString& str) const {
    QString escaped = str;

    // Replace backslashes first
    escaped.replace("\\", "\\\\");

    // Escape double quotes
    escaped.replace("\"", "\\\"");

    // Escape newlines
    escaped.replace("\n", "\\n");

    // Escape carriage returns
    escaped.replace("\r", "\\r");

    // Escape tabs
    escaped.replace("\t", "\\t");

    // NOTE: Thai UTF-8 characters are preserved as-is (no escaping needed)

    return escaped;
}

QString YAMLOutputGenerator::formatHex(quint16 value) const {
    // Format as 0xABCD (lowercase 'x', uppercase hex digits)
    QString hex = QString::number(value, 16).toUpper().rightJustified(4, '0');
    return QString("0x") + hex;
}

QString YAMLOutputGenerator::formatHex(quint32 value) const {
    // Format as 0x12345678 (lowercase 'x', uppercase hex digits)
    QString hex = QString::number(value, 16).toUpper().rightJustified(8, '0');
    return QString("0x") + hex;
}

QString YAMLOutputGenerator::formatProtectionLevel(const eti::fig::Fig01SubchannelInfo& subchannel) const {
    if (subchannel.short_form) {
        // UEP (Unequal Error Protection)
        return QString("UEP");
    } else {
        // EEP (Equal Error Protection)
        QString eep_level;
        if (subchannel.option == 0) {
            eep_level = QString("%1A").arg(subchannel.protection_level);
        } else {
            eep_level = QString("%1B").arg(subchannel.protection_level);
        }
        return QString("EEP-") + eep_level;
    }
}

QString YAMLOutputGenerator::getServiceType(const eti::DabService& service) const {
    if (service.is_programme) {
        // Check if any component uses DAB+ (ASCTy = 0x3F for DAB+)
        bool is_dabplus = false;
        for (const auto& comp : service.components) {
            if (comp.asc_ty == 0x3F || comp.asc_ty >= 0x02) {
                is_dabplus = true;
                break;
            }
        }
        return is_dabplus ? "DAB+ Audio" : "DAB Audio";
    } else {
        return "Data Service";
    }
}

bool YAMLOutputGenerator::writeToStream(QTextStream& stream, const QString& yaml) const {
    if (!stream.device() || !stream.device()->isOpen()) {
        qWarning() << "Stream device is not open";
        return false;
    }

    stream << yaml;
    stream.flush();

    return stream.status() == QTextStream::Ok;
}

} // namespace cli
