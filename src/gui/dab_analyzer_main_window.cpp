#include "dab_analyzer_main_window.h"
#include "../core/enhanced_eti_processor_qt.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QHeaderView>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QTime>
#include <QtGui/QFont>
#include <QtGui/QFontMetrics>

// DAB Analyzer Professional Theme Colors
namespace DABAnalyzerTheme {
    const QColor BACKGROUND_LIGHT(0xF5, 0xF5, 0xF5);    // #F5F5F5
    const QColor PANEL_BACKGROUND(0xFF, 0xFF, 0xFF);    // #FFFFFF
    const QColor BORDER_COLOR(0xCC, 0xCC, 0xCC);        // #CCCCCC
    const QColor ACCENT_BLUE(0x00, 0x66, 0xCC);         // #0066CC
    const QColor SUCCESS_GREEN(0x00, 0x99, 0x00);       // #009900
    const QColor WARNING_AMBER(0xFF, 0x99, 0x00);       // #FF9900
    const QColor ERROR_RED(0xCC, 0x00, 0x00);           // #CC0000
    const QColor SERVICE_AUDIO(0x00, 0x99, 0x00);       // Green
    const QColor SERVICE_DATA(0xFF, 0x66, 0x00);        // Orange
    const QColor SERVICE_ENSEMBLE(0x00, 0x66, 0xCC);    // Blue
    const QColor SELECTED_ITEM(0xE6, 0xF2, 0xFF);       // Light Blue
    const QColor TEXT_PRIMARY(0x00, 0x00, 0x00);        // Black
    const QColor TEXT_SECONDARY(0x66, 0x66, 0x66);      // Dark Gray
}

DABAnalyzerMainWindow::DABAnalyzerMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_etiProcessor(std::make_unique<EnhancedETIProcessorQt>())
    , m_figAnalyzer(std::make_unique<AdvancedFIGAnalyzer>())
    , m_mainSplitter(nullptr)
    , m_horizontalSplitter(nullptr)
    , m_verticalSplitter(nullptr)
    , m_isRealTimeMode(false)
    , m_currentFrameIndex(0)
    , m_totalFrames(0)
    , m_processingFPS(0.0)
    , m_memoryUsageMB(0.0)
    , m_isConnected(false)
{
    setWindowTitle("StreamDAB Stream Analyzer - DAB Analyzer Professional");
    setMinimumSize(1200, 800);
    resize(1400, 1000);

    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupSignalConnections();
    applyDABAnalyzerTheme();

    // Initialize update timers
    m_statusUpdateTimer = new QTimer(this);
    m_qualityUpdateTimer = new QTimer(this);

    connect(m_statusUpdateTimer, &QTimer::timeout, this, &DABAnalyzerMainWindow::updateStatusBar);
    connect(m_qualityUpdateTimer, &QTimer::timeout, this, &DABAnalyzerMainWindow::updateSignalQuality);

    m_statusUpdateTimer->start(1000);  // Update every second
    m_qualityUpdateTimer->start(500);  // Update twice per second

    // Initialize ETI processor
    // Note: EnhancedETIProcessorQt doesn't have setRealTimeMode method
}

DABAnalyzerMainWindow::~DABAnalyzerMainWindow() = default;

void DABAnalyzerMainWindow::setupUI()
{
    // Create main central widget with splitter layout
    m_mainSplitter = new QSplitter(Qt::Vertical, this);
    setCentralWidget(m_mainSplitter);

    // Create horizontal splitter for top panels
    m_horizontalSplitter = new QSplitter(Qt::Horizontal);
    m_mainSplitter->addWidget(m_horizontalSplitter);

    // Create and add panels to horizontal splitter
    m_ensemblePanel = createEnsembleTreePanel();
    m_frameNavigatorPanel = createFrameNavigatorPanel();
    m_serviceDetailsPanel = createServiceDetailsPanel();

    m_horizontalSplitter->addWidget(m_ensemblePanel);
    m_horizontalSplitter->addWidget(m_frameNavigatorPanel);
    m_horizontalSplitter->addWidget(m_serviceDetailsPanel);

    // Set horizontal panel sizes (25%, 50%, 25%)
    m_horizontalSplitter->setSizes({280, 600, 320});
    m_horizontalSplitter->setStretchFactor(0, 0);  // Fixed left panel
    m_horizontalSplitter->setStretchFactor(1, 1);  // Expandable center
    m_horizontalSplitter->setStretchFactor(2, 0);  // Fixed right panel

    // Create bottom panel for FIC analysis
    m_ficAnalysisPanel = createFICAnalysisPanel();
    m_mainSplitter->addWidget(m_ficAnalysisPanel);

    // Set vertical panel sizes (70% top, 30% bottom)
    m_mainSplitter->setSizes({700, 300});
    m_mainSplitter->setStretchFactor(0, 1);  // Expandable top
    m_mainSplitter->setStretchFactor(1, 0);  // Fixed bottom height
}

QWidget* DABAnalyzerMainWindow::createEnsembleTreePanel()
{
    QWidget* panel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(10);

    // Ensemble Tree section
    QLabel* treeLabel = new QLabel("Ensemble Tree");
    treeLabel->setFont(QFont("Arial", 12, QFont::Bold));
    layout->addWidget(treeLabel);

    m_ensembleTree = new QTreeWidget();
    m_ensembleTree->setHeaderLabels({"Service", "Type", "SID"});
    m_ensembleTree->header()->setStretchLastSection(false);
    m_ensembleTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_ensembleTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_ensembleTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_ensembleTree->setAlternatingRowColors(true);
    // Note: Checkboxes will be set on individual items using setCheckState()
    layout->addWidget(m_ensembleTree);

    // Frame Details section
    m_frameDetailsGroup = new QGroupBox("Frame Details");
    QVBoxLayout* frameLayout = new QVBoxLayout(m_frameDetailsGroup);

    m_selectedFrameLabel = new QLabel("Selected: -");
    m_frameTimeLabel = new QLabel("Time: -");
    m_frameSizeLabel = new QLabel("Size: -");
    m_frameFormatLabel = new QLabel("Format: -");
    m_frameSyncStatus = new QLabel("SYNC: -");
    m_frameCRCStatus = new QLabel("CRC: -");

    frameLayout->addWidget(m_selectedFrameLabel);
    frameLayout->addWidget(m_frameTimeLabel);
    frameLayout->addWidget(m_frameSizeLabel);
    frameLayout->addWidget(m_frameFormatLabel);
    frameLayout->addWidget(m_frameSyncStatus);
    frameLayout->addWidget(m_frameCRCStatus);
    frameLayout->addStretch();

    layout->addWidget(m_frameDetailsGroup);
    layout->addStretch();

    return panel;
}

QWidget* DABAnalyzerMainWindow::createFrameNavigatorPanel()
{
    QWidget* panel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(10);

    // Frame List section
    QLabel* frameLabel = new QLabel("Frame Navigator");
    frameLabel->setFont(QFont("Arial", 12, QFont::Bold));
    layout->addWidget(frameLabel);

    m_frameList = new QListWidget();
    m_frameList->setAlternatingRowColors(true);
    m_frameList->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_frameList, 1);  // Give it more space

    // Frame Content Display section
    QLabel* contentLabel = new QLabel("Frame Content Display");
    contentLabel->setFont(QFont("Arial", 11, QFont::Bold));
    layout->addWidget(contentLabel);

    m_frameContentTabs = new QTabWidget();

    // Hex Viewer tab
    m_hexViewer = new QTextEdit();
    m_hexViewer->setFont(QFont("Courier New", 9));
    m_hexViewer->setReadOnly(true);
    m_frameContentTabs->addTab(m_hexViewer, "Hex/MSC Data");

    // MSC Data tab
    m_mscViewer = new QTextEdit();
    m_mscViewer->setFont(QFont("Courier New", 9));
    m_mscViewer->setReadOnly(true);
    m_frameContentTabs->addTab(m_mscViewer, "MSC Content");

    // FIC Data tab
    m_ficViewer = new QTextEdit();
    m_ficViewer->setFont(QFont("Courier New", 9));
    m_ficViewer->setReadOnly(true);
    m_frameContentTabs->addTab(m_ficViewer, "FIC Data");

    layout->addWidget(m_frameContentTabs, 1);  // Give it equal space with frame list

    return panel;
}

QWidget* DABAnalyzerMainWindow::createServiceDetailsPanel()
{
    QWidget* panel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(10);

    // Current Service section
    m_currentServiceGroup = new QGroupBox("Current Service");
    QVBoxLayout* serviceLayout = new QVBoxLayout(m_currentServiceGroup);

    m_serviceSIDLabel = new QLabel("SID: -");
    m_serviceTypeLabel = new QLabel("Type: -");
    m_serviceBitrateLabel = new QLabel("Bitrate: -");
    m_serviceProtectionLabel = new QLabel("Protection: -");
    m_serviceSubChannelLabel = new QLabel("SubCh: -");

    serviceLayout->addWidget(m_serviceSIDLabel);
    serviceLayout->addWidget(m_serviceTypeLabel);
    serviceLayout->addWidget(m_serviceBitrateLabel);
    serviceLayout->addWidget(m_serviceProtectionLabel);
    serviceLayout->addWidget(m_serviceSubChannelLabel);

    layout->addWidget(m_currentServiceGroup);

    // Signal Quality section
    m_signalQualityGroup = new QGroupBox("Signal Quality");
    QVBoxLayout* qualityLayout = new QVBoxLayout(m_signalQualityGroup);

    m_signalStrengthLabel = new QLabel("Signal: -");
    m_snrLabel = new QLabel("SNR: -");
    m_frequencyErrorLabel = new QLabel("Frequency Error: -");

    m_qualityMeter = new QProgressBar();
    m_qualityMeter->setRange(0, 100);
    m_qualityMeter->setValue(0);
    m_qualityMeter->setTextVisible(true);
    m_qualityMeter->setFormat("Quality: %p%");

    qualityLayout->addWidget(m_signalStrengthLabel);
    qualityLayout->addWidget(m_snrLabel);
    qualityLayout->addWidget(m_frequencyErrorLabel);
    qualityLayout->addWidget(m_qualityMeter);

    layout->addWidget(m_signalQualityGroup);
    layout->addStretch();

    return panel;
}

QWidget* DABAnalyzerMainWindow::createFICAnalysisPanel()
{
    QWidget* panel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);

    // FIC Analysis Hub with tabs
    m_ficTabs = new QTabWidget();

    // FIC-Analyzer tab
    m_ficAnalyzerTab = new QWidget();
    QVBoxLayout* xplorerLayout = new QVBoxLayout(m_ficAnalyzerTab);

    QLabel* figLabel = new QLabel("FIG Analysis");
    figLabel->setFont(QFont("Arial", 11, QFont::Bold));
    xplorerLayout->addWidget(figLabel);

    // Create FIG type checkboxes
    m_figType00CheckBox = new QCheckBox("FIG 0/0 - Ensemble Organization");
    m_figType01CheckBox = new QCheckBox("FIG 0/1 - Subchannel Organization");
    m_figType02CheckBox = new QCheckBox("FIG 0/2 - Service Organization");
    m_figType10CheckBox = new QCheckBox("FIG 1/0 - Service Labels");
    m_figType11CheckBox = new QCheckBox("FIG 1/1 - Programme Information");
    m_figType14CheckBox = new QCheckBox("FIG 1/4 - Service Component Labels");

    // Set default selections
    m_figType00CheckBox->setChecked(true);
    m_figType01CheckBox->setChecked(true);
    m_figType02CheckBox->setChecked(true);
    m_figType10CheckBox->setChecked(true);

    xplorerLayout->addWidget(m_figType00CheckBox);
    xplorerLayout->addWidget(m_figType01CheckBox);
    xplorerLayout->addWidget(m_figType02CheckBox);
    xplorerLayout->addWidget(m_figType10CheckBox);
    xplorerLayout->addWidget(m_figType11CheckBox);
    xplorerLayout->addWidget(m_figType14CheckBox);
    xplorerLayout->addStretch();

    m_ficTabs->addTab(m_ficAnalyzerTab, "FIC-Analyzer");

    // FIC-Extractor tab
    m_ficExtractorTab = new QWidget();
    QVBoxLayout* xtractorLayout = new QVBoxLayout(m_ficExtractorTab);

    QLabel* extractedLabel = new QLabel("Extracted Information");
    extractedLabel->setFont(QFont("Arial", 11, QFont::Bold));
    xtractorLayout->addWidget(extractedLabel);

    // Service Labels section
    QGroupBox* labelsGroup = new QGroupBox("Service Labels");
    QVBoxLayout* labelsLayout = new QVBoxLayout(labelsGroup);
    m_serviceLabelsText = new QTextEdit();
    m_serviceLabelsText->setMaximumHeight(80);
    m_serviceLabelsText->setReadOnly(true);
    labelsLayout->addWidget(m_serviceLabelsText);
    xtractorLayout->addWidget(labelsGroup);

    // Programme Information section
    QGroupBox* programmeGroup = new QGroupBox("Programme Information");
    QVBoxLayout* programmeLayout = new QVBoxLayout(programmeGroup);
    m_programmeInfoText = new QTextEdit();
    m_programmeInfoText->setMaximumHeight(60);
    m_programmeInfoText->setReadOnly(true);
    m_timeDisplayLabel = new QLabel("Time: -");
    m_dateDisplayLabel = new QLabel("Date: -");
    programmeLayout->addWidget(m_programmeInfoText);
    programmeLayout->addWidget(m_timeDisplayLabel);
    programmeLayout->addWidget(m_dateDisplayLabel);
    xtractorLayout->addWidget(programmeGroup);

    m_ficTabs->addTab(m_ficExtractorTab, "FIC-Extractor");

    // System Messages tab
    m_systemMessagesTab = new QWidget();
    QVBoxLayout* messagesLayout = new QVBoxLayout(m_systemMessagesTab);

    QLabel* messagesLabel = new QLabel("Warnings & Errors");
    messagesLabel->setFont(QFont("Arial", 11, QFont::Bold));
    messagesLayout->addWidget(messagesLabel);

    m_systemMessages = new QTextEdit();
    m_systemMessages->setReadOnly(true);
    messagesLayout->addWidget(m_systemMessages);

    // Status indicators
    m_processingRateLabel = new QLabel("📊 Rate: 0 FPS");
    m_memoryUsageLabel = new QLabel("💾 Memory: 0 MB");
    m_networkStatusLabel = new QLabel("🌐 Network: Offline");

    messagesLayout->addWidget(m_processingRateLabel);
    messagesLayout->addWidget(m_memoryUsageLabel);
    messagesLayout->addWidget(m_networkStatusLabel);

    m_ficTabs->addTab(m_systemMessagesTab, "System Messages");

    layout->addWidget(m_ficTabs);

    return panel;
}

void DABAnalyzerMainWindow::setupMenuBar()
{
    m_menuBar = menuBar();

    // File menu
    QMenu* fileMenu = m_menuBar->addMenu("&File");
    fileMenu->addAction("&Open ETI File...", this, &DABAnalyzerMainWindow::openFile, QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction("&Export Analysis...", this, &DABAnalyzerMainWindow::exportAnalysis, QKeySequence("Ctrl+E"));
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QWidget::close, QKeySequence::Quit);

    // View menu
    QMenu* viewMenu = m_menuBar->addMenu("&View");
    viewMenu->addAction("Toggle &Frame Navigator", this, &DABAnalyzerMainWindow::toggleFrameNavigator);
    viewMenu->addAction("Toggle &Service Details", this, &DABAnalyzerMainWindow::toggleServiceDetails);
    viewMenu->addAction("Toggle &FIC Analysis", this, &DABAnalyzerMainWindow::toggleFICAnalysis);

    // Stream menu
    QMenu* streamMenu = m_menuBar->addMenu("&Stream");
    streamMenu->addAction("&Connect ETI Stream...", this, &DABAnalyzerMainWindow::connectETIStream);
    streamMenu->addAction("&Record Stream...", this, &DABAnalyzerMainWindow::recordStream);
    streamMenu->addSeparator();
    streamMenu->addAction("&Pause Processing", this, &DABAnalyzerMainWindow::pauseProcessing);

    // Tools menu
    QMenu* toolsMenu = m_menuBar->addMenu("&Tools");
    toolsMenu->addAction("&Settings...", this, &DABAnalyzerMainWindow::showSettings);
    toolsMenu->addAction("🔧 &FIC-Analyzer", [this]() { m_ficTabs->setCurrentIndex(0); });

    // Help menu
    QMenu* helpMenu = m_menuBar->addMenu("&Help");
    helpMenu->addAction("&About StreamDAB Analyzer...", this, &DABAnalyzerMainWindow::showAbout);
}

void DABAnalyzerMainWindow::setupToolBar()
{
    m_toolBar = addToolBar("Main Toolbar");
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    m_toolBar->addAction("📁 Open", this, &DABAnalyzerMainWindow::openFile);
    m_toolBar->addAction("▶️ Stream", this, &DABAnalyzerMainWindow::connectETIStream);
    m_toolBar->addAction("⏹️ Stop", this, &DABAnalyzerMainWindow::stopRealTimeMode);
    m_toolBar->addAction("🔴 Record", this, &DABAnalyzerMainWindow::recordStream);
    m_toolBar->addSeparator();
    m_toolBar->addAction("⚙️ Settings", this, &DABAnalyzerMainWindow::showSettings);
    m_toolBar->addAction("📊 Export", this, &DABAnalyzerMainWindow::exportAnalysis);
    m_toolBar->addAction("🔧 FIC-Analyzer", [this]() { m_ficTabs->setCurrentIndex(0); });
}

void DABAnalyzerMainWindow::setupStatusBar()
{
    m_statusBar = statusBar();
    m_statusBar->showMessage("🟢 Ready");
}

void DABAnalyzerMainWindow::setupSignalConnections()
{
    // ETI Processor connections
    connect(m_etiProcessor.get(), &EnhancedETIProcessorQt::frameProcessed,
            this, &DABAnalyzerMainWindow::onFrameProcessedQt);
    connect(m_etiProcessor.get(), &EnhancedETIProcessorQt::processingError,
            this, &DABAnalyzerMainWindow::onProcessingError);
    
    // Advanced FIG Analyzer connections (Phase 1.4)
    connect(m_figAnalyzer.get(), &AdvancedFIGAnalyzer::serviceDiscovered,
            this, &DABAnalyzerMainWindow::onServiceDiscoveredFIG);
    connect(m_figAnalyzer.get(), &AdvancedFIGAnalyzer::ensembleUpdated,
            this, &DABAnalyzerMainWindow::onEnsembleUpdatedFIG);
    connect(m_figAnalyzer.get(), &AdvancedFIGAnalyzer::serviceLabelUpdated,
            this, &DABAnalyzerMainWindow::onServiceLabelUpdated);
    connect(m_figAnalyzer.get(), &AdvancedFIGAnalyzer::figProcessed,
            this, &DABAnalyzerMainWindow::onFIGProcessed);
    connect(m_figAnalyzer.get(), &AdvancedFIGAnalyzer::figError,
            this, &DABAnalyzerMainWindow::onFIGError);
    connect(m_figAnalyzer.get(), &AdvancedFIGAnalyzer::programmeTypeUpdated,
            this, &DABAnalyzerMainWindow::onProgrammeTypeUpdated);
    connect(m_figAnalyzer.get(), &AdvancedFIGAnalyzer::timeInfoUpdated,
            this, &DABAnalyzerMainWindow::onTimeInfoUpdated);

    // UI element connections
    connect(m_ensembleTree, &QTreeWidget::itemChanged,
            this, &DABAnalyzerMainWindow::onEnsembleItemChanged);
    connect(m_ensembleTree, &QTreeWidget::itemSelectionChanged,
            this, &DABAnalyzerMainWindow::onServiceSelectionChanged);
    connect(m_frameList, &QListWidget::itemSelectionChanged,
            this, &DABAnalyzerMainWindow::onFrameSelectionChanged);

    // FIG type checkbox connections
    connect(m_figType00CheckBox, &QCheckBox::toggled, this, &DABAnalyzerMainWindow::onFIGTypeToggled);
    connect(m_figType01CheckBox, &QCheckBox::toggled, this, &DABAnalyzerMainWindow::onFIGTypeToggled);
    connect(m_figType02CheckBox, &QCheckBox::toggled, this, &DABAnalyzerMainWindow::onFIGTypeToggled);
    connect(m_figType10CheckBox, &QCheckBox::toggled, this, &DABAnalyzerMainWindow::onFIGTypeToggled);
    connect(m_figType11CheckBox, &QCheckBox::toggled, this, &DABAnalyzerMainWindow::onFIGTypeToggled);
    connect(m_figType14CheckBox, &QCheckBox::toggled, this, &DABAnalyzerMainWindow::onFIGTypeToggled);
}

void DABAnalyzerMainWindow::applyDABAnalyzerTheme()
{
    // EMERGENCY: ALL CUSTOM THEMING DISABLED FOR TEXT VISIBILITY
    // Using pure Qt system default theme for maximum compatibility
    // This ensures text is always visible regardless of system settings
    
    // Clear any existing custom stylesheets to revert to system defaults
    setStyleSheet("");
    
    // Ensure all child widgets also use system defaults
    QList<QWidget*> allWidgets = findChildren<QWidget*>();
    for (QWidget* widget : allWidgets) {
        if (widget) {
            widget->setStyleSheet("");
        }
    }
    
    // Let the operating system handle all styling for guaranteed visibility
    // Qt will automatically use appropriate colors for dark/light themes
}

// File Operations
bool DABAnalyzerMainWindow::openETIFile(const QString& filePath)
{
    if (filePath.isEmpty()) return false;

    try {
        // Process ETI file with enhanced processor
        bool success = m_etiProcessor->processETIFile(filePath);
        if (success) {
            m_currentFilePath = filePath;

            // Update status bar
            QString fileName = QFileInfo(filePath).fileName();
            m_statusBar->showMessage(QString("🟢 Ready | File: %1").arg(fileName));

            // Clear and populate frame list
            updateFrameList();

            return true;
        } else {
            QMessageBox::warning(this, "Error", "Failed to process ETI file");
            return false;
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", QString("Error processing file: %1").arg(e.what()));
        return false;
    }
}

void DABAnalyzerMainWindow::updateFrameList()
{
    m_frameList->clear();

    // Note: Will be populated as frames are processed
    m_totalFrames = m_frames.size();

    // Populate frame list with 24ms precision
    for (int i = 0; i < m_frames.size(); ++i) {
        const auto& frame = m_frames[i];
        QString frameText = QString("#%1 [%2ms]")
            .arg(i + 1, 3, 10, QChar('0'))
            .arg(i * 24);  // 24ms per frame

        QListWidgetItem* item = new QListWidgetItem(frameText);
        item->setData(Qt::UserRole, i);  // Store frame index
        m_frameList->addItem(item);
    }

    // Select first frame by default
    if (m_frameList->count() > 0) {
        m_frameList->setCurrentRow(0);
    }
}

// New slot implementations for EnhancedETIProcessorQt compatibility
void DABAnalyzerMainWindow::onFrameProcessedQt(const ProcessedFrame& frame)
{
    // Convert ProcessedFrame to ETIFrame for internal use
    ETIFrame etiFrame;
    etiFrame.frameNumber = frame.frame_number;
    etiFrame.format = frame.format == ETIFormat::ETI_NI ? "ETI-NI" :
                     frame.format == ETIFormat::ETI_LI_A ? "ETI-LI-A" :
                     frame.format == ETIFormat::ETI_LI_B ? "ETI-LI-B" : "UNKNOWN";
    etiFrame.syncValid = frame.is_valid;
    etiFrame.crcValid = frame.is_valid;
    etiFrame.timestamp = frame.frame_number * 24; // 24ms per frame
    etiFrame.ficContent = QString::fromUtf8(frame.fic_data);
    etiFrame.mscContent = QString::fromUtf8(frame.msc_data);

    // Add to our internal frame list
    m_frames.append(etiFrame);

    // Update frame list display
    QString frameText = QString("#%1 [%2ms]")
        .arg(frame.frame_number, 3, 10, QChar('0'))
        .arg(frame.frame_number * 24);

    QListWidgetItem* item = new QListWidgetItem(frameText);
    item->setData(Qt::UserRole, m_frames.size() - 1);
    m_frameList->addItem(item);

    // Auto-scroll to latest frame
    m_frameList->scrollToBottom();

    // Update processing rate display
    m_processingFPS = 900.0; // Placeholder value
    
    // Extract FIC data for advanced analysis (Phase 1.4)
    if (!frame.fic_data.isEmpty()) {
        m_figAnalyzer->analyzeFICData(frame.fic_data);
    }
}

void DABAnalyzerMainWindow::onProcessingProgress(int percentage, int currentFrame, int totalFrames)
{
    Q_UNUSED(percentage)
    m_currentFrameIndex = currentFrame - 1; // Convert to 0-based index
    m_totalFrames = totalFrames;

    // Update status bar with progress
    QString status = QString("🔄 Processing | Frame %1/%2 | %3%")
        .arg(currentFrame)
        .arg(totalFrames)
        .arg(percentage);
    m_statusBar->showMessage(status);
}

void DABAnalyzerMainWindow::onProcessingComplete(int totalFrames, int processingTimeMs)
{
    m_totalFrames = totalFrames;

    // Update status bar with completion
    QString status = QString("✅ Complete | %1 frames processed in %2ms")
        .arg(totalFrames)
        .arg(processingTimeMs);
    m_statusBar->showMessage(status, 3000);

    // Select first frame if available
    if (m_frameList->count() > 0) {
        m_frameList->setCurrentRow(0);
    }
}

// Legacy slot implementations for compatibility
void DABAnalyzerMainWindow::onFrameProcessed(const ETIFrame& frame)
{
    // Add frame to our list
    m_frames.append(frame);

    // Update frame list if in real-time mode
    if (m_isRealTimeMode) {
        QString frameText = QString("#%1 [%2ms]")
            .arg(m_frames.size(), 3, 10, QChar('0'))
            .arg((m_frames.size() - 1) * 24);

        QListWidgetItem* item = new QListWidgetItem(frameText);
        item->setData(Qt::UserRole, m_frames.size() - 1);
        m_frameList->addItem(item);

        // Auto-scroll to latest frame
        m_frameList->scrollToBottom();
    }
}

void DABAnalyzerMainWindow::onServiceDiscovered(const ServiceInfo& service)
{
    m_services.append(service);

    // Add service to ensemble tree
    QTreeWidgetItem* serviceItem = new QTreeWidgetItem();
    serviceItem->setText(0, service.label);
    serviceItem->setText(1, service.type);
    serviceItem->setText(2, QString("0x%1").arg(service.serviceId, 0, 16));
    serviceItem->setCheckState(0, Qt::Checked);  // Default to checked

    // Set service type icon and color
    serviceItem->setForeground(1, QBrush(getServiceTypeColor(service.type)));

    // Add to ensemble tree (find or create ensemble item)
    QTreeWidgetItem* ensembleItem = nullptr;
    for (int i = 0; i < m_ensembleTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_ensembleTree->topLevelItem(i);
        if (item->text(2) == QString("0x%1").arg(service.ensembleId, 0, 16)) {
            ensembleItem = item;
            break;
        }
    }

    if (!ensembleItem) {
        ensembleItem = new QTreeWidgetItem();
        ensembleItem->setText(0, QString("Ensemble 0x%1").arg(service.ensembleId, 0, 16));
        ensembleItem->setText(1, "Ensemble");
        ensembleItem->setText(2, QString("0x%1").arg(service.ensembleId, 0, 16));
        ensembleItem->setCheckState(0, Qt::Checked);
        ensembleItem->setForeground(1, QBrush(DABAnalyzerTheme::SERVICE_ENSEMBLE));
        m_ensembleTree->addTopLevelItem(ensembleItem);
        ensembleItem->setExpanded(true);
    }

    ensembleItem->addChild(serviceItem);
}

void DABAnalyzerMainWindow::onEnsembleUpdated(const EnsembleInfo& ensemble)
{
    m_currentEnsemble = ensemble;

    // Update ensemble information in tree
    populateEnsembleTree(ensemble);
}

void DABAnalyzerMainWindow::onProcessingError(const QString& error)
{
    // Add error to system messages
    QString timestamp = QTime::currentTime().toString();
    QString errorMsg = QString("[%1] 🔴 Error: %2").arg(timestamp, error);

    m_systemMessages->append(errorMsg);

    // Show in status bar temporarily
    m_statusBar->showMessage(QString("🔴 Error: %1").arg(error), 5000);
}

// Advanced FIG Analyzer slot implementations (Phase 1.4)
void DABAnalyzerMainWindow::onServiceDiscoveredFIG(const ServiceInfo& service)
{
    // Add service to ensemble tree with enhanced information
    QTreeWidgetItem* serviceItem = new QTreeWidgetItem();
    serviceItem->setText(0, service.label.isEmpty() ? QString("Service 0x%1").arg(service.serviceId, 4, 16, QChar('0')) : service.label);
    serviceItem->setText(1, service.type);
    serviceItem->setText(2, QString("0x%1").arg(service.serviceId, 4, 16, QChar('0')));
    serviceItem->setCheckState(0, Qt::Checked);
    
    // Set service type icon and color
    serviceItem->setForeground(1, QBrush(getServiceTypeColor(service.type)));
    
    // Find or create ensemble item
    QTreeWidgetItem* ensembleItem = nullptr;
    for (int i = 0; i < m_ensembleTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_ensembleTree->topLevelItem(i);
        if (item->text(2) == QString("0x%1").arg(service.ensembleId, 4, 16, QChar('0'))) {
            ensembleItem = item;
            break;
        }
    }
    
    if (!ensembleItem) {
        ensembleItem = new QTreeWidgetItem();
        ensembleItem->setText(0, QString("Ensemble 0x%1").arg(service.ensembleId, 4, 16, QChar('0')));
        ensembleItem->setText(1, "Ensemble");
        ensembleItem->setText(2, QString("0x%1").arg(service.ensembleId, 4, 16, QChar('0')));
        ensembleItem->setCheckState(0, Qt::Checked);
        ensembleItem->setForeground(1, QBrush(DABAnalyzerTheme::SERVICE_ENSEMBLE));
        m_ensembleTree->addTopLevelItem(ensembleItem);
        ensembleItem->setExpanded(true);
    }
    
    ensembleItem->addChild(serviceItem);
    
    // Update internal service list
    m_services.append(service);
}

void DABAnalyzerMainWindow::onEnsembleUpdatedFIG(const EnsembleInfo& ensemble)
{
    m_currentEnsemble = ensemble;
    
    // Update ensemble label in tree if available
    for (int i = 0; i < m_ensembleTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_ensembleTree->topLevelItem(i);
        if (item->text(2) == QString("0x%1").arg(ensemble.ensembleId, 4, 16, QChar('0'))) {
            if (!ensemble.ensembleLabel.isEmpty()) {
                item->setText(0, ensemble.ensembleLabel);
            }
            break;
        }
    }
    
    // Update status bar with ensemble information
    QString status = QString("🟢 Ensemble: %1 | Services: %2 | Country: %3")
        .arg(ensemble.ensembleLabel.isEmpty() ? QString("0x%1").arg(ensemble.ensembleId, 4, 16, QChar('0')) : ensemble.ensembleLabel)
        .arg(ensemble.serviceCount)
        .arg(ensemble.countryCode);
    m_statusBar->showMessage(status);
}

void DABAnalyzerMainWindow::onServiceLabelUpdated(uint32_t serviceId, const QString& label)
{
    // Update service label in ensemble tree
    for (int i = 0; i < m_ensembleTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* ensembleItem = m_ensembleTree->topLevelItem(i);
        for (int j = 0; j < ensembleItem->childCount(); ++j) {
            QTreeWidgetItem* serviceItem = ensembleItem->child(j);
            QString sidText = serviceItem->text(2);
            uint32_t sid = sidText.mid(2).toUInt(nullptr, 16);
            if (sid == serviceId) {
                serviceItem->setText(0, label);
                break;
            }
        }
    }
    
    // Update FIC-Extractor service labels display
    QStringList labels = m_figAnalyzer->getServiceLabels();
    m_serviceLabelsText->setPlainText(labels.join("\n"));
}

void DABAnalyzerMainWindow::onFIGProcessed(const FIGData& figData)
{
    // Update FIC analysis display based on FIG type
    QString figInfo = QString("[FIG %1/%2] %3")
        .arg(figData.figType)
        .arg(figData.figExtension)
        .arg(figData.description);
    
    // Add to system messages with timestamp
    QString timestamp = QTime::currentTime().toString();
    QString message = QString("[%1] ✅ %2").arg(timestamp, figInfo);
    m_systemMessages->append(message);
}

void DABAnalyzerMainWindow::onFIGError(uint8_t figType, uint8_t extension, const QString& error)
{
    // Add error to system messages
    QString timestamp = QTime::currentTime().toString();
    QString errorMsg = QString("[%1] 🔴 FIG %2/%3 Error: %4")
        .arg(timestamp)
        .arg(figType)
        .arg(extension)
        .arg(error);
    
    m_systemMessages->append(errorMsg);
    
    // Show in status bar temporarily
    m_statusBar->showMessage(QString("🔴 FIG Error: %1").arg(error), 5000);
}

void DABAnalyzerMainWindow::onProgrammeTypeUpdated(uint32_t serviceId, uint8_t ptyCode)
{
    Q_UNUSED(serviceId)
    Q_UNUSED(ptyCode)
    
    // Update FIC-Extractor programme information display
    QStringList programmeInfo = m_figAnalyzer->getProgrammeInfo();
    m_programmeInfoText->setPlainText(programmeInfo.join("\n"));
}

void DABAnalyzerMainWindow::onTimeInfoUpdated(const QDateTime& currentTime)
{
    // Update time display in FIC-Extractor
    m_timeDisplayLabel->setText(QString("Time: %1").arg(currentTime.toString("hh:mm:ss UTC")));
    m_dateDisplayLabel->setText(QString("Date: %1").arg(currentTime.toString("dd/MM/yyyy")));
}

void DABAnalyzerMainWindow::onFrameSelectionChanged()
{
    QListWidgetItem* currentItem = m_frameList->currentItem();
    if (!currentItem) return;

    int frameIndex = currentItem->data(Qt::UserRole).toInt();
    if (frameIndex >= 0 && frameIndex < m_frames.size()) {
        m_currentFrameIndex = frameIndex;
        const ETIFrame& frame = m_frames[frameIndex];

        // Update frame details
        updateSelectedFrameDetails(frame);

        // Update frame content display
        updateFrameContentDisplay(frame);
    }
}

void DABAnalyzerMainWindow::updateSelectedFrameDetails(const ETIFrame& frame)
{
    m_selectedFrameLabel->setText(QString("Selected: #%1").arg(m_currentFrameIndex + 1));
    m_frameTimeLabel->setText(QString("Time: %1ms").arg(m_currentFrameIndex * 24));
    m_frameSizeLabel->setText(QString("Size: %1 B").arg(frame.data.size()));
    m_frameFormatLabel->setText(QString("Format: %1").arg(frame.format));

    // Update sync and CRC status - using system default colors for visibility
    if (frame.syncValid) {
        m_frameSyncStatus->setText("SYNC: ✅ Valid");
    } else {
        m_frameSyncStatus->setText("SYNC: ❌ Invalid");
    }

    if (frame.crcValid) {
        m_frameCRCStatus->setText("CRC: ✅ Valid");
    } else {
        m_frameCRCStatus->setText("CRC: ❌ Invalid");
    }
}

void DABAnalyzerMainWindow::updateFrameContentDisplay(const ETIFrame& frame)
{
    // Update hex viewer
    QString hexData;
    const QByteArray& data = frame.data;
    for (int i = 0; i < data.size(); ++i) {
        if (i % 16 == 0) {
            hexData += QString("%1: ").arg(i, 4, 16, QChar('0')).toUpper();
        }
        hexData += QString("%1 ").arg(static_cast<unsigned char>(data[i]), 2, 16, QChar('0')).toUpper();
        if (i % 16 == 15) {
            hexData += "\n";
        }
    }
    m_hexViewer->setPlainText(hexData);

    // Update MSC viewer with decoded content
    m_mscViewer->setPlainText(frame.mscContent);

    // Update FIC viewer with FIC data
    m_ficViewer->setPlainText(frame.ficContent);
}

// Menu action implementations
void DABAnalyzerMainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Open ETI File",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "ETI Files (*.eti);;All Files (*.*)"
    );

    if (!fileName.isEmpty()) {
        openETIFile(fileName);
    }
}

// Additional slot implementations
void DABAnalyzerMainWindow::onEnsembleItemChanged(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column)
    // Handle checkbox state changes for service filtering
    updateServiceFiltering();
}

void DABAnalyzerMainWindow::onServiceSelectionChanged()
{
    QTreeWidgetItem* currentItem = m_ensembleTree->currentItem();
    if (!currentItem) return;

    // Update service details panel based on selection
    QString serviceType = currentItem->text(1);
    if (serviceType != "Ensemble") {
        // Find corresponding service info
        QString sidText = currentItem->text(2);
        uint32_t sid = sidText.mid(2).toUInt(nullptr, 16);  // Remove "0x" prefix

        for (const auto& service : m_services) {
            if (service.serviceId == sid) {
                updateServiceDetails(service);
                break;
            }
        }
    }
}

void DABAnalyzerMainWindow::updateServiceDetails(const ServiceInfo& service)
{
    m_serviceSIDLabel->setText(QString("SID: 0x%1").arg(service.serviceId, 0, 16));
    m_serviceTypeLabel->setText(QString("Type: %1").arg(service.type));
    m_serviceBitrateLabel->setText(QString("Bitrate: %1 kbps").arg(service.bitrate));
    m_serviceProtectionLabel->setText(QString("Protection: %1").arg(service.protection));
    m_serviceSubChannelLabel->setText(QString("SubCh: %1").arg(service.subchannelId));

    // Update service labels in FIC-Extractor
    QString labels = m_serviceLabelsText->toPlainText();
    QString serviceLabel = QString("%1: %2").arg(service.label, service.description);
    if (!labels.contains(serviceLabel)) {
        labels += serviceLabel + "\n";
        m_serviceLabelsText->setPlainText(labels);
    }
}

void DABAnalyzerMainWindow::updateStatusBar()
{
    if (!m_currentFilePath.isEmpty()) {
        QString fileName = QFileInfo(m_currentFilePath).fileName();
        QString status = QString("🟢 Ready | File: %1 | Frame %2/%3 | Format: ETI-LI | ✅")
            .arg(fileName)
            .arg(m_currentFrameIndex + 1)
            .arg(m_totalFrames);
        m_statusBar->showMessage(status);
    }

    // Update processing rate in system messages
    m_processingRateLabel->setText(QString("📊 Rate: %1+ FPS").arg(static_cast<int>(m_processingFPS)));
    m_memoryUsageLabel->setText(QString("💾 Memory: <%1MB").arg(static_cast<int>(m_memoryUsageMB)));
    m_networkStatusLabel->setText("🌐 Network: Offline");
}

void DABAnalyzerMainWindow::updateSignalQuality()
{
    // Simulate signal quality updates (in real implementation, get from ETI processor)
    static int qualityValue = 75;
    m_qualityMeter->setValue(qualityValue);

    m_signalStrengthLabel->setText("Signal: -45 dBm");
    m_snrLabel->setText("SNR: 25.4 dB");
    m_frequencyErrorLabel->setText("Frequency Error: +127 Hz");
}

// Helper methods
QColor DABAnalyzerMainWindow::getServiceTypeColor(const QString& serviceType) const
{
    if (serviceType.contains("Audio", Qt::CaseInsensitive)) {
        return DABAnalyzerTheme::SERVICE_AUDIO;
    } else if (serviceType.contains("Data", Qt::CaseInsensitive)) {
        return DABAnalyzerTheme::SERVICE_DATA;
    } else if (serviceType.contains("Ensemble", Qt::CaseInsensitive)) {
        return DABAnalyzerTheme::SERVICE_ENSEMBLE;
    } else {
        return DABAnalyzerTheme::TEXT_PRIMARY;
    }
}

void DABAnalyzerMainWindow::populateEnsembleTree(const EnsembleInfo& ensemble)
{
    // Implementation for populating ensemble tree
    // This will be called when ensemble information is updated
}

// Placeholder implementations for menu actions
void DABAnalyzerMainWindow::exportAnalysis()
{
    QMessageBox::information(this, "Export Analysis", "Export functionality will be implemented in Phase 2");
}

void DABAnalyzerMainWindow::showSettings()
{
    QMessageBox::information(this, "Settings", "Settings dialog will be implemented in Phase 2");
}

void DABAnalyzerMainWindow::showAbout()
{
    QMessageBox::about(this, "About StreamDAB Analyzer",
        "StreamDAB Stream Analyzer v1.4\n"
        "DAB Analyzer Professional Interface\n\n"
        "Advanced FIG analysis with Bangkok support\n"
        "Qt6 + C++20 Implementation\n\n"
        "Phase 1.4 - Advanced FIG Analysis & Real Service Discovery");
}

void DABAnalyzerMainWindow::toggleFrameNavigator()
{
    m_frameNavigatorPanel->setVisible(!m_frameNavigatorPanel->isVisible());
}

void DABAnalyzerMainWindow::toggleServiceDetails()
{
    m_serviceDetailsPanel->setVisible(!m_serviceDetailsPanel->isVisible());
}

void DABAnalyzerMainWindow::toggleFICAnalysis()
{
    m_ficAnalysisPanel->setVisible(!m_ficAnalysisPanel->isVisible());
}

void DABAnalyzerMainWindow::connectETIStream()
{
    QMessageBox::information(this, "ETI Stream", "Real-time ETI streaming will be implemented in Phase 2");
}

void DABAnalyzerMainWindow::recordStream()
{
    QMessageBox::information(this, "Record Stream", "Stream recording will be implemented in Phase 2");
}

void DABAnalyzerMainWindow::pauseProcessing()
{
    QMessageBox::information(this, "Pause Processing", "Processing control will be implemented in Phase 2");
}

void DABAnalyzerMainWindow::startRealTimeMode()
{
    m_isRealTimeMode = true;
    // Note: EnhancedETIProcessorQt doesn't have setRealTimeMode method
}

void DABAnalyzerMainWindow::stopRealTimeMode()
{
    m_isRealTimeMode = false;
    // Note: EnhancedETIProcessorQt doesn't have setRealTimeMode method
}

void DABAnalyzerMainWindow::closeCurrentFile()
{
    m_currentFilePath.clear();
    m_frames.clear();
    m_services.clear();
    m_frameList->clear();
    m_ensembleTree->clear();
    m_hexViewer->clear();
    m_mscViewer->clear();
    m_ficViewer->clear();
    m_statusBar->showMessage("🟢 Ready");
}

void DABAnalyzerMainWindow::onFIGTypeToggled(bool enabled)
{
    Q_UNUSED(enabled)
    
    // Update FIG processing based on checkbox states
    m_figAnalyzer->enableFIGType(0, 0, m_figType00CheckBox->isChecked());
    m_figAnalyzer->enableFIGType(0, 1, m_figType01CheckBox->isChecked());
    m_figAnalyzer->enableFIGType(0, 2, m_figType02CheckBox->isChecked());
    m_figAnalyzer->enableFIGType(1, 0, m_figType10CheckBox->isChecked());
    m_figAnalyzer->enableFIGType(1, 1, m_figType11CheckBox->isChecked());
    m_figAnalyzer->enableFIGType(1, 4, m_figType14CheckBox->isChecked());
    
    // Update FIC analysis display
    updateFICAnalysis();
}

void DABAnalyzerMainWindow::updateFICAnalysis()
{
    // Update FIC analysis based on enabled FIG types
    // Force reprocessing of current frame's FIC data if available
    if (m_currentFrameIndex >= 0 && m_currentFrameIndex < m_frames.size()) {
        const ETIFrame& frame = m_frames[m_currentFrameIndex];
        if (!frame.ficContent.isEmpty()) {
            // Convert QString back to QByteArray for reprocessing
            QByteArray ficData = frame.ficContent.toUtf8();
            m_figAnalyzer->analyzeFICData(ficData);
        }
    }
    
    // Update FIC-Extractor displays with current analyzer state
    QStringList labels = m_figAnalyzer->getServiceLabels();
    m_serviceLabelsText->setPlainText(labels.join("\n"));
    
    QStringList programmeInfo = m_figAnalyzer->getProgrammeInfo();
    m_programmeInfoText->setPlainText(programmeInfo.join("\n"));
    
    // Update system messages with FIG processing status
    QString timestamp = QTime::currentTime().toString();
    QString message = QString("[%1] 🔧 FIG Analysis updated - Types: %2%3%4%5%6%7")
        .arg(timestamp)
        .arg(m_figType00CheckBox->isChecked() ? "0/0 " : "")
        .arg(m_figType01CheckBox->isChecked() ? "0/1 " : "")
        .arg(m_figType02CheckBox->isChecked() ? "0/2 " : "")
        .arg(m_figType10CheckBox->isChecked() ? "1/0 " : "")
        .arg(m_figType11CheckBox->isChecked() ? "1/1 " : "")
        .arg(m_figType14CheckBox->isChecked() ? "1/4 " : "");
    
    m_systemMessages->append(message);
}

void DABAnalyzerMainWindow::updateServiceFiltering()
{
    // Update frame display based on selected services
    // This will be implemented with actual filtering logic
}
