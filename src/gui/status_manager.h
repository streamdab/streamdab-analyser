#pragma once

#include <QObject>
#include <QLabel>
#include <QQueue>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <memory>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

/**
 * @class StatusManager
 * @brief Professional status message system with efficient queuing and display
 * 
 * This class replaces the minimal GREEN phase status label operations with a 
 * professional, memory-efficient status management system that handles high-frequency
 * status updates without blocking the UI thread.
 * 
 * Features:
 * - Thread-safe message queuing for >900 FPS processing
 * - Memory-efficient message batching and deduplication
 * - Professional status categorization (Info, Warning, Error, Success)
 * - Automatic message expiration and cleanup
 * - ETSI compliance status integration
 * - Performance-optimized batch updates
 * - Cross-platform status bar integration
 */
class StatusManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Status message priority levels
     */
    enum class StatusLevel {
        Info,       ///< Informational messages (default)
        Success,    ///< Success confirmations (green)
        Warning,    ///< Warning messages (yellow)
        Error,      ///< Error messages (red)
        Critical    ///< Critical system messages (red, persistent)
    };

    /**
     * @brief Status message structure
     */
    struct StatusMessage {
        QString text;              ///< Message text content
        StatusLevel level;         ///< Message priority level
        QString context;           ///< Context/category (e.g., "ETI Processing")
        qint64 timestamp;          ///< Message creation time
        int duration;              ///< Display duration in milliseconds
        bool persistent;           ///< Whether message persists until manually cleared
        
        StatusMessage()
            : level(StatusLevel::Info)
            , timestamp(0)
            , duration(3000)
            , persistent(false) {}
            
        StatusMessage(const QString& msg, StatusLevel lvl = StatusLevel::Info, 
                     const QString& ctx = QString(), int dur = 3000)
            : text(msg), level(lvl), context(ctx), duration(dur), persistent(false)
        {
            timestamp = QDateTime::currentMSecsSinceEpoch();
        }
    };

    /**
     * @brief Constructor
     * @param parent Parent widget for status display
     */
    explicit StatusManager(QWidget* parent = nullptr);

    /**
     * @brief Destructor with proper cleanup
     */
    ~StatusManager() override;

    /**
     * @brief Set status label widget for display updates
     * @param statusLabel Status label widget to control
     */
    void setStatusLabel(QLabel* statusLabel);

    /**
     * @brief Show status message (thread-safe)
     * @param message Message text
     * @param level Message priority level
     * @param context Optional context/category
     * @param duration Display duration in milliseconds (0 = default)
     */
    void showStatus(const QString& message, 
                   StatusLevel level = StatusLevel::Info,
                   const QString& context = QString(),
                   int duration = 0);

    /**
     * @brief Show persistent status that must be manually cleared
     * @param message Message text
     * @param level Message priority level
     * @param context Optional context/category
     */
    void showPersistentStatus(const QString& message,
                             StatusLevel level = StatusLevel::Info,
                             const QString& context = QString());

    /**
     * @brief Clear all status messages
     */
    void clearStatus();

    /**
     * @brief Clear status messages by level
     * @param level Priority level to clear
     */
    void clearStatusByLevel(StatusLevel level);

    /**
     * @brief Clear status messages by context
     * @param context Context/category to clear
     */
    void clearStatusByContext(const QString& context);

    /**
     * @brief Get current visible status message
     * @return Current status message or empty if none
     */
    StatusMessage getCurrentStatus() const;

    /**
     * @brief Check if any messages are queued
     * @return true if messages are pending display
     */
    bool hasQueuedMessages() const;

    /**
     * @brief Get performance statistics
     */
    struct StatusPerformanceMetrics {
        int messagesPerSecond;    ///< Messages processed per second
        int queueSize;            ///< Current queue size
        double averageLatency;    ///< Average display latency in milliseconds
        int droppedMessages;      ///< Messages dropped due to overload
    };
    StatusPerformanceMetrics getPerformanceMetrics() const;

public slots:
    /**
     * @brief Thread-safe slot for ETI frame processing status
     * @param frameNumber Current frame number
     * @param totalFrames Total frames in stream
     */
    void onFrameProcessed(quint64 frameNumber, quint64 totalFrames);

    /**
     * @brief Thread-safe slot for general status updates
     * @param status Status message text
     */
    void onStatusChanged(const QString& status);

    /**
     * @brief Thread-safe slot for error status display
     * @param errorMessage Error description
     * @param context Optional error context
     */
    void onErrorOccurred(const QString& errorMessage, const QString& context = QString());

    /**
     * @brief Thread-safe slot for success status display
     * @param successMessage Success description
     * @param context Optional success context
     */
    void onSuccessMessage(const QString& successMessage, const QString& context = QString());

    /**
     * @brief Thread-safe slot for ETSI compliance status
     * @param complianceStatus Compliance check result
     * @param isValid Whether check passed
     */
    void onComplianceStatus(const QString& complianceStatus, bool isValid);

signals:
    /**
     * @brief Signal emitted when status changes
     * @param message Current status message
     */
    void statusChanged(const StatusMessage& message);

    /**
     * @brief Signal emitted for critical errors requiring attention
     * @param errorMessage Critical error description
     */
    void criticalError(const QString& errorMessage);

    /**
     * @brief Signal for performance monitoring
     * @param metrics Current performance statistics
     */
    void performanceUpdate(const StatusPerformanceMetrics& metrics);

private slots:
    /**
     * @brief Process queued status messages
     */
    void processMessageQueue();

    /**
     * @brief Update status label (UI thread only)
     */
    void updateStatusLabelSafely();

    /**
     * @brief Clean up expired messages
     */
    void cleanupExpiredMessages();

private:
    // Thread-safe message queue
    mutable QMutex m_messageMutex;
    QQueue<StatusMessage> m_messageQueue;
    StatusMessage m_currentMessage;
    
    // UI components (UI thread only)
    QLabel* m_statusLabel;
    QWidget* m_parentWidget;
    
    // Performance optimization
    QTimer* m_processTimer;
    QTimer* m_cleanupTimer;
    
    // Performance tracking
    mutable QMutex m_performanceMutex;
    StatusPerformanceMetrics m_performanceMetrics;
    qint64 m_lastUpdateTime;
    int m_messageCounter;
    int m_droppedMessages;
    
    // Configuration
    static const int MAX_QUEUE_SIZE = 100;
    static const int PROCESS_INTERVAL_MS = 50;  // 20 FPS status updates
    static const int CLEANUP_INTERVAL_MS = 1000; // 1 second cleanup
    
    // Helper methods
    void enqueueMessage(const StatusMessage& message);
    QString formatStatusText(const StatusMessage& message) const;
    QString getStyleSheetForLevel(StatusLevel level) const;
    void updatePerformanceMetrics();
    bool isUIThread() const;
    void ensureUIThread(const QString& functionName) const;
    void dropOldestMessages();
};

/**
 * @brief Helper function to convert StatusLevel to human-readable string
 * @param level Status level enumeration
 * @return Human-readable level description
 */
QString statusLevelToString(StatusManager::StatusLevel level);