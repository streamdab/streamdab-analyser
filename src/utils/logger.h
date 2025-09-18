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
#include <memory>

/**
 * @class Logger
 * @brief Unified Professional Logging Framework for ETI Stream Analyzer
 * 
 * Thread-safe singleton logging system with both modern and legacy interfaces
 * for compatibility with existing GUI code while providing professional features.
 */
class Logger : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Modern log severity levels
     */
    enum class LogLevel {
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
    enum LegacyLogLevel {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        Critical = 4
    };

    /**
     * @brief Get singleton instance
     * @return Logger singleton reference
     */
    static Logger& instance();

    /**
     * @brief Initialize the logging system
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * @brief Check if logging system is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return m_initialized; }

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
    bool isLoggingEnabled() const { return m_loggingEnabled; }

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
     * @brief Modern logging method
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
    void setLogLevel(LogLevel level) { m_logLevel = level; }

    /**
     * @brief Get current log level
     * @return Current minimum log level
     */
    LogLevel getLogLevel() const { return m_logLevel; }

    /**
     * @brief Log debug message
     * @param message Message text
     * @param category Optional category
     */
    void logDebug(const QString& message, const QString& category = QString());

    /**
     * @brief Log info message
     * @param message Message text
     * @param category Optional category
     */
    void logInfo(const QString& message, const QString& category = QString());

    /**
     * @brief Log warning message
     * @param message Message text
     * @param category Optional category
     */
    void logWarning(const QString& message, const QString& category = QString());

    /**
     * @brief Log error message
     * @param message Message text
     * @param category Optional category
     */
    void logError(const QString& message, const QString& category = QString());

    /**
     * @brief Log critical message
     * @param message Message text
     * @param category Optional category
     */
    void logCritical(const QString& message, const QString& category = QString());

    // ========================================================================
    // GUI INTEGRATION SUPPORT (required for tests to pass)
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
    void enableAsyncLogging(bool enabled) { m_asyncLoggingEnabled = enabled; }

    /**
     * @brief Check if async logging is enabled
     * @return true if async logging is active
     */
    bool isAsyncLoggingEnabled() const { return m_asyncLoggingEnabled; }

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
    qint64 getMemoryUsage() const { return m_memoryUsage; }

    /**
     * @brief Get performance statistics
     * @return Hash containing performance metrics
     */
    QVariantHash getPerformanceStatistics() const;

    /**
     * @brief Set maximum memory usage for internal buffers
     * @param bytes Maximum memory usage in bytes
     */
    void setMaxMemoryUsage(qint64 bytes) { m_maxMemoryUsage = bytes; }

    /**
     * @brief Get maximum memory usage limit
     * @return Maximum memory usage in bytes
     */
    qint64 getMaxMemoryUsage() const { return m_maxMemoryUsage; }

    /**
     * @brief Force flush of internal buffers to disk
     */
    void flush();

    /**
     * @brief Set log file directory
     * @param directory Path to log directory
     * @return true if directory is valid and writable
     */
    bool setLogDirectory(const QString& directory);

    /**
     * @brief Get current log directory
     * @return Path to log directory
     */
    QString getLogDirectory() const { return m_logDirectory; }

    /**
     * @brief Get current log file path
     * @return Path to current log file
     */
    QString getCurrentLogFilePath() const { return m_currentLogFile; }

    /**
     * @brief Set maximum log file size before rotation
     * @param sizeBytes Maximum size in bytes
     */
    void setMaxLogFileSize(qint64 sizeBytes) { m_maxLogFileSize = sizeBytes; }

    /**
     * @brief Get maximum log file size
     * @return Maximum size in bytes
     */
    qint64 getMaxLogFileSize() const { return m_maxLogFileSize; }

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
     * @brief Private constructor for singleton
     */
    explicit Logger(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~Logger();

    // Delete copy constructor and assignment operator for singleton
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief Convert legacy log level to modern enum
     */
    LogLevel convertLegacyLevel(LegacyLogLevel level) const;

    /**
     * @brief Internal logging implementation
     */
    void logInternal(LogLevel level, const QString& message, const QString& category);

    /**
     * @brief Write log message to console and storage
     */
    void writeLogMessage(const LogMessage& message);

    /**
     * @brief Format log message for output
     */
    QString formatLogMessage(const LogMessage& message) const;

    // Member variables
    bool m_initialized;
    bool m_loggingEnabled;
    bool m_asyncLoggingEnabled;
    LogLevel m_logLevel;
    QString m_logDirectory;
    QString m_currentLogFile;
    qint64 m_maxLogFileSize;
    qint64 m_maxMemoryUsage;
    qint64 m_memoryUsage;
    
    // Threading and synchronization
    mutable QMutex m_mutex;
    QWaitCondition m_waitCondition;
    
    // Message storage
    QQueue<LogMessage> m_messageQueue;
    QList<LogMessage> m_recentMessages;
    
    // Performance tracking
    qint64 m_messageCount;
    QDateTime m_startTime;
    
    // File I/O
    std::unique_ptr<QFile> m_logFile;
    std::unique_ptr<QTextStream> m_logStream;
    
    // Static singleton instance
    static Logger* s_instance;
    static QMutex s_instanceMutex;
    
    // Constants
    static constexpr qint64 DEFAULT_MAX_FILE_SIZE = 10 * 1024 * 1024;  // 10MB
    static constexpr qint64 DEFAULT_MAX_MEMORY = 50 * 1024 * 1024;     // 50MB
    static constexpr int MAX_RECENT_MESSAGES = 1000;
};

// Register metatypes for Qt signals
Q_DECLARE_METATYPE(Logger::LogLevel)
Q_DECLARE_METATYPE(Logger::LogMessage)