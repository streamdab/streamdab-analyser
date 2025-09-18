/**
 * @file main_window_alert_integration.cpp
 * @brief MainWindow Alert System Integration Implementation
 * 
 * Implements comprehensive Alert System integration with professional broadcast
 * workflows, real-time notifications, and visual status indicators.
 * 
 * @author UI/UX Agent - Phase 4 Integration
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyzer Team
 */

#include "main_window_alert_integration.h"
#include "utils/logger_compat.h"

#include <QApplication>
#include <QMessageBox>
#include <QListWidgetItem>
#include <QDateTime>
#include <QPropertyAnimation>
#include <QGraphicsEffect>
#include <QGraphicsOpacityEffect>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QSplitter>
#include <QTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

MainWindowAlertIntegration::MainWindowAlertIntegration(MainWindow *mainWindow, QObject *parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
    , m_alertSystem(nullptr)
    , m_recordingManager(nullptr)
    , m_audioPipeline(nullptr)
    , m_configDialog(nullptr)
    , m_alertDock(nullptr)
    , m_alertDockContent(nullptr)
    , m_alertDockLayout(nullptr)
    , m_alertStatusGroup(nullptr)
    , m_activeAlertsList(nullptr)
    , m_alertCountLabel(nullptr)
    , m_alertLevelIndicator(nullptr)
    , m_clearAlertsButton(nullptr)
    , m_acknowledgeButton(nullptr)
    , m_configureButton(nullptr)
    , m_infoStatusWidget(nullptr)
    , m_warningStatusWidget(nullptr)
    , m_criticalStatusWidget(nullptr)
    , m_emergencyStatusWidget(nullptr)
    , m_alertMonitoringAction(nullptr)
    , m_alertStatusLabel(nullptr)
    , m_alertStatusIndicator(nullptr)
    , m_alertConfigButton(nullptr)
    , m_recordingToolbar(nullptr)
    , m_startRecordingAction(nullptr)
    , m_stopRecordingAction(nullptr)
    , m_pauseRecordingAction(nullptr)
    , m_recordingSettingsAction(nullptr)
    , m_recordingStatusLabel(nullptr)
    , m_audioToolbar(nullptr)
    , m_startAudioExtractionAction(nullptr)
    , m_stopAudioExtractionAction(nullptr)
    , m_audioExtractionSettingsAction(nullptr)
    , m_audioExtractionStatusLabel(nullptr)
    , m_systemTray(nullptr)
    , m_systemTrayMenu(nullptr)
    , m_showMainWindowAction(nullptr)
    , m_showAlertsAction(nullptr)
    , m_exitAction(nullptr)
    , m_statusUpdateTimer(nullptr)
    , m_indicatorUpdateTimer(nullptr)
    , m_initialized(false)
    , m_alertMonitoringEnabled(false)
    , m_totalAlertCount(0)
    , m_activeAlertCount(0)
{
    if (!mainWindow) {
        qCritical() << "MainWindowAlertIntegration: MainWindow cannot be null";
        return;
    }

    // Initialize alert count tracking
    m_alertCountByLevel[AlertSystem::AlertLevel::Info] = 0;
    m_alertCountByLevel[AlertSystem::AlertLevel::Warning] = 0;
    m_alertCountByLevel[AlertSystem::AlertLevel::Critical] = 0;
    m_alertCountByLevel[AlertSystem::AlertLevel::Emergency] = 0;
}

MainWindowAlertIntegration::~MainWindowAlertIntegration()
{
    if (m_statusUpdateTimer) {
        m_statusUpdateTimer->stop();
    }
    if (m_indicatorUpdateTimer) {
        m_indicatorUpdateTimer->stop();
    }
}

bool MainWindowAlertIntegration::initialize()
{
    if (m_initialized) {
        return true;
    }

    try {
        qInfo() << "Initializing MainWindow Alert System integration...";

        // Create core components
        m_alertSystem = std::make_unique<AlertSystem>(this);
        m_recordingManager = std::make_unique<eti::StreamRecordingManager>(this);
        m_audioPipeline = std::make_unique<eti::AudioExtractionPipeline>(this);

        // Initialize components
        if (!m_alertSystem->initialize()) {
            qCritical() << "Failed to initialize Alert System";
            return false;
        }

        // Create GUI components
        createAlertDockPanel();
        createToolbarIndicators();
        createRecordingToolbar();
        createAudioExtractionControls();
        createSystemTrayIntegration();

        // Setup connections
        setupAlertSystemConnections();

        // Create configuration dialog
        m_configDialog = std::make_unique<AlertConfigurationDialog>(m_alertSystem.get(), m_mainWindow);
        if (!m_configDialog->initialize()) {
            qCritical() << "Failed to initialize Alert Configuration Dialog";
            return false;
        }

        // Setup update timers
        m_statusUpdateTimer = new QTimer(this);
        m_statusUpdateTimer->setInterval(STATUS_UPDATE_INTERVAL_MS);
        connect(m_statusUpdateTimer, &QTimer::timeout, this, &MainWindowAlertIntegration::updateAlertStatusDisplay);

        m_indicatorUpdateTimer = new QTimer(this);
        m_indicatorUpdateTimer->setInterval(INDICATOR_UPDATE_INTERVAL_MS);
        connect(m_indicatorUpdateTimer, &QTimer::timeout, this, &MainWindowAlertIntegration::updateToolbarIndicators);

        // Apply professional styling
        applyBroadcastStyling();

        // Start timers
        m_statusUpdateTimer->start();
        m_indicatorUpdateTimer->start();

        m_initialized = true;
        qInfo() << "MainWindow Alert System integration initialized successfully";
        return true;

    } catch (const std::exception& e) {
        qCritical() << "MainWindowAlertIntegration initialization failed:" << e.what();
        return false;
    }
}

void MainWindowAlertIntegration::createAlertDockPanel()
{
    // Create alert dock widget
    m_alertDock = new QDockWidget(tr("Alert Monitor"), m_mainWindow);
    m_alertDock->setObjectName("AlertMonitorDock");
    m_alertDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_alertDock->setFeatures(QDockWidget::DockWidgetMovable | 
                            QDockWidget::DockWidgetFloatable | 
                            QDockWidget::DockWidgetClosable);

    // Create dock content widget
    m_alertDockContent = new QWidget();
    m_alertDockLayout = new QVBoxLayout(m_alertDockContent);
    m_alertDockLayout->setSpacing(8);
    m_alertDockLayout->setContentsMargins(8, 8, 8, 8);

    // Alert status group
    m_alertStatusGroup = new QGroupBox(tr("Alert Status"));
    QVBoxLayout *statusLayout = new QVBoxLayout(m_alertStatusGroup);

    // Alert count label
    m_alertCountLabel = new QLabel(tr("No active alerts"));
    m_alertCountLabel->setStyleSheet("QLabel { font-weight: bold; color: green; }");
    statusLayout->addWidget(m_alertCountLabel);

    // Alert level indicator
    m_alertLevelIndicator = new QProgressBar();
    m_alertLevelIndicator->setRange(0, 3);
    m_alertLevelIndicator->setValue(0);
    m_alertLevelIndicator->setFormat(tr("Alert Level: %v"));
    m_alertLevelIndicator->setStyleSheet(
        "QProgressBar::chunk { background-color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, "
        "stop: 0 #00ff00, stop: 0.33 #ffff00, stop: 0.66 #ff8800, stop: 1.0 #ff0000); }"
    );
    statusLayout->addWidget(m_alertLevelIndicator);

    // Create status widgets for each alert level
    QHBoxLayout *statusWidgetsLayout = new QHBoxLayout();
    m_infoStatusWidget = createAlertStatusWidget(tr("Info"), m_alertStatusGroup);
    m_warningStatusWidget = createAlertStatusWidget(tr("Warning"), m_alertStatusGroup);
    m_criticalStatusWidget = createAlertStatusWidget(tr("Critical"), m_alertStatusGroup);
    m_emergencyStatusWidget = createAlertStatusWidget(tr("Emergency"), m_alertStatusGroup);

    statusWidgetsLayout->addWidget(m_infoStatusWidget);
    statusWidgetsLayout->addWidget(m_warningStatusWidget);
    statusWidgetsLayout->addWidget(m_criticalStatusWidget);
    statusWidgetsLayout->addWidget(m_emergencyStatusWidget);
    statusLayout->addLayout(statusWidgetsLayout);

    m_alertDockLayout->addWidget(m_alertStatusGroup);

    // Active alerts list
    QGroupBox *activeAlertsGroup = new QGroupBox(tr("Active Alerts"));
    QVBoxLayout *alertsLayout = new QVBoxLayout(activeAlertsGroup);

    m_activeAlertsList = new QListWidget();
    m_activeAlertsList->setAlternatingRowColors(true);
    m_activeAlertsList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_activeAlertsList->setMaximumHeight(200);
    alertsLayout->addWidget(m_activeAlertsList);

    // Alert control buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_acknowledgeButton = new QPushButton(tr("Acknowledge"));
    m_acknowledgeButton->setIcon(QIcon(":/icons/acknowledge.png"));
    m_acknowledgeButton->setEnabled(false);
    buttonLayout->addWidget(m_acknowledgeButton);

    m_clearAlertsButton = new QPushButton(tr("Clear All"));
    m_clearAlertsButton->setIcon(QIcon(":/icons/clear_all.png"));
    buttonLayout->addWidget(m_clearAlertsButton);

    m_configureButton = new QPushButton(tr("Configure"));
    m_configureButton->setIcon(QIcon(":/icons/configure.png"));
    buttonLayout->addWidget(m_configureButton);

    alertsLayout->addLayout(buttonLayout);
    m_alertDockLayout->addWidget(activeAlertsGroup);

    // Add stretch to push everything to top
    m_alertDockLayout->addStretch();

    // Set dock content
    m_alertDock->setWidget(m_alertDockContent);

    // Add dock to main window
    m_mainWindow->addDockWidget(Qt::RightDockWidgetArea, m_alertDock);

    // Connect signals
    connect(m_activeAlertsList, &QListWidget::itemSelectionChanged,
            this, &MainWindowAlertIntegration::handleAlertListSelectionChanged);
    connect(m_acknowledgeButton, &QPushButton::clicked,
            this, &MainWindowAlertIntegration::acknowledgeSelectedAlert);
    connect(m_clearAlertsButton, &QPushButton::clicked,
            this, &MainWindowAlertIntegration::clearAllAlerts);
    connect(m_configureButton, &QPushButton::clicked,
            this, &MainWindowAlertIntegration::showAlertConfiguration);
}

void MainWindowAlertIntegration::createToolbarIndicators()
{
    // Get main toolbar from MainWindow
    QToolBar *mainToolbar = m_mainWindow->findChild<QToolBar*>("mainToolBar");
    if (!mainToolbar) {
        qWarning() << "Main toolbar not found, creating alert indicators in new toolbar";
        mainToolbar = m_mainWindow->addToolBar(tr("Alert Indicators"));
    }

    // Add separator
    mainToolbar->addSeparator();

    // Alert monitoring toggle action
    m_alertMonitoringAction = new QAction(QIcon(":/icons/alert_monitor.png"), tr("Alert Monitoring"), this);
    m_alertMonitoringAction->setCheckable(true);
    m_alertMonitoringAction->setChecked(false);
    m_alertMonitoringAction->setToolTip(tr("Toggle alert monitoring"));
    mainToolbar->addAction(m_alertMonitoringAction);

    // Alert status label
    m_alertStatusLabel = new QLabel(tr("Alerts: OFF"));
    m_alertStatusLabel->setStyleSheet("QLabel { margin: 4px; padding: 2px; }");
    mainToolbar->addWidget(m_alertStatusLabel);

    // Alert status indicator
    m_alertStatusIndicator = new QProgressBar();
    m_alertStatusIndicator->setRange(0, 3);
    m_alertStatusIndicator->setValue(0);
    m_alertStatusIndicator->setMaximumWidth(100);
    m_alertStatusIndicator->setMaximumHeight(20);
    m_alertStatusIndicator->setTextVisible(false);
    mainToolbar->addWidget(m_alertStatusIndicator);

    // Alert configuration button
    m_alertConfigButton = new QPushButton();
    m_alertConfigButton->setIcon(QIcon(":/icons/alert_config.png"));
    m_alertConfigButton->setToolTip(tr("Configure alerts"));
    m_alertConfigButton->setMaximumSize(24, 24);
    m_alertConfigButton->setFlat(true);
    mainToolbar->addWidget(m_alertConfigButton);

    // Connect signals
    connect(m_alertMonitoringAction, &QAction::triggered,
            this, &MainWindowAlertIntegration::toggleAlertMonitoring);
    connect(m_alertConfigButton, &QPushButton::clicked,
            this, &MainWindowAlertIntegration::showAlertConfiguration);
}

void MainWindowAlertIntegration::createRecordingToolbar()
{
    // Create recording toolbar
    m_recordingToolbar = m_mainWindow->addToolBar(tr("Recording Controls"));
    m_recordingToolbar->setObjectName("RecordingToolBar");
    m_recordingToolbar->setMovable(true);
    m_recordingToolbar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);

    // Start recording action
    m_startRecordingAction = new QAction(QIcon(":/icons/record_start.png"), tr("Start Recording"), this);
    m_startRecordingAction->setToolTip(tr("Start ETI stream recording"));
    m_startRecordingAction->setShortcut(QKeySequence(tr("Ctrl+R")));
    m_recordingToolbar->addAction(m_startRecordingAction);

    // Stop recording action
    m_stopRecordingAction = new QAction(QIcon(":/icons/record_stop.png"), tr("Stop Recording"), this);
    m_stopRecordingAction->setToolTip(tr("Stop ETI stream recording"));
    m_stopRecordingAction->setShortcut(QKeySequence(tr("Ctrl+Shift+R")));
    m_stopRecordingAction->setEnabled(false);
    m_recordingToolbar->addAction(m_stopRecordingAction);

    // Pause recording action
    m_pauseRecordingAction = new QAction(QIcon(":/icons/record_pause.png"), tr("Pause Recording"), this);
    m_pauseRecordingAction->setToolTip(tr("Pause ETI stream recording"));
    m_pauseRecordingAction->setEnabled(false);
    m_recordingToolbar->addAction(m_pauseRecordingAction);

    // Separator
    m_recordingToolbar->addSeparator();

    // Recording settings action
    m_recordingSettingsAction = new QAction(QIcon(":/icons/record_settings.png"), tr("Recording Settings"), this);
    m_recordingSettingsAction->setToolTip(tr("Configure recording settings"));
    m_recordingToolbar->addAction(m_recordingSettingsAction);

    // Recording status label
    m_recordingStatusLabel = new QLabel(tr("Recording: Stopped"));
    m_recordingStatusLabel->setStyleSheet("QLabel { margin: 4px; padding: 2px; color: red; }");
    m_recordingToolbar->addWidget(m_recordingStatusLabel);

    // Connect recording signals
    connect(m_startRecordingAction, &QAction::triggered, this, [this]() {
        // TODO: Implement start recording
        m_startRecordingAction->setEnabled(false);
        m_stopRecordingAction->setEnabled(true);
        m_pauseRecordingAction->setEnabled(true);
        m_recordingStatusLabel->setText(tr("Recording: Active"));
        m_recordingStatusLabel->setStyleSheet("QLabel { margin: 4px; padding: 2px; color: green; }");
        qInfo() << "Recording started";
    });

    connect(m_stopRecordingAction, &QAction::triggered, this, [this]() {
        // TODO: Implement stop recording
        m_startRecordingAction->setEnabled(true);
        m_stopRecordingAction->setEnabled(false);
        m_pauseRecordingAction->setEnabled(false);
        m_recordingStatusLabel->setText(tr("Recording: Stopped"));
        m_recordingStatusLabel->setStyleSheet("QLabel { margin: 4px; padding: 2px; color: red; }");
        qInfo() << "Recording stopped";
    });

    connect(m_pauseRecordingAction, &QAction::triggered, this, [this]() {
        // TODO: Implement pause recording
        if (m_pauseRecordingAction->text() == tr("Pause Recording")) {
            m_pauseRecordingAction->setText(tr("Resume Recording"));
            m_pauseRecordingAction->setIcon(QIcon(":/icons/record_resume.png"));
            m_recordingStatusLabel->setText(tr("Recording: Paused"));
            m_recordingStatusLabel->setStyleSheet("QLabel { margin: 4px; padding: 2px; color: orange; }");
            qInfo() << "Recording paused";
        } else {
            m_pauseRecordingAction->setText(tr("Pause Recording"));
            m_pauseRecordingAction->setIcon(QIcon(":/icons/record_pause.png"));
            m_recordingStatusLabel->setText(tr("Recording: Active"));
            m_recordingStatusLabel->setStyleSheet("QLabel { margin: 4px; padding: 2px; color: green; }");
            qInfo() << "Recording resumed";
        }
    });

    connect(m_recordingSettingsAction, &QAction::triggered,
            this, &MainWindowAlertIntegration::showRecordingControls);
}

void MainWindowAlertIntegration::createAudioExtractionControls()
{
    // Create audio extraction toolbar
    m_audioToolbar = m_mainWindow->addToolBar(tr("Audio Extraction"));
    m_audioToolbar->setObjectName("AudioExtractionToolBar");
    m_audioToolbar->setMovable(true);
    m_audioToolbar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);

    // Start audio extraction action
    m_startAudioExtractionAction = new QAction(QIcon(":/icons/audio_extract_start.png"), tr("Start Audio Extraction"), this);
    m_startAudioExtractionAction->setToolTip(tr("Start audio extraction from services"));
    m_startAudioExtractionAction->setShortcut(QKeySequence(tr("Ctrl+A")));
    m_audioToolbar->addAction(m_startAudioExtractionAction);

    // Stop audio extraction action
    m_stopAudioExtractionAction = new QAction(QIcon(":/icons/audio_extract_stop.png"), tr("Stop Audio Extraction"), this);
    m_stopAudioExtractionAction->setToolTip(tr("Stop audio extraction"));
    m_stopAudioExtractionAction->setShortcut(QKeySequence(tr("Ctrl+Shift+A")));
    m_stopAudioExtractionAction->setEnabled(false);
    m_audioToolbar->addAction(m_stopAudioExtractionAction);

    // Separator
    m_audioToolbar->addSeparator();

    // Audio extraction settings action
    m_audioExtractionSettingsAction = new QAction(QIcon(":/icons/audio_settings.png"), tr("Audio Settings"), this);
    m_audioExtractionSettingsAction->setToolTip(tr("Configure audio extraction settings"));
    m_audioToolbar->addAction(m_audioExtractionSettingsAction);

    // Audio extraction status label
    m_audioExtractionStatusLabel = new QLabel(tr("Audio: Stopped"));
    m_audioExtractionStatusLabel->setStyleSheet("QLabel { margin: 4px; padding: 2px; color: red; }");
    m_audioToolbar->addWidget(m_audioExtractionStatusLabel);

    // Connect audio extraction signals
    connect(m_startAudioExtractionAction, &QAction::triggered, this, [this]() {
        // TODO: Implement start audio extraction
        m_startAudioExtractionAction->setEnabled(false);
        m_stopAudioExtractionAction->setEnabled(true);
        m_audioExtractionStatusLabel->setText(tr("Audio: Extracting"));
        m_audioExtractionStatusLabel->setStyleSheet("QLabel { margin: 4px; padding: 2px; color: green; }");
        qInfo() << "Audio extraction started";
    });

    connect(m_stopAudioExtractionAction, &QAction::triggered, this, [this]() {
        // TODO: Implement stop audio extraction
        m_startAudioExtractionAction->setEnabled(true);
        m_stopAudioExtractionAction->setEnabled(false);
        m_audioExtractionStatusLabel->setText(tr("Audio: Stopped"));
        m_audioExtractionStatusLabel->setStyleSheet("QLabel { margin: 4px; padding: 2px; color: red; }");
        qInfo() << "Audio extraction stopped";
    });

    connect(m_audioExtractionSettingsAction, &QAction::triggered,
            this, &MainWindowAlertIntegration::showAudioExtractionDialog);
}

void MainWindowAlertIntegration::createSystemTrayIntegration()
{
    // Check if system tray is available
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning() << "System tray not available on this system";
        return;
    }

    // Create system tray icon
    m_systemTray = new QSystemTrayIcon(QIcon(":/icons/app_icon.png"), this);
    m_systemTray->setToolTip(tr("ETI Stream Analyzer"));

    // Create system tray menu
    m_systemTrayMenu = new QMenu();

    m_showMainWindowAction = new QAction(QIcon(":/icons/show_window.png"), tr("Show Main Window"), this);
    m_systemTrayMenu->addAction(m_showMainWindowAction);

    m_showAlertsAction = new QAction(QIcon(":/icons/show_alerts.png"), tr("Show Alerts"), this);
    m_systemTrayMenu->addAction(m_showAlertsAction);

    m_systemTrayMenu->addSeparator();

    m_exitAction = new QAction(QIcon(":/icons/exit.png"), tr("Exit"), this);
    m_systemTrayMenu->addAction(m_exitAction);

    m_systemTray->setContextMenu(m_systemTrayMenu);

    // Connect system tray signals
    connect(m_systemTray, &QSystemTrayIcon::activated,
            this, &MainWindowAlertIntegration::handleSystemTrayActivation);
    connect(m_showMainWindowAction, &QAction::triggered, this, [this]() {
        m_mainWindow->show();
        m_mainWindow->raise();
        m_mainWindow->activateWindow();
    });
    connect(m_showAlertsAction, &QAction::triggered,
            this, &MainWindowAlertIntegration::showAlertConfiguration);
    connect(m_exitAction, &QAction::triggered,
            QApplication::instance(), &QApplication::quit);

    // Show system tray icon
    m_systemTray->show();
}

void MainWindowAlertIntegration::setupAlertSystemConnections()
{
    if (!m_alertSystem) return;

    // Connect alert system signals
    connect(m_alertSystem.get(), &AlertSystem::alertTriggered,
            this, &MainWindowAlertIntegration::handleAlertTriggered);
    connect(m_alertSystem.get(), &AlertSystem::alertAcknowledged,
            this, &MainWindowAlertIntegration::handleAlertAcknowledged);

    // Connect configuration dialog signals
    if (m_configDialog) {
        connect(m_configDialog.get(), &AlertConfigurationDialog::testAlertTriggered,
                this, &MainWindowAlertIntegration::handleTestAlert);
        connect(m_configDialog.get(), &AlertConfigurationDialog::configurationApplied,
                this, [this]() {
                    qInfo() << "Alert configuration updated";
                    updateAlertStatusDisplay();
                });
    }
}

QWidget* MainWindowAlertIntegration::createAlertStatusWidget(const QString& title, QWidget* parent)
{
    QWidget *widget = new QWidget(parent);
    widget->setFixedSize(60, 40);
    
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setSpacing(2);
    layout->setContentsMargins(4, 2, 4, 2);

    QLabel *titleLabel = new QLabel(title);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { font-size: 8px; font-weight: bold; }");
    layout->addWidget(titleLabel);

    QLabel *countLabel = new QLabel("0");
    countLabel->setObjectName("countLabel");
    countLabel->setAlignment(Qt::AlignCenter);
    countLabel->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; background-color: #404040; border: 1px solid #666666; border-radius: 3px; }");
    layout->addWidget(countLabel);

    return widget;
}

void MainWindowAlertIntegration::updateAlertStatusWidget(QWidget* widget, AlertSystem::AlertLevel level, int count)
{
    QLabel *countLabel = widget->findChild<QLabel*>("countLabel");
    if (countLabel) {
        countLabel->setText(QString::number(count));
        
        QColor color = getAlertLevelColor(level);
        QString bgColor = (count > 0) ? color.name() : "#404040";
        QString textColor = (count > 0) ? "#ffffff" : "#cccccc";
        
        countLabel->setStyleSheet(QString(
            "QLabel { "
            "font-size: 14px; font-weight: bold; "
            "background-color: %1; "
            "color: %2; "
            "border: 1px solid #666666; "
            "border-radius: 3px; "
            "}"
        ).arg(bgColor).arg(textColor));
    }
}

QColor MainWindowAlertIntegration::getAlertLevelColor(AlertSystem::AlertLevel level) const
{
    switch (level) {
        case AlertSystem::AlertLevel::Info:      return QColor(0, 255, 0);    // Green
        case AlertSystem::AlertLevel::Warning:   return QColor(255, 255, 0);  // Yellow
        case AlertSystem::AlertLevel::Critical:  return QColor(255, 165, 0);  // Orange
        case AlertSystem::AlertLevel::Emergency: return QColor(255, 0, 0);    // Red
        default: return QColor(128, 128, 128);                               // Gray
    }
}

QIcon MainWindowAlertIntegration::getAlertLevelIcon(AlertSystem::AlertLevel level) const
{
    switch (level) {
        case AlertSystem::AlertLevel::Info:      return QIcon(":/icons/alert_info.png");
        case AlertSystem::AlertLevel::Warning:   return QIcon(":/icons/alert_warning.png");
        case AlertSystem::AlertLevel::Critical:  return QIcon(":/icons/alert_critical.png");
        case AlertSystem::AlertLevel::Emergency: return QIcon(":/icons/alert_emergency.png");
        default: return QIcon(":/icons/alert_unknown.png");
    }
}

QString MainWindowAlertIntegration::formatAlertMessage(AlertSystem::AlertLevel level, 
                                                      AlertSystem::AlertType type, 
                                                      const QString& message) const
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString levelStr;
    QString typeStr;

    switch (level) {
        case AlertSystem::AlertLevel::Info:      levelStr = "INFO"; break;
        case AlertSystem::AlertLevel::Warning:   levelStr = "WARN"; break;
        case AlertSystem::AlertLevel::Critical:  levelStr = "CRIT"; break;
        case AlertSystem::AlertLevel::Emergency: levelStr = "EMRG"; break;
    }

    switch (type) {
        case AlertSystem::AlertType::SignalQuality: typeStr = "Signal"; break;
        case AlertSystem::AlertType::ErrorRate:     typeStr = "Error"; break;
        case AlertSystem::AlertType::AudioLevel:    typeStr = "Audio"; break;
        case AlertSystem::AlertType::ServiceLoss:   typeStr = "Service"; break;
        case AlertSystem::AlertType::SystemError:   typeStr = "System"; break;
    }

    return QString("[%1] %2/%3: %4").arg(timestamp, levelStr, typeStr, message);
}

void MainWindowAlertIntegration::applyBroadcastStyling()
{
    // Apply professional broadcast styling to dock panels and toolbars
    QString dockStyle = R"(
        QDockWidget {
            background-color: #2b2b2b;
            color: #ffffff;
            border: 1px solid #555555;
        }
        
        QDockWidget::title {
            background-color: #404040;
            padding: 4px;
            border-bottom: 1px solid #555555;
        }
        
        QToolBar {
            background-color: #3a3a3a;
            border: 1px solid #555555;
            spacing: 2px;
        }
        
        QToolBar::separator {
            background-color: #555555;
            width: 1px;
            margin: 2px;
        }
        
        QGroupBox {
            font-weight: bold;
            border: 2px solid #555555;
            border-radius: 5px;
            margin-top: 1ex;
            padding-top: 8px;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
        }
        
        QListWidget {
            background-color: #404040;
            alternate-background-color: #4a4a4a;
            border: 1px solid #666666;
            border-radius: 3px;
        }
        
        QPushButton {
            background-color: #505050;
            border: 1px solid #666666;
            border-radius: 4px;
            padding: 4px 8px;
            min-width: 60px;
        }
        
        QPushButton:hover {
            background-color: #606060;
        }
        
        QPushButton:pressed {
            background-color: #404040;
        }
        
        QProgressBar {
            border: 1px solid #666666;
            border-radius: 3px;
            text-align: center;
        }
    )";

    if (m_alertDock) {
        m_alertDock->setStyleSheet(dockStyle);
    }
    if (m_recordingToolbar) {
        m_recordingToolbar->setStyleSheet(dockStyle);
    }
    if (m_audioToolbar) {
        m_audioToolbar->setStyleSheet(dockStyle);
    }
}

// Public slot implementations
void MainWindowAlertIntegration::showAlertConfiguration()
{
    if (m_configDialog) {
        m_configDialog->show();
        m_configDialog->raise();
        m_configDialog->activateWindow();
    }
}

void MainWindowAlertIntegration::setAlertDockVisible(bool visible)
{
    if (m_alertDock) {
        m_alertDock->setVisible(visible);
    }
}

void MainWindowAlertIntegration::clearAllAlerts()
{
    if (m_alertSystem) {
        m_alertSystem->clearActiveAlerts();
        updateAlertStatusDisplay();
    }
}

void MainWindowAlertIntegration::acknowledgeSelectedAlert()
{
    QListWidgetItem *item = m_activeAlertsList->currentItem();
    if (item && m_alertSystem) {
        int alertId = item->data(Qt::UserRole).toInt();
        m_alertSystem->acknowledgeAlert(alertId);
    }
}

void MainWindowAlertIntegration::toggleAlertMonitoring()
{
    m_alertMonitoringEnabled = !m_alertMonitoringEnabled;
    
    if (m_alertMonitoringEnabled) {
        startAlertMonitoring();
    } else {
        stopAlertMonitoring();
    }
    
    emit alertMonitoringChanged(m_alertMonitoringEnabled);
}

void MainWindowAlertIntegration::startAlertMonitoring()
{
    if (m_alertSystem && !m_alertMonitoringEnabled) {
        m_alertSystem->enableMonitoring(true);
        m_alertMonitoringEnabled = true;
        
        if (m_alertMonitoringAction) {
            m_alertMonitoringAction->setChecked(true);
        }
        if (m_alertStatusLabel) {
            m_alertStatusLabel->setText(tr("Alerts: ON"));
            m_alertStatusLabel->setStyleSheet("QLabel { margin: 4px; padding: 2px; color: green; }");
        }
        
        qInfo() << "Alert monitoring started";
    }
}

void MainWindowAlertIntegration::stopAlertMonitoring()
{
    if (m_alertSystem && m_alertMonitoringEnabled) {
        m_alertSystem->enableMonitoring(false);
        m_alertMonitoringEnabled = false;
        
        if (m_alertMonitoringAction) {
            m_alertMonitoringAction->setChecked(false);
        }
        if (m_alertStatusLabel) {
            m_alertStatusLabel->setText(tr("Alerts: OFF"));
            m_alertStatusLabel->setStyleSheet("QLabel { margin: 4px; padding: 2px; color: red; }");
        }
        
        qInfo() << "Alert monitoring stopped";
    }
}

void MainWindowAlertIntegration::showRecordingControls()
{
    // TODO: Implement recording controls dialog
    QMessageBox::information(m_mainWindow, tr("Recording Controls"), 
                           tr("Recording controls dialog will be implemented in a future update."));
}

void MainWindowAlertIntegration::showAudioExtractionDialog()
{
    // TODO: Implement audio extraction dialog
    QMessageBox::information(m_mainWindow, tr("Audio Extraction"), 
                           tr("Audio extraction dialog will be implemented in a future update."));
}

// Private slot implementations
void MainWindowAlertIntegration::handleAlertTriggered(AlertSystem::AlertLevel level, 
                                                     AlertSystem::AlertType type, 
                                                     const QString& message, 
                                                     const QVariantHash& parameters)
{
    // Update alert counts
    m_alertCountByLevel[level]++;
    m_activeAlertCount++;
    m_totalAlertCount++;

    // Create list item
    QString formattedMessage = formatAlertMessage(level, type, message);
    QListWidgetItem *item = new QListWidgetItem(formattedMessage);
    item->setIcon(getAlertLevelIcon(level));
    item->setForeground(QBrush(getAlertLevelColor(level)));
    item->setData(Qt::UserRole, m_totalAlertCount); // Use as alert ID

    // Insert at top of list
    m_activeAlertsList->insertItem(0, item);

    // Limit list size
    while (m_activeAlertsList->count() > MAX_DISPLAYED_ALERTS) {
        delete m_activeAlertsList->takeItem(m_activeAlertsList->count() - 1);
    }

    // Show system tray notification
    if (m_systemTray) {
        QString title = tr("ETI Stream Analyzer Alert");
        QString trayMessage = QString("%1: %2").arg(QMetaEnum::fromType<AlertSystem::AlertLevel>().valueToKey(static_cast<int>(level)), message);
        QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information;
        
        switch (level) {
            case AlertSystem::AlertLevel::Warning:   icon = QSystemTrayIcon::Warning; break;
            case AlertSystem::AlertLevel::Critical:
            case AlertSystem::AlertLevel::Emergency: icon = QSystemTrayIcon::Critical; break;
            default: break;
        }
        
        m_systemTray->showMessage(title, trayMessage, icon, 5000);
    }

    // Update displays
    updateAlertStatusDisplay();
    updateToolbarIndicators();

    emit alertReceived(level, type, message);
}

void MainWindowAlertIntegration::handleAlertAcknowledged(int alertId)
{
    // Find and remove acknowledged alert from list
    for (int i = 0; i < m_activeAlertsList->count(); ++i) {
        QListWidgetItem *item = m_activeAlertsList->item(i);
        if (item && item->data(Qt::UserRole).toInt() == alertId) {
            delete m_activeAlertsList->takeItem(i);
            m_activeAlertCount--;
            break;
        }
    }

    updateAlertStatusDisplay();
    emit alertAcknowledged(alertId);
}

void MainWindowAlertIntegration::handleSystemTrayActivation(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
            m_mainWindow->show();
            m_mainWindow->raise();
            m_mainWindow->activateWindow();
            break;
        case QSystemTrayIcon::Context:
            // Context menu is handled automatically
            break;
        default:
            break;
    }
}

void MainWindowAlertIntegration::updateAlertStatusDisplay()
{
    if (!m_alertSystem) return;

    // Update alert count label
    QString countText;
    if (m_activeAlertCount == 0) {
        countText = tr("No active alerts");
        m_alertCountLabel->setStyleSheet("QLabel { font-weight: bold; color: green; }");
    } else {
        countText = tr("%1 active alert%2").arg(m_activeAlertCount).arg(m_activeAlertCount > 1 ? "s" : "");
        
        // Determine color based on highest alert level
        QString color = "green";
        if (m_alertCountByLevel[AlertSystem::AlertLevel::Emergency] > 0) {
            color = "red";
        } else if (m_alertCountByLevel[AlertSystem::AlertLevel::Critical] > 0) {
            color = "orange";
        } else if (m_alertCountByLevel[AlertSystem::AlertLevel::Warning] > 0) {
            color = "yellow";
        }
        
        m_alertCountLabel->setStyleSheet(QString("QLabel { font-weight: bold; color: %1; }").arg(color));
    }
    m_alertCountLabel->setText(countText);

    // Update alert level indicator
    int maxLevel = 0;
    if (m_alertCountByLevel[AlertSystem::AlertLevel::Emergency] > 0) maxLevel = 3;
    else if (m_alertCountByLevel[AlertSystem::AlertLevel::Critical] > 0) maxLevel = 2;
    else if (m_alertCountByLevel[AlertSystem::AlertLevel::Warning] > 0) maxLevel = 1;
    
    m_alertLevelIndicator->setValue(maxLevel);

    // Update status widgets
    updateAlertStatusWidget(m_infoStatusWidget, AlertSystem::AlertLevel::Info, 
                           m_alertCountByLevel[AlertSystem::AlertLevel::Info]);
    updateAlertStatusWidget(m_warningStatusWidget, AlertSystem::AlertLevel::Warning, 
                           m_alertCountByLevel[AlertSystem::AlertLevel::Warning]);
    updateAlertStatusWidget(m_criticalStatusWidget, AlertSystem::AlertLevel::Critical, 
                           m_alertCountByLevel[AlertSystem::AlertLevel::Critical]);
    updateAlertStatusWidget(m_emergencyStatusWidget, AlertSystem::AlertLevel::Emergency, 
                           m_alertCountByLevel[AlertSystem::AlertLevel::Emergency]);

    // Enable/disable acknowledge button
    m_acknowledgeButton->setEnabled(m_activeAlertsList->currentItem() != nullptr);
}

void MainWindowAlertIntegration::updateToolbarIndicators()
{
    if (!m_alertStatusIndicator) return;

    // Update status indicator
    int maxLevel = 0;
    if (m_alertCountByLevel[AlertSystem::AlertLevel::Emergency] > 0) maxLevel = 3;
    else if (m_alertCountByLevel[AlertSystem::AlertLevel::Critical] > 0) maxLevel = 2;
    else if (m_alertCountByLevel[AlertSystem::AlertLevel::Warning] > 0) maxLevel = 1;
    
    m_alertStatusIndicator->setValue(maxLevel);

    // Update alert status label
    if (m_alertStatusLabel) {
        QString status = m_alertMonitoringEnabled ? tr("ON") : tr("OFF");
        QString color = m_alertMonitoringEnabled ? "green" : "red";
        m_alertStatusLabel->setText(tr("Alerts: %1").arg(status));
        m_alertStatusLabel->setStyleSheet(QString("QLabel { margin: 4px; padding: 2px; color: %1; }").arg(color));
    }
}

void MainWindowAlertIntegration::handleAlertListSelectionChanged()
{
    m_acknowledgeButton->setEnabled(m_activeAlertsList->currentItem() != nullptr);
}

void MainWindowAlertIntegration::handleTestAlert(AlertSystem::AlertLevel level, 
                                                AlertSystem::AlertType type, 
                                                const QString& message)
{
    // Handle test alert from configuration dialog
    handleAlertTriggered(level, type, message, QVariantHash());
}

#include "main_window_alert_integration.moc"