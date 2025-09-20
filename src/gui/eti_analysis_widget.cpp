#include "eti_analysis_widget.h"
#include "constellation_widget.h"
#include "core/eti_processor.h"
#include "utils/logger.h"
#include <QApplication>
#include <QHeaderView>
#include <QScrollBar>
#include <QSplitterHandle>
#include <QDateTime>
#include <QDebug>
#include <cmath>

EtiAnalysisWidget::EtiAnalysisWidget(QWidget *parent)
    : QWidget(parent)
    , m_etiProcessor(nullptr)
    , m_constellationWidget(nullptr)
    , m_frameListModel(nullptr)
    , m_mainLayout(nullptr)
    , m_mainSplitter(nullptr)
    , m_contentSplitter(nullptr)
    , m_controlHeader(nullptr)
    , m_headerLabel(nullptr)
    , m_statsLabel(nullptr)
    , m_displayModeCombo(nullptr)
    , m_analysisModeCombo(nullptr)
    , m_startStopButton(nullptr)
    , m_analysisTabWidget(nullptr)
    , m_frameListTable(nullptr)
    , m_frameDetailsText(nullptr)
    , m_timelineWidget(nullptr)
    , m_packetWidget(nullptr)
    , m_statisticsGroup(nullptr)
    , m_statisticsLayout(nullptr)
    , m_frameCountLabel(nullptr)
    , m_processingRateLabel(nullptr)
    , m_errorCountLabel(nullptr)
    , m_signalQualityLabel(nullptr)
    , m_cpuUsageBar(nullptr)
    , m_memoryUsageBar(nullptr)
    , m_signalQualityBar(nullptr)
    , m_constellationGroup(nullptr)
    , m_constellationLayout(nullptr)
    , m_legendFrame(nullptr)
    , m_legendLayout(nullptr)
    , m_realTimeTimer(nullptr)
    , m_realTimeEnabled(false)
    , m_isInitialized(false)
    , m_frameDisplayMode(FrameDisplayMode::FrameList)
    , m_analysisMode(AnalysisMode::RealTime)
    , m_frameCount(0)
    , m_processingRate(0.0)
    , m_errorCount(0)
    , m_signalQuality(0.0)
    , m_cpuUsage(0.0)
    , m_memoryUsage(0.0)
    , m_selectedFrameIndex(0)
    , m_lastUpdateTime(0)
    , m_lastFrameTime(0)
{
    setObjectName("EtiAnalysisWidget");
    setAttribute(Qt::WA_DeleteOnClose);
    
    // Initialize frame times for performance tracking
    m_frameTimes.reserve(PERFORMANCE_SAMPLE_SIZE);
}

EtiAnalysisWidget::~EtiAnalysisWidget()
{
    if (m_realTimeTimer && m_realTimeTimer->isActive()) {
        m_realTimeTimer->stop();
    }
    
    Logger::instance().log(Logger::Debug, "EtiAnalysisWidget", "Widget destroyed");
}

bool EtiAnalysisWidget::initialize()
{
    if (m_isInitialized) {
        Logger::instance().log(Logger::Warning, "EtiAnalysisWidget", "Already initialized");
        return true;
    }
    
    Logger::instance().log(Logger::Info, "EtiAnalysisWidget", "Initializing professional ETI analysis widget...");
    
    try {
        // Create main layout structure
        createMainLayout();
        
        // Create control header
        createControlHeader();
        
        // Create frame analysis section
        createFrameAnalysisSection();
        
        // Create statistics dashboard
        createStatisticsDashboard();
        
        // Create constellation integration
        createConstellationIntegration();
        
        // Setup professional theme
        setupProfessionalTheme();
        
        // Setup real-time timer
        setupRealTimeTimer();
        
        // Connect internal signals
        connectInternalSignals();
        
        m_isInitialized = true;
        Logger::instance().log(Logger::Info, "EtiAnalysisWidget", "Professional ETI analysis widget initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "EtiAnalysisWidget", 
                             QString("Initialization failed: %1").arg(e.what()));
        return false;
    }
}

void EtiAnalysisWidget::setEtiProcessor(EtiProcessor *processor)
{
    if (m_etiProcessor == processor) {
        return;
    }
    
    // Disconnect previous processor
    if (m_etiProcessor) {
        disconnect(m_etiProcessor, nullptr, this, nullptr);
    }
    
    m_etiProcessor = processor;
    
    if (m_etiProcessor) {
        // Connect signals for real-time frame updates
        connect(m_etiProcessor, &EtiProcessor::frameProcessed,
                this, &EtiAnalysisWidget::onFrameProcessed);
        connect(m_etiProcessor, &EtiProcessor::ensembleDiscovered,
                this, &EtiAnalysisWidget::onEnsembleDiscovered);
        connect(m_etiProcessor, &EtiProcessor::statusChanged,
                this, &EtiAnalysisWidget::onStatusChanged);
        connect(m_etiProcessor, &EtiProcessor::errorOccurred,
                this, &EtiAnalysisWidget::onErrorOccurred);
        
        Logger::instance().log(Logger::Info, "EtiAnalysisWidget", "ETI processor connected with signal connections");
    }
    
    // Update constellation widget if available
    if (m_constellationWidget && m_etiProcessor) {
        m_constellationWidget->setEtiProcessor(m_etiProcessor);
    }
}

void EtiAnalysisWidget::setFrameDisplayMode(FrameDisplayMode mode)
{
    if (m_frameDisplayMode == mode) {
        return;
    }
    
    m_frameDisplayMode = mode;
    
    // Update display mode combo
    if (m_displayModeCombo) {
        m_displayModeCombo->setCurrentIndex(static_cast<int>(mode));
    }
    
    // Switch to appropriate tab
    if (m_analysisTabWidget) {
        m_analysisTabWidget->setCurrentIndex(static_cast<int>(mode));
    }
    
    Logger::instance().log(Logger::Debug, "EtiAnalysisWidget", 
                         QString("Frame display mode changed to: %1").arg(static_cast<int>(mode)));
    
    emit displayModeChanged(mode);
}

void EtiAnalysisWidget::setAnalysisMode(AnalysisMode mode)
{
    if (m_analysisMode == mode) {
        return;
    }
    
    m_analysisMode = mode;
    
    // Update analysis mode combo
    if (m_analysisModeCombo) {
        m_analysisModeCombo->setCurrentIndex(static_cast<int>(mode));
    }
    
    // Update start/stop button text
    if (m_startStopButton) {
        switch (mode) {
        case AnalysisMode::RealTime:
            m_startStopButton->setText(m_realTimeEnabled ? tr("Stop") : tr("Start"));
            break;
        case AnalysisMode::Playback:
            m_startStopButton->setText(tr("Play"));
            break;
        case AnalysisMode::Comparison:
            m_startStopButton->setText(tr("Compare"));
            break;
        }
    }
    
    Logger::instance().log(Logger::Debug, "EtiAnalysisWidget", 
                         QString("Analysis mode changed to: %1").arg(static_cast<int>(mode)));
    
    emit analysisModeChanged(mode);
}

void EtiAnalysisWidget::startRealTimeAnalysis()
{
    if (m_realTimeEnabled) {
        Logger::instance().log(Logger::Warning, "EtiAnalysisWidget", "Real-time analysis already active");
        return;
    }
    
    m_realTimeEnabled = true;
    
    if (m_realTimeTimer) {
        m_realTimeTimer->start(UPDATE_INTERVAL_MS);
    }
    
    if (m_startStopButton) {
        m_startStopButton->setText(tr("Stop"));
    }
    
    // Start constellation widget updates
    if (m_constellationWidget) {
        m_constellationWidget->startRealTimeUpdates();
    }
    
    Logger::instance().log(Logger::Info, "EtiAnalysisWidget", "Real-time analysis started");
}

void EtiAnalysisWidget::stopRealTimeAnalysis()
{
    if (!m_realTimeEnabled) {
        Logger::instance().log(Logger::Warning, "EtiAnalysisWidget", "Real-time analysis not active");
        return;
    }
    
    m_realTimeEnabled = false;
    
    if (m_realTimeTimer) {
        m_realTimeTimer->stop();
    }
    
    if (m_startStopButton) {
        m_startStopButton->setText(tr("Start"));
    }
    
    // Stop constellation widget updates
    if (m_constellationWidget) {
        m_constellationWidget->stopRealTimeUpdates();
    }
    
    Logger::instance().log(Logger::Info, "EtiAnalysisWidget", "Real-time analysis stopped");
}

void EtiAnalysisWidget::updateAnalysisDisplay()
{
    if (!m_isInitialized) {
        return;
    }
    
    QMutexLocker locker(&m_dataMutex);
    
    // Update based on current display mode
    switch (m_frameDisplayMode) {
    case FrameDisplayMode::FrameList:
        updateFrameList();
        break;
    case FrameDisplayMode::FrameDetails:
        updateFrameDetails();
        break;
    case FrameDisplayMode::Timeline:
        updateTimelineDisplay();
        break;
    case FrameDisplayMode::Packets:
        updatePacketDisplay();
        break;
    }
    
    // Update statistics dashboard
    updateStatisticsDashboard();
    
    // Update constellation display
    updateConstellationDisplay();
}

void EtiAnalysisWidget::clearAnalysisData()
{
    QMutexLocker locker(&m_dataMutex);
    
    m_frameData.clear();
    m_frameCount = 0;
    m_errorCount = 0;
    m_processingRate = 0.0;
    m_signalQuality = 0.0;
    m_frameTimes.clear();
    
    // Clear frame list table
    if (m_frameListTable) {
        m_frameListTable->setRowCount(0);
    }
    
    // Clear frame details
    if (m_frameDetailsText) {
        m_frameDetailsText->clear();
    }
    
    // Update display
    updateAnalysisDisplay();
    
    Logger::instance().log(Logger::Info, "EtiAnalysisWidget", "Analysis data cleared");
}

void EtiAnalysisWidget::handleFrameDataUpdate(const QVariantMap& frameData)
{
    QMutexLocker locker(&m_dataMutex);
    
    // Add frame data to storage
    m_frameData.append(frameData);
    m_frameCount++;
    
    // Limit frame history
    if (m_frameData.size() > MAX_FRAME_HISTORY) {
        m_frameData.removeFirst();
    }
    
    // Update performance metrics
    calculatePerformanceMetrics();
    
    // Check for errors
    if (frameData.contains("error") && frameData["error"].toBool()) {
        m_errorCount++;
        emit errorDetected(frameData["errorType"].toString(), 
                          frameData["errorDescription"].toString());
    }
    
    // Update display if real-time mode
    if (m_realTimeEnabled) {
        updateAnalysisDisplay();
    }
}

void EtiAnalysisWidget::handleFrameSelection(quint64 frameIndex)
{
    if (frameIndex >= static_cast<quint64>(m_frameData.size())) {
        Logger::instance().log(Logger::Warning, "EtiAnalysisWidget", 
                             QString("Invalid frame index: %1").arg(frameIndex));
        return;
    }
    
    m_selectedFrameIndex = frameIndex;
    
    // Update frame details if in details mode
    if (m_frameDisplayMode == FrameDisplayMode::FrameDetails) {
        updateFrameDetails();
    }
    
    emit frameSelected(frameIndex);
}

void EtiAnalysisWidget::updateStatisticsDashboard()
{
    if (!m_statisticsGroup) {
        return;
    }
    
    // Update frame count
    if (m_frameCountLabel) {
        m_frameCountLabel->setText(QString("Frames: %1").arg(m_frameCount));
    }
    
    // Update processing rate
    if (m_processingRateLabel) {
        m_processingRateLabel->setText(QString("Rate: %1 fps").arg(m_processingRate, 0, 'f', 1));
    }
    
    // Update error count
    if (m_errorCountLabel) {
        m_errorCountLabel->setText(QString("Errors: %1").arg(m_errorCount));
        
        // Color code based on error count
        if (m_errorCount > 0) {
            m_errorCountLabel->setStyleSheet("color: #F44336; font-weight: bold;");
        } else {
            m_errorCountLabel->setStyleSheet("color: #4CAF50; font-weight: bold;");
        }
    }
    
    // Update signal quality
    if (m_signalQualityLabel) {
        m_signalQualityLabel->setText(QString("Quality: %1%").arg(m_signalQuality, 0, 'f', 1));
        
        // Color code based on quality
        if (m_signalQuality >= 90.0) {
            m_signalQualityLabel->setStyleSheet("color: #4CAF50; font-weight: bold;");
        } else if (m_signalQuality >= 70.0) {
            m_signalQualityLabel->setStyleSheet("color: #FF9800; font-weight: bold;");
        } else {
            m_signalQualityLabel->setStyleSheet("color: #F44336; font-weight: bold;");
        }
    }
    
    // Update progress bars
    if (m_cpuUsageBar) {
        m_cpuUsageBar->setValue(static_cast<int>(m_cpuUsage));
    }
    
    if (m_memoryUsageBar) {
        m_memoryUsageBar->setValue(static_cast<int>(m_memoryUsage));
    }
    
    if (m_signalQualityBar) {
        m_signalQualityBar->setValue(static_cast<int>(m_signalQuality));
    }
    
    // Emit performance metrics
    QVariantMap metrics;
    metrics["frameCount"] = m_frameCount;
    metrics["processingRate"] = m_processingRate;
    metrics["errorCount"] = m_errorCount;
    metrics["signalQuality"] = m_signalQuality;
    metrics["cpuUsage"] = m_cpuUsage;
    metrics["memoryUsage"] = m_memoryUsage;
    
    emit performanceMetricsUpdated(metrics);
}

void EtiAnalysisWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    // Professional background rendering
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Background gradient
    QLinearGradient gradient(0, 0, 0, height());
    gradient.setColorAt(0, QColor(250, 250, 250));
    gradient.setColorAt(1, QColor(240, 240, 240));
    
    painter.fillRect(rect(), gradient);
    
    // Professional border
    painter.setPen(QPen(QColor(200, 200, 200), 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

void EtiAnalysisWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // Adjust layout proportions if needed
    if (m_mainSplitter && event->oldSize().isValid()) {
        // Maintain proportional layout
    }
}

void EtiAnalysisWidget::handleRealTimeUpdate()
{
    if (!m_realTimeEnabled || !m_isInitialized) {
        return;
    }
    
    // Update analysis display
    updateAnalysisDisplay();
    
    // Calculate performance metrics
    calculatePerformanceMetrics();
    
    // Update constellation if available
    if (m_constellationWidget) {
        m_constellationWidget->updateDisplay();
    }
}

void EtiAnalysisWidget::handleFrameDisplayModeChange(int index)
{
    if (index < 0 || index >= 4) {
        return;
    }
    
    setFrameDisplayMode(static_cast<FrameDisplayMode>(index));
}

void EtiAnalysisWidget::handleConstellationUpdate()
{
    if (m_constellationWidget) {
        // Get signal quality from constellation
        m_signalQuality = m_constellationWidget->getSignalQuality();
    }
}

void EtiAnalysisWidget::createMainLayout()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(8);
    
    // Create main splitter for professional layout
    m_mainSplitter = new QSplitter(Qt::Vertical, this);
    m_mainSplitter->setHandleWidth(4);
    m_mainSplitter->setChildrenCollapsible(false);
    
    m_mainLayout->addWidget(m_mainSplitter);
}

void EtiAnalysisWidget::createControlHeader()
{
    m_controlHeader = new QFrame();
    m_controlHeader->setFrameStyle(QFrame::NoFrame);
    m_controlHeader->setFixedHeight(60);
    
    QHBoxLayout *headerLayout = new QHBoxLayout(m_controlHeader);
    headerLayout->setContentsMargins(12, 8, 12, 8);
    headerLayout->setSpacing(16);
    
    // Header label
    m_headerLabel = new QLabel(tr("ETI Frame Analysis"));
    m_headerLabel->setObjectName("AnalysisHeaderLabel");
    
    // Statistics label
    m_statsLabel = new QLabel(tr("Ready"));
    m_statsLabel->setObjectName("AnalysisStatsLabel");
    
    // Display mode combo
    m_displayModeCombo = new QComboBox();
    m_displayModeCombo->addItems({tr("Frame List"), tr("Frame Details"), tr("Timeline"), tr("Packets")});
    m_displayModeCombo->setMinimumWidth(120);
    
    // Analysis mode combo
    m_analysisModeCombo = new QComboBox();
    m_analysisModeCombo->addItems({tr("Real-Time"), tr("Playback"), tr("Comparison")});
    m_analysisModeCombo->setMinimumWidth(120);
    
    // Start/Stop button
    m_startStopButton = new QPushButton(tr("Start"));
    m_startStopButton->setMinimumWidth(80);
    
    headerLayout->addWidget(m_headerLabel);
    headerLayout->addWidget(m_statsLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(new QLabel(tr("View:")));
    headerLayout->addWidget(m_displayModeCombo);
    headerLayout->addWidget(new QLabel(tr("Mode:")));
    headerLayout->addWidget(m_analysisModeCombo);
    headerLayout->addWidget(m_startStopButton);
    
    m_mainSplitter->addWidget(m_controlHeader);
}

void EtiAnalysisWidget::createFrameAnalysisSection()
{
    m_analysisTabWidget = new QTabWidget();
    m_analysisTabWidget->setObjectName("FrameAnalysisTabWidget");
    
    // Create different view widgets
    createFrameListTable();
    createDetailedFrameView();
    createTimelineView();
    createPacketView();
    
    // Add tabs
    m_analysisTabWidget->addTab(m_frameListTable, tr("Frame List"));
    m_analysisTabWidget->addTab(m_frameDetailsText, tr("Frame Details"));
    m_analysisTabWidget->addTab(m_timelineWidget, tr("Timeline"));
    m_analysisTabWidget->addTab(m_packetWidget, tr("Packets"));
    
    m_mainSplitter->addWidget(m_analysisTabWidget);
}

void EtiAnalysisWidget::createStatisticsDashboard()
{
    m_statisticsGroup = new QGroupBox(tr("Real-Time Statistics"));
    m_statisticsGroup->setObjectName("StatisticsDashboard");
    m_statisticsGroup->setFixedHeight(120);
    
    m_statisticsLayout = new QGridLayout(m_statisticsGroup);
    m_statisticsLayout->setContentsMargins(12, 16, 12, 12);
    m_statisticsLayout->setSpacing(8);
    
    // Create statistics labels
    m_frameCountLabel = new QLabel(tr("Frames: 0"));
    m_processingRateLabel = new QLabel(tr("Rate: 0.0 fps"));
    m_errorCountLabel = new QLabel(tr("Errors: 0"));
    m_signalQualityLabel = new QLabel(tr("Quality: 0.0%"));
    
    // Create progress bars
    m_cpuUsageBar = new QProgressBar();
    m_cpuUsageBar->setRange(0, 100);
    m_cpuUsageBar->setTextVisible(true);
    m_cpuUsageBar->setFormat("CPU: %p%");
    
    m_memoryUsageBar = new QProgressBar();
    m_memoryUsageBar->setRange(0, 100);
    m_memoryUsageBar->setTextVisible(true);
    m_memoryUsageBar->setFormat("Memory: %p%");
    
    m_signalQualityBar = new QProgressBar();
    m_signalQualityBar->setRange(0, 100);
    m_signalQualityBar->setTextVisible(true);
    m_signalQualityBar->setFormat("Signal: %p%");
    
    // Layout statistics
    m_statisticsLayout->addWidget(m_frameCountLabel, 0, 0);
    m_statisticsLayout->addWidget(m_processingRateLabel, 0, 1);
    m_statisticsLayout->addWidget(m_errorCountLabel, 0, 2);
    m_statisticsLayout->addWidget(m_signalQualityLabel, 0, 3);
    
    m_statisticsLayout->addWidget(m_cpuUsageBar, 1, 0);
    m_statisticsLayout->addWidget(m_memoryUsageBar, 1, 1);
    m_statisticsLayout->addWidget(m_signalQualityBar, 1, 2, 1, 2);
    
    m_mainSplitter->addWidget(m_statisticsGroup);
}

void EtiAnalysisWidget::createConstellationIntegration()
{
    m_constellationGroup = new QGroupBox(tr("Signal Constellation"));
    m_constellationGroup->setObjectName("ConstellationIntegration");
    m_constellationGroup->setFixedHeight(200);
    
    m_constellationLayout = new QVBoxLayout(m_constellationGroup);
    m_constellationLayout->setContentsMargins(8, 16, 8, 8);
    m_constellationLayout->setSpacing(4);
    
    // Create constellation widget
    m_constellationWidget = new ConstellationWidget(this);
    m_constellationWidget->setObjectName("IntegratedConstellationWidget");
    
    m_constellationLayout->addWidget(m_constellationWidget);
    
    m_mainSplitter->addWidget(m_constellationGroup);
    
    // Set splitter proportions (40% analysis, 20% stats, 40% constellation)
    m_mainSplitter->setStretchFactor(0, 0);  // Header (fixed)
    m_mainSplitter->setStretchFactor(1, 4);  // Analysis tabs
    m_mainSplitter->setStretchFactor(2, 0);  // Statistics (fixed)
    m_mainSplitter->setStretchFactor(3, 2);  // Constellation
}

void EtiAnalysisWidget::setupProfessionalTheme()
{
    // Professional broadcast industry theme
    setStyleSheet(
        "QGroupBox { "
        "    font-family: 'Segoe UI'; "
        "    font-weight: bold; "
        "    font-size: 11px; "
        "    color: #333333; "
        "    border: 2px solid #CCCCCC; "
        "    border-radius: 5px; "
        "    margin-top: 8px; "
        "    background-color: #FAFAFA; "
        "} "
        "QGroupBox::title { "
        "    subcontrol-origin: margin; "
        "    subcontrol-position: top left; "
        "    padding: 0 8px; "
        "    background-color: #FAFAFA; "
        "    color: #0078D4; "
        "} "
        "QLabel#AnalysisHeaderLabel { "
        "    font-family: 'Segoe UI'; "
        "    font-weight: bold; "
        "    font-size: 14px; "
        "    color: #0078D4; "
        "} "
        "QLabel#AnalysisStatsLabel { "
        "    font-family: 'Segoe UI'; "
        "    font-size: 10px; "
        "    color: #666666; "
        "    padding: 4px 8px; "
        "    background-color: #F0F0F0; "
        "    border: 1px solid #DDDDDD; "
        "    border-radius: 3px; "
        "} "
        "QComboBox { "
        "    border: 1px solid #CCCCCC; "
        "    border-radius: 3px; "
        "    padding: 4px 8px; "
        "    font-size: 10px; "
        "    background-color: #FFFFFF; "
        "} "
        "QComboBox:focus { "
        "    border: 2px solid #0078D4; "
        "} "
        "QPushButton { "
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 #E8F4FF, stop:1 #CCE8FF); "
        "    border: 1px solid #0078D4; "
        "    border-radius: 4px; "
        "    padding: 6px 12px; "
        "    font-size: 10px; "
        "    font-weight: bold; "
        "    color: #0078D4; "
        "} "
        "QPushButton:hover { "
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 #CCE8FF, stop:1 #99D6FF); "
        "} "
        "QPushButton:pressed { "
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 #99D6FF, stop:1 #66C2FF); "
        "} "
        "QProgressBar { "
        "    border: 1px solid #CCCCCC; "
        "    border-radius: 3px; "
        "    text-align: center; "
        "    font-size: 9px; "
        "    font-weight: bold; "
        "} "
        "QProgressBar::chunk { "
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "        stop:0 #4CAF50, stop:1 #66BB6A); "
        "    border-radius: 2px; "
        "}"
    );
}

void EtiAnalysisWidget::setupRealTimeTimer()
{
    m_realTimeTimer = new QTimer(this);
    m_realTimeTimer->setInterval(UPDATE_INTERVAL_MS);
    m_realTimeTimer->setSingleShot(false);
    
    connect(m_realTimeTimer, &QTimer::timeout, this, &EtiAnalysisWidget::handleRealTimeUpdate);
}

void EtiAnalysisWidget::connectInternalSignals()
{
    // Connect control signals
    connect(m_displayModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EtiAnalysisWidget::handleFrameDisplayModeChange);
    
    connect(m_analysisModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
                if (index >= 0 && index < 3) {
                    setAnalysisMode(static_cast<AnalysisMode>(index));
                }
            });
    
    connect(m_startStopButton, &QPushButton::clicked, [this]() {
        if (m_analysisMode == AnalysisMode::RealTime) {
            if (m_realTimeEnabled) {
                stopRealTimeAnalysis();
            } else {
                startRealTimeAnalysis();
            }
        }
    });
    
    // Connect constellation updates
    if (m_constellationWidget) {
        connect(m_constellationWidget, &ConstellationWidget::signalQualityChanged,
                this, &EtiAnalysisWidget::handleConstellationUpdate);
    }
}

void EtiAnalysisWidget::createFrameListTable()
{
    m_frameListTable = new QTableWidget();
    m_frameListTable->setObjectName("ProfessionalFrameListTable");
    
    // Set up columns
    m_frameListTable->setColumnCount(8);
    QStringList headers = {
        tr("Frame #"), tr("Type"), tr("Size"), tr("FIC"), 
        tr("MSC"), tr("Timestamp"), tr("Quality"), tr("Status")
    };
    m_frameListTable->setHorizontalHeaderLabels(headers);
    
    // Professional table styling with color coding
    m_frameListTable->setStyleSheet(
        "QTableWidget { "
        "    background-color: #FFFFFF; "
        "    alternate-background-color: #F8F8F8; "
        "    selection-background-color: #CCE8FF; "
        "    selection-color: #000000; "
        "    gridline-color: #E8E8E8; "
        "    border: 1px solid #CCCCCC; "
        "    outline: none; "
        "    font-family: 'Consolas', monospace; "
        "    font-size: 10px; "
        "} "
        "QTableWidget::item { "
        "    padding: 4px 6px; "
        "    border-bottom: 1px solid #F0F0F0; "
        "    height: 18px; "
        "} "
        "QHeaderView::section { "
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 #F8F8F8, stop:1 #E8E8E8); "
        "    border: 1px solid #CCCCCC; "
        "    padding: 6px 8px; "
        "    font-weight: bold; "
        "    font-size: 9px; "
        "    color: #333333; "
        "}"
    );
    
    // Configure table properties
    m_frameListTable->setAlternatingRowColors(true);
    m_frameListTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_frameListTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_frameListTable->setSortingEnabled(true);
    m_frameListTable->verticalHeader()->setVisible(false);
    
    // Set column widths
    m_frameListTable->setColumnWidth(0, 80);   // Frame #
    m_frameListTable->setColumnWidth(1, 60);   // Type
    m_frameListTable->setColumnWidth(2, 70);   // Size
    m_frameListTable->setColumnWidth(3, 50);   // FIC
    m_frameListTable->setColumnWidth(4, 50);   // MSC
    m_frameListTable->setColumnWidth(5, 120);  // Timestamp
    m_frameListTable->setColumnWidth(6, 80);   // Quality
    
    // Connect selection signal
    connect(m_frameListTable, &QTableWidget::itemSelectionChanged, [this]() {
        int currentRow = m_frameListTable->currentRow();
        if (currentRow >= 0) {
            handleFrameSelection(currentRow);
        }
    });
}

void EtiAnalysisWidget::createDetailedFrameView()
{
    m_frameDetailsText = new QTextEdit();
    m_frameDetailsText->setObjectName("FrameDetailsText");
    m_frameDetailsText->setReadOnly(true);
    m_frameDetailsText->setFont(QFont("Consolas", 9));
    
    m_frameDetailsText->setStyleSheet(
        "QTextEdit { "
        "    background-color: #FFFFFF; "
        "    border: 1px solid #CCCCCC; "
        "    font-family: 'Consolas', monospace; "
        "    font-size: 9px; "
        "    line-height: 1.2; "
        "} "
    );
}

void EtiAnalysisWidget::createTimelineView()
{
    m_timelineWidget = new QWidget();
    m_timelineWidget->setObjectName("TimelineWidget");
    
    QVBoxLayout *layout = new QVBoxLayout(m_timelineWidget);
    layout->addWidget(new QLabel(tr("Timeline view - Coming soon")));
    
    m_timelineWidget->setStyleSheet(
        "QWidget { "
        "    background-color: #FAFAFA; "
        "    border: 1px solid #DDDDDD; "
        "} "
    );
}

void EtiAnalysisWidget::createPacketView()
{
    m_packetWidget = new QWidget();
    m_packetWidget->setObjectName("PacketWidget");
    
    QVBoxLayout *layout = new QVBoxLayout(m_packetWidget);
    layout->addWidget(new QLabel(tr("Packet view - Coming soon")));
    
    m_packetWidget->setStyleSheet(
        "QWidget { "
        "    background-color: #FAFAFA; "
        "    border: 1px solid #DDDDDD; "
        "} "
    );
}

void EtiAnalysisWidget::updateFrameList()
{
    if (!m_frameListTable) {
        return;
    }
    
    // Update frame list with latest data
    int currentRows = m_frameListTable->rowCount();
    int targetRows = qMin(m_frameData.size(), MAX_FRAME_HISTORY);
    
    // Add new rows if needed
    if (targetRows > currentRows) {
        m_frameListTable->setRowCount(targetRows);
        
        // Add new frame items
        for (int i = currentRows; i < targetRows; ++i) {
            if (i < m_frameData.size()) {
                createColorCodedFrameItem(m_frameData[i], i);
            }
        }
    }
    
    // Auto-scroll to latest frame
    if (targetRows > 0) {
        m_frameListTable->scrollToBottom();
    }
}

void EtiAnalysisWidget::updateFrameDetails()
{
    if (!m_frameDetailsText || m_selectedFrameIndex >= static_cast<quint64>(m_frameData.size())) {
        return;
    }
    
    const QVariantMap& frameData = m_frameData[m_selectedFrameIndex];
    
    QString details;
    details += QString("Frame #%1 Details\n").arg(m_selectedFrameIndex);
    details += QString("==================\n\n");
    
    for (auto it = frameData.begin(); it != frameData.end(); ++it) {
        details += QString("%1: %2\n").arg(it.key(), it.value().toString());
    }
    
    m_frameDetailsText->setPlainText(details);
}

void EtiAnalysisWidget::updateTimelineDisplay()
{
    // TODO: Implement timeline visualization
}

void EtiAnalysisWidget::updatePacketDisplay()
{
    // TODO: Implement packet visualization
}

void EtiAnalysisWidget::createColorCodedFrameItem(const QVariantMap& frameData, int row)
{
    if (!m_frameListTable || row < 0) {
        return;
    }
    
    // Extract frame information
    QString frameNumber = frameData.value("frameNumber", QString::number(row)).toString();
    QString frameType = frameData.value("type", "Unknown").toString();
    QString frameSize = frameData.value("size", "0").toString();
    QString ficInfo = frameData.value("fic", "N/A").toString();
    QString mscInfo = frameData.value("msc", "N/A").toString();
    QString timestamp = frameData.value("timestamp", QDateTime::currentDateTime().toString()).toString();
    QString quality = frameData.value("quality", "Unknown").toString();
    QString status = frameData.value("status", "OK").toString();
    
    // Determine status color
    QColor statusColor = QColor(76, 175, 80);  // Green (OK)
    if (status.contains("Error", Qt::CaseInsensitive)) {
        statusColor = QColor(244, 67, 54);  // Red (Error)
    } else if (status.contains("Warning", Qt::CaseInsensitive)) {
        statusColor = QColor(255, 152, 0);  // Orange (Warning)
    }
    
    // Create table items with color coding
    auto createColoredItem = [&](const QString& text, const QColor& bgColor = QColor()) -> QTableWidgetItem* {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        if (bgColor.isValid()) {
            item->setBackground(QBrush(bgColor));
        }
        return item;
    };
    
    m_frameListTable->setItem(row, 0, createColoredItem(frameNumber));
    m_frameListTable->setItem(row, 1, createColoredItem(frameType));
    m_frameListTable->setItem(row, 2, createColoredItem(frameSize));
    m_frameListTable->setItem(row, 3, createColoredItem(ficInfo));
    m_frameListTable->setItem(row, 4, createColoredItem(mscInfo));
    m_frameListTable->setItem(row, 5, createColoredItem(timestamp));
    m_frameListTable->setItem(row, 6, createColoredItem(quality));
    m_frameListTable->setItem(row, 7, createColoredItem(status, statusColor));
}

void EtiAnalysisWidget::calculatePerformanceMetrics()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    if (m_lastFrameTime > 0) {
        qint64 frameInterval = currentTime - m_lastFrameTime;
        m_frameTimes.append(frameInterval);
        
        // Keep only recent samples
        if (m_frameTimes.size() > PERFORMANCE_SAMPLE_SIZE) {
            m_frameTimes.removeFirst();
        }
        
        // Calculate average processing rate
        if (!m_frameTimes.isEmpty()) {
            double averageInterval = 0.0;
            for (qint64 interval : m_frameTimes) {
                averageInterval += interval;
            }
            averageInterval /= m_frameTimes.size();
            
            if (averageInterval > 0) {
                m_processingRate = 1000.0 / averageInterval;  // Convert to FPS
            }
        }
    }
    
    m_lastFrameTime = currentTime;
    
    // Simulate CPU and memory usage (replace with actual measurements)
    m_cpuUsage = qMin(100.0, (m_processingRate / TARGET_FPS) * 100.0);
    m_memoryUsage = qMin(100.0, (m_frameData.size() / static_cast<double>(MAX_FRAME_HISTORY)) * 100.0);
}

void EtiAnalysisWidget::updateConstellationDisplay()
{
    if (m_constellationWidget) {
        m_constellationWidget->updateDisplay();
    }
}

bool EtiAnalysisWidget::validateState() const
{
    return m_isInitialized && 
           m_mainLayout != nullptr && 
           m_mainSplitter != nullptr;
}

void EtiAnalysisWidget::onFrameProcessed(quint64 frameNumber, const QByteArray& data)
{
    QMutexLocker locker(&m_dataMutex);
    
    // Create frame data variant map
    QVariantMap frameData;
    frameData["frameNumber"] = frameNumber;
    frameData["size"] = data.size();
    frameData["timestamp"] = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    frameData["type"] = "ETI";
    frameData["status"] = "OK";
    frameData["quality"] = "95"; // Default quality
    frameData["fic"] = QString("FIC_%1").arg(frameNumber % 4);
    frameData["msc"] = QString("MSC_%1").arg((frameNumber % 76) + 1);
    
    // Add frame to display data
    handleFrameDataUpdate(frameData);
    
    // Update status
    if (m_statsLabel) {
        m_statsLabel->setText(QString("Frame: %1 | Size: %2 bytes | Rate: %3 fps")
                              .arg(frameNumber)
                              .arg(data.size())
                              .arg(m_processingRate, 0, 'f', 1));
    }
    
    Logger::instance().log(Logger::Debug, "EtiAnalysisWidget",
                          QString("Frame processed: %1 (Size: %2 bytes)")
                          .arg(frameNumber)
                          .arg(data.size()));
}

void EtiAnalysisWidget::onEnsembleDiscovered(const eti::Ensemble& ensemble)
{
    // Update display with ensemble information
    if (m_headerLabel) {
        m_headerLabel->setText(QString("ETI Frame Analysis - %1 (EID: 0x%2)")
                               .arg(QString::fromStdString(ensemble.label))
                               .arg(ensemble.ensemble_id, 4, 16, QChar('0')));  // Use correct field name
    }
    
    Logger::instance().log(Logger::Info, "EtiAnalysisWidget",
                          QString("Ensemble discovered: %1 (EID: 0x%2, Services: %3)")
                          .arg(QString::fromStdString(ensemble.label))
                          .arg(ensemble.ensemble_id, 4, 16, QChar('0'))  // Use correct field name
                          .arg(ensemble.services.size()));
}

void EtiAnalysisWidget::onStatusChanged(const QString& status)
{
    // Update status display
    if (m_statsLabel) {
        m_statsLabel->setText(status);
    }
    
    Logger::instance().log(Logger::Debug, "EtiAnalysisWidget",
                          QString("Status changed: %1").arg(status));
}

void EtiAnalysisWidget::onErrorOccurred(const QString& error)
{
    // Update error count and display error
    m_errorCount++;
    
    // Create error frame data
    QVariantMap errorData;
    errorData["frameNumber"] = m_frameCount;
    errorData["timestamp"] = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    errorData["type"] = "ERROR";
    errorData["status"] = "Error";
    errorData["error"] = true;
    errorData["errorType"] = "Processing Error";
    errorData["errorDescription"] = error;
    errorData["size"] = 0;
    errorData["quality"] = "0";
    errorData["fic"] = "N/A";
    errorData["msc"] = "N/A";
    
    // Add error to display
    handleFrameDataUpdate(errorData);
    
    // Update error count display
    updateStatisticsDashboard();
    
    // Emit error signal
    emit errorDetected("Processing Error", error);
    
    Logger::instance().log(Logger::Error, "EtiAnalysisWidget",
                          QString("Error occurred: %1").arg(error));
}