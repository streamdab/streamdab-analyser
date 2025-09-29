#ifndef DAB_ANALYZER_MAIN_WINDOW_H
#define DAB_ANALYZER_MAIN_WINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QToolBar>
#include <QtCore/QTimer>
#include <memory>

// Include ETI types and processors
#include "../core/eti_types.h"
#include "../core/enhanced_eti_processor_qt.h"
#include "../core/advanced_fig_analyzer.h"

/**
 * @brief DAB Analyzer Professional Main Window
 *
 * Implements comprehensive DAB Analyzer interface pattern with:
 * - Ensemble Tree Explorer (left panel) with checkbox selection
 * - Frame Navigator with selectable frame list (center panel)
 * - Service Details & Signal Quality (right panel)
 * - FIC Analysis Hub with FIC-Analyzer/FIC-Extractor tabs (bottom panel)
 *
 * Reference: DAB Analyzer Application manual Figures 1, 10, 14, 15
 */
class DABAnalyzerMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DABAnalyzerMainWindow(QWidget *parent = nullptr);
    ~DABAnalyzerMainWindow();

    // File operations following DAB Analyzer patterns
    bool openETIFile(const QString& filePath);
    void closeCurrentFile();

    // Real-time streaming support
    void startRealTimeMode();
    void stopRealTimeMode();

public slots:
    // ETI Processing slots (compatible with EnhancedETIProcessorQt)
    void onFrameProcessedQt(const ProcessedFrame& frame);
    void onProcessingError(const QString& error);
    void onProcessingProgress(int percentage, int currentFrame, int totalFrames);
    void onProcessingComplete(int totalFrames, int processingTimeMs);

    // Advanced FIG Analyzer slots (Phase 1.4)
    void onServiceDiscoveredFIG(const ServiceInfo& service);
    void onEnsembleUpdatedFIG(const EnsembleInfo& ensemble);
    void onServiceLabelUpdated(uint32_t serviceId, const QString& label);
    void onFIGProcessed(const FIGData& figData);
    void onFIGError(uint8_t figType, uint8_t extension, const QString& error);
    void onProgrammeTypeUpdated(uint32_t serviceId, uint8_t ptyCode);
    void onTimeInfoUpdated(const QDateTime& currentTime);

    // Legacy slots for compatibility with ETI types
    void onFrameProcessed(const ETIFrame& frame);
    void onServiceDiscovered(const ServiceInfo& service);
    void onEnsembleUpdated(const EnsembleInfo& ensemble);

private slots:
    // Panel interaction slots
    void onEnsembleItemChanged(QTreeWidgetItem* item, int column);
    void onServiceSelectionChanged();
    void onFrameSelectionChanged();
    void onFIGTypeToggled(bool enabled);

    // File menu actions
    void openFile();
    void exportAnalysis();
    void showSettings();
    void showAbout();

    // View menu actions
    void toggleFrameNavigator();
    void toggleServiceDetails();
    void toggleFICAnalysis();

    // Stream menu actions
    void connectETIStream();
    void recordStream();
    void pauseProcessing();

    // Update timers
    void updateStatusBar();
    void updateSignalQuality();

private:
    // Core setup methods
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupSplitters();
    void setupSignalConnections();
    void applyDABAnalyzerTheme();

    // Panel creation methods
    QWidget* createEnsembleTreePanel();
    QWidget* createFrameNavigatorPanel();
    QWidget* createServiceDetailsPanel();
    QWidget* createFICAnalysisPanel();

    // Helper methods
    void populateEnsembleTree(const EnsembleInfo& ensemble);
    void updateFrameList();
    void updateServiceDetails(const ServiceInfo& service);
    void updateFICAnalysis();
    void updateSelectedFrameDetails(const ETIFrame& frame);
    void updateFrameContentDisplay(const ETIFrame& frame);
    void updateServiceFiltering();

    // Theme and styling
    void setupDABAnalyzerColors();
    QString getServiceTypeIcon(const QString& serviceType) const;
    QColor getServiceTypeColor(const QString& serviceType) const;

private:
    // Core components
    std::unique_ptr<EnhancedETIProcessorQt> m_etiProcessor;
    std::unique_ptr<AdvancedFIGAnalyzer> m_figAnalyzer;

    // Main layout components
    QSplitter* m_mainSplitter;
    QSplitter* m_horizontalSplitter;
    QSplitter* m_verticalSplitter;

    // Left Panel: Ensemble Tree Explorer
    QWidget* m_ensemblePanel;
    QTreeWidget* m_ensembleTree;
    QGroupBox* m_frameDetailsGroup;
    QLabel* m_selectedFrameLabel;
    QLabel* m_frameTimeLabel;
    QLabel* m_frameSizeLabel;
    QLabel* m_frameFormatLabel;
    QLabel* m_frameSyncStatus;
    QLabel* m_frameCRCStatus;

    // Center Panel: Frame Navigator & Content Viewer
    QWidget* m_frameNavigatorPanel;
    QListWidget* m_frameList;
    QTabWidget* m_frameContentTabs;
    QTextEdit* m_hexViewer;
    QTextEdit* m_mscViewer;
    QTextEdit* m_ficViewer;

    // Right Panel: Service Details & Signal Quality
    QWidget* m_serviceDetailsPanel;
    QGroupBox* m_currentServiceGroup;
    QLabel* m_serviceSIDLabel;
    QLabel* m_serviceTypeLabel;
    QLabel* m_serviceBitrateLabel;
    QLabel* m_serviceProtectionLabel;
    QLabel* m_serviceSubChannelLabel;
    QGroupBox* m_signalQualityGroup;
    QLabel* m_signalStrengthLabel;
    QLabel* m_snrLabel;
    QLabel* m_frequencyErrorLabel;
    QProgressBar* m_qualityMeter;

    // Bottom Panel: FIC Analysis Hub
    QWidget* m_ficAnalysisPanel;
    QTabWidget* m_ficTabs;
    QWidget* m_ficAnalyzerTab;
    QWidget* m_ficExtractorTab;
    QWidget* m_systemMessagesTab;

    // FIC-Analyzer components
    QCheckBox* m_figType00CheckBox;
    QCheckBox* m_figType01CheckBox;
    QCheckBox* m_figType02CheckBox;
    QCheckBox* m_figType10CheckBox;
    QCheckBox* m_figType11CheckBox;
    QCheckBox* m_figType14CheckBox;

    // FIC-Extractor components
    QTextEdit* m_serviceLabelsText;
    QTextEdit* m_programmeInfoText;
    QLabel* m_timeDisplayLabel;
    QLabel* m_dateDisplayLabel;

    // System Messages components
    QTextEdit* m_systemMessages;
    QLabel* m_processingRateLabel;
    QLabel* m_memoryUsageLabel;
    QLabel* m_networkStatusLabel;

    // Menu and toolbar
    QMenuBar* m_menuBar;
    QToolBar* m_toolBar;
    QStatusBar* m_statusBar;

    // Status and update timers
    QTimer* m_statusUpdateTimer;
    QTimer* m_qualityUpdateTimer;

    // Current state
    QString m_currentFilePath;
    bool m_isRealTimeMode;
    int m_currentFrameIndex;
    int m_totalFrames;
    QList<ETIFrame> m_frames;
    QList<ServiceInfo> m_services;
    EnsembleInfo m_currentEnsemble;

    // Performance metrics
    double m_processingFPS;
    double m_memoryUsageMB;
    bool m_isConnected;
};

#endif // DAB_ANALYZER_MAIN_WINDOW_H
