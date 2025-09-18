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
#include <memory>

/**
 * @class Logger
 * @brief Professional Logging Framework for ETI Stream Analyzer
 * 
 * Thread-safe logging system with structured output and professional
 * export capabilities for broadcast industry standards.
 * 
 * Features:
 * - Multi-level logging (Info/Warning/Error/Critical)
 * - Thread-safe operation for real-time environments
 * - Structured output formats (JSON, XML, Plain text)
 * - Professional export capabilities (PDF, HTML, CSV)
 * - Log rotation and archival management
 * - Memory-efficient buffering system
 * - Asynchronous logging for performance
 * - GUI integration support
 * 
 * TDD Implementation:
 * - All features implemented using Test-Driven Development
 * - Comprehensive test coverage for reliability
 * - Performance-validated for real-time operation
 */
class Logger : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Log severity levels for message classification
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
     * @brief Output format options for log files
     */
    enum class OutputFormat {
        PlainText = 0,  // Human-readable plain text
        Json = 1,       // Structured JSON format
        Xml = 2,        // XML format for integration
        Binary = 3      // Binary format for performance
    };
    Q_ENUM(OutputFormat)

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
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit Logger(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~Logger();

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
    // CONFIGURATION
    // ========================================================================

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
     * @brief Set output format for log files
     * @param format Output format
     */
    void setOutputFormat(OutputFormat format) { m_outputFormat = format; }

    /**
     * @brief Get current output format
     * @return Current output format
     */
    OutputFormat getOutputFormat() const { return m_outputFormat; }

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
     * @brief Set maximum number of log files to keep
     * @param count Maximum file count
     */
    void setMaxLogFileCount(int count) { m_maxLogFileCount = count; }

    /**
     * @brief Get maximum log file count
     * @return Maximum file count
     */
    int getMaxLogFileCount() const { return m_maxLogFileCount; }

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
     * @brief Set buffer size for batched writes
     * @param sizeBytes Buffer size in bytes
     */
    void setBufferSize(int sizeBytes) { m_bufferSize = sizeBytes; }

    /**
     * @brief Get current buffer size
     * @return Buffer size in bytes
     */
    int getBufferSize() const { return m_bufferSize; }

    /**
     * @brief Enable/disable asynchronous logging
     * @param enabled true to enable async logging
     */
    void enableAsyncLogging(bool enabled);

    /**
     * @brief Check if async logging is enabled
     * @return true if async logging is active
     */
    bool isAsyncLoggingEnabled() const { return m_asyncLoggingEnabled; }

    // ========================================================================
    // LOGGING METHODS
    // ========================================================================

    /**
     * @brief Log debug message
     * @param message Message text
     * @param category Optional category
     * @param metadata Optional metadata
     */
    void logDebug(const QString& message, const QString& category = QString(), 
                  const QVariantHash& metadata = QVariantHash());

    /**
     * @brief Log info message
     * @param message Message text
     * @param category Optional category
     * @param metadata Optional metadata
     */
    void logInfo(const QString& message, const QString& category = QString(), 
                 const QVariantHash& metadata = QVariantHash());

    /**
     * @brief Log warning message
     * @param message Message text
     * @param category Optional category
     * @param metadata Optional metadata
     */
    void logWarning(const QString& message, const QString& category = QString(), 
                    const QVariantHash& metadata = QVariantHash());

    /**
     * @brief Log error message
     * @param message Message text
     * @param category Optional category
     * @param metadata Optional metadata
     */
    void logError(const QString& message, const QString& category = QString(), 
                  const QVariantHash& metadata = QVariantHash());

    /**
     * @brief Log critical message
     * @param message Message text
     * @param category Optional category
     * @param metadata Optional metadata
     */
    void logCritical(const QString& message, const QString& category = QString(), 
                     const QVariantHash& metadata = QVariantHash());

    /**
     * @brief Log message with specified level
     * @param level Log level
     * @param message Message text
     * @param category Optional category
     * @param metadata Optional metadata
     */
    void log(LogLevel level, const QString& message, const QString& category = QString(), 
             const QVariantHash& metadata = QVariantHash());

    // ========================================================================
    // FILE MANAGEMENT
    // ========================================================================

    /**
     * @brief Get current log file path
     * @return Path to current log file
     */
    QString getCurrentLogFilePath() const;

    /**
     * @brief Force flush of internal buffers to disk
     */
    void flush();

    /**
     * @brief Wait for async logging operations to complete
     * @param timeoutMs Maximum wait time in milliseconds
     * @return true if all operations completed within timeout
     */
    bool waitForAsyncCompletion(int timeoutMs = 30000);

    /**
     * @brief Rotate log file immediately
     * @return true if rotation successful
     */
    bool rotateLogFile();

    /**
     * @brief Get list of all log files in directory
     * @return List of log file paths
     */
    QStringList getLogFileList() const;

    // ========================================================================
    // EXPORT FUNCTIONALITY
    // ========================================================================

    /**
     * @brief Export logs to PDF format
     * @param filePath Output file path
     * @param startTime Optional start time filter
     * @param endTime Optional end time filter
     * @return true if export successful
     */
    bool exportToPdf(const QString& filePath, 
                     const QDateTime& startTime = QDateTime(),
                     const QDateTime& endTime = QDateTime()) const;

    /**
     * @brief Export logs to HTML format
     * @param filePath Output file path
     * @param startTime Optional start time filter
     * @param endTime Optional end time filter
     * @return true if export successful
     */
    bool exportToHtml(const QString& filePath,
                      const QDateTime& startTime = QDateTime(),
                      const QDateTime& endTime = QDateTime()) const;

    /**
     * @brief Export logs to CSV format
     * @param filePath Output file path
     * @param startTime Optional start time filter
     * @param endTime Optional end time filter
     * @return true if export successful
     */
    bool exportToCsv(const QString& filePath,
                     const QDateTime& startTime = QDateTime(),
                     const QDateTime& endTime = QDateTime()) const;

    // ========================================================================
    // CONFIGURATION PERSISTENCE
    // ========================================================================

    /**
     * @brief Save logger configuration to file
     * @param filePath Configuration file path
     * @return true if saved successfully
     */
    bool saveConfiguration(const QString& filePath) const;

    /**
     * @brief Load logger configuration from file
     * @param filePath Configuration file path
     * @return true if loaded successfully
     */
    bool loadConfiguration(const QString& filePath);

    // ========================================================================
    // PERFORMANCE MONITORING
    // ========================================================================

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
     * @brief Get recent log messages for GUI display
     * @param maxCount Maximum number of messages to return
     * @return List of recent log messages
     */
    QList<LogMessage> getRecentMessages(int maxCount = 100) const;

    // ========================================================================
    // GUI INTEGRATION SUPPORT
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

signals:
    /**
     * @brief Emitted when logging system starts
     */
    void loggingStarted();

    /**
     * @brief Emitted when logging system stops
     */
    void loggingStopped();

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

    /**
     * @brief Emitted when log file is rotated
     * @param newFilePath Path to new log file
     */
    void logFileRotated(const QString& newFilePath);

    /**
     * @brief Emitted when configuration changes
     */
    void configurationChanged();

private slots:
    /**
     * @brief Process pending log messages from async queue
     */
    void processPendingMessages();

    /**
     * @brief Perform periodic maintenance tasks
     */
    void performMaintenance();

private:
    /**
     * @brief Internal logging worker for async operation
     */
    class LoggingWorker;
    friend class LoggingWorker;

    /**
     * @brief Write log message to file
     * @param message Log message to write
     */
    void writeLogMessage(const LogMessage& message);

    /**
     * @brief Format log message for output
     * @param message Message to format
     * @return Formatted string
     */
    QString formatLogMessage(const LogMessage& message) const;

    /**
     * @brief Check if log file needs rotation
     * @return true if rotation is needed
     */
    bool needsRotation() const;

    /**
     * @brief Perform log file rotation
     */
    void performRotation();

    /**
     * @brief Clean up old log files
     */
    void cleanupOldLogFiles();

    /**
     * @brief Create new log file
     * @return true if creation successful
     */
    bool createNewLogFile();

    /**
     * @brief Generate timestamp for log files
     * @return Timestamp string for file naming
     */
    QString generateLogFileTimestamp() const;

    /**
     * @brief Load default configuration
     */
    void loadDefaultConfiguration();

    /**
     * @brief Convert log level to string
     * @param level Log level
     * @return String representation
     */
    QString logLevelToString(LogLevel level) const;

    // Member variables
    bool m_initialized;
    bool m_loggingEnabled;
    bool m_asyncLoggingEnabled;

    // Configuration
    LogLevel m_logLevel;
    OutputFormat m_outputFormat;
    QString m_logDirectory;
    qint64 m_maxLogFileSize;
    int m_maxLogFileCount;
    qint64 m_maxMemoryUsage;
    int m_bufferSize;

    // File management
    QString m_currentLogFile;
    std::unique_ptr<QFile> m_logFile;
    std::unique_ptr<QTextStream> m_logStream;

    // Threading and synchronization
    mutable QMutex m_mutex;
    QWaitCondition m_waitCondition;
    std::unique_ptr<LoggingWorker> m_worker;
    std::unique_ptr<QThread> m_workerThread;

    // Message queuing
    QQueue<LogMessage> m_messageQueue;
    QList<LogMessage> m_recentMessages;

    // Performance monitoring
    qint64 m_messageCount;
    qint64 m_bytesWritten;
    QDateTime m_startTime;

    // Timers
    QTimer* m_maintenanceTimer;
    QTimer* m_flushTimer;

    // Constants
    static constexpr qint64 DEFAULT_MAX_FILE_SIZE = 10 * 1024 * 1024;  // 10MB
    static constexpr int DEFAULT_MAX_FILE_COUNT = 10;
    static constexpr qint64 DEFAULT_MAX_MEMORY = 50 * 1024 * 1024;     // 50MB
    static constexpr int DEFAULT_BUFFER_SIZE = 8192;                    // 8KB
    static constexpr int MAINTENANCE_INTERVAL_MS = 60000;              // 1 minute
    static constexpr int FLUSH_INTERVAL_MS = 5000;                     // 5 seconds
    static constexpr int MAX_RECENT_MESSAGES = 1000;
};

// Register metatypes for Qt signals
Q_DECLARE_METATYPE(Logger::LogLevel)
Q_DECLARE_METATYPE(Logger::OutputFormat)
Q_DECLARE_METATYPE(Logger::LogMessage)