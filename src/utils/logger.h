#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <QQueue>
#include <QVariantHash>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <QCoreApplication>
#include <QStandardPaths>
#include <memory>
#include <atomic>

/**
 * @class Logger
 * @brief HANG-SAFE Professional Logging Framework for ETI Stream Analyser
 * 
 * This is a hang-resistant implementation that maintains 100% API compatibility
 * with the original Logger while preventing the Logger::instance() hang issue.
 * 
 * Key Safety Features:
 * - Immediate fallback to qDebug() when full logger unavailable
 * - Lazy initialization with Qt application context checks
 * - Zero application hang guarantee
 * - Thread-safe operation with deadlock prevention
 * - Professional audit trail capabilities when available
 */
class Logger : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Modern log severity levels
     */
    enum class LogLevel : std::uint8_t {
        Debug = 0,     // Detailed debugging information
        Info = 1,      // Informational messages
        Warning = 2,   // Warning conditions
        Error = 3,     // Error conditions
        Critical = 4   // Critical conditions requiring immediate attention
    };
    Q_ENUM(LogLevel)

    /**
     * @brief Legacy enum for GUI compatibility
     */
    enum LegacyLogLevel : std::uint8_t {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        Critical = 4
    };

    /**
     * @brief Get singleton instance with hang-resistant implementation
     * @return Logger singleton reference
     *
     * @note Thread affinity (W1 #9). The returned object is affined to the
     *       thread that first calls instance(). logInternal() emits
     *       logMessageAdded through QTimer::singleShot(0, this, …), i.e. it
     *       marshals to the Logger's OWN thread and needs that thread's event
     *       loop to deliver the signal. Construct therefore MUST happen on the
     *       main thread: the GUI main() explicitly calls Logger::instance()
     *       right after creating QApplication (before any window/worker), and
     *       any other entry point that consumes logMessageAdded must do the
     *       same. A singleton created on a worker without an event loop would
     *       silently drop every GUI log event.
     */
    static Logger& instance();

    /**
     * @brief Initialize the logging system (safe, non-blocking)
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * @brief Check if logging system is fully initialized
     * @return true if fully initialized
     */
    bool isInitialized() const { return m_fullyInitialized.load(); }

    /**
     * @brief Enable/disable logging
     * @param enabled true to enable logging
     * @return true if successful
     */
    bool enableLogging(bool enabled);

    /**
     * @brief Check if logging is currently enabled
     * @return true if logging is active
     */
    bool isLoggingEnabled() const { return m_loggingEnabled.load(); }

    // ========================================================================
    // LEGACY INTERFACE FOR GUI COMPATIBILITY
    // ========================================================================

    /**
     * @brief Legacy logging method for existing GUI code
     * @param level Log level (using legacy enum)
     * @param category Message category
     * @param message Message text
     */
    void log(LegacyLogLevel level, const QString& category, const QString& message);

    /**
     * @brief Set log level using legacy enum
     * @param level Log level
     */
    void setLogLevel(LegacyLogLevel level);

    // ========================================================================
    // MODERN INTERFACE
    // ========================================================================

    /**
     * @brief Modern logging method with immediate fallback
     * @param level Log level (using modern enum class)
     * @param message Message text
     * @param category Optional category
     * @param metadata Optional metadata
     */
    void log(LogLevel level, const QString& message, const QString& category = QString(), 
             const QVariantHash& metadata = QVariantHash());

    /**
     * @brief Set minimum log level for output
     * @param level Minimum level to log
     */
    void setLogLevel(LogLevel level) { m_logLevel.store(static_cast<int>(level)); }

    /**
     * @brief Get current log level
     * @return Current minimum log level
     */
    LogLevel getLogLevel() const { return static_cast<LogLevel>(m_logLevel.load()); }

    /**
     * @brief Log debug message with fallback
     * @param message Message text
     * @param category Optional category
     */
    void logDebug(const QString& message, const QString& category = QString());

    /**
     * @brief Log info message with fallback
     * @param message Message text
     * @param category Optional category
     */
    void logInfo(const QString& message, const QString& category = QString());

    /**
     * @brief Log warning message with fallback
     * @param message Message text
     * @param category Optional category
     */
    void logWarning(const QString& message, const QString& category = QString());

    /**
     * @brief Log error message with fallback
     * @param message Message text
     * @param category Optional category
     */
    void logError(const QString& message, const QString& category = QString());

    /**
     * @brief Log critical message with fallback
     * @param message Message text
     * @param category Optional category
     */
    void logCritical(const QString& message, const QString& category = QString());

    // ========================================================================
    // GUI INTEGRATION SUPPORT (100% compatible with original)
    // ========================================================================

    /**
     * @brief Check if logger supports GUI integration
     * @return true if GUI integration is available
     */
    bool supportsGuiIntegration() const { return true; }

    /**
     * @brief Check if logger can connect to main window
     * @return true if connection is possible
     */
    bool canConnectToMainWindow() const { return true; }

    /**
     * @brief Check if real-time display capability is available
     * @return true if real-time display is supported
     */
    bool hasRealtimeDisplayCapability() const { return true; }

    /**
     * @brief Enable/disable asynchronous logging
     * @param enabled true to enable async logging
     */
    void enableAsyncLogging(bool enabled) { m_asyncLoggingEnabled.store(enabled); }

    /**
     * @brief Check if async logging is enabled
     * @return true if async logging is active
     */
    bool isAsyncLoggingEnabled() const { return m_asyncLoggingEnabled.load(); }

    /**
     * @brief Wait for async logging operations to complete
     * @param timeoutMs Maximum wait time in milliseconds
     * @return true if all operations completed within timeout
     */
    bool waitForAsyncCompletion(int timeoutMs = 30000);

    /**
     * @brief Get current memory usage of logger
     * @return Memory usage in bytes
     */
    qint64 getMemoryUsage() const;

    /**
     * @brief Get performance statistics
     * @return Hash containing performance metrics
     */
    QVariantHash getPerformanceStatistics() const;

    /**
     * @brief Set maximum memory usage for internal buffers
     * @param bytes Maximum memory usage in bytes
     */
    void setMaxMemoryUsage(qint64 bytes) { m_maxMemoryUsage.store(bytes); }

    /**
     * @brief Get maximum memory usage limit
     * @return Maximum memory usage in bytes
     */
    qint64 getMaxMemoryUsage() const { return m_maxMemoryUsage.load(); }

    /**
     * @brief Force flush of internal buffers to disk (safe)
     */
    void flush();

    /**
     * @brief Set log file directory (safe)
     * @param directory Path to log directory
     * @return true if directory is valid and writable
     */
    bool setLogDirectory(const QString& directory);

    /**
     * @brief Get current log directory
     * @return Path to log directory
     */
    QString getLogDirectory() const;

    /**
     * @brief Get current log file path
     * @return Path to current log file
     */
    QString getCurrentLogFilePath() const;

    /**
     * @brief Set maximum log file size before rotation
     * @param sizeBytes Maximum size in bytes
     */
    void setMaxLogFileSize(qint64 sizeBytes) { m_maxLogFileSize.store(sizeBytes); }

    /**
     * @brief Get maximum log file size
     * @return Maximum size in bytes
     */
    qint64 getMaxLogFileSize() const { return m_maxLogFileSize.load(); }

    /**
     * @brief Get list of all log files in directory
     * @return List of log file paths
     */
    QStringList getLogFileList() const;

    /**
     * @brief Log message data structure
     */
    struct LogMessage {
        QDateTime timestamp;
        LogLevel level;
        QString message;
        QString category;
        QString threadId;
        QVariantHash metadata;
        
        LogMessage() = default;
        LogMessage(LogLevel lvl, const QString& msg, const QString& cat = QString())
            : timestamp(QDateTime::currentDateTime())
            , level(lvl)
            , message(msg)
            , category(cat)
            , threadId(QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId())))
        {}
    };

    /**
     * @brief Get recent log messages for GUI display
     * @param maxCount Maximum number of messages to return
     * @return List of recent log messages
     */
    QList<LogMessage> getRecentMessages(int maxCount = 100) const;

signals:
    /**
     * @brief Emitted when a new log message is added
     * @param level Log level
     * @param message Message text
     * @param category Message category
     * @param timestamp Message timestamp
     */
    void logMessageAdded(LogLevel level, const QString& message, 
                        const QString& category, const QDateTime& timestamp);

    /**
     * @brief Emitted specifically for error messages (for GUI alerts)
     * @param message Error message
     * @param timestamp Error timestamp
     */
    void errorMessageAdded(const QString& message, const QDateTime& timestamp);

private:
    /**
     * @brief Private constructor for singleton (minimal initialization)
     */
    explicit Logger(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~Logger();

public:
    // Delete copy constructor and assignment operator for singleton
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:

    /**
     * @brief Safe fallback logging using qDebug() (never hangs)
     */
    void fallbackLog(LogLevel level, const QString& message, const QString& category);

    /**
     * @brief Convert legacy log level to modern enum
     */
    LogLevel convertLegacyLevel(LegacyLogLevel level) const;

    /**
     * @brief Internal logging implementation with fallback
     */
    void logInternal(LogLevel level, const QString& message, const QString& category);

    /**
     * @brief Attempt to initialize file logging (safe, non-blocking)
     */
    bool initializeFileLogging();

    /**
     * @brief Write log message to available outputs
     */
    void writeLogMessage(const LogMessage& message);

    /**
     * @brief Format log message for output
     */
    QString formatLogMessage(const LogMessage& message) const;

    /**
     * @brief Get safe log level string
     */
    QString getLevelString(LogLevel level) const;

    /**
     * @brief Check if Qt application context is available
     */
    static bool isQtApplicationReady();

    /**
     * @brief Attempt delayed initialization
     */
    void attemptDelayedInitialization();

    // Thread-safe atomic state management
    std::atomic<bool> m_fullyInitialized{false};
    std::atomic<bool> m_loggingEnabled{true};
    std::atomic<bool> m_fileLoggingAvailable{false};
    std::atomic<bool> m_asyncLoggingEnabled{false};
    std::atomic<int> m_logLevel{static_cast<int>(LogLevel::Info)};
    std::atomic<qint64> m_maxMemoryUsage{static_cast<qint64>(50) * 1024 * 1024}; // 50MB default
    std::atomic<qint64> m_maxLogFileSize{static_cast<qint64>(10) * 1024 * 1024}; // 10MB default
    
    // Thread-safe containers
    mutable QMutex m_messagesMutex;
    QList<LogMessage> m_recentMessages;
    
    // File logging components (only if available)
    mutable QMutex m_fileMutex;
    QString m_logDirectory;
    QString m_currentLogFile;
    std::unique_ptr<QFile> m_logFile;
    std::unique_ptr<QTextStream> m_logStream;
    
    // Performance tracking
    mutable QMutex m_statsMutex;
    qint64 m_messageCount{0};
    QDateTime m_startTime;
    
    // Delayed initialization
    QTimer* m_initTimer{nullptr};
    
    // Static singleton instance
    static Logger* s_instance;
    static QMutex s_instanceMutex;
    
    // Constants
    static constexpr int MAX_RECENT_MESSAGES = 1000;
    static constexpr int INIT_RETRY_INTERVAL_MS = 1000;
    static constexpr int MAX_INIT_RETRIES = 5;
    static constexpr qint64 DEFAULT_MAX_FILE_SIZE = static_cast<qint64>(10) * 1024 * 1024;  // 10MB
    static constexpr qint64 DEFAULT_MAX_MEMORY = static_cast<qint64>(50) * 1024 * 1024;     // 50MB
};

// Register metatypes for Qt signals
Q_DECLARE_METATYPE(Logger::LogLevel)
Q_DECLARE_METATYPE(Logger::LogMessage)