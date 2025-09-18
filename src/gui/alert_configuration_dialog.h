/**
 * @file alert_configuration_dialog.h
 * @brief Professional Alert Configuration Dialog for ETI Stream Analyzer
 * 
 * Provides comprehensive GUI for configuring alert thresholds, notification settings,
 * and broadcast-standard alert behavior. Integrates with AlertSystem backend for
 * real-time monitoring configuration.
 * 
 * @author UI/UX Agent - Phase 4 Integration
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyzer Team
 */

#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QProgressBar>
#include <QTextEdit>
#include <QListWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QSplitter>
#include <QTimer>
#include <QMutex>
#include <memory>

#include "core/alert_system.h"

/**
 * @class AlertConfigurationDialog
 * @brief Professional broadcast-standard alert configuration interface
 * 
 * Features:
 * - Multi-tab interface for different alert categories
 * - Real-time threshold testing with live signal
 * - Visual threshold indicators and status displays
 * - Professional broadcast color scheme
 * - Configuration import/export capabilities
 * - Live preview of alert conditions
 * - Audio/visual notification configuration
 * - Alert history and statistics display
 */
class AlertConfigurationDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param alertSystem Pointer to the alert system instance
     * @param parent Parent widget
     */
    explicit AlertConfigurationDialog(AlertSystem *alertSystem, QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~AlertConfigurationDialog();

    /**
     * @brief Initialize the dialog with current alert system configuration
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * @brief Get the configured alert system
     * @return Pointer to alert system
     */
    AlertSystem* getAlertSystem() const { return m_alertSystem; }

public slots:
    /**
     * @brief Apply current configuration to alert system
     */
    void applyConfiguration();

    /**
     * @brief Reset configuration to defaults
     */
    void resetToDefaults();

    /**
     * @brief Load configuration from file
     */
    void loadConfiguration();

    /**
     * @brief Save configuration to file
     */
    void saveConfiguration();

    /**
     * @brief Test current alert thresholds with live signal
     */
    void testAlertThresholds();

    /**
     * @brief Preview alert notification
     * @param level Alert level to preview
     * @param type Alert type to preview
     */
    void previewAlert(AlertSystem::AlertLevel level, AlertSystem::AlertType type);

signals:
    /**
     * @brief Emitted when configuration is applied
     */
    void configurationApplied();

    /**
     * @brief Emitted when configuration is reset
     */
    void configurationReset();

    /**
     * @brief Emitted when test alert is triggered
     * @param level Alert level
     * @param type Alert type
     * @param message Test message
     */
    void testAlertTriggered(AlertSystem::AlertLevel level, AlertSystem::AlertType type, 
                           const QString& message);

protected:
    /**
     * @brief Handle accept (OK button)
     */
    void accept() override;

    /**
     * @brief Handle reject (Cancel button)
     */
    void reject() override;

private slots:
    /**
     * @brief Handle signal quality threshold change
     */
    void handleSignalQualityChanged();

    /**
     * @brief Handle error rate threshold change
     */
    void handleErrorRateChanged();

    /**
     * @brief Handle audio level threshold change
     */
    void handleAudioLevelChanged();

    /**
     * @brief Handle cooldown period change
     */
    void handleCooldownChanged();

    /**
     * @brief Handle alert level selection change
     */
    void handleAlertLevelChanged();

    /**
     * @brief Handle notification settings change
     */
    void handleNotificationSettingsChanged();

    /**
     * @brief Handle alert history selection
     */
    void handleAlertHistorySelection();

    /**
     * @brief Update live preview displays
     */
    void updateLivePreviews();

private:
    /**
     * @brief Create the dialog UI
     */
    void createUI();

    /**
     * @brief Create threshold configuration tab
     */
    void createThresholdTab();

    /**
     * @brief Create notification settings tab
     */
    void createNotificationTab();

    /**
     * @brief Create alert history tab
     */
    void createHistoryTab();

    /**
     * @brief Create advanced settings tab
     */
    void createAdvancedTab();

    /**
     * @brief Create button box
     */
    void createButtonBox();

    /**
     * @brief Setup threshold controls for specific alert type
     * @param parent Parent widget
     * @param type Alert type
     * @param title Group title
     */
    void setupThresholdControls(QWidget *parent, AlertSystem::AlertType type, const QString& title);

    /**
     * @brief Create visual threshold indicator
     * @param parent Parent widget
     * @param title Indicator title
     * @param minValue Minimum value
     * @param maxValue Maximum value
     * @param unit Value unit
     * @return Created indicator widget
     */
    QWidget* createThresholdIndicator(QWidget *parent, const QString& title,
                                     double minValue, double maxValue, const QString& unit);

    /**
     * @brief Load current configuration from alert system
     */
    void loadCurrentConfiguration();

    /**
     * @brief Apply UI settings to alert system
     */
    void applyUISettings();

    /**
     * @brief Update threshold displays with current values
     */
    void updateThresholdDisplays();

    /**
     * @brief Update notification controls
     */
    void updateNotificationControls();

    /**
     * @brief Update alert history display
     */
    void updateAlertHistoryDisplay();

    /**
     * @brief Validate configuration settings
     * @return true if configuration is valid
     */
    bool validateConfiguration() const;

    /**
     * @brief Apply professional broadcast styling
     */
    void applyBroadcastStyling();

    // Core components
    AlertSystem *m_alertSystem;

    // Main layout
    QVBoxLayout *m_mainLayout;
    QTabWidget *m_tabWidget;

    // Threshold configuration tab
    QWidget *m_thresholdTab;
    QGroupBox *m_signalQualityGroup;
    QGroupBox *m_errorRateGroup;
    QGroupBox *m_audioLevelGroup;

    // Signal quality controls
    QDoubleSpinBox *m_signalInfoThreshold;
    QDoubleSpinBox *m_signalWarningThreshold;
    QDoubleSpinBox *m_signalCriticalThreshold;
    QDoubleSpinBox *m_signalEmergencyThreshold;
    QProgressBar *m_signalQualityIndicator;
    QLabel *m_signalStatusLabel;

    // Error rate controls
    QDoubleSpinBox *m_errorInfoThreshold;
    QDoubleSpinBox *m_errorWarningThreshold;
    QDoubleSpinBox *m_errorCriticalThreshold;
    QDoubleSpinBox *m_errorEmergencyThreshold;
    QProgressBar *m_errorRateIndicator;
    QLabel *m_errorStatusLabel;

    // Audio level controls
    QDoubleSpinBox *m_audioInfoThreshold;
    QDoubleSpinBox *m_audioWarningThreshold;
    QDoubleSpinBox *m_audioCriticalThreshold;
    QDoubleSpinBox *m_audioEmergencyThreshold;
    QProgressBar *m_audioLevelIndicator;
    QLabel *m_audioStatusLabel;

    // Notification settings tab
    QWidget *m_notificationTab;
    QGroupBox *m_visualNotificationGroup;
    QGroupBox *m_audioNotificationGroup;
    QGroupBox *m_cooldownGroup;

    // Visual notification controls
    QCheckBox *m_enableVisualAlerts;
    QCheckBox *m_enableFlashingAlerts;
    QCheckBox *m_enableColorCoding;
    QComboBox *m_alertColorScheme;
    QSpinBox *m_alertDisplayDuration;

    // Audio notification controls
    QCheckBox *m_enableAudioAlerts;
    QComboBox *m_alertSoundScheme;
    QSlider *m_alertVolumeSlider;
    QPushButton *m_testSoundButton;

    // Cooldown controls
    QSpinBox *m_cooldownPeriod;
    QCheckBox *m_enableCooldownByType;
    QCheckBox *m_enableAlertEscalation;

    // Alert history tab
    QWidget *m_historyTab;
    QSplitter *m_historySplitter;
    QTableWidget *m_alertHistoryTable;
    QTextEdit *m_alertDetailsText;
    QPushButton *m_clearHistoryButton;
    QPushButton *m_exportHistoryButton;

    // Advanced settings tab
    QWidget *m_advancedTab;
    QGroupBox *m_performanceGroup;
    QGroupBox *m_integrationGroup;

    // Performance controls
    QSpinBox *m_processingInterval;
    QSpinBox *m_maxHistoryEntries;
    QCheckBox *m_enableMemoryOptimization;
    QProgressBar *m_memoryUsageIndicator;

    // Integration controls
    QCheckBox *m_enableMainWindowIntegration;
    QCheckBox *m_enableStatusBarAlerts;
    QCheckBox *m_enableDockPanelAlerts;
    QCheckBox *m_enableToolbarIndicators;

    // Button box
    QHBoxLayout *m_buttonLayout;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;
    QPushButton *m_applyButton;
    QPushButton *m_resetButton;
    QPushButton *m_testButton;
    QPushButton *m_loadButton;
    QPushButton *m_saveButton;

    // Live preview and testing
    QTimer *m_previewTimer;
    QMutex m_previewMutex;
    bool m_previewEnabled;

    // Configuration state
    bool m_configurationChanged;
    bool m_isInitialized;

    // Constants for broadcast standards
    static constexpr double MIN_SIGNAL_QUALITY_DB = 0.0;
    static constexpr double MAX_SIGNAL_QUALITY_DB = 50.0;
    static constexpr double DEFAULT_SIGNAL_WARNING_DB = 15.0;
    static constexpr double DEFAULT_SIGNAL_CRITICAL_DB = 10.0;
    static constexpr double DEFAULT_SIGNAL_EMERGENCY_DB = 5.0;

    static constexpr double MIN_ERROR_RATE = 0.0;
    static constexpr double MAX_ERROR_RATE = 1.0;
    static constexpr double DEFAULT_ERROR_WARNING = 0.01;    // 1%
    static constexpr double DEFAULT_ERROR_CRITICAL = 0.05;   // 5%
    static constexpr double DEFAULT_ERROR_EMERGENCY = 0.10;  // 10%

    static constexpr double MIN_AUDIO_LEVEL_DBFS = -120.0;
    static constexpr double MAX_AUDIO_LEVEL_DBFS = 0.0;
    static constexpr double DEFAULT_AUDIO_WARNING_DBFS = -18.0;
    static constexpr double DEFAULT_AUDIO_CRITICAL_DBFS = -12.0;
    static constexpr double DEFAULT_AUDIO_EMERGENCY_DBFS = -6.0;

    static constexpr int DEFAULT_COOLDOWN_MS = 5000;         // 5 seconds
    static constexpr int MIN_COOLDOWN_MS = 1000;             // 1 second
    static constexpr int MAX_COOLDOWN_MS = 300000;           // 5 minutes

    static constexpr int PREVIEW_UPDATE_INTERVAL_MS = 100;   // 100ms updates
    static constexpr int MAX_HISTORY_DISPLAY_ENTRIES = 1000; // Maximum history entries
};