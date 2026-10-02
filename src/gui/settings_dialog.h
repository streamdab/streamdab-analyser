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
#include "../core/analyzer_settings.hpp"

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

    /**
     * @brief Emitted when the Analyzer / Decode options change and are applied.
     *
     * Consumers (analyzer decode paths) should re-read the settings from
     * QSettings; at minimum they are honored on the next file load.
     */
    void analyzerSettingsChanged();

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
     * @brief Create the user interface
     */
    void createUI();

    /**
     * @brief Create individual tabs
     */
    void createGeneralTab();
    void createAnalysisTab();
    void createDisplayTab();
    void createAudioTab();
    void createNetworkTab();
    void createPerformanceTab();
    void createAdvancedTab();
    void createAnalyzerTab();

    /**
     * @brief Initialize combo boxes with values
     */
    void initializeComboBoxes();

    /**
     * @brief Update UI controls with current settings
     */
    void updateUI();

    /**
     * @brief Update color button appearance
     */
    void updateColorButtons();

    /**
     * @brief Validate settings values
     * @return true if all settings are valid
     */
    bool validateSettings();

    /**
     * @brief Get default settings values
     * @return Default settings map
     */
    QVariantMap getDefaultSettings() const;

    // UI components
    QTabWidget *m_tabWidget;
    QDialogButtonBox *m_buttonBox;
    
    // General tab
    QWidget *m_generalTab;
    QGroupBox *m_appearanceGroup;
    QGroupBox *m_behaviorGroup;
    QComboBox *m_themeCombo;
    QCheckBox *m_autoSaveCheck;
    QCheckBox *m_confirmExitCheck;
    QSpinBox *m_autoSaveIntervalSpin;
    
    // Analysis tab
    QWidget *m_analysisTab;
    QGroupBox *m_performanceGroup;
    QGroupBox *m_thresholdGroup;
    QSpinBox *m_updateRateSpin;
    QSpinBox *m_bufferSizeSpin;
    QCheckBox *m_realTimeCheck;
    QCheckBox *m_hardwareAccelCheck;
    
    // Display tab
    QWidget *m_displayTab;
    QGroupBox *m_colorsGroup;
    QGroupBox *m_fontsGroup;
    QPushButton *m_backgroundColorButton;
    QPushButton *m_gridColorButton;
    QPushButton *m_textColorButton;
    QPushButton *m_fontButton;
    
    // Audio tab
    QWidget *m_audioTab;
    QGroupBox *m_audioDeviceGroup;
    QGroupBox *m_levelGroup;
    QComboBox *m_audioDeviceCombo;
    QSlider *m_volumeSlider;
    QCheckBox *m_audioMonitorCheck;
    
    // Network tab
    QWidget *m_networkTab;
    QGroupBox *m_connectionGroup;
    QLineEdit *m_proxyHostEdit;
    QSpinBox *m_proxyPortSpin;
    QCheckBox *m_useProxyCheck;
    
    // Performance tab
    QWidget *m_performanceTab;
    QGroupBox *m_memoryGroup;
    QSpinBox *m_memoryLimitSpin;
    QComboBox *m_priorityCombo;
    
    // Advanced tab
    QWidget *m_advancedTab;
    QGroupBox *m_debugGroup;
    QCheckBox *m_debugModeCheck;
    QComboBox *m_logLevelCombo;

    // Analyzer / Decode options tab (option-variant matrix rows 1-14)
    QWidget *m_analyzerTab;
    // Row A (charset)
    QSpinBox *m_forceCharsetSpin;
    QComboBox *m_charsetFallbackCombo;
    // Row B (ETI format)
    QComboBox *m_etiModeCombo;
    // Row C (NST legacy correction)
    QCheckBox *m_nstOffset1Check;
    // Row D/E (FIC framework + FIB CRC)
    QComboBox *m_ficModeCombo;
    QCheckBox *m_fibIgnoreCrcCheck;
    // Row F/G (ECC override + per-service deep parse)
    QSpinBox *m_eccOverrideSpin;
    QCheckBox *m_serviceEccDeepParseCheck;
    // Row H (label source + short label)
    QComboBox *m_labelSourceCombo;
    QCheckBox *m_showShortLabelsCheck;
    // Row I (timestamp source)
    QComboBox *m_timestampSourceCombo;
    // Row J (strict frame CRC)
    QCheckBox *m_strictFrameCrcCheck;
    // Row K (SId display)
    QComboBox *m_sidDisplayCombo;

    // Custom members for business logic
    QColor m_backgroundColor;
    QColor m_gridColor;
    QColor m_textColor;
    QFont m_displayFont;

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