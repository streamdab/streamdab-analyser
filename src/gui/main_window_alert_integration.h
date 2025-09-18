/**
 * @file main_window_alert_integration.h
 * @brief MainWindow Alert System Integration Extensions
 * 
 * Extends MainWindow with comprehensive Alert System integration including
 * GUI elements, real-time notifications, and professional broadcast workflows.
 * 
 * @author UI/UX Agent - Phase 4 Integration
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyzer Team
 */

#pragma once

#include "main_window.h"
#include "alert_configuration_dialog.h"
#include "core/alert_system.h"
#include "core/stream_recording_manager.h"
#include "core/audio_extraction_pipeline.h"

#include <QDockWidget>
#include <QListWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSystemTrayIcon>
#include <QMenu>
#include <memory>

/**
 * @class MainWindowAlertIntegration
 * @brief Extension class for MainWindow Alert System integration
 * 
 * This class extends the MainWindow with comprehensive Alert System integration:
 * - Alert status dock panel with real-time notifications
 * - Toolbar alert indicators with visual status
 * - System tray integration for background alerts
 * - Alert configuration dialog management
 * - Professional broadcast workflow integration
 * - Stream recording and audio extraction controls
 */
class MainWindowAlertIntegration : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param mainWindow Parent MainWindow instance
     * @param parent QObject parent
     */
    explicit MainWindowAlertIntegration(MainWindow *mainWindow, QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~MainWindowAlertIntegration();

    /**
     * @brief Initialize alert system integration
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * @brief Check if integration is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Get alert system instance
     * @return Pointer to alert system
     */
    AlertSystem* getAlertSystem() const { return m_alertSystem.get(); }

    /**
     * @brief Get stream recording manager
     * @return Pointer to recording manager
     */
    eti::StreamRecordingManager* getRecordingManager() const { return m_recordingManager.get(); }

    /**
     * @brief Get audio extraction pipeline
     * @return Pointer to extraction pipeline
     */
    eti::AudioExtractionPipeline* getAudioPipeline() const { return m_audioPipeline.get(); }

    /**
     * @brief Get alert configuration dialog
     * @return Pointer to configuration dialog
     */
    AlertConfigurationDialog* getConfigurationDialog() const { return m_configDialog.get(); }

public slots:
    /**
     * @brief Show alert configuration dialog
     */
    void showAlertConfiguration();

    /**
     * @brief Show/hide alert dock panel
     * @param visible Visibility state
     */
    void setAlertDockVisible(bool visible);

    /**
     * @brief Clear all active alerts
     */
    void clearAllAlerts();

    /**
     * @brief Acknowledge selected alert
     */
    void acknowledgeSelectedAlert();

    /**
     * @brief Toggle alert monitoring
     */
    void toggleAlertMonitoring();

    /**
     * @brief Show recording controls dialog
     */
    void showRecordingControls();

    /**
     * @brief Show audio extraction dialog
     */
    void showAudioExtractionDialog();

    /**
     * @brief Start alert system monitoring
     */
    void startAlertMonitoring();

    /**
     * @brief Stop alert system monitoring
     */
    void stopAlertMonitoring();

signals:
    /**
     * @brief Emitted when alert system status changes
     * @param monitoring true if monitoring is active
     */
    void alertMonitoringChanged(bool monitoring);

    /**
     * @brief Emitted when new alert is received
     * @param level Alert level
     * @param type Alert type
     * @param message Alert message
     */
    void alertReceived(AlertSystem::AlertLevel level, AlertSystem::AlertType type, 
                      const QString& message);

    /**
     * @brief Emitted when alert is acknowledged
     * @param alertId Alert identifier
     */
    void alertAcknowledged(int alertId);

private slots:
    /**
     * @brief Handle alert triggered from alert system
     * @param level Alert level
     * @param type Alert type
     * @param message Alert message
     * @param parameters Alert parameters
     */
    void handleAlertTriggered(AlertSystem::AlertLevel level, AlertSystem::AlertType type, 
                             const QString& message, const QVariantHash& parameters);

    /**
     * @brief Handle alert acknowledged from alert system
     * @param alertId Alert identifier
     */
    void handleAlertAcknowledged(int alertId);

    /**
     * @brief Handle system tray activation
     * @param reason Activation reason
     */
    void handleSystemTrayActivation(QSystemTrayIcon::ActivationReason reason);

    /**
     * @brief Update alert status display
     */
    void updateAlertStatusDisplay();

    /**
     * @brief Update toolbar alert indicators
     */
    void updateToolbarIndicators();

    /**
     * @brief Handle alert list selection change
     */
    void handleAlertListSelectionChanged();

    /**
     * @brief Handle test alert from configuration dialog
     * @param level Alert level
     * @param type Alert type
     * @param message Test message
     */
    void handleTestAlert(AlertSystem::AlertLevel level, AlertSystem::AlertType type, 
                        const QString& message);

private:
    /**
     * @brief Create alert dock panel
     */
    void createAlertDockPanel();

    /**
     * @brief Create toolbar alert indicators
     */
    void createToolbarIndicators();

    /**
     * @brief Create system tray integration
     */
    void createSystemTrayIntegration();

    /**
     * @brief Setup alert system connections
     */
    void setupAlertSystemConnections();

    /**
     * @brief Create recording controls toolbar
     */
    void createRecordingToolbar();

    /**
     * @brief Create audio extraction controls
     */
    void createAudioExtractionControls();

    /**
     * @brief Apply professional broadcast styling
     */
    void applyBroadcastStyling();

    /**
     * @brief Create alert status widget
     * @param title Widget title
     * @param parent Parent widget
     * @return Created status widget
     */
    QWidget* createAlertStatusWidget(const QString& title, QWidget* parent);

    /**
     * @brief Update alert status widget
     * @param widget Status widget to update
     * @param level Current alert level
     * @param count Number of alerts
     */
    void updateAlertStatusWidget(QWidget* widget, AlertSystem::AlertLevel level, int count);

    /**
     * @brief Get alert level color
     * @param level Alert level
     * @return Color for the alert level
     */
    QColor getAlertLevelColor(AlertSystem::AlertLevel level) const;

    /**
     * @brief Get alert level icon
     * @param level Alert level
     * @return Icon for the alert level
     */
    QIcon getAlertLevelIcon(AlertSystem::AlertLevel level) const;

    /**
     * @brief Format alert message for display
     * @param level Alert level
     * @param type Alert type
     * @param message Original message
     * @return Formatted message
     */
    QString formatAlertMessage(AlertSystem::AlertLevel level, AlertSystem::AlertType type, 
                              const QString& message) const;

    // Core components
    MainWindow *m_mainWindow;
    std::unique_ptr<AlertSystem> m_alertSystem;
    std::unique_ptr<eti::StreamRecordingManager> m_recordingManager;
    std::unique_ptr<eti::AudioExtractionPipeline> m_audioPipeline;

    // GUI components
    std::unique_ptr<AlertConfigurationDialog> m_configDialog;
    QDockWidget *m_alertDock;
    QWidget *m_alertDockContent;
    QVBoxLayout *m_alertDockLayout;

    // Alert status widgets
    QGroupBox *m_alertStatusGroup;
    QListWidget *m_activeAlertsList;
    QLabel *m_alertCountLabel;
    QProgressBar *m_alertLevelIndicator;
    QPushButton *m_clearAlertsButton;
    QPushButton *m_acknowledgeButton;
    QPushButton *m_configureButton;

    // Alert status summary widgets
    QWidget *m_infoStatusWidget;
    QWidget *m_warningStatusWidget;
    QWidget *m_criticalStatusWidget;
    QWidget *m_emergencyStatusWidget;

    // Toolbar indicators
    QAction *m_alertMonitoringAction;
    QLabel *m_alertStatusLabel;
    QProgressBar *m_alertStatusIndicator;
    QPushButton *m_alertConfigButton;

    // Recording controls
    QToolBar *m_recordingToolbar;
    QAction *m_startRecordingAction;
    QAction *m_stopRecordingAction;
    QAction *m_pauseRecordingAction;
    QAction *m_recordingSettingsAction;
    QLabel *m_recordingStatusLabel;

    // Audio extraction controls
    QToolBar *m_audioToolbar;
    QAction *m_startAudioExtractionAction;
    QAction *m_stopAudioExtractionAction;
    QAction *m_audioExtractionSettingsAction;
    QLabel *m_audioExtractionStatusLabel;

    // System tray integration
    QSystemTrayIcon *m_systemTray;
    QMenu *m_systemTrayMenu;
    QAction *m_showMainWindowAction;
    QAction *m_showAlertsAction;
    QAction *m_exitAction;

    // Update timers
    QTimer *m_statusUpdateTimer;
    QTimer *m_indicatorUpdateTimer;

    // State tracking
    bool m_initialized;
    bool m_alertMonitoringEnabled;
    int m_totalAlertCount;
    int m_activeAlertCount;
    QHash<AlertSystem::AlertLevel, int> m_alertCountByLevel;

    // Constants
    static constexpr int STATUS_UPDATE_INTERVAL_MS = 1000;    // 1 second
    static constexpr int INDICATOR_UPDATE_INTERVAL_MS = 250;  // 4 FPS for smooth indicators
    static constexpr int MAX_DISPLAYED_ALERTS = 100;          // Maximum alerts in list
    static constexpr int ALERT_ANIMATION_DURATION_MS = 500;   // Alert animation duration
};