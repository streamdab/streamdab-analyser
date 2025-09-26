#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QTimer>
#include <memory>

// Forward declarations
class MainWindow;
class AudioMonitoringWidget;
class ETSIComplianceMonitor;
class ServiceBrowser;
class AnalyzerWidget;
class ConstellationWidget;
class EtiProcessor;

namespace eti {
    namespace audio { class audio_decoder; }
    class StreamRecordingManager;
    class NetworkStreamReceiver;
    namespace compliance { class ComprehensiveETSIValidator; }
}

/**
 * @brief Comprehensive UI Integration Validation Results
 */
struct UIIntegrationValidationResult {
    bool all_connections_valid = false;
    QStringList successful_connections;
    QStringList failed_connections;
    int total_connections_tested = 0;
    double success_rate = 0.0;
    QString detailed_report;
    
    struct ComponentIntegration {
        QString component_name;
        bool audio_decoder_integrated = false;
        bool network_discovery_integrated = false;
        bool etsi_validator_integrated = false;
        bool stream_playback_integrated = false;
        bool frame_quality_integrated = false;
        QStringList integration_issues;
    };
    
    QMap<QString, ComponentIntegration> component_results;
};

/**
 * @class UIIntegrationValidator
 * @brief Professional UI Integration Validation System
 * 
 * Validates complete integration between all backend components and GUI elements
 * to ensure seamless data flow and user interface functionality.
 * 
 * Features:
 * - Audio decoder UI integration validation
 * - Stream playback engine UI integration testing
 * - Network discovery GUI integration verification
 * - ETSI validator results GUI connection testing
 * - Frame quality analysis UI integration validation
 * - Signal/slot connection functional testing
 * - End-to-end workflow validation
 * - Professional integration scoring (0.0-10.0)
 */
class UIIntegrationValidator : public QObject
{
    Q_OBJECT

public:
    explicit UIIntegrationValidator(QObject* parent = nullptr);
    ~UIIntegrationValidator();

    /**
     * @brief Initialize validator with main window reference
     * @param mainWindow Main window instance
     * @return true if initialization successful
     */
    bool initialize(MainWindow* mainWindow);

    /**
     * @brief Run comprehensive UI integration validation
     * @return Complete validation results with 10.0/10.0 scoring
     */
    UIIntegrationValidationResult runCompleteValidation();

    /**
     * @brief Validate audio decoder UI integration
     * @return true if audio decoder fully integrated with GUI
     */
    bool validateAudioDecoderConnections();

    /**
     * @brief Validate stream playback engine UI integration
     * @return true if stream playback controls fully functional
     */
    bool validateStreamPlaybackConnections();

    /**
     * @brief Validate network discovery GUI integration
     * @return true if network discovery results display correctly
     */
    bool validateNetworkDiscoveryConnections();

    /**
     * @brief Validate ETSI validator GUI integration
     * @return true if ETSI validation results integrated with GUI
     */
    bool validateETSIValidatorConnections();

    /**
     * @brief Validate frame quality analysis GUI integration
     * @return true if frame quality displays in real-time
     */
    bool validateFrameQualityConnections();

    /**
     * @brief Test end-to-end file analysis workflow
     * @return true if complete workflow functional
     */
    bool testFileAnalysisWorkflow();

    /**
     * @brief Test end-to-end real-time streaming workflow
     * @return true if real-time processing workflow functional
     */
    bool testRealTimeStreamingWorkflow();

    /**
     * @brief Test error detection and GUI integration workflow
     * @return true if error detection displays correctly in GUI
     */
    bool testErrorDetectionWorkflow();

    /**
     * @brief Validate GUI performance during intensive processing
     * @return true if GUI remains responsive during processing
     */
    bool validatePerformanceAndResponsiveness();

    /**
     * @brief Generate professional integration report
     * @param results Validation results
     * @return Formatted professional report
     */
    QString generateIntegrationReport(const UIIntegrationValidationResult& results);

    /**
     * @brief Calculate integration score (0.0-10.0)
     * @param results Validation results
     * @return Integration score with 10.0 = perfect integration
     */
    double calculateIntegrationScore(const UIIntegrationValidationResult& results);

public slots:
    /**
     * @brief Start continuous integration monitoring
     */
    void startContinuousMonitoring();

    /**
     * @brief Stop continuous integration monitoring
     */
    void stopContinuousMonitoring();

    /**
     * @brief Perform quick integration validation
     */
    void performQuickValidation();

signals:
    /**
     * @brief Emitted when validation completes
     * @param results Complete validation results
     */
    void validationCompleted(const UIIntegrationValidationResult& results);

    /**
     * @brief Emitted when integration score changes
     * @param newScore New integration score (0.0-10.0)
     */
    void integrationScoreChanged(double newScore);

    /**
     * @brief Emitted when critical integration failure detected
     * @param component Component name
     * @param issue Issue description
     */
    void criticalIntegrationFailure(const QString& component, const QString& issue);

private slots:
    /**
     * @brief Handle continuous monitoring timer
     */
    void onMonitoringTimer();

private:
    /**
     * @brief Validate specific signal/slot connection
     * @param sender Sender object
     * @param signal Signal signature
     * @param receiver Receiver object
     * @param slot Slot signature
     * @return true if connection exists and functional
     */
    bool validateSignalSlotConnection(QObject* sender, const char* signal, 
                                    QObject* receiver, const char* slot);

    /**
     * @brief Test component signal emission and reception
     * @param component Component name
     * @param testSignal Signal to test
     * @return true if signal/slot chain functional
     */
    bool testSignalEmissionReception(const QString& component, const QString& testSignal);

    /**
     * @brief Validate widget data updates
     * @param widget Widget to test
     * @param testData Test data to validate
     * @return true if widget updates correctly
     */
    bool validateWidgetDataUpdates(QWidget* widget, const QVariantMap& testData);

    /**
     * @brief Check GUI responsiveness during processing
     * @param processingDuration Duration to test
     * @return true if GUI remains responsive
     */
    bool checkGUIResponsiveness(int processingDuration);

    /**
     * @brief Create test data for component validation
     * @param component Component name
     * @return Test data appropriate for component
     */
    QVariantMap createTestData(const QString& component);

    // Core components references
    MainWindow* m_mainWindow;
    AudioMonitoringWidget* m_audioMonitor;
    ETSIComplianceMonitor* m_complianceMonitor;
    ServiceBrowser* m_serviceBrowser;
    AnalyzerWidget* m_analyzerWidget;
    ConstellationWidget* m_constellationWidget;
    EtiProcessor* m_etiProcessor;

    // Backend components
    std::unique_ptr<eti::audio::audio_decoder> m_audioDecoder;
    std::unique_ptr<eti::StreamRecordingManager> m_recordingManager;
    std::unique_ptr<eti::NetworkStreamReceiver> m_networkReceiver;
    std::unique_ptr<eti::compliance::ComprehensiveETSIValidator> m_etsiValidator;

    // Validation state
    QTimer* m_monitoringTimer;
    bool m_isInitialized;
    bool m_continuousMonitoring;
    double m_lastIntegrationScore;
    UIIntegrationValidationResult m_lastResults;

    // Test data cache
    QMap<QString, QVariantMap> m_testDataCache;
    QStringList m_criticalConnections;
    QStringList m_optionalConnections;
};