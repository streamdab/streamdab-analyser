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
#include "analyzer_settings.hpp"
#include "../cli/yaml_output_generator.hpp"  // Need full definition for Qt MOC

// Forward declarations for Qt classes only
class EtiProcessor;
namespace eti {
namespace modern {
    class ModernETIFrameParser;
    class EnhancedFIGAnalyzer;
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

        // Row 11 (strict_frame_crc): number of frames whose EOH gap CRC
        // (bytes 8+4*NST .. 11+4*NST, CCITT-FALSE complement) mismatched.
        // Only counted when the setting is enabled; decode stays independent.
        quint64 frame_crc_failures = 0;

        // ETSI compliance
        bool etsiCompliant = true;
        QStringList complianceWarnings;
        QStringList complianceErrors;
    };

    explicit HeadlessETIProcessor(QObject* parent = nullptr);
    ~HeadlessETIProcessor();

    /**
     * @brief Set the user-selected analyzer decode options.
     *
     * Applied on the next process_file() call (threaded through the modern
     * frame parser, the service merge, verbose timestamps and the strict
     * frame-CRC counter). Defaults to AnalyzerSettings::defaults().
     *
     * @param settings Analyzer decode options to use
     */
    void setAnalyzerSettings(const eti::AnalyzerSettings& settings) { m_settings = settings; }

    /**
     * @brief Current analyzer decode options.
     */
    const eti::AnalyzerSettings& analyzerSettings() const noexcept { return m_settings; }

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
    std::unique_ptr<eti::modern::EnhancedFIGAnalyzer> m_figAnalyzer;
    // DISABLED FOR CLI BUILD: ComprehensiveETSIValidator all methods commented out
    // std::unique_ptr<eti::compliance::ComprehensiveETSIValidator> m_etsiValidator;

    // Processing state
    double m_progress = 0.0;
    QString m_statusText;
    ProcessingResult m_currentResult;
    QElapsedTimer m_processingTimer;
    bool m_verbose_mode = false;  // Verbose frame-by-frame output

    // User-selected analyzer decode options (option-variant matrix rows 1-14).
    eti::AnalyzerSettings m_settings;

    // Private methods
    bool initialize_components();
    void reset_processing_state();
    bool save_results_to_file(const QString& outputFile, const ProcessingResult& result);
    QString generate_yaml_output(const ProcessingResult& result);
    QString generate_json_output(const ProcessingResult& result);
};
