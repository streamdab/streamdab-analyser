#pragma once

#include <QObject>
#include <QTimer>
#include <QTime>
#include <QDateTime>
#include <QList>
#include <QHash>
#include <QString>
#include <QVariant>
#include <QMutex>
#include <memory>

// Forward declarations
namespace eti {
    struct EtiFrameData;
}
struct AudioLevelData;

/**
 * @class AlertSystem
 * @brief Professional Alert System for ETI Stream Analyzer
 * 
 * Real-time monitoring system with configurable thresholds for broadcast
 * industry standards. Provides visual and audio notifications for signal
 * quality issues, error rates, and audio level problems.
 * 
 * Features:
 * - Configurable alert thresholds for multiple parameters
 * - Real-time threshold detection with <100ms latency
 * - Professional broadcast alert standards compliance
 * - Alert cooldown system to prevent spam
 * - Alert history and logging integration
 * - Memory-efficient processing for continuous operation
 * 
 * TDD Implementation:
 * - All features implemented using Test-Driven Development
 * - Comprehensive test coverage for reliability
 * - Performance-validated for real-time operation
 */
class AlertSystem : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Alert severity levels following broadcast standards
     */
    enum class AlertLevel {
        Info = 0,      // Informational messages
        Warning = 1,   // Warning conditions that need attention
        Critical = 2,  // Critical conditions affecting service quality
        Emergency = 3  // Emergency conditions requiring immediate action
    };
    Q_ENUM(AlertLevel)

    /**
     * @brief Alert types for different monitoring categories
     */
    enum class AlertType {
        SignalQuality = 0,  // SNR, BER, constellation quality
        ErrorRate = 1,      // Frame errors, CRC failures
        AudioLevel = 2,     // Audio silence, overload, clipping
        ServiceLoss = 3,    // Service interruption, FIG errors
        SystemError = 4     // Application or hardware errors
    };
    Q_ENUM(AlertType)

    /**
     * @brief Alert data structure for history and notifications
     */
    struct AlertData {
        AlertLevel level;
        AlertType type;
        QString message;
        QDateTime timestamp;
        QVariantHash parameters;
        
        AlertData() = default;
        AlertData(AlertLevel lvl, AlertType tp, const QString& msg, 
                 const QDateTime& time = QDateTime::currentDateTime())
            : level(lvl), type(tp), message(msg), timestamp(time) {}
    };

    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit AlertSystem(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~AlertSystem();

    /**
     * @brief Initialize the alert system
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * @brief Check if alert system is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Enable/disable real-time monitoring
     * @param enabled true to enable monitoring
     * @return true if successful
     */
    bool enableMonitoring(bool enabled);

    /**
     * @brief Check if monitoring is currently enabled
     * @return true if monitoring is active
     */
    bool isMonitoringEnabled() const { return m_monitoringEnabled; }

    /**
     * @brief Get current number of active alerts
     * @return Number of active alerts
     */
    int getActiveAlertCount() const { return m_activeAlerts.size(); }

    // ========================================================================
    // THRESHOLD CONFIGURATION
    // ========================================================================

    /**
     * @brief Set signal quality threshold
     * @param level Alert level for this threshold
     * @param threshold SNR threshold in dB (must be >= 0)
     * @return true if threshold set successfully
     */
    bool setSignalQualityThreshold(AlertLevel level, double threshold);

    /**
     * @brief Get signal quality threshold
     * @param level Alert level
     * @return Threshold value in dB SNR
     */
    double getSignalQualityThreshold(AlertLevel level) const;

    /**
     * @brief Set error rate threshold
     * @param level Alert level for this threshold
     * @param threshold BER threshold (0.0 to 1.0)
     * @return true if threshold set successfully
     */
    bool setErrorRateThreshold(AlertLevel level, double threshold);

    /**
     * @brief Get error rate threshold
     * @param level Alert level
     * @return Threshold value as BER ratio
     */
    double getErrorRateThreshold(AlertLevel level) const;

    /**
     * @brief Set audio level threshold
     * @param level Alert level for this threshold
     * @param threshold Audio level threshold in dBFS (must be <= 0)
     * @return true if threshold set successfully
     */
    bool setAudioLevelThreshold(AlertLevel level, double threshold);

    /**
     * @brief Get audio level threshold
     * @param level Alert level
     * @return Threshold value in dBFS
     */
    double getAudioLevelThreshold(AlertLevel level) const;

    /**
     * @brief Set alert cooldown period to prevent duplicate alerts
     * @param milliseconds Cooldown period in milliseconds
     */
    void setAlertCooldownPeriod(int milliseconds) { m_cooldownMs = milliseconds; }

    /**
     * @brief Get current alert cooldown period
     * @return Cooldown period in milliseconds
     */
    int getAlertCooldownPeriod() const { return m_cooldownMs; }

    // ========================================================================
    // REAL-TIME PROCESSING
    // ========================================================================

    /**
     * @brief Process ETI frame for alert conditions
     * @param frame ETI frame data containing signal quality metrics
     */
    void processEtiFrame(const eti::EtiFrameData& frame);

    /**
     * @brief Process audio data for level alerts
     * @param audioData Audio level measurements
     */
    void processAudioData(const AudioLevelData& audioData);

    // ========================================================================
    // ALERT MANAGEMENT
    // ========================================================================

    /**
     * @brief Get alert history
     * @param maxEntries Maximum number of recent entries (default: 100)
     * @return List of recent alerts
     */
    QList<AlertData> getAlertHistory(int maxEntries = 100) const;

    /**
     * @brief Clear alert history
     */
    void clearAlertHistory();

    /**
     * @brief Acknowledge an alert (removes from active list)
     * @param alertId Alert identifier
     * @return true if alert was acknowledged
     */
    bool acknowledgeAlert(int alertId);

    /**
     * @brief Clear all active alerts
     */
    void clearActiveAlerts();

    // ========================================================================
    // CONFIGURATION PERSISTENCE
    // ========================================================================

    /**
     * @brief Save alert configuration to file
     * @param filename Configuration file path
     * @return true if saved successfully
     */
    bool saveConfiguration(const QString& filename) const;

    /**
     * @brief Load alert configuration from file
     * @param filename Configuration file path
     * @return true if loaded successfully
     */
    bool loadConfiguration(const QString& filename);

    // ========================================================================
    // BROADCAST STANDARDS COMPLIANCE
    // ========================================================================

    /**
     * @brief Validate alert level according to broadcast standards
     * @param level Alert level to validate
     * @return true if valid
     */
    static bool isValidAlertLevel(AlertLevel level);

    /**
     * @brief Validate alert type according to broadcast standards
     * @param type Alert type to validate
     * @return true if valid
     */
    static bool isValidAlertType(AlertType type);

    /**
     * @brief Format alert message according to broadcast standards
     * @param level Alert level
     * @param type Alert type
     * @param parameters Alert-specific parameters
     * @return Formatted alert message
     */
    static QString formatAlertMessage(AlertLevel level, AlertType type, 
                                    const QVariantHash& parameters);

    // ========================================================================
    // PERFORMANCE MONITORING
    // ========================================================================

    /**
     * @brief Get current memory usage of alert system
     * @return Memory usage in bytes
     */
    qint64 getMemoryUsage() const;

    /**
     * @brief Get processing statistics
     * @return Hash containing performance metrics
     */
    QVariantHash getProcessingStatistics() const;

    // ========================================================================
    // GUI INTEGRATION SUPPORT
    // ========================================================================

    /**
     * @brief Check if alert system supports GUI integration
     * @return true if GUI integration is available
     */
    bool supportsGuiIntegration() const { return true; }

    /**
     * @brief Check if alert system can connect to main window
     * @return true if connection is possible
     */
    bool canConnectToMainWindow() const { return true; }

signals:
    /**
     * @brief Emitted when monitoring starts
     */
    void monitoringStarted();

    /**
     * @brief Emitted when monitoring stops
     */
    void monitoringStopped();

    /**
     * @brief Emitted when an alert is triggered
     * @param level Alert severity level
     * @param type Alert type/category
     * @param message Formatted alert message
     * @param parameters Additional alert parameters
     */
    void alertTriggered(AlertLevel level, AlertType type, const QString& message, 
                       const QVariantHash& parameters = QVariantHash());

    /**
     * @brief Emitted when an alert is acknowledged
     * @param alertId Alert identifier
     */
    void alertAcknowledged(int alertId);

    /**
     * @brief Emitted when alert history is cleared
     */
    void alertHistoryCleared();

    /**
     * @brief Emitted when configuration changes
     */
    void configurationChanged();

private slots:
    /**
     * @brief Process pending alerts from queue
     */
    void processPendingAlerts();

private:
    /**
     * @brief Check if alert should be triggered based on thresholds
     * @param level Alert level to check
     * @param type Alert type
     * @param currentValue Current measured value
     * @param threshold Configured threshold
     * @param isAboveThreshold true if alert when above threshold, false if below
     * @return true if alert should be triggered
     */
    bool shouldTriggerAlert(AlertLevel level, AlertType type, 
                           double currentValue, double threshold, 
                           bool isAboveThreshold) const;

    /**
     * @brief Trigger an alert with specified parameters
     * @param level Alert level
     * @param type Alert type
     * @param parameters Alert parameters
     */
    void triggerAlert(AlertLevel level, AlertType type, 
                     const QVariantHash& parameters);

    /**
     * @brief Check if alert is in cooldown period
     * @param type Alert type
     * @return true if in cooldown
     */
    bool isInCooldown(AlertType type) const;

    /**
     * @brief Update cooldown timestamp for alert type
     * @param type Alert type
     */
    void updateCooldown(AlertType type);

    /**
     * @brief Load default configuration values
     */
    void loadDefaultConfiguration();

    // Member variables
    bool m_initialized;
    bool m_monitoringEnabled;

    // Threshold configuration
    QHash<AlertLevel, double> m_signalQualityThresholds;
    QHash<AlertLevel, double> m_errorRateThresholds;
    QHash<AlertLevel, double> m_audioLevelThresholds;

    // Alert management
    QList<AlertData> m_alertHistory;
    QHash<int, AlertData> m_activeAlerts;
    int m_nextAlertId;

    // Cooldown management
    QHash<AlertType, QDateTime> m_lastAlertTime;
    int m_cooldownMs;

    // Performance monitoring
    mutable QMutex m_mutex;
    QTimer* m_processingTimer;
    qint64 m_processedFrameCount;
    qint64 m_triggeredAlertCount;

    // Constants
    static constexpr int DEFAULT_COOLDOWN_MS = 5000;     // 5 seconds
    static constexpr int MAX_HISTORY_ENTRIES = 1000;    // Maximum alert history
    static constexpr int PROCESSING_INTERVAL_MS = 16;   // ~60 FPS processing
    static constexpr double MIN_SIGNAL_QUALITY = 0.0;   // Minimum valid SNR
    static constexpr double MAX_SIGNAL_QUALITY = 50.0;  // Maximum valid SNR
    static constexpr double MIN_ERROR_RATE = 0.0;       // Minimum BER
    static constexpr double MAX_ERROR_RATE = 1.0;       // Maximum BER
    static constexpr double MIN_AUDIO_LEVEL = -120.0;   // Minimum dBFS
    static constexpr double MAX_AUDIO_LEVEL = 0.0;      // Maximum dBFS
};

// Register metatypes for Qt signals
Q_DECLARE_METATYPE(AlertSystem::AlertLevel)
Q_DECLARE_METATYPE(AlertSystem::AlertType)
Q_DECLARE_METATYPE(AlertSystem::AlertData)