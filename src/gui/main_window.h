#pragma once

#include <QMainWindow>
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
#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QMoveEvent>
#include <memory>

// Forward declarations
class AnalyzerWidget;
class ConstellationWidget;
class ServiceBrowser;
class SettingsDialog;
class AboutDialog;
class EtiProcessor;
class ConfigManager;

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
    ~MainWindow();

    /**
     * @brief Initialize the main window with all components
     * @return true if initialization successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Get current ETI processor instance
     * @return Pointer to ETI processor (may be nullptr)
     */
    EtiProcessor* getEtiProcessor() const { return m_etiProcessor.get(); }

    /**
     * @brief Get current analyzer widget
     * @return Pointer to analyzer widget
     */
    AnalyzerWidget* getAnalyzerWidget() const { return m_analyzerWidget; }

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

public slots:
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

private slots:
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

private:
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
     * @brief Setup actions for menus and toolbar
     */
    void createActions();

    /**
     * @brief Connect signals and slots
     */
    void connectSignals();

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

    // Core components
    std::unique_ptr<EtiProcessor> m_etiProcessor;
    std::unique_ptr<ConfigManager> m_configManager;

    // Central widget and main layout
    QWidget *m_centralWidget;
    QSplitter *m_mainSplitter;
    QTabWidget *m_analysisTabWidget;

    // GUI components
    AnalyzerWidget *m_analyzerWidget;
    ConstellationWidget *m_constellationWidget;
    ServiceBrowser *m_serviceBrowser;

    // Dockable panels
    QDockWidget *m_fileExplorerDock;
    QDockWidget *m_yamlViewerDock;
    QDockWidget *m_figAnalysisDock;
    QDockWidget *m_audioMonitorDock;
    QDockWidget *m_errorLogDock;

    // Dock contents
    QTextEdit *m_fileExplorer;
    QTextEdit *m_yamlViewer;
    QTextEdit *m_figAnalysis;
    QTextEdit *m_audioMonitor;
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

    // View actions for docks
    QAction *m_showFileExplorerAction;
    QAction *m_showYamlViewerAction;
    QAction *m_showFigAnalysisAction;
    QAction *m_showAudioMonitorAction;
    QAction *m_showErrorLogAction;

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

    // Dialog instances
    SettingsDialog *m_settingsDialog;
    AboutDialog *m_aboutDialog;
};