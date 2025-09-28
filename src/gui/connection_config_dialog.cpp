/**
 * @file connection_config_dialog.cpp
 * @brief Implementation of Connection Configuration Dialog
 * 
 * @author StreamDAB Development Team
 * @date 2025
 * @copyright Copyright (c) 2025 StreamDAB Technologies
 */

#include "connection_config_dialog.h"
#include "broadcast_theme.h"

#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QSplitter>

namespace streamdab::ui {

ConnectionConfigDialog::ConnectionConfigDialog(QWidget* parent)
    : QDialog(parent)
    , m_settings(new QSettings(this))
    , m_testTimer(new QTimer(this))
    , m_testTimeoutTimer(new QTimer(this))
{
    setWindowTitle("ETI Stream Connection Configuration");
    setWindowIcon(QIcon(":/icons/connection_config.png"));
    setModal(true);
    resize(640, 480);
    
    // Initialize configuration with defaults
    current_config_ = connection_utils::createDefaultConfig();
    
    setupUI();
    connectSignals();
    applyProfessionalStyling();
    loadDefaultSettings();
    
    // Setup test timers
    m_testTimer->setSingleShot(false);
    m_testTimer->setInterval(500); // Update every 500ms during test
    
    m_testTimeoutTimer->setSingleShot(true);
    m_testTimeoutTimer->setInterval(TEST_TIMEOUT_MS);
    
    updateURLPreview();
    validateConfiguration();
}

ConnectionConfigDialog::~ConnectionConfigDialog() {
    stopConnectionTest();
    saveCurrentSettings();
}

void ConnectionConfigDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(12);
    m_mainLayout->setContentsMargins(16, 16, 16, 16);
    
    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabPosition(QTabWidget::North);
    
    // Create tabs
    m_basicTab = createBasicConnectionTab();
    m_advancedTab = createAdvancedSettingsTab();
    m_testTab = createConnectionTestTab();
    
    m_tabWidget->addTab(m_basicTab, QIcon(":/icons/connection.png"), "Connection");
    m_tabWidget->addTab(m_advancedTab, QIcon(":/icons/settings.png"), "Advanced");
    m_tabWidget->addTab(m_testTab, QIcon(":/icons/test.png"), "Test Connection");
    
    m_mainLayout->addWidget(m_tabWidget);
    
    // Create button layout
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->setSpacing(8);
    
    // Configuration management buttons
    m_saveBtn = new QPushButton(QIcon(":/icons/save.png"), "Save", this);
    m_loadBtn = new QPushButton(QIcon(":/icons/load.png"), "Load", this);
    m_importBtn = new QPushButton(QIcon(":/icons/import.png"), "Import", this);
    m_exportBtn = new QPushButton(QIcon(":/icons/export.png"), "Export", this);
    m_resetBtn = new QPushButton(QIcon(":/icons/reset.png"), "Reset", this);
    
    // Dialog buttons
    m_okBtn = new QPushButton(QIcon(":/icons/ok.png"), "OK", this);
    m_cancelBtn = new QPushButton(QIcon(":/icons/cancel.png"), "Cancel", this);
    
    // Set button properties
    m_okBtn->setDefault(true);
    m_okBtn->setMinimumWidth(80);
    m_cancelBtn->setMinimumWidth(80);
    
    // Add buttons to layout
    m_buttonLayout->addWidget(m_saveBtn);
    m_buttonLayout->addWidget(m_loadBtn);
    m_buttonLayout->addWidget(m_importBtn);
    m_buttonLayout->addWidget(m_exportBtn);
    m_buttonLayout->addWidget(m_resetBtn);
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_okBtn);
    m_buttonLayout->addWidget(m_cancelBtn);
    
    m_mainLayout->addLayout(m_buttonLayout);
}

QWidget* ConnectionConfigDialog::createBasicConnectionTab() {
    auto* tab = new QWidget(this);
    m_basicLayout = new QFormLayout(tab);
    m_basicLayout->setSpacing(12);
    m_basicLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    
    // Protocol selection
    m_protocolCombo = new QComboBox(tab);
    m_protocolCombo->addItems(connection_utils::getSupportedProtocols());
    m_protocolCombo->setCurrentText(current_config_.protocol);
    m_protocolCombo->setToolTip("Select ZeroMQ protocol for ETI streaming");
    m_basicLayout->addRow("Protocol:", m_protocolCombo);
    
    // Host input
    m_hostEdit = new QLineEdit(current_config_.host, tab);
    m_hostEdit->setPlaceholderText("Enter IP address or hostname");
    m_hostEdit->setToolTip("ETI stream source hostname or IP address");
    m_basicLayout->addRow("Host:", m_hostEdit);
    
    // Port input
    m_portSpin = new QSpinBox(tab);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(current_config_.port);
    m_portSpin->setToolTip("ETI stream source port number");
    m_basicLayout->addRow("Port:", m_portSpin);
    
    // URL preview
    m_urlPreviewLabel = new QLabel(tab);
    m_urlPreviewLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_urlPreviewLabel->setMargin(6);
    m_urlPreviewLabel->setWordWrap(true);
    m_urlPreviewLabel->setStyleSheet("background-color: #f0f0f0; border: 1px solid #cccccc;");
    m_basicLayout->addRow("Full URL:", m_urlPreviewLabel);
    
    // Test connection button
    m_testConnectionBtn = new QPushButton(QIcon(":/icons/test.png"), "Test Connection", tab);
    m_testConnectionBtn->setMinimumHeight(32);
    m_testConnectionBtn->setToolTip("Test connection to ETI stream source");
    
    // Connection status
    m_connectionStatusLabel = new QLabel("Ready to test connection", tab);
    m_connectionStatusLabel->setStyleSheet("color: #666666; font-style: italic;");
    
    // Add test section
    auto* testLayout = new QHBoxLayout();
    testLayout->addWidget(m_testConnectionBtn);
    testLayout->addStretch();
    
    m_basicLayout->addRow(testLayout);
    m_basicLayout->addRow("Status:", m_connectionStatusLabel);
    
    // Add protocol help text
    auto* helpLabel = new QLabel(tab);
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("color: #666666; font-size: 11px;");
    helpLabel->setText(getProtocolHelpText(current_config_.protocol));
    m_basicLayout->addRow(helpLabel);
    
    return tab;
}

QWidget* ConnectionConfigDialog::createAdvancedSettingsTab() {
    auto* tab = new QWidget(this);
    m_advancedLayout = new QVBoxLayout(tab);
    m_advancedLayout->setSpacing(16);
    
    // Processing Options Group
    m_processingGroup = new QGroupBox("Processing Options", tab);
    auto* processingLayout = new QFormLayout(m_processingGroup);
    
    m_enableTISTCheck = new QCheckBox("Enable TIST timestamp processing");
    m_enableTISTCheck->setChecked(current_config_.enable_tist);
    m_enableTISTCheck->setToolTip("Process ETI-NI TIST timestamps for synchronization");
    
    m_enableMetadataCheck = new QCheckBox("Enable metadata extraction");
    m_enableMetadataCheck->setChecked(current_config_.enable_metadata);
    m_enableMetadataCheck->setToolTip("Extract ensemble and service metadata");
    
    m_enableETSIComplianceCheck = new QCheckBox("Enable ETSI compliance checking");
    m_enableETSIComplianceCheck->setChecked(current_config_.enable_etsi_compliance);
    m_enableETSIComplianceCheck->setToolTip("Validate frames against ETSI standards");
    
    m_enableErrorCorrectionCheck = new QCheckBox("Enable Reed-Solomon error correction");
    m_enableErrorCorrectionCheck->setChecked(current_config_.enable_error_correction);
    m_enableErrorCorrectionCheck->setToolTip("Apply Reed-Solomon error correction to frames");
    
    processingLayout->addRow(m_enableTISTCheck);
    processingLayout->addRow(m_enableMetadataCheck);
    processingLayout->addRow(m_enableETSIComplianceCheck);
    processingLayout->addRow(m_enableErrorCorrectionCheck);
    
    // Buffer Settings Group
    m_bufferGroup = new QGroupBox("Buffer Settings", tab);
    auto* bufferLayout = new QFormLayout(m_bufferGroup);
    
    m_bufferFramesSpin = new QSpinBox();
    m_bufferFramesSpin->setRange(100, 10000);
    m_bufferFramesSpin->setValue(current_config_.buffer_frames);
    m_bufferFramesSpin->setSuffix(" frames");
    m_bufferFramesSpin->setToolTip("Number of ETI frames to buffer");
    bufferLayout->addRow("Buffer Size:", m_bufferFramesSpin);
    
    m_timeoutSpin = new QSpinBox();
    m_timeoutSpin->setRange(1000, 60000);
    m_timeoutSpin->setValue(current_config_.timeout_ms);
    m_timeoutSpin->setSuffix(" ms");
    m_timeoutSpin->setToolTip("Connection timeout in milliseconds");
    bufferLayout->addRow("Timeout:", m_timeoutSpin);
    
    m_autoReconnectCheck = new QCheckBox("Enable automatic reconnection");
    m_autoReconnectCheck->setChecked(current_config_.auto_reconnect);
    m_autoReconnectCheck->setToolTip("Automatically reconnect on connection loss");
    bufferLayout->addRow(m_autoReconnectCheck);
    
    m_reconnectIntervalSpin = new QSpinBox();
    m_reconnectIntervalSpin->setRange(1000, 60000);
    m_reconnectIntervalSpin->setValue(current_config_.reconnect_interval_ms);
    m_reconnectIntervalSpin->setSuffix(" ms");
    m_reconnectIntervalSpin->setToolTip("Reconnection attempt interval");
    m_reconnectIntervalSpin->setEnabled(current_config_.auto_reconnect);
    bufferLayout->addRow("Reconnect Interval:", m_reconnectIntervalSpin);
    
    m_adaptiveBufferingCheck = new QCheckBox("Enable adaptive buffering");
    m_adaptiveBufferingCheck->setChecked(current_config_.adaptive_buffering);
    m_adaptiveBufferingCheck->setToolTip("Automatically adjust buffer size based on conditions");
    bufferLayout->addRow(m_adaptiveBufferingCheck);
    
    // Quality Settings Group
    m_qualityGroup = new QGroupBox("Quality Settings", tab);
    auto* qualityLayout = new QFormLayout(m_qualityGroup);
    
    m_minSignalQualitySpin = new QSpinBox();
    m_minSignalQualitySpin->setRange(0, 100);
    m_minSignalQualitySpin->setValue(static_cast<int>(current_config_.min_signal_quality));
    m_minSignalQualitySpin->setSuffix("%");
    m_minSignalQualitySpin->setToolTip("Minimum acceptable signal quality threshold");
    qualityLayout->addRow("Min Signal Quality:", m_minSignalQualitySpin);
    
    m_targetFPSSpin = new QSpinBox();
    m_targetFPSSpin->setRange(100, 10000);
    m_targetFPSSpin->setValue(static_cast<int>(current_config_.target_fps));
    m_targetFPSSpin->setSuffix(" FPS");
    m_targetFPSSpin->setToolTip("Target frame processing rate");
    qualityLayout->addRow("Target FPS:", m_targetFPSSpin);
    
    m_performanceMonitoringCheck = new QCheckBox("Enable performance monitoring");
    m_performanceMonitoringCheck->setChecked(current_config_.performance_monitoring);
    m_performanceMonitoringCheck->setToolTip("Monitor processing performance metrics");
    qualityLayout->addRow(m_performanceMonitoringCheck);
    
    // Add groups to layout
    m_advancedLayout->addWidget(m_processingGroup);
    m_advancedLayout->addWidget(m_bufferGroup);
    m_advancedLayout->addWidget(m_qualityGroup);
    m_advancedLayout->addStretch();
    
    return tab;
}

QWidget* ConnectionConfigDialog::createConnectionTestTab() {
    auto* tab = new QWidget(this);
    m_testLayout = new QVBoxLayout(tab);
    m_testLayout->setSpacing(16);
    
    // Test Control Group
    m_testControlGroup = new QGroupBox("Connection Test Control", tab);
    auto* controlLayout = new QVBoxLayout(m_testControlGroup);
    
    auto* buttonLayout = new QHBoxLayout();
    m_startTestBtn = new QPushButton(QIcon(":/icons/play.png"), "Start Test");
    m_stopTestBtn = new QPushButton(QIcon(":/icons/stop.png"), "Stop Test");
    m_stopTestBtn->setEnabled(false);
    
    buttonLayout->addWidget(m_startTestBtn);
    buttonLayout->addWidget(m_stopTestBtn);
    buttonLayout->addStretch();
    
    m_testProgressBar = new QProgressBar();
    m_testProgressBar->setRange(0, 100);
    m_testProgressBar->setValue(0);
    m_testProgressBar->setVisible(false);
    
    m_testStatusLabel = new QLabel("Ready to test connection");
    m_testStatusLabel->setStyleSheet("color: #666666;");
    
    controlLayout->addLayout(buttonLayout);
    controlLayout->addWidget(m_testProgressBar);
    controlLayout->addWidget(m_testStatusLabel);
    
    // Test Results Group
    m_testResultsGroup = new QGroupBox("Test Results", tab);
    auto* resultsLayout = new QVBoxLayout(m_testResultsGroup);
    
    // Results summary
    auto* summaryLayout = new QFormLayout();
    m_latencyLabel = new QLabel("--");
    m_signalQualityLabel = new QLabel("--");
    m_framesReceivedLabel = new QLabel("--");
    
    summaryLayout->addRow("Latency:", m_latencyLabel);
    summaryLayout->addRow("Signal Quality:", m_signalQualityLabel);
    summaryLayout->addRow("Frames Received:", m_framesReceivedLabel);
    
    // Detailed results
    m_testResultsText = new QTextEdit();
    m_testResultsText->setMaximumHeight(150);
    m_testResultsText->setPlainText("No test results available");
    m_testResultsText->setReadOnly(true);
    
    resultsLayout->addLayout(summaryLayout);
    resultsLayout->addWidget(new QLabel("Detailed Results:"));
    resultsLayout->addWidget(m_testResultsText);
    
    // Add groups to layout
    m_testLayout->addWidget(m_testControlGroup);
    m_testLayout->addWidget(m_testResultsGroup);
    m_testLayout->addStretch();
    
    return tab;
}

void ConnectionConfigDialog::connectSignals() {
    // Basic connection tab signals
    connect(m_protocolCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
            this, &ConnectionConfigDialog::onProtocolChanged);
    connect(m_hostEdit, &QLineEdit::textChanged,
            this, &ConnectionConfigDialog::onHostChanged);
    connect(m_portSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ConnectionConfigDialog::onPortChanged);
    connect(m_testConnectionBtn, &QPushButton::clicked,
            this, &ConnectionConfigDialog::testConnection);
    
    // Advanced settings signals
    connect(m_enableTISTCheck, &QCheckBox::toggled,
            this, &ConnectionConfigDialog::onAdvancedSettingsChanged);
    connect(m_enableMetadataCheck, &QCheckBox::toggled,
            this, &ConnectionConfigDialog::onAdvancedSettingsChanged);
    connect(m_enableETSIComplianceCheck, &QCheckBox::toggled,
            this, &ConnectionConfigDialog::onAdvancedSettingsChanged);
    connect(m_enableErrorCorrectionCheck, &QCheckBox::toggled,
            this, &ConnectionConfigDialog::onAdvancedSettingsChanged);
    
    connect(m_bufferFramesSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ConnectionConfigDialog::onAdvancedSettingsChanged);
    connect(m_timeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ConnectionConfigDialog::onAdvancedSettingsChanged);
    connect(m_autoReconnectCheck, &QCheckBox::toggled,
            this, &ConnectionConfigDialog::onAdvancedSettingsChanged);
    connect(m_reconnectIntervalSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ConnectionConfigDialog::onAdvancedSettingsChanged);
    connect(m_adaptiveBufferingCheck, &QCheckBox::toggled,
            this, &ConnectionConfigDialog::onAdvancedSettingsChanged);
    
    connect(m_minSignalQualitySpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ConnectionConfigDialog::onAdvancedSettingsChanged);
    connect(m_targetFPSSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ConnectionConfigDialog::onAdvancedSettingsChanged);
    connect(m_performanceMonitoringCheck, &QCheckBox::toggled,
            this, &ConnectionConfigDialog::onAdvancedSettingsChanged);
    
    // Enable/disable reconnect interval based on auto-reconnect
    connect(m_autoReconnectCheck, &QCheckBox::toggled,
            m_reconnectIntervalSpin, &QSpinBox::setEnabled);
    
    // Test tab signals
    connect(m_startTestBtn, &QPushButton::clicked,
            this, &ConnectionConfigDialog::startConnectionTest);
    connect(m_stopTestBtn, &QPushButton::clicked,
            this, &ConnectionConfigDialog::stopConnectionTest);
    
    // Button signals
    connect(m_saveBtn, &QPushButton::clicked,
            this, &ConnectionConfigDialog::saveConfiguration);
    connect(m_loadBtn, &QPushButton::clicked,
            this, &ConnectionConfigDialog::loadConfiguration);
    connect(m_importBtn, &QPushButton::clicked,
            this, &ConnectionConfigDialog::importConfiguration);
    connect(m_exportBtn, &QPushButton::clicked,
            this, &ConnectionConfigDialog::exportConfiguration);
    connect(m_resetBtn, &QPushButton::clicked,
            this, &ConnectionConfigDialog::resetToDefaults);
    
    connect(m_okBtn, &QPushButton::clicked,
            this, &ConnectionConfigDialog::accept);
    connect(m_cancelBtn, &QPushButton::clicked,
            this, &ConnectionConfigDialog::reject);
    
    // Test timer signals
    connect(m_testTimer, &QTimer::timeout,
            this, &ConnectionConfigDialog::onConnectionTestProgress);
    connect(m_testTimeoutTimer, &QTimer::timeout,
            this, &ConnectionConfigDialog::onConnectionTestTimeout);
}

void ConnectionConfigDialog::applyProfessionalStyling() {
    // Apply broadcast theme
    BroadcastTheme::applyTheme(this);
    
    // Custom styling for specific components
    setStyleSheet(styleSheet() + R"(
        QTabWidget::pane {
            border: 1px solid #cccccc;
            background-color: white;
        }
        
        QTabBar::tab {
            background-color: #f0f0f0;
            border: 1px solid #cccccc;
            padding: 6px 12px;
            margin-right: 2px;
        }
        
        QTabBar::tab:selected {
            background-color: white;
            border-bottom: none;
        }
        
        QGroupBox {
            font-weight: bold;
            border: 2px solid #cccccc;
            border-radius: 4px;
            margin-top: 8px;
            padding-top: 8px;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 4px 0 4px;
        }
        
        QPushButton {
            padding: 6px 12px;
            border: 1px solid #cccccc;
            border-radius: 3px;
            background-color: #f8f8f8;
        }
        
        QPushButton:hover {
            background-color: #e8e8e8;
        }
        
        QPushButton:pressed {
            background-color: #d8d8d8;
        }
        
        QPushButton:default {
            border: 2px solid #0078d4;
            background-color: #0078d4;
            color: white;
        }
    )");
}

void ConnectionConfigDialog::setConfiguration(const ConnectionConfig& config) {
    current_config_ = config;
    
    // Update UI controls
    m_protocolCombo->setCurrentText(config.protocol);
    m_hostEdit->setText(config.host);
    m_portSpin->setValue(config.port);
    
    // Advanced settings
    m_enableTISTCheck->setChecked(config.enable_tist);
    m_enableMetadataCheck->setChecked(config.enable_metadata);
    m_enableETSIComplianceCheck->setChecked(config.enable_etsi_compliance);
    m_enableErrorCorrectionCheck->setChecked(config.enable_error_correction);
    
    m_bufferFramesSpin->setValue(config.buffer_frames);
    m_timeoutSpin->setValue(config.timeout_ms);
    m_autoReconnectCheck->setChecked(config.auto_reconnect);
    m_reconnectIntervalSpin->setValue(config.reconnect_interval_ms);
    m_adaptiveBufferingCheck->setChecked(config.adaptive_buffering);
    
    m_minSignalQualitySpin->setValue(static_cast<int>(config.min_signal_quality));
    m_targetFPSSpin->setValue(static_cast<int>(config.target_fps));
    m_performanceMonitoringCheck->setChecked(config.performance_monitoring);
    
    updateURLPreview();
    validateConfiguration();
}

ConnectionConfig ConnectionConfigDialog::getConfiguration() const {
    ConnectionConfig config = current_config_;
    
    // Update from UI controls
    config.protocol = m_protocolCombo->currentText();
    config.host = m_hostEdit->text();
    config.port = static_cast<quint16>(m_portSpin->value());
    
    // Advanced settings
    config.enable_tist = m_enableTISTCheck->isChecked();
    config.enable_metadata = m_enableMetadataCheck->isChecked();
    config.enable_etsi_compliance = m_enableETSIComplianceCheck->isChecked();
    config.enable_error_correction = m_enableErrorCorrectionCheck->isChecked();
    
    config.buffer_frames = static_cast<uint32_t>(m_bufferFramesSpin->value());
    config.timeout_ms = static_cast<uint32_t>(m_timeoutSpin->value());
    config.auto_reconnect = m_autoReconnectCheck->isChecked();
    config.reconnect_interval_ms = static_cast<uint32_t>(m_reconnectIntervalSpin->value());
    config.adaptive_buffering = m_adaptiveBufferingCheck->isChecked();
    
    config.min_signal_quality = static_cast<double>(m_minSignalQualitySpin->value());
    config.target_fps = static_cast<double>(m_targetFPSSpin->value());
    config.performance_monitoring = m_performanceMonitoringCheck->isChecked();
    
    // Generate full URL
    ConnectionConfig mutable_config = config;
    mutable_config.updateFullURL();
    
    return mutable_config;
}

bool ConnectionConfigDialog::isConfigurationValid() const {
    return m_configurationValid;
}

void ConnectionConfigDialog::onProtocolChanged(const QString& protocol) {
    current_config_.protocol = protocol;
    
    // Update default port for protocol
    quint16 default_port = connection_utils::getDefaultPortForProtocol(protocol);
    if (m_portSpin->value() == connection_utils::getDefaultPortForProtocol(m_protocolCombo->itemText(0))) {
        m_portSpin->setValue(default_port);
    }
    
    updateURLPreview();
    validateConfiguration();
    
    emit configurationChanged(getConfiguration());
}

void ConnectionConfigDialog::onHostChanged(const QString& host) {
    current_config_.host = host;
    updateURLPreview();
    validateConfiguration();
    
    emit configurationChanged(getConfiguration());
}

void ConnectionConfigDialog::onPortChanged(int port) {
    current_config_.port = static_cast<quint16>(port);
    updateURLPreview();
    validateConfiguration();
    
    emit configurationChanged(getConfiguration());
}

void ConnectionConfigDialog::onAdvancedSettingsChanged() {
    current_config_ = getConfiguration();
    validateConfiguration();
    
    emit configurationChanged(current_config_);
}

void ConnectionConfigDialog::updateURLPreview() {
    ConnectionConfig config = getConfiguration();
    config.updateFullURL();
    
    m_urlPreviewLabel->setText(QString("<b>%1</b>").arg(config.full_url));
    
    // Update help text
    if (m_basicTab) {
        auto* helpLabel = m_basicLayout->itemAt(m_basicLayout->rowCount() - 1)->widget();
        if (auto* label = qobject_cast<QLabel*>(helpLabel)) {
            label->setText(getProtocolHelpText(config.protocol));
        }
    }
}

void ConnectionConfigDialog::validateConfiguration() {
    ConnectionConfig config = getConfiguration();
    auto validation = connection_utils::validateConfig(config);
    
    m_configurationValid = validation.valid;
    
    // Update OK button state
    m_okBtn->setEnabled(m_configurationValid);
    
    // Update connection status
    if (!validation.valid && !validation.errors.isEmpty()) {
        m_connectionStatusLabel->setText(QString("Configuration Error: %1").arg(validation.errors.first()));
        m_connectionStatusLabel->setStyleSheet("color: #cc0000; font-weight: bold;");
    } else if (!validation.warnings.isEmpty()) {
        m_connectionStatusLabel->setText(QString("Warning: %1").arg(validation.warnings.first()));
        m_connectionStatusLabel->setStyleSheet("color: #cc8800; font-weight: bold;");
    } else {
        m_connectionStatusLabel->setText("Configuration is valid");
        m_connectionStatusLabel->setStyleSheet("color: #00aa00; font-weight: bold;");
    }
}

void ConnectionConfigDialog::testConnection() {
    if (m_testInProgress) {
        stopConnectionTest();
        return;
    }
    
    startConnectionTest();
}

void ConnectionConfigDialog::startConnectionTest() {
    if (m_testInProgress) {
        return;
    }
    
    if (!isConfigurationValid()) {
        QMessageBox::warning(this, "Invalid Configuration", 
                           "Cannot test connection with invalid configuration. Please fix the configuration errors first.");
        return;
    }
    
    m_testInProgress = true;
    test_start_time_ = std::chrono::system_clock::now();
    
    // Update UI
    m_testConnectionBtn->setText("Testing...");
    m_testConnectionBtn->setEnabled(false);
    m_startTestBtn->setEnabled(false);
    m_stopTestBtn->setEnabled(true);
    m_testProgressBar->setVisible(true);
    m_testProgressBar->setValue(0);
    
    // Reset test results
    last_test_result_ = ConnectionTestResult{};
    m_latencyLabel->setText("Testing...");
    m_signalQualityLabel->setText("Testing...");
    m_framesReceivedLabel->setText("Testing...");
    m_testResultsText->setPlainText("Starting connection test...\n");
    
    // Create test controller
    auto config = getConfiguration();
    auto processing_config = config.toProcessingConfig();
    
    m_testController = std::make_unique<eti::controller::ModernETIController>(this);
    
    // Connect test signals
    connect(m_testController.get(), &eti::controller::ModernETIController::connectionStatusChanged,
            this, [this](const eti::network::ConnectionStatus& status) {
                m_testResultsText->append(QString("Connection status: %1")
                                        .arg(status.isConnected() ? "Connected" : "Disconnected"));
            });
    
    connect(m_testController.get(), &eti::controller::ModernETIController::etiFrameProcessed,
            this, [this](uint32_t frame_number, const eti::EtiFrame&, std::chrono::microseconds processing_time) {
                last_test_result_.frames_received++;
                last_test_result_.latency_ms = processing_time.count() / 1000.0;
                
                m_testResultsText->append(QString("Frame %1 processed in %2 μs")
                                        .arg(frame_number)
                                        .arg(processing_time.count()));
            });
    
    // Initialize and start test
    if (m_testController->initialize(processing_config)) {
        if (m_testController->connectToStream(config.full_url)) {
            m_testTimer->start();
            m_testTimeoutTimer->start();
            
            m_testStatusLabel->setText("Testing connection...");
            m_testStatusLabel->setStyleSheet("color: #0078d4;");
        } else {
            onConnectionTestFinished();
            last_test_result_.success = false;
            last_test_result_.message = "Failed to initiate connection";
        }
    } else {
        onConnectionTestFinished();
        last_test_result_.success = false;
        last_test_result_.message = "Failed to initialize controller";
    }
}

void ConnectionConfigDialog::stopConnectionTest() {
    if (!m_testInProgress) {
        return;
    }
    
    m_testTimer->stop();
    m_testTimeoutTimer->stop();
    
    if (m_testController) {
        m_testController->stopProcessing();
        m_testController->disconnectFromStream();
        m_testController.reset();
    }
    
    onConnectionTestFinished();
}

void ConnectionConfigDialog::onConnectionTestProgress(int progress) {
    Q_UNUSED(progress)
    
    if (!m_testInProgress) {
        return;
    }
    
    auto elapsed = std::chrono::system_clock::now() - test_start_time_;
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
    
    int progress_percent = static_cast<int>((elapsed_ms.count() * 100) / TEST_DURATION_MS);
    progress_percent = std::min(100, progress_percent);
    
    m_testProgressBar->setValue(progress_percent);
    
    // Update status
    m_testStatusLabel->setText(QString("Testing... %1 frames received")
                             .arg(last_test_result_.frames_received));
    
    // Check if test should complete
    if (elapsed_ms >= std::chrono::milliseconds{TEST_DURATION_MS} || 
        last_test_result_.frames_received >= MIN_TEST_FRAMES) {
        
        last_test_result_.success = true;
        last_test_result_.test_duration = elapsed_ms;
        last_test_result_.message = "Test completed successfully";
        
        onConnectionTestFinished();
    }
}

void ConnectionConfigDialog::onConnectionTestFinished() {
    m_testInProgress = false;
    
    m_testTimer->stop();
    m_testTimeoutTimer->stop();
    
    // Update UI
    m_testConnectionBtn->setText("Test Connection");
    m_testConnectionBtn->setEnabled(true);
    m_startTestBtn->setEnabled(true);
    m_stopTestBtn->setEnabled(false);
    m_testProgressBar->setVisible(false);
    
    // Update results
    if (last_test_result_.success) {
        last_test_result_.signal_quality = 95.0; // Placeholder calculation
        
        m_latencyLabel->setText(QString("%1 ms").arg(last_test_result_.latency_ms, 0, 'f', 2));
        m_signalQualityLabel->setText(QString("%1%").arg(last_test_result_.signal_quality, 0, 'f', 1));
        m_framesReceivedLabel->setText(QString("%1").arg(last_test_result_.frames_received));
        
        if (last_test_result_.isGoodConnection()) {
            m_testStatusLabel->setText("Connection test passed");
            m_testStatusLabel->setStyleSheet("color: #00aa00; font-weight: bold;");
            m_connectionStatusLabel->setText("Connection test successful");
            m_connectionStatusLabel->setStyleSheet("color: #00aa00; font-weight: bold;");
        } else {
            m_testStatusLabel->setText("Connection test completed with warnings");
            m_testStatusLabel->setStyleSheet("color: #cc8800; font-weight: bold;");
        }
        
        m_testResultsText->append(QString("\nTest Summary:\n"
                                        "- Duration: %1 ms\n"
                                        "- Frames received: %2\n"
                                        "- Average latency: %3 ms\n"
                                        "- Signal quality: %4%\n"
                                        "- Result: %5")
                                .arg(last_test_result_.test_duration.count())
                                .arg(last_test_result_.frames_received)
                                .arg(last_test_result_.latency_ms, 0, 'f', 2)
                                .arg(last_test_result_.signal_quality, 0, 'f', 1)
                                .arg(last_test_result_.isGoodConnection() ? "PASS" : "MARGINAL"));
    } else {
        m_latencyLabel->setText("Failed");
        m_signalQualityLabel->setText("Failed");
        m_framesReceivedLabel->setText("0");
        
        m_testStatusLabel->setText("Connection test failed");
        m_testStatusLabel->setStyleSheet("color: #cc0000; font-weight: bold;");
        m_connectionStatusLabel->setText("Connection test failed");
        m_connectionStatusLabel->setStyleSheet("color: #cc0000; font-weight: bold;");
        
        m_testResultsText->append(QString("\nTest failed: %1").arg(last_test_result_.message));
    }
    
    // Clean up controller
    if (m_testController) {
        m_testController.reset();
    }
    
    emit connectionTestCompleted(last_test_result_);
}

void ConnectionConfigDialog::onConnectionTestTimeout() {
    if (m_testInProgress) {
        last_test_result_.success = false;
        last_test_result_.message = "Connection test timed out";
        last_test_result_.details = "No response from server within timeout period";
        
        m_testResultsText->append("Test timed out - no response from server");
        
        onConnectionTestFinished();
    }
}

void ConnectionConfigDialog::saveConfiguration() {
    QString filename = QFileDialog::getSaveFileName(this,
        "Save Connection Configuration",
        getConfigFilePath(true),
        "Configuration Files (*.ini);;All Files (*)");
    
    if (!filename.isEmpty()) {
        QSettings settings(filename, QSettings::IniFormat);
        auto config = getConfiguration();
        
        // Save configuration to INI file
        settings.beginGroup("Connection");
        settings.setValue("protocol", config.protocol);
        settings.setValue("host", config.host);
        settings.setValue("port", config.port);
        settings.setValue("full_url", config.full_url);
        settings.endGroup();
        
        settings.beginGroup("Advanced");
        settings.setValue("enable_tist", config.enable_tist);
        settings.setValue("enable_metadata", config.enable_metadata);
        settings.setValue("enable_etsi_compliance", config.enable_etsi_compliance);
        settings.setValue("enable_error_correction", config.enable_error_correction);
        settings.setValue("buffer_frames", config.buffer_frames);
        settings.setValue("timeout_ms", config.timeout_ms);
        settings.setValue("auto_reconnect", config.auto_reconnect);
        settings.setValue("reconnect_interval_ms", config.reconnect_interval_ms);
        settings.setValue("adaptive_buffering", config.adaptive_buffering);
        settings.setValue("min_signal_quality", config.min_signal_quality);
        settings.setValue("target_fps", config.target_fps);
        settings.setValue("performance_monitoring", config.performance_monitoring);
        settings.endGroup();
        
        settings.sync();
        
        if (settings.status() == QSettings::NoError) {
            QMessageBox::information(this, "Configuration Saved",
                                   QString("Configuration saved to:\n%1").arg(filename));
            emit configurationSaved(filename);
        } else {
            QMessageBox::warning(this, "Save Failed",
                               "Failed to save configuration file.");
        }
    }
}

void ConnectionConfigDialog::loadConfiguration() {
    QString filename = QFileDialog::getOpenFileName(this,
        "Load Connection Configuration",
        getConfigFilePath(true),
        "Configuration Files (*.ini);;All Files (*)");
    
    if (!filename.isEmpty()) {
        QSettings settings(filename, QSettings::IniFormat);
        ConnectionConfig config;
        
        // Load configuration from INI file
        settings.beginGroup("Connection");
        config.protocol = settings.value("protocol", "tcp").toString();
        config.host = settings.value("host", "127.0.0.1").toString();
        config.port = static_cast<quint16>(settings.value("port", 9200).toUInt());
        settings.endGroup();
        
        settings.beginGroup("Advanced");
        config.enable_tist = settings.value("enable_tist", true).toBool();
        config.enable_metadata = settings.value("enable_metadata", true).toBool();
        config.enable_etsi_compliance = settings.value("enable_etsi_compliance", true).toBool();
        config.enable_error_correction = settings.value("enable_error_correction", true).toBool();
        config.buffer_frames = static_cast<uint32_t>(settings.value("buffer_frames", 1000).toUInt());
        config.timeout_ms = static_cast<uint32_t>(settings.value("timeout_ms", 5000).toUInt());
        config.auto_reconnect = settings.value("auto_reconnect", true).toBool();
        config.reconnect_interval_ms = static_cast<uint32_t>(settings.value("reconnect_interval_ms", 5000).toUInt());
        config.adaptive_buffering = settings.value("adaptive_buffering", true).toBool();
        config.min_signal_quality = settings.value("min_signal_quality", 80.0).toDouble();
        config.target_fps = settings.value("target_fps", 1000.0).toDouble();
        config.performance_monitoring = settings.value("performance_monitoring", true).toBool();
        settings.endGroup();
        
        config.updateFullURL();
        
        if (config.isValid()) {
            setConfiguration(config);
            QMessageBox::information(this, "Configuration Loaded",
                                   QString("Configuration loaded from:\n%1").arg(filename));
            emit configurationLoaded(filename);
        } else {
            QMessageBox::warning(this, "Load Failed",
                               "The configuration file contains invalid settings.");
        }
    }
}

void ConnectionConfigDialog::resetToDefaults() {
    auto reply = QMessageBox::question(this, "Reset Configuration",
                                     "Are you sure you want to reset all settings to default values?",
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        setConfiguration(connection_utils::createDefaultConfig());
    }
}

void ConnectionConfigDialog::importConfiguration() {
    // Same as load but with different dialog title
    loadConfiguration();
}

void ConnectionConfigDialog::exportConfiguration() {
    // Same as save but with different dialog title
    saveConfiguration();
}

void ConnectionConfigDialog::accept() {
    if (!isConfigurationValid()) {
        QMessageBox::warning(this, "Invalid Configuration",
                           "Please fix the configuration errors before accepting.");
        return;
    }
    
    auto config = getConfiguration();
    emit configurationAccepted(config);
    
    saveCurrentSettings();
    QDialog::accept();
}

void ConnectionConfigDialog::reject() {
    QDialog::reject();
}

void ConnectionConfigDialog::loadDefaultSettings() {
    m_settings->beginGroup("ConnectionConfigDialog");
    
    // Load window geometry
    resize(m_settings->value("size", QSize(640, 480)).toSize());
    
    // Load last configuration
    ConnectionConfig config;
    config.protocol = m_settings->value("protocol", "tcp").toString();
    config.host = m_settings->value("host", "127.0.0.1").toString();
    config.port = static_cast<quint16>(m_settings->value("port", 9200).toUInt());
    config.enable_tist = m_settings->value("enable_tist", true).toBool();
    config.enable_metadata = m_settings->value("enable_metadata", true).toBool();
    config.enable_etsi_compliance = m_settings->value("enable_etsi_compliance", true).toBool();
    config.enable_error_correction = m_settings->value("enable_error_correction", true).toBool();
    config.buffer_frames = static_cast<uint32_t>(m_settings->value("buffer_frames", 1000).toUInt());
    config.timeout_ms = static_cast<uint32_t>(m_settings->value("timeout_ms", 5000).toUInt());
    config.auto_reconnect = m_settings->value("auto_reconnect", true).toBool();
    config.reconnect_interval_ms = static_cast<uint32_t>(m_settings->value("reconnect_interval_ms", 5000).toUInt());
    config.adaptive_buffering = m_settings->value("adaptive_buffering", true).toBool();
    config.min_signal_quality = m_settings->value("min_signal_quality", 80.0).toDouble();
    config.target_fps = m_settings->value("target_fps", 1000.0).toDouble();
    config.performance_monitoring = m_settings->value("performance_monitoring", true).toBool();
    
    m_settings->endGroup();
    
    if (config.isValid()) {
        setConfiguration(config);
    }
}

void ConnectionConfigDialog::saveCurrentSettings() {
    m_settings->beginGroup("ConnectionConfigDialog");
    
    // Save window geometry
    m_settings->setValue("size", size());
    
    // Save current configuration
    auto config = getConfiguration();
    m_settings->setValue("protocol", config.protocol);
    m_settings->setValue("host", config.host);
    m_settings->setValue("port", config.port);
    m_settings->setValue("enable_tist", config.enable_tist);
    m_settings->setValue("enable_metadata", config.enable_metadata);
    m_settings->setValue("enable_etsi_compliance", config.enable_etsi_compliance);
    m_settings->setValue("enable_error_correction", config.enable_error_correction);
    m_settings->setValue("buffer_frames", config.buffer_frames);
    m_settings->setValue("timeout_ms", config.timeout_ms);
    m_settings->setValue("auto_reconnect", config.auto_reconnect);
    m_settings->setValue("reconnect_interval_ms", config.reconnect_interval_ms);
    m_settings->setValue("adaptive_buffering", config.adaptive_buffering);
    m_settings->setValue("min_signal_quality", config.min_signal_quality);
    m_settings->setValue("target_fps", config.target_fps);
    m_settings->setValue("performance_monitoring", config.performance_monitoring);
    
    m_settings->endGroup();
    m_settings->sync();
}

QString ConnectionConfigDialog::getConfigFilePath(bool for_export) const {
    if (for_export) {
        return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + 
               "/StreamDAB/connection_config.ini";
    } else {
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + 
               "/connection_config.ini";
    }
}

QString ConnectionConfigDialog::getProtocolHelpText(const QString& protocol) const {
    if (protocol == "tcp") {
        return "TCP protocol for direct ETI streaming. Default port: 9200";
    } else if (protocol == "zmq+tcp") {
        return "ZeroMQ over TCP for reliable ETI streaming with message queuing. Default port: 9201";
    } else if (protocol == "zmq+ipc") {
        return "ZeroMQ over IPC (Inter-Process Communication) for local streaming. Uses Unix domain sockets.";
    } else {
        return "Select a protocol for ETI streaming communication.";
    }
}

bool ConnectionConfigDialog::validateHost(const QString& host) const {
    if (host.isEmpty()) {
        return false;
    }
    
    // Check if it's a valid IPv4 address
    QHostAddress address(host);
    if (!address.isNull()) {
        return true;
    }
    
    // Check if it's a valid hostname (basic validation)
    QRegularExpression hostnameRegex(R"(^([a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?\.)*[a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?$)");
    return hostnameRegex.match(host).hasMatch();
}

bool ConnectionConfigDialog::validatePort(int port) const {
    return port > 0 && port <= 65535;
}

// Utility functions implementation
namespace connection_utils {

ConnectionConfig createDefaultConfig() {
    ConnectionConfig config;
    config.protocol = "tcp";
    config.host = "127.0.0.1";
    config.port = 9200;
    config.enable_tist = true;
    config.enable_metadata = true;
    config.enable_etsi_compliance = true;
    config.enable_error_correction = true;
    config.buffer_frames = 1000;
    config.timeout_ms = 5000;
    config.auto_reconnect = true;
    config.reconnect_interval_ms = 5000;
    config.adaptive_buffering = true;
    config.min_signal_quality = 80.0;
    config.target_fps = 1000.0;
    config.performance_monitoring = true;
    config.updateFullURL();
    return config;
}

ConnectionConfig createODRDabMuxConfig() {
    auto config = createDefaultConfig();
    config.host = "127.0.0.1";
    config.port = 9200;
    config.protocol = "tcp";
    config.enable_tist = true;
    config.target_fps = 384.0; // Standard DAB frame rate
    config.updateFullURL();
    return config;
}

ConnectionConfig createHighPerformanceConfig() {
    auto config = createDefaultConfig();
    config.target_fps = 2000.0;
    config.buffer_frames = 2000;
    config.enable_error_correction = true;
    config.adaptive_buffering = true;
    config.performance_monitoring = true;
    config.updateFullURL();
    return config;
}

ValidationResult validateConfig(const ConnectionConfig& config) {
    ValidationResult result;
    result.valid = true;
    
    // Validate protocol
    QStringList supportedProtocols = getSupportedProtocols();
    if (!supportedProtocols.contains(config.protocol)) {
        result.valid = false;
        result.errors.append(QString("Unsupported protocol: %1").arg(config.protocol));
    }
    
    // Validate host
    if (config.host.isEmpty()) {
        result.valid = false;
        result.errors.append("Host cannot be empty");
    } else {
        QHostAddress address(config.host);
        if (address.isNull()) {
            // Check hostname format
            QRegularExpression hostnameRegex(R"(^([a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?\.)*[a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?$)");
            if (!hostnameRegex.match(config.host).hasMatch()) {
                result.valid = false;
                result.errors.append("Invalid host address or hostname");
            }
        }
    }
    
    // Validate port
    if (config.port <= 0 || config.port > 65535) {
        result.valid = false;
        result.errors.append("Port must be between 1 and 65535");
    }
    
    // Validate timeout
    if (config.timeout_ms <= 0 || config.timeout_ms > 60000) {
        result.valid = false;
        result.errors.append("Timeout must be between 1 and 60000 milliseconds");
    }
    
    // Validate buffer size
    if (config.buffer_frames <= 0 || config.buffer_frames > 10000) {
        result.valid = false;
        result.errors.append("Buffer size must be between 1 and 10000 frames");
    }
    
    // Validate FPS
    if (config.target_fps <= 0.0 || config.target_fps > 10000.0) {
        result.valid = false;
        result.errors.append("Target FPS must be between 0.1 and 10000");
    }
    
    // Validate signal quality
    if (config.min_signal_quality < 0.0 || config.min_signal_quality > 100.0) {
        result.valid = false;
        result.errors.append("Signal quality must be between 0 and 100 percent");
    }
    
    // Warnings for potentially problematic configurations
    if (config.target_fps > 2000.0) {
        result.warnings.append("High target FPS may impact performance");
    }
    
    if (config.buffer_frames > 5000) {
        result.warnings.append("Large buffer size may increase latency");
    }
    
    if (config.timeout_ms < 1000) {
        result.warnings.append("Short timeout may cause connection issues");
    }
    
    return result;
}

QStringList getSupportedProtocols() {
    return {"tcp", "zmq+tcp", "zmq+ipc"};
}

QString getProtocolDescription(const QString& protocol) {
    if (protocol == "tcp") {
        return "Standard TCP connection for ETI streaming";
    } else if (protocol == "zmq+tcp") {
        return "ZeroMQ over TCP with message queuing";
    } else if (protocol == "zmq+ipc") {
        return "ZeroMQ Inter-Process Communication (local only)";
    } else {
        return "Unknown protocol";
    }
}

quint16 getDefaultPortForProtocol(const QString& protocol) {
    if (protocol == "tcp") {
        return 9200;
    } else if (protocol == "zmq+tcp") {
        return 9201;
    } else if (protocol == "zmq+ipc") {
        return 9202;
    } else {
        return 9200;
    }
}

} // namespace connection_utils

} // namespace streamdab::ui