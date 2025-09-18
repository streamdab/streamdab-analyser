#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QGroupBox>
#include <QTabWidget>
#include <QScrollArea>
#include <QTextEdit>
#include <QTableWidget>
#include <QTreeWidget>
#include <QListWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QSlider>
#include <QDial>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QMutex>
#include <QThread>
#include <memory>

// Forward declarations
class EtiProcessor;
class DabDecoder;
class ConstellationWidget;

/**
 * @class AnalyzerWidget
 * @brief Professional ETI stream analysis display widget
 * 
 * This widget provides comprehensive real-time analysis of ETI streams with
 * professional broadcast industry visualization standards. Features include:
 * - Real-time ETI frame analysis and visualization
 * - Service and ensemble information display
 * - Audio level monitoring and quality metrics
 * - Signal constellation and spectrum analysis
 * - Error detection and logging
 * - Performance monitoring (60 FPS updates)
 * - Memory-efficient data handling for continuous operation
 */
class AnalyzerWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Analysis mode enumeration
     */
    enum class AnalysisMode {
        File,           ///< File-based analysis
        RealTime,       ///< Real-time stream analysis
        Network,        ///< Network stream analysis
        Playback        ///< Playback with controls
    };

    /**
     * @brief Display mode enumeration
     */
    enum class DisplayMode {
        Overview,       ///< High-level overview
        Detailed,       ///< Detailed technical view
        Monitoring,     ///< Monitoring dashboard
        Debug          ///< Debug and diagnostic view
    };

    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit AnalyzerWidget(QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~AnalyzerWidget();

    /**
     * @brief Initialize the analyzer widget
     * @return true if successful
     */
    bool initialize();

    /**
     * @brief Set ETI processor for analysis
     * @param processor ETI processor instance
     */
    void setEtiProcessor(EtiProcessor *processor);

    /**
     * @brief Set analysis mode
     * @param mode Analysis mode to set
     */
    void setAnalysisMode(AnalysisMode mode);

    /**
     * @brief Get current analysis mode
     * @return Current analysis mode
     */
    AnalysisMode getAnalysisMode() const { return m_analysisMode; }

    /**
     * @brief Set display mode
     * @param mode Display mode to set
     */
    void setDisplayMode(DisplayMode mode);

    /**
     * @brief Get current display mode
     * @return Current display mode
     */
    DisplayMode getDisplayMode() const { return m_displayMode; }

    /**
     * @brief Start analysis
     * @return true if started successfully
     */
    bool startAnalysis();

    /**
     * @brief Stop analysis
     */
    void stopAnalysis();

    /**
     * @brief Check if analysis is running
     * @return true if analysis is active
     */
    bool isAnalysisRunning() const { return m_analysisRunning; }

    /**
     * @brief Get current frame rate
     * @return Current update frame rate
     */
    double getCurrentFrameRate() const { return m_currentFrameRate; }

    /**
     * @brief Get total processed frames
     * @return Number of processed frames
     */
    quint64 getProcessedFrames() const { return m_processedFrames; }

    /**
     * @brief Get error count
     * @return Number of detected errors
     */
    quint32 getErrorCount() const { return m_errorCount; }

public slots:
    /**
     * @brief Update display with new ETI data
     */
    void updateDisplay();

    /**
     * @brief Clear all display data
     */
    void clearDisplay();

    /**
     * @brief Reset analysis statistics
     */
    void resetStatistics();

    /**
     * @brief Export analysis data
     */
    void exportData();

    /**
     * @brief Take screenshot of analysis
     */
    void takeScreenshot();

signals:
    /**
     * @brief Emitted when analysis starts
     */
    void analysisStarted();

    /**
     * @brief Emitted when analysis stops
     */
    void analysisStopped();

    /**
     * @brief Emitted when error is detected
     * @param error Error description
     */
    void errorDetected(const QString& error);

    /**
     * @brief Emitted when frame rate changes
     * @param frameRate New frame rate
     */
    void frameRateChanged(double frameRate);

    /**
     * @brief Emitted when statistics update
     * @param frames Total frames processed
     * @param errors Total errors detected
     */
    void statisticsUpdated(quint64 frames, quint32 errors);

protected:
    /**
     * @brief Handle paint events for custom drawing
     * @param event Paint event
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief Handle resize events
     * @param event Resize event
     */
    void resizeEvent(QResizeEvent *event) override;

private slots:
    /**
     * @brief Handle real-time update timer
     */
    void handleRealTimeUpdate();

    /**
     * @brief Handle analysis mode change
     */
    void handleAnalysisModeChanged();

    /**
     * @brief Handle display mode change
     */
    void handleDisplayModeChanged();

    /**
     * @brief Handle ETI processor data
     */
    void handleEtiData();

    /**
     * @brief Handle service selection change
     * @param serviceId Selected service ID
     */
    void handleServiceSelectionChanged(int serviceId);

private:
    /**
     * @brief Create the UI layout
     */
    void createUI();

    /**
     * @brief Create overview tab
     */
    void createOverviewTab();

    /**
     * @brief Create detailed analysis tab
     */
    void createDetailedTab();

    /**
     * @brief Create monitoring tab
     */
    void createMonitoringTab();

    /**
     * @brief Create debug tab
     */
    void createDebugTab();

    /**
     * @brief Create control panel
     */
    void createControlPanel();

    /**
     * @brief Setup real-time timer
     */
    void setupRealTimeTimer();

    /**
     * @brief Update overview display
     */
    void updateOverviewDisplay();

    /**
     * @brief Update detailed display
     */
    void updateDetailedDisplay();

    /**
     * @brief Update monitoring display
     */
    void updateMonitoringDisplay();

    /**
     * @brief Update debug display
     */
    void updateDebugDisplay();

    /**
     * @brief Update service information
     */
    void updateServiceInfo();

    /**
     * @brief Update ensemble information
     */
    void updateEnsembleInfo();

    /**
     * @brief Update audio information
     */
    void updateAudioInfo();

    /**
     * @brief Update error statistics
     */
    void updateErrorStats();

    /**
     * @brief Calculate frame rate
     */
    void calculateFrameRate();

    /**
     * @brief Update control state
     */
    void updateControlState();

    /**
     * @brief Validate widget state
     * @return true if state is valid
     */
    bool validateState() const;

    /**
     * @brief Update frame rate display
     */
    void updateFrameRateDisplay();

    /**
     * @brief Add debug message to debug log
     * @param component Component name
     * @param message Debug message
     */
    void addDebugMessage(const QString& component, const QString& message);

    // Core components
    EtiProcessor *m_etiProcessor;
    DabDecoder *m_dabDecoder;

    // Main layout
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_controlLayout;

    // Tab widget for different views
    QTabWidget *m_tabWidget;

    // Overview tab
    QWidget *m_overviewTab;
    QGroupBox *m_ensembleGroup;
    QGroupBox *m_servicesGroup;
    QGroupBox *m_statusGroup;
    QLabel *m_ensembleLabel;
    QLabel *m_ensembleId;
    QLabel *m_ensembleName;
    QTableWidget *m_servicesTable;
    QLabel *m_statusLabel;
    QProgressBar *m_signalQuality;

    // Detailed tab
    QWidget *m_detailedTab;
    QGroupBox *m_frameGroup;
    QGroupBox *m_figGroup;
    QGroupBox *m_audioGroup;
    QTreeWidget *m_frameTree;
    QTreeWidget *m_figTree;
    QTableWidget *m_audioTable;

    // Monitoring tab
    QWidget *m_monitoringTab;
    QGroupBox *m_realTimeGroup;
    QGroupBox *m_statisticsGroup;
    QLabel *m_frameRateLabel;
    QLabel *m_bitRateLabel;
    QLabel *m_errorRateLabel;
    QProgressBar *m_cpuUsage;
    QProgressBar *m_memoryUsage;

    // Debug tab
    QWidget *m_debugTab;
    QTextEdit *m_debugLog;
    QTableWidget *m_debugTable;

    // Control panel
    QGroupBox *m_controlGroup;
    QComboBox *m_analysisModeCombo;
    QComboBox *m_displayModeCombo;
    QPushButton *m_startButton;
    QPushButton *m_stopButton;
    QPushButton *m_clearButton;
    QPushButton *m_exportButton;

    // Analysis state
    AnalysisMode m_analysisMode;
    DisplayMode m_displayMode;
    bool m_analysisRunning;
    bool m_isInitialized;

    // Real-time update system
    QTimer *m_updateTimer;
    QMutex m_dataMutex;

    // Statistics
    quint64 m_processedFrames;
    quint32 m_errorCount;
    double m_currentFrameRate;
    qint64 m_lastUpdateTime;
    QList<qint64> m_frameTimestamps;

    // Performance tracking
    double m_averageProcessingTime;
    double m_maxProcessingTime;
    int m_frameDropCount;

    // Constants
    static constexpr int UPDATE_INTERVAL_MS = 16;  // ~60 FPS
    static constexpr int FRAME_RATE_SAMPLES = 30;  // For average calculation
    static constexpr int MAX_DEBUG_LINES = 1000;   // Maximum debug log lines
    static constexpr int SERVICE_TABLE_COLUMNS = 6; // Service table columns
};