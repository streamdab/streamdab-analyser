#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QSlider>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QColorDialog>
#include <QFontDialog>
#include <QSettings>
#include <QFont>
#include <QColor>
#include <QString>
#include <QStringList>

/**
 * @class SettingsDialog
 * @brief Professional settings configuration dialog for ETI Stream Analyzer
 * 
 * This dialog provides comprehensive configuration options for the ETI Stream
 * Analyzer application with professional broadcast industry settings including:
 * - General application preferences
 * - Analysis and processing parameters
 * - Display and visualization settings
 * - Audio monitoring configuration
 * - Network and stream settings
 * - Performance optimization options
 * - Professional broadcast workflow settings
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Settings category enumeration
     */
    enum class SettingsCategory {
        General = 0,
        Analysis,
        Display,
        Audio,
        Network,
        Performance,
        Advanced
    };

    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit SettingsDialog(QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~SettingsDialog();

    /**
     * @brief Initialize the settings dialog
     * @return true if successful
     */
    bool initialize();

    /**
     * @brief Load settings from configuration
     */
    void loadSettings();

    /**
     * @brief Save settings to configuration
     */
    void saveSettings();

    /**
     * @brief Reset settings to defaults
     */
    void resetToDefaults();

    /**
     * @brief Get current update rate setting
     * @return Update rate in FPS
     */
    int getUpdateRate() const;

    /**
     * @brief Get current buffer size setting
     * @return Buffer size in frames
     */
    int getBufferSize() const;

    /**
     * @brief Get current memory limit setting
     * @return Memory limit in MB
     */
    int getMemoryLimit() const;

    /**
     * @brief Check if real-time processing is enabled
     * @return true if real-time processing enabled
     */
    bool isRealTimeProcessingEnabled() const;

    /**
     * @brief Check if hardware acceleration is enabled
     * @return true if hardware acceleration enabled
     */
    bool isHardwareAccelerationEnabled() const;

signals:
    /**
     * @brief Emitted when settings are changed and accepted
     */
    void settingsChanged();

    /**
     * @brief Emitted when specific setting category changes
     * @param category Changed category
     */
    void categoryChanged(SettingsCategory category);

protected:
    /**
     * @brief Handle accept button click
     */
    void accept() override;

    /**
     * @brief Handle reject button click
     */
    void reject() override;

private slots:
    /**
     * @brief Handle apply button click
     */
    void handleApply();

    /**
     * @brief Handle reset button click
     */
    void handleReset();

    /**
     * @brief Handle defaults button click
     */
    void handleDefaults();

    /**
     * @brief Handle file path browse button
     */
    void handleBrowseFilePath();

    /**
     * @brief Handle color selection button
     */
    void handleColorSelection();

    /**
     * @brief Handle font selection button
     */
    void handleFontSelection();

    /**
     * @brief Handle settings validation
     */
    void handleSettingsValidation();

private:
    /**
     * @brief Create the UI layout
     */
    void createUI();

    /**
     * @brief Create general settings tab
     */
    void createGeneralTab();

    /**
     * @brief Create analysis settings tab
     */
    void createAnalysisTab();

    /**
     * @brief Create display settings tab
     */
    void createDisplayTab();

    /**
     * @brief Create audio settings tab
     */
    void createAudioTab();

    /**
     * @brief Create network settings tab
     */
    void createNetworkTab();

    /**
     * @brief Create performance settings tab
     */
    void createPerformanceTab();

    /**
     * @brief Create advanced settings tab
     */
    void createAdvancedTab();

    /**
     * @brief Create button box
     */
    void createButtonBox();

    /**
     * @brief Update UI controls with current settings
     */
    void updateUI();

    /**
     * @brief Validate settings values
     * @return true if all settings are valid
     */
    bool validateSettings();

    /**
     * @brief Apply theme settings
     */
    void applyTheme();

    /**
     * @brief Get default settings values
     * @return Default settings map
     */
    QVariantMap getDefaultSettings() const;

    // Main layout
    QVBoxLayout *m_mainLayout;
    QTabWidget *m_tabWidget;
    QDialogButtonBox *m_buttonBox;
    QPushButton *m_applyButton;
    QPushButton *m_resetButton;
    QPushButton *m_defaultsButton;

    // General tab
    QWidget *m_generalTab;
    QCheckBox *m_autoStartAnalysis;
    QCheckBox *m_rememberWindowState;
    QCheckBox *m_showSplashScreen;
    QCheckBox *m_checkForUpdates;
    QComboBox *m_languageCombo;
    QComboBox *m_themeCombo;
    QLineEdit *m_defaultDirectoryEdit;
    QPushButton *m_browseDirectoryButton;

    // Analysis tab
    QWidget *m_analysisTab;
    QSpinBox *m_updateRateSpin;
    QSpinBox *m_bufferSizeSpin;
    QDoubleSpinBox *m_analysisTimeoutSpin;
    QCheckBox *m_realTimeProcessingCheck;
    QCheckBox *m_enableErrorDetection;
    QCheckBox *m_enableFigAnalysis;
    QCheckBox *m_enableAudioAnalysis;
    QComboBox *m_processingModeCombo;
    QSlider *m_sensitivitySlider;

    // Display tab
    QWidget *m_displayTab;
    QSpinBox *m_frameRateSpin;
    QCheckBox *m_enableAntialiasing;
    QCheckBox *m_enableVSync;
    QCheckBox *m_showGridLines;
    QCheckBox *m_showStatusBar;
    QComboBox *m_colorSchemeCombo;
    QPushButton *m_backgroundColorButton;
    QPushButton *m_gridColorButton;
    QPushButton *m_textColorButton;
    QPushButton *m_fontButton;
    QColor m_backgroundColor;
    QColor m_gridColor;
    QColor m_textColor;
    QFont m_displayFont;

    // Audio tab
    QWidget *m_audioTab;
    QCheckBox *m_enableAudioMonitoring;
    QSlider *m_audioVolumeSlider;
    QSpinBox *m_audioBufferSizeSpin;
    QComboBox *m_audioOutputDeviceCombo;
    QComboBox *m_audioSampleRateCombo;
    QCheckBox *m_enableAudioRecording;
    QLineEdit *m_audioRecordingPathEdit;
    QPushButton *m_browseAudioPathButton;

    // Network tab
    QWidget *m_networkTab;
    QCheckBox *m_enableNetworkStreaming;
    QLineEdit *m_networkAddressEdit;
    QSpinBox *m_networkPortSpin;
    QSpinBox *m_networkTimeoutSpin;
    QSpinBox *m_maxConnectionsSpin;
    QCheckBox *m_enableMulticast;
    QLineEdit *m_multicastAddressEdit;
    QSpinBox *m_multicastPortSpin;

    // Performance tab
    QWidget *m_performanceTab;
    QSpinBox *m_memoryLimitSpin;
    QSpinBox *m_threadCountSpin;
    QCheckBox *m_enableHardwareAcceleration;
    QCheckBox *m_enableGpuAcceleration;
    QCheckBox *m_enableMultiThreading;
    QSlider *m_processingPrioritySlider;
    QCheckBox *m_enableMemoryOptimization;
    QCheckBox *m_enableCpuOptimization;

    // Advanced tab
    QWidget *m_advancedTab;
    QCheckBox *m_enableDebugLogging;
    QComboBox *m_logLevelCombo;
    QLineEdit *m_logFilePathEdit;
    QPushButton *m_browseLogPathButton;
    QCheckBox *m_enableProfiling;
    QCheckBox *m_enableCrashReporting;
    QLineEdit *m_customConfigEdit;
    QSpinBox *m_maxLogFilesSpin;
    QSpinBox *m_maxLogSizeSpin;

    // Settings state
    bool m_settingsLoaded;
    bool m_settingsChanged;
    QSettings *m_settings;

    // Constants
    static constexpr int MIN_UPDATE_RATE = 1;
    static constexpr int MAX_UPDATE_RATE = 120;
    static constexpr int DEFAULT_UPDATE_RATE = 60;
    static constexpr int MIN_BUFFER_SIZE = 10;
    static constexpr int MAX_BUFFER_SIZE = 10000;
    static constexpr int DEFAULT_BUFFER_SIZE = 1000;
    static constexpr int MIN_MEMORY_LIMIT = 50;
    static constexpr int MAX_MEMORY_LIMIT = 2048;
    static constexpr int DEFAULT_MEMORY_LIMIT = 150;
};