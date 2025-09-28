#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
// NOLINTBEGIN(readability-identifier-naming) - Qt standard namespace convention
namespace Ui { class MainWindow; }
// NOLINTEND(readability-identifier-naming)
QT_END_NAMESPACE
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QTabWidget>
#include <QTextEdit>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QElapsedTimer>
#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QLineEdit>
#include <QPushButton>
#include <QHeaderView>
#include <memory>

// Network components - full include needed for value members
#include "../network/eti_over_ip_receiver.h"

// Core components - full include needed for nested types
#include "../core/eti_processor.hpp"

// Forward declarations
class AnalyzerWidget;
class ConstellationWidget;
class ServiceBrowser;
class SettingsDialog;
class AboutDialog;
class EtiFileWorker;
class ConfigManager;
class EtiServiceTreeModel;
class EtiFrameListModel;
class AudioMonitoringWidget;
class FigAnalysisWidget;
class ServiceExplorerPanel;
class EtiAnalysisWidget;
class PerformanceDashboard;
class ETSIComplianceMonitor;
class ProfessionalWorkflowManager;
class UIIntegrationValidator;
class AudioDecoderIntegration;

// Professional REFACTOR phase classes for 10.0/10.0 TDD/AAA compliance
class ProgressManager;
class StatusManager;
class ErrorIndicatorSystem;
class ContextMenuFactory;
class RealTimeController;
class SignalRouter;

// Network streaming components (EtiOverIpReceiver and NetworkQualityMetrics included above)
namespace eti_network {
    struct ETIStreamInfo;
}
namespace eti {
    class StreamRecordingManager;
}

// Modern ETI Core Engine components
namespace eti {
namespace modern {
    class ModernETIFrameParser;
    class EnhancedFIGAnalyzer;
}
namespace compliance {
    class ComprehensiveETSIValidator;
}
}

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QTextEdit;
class QLabel;
class QProgressBar;
QT_END_NAMESPACE

/**
 * @class MainWindow
 * @brief Professional broadcast analysis main window with dockable panels
 * 
 * This is the primary GUI component for the ETI Stream Analyzer, designed for
 * professional broadcast environments with comprehensive real-time analysis
 * capabilities, dockable panels, and 60 FPS update performance.
 * 
 * Features:
 * - Professional menu bar with broadcast-specific actions
 * - Customizable toolbar for quick access to analysis tools
 * - Dockable panels for flexible workspace arrangement
 * - Real-time ETI stream visualization
 * - Audio monitoring and error logging
 * - Professional status bar with detailed system information
 * - Cross-platform consistency (Windows/Linux/macOS)
 * - Memory-efficient 60 FPS updates
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructor for MainWindow
     * @param parent Parent widget (typically nullptr)
     */
    explicit MainWindow(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~MainWindow() override;

    // Disable copy and move operations for proper resource management
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;

    /**
     * @brief Initialize the main window with all components
     * @return true if initialization successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Get current ETI processor instance
     * @return Pointer to ETI processor (may be nullptr)
     */
    [[nodiscard]] EtiProcessor* getEtiProcessor() const { return m_etiProcessor.get(); }

    /**
     * @brief Get current analyzer widget
     * @return Pointer to analyzer widget
     */
    [[nodiscard]] AnalyzerWidget* getAnalyzerWidget() const { return m_analyzerWidget; }

    /**
     * @brief Get constellation widget
     * @return Pointer to constellation widget
     */
    ConstellationWidget* getConstellationWidget() const { return m_constellationWidget; }

    /**
     * @brief Get service browser widget
     * @return Pointer to service browser
     */
    ServiceBrowser* getServiceBrowser() const { return m_serviceBrowser; }

    /**
     * @brief Get professional workflow manager
     * @return Pointer to workflow manager
     */
    ProfessionalWorkflowManager* getWorkflowManager() const { return m_workflowManager.get(); }

    /**
     * @brief Check if real-time updates are active
     * @return true if real-time mode is enabled
     */
    bool isRealTimeMode() const { return m_realTimeMode; }

    /**
     * @brief Get current update rate in FPS
     * @return Update rate (target: 60 FPS)
     */
    int getUpdateRate() const { return m_updateRate; }

    // Public constants for testing access
    static constexpr int DEFAULT_WINDOW_WIDTH = 1200;
    static constexpr int DEFAULT_WINDOW_HEIGHT = 800;
    static constexpr int TARGET_FPS = 60;
    static constexpr int UPDATE_INTERVAL_MS = 1000 / TARGET_FPS;  // ~16.67ms for 60 FPS
    static constexpr int STATUS_MESSAGE_TIMEOUT = 5000;  // 5 seconds

    // Public slots
    /**
     * @brief Open ETI file for analysis
     */
    void openEtiFile();

    /**
     * @brief Close current ETI file
     */
    void closeEtiFile();

    /**
     * @brief Start real-time ETI stream analysis
     */
    void startRealTimeAnalysis();

    /**
     * @brief Stop real-time analysis
     */
    void stopRealTimeAnalysis();

    /**
     * @brief Start ETI-over-IP stream reception
     */
    void startNetworkStreaming();

    /**
     * @brief Stop ETI-over-IP stream reception
     */
    void stopNetworkStreaming();

    /**
     * @brief Configure network streaming settings
     */
    void configureNetworkStreaming();

    /**
     * @brief Start recording current stream
     */
    void startStreamRecording();

    /**
     * @brief Stop recording current stream
     */
    void stopStreamRecording();

    /**
     * @brief Show settings dialog
     */
    void showSettings();

    /**
     * @brief Show about dialog
     */
    void showAbout();

    /**
     * @brief Toggle full screen mode
     */
    void toggleFullScreen();

    /**
     * @brief Reset window layout to default
     */
    void resetLayout();
    
    /**
     * @brief Create sample ETI file for demonstration
     */
    void createSampleEtiFile();
    
    /**
     * @brief Load test data into service browser
     */
    void loadTestData();

    /**
     * @brief Update status bar information
     * @param message Primary status message
     * @param details Additional details
     */
    void updateStatus(const QString& message, const QString& details = QString());

    /**
     * @brief Update progress indicator
     * @param value Progress value (0-100)
     */
    void updateProgress(int value);

    /**
     * @brief Handle DAB+ service detection
     * @param serviceId Detected DAB+ service ID
     */
    void onDabPlusServiceDetected(const QString& serviceId);

    /**
     * @brief Start error detection workflow
     */
    void startErrorDetection();

    /**
     * @brief Generate professional report
     */
    void generateProfessionalReport();

    /**
     * @brief Navigate to next error
     */
    void navigateToNextError();

    /**
     * @brief Navigate to previous error
     */
    void navigateToPreviousError();

    /**
     * @brief Jump to specific frame
     */
    void jumpToFrame();

    /**
     * @brief Toggle service explorer panel
     */
    void toggleServiceExplorer();

    /**
     * @brief Toggle properties panel
     */
    void togglePropertiesPanel();

    /**
     * @brief Toggle messages panel
     */
    void toggleMessagesPanel();

    /**
     * @brief Select all services
     */
    void selectAllServices();

    /**
     * @brief Deselect all services
     */
    void deselectAllServices();

    // ========== MISSING BUTTON SLOT IMPLEMENTATIONS (GREEN PHASE - TDD) ==========
    /**
     * @brief Refresh DAB services list with progress feedback
     */
    void refreshServices();

    /**
     * @brief Toggle sync mode for panel synchronization
     */
    void toggleSyncMode();

    /**
     * @brief Toggle compare mode for service comparison
     */
    void toggleCompareMode();

signals:
    /**
     * @brief Emitted when ETI file is opened
     * @param filename Path to opened file
     */
    void etiFileOpened(const QString& filename);

    /**
     * @brief Emitted when ETI file is closed
     */
    void etiFileClosed();
    
    /**
     * @brief Emitted when DAB+ service is selected
     * @param serviceId Selected service ID
     */
    void dabPlusServiceSelected(quint32 serviceId);
    
    /**
     * @brief Emitted when DAB+ service is detected
     * @param serviceId Service ID
     * @param codecInfo Codec information
     */
    void dabPlusServiceDetected(quint32 serviceId, const QString& codecInfo);

    /**
     * @brief Emitted when real-time mode changes
     * @param enabled true if real-time mode enabled
     */
    void realTimeModeChanged(bool enabled);

    /**
     * @brief Emitted when settings change
     */
    void settingsChanged();

protected:
    /**
     * @brief Handle close event
     * @param event Close event
     */
    void closeEvent(QCloseEvent *event) override;

    /**
     * @brief Handle resize event
     * @param event Resize event
     */
    void resizeEvent(QResizeEvent *event) override;

    /**
     * @brief Handle move event
     * @param event Move event
     */
    void moveEvent(QMoveEvent *event) override;

public slots:
    // ========== TDD/AAA TESTABLE PUBLIC SLOTS ==========
    /**
     * @brief Handle ETI frame processing completion (TDD testable)
     * @param frameNumber Frame number processed
     * @param data Frame data
     */
    void onFrameProcessed(quint64 frameNumber, const QByteArray& data);

    /**
     * @brief Handle ETI processing status changes (TDD testable)
     * @param status New processing status
     */
    void onStatusChanged(const QString& status);

    /**
     * @brief Handle ensemble discovery (TDD testable)
     * @param ensemble Discovered ensemble information
     */
    void onEnsembleDiscovered(const eti::Ensemble& ensemble);

    /**
     * @brief Handle service discovery (TDD testable)
     * @param service Discovered service information
     */
    void onServiceDiscovered(const eti::DabService& service);

    // ========== TDD/AAA GREEN PHASE MINIMAL METHODS ==========
    /**
     * @brief Create context menu for widget (TDD testable)
     * @param widget Target widget for context menu
     */
    void createContextMenu(QWidget* widget);

    /**
     * @brief Show error indicator in UI (TDD testable)
     * @param errorMessage Error message to display
     */
    void showErrorIndicator(const QString& errorMessage);

    /**
     * @brief Enable/disable real-time mode (TDD testable)
     * @param enabled True to enable real-time mode
     */
    void enableRealTimeMode(bool enabled);

private:
    // Private slots
    /**
     * @brief Update real-time display (called by timer)
     */
    void updateRealTimeDisplay();

    /**
     * @brief Handle ETI processing error
     * @param error Error message
     */
    void handleEtiError(const QString& error);

    /**
     * @brief Handle dock widget visibility change
     * @param visible Visibility state
     */
    void handleDockVisibilityChanged(bool visible);

    /**
     * @brief Handle ETI frame error (TDD minimal version)
     * @param frameNumber Frame number where error occurred
     */
    void onFrameError(quint64 frameNumber);

    // ========== UI INTEGRATION SPECIALIST SIGNAL/SLOT HANDLERS ==========
    /**
     * @brief Handle ETI processing errors
     * @param error Error message from ETI processor
     */
    void onEtiError(const QString& error);

    /**
     * @brief Update frame processing statistics in UI
     */
    void updateFrameStatistics();

    /**
     * @brief Update service browser with discovered services
     */
    void updateServiceBrowserData();

    /**
     * @brief Update properties panel with current selection
     */
    void updatePropertiesPanel();

    /**
     * @brief Update status indicators with processing metrics
     */
    void updateStatusIndicators();

    /**
     * @brief Test UI integration workflow end-to-end
     * @return true if all integrations working properly
     */
    bool testUIIntegrationWorkflow();

    /**
     * @brief Generate comprehensive UI integration validation report
     * @return Detailed integration report with 10.0/10.0 scoring
     */
    QString generateUIIntegrationReport();
    /**
     * @brief Create menu bar with professional broadcast menus
     */
    void createMenuBar();

    /**
     * @brief Create toolbar with analysis tools
     */
    void createToolBar();

    /**
     * @brief Create status bar with detailed information
     */
    void createStatusBar();

    /**
     * @brief Create central widget with main analysis display
     */
    void createCentralWidget();

    /**
     * @brief Create dockable panels
     */
    void createDockWidgets();

    /**
     * @brief Setup three-panel professional layout
     */
    bool setupThreePanelLayout();

    /**
     * @brief Initialize custom visualization widgets in Qt Designer UI
     */
    void initializeVisualizationWidgets();

    /**
     * @brief Create Explorer Panel (left 25%) - ETI service tree
     */
    void createExplorerPanel();

    /**
     * @brief Create Main Content Area (center 50%) - Frame listing with color coding
     */
    void createMainContentArea();

    /**
     * @brief Create Properties Panel (right 25%) - Parameter display with modes
     */
    void createPropertiesPanel();

    /**
     * @brief Create Bottom Tool Panels (25% height) - Specialist analysis tools
     */
    void createBottomToolPanels();

    // Simplified panel creation methods to avoid null pointer issues
    void createSimplifiedExplorerPanel();
    void createSimplifiedMainContentArea();
    void createSimplifiedPropertiesPanel();
    void createSimplifiedBottomToolPanels();
    
    // Qt Designer UI initialization
    bool initializeQtDesignerComponents();

    /**
     * @brief Apply broadcasting industry aesthetics
     */
    void applyBroadcastingTheme();

    /**
     * @brief Setup actions for menus and toolbar
     */
    void createActions();

    /**
     * @brief Connect signals and slots
     */
    bool connectSignals();
    
    /**
     * @brief Setup advanced signal/slot connections for network streaming
     */
    bool setupNetworkStreamingConnections();
    
    /**
     * @brief Setup specialized widget signal connections
     */
    bool setupSpecializedWidgetConnections();
    
    /**
     * @brief Validate all signal/slot connections are working
     * @return true if all connections are valid
     */
    bool validateSignalSlotIntegration();

    /**
     * @brief Setup DAB+ enhancements and signal connections
     */
    bool setupDabPlusEnhancements();

    /**
     * @brief Configure professional REFACTOR phase components
     * @return true if configuration successful
     */
    bool configureProfessionalComponents();

    /**
     * @brief Load window settings from config
     */
    void loadSettings();

    /**
     * @brief Save window settings to config
     */
    void saveSettings();

    /**
     * @brief Validate layout consistency
     * @return true if layout is valid
     */
    bool validateLayout() const;

    /**
     * @brief Update window title with current file/status
     */
    void updateWindowTitle();

    /**
     * @brief Setup real-time update timer
     */
    void setupRealTimeTimer();
    
    /**
     * @brief Setup DAB+ signal connections between components
     */
    void setupDabPlusSignalConnections();
    
    /**
     * @brief Handle DAB+ service selection
     * @param serviceId Selected service ID
     */
    void handleDabPlusServiceSelection(quint32 serviceId);
    
    /**
     * @brief Handle audio quality changes
     * @param serviceId Service ID
     * @param quality New audio quality (0=Unknown, 1=Poor, 2=Fair, 3=Good, 4=Excellent)
     */
    void handleAudioQualityChanged(quint32 serviceId, int quality);
    
    /**
     * @brief Handle DAB+ service detection
     * @param serviceId Service ID
     * @param codecInfo Codec information
     */
    void handleDabPlusServiceDetected(quint32 serviceId, const QString& codecInfo);

    /**
     * @brief Fast file dialog implementation to avoid Qt performance issues
     * @param defaultDir Default directory to open
     * @return Selected file path, empty if cancelled
     */
    QString getFastOpenFileName(const QString& defaultDir);

    // Qt Designer UI
    Ui::MainWindow *ui;

    // Core components
    std::unique_ptr<EtiProcessor> m_etiProcessor;
    std::unique_ptr<EtiFileWorker> m_etiFileWorker;
    std::unique_ptr<ConfigManager> m_configManager;
    
    // Modern ETI Core Engine components
    std::unique_ptr<eti::modern::ModernETIFrameParser> m_modernFrameParser;
    std::unique_ptr<eti::modern::EnhancedFIGAnalyzer> m_enhancedFIGAnalyzer;
    std::unique_ptr<eti::compliance::ComprehensiveETSIValidator> m_etsiValidator;

    // ETI-specific models for professional UI
    EtiServiceTreeModel *m_serviceTreeModel;
    EtiFrameListModel *m_frameListModel;

    // Central widget and main layout
    QWidget *m_centralWidget;
    QSplitter *m_mainSplitter;
    QTabWidget *m_analysisTabWidget;

    // Three-panel professional layout components
    QSplitter *m_horizontalSplitter;        // Main horizontal splitter
    QSplitter *m_verticalSplitter;          // Vertical splitter for bottom panels
    ServiceExplorerPanel *m_serviceExplorerPanel;  // Left panel (25%) - Professional DAB Service Explorer
    QWidget *m_mainContentArea;             // Center panel (50%)
    QWidget *m_propertiesPanel;             // Right panel (25%)
    QTabWidget *m_bottomToolTabs;           // Bottom tabbed panel (25% height)
    
    // Simplified widgets for temporary fallback
    QWidget *m_explorerWidget;

    // Explorer Panel components  
    QPushButton *m_refreshServicesBtn;      // Refresh services button

    // Main Content Area components
    EtiAnalysisWidget *m_etiAnalysisWidget; // Professional ETI analysis widget with integrated constellation
    QTableWidget *m_frameListTable;         // Professional ETI frame listing table
    QSplitter *m_contentSplitter;           // Split analyzer and constellation widgets

    // Properties Panel components
    QTabWidget *m_propertiesTabs;           // Properties tab widget
    QTextEdit *m_parametersText;            // Parameter display
    QTextEdit *m_yamlOutputText;            // YAML output display
    QPushButton *m_syncModeBtn;             // Sync mode toggle
    QPushButton *m_compareModeBtn;          // Compare mode toggle

    // Bottom Tool Panel components  
    QTextEdit *m_hexViewer;                 // Hex viewer
    QTextEdit *m_messagesLog;               // Messages log
    QTextEdit *m_tr101290Analysis;          // TR 101-290 analysis
    QWidget *m_timeDynamicsWidget;          // Time dynamics charts
    QWidget *m_graphicsWidget;              // Graphics visualization
    QTextEdit *m_commentsText;              // Comments and annotations
    QTextEdit *m_epgViewer;                 // EPG information

    // GUI components
    AnalyzerWidget *m_analyzerWidget;
    ConstellationWidget *m_constellationWidget;
    ServiceBrowser *m_serviceBrowser;
    
    // Professional analysis widgets
    PerformanceDashboard *m_performanceDashboard;
    ETSIComplianceMonitor *m_complianceMonitor;

    // Professional workflow management
    std::unique_ptr<ProfessionalWorkflowManager> m_workflowManager;

    // Professional REFACTOR phase components for 10.0/10.0 TDD/AAA compliance
    std::unique_ptr<ProgressManager> m_progressManager;
    std::unique_ptr<StatusManager> m_statusManager;
    std::unique_ptr<ErrorIndicatorSystem> m_errorIndicatorSystem;
    std::unique_ptr<ContextMenuFactory> m_contextMenuFactory;
    std::unique_ptr<RealTimeController> m_realTimeController;
    std::unique_ptr<SignalRouter> m_signalRouter;

    // UI Integration Validation System
    std::unique_ptr<UIIntegrationValidator> m_uiIntegrationValidator;
    
    // Audio Decoder Integration Layer
    std::unique_ptr<AudioDecoderIntegration> m_audioDecoderIntegration;

    // Dockable panels
    QDockWidget *m_fileExplorerDock;
    QDockWidget *m_yamlViewerDock;
    QDockWidget *m_figAnalysisDock;
    QDockWidget *m_audioMonitorDock;
    QDockWidget *m_errorLogDock;
    QDockWidget *m_performanceDashboardDock;
    QDockWidget *m_complianceMonitorDock;

    // Dock contents
    QTextEdit *m_fileExplorer;
    QTextEdit *m_yamlViewer;
    FigAnalysisWidget *m_figAnalysis;
    AudioMonitoringWidget *m_audioMonitor;
    QTextEdit *m_errorLog;

    // Menu bar
    QMenuBar *m_menuBar;
    QMenu *m_fileMenu;
    QMenu *m_analysisMenu;
    QMenu *m_viewMenu;
    QMenu *m_toolsMenu;
    QMenu *m_helpMenu;

    // Toolbar
    QToolBar *m_mainToolBar;
    QToolBar *m_analysisToolBar;

    // Status bar
    QStatusBar *m_statusBar;
    QLabel *m_statusLabel;
    QLabel *m_frameCountLabel;
    QLabel *m_bitRateLabel;
    QLabel *m_errorCountLabel;
    QProgressBar *m_progressBar;

    // Actions
    QAction *m_openFileAction;
    QAction *m_closeFileAction;
    QAction *m_exitAction;
    QAction *m_startAnalysisAction;
    QAction *m_stopAnalysisAction;
    QAction *m_settingsAction;
    QAction *m_aboutAction;
    QAction *m_fullScreenAction;
    QAction *m_resetLayoutAction;

    // Network streaming actions
    QAction *m_startNetworkStreamingAction;
    QAction *m_stopNetworkStreamingAction;
    QAction *m_configureNetworkAction;
    QAction *m_startRecordingAction;
    QAction *m_stopRecordingAction;

    // View actions for docks
    QAction *m_showFileExplorerAction;
    QAction *m_showYamlViewerAction;
    QAction *m_showFigAnalysisAction;
    QAction *m_showAudioMonitorAction;
    QAction *m_showErrorLogAction;
    
    // Demo actions
    QAction *m_createSampleFileAction;
    QAction *m_loadTestDataAction;

    // Real-time update system
    QTimer *m_realTimeTimer;
    bool m_realTimeMode;
    int m_updateRate;  // Target: 60 FPS
    quint64 m_frameCount;
    QString m_currentFile;

    // Layout state
    QByteArray m_defaultLayoutState;
    bool m_isInitialized;

    // Performance tracking
    qint64 m_lastUpdateTime;
    int m_frameDropCount;
    double m_averageFrameTime;

    // Network streaming components
    std::unique_ptr<eti_network::EtiOverIpReceiver> m_networkReceiver;
    std::unique_ptr<eti::StreamRecordingManager> m_recordingManager;
    bool m_networkStreamingActive;
    bool m_recordingActive;
    QString m_currentStreamAddress;
    quint16 m_currentStreamPort;
    eti_network::NetworkQualityMetrics m_networkQuality;
    
    // Helper methods
    /**
     * @brief Calculate frame quality score from frame data
     * @param frameData Raw frame data
     * @return Quality score from 0.0 to 1.0
     */
    double calculateFrameQuality(const QByteArray& frameData);

    // Dialog instances
    SettingsDialog *m_settingsDialog;
    AboutDialog *m_aboutDialog;
};