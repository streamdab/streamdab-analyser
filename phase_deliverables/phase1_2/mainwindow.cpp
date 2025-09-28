#include "mainwindow.h"
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_etiProcessor(new EnhancedETIProcessorQt(this))
    , m_currentFileName("No file loaded")
    , m_totalFramesExpected(0)
    , m_totalFramesProcessed(0)
    , m_figCount(0)
{
    setWindowTitle("StreamDAB Analyzer - Phase 1.2 Qt GUI Integration");
    setWindowIcon(QIcon()); // TODO: Add application icon
    resize(1000, 700);
    
    setupUI();
    applyProfessionalTheme();
    
    // Connect ETI processor signals
    connect(m_etiProcessor, &EnhancedETIProcessorQt::processingStarted,
            this, &MainWindow::onProcessingStarted);
    connect(m_etiProcessor, &EnhancedETIProcessorQt::frameProcessed,
            this, &MainWindow::onFrameProcessed);
    connect(m_etiProcessor, &EnhancedETIProcessorQt::processingProgress,
            this, &MainWindow::onProcessingProgress);
    connect(m_etiProcessor, &EnhancedETIProcessorQt::processingComplete,
            this, &MainWindow::onProcessingComplete);
    connect(m_etiProcessor, &EnhancedETIProcessorQt::processingError,
            this, &MainWindow::onProcessingError);
    connect(m_etiProcessor, &EnhancedETIProcessorQt::formatDetected,
            this, &MainWindow::onFormatDetected);
    connect(m_etiProcessor, &EnhancedETIProcessorQt::figDiscovered,
            this, &MainWindow::onFigDiscovered);
    
    // Initialize display
    clearDisplay();
    m_statusBarLabel->setText("Ready - Click 'Open ETI File' to begin");
}

MainWindow::~MainWindow()
{
    if (m_etiProcessor) {
        m_etiProcessor->stopProcessing();
    }
}

void MainWindow::setupUI()
{
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupCentralWidget();
}

void MainWindow::setupMenuBar()
{
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");
    
    m_openAction = new QAction("&Open ETI File...", this);
    m_openAction->setShortcut(QKeySequence::Open);
    m_openAction->setStatusTip("Open an ETI file for analysis");
    connect(m_openAction, &QAction::triggered, this, &MainWindow::openETIFile);
    fileMenu->addAction(m_openAction);
    
    fileMenu->addSeparator();
    
    m_exitAction = new QAction("E&xit", this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip("Exit the application");
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(m_exitAction);
    
    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    QAction* aboutAction = new QAction("&About", this);
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "About StreamDAB Analyzer Phase 1.2",
            "StreamDAB Analyzer - Phase 1.2 Qt GUI Integration\n\n"
            "Features:\n"
            "✅ Multi-format ETI processing (ETI-NI + ETI-LI)\n"
            "✅ Bangkok DAB file support\n"
            "✅ Real-time processing display\n"
            "✅ ETSI EN 300 799 compliance\n\n"
            "Version: 1.2.0\n"
            "Built with Qt6 + C++20");
    });
    helpMenu->addAction(aboutAction);
}

void MainWindow::setupToolBar()
{
    QToolBar* toolbar = addToolBar("Main");
    toolbar->addAction(m_openAction);
    toolbar->addSeparator();
    
    // Add quick info label
    QLabel* toolbarLabel = new QLabel("Phase 1.2: ETI-LI Bangkok File Support");
    toolbarLabel->setStyleSheet("QLabel { margin-left: 10px; font-weight: bold; }");
    toolbar->addWidget(toolbarLabel);
}

void MainWindow::setupStatusBar()
{
    m_statusBarLabel = new QLabel("Ready");
    statusBar()->addWidget(m_statusBarLabel, 1);
    
    m_versionLabel = new QLabel("Phase 1.2 v1.2.0");
    statusBar()->addPermanentWidget(m_versionLabel);
}

void MainWindow::setupCentralWidget()
{
    m_centralWidget = new QWidget();
    setCentralWidget(m_centralWidget);
    
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // File operations panel
    m_fileGroup = new QGroupBox("File Operations");
    m_fileLayout = new QHBoxLayout(m_fileGroup);
    
    m_openButton = new QPushButton("Open ETI File");
    m_openButton->setMinimumHeight(40);
    connect(m_openButton, &QPushButton::clicked, this, &MainWindow::openETIFile);
    
    m_fileLabel = new QLabel(m_currentFileName);
    m_fileLabel->setStyleSheet("QLabel { font-weight: bold; color: #0078D4; }");
    
    m_fileLayout->addWidget(m_openButton);
    m_fileLayout->addWidget(m_fileLabel, 1);
    m_mainLayout->addWidget(m_fileGroup);
    
    // Processing status panel
    m_statusGroup = new QGroupBox("Processing Status");
    m_statusLayout = new QGridLayout(m_statusGroup);
    
    m_statusLabel = new QLabel("Ready");
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_frameCountLabel = new QLabel("Frames: 0 / 0");
    m_formatLabel = new QLabel("Format: Not detected");
    m_processingTimeLabel = new QLabel("Time: 0 ms");
    
    m_statusLayout->addWidget(new QLabel("Status:"), 0, 0);
    m_statusLayout->addWidget(m_statusLabel, 0, 1);
    m_statusLayout->addWidget(new QLabel("Progress:"), 1, 0);
    m_statusLayout->addWidget(m_progressBar, 1, 1);
    m_statusLayout->addWidget(m_frameCountLabel, 0, 2);
    m_statusLayout->addWidget(m_formatLabel, 1, 2);
    m_statusLayout->addWidget(m_processingTimeLabel, 0, 3);
    
    m_mainLayout->addWidget(m_statusGroup);
    
    // Create horizontal layout for frame analysis and statistics
    QHBoxLayout* analysisLayout = new QHBoxLayout();
    
    // Frame analysis panel
    m_frameGroup = new QGroupBox("Current Frame Analysis");
    m_frameLayout = new QGridLayout(m_frameGroup);
    
    m_currentFrameLabel = new QLabel("Frame: -");
    m_syncPatternLabel = new QLabel("SYNC: -");
    m_lidataLabel = new QLabel("LIDATA: -");
    m_crcLabel = new QLabel("CRC: -");
    m_ficSizeLabel = new QLabel("FIC: - bytes");
    m_mscSizeLabel = new QLabel("MSC: - bytes");
    
    m_frameLayout->addWidget(new QLabel("Frame #:"), 0, 0);
    m_frameLayout->addWidget(m_currentFrameLabel, 0, 1);
    m_frameLayout->addWidget(new QLabel("SYNC Pattern:"), 1, 0);
    m_frameLayout->addWidget(m_syncPatternLabel, 1, 1);
    m_frameLayout->addWidget(new QLabel("LIDATA:"), 2, 0);
    m_frameLayout->addWidget(m_lidataLabel, 2, 1);
    m_frameLayout->addWidget(new QLabel("CRC:"), 0, 2);
    m_frameLayout->addWidget(m_crcLabel, 0, 3);
    m_frameLayout->addWidget(new QLabel("FIC Size:"), 1, 2);
    m_frameLayout->addWidget(m_ficSizeLabel, 1, 3);
    m_frameLayout->addWidget(new QLabel("MSC Size:"), 2, 2);
    m_frameLayout->addWidget(m_mscSizeLabel, 2, 3);
    
    analysisLayout->addWidget(m_frameGroup);
    
    // Statistics panel
    m_statsGroup = new QGroupBox("Processing Statistics");
    m_statsLayout = new QGridLayout(m_statsGroup);
    
    m_totalFramesLabel = new QLabel("Total: 0");
    m_etiNICountLabel = new QLabel("ETI-NI: 0");
    m_etiLIACountLabel = new QLabel("ETI-LI A: 0");
    m_etiLIBCountLabel = new QLabel("ETI-LI B: 0");
    m_processingRateLabel = new QLabel("Rate: 0 FPS");
    
    m_statsLayout->addWidget(new QLabel("Total Frames:"), 0, 0);
    m_statsLayout->addWidget(m_totalFramesLabel, 0, 1);
    m_statsLayout->addWidget(new QLabel("ETI-NI Count:"), 1, 0);
    m_statsLayout->addWidget(m_etiNICountLabel, 1, 1);
    m_statsLayout->addWidget(new QLabel("ETI-LI Pattern A:"), 2, 0);
    m_statsLayout->addWidget(m_etiLIACountLabel, 2, 1);
    m_statsLayout->addWidget(new QLabel("ETI-LI Pattern B:"), 3, 0);
    m_statsLayout->addWidget(m_etiLIBCountLabel, 3, 1);
    m_statsLayout->addWidget(new QLabel("Processing Rate:"), 4, 0);
    m_statsLayout->addWidget(m_processingRateLabel, 4, 1);
    
    analysisLayout->addWidget(m_statsGroup);
    m_mainLayout->addLayout(analysisLayout);
    
    // Processing log
    m_logGroup = new QGroupBox("Processing Log");
    m_logLayout = new QVBoxLayout(m_logGroup);
    
    m_logDisplay = new QTextEdit();
    m_logDisplay->setMaximumHeight(200);
    m_logDisplay->setReadOnly(true);
    m_logDisplay->setFont(QFont("Consolas", 9));
    
    m_logLayout->addWidget(m_logDisplay);
    m_mainLayout->addWidget(m_logGroup);
}

void MainWindow::applyProfessionalTheme()
{
    // Professional broadcast industry color scheme
    setStyleSheet(
        "QMainWindow {"
        "    background-color: #2D2D30;"
        "    color: white;"
        "}"
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #3E3E42;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px 0 5px;"
        "}"
        "QPushButton {"
        "    background-color: #0078D4;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 4px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #106EBE;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #005A9E;"
        "}"
        "QLabel {"
        "    color: white;"
        "}"
        "QTextEdit {"
        "    background-color: #1E1E1E;"
        "    border: 1px solid #3E3E42;"
        "    color: white;"
        "}"
        "QProgressBar {"
        "    border: 2px solid #3E3E42;"
        "    border-radius: 5px;"
        "    text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #0078D4;"
        "    border-radius: 3px;"
        "}"
        "QMenuBar {"
        "    background-color: #2D2D30;"
        "    color: white;"
        "}"
        "QMenuBar::item:selected {"
        "    background-color: #0078D4;"
        "}"
        "QMenu {"
        "    background-color: #2D2D30;"
        "    color: white;"
        "    border: 1px solid #3E3E42;"
        "}"
        "QMenu::item:selected {"
        "    background-color: #0078D4;"
        "}"
        "QToolBar {"
        "    background-color: #2D2D30;"
        "    border: none;"
        "}"
        "QStatusBar {"
        "    background-color: #007ACC;"
        "    color: white;"
        "}"
    );
}

void MainWindow::openETIFile()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Open ETI File",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "ETI Files (*.eti);;All Files (*)"
    );
    
    if (!fileName.isEmpty()) {
        clearDisplay();
        m_currentFileName = QDir::toNativeSeparators(fileName);
        m_fileLabel->setText(QFileInfo(fileName).fileName());
        
        m_logDisplay->append(QString("Loading file: %1").arg(QFileInfo(fileName).fileName()));
        
        if (m_etiProcessor->processETIFile(fileName)) {
            m_logDisplay->append("File loaded successfully, starting processing...");
        }
    }
}

void MainWindow::onProcessingStarted(const QString& fileName, int estimatedFrames)
{
    m_totalFramesExpected = estimatedFrames;
    m_processingStartTime = QDateTime::currentDateTime();
    
    m_statusLabel->setText("Processing...");
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    
    m_logDisplay->append(QString("Processing started: %1 frames estimated").arg(estimatedFrames));
    m_statusBarLabel->setText(QString("Processing %1...").arg(fileName));
}

void MainWindow::onFrameProcessed(const ProcessedFrame& frame)
{
    updateFrameDisplay(frame);
    updateStatistics();
    
    // Log frame processing (limit to avoid spam)
    if (frame.frame_number <= 5 || frame.frame_number % 5 == 0) {
        m_logDisplay->append(QString("Frame %1: %2 (SYNC: %3)")
                           .arg(frame.frame_number)
                           .arg(m_etiProcessor->formatToString(frame.format))
                           .arg(m_etiProcessor->syncPatternToString(frame.sync_pattern)));
    }
}

void MainWindow::onProcessingProgress(int percentage, int currentFrame, int totalFrames)
{
    m_progressBar->setValue(percentage);
    m_frameCountLabel->setText(QString("Frames: %1 / %2").arg(currentFrame).arg(totalFrames));
    
    QApplication::processEvents(); // Keep UI responsive
}

void MainWindow::onProcessingComplete(int totalFrames, int processingTimeMs)
{
    m_progressBar->setVisible(false);
    m_statusLabel->setText("Complete");
    
    updateStatistics();
    
    double fps = totalFrames / (processingTimeMs / 1000.0);
    m_processingRateLabel->setText(QString("Rate: %1 FPS").arg(fps, 0, 'f', 1));
    m_processingTimeLabel->setText(QString("Time: %1 ms").arg(processingTimeMs));
    
    m_logDisplay->append(QString("Processing complete: %1 frames in %2 ms (%.1f FPS)")
                       .arg(totalFrames).arg(processingTimeMs).arg(fps));
    m_statusBarLabel->setText("Processing complete - Bangkok ETI-LI file processed successfully!");
}

void MainWindow::onProcessingError(const QString& error)
{
    m_progressBar->setVisible(false);
    m_statusLabel->setText("Error");
    
    m_logDisplay->append(QString("ERROR: %1").arg(error));
    m_statusBarLabel->setText(QString("Error: %1").arg(error));
    
    QMessageBox::warning(this, "Processing Error", error);
}

void MainWindow::onFormatDetected(ETIFormat format, const QString& formatName)
{
    m_formatLabel->setText(QString("Format: %1").arg(formatName));
    
    // Color-code format detection
    if (format == ETIFormat::ETI_LI_A || format == ETIFormat::ETI_LI_B) {
        m_formatLabel->setStyleSheet("QLabel { color: #00D000; font-weight: bold; }");
    } else if (format == ETIFormat::ETI_NI) {
        m_formatLabel->setStyleSheet("QLabel { color: #0078D4; font-weight: bold; }");
    }
}

void MainWindow::onFigDiscovered(const FIGInfo& fig)
{
    m_figCount++;
    
    // Log FIG discovery (limit output)
    if (m_figCount <= 10) {
        m_logDisplay->append(QString("  FIG Type %1 discovered (length: %2 bytes)")
                           .arg(fig.type).arg(fig.length));
    }
}

void MainWindow::updateFrameDisplay(const ProcessedFrame& frame)
{
    m_currentFrameLabel->setText(QString::number(frame.frame_number));
    m_syncPatternLabel->setText(m_etiProcessor->syncPatternToString(frame.sync_pattern));
    m_lidataLabel->setText(QString("0x%1").arg(frame.lidata, 8, 16, QChar('0')));
    m_crcLabel->setText(QString("0x%1").arg(frame.crc, 8, 16, QChar('0')));
    m_ficSizeLabel->setText(QString("%1 bytes").arg(frame.fic_data.size()));
    m_mscSizeLabel->setText(QString("%1 bytes").arg(frame.msc_data.size()));
}

void MainWindow::updateStatistics()
{
    m_totalFramesLabel->setText(QString::number(m_etiProcessor->getFrameCount()));
    m_etiNICountLabel->setText(QString::number(m_etiProcessor->getETINICount()));
    m_etiLIACountLabel->setText(QString::number(m_etiProcessor->getETILIACount()));
    m_etiLIBCountLabel->setText(QString::number(m_etiProcessor->getETILIBCount()));
}

void MainWindow::clearDisplay()
{
    m_statusLabel->setText("Ready");
    m_progressBar->setVisible(false);
    m_frameCountLabel->setText("Frames: 0 / 0");
    m_formatLabel->setText("Format: Not detected");
    m_processingTimeLabel->setText("Time: 0 ms");
    
    m_currentFrameLabel->setText("-");
    m_syncPatternLabel->setText("-");
    m_lidataLabel->setText("-");
    m_crcLabel->setText("-");
    m_ficSizeLabel->setText("- bytes");
    m_mscSizeLabel->setText("- bytes");
    
    m_totalFramesLabel->setText("0");
    m_etiNICountLabel->setText("0");
    m_etiLIACountLabel->setText("0");
    m_etiLIBCountLabel->setText("0");
    m_processingRateLabel->setText("0 FPS");
    
    m_logDisplay->clear();
    m_figCount = 0;
}

void MainWindow::onRecentFileTriggered()
{
    // TODO: Implement recent files functionality
}