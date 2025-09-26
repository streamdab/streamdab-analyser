#pragma once

#include <QObject>
#include <QString>
#include <QElapsedTimer>
#include <memory>

// Forward declarations
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
        
        // Analysis results
        quint32 servicesFound = 0;
        quint32 ensemblesFound = 0;
        QString ensembleName;
        QStringList serviceNames;
        
        // ETSI compliance
        bool etsiCompliant = true;
        QStringList complianceWarnings;
        QStringList complianceErrors;
    };

    explicit HeadlessETIProcessor(QObject* parent = nullptr);
    ~HeadlessETIProcessor();

    /**
     * @brief Process ETI file and generate analysis
     * @param inputFile Path to input ETI file
     * @param outputFile Path to output analysis file
     * @return Processing result with statistics and analysis
     */
    ProcessingResult process_file(const QString& inputFile, const QString& outputFile);

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
    std::unique_ptr<eti::compliance::ComprehensiveETSIValidator> m_etsiValidator;

    // Processing state
    double m_progress = 0.0;
    QString m_statusText;
    ProcessingResult m_currentResult;
    QElapsedTimer m_processingTimer;

    // Private methods
    bool initialize_components();
    void reset_processing_state();
    bool save_results_to_file(const QString& outputFile, const ProcessingResult& result);
    QString generate_yaml_output(const ProcessingResult& result);
    QString generate_json_output(const ProcessingResult& result);
};