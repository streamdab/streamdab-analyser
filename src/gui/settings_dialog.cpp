/**
 * @file settings_dialog.cpp
 * @brief Professional settings configuration dialog implementation
 * 
 * This file implements the SettingsDialog class providing comprehensive
 * configuration options for the ETI Stream Analyzer application following
 * professional broadcast industry standards with manual widget creation.
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
#include <QFormLayout>
#include <QIcon>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_tabWidget(nullptr)
    , m_buttonBox(nullptr)
    , m_backgroundColor(20, 20, 30)
    , m_gridColor(60, 60, 80)
    , m_textColor(200, 200, 220)
    , m_displayFont("Arial", 9)
    , m_settingsLoaded(false)
    , m_settingsChanged(false)
    , m_settings(nullptr)
{
    // Create UI manually
    createUI();
    
    // Initialize settings
    m_settings = new QSettings(this);
    
    Logger::instance().log(Logger::Info, "SettingsDialog", "Constructor completed");
}

SettingsDialog::~SettingsDialog()
{
    Logger::instance().log(Logger::Info, "SettingsDialog", "Destructor starting");
    
    // Qt handles widget cleanup automatically
    
    Logger::instance().log(Logger::Info, "SettingsDialog", "Destructor completed");
}

void SettingsDialog::createUI()
{
    // Create main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    
    // Create tabs
    createGeneralTab();
    createAnalysisTab();
    createDisplayTab();
    createAudioTab();
    createNetworkTab();
    createPerformanceTab();
    createAdvancedTab();
    
    // Create button box
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | 
        QDialogButtonBox::Apply | QDialogButtonBox::RestoreDefaults,
        this
    );
    
    // Add to main layout
    mainLayout->addWidget(m_tabWidget);
    mainLayout->addWidget(m_buttonBox);
    
    // Connect signals
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    connect(m_buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &SettingsDialog::handleApply);
    connect(m_buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked,
            this, &SettingsDialog::handleDefaults);
    
    // Set dialog properties
    setObjectName("SettingsDialog");
    setWindowTitle(tr("StreamDAB Stream Analyzer - Settings"));
    setWindowIcon(QIcon(":/icons/settings.png"));
    setModal(true);
    setMinimumSize(600, 450);
    resize(700, 550);
    
    // Center dialog on parent
    if (parentWidget()) {
        QRect parentGeometry = parentWidget()->geometry();
        int x = parentGeometry.x() + (parentGeometry.width() - width()) / 2;
        int y = parentGeometry.y() + (parentGeometry.height() - height()) / 2;
        move(x, y);
    }
}

void SettingsDialog::createGeneralTab()
{
    m_generalTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_generalTab);
    
    // Appearance group
    m_appearanceGroup = new QGroupBox(tr("Appearance"), m_generalTab);
    QFormLayout *appearanceLayout = new QFormLayout(m_appearanceGroup);
    
    m_themeCombo = new QComboBox();
    m_themeCombo->addItems({tr("Dark"), tr("Light"), tr("System")});
    appearanceLayout->addRow(tr("Theme:"), m_themeCombo);
    
    // Behavior group
    m_behaviorGroup = new QGroupBox(tr("Behavior"), m_generalTab);
    QFormLayout *behaviorLayout = new QFormLayout(m_behaviorGroup);
    
    m_autoSaveCheck = new QCheckBox(tr("Enable auto-save"));
    m_confirmExitCheck = new QCheckBox(tr("Confirm on exit"));
    m_autoSaveIntervalSpin = new QSpinBox();
    m_autoSaveIntervalSpin->setRange(30, 600);
    m_autoSaveIntervalSpin->setSuffix(tr(" seconds"));
    m_autoSaveIntervalSpin->setValue(60);
    
    behaviorLayout->addRow(m_autoSaveCheck);
    behaviorLayout->addRow(m_confirmExitCheck);
    behaviorLayout->addRow(tr("Auto-save interval:"), m_autoSaveIntervalSpin);
    
    layout->addWidget(m_appearanceGroup);
    layout->addWidget(m_behaviorGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_generalTab, tr("General"));
}

void SettingsDialog::createAnalysisTab()
{
    m_analysisTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_analysisTab);
    
    // Performance group
    m_performanceGroup = new QGroupBox(tr("Performance"), m_analysisTab);
    QFormLayout *performanceLayout = new QFormLayout(m_performanceGroup);
    
    m_updateRateSpin = new QSpinBox();
    m_updateRateSpin->setRange(MIN_UPDATE_RATE, MAX_UPDATE_RATE);
    m_updateRateSpin->setValue(DEFAULT_UPDATE_RATE);
    m_updateRateSpin->setSuffix(tr(" FPS"));
    
    m_bufferSizeSpin = new QSpinBox();
    m_bufferSizeSpin->setRange(MIN_BUFFER_SIZE, MAX_BUFFER_SIZE);
    m_bufferSizeSpin->setValue(DEFAULT_BUFFER_SIZE);
    m_bufferSizeSpin->setSuffix(tr(" frames"));
    
    m_realTimeCheck = new QCheckBox(tr("Enable real-time processing"));
    m_hardwareAccelCheck = new QCheckBox(tr("Enable hardware acceleration"));
    
    performanceLayout->addRow(tr("Update rate:"), m_updateRateSpin);
    performanceLayout->addRow(tr("Buffer size:"), m_bufferSizeSpin);
    performanceLayout->addRow(m_realTimeCheck);
    performanceLayout->addRow(m_hardwareAccelCheck);
    
    layout->addWidget(m_performanceGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_analysisTab, tr("Analysis"));
}

void SettingsDialog::createDisplayTab()
{
    m_displayTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_displayTab);
    
    // Colors group
    m_colorsGroup = new QGroupBox(tr("Colors"), m_displayTab);
    QFormLayout *colorsLayout = new QFormLayout(m_colorsGroup);
    
    m_backgroundColorButton = new QPushButton(tr("Background Color"));
    m_gridColorButton = new QPushButton(tr("Grid Color"));
    m_textColorButton = new QPushButton(tr("Text Color"));
    
    connect(m_backgroundColorButton, &QPushButton::clicked, this, &SettingsDialog::handleColorSelection);
    connect(m_gridColorButton, &QPushButton::clicked, this, &SettingsDialog::handleColorSelection);
    connect(m_textColorButton, &QPushButton::clicked, this, &SettingsDialog::handleColorSelection);
    
    colorsLayout->addRow(tr("Background:"), m_backgroundColorButton);
    colorsLayout->addRow(tr("Grid:"), m_gridColorButton);
    colorsLayout->addRow(tr("Text:"), m_textColorButton);
    
    // Fonts group
    m_fontsGroup = new QGroupBox(tr("Fonts"), m_displayTab);
    QFormLayout *fontsLayout = new QFormLayout(m_fontsGroup);
    
    m_fontButton = new QPushButton(tr("Select Font"));
    connect(m_fontButton, &QPushButton::clicked, this, &SettingsDialog::handleFontSelection);
    
    fontsLayout->addRow(tr("Display font:"), m_fontButton);
    
    layout->addWidget(m_colorsGroup);
    layout->addWidget(m_fontsGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_displayTab, tr("Display"));
}

void SettingsDialog::createAudioTab()
{
    m_audioTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_audioTab);
    
    // Audio device group
    m_audioDeviceGroup = new QGroupBox(tr("Audio Device"), m_audioTab);
    QFormLayout *deviceLayout = new QFormLayout(m_audioDeviceGroup);
    
    m_audioDeviceCombo = new QComboBox();
    m_audioDeviceCombo->addItems({tr("Default"), tr("System Audio"), tr("ASIO Driver")});
    
    m_audioMonitorCheck = new QCheckBox(tr("Enable audio monitoring"));
    
    deviceLayout->addRow(tr("Output device:"), m_audioDeviceCombo);
    deviceLayout->addRow(m_audioMonitorCheck);
    
    // Level group
    m_levelGroup = new QGroupBox(tr("Audio Levels"), m_audioTab);
    QFormLayout *levelLayout = new QFormLayout(m_levelGroup);
    
    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(50);
    
    levelLayout->addRow(tr("Volume:"), m_volumeSlider);
    
    layout->addWidget(m_audioDeviceGroup);
    layout->addWidget(m_levelGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_audioTab, tr("Audio"));
}

void SettingsDialog::createNetworkTab()
{
    m_networkTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_networkTab);
    
    // Connection group
    m_connectionGroup = new QGroupBox(tr("Network Connection"), m_networkTab);
    QFormLayout *connectionLayout = new QFormLayout(m_connectionGroup);
    
    m_useProxyCheck = new QCheckBox(tr("Use proxy server"));
    m_proxyHostEdit = new QLineEdit();
    m_proxyPortSpin = new QSpinBox();
    m_proxyPortSpin->setRange(1, 65535);
    m_proxyPortSpin->setValue(8080);
    
    connectionLayout->addRow(m_useProxyCheck);
    connectionLayout->addRow(tr("Proxy host:"), m_proxyHostEdit);
    connectionLayout->addRow(tr("Proxy port:"), m_proxyPortSpin);
    
    layout->addWidget(m_connectionGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_networkTab, tr("Network"));
}

void SettingsDialog::createPerformanceTab()
{
    m_performanceTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_performanceTab);
    
    // Memory group
    m_memoryGroup = new QGroupBox(tr("Memory Management"), m_performanceTab);
    QFormLayout *memoryLayout = new QFormLayout(m_memoryGroup);
    
    m_memoryLimitSpin = new QSpinBox();
    m_memoryLimitSpin->setRange(MIN_MEMORY_LIMIT, MAX_MEMORY_LIMIT);
    m_memoryLimitSpin->setValue(DEFAULT_MEMORY_LIMIT);
    m_memoryLimitSpin->setSuffix(tr(" MB"));
    
    m_priorityCombo = new QComboBox();
    m_priorityCombo->addItems({tr("Low"), tr("Normal"), tr("High"), tr("Real-time")});
    m_priorityCombo->setCurrentIndex(1); // Normal
    
    memoryLayout->addRow(tr("Memory limit:"), m_memoryLimitSpin);
    memoryLayout->addRow(tr("Process priority:"), m_priorityCombo);
    
    layout->addWidget(m_memoryGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_performanceTab, tr("Performance"));
}

void SettingsDialog::createAdvancedTab()
{
    m_advancedTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_advancedTab);
    
    // Debug group
    m_debugGroup = new QGroupBox(tr("Advanced Options"), m_advancedTab);
    QFormLayout *debugLayout = new QFormLayout(m_debugGroup);
    
    m_debugModeCheck = new QCheckBox(tr("Enable debug mode"));
    m_logLevelCombo = new QComboBox();
    m_logLevelCombo->addItems({tr("Error"), tr("Warning"), tr("Info"), tr("Debug")});
    m_logLevelCombo->setCurrentIndex(2); // Info
    
    debugLayout->addRow(m_debugModeCheck);
    debugLayout->addRow(tr("Log level:"), m_logLevelCombo);
    
    layout->addWidget(m_debugGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_advancedTab, tr("Advanced"));
}

bool SettingsDialog::initialize()
{
    Logger::instance().log(Logger::Info, "SettingsDialog", "Initialization starting");
    
    try {
        // Initialize combo boxes
        initializeComboBoxes();
        
        // Update UI with current settings
        updateUI();
        
        // Mark as loaded
        m_settingsLoaded = true;
        
        Logger::instance().log(Logger::Info, "SettingsDialog", "Initialization completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "SettingsDialog", 
                             QString("Initialization failed: %1").arg(e.what()));
        return false;
    }
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
        m_themeCombo->setCurrentIndex(m_settings->value("general/theme", 0).toInt());
        m_autoSaveCheck->setChecked(m_settings->value("general/autoSave", true).toBool());
        m_confirmExitCheck->setChecked(m_settings->value("general/confirmExit", true).toBool());
        m_autoSaveIntervalSpin->setValue(m_settings->value("general/autoSaveInterval", 60).toInt());
        
        // Load analysis settings
        m_updateRateSpin->setValue(m_settings->value("analysis/updateRate", DEFAULT_UPDATE_RATE).toInt());
        m_bufferSizeSpin->setValue(m_settings->value("analysis/bufferSize", DEFAULT_BUFFER_SIZE).toInt());
        m_realTimeCheck->setChecked(m_settings->value("analysis/realTime", false).toBool());
        m_hardwareAccelCheck->setChecked(m_settings->value("analysis/hardwareAccel", false).toBool());
        
        // Load display settings
        m_backgroundColor = m_settings->value("display/backgroundColor", QColor(20, 20, 30)).value<QColor>();
        m_gridColor = m_settings->value("display/gridColor", QColor(60, 60, 80)).value<QColor>();
        m_textColor = m_settings->value("display/textColor", QColor(200, 200, 220)).value<QColor>();
        m_displayFont = m_settings->value("display/font", QFont("Arial", 9)).value<QFont>();
        
        // Load performance settings
        m_memoryLimitSpin->setValue(m_settings->value("performance/memoryLimit", DEFAULT_MEMORY_LIMIT).toInt());
        m_priorityCombo->setCurrentIndex(m_settings->value("performance/priority", 1).toInt());
        
        // Load advanced settings
        m_debugModeCheck->setChecked(m_settings->value("advanced/debugMode", false).toBool());
        m_logLevelCombo->setCurrentIndex(m_settings->value("advanced/logLevel", 2).toInt());
        
        // Update UI
        updateColorButtons();
        m_fontButton->setText(QString("%1, %2pt").arg(m_displayFont.family()).arg(m_displayFont.pointSize()));
        
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
        m_settings->setValue("general/theme", m_themeCombo->currentIndex());
        m_settings->setValue("general/autoSave", m_autoSaveCheck->isChecked());
        m_settings->setValue("general/confirmExit", m_confirmExitCheck->isChecked());
        m_settings->setValue("general/autoSaveInterval", m_autoSaveIntervalSpin->value());
        
        // Save analysis settings
        m_settings->setValue("analysis/updateRate", m_updateRateSpin->value());
        m_settings->setValue("analysis/bufferSize", m_bufferSizeSpin->value());
        m_settings->setValue("analysis/realTime", m_realTimeCheck->isChecked());
        m_settings->setValue("analysis/hardwareAccel", m_hardwareAccelCheck->isChecked());
        
        // Save display settings
        m_settings->setValue("display/backgroundColor", m_backgroundColor);
        m_settings->setValue("display/gridColor", m_gridColor);
        m_settings->setValue("display/textColor", m_textColor);
        m_settings->setValue("display/font", m_displayFont);
        
        // Save performance settings
        m_settings->setValue("performance/memoryLimit", m_memoryLimitSpin->value());
        m_settings->setValue("performance/priority", m_priorityCombo->currentIndex());
        
        // Save advanced settings
        m_settings->setValue("advanced/debugMode", m_debugModeCheck->isChecked());
        m_settings->setValue("advanced/logLevel", m_logLevelCombo->currentIndex());
        
        m_settings->sync();
        m_settingsChanged = false;
        
        Logger::instance().log(Logger::Debug, "SettingsDialog", "Settings saved successfully");
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "SettingsDialog", 
                             QString("Failed to save settings: %1").arg(e.what()));
    }
}

void SettingsDialog::resetToDefaults()
{
    Logger::instance().log(Logger::Debug, "SettingsDialog", "Resetting settings to defaults");
    
    // Reset to default values
    m_themeCombo->setCurrentIndex(0);
    m_autoSaveCheck->setChecked(true);
    m_confirmExitCheck->setChecked(true);
    m_autoSaveIntervalSpin->setValue(60);
    
    m_updateRateSpin->setValue(DEFAULT_UPDATE_RATE);
    m_bufferSizeSpin->setValue(DEFAULT_BUFFER_SIZE);
    m_realTimeCheck->setChecked(false);
    m_hardwareAccelCheck->setChecked(false);
    
    m_backgroundColor = QColor(20, 20, 30);
    m_gridColor = QColor(60, 60, 80);
    m_textColor = QColor(200, 200, 220);
    m_displayFont = QFont("Arial", 9);
    
    m_memoryLimitSpin->setValue(DEFAULT_MEMORY_LIMIT);
    m_priorityCombo->setCurrentIndex(1);
    
    m_debugModeCheck->setChecked(false);
    m_logLevelCombo->setCurrentIndex(2);
    
    updateColorButtons();
    m_fontButton->setText(QString("%1, %2pt").arg(m_displayFont.family()).arg(m_displayFont.pointSize()));
    
    m_settingsChanged = true;
}

// Getters
int SettingsDialog::getUpdateRate() const
{
    return m_updateRateSpin ? m_updateRateSpin->value() : DEFAULT_UPDATE_RATE;
}

int SettingsDialog::getBufferSize() const
{
    return m_bufferSizeSpin ? m_bufferSizeSpin->value() : DEFAULT_BUFFER_SIZE;
}

int SettingsDialog::getMemoryLimit() const
{
    return m_memoryLimitSpin ? m_memoryLimitSpin->value() : DEFAULT_MEMORY_LIMIT;
}

bool SettingsDialog::isRealTimeProcessingEnabled() const
{
    return m_realTimeCheck ? m_realTimeCheck->isChecked() : false;
}

bool SettingsDialog::isHardwareAccelerationEnabled() const
{
    return m_hardwareAccelCheck ? m_hardwareAccelCheck->isChecked() : false;
}

// Slot implementations
void SettingsDialog::accept()
{
    saveSettings();
    emit settingsChanged();
    QDialog::accept();
}

void SettingsDialog::reject()
{
    QDialog::reject();
}

void SettingsDialog::handleApply()
{
    saveSettings();
    emit settingsChanged();
}

void SettingsDialog::handleReset()
{
    resetToDefaults();
}

void SettingsDialog::handleDefaults()
{
    resetToDefaults();
}

void SettingsDialog::handleBrowseFilePath()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Directory"));
    if (!dir.isEmpty()) {
        // Handle directory selection
        Logger::instance().log(Logger::Debug, "SettingsDialog", QString("Directory selected: %1").arg(dir));
    }
}

void SettingsDialog::handleColorSelection()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    QColor currentColor;
    QColor *targetColor = nullptr;
    
    if (button == m_backgroundColorButton) {
        currentColor = m_backgroundColor;
        targetColor = &m_backgroundColor;
    } else if (button == m_gridColorButton) {
        currentColor = m_gridColor;
        targetColor = &m_gridColor;
    } else if (button == m_textColorButton) {
        currentColor = m_textColor;
        targetColor = &m_textColor;
    }
    
    if (targetColor) {
        QColor color = QColorDialog::getColor(currentColor, this, tr("Select Color"));
        if (color.isValid()) {
            *targetColor = color;
            updateColorButtons();
            m_settingsChanged = true;
        }
    }
}

void SettingsDialog::handleFontSelection()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_displayFont, this, tr("Select Font"));
    
    if (ok) {
        m_displayFont = font;
        m_fontButton->setText(QString("%1, %2pt").arg(font.family()).arg(font.pointSize()));
    }
}

void SettingsDialog::handleSettingsValidation()
{
    // Placeholder for settings validation
    Logger::instance().log(Logger::Debug, "SettingsDialog", "Settings validation - placeholder");
}

// Private helper methods
void SettingsDialog::initializeComboBoxes()
{
    // Combo boxes are already initialized in createUI methods
}

void SettingsDialog::updateUI()
{
    // Load settings if not already loaded
    if (!m_settingsLoaded) {
        loadSettings();
    }
}

void SettingsDialog::updateColorButtons()
{
    if (m_backgroundColorButton) {
        m_backgroundColorButton->setStyleSheet(QString("background-color: %1").arg(m_backgroundColor.name()));
    }
    if (m_gridColorButton) {
        m_gridColorButton->setStyleSheet(QString("background-color: %1").arg(m_gridColor.name()));
    }
    if (m_textColorButton) {
        m_textColorButton->setStyleSheet(QString("background-color: %1").arg(m_textColor.name()));
    }
}

bool SettingsDialog::validateSettings()
{
    // Basic validation
    return true;
}

QVariantMap SettingsDialog::getDefaultSettings() const
{
    QVariantMap defaults;
    defaults["general/theme"] = 0;
    defaults["general/autoSave"] = true;
    defaults["general/confirmExit"] = true;
    defaults["general/autoSaveInterval"] = 60;
    defaults["analysis/updateRate"] = DEFAULT_UPDATE_RATE;
    defaults["analysis/bufferSize"] = DEFAULT_BUFFER_SIZE;
    defaults["analysis/realTime"] = false;
    defaults["analysis/hardwareAccel"] = false;
    defaults["performance/memoryLimit"] = DEFAULT_MEMORY_LIMIT;
    defaults["performance/priority"] = 1;
    defaults["advanced/debugMode"] = false;
    defaults["advanced/logLevel"] = 2;
    return defaults;
}