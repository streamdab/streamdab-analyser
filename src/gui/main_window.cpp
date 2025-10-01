#include "main_window.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QGroupBox>
#include <QCheckBox>
#include <QTabWidget>
#include <QTimer>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QDateTime>
#include <QDebug>

Phase14DABAnalyzerWindow::Phase14DABAnalyzerWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Phase 1.4 DAB-XPlorer - Bangkok ETI Analysis");
    setMinimumSize(1200, 800);
    resize(1400, 1000);

    // Initialize SIMPLIFIED ARCHITECTURE: Basic components
    initializeSimpleComponents();

    setupUI();
    setupMenuBar();
    setupSignalConnections();
    setupStatusBar();
}

void Phase14DABAnalyzerWindow::initializeSimpleComponents()
{
    qDebug() << "🚀 Initializing SIMPLIFIED ARCHITECTURE: Single thread with timer processing";
    
    // Create basic ETI processor immediately
    m_etiProcessor = std::make_unique<EnhancedETIProcessorQt>();
    
    // Connect basic signals for frame processing (single frame, not batched)
    connect(m_etiProcessor.get(), &EnhancedETIProcessorQt::frameProcessed,
            this, &Phase14DABAnalyzerWindow::onSingleFrameProcessed, Qt::QueuedConnection);
    connect(m_etiProcessor.get(), &EnhancedETIProcessorQt::processingProgress,
            this, &Phase14DABAnalyzerWindow::onProgressUpdated, Qt::QueuedConnection);
    connect(m_etiProcessor.get(), &EnhancedETIProcessorQt::processingComplete,
            this, &Phase14DABAnalyzerWindow::onProcessingCompleted, Qt::QueuedConnection);
    
    // Initialize lightweight components immediately
    m_figAnalyzer = std::make_unique<AdvancedFIGAnalyzer>();
    m_etsiComplianceEngine = std::make_unique<etsi::ETSIComplianceEngine>();
    
    // Initialize error detection for real-time monitoring during ETI processing
    m_errorDetector = std::make_unique<error_detection::AdvancedErrorDetector>();
    
    qDebug() << "✅ SIMPLIFIED ARCHITECTURE: All components ready!";
}

void Phase14DABAnalyzerWindow::setupUI()
{
    // Create main container
    QWidget* mainContainer = new QWidget();
    setCentralWidget(mainContainer);
    QVBoxLayout* containerLayout = new QVBoxLayout(mainContainer);
    
    // Create main splitter layout
    m_mainSplitter = new QSplitter(Qt::Vertical);
    containerLayout->addWidget(m_mainSplitter);

    // Top horizontal splitter
    QSplitter* topSplitter = new QSplitter(Qt::Horizontal);
    m_mainSplitter->addWidget(topSplitter);

    // Left Panel: Ensemble Tree
    createEnsemblePanel(topSplitter);
    
    // Center Panel: Frame Navigator
    createFrameNavigatorPanel(topSplitter);
    
    // Right Panel: Service Details
    createServiceDetailsPanel(topSplitter);

    // Bottom Panel: Analysis Tabs
    createAnalysisTabsPanel(m_mainSplitter);

    // Set sizes (25%, 50%, 25% horizontal; 70%, 30% vertical)
    topSplitter->setSizes({280, 600, 320});
    m_mainSplitter->setSizes({700, 300});
}

void Phase14DABAnalyzerWindow::setupMenuBar()
{
    QMenuBar* menuBar = this->menuBar();
    
    // File menu
    QMenu* fileMenu = menuBar->addMenu("File");
    QAction* openAction = fileMenu->addAction("Open ETI File...");
    connect(openAction, &QAction::triggered, this, &Phase14DABAnalyzerWindow::openFile);
    fileMenu->addSeparator();
    fileMenu->addAction("Exit", this, &QWidget::close);
    
    // View menu
    QMenu* viewMenu = menuBar->addMenu("View");
    viewMenu->addAction("Refresh", [this]() {
        if (m_frameList) m_frameList->clear();
        if (m_serviceTree) m_serviceTree->clear();
    });
    
    // Tools menu
    QMenu* toolsMenu = menuBar->addMenu("Tools");
    toolsMenu->addAction("Error Detection", [this]() { onErrorDetected(); });
}

void Phase14DABAnalyzerWindow::setupSignalConnections()
{
    // Connect frame list selection to hex viewer
    if (m_frameList) {
        connect(m_frameList, &QListWidget::itemSelectionChanged, [this]() {
            int currentRow = m_frameList->currentRow();
            if (currentRow >= 0) {
                displayFrameHexData(currentRow);
            }
        });
    }
}

void Phase14DABAnalyzerWindow::setupStatusBar()
{
    m_statusLabel = new QLabel("Ready");
    statusBar()->addWidget(m_statusLabel);
}

void Phase14DABAnalyzerWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open ETI File", "", "ETI Files (*.eti);;All Files (*)");
    if (!fileName.isEmpty()) {
        m_statusLabel->setText("Processing ETI file...");
        if (m_etiProcessor) {
            m_etiProcessor->processETIFile(fileName);
        }
    }
}

void Phase14DABAnalyzerWindow::onSingleFrameProcessed(const ProcessedFrame& frame)
{
    // UI updates with simulated frame data
    QString frameText = QString("Frame %1: %2 (0x%3)")
                       .arg(frame.frame_number)
                       .arg("ETI-LI") // formatToString(frame.format)
                       .arg(frame.sync_pattern, 8, 16, QChar('0'));
    if (m_frameList) {
        m_frameList->addItem(frameText);
    }
    
    // Trigger error detection for this frame (every 10 frames to avoid overload)
    if (frame.frame_number % 10 == 0 && m_errorDetector) {
        std::vector<uint8_t> frame_data(6144, 0x00);
        QTimer::singleShot(1, this, [this, frame_data, frame]() {
            if (m_errorDetector) {
                m_errorDetector->detectFrameStructureErrors(frame_data, frame.frame_number);
            }
        });
    }
}

void Phase14DABAnalyzerWindow::onProgressUpdated(int percentage, int currentFrame, int totalFrames)
{
    if (m_progressBar) {
        m_progressBar->setValue(percentage);
        m_progressBar->setFormat(QString("Processing: %1 / %2 frames (%3%)")
                                .arg(currentFrame).arg(totalFrames).arg(percentage));
    }
}

void Phase14DABAnalyzerWindow::onProcessingCompleted()
{
    if (m_statusLabel) {
        m_statusLabel->setText("ETI processing completed");
    }
    if (m_progressBar) {
        m_progressBar->setFormat("Processing completed");
    }
}

void Phase14DABAnalyzerWindow::onErrorDetected()
{
    // Simple error detection placeholder
    if (m_statusLabel) {
        m_statusLabel->setText("Error detection running...");
    }
}

// Placeholder implementations for UI creation methods
void Phase14DABAnalyzerWindow::createEnsemblePanel(QSplitter* parent)
{
    QWidget* ensembleWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(ensembleWidget);
    
    layout->addWidget(new QLabel("DAB Ensemble Explorer"));
    m_serviceTree = new QTreeWidget();
    m_serviceTree->setHeaderLabels({"Service", "Label", "Type"});
    layout->addWidget(m_serviceTree);
    
    parent->addWidget(ensembleWidget);
}

void Phase14DABAnalyzerWindow::createFrameNavigatorPanel(QSplitter* parent)
{
    QWidget* frameWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(frameWidget);
    
    // Progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFormat("Ready...");
    m_progressBar->setVisible(true);
    layout->addWidget(m_progressBar);
    
    // Frame list
    layout->addWidget(new QLabel("ETI Frame Navigator"));
    m_frameList = new QListWidget();
    layout->addWidget(m_frameList);
    
    parent->addWidget(frameWidget);
}

void Phase14DABAnalyzerWindow::createServiceDetailsPanel(QSplitter* parent)
{
    QWidget* detailsWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(detailsWidget);
    
    // Service information
    QGroupBox* serviceGroup = new QGroupBox("Current Service");
    QVBoxLayout* serviceLayout = new QVBoxLayout(serviceGroup);
    QLabel* serviceInfo = new QLabel("No service selected");
    serviceInfo->setWordWrap(true);
    serviceLayout->addWidget(serviceInfo);
    layout->addWidget(serviceGroup);
    
    layout->addStretch();
    parent->addWidget(detailsWidget);
}

void Phase14DABAnalyzerWindow::createAnalysisTabsPanel(QSplitter* parent)
{
    m_analysisTabs = new QTabWidget();
    
    // Tab 1: Ensemble Analysis
    m_analysisTabs->addTab(createEnsembleAnalysisTab(), "Ensemble Analysis");
    
    // Tab 2: FIC Analysis
    m_analysisTabs->addTab(createFICAnalysisTab(), "FIC Analysis");
    
    // Tab 3: Service Analysis
    m_analysisTabs->addTab(createServiceAnalysisTab(), "Service Analysis");
    
    // Tab 4: Error Analysis
    m_analysisTabs->addTab(createErrorAnalysisTab(), "Error Analysis");
    
    // Tab 5: Audio Analysis
    m_analysisTabs->addTab(createAudioAnalysisTab(), "Audio Analysis");
    
    parent->addWidget(m_analysisTabs);
}

QWidget* Phase14DABAnalyzerWindow::createEnsembleAnalysisTab()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->addWidget(new QLabel("Ensemble Analysis - Placeholder"));
    return widget;
}

QWidget* Phase14DABAnalyzerWindow::createFICAnalysisTab()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->addWidget(new QLabel("FIC Analysis - Placeholder"));
    return widget;
}

QWidget* Phase14DABAnalyzerWindow::createServiceAnalysisTab()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->addWidget(new QLabel("Service Analysis - Placeholder"));
    return widget;
}

QWidget* Phase14DABAnalyzerWindow::createErrorAnalysisTab()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->addWidget(new QLabel("Error Analysis - Placeholder"));
    return widget;
}

QWidget* Phase14DABAnalyzerWindow::createAudioAnalysisTab()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->addWidget(new QLabel("Audio Analysis - Placeholder"));
    return widget;
}

void Phase14DABAnalyzerWindow::displayFrameHexData(int frameIndex)
{
    // Placeholder implementation
    if (m_statusLabel) {
        m_statusLabel->setText(QString("Displaying hex data for frame %1").arg(frameIndex + 1));
    }
}

// MOC is handled automatically by CMake