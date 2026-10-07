/**
 * @file headless_eti_processor.cpp
 * @brief FIXED: HeadlessETIProcessor with REAL ETI frame parsing
 *
 * BUG FIX (2025-11-06): Added actual frame parsing and FIG analysis
 * Previously: Frames were counted but NOT parsed - all data empty
 * Now: Full ETI frame parsing, FIG extraction, service/subchannel discovery
 *
 * AGENT 45 FIX (2025-11-08): Added result aggregation for YAML output
 * Previously: Data discovered but NOT exported to ProcessingResult
 * Now: Ensemble, services, subchannels, and FIG counts populated in result
 *
 * VERBOSE FRAMES FIX (2025-11-08): Added per-frame detailed data collection
 * Collects: ETI header, STC fields, FIC data, FIG blocks for each frame
 */

#include "headless_eti_processor.hpp"
#include "eti_processor.hpp"
#include "modern_eti_frame_parser.hpp"
#include "enhanced_fig_analyser.hpp"
// #include "comprehensive_etsi_validator.hpp"  // DISABLED: All methods commented out for CLI build
#include "utils/logger.h"
#include "eti_types.hpp"
#include "fig_parser.hpp"
#include "protection_tables.hpp"
#include "../cli/yaml_output_generator.hpp"  // For FrameAnalysisData

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QLoggingCategory>
#include <QSet>
#include <QMap>
#include <QDateTime>

#include <iostream>
#include <set>

// Define logging category for CLI mode
Q_LOGGING_CATEGORY(etiMain, "streamdab.eti")

namespace {

/**
 * @brief Validate the EOH gap CRC (option-variant matrix row 11).
 *
 * EN 300 799 clause 5.3.4 / etisnoop etianalyse.cpp "Header CRC": the 2-byte
 * CRC at offset 8+4*NST+2 validates the FC field (bytes 4..7) plus the STC
 * region (bytes 8 .. 8+4*NST-1) with CRC-16/CCITT-FALSE (init 0xFFFF) and is
 * transmitted complemented (ccitt-false + ~). Frame-relative offsets work for
 * both the ETI-LI (0x49.. sync) and ETI-NI (0xFF.. sync) layouts used here.
 *
 * @param frameData 6144-byte frame buffer
 * @param nst       Effective number of sub-channels
 * @return true when the stored CRC matches the computed value
 */
bool validateEohCrc(const uint8_t* frameData, quint64 nst)
{
    const size_t crc_off = 8 + static_cast<size_t>(nst) * 4 + 2;
    if (crc_off + 2 > 6144) {
        return false;
    }
    const uint16_t stored = static_cast<uint16_t>((frameData[crc_off] << 8) |
                                                  frameData[crc_off + 1]);
    const size_t data_len = 4 + static_cast<size_t>(nst) * 4;  // FC + STC
    const uint16_t calc = static_cast<uint16_t>(
        ~eti::crc16ccitt_false(frameData + 4, data_len));
    return calc == stored;
}

/**
 * @brief Escape a FIG trace label value for key=value output.
 *
 * The trace lines stay machine-parseable: values are double-quoted and
 * embedded quotes/backslashes are escaped. Thai UTF-8 passes through raw.
 */
QString escapeTraceValue(const QString& value)
{
    QString escaped = value;
    escaped.replace('\\', "\\\\");
    escaped.replace('"', "\\\"");
    escaped.replace('\n', " ");
    return escaped;
}

/**
 * @brief Count sub-channel entries carried by one FIG 0/1 instance.
 *
 * Mirrors ModernETIFrameParser::processFIG01_SubchannelOrg's entry walk:
 * [ext byte] then per entry SubChId|SADr (2 bytes) + form bit at entry byte 2
 * (long/EEP form = 4 bytes, short/UEP form = 3 bytes).
 */
int countFig01Subchannels(const eti::FigBlock& fig)
{
    if (fig.data.size() < 2) {
        return 0;
    }
    int count = 0;
    size_t offset = 1;  // Skip the FIG 0 extension byte
    while (offset + 2 < fig.data.size()) {
        if (fig.data[offset + 2] & 0x80) {
            // Long form (EEP): needs the 4th byte
            if (offset + 3 >= fig.data.size()) {
                break;
            }
            offset += 4;
        } else {
            // Short form (UEP)
            offset += 3;
        }
        ++count;
    }
    return count;
}

/**
 * @brief Count service components carried by one FIG 0/2 instance.
 *
 * Mirrors ModernETIFrameParser::processFIG02_ServiceOrg's entry walk:
 * [ext byte] then per service entry: SId (2 bytes programme / 4 bytes data,
 * P/D flag of the ext byte), 1 byte local|CAid|ncomp, ncomp x 2-byte
 * components. Returns the number of component entries consumed.
 */
int countFig02Components(const eti::FigBlock& fig)
{
    if (fig.data.size() < 4) {
        return 0;
    }
    const bool pd = (fig.data[0] & 0x20) != 0;  // P/D flag
    int components = 0;
    size_t offset = 1;
    while (offset + 3 < fig.data.size()) {
        offset += pd ? 4 : 2;
        if (offset >= fig.data.size()) {
            break;
        }
        const uint8_t ncomp = fig.data[offset] & 0x0F;
        ++offset;
        uint8_t consumed = 0;
        for (; consumed < ncomp && offset + 1 < fig.data.size(); ++consumed) {
            offset += 2;
        }
        components += consumed;
    }
    return components;
}

/**
 * @brief Number of service-label entries carried by one FIG 1/1 instance.
 *
 * Entry layout: [charset|OE|ext 1B] [SId 2B] [label 16B] (+[mask 2B] when the
 * 21-byte form fits), i.e. 19 or 21 bytes per entry.
 */
int countFig11Labels(const eti::FigBlock& fig)
{
    int entries = 0;
    size_t pos = 0;
    const size_t size = fig.data.size();
    while (pos + 19 <= size) {
        pos += (pos + 21 <= size) ? 21 : 19;
        ++entries;
    }
    return entries;
}

/**
 * @brief Readable protection summary of the first FIG 0/1 entry.
 *
 * Walks the entry list with the same rules as processFIG01_SubchannelOrg and
 * emits the representative protection of the first entry: long form ->
 * "protection=EEP-3A cu_size=6" (EEP-A 16/8/6/4, EEP-B 27/21/18/15 CU per
 * level 1..4); short form -> "protection=UEP-3 index=7 cu_size=35" (UEP
 * protection table per EN 300 401 §6.2). Backlog item (8).
 */
QString fig01ProtectionSummary(const eti::FigBlock& fig)
{
    if (fig.data.size() < 2) {
        return QString();
    }
    size_t offset = 1;  // Skip the FIG 0 extension byte
    QString out;
    while (offset + 2 < fig.data.size()) {
        const uint8_t b2 = fig.data[offset + 2];
        if (b2 & 0x80) {
            // Long form (EEP)
            if (offset + 3 >= fig.data.size()) {
                break;
            }
            const uint8_t option = static_cast<uint8_t>((b2 >> 4) & 0x07);
            const uint8_t level = static_cast<uint8_t>(((b2 >> 2) & 0x03) + 1);
            const uint16_t cu = eti::eepSizeCu(option, level);
            if (out.isEmpty()) {
                const QChar optionChar = (option == 0) ? QChar('A')
                    : (option == 1) ? QChar('B') : QChar('?');
                out = QString(" protection=EEP-%1%2 cu_size=%3")
                          .arg(level).arg(optionChar).arg(cu);
            }
            offset += 4;
        } else {
            // Short form (UEP)
            const uint8_t table_index = static_cast<uint8_t>(b2 & 0x3F);
            if (out.isEmpty()) {
                out = QString(" protection=UEP-%1 index=%2 cu_size=%3")
                          .arg(eti::uepLevel1Based(table_index))
                          .arg(table_index)
                          .arg(eti::uepSizeCu(table_index));
            }
            offset += 3;
        }
    }
    return out;
}

/**
 * @brief Compact readable summary for a decoded FIG instance.
 *
 * Appends key=value pairs (comma-separated, already prefixed with a space)
 * for the FIGs whose decoded data is available:
 *   FIG 0/0 -> ensemble id, FIG 0/1 -> sub-channel entry count + first
 *   entry's protection (EEP/UEP table value, backlog item 8),
 *   FIG 0/2 -> service component count, FIG 0/9 -> ECC,
 *   FIG 0/21 -> frequency(ies), FIG 0/22 -> coordinates,
 *   FIG 1/0 -> ensemble label, FIG 1/1 -> per-instance service labels,
 *   FIG 1/2 + 1/3 -> component / data-service labels
 * (charset-decoded by the parser; labels are drawn from the per-frame
 * decoded service list in FIG order).
 *
 * @param fig            The FIG block
 * @param ensembleLabel  Charset-decoded ensemble label (FIG 1/0)
 * @param serviceLabels  Per-frame decoded service labels in FIG order (FIG 1/1)
 * @param decoded        Per-frame decode result (backlog decoders)
 * @param counts         Per-block decoder entry counts for @p fig
 * @param labelsConsumed  In/out: how many labels of @p serviceLabels were
 *                        already assigned to earlier 1/1 instances
 * @param freqsConsumed  In/out: consumed FIG 0/21 entries
 * @param geosConsumed   In/out: consumed FIG 0/22 entries
 * @param compsConsumed  In/out: consumed FIG 1/2 + 1/3 labels
 * @param fig2sConsumed  In/out: consumed FIG type 2 entries
 * @return Empty string or " key=value key=value .." (leading space)
 */
QString figReadableSummary(const eti::FigBlock& fig,
                           const QString& ensembleLabel,
                           const QStringList& serviceLabels,
                           const eti::modern::FIGAnalysisResult& decoded,
                           const eti::modern::FigDecodeCounts& counts,
                           size_t& labelsConsumed,
                           size_t& freqsConsumed,
                           size_t& geosConsumed,
                           size_t& compsConsumed,
                           size_t& fig2sConsumed)
{
    QString out;
    const uint8_t ext = fig.get_extension();

    switch (fig.fig_type) {
        case 0:
            if (ext == 0 && fig.data.size() >= 5) {
                const uint16_t eid = static_cast<uint16_t>((fig.data[1] << 8) | fig.data[2]);
                out += QString(" ensemble=0x%1").arg(eid, 4, 16, QChar('0'));
            } else if (ext == 1) {
                out += QString(" subch=%1").arg(countFig01Subchannels(fig));
                out += fig01ProtectionSummary(fig);
            } else if (ext == 2) {
                out += QString(" components=%1").arg(countFig02Components(fig));
            } else if (ext == 9 && fig.data.size() >= 3) {
                out += QString(" ecc=0x%1").arg(fig.data[2], 2, 16, QChar('0'));
            } else if (ext == 21) {
                // FIG 0/21: frequency info (backlog P2/P3). The first
                // frequency of the instance is the representative value;
                // additional frequencies are listed with n_freqs.
                QStringList freqs;
                for (uint8_t k = 0; k < counts.frequencies; ++k) {
                    const size_t ix = freqsConsumed + k;
                    if (ix >= decoded.frequency_infos.size()) break;
                    const auto& fe = decoded.frequency_infos[ix];
                    if (fe.freq_khz != 0) {
                        freqs << QString("%1MHz")
                                     .arg(QString::number(fe.freq_khz / 1000.0, 'f', 3));
                    }
                }
                if (freqs.size() == 1) {
                    out += QString(" freq=%1").arg(freqs.first());
                } else if (!freqs.isEmpty()) {
                    out += QString(" freqs=%1").arg(freqs.join("|"));
                }
                out += QString(" n_freqs=%1").arg(counts.frequencies);
            } else if (ext == 22) {
                // FIG 0/22: geographical location (backlog P2/P3).
                QStringList coords;
                QStringList offsets;
                for (uint8_t k = 0; k < counts.geos; ++k) {
                    const size_t ix = geosConsumed + k;
                    if (ix >= decoded.geo_coords.size()) break;
                    const auto& ge = decoded.geo_coords[ix];
                    const QString lat = QString("%1%2")
                        .arg(ge.latitude < 0 ? '-' : '+')
                        .arg(QString::number(qAbs(ge.latitude), 'f', 1));
                    const QString lon = QString("%1%2")
                        .arg(ge.longitude < 0 ? '-' : '+')
                        .arg(QString::number(qAbs(ge.longitude), 'f', 1));
                    coords << QString("lat=%1 lon=%2").arg(lat, lon);
                    if (!ge.is_main) {
                        offsets << QString("td=%1 dlat=%2 dlon=%3")
                                       .arg(ge.td).arg(ge.lat_offset).arg(ge.lon_offset);
                    }
                }
                if (coords.size() == 1) {
                    out += QString(" %1").arg(coords.first());
                } else if (!coords.isEmpty()) {
                    out += QString(" coords=[%1]").arg(coords.join("; "));
                }
                if (!offsets.isEmpty()) {
                    out += QString(" offsets=[%1]").arg(offsets.join("; "));
                }
            }
            break;
        case 1:
            if (ext == 0 && !ensembleLabel.isEmpty()) {
                out += QString(" label=\"%1\"").arg(escapeTraceValue(ensembleLabel));
            } else if (ext == 1) {
                const int entries = countFig11Labels(fig);
                QStringList labels;
                while (labels.size() < entries && labelsConsumed < serviceLabels.size()) {
                    labels.append(serviceLabels[labelsConsumed]);
                    ++labelsConsumed;
                }
                if (!labels.isEmpty()) {
                    out += QString(" label=\"%1\"")
                               .arg(escapeTraceValue(labels.join("|")));
                }
            } else if (ext == 2) {
                // FIG 1/2: service component label (backlog P2/P3)
                for (uint8_t k = 0; k < counts.component_labels; ++k) {
                    const size_t ix = compsConsumed + k;
                    if (ix >= decoded.component_labels.size()) break;
                    const auto& cl = decoded.component_labels[ix];
                    out += QString(" scids=%1 sid=0x%2 label=\"%3\"")
                               .arg(cl.scids)
                               .arg(cl.service_id, 8, 16, QChar('0'))
                               .arg(escapeTraceValue(QString::fromStdString(cl.label)));
                }
            } else if (ext == 3) {
                // FIG 1/3: data service label (backlog P2/P3)
                for (uint8_t k = 0; k < counts.component_labels; ++k) {
                    const size_t ix = compsConsumed + k;
                    if (ix >= decoded.component_labels.size()) break;
                    const auto& cl = decoded.component_labels[ix];
                    out += QString(" sid=0x%1 label=\"%2\"")
                               .arg(cl.service_id, 8, 16, QChar('0'))
                               .arg(escapeTraceValue(QString::fromStdString(cl.label)));
                }
            }
            break;
        case 2: {
            // FIG type 2 (OTH): minimal readable header + first-segment label
            // (backlog P2/P3).
            for (uint8_t k = 0; k < counts.fig2; ++k) {
                const size_t ix = fig2sConsumed + k;
                if (ix >= decoded.fig2_infos.size()) break;
                const auto& f2 = decoded.fig2_infos[ix];
                out += QString(" fig2:ext=%1 toggle=%2 seg=%3")
                           .arg(f2.extension).arg(f2.toggle).arg(f2.segment_index);
                if (f2.ucs2) {
                    out += " enc=UCS-2";
                }
                if (f2.service_id != 0) {
                    out += QString(" id=0x%1").arg(f2.service_id, 8, 16, QChar('0'));
                }
                if (!f2.label.empty()) {
                    out += QString(" label=\"%1\"")
                               .arg(escapeTraceValue(QString::fromStdString(f2.label)));
                }
            }
            break;
        }
        default:
            break;
    }

    // Advance the consumed-counters for the backlog decoder entries that
    // belong to this block (their counts were recorded per-block by the
    // parser; labelsConsumed advances inside the FIG 1/1 loop above).
    freqsConsumed += counts.frequencies;
    geosConsumed += counts.geos;
    compsConsumed += counts.component_labels;
    fig2sConsumed += counts.fig2;

    return out;
}

/**
 * @brief Per-frame MCI state used for reconfiguration detection (P1).
 *
 * "MCI change" = the ensemble id, the set of service ids, or the set of
 * sub-channel ids decoded from this frame differs from the previous frame's.
 */
struct MciState {
    uint16_t ensembleId{0};
    std::set<uint32_t> services;
    std::set<uint16_t> subchannels;
    bool valid{false};

    bool operator==(const MciState& other) const {
        return ensembleId == other.ensembleId &&
               services == other.services &&
               subchannels == other.subchannels;
    }
    bool operator!=(const MciState& other) const { return !(*this == other); }
};

/**
 * @brief Aggregated per-FIG instance group for the --figs-by-type view.
 *
 * Keyed by (fig_type << 8) | extension; collects the 1-based frame numbers
 * of every instance and the deduplicated readable value strings (the
 * fig-trace summary content without the leading space).
 */
struct FigTypeGroup {
    quint64 count{0};
    QStringList frames;
    QStringList values;
};

/**
 * @brief Per-FIB hex dump lines for one frame (--fib-hex, backlog P2/P3).
 *
 * Walks the raw frame's FIC (offset 12 + 4*NST, EN 300 799: SYNC 4 + FC 4 +
 * STC 4*NST + EOH 4) and emits, per FIB:
 *   fib-hex: frame=N fib=F crc=ok|bad
 *   bytes: <30 hex bytes of the FIG area>
 *   fig: [a-b] FIG t/e [c-d] FIG t/e ...
 * The FIB CRC is recomputed over the 30 FIG bytes (CRC-16/CCITT-FALSE,
 * complemented, big-endian stored at bytes 30-31). FIG markers follow the
 * shared walk rules: header type(3)|length(5), type 7 = padding end, a FIG
 * occupies bytes [pos, pos+length] inclusive.
 *
 * @param frameData    Raw 6144-byte frame
 * @param nst          Effective NST (nst_offset_1 correction applied)
 * @param frameNumber  1-based frame number for the output
 * @param fibCount     Number of FIBs walked (strict walk; 0 -> no output)
 * @return Emitted lines (caller forwards them to stderr)
 */
QStringList fibHexDumpLines(const QByteArray& frameData, quint64 nst,
                            quint64 frameNumber, quint64 fibCount)
{
    QStringList lines;
    if (fibCount == 0 || frameData.size() < 6144) {
        return lines;
    }
    const size_t ficOffset = 12 + static_cast<size_t>(nst) * 4;
    for (quint64 fib = 0; fib < fibCount; ++fib) {
        const size_t base = ficOffset + static_cast<size_t>(fib) * 32;
        if (base + 32 > 6144) {
            break;
        }
        const uint8_t* f = reinterpret_cast<const uint8_t*>(frameData.constData()) + base;

        // Recompute the FIB CRC over the 30-byte FIG area.
        const uint16_t stored = static_cast<uint16_t>((f[30] << 8) | f[31]);
        const uint16_t expected = static_cast<uint16_t>(
            ~eti::crc16ccitt_false(f, 30));
        const bool crcOk = (stored == expected);

        lines << QString("fib-hex: frame=%1 fib=%2 crc=%3")
                     .arg(frameNumber).arg(fib + 1).arg(crcOk ? "ok" : "bad");

        QString hex;
        for (int b = 0; b < 30; ++b) {
            hex += QString("%1 ").arg(f[b], 2, 16, QChar('0'));
        }
        lines << "  bytes: " + hex.trimmed();

        // FIG byte-range markers under the hex line.
        QStringList markers;
        int pos = 0;
        while (pos < 30) {
            const uint8_t hdr = f[pos];
            if ((hdr & 0xE0) == 0xE0) {
                break;  // type 7: padding end marker
            }
            const uint8_t type = static_cast<uint8_t>((hdr >> 5) & 0x07);
            const uint8_t len = hdr & 0x1F;
            if (len == 0 || pos + len > 29) {
                break;
            }
            // Extension extraction mirrors the parser dispatch (FIG 1 and
            // type 2 pack the ext in bits 2-0 of their first data byte).
            const uint8_t dataByte = f[pos + 1];
            const uint8_t ext = (type == 1 || type == 2)
                ? static_cast<uint8_t>(dataByte & 0x07)
                : static_cast<uint8_t>(dataByte & 0x1F);
            markers << QString("[%1-%2] FIG %3/%4")
                           .arg(pos).arg(pos + len).arg(type).arg(ext);
            pos += len + 1;
        }
        if (!markers.isEmpty()) {
            lines << "  fig: " + markers.join(" ");
        }
    }
    return lines;
}

} // namespace

HeadlessETIProcessor::HeadlessETIProcessor(QObject* parent)
    : QObject(parent)
    , m_progress(0.0)
    , m_statusText("")
    , m_verbose_mode(false)
    , m_fig_trace_mode(false)
    , m_max_frames(0)
{
    Logger::instance().log(Logger::Info, "HeadlessETIProcessor", "HeadlessETIProcessor created for batch processing");
}

HeadlessETIProcessor::~HeadlessETIProcessor()
{
    Logger::instance().log(Logger::Info, "HeadlessETIProcessor", "HeadlessETIProcessor destroyed");
}

HeadlessETIProcessor::ProcessingResult HeadlessETIProcessor::process_file(const QString& inputFile, const QString& outputFile)
{
    ProcessingResult result;

    // Reset processing state
    reset_processing_state();

    // Set verbose mode in result
    result.verbose_mode = m_verbose_mode;

    // Validate input file exists
    QFileInfo inputInfo(inputFile);
    if (!inputInfo.exists() || !inputInfo.isReadable()) {
        result.success = false;
        result.errorMessage = QString("Input file not found or not readable: %1").arg(inputFile);
        return result;
    }

    // Start processing timer
    m_processingTimer.start();

    // Initialize components
    if (!initialize_components()) {
        result.success = false;
        result.errorMessage = "Failed to initialize processing components";
        return result;
    }

    // Emit initial status
    m_statusText = "Starting ETI file processing...";
    emit status_changed(m_statusText);
    emit progress_changed(0.1);

    // Basic file validation (check if it looks like ETI data)
    QFile file(inputFile);
    if (!file.open(QIODevice::ReadOnly)) {
        result.success = false;
        result.errorMessage = QString("Cannot open input file: %1").arg(inputFile);
        return result;
    }

    // Read first few bytes to validate ETI format
    QByteArray header = file.read(16);
    file.close();

    if (header.size() < 4) {
        result.success = false;
        result.errorMessage = "Input file too small to be valid ETI";
        return result;
    }

    // Check for ETI sync pattern: ETI-LI or ETI-NI
    // ETI-LI: 0x49 0x93 0x1E 0x03
    // ETI-NI: 0xFF 0x?? 0x?? 0x?? (with CRC)
    bool isETI_LI = (static_cast<uint8_t>(header[0]) == 0x49 &&
                     static_cast<uint8_t>(header[1]) == 0x93 &&
                     static_cast<uint8_t>(header[2]) == 0x1E &&
                     static_cast<uint8_t>(header[3]) == 0x03);

    bool isETI_NI = (static_cast<uint8_t>(header[0]) == 0xFF);

    bool looksLikeETI = (isETI_LI || isETI_NI);

    if (!looksLikeETI) {
        result.success = false;
        result.errorMessage = QString("Input file does not appear to be valid ETI format (sync=%1 %2 %3 %4)")
            .arg(static_cast<uint8_t>(header[0]), 2, 16, QChar('0'))
            .arg(static_cast<uint8_t>(header[1]), 2, 16, QChar('0'))
            .arg(static_cast<uint8_t>(header[2]), 2, 16, QChar('0'))
            .arg(static_cast<uint8_t>(header[3]), 2, 16, QChar('0'))
            ;
        return result;
    }

    qCInfo(etiMain) << "Detected ETI format:" << (isETI_LI ? "ETI-LI" : "ETI-NI");

    // Calculate expected number of ETI frames
    QFileInfo fileInfo(inputFile);
    qint64 fileSize = fileInfo.size();
    const qint64 ETI_FRAME_SIZE = 6144;
    qint64 expectedFrames = fileSize / ETI_FRAME_SIZE;

    qCInfo(etiMain) << "File size:" << fileSize << "bytes, Expected frames:" << expectedFrames;

    // ============================================================================
    // FIX: Data accumulators for real parsing results
    // ============================================================================
    QMap<uint16_t, eti::Ensemble> discovered_ensembles;
    QMap<uint32_t, eti::DabService> discovered_services;
    QMap<uint8_t, eti::SubChannelInfo> discovered_subchannels;
    QMap<QString, int> fig_type_counts;

    // P1 (CLI FIG detail wave): per-frame FIG trace lines, FIG inventory
    // (key = (type << 8) | extension, deterministic ordering), FIC health
    // counters and the MCI reconfiguration detector.
    QStringList figTraceLines;
    QMap<quint32, cli::FIGInventoryEntry> figInventory;
    quint64 totalFibs = 0;
    quint64 fibCrcFailures = 0;
    quint64 erroneousFigs = 0;
    quint64 reconfigurationCount = 0;
    MciState accMciState;
    const bool traceEnabled = (m_verbose_mode || m_fig_trace_mode);

    // Backlog P2/P3: grouped-by-FIG aggregation (--figs-by-type) and the
    // per-FIB hex dump lines (--fib-hex).
    QMap<quint32, FigTypeGroup> figGroups;
    QStringList fibHexLines;


    // Process frames with real ETI parsing
    qint64 processedFrames = 0;
    qint64 errorFrames = 0;
    qint64 valid_parsed_frames = 0;

    QFile frameFile(inputFile);
    if (frameFile.open(QIODevice::ReadOnly)) {
        QByteArray frameData;
        frameData.resize(ETI_FRAME_SIZE);

        while (!frameFile.atEnd()) {
            qint64 bytesRead = frameFile.read(frameData.data(), ETI_FRAME_SIZE);

            if (bytesRead == ETI_FRAME_SIZE) {
                processedFrames++;

                // ============================================================================
                // FIX: ACTUAL FRAME PARSING (was missing before!)
                // ============================================================================
                try {
                    // Step 1: Parse the ETI frame structure
                    auto parseResult = m_frameParser->parseFrame(frameData);

                    if (parseResult.success) {
                        valid_parsed_frames++;

                        // Effective NST of this frame (raw STC count with the
                        // legacy nst_offset_1 correction applied; used by the
                        // EOH CRC, the FIB-hex dump and the FIC offset).
                        const quint64 frame_nst =
                            (parseResult.detected_format == eti::ETIFormat::ETI_NI)
                                ? static_cast<quint64>(parseResult.subchannels_from_stc.size())
                                : static_cast<quint64>(parseResult.frame.get_lidata_field().nst)
                                    + (m_settings.nst_offset_1 ? 1u : 0u);

                        // Row 11 (strict_frame_crc): count EOH gap CRC
                        // mismatches; the decode stays independent (never
                        // blocks parsing).
                        if (m_settings.strict_frame_crc) {
                            if (!validateEohCrc(reinterpret_cast<const uint8_t*>(frameData.constData()),
                                                frame_nst)) {
                                result.frame_crc_failures++;
                            }
                        }

                        // Step 2: Analyze FIG data from the frame
                        auto figResult = m_frameParser->analyzeFIGData(parseResult.frame, parseResult);

                        // ============================================================
                        // P1: per-frame FIG trace, FIG inventory + FIC health
                        // ============================================================
                        const bool frameHasFic = !figResult.fig_trace.empty();

                        // MCI reconfiguration detection: a frame "reconfigures" the multiplex when
                        // its decoded MCI state grows the ACCUMULATED
                        // configuration — a new ensemble id, new sub-channel
                        // ids, or new component-carrying service ids (FIG
                        // 0/2). The on-air FIC rotates FIG 0/1/0/2 table
                        // slices across frames (16 sub-channels cannot fit a
                        // single 3-FIB FIC), so comparing consecutive frames
                        // directly would flag every rotation as a change on a
                        // perfectly static mux; the accumulated comparison
                        // yields the true reconfigurations (initial lock-in +
                        // real config changes). Pure label rotation (FIG 1/1
                        // creates label-only service entries) is excluded, and
                        // FIG 0/0 (every 4th frame here) carries its ensemble
                        // id forward so its absence is not a change.
                        bool frameReconfigured = false;
                        if (frameHasFic) {
                            MciState state;
                            if (figResult.ensemble_info.validate_ensemble_id()) {
                                state.ensembleId = figResult.ensemble_info.ensemble_id;
                            } else if (accMciState.valid && accMciState.ensembleId != 0) {
                                state.ensembleId = accMciState.ensembleId;
                            }
                            for (const auto& service : figResult.discovered_services) {
                                if (!service.validate_service_id()) {
                                    continue;
                                }
                                // Only services with components define the MCI;
                                // FIG 1/1 label entries (no components) rotate
                                // and must not count as a reconfiguration.
                                if (service.components.empty()) {
                                    continue;
                                }
                                // F8/T35: use the full SID as the identity (data
                                // services: 32-bit `sid32`; programme: 16-bit id).
                                // Data-vs-programme is inferred from `sid32` (data
                                // services set it, programme leave it 0 — FIG 0/2
                                // P/D); keep in sync.
                                state.services.insert(service.sid32 > 0xFFFFu
                                                          ? service.sid32
                                                          : static_cast<uint32_t>(service.service_id));
                            }
                            // Sub-channels from FIG 0/1 plus the STC-carrying
                            // sub-channels of the frame.
                            for (const auto& subchannel : figResult.subchannels) {
                                if (subchannel.validate_sub_channel_id()) {
                                    state.subchannels.insert(subchannel.sub_channel_id);
                                }
                            }
                            for (const auto& subchannel : parseResult.subchannels_from_stc) {
                                if (subchannel.validate_sub_channel_id()) {
                                    state.subchannels.insert(subchannel.sub_channel_id);
                                }
                            }

                            // Accumulate: this frame reconfigures the mux iff
                            // it introduces elements unknown to the merged
                            // configuration seen so far.
                            if (!accMciState.valid) {
                                accMciState = state;
                                accMciState.valid = true;
                                frameReconfigured = !state.services.empty() ||
                                                    !state.subchannels.empty();
                            } else {
                                if (state.ensembleId != accMciState.ensembleId) {
                                    accMciState.ensembleId = state.ensembleId;
                                    frameReconfigured = true;
                                }
                                for (const uint32_t sid : state.services) {
                                    if (accMciState.services.insert(sid).second) {
                                        frameReconfigured = true;
                                    }
                                }
                                for (const uint16_t scid : state.subchannels) {
                                    if (accMciState.subchannels.insert(scid).second) {
                                        frameReconfigured = true;
                                    }
                                }
                            }
                            if (frameReconfigured) {
                                ++reconfigurationCount;
                            }
                        }

                        // Per-instance trace + inventory walk
                        QStringList frameServiceLabels;  // per-frame, FIG order
                        {
                            for (const auto& service : figResult.discovered_services) {
                                const QString label = QString::fromStdString(service.label);
                                if (!label.isEmpty()) {
                                    frameServiceLabels.append(label);
                                }
                            }

                            const QString ensembleLabel =
                                QString::fromStdString(figResult.ensemble_info.label);

                            // Consumed-counters for the backlog decoders (the
                            // per-block entry counts come from fig_decode_counts,
                            // parallel to fig_blocks; see FigDecodeCounts).
                            size_t labelsConsumed = 0;
                            size_t freqsConsumed = 0;
                            size_t geosConsumed = 0;
                            size_t compsConsumed = 0;
                            size_t fig2sConsumed = 0;

                            const bool collectByType = m_figs_by_type_mode;

                            for (size_t i = 0; i < figResult.fig_trace.size(); ++i) {
                                const auto& entry = figResult.fig_trace[i];
                                const eti::FigBlock& block = figResult.fig_blocks[i];
                                const eti::modern::FigDecodeCounts counts =
                                    (i < figResult.fig_decode_counts.size())
                                        ? figResult.fig_decode_counts[i]
                                        : eti::modern::FigDecodeCounts{};

                                // Inventory aggregation (always — feeds YAML)
                                {
                                    const quint32 key = (static_cast<quint32>(entry.fig_type) << 8) |
                                                        entry.extension;
                                    auto it = figInventory.find(key);
                                    if (it == figInventory.end()) {
                                        cli::FIGInventoryEntry invEntry;
                                        invEntry.fig_type = entry.fig_type;
                                        invEntry.extension = entry.extension;
                                        invEntry.count = 1;
                                        invEntry.first_frame = static_cast<quint64>(processedFrames);
                                        invEntry.last_frame = static_cast<quint64>(processedFrames);
                                        invEntry.crc_failures = entry.crc_ok ? 0 : 1;
                                        figInventory.insert(key, invEntry);
                                    } else {
                                        it->count++;
                                        it->last_frame = static_cast<quint64>(processedFrames);
                                        if (!entry.crc_ok) {
                                            it->crc_failures++;
                                        }
                                    }
                                }

                                // Backlog (P2/P3): grouped-by-FIG aggregation —
                                // the readable value is computed once and
                                // shared with the trace line.
                                QString readable;
                                if (traceEnabled || collectByType) {
                                    readable = figReadableSummary(block, ensembleLabel,
                                                                  frameServiceLabels,
                                                                  figResult, counts,
                                                                  labelsConsumed,
                                                                  freqsConsumed,
                                                                  geosConsumed,
                                                                  compsConsumed,
                                                                  fig2sConsumed);
                                }

                                if (collectByType) {
                                    const quint32 key = (static_cast<quint32>(entry.fig_type) << 8) |
                                                        entry.extension;
                                    auto& group = figGroups[key];
                                    group.count++;
                                    group.frames.append(QString::number(processedFrames));
                                    const QString value = readable.trimmed();
                                    if (!value.isEmpty() && !group.values.contains(value)) {
                                        group.values.append(value);
                                    }
                                }

                                // Trace line (diagnostics mode)
                                if (traceEnabled) {
                                    QString line = QString("fig-trace: frame=%1 type=%2 ext=%3 len=%4 fib=%5 crc=%6 reconfig=%7")
                                        .arg(processedFrames)
                                        .arg(entry.fig_type)
                                        .arg(entry.extension)
                                        .arg(entry.length)
                                        .arg(entry.fib_index)
                                        .arg(entry.crc_ok ? "ok" : "bad")
                                        .arg(frameReconfigured ? "true" : "false");
                                    line += readable;
                                    figTraceLines.append(line);
                                    std::cerr << line.toStdString() << std::endl;
                                }
                            }
                        }

                        // Backlog (P2/P3): --fib-hex per-FIB dumps. The raw
                        // FIC position uses the effective NST; FIB count comes
                        // from the strict walk (raw-fallback frames produce no
                        // structured FIB hex).
                        if (m_fib_hex_mode && frameHasFic && figResult.total_fibs > 0) {
                            const QStringList hexLines = fibHexDumpLines(
                                frameData, frame_nst,
                                static_cast<quint64>(processedFrames),
                                figResult.total_fibs);
                            for (const QString& line : hexLines) {
                                fibHexLines.append(line);
                                std::cerr << line.toStdString() << std::endl;
                            }
                        }

                        // FIC health counters (always aggregated)
                        totalFibs += figResult.total_fibs;
                        fibCrcFailures += figResult.fib_crc_failures;
                        erroneousFigs += figResult.erroneous_figs;
                        // ============================================================

                        // *** VERBOSE MODE: Collect per-frame data ***
                        if (m_verbose_mode) {
                            cli::FrameAnalysisData frameAnalysis;

                            // Extract ETI header (LIDATA field)
                            auto lidata = parseResult.frame.get_lidata_field();

                            frameAnalysis.frame_number = processedFrames - 1;
                            frameAnalysis.timestamp = QDateTime::currentDateTime().addMSecs((processedFrames - 1) * 24);

                            // ETI header fields
                            frameAnalysis.header.fct = lidata.fc;
                            frameAnalysis.header.ficf = lidata.ficf;
                            frameAnalysis.header.nst = lidata.nst;
                            frameAnalysis.header.mid = lidata.mid;
                            frameAnalysis.header.fp = lidata.fp;
                            frameAnalysis.header.fl = lidata.fl;
                            // TIST source (option-variant matrix row 10): RAW/NI
                            // frames carry the 32-bit timestamp at the frame tail
                            // (bytes 6140..6143); bytes 8-11 are only a legacy
                            // ETI-LI assumption and stay for the LI path.
                            if (parseResult.detected_format == eti::ETIFormat::ETI_NI) {
                                frameAnalysis.header.tist = parseResult.frame.get_tail_tist();
                                frameAnalysis.header.tist_ms = parseResult.frame.get_tail_tist_ms();
                            } else {
                                frameAnalysis.header.tist = lidata.tist;
                            }
                            // Row 10 (timestamp_source): mtime mode replaces
                            // the per-frame TIST ms with a file-mtime-derived
                            // value (24 ms per-frame stepping).
                            if (m_settings.timestamp_source == eti::TimestampSource::Mtime) {
                                const qint64 mtime_ms = fileInfo.lastModified().toMSecsSinceEpoch();
                                frameAnalysis.header.tist_ms =
                                    static_cast<double>(mtime_ms) + (processedFrames - 1) * 24.0;
                            }
                            frameAnalysis.header.format = (eti::detectETIFormat(
                                reinterpret_cast<const uint8_t*>(frameData.data()),
                                frameData.size()
                            ) == eti::ETIFormat::ETI_NI) ? "ETI-NI" : "ETI-LI";

                            // STC fields (from parseResult.subchannels_from_stc for ETI-NI)
                            for (const auto& subchannel : parseResult.subchannels_from_stc) {
                                cli::FrameAnalysisData::STCField stc;
                                stc.scid = subchannel.sub_channel_id;
                                stc.sad = subchannel.start_address;
                                stc.size_cu = subchannel.size;
                                // Protection info (simplified)
                                stc.protection_form = subchannel.uep_flag ? "UEP" : "EEP";
                                stc.protection_level = QString("%1").arg(subchannel.protection_level);
                                // Backlog (8): protection table name + table
                                // CU size (EEP-A 16/8/6/4, EEP-B 27/21/18/15,
                                // UEP per Table 6.2) via protection_tables.hpp.
                                stc.protection_table = QString::fromStdString(
                                    eti::protectionTableName(subchannel.protection_option,
                                                             subchannel.uep_flag,
                                                             subchannel.table_index));
                                stc.cu_size = subchannel.uep_flag
                                    ? eti::uepSizeCu(subchannel.table_index)
                                    : eti::eepSizeCu(subchannel.protection_option,
                                                     subchannel.protection_level);
                                // Bitrate: STL in CUs × 8 kbps per CU (size 24 -> 192 kbps).
                                // Consistent with the YAML subchannel output; NOT etisnoop's
                                // stl*8/3 print, which uses a different unit (F9 resolution).
                                stc.bitrate_kbps = subchannel.size * 8;
                                frameAnalysis.stc_fields.append(stc);
                            }

                            // FIC data (FIG blocks)
                            if (lidata.ficf) {
                                frameAnalysis.fic.fib_count = 3;  // Standard DAB has 3 FIBs
                                // Note: FIB-level CRC checking would require FIC parser enhancement
                                // For now, we just collect the FIG blocks
                                cli::FrameAnalysisData::FICData::FIB fib;
                                fib.fib_number = 0;  // Combined FIBs
                                fib.crc_ok = parseResult.validation.valid;
                                for (const auto& fb : figResult.fig_blocks) fib.fig_blocks.append(fb);
                                frameAnalysis.fic.fibs.append(fib);
                            }

                            // Frame validation
                            frameAnalysis.crc_ok = parseResult.validation.valid;
                            if (!parseResult.validation.valid) {
                                for (const auto& err : parseResult.validation.errors) {
                                    frameAnalysis.errors.append(QString::fromStdString(err));
                                }
                            }

                            // Store in result
                            result.frame_analyses.push_back(frameAnalysis);
                        }

                        // Step 3: Collect ensemble information
                        if (figResult.ensemble_info.validate_ensemble_id()) {
                            uint16_t eid = figResult.ensemble_info.ensemble_id;
                            if (!discovered_ensembles.contains(eid)) {
                                discovered_ensembles[eid] = figResult.ensemble_info;
                                qCInfo(etiMain) << "Discovered ensemble:" << QString("0x%1").arg(eid, 4, 16, QChar('0'))
                                               << "Label:" << QString::fromStdString(figResult.ensemble_info.label);
                            } else {
                                auto& stored = discovered_ensembles[eid];
                                // Update label if not set yet
                                if (stored.label.empty() && !figResult.ensemble_info.label.empty()) {
                                    stored.label = figResult.ensemble_info.label;
                                }
                                // Row 8: propagate the ensemble short label / mask
                                if (stored.short_label.empty() && !figResult.ensemble_info.short_label.empty()) {
                                    stored.short_label = figResult.ensemble_info.short_label;
                                    stored.character_flag = figResult.ensemble_info.character_flag;
                                }
                                // Propagate the Extended Country Code once known (FIG 0/9)
                                if (stored.extended_country_code == 0 && figResult.ensemble_info.extended_country_code != 0) {
                                    stored.extended_country_code = figResult.ensemble_info.extended_country_code;
                                }
                                // Alarm flag is transient: once set, keep it set
                                if (figResult.ensemble_info.alarm_flag) {
                                    stored.alarm_flag = true;
                                }
                            }
                        }

                        // Step 4: Collect service information
                        // F3: programme services encode CId only (16-bit PD);
                        // their ECC must be inherited from the ensemble ECC
                        // (FIG 0/9) when the service carries no ECC itself.
                        uint8_t service_fallback_ecc = figResult.ensemble_info.extended_country_code;
                        if (service_fallback_ecc == 0) {
                            for (const auto& ens : discovered_ensembles) {
                                if (ens.extended_country_code != 0) {
                                    service_fallback_ecc = ens.extended_country_code;
                                    break;
                                }
                            }
                        }

                        for (const auto& service : figResult.discovered_services) {
                            if (service.validate_service_id()) {
                                // F8/T35: key the aggregate on the service's FULL
                                // identity. Data services (PD=1) carry a 32-bit SId
                                // in `sid32`; keying on the truncated 16-bit SRef
                                // both dropped SRef==0 (0xF3200000) and could collide
                                // it with programme SId 0x0000. Data-vs-programme is
                                // inferred from `sid32` (data services set it,
                                // programme leave it 0 — FIG 0/2 P/D); keep in sync.
                                // NOTE: the map is a QMap<uint32_t, ...>, so the key
                                // also defines the emitted order — full 32-bit data
                                // SIDs (0xF3...) now sort AFTER the 16-bit programme
                                // SIDs instead of at the head. That order change is
                                // intentional (it matches ascending full SID; the GUI
                                // list is keyed the same way).
                                const uint32_t sid = (service.sid32 > 0xFFFFu)
                                                         ? service.sid32
                                                         : static_cast<uint32_t>(service.service_id);
                                if (!discovered_services.contains(sid)) {
                                    eti::DabService stored_service = service;
                                    if (stored_service.extended_country_code == 0 && service_fallback_ecc != 0) {
                                        stored_service.extended_country_code = service_fallback_ecc;
                                    }
                                    discovered_services[sid] = stored_service;
                                    qCInfo(etiMain) << "Discovered service:" << QString("0x%1").arg(sid, 8, 16, QChar('0'))
                                                   << "Label:" << QString::fromStdString(service.label);
                                } else {
                                    // Merge label, country info and components
                                    auto& stored = discovered_services[sid];
                                    if (stored.label.empty() && !service.label.empty()) {
                                        stored.label = service.label;
                                    }
                                    // Row 8: propagate the short label / mask
                                    // once known (never clear a known value).
                                    if (stored.short_label.empty() && !service.short_label.empty()) {
                                        stored.short_label = service.short_label;
                                        stored.character_flag = service.character_flag;
                                    }
                                    if (stored.extended_country_code == 0 && service.extended_country_code != 0) {
                                        stored.extended_country_code = service.extended_country_code;
                                    }
                                    // F3: fall back to the ensemble ECC while merging
                                    if (stored.extended_country_code == 0 && service_fallback_ecc != 0) {
                                        stored.extended_country_code = service_fallback_ecc;
                                    }
                                    // F8: keep the full 32-bit SId of data services
                                    if (stored.sid32 == 0 && service.sid32 != 0) {
                                        stored.sid32 = service.sid32;
                                    }
                                    for (const auto& comp : service.components) {
                                        bool found = false;
                                        for (const auto& existing_comp : stored.components) {
                                            if (existing_comp.sub_channel_id == comp.sub_channel_id) {
                                                found = true;
                                                break;
                                            }
                                        }
                                        if (!found) {
                                            stored.components.push_back(comp);
                                        }
                                    }
                                }
                            }
                        }

                        // Step 5: Collect subchannel information
                        // For ETI-NI: subchannels from STC fields (parseResult.subchannels_from_stc)
                        for (const auto& subchannel : parseResult.subchannels_from_stc) {
                            if (subchannel.validate_sub_channel_id()) {
                                uint8_t subch_id = subchannel.sub_channel_id;
                                if (!discovered_subchannels.contains(subch_id)) {
                                    discovered_subchannels[subch_id] = subchannel;
                                    qCInfo(etiMain) << "Discovered subchannel (STC):" << subch_id
                                                   << "Start:" << QString("0x%1").arg(subchannel.start_address, 4, 16, QChar('0'))
                                                   << "Size:" << subchannel.size << "CUs";
                                }
                            }
                        }
                        // For ETI-LI: subchannels from FIG 0/1 (figResult.subchannels)
                        for (const auto& subchannel : figResult.subchannels) {
                            if (subchannel.validate_sub_channel_id()) {
                                uint8_t subch_id = subchannel.sub_channel_id;
                                if (!discovered_subchannels.contains(subch_id)) {
                                    discovered_subchannels[subch_id] = subchannel;
                                    qCInfo(etiMain) << "Discovered subchannel (FIG):" << subch_id
                                                   << "Start:" << QString("0x%1").arg(subchannel.start_address, 4, 16, QChar('0'))
                                                   << "Size:" << subchannel.size << "CUs";
                                }
                            }
                        }


                        // Step 6: Count FIG types
                        for (const auto& fig_block : figResult.fig_blocks) {
                            QString fig_key = QString("FIG %1/%2")
                                .arg(fig_block.fig_type)
                                .arg(fig_block.get_extension());
                            fig_type_counts[fig_key]++;
                        }

                    } else {
                        // Frame parsing failed
                        errorFrames++;
                        qCWarning(etiMain) << "Frame" << processedFrames << "parsing failed";
                    }

                } catch (const std::exception& e) {
                    errorFrames++;
                    qCWarning(etiMain) << "Exception parsing frame" << processedFrames << ":" << e.what();
                }

                // P1: --max-frames limit — stop after N complete frames have
                // been processed (all statistics then cover the prefix).
                if (m_max_frames > 0 && processedFrames >= m_max_frames) {
                    break;
                }

                // Update progress
                double progress = static_cast<double>(processedFrames) / expectedFrames;
                m_progress = qMin(progress, 1.0);
                m_statusText = QString("Processing frame %1 of %2...").arg(processedFrames).arg(expectedFrames);
                emit progress_changed(m_progress);
                emit status_changed(m_statusText);

                // Small delay every 1000 frames for UI responsiveness
                if (processedFrames % 1000 == 0) {
                    QThread::msleep(1);
                    qCInfo(etiMain) << "Progress:" << processedFrames << "frames,"
                                   << discovered_services.size() << "services,"
                                   << discovered_subchannels.size() << "subchannels";
                    if (m_verbose_mode) {
                        qCInfo(etiMain) << "  Verbose: Collected" << result.frame_analyses.size() << "frame analyses";
                    }
                }

            } else if (bytesRead > 0) {
                // Partial frame - count as error
                errorFrames++;
                qCWarning(etiMain) << "Partial frame detected:" << bytesRead << "bytes instead of" << ETI_FRAME_SIZE;
            }
        }
        frameFile.close();
    }

    // Row 6 (ecc_override): manual ECC for country resolution when set.
    // Applied to the merged ensemble/services so the YAML country lookup and
    // every other consumer see the override.
    if (m_settings.ecc_override >= 0) {
        for (auto& ens : discovered_ensembles) {
            ens.extended_country_code = static_cast<uint8_t>(m_settings.ecc_override);
        }
        for (auto& service : discovered_services) {
            service.extended_country_code = static_cast<uint8_t>(m_settings.ecc_override);
        }
    }

    // Calculate processing statistics
    qint64 processingTime = m_processingTimer.elapsed();
    result.processingTimeMs = processingTime;

    // ============================================================================
    // FIX: Real processing results (not hardcoded!)
    // ============================================================================
    result.success = true;
    result.totalFrames = expectedFrames;
    result.processedFrames = processedFrames;
    result.errorFrames = errorFrames;
    result.averageFPS = processedFrames / (processingTime / 1000.0);

    // Populate ensemble information
    result.ensemblesFound = discovered_ensembles.size();
    if (!discovered_ensembles.isEmpty()) {
        // Use first ensemble as primary
        auto ensemble = discovered_ensembles.first();
        result.ensembleName = QString::fromStdString(ensemble.label);

        // AGENT 45 FIX: Store full ensemble structure in result
        result.ensemble = ensemble;

        // Emit ensemble discovered signal
        emit ensemble_discovered(ensemble.ensemble_id, result.ensembleName);
    }

    // Populate service information
    result.servicesFound = discovered_services.size();
    for (const auto& service : discovered_services) {
        // F8/T35: identify a service by its full SID (data services use the
        // 32-bit `sid32`; programme services use the 16-bit `service_id`).
        // Data-vs-programme is inferred from `sid32` (data services set it,
        // programme leave it 0 — FIG 0/2 P/D); keep in sync.
        const quint32 fullSid = (service.sid32 > 0xFFFFu)
                                    ? service.sid32
                                    : static_cast<quint32>(service.service_id);
        QString service_name = QString::fromStdString(service.label);
        if (service_name.isEmpty()) {
            service_name = QString("Service 0x%1")
                               .arg(fullSid, fullSid > 0xFFFFu ? 8 : 4, 16, QChar('0'));
        }
        result.serviceNames << service_name;

        // AGENT 45 FIX: Store full service structures in result
        result.services.push_back(service);

        // Determine service type
        QString service_type = "Unknown";
        if (service.is_programme) {
            // Check if DAB+ (ASCTy = 0x3F or >= 0x02)
            bool is_dabplus = false;
            for (const auto& comp : service.components) {
                if (comp.asc_ty == 0x3F || comp.asc_ty >= 0x02) {
                    is_dabplus = true;
                    break;
                }
            }
            service_type = is_dabplus ? "DAB+ Audio" : "DAB Audio";
        } else {
            service_type = "Data Service";
        }

        // Emit service discovered signal (full SID so data services surface
        // their 32-bit identity, e.g. 0xF3200000).
        emit service_discovered(fullSid, service_name, service_type);
    }
    // AGENT 45 FIX: Convert SubChannelInfo to Fig01SubchannelInfo for YAML output
    for (const auto& subchannel : discovered_subchannels) {
        eti::fig::Fig01SubchannelInfo fig01_sc{};
        fig01_sc.subchannel_id = subchannel.sub_channel_id;
        fig01_sc.start_address = subchannel.start_address;

        // Determine form based on UEP flag.
        // Convention: short form (3-byte entry) == UEP, long form (4-byte
        // entry) == EEP (EN 300 401 8.1.2.1).
        fig01_sc.short_form = subchannel.uep_flag;

        if (fig01_sc.short_form) {
            // Short form (UEP): the wire table_switch/table_index select the
            // UEP protection-table entry (EN 300 401 §6.2); the size is the
            // table value, not signalled in the stream (backlog item 8).
            fig01_sc.table_switch = subchannel.table_switch;
            fig01_sc.table_index = subchannel.table_index;
        } else {
            // Long form (EEP)
            fig01_sc.subchannel_size = subchannel.size;
            fig01_sc.protection_level = subchannel.protection_level;
            fig01_sc.option = subchannel.protection_option;  // 0 = EEP-x-A, 1 = EEP-x-B
        }
        fig01_sc.is_valid = true;
        result.subchannels.push_back(fig01_sc);
    }

    // AGENT 45 FIX: Populate FIG type counts
    result.fig_type_counts = fig_type_counts;

    // P1: FIG inventory (sorted by (type, ext) via QMap key order), FIC/FIB
    // health counters and the per-frame FIG trace lines.
    result.fig_inventory.reserve(static_cast<size_t>(figInventory.size()));
    for (const auto& entry : figInventory) {
        result.fig_inventory.push_back(entry);
    }
    result.fic_health.fib_crc_failures = fibCrcFailures;
    result.fic_health.total_fibs = totalFibs;
    result.fic_health.erroneous_figs = erroneousFigs;
    result.fic_health.reconfiguration_count = reconfigurationCount;
    result.fig_trace_lines = figTraceLines;

    // Backlog P2/P3: grouped-by-FIG lines (deterministic (type, ext) order
    // via QMap; frames capped at 64 for readability) and per-FIB hex lines.
    {
        constexpr int kFramesShown = 64;
        for (auto it = figGroups.constBegin(); it != figGroups.constEnd(); ++it) {
            const quint32 key = it.key();
            const FigTypeGroup& group = it.value();
            QStringList shown = group.frames.mid(0, kFramesShown);
            QString line = QString("figs-by-type: fig=%1/%2 count=%3 frames=\"%4\"")
                .arg(key >> 8).arg(key & 0xFF)
                .arg(group.count).arg(shown.join(","));
            if (group.frames.size() > kFramesShown) {
                line += QString(" (+%1 more)").arg(group.frames.size() - kFramesShown);
            }
            line += QString(" values=[%1]").arg(group.values.join("; "));
            result.figs_by_type_lines.append(line);
            std::cerr << line.toStdString() << std::endl;
        }
    }
    result.fib_hex_lines = fibHexLines;

    // ETSI compliance (T39). This headless path's own protocol summary of the
    // FIC/FIB/FIG decode (it is not a mirror of the GUI engine, whose input is
    // synthetic self-consistent data), rather than "zero transport error frames":
    //   * every walked FIB passed its CRC-16 (no fib_crc_failures),
    //   * no FIG header was skipped for a length/padding anomaly
    //     (no erroneous_figs),
    //   * a FIC was decoded — either the strict walk visited FIBs OR FIG blocks
    //     were recovered (Raw mode / auto-fallback walk no FIBs but still decode
    //     the CRC-less FIC dump; F6), and
    //   * the ensemble/service decode is intact (at least one ensemble and one
    //     service recovered),
    //   * when strict_frame_crc is enabled (row 11), no EOH gap CRC failures.
    // Transport error frames (mux-level) are a separate, legitimate condition
    // on an otherwise protocol-conformant multiplex; they stay reported via
    // errorFrames / statistics and the FIC health counters.
    const bool strictFrameCrcEnabled = m_settings.strict_frame_crc;
    const bool fibCrcClean = (fibCrcFailures == 0);
    const bool figsClean = (erroneousFigs == 0);
    const bool ficDecoded = (totalFibs > 0) || !figInventory.isEmpty();
    const bool decodeIntact =
        !discovered_ensembles.empty() && !discovered_services.empty();
    const bool frameCrcClean =
        !strictFrameCrcEnabled || (result.frame_crc_failures == 0);
    result.etsiCompliant =
        fibCrcClean && figsClean && ficDecoded && decodeIntact && frameCrcClean;

    result.complianceReason = QString(
        "FIB CRC failures=%1, erroneous FIGs=%2, FIBs walked=%3, "
        "FIG types decoded=%4, ensembles=%5, services=%6, "
        "transport error frames=%7 (separate), "
        "strict frame CRC failures=%8 (strict_frame_crc=%9)")
        .arg(fibCrcFailures)
        .arg(erroneousFigs)
        .arg(totalFibs)
        .arg(figInventory.size())
        .arg(discovered_ensembles.size())
        .arg(discovered_services.size())
        .arg(errorFrames)
        .arg(result.frame_crc_failures)
        .arg(strictFrameCrcEnabled ? QStringLiteral("on") : QStringLiteral("off"));

    // A NO must be explainable: name the failed protocol checks in the warnings.
    if (!result.etsiCompliant) {
        if (fibCrcFailures > 0) {
            result.complianceWarnings << QString("%1 FIB CRC failures").arg(fibCrcFailures);
        }
        if (erroneousFigs > 0) {
            result.complianceWarnings << QString("%1 erroneous FIGs").arg(erroneousFigs);
        }
        if (!ficDecoded) {
            result.complianceWarnings << "No FIC decoded (no FIBs walked, no FIG blocks)";
        }
        if (!decodeIntact) {
            result.complianceWarnings << "Ensemble/service decode incomplete";
        }
        if (!frameCrcClean) {
            result.complianceWarnings
                << QString("%1 strict frame CRC failures").arg(result.frame_crc_failures);
        }
    }
    if (discovered_services.isEmpty()) {
        result.complianceWarnings << "No services discovered (unusual for valid ETI)";
    }
    if (discovered_subchannels.isEmpty()) {
        result.complianceWarnings << "No subchannels discovered (unusual for valid ETI)";
    }

    // Final progress update
    m_progress = 1.0;
    m_statusText = "Processing complete";
    emit progress_changed(m_progress);
    emit status_changed(m_statusText);

    // Log final summary
    qCInfo(etiMain) << "Processing complete:";
    qCInfo(etiMain) << "  Frames processed:" << processedFrames << "/" << expectedFrames;
    qCInfo(etiMain) << "  Valid frames:" << valid_parsed_frames;
    qCInfo(etiMain) << "  Error frames:" << errorFrames;
    qCInfo(etiMain) << "  Ensembles discovered:" << discovered_ensembles.size();
    qCInfo(etiMain) << "  Services discovered:" << discovered_services.size();
    qCInfo(etiMain) << "  Subchannels discovered:" << discovered_subchannels.size();
    qCInfo(etiMain) << "  FIG types seen:" << fig_type_counts.size();
    qCInfo(etiMain) << "  Processing time:" << processingTime << "ms";
    qCInfo(etiMain) << "  Average FPS:" << QString::number(result.averageFPS, 'f', 1);
    if (m_verbose_mode) {
        qCInfo(etiMain) << "  Verbose frames collected:" << result.frame_analyses.size();
    }

    // Save results to output file
    if (!save_results_to_file(outputFile, result)) {
        result.success = false;
        result.errorMessage = QString("Failed to save results to: %1").arg(outputFile);
        return result;
    }

    // Emit completion signal
    emit processing_complete(result);

    return result;
}

bool HeadlessETIProcessor::initialize_components()
{
    try {
        // Create core processing components
        m_etiProcessor = std::make_unique<EtiProcessor>();
        if (!m_etiProcessor->initialize()) {
            Logger::instance().log(Logger::Error, "HeadlessETIProcessor", "Failed to initialize EtiProcessor");
            return false;
        }

        // Initialize modern frame parser with default config
        m_frameParser = std::make_unique<eti::modern::ModernETIFrameParser>();
        eti::modern::ProcessingConfig config;
        config.enable_threading = false;  // Single-threaded for CLI mode
        config.enable_caching = false;    // Disable caching for simplicity
        config.target_fps = 1000.0;       // Target 1000 FPS

        if (!m_frameParser->initialize(config)) {
            Logger::instance().log(Logger::Error, "HeadlessETIProcessor", "Failed to initialize ModernETIFrameParser");
            return false;
        }

        // Thread the user-selected analyser settings (rows 1-14) through to
        // the decode path; defaults() keep a plain run byte-identical.
        m_frameParser->setAnalyserSettings(m_settings);

        m_figAnalyser = std::make_unique<eti::modern::EnhancedFIGAnalyser>();
        if (!m_figAnalyser->initialize()) {
            Logger::instance().log(Logger::Error, "HeadlessETIProcessor", "Failed to initialize EnhancedFIGAnalyser");
            return false;
        }

        // DISABLED FOR CLI BUILD: ComprehensiveETSIValidator all methods commented out
        // m_etsiValidator = std::make_unique<eti::compliance::ComprehensiveETSIValidator>();
        // if (!m_etsiValidator->initialize()) {
        //     Logger::instance().log(Logger::Error, "HeadlessETIProcessor", "Failed to initialize ComprehensiveETSIValidator");
        //     return false;
        // }

        Logger::instance().log(Logger::Info, "HeadlessETIProcessor", "All processing components initialized successfully");
        return true;

    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "HeadlessETIProcessor",
                               QString("Component initialization failed: %1").arg(e.what()));
        return false;
    }
}

void HeadlessETIProcessor::reset_processing_state()
{
    m_progress = 0.0;
    m_statusText = "";
    m_currentResult = ProcessingResult();
}

bool HeadlessETIProcessor::save_results_to_file(const QString& outputFile, const ProcessingResult& result)
{
    try {
        QFile file(outputFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            Logger::instance().log(Logger::Error, "HeadlessETIProcessor",
                                   QString("Cannot open output file for writing: %1").arg(outputFile));
            return false;
        }

        QString yamlOutput = generate_yaml_output(result);
        file.write(yamlOutput.toUtf8());
        file.close();

        Logger::instance().log(Logger::Info, "HeadlessETIProcessor",
                              QString("Results saved to: %1").arg(outputFile));
        return true;

    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "HeadlessETIProcessor",
                               QString("Failed to save results: %1").arg(e.what()));
        return false;
    }
}

QString HeadlessETIProcessor::generate_yaml_output(const ProcessingResult& result)
{
    QString yaml;
    yaml += "# ETI Stream Analysis Results\n";
    yaml += "# Generated by StreamDAB Stream Analyser Headless Mode\n\n";

    yaml += "processing:\n";
    yaml += QString("  success: %1\n").arg(result.success ? "true" : "false");
    if (!result.errorMessage.isEmpty()) {
        yaml += QString("  errorMessage: \"%1\"\n").arg(result.errorMessage);
    }
    yaml += "\n";

    yaml += "statistics:\n";
    yaml += QString("  totalFrames: %1\n").arg(result.totalFrames);
    yaml += QString("  processedFrames: %1\n").arg(result.processedFrames);
    yaml += QString("  errorFrames: %1\n").arg(result.errorFrames);
    yaml += QString("  processingTimeMs: %1\n").arg(result.processingTimeMs, 0, 'f', 2);
    yaml += QString("  averageFPS: %1\n").arg(result.averageFPS, 0, 'f', 2);
    yaml += "\n";

    yaml += "ensemble:\n";
    yaml += QString("  ensemblesFound: %1\n").arg(result.ensemblesFound);
    if (!result.ensembleName.isEmpty()) {
        yaml += QString("  ensembleName: \"%1\"\n").arg(result.ensembleName);
    }
    yaml += "\n";

    yaml += "services:\n";
    yaml += QString("  servicesFound: %1\n").arg(result.servicesFound);
    if (!result.serviceNames.isEmpty()) {
        yaml += "  serviceNames:\n";
        for (const QString& name : result.serviceNames) {
            yaml += QString("    - \"%1\"\n").arg(name);
        }
    } else {
        yaml += "  serviceNames: []\n";
    }
    yaml += "\n";

    yaml += "etsiCompliance:\n";
    yaml += QString("  etsiCompliant: %1\n").arg(result.etsiCompliant ? "true" : "false");
    if (!result.complianceReason.isEmpty()) {
        // F12: escape embedded quotes/backslashes like the YAML writers.
        yaml += QString("  reason: \"%1\"\n").arg(escapeTraceValue(result.complianceReason));
    }
    if (!result.complianceWarnings.isEmpty()) {
        yaml += "  warnings:\n";
        for (const QString& warning : result.complianceWarnings) {
            yaml += QString("    - \"%1\"\n").arg(warning);
        }
    }
    if (!result.complianceErrors.isEmpty()) {
        yaml += "  errors:\n";
        for (const QString& error : result.complianceErrors) {
            yaml += QString("    - \"%1\"\n").arg(error);
        }
    }

    return yaml;
}

QString HeadlessETIProcessor::generate_json_output(const ProcessingResult& result)
{
    QJsonObject json;

    QJsonObject processing;
    processing["success"] = result.success;
    if (!result.errorMessage.isEmpty()) {
        processing["errorMessage"] = result.errorMessage;
    }
    json["processing"] = processing;

    QJsonObject statistics;
    statistics["totalFrames"] = static_cast<qint64>(result.totalFrames);
    statistics["processedFrames"] = static_cast<qint64>(result.processedFrames);
    statistics["errorFrames"] = static_cast<qint64>(result.errorFrames);
    statistics["processingTimeMs"] = result.processingTimeMs;
    statistics["averageFPS"] = result.averageFPS;
    json["statistics"] = statistics;

    QJsonObject services;
    services["servicesFound"] = static_cast<qint64>(result.servicesFound);
    services["ensemblesFound"] = static_cast<qint64>(result.ensemblesFound);
    if (!result.ensembleName.isEmpty()) {
        services["ensembleName"] = result.ensembleName;
    }
    json["services"] = services;

    QJsonObject compliance;
    compliance["etsiCompliant"] = result.etsiCompliant;
    if (!result.complianceReason.isEmpty()) {
        compliance["reason"] = result.complianceReason;
    }
    json["etsiCompliance"] = compliance;

    QJsonDocument doc(json);
    return doc.toJson();
}

void HeadlessETIProcessor::on_frame_processed()
{
    // Callback for frame processing events
}

void HeadlessETIProcessor::on_error_detected(const QString& error)
{
    Logger::instance().log(Logger::Warning, "HeadlessETIProcessor",
                           QString("Processing error detected: %1").arg(error));
}
