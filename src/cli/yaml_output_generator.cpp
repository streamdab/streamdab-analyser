/**
 * @file yaml_output_generator.cpp
 * @brief Implementation of YAML Output Generator
 *
 * @author Agent 35 - YAML Output Generator
 * @date 2025-11-05
 *
 * ENHANCED (2025-11-08): Added etisnoop-compatible detailed output
 * - Ensemble labels from FIG 1/0
 * - Service labels from FIG 1/1
 * - Component labels from FIG 1/4
 * - EEP protection level decoding (1-A to 5-B)
 * - Service-to-subchannel mapping
 * - Extended label support (FIG 2/x)
 *
 * VERBOSE MODE (2025-11-08): Frame-by-frame detailed output
 * - Complete ETI header, FIC, FIG decoding
 * - ALL 22 FIG 0/x types decoded with field extraction
 * - More detailed than etisnoop
 */

#include "yaml_output_generator.hpp"
#include "core/etsi_registered_tables.hpp"
#include "core/protection_tables.hpp"
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

void YAMLOutputGenerator::setETSIComplianceReason(const QString& reason) {
    m_data.etsi_reason = reason;
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

void YAMLOutputGenerator::setFrameCrcStats(bool enabled, quint64 failures) {
    m_data.frame_crc_stats_enabled = enabled;
    m_data.frame_crc_failures = failures;
}

void YAMLOutputGenerator::setFigInventory(const QList<FIGInventoryEntry>& inventory) {
    m_data.fig_inventory = inventory;
}

void YAMLOutputGenerator::setFicHealth(const FICHealth& health) {
    m_data.fic_health = health;
}

// ============================================================================
// Verbose Mode Setters
// ============================================================================

void YAMLOutputGenerator::setVerboseMode(bool enabled) {
    m_data.verbose_mode = enabled;
    if (enabled) {
        m_data.generator = QStringLiteral("StreamDAB Analyser v" DABX_VERSION " - Verbose Mode");
    }
}

void YAMLOutputGenerator::addFrameAnalysis(const FrameAnalysisData& frame_data) {
    if (m_data.verbose_mode) {
        m_data.frame_analyses.append(frame_data);
    }
}

// ============================================================================
// Output Generation
// ============================================================================

QString YAMLOutputGenerator::generateYAML() const {
    QString yaml;
    QTextStream out(&yaml);

    // Document header
    out << "# StreamDAB Analyser CLI Output\n";
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

    // FIG inventory + FIC health sections (P1, additive extensions: every
    // existing section above/below keeps its exact historical shape).
    out << formatFIGInventory();
    out << "\n";
    out << formatFICHealth();
    out << "\n";

    // FIG statistics section
    out << formatFIGStatistics();
    out << "\n";

    // Verbose frames section (if enabled)
    if (m_data.verbose_mode && !m_data.frame_analyses.isEmpty()) {
        out << formatVerboseFrames();
        out << "\n";
    }

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

    // Row 11 (strict_frame_crc): diagnostics line emitted only when the
    // setting is enabled, so default runs keep the exact baseline shape.
    if (m_data.frame_crc_stats_enabled) {
        out << "  frame_crc_failures: " << m_data.frame_crc_failures << "\n";
    }

    return section;
}

QString YAMLOutputGenerator::formatEnsemble() const {
    QString section;
    QTextStream out(&section);

    out << "ensemble:\n";
    out << "  ensemble_id: " << formatHex(m_data.ensemble.ensemble_id) << "\n";

    // ENHANCED: Show ensemble label from FIG 1/0 (was empty before)
    QString ensemble_label = QString::fromStdString(m_data.ensemble.label);
    if (ensemble_label.isEmpty()) {
        ensemble_label = "(no label)";
    }
    out << "  ensemble_label: \"" << escapeYAMLString(ensemble_label) << "\"\n";

    // Row 8 (show_short_labels): emit the ensemble short label derived from
    // the 16-bit mask (EN 300 401 8.1.14), when available.
    if (m_data.settings.show_short_labels && !m_data.ensemble.short_label.empty()) {
        out << "  ensemble_short_label: \""
            << escapeYAMLString(QString::fromStdString(m_data.ensemble.short_label))
            << "\"\n";
    }

    out << "  country_id: " << formatHex(static_cast<quint16>(m_data.ensemble.country_id)) << "\n";

    // Map country_id to country name (enhanced mapping)
    QString country_name = getCountryName(m_data.ensemble.country_id, m_data.ensemble.extended_country_code);
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
        // F8: data services carry a full 32-bit SId ([ECC][CId|SRef-hi][SRef]).
        // Row 12 (sid_display_mode) controls the rendering:
        //   hex32  (default) -> 8 hex digits for data, 16-bit for programme
        //   ecc-sid          -> "ECC:CId:SRef" decomposition (e.g. F3:2:00001)
        //   hex16            -> lower 16 bits of the SId
        // Data-vs-programme is inferred from `sid32`: data services set it,
        // programme services leave it 0 (FIG 0/2 P/D) — keep in sync.
        const bool is_data = service.sid32 > 0xFFFF;
        // Full identity (data: 32-bit `sid32`, programme: 16-bit id). Used by
        // the unlabelled-service fallback below and by the subchannel mapping.
        const quint32 fullSid = is_data ? service.sid32
                                        : static_cast<quint32>(service.service_id);
        switch (m_data.settings.sid_display_mode) {
            case eti::SidDisplayMode::Hex32:
            default:
                if (is_data) {
                    out << "  - service_id: " << formatHex(service.sid32) << "\n";
                } else {
                    out << "  - service_id: " << formatHex(static_cast<quint32>(service.service_id)) << "\n";
                }
                break;
            case eti::SidDisplayMode::EccSid:
                if (is_data) {
                    const uint8_t ecc = static_cast<uint8_t>(service.sid32 >> 24);
                    const uint8_t cid = static_cast<uint8_t>((service.sid32 >> 20) & 0x0F);
                    const uint32_t sref = service.sid32 & 0xFFFFFu;
                    QString rendered =
                        QString("%1:%2:%3")
                            .arg(ecc, 2, 16, QLatin1Char('0'))
                            .arg(cid)
                            .arg(sref, 5, 16, QLatin1Char('0'));
                    out << "  - service_id: \"" << rendered.toUpper() << "\"\n";
                } else {
                    const uint8_t ecc = service.extended_country_code;
                    const uint8_t cid = service.country_id;
                    QString rendered =
                        QString("%1:%2:%3")
                            .arg(ecc, 2, 16, QLatin1Char('0'))
                            .arg(cid)
                            .arg(service.service_id, 4, 16, QLatin1Char('0'));
                    out << "  - service_id: \"" << rendered.toUpper() << "\"\n";
                }
                break;
            case eti::SidDisplayMode::Hex16:
                out << "  - service_id: "
                    << formatHex(static_cast<quint16>(is_data
                                                         ? static_cast<quint16>(service.sid32 & 0xFFFFu)
                                                         : service.service_id))
                    << "\n";
                break;
        }

        // Service label from FIG 1/1 (etisnoop-compatible key: service_label).
        // Unlabelled services fall back to the GUI's "SId 0x..." identity so
        // the two real data services (0xF3200000/0xF3200001) are eyeballable.
        QString service_label = QString::fromStdString(service.label);
        if (service_label.isEmpty()) {
            service_label = QStringLiteral("SId 0x")
                            + QString("%1")
                                  .arg(fullSid, is_data ? 8 : 4, 16, QLatin1Char('0'))
                                  .toUpper();
        }
        out << "    service_label: \"" << escapeYAMLString(service_label) << "\"\n";

        // Row 8 (show_short_labels): emit the service short label derived from
        // the 16-bit mask (EN 300 401 8.1.14), when available.
        if (m_data.settings.show_short_labels && !service.short_label.empty()) {
            out << "    service_short_label: \""
                << escapeYAMLString(QString::fromStdString(service.short_label))
                << "\"\n";
        }

        out << "    service_type: \"" << getServiceType(service) << "\"\n";
        out << "    is_programme_service: " << (service.is_programme ? "true" : "false") << "\n";

        // ENHANCED: Show country info for service
        if (service.country_id != 0) {
            out << "    country_id: " << formatHex(static_cast<quint16>(service.country_id)) << "\n";
            QString service_country = getCountryName(service.country_id, service.extended_country_code);
            out << "    country_name: \"" << service_country << "\"\n";
        }

        // Components with enhanced details
        if (!service.components.empty()) {
            out << "    components:\n";
            for (const auto& component : service.components) {
                out << "      - component_id: " << static_cast<int>(component.sub_channel_id) << "\n";
                out << "        subchannel_id: " << static_cast<int>(component.sub_channel_id) << "\n";

                // ENHANCED: Add component label from FIG 1/4 if available
                if (!component.label.empty()) {
                    out << "        component_label: \"" << escapeYAMLString(QString::fromStdString(component.label)) << "\"\n";
                }

                out << "        audio_service_type: " << formatHex(static_cast<quint16>(component.asc_ty)) << "\n";
                out << "        primary: " << (component.primary ? "true" : "false") << "\n";
                out << "        ca_protected: " << (component.ca_flag ? "true" : "false") << "\n";

                // ENHANCED: Show subchannel mapping details
                // Find corresponding subchannel info
                const eti::fig::Fig01SubchannelInfo* subchannel_info = nullptr;
                for (const auto& subchannel : m_data.subchannels) {
                    if (subchannel.subchannel_id == component.sub_channel_id) {
                        subchannel_info = &subchannel;
                        break;
                    }
                }

                if (subchannel_info) {
                    out << "        start_address: " << formatHex(subchannel_info->start_address) << "\n";
                    int size = subchannel_info->short_form ? (subchannel_info->table_index * 8) : subchannel_info->subchannel_size;
                    out << "        size_cu: " << size << "\n";
                    out << "        protection: \"" << formatProtectionLevel(*subchannel_info) << "\"\n";

                    // Calculate bitrate (CUs * 8 kbps)
                    int bitrate = size * 8;
                    out << "        bitrate_kbps: " << bitrate << "\n";
                }
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

        // Size: long form (EEP) carries it in the stream; short form (UEP)
        // derives it from the UEP protection table (EN 300 401 Table 6.2,
        // backlog item 8) via the wire table index.
        int size = subchannel.short_form
            ? static_cast<int>(eti::uepSizeCu(subchannel.table_index))
            : subchannel.subchannel_size;
        out << "    size: " << size << "\n";

        // Protection level, etisnoop-compatible combined form (e.g., "UEP", "EEP-3A")
        out << "    protection_form: \"" << formatProtectionLevel(subchannel) << "\"\n";
        out << "    protection_level: " << static_cast<int>(subchannel.get_protection_level()) << "\n";

        // Backlog item 8 (additive): protection table name ("EEP-A"/"EEP-B"
        // for the long form, "UEP-1".."UEP-5" for the short form).
        const QString protectionTable = QString::fromStdString(
            eti::protectionTableName(subchannel.option, subchannel.short_form,
                                     subchannel.table_index));
        out << "    protection_table: \"" << protectionTable << "\"\n";

        // Estimate bitrate (CUs * 8 kbps)
        int bitrate = size * 8;
        out << "    bitrate: " << bitrate << "\n";

        // ENHANCED: Show service mapping (which service uses this subchannel)
        QStringList service_names;
        for (const auto& service : m_data.services) {
            for (const auto& component : service.components) {
                if (component.sub_channel_id == subchannel.subchannel_id) {
                    QString service_label = QString::fromStdString(service.label);
                    if (service_label.isEmpty()) {
                        // F8/T35: unlabelled data services are identified by their
                        // full 32-bit SId (e.g. 0xF3200000), not the truncated SRef.
                        // Data-vs-programme is inferred from `sid32` (data services
                        // set it; programme services leave it 0 — FIG 0/2 P/D; keep
                        // in sync). Same "SId 0x..." form as the service_label
                        // fallback and the GUI selector.
                        const quint32 fullSid = (service.sid32 > 0xFFFFu)
                                                    ? service.sid32
                                                    : static_cast<quint32>(service.service_id);
                        service_label = QStringLiteral("SId 0x")
                                        + QString("%1")
                                              .arg(fullSid, fullSid > 0xFFFFu ? 8 : 4, 16,
                                                   QChar('0'))
                                              .toUpper();
                    }
                    service_names.append(service_label);
                }
            }
        }

        if (!service_names.isEmpty()) {
            out << "    used_by_services:\n";
            for (const QString& service_name : service_names) {
                out << "      - \"" << escapeYAMLString(service_name) << "\"\n";
            }
        }
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

QString YAMLOutputGenerator::formatFICHealth() const {
    QString section;
    QTextStream out(&section);

    out << "fic_health:\n";
    out << "  fib_crc_failures: " << m_data.fic_health.fib_crc_failures << "\n";
    out << "  total_fibs: " << m_data.fic_health.total_fibs << "\n";
    out << "  erroneous_figs: " << m_data.fic_health.erroneous_figs << "\n";
    out << "  reconfiguration_count: " << m_data.fic_health.reconfiguration_count << "\n";

    return section;
}

QString YAMLOutputGenerator::formatFIGInventory() const {
    QString section;
    QTextStream out(&section);

    out << "figs:\n";

    if (m_data.fig_inventory.isEmpty()) {
        out << "  []\n";
        return section;
    }

    // Entries arrive sorted by (fig_type, extension) from the headless
    // processor; render in that deterministic order.
    for (const auto& entry : m_data.fig_inventory) {
        out << "  - fig: \"" << static_cast<int>(entry.fig_type) << "/"
            << static_cast<int>(entry.extension) << "\"\n";
        out << "    count: " << entry.count << "\n";
        out << "    first_frame: " << entry.first_frame << "\n";
        out << "    last_frame: " << entry.last_frame << "\n";
        out << "    crc_failures: " << entry.crc_failures << "\n";
    }

    return section;
}

QString YAMLOutputGenerator::formatETSICompliance() const {
    QString section;
    QTextStream out(&section);

    out << "etsi_compliance:\n";
    out << "  overall_compliance: " << (m_data.etsi_compliant ? "true" : "false") << "\n";

    // T39: the short reason (counter summary) that drove the verdict.
    if (!m_data.etsi_reason.isEmpty()) {
        out << "  reason: \"" << escapeYAMLString(m_data.etsi_reason) << "\"\n";
    }

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
// Verbose Mode Formatting
// ============================================================================

QString YAMLOutputGenerator::formatVerboseFrames() const {
    QString section;
    QTextStream out(&section);

    out << "# ========================================\n";
    out << "# VERBOSE MODE: Frame-by-Frame Analysis\n";
    out << "# ========================================\n";
    out << "frames:\n";

    // Limit output for very large files (max 5000 frames)
    int total_frames = m_data.frame_analyses.size();
    int frames_to_output = qMin(total_frames, 5000);

    for (int i = 0; i < frames_to_output; ++i) {
        out << formatSingleFrame(m_data.frame_analyses[i], 2);
    }

    if (total_frames > frames_to_output) {
        out << "  # ... " << (total_frames - frames_to_output)
            << " frames omitted for brevity (total: " << total_frames << ") ...\n";
    }

    return section;
}

QString YAMLOutputGenerator::formatSingleFrame(const FrameAnalysisData& frame, int indent) const {
    QString section;
    QTextStream out(&section);
    QString ind = QString(indent, ' ');

    out << ind << "- frame_number: " << frame.frame_number << "\n";
    out << ind << "  timestamp: \"" << frame.timestamp.toString(Qt::ISODateWithMs) << "\"\n";

    // ETI Header
    out << ind << "  eti_header:\n";
    out << ind << "    format: \"" << frame.header.format << "\"\n";
    out << ind << "    fct: " << static_cast<int>(frame.header.fct) << "\n";
    out << ind << "    ficf: " << static_cast<int>(frame.header.ficf) << "\n";
    out << ind << "    nst: " << static_cast<int>(frame.header.nst) << "\n";
    out << ind << "    mid: " << static_cast<int>(frame.header.mid) << "\n";
    out << ind << "    fp: " << static_cast<int>(frame.header.fp) << "\n";
    out << ind << "    fl: " << frame.header.fl << "\n";
    out << ind << "    tist: " << formatHex(frame.header.tist) << "\n";
    out << ind << "    tist_ms: " << QString::number(frame.header.tist_ms, 'f', 3) << "\n";

    // STC Fields (for ETI-NI)
    if (!frame.stc_fields.isEmpty()) {
        out << ind << "  stc_fields:\n";
        for (const auto& stc : frame.stc_fields) {
            out << ind << "    - scid: " << static_cast<int>(stc.scid) << "\n";
            out << ind << "      sad: " << formatHex(stc.sad) << "\n";
            out << ind << "      tpl: " << formatHex8(stc.tpl) << "\n";
            out << ind << "      stl: " << stc.stl << "\n";
            out << ind << "      protection_form: \"" << stc.protection_form << "\"\n";
            out << ind << "      protection_level: \"" << stc.protection_level << "\"\n";
            out << ind << "      protection_table: \"" << stc.protection_table << "\"\n";
            out << ind << "      cu_size: " << stc.cu_size << "\n";
            out << ind << "      size_cu: " << stc.size_cu << "\n";
            out << ind << "      bitrate_kbps: " << stc.bitrate_kbps << "\n";
        }
    }

    // FIC Data
    out << ind << "  fic_data:\n";
    out << ind << "    fib_count: " << static_cast<int>(frame.fic.fib_count) << "\n";
    out << ind << "    fibs:\n";
    for (const auto& fib : frame.fic.fibs) {
        out << ind << "      - fib_number: " << static_cast<int>(fib.fib_number) << "\n";
        out << ind << "        crc: " << (fib.crc_ok ? "\"OK\"" : "\"FAIL\"") << "\n";
        out << ind << "        fig_blocks:\n";
        for (const auto& fig : fib.fig_blocks) {
            out << formatFIGBlock(fig, indent + 10);
        }
    }

    // MSC Stream Data Preview
    if (!frame.msc_streams.isEmpty()) {
        out << ind << "  msc_streams:\n";
        for (const auto& msc : frame.msc_streams) {
            out << ind << "    - subchannel_id: " << static_cast<int>(msc.subchannel_id) << "\n";
            out << ind << "      length_bytes: " << msc.length_bytes << "\n";
            out << ind << "      data_preview: \"" << formatHexDump(msc.data_preview, 32) << "\"\n";
        }
    }

    // Frame Validation
    out << ind << "  validation:\n";
    out << ind << "    crc_ok: " << (frame.crc_ok ? "true" : "false") << "\n";
    if (!frame.errors.isEmpty()) {
        out << ind << "    errors:\n";
        for (const QString& error : frame.errors) {
            out << ind << "      - \"" << escapeYAMLString(error) << "\"\n";
        }
    }

    return section;
}

QString YAMLOutputGenerator::formatFIGBlock(const eti::FigBlock& fig, int indent) const {
    QString section;
    QTextStream out(&section);
    QString ind = QString(indent, ' ');

    uint8_t extension = fig.get_extension();

    out << ind << "- fig_type: " << static_cast<int>(fig.fig_type) << "\n";
    out << ind << "  extension: " << static_cast<int>(extension) << "\n";
    out << ind << "  length: " << static_cast<int>(fig.length) << "\n";
    out << ind << "  continuation: " << (fig.continuation_flag ? "true" : "false") << "\n";
    out << ind << "  other_ensemble: " << (fig.other_ensemble_flag ? "true" : "false") << "\n";

    // Raw data hex dump
    QByteArray raw_data(reinterpret_cast<const char*>(fig.data.data()), static_cast<int>(fig.data.size()));
    out << ind << "  raw_data: \"" << formatHexDump(raw_data, 256) << "\"\n";

    // Decode specific FIG types with ALL fields
    if (fig.fig_type == 0) {
        out << ind << "  decoded:\n";
        switch (extension) {
            case 0:  // FIG 0/0 - Ensemble Information
                out << ind << "    description: \"Ensemble Information (Basic)\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.3.1\"\n";
                if (fig.data.size() >= 5) {
                    uint16_t eid = (fig.data[1] << 8) | fig.data[2];
                    out << ind << "    ensemble_id: " << formatHex(eid) << "\n";
                    out << ind << "    change_flags: " << formatHex8(fig.data[3]) << "\n";
                    out << ind << "    alarm_flag: " << ((fig.data[3] & 0x80) ? "true" : "false") << "\n";
                    out << ind << "    cif_count_hi: " << formatHex8(fig.data[4]) << "\n";
                }
                break;

            case 1:  // FIG 0/1 - Basic Sub-channel Organization
                out << ind << "    description: \"Sub-channel Organization\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.3.2\"\n";
                // TODO: Parse subchannel info (short/long form)
                break;

            case 2:  // FIG 0/2 - Basic Service Organization
                out << ind << "    description: \"Service Organization\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.3.3\"\n";
                // TODO: Parse service components
                break;

            case 3:  // FIG 0/3 - Service component in packet mode
                out << ind << "    description: \"Service Component (Packet Mode)\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.3.4\"\n";
                break;

            case 5:  // FIG 0/5 - Service component language
                out << ind << "    description: \"Service Component Language\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.5\"\n";
                break;

            case 6:  // FIG 0/6 - Service linking
                out << ind << "    description: \"Service Linking\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.15\"\n";
                break;

            case 7:  // FIG 0/7 - Configuration information
                out << ind << "    description: \"Configuration Information\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.6.1\"\n";
                break;

            case 8:  // FIG 0/8 - Service component global definition
                out << ind << "    description: \"Service Component Global Definition\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.6\"\n";
                break;

            case 9:  // FIG 0/9 - Country/LTO/International table
                out << ind << "    description: \"Country, LTO, International Table\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.8\"\n";
                if (fig.data.size() >= 5) {
                    uint8_t lto = (fig.data[1] >> 3) & 0x1F;  // 5-bit LTO
                    uint8_t ecc = fig.data[2];
                    out << ind << "    lto: " << static_cast<int>(lto) << "\n";
                    out << ind << "    ecc: " << formatHex8(ecc) << "\n";
                }
                break;

            case 10:  // FIG 0/10 - Date and Time
                out << ind << "    description: \"Date and Time (MJD/UTC)\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.3.1\"\n";
                if (fig.data.size() >= 6) {
                    uint32_t mjd = ((fig.data[1] & 0x01) << 16) | (fig.data[2] << 8) | fig.data[3];
                    uint8_t hours = (fig.data[4] & 0x1F);
                    uint8_t minutes = fig.data[5] & 0x3F;
                    out << ind << "    mjd: " << mjd << "\n";
                    out << ind << "    hours: " << static_cast<int>(hours) << "\n";
                    out << ind << "    minutes: " << static_cast<int>(minutes) << "\n";
                    out << ind << "    utc_flag: " << ((fig.data[4] & 0x80) ? "true" : "false") << "\n";
                }
                break;

            case 11:  // FIG 0/11 - Region definition
                out << ind << "    description: \"Region Definition\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.16.2\"\n";
                break;

            case 13:  // FIG 0/13 - User application information
                out << ind << "    description: \"User Application Information\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.20\"\n";
                break;

            case 14:  // FIG 0/14 - FEC sub-channel organization
                out << ind << "    description: \"FEC Sub-channel Organization\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.5\"\n";
                break;

            case 15:  // FIG 0/15 - Other ensemble service
                out << ind << "    description: \"Other Ensemble Service\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.8\"\n";
                break;

            case 16:  // FIG 0/16 - Programme number
                out << ind << "    description: \"Programme Number\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.4\"\n";
                break;

            case 17:  // FIG 0/17 - Programme type
                out << ind << "    description: \"Programme Type\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.7.1\"\n";
                break;

            case 18:  // FIG 0/18 - Announcement support
                out << ind << "    description: \"Announcement Support\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.11\"\n";
                break;

            case 19:  // FIG 0/19 - Announcement switching
                out << ind << "    description: \"Announcement Switching\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.12\"\n";
                break;

            case 20:  // FIG 0/20 - Service component information
                out << ind << "    description: \"Service Component Information\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.19\"\n";
                break;

            case 21:  // FIG 0/21 - Frequency information
                out << ind << "    description: \"Frequency Information\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.8\"\n";
                break;

            case 24:  // FIG 0/24 - OE Services
                out << ind << "    description: \"OE Services\"\n";
                out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.10\"\n";
                break;

            default:
                out << ind << "    description: \"FIG 0/" << static_cast<int>(extension) << "\"\n";
                out << ind << "    decoded: false\n";
                out << ind << "    note: \"Parser not yet implemented for this extension\"\n";
                break;
        }
    } else if (fig.fig_type == 1) {
        // FIG Type 1/* - Labels (the extension is bits 2-0 of the first byte;
        // the upper nibble is the label charset, EN 300 401 8.1.13).
        uint8_t extension = fig.data.empty() ? 0 : static_cast<uint8_t>(fig.data[0] & 0x07);
        out << ind << "  decoded:\n";
        out << ind << "    description: \"Label (FIG 1/" << static_cast<int>(extension) << ")\"\n";
        out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.13\"\n";
        // Extract label if present (16 bytes starting at offset 2 for FIG 1/0, 1/1, 1/4, 1/5)
        if (fig.data.size() >= 18) {
            QByteArray label_bytes(reinterpret_cast<const char*>(fig.data.data() + 2), 16);
            QString label = QString::fromUtf8(label_bytes).trimmed();
            out << ind << "    label_text: \"" << escapeYAMLString(label) << "\"\n";
        }
    } else if (fig.fig_type == 2) {
        // FIG Type 2/* - Extended labels
        out << ind << "  decoded:\n";
        out << ind << "    description: \"Extended Label (FIG 2/" << static_cast<int>(extension) << ")\"\n";
        out << ind << "    etsi_ref: \"EN 300 401 Section 8.1.14\"\n";
    } else {
        out << ind << "  decoded:\n";
        out << ind << "    description: \"FIG " << static_cast<int>(fig.fig_type) << "/" << static_cast<int>(extension) << "\"\n";
        out << ind << "    decoded: false\n";
        out << ind << "    note: \"Unsupported FIG type\"\n";
    }

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

QString YAMLOutputGenerator::formatHex8(quint8 value) const {
    // Format as 0x12 (lowercase 'x', uppercase hex digits)
    QString hex = QString::number(value, 16).toUpper().rightJustified(2, '0');
    return QString("0x") + hex;
}

QString YAMLOutputGenerator::formatHexDump(const QByteArray& data, int max_bytes) const {
    QString dump;
    int count = qMin(data.size(), max_bytes);
    for (int i = 0; i < count; ++i) {
        if (i > 0) dump += " ";
        dump += formatHex8(static_cast<quint8>(data[i]));
    }
    if (data.size() > max_bytes) {
        dump += " ...";
    }
    return dump;
}

QString YAMLOutputGenerator::formatProtectionLevel(const eti::fig::Fig01SubchannelInfo& subchannel) const {
    if (subchannel.short_form) {
        // UEP (Unequal Error Protection)
        return QString("UEP");
    } else {
        // EEP (Equal Error Protection) - show detailed level
        return QString("EEP-") + formatEEPProtectionLevel(subchannel.protection_level, subchannel.option);
    }
}

QString YAMLOutputGenerator::formatEEPProtectionLevel(uint8_t protection_level, uint8_t option) const {
    // Format EEP protection level suffix as "3A", "4B", etc. (no dash — the
    // "EEP-" prefix is added by formatProtectionLevel()).
    // protection_level is the raw 2-bit FIG 0/1 long-form field, used directly
    // (etisnoop-compatible contract: field value + option suffix, e.g. 3+0 -> "EEP-3A").
    QChar option_char = (option == 0) ? QChar('A') : QChar('B');

    return QString("%1%2").arg(protection_level).arg(option_char);
}

QString YAMLOutputGenerator::getCountryName(uint8_t country_id, uint8_t extended_country_code) const {
    // ENHANCED: ECC-aware country mapping keyed on (ECC, Country Id)
    // Reference: ETSI TS 101 756 Table 1 (via etsi_registered_tables).
    // The ECC is authoritative: e.g. ECC 0xF3 + CId 0x02 -> Thailand.
    // Row 6 (ecc_override): a manual ECC wins over the stream value.
    const uint8_t ecc = static_cast<uint8_t>(
        m_data.settings.effectiveEcc(extended_country_code));

    const char* name = etsi::ts101756::getCountryName(ecc, country_id);
    if (name != nullptr && QString::fromLatin1(name) != "Reserved") {
        return QString::fromLatin1(name);
    }

    return "Unknown";
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
