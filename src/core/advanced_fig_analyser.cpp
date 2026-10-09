/**
 * @file advanced_fig_analyser.cpp
 * @brief FIG (Fast Information Group) Processor
 *
 * Implements decoding of FIG data from the FIC per ETSI EN 300 401.
 * Processes service information, sub-channel organization, and labels.
 *
 * Supported FIG Types:
 * - FIG 0/0: Ensemble information
 * - FIG 0/1: Sub-channel organization (EEP/UEP)
 * - FIG 0/2: Service organization
 * - FIG 1/0: Ensemble label
 * - FIG 1/1: Service label (programme and data services)
 * - FIG 1/4: Service component label
 *
 * @see ETSI EN 300 401 Section 5 (Multiplex Configuration Information)
 * @see ETSI EN 300 401 Section 6 (FIG Type 0)
 * @see ETSI EN 300 401 Section 8 (FIG Type 1)
 */

#include "advanced_fig_analyser.h"
#include "charset_converter.hpp"
#include "analyser_settings.hpp"
#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QTimeZone>
#include <algorithm>
#include "crc16.hpp"

namespace {
// FIG extension field extraction per EN 300 401 (same masks the analyser's
// dispatch uses): FIG 0 = 5 bits (b4-b0), FIG 1/2 = 3 bits (b2-b0).
uint8_t figExtensionOf(uint8_t figType, uint8_t data0)
{
    switch (figType) {
        case 0:  return data0 & 0x1F;
        case 1:  return data0 & 0x07;
        case 2:  return data0 & 0x07;
        default: return 0;
    }
}

// Maximum legal FIG data length (EN 300 401; eti_types.hpp MAX_FIG_LENGTH).
// Longer FIGs are out-of-spec and are skipped by the CLI parse_fig_area.
constexpr size_t kMaxFigDataLength = 29;

// T20: a live stream re-parses the rotating FIC continuously, so per-FIG
// diagnostics would flood the console (hundreds of lines/second). Keep the
// first 30 occurrences (startup diagnostics) and every 250th per call site.
//
// Thread-safety (F9): the `static` counters at each call site are local and
// unsynchronised. Safe only because AdvancedFIGAnalyser is driven from a
// single (GUI) thread; if that ever changes, promote them to
// std::atomic<uint64_t> members.
inline bool shouldLogFigDiagnostic(unsigned long long counter)
{
    return counter <= 30 || (counter % 250) == 0;
}
}  // namespace

// ============================================================================
// DABLabel implementation
// ============================================================================

QString DABLabel::getFullLabel() const {
    // Charset-aware label decode (EN 300 401 clause 8.1.13 / TS 101 756):
    //   0 = Complete EBU Latin, 3 = UTF-8 passthrough, 6 = TIS-620 (Thai).
    // 0xFF = label skipped by the settings fallback policy (Skip).
    // NUL padding bytes are skipped; the 16-character size is preserved and
    // callers trim trailing whitespace as needed (matches the CLI path in
    // ModernETIFrameParser::decodeLabelCharacters).
    if (charset == 0xFF) {
        return QString();
    }

    std::string raw;
    raw.reserve(16);
    for (char c : full_label) {
        if (c != '\0') {
            raw += c;
        }
    }

    switch (charset) {
        case 0: return QString::fromStdString(convert_ebu_to_utf8(raw));
        case 3: return QString::fromUtf8(raw.c_str(), static_cast<int>(raw.size()));
        case 6: return QString::fromStdString(convert_tis620_to_utf8(raw));
        default:
            // Unknown charset: raw passthrough (settings fallback policy was
            // already applied at parse time by effectiveLabelCharset()).
            return QString::fromLatin1(raw.c_str(), static_cast<int>(raw.size()));
    }
}

/**
 * Extract short label using character flag per ETSI EN 300 401 Section 8.1.13
 *
 * Character Flag Bit Mapping (MSB-first, per EN 300 401 8.1.14 / etisnoop
 * label_t::shortlabel() / ModernETIFrameParser::decodeShortLabel — the CLI
 * oracle):
 * - Bit 15 (MSB) = character position 0 (leftmost character)
 * - Bit 14 = character position 1
 * - ...
 * - Bit 1 = character position 14
 * - Bit 0 (LSB) = character position 15 (rightmost character)
 *
 * When a bit is SET (1), the corresponding character is included in the
 * short label (charset-converted after selection, CLI parity). The short
 * label is limited to 8 characters and trailing whitespace is trimmed.
 *
 * Examples:
 *   Full label: "BBC Radio 1     " (16 characters)
 *   Char flag:  0xE000 = 0b1110000000000000
 *   Bits set:   15, 14, 13
 *   Positions:  0, 1, 2
 *   Result: "BBC" ✓
 *
 * @return Short label as QString, trimmed of whitespace, maximum 8 characters
 */
QString DABLabel::getShortLabel() const {
    // 0xFF = label skipped by the settings fallback policy (Skip)
    if (charset == 0xFF) {
        return QString();
    }

    QString short_label;

    // Extract short label using character flag
    // ETSI EN 300 401: Bit 15 (MSB) = position 0, Bit 0 (LSB) = position 15.
    // Select the flagged bytes first, then charset-convert the selection
    // (CLI parity: ModernETIFrameParser::decodeShortLabel).
    std::string selected;
    for (int pos = 0; pos < 16; ++pos) {
        if (character_flag & (0x8000 >> pos)) {
            selected += full_label[pos];
            if (selected.size() >= 8) break;
        }
    }

    switch (charset) {
        case 0: short_label = QString::fromStdString(convert_ebu_to_utf8(selected)); break;
        case 3: short_label = QString::fromUtf8(selected.c_str(), static_cast<int>(selected.size())); break;
        case 6: short_label = QString::fromStdString(convert_tis620_to_utf8(selected)); break;
        default: short_label = QString::fromLatin1(selected.c_str(), static_cast<int>(selected.size())); break;
    }

    return short_label.trimmed();
}

bool DABLabel::isValid() const {
    // Check if label contains at least one non-space character
    for (char c : full_label) {
        if (c != ' ' && c != '\0') {
            return true;
        }
    }
    return false;
}

QString DABLabel::toString() const {
    return QString("Label: \"%1\" | Short: \"%2\" | CharFlag: 0x%3")
        .arg(getFullLabel())
        .arg(getShortLabel())
        .arg(character_flag, 4, 16, QChar('0'));
}

// ============================================================================
// AdvancedFIGAnalyser implementation
// ============================================================================

 AdvancedFIGAnalyser::AdvancedFIGAnalyser(QObject *parent)
    : QObject(parent)
    , m_totalFIGsProcessed(0)
    , m_validFIGsCount(0)
    , m_ensembleValid(false)
    , m_ensembleId(0)
    , m_changeFlags(0)
    , m_timeValid(false)
    , m_date_time_valid(false)
{
    initializeDefaultFIGTypes();
}

AdvancedFIGAnalyser::~AdvancedFIGAnalyser() = default;

void AdvancedFIGAnalyser::setAnalyserSettings(const eti::AnalyserSettings& settings)
{
    m_settings = std::make_unique<eti::AnalyserSettings>(settings);
}

QVector<AdvancedFIGAnalyser::FigTypeCount> AdvancedFIGAnalyser::getFigTypeCounts() const
{
    QVector<FigTypeCount> counts;
    counts.reserve(static_cast<int>(m_figTypeCounts.size()));
    for (const auto& [key, count] : m_figTypeCounts) {
        FigTypeCount entry;
        entry.type = static_cast<uint8_t>((key >> 8) & 0xFF);
        entry.ext = static_cast<uint8_t>(key & 0xFF);
        entry.count = count;
        counts.append(entry);
    }
    return counts;
}

void AdvancedFIGAnalyser::initializeDefaultFIGTypes()
{
    // Enable essential FIG types by default for DAB Analyser compatibility
    m_enabledFIGTypes["0.0"] = true;   // Ensemble organization
    m_enabledFIGTypes["0.1"] = true;   // Subchannel organization
    m_enabledFIGTypes["0.2"] = true;   // Service organization
    m_enabledFIGTypes["0.3"] = false;  // Service component in packet mode (optional)
    m_enabledFIGTypes["0.5"] = false;  // Service component language (optional)
    m_enabledFIGTypes["0.6"] = false;  // Service linking (optional)
    m_enabledFIGTypes["0.7"] = false;  // Configuration information (optional)
    m_enabledFIGTypes["0.8"] = false;  // Service component global (optional)
    m_enabledFIGTypes["0.9"] = true;   // Country, LTO, international table
    m_enabledFIGTypes["0.10"] = true;  // Date and time
    m_enabledFIGTypes["0.13"] = false; // User application (optional)
    m_enabledFIGTypes["0.14"] = false; // FEC sub-channel organization (optional)
    m_enabledFIGTypes["0.17"] = true;  // Programme type (recommended)
    m_enabledFIGTypes["0.18"] = true;  // Announcement support (EWS CRITICAL)
    m_enabledFIGTypes["0.19"] = true;  // Announcement switching (EWS CRITICAL)
    m_enabledFIGTypes["1.0"] = true;   // Ensemble labels
    m_enabledFIGTypes["1.1"] = true;   // Service labels
    m_enabledFIGTypes["1.4"] = true;   // Service component labels
    m_enabledFIGTypes["1.5"] = false;  // Data service labels (optional)
    m_enabledFIGTypes["2.0"] = false;  // Ensemble extended labels (optional)
    m_enabledFIGTypes["2.1"] = false;  // Service extended labels (optional)
    m_enabledFIGTypes["2.4"] = false;  // Component extended labels (optional)
    m_enabledFIGTypes["2.5"] = false;  // Data service extended labels (optional)
}

void AdvancedFIGAnalyser::analyzeFICData(const QByteArray& ficData)
{
    // CRITICAL-006: Input validation. The FIC is 96 bytes (3 FIBs, Modes
    // I/II/IV) or 128 bytes (4 FIBs, Mode III) in ETI, but a raw on-air FIC
    // (768 bytes, 24 FIBs) is also legal, so accept up to 768 bytes.
    if (ficData.isEmpty()) {
        m_lastError = "FIC data is empty";
        return;
    }

    if (ficData.size() > 768) {
        m_lastError = QString("FIC data too large: %1 bytes (max 768)").arg(ficData.size());
        return;
    }

    m_currentFICData = ficData;
    m_lastFicSize = ficData.size();

    // Effective decode settings: the analyser settings when wired, otherwise
    // the historical fixed behavior (strict FIB CRC validation, no raw
    // fallback, no forced charset).
    eti::FicDecodeMode mode = eti::FicDecodeMode::Strict;
    bool ignore_fib_crc = false;
    if (m_settings) {
        mode = eti::AnalyserSettings::toFicDecodeMode(m_settings->fic_mode);
        ignore_fib_crc = m_settings->fib_ignore_crc;
    }

    // FIB-structured FIC: 96/128 bytes = 3/4 FIBs of 32 bytes each (30-byte
    // FIG area + 2-byte big-endian complemented CRC-16/CCITT-FALSE). Smaller
    // inputs are raw FIG streams (legacy callers/tests) and are walked as one
    // area: a 32-byte legacy FIB keeps its 30-byte FIG area semantics.
    const bool fib_structured =
        static_cast<size_t>(ficData.size()) > eti::ETI_FIC_FIB_SIZE &&
        (static_cast<size_t>(ficData.size()) / eti::ETI_FIC_FIB_SIZE) >= 3;

    // Explicit raw mode: walk the whole FIC as one FIG stream (no per-FIB CRC).
    if (mode == eti::FicDecodeMode::Raw) {
        m_figFibIndex = -1;
        m_figFibCrcOk = true;
        walkFigArea(reinterpret_cast<const uint8_t*>(ficData.constData()),
                    static_cast<size_t>(ficData.size()));
        return;
    }

    const size_t fib_count = fib_structured
        ? static_cast<size_t>(ficData.size()) / eti::ETI_FIC_FIB_SIZE
        : 1;
    int fib_crc_failures = 0;

    for (size_t fib = 0; fib < fib_count; ++fib) {
        const size_t fib_base = fib * eti::ETI_FIC_FIB_SIZE;
        if (fib_structured) {
            // FIB walk: each 32-byte FIB must fit in the input
            if (fib_base + eti::ETI_FIC_FIB_SIZE > static_cast<size_t>(ficData.size())) {
                break;
            }
        } else if (fib_base >= static_cast<size_t>(ficData.size())) {
            break;  // raw stream: area starts at the first byte
        }
        const uint8_t* fib_data =
            reinterpret_cast<const uint8_t*>(ficData.constData()) + fib_base;

        // Per-FIB context for the FIG instance collector (FIC-XTractor).
        m_figFibIndex = static_cast<int>(fib);
        m_figFibCrcOk = true;

        if (fib_structured && !validateFibCrc(fib_data)) {
            ++fib_crc_failures;
            m_figFibCrcOk = false;
            if (!ignore_fib_crc) {
                qWarning() << "FIB" << fib << "CRC mismatch, skipping (strict mode)";
                continue;
            }
        }

        // A non-structured input is a raw FIG stream (legacy callers/tests);
        // a legacy 32-byte FIB keeps its 30-byte FIG-area semantics (the last
        // two bytes are the CRC, same layout as the CLI path).
        const size_t area_size = (fib_structured || static_cast<size_t>(ficData.size()) == eti::ETI_FIC_FIB_SIZE)
            ? eti::ETI_FIC_FIG_AREA_SIZE
            : static_cast<size_t>(ficData.size());
        walkFigArea(fib_data, area_size);
    }
    m_fib_crc_failures += fib_crc_failures;

    // Auto fallback (CLI parity, EtiFicField::decodeFigBlocks): when every
    // FIB of a FIB-structured FIC failed its CRC the data is most likely a
    // CRC-less FIG stream; re-walk the whole FIC so FIGs are not lost.
    if (mode == eti::FicDecodeMode::AutoFallback &&
        fib_structured &&
        fib_crc_failures == static_cast<int>(fib_count)) {
        m_rawFicFallbackUsed = true;
        if (!m_rawFicFallbackWarned) {
            m_rawFicFallbackWarned = true;
            qWarning() << "FIC decode fallback: all FIB CRCs failed -> re-decoded"
                          " FIC as a raw FIG stream (no per-FIB CRC). Set"
                          " fic_mode=strict (AnalyserSettings) to disable.";
        }
        // The fallback re-walk would double-record every FIG (once per
        // failed FIB, once raw); suppress instance collection for it.
        m_figRecording = false;
        walkFigArea(reinterpret_cast<const uint8_t*>(ficData.constData()),
                    static_cast<size_t>(ficData.size()));
        m_figRecording = true;
    }
}

bool AdvancedFIGAnalyser::validateFibCrc(const uint8_t* fib) const
{
    // FIB CRC: CRC-16/CCITT-FALSE over the 30-byte FIG area; the transmitted
    // value is the one's complement, big-endian (EN 300 799 clause 5.2 /
    // EN 300 401 clause 5.2 — shared crc16.hpp implementation).
    const uint16_t expected = static_cast<uint16_t>(
        ~eti::crc16ccitt_false(fib, eti::ETI_FIC_FIG_AREA_SIZE));
    const uint16_t stored = static_cast<uint16_t>((fib[30] << 8) | fib[31]);
    return expected == stored;
}

void AdvancedFIGAnalyser::walkFigArea(const uint8_t* area, size_t area_size)
{
    // Walk one FIG area (30-byte FIB area or whole-FIC raw stream) with the
    // correct EN 300 401 FIG header semantics (mirrors parse_fig_area in
    // eti_types.hpp, the CLI parity reference):
    //   header byte b0: FIG type = b7-b5, length = b4-b0 (data AFTER header)
    //   figData = bytes [1 .. 1+len-1] (starts at the ext/charset byte)
    //   advance by 1 + len; stop on len==0, type 7 (padding) or area end.
    size_t offset = 0;

    while (offset + 1 <= area_size) {
        const uint8_t header = area[offset];
        const uint8_t figType = (header >> 5) & 0x07;

        if (figType == 7) {
            // Padding end marker
            break;
        }

        const uint8_t figLength = header & 0x1F;

        if (figLength == 0 || offset + 1 + figLength > area_size) {
            // Invalid/truncated FIG — stop processing this area (CLI parity)
            if (figLength == 0) {
                m_lastError = QString("FIG at offset %1 has zero length").arg(offset);
                qWarning() << m_lastError;
            } else {
                m_lastError = QString("FIG length %1 exceeds data at offset %2")
                                  .arg(figLength).arg(offset);
            }
            break;
        }

        // Out-of-spec FIG lengths (30/31, > MAX_FIG_LENGTH): mirror the CLI
        // parse_fig_area semantics (eti_types.hpp) — count the anomaly, SKIP
        // the block and keep advancing so the walk (incl. the raw-fallback
        // re-walk and the FIG instance collector) stays consistent with the
        // CLI's fig_blocks stream.
        if (figLength > kMaxFigDataLength) {
            m_lastError = QString("FIG at offset %1 has out-of-spec length %2 (> 29); skipped")
                              .arg(offset).arg(figLength);
            qWarning() << m_lastError;
            offset += 1 + figLength;
            continue;
        }

        QByteArray figData(reinterpret_cast<const char*>(area + offset + 1),
                           figLength);

        // FIC-XTractor: record every valid-headered FIG block (chronological
        // wire order). Purely additive — decode behaviour below is unchanged.
        if (m_figRecording && m_figInstances.size() < MAX_FIG_INSTANCES) {
            const uint8_t data0 = static_cast<uint8_t>(figData[0]);
            FigInstance inst;
            inst.frame = m_figFrameNumber;
            inst.type = figType;
            inst.ext = figExtensionOf(figType, data0);
            inst.length = figLength;
            inst.fibIdx = m_figFibIndex;
            inst.crcOk = m_figFibCrcOk;
            // FIG 0/1 + 0/2 carry the MCI reconfiguration C/N flag in bit 7
            inst.reconfig = (figType == 0 && (inst.ext == 1 || inst.ext == 2) &&
                             (data0 & 0x80) != 0);
            inst.raw = figData;
            m_figInstances.append(std::move(inst));
            ++m_figTypeCounts[(static_cast<uint16_t>(figType) << 8) | inst.ext];
        }

        // Process based on FIG type
        switch (figType) {
            case 0:
                processFIG0(figData);
                break;
            case 1:
                processFIG1(figData);
                break;
            case 2:
                processFIG2(figData);
                break;
            default:
                qDebug() << "Unsupported FIG type:" << figType;
                m_lastError = QString("Unsupported FIG type: %1").arg(figType);
                // Note: Don't increment m_totalFIGsProcessed for unsupported types
                break;
        }

        offset += 1 + figLength;
    }
}

void AdvancedFIGAnalyser::processFIG0(const QByteArray& figData)
{
    if (figData.size() < 1) {
        m_lastError = "FIG 0: Insufficient data";
        return; // Don't count insufficient data as processed
    }

    // FIG 0 extension byte: C/N (b7) + O/E (b6) + rfu (b5) + extension (b4-b0)
    // per EN 300 401 clause 6.1.1 (etisnoop fig0_common_t::ext()).
    uint8_t extension = static_cast<uint8_t>(figData[0]) & 0x1F;

    QString figKey = QString("0.%1").arg(extension);
    if (!m_enabledFIGTypes.value(figKey, false)) {
        m_lastError = QString("FIG 0/%1: Disabled").arg(extension);
        return; // Don't count disabled FIGs as processed
    }

    // Process based on extension
    switch (extension) {
        case 0:
            processFIG0_0(figData);
            break;
        case 1:
            processFIG0_1(figData);
            break;
        case 2:
            processFIG0_2(figData);
            break;
        case 3:
            processFIG0_3(figData);
            break;
        case 5:
            processFIG0_5(figData);
            break;
        case 6:
            processFIG0_6(figData);
            break;
        case 7:
            processFIG0_7(figData);
            break;
        case 8:
            processFIG0_8(figData);
            break;
        case 9:
            processFIG0_9(figData);
            break;
        case 10:
            processFIG0_10(figData);
            break;
        case 13:
            processFIG0_13(figData);
            break;
        case 14:
            processFIG0_14(figData);
            break;
        case 17:
            processFIG0_17(figData);
            break;
        case 18:
            processFIG0_18(figData);
            break;
        case 19:
            processFIG0_19(figData);
            break;
        default:
            qDebug() << "Unsupported FIG 0 extension:" << extension;
            m_lastError = QString("FIG 0/%1: Unsupported extension").arg(extension);
            // Don't count unsupported extensions as processed
            break;
    }
}

void AdvancedFIGAnalyser::processFIG1(const QByteArray& figData)
{
    if (figData.size() < 1) {
        m_lastError = "FIG 1: Insufficient data";
        return; // Don't count insufficient data as processed
    }

    // FIG 1 label extension byte: charset (b7-b4) + O/E (b3) + extension
    // (b2-b0) per EN 300 401 clause 8.1.13 (etisnoop fig1.cpp: ext = f[0]&0x07;
    // the modern CLI parser uses the same mask to avoid folding the charset's
    // low bit into the extension).
    uint8_t extension = static_cast<uint8_t>(figData[0]) & 0x07;

    QString figKey = QString("1.%1").arg(extension);
    if (!m_enabledFIGTypes.value(figKey, false)) {
        m_lastError = QString("FIG 1/%1: Disabled").arg(extension);
        return; // Don't count disabled FIGs as processed
    }

    // Process based on extension
    switch (extension) {
        case 0:
            processFIG1_0(figData);
            break;
        case 1:
            processFIG1_1(figData);
            break;
        case 4:
            processFIG1_4(figData);
            break;
        case 5:
            processFIG1_5(figData);
            break;
        default:
            qDebug() << "Unsupported FIG 1 extension:" << extension;
            m_lastError = QString("FIG 1/%1: Unsupported extension").arg(extension);
            // Don't count unsupported extensions as processed
            break;
    }
}

/**
 * Process FIG Type 0 Extension 0: Ensemble Information
 * ETSI EN 300 401 Section 6.4.1
 *
 * Provides basic ensemble identification including:
 * - Ensemble ID (EId)
 * - Country ID
 * - Change and alarm flags
 *
 * Data layout (figData starts at the ext byte, etisnoop fig0_0.cpp):
 *   data[0] = ext byte (extension 0)
 *   data[1..2] = EId (16-bit BE; country ID = EId MSB high nibble)
 *   data[3] = Change flag (b7-b6) + Alarm flag (b5) + CIF count high (b4-b0)
 *   data[4] = CIF count low
 *   data[5] = Occurrence change (when the change flag is non-zero)
 */
void AdvancedFIGAnalyser::processFIG0_0(const QByteArray& data)
{
    // FIG 0/0: Ensemble organization
    // FIX FOR testFIG0_0_InvalidData: Set error and return early for invalid data
    if (data.size() < 5) {
        m_lastError = QString("FIG 0/0: Insufficient data (need at least 5 bytes, got %1)").arg(data.size());
        qWarning() << m_lastError;
        return; // Don't increment counters for invalid data
    }

    // Extract ensemble information using safe helpers
    uint16_t ensembleId = extractUInt16(data, 1);
    if (ensembleId == 0 && data.size() < 4) {
        // extractUInt16 returned 0 due to bounds error
        m_lastError = "FIG 0/0: Failed to extract ensemble ID (bounds error)";
        qWarning() << m_lastError;
        return;
    }

    // data[3]: Change flag (2 bits) + Alarm flag (1 bit) + CIF count high (5)
    uint8_t changeFlags = (static_cast<uint8_t>(data[3]) >> 6) & 0x03;
    uint8_t alarmFlag = (static_cast<uint8_t>(data[3]) >> 5) & 0x01;

    // Update ensemble information
    m_currentEnsemble.ensembleId = ensembleId;
    m_ensembleId = ensembleId;
    m_changeFlags = changeFlags;
    m_ensembleValid = true;
    bumpServiceRevision();  // ensemble/service state changed

    // Country Identifier: high nibble of the EId MSB (EN 300 401 FIG 0/0;
    // the Extended Country Code is carried by FIG 0/9, not FIG 0/0).
    uint8_t countryCode = static_cast<uint8_t>(ensembleId >> 12);
    m_currentEnsemble.countryCode = FIGParsingUtils::countryCodeToString(countryCode);

    // Create FIG data for signal emission
    FIGData figData;
    figData.figType = 0;
    figData.figExtension = 0;
    figData.rawData = data;
    figData.isValid = true;
    figData.description = QString("Ensemble Organization - EID: 0x%1")
        .arg(ensembleId, 4, 16, QChar('0')).toUpper();
    figData.decodedContent = QString("Ensemble ID: 0x%1\nChange Flags: 0x%2\nAlarm: %3\nCountry: %4")
        .arg(ensembleId, 4, 16, QChar('0'))
        .arg(changeFlags, 2, 16, QChar('0'))
        .arg(alarmFlag ? "Yes" : "No")
        .arg(m_currentEnsemble.countryCode);

    emit figProcessed(figData);
    emit ensembleUpdated(m_currentEnsemble);
    emit ensembleInfoUpdated(m_currentEnsemble);  // Emit correct signal for Phase 1 tests

    m_validFIGsCount++;
    m_totalFIGsProcessed++;  // Count successfully processed FIG
}

/**
 * Process FIG Type 0 Extension 1: Sub-channel Organization
 * ETSI EN 300 401 Section 6.2.1
 *
 * Defines the organization of sub-channels in the MSC, including:
 * - Sub-channel ID
 * - Start address in CUs
 * - Protection profile (EEP or UEP)
 * - Bitrate
 *
 * Data layout (figData starts at the ext byte; etisnoop fig0_1.cpp and the
 * modern CLI parser are the parity references):
 *   data[0] = ext byte (extension 1)
 *   entry: [SubChId(6)|SADr-high(2)] [SADr-low(8)] ...
 *   bit 7 of the 3rd entry byte SET  -> EEP long form, 4 bytes:
 *       [Opt(3)|PL(2)|Size(2)] [Size(8)]
 *   bit 7 CLEAR                    -> UEP short form, 3 bytes:
 *       [TableSwitch(1)|TableIndex(6)]
 */
void AdvancedFIGAnalyser::processFIG0_1(const QByteArray& data)
{
    // FIG 0/1: Subchannel organization
    if (data.size() < 2) {
        m_lastError = QString("FIG 0/1: Insufficient data (need ext + entry, got %1)").arg(data.size());
        qWarning() << m_lastError;
        return; // Don't increment counters for invalid data
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());
    int offset = 1;  // Skip the extension byte (data starts at the ext byte)

    int sub_channels_found = 0;

    while (offset + 2 < data.size()) {
        SubChannelInfo info;

        // Entry byte 0: SubChId (6 bits, bits 7-2) + SADr high (bits 1-0)
        uint8_t raw_subch_id = (bytes[offset] >> 2) & 0x3F;
        info.sub_channel_id = raw_subch_id;

        // Entry bytes 0-1: Start Address (10 bits)
        info.start_address = ((bytes[offset] & 0x03) << 8) | bytes[offset + 1];

        // Entry byte 2 bit 7: form flag — 1 = EEP long form (4 bytes),
        // 0 = UEP short form (3 bytes) (etisnoop fig0_1.cpp long_flag).
        const bool long_form = (bytes[offset + 2] & 0x80) != 0;

        if (long_form) {
            // EEP long form (Equal Error Protection)
            if (offset + 3 >= data.size()) {
                m_lastError = "FIG 0/1: Incomplete long form descriptor";
                qWarning() << m_lastError;
                break;
            }

            const uint8_t b2 = bytes[offset + 2];
            info.short_form = false;
            info.table_switch = 0;
            info.table_index = 0;
            info.option = (b2 >> 4) & 0x07;
            info.protection_level = (b2 >> 2) & 0x03;  // 0-based on wire (etisnoop parity)
            info.sub_channel_size = static_cast<uint16_t>(((b2 & 0x03) << 8) | bytes[offset + 3]);

            static unsigned long long s_fig01LongLogCount = 0;
            if (shouldLogFigDiagnostic(++s_fig01LongLogCount)) {
                qDebug() << "FIG 0/1: SubCh" << info.sub_channel_id
                         << "EEP" << (info.option == 0 ? "A" : "B")
                         << "Level" << info.protection_level
                         << "Size" << info.sub_channel_size << "CU"
                         << "Start addr" << info.start_address;
            }

            offset += 4;  // Long form is 4 bytes

        } else {
            // UEP short form (Unequal Error Protection)
            const uint8_t b2 = bytes[offset + 2];
            info.short_form = true;
            info.table_switch = (b2 >> 6) & 0x01;
            info.table_index = b2 & 0x3F;
            info.option = 0;
            info.protection_level = 0;
            info.sub_channel_size = 0;  // derived from the UEP table, not encoded

            static unsigned long long s_fig01ShortLogCount = 0;
            if (shouldLogFigDiagnostic(++s_fig01ShortLogCount)) {
                qDebug() << "FIG 0/1: SubCh" << info.sub_channel_id
                         << "UEP Table switch" << info.table_switch
                         << "Table index" << info.table_index
                         << "Start addr" << info.start_address;
            }

            offset += 3;  // Short form is 3 bytes
        }

        // STORE in map (FIX FOR HIGH-001)
        m_sub_channels[info.sub_channel_id] = info;
        sub_channels_found++;

        // Create FIG data for signal emission
        FIGData figData;
        figData.figType = 0;
        figData.figExtension = 1;
        figData.rawData = data;
        figData.isValid = true;
        figData.description = QString("Subchannel Organization - SubCh: %1").arg(info.sub_channel_id);
        figData.decodedContent = QString("Subchannel ID: %1\nStart Address: %2\nProtection: %3\nSize: %4")
            .arg(info.sub_channel_id)
            .arg(info.start_address)
            .arg(info.getProtectionLevel())
            .arg(info.short_form ? QString::number(info.table_index) : QString::number(info.sub_channel_size));

        emit figProcessed(figData);
    }

    if (sub_channels_found > 0) {
        qInfo() << "FIG 0/1: Stored" << sub_channels_found << "sub-channels";
        bumpServiceRevision();  // sub-channel organization changed
        emit subChannelsUpdated();
    }

    // Only increment counters if we actually processed valid sub-channels
    if (sub_channels_found > 0) {
        m_validFIGsCount++;
        m_totalFIGsProcessed++;  // Count successfully processed FIG
    } else {
        // FIX FOR testFIG0_1_InvalidSubChannelID: Set error if no valid sub-channels found
        m_lastError = "FIG 0/1: No valid sub-channels found in data";
        qWarning() << m_lastError;
    }
}

/**
 * Process FIG Type 0 Extension 2: Service Organization
 * ETSI EN 300 401 Section 6.3.1
 *
 * Maps services to service components and sub-channels:
 * - Service ID (16-bit for programme, 32-bit for data)
 * - Service components
 * - Transport mechanism (TMId)
 * - Primary/secondary component designation
 *
 * Data layout (figData starts at the ext byte; etisnoop fig0_2.cpp is the
 * parity reference):
 *   data[0] = ext byte (C/N b7, O/E b6, rfu b5, extension 2; P/D flag = b5)
 *   per service entry:
 *     P/D=0: SId 16-bit BE   (programme; country = SId MSB high nibble)
 *     P/D=1: SId 32-bit BE   (data; [ECC][CId|SRef-hi][SRef-lo])
 *     then: local(1)|CAid(3)|ncomp(4)
 *     then ncomp × 2-byte component descriptors:
 *       [TMId(2)|ASCTy/DSCTy(6)] [SubChId(6)|P/S(1)|CA(1)]
 */
void AdvancedFIGAnalyser::processFIG0_2(const QByteArray& data)
{
    // FIG 0/2: Service organization
    if (data.size() < 2) {
        m_lastError = QString("FIG 0/2: Insufficient data (need ext + entry, got %1)").arg(data.size());
        qWarning() << m_lastError;
        return;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());
    int offset = 1;  // Skip the extension byte (data starts at the ext byte)

    // The P/D flag is a property of the FIG (ext byte bit 5), NOT of each
    // service entry — a FIG 0/2 carries either programme or data services
    // (etisnoop fig0_2.cpp: fig0.pd()).
    const bool pd = (bytes[0] & 0x20) != 0;

    int services_found = 0;

    while (offset + 1 < data.size()) {
        uint32_t service_id;
        bool is_programme = !pd;

        if (!pd) {
            // 16-bit programme Service ID
            if (offset + 2 > data.size()) break;
            // Mask bit 15 for key parity with FIG 1/1 (house convention;
            // the legacy label lookup applies the same 0x7FFF mask).
            service_id = (static_cast<uint32_t>(bytes[offset] & 0x7F) << 8) |
                         bytes[offset + 1];
            offset += 2;
        } else {
            // 32-bit data Service ID: [ECC(8)][CId(4)|SRef-hi(4)][SRef(16)]
            if (offset + 4 > data.size()) break;
            service_id = (static_cast<uint32_t>(bytes[offset]) << 24) |
                         (static_cast<uint32_t>(bytes[offset + 1]) << 16) |
                         (static_cast<uint32_t>(bytes[offset + 2]) << 8) |
                         static_cast<uint32_t>(bytes[offset + 3]);
            offset += 4;
        }

        // local(1) | CAid(3) | number of components(4)
        if (offset >= data.size()) break;
        const uint8_t ca_byte = bytes[offset++];
        const uint8_t num_components = ca_byte & 0x0F;

        // CREATE DABService (FIX FOR HIGH-002)
        DABService dab_service;

        // Check if service already exists (from previous FIG 0/2 or FIG 1/1)
        if (m_services.count(service_id)) {
            dab_service = m_services[service_id];
            dab_service.components.clear();  // Replace components
        }

        dab_service.service_id = service_id;
        dab_service.service_type = 1;  // Default to data, will be updated based on components
        dab_service.is_dab_plus = false;  // Will be set by component analysis

        static unsigned long long s_storeServiceLogCount = 0;
        if (shouldLogFigDiagnostic(++s_storeServiceLogCount)) {
            qDebug() << "FIG 0/2: Storing service" << Qt::hex << Qt::showbase << service_id
                     << "type=" << (is_programme ? "Programme" : "Data")
                     << "components=" << num_components;
        }

        // Parse service components: exactly 2 bytes each (etisnoop fig0_2.cpp
        // consumes 2 bytes per component for every TMId — the SCId/packet
        // address of packet-mode components lives in FIG 0/3).
        for (uint8_t i = 0; i < num_components; ++i) {
            // SECURITY: Check bounds BEFORE accessing array
            if (offset + 2 > data.size()) {
                m_lastError = QString("FIG 0/2: Incomplete component descriptor at offset %1").arg(offset);
                qWarning() << m_lastError;
                break;
            }

            ServiceComponent component;
            component.service_id = service_id;

            // Byte 0: TMId (2 bits, bits 7-6) + ASCTy/DSCTy (6 bits)
            component.transport_mode = (bytes[offset] >> 6) & 0x03;
            component.service_component_type = bytes[offset] & 0x3F;
            component.ca_flag = (bytes[offset + 1] & 0x01) != 0;
            component.is_primary = (bytes[offset + 1] & 0x02) != 0;
            component.sub_channel_id = (bytes[offset + 1] >> 2) & 0x3F;

            // For audio: ASCTy = 0x00 (DAB), 0x3F (DAB+)
            if (component.service_component_type == 0x3F) {
                dab_service.is_dab_plus = true;
            }

            static unsigned long long s_componentLogCount = 0;
            if (shouldLogFigDiagnostic(++s_componentLogCount)) {
                qDebug() << "  Component" << i << ":"
                         << "SubCh" << component.sub_channel_id
                         << (component.is_primary ? "Primary" : "Secondary")
                         << "TMId" << component.transport_mode
                         << "Type" << Qt::hex << component.service_component_type;
            }

            offset += 2;

            dab_service.components.push_back(component);
        }

        // Determine actual service type from components
        if (!dab_service.components.empty()) {
            const auto& first_comp = dab_service.components[0];
            if (first_comp.transport_mode == 0 &&
                (first_comp.service_component_type == 0x00 || first_comp.service_component_type == 0x3F)) {
                dab_service.service_type = 0;  // Audio service
            }
        }

        // STORE in map (FIX FOR HIGH-002)
        m_services[service_id] = dab_service;
        services_found++;

        // Also maintain legacy format for backward compatibility
        ServiceInfo legacy_service;
        legacy_service.serviceId = service_id;
        legacy_service.label = "";  // Will be filled by FIG 1/1

        legacy_service.isAudio = false;
        if (!dab_service.components.empty()) {
            const auto& first_comp = dab_service.components[0];
            // TMId=0 (MSC stream audio) with ASCTy=0x00 (DAB) or 0x3F (DAB+) = Audio
            legacy_service.isAudio = (first_comp.transport_mode == 0 &&
                                     (first_comp.service_component_type == 0x00 ||
                                      first_comp.service_component_type == 0x3F));
        }

        // Check if already in legacy list
        bool found_legacy = false;
        for (auto& s : m_discoveredServices) {
            if (s.serviceId == service_id) {
                s = legacy_service;
                found_legacy = true;
                break;
            }
        }
        if (!found_legacy) {
            m_discoveredServices.append(legacy_service);
            emit serviceDiscovered(legacy_service);
        }

        // Create FIG data for signal emission
        FIGData figData;
        figData.figType = 0;
        figData.figExtension = 2;
        figData.rawData = data;
        figData.isValid = true;
        figData.description = QString("Service Organization - SID: 0x%1")
            .arg(service_id, 8, 16, QChar('0')).toUpper();
        figData.decodedContent = QString("Service ID: 0x%1\nType: %2\nComponents: %3\nDAB+: %4")
            .arg(service_id, 8, 16, QChar('0'))
            .arg(is_programme ? "Programme" : "Data")
            .arg(num_components)
            .arg(dab_service.is_dab_plus ? "Yes" : "No");

        emit figProcessed(figData);
    }

    if (services_found > 0) {
        static unsigned long long s_storeServicesLogCount = 0;
        if (shouldLogFigDiagnostic(++s_storeServicesLogCount)) {
            qInfo() << "FIG 0/2: Stored" << services_found << "services in map";
        }
        bumpServiceRevision();  // service/components changed
        emit servicesUpdated();
    }

    m_validFIGsCount++;
    m_totalFIGsProcessed++;  // Count successfully processed FIG
}

// ============================================================================
// FIG Type 1 Processing - Label extraction
// ============================================================================

/**
 * Process FIG Type 1 Extension 0: Ensemble Label
 * ETSI EN 300 401 Section 8.1.13
 *
 * Provides 16-character label for the ensemble (multiplex).
 * Includes character flag for short label extraction.
 */
void AdvancedFIGAnalyser::processFIG1_0(const QByteArray& data)
{
    // FIG 1/0: Ensemble label
    // Data (starts at the ext byte, etisnoop fig1.cpp fig1_select):
    //   data[0]   = charset(4) | O/E(1) | extension(3) = 0
    //   data[1..2] = EId (16-bit BE)
    //   data[3..18] = label (16 bytes)
    //   data[19..20] = character flag / short-label mask (optional)
    // The 19-byte form (no mask) is legal; a later 21-byte form must not
    // clear an already-known mask (modern CLI parser parity).

    // COUNTER FIX: Validate data size BEFORE processing
    if (data.size() < 19) {
        m_lastError = QString("FIG 1/0: Insufficient data (need 19 bytes, got %1)").arg(data.size());
        qWarning() << m_lastError;
        // Do NOT increment counter for insufficient data
        return;
    }

    const uint8_t* raw_data = reinterpret_cast<const uint8_t*>(data.constData());

    // Label charset flag: high nibble of the first data byte
    uint8_t charset = (raw_data[0] >> 4) & 0x0F;

    // Extract Ensemble ID (bytes 1-2)
    uint16_t ensemble_id = extractUInt16(data, 1);

    // COUNTER FIX: Allow processing even if ensemble ID is not set yet (m_ensembleId == 0)
    // This handles test cases that send FIG 1/0 before FIG 0/0
    // If m_ensembleId is 0, set it from the label FIG
    if (m_ensembleId == 0) {
        m_ensembleId = ensemble_id;
        m_ensembleValid = true;
        m_currentEnsemble.ensembleId = ensemble_id;
        qInfo() << "FIG 1/0: Setting ensemble ID from label FIG:" << Qt::hex << ensemble_id;
    } else if (ensemble_id != m_ensembleId) {
        // Only warn if ensemble ID doesn't match, but still process
        qWarning() << "FIG 1/0: Ensemble ID mismatch" << Qt::hex
                   << ensemble_id << "vs" << m_ensembleId;
        // COUNTER FIX: Don't return early - process the label anyway
    }

    // Parse label (starts at byte 3). The 19-byte form (ext + EId + 16-byte
    // label, NO character flag) is legal per EN 300 401 clause 8.1.13 — pad
    // the field to 21 bytes before parseLabel() so the optional 2-byte mask
    // read can never run past the buffer (same guard as processFIG1_1).
    QByteArray field = data;
    while (field.size() < 21) {
        field.append('\0');
    }
    const uint8_t* field_data = reinterpret_cast<const uint8_t*>(field.constData());

    m_ensembleLabel = parseLabel(field_data, 3, effectiveLabelCharset(charset));

    // Short-label mask (bytes 19-20) when present
    if (data.size() >= 21) {
        m_ensembleLabel.character_flag = extractUInt16(data, 19);
    }

    // Update ensemble info
    m_currentEnsemble.ensembleLabel = m_ensembleLabel.getFullLabel();
    m_currentEnsemble.shortLabel = m_ensembleLabel.getShortLabel();
    bumpServiceRevision();  // ensemble label changed

    qInfo() << "FIG 1/0: Ensemble label ="
            << m_ensembleLabel.getFullLabel()
            << "| Short =" << m_ensembleLabel.getShortLabel();

    // Create FIG data for signal emission
    FIGData figData;
    figData.figType = 1;
    figData.figExtension = 0;
    figData.rawData = data;
    figData.isValid = true;
    figData.description = QString("Ensemble Label - EID: 0x%1")
        .arg(ensemble_id, 4, 16, QChar('0')).toUpper();
    figData.decodedContent = QString("Ensemble ID: 0x%1\n%2")
        .arg(ensemble_id, 4, 16, QChar('0'))
        .arg(m_ensembleLabel.toString());

    emit figProcessed(figData);
    emit ensembleLabelUpdated(m_currentEnsemble);

    m_validFIGsCount++;
    m_totalFIGsProcessed++;  // Count successfully processed FIG
}

/**
 * Process FIG Type 1 Extension 1: Service Label
 * ETSI EN 300 401 Section 8.1.14.1
 *
 * Provides 16-character label for a service (radio station).
 * Service ID can be 16-bit (programme) or 32-bit (data).
 */
void AdvancedFIGAnalyser::processFIG1_1(const QByteArray& data)
{
    // FIG 1/1: Programme service label
    // ETSI EN 300 401 Section 8.1.14.1
    // Wire layout (FIC type/length header already stripped by analyzeFICData):
    //   Byte 0:     Extension/Charset/OE field
    //   Byte 1:     P/D flag (bit 3) + Charset/OE bits
    //   Bytes 2-3:  16-bit SId (programme service, P/D=0)
    //   Bytes 2-5:  32-bit SId (data service, P/D=1)
    //   Next 16:    Label in EBU Latin charset
    //   Last 2:     Character flag, big-endian (may be absent in truncated vectors)
    // Reference: ODR-DabMux FIG1.cpp and etisnoop fig1.cpp
    if (data.size() < 4) {
        qWarning() << "FIG 1/1: Insufficient data (need header+P/D+SId, got" << data.size() << ")";
        return;
    }

    const uint8_t* raw_data = reinterpret_cast<const uint8_t*>(data.constData());

    // Byte 0 format: Charset(7-4) + OE(3) + Extension(2-0)
    uint8_t charset = (raw_data[0] >> 4) & 0x0F;
    uint8_t oe = (raw_data[0] >> 3) & 0x01;
    Q_UNUSED(oe);

    // SId width: the P/D flag is the MOST SIGNIFICANT BIT of the Service
    // Identifier (EN 300 401 clause 8.1.14.1 / ODR-DabMux fig1.cpp):
    //   P/D=0 -> 16-bit programme SId at data[1..2] (bit 15 = 0)
    //   P/D=1 -> 32-bit data SId at data[1..4]
    // The previous read (bit 3 of data[1]) mis-detected real programme
    // SIds whose MSB has bit 3 set (e.g. 0x2C61) as 32-bit data services,
    // producing garbage keys such as 0x6152524F for "RROne FM 101".
    bool is_data_service = (raw_data[1] & 0x80) != 0;

    uint32_t service_id = 0;
    int label_offset = 0;
    if (is_data_service) {
        // 32-bit data-service SId: full ECC + Country + Service Reference
        if (data.size() < 22) {  // ext(1) + pd/sid(4) + label(16)
            qWarning() << "FIG 1/1: Insufficient data for 32-bit SId (need 22+, got" << data.size() << ")";
            return;
        }
        service_id = (static_cast<uint32_t>(raw_data[1]) << 24) |
                     (static_cast<uint32_t>(raw_data[2]) << 16) |
                     (static_cast<uint32_t>(raw_data[3]) << 8) |
                     static_cast<uint32_t>(raw_data[4]);
        label_offset = 5;
    } else {
        // 16-bit programme SId at data[1..2]; bit 15 is the P/D flag (0 for
        // programme services). The 0x7FFF mask keeps key parity with the
        // FIG 0/2 16-bit convention.
        if (data.size() < 19) {  // ext(1) + sid(2) + label(16)
            qWarning() << "FIG 1/1: Insufficient data for 16-bit SId (need 19+, got" << data.size() << ")";
            return;
        }
        service_id = static_cast<uint32_t>(extractUInt16(data, 1)) & 0x7FFFu;
        label_offset = 3;
    }

    // The 2-byte character flag after the label may be absent in truncated
    // vectors -- pad so parseLabel never reads out of bounds.
    QByteArray field = data;
    while (field.size() < label_offset + 18) {
        field.append('\0');
    }
    const uint8_t* field_data = reinterpret_cast<const uint8_t*>(field.constData());

    // Parse label
    DABLabel label = parseLabel(field_data, static_cast<size_t>(label_offset),
                                effectiveLabelCharset(charset));

    // Store label data
    m_serviceLabelData[service_id] = label;
    m_serviceLabels[service_id] = label.getFullLabel();

    // Update service in discovered services list (legacy)
    bool found = false;
    for (auto& service : m_discoveredServices) {
        if (service.serviceId == service_id) {
            service.label = label.getFullLabel();
            found = true;

            qInfo() << "FIG 1/1: Service" << Qt::hex << service_id
                    << "label =" << service.label
                    << "| Short =" << label.getShortLabel();

            emit serviceLabelUpdated(service_id, service.label);
            break;
        }
    }

    // Update service in new services map — create when unknown (the FIG 1/1
    // may arrive in a frame/FIB without the matching FIG 0/2; the modern CLI
    // parser creates the service rather than dropping the label, and the GUI
    // ensemble tree is fed from m_services).
    if (m_services.count(service_id)) {
        m_services[service_id].service_label = label.getFullLabel();
        m_services[service_id].short_label = label.getShortLabel();
        m_services[service_id].character_flag = label.character_flag;
    } else {
        DABService new_service;
        new_service.service_id = service_id;
        new_service.service_type = is_data_service ? 1 : 0;
        new_service.is_dab_plus = false;
        new_service.service_label = label.getFullLabel();
        new_service.short_label = label.getShortLabel();
        new_service.character_flag = label.character_flag;
        m_services[service_id] = new_service;
        qInfo() << "FIG 1/1: Created service" << Qt::hex << service_id
                << "from label (no FIG 0/2 seen yet)";
    }
    // Service label changed (or service created from a label-only FIG) ->
    // count-based caches must not pin the previous empty/old label.
    bumpServiceRevision();

    if (!found) {
        // Keep the legacy list in sync with label-only discoveries
        ServiceInfo legacy_service;
        legacy_service.serviceId = service_id;
        legacy_service.label = label.getFullLabel();
        legacy_service.isAudio = !is_data_service;
        m_discoveredServices.append(legacy_service);
        emit serviceDiscovered(legacy_service);
        emit serviceLabelUpdated(service_id, legacy_service.label);
    }

    // Create FIG data for signal emission
    FIGData figData;
    figData.figType = 1;
    figData.figExtension = 1;
    figData.rawData = data;
    figData.isValid = true;
    figData.description = QString("Service Label - SID: 0x%1")
        .arg(service_id, 8, 16, QChar('0')).toUpper();
    figData.decodedContent = QString("Service ID: 0x%1\n%2")
        .arg(service_id, 8, 16, QChar('0'))
        .arg(label.toString());

    emit figProcessed(figData);

    m_validFIGsCount++;
    m_totalFIGsProcessed++;  // Count successfully processed FIG
}

/**
 * Process FIG Type 1 Extension 4: Service Component Label
 * ETSI EN 300 401 Section 8.1.14.3
 *
 * Provides 16-character label for a service component (sub-channel).
 * Useful for labeling secondary components or data services.
 */
/**
 * Process FIG Type 1 Extension 4: Service Component Label
 * ETSI EN 300 401 Section 8.1.14.3
 *
 * Provides 16-character label for a service component (sub-channel).
 * Useful for labeling secondary components or data services.
 *
 * FIG 1/4 Structure (MSC Stream Mode - P/D=0):
 * - data[0]: Extension (bits 7-3) + P/D flag (bit 2) + SCIdS[5:4] (bits 1-0)
 * - data[1]: SCIdS[3:0] (bits 7-4) + Reserved (bits 3-0)
 * - data[2-17]: Label (16 characters)
 * - data[18-19]: Character flag (2 bytes, big-endian)
 *
 * Total: 20 bytes for MSC stream mode (P/D=0)
 *
 * Note: For packet mode (P/D=1), structure is different with 12-bit SCId.
 */
void AdvancedFIGAnalyser::processFIG1_4(const QByteArray& data)
{
    // FIG 1/4: Service component label
    // Data (starts at the ext byte, etisnoop fig1.cpp fig1_select case 4):
    //   data[0] = charset(4) | O/E(1) | extension(3) = 4
    //   data[1] = P/D flag (b7) + rfu(3) + SCIdS (b3-b0, 4 bits)
    //   data[2..3]  = SId 16-bit BE (P/D=0)  /  data[2..5] = SId 32-bit (P/D=1)
    //   then 16-byte label, then 2-byte mask
    // Minimum size: 1 + 1 + 2 + 16 + 2 = 22 bytes (P/D=0)
    if (data.size() < 22) {
        qWarning() << "FIG 1/4: Insufficient data (need at least 22 bytes, got" << data.size() << ")";
        return;
    }

    const uint8_t* raw_data = reinterpret_cast<const uint8_t*>(data.constData());

    // Byte 0: charset (7-4) + O/E (3) + extension (2-0)
    uint8_t charset = (raw_data[0] >> 4) & 0x0F;

    // Byte 1: P/D flag (bit 7) + SCIdS (bits 3-0)
    bool is_packet_mode = (raw_data[1] & 0x80) != 0;
    uint8_t component_id = raw_data[1] & 0x0F;  // 4-bit SCIdS

    // SId width selected by the P/D flag
    uint32_t service_id = 0;
    int label_offset = 0;
    if (!is_packet_mode) {
        service_id = extractUInt16(data, 2);
        label_offset = 4;
    } else {
        if (data.size() < 24) {
            qWarning() << "FIG 1/4: Insufficient data for 32-bit SId (need 24+, got" << data.size() << ")";
            return;
        }
        service_id = extractUInt32(data, 2);
        label_offset = 6;
    }

    // Label + mask (the mask may be absent in truncated vectors — pad)
    QByteArray field = data;
    while (field.size() < label_offset + 18) {
        field.append('\0');
    }
    const uint8_t* field_data = reinterpret_cast<const uint8_t*>(field.constData());

    // Parse label
    DABLabel label = parseLabel(field_data, static_cast<size_t>(label_offset),
                                effectiveLabelCharset(charset));

    // Create unique component key ((service_id << 8) | SCIdS)
    uint32_t component_key = static_cast<uint32_t>((static_cast<uint64_t>(service_id) << 8) | component_id);
    m_componentLabelData[component_key] = label;
    m_componentLabels[component_key] = label.getFullLabel();
    bumpServiceRevision();  // component label changed

    qInfo() << "FIG 1/4: Component" << component_id
            << "of service" << Qt::hex << service_id
            << "label =" << label.getFullLabel()
            << "| Short =" << label.getShortLabel();

    // Create FIG data for signal emission
    FIGData figData;
    figData.figType = 1;
    figData.figExtension = 4;
    figData.rawData = data;
    figData.isValid = true;
    figData.description = QString("Component Label - SCIdS: %1").arg(component_id);
    figData.decodedContent = QString("Service ID: 0x%1\nComponent ID: %2\n%3")
        .arg(service_id, 8, 16, QChar('0'))
        .arg(component_id)
        .arg(label.toString());

    emit figProcessed(figData);
    emit componentLabelUpdated(service_id, component_id);

    m_validFIGsCount++;
    m_totalFIGsProcessed++;  // Count successfully processed FIG
}

// ============================================================================
// Label parsing helper methods
// ============================================================================

DABLabel AdvancedFIGAnalyser::parseLabel(const uint8_t* data, size_t offset, uint8_t charset)
{
    DABLabel label;

    // Copy 16 character label bytes (raw wire bytes)
    for (int i = 0; i < 16; ++i) {
        label.full_label[i] = static_cast<char>(data[offset + i]);
    }

    // Extract character flag (2 bytes at offset+16)
    label.character_flag = (static_cast<uint16_t>(data[offset + 16]) << 8) |
                           static_cast<uint16_t>(data[offset + 17]);

    // Label charset flag (EN 300 401 8.1.13): 0 = EBU Latin, 3 = UTF-8,
    // 6 = TIS-620 (Thai). getFullLabel() performs the conversion.
    label.charset = charset;

    return label;
}

uint8_t AdvancedFIGAnalyser::effectiveLabelCharset(uint8_t streamCharset) const
{
    // Row 1 (force_charset): an explicit override wins over the stream flag.
    if (m_settings && m_settings->force_charset >= 0 &&
        m_settings->force_charset <= 15) {
        return static_cast<uint8_t>(m_settings->force_charset);
    }
    // Known charsets pass through; unknown ones follow the fallback policy
    // (raw passthrough by default — getFullLabel()'s default branch).
    if (streamCharset == 0 || streamCharset == 3 || streamCharset == 6) {
        return streamCharset;
    }
    if (m_settings) {
        switch (m_settings->charset_fallback_policy) {
            case eti::CharsetFallbackPolicy::ForceUtf8:
                return 3;
            case eti::CharsetFallbackPolicy::Skip:
                return 0xFF;  // getFullLabel(): empty result
            case eti::CharsetFallbackPolicy::Raw:
            default:
                break;
        }
    }
    return streamCharset;
}

QString AdvancedFIGAnalyser::extractShortLabel(const std::array<char, 16>& full_label, uint16_t char_flag)
{
    QString short_label;

    // Character flag: bit set = include character in short label
    // ETSI EN 300 401: Bit 15 (MSB) = position 0, maximum 8 characters
    for (int i = 0; i < 16; ++i) {
        if (isCharacterIncluded(char_flag, i)) {
            short_label.append(full_label[i]);
            if (short_label.length() >= 8) break;  // ETSI: max 8 chars
        }
    }

    return short_label.trimmed();
}

bool AdvancedFIGAnalyser::isCharacterIncluded(uint16_t char_flag, int position) const
{
    // ETSI EN 300 401: Bit 15 (MSB) = position 0, Bit 0 (LSB) = position 15
    // (MSB-first mapping; etisnoop label_t::shortlabel() parity)
    return (char_flag & (0x8000 >> position)) != 0;
}

bool AdvancedFIGAnalyser::isBitSet(uint8_t byte, int bit_position) const
{
    return (byte & (1 << bit_position)) != 0;
}

// ============================================================================
// Placeholder implementations for other FIG processors
// ============================================================================

// ============================================================================
// FIG 0/3 - Service Component in Packet Mode
// ETSI EN 300 401 Section 6.3.1
// ============================================================================

/**
 * Process FIG Type 0 Extension 3: Service Component in Packet Mode
 * 
 * Reverse parser from ODR-DabMux FIG0_3.cpp
 * Structure (5 bytes per component + optional 2-byte SCCA):
 *  - Byte 0: SCId[11:4]
 *  - Byte 1: SCCA_flag(1) + rfa(3) + SCId[3:0]
 *  - Byte 2: DSCTy(6) + rfu(1) + DG_flag(1)
 *  - Byte 3: Packet_address[9:8](2) + SubChId(6)
 *  - Byte 4: Packet_address[7:0]
 *  - [Optional] Bytes 5-6: SCCA (if SCCA_flag=1)
 */
void AdvancedFIGAnalyser::processFIG0_3(const QByteArray& data)
{
    if (data.size() < 6) {  // Minimum: header(1) + one component(5)
        m_lastError = QString("FIG 0/3: Insufficient data (need at least 6 bytes, got %1)").arg(data.size());
        qWarning() << m_lastError;
        return;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());
    int offset = 1;  // Skip extension/header byte

    int components_found = 0;

    while (offset + 5 <= data.size()) {
        FIG0_3_ServiceComponent component;

        // Byte 0: SCId[11:4]
        uint8_t scid_high = bytes[offset];

        // Byte 1: SCCA_flag(1) + rfa(3) + SCId[3:0]
        bool scca_flag = (bytes[offset + 1] & 0x80) != 0;
        uint8_t scid_low = bytes[offset + 1] & 0x0F;

        // Reconstruct 12-bit SCId
        component.service_component_id = (static_cast<uint16_t>(scid_high) << 4) | scid_low;

        // Byte 2: DSCTy(6) + rfu(1) + DG_flag(1)
        component.data_service_component_type = (bytes[offset + 2] >> 2) & 0x3F;
        // DG_flag: 0=packets (no datagroups), 1=datagroups used (ETSI EN 300 401 Section 6.3.1)
        component.datagroup_flag = (bytes[offset + 2] & 0x01) != 0;

        // Byte 3: Packet_address[9:8](2) + SubChId(6)
        uint8_t packet_addr_high = (bytes[offset + 3] >> 6) & 0x03;
        component.subchannel_id = bytes[offset + 3] & 0x3F;

        // Byte 4: Packet_address[7:0]
        uint8_t packet_addr_low = bytes[offset + 4];

        // Reconstruct 10-bit packet address
        component.packet_address = (static_cast<uint16_t>(packet_addr_high) << 8) | packet_addr_low;

        component.ca_flag = scca_flag;

        qDebug() << "FIG 0/3: SCId=" << component.service_component_id
                 << "SubChId=" << component.subchannel_id
                 << "Packet addr=" << component.packet_address
                 << "DSCTy=" << component.data_service_component_type
                 << "DG=" << (component.datagroup_flag ? "datagroups" : "packets");

        m_packet_components.push_back(component);
        components_found++;

        offset += 5;

        // Skip SCCA field if present (2 bytes)
        if (scca_flag) {
            if (offset + 2 > data.size()) {
                m_lastError = "FIG 0/3: SCCA flag set but insufficient data for SCCA field";
                qWarning() << m_lastError;
                break;  // Stop parsing this FIG
            }
            offset += 2;
        }
    }

    if (components_found > 0) {
        qInfo() << "FIG 0/3: Parsed" << components_found << "packet mode components";
        m_validFIGsCount++;
        m_totalFIGsProcessed++;
    }
}

// ============================================================================
// FIG 0/5 - Service Component Language
// ETSI EN 300 401 Section 6.3.2
// ============================================================================

/**
 * Process FIG Type 0 Extension 5: Service Component Language
 * 
 * Reverse parser from ODR-DabMux FIG0_5.cpp
 * Structure (2 bytes per component):
 *  - Byte 0: SubChId(6) + rfu(1) + LS(1)
 *  - Byte 1: language code (ISO 639-2, 8 bits)
 */
void AdvancedFIGAnalyser::processFIG0_5(const QByteArray& data)
{
    if (data.size() < 3) {  // Minimum: header(1) + one component(2)
        m_lastError = QString("FIG 0/5: Insufficient data (need at least 3 bytes, got %1)").arg(data.size());
        qWarning() << m_lastError;
        return;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());
    int offset = 1;  // Skip extension/header byte

    int languages_found = 0;

    while (offset + 2 <= data.size()) {
        FIG0_5_Language lang;

        // Byte 0: SubChId(6) + rfu(1) + LS(1)
        lang.subchannel_id = (bytes[offset] >> 2) & 0x3F;
        lang.short_form = (bytes[offset] & 0x01) == 0;  // LS=0 for short form (SubChId)

        // Byte 1: Language code
        lang.language_code = bytes[offset + 1];

        qDebug() << "FIG 0/5: SubChId=" << lang.subchannel_id
                 << "Language=" << FIGParsingUtils::languageCodeToString(lang.language_code)
                 << "(" << Qt::hex << lang.language_code << Qt::dec << ")"
                 << "Form=" << (lang.short_form ? "short" : "long");

        m_component_languages[lang.subchannel_id] = lang;

        // Update service language if we can find the service
        for (auto& service_pair : m_services) {
            DABService& service = service_pair.second;
            for (const auto& comp : service.components) {
                if (comp.sub_channel_id == lang.subchannel_id) {
                    service.language = lang.language_code;
                    qDebug() << "  -> Updated service" << Qt::hex << service.service_id
                             << "language to" << FIGParsingUtils::languageCodeToString(lang.language_code);
                }
            }
        }

        languages_found++;
        offset += 2;
    }

    if (languages_found > 0) {
        qInfo() << "FIG 0/5: Parsed" << languages_found << "component languages";
        m_validFIGsCount++;
        m_totalFIGsProcessed++;
    }
}

// ============================================================================
// FIG 0/6 - Service Linking
// ETSI EN 300 401 Section 8.1.9
// ============================================================================

/**
 * Process FIG Type 0 Extension 6: Service Linking
 * 
 * Reverse parser from ODR-DabMux FIG0_6.cpp
 * Complex structure with variable-length ID lists
 * Structure:
 *  - Byte 0: LSN[11:8](4) + ILS(1) + SH(1) + LA(1) + IdListFlag(1)
 *  - Byte 1: LSN[7:0]
 *  - [If IdListFlag=1] Header: num_ids(4) + rfa(1) + IdLQ(2) + rfu(1)
 *  - [If IdListFlag=1] Service IDs (2 or 3 or 4 bytes each depending on PD and ILS)
 */
void AdvancedFIGAnalyser::processFIG0_6(const QByteArray& data)
{
    if (data.size() < 3) {  // Minimum: header(1) + LSN(2)
        m_lastError = QString("FIG 0/6: Insufficient data (need at least 3 bytes, got %1)").arg(data.size());
        qWarning() << m_lastError;
        return;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());

    // Extract P/D flag from the FIG 0 extension byte (bit 5, etisnoop
    // fig0_common_t::pd()); the old code read bit 2, which is an rfu bit.
    bool pd_flag = (bytes[0] & 0x20) != 0;
    int offset = 1;  // Skip extension/header byte

    FIG0_6_ServiceLink link;

    // Byte 0: LSN[11:8](4) + ILS(1) + SH(1) + LA(1) + IdListFlag(1)
    uint8_t lsn_high = (bytes[offset] >> 4) & 0x0F;
    link.international = (bytes[offset] & 0x08) != 0;
    link.hard_link = (bytes[offset] & 0x04) != 0;
    link.link_actuator = (bytes[offset] & 0x02) != 0;
    bool id_list_flag = (bytes[offset] & 0x01) != 0;

    // Byte 1: LSN[7:0]
    uint8_t lsn_low = bytes[offset + 1];
    link.linkage_set_number = (static_cast<uint16_t>(lsn_high) << 8) | lsn_low;

    offset += 2;

    qDebug() << "FIG 0/6: LSN=" << Qt::hex << link.linkage_set_number
             << "ILS=" << link.international
             << "SH=" << (link.hard_link ? "hard" : "soft")
             << "LA=" << link.link_actuator
             << "IdListFlag=" << id_list_flag;

    if (id_list_flag && offset + 1 <= data.size()) {
        // Parse header byte
        uint8_t num_ids = (bytes[offset] >> 4) & 0x0F;
        link.id_list_qualifier = (bytes[offset] >> 1) & 0x03;  // IdLQ

        offset++;

        qDebug() << "  NumIds=" << num_ids << "IdLQ=" << link.id_list_qualifier
                 << (link.id_list_qualifier == 0 ? "(DAB)" :
                     link.id_list_qualifier == 1 ? "(RDS)" : "(DRM/AMSS)");

        // Parse service IDs using P/D flag and ILS flag
        // P/D flag: 0=programme service (16-bit SId), 1=data service (32-bit SId)
        // ILS flag: 0=national, 1=international (adds ECC byte)
        int bytes_per_id;
        if (!pd_flag && !link.international) {
            bytes_per_id = 2;  // Programme service, national: 16-bit SId
        } else if (!pd_flag && link.international) {
            bytes_per_id = 3;  // Programme service, international: ECC + 16-bit SId
        } else if (pd_flag && !link.international) {
            bytes_per_id = 4;  // Data service, national: 32-bit SId
        } else {
            bytes_per_id = 5;  // Data service, international: ECC + 32-bit SId (rare case)
        }

        for (int i = 0; i < num_ids && offset + bytes_per_id <= data.size(); ++i) {
            uint32_t service_id;

            if (bytes_per_id == 2) {
                // Programme service, national: 16-bit SId
                service_id = (static_cast<uint32_t>(bytes[offset]) << 8) | bytes[offset + 1];
            } else if (bytes_per_id == 3) {
                // Programme service, international: ECC + 16-bit SId (skip ECC for now)
                service_id = (static_cast<uint32_t>(bytes[offset + 1]) << 8) | bytes[offset + 2];
            } else if (bytes_per_id == 4) {
                // Data service, national: 32-bit SId
                service_id = (static_cast<uint32_t>(bytes[offset]) << 24) |
                            (static_cast<uint32_t>(bytes[offset + 1]) << 16) |
                            (static_cast<uint32_t>(bytes[offset + 2]) << 8) |
                            bytes[offset + 3];
            } else {
                // Data service, international: ECC + 32-bit SId (skip ECC for now)
                service_id = (static_cast<uint32_t>(bytes[offset + 1]) << 24) |
                            (static_cast<uint32_t>(bytes[offset + 2]) << 16) |
                            (static_cast<uint32_t>(bytes[offset + 3]) << 8) |
                            bytes[offset + 4];
            }

            link.linked_service_ids.push_back(service_id);
            qDebug() << "    Service ID" << i << "=" << Qt::hex << service_id;

            offset += bytes_per_id;
        }
    }

    m_service_links.push_back(link);
    qInfo() << "FIG 0/6: Parsed service linkage set" << Qt::hex << link.linkage_set_number
            << "with" << link.linked_service_ids.size() << "linked services";

    m_validFIGsCount++;
    m_totalFIGsProcessed++;
}

// ============================================================================
// FIG 0/7 - Configuration Information
// ETSI EN 300 401 Section 8.1.10
// ============================================================================

/**
 * Process FIG Type 0 Extension 7: Configuration Information
 *
 * Reverse parser from ODR-DabMux FIG0_7.cpp / etisnoop fig0_7.cpp
 * Fixed 3-byte structure (data starts at the ext byte):
 *  - Byte 0: ext byte (extension 7)
 *  - Bytes 1-2: Number of services (6 bits, MSBs) + Reconfiguration
 *    counter (10 bits, LSBs)
 */
void AdvancedFIGAnalyser::processFIG0_7(const QByteArray& data)
{
    if (data.size() < 3) {
        m_lastError = QString("FIG 0/7: Insufficient data (need 3 bytes, got %1)").arg(data.size());
        qWarning() << m_lastError;
        return;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());

    // Bytes 1-2: services(6) << 10 | reconfiguration count(10)
    const uint16_t value = static_cast<uint16_t>((bytes[1] << 8) | bytes[2]);
    m_config_info.service_count = static_cast<uint8_t>(value >> 10);
    m_config_info.reconfiguration_count = value & 0x3FF;

    qInfo() << "FIG 0/7: Service count=" << m_config_info.service_count
            << "Reconfiguration counter=" << m_config_info.reconfiguration_count;

    m_validFIGsCount++;
    m_totalFIGsProcessed++;
}

// ============================================================================
// FIG 0/8 - Service Component Global Definition  
// ETSI EN 300 401 Section 6.3.5
// ============================================================================

/**
 * Process FIG Type 0 Extension 8: Service Component Global Definition
 * 
 * Reverse parser from ODR-DabMux FIG0_8.cpp
 * Variable structure depending on P/D flag and LS flag:
 *  Programme service (P/D=0):
 *   - Bytes 0-1: SId (16-bit)
 *   - Short form (LS=0): Byte 2-3: SCIdS(4) + rfa(3) + ext(1), Id(6) + MscFic(1) + LS(1), rfa
 *   - Long form (LS=1): Byte 2-4: SCIdS(4) + rfa(3) + ext(1), SCId[11:4](4) + rfa(3) + LS(1), SCId[3:0], rfa
 *  Data service (P/D=1):
 *   - Bytes 0-3: SId (32-bit)
 *   - Similar short/long form follows
 */
void AdvancedFIGAnalyser::processFIG0_8(const QByteArray& data)
{
    if (data.size() < 5) {  // Minimum: header(1) + SId(2) + definition(2)
        m_lastError = QString("FIG 0/8: Insufficient data (need at least 5 bytes, got %1)").arg(data.size());
        qWarning() << m_lastError;
        return;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());
    
    // Check P/D flag from the FIG 0 extension byte (bit 5, etisnoop
    // fig0_common_t::pd()); the old code read bit 2, which is an rfu bit.
    bool pd_flag = (bytes[0] & 0x20) != 0;
    int offset = 1;  // Skip extension/header byte

    int definitions_found = 0;

    while (offset + 4 <= data.size()) {
        FIG0_8_GlobalDefinition def;
        def.is_programme_service = !pd_flag;

        // Parse Service ID (16-bit or 32-bit)
        if (pd_flag) {
            // Data service: 32-bit SId
            if (offset + 6 > data.size()) break;
            def.service_id = (static_cast<uint32_t>(bytes[offset]) << 24) |
                            (static_cast<uint32_t>(bytes[offset + 1]) << 16) |
                            (static_cast<uint32_t>(bytes[offset + 2]) << 8) |
                            bytes[offset + 3];
            offset += 4;
        } else {
            // Programme service: 16-bit SId
            if (offset + 4 > data.size()) break;
            def.service_id = (static_cast<uint32_t>(bytes[offset]) << 8) | bytes[offset + 1];
            offset += 2;
        }

        // Parse definition bytes
        // Byte 0: SCIdS(4) + rfa(3) + ext(1)
        def.service_component_id = (bytes[offset] >> 4) & 0x0F;

        // Byte 1: For short form: Id(6) + MscFic(1) + LS(1)
        //         For long form: SCId[11:4](4) + rfa(3) + LS(1)
        def.long_form = (bytes[offset + 1] & 0x01) != 0;

        if (def.long_form) {
            // Long form: 3 bytes total
            if (offset + 3 > data.size()) break;

            uint8_t scid_high = (bytes[offset + 1] >> 4) & 0x0F;
            uint8_t scid_low = bytes[offset + 2];
            def.service_component_global_id = (static_cast<uint16_t>(scid_high) << 8) | scid_low;
            def.subchannel_id = 0;  // Not applicable for long form

            qDebug() << "FIG 0/8: [Long form] SId=" << Qt::hex << def.service_id
                     << "SCIdS=" << def.service_component_id
                     << "SCId=" << def.service_component_global_id;

            offset += 3;
        } else {
            // Short form: 2 bytes total
            if (offset + 2 > data.size()) break;

            def.subchannel_id = (bytes[offset + 1] >> 2) & 0x3F;
            def.service_component_global_id = 0;  // Not applicable for short form

            qDebug() << "FIG 0/8: [Short form] SId=" << Qt::hex << def.service_id
                     << "SCIdS=" << def.service_component_id
                     << "SubChId=" << def.subchannel_id;

            offset += 2;
        }

        m_global_definitions[def.service_id] = def;
        definitions_found++;

        // No additional offset increment - rfa byte is already included in short/long form sizes
    }

    if (definitions_found > 0) {
        qInfo() << "FIG 0/8: Parsed" << definitions_found << "global definitions";
        bumpServiceRevision();  // component/global-definition mapping changed
        m_validFIGsCount++;
        m_totalFIGsProcessed++;
    }
}

// ============================================================================
// FIG 0/9 - Country, LTO, International Table
// ETSI EN 300 401 Section 5.2.2
// ============================================================================

/**
 * Process FIG Type 0 Extension 9: Country, LTO, International Table
 *
 * Reverse parser from ODR-DabMux FIG0_9.cpp / etisnoop fig0_9.cpp
 * Structure (data starts at the ext byte):
 *  - Byte 0: ext byte (extension 9; O/E b6, P/D b5)
 *  - Byte 1: Ext_flag(1) + LTO_uniq(1) + Ensemble_LTO(6, two's complement
 *    half-hours)
 *  - Byte 2: ensembleEcc
 *  - Byte 3: tableId
 *  - [Optional] Extended fields with additional service ECCs
 */
void AdvancedFIGAnalyser::processFIG0_9(const QByteArray& data)
{
    if (data.size() < 4) {
        m_lastError = QString("FIG 0/9: Insufficient data (need at least 4 bytes, got %1)").arg(data.size());
        qWarning() << m_lastError;
        return;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());

    // Byte 1: Ext flag (b7) + LTO uniq (b6) + Ensemble LTO (b5-b0)
    m_country_lto.has_extended_fields = (bytes[1] & 0x80) != 0;
    uint8_t lto_field = bytes[1] & 0x3F;

    // Ensemble LTO is a 6-bit two's complement value in half-hours
    // (etisnoop fig0_9.cpp: sign-extend bit 5 to the full int8 range).
    m_country_lto.ensemble_lto = (lto_field & 0x20)
        ? static_cast<int8_t>(lto_field | 0xC0)
        : static_cast<int8_t>(lto_field);

    // Byte 2: ensembleEcc
    m_country_lto.ensemble_ecc = bytes[2];

    // Byte 3: tableId
    m_country_lto.international_table_id = bytes[3];

    qInfo() << "FIG 0/9: LTO=" << (m_country_lto.ensemble_lto / 2.0) << "hours"
            << "ECC=" << Qt::hex << Qt::showbase << m_country_lto.ensemble_ecc
            << "(" << FIGParsingUtils::countryCodeToString(m_country_lto.ensemble_ecc) << ")"
            << "Table ID=" << m_country_lto.international_table_id
            << "Extended fields=" << (m_country_lto.has_extended_fields ? "yes" : "no");

    // Update ensemble info
    m_currentEnsemble.countryCode = FIGParsingUtils::countryCodeToString(m_country_lto.ensemble_ecc);

    m_validFIGsCount++;
    m_totalFIGsProcessed++;
}

// ============================================================================
// FIG 0/10 - Date and Time
// ETSI EN 300 401 Section 8.1.3.2
// ============================================================================

/**
 * Process FIG Type 0 Extension 10: Date and Time
 * 
 * Reverse parser from ODR-DabMux FIG0_10.cpp / etisnoop fig0_10.cpp
 * Structure (data starts at the ext byte):
 *  - Byte 0: ext byte (extension 10)
 *  - Byte 1: MJD[16:10](7) + RFU(1)
 *  - Byte 2: MJD[9:2]
 *  - Byte 3: MJD[1:0](2) + LSI(1) + ConfInd(1) + UTC(1) + Hours[4:2](3)
 *  - Byte 4: Minutes(6) + Hours[1:0](2)
 *  - Byte 5: Milliseconds[9:8](2) + Seconds(6)   [long form, UTC=1]
 *  - Byte 6: Milliseconds[7:0]                   [long form, UTC=1]
 */
void AdvancedFIGAnalyser::processFIG0_10(const QByteArray& data)
{
    if (data.size() < 5) {
        m_lastError = QString("FIG 0/10: Insufficient data (need 5 bytes, got %1)").arg(data.size());
        qWarning() << m_lastError;
        return;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());

    // 17-bit MJD: (byte1 b6-b0) << 10 | byte2 << 2 | (byte3 b7-b6)
    uint32_t mjd_high = (bytes[1] >> 1) & 0x7F;
    uint32_t mjd_med = bytes[2];
    uint32_t mjd_low = (bytes[3] >> 6) & 0x03;
    m_date_time.mjd = (mjd_high << 10) | (mjd_med << 2) | mjd_low;

    // Byte 3: LSI (b5) + ConfInd (b4) + UTC (b3) + Hours high (b2-b0)
    m_date_time.lsi_flag = (bytes[3] & 0x20) != 0;
    m_date_time.conf_flag = (bytes[3] & 0x10) != 0;
    m_date_time.utc_flag = (bytes[3] & 0x08) != 0;
    uint8_t hours_high = bytes[3] & 0x07;

    // Byte 4: Minutes (b5-b0) + Hours low (b7-b6)
    uint8_t hours_low = (bytes[4] >> 6) & 0x03;
    m_date_time.hours = (hours_high << 2) | hours_low;
    m_date_time.minutes = bytes[4] & 0x3F;

    if (m_date_time.utc_flag && data.size() >= 7) {
        // Long form: Byte 5: milliseconds high (b7-b6) + seconds (b5-b0)
        m_date_time.seconds = bytes[5] & 0x3F;
        uint16_t ms_high = (bytes[5] >> 6) & 0x03;

        // Byte 6: milliseconds low
        m_date_time.milliseconds = static_cast<uint16_t>((ms_high << 8) | bytes[6]);
    } else {
        // Short form: no seconds/milliseconds field
        m_date_time.seconds = 0;
        m_date_time.milliseconds = 0;
    }

    // Convert to QDateTime
    m_currentDABTime = FIGParsingUtils::mjdToDateTime(
        m_date_time.mjd,
        m_date_time.hours,
        m_date_time.minutes,
        m_date_time.seconds
    );
    m_timeValid = true;
    m_date_time_valid = true;

    qInfo() << "FIG 0/10: MJD=" << m_date_time.mjd
            << "Time=" << QString("%1:%2:%3.%4")
                .arg(m_date_time.hours, 2, 10, QChar('0'))
                .arg(m_date_time.minutes, 2, 10, QChar('0'))
                .arg(m_date_time.seconds, 2, 10, QChar('0'))
                .arg(m_date_time.milliseconds, 3, 10, QChar('0'))
            << "UTC=" << m_date_time.utc_flag
            << "LSI=" << m_date_time.lsi_flag
            << "ConfInd=" << m_date_time.conf_flag
            << "DateTime=" << m_currentDABTime.toString(Qt::ISODate);

    emit timeInfoUpdated(m_currentDABTime);

    m_validFIGsCount++;
    m_totalFIGsProcessed++;
}


// ============================================================================
// FIG 0/13 - User Application Information
// ETSI EN 300 401 Section 6.3.6 (Phase 2A Week 6)
// ============================================================================

/**
 * Process FIG Type 0 Extension 13: User Application Information
 * 
 * Reverse-engineered from ODR-DabMux FIG0_13.cpp
 * Indicates user applications carried by service components (MOT Slideshow, EPG, etc.)
 * 
 * Structure per service component:
 * - Service ID: 16-bit (P/D=0) or 32-bit (P/D=1)
 * - SCIdS: 4-bit service component identifier
 * - No: 4-bit number of user applications
 * For each user application:
 *   - User app type: 11-bit (typeHigh 8 bits + typeLow 3 bits)
 *   - User app data length: 5-bit
 *   - User app data: variable length (application-specific)
 */
void AdvancedFIGAnalyser::processFIG0_13(const QByteArray& data)
{
    if (data.size() < 4) {  // Minimum: header(1) + SId(2) + SCIdS+No(1)
        m_lastError = QString("FIG 0/13: Insufficient data (need at least 4 bytes, got %1)").arg(data.size());
        qWarning() << m_lastError;
        return;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());
    
    // Extract P/D flag from the FIG 0 extension byte (bit 5, etisnoop
    // fig0_common_t::pd()); the old code read bit 2, which is an rfu bit.
    bool pd_flag = (bytes[0] & 0x20) != 0;
    int offset = 1;  // Skip extension/header byte

    int applications_found = 0;

    while (offset + 3 <= data.size()) {
        FIG0_13_UserApp app;

        // Parse Service ID (16-bit or 32-bit based on P/D flag)
        if (pd_flag) {
            // Data service: 32-bit SId
            if (offset + 4 > data.size()) break;
            app.service_id = (static_cast<uint32_t>(bytes[offset]) << 24) |
                            (static_cast<uint32_t>(bytes[offset + 1]) << 16) |
                            (static_cast<uint32_t>(bytes[offset + 2]) << 8) |
                            bytes[offset + 3];
            offset += 4;
        } else {
            // Programme service: 16-bit SId
            if (offset + 2 > data.size()) break;
            app.service_id = (static_cast<uint32_t>(bytes[offset]) << 8) | bytes[offset + 1];
            offset += 2;
        }

        // Parse SCIdS (4 bits, high nibble) and No (4 bits, low nibble)
        if (offset >= data.size()) break;
        app.component_id = (bytes[offset] >> 4) & 0x0F;
        uint8_t num_apps = bytes[offset] & 0x0F;
        offset++;

        qDebug() << "FIG 0/13: Service ID=" << Qt::hex << app.service_id
                 << "SCIdS=" << app.component_id
                 << "NumApps=" << Qt::dec << num_apps;

        // Parse each user application
        for (uint8_t i = 0; i < num_apps && offset + 2 <= data.size(); ++i) {
            // Byte 0: User app type high byte (bits 10-3 of 11-bit type)
            uint8_t type_high = bytes[offset];

            // Byte 1: Length(5 bits, bits 7-3) + Type low(3 bits, bits 2-0)
            uint8_t length = (bytes[offset + 1] >> 3) & 0x1F;
            uint8_t type_low = bytes[offset + 1] & 0x07;

            // Reconstruct 11-bit user application type
            app.user_app_type = (static_cast<uint16_t>(type_high) << 3) | type_low;

            offset += 2;

            // Read user application data
            if (offset + length > data.size()) {
                m_lastError = "FIG 0/13: User app data length exceeds buffer";
                qWarning() << m_lastError;
                return;  // Exit entire function - nested loop requires early return
            }

            app.user_app_data.clear();
            for (uint8_t j = 0; j < length; ++j) {
                app.user_app_data.push_back(bytes[offset + j]);
            }
            offset += length;

            // Log application type
            QString app_type_name;
            switch (app.user_app_type) {
                case USER_APP_SPI: app_type_name = "SPI"; break;
                case USER_APP_MOT_SLIDESHOW: app_type_name = "MOT Slideshow"; break;
                case USER_APP_MOT_BWS: app_type_name = "MOT BWS"; break;
                case USER_APP_TPEG: app_type_name = "TPEG"; break;
                case USER_APP_EPG: app_type_name = "EPG"; break;
                case USER_APP_JOURNALINE: app_type_name = "Journaline"; break;
                default: app_type_name = QString("Type 0x%1").arg(app.user_app_type, 3, 16, QChar('0'));
            }

            qInfo() << "  User app" << i << ":" << app_type_name
                    << "(" << Qt::hex << Qt::showbase << app.user_app_type << Qt::dec << ")"
                    << "data length=" << length;

            m_user_applications.push_back(app);
            applications_found++;
        }
    }

    if (applications_found > 0) {
        qInfo() << "FIG 0/13: Parsed" << applications_found << "user applications";
        m_validFIGsCount++;
        m_totalFIGsProcessed++;
    }
}

// ============================================================================
// FIG 0/14 - FEC Sub-channel Organization
// ETSI EN 300 401 Section 6.2.2 (Phase 2A Week 6)
// ============================================================================

/**
 * Process FIG Type 0 Extension 14: FEC Sub-channel Organization
 * 
 * Reverse-engineered from ODR-DabMux FIG0_14.cpp
 * Defines Forward Error Correction (FEC) scheme for enhanced packet mode sub-channels
 * 
 * Structure (1 byte per sub-channel):
 * - SubChId: 6 bits (bits 7-2)
 * - FEC scheme: 2 bits (bits 1-0)
 */
void AdvancedFIGAnalyser::processFIG0_14(const QByteArray& data)
{
    if (data.size() < 2) {  // Minimum: header(1) + one subchannel(1)
        m_lastError = QString("FIG 0/14: Insufficient data (need at least 2 bytes, got %1)").arg(data.size());
        qWarning() << m_lastError;
        return;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());
    int offset = 1;  // Skip extension/header byte

    int fec_schemes_found = 0;

    while (offset < data.size()) {
        FIG0_14_FECScheme fec;

        // Byte format: SubChId(6 bits, bits 7-2) + FEC scheme(2 bits, bits 1-0)
        fec.subchannel_id = (bytes[offset] >> 2) & 0x3F;
        fec.fec_scheme = bytes[offset] & 0x03;

        QString fec_name;
        switch (fec.fec_scheme) {
            case FEC_SCHEME_NO_FEC: fec_name = "No FEC"; break;
            case FEC_SCHEME_RS: fec_name = "Reed-Solomon"; break;
            case FEC_SCHEME_RS_2: fec_name = "Reed-Solomon variant"; break;
            case FEC_SCHEME_RESERVED: fec_name = "Reserved"; break;
            default: fec_name = QString("Unknown(%1)").arg(fec.fec_scheme);
        }

        qDebug() << "FIG 0/14: SubChId=" << fec.subchannel_id
                 << "FEC scheme=" << fec_name;

        m_fec_schemes[fec.subchannel_id] = fec;
        fec_schemes_found++;

        offset++;
    }

    if (fec_schemes_found > 0) {
        qInfo() << "FIG 0/14: Parsed" << fec_schemes_found << "FEC scheme definitions";
        m_validFIGsCount++;
        m_totalFIGsProcessed++;
    }
}

// ============================================================================
// FIG 0/17 - Programme Type
// ETSI EN 300 401 Section 8.1.5 (Phase 2A Week 6)
// ============================================================================

/**
 * Process FIG Type 0 Extension 17: Programme Type
 * 
 * Reverse-engineered from ODR-DabMux FIG0_17.cpp
 * Classifies programmes by genre for EPG and service filtering
 * 
 * Structure (4 bytes per service):
 * - Bytes 0-1: SId (16-bit, programme services only)
 * - Byte 2: rfa2_high(4) + rfu1(2) + rfa1(1) + SD(1)
 * - Byte 3: IntCode(5) + rfu2(1) + rfa2_low(2)
 */
void AdvancedFIGAnalyser::processFIG0_17(const QByteArray& data)
{
    if (data.size() < 5) {  // Minimum: header(1) + one service(4)
        m_lastError = QString("FIG 0/17: Insufficient data (need at least 5 bytes, got %1)").arg(data.size());
        qWarning() << m_lastError;
        return;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());
    int offset = 1;  // Skip extension/header byte

    int programme_types_found = 0;

    while (offset + 4 <= data.size()) {
        FIG0_17_ProgrammeType pty;

        // Bytes 0-1: Service ID (16-bit, only for programme services)
        pty.service_id = (static_cast<uint16_t>(bytes[offset]) << 8) | bytes[offset + 1];

        // Byte 2: SD flag (bit 0) - Static/Dynamic PTy flag
        pty.is_static = (bytes[offset + 2] & 0x01) == 0;  // SD=0 means static

        // Byte 3: IntCode (5 bits, bits 7-3) - Programme Type code
        pty.programme_type = (bytes[offset + 3] >> 3) & 0x1F;

        qInfo() << "FIG 0/17: Service ID=" << Qt::hex << Qt::showbase << pty.service_id
                << "PTy=" << Qt::dec << pty.programme_type
                << "(" << FIGParsingUtils::programmeTypeToString(pty.programme_type) << ")"
                << "SD=" << (pty.is_static ? "static" : "dynamic");

        m_programme_type_info[pty.service_id] = pty;

        // Update service in m_services map
        if (m_services.count(pty.service_id)) {
            m_services[pty.service_id].programme_type = pty.programme_type;
        }

        // Update legacy programme types map
        m_programmeTypes[pty.service_id] = pty.programme_type;

        emit programmeTypeUpdated(pty.service_id, pty.programme_type);

        programme_types_found++;
        offset += 4;
    }

    if (programme_types_found > 0) {
        qInfo() << "FIG 0/17: Parsed" << programme_types_found << "programme type definitions";
        bumpServiceRevision();  // service PTy changed
        m_validFIGsCount++;
        m_totalFIGsProcessed++;
    }
}

// ============================================================================
// FIG 0/18 - Announcement Support
// ETSI EN 300 401 Section 8.1.6.1 (Phase 2A Week 6)
// ============================================================================

/**
 * Process FIG Type 0 Extension 18: Announcement Support
 * 
 * Reverse-engineered from ODR-DabMux FIG0_18.cpp
 * Indicates which announcement types a service can carry (including EWS)
 * 
 * Structure per service:
 * - Bytes 0-1: SId (16-bit)
 * - Bytes 2-3: ASu flags (16-bit announcement support flags)
 * - Byte 4: NumClusters(5 bits, bits 7-3) + Rfa(3 bits)
 * - Followed by cluster IDs (1 byte each)
 * 
 * Note: Per ETSI EN 300 401, bit 0 of ASu (Alarm) and cluster 0xFF are handled specially
 */
void AdvancedFIGAnalyser::processFIG0_18(const QByteArray& data)
{
    if (data.size() < 6) {  // Minimum: header(1) + SId(2) + ASu(2) + NumClusters(1)
        m_lastError = QString("FIG 0/18: Insufficient data (need at least 6 bytes, got %1)").arg(data.size());
        qWarning() << m_lastError;
        return;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());
    int offset = 1;  // Skip extension/header byte

    int announcements_found = 0;

    while (offset + 5 <= data.size()) {
        FIG0_18_AnnouncementSupport support;

        // Bytes 0-1: Service ID (16-bit)
        support.service_id = (static_cast<uint16_t>(bytes[offset]) << 8) | bytes[offset + 1];

        // Bytes 2-3: ASu flags (16-bit)
        // Note: Per ETSI EN 300 401 Section 8.1.6, bit 0 must be 0 (Alarm declared via FIG 0/0)
        support.announcement_support_flags = (static_cast<uint16_t>(bytes[offset + 2]) << 8) | bytes[offset + 3];

        // Byte 4: NumClusters (5 bits, bits 7-3)
        uint8_t num_clusters = (bytes[offset + 4] >> 3) & 0x1F;

        offset += 5;

        // Read cluster IDs
        support.cluster_ids.clear();
        for (uint8_t i = 0; i < num_clusters && offset < data.size(); ++i) {
            uint8_t cluster_id = bytes[offset];
            // Per ETSI EN 300 401, cluster 0xFF must not be sent in FIG 0/18
            if (cluster_id != 0xFF) {
                support.cluster_ids.push_back(cluster_id);
            }
            offset++;
        }

        qInfo() << "FIG 0/18: Service ID=" << Qt::hex << Qt::showbase << support.service_id
                << "ASu flags=" << Qt::hex << support.announcement_support_flags
                << "Clusters=" << Qt::dec << support.cluster_ids.size();

        // Log supported announcement types
        if (support.announcement_support_flags & ANNOUNCEMENT_ALARM) {
            qInfo() << "  -> Supports EMERGENCY ALARMS (EWS capable)";
        }
        if (support.announcement_support_flags & ANNOUNCEMENT_ROAD_TRAFFIC) {
            qInfo() << "  -> Supports Road Traffic Flash";
        }
        if (support.announcement_support_flags & ANNOUNCEMENT_NEWS_FLASH) {
            qInfo() << "  -> Supports News Flash";
        }

        m_announcement_support.push_back(support);
        announcements_found++;
    }

    if (announcements_found > 0) {
        qInfo() << "FIG 0/18: Parsed" << announcements_found << "announcement support definitions";
        m_validFIGsCount++;
        m_totalFIGsProcessed++;
    }
}

// ============================================================================
// FIG 0/19 - Announcement Switching (EWS CRITICAL)
// ETSI EN 300 401 Section 8.1.6.2 (Phase 2A Week 6)
// ============================================================================

/**
 * Process FIG Type 0 Extension 19: Announcement Switching
 * 
 * **CRITICAL FOR THAI NBTC EMERGENCY WARNING SYSTEM (EWS)**
 * 
 * Reverse-engineered from ODR-DabMux FIG0_19.cpp
 * Real-time announcement activation/deactivation including EMERGENCY ALARMS
 * 
 * Structure per cluster (4 bytes minimum):
 * - Byte 0: ClusterId (8 bits)
 * - Bytes 1-2: ASw flags (16-bit, active announcement types RIGHT NOW)
 * - Byte 3: SubChId(6 bits, bits 5-0) + RegionFlag(1 bit, bit 6) + NewFlag(1 bit, bit 7)
 * - [Optional] Byte 4: regionId_lower (6 bits) if RegionFlag=1
 * 
 * **EWS CRITICAL**: When ASw bit 0 (ANNOUNCEMENT_ALARM) is set, EMERGENCY ALARM IS ACTIVE!
 */
void AdvancedFIGAnalyser::processFIG0_19(const QByteArray& data)
{
    if (data.size() < 5) {  // Minimum: header(1) + one cluster(4)
        m_lastError = QString("FIG 0/19: Insufficient data (need at least 5 bytes, got %1)").arg(data.size());
        qWarning() << m_lastError;
        return;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());
    int offset = 1;  // Skip extension/header byte

    // Clear previous active announcements (real-time status)
    m_active_announcements.clear();

    int announcements_active = 0;

    while (offset + 4 <= data.size()) {
        FIG0_19_AnnouncementSwitching switching;

        // Byte 0: Cluster ID
        switching.cluster_id = bytes[offset];

        // Bytes 1-2: ASw flags (16-bit, ACTIVE announcement types)
        switching.announcement_flags = (static_cast<uint16_t>(bytes[offset + 1]) << 8) | bytes[offset + 2];

        // Byte 3: NewFlag(1) + RegionFlag(1) + SubChId(6)
        switching.new_flag = (bytes[offset + 3] & 0x80) != 0;
        switching.region_flag = (bytes[offset + 3] & 0x40) != 0;
        switching.subchannel_id = bytes[offset + 3] & 0x3F;

        offset += 4;

        // Read region ID if present (ETSI EN 300 401 Section 8.1.6.2)
        if (switching.region_flag) {
            if (offset >= data.size()) {
                m_lastError = "FIG 0/19: Missing region ID byte when RegionFlag=1";
                qWarning() << m_lastError;
                break;  // Exit loop - incomplete announcement data
            }
            switching.region_id = bytes[offset] & 0x3F;
            offset++;
        } else {
            switching.region_id = 0;
        }

        qInfo() << "FIG 0/19: Cluster ID=" << switching.cluster_id
                << "ASw flags=" << Qt::hex << Qt::showbase << switching.announcement_flags
                << "SubChId=" << Qt::dec << switching.subchannel_id
                << "New=" << switching.new_flag
                << "Region=" << (switching.region_flag ? QString::number(switching.region_id) : "none");

        // **EWS CRITICAL DETECTION**
        if (switching.announcement_flags & ANNOUNCEMENT_ALARM) {
            qCritical() << "*** EMERGENCY ALARM ACTIVE *** Cluster" << switching.cluster_id
                        << "SubChannel" << switching.subchannel_id
                        << (switching.region_flag ? QString("Region %1").arg(switching.region_id) : "All regions");

            // Emit EWS signal for UI and logging
            emit emergencyAlarmActivated(switching);
        }

        // Detect other announcement types
        if (switching.announcement_flags & ANNOUNCEMENT_ROAD_TRAFFIC) {
            qWarning() << "Road Traffic Flash ACTIVE on cluster" << switching.cluster_id;
        }
        if (switching.announcement_flags & ANNOUNCEMENT_NEWS_FLASH) {
            qInfo() << "News Flash ACTIVE on cluster" << switching.cluster_id;
        }

        // New announcement detection for immediate UI updates
        if (switching.new_flag) {
            qInfo() << "NEW announcement started on cluster" << switching.cluster_id;
            emit announcementActivated(switching.cluster_id, switching.announcement_flags);
        }

        // Deactivation detection (ASw = 0 means announcement ended)
        if (switching.announcement_flags == 0) {
            qInfo() << "Announcement DEACTIVATED on cluster" << switching.cluster_id;
            emit announcementDeactivated(switching.cluster_id);
        }

        m_active_announcements.push_back(switching);
        announcements_active++;
    }

    if (announcements_active > 0) {
        qInfo() << "FIG 0/19: Parsed" << announcements_active << "announcement switching states";
        m_validFIGsCount++;
        m_totalFIGsProcessed++;
    }
}



void AdvancedFIGAnalyser::processFIG1_5(const QByteArray& data)
{
    // ETSI EN 300 401 Section 5.2.2.6: FIG 1/5 Data Service Label
    // Data (starts at the ext byte, etisnoop fig1.cpp case 5):
    //   data[0] = charset(4) | O/E(1) | extension(3) = 5
    //   per entry: [SId 32-bit BE (4)] [Label (16)] [CharFlag (2)]
    // Minimum size: 1 + 22 = 23 bytes for one data service label

    if (data.size() < 23) {
        m_lastError = QString("FIG 1/5: Insufficient data (need at least 23 bytes, got %1)").arg(data.size());
        qWarning() << m_lastError;
        return;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());
    int offset = 1;  // Skip the charset/ext byte (data starts at the ext byte)

    int labels_found = 0;

    // Parse all data service labels in the FIG
    while (offset + 22 <= data.size()) {
        FIG1_5_DataServiceLabel label_info;

        // Parse 32-bit service ID (big-endian)
        label_info.service_id = (static_cast<uint32_t>(bytes[offset]) << 24) |
                                (static_cast<uint32_t>(bytes[offset + 1]) << 16) |
                                (static_cast<uint32_t>(bytes[offset + 2]) << 8) |
                                bytes[offset + 3];
        offset += 4;

        // Extract 16-character label
        std::memcpy(label_info.label, &bytes[offset], 16);
        label_info.label[16] = '\0';  // Null-terminate
        offset += 16;

        // Parse character flag field (16 bits, big-endian)
        label_info.character_flag = (static_cast<uint16_t>(bytes[offset]) << 8) |
                                    bytes[offset + 1];
        offset += 2;

        // Store in data service labels map
        m_data_service_labels[label_info.service_id] = label_info;
        labels_found++;

        qInfo() << "FIG 1/5: Data service 0x" << Qt::hex << label_info.service_id
                << "label:" << label_info.label
                << "char_flag: 0x" << label_info.character_flag;
    }

    if (labels_found > 0) {
        bumpServiceRevision();  // data-service label changed
        m_validFIGsCount++;
        m_totalFIGsProcessed++;
        qDebug() << "FIG 1/5: Processed" << labels_found << "data service label(s)";
    } else {
        m_lastError = "FIG 1/5: No valid labels found";
        qWarning() << m_lastError;
    }
}


void AdvancedFIGAnalyser::processFIG2(const QByteArray& figData)
{
    // ETSI EN 300 401 Section 8.1.13.1: FIG Type 2 Extended Labels
    
    if (figData.size() < 1) {
        m_lastError = "FIG 2: Insufficient data";
        return;
    }

    // FIG 2 extended-label extension byte has the same charset/OE/ext layout
    // as FIG 1 (EN 300 401 clause 8.1.13.1): extension = bits 2-0.
    uint8_t extension = static_cast<uint8_t>(figData[0]) & 0x07;

    // Check if FIG type is enabled
    QString figKey = QString("2.%1").arg(extension);
    if (!m_enabledFIGTypes.value(figKey, false)) {
        m_lastError = QString("FIG 2/%1: Disabled").arg(extension);
        return;
    }

    // Dispatch to appropriate processor
    switch (extension) {
        case 0:
            processFIG2_0(figData);
            break;
        case 1:
            processFIG2_1(figData);
            break;
        case 4:
            processFIG2_4(figData);
            break;
        case 5:
            processFIG2_5(figData);
            break;
        default:
            qDebug() << "Unsupported FIG 2 extension:" << extension;
            m_lastError = QString("FIG 2/%1: Unsupported extension").arg(extension);
            break;
    }
}

/**
 * Process FIG Type 2 Extension 0: Ensemble Extended Label
 * ETSI EN 300 401 Section 8.1.13.1
 *
 * Provides UTF-8 extended labels for ensemble (up to 128 bytes in 8 segments)
 * Structure: EId (16 bits) + Rfa (4 bits) + Toggle (1 bit) + SegmentIndex (3 bits) + Text (variable)
 */
void AdvancedFIGAnalyser::processFIG2_0(const QByteArray& data)
{
    // ETSI EN 300 401 Section 8.1.13.1: FIG 2/0 Ensemble Extended Label
    // Minimum: FIG header (1) + EId (2) + flags (1) = 4 bytes
    
    if (data.size() < 4) {
        m_lastError = QString("FIG 2/0: Insufficient data (need at least 4 bytes, got %1)").arg(data.size());
        qWarning() << m_lastError;
        return;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());
    int offset = 1;  // Skip FIG header

    // Parse Ensemble ID (16 bits, big-endian)
    uint16_t ensemble_id = (static_cast<uint16_t>(bytes[offset]) << 8) |
                           bytes[offset + 1];
    offset += 2;

    // Parse flags byte: Rfa (4 bits) + Toggle (1 bit) + SegmentIndex (3 bits)
    uint8_t toggle_flag = (bytes[offset] & 0x08) >> 3;  // Bit 3
    uint8_t segment_index = bytes[offset] & 0x07;       // Bits 2-0
    offset += 1;

    // Remaining bytes are UTF-8 text segment
    int text_length = data.size() - offset;
    std::vector<uint8_t> text_segment;
    for (int i = 0; i < text_length; ++i) {
        text_segment.push_back(bytes[offset + i]);
    }

    // Store segment
    ExtendedLabelSegment segment;
    segment.toggle_flag = toggle_flag;
    segment.segment_index = segment_index;
    segment.text_segment = text_segment;

    m_ensemble_extended_labels[ensemble_id].ensemble_id = ensemble_id;
    m_ensemble_extended_labels[ensemble_id].segments[segment_index] = segment;

    qInfo() << "FIG 2/0: Ensemble 0x" << Qt::hex << ensemble_id
            << "segment" << segment_index
            << "toggle" << toggle_flag
            << "length" << text_length << "bytes";

    bumpServiceRevision();  // extended ensemble label changed
    m_validFIGsCount++;
    m_totalFIGsProcessed++;
}

/**
 * Process FIG Type 2 Extension 1: Service Extended Label
 * ETSI EN 300 401 Section 8.1.13.1
 *
 * Provides UTF-8 extended labels for services (programme or data)
 * Structure: SId (16 or 32 bits) + flags + Text (variable)
 */
void AdvancedFIGAnalyser::processFIG2_1(const QByteArray& data)
{
    // ETSI EN 300 401 Section 8.1.13.1: FIG 2/1 Service Extended Label
    
    if (data.size() < 1) {
        m_lastError = "FIG 2/1: Insufficient data";
        qWarning() << m_lastError;
        return;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());

    // FIG 2/1: data starts at the charset/OE/ext byte; the P/D flag is the
    // most significant bit of the Service Identifier in the second byte
    // (EN 300 401 clause 8.1.13.1).
    bool pd_flag = (bytes[1] & 0x80) != 0;
    int offset = 1;  // Skip the ext byte (data starts at the ext byte)

    // Service ID length depends on P/D flag
    int sid_length = pd_flag ? 4 : 2;  // 32-bit (data) or 16-bit (programme)

    // Check minimum size
    if (offset + sid_length + 1 > data.size()) {
        m_lastError = QString("FIG 2/1: Insufficient data for service ID and flags");
        qWarning() << m_lastError;
        return;
    }

    uint32_t service_id;

    // Parse Service ID (data[1..] = SId; P/D bit excluded from 16-bit keys)

    // Parse Service ID
    if (pd_flag) {
        // 32-bit data service ID (big-endian)
        service_id = (static_cast<uint32_t>(bytes[offset]) << 24) |
                     (static_cast<uint32_t>(bytes[offset + 1]) << 16) |
                     (static_cast<uint32_t>(bytes[offset + 2]) << 8) |
                     bytes[offset + 3];
    } else {
        // 16-bit programme service ID (big-endian)
        service_id = (static_cast<uint32_t>(bytes[offset]) << 8) |
                     bytes[offset + 1];
    }
    offset += sid_length;

    // Parse flags byte: Rfa (4 bits) + Toggle (1 bit) + SegmentIndex (3 bits)
    uint8_t toggle_flag = (bytes[offset] & 0x08) >> 3;  // Bit 3
    uint8_t segment_index = bytes[offset] & 0x07;       // Bits 2-0
    offset += 1;

    // Remaining bytes are UTF-8 text segment
    int text_length = data.size() - offset;
    std::vector<uint8_t> text_segment;
    for (int i = 0; i < text_length; ++i) {
        text_segment.push_back(bytes[offset + i]);
    }

    // Store segment
    ExtendedLabelSegment segment;
    segment.toggle_flag = toggle_flag;
    segment.segment_index = segment_index;
    segment.text_segment = text_segment;

    m_service_extended_labels[service_id].service_id = service_id;
    m_service_extended_labels[service_id].segments[segment_index] = segment;

    qInfo() << "FIG 2/1: Service 0x" << Qt::hex << service_id
            << (pd_flag ? "(data)" : "(programme)")
            << "segment" << segment_index
            << "toggle" << toggle_flag
            << "length" << text_length << "bytes";

    bumpServiceRevision();  // extended service label changed
    m_validFIGsCount++;
    m_totalFIGsProcessed++;
}

/**
 * Process FIG Type 2 Extension 4: Service Component Extended Label
 * ETSI EN 300 401 Section 8.1.13.1
 *
 * Provides UTF-8 extended labels for service components
 * Structure: SId (16 or 32 bits) + SCIdS (4 bits) + flags + Text (variable)
 */
void AdvancedFIGAnalyser::processFIG2_4(const QByteArray& data)
{
    // ETSI EN 300 401 Section 8.1.13.1: FIG 2/4 Component Extended Label
    
    if (data.size() < 1) {
        m_lastError = "FIG 2/4: Insufficient data";
        qWarning() << m_lastError;
        return;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());

    // FIG 2/4: data starts at the charset/OE/ext byte; the P/D flag is the
    // most significant bit of the Service Identifier in the second byte
    // (EN 300 401 clause 8.1.13.1).
    bool pd_flag = (bytes[1] & 0x80) != 0;
    int offset = 1;  // Skip the ext byte (data starts at the ext byte)

    // Service ID length depends on P/D flag
    int sid_length = pd_flag ? 4 : 2;

    // Check minimum size
    if (offset + sid_length + 1 > data.size()) {
        m_lastError = QString("FIG 2/4: Insufficient data for service ID and flags");
        qWarning() << m_lastError;
        return;
    }

    uint32_t service_id;

    // Parse Service ID
    if (pd_flag) {
        // 32-bit data service ID
        service_id = (static_cast<uint32_t>(bytes[offset]) << 24) |
                     (static_cast<uint32_t>(bytes[offset + 1]) << 16) |
                     (static_cast<uint32_t>(bytes[offset + 2]) << 8) |
                     bytes[offset + 3];
    } else {
        // 16-bit programme service ID
        service_id = (static_cast<uint32_t>(bytes[offset]) << 8) |
                     bytes[offset + 1];
    }
    offset += sid_length;

    // Parse flags byte: SCIdS (4 bits) + Rfa (1 bit) + Toggle (1 bit) + SegmentIndex (3 bits) - Wait, let me check ETSI
    // Actually per ETSI EN 300 401 Table 9: SCIdS (4 bits, bits 7-4) + Rfa (1 bit) + Toggle (1 bit) + SegmentIndex (3 bits)
    uint8_t component_id = (bytes[offset] >> 4) & 0x0F;  // Bits 7-4
    uint8_t toggle_flag = (bytes[offset] & 0x08) >> 3;    // Bit 3
    uint8_t segment_index = bytes[offset] & 0x07;         // Bits 2-0
    offset += 1;

    // Remaining bytes are UTF-8 text segment
    int text_length = data.size() - offset;
    std::vector<uint8_t> text_segment;
    for (int i = 0; i < text_length; ++i) {
        text_segment.push_back(bytes[offset + i]);
    }

    // Store segment using composite key: (service_id << 8) | component_id
    uint64_t key = (static_cast<uint64_t>(service_id) << 8) | component_id;

    ExtendedLabelSegment segment;
    segment.toggle_flag = toggle_flag;
    segment.segment_index = segment_index;
    segment.text_segment = text_segment;

    m_component_extended_labels[key].service_id = service_id;
    m_component_extended_labels[key].component_id = component_id;
    m_component_extended_labels[key].segments[segment_index] = segment;

    qInfo() << "FIG 2/4: Service 0x" << Qt::hex << service_id
            << "component" << component_id
            << "segment" << segment_index
            << "toggle" << toggle_flag
            << "length" << text_length << "bytes";

    bumpServiceRevision();  // extended component label changed
    m_validFIGsCount++;
    m_totalFIGsProcessed++;
}

/**
 * Process FIG Type 2 Extension 5: Data Service Extended Label
 * ETSI EN 300 401 Section 8.1.13.1
 *
 * Provides UTF-8 extended labels for data services (always 32-bit SId)
 * Semantically identical to FIG 2/1 with P/D=1
 */
void AdvancedFIGAnalyser::processFIG2_5(const QByteArray& data)
{
    // ETSI EN 300 401 Section 8.1.13.1: FIG 2/5 Data Service Extended Label
    // Always uses 32-bit service ID (data services)
    
    if (data.size() < 6) {  // Minimum: header (1) + SId (4) + flags (1)
        m_lastError = QString("FIG 2/5: Insufficient data (need at least 6 bytes, got %1)").arg(data.size());
        qWarning() << m_lastError;
        return;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());
    int offset = 1;  // Skip FIG header

    // Parse 32-bit service ID (always for data services)
    uint32_t service_id = (static_cast<uint32_t>(bytes[offset]) << 24) |
                          (static_cast<uint32_t>(bytes[offset + 1]) << 16) |
                          (static_cast<uint32_t>(bytes[offset + 2]) << 8) |
                          bytes[offset + 3];
    offset += 4;

    // Parse flags byte
    uint8_t toggle_flag = (bytes[offset] & 0x08) >> 3;  // Bit 3
    uint8_t segment_index = bytes[offset] & 0x07;       // Bits 2-0
    offset += 1;

    // Remaining bytes are UTF-8 text segment
    int text_length = data.size() - offset;
    std::vector<uint8_t> text_segment;
    for (int i = 0; i < text_length; ++i) {
        text_segment.push_back(bytes[offset + i]);
    }

    // Store segment (reuse FIG 2/1 storage since they're semantically identical)
    ExtendedLabelSegment segment;
    segment.toggle_flag = toggle_flag;
    segment.segment_index = segment_index;
    segment.text_segment = text_segment;

    m_service_extended_labels[service_id].service_id = service_id;
    m_service_extended_labels[service_id].segments[segment_index] = segment;

    qInfo() << "FIG 2/5: Data service 0x" << Qt::hex << service_id
            << "segment" << segment_index
            << "toggle" << toggle_flag
            << "length" << text_length << "bytes";

    bumpServiceRevision();  // extended data-service label changed
    m_validFIGsCount++;
    m_totalFIGsProcessed++;
}

// ============================================================================
// Phase 2A Week 7: Segment Reassembly Helper Methods
// ============================================================================

/**
 * Reassemble ensemble extended label from segments
 * CRITICAL: UTF-8 multi-byte character support for Thai language
 */
QString AdvancedFIGAnalyser::getEnsembleExtendedLabel(uint16_t ensemble_id)
{
    auto it = m_ensemble_extended_labels.find(ensemble_id);
    if (it == m_ensemble_extended_labels.end()) {
        return QString();  // No extended label found
    }

    auto& label_info = it->second;
    QByteArray full_label;

    // Concatenate segments 0-7 in order
    for (int i = 0; i < 8; ++i) {
        auto seg_it = label_info.segments.find(i);
        if (seg_it != label_info.segments.end()) {
            const auto& segment = seg_it->second;
            for (uint8_t byte : segment.text_segment) {
                full_label.append(byte);
            }
        }
    }

    // Convert UTF-8 to QString (supports Thai multi-byte characters)
    // Thai characters use 3 bytes per character in UTF-8
    // Example: "ภาษาไทย" = 27 bytes
    return QString::fromUtf8(full_label);
}

/**
 * Reassemble service extended label from segments
 * CRITICAL: UTF-8 multi-byte character support for Thai language
 */
QString AdvancedFIGAnalyser::getServiceExtendedLabel(uint32_t service_id)
{
    auto it = m_service_extended_labels.find(service_id);
    if (it == m_service_extended_labels.end()) {
        return QString();  // No extended label found
    }

    auto& label_info = it->second;
    QByteArray full_label;

    // Concatenate segments 0-7 in order
    for (int i = 0; i < 8; ++i) {
        auto seg_it = label_info.segments.find(i);
        if (seg_it != label_info.segments.end()) {
            const auto& segment = seg_it->second;
            for (uint8_t byte : segment.text_segment) {
                full_label.append(byte);
            }
        }
    }

    // Convert UTF-8 to QString (supports Thai multi-byte characters)
    return QString::fromUtf8(full_label);
}

/**
 * Reassemble component extended label from segments
 * CRITICAL: UTF-8 multi-byte character support for Thai language
 */
QString AdvancedFIGAnalyser::getComponentExtendedLabel(uint32_t service_id, uint8_t component_id)
{
    uint64_t key = (static_cast<uint64_t>(service_id) << 8) | component_id;
    
    auto it = m_component_extended_labels.find(key);
    if (it == m_component_extended_labels.end()) {
        return QString();  // No extended label found
    }

    auto& label_info = it->second;
    QByteArray full_label;

    // Concatenate segments 0-7 in order
    for (int i = 0; i < 8; ++i) {
        auto seg_it = label_info.segments.find(i);
        if (seg_it != label_info.segments.end()) {
            const auto& segment = seg_it->second;
            for (uint8_t byte : segment.text_segment) {
                full_label.append(byte);
            }
        }
    }

    // Convert UTF-8 to QString (supports Thai multi-byte characters)
    return QString::fromUtf8(full_label);
}

// ============================================================================
// Phase 2A Week 7: Data Accessor
// ============================================================================

/**
 * Get all data service labels (FIG 1/5)
 */
std::vector<FIG1_5_DataServiceLabel> AdvancedFIGAnalyser::getDataServiceLabels() const
{
    std::vector<FIG1_5_DataServiceLabel> labels;
    for (const auto& pair : m_data_service_labels) {
        labels.push_back(pair.second);
    }
    return labels;
}

// FIG Type 2 extended label map accessors (Phase 2C - GUI)
const std::map<uint16_t, FIG2_0_EnsembleExtLabel>& AdvancedFIGAnalyser::getEnsembleExtendedLabelsMap() const
{
    return m_ensemble_extended_labels;
}

const std::map<uint32_t, FIG2_1_ServiceExtLabel>& AdvancedFIGAnalyser::getServiceExtendedLabelsMap() const
{
    return m_service_extended_labels;
}

const std::map<uint64_t, FIG2_4_ComponentExtLabel>& AdvancedFIGAnalyser::getComponentExtendedLabelsMap() const
{
    return m_component_extended_labels;
}

// ============================================================================
// Control methods
// ============================================================================

void AdvancedFIGAnalyser::enableFIGType(uint8_t type, uint8_t extension, bool enabled)
{
    QString figKey = QString("%1.%2").arg(type).arg(extension);
    m_enabledFIGTypes[figKey] = enabled;
}

bool AdvancedFIGAnalyser::isFIGTypeEnabled(uint8_t type, uint8_t extension) const
{
    QString figKey = QString("%1.%2").arg(type).arg(extension);
    return m_enabledFIGTypes.value(figKey, false);
}

void AdvancedFIGAnalyser::reset()
{
    m_currentEnsemble = EnsembleInfo();
    m_discoveredServices.clear();
    m_serviceLabels.clear();
    m_componentLabels.clear();
    m_programmeTypes.clear();
    m_serviceLabelData.clear();
    m_componentLabelData.clear();
    m_ensembleLabel = DABLabel();
    m_sub_channels.clear();
    m_services.clear();
    m_packet_components.clear();
    m_component_languages.clear();
    m_service_links.clear();
    m_global_definitions.clear();
    m_user_applications.clear();
    m_fec_schemes.clear();
    m_data_service_labels.clear();
    m_ensemble_extended_labels.clear();
    m_service_extended_labels.clear();
    m_component_extended_labels.clear();
    m_programme_type_info.clear();
    m_announcement_support.clear();
    m_active_announcements.clear();
    m_totalFIGsProcessed = 0;
    m_validFIGsCount = 0;
    m_lastError.clear();
    m_ensembleValid = false;
    m_timeValid = false;
    m_date_time_valid = false;
    m_fib_crc_failures = 0;
    m_rawFicFallbackUsed = false;
    m_rawFicFallbackWarned = false;
    m_lastFicSize = 0;
    // FIC-XTractor collector state
    m_figInstances.clear();
    m_figTypeCounts.clear();
    m_figFrameNumber = 0;
    m_figFibIndex = -1;
    m_figFibCrcOk = true;
    m_figRecording = true;
    // Invalidate every derived cache: the state was cleared, so the revision
    // must advance (never reset to 0) to remain monotonic.
    bumpServiceRevision();
}

// ============================================================================
// Data accessors

QList<ServiceInfo> AdvancedFIGAnalyser::getDiscoveredServices() const
{
    // Convert m_services map to legacy QList<ServiceInfo> format
    QList<ServiceInfo> services;

    for (auto it = m_services.begin(); it != m_services.end(); ++it) {
        const DABService& dab_service = it->second;

        ServiceInfo info;
        info.serviceId = dab_service.service_id;
        info.label = dab_service.service_label;
        info.isAudio = (dab_service.service_type == 0);  // 0=audio, 1=data

        // Get subchannel ID from primary component
        if (!dab_service.components.empty()) {
            ServiceComponent primary = dab_service.getPrimaryComponent();
            info.subchannelId = primary.sub_channel_id;
        } else {
            info.subchannelId = 0;
        }

        services.append(info);
    }

    qDebug() << "getDiscoveredServices: Returning" << services.size()
             << "services from m_services map (total in map:" << m_services.size() << ")";

    return services;
}
// ============================================================================

QStringList AdvancedFIGAnalyser::getServiceLabels() const
{
    QStringList labels;
    for (auto it = m_serviceLabels.begin(); it != m_serviceLabels.end(); ++it) {
        labels.append(QString("SID 0x%1: %2")
                     .arg(it.key(), 8, 16, QChar('0'))
                     .arg(it.value()));
    }
    return labels;
}

QStringList AdvancedFIGAnalyser::getProgrammeInfo() const
{
    QStringList info;

    // Add time information if available
    if (m_timeValid) {
        info.append(QString("Time: %1").arg(FIGParsingUtils::formatDABTime(m_currentDABTime)));
    }

    // Add programme type information
    for (auto it = m_programmeTypes.begin(); it != m_programmeTypes.end(); ++it) {
        info.append(QString("SID 0x%1: %2")
                   .arg(it.key(), 8, 16, QChar('0'))
                   .arg(FIGParsingUtils::programmeTypeToString(it.value())));
    }

    return info;
}

// ============================================================================
// Helper methods with CRITICAL-006 bounds checking
// ============================================================================

uint16_t AdvancedFIGAnalyser::extractUInt16(const QByteArray& data, int offset)
{
    // CRITICAL-006: Bounds checking with warning
    if (offset < 0 || offset + 1 >= data.size()) {
        qWarning() << "extractUInt16: Invalid offset" << offset
                   << "for data size" << data.size();
        return 0;
    }
    return (static_cast<uint16_t>(static_cast<uint8_t>(data[offset])) << 8) |
           static_cast<uint16_t>(static_cast<uint8_t>(data[offset + 1]));
}

uint32_t AdvancedFIGAnalyser::extractUInt32(const QByteArray& data, int offset)
{
    // CRITICAL-006: Bounds checking with warning
    if (offset < 0 || offset + 3 >= data.size()) {
        qWarning() << "extractUInt32: Invalid offset" << offset
                   << "for data size" << data.size();
        return 0;
    }
    return (static_cast<uint32_t>(static_cast<uint8_t>(data[offset])) << 24) |
           (static_cast<uint32_t>(static_cast<uint8_t>(data[offset + 1])) << 16) |
           (static_cast<uint32_t>(static_cast<uint8_t>(data[offset + 2])) << 8) |
           static_cast<uint32_t>(static_cast<uint8_t>(data[offset + 3]));
}

// ============================================================================
// FIG Parsing Utilities Implementation
// ============================================================================

QString FIGParsingUtils::convertDABCharset(const QByteArray& data, int offset, int length)
{
    if (offset + length > data.size()) return QString();

    // Simple ASCII conversion for now - can be enhanced for full EBU character set
    QString result;
    for (int i = 0; i < length; ++i) {
        char c = data[offset + i];
        if (c >= 0x20 && c <= 0x7E) { // Printable ASCII
            result.append(c);
        } else if (c == 0x00) {
            break; // Null terminator
        }
    }

    return result.trimmed();
}

QString FIGParsingUtils::countryCodeToString(uint8_t countryCode)
{
    // ECC country codes per ETSI EN 300 401 Table 10
    switch (countryCode) {
        case 0x0E: return "TH"; // Thailand (correct code)
        case 0xE0: return "DE"; // Germany
        case 0xE1: return "GR"; // Greece
        case 0xE2: return "UK"; // United Kingdom
        case 0xE3: return "FR"; // France
        default: return QString("0x%1").arg(countryCode, 2, 16, QChar('0'));
    }
}

QString FIGParsingUtils::serviceTypeToString(uint8_t serviceType)
{
    switch (serviceType) {
        case 0x00: return "DAB Audio";
        case 0x01: return "DAB+ Audio";
        case 0x02: return "Data Service";
        case 0x03: return "Packet Data";
        case 0x18: return "DAB+ Enhanced";
        default: return QString("Type %1").arg(serviceType);
    }
}

bool FIGParsingUtils::isAudioService(uint8_t serviceType)
{
    return (serviceType == 0x00 || serviceType == 0x01 || serviceType == 0x18);
}

bool FIGParsingUtils::isDataService(uint8_t serviceType)
{
    return (serviceType == 0x02 || serviceType == 0x03);
}

QString FIGParsingUtils::protectionLevelToString(uint8_t protectionLevel)
{
    switch (protectionLevel) {
        case 0: return "UEP-1";
        case 1: return "UEP-2";
        case 2: return "UEP-3";
        case 3: return "UEP-4";
        default: return QString("UEP-%1").arg(protectionLevel);
    }
}

QString FIGParsingUtils::programmeTypeToString(uint8_t ptyCode)
{
    // International programme type codes
    static const char* ptyNames[] = {
        "No programme type", "News", "Current Affairs", "Information",
        "Sport", "Education", "Drama", "Culture", "Science", "Varied",
        "Pop Music", "Rock Music", "Easy Listening", "Light Classical",
        "Serious Classical", "Other Music", "Weather/Meteorology", "Finance/Business",
        "Children's programmes", "Social Affairs", "Religion", "Phone In",
        "Travel", "Leisure", "Jazz Music", "Country Music", "National Music",
        "Oldies Music", "Folk Music", "Documentary", "Alarm Test", "Alarm"
    };

    if (ptyCode < 32) {
        return QString(ptyNames[ptyCode]);
    } else {
        return QString("PTy %1").arg(ptyCode);
    }
}

QDateTime FIGParsingUtils::convertDABTime(uint32_t dabTime)
{
    // DAB time conversion - simplified implementation
    // Real implementation would handle Modified Julian Date properly
    return QDateTime::currentDateTime();
}

QString FIGParsingUtils::formatDABTime(const QDateTime& time)
{
    return time.toString("hh:mm:ss UTC");
}

// ============================================================================
// SubChannelInfo Implementation (Agent 3 - FIG Type 0)
// ============================================================================

uint16_t SubChannelInfo::getBitrate() const
{
    return FIGParsingUtils::calculateBitrate(short_form, table_index,
                                             sub_channel_size, protection_level);
}

QString SubChannelInfo::getProtectionLevel() const
{
    // short_form = UEP (table_switch + table_index), long form = EEP
    // (option A/B + protection level), per etisnoop fig0_1.cpp semantics.
    if (short_form) {
        return QString("UEP-%1/%2").arg(table_switch).arg(table_index);
    } else {
        return QString("EEP-%1-%2").arg(option == 0 ? "A" : "B").arg(protection_level + 1);
    }
}

QString SubChannelInfo::toString() const
{
    return QString("SubCh %1: Start=%2 CU, %3, Bitrate=%4 kbps")
        .arg(sub_channel_id)
        .arg(start_address)
        .arg(getProtectionLevel())
        .arg(getBitrate());
}

// ============================================================================
// ServiceComponent Implementation (Agent 3 - FIG Type 0)
// ============================================================================

bool ServiceComponent::isDabPlus() const
{
    // ASCTy = 0x3F (63) indicates DAB+
    return (service_component_type == 0x3F);
}

QString ServiceComponent::toString() const
{
    return QString("Component %1: SubCh=%2, TMId=%3, %4")
        .arg(component_id)
        .arg(sub_channel_id)
        .arg(transport_mode)
        .arg(is_primary ? "Primary" : "Secondary");
}

// ============================================================================
// DABService Implementation (Agent 3 - FIG Type 0)
// ============================================================================

ServiceComponent DABService::getPrimaryComponent() const
{
    for (const auto& comp : components) {
        if (comp.is_primary) {
            return comp;
        }
    }
    return components.empty() ? ServiceComponent() : components[0];
}

QString DABService::toString() const
{
    return QString("Service 0x%1: %2 (%3, %4 components)")
        .arg(service_id, 8, 16, QChar('0'))
        .arg(service_label)
        .arg(is_dab_plus ? "DAB+" : "DAB")
        .arg(components.size());
}

// ============================================================================
// FIG Type 0 Data Accessor Methods (Agent 3)
// ============================================================================

SubChannelInfo AdvancedFIGAnalyser::getSubChannelInfo(uint8_t sub_channel_id) const
{
    auto it = m_sub_channels.find(sub_channel_id);
    if (it != m_sub_channels.end()) {
        return it->second;
    }
    return SubChannelInfo();
}

std::vector<SubChannelInfo> AdvancedFIGAnalyser::getAllSubChannels() const
{
    std::vector<SubChannelInfo> result;
    for (const auto& pair : m_sub_channels) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<DABService> AdvancedFIGAnalyser::getDABServices() const
{
    std::vector<DABService> result;
    for (const auto& pair : m_services) {
        result.push_back(pair.second);
    }
    return result;
}

DABService AdvancedFIGAnalyser::getService(uint32_t service_id) const
{
    auto it = m_services.find(service_id);
    if (it != m_services.end()) {
        return it->second;
    }
    return DABService();
}

int AdvancedFIGAnalyser::getServiceCount() const
{
    return static_cast<int>(m_services.size());
}

int AdvancedFIGAnalyser::getSubChannelCount() const
{
    return static_cast<int>(m_sub_channels.size());
}

// ============================================================================
// Helper Methods (Agent 3 - Bit manipulation)
// ============================================================================

uint8_t AdvancedFIGAnalyser::extractUInt8(const QByteArray& data, int offset)
{
    // CRITICAL-006: Bounds checking with warning
    if (offset < 0 || offset >= data.size()) {
        qWarning() << "extractUInt8: Invalid offset" << offset
                   << "for data size" << data.size();
        return 0;
    }
    return static_cast<uint8_t>(data[offset]);
}

uint16_t AdvancedFIGAnalyser::extractBits(const QByteArray& data, int bit_offset, int bit_count)
{
    // ========================================================================
    // SECURITY FIX: Comprehensive input validation and overflow protection
    // Issue: ISSUE-001 - Integer Overflow in extractBits()
    // Date: 2025-10-27 | Agent: Agent 5
    // ========================================================================
    
    // VALIDATION 1: bit_count must be positive and fit in return type
    if (bit_count <= 0 || bit_count > 16) {
        qWarning() << "extractBits: Invalid bit_count" << bit_count << "(must be 1-16)";
        return 0;
    }
    
    // VALIDATION 2: Data buffer must not be empty
    if (data.isEmpty()) {
        qWarning() << "extractBits: Empty data buffer";
        return 0;
    }
    
    // VALIDATION 3: bit_offset must be non-negative
    if (bit_offset < 0) {
        qWarning() << "extractBits: Negative bit_offset" << bit_offset;
        return 0;
    }
    
    // VALIDATION 4-7: Overflow-safe bounds checking
    const size_t total_bits = static_cast<size_t>(data.size()) * 8;
    if (static_cast<size_t>(bit_offset) >= total_bits) {
        qWarning() << "extractBits: bit_offset exceeds buffer";
        return 0;
    }
    
    // CRITICAL: Cast to size_t BEFORE addition prevents signed overflow
    const size_t bit_end = static_cast<size_t>(bit_offset) + static_cast<size_t>(bit_count);
    if (bit_end > total_bits) {
        qWarning() << "extractBits: Bit range exceeds buffer bounds";
        return 0;
    }
    
    // ========================================================================
    // Original extraction logic (now validated as safe)
    // ========================================================================
    uint16_t result = 0;
    int byte_offset = bit_offset / 8;
    int bit_in_byte = bit_offset % 8;

    for (int i = 0; i < bit_count; ++i) {
        if (byte_offset >= data.size()) break;

        bool bit = isBitSet(static_cast<uint8_t>(data[byte_offset]), 7 - bit_in_byte);
        result = (result << 1) | (bit ? 1 : 0);

        bit_in_byte++;
        if (bit_in_byte >= 8) {
            bit_in_byte = 0;
            byte_offset++;
        }
    }

    return result;
}

// ============================================================================
// FIG Parsing Utilities - Bitrate Calculation (Agent 3)
// ============================================================================

uint16_t FIGParsingUtils::calculateBitrate(bool short_form, uint8_t table_index,
                                           uint16_t sub_channel_size, uint8_t protection_level)
{
    if (short_form) {
        // EEP (Equal Error Protection) bitrate table per ETSI EN 300 401
        // Simplified table - full ETSI table has 64 entries
        static const uint16_t eep_bitrate_table[] = {
            32, 32, 32, 32, 48, 48, 48, 48,      // 0-7
            56, 56, 56, 56, 64, 64, 64, 64,      // 8-15
            80, 80, 80, 80, 96, 96, 96, 96,      // 16-23
            112, 112, 112, 112, 128, 128, 128, 128, // 24-31
            160, 160, 160, 160, 192, 192, 192, 192, // 32-39
            224, 224, 224, 224, 256, 256, 256, 256, // 40-47
            320, 320, 320, 320, 384, 384, 384, 384, // 48-55
            // Extended entries omitted - approximation for higher indices
        };

        if (table_index < sizeof(eep_bitrate_table) / sizeof(eep_bitrate_table[0])) {
            return eep_bitrate_table[table_index];
        } else {
            // Fallback calculation for high indices
            return 32 + (table_index * 8);
        }
    } else {
        // UEP (Unequal Error Protection) bitrate calculation
        // Based on sub-channel size in CUs (Capacity Units)
        // 1 CU = 8 kbps * protection level factor
        // Simplified calculation per ETSI EN 300 401
        double protection_factor = 1.0;
        switch (protection_level) {
            case 0: protection_factor = 1.0; break;   // UEP-1 (strongest protection)
            case 1: protection_factor = 0.75; break;  // UEP-2
            case 2: protection_factor = 0.625; break; // UEP-3
            case 3: protection_factor = 0.5; break;   // UEP-4 (weakest protection)
        }

        // Approximate bitrate calculation: CU size * 8 kbps/CU * protection factor
        return static_cast<uint16_t>(sub_channel_size * 8 * protection_factor / 3);
    }
}

int FIGParsingUtils::protectionLevelToBitrate(uint8_t protectionLevel)
{
    // Legacy function - returns approximate bitrate
    switch (protectionLevel) {
        case 0: return 32;
        case 1: return 64;
        case 2: return 96;
        case 3: return 128;
        default: return 64;
    }

}
// ============================================================================
// FIG 0/3 - 0/10 Data Accessor Methods
// ============================================================================

std::vector<FIG0_3_ServiceComponent> AdvancedFIGAnalyser::getPacketModeComponents() const
{
    return m_packet_components;
}

std::vector<FIG0_5_Language> AdvancedFIGAnalyser::getComponentLanguages() const
{
    std::vector<FIG0_5_Language> result;
    for (const auto& pair : m_component_languages) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<FIG0_6_ServiceLink> AdvancedFIGAnalyser::getServiceLinks() const
{
    return m_service_links;
}

FIG0_7_ConfigInfo AdvancedFIGAnalyser::getConfigurationInfo() const
{
    return m_config_info;
}

std::vector<FIG0_8_GlobalDefinition> AdvancedFIGAnalyser::getGlobalDefinitions() const
{
    std::vector<FIG0_8_GlobalDefinition> result;
    for (const auto& pair : m_global_definitions) {
        result.push_back(pair.second);
    }
    return result;
}

FIG0_9_CountryLTO AdvancedFIGAnalyser::getCountryLTO() const
{
    return m_country_lto;
}

FIG0_10_DateTime AdvancedFIGAnalyser::getDateTime() const
{
    return m_date_time;
}

QString FIGParsingUtils::languageCodeToString(uint8_t languageCode)
{
    // ISO 639-2 language codes (subset)
    switch (languageCode) {
        case 0x00: return "Unknown";
        case 0x01: return "Albanian";
        case 0x02: return "Breton";
        case 0x03: return "Catalan";
        case 0x04: return "Croatian";
        case 0x05: return "Welsh";
        case 0x06: return "Czech";
        case 0x07: return "Danish";
        case 0x08: return "German";
        case 0x09: return "English";
        case 0x0A: return "Spanish";
        case 0x0B: return "Esperanto";
        case 0x0C: return "Estonian";
        case 0x0D: return "Basque";
        case 0x0E: return "Faroese";
        case 0x0F: return "French";
        case 0x10: return "Frisian";
        case 0x11: return "Irish";
        case 0x12: return "Gaelic";
        case 0x13: return "Galician";
        case 0x14: return "Icelandic";
        case 0x15: return "Italian";
        case 0x16: return "Lappish";
        case 0x17: return "Latin";
        case 0x18: return "Latvian";
        case 0x19: return "Luxembourgian";
        case 0x1A: return "Lithuanian";
        case 0x1B: return "Hungarian";
        case 0x1C: return "Maltese";
        case 0x1D: return "Dutch";
        case 0x1E: return "Norwegian";
        case 0x1F: return "Occitan";
        case 0x20: return "Polish";
        case 0x21: return "Portuguese";
        case 0x22: return "Romanian";
        case 0x23: return "Romansh";
        case 0x24: return "Serbian";
        case 0x25: return "Slovak";
        case 0x26: return "Slovene";
        case 0x27: return "Finnish";
        case 0x28: return "Swedish";
        case 0x29: return "Turkish";
        case 0x2A: return "Flemish";
        case 0x2B: return "Walloon";
        case 0x45: return "Thai";
        default: return QString("Lang 0x%1").arg(languageCode, 2, 16, QChar('0'));
    }
}

QString FIGParsingUtils::programmeTypeToDescription(uint8_t ptyCode)
{
    // Same as programmeTypeToString for now
    return programmeTypeToString(ptyCode);
}

QDateTime FIGParsingUtils::mjdToDateTime(uint32_t mjd, uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    // Modified Julian Date conversion
    // MJD 0 = November 17, 1858
    // MJD starts from JD 2400000.5
    
    // Convert MJD to Gregorian calendar
    int a = mjd + 2400001;
    int b = a + 1537;
    int c = (b - 122.1) / 365.25;
    int d = 365.25 * c;
    int e = (b - d) / 30.6001;
    
    int day = b - d - int(30.6001 * e);
    int month = e - 1 - 12 * (e / 14);
    int year = c - 4715 - ((7 + month) / 10);
    
    QDate date(year, month, day);
    QTime time(hours, minutes, seconds);
    
    return QDateTime(date, time, QTimeZone::utc());
}




// ============================================================================
// FIG 0/13 - User Application Information (Placeholder)
// ============================================================================



// ============================================================================
// FIG 0/13 - 0/19 Data Accessor Methods (Phase 2A Week 6)
// ============================================================================

std::vector<FIG0_13_UserApp> AdvancedFIGAnalyser::getUserApplications() const
{
    return m_user_applications;
}

std::vector<FIG0_14_FECScheme> AdvancedFIGAnalyser::getFECSchemes() const
{
    std::vector<FIG0_14_FECScheme> result;
    for (const auto& pair : m_fec_schemes) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<FIG0_17_ProgrammeType> AdvancedFIGAnalyser::getProgrammeTypes() const
{
    std::vector<FIG0_17_ProgrammeType> result;
    for (const auto& pair : m_programme_type_info) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<FIG0_18_AnnouncementSupport> AdvancedFIGAnalyser::getAnnouncementSupport() const
{
    return m_announcement_support;
}

std::vector<FIG0_19_AnnouncementSwitching> AdvancedFIGAnalyser::getActiveAnnouncements() const
{
    return m_active_announcements;
}
