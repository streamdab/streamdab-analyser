/**
 * @file yaml_output_generator.hpp
 * @brief ETISnoop-Compatible YAML Output Generator for StreamDAB Analyser
 *
 * Generates YAML formatted output compatible with ETISnoop for comprehensive
 * DAB/DAB+ stream analysis results. Supports Thai UTF-8 encoding and follows
 * ETSI EN 300 401 naming conventions.
 *
 * @author Agent 35 - YAML Output Generator
 * @date 2025-11-05
 * @copyright StreamDAB Analyser Project - PDCA Week 8
 *
 * ENHANCED (2025-11-08): Added etisnoop-compatible detailed output
 * - Ensemble labels from FIG 1/0
 * - Service labels from FIG 1/1
 * - Component labels from FIG 1/4
 * - EEP protection level decoding (1-A to 5-B)
 * - Service-to-subchannel mapping
 * - Extended label support (FIG 2/x)
 *
 * VERBOSE MODE (2025-11-08): Added frame-by-frame detailed analysis
 * - Per-frame ETI header extraction
 * - Complete FIC/FIG decoding for each frame
 * - STC field details for all subchannels
 * - MSC stream data preview
 * - MORE detailed than etisnoop
 */

#ifndef YAML_OUTPUT_GENERATOR_HPP
#define YAML_OUTPUT_GENERATOR_HPP

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QMap>
#include <QList>
#include <QFile>
#include <QTextStream>
#include "../core/eti_types.hpp"
#include "../core/product_version.hpp"
#include "../core/fig_parser.hpp"
#include "../core/analyser_settings.hpp"

namespace cli {

/**
 * @brief One observed FIG type/extension in the FIG inventory (P1).
 *
 * Aggregated over all processed frames: how many instances of this FIG were
 * seen, in which frame range, and how many of those instances were carried
 * by a FIB whose CRC failed. Frames are 1-based.
 */
struct FIGInventoryEntry {
    uint8_t fig_type{0};
    uint8_t extension{0};
    quint64 count{0};
    quint64 first_frame{0};
    quint64 last_frame{0};
    quint64 crc_failures{0};
};

/**
 * @brief Aggregated FIC/FIB health counters (P1).
 *
 * fib_crc_failures: FIBs whose CRC-16 (CCITT-FALSE, complemented) mismatched
 *   in the strict FIB walk. When the auto-fallback raw re-walk produced the
 *   blocks (CRC-less dump heuristic), the strict walk failures are retained
 *   (every FIB failed) — the raw re-walk only prevents the FIGs from being
 *   lost, it does not clear the failure count.
 * total_fibs: FIBs walked (3/4 per frame on structured FICs; 1 per legacy
 *   32-byte raw field). Raw-mode decodes walk no FIBs.
 * erroneous_figs: FIG headers skipped for length/padding anomalies.
 * reconfiguration_count: frames whose MCI state (ensemble id / service id
 *   set / subchannel set) changed vs the previous frame.
 */
struct FICHealth {
    quint64 fib_crc_failures{0};
    quint64 total_fibs{0};
    quint64 erroneous_figs{0};
    quint64 reconfiguration_count{0};
};

/**
 * @brief Per-frame detailed analysis data
 */
struct FrameAnalysisData {
    uint64_t frame_number{0};
    QDateTime timestamp;

    // ETI Header (LIDATA field)
    struct ETIHeader {
        uint8_t fct{0};          // Frame Count
        uint8_t ficf{0};         // FIC Flag
        uint8_t nst{0};          // Number of Sub-channels
        uint8_t mid{0};          // Mode Identity
        uint8_t fp{0};           // Frame Phase
        uint16_t fl{0};          // Frame Length
        uint32_t tist{0};        // Time Stamp (raw 32-bit; tail TIST for ETI-NI)
        double tist_ms{0.0};     // Time Stamp in ms (etisnoop parity: (TIST & 0xFFFFFF) / 16384)
        QString format;          // "ETI-LI" or "ETI-NI"
    } header;

    // STC fields (for ETI-NI) - one per subchannel
    struct STCField {
        uint8_t scid{0};         // Sub-Channel ID
        uint16_t sad{0};         // Sub-channel Start Address
        uint8_t tpl{0};          // Sub-channel TPL (Table/Protection/Length)
        uint16_t stl{0};         // Sub-channel STL (Stream Length)

        // Decoded protection info
        QString protection_form; // "UEP" or "EEP"
        QString protection_level;// "3-A", "4-B", etc.
        uint16_t size_cu{0};     // Size in CUs
        uint32_t bitrate_kbps{0};// Calculated bitrate

        // Backlog item 8: protection table name ("EEP-A", "UEP-3"...) and the
        // table CU size (EEP-A 16/8/6/4, EEP-B 27/21/18/15, UEP per Table 6.2).
        QString protection_table;
        uint16_t cu_size{0};
    };
    QList<STCField> stc_fields;

    // FIC data
    struct FICData {
        uint8_t fib_count{0};    // Number of FIBs (0-3)
        struct FIB {
            uint8_t fib_number{0};
            bool crc_ok{false};
            QList<eti::FigBlock> fig_blocks;
        };
        QList<FIB> fibs;
    } fic;

    // MSC stream data (optional preview)
    struct MSCData {
        uint8_t subchannel_id{0};
        uint32_t length_bytes{0};
        QByteArray data_preview;  // First 32 bytes
    };
    QList<MSCData> msc_streams;

    // Frame validation
    bool crc_ok{true};
    QStringList errors;
};

/**
 * @brief YAML Output Generator for StreamDAB Analyser
 *
 * Generates ETISnoop-compatible YAML output containing:
 * - ETI stream metadata (format, frames, errors, timing)
 * - Ensemble information (ID, label, country, flags)
 * - Service listings (ID, label, type, components)
 * - Sub-channel organization (address, size, protection)
 * - FIG statistics (counts by type)
 * - ETSI compliance status
 * - Performance metrics
 *
 * VERBOSE MODE: Frame-by-frame detailed analysis with complete FIG decoding
 *
 * Output format follows YAML 1.2 specification with:
 * - 2-space indentation
 * - Proper string escaping
 * - ISO 8601 timestamps
 * - Hexadecimal 0xNNNN notation
 * - Thai UTF-8 preservation
 */
class YAMLOutputGenerator {
public:
    /**
     * @brief Construct YAML output generator
     */
    YAMLOutputGenerator();

    /**
     * @brief Destructor
     */
    ~YAMLOutputGenerator() = default;

    // ========================================================================
    // Data Setters
    // ========================================================================

    /**
     * @brief Set input file name
     * @param filename Path to input ETI file
     */
    void setInputFile(const QString& filename);

    /**
     * @brief Set total processing time
     * @param ms Processing duration in milliseconds
     */
    void setProcessingTime(qint64 ms);

    /**
     * @brief Set ensemble information
     * @param ensemble Ensemble structure from ETI processing
     */
    void setEnsembleInfo(const eti::Ensemble& ensemble);

    /**
     * @brief Add service to output
     * @param service DAB service structure
     */
    void addService(const eti::DabService& service);

    /**
     * @brief Add sub-channel to output
     * @param subchannel Sub-channel information structure
     */
    void addSubchannel(const eti::fig::Fig01SubchannelInfo& subchannel);

    /**
     * @brief Set FIG type statistics
     * @param fig_counts Map of FIG type names to counts (e.g., "0/0" -> 4200)
     */
    void setFIGStatistics(const QMap<QString, int>& fig_counts);

    /**
     * @brief Set ETSI compliance status
     * @param compliant Overall compliance status
     * @param warnings List of compliance warning messages
     */
    void setETSICompliance(bool compliant, const QStringList& warnings);

    /**
     * @brief Set the short ETSI-compliance verdict reason (T39).
     *
     * The counter summary that drove the verdict (FIB CRC failures, erroneous
     * FIGs, FIBs walked, ensembles/services, transport error frames). Emitted
     * as the `reason:` field of the `etsi_compliance:` YAML section so a NO is
     * explainable. Optional; an empty string omits the field.
     *
     * @param reason Human-readable verdict reason
     */
    void setETSIComplianceReason(const QString& reason);

    /**
     * @brief Set performance metrics
     * @param processing_time Total processing time in milliseconds
     * @param fps Frames processed per second
     */
    void setPerformanceMetrics(qint64 processing_time, double fps);

    /**
     * @brief Set ETI stream information
     * @param total_frames Total frames in stream
     * @param valid_frames Number of valid frames
     * @param error_frames Number of error frames
     * @param crc_errors Number of CRC errors detected
     */
    void setETIStreamInfo(int total_frames, int valid_frames, int error_frames, int crc_errors);

    /**
     * @brief Set the user-selected analyser decode options (rows 8/12/6).
     *
     * Drives service_id rendering (sid_display_mode), the optional
     * service_short_label / ensemble_short_label emission (show_short_labels)
     * and the ECC used for country lookup (ecc_override). Defaults match the
     * pre-settings output so default runs are byte-identical.
     *
     * @param settings Analyser decode options to use
     */
    void setAnalyserSettings(const eti::AnalyserSettings& settings) { m_data.settings = settings; }

    /**
     * @brief Enable the strict frame-CRC stats line in the ETI stream section.
     * @param enabled When true a "frame_crc_failures" line is emitted
     * @param failures Number of frames with a mismatching EOH gap CRC
     */
    void setFrameCrcStats(bool enabled, quint64 failures);

    /**
     * @brief Set the aggregated FIG inventory (P1, `figs:` section).
     * @param inventory Observed FIG type/extension entries with frame ranges
     */
    void setFigInventory(const QList<FIGInventoryEntry>& inventory);

    /**
     * @brief Set the aggregated FIC/FIB health counters (P1, `fic_health:`).
     * @param health FIB CRC failures / total FIBs / erroneous FIGs / reconfigs
     */
    void setFicHealth(const FICHealth& health);

    // ========================================================================
    // Verbose Mode - Frame-by-Frame Analysis
    // ========================================================================

    /**
     * @brief Enable verbose frame-by-frame output
     * @param enabled Enable verbose mode
     */
    void setVerboseMode(bool enabled);

    /**
     * @brief Add per-frame analysis data (verbose mode)
     * @param frame_data Detailed frame analysis
     */
    void addFrameAnalysis(const FrameAnalysisData& frame_data);

    // ========================================================================
    // Output Generation
    // ========================================================================

    /**
     * @brief Generate complete YAML output
     * @return YAML formatted string
     */
    QString generateYAML() const;

    /**
     * @brief Write YAML output to file
     * @param filename Output file path
     * @return true if write successful
     */
    bool writeToFile(const QString& filename);

    /**
     * @brief Write YAML output to stdout
     * @return true if write successful
     */
    bool writeToStdout();

private:
    // ========================================================================
    // Internal Data Structures
    // ========================================================================

    /**
     * @brief Complete YAML output data
     */
    struct YAMLData {
        QString version{DABX_VERSION};
        QString generator{"StreamDAB Analyser v" DABX_VERSION " - Verbose Mode"};
        QDateTime timestamp;
        QString input_file;
        qint64 analysis_duration_ms{0};

        struct ETIStream {
            QString format{"ETI-LI"};
            int frame_size{6144};
            int total_frames{0};
            int valid_frames{0};
            int error_frames{0};
            double frame_rate{250.0};
            int crc_errors{0};
            double duration_seconds{0.0};
        } eti_stream;

        eti::Ensemble ensemble;
        QList<eti::DabService> services;
        QList<eti::fig::Fig01SubchannelInfo> subchannels;
        QMap<QString, int> fig_counts;

        bool etsi_compliant{true};
        QString etsi_reason;
        QStringList etsi_warnings;

        qint64 processing_time_ms{0};
        double frames_per_second{0.0};

        // Strict frame-CRC stats (row 11) — line only emitted when enabled
        bool frame_crc_stats_enabled{false};
        quint64 frame_crc_failures{0};

        // FIG inventory + FIC health (P1, additive sections)
        QList<FIGInventoryEntry> fig_inventory;
        FICHealth fic_health;

        // Verbose mode data
        bool verbose_mode{false};
        QList<FrameAnalysisData> frame_analyses;

        // User-selected analyser decode options (rows 6/8/12)
        eti::AnalyserSettings settings;
    } m_data;

    // ========================================================================
    // YAML Formatting Methods
    // ========================================================================

    /**
     * @brief Format ETI stream section
     * @return YAML formatted ETI stream info
     */
    QString formatETIStream() const;

    /**
     * @brief Format ensemble section
     * @return YAML formatted ensemble info
     */
    QString formatEnsemble() const;

    /**
     * @brief Format services section
     * @return YAML formatted service list
     */
    QString formatServices() const;

    /**
     * @brief Format sub-channels section
     * @return YAML formatted sub-channel list
     */
    QString formatSubchannels() const;

    /**
     * @brief Format FIG statistics section
     * @return YAML formatted FIG statistics
     */
    QString formatFIGStatistics() const;

    /**
     * @brief Format the FIG inventory section (P1: `figs:`)
     * @return YAML formatted observed-FIG list with frame ranges
     */
    QString formatFIGInventory() const;

    /**
     * @brief Format the FIC/FIB health section (P1: `fic_health:`)
     * @return YAML formatted FIB/FIG error + reconfiguration counters
     */
    QString formatFICHealth() const;

    /**
     * @brief Format ETSI compliance section
     * @return YAML formatted compliance status
     */
    QString formatETSICompliance() const;

    /**
     * @brief Format performance section
     * @return YAML formatted performance metrics
     */
    QString formatPerformance() const;

    /**
     * @brief Format verbose frames section (DETAILED per-frame analysis)
     * @return YAML formatted frame-by-frame data
     */
    QString formatVerboseFrames() const;

    /**
     * @brief Format single frame analysis (verbose mode)
     * @param frame Frame analysis data
     * @param indent Indentation level (spaces)
     * @return YAML formatted frame data
     */
    QString formatSingleFrame(const FrameAnalysisData& frame, int indent = 2) const;

    /**
     * @brief Format FIG block with complete field decoding
     * @param fig FIG block data
     * @param indent Indentation level (spaces)
     * @return YAML formatted FIG with all fields
     */
    QString formatFIGBlock(const eti::FigBlock& fig, int indent = 6) const;

    // ========================================================================
    // Helper Methods
    // ========================================================================

    /**
     * @brief Escape YAML special characters in string
     * @param str Input string (may contain Thai UTF-8)
     * @return Escaped string safe for YAML
     */
    QString escapeYAMLString(const QString& str) const;

    /**
     * @brief Format 16-bit value as hexadecimal
     * @param value Value to format
     * @return Hexadecimal string (e.g., "0x1234")
     */
    QString formatHex(quint16 value) const;

    /**
     * @brief Format 32-bit value as hexadecimal
     * @param value Value to format
     * @return Hexadecimal string (e.g., "0x12345678")
     */
    QString formatHex(quint32 value) const;

    /**
     * @brief Format 8-bit value as hexadecimal
     * @param value Value to format
     * @return Hexadecimal string (e.g., "0x12")
     */
    QString formatHex8(quint8 value) const;

    /**
     * @brief Format byte array as hex dump
     * @param data Byte array
     * @param max_bytes Maximum bytes to show
     * @return Hex string (e.g., "0x12 0x34 0x56 ...")
     */
    QString formatHexDump(const QByteArray& data, int max_bytes = 32) const;

    /**
     * @brief Format protection level description
     * @param subchannel Sub-channel info
     * @return Protection level string (e.g., "UEP-3", "EEP-2A")
     */
    QString formatProtectionLevel(const eti::fig::Fig01SubchannelInfo& subchannel) const;

    /**
     * @brief Format EEP protection level detail
     * @param protection_level Protection level (0-3)
     * @param option Option flag (0=A, 1=B)
     * @return Protection level string (e.g., "1-A", "4-B")
     */
    QString formatEEPProtectionLevel(uint8_t protection_level, uint8_t option) const;

    /**
     * @brief Get country name from country ID and ECC
     * @param country_id Country ID (4-bit)
     * @param extended_country_code Extended Country Code (8-bit)
     * @return Country name string
     */
    QString getCountryName(uint8_t country_id, uint8_t extended_country_code) const;

    /**
     * @brief Get service type description
     * @param service DAB service
     * @return Service type string (e.g., "DAB+ Audio", "Data Service")
     */
    QString getServiceType(const eti::DabService& service) const;

    /**
     * @brief Write YAML to text stream
     * @param stream Output text stream
     * @param yaml YAML content
     * @return true if write successful
     */
    bool writeToStream(QTextStream& stream, const QString& yaml) const;
};

} // namespace cli

#endif // YAML_OUTPUT_GENERATOR_HPP
