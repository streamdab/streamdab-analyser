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
    , m_mainLayout(nullptr)
    , m_tabWidget(nullptr)
    , m_buttonBox(nullptr)
    , m_applyButton(nullptr)
    , m_resetButton(nullptr)
    , m_defaultsButton(nullptr)
    , m_generalTab(nullptr)
    , m_autoStartAnalysis(nullptr)
    , m_rememberWindowState(nullptr)
    , m_showSplashScreen(nullptr)
    , m_checkForUpdates(nullptr)
    , m_languageCombo(nullptr)
    , m_themeCombo(nullptr)
    , m_defaultDirectoryEdit(nullptr)
    , m_browseDirectoryButton(nullptr)
    , m_analysisTab(nullptr)
    , m_updateRateSpin(nullptr)
    , m_bufferSizeSpin(nullptr)
    , m_analysisTimeoutSpin(nullptr)
    , m_realTimeProcessingCheck(nullptr)
    , m_enableErrorDetection(nullptr)
    , m_enableFigAnalysis(nullptr)
    , m_enableAudioAnalysis(nullptr)
    , m_processingModeCombo(nullptr)
    , m_sensitivitySlider(nullptr)
    , m_displayTab(nullptr)
    , m_frameRateSpin(nullptr)
    , m_enableAntialiasing(nullptr)
    , m_enableVSync(nullptr)
    , m_showGridLines(nullptr)
    , m_showStatusBar(nullptr)
    , m_colorSchemeCombo(nullptr)
    , m_backgroundColorButton(nullptr)
    , m_gridColorButton(nullptr)
    , m_textColorButton(nullptr)
    , m_fontButton(nullptr)
    , m_backgroundColor(20, 20, 30)
    , m_gridColor(60, 60, 80)
    , m_textColor(200, 200, 220)
    , m_displayFont("Arial", 9)
    , m_audioTab(nullptr)
    , m_enableAudioMonitoring(nullptr)
    , m_audioVolumeSlider(nullptr)
    , m_audioBufferSizeSpin(nullptr)
    , m_audioOutputDeviceCombo(nullptr)
    , m_audioSampleRateCombo(nullptr)
    , m_enableAudioRecording(nullptr)
    , m_audioRecordingPathEdit(nullptr)
    , m_browseAudioPathButton(nullptr)
    , m_networkTab(nullptr)
    , m_enableNetworkStreaming(nullptr)
    , m_networkAddressEdit(nullptr)
    , m_networkPortSpin(nullptr)
    , m_networkTimeoutSpin(nullptr)
    , m_maxConnectionsSpin(nullptr)
    , m_enableMulticast(nullptr)
    , m_multicastAddressEdit(nullptr)
    , m_multicastPortSpin(nullptr)
    , m_performanceTab(nullptr)
    , m_memoryLimitSpin(nullptr)
    , m_threadCountSpin(nullptr)
    , m_enableHardwareAcceleration(nullptr)
    , m_enableGpuAcceleration(nullptr)
    , m_enableMultiThreading(nullptr)
    , m_processingPrioritySlider(nullptr)
    , m_enableMemoryOptimization(nullptr)
    , m_enableCpuOptimization(nullptr)
    , m_advancedTab(nullptr)
    , m_enableDebugLogging(nullptr)
    , m_logLevelCombo(nullptr)
    , m_logFilePathEdit(nullptr)
    , m_browseLogPathButton(nullptr)
    , m_enableProfiling(nullptr)
    , m_enableCrashReporting(nullptr)
    , m_customConfigEdit(nullptr)
    , m_maxLogFilesSpin(nullptr)
    , m_maxLogSizeSpin(nullptr)
    , m_settingsLoaded(false)
    , m_settingsChanged(false)
    , m_settings(nullptr)
{
    // Set dialog properties
    setObjectName("SettingsDialog");
    setWindowTitle(tr("ETI Stream Analyzer - Settings"));
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
    
    // Settings object will be cleaned up automatically
    
    Logger::instance().log(Logger::Info, "SettingsDialog", "Destructor completed");
}

bool SettingsDialog::initialize()
{
    Logger::instance().log(Logger::Info, "SettingsDialog", "Initialization starting");
    
    try {
        // Create UI components
        createUI();
        
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

void SettingsDialog::createUI()
{
    // Main layout
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(10);
    
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
    createButtonBox();
    
    // Add to main layout
    m_mainLayout->addWidget(m_tabWidget, 1);
    m_mainLayout->addWidget(m_buttonBox);
    
    Logger::instance().log(Logger::Debug, "SettingsDialog", "UI creation completed");
}

void SettingsDialog::createGeneralTab()
{
    m_generalTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_generalTab);
    
    // Application behavior group
    QGroupBox *behaviorGroup = new QGroupBox(tr("Application Behavior"), m_generalTab);
    QVBoxLayout *behaviorLayout = new QVBoxLayout(behaviorGroup);
    
    m_autoStartAnalysis = new QCheckBox(tr("Automatically start analysis when file is opened"), behaviorGroup);
    m_rememberWindowState = new QCheckBox(tr("Remember window position and size"), behaviorGroup);
    m_showSplashScreen = new QCheckBox(tr("Show splash screen on startup"), behaviorGroup);
    m_checkForUpdates = new QCheckBox(tr("Check for updates automatically"), behaviorGroup);
    
    behaviorLayout->addWidget(m_autoStartAnalysis);
    behaviorLayout->addWidget(m_rememberWindowState);
    behaviorLayout->addWidget(m_showSplashScreen);
    behaviorLayout->addWidget(m_checkForUpdates);
    
    // Localization group
    QGroupBox *localizationGroup = new QGroupBox(tr("Localization"), m_generalTab);
    QGridLayout *localizationLayout = new QGridLayout(localizationGroup);
    
    QLabel *languageLabel = new QLabel(tr("Language:"), localizationGroup);
    m_languageCombo = new QComboBox(localizationGroup);
    m_languageCombo->addItems({tr("English"), tr("German"), tr("French"), tr("Spanish")});
    
    QLabel *themeLabel = new QLabel(tr("Theme:"), localizationGroup);
    m_themeCombo = new QComboBox(localizationGroup);
    m_themeCombo->addItems({tr("Professional Dark"), tr("Professional Light"), tr("High Contrast"), tr("System")});
    
    localizationLayout->addWidget(languageLabel, 0, 0);
    localizationLayout->addWidget(m_languageCombo, 0, 1);
    localizationLayout->addWidget(themeLabel, 1, 0);
    localizationLayout->addWidget(m_themeCombo, 1, 1);
    
    // Paths group
    QGroupBox *pathsGroup = new QGroupBox(tr("Default Paths"), m_generalTab);
    QGridLayout *pathsLayout = new QGridLayout(pathsGroup);
    
    QLabel *directoryLabel = new QLabel(tr("Default ETI Directory:"), pathsGroup);
    m_defaultDirectoryEdit = new QLineEdit(pathsGroup);
    m_browseDirectoryButton = new QPushButton(tr("Browse..."), pathsGroup);
    
    pathsLayout->addWidget(directoryLabel, 0, 0);
    pathsLayout->addWidget(m_defaultDirectoryEdit, 0, 1);
    pathsLayout->addWidget(m_browseDirectoryButton, 0, 2);
    
    // Add groups to tab layout
    layout->addWidget(behaviorGroup);
    layout->addWidget(localizationGroup);
    layout->addWidget(pathsGroup);
    layout->addStretch();
    
    // Connect signals
    connect(m_browseDirectoryButton, &QPushButton::clicked, this, &SettingsDialog::handleBrowseFilePath);
    
    // Add tab to widget
    m_tabWidget->addTab(m_generalTab, tr("General"));
}

void SettingsDialog::createAnalysisTab()
{
    m_analysisTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_analysisTab);
    
    // Processing group
    QGroupBox *processingGroup = new QGroupBox(tr("Processing Settings"), m_analysisTab);
    QGridLayout *processingLayout = new QGridLayout(processingGroup);
    
    QLabel *updateRateLabel = new QLabel(tr("Update Rate (FPS):"), processingGroup);
    m_updateRateSpin = new QSpinBox(processingGroup);
    m_updateRateSpin->setRange(MIN_UPDATE_RATE, MAX_UPDATE_RATE);
    m_updateRateSpin->setValue(DEFAULT_UPDATE_RATE);
    m_updateRateSpin->setSuffix(" FPS");
    
    QLabel *bufferSizeLabel = new QLabel(tr("Buffer Size (frames):"), processingGroup);
    m_bufferSizeSpin = new QSpinBox(processingGroup);
    m_bufferSizeSpin->setRange(MIN_BUFFER_SIZE, MAX_BUFFER_SIZE);
    m_bufferSizeSpin->setValue(DEFAULT_BUFFER_SIZE);
    
    QLabel *timeoutLabel = new QLabel(tr("Analysis Timeout (s):"), processingGroup);
    m_analysisTimeoutSpin = new QDoubleSpinBox(processingGroup);
    m_analysisTimeoutSpin->setRange(0.1, 30.0);
    m_analysisTimeoutSpin->setValue(5.0);
    m_analysisTimeoutSpin->setSuffix(" s");
    
    QLabel *modeLabel = new QLabel(tr("Processing Mode:"), processingGroup);
    m_processingModeCombo = new QComboBox(processingGroup);
    m_processingModeCombo->addItems({tr("Standard"), tr("High Quality"), tr("Fast"), tr("Custom")});
    
    processingLayout->addWidget(updateRateLabel, 0, 0);
    processingLayout->addWidget(m_updateRateSpin, 0, 1);
    processingLayout->addWidget(bufferSizeLabel, 1, 0);
    processingLayout->addWidget(m_bufferSizeSpin, 1, 1);
    processingLayout->addWidget(timeoutLabel, 2, 0);
    processingLayout->addWidget(m_analysisTimeoutSpin, 2, 1);
    processingLayout->addWidget(modeLabel, 3, 0);
    processingLayout->addWidget(m_processingModeCombo, 3, 1);
    
    // Features group
    QGroupBox *featuresGroup = new QGroupBox(tr("Analysis Features"), m_analysisTab);
    QVBoxLayout *featuresLayout = new QVBoxLayout(featuresGroup);
    
    m_realTimeProcessingCheck = new QCheckBox(tr("Enable real-time processing"), featuresGroup);
    m_enableErrorDetection = new QCheckBox(tr("Enable error detection"), featuresGroup);
    m_enableFigAnalysis = new QCheckBox(tr("Enable FIG analysis"), featuresGroup);
    m_enableAudioAnalysis = new QCheckBox(tr("Enable audio analysis"), featuresGroup);
    
    featuresLayout->addWidget(m_realTimeProcessingCheck);
    featuresLayout->addWidget(m_enableErrorDetection);
    featuresLayout->addWidget(m_enableFigAnalysis);
    featuresLayout->addWidget(m_enableAudioAnalysis);
    
    // Sensitivity group
    QGroupBox *sensitivityGroup = new QGroupBox(tr("Detection Sensitivity"), m_analysisTab);
    QVBoxLayout *sensitivityLayout = new QVBoxLayout(sensitivityGroup);
    
    QLabel *sensitivityLabel = new QLabel(tr("Error Detection Sensitivity:"), sensitivityGroup);
    m_sensitivitySlider = new QSlider(Qt::Horizontal, sensitivityGroup);
    m_sensitivitySlider->setRange(1, 10);
    m_sensitivitySlider->setValue(5);
    m_sensitivitySlider->setTickPosition(QSlider::TicksBelow);
    m_sensitivitySlider->setTickInterval(1);
    
    QHBoxLayout *sensitivityLabelsLayout = new QHBoxLayout();
    sensitivityLabelsLayout->addWidget(new QLabel(tr("Low"), sensitivityGroup));
    sensitivityLabelsLayout->addStretch();
    sensitivityLabelsLayout->addWidget(new QLabel(tr("High"), sensitivityGroup));
    
    sensitivityLayout->addWidget(sensitivityLabel);
    sensitivityLayout->addWidget(m_sensitivitySlider);
    sensitivityLayout->addLayout(sensitivityLabelsLayout);
    
    // Add groups to tab layout
    layout->addWidget(processingGroup);
    layout->addWidget(featuresGroup);
    layout->addWidget(sensitivityGroup);
    layout->addStretch();
    
    // Add tab to widget
    m_tabWidget->addTab(m_analysisTab, tr("Analysis"));
}

void SettingsDialog::createDisplayTab()
{
    m_displayTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_displayTab);
    
    // Rendering group
    QGroupBox *renderingGroup = new QGroupBox(tr("Rendering Settings"), m_displayTab);
    QGridLayout *renderingLayout = new QGridLayout(renderingGroup);
    
    QLabel *frameRateLabel = new QLabel(tr("Display Frame Rate:"), renderingGroup);
    m_frameRateSpin = new QSpinBox(renderingGroup);
    m_frameRateSpin->setRange(15, 120);
    m_frameRateSpin->setValue(60);
    m_frameRateSpin->setSuffix(" FPS");
    
    m_enableAntialiasing = new QCheckBox(tr("Enable antialiasing"), renderingGroup);
    m_enableVSync = new QCheckBox(tr("Enable vertical sync"), renderingGroup);
    m_showGridLines = new QCheckBox(tr("Show grid lines"), renderingGroup);
    m_showStatusBar = new QCheckBox(tr("Show status bar"), renderingGroup);
    
    renderingLayout->addWidget(frameRateLabel, 0, 0);
    renderingLayout->addWidget(m_frameRateSpin, 0, 1);
    renderingLayout->addWidget(m_enableAntialiasing, 1, 0, 1, 2);
    renderingLayout->addWidget(m_enableVSync, 2, 0, 1, 2);
    renderingLayout->addWidget(m_showGridLines, 3, 0, 1, 2);
    renderingLayout->addWidget(m_showStatusBar, 4, 0, 1, 2);
    
    // Color scheme group
    QGroupBox *colorGroup = new QGroupBox(tr("Color Scheme"), m_displayTab);
    QGridLayout *colorLayout = new QGridLayout(colorGroup);
    
    QLabel *schemeLabel = new QLabel(tr("Color Scheme:"), colorGroup);
    m_colorSchemeCombo = new QComboBox(colorGroup);
    m_colorSchemeCombo->addItems({tr("Professional Dark"), tr("Professional Light"), tr("High Contrast"), tr("Custom")});
    
    QLabel *backgroundLabel = new QLabel(tr("Background Color:"), colorGroup);
    m_backgroundColorButton = new QPushButton(colorGroup);
    m_backgroundColorButton->setFixedSize(50, 25);
    
    QLabel *gridLabel = new QLabel(tr("Grid Color:"), colorGroup);
    m_gridColorButton = new QPushButton(colorGroup);
    m_gridColorButton->setFixedSize(50, 25);
    
    QLabel *textLabel = new QLabel(tr("Text Color:"), colorGroup);
    m_textColorButton = new QPushButton(colorGroup);
    m_textColorButton->setFixedSize(50, 25);
    
    colorLayout->addWidget(schemeLabel, 0, 0);
    colorLayout->addWidget(m_colorSchemeCombo, 0, 1);
    colorLayout->addWidget(backgroundLabel, 1, 0);
    colorLayout->addWidget(m_backgroundColorButton, 1, 1);
    colorLayout->addWidget(gridLabel, 2, 0);
    colorLayout->addWidget(m_gridColorButton, 2, 1);
    colorLayout->addWidget(textLabel, 3, 0);
    colorLayout->addWidget(m_textColorButton, 3, 1);
    
    // Font group
    QGroupBox *fontGroup = new QGroupBox(tr("Font Settings"), m_displayTab);
    QHBoxLayout *fontLayout = new QHBoxLayout(fontGroup);
    
    QLabel *fontLabel = new QLabel(tr("Display Font:"), fontGroup);
    m_fontButton = new QPushButton(tr("Select Font..."), fontGroup);
    
    fontLayout->addWidget(fontLabel);
    fontLayout->addWidget(m_fontButton);
    fontLayout->addStretch();
    
    // Add groups to tab layout
    layout->addWidget(renderingGroup);
    layout->addWidget(colorGroup);
    layout->addWidget(fontGroup);
    layout->addStretch();
    
    // Connect signals
    connect(m_backgroundColorButton, &QPushButton::clicked, this, &SettingsDialog::handleColorSelection);
    connect(m_gridColorButton, &QPushButton::clicked, this, &SettingsDialog::handleColorSelection);
    connect(m_textColorButton, &QPushButton::clicked, this, &SettingsDialog::handleColorSelection);
    connect(m_fontButton, &QPushButton::clicked, this, &SettingsDialog::handleFontSelection);
    
    // Add tab to widget
    m_tabWidget->addTab(m_displayTab, tr("Display"));
}

void SettingsDialog::createAudioTab()
{
    m_audioTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_audioTab);
    
    // Audio monitoring group
    QGroupBox *monitoringGroup = new QGroupBox(tr("Audio Monitoring"), m_audioTab);
    QGridLayout *monitoringLayout = new QGridLayout(monitoringGroup);
    
    m_enableAudioMonitoring = new QCheckBox(tr("Enable audio monitoring"), monitoringGroup);
    
    QLabel *volumeLabel = new QLabel(tr("Monitor Volume:"), monitoringGroup);
    m_audioVolumeSlider = new QSlider(Qt::Horizontal, monitoringGroup);
    m_audioVolumeSlider->setRange(0, 100);
    m_audioVolumeSlider->setValue(50);
    
    QLabel *bufferLabel = new QLabel(tr("Audio Buffer Size:"), monitoringGroup);
    m_audioBufferSizeSpin = new QSpinBox(monitoringGroup);
    m_audioBufferSizeSpin->setRange(256, 8192);
    m_audioBufferSizeSpin->setValue(1024);
    
    QLabel *deviceLabel = new QLabel(tr("Output Device:"), monitoringGroup);
    m_audioOutputDeviceCombo = new QComboBox(monitoringGroup);
    m_audioOutputDeviceCombo->addItems({tr("Default Device"), tr("System Audio"), tr("USB Audio")});
    
    QLabel *sampleRateLabel = new QLabel(tr("Sample Rate:"), monitoringGroup);
    m_audioSampleRateCombo = new QComboBox(monitoringGroup);
    m_audioSampleRateCombo->addItems({"48000 Hz", "44100 Hz", "32000 Hz"});
    
    monitoringLayout->addWidget(m_enableAudioMonitoring, 0, 0, 1, 2);
    monitoringLayout->addWidget(volumeLabel, 1, 0);
    monitoringLayout->addWidget(m_audioVolumeSlider, 1, 1);
    monitoringLayout->addWidget(bufferLabel, 2, 0);
    monitoringLayout->addWidget(m_audioBufferSizeSpin, 2, 1);
    monitoringLayout->addWidget(deviceLabel, 3, 0);
    monitoringLayout->addWidget(m_audioOutputDeviceCombo, 3, 1);
    monitoringLayout->addWidget(sampleRateLabel, 4, 0);
    monitoringLayout->addWidget(m_audioSampleRateCombo, 4, 1);
    
    // Audio recording group
    QGroupBox *recordingGroup = new QGroupBox(tr("Audio Recording"), m_audioTab);
    QGridLayout *recordingLayout = new QGridLayout(recordingGroup);
    
    m_enableAudioRecording = new QCheckBox(tr("Enable audio recording"), recordingGroup);
    
    QLabel *pathLabel = new QLabel(tr("Recording Path:"), recordingGroup);
    m_audioRecordingPathEdit = new QLineEdit(recordingGroup);
    m_browseAudioPathButton = new QPushButton(tr("Browse..."), recordingGroup);
    
    recordingLayout->addWidget(m_enableAudioRecording, 0, 0, 1, 3);
    recordingLayout->addWidget(pathLabel, 1, 0);
    recordingLayout->addWidget(m_audioRecordingPathEdit, 1, 1);
    recordingLayout->addWidget(m_browseAudioPathButton, 1, 2);
    
    // Add groups to tab layout
    layout->addWidget(monitoringGroup);
    layout->addWidget(recordingGroup);
    layout->addStretch();
    
    // Add tab to widget
    m_tabWidget->addTab(m_audioTab, tr("Audio"));
}

void SettingsDialog::createNetworkTab()
{
    m_networkTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_networkTab);
    
    // Network streaming group
    QGroupBox *streamingGroup = new QGroupBox(tr("Network Streaming"), m_networkTab);
    QGridLayout *streamingLayout = new QGridLayout(streamingGroup);
    
    m_enableNetworkStreaming = new QCheckBox(tr("Enable network streaming"), streamingGroup);
    
    QLabel *addressLabel = new QLabel(tr("Server Address:"), streamingGroup);
    m_networkAddressEdit = new QLineEdit(streamingGroup);
    m_networkAddressEdit->setPlaceholderText("192.168.1.100");
    
    QLabel *portLabel = new QLabel(tr("Port:"), streamingGroup);
    m_networkPortSpin = new QSpinBox(streamingGroup);
    m_networkPortSpin->setRange(1024, 65535);
    m_networkPortSpin->setValue(9000);
    
    QLabel *timeoutLabel = new QLabel(tr("Timeout (ms):"), streamingGroup);
    m_networkTimeoutSpin = new QSpinBox(streamingGroup);
    m_networkTimeoutSpin->setRange(100, 30000);
    m_networkTimeoutSpin->setValue(5000);
    
    QLabel *connectionsLabel = new QLabel(tr("Max Connections:"), streamingGroup);
    m_maxConnectionsSpin = new QSpinBox(streamingGroup);
    m_maxConnectionsSpin->setRange(1, 100);
    m_maxConnectionsSpin->setValue(10);
    
    streamingLayout->addWidget(m_enableNetworkStreaming, 0, 0, 1, 2);
    streamingLayout->addWidget(addressLabel, 1, 0);
    streamingLayout->addWidget(m_networkAddressEdit, 1, 1);
    streamingLayout->addWidget(portLabel, 2, 0);
    streamingLayout->addWidget(m_networkPortSpin, 2, 1);
    streamingLayout->addWidget(timeoutLabel, 3, 0);
    streamingLayout->addWidget(m_networkTimeoutSpin, 3, 1);
    streamingLayout->addWidget(connectionsLabel, 4, 0);
    streamingLayout->addWidget(m_maxConnectionsSpin, 4, 1);
    
    // Multicast group
    QGroupBox *multicastGroup = new QGroupBox(tr("Multicast Settings"), m_networkTab);
    QGridLayout *multicastLayout = new QGridLayout(multicastGroup);
    
    m_enableMulticast = new QCheckBox(tr("Enable multicast"), multicastGroup);
    
    QLabel *multicastAddressLabel = new QLabel(tr("Multicast Address:"), multicastGroup);
    m_multicastAddressEdit = new QLineEdit(multicastGroup);
    m_multicastAddressEdit->setPlaceholderText("239.255.0.1");
    
    QLabel *multicastPortLabel = new QLabel(tr("Multicast Port:"), multicastGroup);
    m_multicastPortSpin = new QSpinBox(multicastGroup);
    m_multicastPortSpin->setRange(1024, 65535);
    m_multicastPortSpin->setValue(9001);
    
    multicastLayout->addWidget(m_enableMulticast, 0, 0, 1, 2);
    multicastLayout->addWidget(multicastAddressLabel, 1, 0);
    multicastLayout->addWidget(m_multicastAddressEdit, 1, 1);
    multicastLayout->addWidget(multicastPortLabel, 2, 0);
    multicastLayout->addWidget(m_multicastPortSpin, 2, 1);
    
    // Add groups to tab layout
    layout->addWidget(streamingGroup);
    layout->addWidget(multicastGroup);
    layout->addStretch();
    
    // Add tab to widget
    m_tabWidget->addTab(m_networkTab, tr("Network"));
}

void SettingsDialog::createPerformanceTab()
{
    m_performanceTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_performanceTab);
    
    // Resource limits group
    QGroupBox *resourceGroup = new QGroupBox(tr("Resource Limits"), m_performanceTab);
    QGridLayout *resourceLayout = new QGridLayout(resourceGroup);
    
    QLabel *memoryLabel = new QLabel(tr("Memory Limit (MB):"), resourceGroup);
    m_memoryLimitSpin = new QSpinBox(resourceGroup);
    m_memoryLimitSpin->setRange(MIN_MEMORY_LIMIT, MAX_MEMORY_LIMIT);
    m_memoryLimitSpin->setValue(DEFAULT_MEMORY_LIMIT);
    m_memoryLimitSpin->setSuffix(" MB");
    
    QLabel *threadsLabel = new QLabel(tr("Thread Count:"), resourceGroup);
    m_threadCountSpin = new QSpinBox(resourceGroup);
    m_threadCountSpin->setRange(1, 16);
    m_threadCountSpin->setValue(4);
    
    resourceLayout->addWidget(memoryLabel, 0, 0);
    resourceLayout->addWidget(m_memoryLimitSpin, 0, 1);
    resourceLayout->addWidget(threadsLabel, 1, 0);
    resourceLayout->addWidget(m_threadCountSpin, 1, 1);
    
    // Acceleration group
    QGroupBox *accelerationGroup = new QGroupBox(tr("Hardware Acceleration"), m_performanceTab);
    QVBoxLayout *accelerationLayout = new QVBoxLayout(accelerationGroup);
    
    m_enableHardwareAcceleration = new QCheckBox(tr("Enable hardware acceleration"), accelerationGroup);
    m_enableGpuAcceleration = new QCheckBox(tr("Enable GPU acceleration"), accelerationGroup);
    m_enableMultiThreading = new QCheckBox(tr("Enable multi-threading"), accelerationGroup);
    
    accelerationLayout->addWidget(m_enableHardwareAcceleration);
    accelerationLayout->addWidget(m_enableGpuAcceleration);
    accelerationLayout->addWidget(m_enableMultiThreading);
    
    // Optimization group
    QGroupBox *optimizationGroup = new QGroupBox(tr("Optimization"), m_performanceTab);
    QVBoxLayout *optimizationLayout = new QVBoxLayout(optimizationGroup);
    
    QLabel *priorityLabel = new QLabel(tr("Processing Priority:"), optimizationGroup);
    m_processingPrioritySlider = new QSlider(Qt::Horizontal, optimizationGroup);
    m_processingPrioritySlider->setRange(1, 5);
    m_processingPrioritySlider->setValue(3);
    m_processingPrioritySlider->setTickPosition(QSlider::TicksBelow);
    
    QHBoxLayout *priorityLabelsLayout = new QHBoxLayout();
    priorityLabelsLayout->addWidget(new QLabel(tr("Low"), optimizationGroup));
    priorityLabelsLayout->addStretch();
    priorityLabelsLayout->addWidget(new QLabel(tr("High"), optimizationGroup));
    
    m_enableMemoryOptimization = new QCheckBox(tr("Enable memory optimization"), optimizationGroup);
    m_enableCpuOptimization = new QCheckBox(tr("Enable CPU optimization"), optimizationGroup);
    
    optimizationLayout->addWidget(priorityLabel);
    optimizationLayout->addWidget(m_processingPrioritySlider);
    optimizationLayout->addLayout(priorityLabelsLayout);
    optimizationLayout->addWidget(m_enableMemoryOptimization);
    optimizationLayout->addWidget(m_enableCpuOptimization);
    
    // Add groups to tab layout
    layout->addWidget(resourceGroup);
    layout->addWidget(accelerationGroup);
    layout->addWidget(optimizationGroup);
    layout->addStretch();
    
    // Add tab to widget
    m_tabWidget->addTab(m_performanceTab, tr("Performance"));
}

void SettingsDialog::createAdvancedTab()
{
    m_advancedTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_advancedTab);
    
    // Logging group
    QGroupBox *loggingGroup = new QGroupBox(tr("Logging Settings"), m_advancedTab);
    QGridLayout *loggingLayout = new QGridLayout(loggingGroup);
    
    m_enableDebugLogging = new QCheckBox(tr("Enable debug logging"), loggingGroup);
    
    QLabel *logLevelLabel = new QLabel(tr("Log Level:"), loggingGroup);
    m_logLevelCombo = new QComboBox(loggingGroup);
    m_logLevelCombo->addItems({tr("Error"), tr("Warning"), tr("Info"), tr("Debug"), tr("Verbose")});
    
    QLabel *logPathLabel = new QLabel(tr("Log File Path:"), loggingGroup);
    m_logFilePathEdit = new QLineEdit(loggingGroup);
    m_browseLogPathButton = new QPushButton(tr("Browse..."), loggingGroup);
    
    QLabel *maxFilesLabel = new QLabel(tr("Max Log Files:"), loggingGroup);
    m_maxLogFilesSpin = new QSpinBox(loggingGroup);
    m_maxLogFilesSpin->setRange(1, 100);
    m_maxLogFilesSpin->setValue(10);
    
    QLabel *maxSizeLabel = new QLabel(tr("Max Log Size (MB):"), loggingGroup);
    m_maxLogSizeSpin = new QSpinBox(loggingGroup);
    m_maxLogSizeSpin->setRange(1, 1000);
    m_maxLogSizeSpin->setValue(10);
    
    loggingLayout->addWidget(m_enableDebugLogging, 0, 0, 1, 3);
    loggingLayout->addWidget(logLevelLabel, 1, 0);
    loggingLayout->addWidget(m_logLevelCombo, 1, 1, 1, 2);
    loggingLayout->addWidget(logPathLabel, 2, 0);
    loggingLayout->addWidget(m_logFilePathEdit, 2, 1);
    loggingLayout->addWidget(m_browseLogPathButton, 2, 2);
    loggingLayout->addWidget(maxFilesLabel, 3, 0);
    loggingLayout->addWidget(m_maxLogFilesSpin, 3, 1, 1, 2);
    loggingLayout->addWidget(maxSizeLabel, 4, 0);
    loggingLayout->addWidget(m_maxLogSizeSpin, 4, 1, 1, 2);
    
    // Debug group
    QGroupBox *debugGroup = new QGroupBox(tr("Debug Features"), m_advancedTab);
    QVBoxLayout *debugLayout = new QVBoxLayout(debugGroup);
    
    m_enableProfiling = new QCheckBox(tr("Enable performance profiling"), debugGroup);
    m_enableCrashReporting = new QCheckBox(tr("Enable crash reporting"), debugGroup);
    
    debugLayout->addWidget(m_enableProfiling);
    debugLayout->addWidget(m_enableCrashReporting);
    
    // Custom configuration group
    QGroupBox *customGroup = new QGroupBox(tr("Custom Configuration"), m_advancedTab);
    QVBoxLayout *customLayout = new QVBoxLayout(customGroup);
    
    QLabel *customLabel = new QLabel(tr("Custom Config (JSON):"), customGroup);
    m_customConfigEdit = new QLineEdit(customGroup);
    m_customConfigEdit->setPlaceholderText(tr("Enter custom JSON configuration..."));
    
    customLayout->addWidget(customLabel);
    customLayout->addWidget(m_customConfigEdit);
    
    // Add groups to tab layout
    layout->addWidget(loggingGroup);
    layout->addWidget(debugGroup);
    layout->addWidget(customGroup);
    layout->addStretch();
    
    // Add tab to widget
    m_tabWidget->addTab(m_advancedTab, tr("Advanced"));
}

void SettingsDialog::createButtonBox()
{
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    
    // Add custom buttons
    m_applyButton = m_buttonBox->addButton(tr("Apply"), QDialogButtonBox::ApplyRole);
    m_resetButton = m_buttonBox->addButton(tr("Reset"), QDialogButtonBox::ResetRole);
    m_defaultsButton = m_buttonBox->addButton(tr("Defaults"), QDialogButtonBox::ResetRole);
    
    // Connect signals
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    connect(m_applyButton, &QPushButton::clicked, this, &SettingsDialog::handleApply);
    connect(m_resetButton, &QPushButton::clicked, this, &SettingsDialog::handleReset);
    connect(m_defaultsButton, &QPushButton::clicked, this, &SettingsDialog::handleDefaults);
}

void SettingsDialog::loadSettings()
{
    if (!m_settings) {
        return;
    }
    
    m_settings->beginGroup("General");
    // Load general settings with defaults
    m_settings->endGroup();
    
    m_settings->beginGroup("Analysis");
    // Load analysis settings with defaults
    m_settings->endGroup();
    
    m_settings->beginGroup("Display");
    // Load display settings with defaults
    m_settings->endGroup();
    
    m_settings->beginGroup("Audio");
    // Load audio settings with defaults
    m_settings->endGroup();
    
    m_settings->beginGroup("Network");
    // Load network settings with defaults
    m_settings->endGroup();
    
    m_settings->beginGroup("Performance");
    // Load performance settings with defaults
    m_settings->endGroup();
    
    m_settings->beginGroup("Advanced");
    // Load advanced settings with defaults
    m_settings->endGroup();
    
    m_settingsLoaded = true;
    
    Logger::instance().log(Logger::Info, "SettingsDialog", "Settings loaded");
}

void SettingsDialog::saveSettings()
{
    if (!m_settings) {
        return;
    }
    
    // Save general settings
    m_settings->beginGroup("General");
    m_settings->setValue("autoStartAnalysis", m_autoStartAnalysis->isChecked());
    m_settings->setValue("rememberWindowState", m_rememberWindowState->isChecked());
    m_settings->setValue("updateRate", m_updateRateSpin->value());
    m_settings->endGroup();
    
    // Save analysis settings
    m_settings->beginGroup("Analysis");
    m_settings->setValue("bufferSize", m_bufferSizeSpin->value());
    m_settings->setValue("realTimeProcessing", m_realTimeProcessingCheck->isChecked());
    m_settings->endGroup();
    
    // Save display settings
    m_settings->beginGroup("Display");
    m_settings->setValue("frameRate", m_frameRateSpin->value());
    m_settings->setValue("antialiasing", m_enableAntialiasing->isChecked());
    m_settings->endGroup();
    
    // Save performance settings
    m_settings->beginGroup("Performance");
    m_settings->setValue("memoryLimit", m_memoryLimitSpin->value());
    m_settings->setValue("hardwareAcceleration", m_enableHardwareAcceleration->isChecked());
    m_settings->endGroup();
    
    m_settings->sync();
    
    Logger::instance().log(Logger::Info, "SettingsDialog", "Settings saved");
}

void SettingsDialog::updateUI()
{
    // Update color buttons
    if (m_backgroundColorButton) {
        QString style = QString("background-color: %1").arg(m_backgroundColor.name());
        m_backgroundColorButton->setStyleSheet(style);
    }
    
    if (m_gridColorButton) {
        QString style = QString("background-color: %1").arg(m_gridColor.name());
        m_gridColorButton->setStyleSheet(style);
    }
    
    if (m_textColorButton) {
        QString style = QString("background-color: %1").arg(m_textColor.name());
        m_textColorButton->setStyleSheet(style);
    }
    
    // Update font button
    if (m_fontButton) {
        m_fontButton->setText(QString("%1, %2pt").arg(m_displayFont.family()).arg(m_displayFont.pointSize()));
    }
}

bool SettingsDialog::validateSettings()
{
    // Validate update rate
    if (m_updateRateSpin && (m_updateRateSpin->value() < MIN_UPDATE_RATE || m_updateRateSpin->value() > MAX_UPDATE_RATE)) {
        QMessageBox::warning(this, tr("Invalid Settings"), 
                           tr("Update rate must be between %1 and %2 FPS.")
                           .arg(MIN_UPDATE_RATE).arg(MAX_UPDATE_RATE));
        return false;
    }
    
    // Validate memory limit
    if (m_memoryLimitSpin && (m_memoryLimitSpin->value() < MIN_MEMORY_LIMIT || m_memoryLimitSpin->value() > MAX_MEMORY_LIMIT)) {
        QMessageBox::warning(this, tr("Invalid Settings"), 
                           tr("Memory limit must be between %1 and %2 MB.")
                           .arg(MIN_MEMORY_LIMIT).arg(MAX_MEMORY_LIMIT));
        return false;
    }
    
    return true;
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
    return m_realTimeProcessingCheck ? m_realTimeProcessingCheck->isChecked() : true;
}

bool SettingsDialog::isHardwareAccelerationEnabled() const
{
    return m_enableHardwareAcceleration ? m_enableHardwareAcceleration->isChecked() : false;
}

// Protected methods
void SettingsDialog::accept()
{
    if (validateSettings()) {
        saveSettings();
        emit settingsChanged();
        QDialog::accept();
        
        Logger::instance().log(Logger::Info, "SettingsDialog", "Settings accepted");
    }
}

void SettingsDialog::reject()
{
    // Reload settings to discard changes
    loadSettings();
    updateUI();
    
    QDialog::reject();
    
    Logger::instance().log(Logger::Info, "SettingsDialog", "Settings rejected");
}

// Private slots
void SettingsDialog::handleApply()
{
    if (validateSettings()) {
        saveSettings();
        emit settingsChanged();
        
        Logger::instance().log(Logger::Info, "SettingsDialog", "Settings applied");
    }
}

void SettingsDialog::handleReset()
{
    loadSettings();
    updateUI();
    
    Logger::instance().log(Logger::Info, "SettingsDialog", "Settings reset");
}

void SettingsDialog::handleDefaults()
{
    QVariantMap defaults = getDefaultSettings();
    
    // Apply defaults to UI controls
    if (m_updateRateSpin) m_updateRateSpin->setValue(DEFAULT_UPDATE_RATE);
    if (m_bufferSizeSpin) m_bufferSizeSpin->setValue(DEFAULT_BUFFER_SIZE);
    if (m_memoryLimitSpin) m_memoryLimitSpin->setValue(DEFAULT_MEMORY_LIMIT);
    
    // Reset checkboxes
    if (m_autoStartAnalysis) m_autoStartAnalysis->setChecked(true);
    if (m_realTimeProcessingCheck) m_realTimeProcessingCheck->setChecked(true);
    if (m_enableHardwareAcceleration) m_enableHardwareAcceleration->setChecked(false);
    
    updateUI();
    
    Logger::instance().log(Logger::Info, "SettingsDialog", "Settings reset to defaults");
}

void SettingsDialog::handleBrowseFilePath()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Select Default ETI Directory"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
    );
    
    if (!dir.isEmpty() && m_defaultDirectoryEdit) {
        m_defaultDirectoryEdit->setText(dir);
    }
}

void SettingsDialog::handleColorSelection()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    QColor currentColor;
    if (button == m_backgroundColorButton) {
        currentColor = m_backgroundColor;
    } else if (button == m_gridColorButton) {
        currentColor = m_gridColor;
    } else if (button == m_textColorButton) {
        currentColor = m_textColor;
    }
    
    QColor newColor = QColorDialog::getColor(currentColor, this, tr("Select Color"));
    if (newColor.isValid()) {
        if (button == m_backgroundColorButton) {
            m_backgroundColor = newColor;
        } else if (button == m_gridColorButton) {
            m_gridColor = newColor;
        } else if (button == m_textColorButton) {
            m_textColor = newColor;
        }
        
        updateUI();
    }
}

void SettingsDialog::handleFontSelection()
{
    bool ok;
    QFont newFont = QFontDialog::getFont(&ok, m_displayFont, this, tr("Select Font"));
    if (ok) {
        m_displayFont = newFont;
        updateUI();
    }
}

void SettingsDialog::handleSettingsValidation()
{
    validateSettings();
}

QVariantMap SettingsDialog::getDefaultSettings() const
{
    QVariantMap defaults;
    defaults["updateRate"] = DEFAULT_UPDATE_RATE;
    defaults["bufferSize"] = DEFAULT_BUFFER_SIZE;
    defaults["memoryLimit"] = DEFAULT_MEMORY_LIMIT;
    defaults["autoStartAnalysis"] = true;
    defaults["realTimeProcessing"] = true;
    defaults["hardwareAcceleration"] = false;
    return defaults;
}