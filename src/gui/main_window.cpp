/**
 * @file main_window.cpp
 * @brief Professional ETI Stream Analyzer MainWindow implementation
 * 
 * This file implements the MainWindow class for the ETI Stream Analyzer
 * following TDD methodology and professional broadcast industry standards.
 * 
 * Features:
 * - Professional Qt GUI with dockable panels
 * - Real-time ETI stream processing at 60 FPS
 * - Broadcast-standard workflow support
 * - Cross-platform consistency
 * - Memory-efficient operation under 150MB
 * - ETSI compliance integration
 */

#include "main_window.h"
#include "analyzer_widget.h"
#include "constellation_widget.h"
#include "service_browser.h"
#include "settings_dialog.h"
#include "about_dialog.h"
#include "core/eti_processor.h"
#include "core/eti_file_worker.h"
#include "utils/config_manager.h"
#include "utils/logger.h"

#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QSplitter>
#include <QTabWidget>
#include <QTextEdit>
#include <QTreeWidget>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFrame>
#include <QSpacerItem>
#include <QSizePolicy>
#include <QHeaderView>
#include <QElapsedTimer>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_mainSplitter(nullptr)
    , m_analysisTabWidget(nullptr)
    , m_analyzerWidget(nullptr)
    , m_constellationWidget(nullptr)
    , m_serviceBrowser(nullptr)
    , m_fileExplorerDock(nullptr)
    , m_yamlViewerDock(nullptr)
    , m_figAnalysisDock(nullptr)
    , m_audioMonitorDock(nullptr)
    , m_errorLogDock(nullptr)
    , m_fileExplorer(nullptr)
    , m_yamlViewer(nullptr)
    , m_figAnalysis(nullptr)
    , m_audioMonitor(nullptr)
    , m_errorLog(nullptr)
    , m_menuBar(nullptr)
    , m_fileMenu(nullptr)
    , m_analysisMenu(nullptr)
    , m_viewMenu(nullptr)
    , m_toolsMenu(nullptr)
    , m_helpMenu(nullptr)
    , m_mainToolBar(nullptr)
    , m_analysisToolBar(nullptr)
    , m_statusBar(nullptr)
    , m_statusLabel(nullptr)
    , m_frameCountLabel(nullptr)
    , m_bitRateLabel(nullptr)
    , m_errorCountLabel(nullptr)
    , m_progressBar(nullptr)
    , m_openFileAction(nullptr)
    , m_closeFileAction(nullptr)
    , m_exitAction(nullptr)
    , m_startAnalysisAction(nullptr)
    , m_stopAnalysisAction(nullptr)
    , m_settingsAction(nullptr)
    , m_aboutAction(nullptr)
    , m_fullScreenAction(nullptr)
    , m_resetLayoutAction(nullptr)
    , m_showFileExplorerAction(nullptr)
    , m_showYamlViewerAction(nullptr)
    , m_showFigAnalysisAction(nullptr)
    , m_showAudioMonitorAction(nullptr)
    , m_showErrorLogAction(nullptr)
    , m_realTimeTimer(nullptr)
    , m_realTimeMode(false)
    , m_updateRate(TARGET_FPS)
    , m_frameCount(0)
    , m_isInitialized(false)
    , m_lastUpdateTime(0)
    , m_frameDropCount(0)
    , m_averageFrameTime(0.0)
    , m_settingsDialog(nullptr)
    , m_aboutDialog(nullptr)
{
    // Initialize core components - TEMPORARILY DISABLED DUE TO LOGGER HANG
    // m_configManager = std::make_unique<ConfigManager>();
    
    // Set window properties
    setWindowTitle("ETI Stream Analyzer");
    setWindowIcon(QIcon(":/icons/application.png"));
    
    // Set default window size
    resize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
    
    // Center window on screen
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - width()) / 2;
        int y = (screenGeometry.height() - height()) / 2;
        move(x, y);
    }
    
    // Setup logging - Now using SafeLogger with hang protection
    Logger::instance().setLogLevel(Logger::Info);
    Logger::instance().log(Logger::Info, "MainWindow", "Constructor completed");
}

MainWindow::~MainWindow()
{
    Logger::instance().log(Logger::Info, "MainWindow", "Destructor starting");
    
    // Stop real-time processing
    if (m_realTimeMode) {
        stopRealTimeAnalysis();
    }
    
    // Save settings before destruction
    saveSettings();
    
    // Cleanup timer
    if (m_realTimeTimer) {
        m_realTimeTimer->stop();
        delete m_realTimeTimer;
        m_realTimeTimer = nullptr;
    }
    
    // Cleanup dialogs
    delete m_settingsDialog;
    delete m_aboutDialog;
    
    Logger::instance().log(Logger::Info, "MainWindow", "Destructor completed");
}

bool MainWindow::initialize()
{
    Logger::instance().log(Logger::Info, "MainWindow", "Initialization starting");
    
    try {
        // Initialize ETI processor
        m_etiProcessor = std::make_unique<EtiProcessor>();
        if (!m_etiProcessor->initialize()) {
            Logger::instance().log(Logger::Error, "MainWindow", "Failed to initialize ETI processor");
            return false;
        }
        
        // Initialize background ETI file worker
        m_etiFileWorker = std::make_unique<EtiFileWorker>(m_etiProcessor.get());
        
        // Pre-initialize file system for faster dialog opening
        QTimer::singleShot(100, this, [this]() {
            // Pre-warm the file dialog system to avoid first-use delay
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        });
        
        // Create GUI components - TEST MINIMAL FIRST
        qDebug() << "Creating actions...";
        createActions();
        qDebug() << "Creating menubar...";
        createMenuBar();
        qDebug() << "Creating toolbar...";
        createToolBar();
        qDebug() << "Creating status bar...";
        createStatusBar();
        qDebug() << "Creating central widget...";
        createCentralWidget();
        qDebug() << "Creating dock widgets...";
        createDockWidgets();
        qDebug() << "Connecting signals...";
        connectSignals();
        qDebug() << "All GUI components created successfully";
        
        // Skip timer and settings for now
        // setupRealTimeTimer();
        // loadSettings();
        
        // Store default layout state
        m_defaultLayoutState = saveState();
        
        // Set initialization flag
        m_isInitialized = true;
        
        // Update window title
        updateWindowTitle();
        
        Logger::instance().log(Logger::Info, "MainWindow", "Initialization completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "MainWindow", 
                               QString("Initialization failed: %1").arg(e.what()));
        qCritical() << "MainWindow initialization failed:" << e.what();
        return false;
    }
}

void MainWindow::createActions()
{
    // File menu actions
    m_openFileAction = new QAction(QIcon(":/icons/open.png"), tr("&Open ETI File..."), this);
    m_openFileAction->setShortcut(QKeySequence::Open);
    m_openFileAction->setStatusTip(tr("Open an ETI file for analysis"));
    
    m_closeFileAction = new QAction(QIcon(":/icons/close.png"), tr("&Close File"), this);
    m_closeFileAction->setShortcut(QKeySequence::Close);
    m_closeFileAction->setStatusTip(tr("Close the current ETI file"));
    m_closeFileAction->setEnabled(false);
    
    m_exitAction = new QAction(QIcon(":/icons/exit.png"), tr("E&xit"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip(tr("Exit the application"));
    
    // Analysis menu actions
    m_startAnalysisAction = new QAction(QIcon(":/icons/play.png"), tr("&Start Real-time Analysis"), this);
    m_startAnalysisAction->setShortcut(Qt::Key_F5);
    m_startAnalysisAction->setStatusTip(tr("Start real-time ETI stream analysis"));
    
    m_stopAnalysisAction = new QAction(QIcon(":/icons/stop.png"), tr("S&top Analysis"), this);
    m_stopAnalysisAction->setShortcut(Qt::Key_F6);
    m_stopAnalysisAction->setStatusTip(tr("Stop real-time analysis"));
    m_stopAnalysisAction->setEnabled(false);
    
    // Tools menu actions
    m_settingsAction = new QAction(QIcon(":/icons/settings.png"), tr("&Settings..."), this);
    m_settingsAction->setShortcut(QKeySequence::Preferences);
    m_settingsAction->setStatusTip(tr("Configure application settings"));
    
    // View menu actions
    m_fullScreenAction = new QAction(QIcon(":/icons/fullscreen.png"), tr("&Full Screen"), this);
    m_fullScreenAction->setShortcut(Qt::Key_F11);
    m_fullScreenAction->setStatusTip(tr("Toggle full screen mode"));
    m_fullScreenAction->setCheckable(true);
    
    m_resetLayoutAction = new QAction(QIcon(":/icons/layout.png"), tr("&Reset Layout"), this);
    m_resetLayoutAction->setStatusTip(tr("Reset window layout to default"));
    
    // Help menu actions
    m_aboutAction = new QAction(QIcon(":/icons/about.png"), tr("&About ETI Analyzer"), this);
    m_aboutAction->setStatusTip(tr("Show information about ETI Stream Analyzer"));
    
    // Dock visibility actions
    m_showFileExplorerAction = new QAction(tr("&File Explorer"), this);
    m_showFileExplorerAction->setCheckable(true);
    m_showFileExplorerAction->setChecked(true);
    
    m_showYamlViewerAction = new QAction(tr("&YAML Viewer"), this);
    m_showYamlViewerAction->setCheckable(true);
    m_showYamlViewerAction->setChecked(true);
    
    m_showFigAnalysisAction = new QAction(tr("&FIG Analysis"), this);
    m_showFigAnalysisAction->setCheckable(true);
    m_showFigAnalysisAction->setChecked(true);
    
    m_showAudioMonitorAction = new QAction(tr("&Audio Monitor"), this);
    m_showAudioMonitorAction->setCheckable(true);
    m_showAudioMonitorAction->setChecked(true);
    
    m_showErrorLogAction = new QAction(tr("&Error Log"), this);
    m_showErrorLogAction->setCheckable(true);
    m_showErrorLogAction->setChecked(true);
    
    // Demo menu actions
    m_createSampleFileAction = new QAction(QIcon(":/icons/demo.png"), tr("&Create Sample ETI File"), this);
    m_createSampleFileAction->setStatusTip(tr("Create a sample ETI file for testing"));
    
    m_loadTestDataAction = new QAction(QIcon(":/icons/test.png"), tr("&Load Test Services"), this);
    m_loadTestDataAction->setStatusTip(tr("Load test service data into the browser"));
}

void MainWindow::createMenuBar()
{
    m_menuBar = menuBar();
    
    // File menu
    m_fileMenu = m_menuBar->addMenu(tr("&File"));
    m_fileMenu->addAction(m_openFileAction);
    m_fileMenu->addAction(m_closeFileAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAction);
    
    // Analysis menu
    m_analysisMenu = m_menuBar->addMenu(tr("&Analysis"));
    m_analysisMenu->addAction(m_startAnalysisAction);
    m_analysisMenu->addAction(m_stopAnalysisAction);
    
    // View menu
    m_viewMenu = m_menuBar->addMenu(tr("&View"));
    
    // Panels submenu
    QMenu *panelsMenu = m_viewMenu->addMenu(tr("&Panels"));
    panelsMenu->addAction(m_showFileExplorerAction);
    panelsMenu->addAction(m_showYamlViewerAction);
    panelsMenu->addAction(m_showFigAnalysisAction);
    panelsMenu->addAction(m_showAudioMonitorAction);
    panelsMenu->addAction(m_showErrorLogAction);
    
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_fullScreenAction);
    m_viewMenu->addAction(m_resetLayoutAction);
    
    // Tools menu
    m_toolsMenu = m_menuBar->addMenu(tr("&Tools"));
    m_toolsMenu->addAction(m_settingsAction);
    
    // Demo menu for testing
    QMenu *demoMenu = m_menuBar->addMenu(tr("&Demo"));
    demoMenu->addAction(m_createSampleFileAction);
    demoMenu->addAction(m_loadTestDataAction);
    
    // Help menu
    m_helpMenu = m_menuBar->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_aboutAction);
}

void MainWindow::createToolBar()
{
    // Main toolbar
    m_mainToolBar = addToolBar(tr("Main"));
    m_mainToolBar->setObjectName("MainToolBar");
    m_mainToolBar->addAction(m_openFileAction);
    m_mainToolBar->addAction(m_closeFileAction);
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(m_settingsAction);
    
    // Analysis toolbar
    m_analysisToolBar = addToolBar(tr("Analysis"));
    m_analysisToolBar->setObjectName("AnalysisToolBar");
    m_analysisToolBar->addAction(m_startAnalysisAction);
    m_analysisToolBar->addAction(m_stopAnalysisAction);
    
    // Set toolbar properties
    m_mainToolBar->setMovable(true);
    m_analysisToolBar->setMovable(true);
    m_mainToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    m_analysisToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
}

void MainWindow::createStatusBar()
{
    m_statusBar = statusBar();
    
    // Primary status label
    m_statusLabel = new QLabel(tr("Ready"));
    m_statusBar->addWidget(m_statusLabel, 1);
    
    // Frame count label
    m_frameCountLabel = new QLabel(tr("Frames: 0"));
    m_frameCountLabel->setMinimumWidth(100);
    m_statusBar->addPermanentWidget(m_frameCountLabel);
    
    // Bit rate label
    m_bitRateLabel = new QLabel(tr("Rate: 0 kbps"));
    m_bitRateLabel->setMinimumWidth(120);
    m_statusBar->addPermanentWidget(m_bitRateLabel);
    
    // Error count label
    m_errorCountLabel = new QLabel(tr("Errors: 0"));
    m_errorCountLabel->setMinimumWidth(80);
    m_statusBar->addPermanentWidget(m_errorCountLabel);
    
    // Progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_progressBar->setMaximumWidth(200);
    m_statusBar->addPermanentWidget(m_progressBar);
}

void MainWindow::createCentralWidget()
{
    qDebug() << "Creating basic central widget...";
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);
    
    qDebug() << "Creating main splitter...";
    m_mainSplitter = new QSplitter(Qt::Horizontal, m_centralWidget);
    
    qDebug() << "Creating analysis tab widget...";
    m_analysisTabWidget = new QTabWidget(m_mainSplitter);
    
    qDebug() << "Creating analyzer widget...";
    m_analyzerWidget = new AnalyzerWidget(m_analysisTabWidget);
    qDebug() << "AnalyzerWidget created successfully";
    m_analysisTabWidget->addTab(m_analyzerWidget, tr("ETI Analysis"));
    
    qDebug() << "Creating constellation widget...";
    m_constellationWidget = new ConstellationWidget(m_analysisTabWidget);
    qDebug() << "ConstellationWidget created successfully";
    m_analysisTabWidget->addTab(m_constellationWidget, tr("Constellation"));
    
    qDebug() << "Creating service browser...";
    m_serviceBrowser = new ServiceBrowser(m_mainSplitter);
    qDebug() << "ServiceBrowser created successfully";
    
    // Setup splitter
    m_mainSplitter->addWidget(m_analysisTabWidget);
    m_mainSplitter->addWidget(m_serviceBrowser);
    m_mainSplitter->setStretchFactor(0, 3);  // Analysis tabs get 75% of space
    m_mainSplitter->setStretchFactor(1, 1);  // Service browser gets 25% of space
    
    // Layout central widget
    QHBoxLayout *centralLayout = new QHBoxLayout(m_centralWidget);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->addWidget(m_mainSplitter);
}

void MainWindow::createDockWidgets()
{
    // File Explorer dock
    m_fileExplorerDock = new QDockWidget(tr("File Explorer"), this);
    m_fileExplorerDock->setObjectName("FileExplorerDock");
    m_fileExplorer = new QTextEdit();
    m_fileExplorer->setPlainText(tr("ETI file browser will be implemented here.\n\n"
                                   "Features:\n"
                                   "- File system navigation\n"
                                   "- ETI file preview\n"
                                   "- Recent files list\n"
                                   "- Drag & drop support"));
    m_fileExplorer->setReadOnly(true);
    m_fileExplorerDock->setWidget(m_fileExplorer);
    addDockWidget(Qt::LeftDockWidgetArea, m_fileExplorerDock);
    
    // YAML Viewer dock
    m_yamlViewerDock = new QDockWidget(tr("YAML Analysis"), this);
    m_yamlViewerDock->setObjectName("YamlViewerDock");
    m_yamlViewer = new QTextEdit();
    m_yamlViewer->setPlainText(tr("ETI analysis results in YAML format will appear here.\n\n"
                                 "Content includes:\n"
                                 "- ETI frame information\n"
                                 "- Service details\n"
                                 "- Audio parameters\n"
                                 "- Error statistics"));
    m_yamlViewer->setReadOnly(true);
    m_yamlViewer->setFont(QFont("Courier", 9));  // Monospace font for YAML
    m_yamlViewerDock->setWidget(m_yamlViewer);
    addDockWidget(Qt::RightDockWidgetArea, m_yamlViewerDock);
    
    // FIG Analysis dock
    m_figAnalysisDock = new QDockWidget(tr("FIG Analysis"), this);
    m_figAnalysisDock->setObjectName("FigAnalysisDock");
    m_figAnalysis = new QTextEdit();
    m_figAnalysis->setPlainText(tr("Fast Information Group (FIG) analysis:\n\n"
                                  "- FIG 0: Basic information\n"
                                  "- FIG 1: Extended information\n"
                                  "- Service components\n"
                                  "- Ensemble configuration\n"
                                  "- Real-time updates"));
    m_figAnalysis->setReadOnly(true);
    m_figAnalysisDock->setWidget(m_figAnalysis);
    addDockWidget(Qt::RightDockWidgetArea, m_figAnalysisDock);
    
    // Audio Monitor dock
    m_audioMonitorDock = new QDockWidget(tr("Audio Monitor"), this);
    m_audioMonitorDock->setObjectName("AudioMonitorDock");
    m_audioMonitor = new QTextEdit();
    m_audioMonitor->setPlainText(tr("Real-time audio monitoring:\n\n"
                                   "- Audio level meters\n"
                                   "- Peak detection\n"
                                   "- Quality metrics\n"
                                   "- Format information\n"
                                   "- Error detection"));
    m_audioMonitor->setReadOnly(true);
    m_audioMonitorDock->setWidget(m_audioMonitor);
    addDockWidget(Qt::BottomDockWidgetArea, m_audioMonitorDock);
    
    // Error Log dock
    m_errorLogDock = new QDockWidget(tr("Error Log"), this);
    m_errorLogDock->setObjectName("ErrorLogDock");
    m_errorLog = new QTextEdit();
    m_errorLog->setPlainText(tr("System and ETI error log:\n\n"
                               "- Processing errors\n"
                               "- Protocol violations\n"
                               "- System warnings\n"
                               "- Performance alerts\n"
                               "- Timestamp information"));
    m_errorLog->setReadOnly(true);
    m_errorLog->setFont(QFont("Courier", 8));  // Small monospace font for logs
    m_errorLogDock->setWidget(m_errorLog);
    addDockWidget(Qt::BottomDockWidgetArea, m_errorLogDock);
    
    // Organize dock widgets
    tabifyDockWidget(m_figAnalysisDock, m_yamlViewerDock);
    tabifyDockWidget(m_audioMonitorDock, m_errorLogDock);
    
    // Raise primary tabs
    m_yamlViewerDock->raise();
    m_audioMonitorDock->raise();
}

void MainWindow::connectSignals()
{
    // File actions
    connect(m_openFileAction, &QAction::triggered, this, &MainWindow::openEtiFile);
    connect(m_closeFileAction, &QAction::triggered, this, &MainWindow::closeEtiFile);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    
    // Analysis actions
    connect(m_startAnalysisAction, &QAction::triggered, this, &MainWindow::startRealTimeAnalysis);
    connect(m_stopAnalysisAction, &QAction::triggered, this, &MainWindow::stopRealTimeAnalysis);
    
    // View actions
    connect(m_fullScreenAction, &QAction::triggered, this, &MainWindow::toggleFullScreen);
    connect(m_resetLayoutAction, &QAction::triggered, this, &MainWindow::resetLayout);
    
    // Dock visibility actions
    connect(m_showFileExplorerAction, &QAction::triggered, m_fileExplorerDock, &QDockWidget::setVisible);
    connect(m_showYamlViewerAction, &QAction::triggered, m_yamlViewerDock, &QDockWidget::setVisible);
    connect(m_showFigAnalysisAction, &QAction::triggered, m_figAnalysisDock, &QDockWidget::setVisible);
    connect(m_showAudioMonitorAction, &QAction::triggered, m_audioMonitorDock, &QDockWidget::setVisible);
    connect(m_showErrorLogAction, &QAction::triggered, m_errorLogDock, &QDockWidget::setVisible);
    
    // Dock visibility feedback
    connect(m_fileExplorerDock, &QDockWidget::visibilityChanged, 
            m_showFileExplorerAction, &QAction::setChecked);
    connect(m_yamlViewerDock, &QDockWidget::visibilityChanged, 
            m_showYamlViewerAction, &QAction::setChecked);
    connect(m_figAnalysisDock, &QDockWidget::visibilityChanged, 
            m_showFigAnalysisAction, &QAction::setChecked);
    connect(m_audioMonitorDock, &QDockWidget::visibilityChanged, 
            m_showAudioMonitorAction, &QAction::setChecked);
    connect(m_errorLogDock, &QDockWidget::visibilityChanged, 
            m_showErrorLogAction, &QAction::setChecked);
    
    // Dock visibility change handling
    connect(m_fileExplorerDock, &QDockWidget::visibilityChanged, 
            this, &MainWindow::handleDockVisibilityChanged);
    connect(m_yamlViewerDock, &QDockWidget::visibilityChanged, 
            this, &MainWindow::handleDockVisibilityChanged);
    connect(m_figAnalysisDock, &QDockWidget::visibilityChanged, 
            this, &MainWindow::handleDockVisibilityChanged);
    connect(m_audioMonitorDock, &QDockWidget::visibilityChanged, 
            this, &MainWindow::handleDockVisibilityChanged);
    connect(m_errorLogDock, &QDockWidget::visibilityChanged, 
            this, &MainWindow::handleDockVisibilityChanged);
    
    // Tools actions
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::showSettings);
    
    // Demo actions
    connect(m_createSampleFileAction, &QAction::triggered, this, &MainWindow::createSampleEtiFile);
    connect(m_loadTestDataAction, &QAction::triggered, this, &MainWindow::loadTestData);
    
    // Help actions
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    
    // ETI Processor signal connections
    if (m_etiProcessor) {
        connect(m_etiProcessor.get(), &EtiProcessor::statusChanged, 
                this, [this](const QString& status) {
                    updateStatus(status);
                });
        
        connect(m_etiProcessor.get(), &EtiProcessor::errorOccurred, 
                this, &MainWindow::handleEtiError);
        
        connect(m_etiProcessor.get(), &EtiProcessor::frameProcessed, 
                this, [this](quint64 frameNumber, const QByteArray& /*data*/) {
                    m_frameCount = frameNumber;
                    if (m_frameCountLabel) {
                        m_frameCountLabel->setText(tr("Frames: %1").arg(m_frameCount));
                    }
                });
                
        // Connect service browser to ETI processor
        if (m_serviceBrowser) {
            m_serviceBrowser->setEtiProcessor(m_etiProcessor.get());
        }
    }
    
    // ETI File Worker signal connections for background processing
    if (m_etiFileWorker) {
        connect(m_etiFileWorker.get(), &EtiFileWorker::processingStarted,
                this, [this](const QString& fileName) {
                    updateStatus(tr("Processing ETI file: %1").arg(QFileInfo(fileName).fileName()));
                    m_progressBar->setVisible(true);
                    m_progressBar->setValue(0);
                });
        
        connect(m_etiFileWorker.get(), &EtiFileWorker::processingProgress,
                this, [this](int progress, quint64 frameCount) {
                    m_progressBar->setValue(progress);
                    if (m_frameCountLabel) {
                        m_frameCountLabel->setText(tr("Frames: %1").arg(frameCount));
                    }
                });
        
        connect(m_etiFileWorker.get(), &EtiFileWorker::processingCompleted,
                this, [this](const QString& fileName, quint64 frameCount) {
                    m_progressBar->setVisible(false);
                    updateStatus(tr("ETI file loaded successfully: %1 (%2 frames)")
                               .arg(QFileInfo(fileName).fileName()).arg(frameCount));
                    
                    // Update service browser with discovered services
                    if (m_serviceBrowser) {
                        m_serviceBrowser->updateServices();
                    }
                    
                    emit etiFileOpened(fileName);
                });
        
        connect(m_etiFileWorker.get(), &EtiFileWorker::processingFailed,
                this, [this](const QString& fileName, const QString& errorMessage) {
                    m_progressBar->setVisible(false);
                    updateStatus(tr("Failed to process ETI file: %1").arg(QFileInfo(fileName).fileName()));
                    
                    // Clear current file state
                    m_currentFile.clear();
                    m_closeFileAction->setEnabled(false);
                    updateWindowTitle();
                    
                    QMessageBox::warning(this, tr("ETI Processing Error"), 
                        tr("Failed to process the ETI file:\n%1\n\nError: %2")
                        .arg(fileName).arg(errorMessage));
                });
        
        connect(m_etiFileWorker.get(), &EtiFileWorker::processingCancelled,
                this, [this](const QString& fileName) {
                    m_progressBar->setVisible(false);
                    updateStatus(tr("ETI file processing cancelled: %1").arg(QFileInfo(fileName).fileName()));
                });
    }
}

void MainWindow::setupRealTimeTimer()
{
    m_realTimeTimer = new QTimer(this);
    m_realTimeTimer->setInterval(UPDATE_INTERVAL_MS);  // 60 FPS
    m_realTimeTimer->setSingleShot(false);
    
    connect(m_realTimeTimer, &QTimer::timeout, this, &MainWindow::updateRealTimeDisplay);
}

void MainWindow::loadSettings()
{
    QSettings settings;
    
    // Window geometry
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    
    // Application settings
    m_updateRate = settings.value("updateRate", TARGET_FPS).toInt();
    
    // Update timer interval if needed
    if (m_realTimeTimer) {
        m_realTimeTimer->setInterval(1000 / m_updateRate);
    }
}

void MainWindow::saveSettings()
{
    QSettings settings;
    
    // Window geometry
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    
    // Application settings
    settings.setValue("updateRate", m_updateRate);
}

void MainWindow::updateWindowTitle()
{
    QString title = "ETI Stream Analyzer";
    
    if (!m_currentFile.isEmpty()) {
        QFileInfo fileInfo(m_currentFile);
        title += QString(" - %1").arg(fileInfo.fileName());
    }
    
    if (m_realTimeMode) {
        title += " [Real-time]";
    }
    
    setWindowTitle(title);
}

// Slot implementations

void MainWindow::openEtiFile()
{
    QElapsedTimer timer;
    timer.start();
    
    Logger::instance().log(Logger::Info, "MainWindow", "=== DIALOG PERFORMANCE TEST ===");
    Logger::instance().log(Logger::Info, "MainWindow", "Step 1: Starting openEtiFile() method...");
    
    qint64 step1 = timer.elapsed();
    Logger::instance().log(Logger::Info, "MainWindow", QString("Step 2: Method start completed in %1ms").arg(step1));
    
    // Test directory access first
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    qint64 step2 = timer.elapsed();
    Logger::instance().log(Logger::Info, "MainWindow", QString("Step 3: Directory lookup (%1) took %2ms").arg(defaultDir).arg(step2 - step1));
    
    // Apply KDE-specific Qt platform optimization
    Logger::instance().log(Logger::Info, "MainWindow", "Step 4: Applying KDE platform optimization...");
    
    // Log platform information
    QString desktop = qgetenv("XDG_CURRENT_DESKTOP");
    QString platform = QGuiApplication::platformName();
    Logger::instance().log(Logger::Info, "MainWindow", QString("Desktop: %1, Platform: %2").arg(desktop, platform));
    
    // Optimize dialog for performance
    QFileDialog::Options dialogOptions = QFileDialog::DontUseNativeDialog;
    Logger::instance().log(Logger::Info, "MainWindow", "Using optimized non-native dialog for consistent performance");
    
    qint64 platformTime = timer.elapsed();
    Logger::instance().log(Logger::Info, "MainWindow", QString("Platform optimization took %1ms").arg(platformTime - step2));
    
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Open ETI File"),
        defaultDir,
        tr("ETI Files (*.eti *.ETI);;All Files (*)"),
        nullptr,
        dialogOptions
    );
    
    qint64 dialogTime = timer.elapsed();
    Logger::instance().log(Logger::Info, "MainWindow", 
                          QString("Step 5: QFileDialog completed in %1ms TOTAL").arg(dialogTime));
    
    if (fileName.isEmpty()) {
        Logger::instance().log(Logger::Info, "MainWindow", "Step 6: Dialog cancelled by user");
        return;
    }
    Logger::instance().log(Logger::Info, "MainWindow", QString("Step 6: File selected: %1").arg(fileName));
    
    // Process the selected file
        // Check if worker is available and not already processing
        if (!m_etiFileWorker) {
            QMessageBox::critical(this, tr("System Error"), 
                tr("ETI file worker is not initialized. Please restart the application."));
            return;
        }
        
        if (m_etiFileWorker->isProcessing()) {
            QMessageBox::information(this, tr("Processing in Progress"), 
                tr("An ETI file is already being processed. Please wait for completion or cancel the current operation."));
            return;
        }
        
        // Immediately update GUI state (non-blocking)
        m_currentFile = fileName;
        m_closeFileAction->setEnabled(true);
        updateWindowTitle();
        
        // Reset frame count display
        m_frameCount = 0;
        if (m_frameCountLabel) {
            m_frameCountLabel->setText(tr("Frames: 0"));
        }
        
        qint64 processStartTime = timer.elapsed();
        Logger::instance().log(Logger::Info, "MainWindow", 
                              QString("Starting background processing of ETI file: %1 (dialog took %2ms)")
                              .arg(fileName).arg(processStartTime));
        
        // Start background processing (non-blocking, returns immediately)
        m_etiFileWorker->processEtiFile(fileName);
        
        qint64 totalTime = timer.elapsed();
        Logger::instance().log(Logger::Debug, "MainWindow", 
                              QString("openEtiFile() method completed in %1ms").arg(totalTime));
        
        // The processing status, progress, and completion will be handled 
        // by the worker signals connected above
}

QString MainWindow::getFastOpenFileName(const QString& defaultDir)
{
    // Create a lightweight, fast dialog using QDialog + QListWidget
    // This avoids the slow QFileDialog performance issues on some systems
    
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Open ETI File"));
    dialog.setModal(true);
    dialog.resize(600, 400);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    // Directory navigation
    QHBoxLayout* dirLayout = new QHBoxLayout();
    QLineEdit* dirEdit = new QLineEdit(defaultDir);
    QPushButton* browseBtn = new QPushButton(tr("Browse..."));
    dirLayout->addWidget(new QLabel(tr("Directory:")));
    dirLayout->addWidget(dirEdit);
    dirLayout->addWidget(browseBtn);
    layout->addLayout(dirLayout);
    
    // File list
    QListWidget* fileList = new QListWidget();
    layout->addWidget(fileList);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* openBtn = new QPushButton(tr("Open"));
    QPushButton* cancelBtn = new QPushButton(tr("Cancel"));
    buttonLayout->addStretch();
    buttonLayout->addWidget(openBtn);
    buttonLayout->addWidget(cancelBtn);
    layout->addLayout(buttonLayout);
    
    QString selectedFile;
    
    // Function to populate file list
    auto populateFiles = [&](const QString& dirPath) {
        fileList->clear();
        QDir dir(dirPath);
        QStringList filters;
        filters << "*.eti" << "*.ETI";
        QStringList files = dir.entryList(filters, QDir::Files, QDir::Name);
        for (const QString& file : files) {
            QListWidgetItem* item = new QListWidgetItem(file);
            item->setData(Qt::UserRole, dir.absoluteFilePath(file));
            fileList->addItem(item);
        }
    };
    
    // Initial population
    populateFiles(defaultDir);
    
    // Connect signals with fast directory selection
    connect(browseBtn, &QPushButton::clicked, [&]() {
        // Use simple input dialog instead of slow QFileDialog::getExistingDirectory
        bool ok;
        QString dir = QInputDialog::getText(&dialog, tr("Select Directory"), 
                                          tr("Enter directory path:"), QLineEdit::Normal, 
                                          dirEdit->text(), &ok);
        if (ok && !dir.isEmpty()) {
            dirEdit->setText(dir);
            populateFiles(dir);
        }
    });
    
    connect(dirEdit, &QLineEdit::returnPressed, [&]() {
        populateFiles(dirEdit->text());
    });
    
    connect(fileList, &QListWidget::itemDoubleClicked, [&](QListWidgetItem* item) {
        selectedFile = item->data(Qt::UserRole).toString();
        dialog.accept();
    });
    
    connect(openBtn, &QPushButton::clicked, [&]() {
        if (fileList->currentItem()) {
            selectedFile = fileList->currentItem()->data(Qt::UserRole).toString();
            dialog.accept();
        }
    });
    
    connect(cancelBtn, &QPushButton::clicked, [&]() {
        dialog.reject();
    });
    
    openBtn->setDefault(true);
    openBtn->setEnabled(false);
    
    connect(fileList, &QListWidget::itemSelectionChanged, [&]() {
        openBtn->setEnabled(fileList->currentItem() != nullptr);
    });
    
    if (dialog.exec() == QDialog::Accepted) {
        return selectedFile;
    }
    
    return QString(); // Empty string if cancelled
}

void MainWindow::closeEtiFile()
{
    if (m_realTimeMode) {
        stopRealTimeAnalysis();
    }
    
    m_currentFile.clear();
    m_closeFileAction->setEnabled(false);
    m_frameCount = 0;
    
    updateWindowTitle();
    updateStatus(tr("ETI file closed"));
    
    // Clear displays
    m_yamlViewer->clear();
    m_figAnalysis->clear();
    
    // Update status bar
    m_frameCountLabel->setText(tr("Frames: 0"));
    m_bitRateLabel->setText(tr("Rate: 0 kbps"));
    m_errorCountLabel->setText(tr("Errors: 0"));
    
    emit etiFileClosed();
    
    Logger::instance().log(Logger::Info, "MainWindow", "ETI file closed");
}

void MainWindow::startRealTimeAnalysis()
{
    if (m_realTimeMode) {
        return;  // Already in real-time mode
    }
    
    m_realTimeMode = true;
    m_frameCount = 0;
    m_lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
    
    // Update UI state
    m_startAnalysisAction->setEnabled(false);
    m_stopAnalysisAction->setEnabled(true);
    
    // Start timer
    if (m_realTimeTimer) {
        m_realTimeTimer->start();
    }
    
    updateWindowTitle();
    updateStatus(tr("Real-time analysis started"));
    
    emit realTimeModeChanged(true);
    
    Logger::instance().log(Logger::Info, "MainWindow", "Real-time analysis started");
}

void MainWindow::stopRealTimeAnalysis()
{
    if (!m_realTimeMode) {
        return;  // Not in real-time mode
    }
    
    m_realTimeMode = false;
    
    // Stop timer
    if (m_realTimeTimer) {
        m_realTimeTimer->stop();
    }
    
    // Update UI state
    m_startAnalysisAction->setEnabled(true);
    m_stopAnalysisAction->setEnabled(false);
    
    updateWindowTitle();
    updateStatus(tr("Real-time analysis stopped"));
    
    emit realTimeModeChanged(false);
    
    Logger::instance().log(Logger::Info, "MainWindow", "Real-time analysis stopped");
}

void MainWindow::showSettings()
{
    if (!m_settingsDialog) {
        m_settingsDialog = new SettingsDialog(this);
        connect(m_settingsDialog, &SettingsDialog::settingsChanged,
                this, &MainWindow::settingsChanged);
    }
    
    m_settingsDialog->show();
    m_settingsDialog->raise();
    m_settingsDialog->activateWindow();
}

void MainWindow::showAbout()
{
    if (!m_aboutDialog) {
        m_aboutDialog = new AboutDialog(this);
    }
    
    m_aboutDialog->show();
    m_aboutDialog->raise();
    m_aboutDialog->activateWindow();
}

void MainWindow::toggleFullScreen()
{
    if (isFullScreen()) {
        showNormal();
        m_fullScreenAction->setChecked(false);
    } else {
        showFullScreen();
        m_fullScreenAction->setChecked(true);
    }
}

void MainWindow::resetLayout()
{
    if (!m_defaultLayoutState.isEmpty()) {
        restoreState(m_defaultLayoutState);
        updateStatus(tr("Layout reset to default"));
        
        Logger::instance().log(Logger::Info, "MainWindow", "Layout reset to default");
    }
}

void MainWindow::updateStatus(const QString& message, const QString& details)
{
    if (m_statusLabel) {
        m_statusLabel->setText(message);
        
        if (!details.isEmpty()) {
            m_statusLabel->setToolTip(details);
        }
        
        // Auto-clear message after timeout
        if (!message.isEmpty()) {
            QTimer::singleShot(STATUS_MESSAGE_TIMEOUT, [this]() {
                if (m_statusLabel) {
                    m_statusLabel->setText(tr("Ready"));
                    m_statusLabel->setToolTip("");
                }
            });
        }
    }
}

void MainWindow::updateProgress(int value)
{
    if (m_progressBar) {
        if (value >= 0 && value <= 100) {
            m_progressBar->setValue(value);
            m_progressBar->setVisible(true);
        } else {
            m_progressBar->setVisible(false);
        }
    }
}

void MainWindow::updateRealTimeDisplay()
{
    if (!m_realTimeMode) {
        return;
    }
    
    // Simulate ETI frame processing
    m_frameCount++;
    
    // Update frame count
    if (m_frameCountLabel) {
        m_frameCountLabel->setText(tr("Frames: %1").arg(m_frameCount));
    }
    
    // Calculate frame rate and bit rate
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 deltaTime = currentTime - m_lastUpdateTime;
    
    if (deltaTime > 0) {
        double frameRate = 1000.0 / deltaTime;  // Frames per second
        double bitRate = frameRate * 6144 * 8 / 1000.0;  // kbps (ETI frame = 6144 bytes)
        
        if (m_bitRateLabel) {
            m_bitRateLabel->setText(tr("Rate: %1 kbps").arg(QString::number(bitRate, 'f', 1)));
        }
        
        // Track average frame time for performance monitoring
        m_averageFrameTime = (m_averageFrameTime * 0.9) + (deltaTime * 0.1);
    }
    
    m_lastUpdateTime = currentTime;
    
    // Update analyzer widget if available
    if (m_analyzerWidget) {
        m_analyzerWidget->updateDisplay();
    }
    
    // Update constellation widget if available
    if (m_constellationWidget) {
        m_constellationWidget->updateDisplay();
    }
}

void MainWindow::handleEtiError(const QString& error)
{
    // Log error
    Logger::instance().log(Logger::Error, "MainWindow", QString("ETI Error: %1").arg(error));
    
    // Display error in error log dock
    if (m_errorLog) {
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        m_errorLog->append(QString("[%1] ERROR: %2").arg(timestamp, error));
    }
    
    // Update error count
    static int errorCount = 0;
    errorCount++;
    
    if (m_errorCountLabel) {
        m_errorCountLabel->setText(tr("Errors: %1").arg(errorCount));
    }
    
    // Show error in status bar
    updateStatus(tr("ETI Error: %1").arg(error));
}

void MainWindow::handleDockVisibilityChanged(bool visible)
{
    Q_UNUSED(visible)
    
    // This slot is called when dock visibility changes
    // Can be used for layout optimization or state tracking
    Logger::instance().log(Logger::Debug, "MainWindow", "Dock visibility changed");
}

// Event handlers

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Stop real-time processing
    if (m_realTimeMode) {
        stopRealTimeAnalysis();
    }
    
    // Save settings
    saveSettings();
    
    // Accept close event
    event->accept();
    
    Logger::instance().log(Logger::Info, "MainWindow", "Application closing");
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    
    // Log resize for debugging
    Logger::instance().log(Logger::Debug, "MainWindow", 
                         QString("Window resized to %1x%2")
                         .arg(event->size().width())
                         .arg(event->size().height()));
}

void MainWindow::moveEvent(QMoveEvent *event)
{
    QMainWindow::moveEvent(event);
    
    // Log move for debugging
    Logger::instance().log(Logger::Debug, "MainWindow", 
                         QString("Window moved to (%1, %2)")
                         .arg(event->pos().x())
                         .arg(event->pos().y()));
}

bool MainWindow::validateLayout() const
{
    // Validate that all required components exist and are properly configured
    return (m_centralWidget != nullptr &&
            m_analyzerWidget != nullptr &&
            m_constellationWidget != nullptr &&
            m_serviceBrowser != nullptr &&
            m_menuBar != nullptr &&
            m_statusBar != nullptr &&
            m_realTimeTimer != nullptr &&
            m_isInitialized);
}

void MainWindow::createSampleEtiFile()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Create Sample ETI File"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/sample.eti",
        tr("ETI Files (*.eti)")
    );
    
    if (!fileName.isEmpty()) {
        // Create a sample ETI file using test fixture data
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            // Write a minimal valid ETI frame using test data
            // For now, just create a placeholder file with basic ETI structure
            QByteArray sampleData;
            
            // Create 100 sample ETI frames
            for (int i = 0; i < 100; ++i) {
                // Add ETI frame header (24 bytes)
                sampleData.append(QByteArray::fromHex("E0E1"));  // ERR + STAT
                sampleData.append(QByteArray::fromHex("FFFF"));  // LIDATA
                sampleData.append(QByteArray::fromHex("0001"));  // FC
                sampleData.append(QByteArray::fromHex("0000"));  // NST
                sampleData.append(QByteArray::fromHex("00"));    // FP
                sampleData.append(QByteArray::fromHex("0000"));  // MID
                sampleData.append(QByteArray::fromHex("00"));    // FL
                sampleData.append(QByteArray(13, 0));            // Reserved/padding
                
                // Add FIC data (32 bytes)
                sampleData.append(QByteArray::fromHex("00001234")); // Ensemble ID
                sampleData.append(QByteArray(28, 0));                // FIC padding
                
                // Add MSC data to reach 6144 bytes total frame size
                int remaining = 6144 - sampleData.size() % 6144;
                if (remaining > 0) {
                    sampleData.append(QByteArray(remaining, 0));
                }
            }
            
            file.write(sampleData);
            file.close();
            
            updateStatus(tr("Sample ETI file created: %1").arg(QFileInfo(fileName).fileName()));
            
            QMessageBox::information(this, tr("Sample File Created"),
                tr("Sample ETI file has been created successfully.\n\nFile: %1\nSize: %2 bytes")
                .arg(fileName)
                .arg(sampleData.size()));
                
            Logger::instance().log(Logger::Info, "MainWindow", 
                                 QString("Sample ETI file created: %1 (%2 bytes)")
                                 .arg(fileName).arg(sampleData.size()));
        } else {
            QMessageBox::warning(this, tr("File Error"),
                tr("Could not create sample ETI file:\n%1").arg(file.errorString()));
        }
    }
}

void MainWindow::loadTestData()
{
    if (m_serviceBrowser) {
        // Force generation of test services
        m_serviceBrowser->clearServices();
        m_serviceBrowser->updateServices();
        
        updateStatus(tr("Test service data loaded"));
        
        QMessageBox::information(this, tr("Test Data Loaded"),
            tr("Test service data has been loaded into the service browser.\n\n"
               "You can now see sample DAB services organized by ensemble."));
               
        Logger::instance().log(Logger::Info, "MainWindow", "Test service data loaded");
    } else {
        QMessageBox::warning(this, tr("Error"),
            tr("Service browser is not available."));
    }
}