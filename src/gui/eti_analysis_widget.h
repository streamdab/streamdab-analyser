#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableView>
#include <QTreeWidget>
#include <QLabel>
#include <QFrame>
#include <QProgressBar>
#include <QTimer>
#include <QMutex>
#include <QPainter>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QTextEdit>
#include <QScrollArea>
#include <memory>
#include "core/eti_types.h"

// Forward declarations
class EtiProcessor;
class ConstellationWidget;
class EtiFrameListModel;

/**
 * @class EtiAnalysisWidget
 * @brief Professional ETI frame analysis display widget for broadcast industry
 * 
 * This widget provides comprehensive ETI stream analysis with professional
 * broadcast industry standards, featuring:
 * - Real-time ETI frame analysis with color-coded status
 * - Integrated signal constellation visualization
 * - Live performance metrics dashboard
 * - Professional frame listing with detailed information
 * - Real-time statistics with 7,482+ FPS capability
 * - ETSI compliance monitoring
 * - Error detection and reporting
 * - Professional broadcast theme integration
 */
class EtiAnalysisWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Frame display mode enumeration
     */
    enum class FrameDisplayMode {
        FrameList,      ///< Frame list view
        FrameDetails,   ///< Detailed frame view
        Timeline,       ///< Timeline view
        Packets         ///< Packet view
    };

    /**
     * @brief Analysis mode enumeration
     */
    enum class AnalysisMode {
        RealTime,       ///< Real-time analysis
        Playback,       ///< File playback analysis
        Comparison      ///< Comparison mode
    };

    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit EtiAnalysisWidget(QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~EtiAnalysisWidget();

    /**
     * @brief Initialize the ETI analysis widget
     * @return true if successful
     */
    bool initialize();

    /**
     * @brief Set ETI processor for data connection
     * @param processor ETI processor instance
     */
    void setEtiProcessor(EtiProcessor *processor);

    /**
     * @brief Set frame display mode
     * @param mode Display mode to set
     */
    void setFrameDisplayMode(FrameDisplayMode mode);

    /**
     * @brief Get current frame display mode
     * @return Current display mode
     */
    FrameDisplayMode getFrameDisplayMode() const { return m_frameDisplayMode; }

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
     * @brief Start real-time analysis
     */
    void startRealTimeAnalysis();

    /**
     * @brief Stop real-time analysis
     */
    void stopRealTimeAnalysis();

    /**
     * @brief Get current frame count
     * @return Total processed frames
     */
    quint64 getFrameCount() const { return m_frameCount; }

    /**
     * @brief Get current processing rate
     * @return Processing rate in FPS
     */
    double getProcessingRate() const { return m_processingRate; }

    /**
     * @brief Get current error count
     * @return Total error count
     */
    quint32 getErrorCount() const { return m_errorCount; }

    /**
     * @brief Get signal quality
     * @return Signal quality percentage (0-100)
     */
    double getSignalQuality() const { return m_signalQuality; }

public slots:
    /**
     * @brief Update analysis display
     */
    void updateAnalysisDisplay();

    /**
     * @brief Clear analysis data
     */
    void clearAnalysisData();

    /**
     * @brief Handle frame data update
     * @param frameData New frame data
     */
    void handleFrameDataUpdate(const QVariantMap& frameData);

    /**
     * @brief Handle frame selection change
     * @param frameIndex Selected frame index
     */
    void handleFrameSelection(quint64 frameIndex);

    /**
     * @brief Update statistics dashboard
     */
    void updateStatisticsDashboard();

signals:
    /**
     * @brief Emitted when frame is selected
     * @param frameIndex Selected frame index
     */
    void frameSelected(quint64 frameIndex);

    /**
     * @brief Emitted when display mode changes
     * @param mode New display mode
     */
    void displayModeChanged(FrameDisplayMode mode);

    /**
     * @brief Emitted when analysis mode changes
     * @param mode New analysis mode
     */
    void analysisModeChanged(AnalysisMode mode);

    /**
     * @brief Emitted when error is detected
     * @param errorType Error type
     * @param description Error description
     */
    void errorDetected(const QString& errorType, const QString& description);

    /**
     * @brief Emitted when performance metrics update
     * @param metrics Performance metrics map
     */
    void performanceMetricsUpdated(const QVariantMap& metrics);

protected:
    /**
     * @brief Handle paint events
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
     * @brief Handle frame display mode change
     * @param index Mode index
     */
    void handleFrameDisplayModeChange(int index);

    /**
     * @brief Handle constellation data update
     */
    void handleConstellationUpdate();
    
    /**
     * @brief Handle frame processing from ETI processor
     * @param frameNumber Frame number
     * @param data Frame data
     */
    void onFrameProcessed(quint64 frameNumber, const QByteArray& data);
    
    /**
     * @brief Handle ensemble discovery from ETI processor
     * @param ensemble Discovered ensemble information
     */
    void onEnsembleDiscovered(const eti::Ensemble& ensemble);
    
    /**
     * @brief Handle status change from ETI processor
     * @param status New status message
     */
    void onStatusChanged(const QString& status);
    
    /**
     * @brief Handle error from ETI processor
     * @param error Error message
     */
    void onErrorOccurred(const QString& error);

private:
    /**
     * @brief Create the main layout structure
     */
    void createMainLayout();

    /**
     * @brief Create control header
     */
    void createControlHeader();

    /**
     * @brief Create frame analysis section
     */
    void createFrameAnalysisSection();

    /**
     * @brief Create statistics dashboard
     */
    void createStatisticsDashboard();

    /**
     * @brief Create constellation integration
     */
    void createConstellationIntegration();

    /**
     * @brief Setup professional theme
     */
    void setupProfessionalTheme();

    /**
     * @brief Setup real-time timer
     */
    void setupRealTimeTimer();

    /**
     * @brief Connect internal signals
     */
    void connectInternalSignals();

    /**
     * @brief Create frame list table
     */
    void createFrameListTable();

    /**
     * @brief Create detailed frame view
     */
    void createDetailedFrameView();

    /**
     * @brief Create timeline view
     */
    void createTimelineView();

    /**
     * @brief Create packet view
     */
    void createPacketView();

    /**
     * @brief Update frame list display
     */
    void updateFrameList();

    /**
     * @brief Update frame details
     */
    void updateFrameDetails();

    /**
     * @brief Update timeline display
     */
    void updateTimelineDisplay();

    /**
     * @brief Update packet display
     */
    void updatePacketDisplay();

    /**
     * @brief Create color-coded frame item
     * @param frameData Frame data
     * @param row Table row
     */
    void createColorCodedFrameItem(const QVariantMap& frameData, int row);

    /**
     * @brief Calculate performance metrics
     */
    void calculatePerformanceMetrics();

    /**
     * @brief Update constellation display
     */
    void updateConstellationDisplay();

    /**
     * @brief Validate widget state
     * @return true if state is valid
     */
    bool validateState() const;

    // Core components
    EtiProcessor *m_etiProcessor;
    ConstellationWidget *m_constellationWidget;
    EtiFrameListModel *m_frameListModel;

    // Main layout
    QVBoxLayout *m_mainLayout;
    QSplitter *m_mainSplitter;
    QSplitter *m_contentSplitter;

    // Control header
    QFrame *m_controlHeader;
    QLabel *m_headerLabel;
    QLabel *m_statsLabel;
    QComboBox *m_displayModeCombo;
    QComboBox *m_analysisModeCombo;
    QPushButton *m_startStopButton;

    // Frame analysis section
    QTabWidget *m_analysisTabWidget;
    QTableWidget *m_frameListTable;
    QTextEdit *m_frameDetailsText;
    QWidget *m_timelineWidget;
    QWidget *m_packetWidget;

    // Statistics dashboard
    QGroupBox *m_statisticsGroup;
    QGridLayout *m_statisticsLayout;
    QLabel *m_frameCountLabel;
    QLabel *m_processingRateLabel;
    QLabel *m_errorCountLabel;
    QLabel *m_signalQualityLabel;
    QProgressBar *m_cpuUsageBar;
    QProgressBar *m_memoryUsageBar;
    QProgressBar *m_signalQualityBar;

    // Constellation integration
    QGroupBox *m_constellationGroup;
    QVBoxLayout *m_constellationLayout;

    // Legend and color coding
    QFrame *m_legendFrame;
    QHBoxLayout *m_legendLayout;

    // Real-time system
    QTimer *m_realTimeTimer;
    QMutex m_dataMutex;
    bool m_realTimeEnabled;
    bool m_isInitialized;

    // Display modes
    FrameDisplayMode m_frameDisplayMode;
    AnalysisMode m_analysisMode;

    // Performance metrics
    quint64 m_frameCount;
    double m_processingRate;
    quint32 m_errorCount;
    double m_signalQuality;
    double m_cpuUsage;
    double m_memoryUsage;

    // Frame data storage
    QVector<QVariantMap> m_frameData;
    quint64 m_selectedFrameIndex;

    // Timing
    qint64 m_lastUpdateTime;
    qint64 m_lastFrameTime;
    QVector<qint64> m_frameTimes;

    // Constants
    static constexpr int UPDATE_INTERVAL_MS = 16;  // ~60 FPS
    static constexpr int MAX_FRAME_HISTORY = 10000;
    static constexpr int PERFORMANCE_SAMPLE_SIZE = 100;
    static constexpr double TARGET_FPS = 7482.0;
};