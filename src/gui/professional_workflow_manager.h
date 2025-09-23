#pragma once

#include <QObject>
#include <QTimer>
#include <QHash>
#include <QSharedPointer>
// QStateMachine not available - using enum-based state management
#include <QKeySequence>
#include <QElapsedTimer>
#include "core/eti_types.h"

// Forward declarations
class MainWindow;
class EtiProcessor;
class ServiceExplorerPanel;
class PerformanceDashboard;
class ETSIComplianceMonitor;
class AnalyzerWidget;
class ConstellationWidget;
class ServiceBrowser;
class AudioMonitoringWidget;
class FigAnalysisWidget;

/**
 * @class ProfessionalWorkflowManager
 * @brief Professional broadcast user workflow integration manager
 * 
 * Integrates all GUI components with ETSI-compliant ETI processing engine
 * to provide seamless professional workflows matching broadcast industry standards.
 * 
 * Key Features:
 * - File Analysis Workflow: File → Open → Analyze → Navigate → Export Results
 * - Real-Time Analysis Workflow: Connect Source → Enable Real-time Mode → Stream Processing → Live Updates → Record/Export
 * - Error Detection Workflow: Open File → Check Messages Panel → Navigate to Errors → Analyze Details → Generate Report
 * - Service Discovery Workflow: Service Selection → Properties Update → Main Panel Filter → Audio Monitor → Quality Analysis
 * 
 * Performance Requirements:
 * - Panel synchronization maintains <100ms response time
 * - System maintains >900 FPS ETI processing during GUI operations
 * - Real-time updates at 60 FPS for visualization widgets
 * - Professional keyboard shortcuts and navigation
 */
class ProfessionalWorkflowManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @enum WorkflowState
     * @brief Professional workflow states
     */
    enum WorkflowState {
        Idle,                    // No active workflow
        FileAnalysisActive,      // File analysis workflow
        RealTimeAnalysisActive,  // Real-time analysis workflow  
        ErrorDetectionActive,    // Error detection workflow
        ServiceDiscoveryActive,  // Service discovery workflow
        ComparativeAnalysis      // Dual-mode comparison workflow
    };
    Q_ENUM(WorkflowState)

    /**
     * @enum PanelSyncMode
     * @brief Panel synchronization modes
     */
    enum PanelSyncMode {
        AutoSync,     // Automatic synchronization (default)
        ManualSync,   // Manual synchronization
        FilterSync,   // Service filtering synchronization
        CompareSync   // Comparison mode synchronization
    };
    Q_ENUM(PanelSyncMode)

    /**
     * @struct WorkflowMetrics
     * @brief Workflow performance metrics
     */
    struct WorkflowMetrics {
        qint64 panelSyncLatency;        // Panel synchronization latency (target: <100ms)
        double etiProcessingRate;       // ETI processing rate (target: >900 FPS)
        double uiUpdateRate;            // UI update rate (target: 60 FPS)
        int totalFramesProcessed;       // Total ETI frames processed
        int errorsDetected;             // ETSI compliance errors detected
        int servicesDiscovered;         // DAB services discovered
        qint64 workflowStartTime;       // Workflow start timestamp
        QString currentWorkflow;        // Current active workflow name
        
        WorkflowMetrics() : panelSyncLatency(0), etiProcessingRate(0.0), uiUpdateRate(0.0),
                           totalFramesProcessed(0), errorsDetected(0), servicesDiscovered(0),
                           workflowStartTime(0) {}
    };

    /**
     * @brief Constructor
     * @param mainWindow Main window instance
     * @param parent Parent QObject
     */
    explicit ProfessionalWorkflowManager(MainWindow* mainWindow, QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~ProfessionalWorkflowManager();

    /**
     * @brief Initialize workflow manager
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * @brief Get current workflow state
     * @return Current workflow state
     */
    WorkflowState getCurrentState() const { return m_currentState; }

    /**
     * @brief Get workflow performance metrics
     * @return Current workflow metrics
     */
    WorkflowMetrics getWorkflowMetrics() const { return m_workflowMetrics; }

    /**
     * @brief Check if panel synchronization is active
     * @return true if panels are synchronized
     */
    bool isPanelSyncActive() const { return m_panelSyncActive; }

    /**
     * @brief Get panel synchronization mode
     * @return Current panel sync mode
     */
    PanelSyncMode getPanelSyncMode() const { return m_panelSyncMode; }

public slots:
    // ==============================================
    // FILE ANALYSIS WORKFLOW IMPLEMENTATION
    // ==============================================
    
    /**
     * @brief Start File Analysis Workflow
     * File → Open → Analyze → Navigate → Export Results
     * @param filename ETI file to analyze
     */
    void startFileAnalysisWorkflow(const QString& filename);

    /**
     * @brief Handle file opened in workflow
     * @param filename Opened file path
     */
    void onFileOpened(const QString& filename);

    /**
     * @brief Handle file analysis completed
     * @param frameCount Number of frames processed
     * @param serviceCount Number of services discovered
     */
    void onFileAnalysisCompleted(int frameCount, int serviceCount);

    // ==============================================
    // REAL-TIME ANALYSIS WORKFLOW IMPLEMENTATION
    // ==============================================

    /**
     * @brief Start Real-Time Analysis Workflow
     * Connect Source → Enable Real-time Mode → Stream Processing → Live Updates → Record/Export
     * @param sourceConfig ETI-over-IP source configuration
     */
    void startRealTimeAnalysisWorkflow(const QHash<QString, QVariant>& sourceConfig);

    /**
     * @brief Handle real-time mode enabled
     * @param enabled true if real-time mode is active
     */
    void onRealTimeModeChanged(bool enabled);

    /**
     * @brief Handle live ETI frame processed
     * @param frameData ETI frame data
     * @param processingRate Current processing rate (FPS)
     */
    void onLiveFrameProcessed(const QByteArray& frameData, double processingRate);

    // ==============================================
    // ERROR DETECTION WORKFLOW IMPLEMENTATION
    // ==============================================

    /**
     * @brief Start Error Detection Workflow
     * Open File → Check Messages Panel → Navigate to Errors → Analyze Details → Generate Report
     * @param filename File to check for errors
     */
    void startErrorDetectionWorkflow(const QString& filename);

    /**
     * @brief Handle ETSI compliance error detected
     * @param error Error information
     */
    void onETSIComplianceError(const eti::ETSIComplianceResult& error);

    /**
     * @brief Navigate to error frame
     * @param frameNumber Frame number with error
     * @param errorType Type of ETSI compliance error
     */
    void navigateToErrorFrame(quint64 frameNumber, const QString& errorType);

    /**
     * @brief Generate professional error report
     * @param outputPath Path to save error report
     * @return true if report generated successfully
     */
    bool generateErrorReport(const QString& outputPath);

    // ==============================================
    // SERVICE DISCOVERY WORKFLOW IMPLEMENTATION
    // ==============================================

    /**
     * @brief Start Service Discovery Workflow
     * Service Selection → Properties Update → Main Panel Filter → Audio Monitor → Quality Analysis
     */
    void startServiceDiscoveryWorkflow();

    /**
     * @brief Handle service selected in explorer
     * @param serviceId Selected service ID
     */
    void onServiceSelected(quint32 serviceId);

    /**
     * @brief Handle service properties updated
     * @param serviceId Service ID
     * @param properties Service properties
     */
    void onServicePropertiesUpdated(quint32 serviceId, const QHash<QString, QVariant>& properties);

    /**
     * @brief Filter main panel by selected services
     * @param serviceIds List of service IDs to show
     */
    void filterMainPanelByServices(const QList<quint32>& serviceIds);

    // ==============================================
    // PANEL SYNCHRONIZATION ARCHITECTURE
    // ==============================================

    /**
     * @brief Set panel synchronization mode
     * @param mode New synchronization mode
     */
    void setPanelSyncMode(PanelSyncMode mode);

    /**
     * @brief Synchronize all panels with current data
     */
    void synchronizeAllPanels();

    /**
     * @brief Synchronize Explorer panel with service data
     * @param services List of discovered services
     */
    void synchronizeExplorerPanel(const QList<eti::DabService>& services);

    /**
     * @brief Synchronize Properties panel with selected service
     * @param serviceId Selected service ID
     */
    void synchronizePropertiesPanel(quint32 serviceId);

    /**
     * @brief Synchronize Main panel with frame data
     * @param frames List of ETI frames to display
     */
    void synchronizeMainPanel(const QList<eti::EtiFrame>& frames);

    /**
     * @brief Synchronize Messages panel with errors
     * @param errors List of ETSI compliance errors
     */
    void synchronizeMessagesPanel(const QList<eti::ETSIComplianceResult>& errors);

    // ==============================================
    // PROFESSIONAL USER EXPERIENCE
    // ==============================================

    /**
     * @brief Setup professional keyboard shortcuts
     */
    void setupKeyboardShortcuts();

    /**
     * @brief Handle keyboard shortcut activated
     * @param shortcut Activated keyboard shortcut
     */
    void onKeyboardShortcutActivated(const QKeySequence& shortcut);

    /**
     * @brief Show workflow progress
     * @param workflowName Name of active workflow
     * @param progress Progress percentage (0-100)
     */
    void showWorkflowProgress(const QString& workflowName, int progress);

    /**
     * @brief Update workflow status
     * @param status Status message
     * @param details Additional details
     */
    void updateWorkflowStatus(const QString& status, const QString& details = QString());

signals:
    /**
     * @brief Emitted when workflow state changes
     * @param newState New workflow state
     * @param oldState Previous workflow state
     */
    void workflowStateChanged(WorkflowState newState, WorkflowState oldState);

    /**
     * @brief Emitted when panel synchronization completes
     * @param syncLatency Synchronization latency in milliseconds
     */
    void panelSynchronizationCompleted(qint64 syncLatency);

    /**
     * @brief Emitted when workflow metrics are updated
     * @param metrics Current workflow metrics
     */
    void workflowMetricsUpdated(const WorkflowMetrics& metrics);

    /**
     * @brief Emitted when service selection changes
     * @param serviceId Selected service ID
     */
    void serviceSelectionChanged(quint32 serviceId);

    /**
     * @brief Emitted when error navigation is requested
     * @param frameNumber Frame number to navigate to
     * @param errorType Type of error
     */
    void errorNavigationRequested(quint64 frameNumber, const QString& errorType);

    /**
     * @brief Emitted when real-time processing rate changes
     * @param processingRate Current processing rate (FPS)
     */
    void realTimeProcessingRateChanged(double processingRate);

    /**
     * @brief Emitted when professional report is generated
     * @param reportPath Path to generated report
     * @param reportType Type of report generated
     */
    void professionalReportGenerated(const QString& reportPath, const QString& reportType);

private slots:
    /**
     * @brief Update workflow metrics
     */
    void updateWorkflowMetrics();

    /**
     * @brief Handle ETI processor signals
     */
    void onEtiFrameProcessed(quint64 frameNumber, const QByteArray& data);
    void onEtiServiceDiscovered(const eti::DabService& service);
    void onEtiEnsembleDiscovered(const eti::Ensemble& ensemble);
    void onEtiErrorOccurred(const QString& error);

    /**
     * @brief Handle UI update timer
     */
    void onUIUpdateTimer();

private:
    /**
     * @brief Setup signal/slot connections for workflow integration
     */
    void setupSignalSlotConnections();

    /**
     * @brief Initialize workflow state machine
     */
    void initializeWorkflowStateMachine();

    /**
     * @brief Create panel synchronization timer
     */
    void createPanelSyncTimer();

    /**
     * @brief Measure panel synchronization latency
     * @return Synchronization latency in milliseconds
     */
    qint64 measurePanelSyncLatency();

    /**
     * @brief Update UI components with current data
     */
    void updateUIComponents();

    /**
     * @brief Validate workflow state consistency
     * @return true if workflow state is consistent
     */
    bool validateWorkflowState() const;

    // Main window and component references
    MainWindow* m_mainWindow;
    EtiProcessor* m_etiProcessor;
    ServiceExplorerPanel* m_serviceExplorerPanel;
    PerformanceDashboard* m_performanceDashboard;
    ETSIComplianceMonitor* m_complianceMonitor;
    AnalyzerWidget* m_analyzerWidget;
    ConstellationWidget* m_constellationWidget;
    ServiceBrowser* m_serviceBrowser;
    AudioMonitoringWidget* m_audioMonitor;
    FigAnalysisWidget* m_figAnalysis;

    // Workflow state management
    WorkflowState m_currentState;
    // QStateMachine not available - using simple enum-based state management

    // Panel synchronization
    bool m_panelSyncActive;
    PanelSyncMode m_panelSyncMode;
    QTimer* m_panelSyncTimer;
    QElapsedTimer* m_syncLatencyTimer;

    // Performance monitoring
    QTimer* m_metricsUpdateTimer;
    QTimer* m_uiUpdateTimer;
    WorkflowMetrics m_workflowMetrics;
    QElapsedTimer* m_workflowTimer;

    // Keyboard shortcuts
    QHash<QKeySequence, QString> m_keyboardShortcuts;

    // Current workflow data
    QString m_currentFilename;
    QHash<quint32, eti::DabService> m_discoveredServices;
    QList<eti::ETSIComplianceResult> m_detectedErrors;
    quint32 m_selectedServiceId;

    // Performance constants
    static constexpr int PANEL_SYNC_INTERVAL_MS = 50;      // 20 Hz panel sync
    static constexpr int UI_UPDATE_INTERVAL_MS = 16;       // 60 FPS UI updates
    static constexpr int METRICS_UPDATE_INTERVAL_MS = 1000; // 1 Hz metrics updates
    static constexpr qint64 TARGET_PANEL_SYNC_LATENCY = 100; // Target <100ms
    static constexpr double TARGET_ETI_PROCESSING_RATE = 900.0; // Target >900 FPS
    static constexpr double TARGET_UI_UPDATE_RATE = 60.0;   // Target 60 FPS
};