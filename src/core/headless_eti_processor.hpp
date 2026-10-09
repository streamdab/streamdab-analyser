#pragma once

#include <QObject>
#include <QString>
#include <QElapsedTimer>
#include <QMap>
#include <memory>
#include <vector>

// Include full definitions needed
#include "eti_types.hpp"
#include "fig_parser.hpp"
#include "analyser_settings.hpp"
#include "../cli/yaml_output_generator.hpp"  // Need full definition for Qt MOC

// Forward declarations for Qt classes only
class EtiProcessor;
namespace eti {
namespace modern {
    class ModernETIFrameParser;
    class EnhancedFIGAnalyser;
}
namespace compliance {
    class ComprehensiveETSIValidator;
}
}

/**
 * @class HeadlessETIProcessor
 * @brief Headless ETI processing engine for batch operations
 *
 * This class provides ETI processing capabilities without GUI dependencies,
 * suitable for command-line batch processing, server deployments, and
 * automated analysis workflows.
 *
 * Features:
 * - Complete ETI frame analysis without GUI
 * - ETSI compliance validation
 * - Service discovery and enumeration
 * - Performance metrics and statistics
 * - YAML/JSON output for integration
 * - VERBOSE MODE: Frame-by-frame detailed analysis
 */
class HeadlessETIProcessor : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Processing result structure
     */
    struct ProcessingResult {
        bool success = false;
        QString errorMessage;

        // Statistics
        quint64 totalFrames = 0;
        quint64 processedFrames = 0;
        quint64 errorFrames = 0;
        double processingTimeMs = 0.0;
        double averageFPS = 0.0;

        // Analysis results (basic counts)
        quint32 servicesFound = 0;
        quint32 ensemblesFound = 0;
        QString ensembleName;
        QStringList serviceNames;

        // AGENT 45 FIX: Add aggregated data structures for YAML output
        // Using std::vector to avoid Qt container restrictions
        eti::Ensemble ensemble;  // Primary ensemble info
        std::vector<eti::DabService> services;  // Discovered services
        std::vector<eti::fig::Fig01SubchannelInfo> subchannels;  // Discovered subchannels
        QMap<QString, int> fig_type_counts;  // FIG type statistics

        // VERBOSE FRAMES: Frame-by-frame detailed analysis
        bool verbose_mode = false;
        std::vector<cli::FrameAnalysisData> frame_analyses;  // Per-frame data

        // P1 (CLI FIG detail wave): per-frame FIG instance trace lines, the
        // aggregated FIG inventory and FIC/FIB health counters. fig_inventory
        // is sorted by (fig_type, extension); frames are 1-based.
        QStringList fig_trace_lines;                 // "fig-trace: frame=.. type=.. .."
        std::vector<cli::FIGInventoryEntry> fig_inventory;
        cli::FICHealth fic_health;

        // Backlog (P2/P3, docs/fixes/CLI_FIG_COVERAGE.md): grouped-by-FIG
        // view lines ("figs-by-type: fig=T/E count=.. frames=.. values=[..]")
        // and per-FIB hex dump lines ("fib-hex: frame=.. fib=.. crc=.." +
        // a 30-byte hex line and a FIG marker line). Both feed stderr in the
        // same diagnostic channel as the trace lines.
        QStringList figs_by_type_lines;
        QStringList fib_hex_lines;

        // Row 11 (strict_frame_crc): number of frames whose EOH gap CRC
        // (bytes 8+4*NST .. 11+4*NST, CCITT-FALSE complement) mismatched.
        // Only counted when the setting is enabled; decode stays independent.
        quint64 frame_crc_failures = 0;

        // ETSI compliance. T39: `etsiCompliant` reflects the FIC/FIB/FIG
        // protocol validation actually performed on this path (no FIB CRC
        // failures, no erroneous FIGs, a decoded FIC and intact ensemble/service
        // decode) — NOT the transport error-frame count, which is reported
        // separately (errorFrames / fic_health). `complianceReason` always
        // carries the short count summary that drove the verdict so a NO is
        // explainable.
        bool etsiCompliant = true;
        QString complianceReason;
        QStringList complianceWarnings;
        QStringList complianceErrors;
    };

    explicit HeadlessETIProcessor(QObject* parent = nullptr);
    ~HeadlessETIProcessor();

    /**
     * @brief Set the user-selected analyser decode options.
     *
     * Applied on the next process_file() call (threaded through the modern
     * frame parser, the service merge, verbose timestamps and the strict
     * frame-CRC counter). Defaults to AnalyserSettings::defaults().
     *
     * @param settings Analyser decode options to use
     */
    void setAnalyserSettings(const eti::AnalyserSettings& settings) { m_settings = settings; }

    /**
     * @brief Current analyser decode options.
     */
    const eti::AnalyserSettings& analyserSettings() const noexcept { return m_settings; }

    /**
     * @brief Process ETI file and generate analysis
     * @param inputFile Path to input ETI file
     * @param outputFile Path to output analysis file
     * @return Processing result with statistics and analysis
     */
    ProcessingResult process_file(const QString& inputFile, const QString& outputFile);

    /**
     * @brief Enable verbose frame-by-frame output mode
     * @param verbose Enable verbose mode
     */
    void setVerboseMode(bool verbose) { m_verbose_mode = verbose; }

    /**
     * @brief Enable per-frame FIG instance trace output (P1).
     *
     * When enabled (or when verbose mode is on), a compact
     * "fig-trace: frame=.. type=.. ext=.. len=.. fib=.. crc=.. reconfig=.."
     * line is emitted to stderr for every FIG instance of every parsed frame.
     *
     * @param enabled Enable the FIG instance trace
     */
    void setFigTraceMode(bool enabled) { m_fig_trace_mode = enabled; }

    /**
     * @brief Limit the number of processed frames (0 = all).
     *
     * Stops the frame loop after @p maxFrames complete frames have been
     * processed. Statistics (frames, FIG inventory, FIC health) then cover
     * the processed prefix only.
     *
     * @param maxFrames Maximum frames to process, 0 for no limit
     */
    void setMaxFrames(int maxFrames) { m_max_frames = maxFrames; }

    /**
     * @brief Enable the grouped-by-FIG view (backlog P2/P3, --figs-by-type).
     *
     * Emits one "figs-by-type: fig=T/E count=N frames=.. values=[..]" line to
     * stderr per observed FIG type/extension, aggregating the per-frame trace
     * data (frame numbers + representative readable values).
     *
     * @param enabled Enable the grouped view
     */
    void setFigsByTypeMode(bool enabled) { m_figs_by_type_mode = enabled; }

    /**
     * @brief Enable per-FIB hex dumps (backlog P2/P3, --fib-hex).
     *
     * For every processed frame with a structured FIC, prints a line per FIB:
     * "fib-hex: frame=N fib=F crc=ok|bad" followed by the 30 FIG-area bytes
     * and one marker line with the analyzed FIG byte ranges ("[0-5] FIG 0/0").
     * Frame selection honours --max-frames / --verbose-frames.
     *
     * @param enabled Enable the FIB hex dumps
     */
    void setFibHexMode(bool enabled) { m_fib_hex_mode = enabled; }

    /**
     * @brief Get processing progress (0.0 to 1.0)
     */
    double get_progress() const { return m_progress; }

    /**
     * @brief Get current processing status
     */
    QString get_status_text() const { return m_statusText; }

signals:
    /**
     * @brief Emitted when processing progress changes
     */
    void progress_changed(double progress);

    /**
     * @brief Emitted when status text changes
     */
    void status_changed(const QString& status);

    /**
     * @brief Emitted when processing is complete
     */
    void processing_complete(const ProcessingResult& result);

    /**
     * @brief Emitted when a DAB service is discovered
     * @param serviceId Service ID
     * @param serviceName Service name/label
     * @param serviceType Type of service (Audio, Data, DAB+, etc.)
     */
    void service_discovered(uint32_t serviceId, const QString& serviceName, const QString& serviceType);

    /**
     * @brief Emitted when a DAB ensemble is discovered
     * @param ensembleId Ensemble ID
     * @param ensembleName Ensemble name/label
     */
    void ensemble_discovered(uint16_t ensembleId, const QString& ensembleName);

    /**
     * @brief Emitted when a frame is processed with analysis
     * @param frameNumber Frame number (0-based)
     * @param frameData Basic frame information
     */
    void frame_analyzed(uint64_t frameNumber, const QString& frameData);

private slots:
    void on_frame_processed();
    void on_error_detected(const QString& error);

private:  // NOLINTNEXTLINE(readability-redundant-access-specifiers) - Required by Qt MOC
    // Core processing components
    std::unique_ptr<EtiProcessor> m_etiProcessor;
    std::unique_ptr<eti::modern::ModernETIFrameParser> m_frameParser;
    std::unique_ptr<eti::modern::EnhancedFIGAnalyser> m_figAnalyser;
    // DISABLED FOR CLI BUILD: ComprehensiveETSIValidator all methods commented out
    // std::unique_ptr<eti::compliance::ComprehensiveETSIValidator> m_etsiValidator;

    // Processing state
    double m_progress = 0.0;
    QString m_statusText;
    ProcessingResult m_currentResult;
    QElapsedTimer m_processingTimer;
    bool m_verbose_mode = false;  // Verbose frame-by-frame output
    bool m_fig_trace_mode = false;  // P1: per-frame FIG instance trace
    bool m_figs_by_type_mode = false;  // P2/P3: grouped-by-FIG view
    bool m_fib_hex_mode = false;       // P2/P3: per-FIB hex dumps
    int m_max_frames = 0;           // P1: process at most N frames (0 = all)

    // User-selected analyser decode options (option-variant matrix rows 1-14).
    eti::AnalyserSettings m_settings;

    // Private methods
    bool initialize_components();
    void reset_processing_state();
    bool save_results_to_file(const QString& outputFile, const ProcessingResult& result);
    QString generate_yaml_output(const ProcessingResult& result);
    QString generate_json_output(const ProcessingResult& result);
};
