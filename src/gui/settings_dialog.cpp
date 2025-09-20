/**
 * @file settings_dialog.cpp
 * @brief Professional settings configuration dialog implementation
 * 
 * This file implements the SettingsDialog class providing comprehensive
 * configuration options for the ETI Stream Analyzer application following
 * professional broadcast industry standards.
 */

#include "settings_dialog.h"
#include "utils/logger.h"

#include <QApplication>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QScreen>
#include <QDebug>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
    , m_backgroundColor(20, 20, 30)
    , m_gridColor(60, 60, 80)
    , m_textColor(200, 200, 220)
    , m_displayFont("Arial", 9)
    , m_settingsLoaded(false)
    , m_settingsChanged(false)
    , m_settings(nullptr)
{
    // Initialize UI from Designer file
    ui->setupUi(this);
    
    // Set dialog properties
    setObjectName("SettingsDialog");
    setWindowTitle(tr("StreamDAB Stream Analyzer - Settings"));
    setWindowIcon(QIcon(":/icons/settings.png"));
    setModal(true);
    setMinimumSize(600, 450);
    resize(700, 550);
    
    // Center dialog on parent
    if (parent) {
        QRect parentGeometry = parent->geometry();
        int x = parentGeometry.x() + (parentGeometry.width() - width()) / 2;
        int y = parentGeometry.y() + (parentGeometry.height() - height()) / 2;
        move(x, y);
    }
    
    // Initialize settings
    m_settings = new QSettings(this);
    
    Logger::instance().log(Logger::Info, "SettingsDialog", "Constructor completed");
}

SettingsDialog::~SettingsDialog()
{
    Logger::instance().log(Logger::Info, "SettingsDialog", "Destructor starting");
    
    // Clean up UI
    delete ui;
    
    Logger::instance().log(Logger::Info, "SettingsDialog", "Destructor completed");
}

bool SettingsDialog::initialize()
{
    Logger::instance().log(Logger::Info, "SettingsDialog", "Initialization starting");
    
    try {
        // Connect UI signals and slots (already set up by Qt Designer connections)
        // Connect button signals
        connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
        connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
        
        // Connect browse buttons
        connect(ui->browseDirectoryButton, &QPushButton::clicked, this, &SettingsDialog::handleBrowseFilePath);
        connect(ui->browseAudioPathButton, &QPushButton::clicked, this, &SettingsDialog::handleBrowseFilePath);
        connect(ui->browseLogPathButton, &QPushButton::clicked, this, &SettingsDialog::handleBrowseFilePath);
        
        // Connect color buttons
        connect(ui->backgroundColorButton, &QPushButton::clicked, this, &SettingsDialog::handleColorSelection);
        connect(ui->gridColorButton, &QPushButton::clicked, this, &SettingsDialog::handleColorSelection);
        connect(ui->textColorButton, &QPushButton::clicked, this, &SettingsDialog::handleColorSelection);
        connect(ui->fontButton, &QPushButton::clicked, this, &SettingsDialog::handleFontSelection);
        // Connect UI signals and slots (Qt Designer connections are automatic)
        // Connect button signals from the UI buttonBox
        connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
        connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
        
        // Connect browse buttons
        connect(ui->browseDirectoryButton, &QPushButton::clicked, this, &SettingsDialog::handleBrowseFilePath);
        connect(ui->browseAudioPathButton, &QPushButton::clicked, this, &SettingsDialog::handleBrowseFilePath);
        connect(ui->browseLogPathButton, &QPushButton::clicked, this, &SettingsDialog::handleBrowseFilePath);
        
        // Connect color buttons
        connect(ui->backgroundColorButton, &QPushButton::clicked, this, &SettingsDialog::handleColorSelection);
        connect(ui->gridColorButton, &QPushButton::clicked, this, &SettingsDialog::handleColorSelection);
        connect(ui->textColorButton, &QPushButton::clicked, this, &SettingsDialog::handleColorSelection);
        connect(ui->fontButton, &QPushButton::clicked, this, &SettingsDialog::handleFontSelection);
        
        // Initialize combo boxes with values
        initializeComboBoxes();
        
        // Load current settings
        loadSettings();
        
        // Update UI with loaded settings
        updateUI();
        
        Logger::instance().log(Logger::Info, "SettingsDialog", "Initialization completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "SettingsDialog", 
                             QString("Initialization failed: %1").arg(e.what()));
        return false;
    }
}

void SettingsDialog::initializeComboBoxes()
{
    // Initialize language combo
    ui->languageCombo->addItem(tr("English"), "en");
    ui->languageCombo->addItem(tr("German"), "de");
    ui->languageCombo->addItem(tr("French"), "fr");
    
    // Initialize theme combo
    ui->themeCombo->addItem(tr("Dark"), "dark");
    ui->themeCombo->addItem(tr("Light"), "light");
    ui->themeCombo->addItem(tr("System"), "system");
    
    // Initialize processing mode combo
    ui->processingModeCombo->addItem(tr("Real-time"), "realtime");
    ui->processingModeCombo->addItem(tr("Batch"), "batch");
    ui->processingModeCombo->addItem(tr("Analysis"), "analysis");
    
    // Initialize color scheme combo
    ui->colorSchemeCombo->addItem(tr("Professional Dark"), "dark");
    ui->colorSchemeCombo->addItem(tr("Professional Light"), "light");
    ui->colorSchemeCombo->addItem(tr("High Contrast"), "contrast");
    
    // Initialize audio sample rate combo
    ui->audioSampleRateCombo->addItem("44100 Hz", 44100);
    ui->audioSampleRateCombo->addItem("48000 Hz", 48000);
    ui->audioSampleRateCombo->addItem("96000 Hz", 96000);
    
    // Initialize log level combo
    ui->logLevelCombo->addItem(tr("Error"), "error");
    ui->logLevelCombo->addItem(tr("Warning"), "warning");
    ui->logLevelCombo->addItem(tr("Info"), "info");
    ui->logLevelCombo->addItem(tr("Debug"), "debug");
}

void SettingsDialog::loadSettings()
{
    Logger::instance().log(Logger::Debug, "SettingsDialog", "Loading settings from configuration");
    
    if (!m_settings) {
        Logger::instance().log(Logger::Warning, "SettingsDialog", "Settings object not initialized");
        return;
    }
    
    try {
        // Load general settings
        ui->autoStartAnalysis->setChecked(m_settings->value("general/autoStartAnalysis", true).toBool());
        ui->rememberWindowState->setChecked(m_settings->value("general/rememberWindowState", true).toBool());
        ui->showSplashScreen->setChecked(m_settings->value("general/showSplashScreen", true).toBool());
        ui->checkForUpdates->setChecked(m_settings->value("general/checkForUpdates", true).toBool());
        
        // Load analysis settings
        ui->updateRateSpin->setValue(m_settings->value("analysis/updateRate", DEFAULT_UPDATE_RATE).toInt());
        ui->bufferSizeSpin->setValue(m_settings->value("analysis/bufferSize", DEFAULT_BUFFER_SIZE).toInt());
        ui->realTimeProcessingCheck->setChecked(m_settings->value("analysis/realTimeProcessing", true).toBool());
        ui->enableErrorDetection->setChecked(m_settings->value("analysis/errorDetection", true).toBool());
        ui->enableFigAnalysis->setChecked(m_settings->value("analysis/figAnalysis", true).toBool());
        ui->enableAudioAnalysis->setChecked(m_settings->value("analysis/audioAnalysis", true).toBool());
        
        // Load display settings
        ui->frameRateSpin->setValue(m_settings->value("display/frameRate", 60).toInt());
        ui->enableAntialiasing->setChecked(m_settings->value("display/antialiasing", true).toBool());
        ui->enableVSync->setChecked(m_settings->value("display/vsync", true).toBool());
        ui->showGridLines->setChecked(m_settings->value("display/gridLines", true).toBool());
        ui->showStatusBar->setChecked(m_settings->value("display/statusBar", true).toBool());
        
        // Load audio settings
        ui->enableAudioMonitoring->setChecked(m_settings->value("audio/monitoring", false).toBool());
        ui->audioVolumeSlider->setValue(m_settings->value("audio/volume", 50).toInt());
        ui->audioBufferSizeSpin->setValue(m_settings->value("audio/bufferSize", 1024).toInt());
        ui->enableAudioRecording->setChecked(m_settings->value("audio/recording", false).toBool());
        
        // Load network settings
        ui->enableNetworkStreaming->setChecked(m_settings->value("network/streaming", false).toBool());
        ui->networkAddressEdit->setText(m_settings->value("network/address", "localhost").toString());
        ui->networkPortSpin->setValue(m_settings->value("network/port", 8080).toInt());
        ui->networkTimeoutSpin->setValue(m_settings->value("network/timeout", 5000).toInt());
        ui->maxConnectionsSpin->setValue(m_settings->value("network/maxConnections", 10).toInt());
        ui->enableMulticast->setChecked(m_settings->value("network/multicast", false).toBool());
        ui->multicastAddressEdit->setText(m_settings->value("network/multicastAddress", "239.255.0.1").toString());
        ui->multicastPortSpin->setValue(m_settings->value("network/multicastPort", 1234).toInt());
        
        // Load performance settings
        ui->memoryLimitSpin->setValue(m_settings->value("performance/memoryLimit", DEFAULT_MEMORY_LIMIT).toInt());
        ui->threadCountSpin->setValue(m_settings->value("performance/threadCount", 4).toInt());
        ui->enableHardwareAcceleration->setChecked(m_settings->value("performance/hardwareAcceleration", true).toBool());
        ui->enableGpuAcceleration->setChecked(m_settings->value("performance/gpuAcceleration", false).toBool());
        ui->enableMultiThreading->setChecked(m_settings->value("performance/multiThreading", true).toBool());
        ui->processingPrioritySlider->setValue(m_settings->value("performance/priority", 3).toInt());
        ui->enableMemoryOptimization->setChecked(m_settings->value("performance/memoryOptimization", true).toBool());
        ui->enableCpuOptimization->setChecked(m_settings->value("performance/cpuOptimization", true).toBool());
        
        // Load advanced settings
        ui->enableDebugLogging->setChecked(m_settings->value("advanced/debugLogging", false).toBool());
        
        m_settingsLoaded = true;
        Logger::instance().log(Logger::Debug, "SettingsDialog", "Settings loaded successfully");
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "SettingsDialog", 
                             QString("Failed to load settings: %1").arg(e.what()));
    }
}

void SettingsDialog::saveSettings()
{
    Logger::instance().log(Logger::Debug, "SettingsDialog", "Saving settings to configuration");
    
    if (!m_settings) {
        Logger::instance().log(Logger::Warning, "SettingsDialog", "Settings object not initialized");
        return;
    }
    
    try {
        // Save general settings
        m_settings->setValue("general/autoStartAnalysis", ui->autoStartAnalysis->isChecked());
        m_settings->setValue("general/rememberWindowState", ui->rememberWindowState->isChecked());
        m_settings->setValue("general/showSplashScreen", ui->showSplashScreen->isChecked());
        m_settings->setValue("general/checkForUpdates", ui->checkForUpdates->isChecked());
        
        // Save analysis settings
        m_settings->setValue("analysis/updateRate", ui->updateRateSpin->value());
        m_settings->setValue("analysis/bufferSize", ui->bufferSizeSpin->value());
        m_settings->setValue("analysis/realTimeProcessing", ui->realTimeProcessingCheck->isChecked());
        m_settings->setValue("analysis/errorDetection", ui->enableErrorDetection->isChecked());
        m_settings->setValue("analysis/figAnalysis", ui->enableFigAnalysis->isChecked());
        m_settings->setValue("analysis/audioAnalysis", ui->enableAudioAnalysis->isChecked());
        
        // Save display settings
        m_settings->setValue("display/frameRate", ui->frameRateSpin->value());
        m_settings->setValue("display/antialiasing", ui->enableAntialiasing->isChecked());
        m_settings->setValue("display/vsync", ui->enableVSync->isChecked());
        m_settings->setValue("display/gridLines", ui->showGridLines->isChecked());
        m_settings->setValue("display/statusBar", ui->showStatusBar->isChecked());
        
        // Save audio settings
        m_settings->setValue("audio/monitoring", ui->enableAudioMonitoring->isChecked());
        m_settings->setValue("audio/volume", ui->audioVolumeSlider->value());
        m_settings->setValue("audio/bufferSize", ui->audioBufferSizeSpin->value());
        m_settings->setValue("audio/recording", ui->enableAudioRecording->isChecked());
        
        // Save network settings
        m_settings->setValue("network/streaming", ui->enableNetworkStreaming->isChecked());
        m_settings->setValue("network/address", ui->networkAddressEdit->text());
        m_settings->setValue("network/port", ui->networkPortSpin->value());
        m_settings->setValue("network/timeout", ui->networkTimeoutSpin->value());
        m_settings->setValue("network/maxConnections", ui->maxConnectionsSpin->value());
        m_settings->setValue("network/multicast", ui->enableMulticast->isChecked());
        m_settings->setValue("network/multicastAddress", ui->multicastAddressEdit->text());
        m_settings->setValue("network/multicastPort", ui->multicastPortSpin->value());
        
        // Save performance settings
        m_settings->setValue("performance/memoryLimit", ui->memoryLimitSpin->value());
        m_settings->setValue("performance/threadCount", ui->threadCountSpin->value());
        m_settings->setValue("performance/hardwareAcceleration", ui->enableHardwareAcceleration->isChecked());
        m_settings->setValue("performance/gpuAcceleration", ui->enableGpuAcceleration->isChecked());
        m_settings->setValue("performance/multiThreading", ui->enableMultiThreading->isChecked());
        m_settings->setValue("performance/priority", ui->processingPrioritySlider->value());
        m_settings->setValue("performance/memoryOptimization", ui->enableMemoryOptimization->isChecked());
        m_settings->setValue("performance/cpuOptimization", ui->enableCpuOptimization->isChecked());
        
        // Save advanced settings
        m_settings->setValue("advanced/debugLogging", ui->enableDebugLogging->isChecked());
        
        m_settings->sync();
        
        Logger::instance().log(Logger::Debug, "SettingsDialog", "Settings saved successfully");
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "SettingsDialog", 
                             QString("Failed to save settings: %1").arg(e.what()));
    }
}

void SettingsDialog::resetToDefaults()
{
    Logger::instance().log(Logger::Info, "SettingsDialog", "Resetting settings to defaults");
    
    // Reset all UI controls to default values
    ui->autoStartAnalysis->setChecked(true);
    ui->rememberWindowState->setChecked(true);
    ui->showSplashScreen->setChecked(true);
    ui->checkForUpdates->setChecked(true);
    
    ui->updateRateSpin->setValue(DEFAULT_UPDATE_RATE);
    ui->bufferSizeSpin->setValue(DEFAULT_BUFFER_SIZE);
    ui->realTimeProcessingCheck->setChecked(true);
    ui->enableErrorDetection->setChecked(true);
    ui->enableFigAnalysis->setChecked(true);
    ui->enableAudioAnalysis->setChecked(true);
    
    ui->frameRateSpin->setValue(60);
    ui->enableAntialiasing->setChecked(true);
    ui->enableVSync->setChecked(true);
    ui->showGridLines->setChecked(true);
    ui->showStatusBar->setChecked(true);
    
    ui->enableAudioMonitoring->setChecked(false);
    ui->audioVolumeSlider->setValue(50);
    ui->audioBufferSizeSpin->setValue(1024);
    ui->enableAudioRecording->setChecked(false);
    
    ui->enableNetworkStreaming->setChecked(false);
    ui->networkAddressEdit->setText("localhost");
    ui->networkPortSpin->setValue(8080);
    ui->networkTimeoutSpin->setValue(5000);
    ui->maxConnectionsSpin->setValue(10);
    ui->enableMulticast->setChecked(false);
    ui->multicastAddressEdit->setText("239.255.0.1");
    ui->multicastPortSpin->setValue(1234);
    
    ui->memoryLimitSpin->setValue(DEFAULT_MEMORY_LIMIT);
    ui->threadCountSpin->setValue(4);
    ui->enableHardwareAcceleration->setChecked(true);
    ui->enableGpuAcceleration->setChecked(false);
    ui->enableMultiThreading->setChecked(true);
    ui->processingPrioritySlider->setValue(3);
    ui->enableMemoryOptimization->setChecked(true);
    ui->enableCpuOptimization->setChecked(true);
    
    ui->enableDebugLogging->setChecked(false);
    
    m_settingsChanged = true;
}

void SettingsDialog::updateUI()
{
    // Update color button appearance
    updateColorButtons();
    
    // Update font button text
    ui->fontButton->setText(QString("%1, %2pt").arg(m_displayFont.family()).arg(m_displayFont.pointSize()));
}

void SettingsDialog::updateColorButtons()
{
    // Update background color button
    ui->backgroundColorButton->setStyleSheet(QString("background-color: %1").arg(m_backgroundColor.name()));
    
    // Update grid color button
    ui->gridColorButton->setStyleSheet(QString("background-color: %1").arg(m_gridColor.name()));
    
    // Update text color button
    ui->textColorButton->setStyleSheet(QString("background-color: %1").arg(m_textColor.name()));
}

// Getter methods for settings access
int SettingsDialog::getUpdateRate() const
{
    return ui ? ui->updateRateSpin->value() : DEFAULT_UPDATE_RATE;
}

int SettingsDialog::getBufferSize() const
{
    return ui ? ui->bufferSizeSpin->value() : DEFAULT_BUFFER_SIZE;
}

int SettingsDialog::getMemoryLimit() const
{
    return ui ? ui->memoryLimitSpin->value() : DEFAULT_MEMORY_LIMIT;
}

bool SettingsDialog::isRealTimeProcessingEnabled() const
{
    return ui ? ui->realTimeProcessingCheck->isChecked() : true;
}

bool SettingsDialog::isHardwareAccelerationEnabled() const
{
    return ui ? ui->enableHardwareAcceleration->isChecked() : true;
}

// Dialog event handlers
void SettingsDialog::accept()
{
    Logger::instance().log(Logger::Info, "SettingsDialog", "Settings accepted by user");
    
    if (validateSettings()) {
        saveSettings();
        emit settingsChanged();
        QDialog::accept();
    }
}

void SettingsDialog::reject()
{
    Logger::instance().log(Logger::Info, "SettingsDialog", "Settings dialog canceled by user");
    QDialog::reject();
}

bool SettingsDialog::validateSettings()
{
    // Validate update rate
    if (ui->updateRateSpin->value() < MIN_UPDATE_RATE || ui->updateRateSpin->value() > MAX_UPDATE_RATE) {
        QMessageBox::warning(this, tr("Invalid Settings"), 
                           tr("Update rate must be between %1 and %2 FPS").arg(MIN_UPDATE_RATE).arg(MAX_UPDATE_RATE));
        return false;
    }
    
    // Validate buffer size
    if (ui->bufferSizeSpin->value() < MIN_BUFFER_SIZE || ui->bufferSizeSpin->value() > MAX_BUFFER_SIZE) {
        QMessageBox::warning(this, tr("Invalid Settings"), 
                           tr("Buffer size must be between %1 and %2 frames").arg(MIN_BUFFER_SIZE).arg(MAX_BUFFER_SIZE));
        return false;
    }
    
    // Validate memory limit
    if (ui->memoryLimitSpin->value() < MIN_MEMORY_LIMIT || ui->memoryLimitSpin->value() > MAX_MEMORY_LIMIT) {
        QMessageBox::warning(this, tr("Invalid Settings"), 
                           tr("Memory limit must be between %1 and %2 MB").arg(MIN_MEMORY_LIMIT).arg(MAX_MEMORY_LIMIT));
        return false;
    }
    
    return true;
}

// Slot implementations
void SettingsDialog::handleApply()
{
    Logger::instance().log(Logger::Debug, "SettingsDialog", "Apply button clicked");
    
    if (validateSettings()) {
        saveSettings();
        emit settingsChanged();
        m_settingsChanged = false;
    }
}

void SettingsDialog::handleReset()
{
    Logger::instance().log(Logger::Debug, "SettingsDialog", "Reset button clicked");
    
    int result = QMessageBox::question(this, tr("Reset Settings"), 
                                     tr("Are you sure you want to reset all settings to current values?"),
                                     QMessageBox::Yes | QMessageBox::No, 
                                     QMessageBox::No);
    
    if (result == QMessageBox::Yes) {
        loadSettings();
        updateUI();
    }
}

void SettingsDialog::handleDefaults()
{
    Logger::instance().log(Logger::Debug, "SettingsDialog", "Defaults button clicked");
    
    int result = QMessageBox::question(this, tr("Default Settings"), 
                                     tr("Are you sure you want to reset all settings to default values?"),
                                     QMessageBox::Yes | QMessageBox::No, 
                                     QMessageBox::No);
    
    if (result == QMessageBox::Yes) {
        resetToDefaults();
        updateUI();
    }
}

void SettingsDialog::handleBrowseFilePath()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Directory"), 
                                                   QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    
    if (!dir.isEmpty()) {
        if (button == ui->browseDirectoryButton) {
            ui->defaultDirectoryEdit->setText(dir);
        } else if (button == ui->browseAudioPathButton) {
            ui->audioRecordingPathEdit->setText(dir);
        } else if (button == ui->browseLogPathButton) {
            ui->logFilePathEdit->setText(dir);
        }
    }
}

void SettingsDialog::handleColorSelection()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    QColor currentColor;
    QColor *targetColor = nullptr;
    
    if (button == ui->backgroundColorButton) {
        currentColor = m_backgroundColor;
        targetColor = &m_backgroundColor;
    } else if (button == ui->gridColorButton) {
        currentColor = m_gridColor;
        targetColor = &m_gridColor;
    } else if (button == ui->textColorButton) {
        currentColor = m_textColor;
        targetColor = &m_textColor;
    }
    
    if (targetColor) {
        QColor color = QColorDialog::getColor(currentColor, this, tr("Select Color"));
        if (color.isValid()) {
            *targetColor = color;
            updateColorButtons();
        }
    }
}

void SettingsDialog::handleFontSelection()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_displayFont, this, tr("Select Font"));
    
    if (ok) {
        m_displayFont = font;
        ui->fontButton->setText(QString("%1, %2pt").arg(font.family()).arg(font.pointSize()));
    }
}

void SettingsDialog::handleSettingsValidation()
{
    m_settingsChanged = true;
}

QVariantMap SettingsDialog::getDefaultSettings() const
{
    QVariantMap defaults;
    
    // General defaults
    defaults["general/autoStartAnalysis"] = true;
    defaults["general/rememberWindowState"] = true;
    defaults["general/showSplashScreen"] = true;
    defaults["general/checkForUpdates"] = true;
    
    // Analysis defaults
    defaults["analysis/updateRate"] = DEFAULT_UPDATE_RATE;
    defaults["analysis/bufferSize"] = DEFAULT_BUFFER_SIZE;
    defaults["analysis/realTimeProcessing"] = true;
    defaults["analysis/errorDetection"] = true;
    defaults["analysis/figAnalysis"] = true;
    defaults["analysis/audioAnalysis"] = true;
    
    // Performance defaults
    defaults["performance/memoryLimit"] = DEFAULT_MEMORY_LIMIT;
    defaults["performance/threadCount"] = 4;
    defaults["performance/hardwareAcceleration"] = true;
    defaults["performance/gpuAcceleration"] = false;
    defaults["performance/multiThreading"] = true;
    
    return defaults;
}