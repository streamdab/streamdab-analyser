#pragma once

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QString>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>
#include <vector>
#include <memory>
#include <atomic>
#include <deque>
#include <unordered_map>
#include <chrono>

namespace professional_logging {

// Log severity levels matching broadcast industry standards
enum class LogLevel {
    TRACE,      // Detailed trace information for debugging
    DEBUG,      // Debug information for development
    INFO,       // General information messages
    WARNING,    // Warning conditions
    ERROR,      // Error conditions
    CRITICAL,   // Critical conditions requiring immediate attention
    EMERGENCY   // System is unusable - broadcast emergency
};

// Log categories for professional broadcast monitoring
enum class LogCategory {
    SYSTEM,         // System-level operations
    ETI_PROCESSING, // ETI frame processing
    SERVICE_DISCOVERY, // DAB service discovery
    AUDIO_ANALYSIS, // Audio processing and analysis
    NETWORK,        // Network operations
    FILE_IO,        // File input/output operations
    USER_INTERFACE, // UI operations and user interactions
    ETSI_COMPLIANCE, // ETSI standards compliance
    ERROR_DETECTION, // Error detection and recovery
    PERFORMANCE,    // Performance monitoring
    SECURITY,       // Security and authentication
    CONFIGURATION   // Configuration management
};

// Professional log entry structure
struct LogEntry {
    QString log_id;
    QDateTime timestamp;
    LogLevel level;
    LogCategory category;
    QString component;          // Source component name
    QString function_name;      // Source function name
    int line_number;           // Source line number
    QString message;           // Primary log message
    QString technical_details; // Technical details for debugging
    QString context_data;      // Additional context information
    uint64_t thread_id;        // Thread ID
    QString session_id;        // Session identifier
    std::chrono::microseconds processing_time; // Time taken for operation
    
    LogEntry() : 
        level(LogLevel::INFO),
        category(LogCategory::SYSTEM),
        line_number(0),
        thread_id(0),
        processing_time(0) {}
};

// Log rotation and archival configuration
struct LogRotationConfig {
    bool enabled;
    uint64_t max_file_size_mb;    // Maximum log file size in MB
    int max_files_count;          // Maximum number of archived log files
    int rotation_interval_hours;  // Auto-rotation interval in hours
    bool compress_archived_logs;  // Compress archived log files
    
    LogRotationConfig() :
        enabled(true),
        max_file_size_mb(100),
        max_files_count(10),
        rotation_interval_hours(24),
        compress_archived_logs(true) {}
};

// Log output configuration
struct LogOutputConfig {
    bool console_output;          // Output to console/debug
    bool file_output;            // Output to log files
    bool network_output;         // Output to network logging server
    bool real_time_monitoring;   // Enable real-time log monitoring
    QString log_directory;       // Directory for log files
    QString log_file_prefix;     // Prefix for log file names
    LogLevel minimum_level;      // Minimum log level to output
    
    LogOutputConfig() :
        console_output(true),
        file_output(true),
        network_output(false),
        real_time_monitoring(true),
        log_file_prefix("dab_analyser"),
        minimum_level(LogLevel::DEBUG) {}
};

// Log filtering and search capabilities
struct LogFilter {
    LogLevel min_level;
    LogLevel max_level;
    std::vector<LogCategory> categories;
    QDateTime from_time;
    QDateTime to_time;
    QString component_filter;
    QString message_filter;
    QString session_filter;
    
    LogFilter() :
        min_level(LogLevel::TRACE),
        max_level(LogLevel::EMERGENCY) {}
};

// Log statistics and metrics
struct LogStatistics {
    uint64_t total_entries;
    uint64_t entries_by_level[8];  // Count for each LogLevel
    uint64_t entries_by_category[12]; // Count for each LogCategory
    QDateTime first_entry_time;
    QDateTime last_entry_time;
    double average_processing_time_ms;
    uint64_t total_file_size_bytes;
    QString most_active_component;
    
    LogStatistics() :
        total_entries(0),
        average_processing_time_ms(0.0),
        total_file_size_bytes(0) {
        memset(entries_by_level, 0, sizeof(entries_by_level));
        memset(entries_by_category, 0, sizeof(entries_by_category));
    }
};

// QThread Worker for background logging operations
class LoggingWorker : public QObject {
    Q_OBJECT

public:
    explicit LoggingWorker(QObject* parent = nullptr);
    ~LoggingWorker() = default;

public slots:
    void startLogging();
    void stopLogging();
    void performLogRotation();
    void performLogCleanup();
    void flushLogBuffers();

signals:
    void logRotationCompleted();
    void logCleanupCompleted();
    void logBuffersFlushed();
    void loggingError(const QString& error);

private:
    std::atomic<bool> m_logging_active{false};
    QTimer* m_rotation_timer{nullptr};
    QTimer* m_cleanup_timer{nullptr};
    QTimer* m_flush_timer{nullptr};
};

// Professional Logging System
class ProfessionalLoggingSystem : public QObject {
    Q_OBJECT

public:
    explicit ProfessionalLoggingSystem(QObject* parent = nullptr);
    ~ProfessionalLoggingSystem();

    // Configuration
    void setOutputConfiguration(const LogOutputConfig& config);
    void setRotationConfiguration(const LogRotationConfig& config);
    void setMinimumLogLevel(LogLevel level);
    void setSessionId(const QString& session_id);
    
    // Timer control for file dialog compatibility
    void pauseTimers();
    void resumeTimers();
    
    // Logging methods with macro support
    void log(LogLevel level, LogCategory category, const QString& component,
             const QString& function, int line, const QString& message,
             const QString& technical_details = "", const QString& context = "");
    
    // Convenience logging methods
    void logTrace(LogCategory category, const QString& component, const QString& message);
    void logDebug(LogCategory category, const QString& component, const QString& message);
    void logInfo(LogCategory category, const QString& component, const QString& message);
    void logWarning(LogCategory category, const QString& component, const QString& message);
    void logError(LogCategory category, const QString& component, const QString& message);
    void logCritical(LogCategory category, const QString& component, const QString& message);
    void logEmergency(LogCategory category, const QString& component, const QString& message);
    
    // Performance logging
    void logPerformance(const QString& operation, std::chrono::microseconds duration,
                       const QString& details = "");
    
    // ETI-specific logging
    void logETIFrame(uint32_t frame_number, const QString& operation, 
                    const QString& details, LogLevel level = LogLevel::DEBUG);
    void logServiceDiscovery(const QString& service_name, const QString& details);
    void logETSICompliance(const QString& test_name, bool passed, const QString& details);
    
    // Log retrieval and filtering
    std::vector<LogEntry> getLogEntries(const LogFilter& filter = LogFilter()) const;
    std::vector<LogEntry> getRecentEntries(int count = 100) const;
    std::vector<LogEntry> searchLogs(const QString& search_term) const;
    
    // Statistics and monitoring
    LogStatistics getLogStatistics() const;
    QStringList getActiveComponents() const;
    uint64_t getLogCount(LogLevel level) const;
    uint64_t getLogCount(LogCategory category) const;
    
    // Export and archival
    bool exportLogsToFile(const QString& file_path, const LogFilter& filter = LogFilter()) const;
    bool exportLogsToJSON(const QString& file_path, const LogFilter& filter = LogFilter()) const;
    bool exportLogsToCSV(const QString& file_path, const LogFilter& filter = LogFilter()) const;
    QString generateLogReport(const QDateTime& from, const QDateTime& to) const;
    
    // Log file management
    void rotateLogFiles();
    void archiveLogFiles();
    void cleanupOldLogs(int days_to_keep = 30);
    QStringList getAvailableLogFiles() const;
    
    // Real-time monitoring
    void startRealTimeMonitoring();
    void stopRealTimeMonitoring();
    bool isRealTimeMonitoringActive() const;
    
    // System integration
    void flushAllBuffers();
    void shutdown();
    bool isInitialized() const;
    
signals:
    void logEntryAdded(const LogEntry& entry);
    void criticalLogDetected(const LogEntry& entry);
    void logFileRotated(const QString& old_file, const QString& new_file);
    void logStatisticsUpdated(const LogStatistics& stats);
    void bufferFull(LogCategory category);
    
private slots:
    void onRotationTimerTimeout();
    void onMonitoringTimerTimeout();
    void onBufferFlushTimerTimeout();
    
private:
    // Configuration
    LogOutputConfig m_output_config;
    LogRotationConfig m_rotation_config;
    std::atomic<LogLevel> m_minimum_level;
    QString m_session_id;
    
    // Thread safety
    mutable QMutex m_log_mutex;
    mutable QMutex m_file_mutex;
    mutable QMutex m_stats_mutex;
    
    // Log storage
    std::deque<LogEntry> m_log_buffer;
    std::unordered_map<LogCategory, std::deque<LogEntry>> m_category_buffers;
    
    // File management
    std::unique_ptr<QFile> m_current_log_file;
    std::unique_ptr<QTextStream> m_log_stream;
    QString m_current_log_filename;
    
    // Statistics tracking
    LogStatistics m_statistics;
    std::unordered_map<QString, uint64_t> m_component_counts;
    
    // QThread-based background processing (replaces blocking timers)
    QThread* m_worker_thread;
    LoggingWorker* m_worker;
    
    // State management
    std::atomic<bool> m_initialized;
    std::atomic<bool> m_real_time_monitoring;
    std::atomic<bool> m_shutdown_requested;
    
    // Internal methods
    void initializeLogFile();
    void writeLogEntry(const LogEntry& entry);
    void writeToConsole(const LogEntry& entry);
    void writeToFile(const LogEntry& entry);
    void updateStatistics(const LogEntry& entry);
    void checkAndRotateFile();
    void performLogRotation();
    
    QString formatLogEntry(const LogEntry& entry, const QString& format = "standard") const;
    QString logLevelToString(LogLevel level) const;
    QString logCategoryToString(LogCategory category) const;
    QString generateLogEntryId() const;
    QString generateLogFileName() const;
    
    bool shouldLogEntry(const LogEntry& entry) const;
    void ensureLogDirectory() const;
    void compressLogFile(const QString& file_path) const;
    
    // Buffer management
    void flushCategoryBuffer(LogCategory category);
    void flushAllCategoryBuffers();
    bool isCategoryBufferFull(LogCategory category) const;
    
    // File utilities
    bool copyLogFile(const QString& source, const QString& destination) const;
    bool deleteLogFile(const QString& file_path) const;
    uint64_t getFileSize(const QString& file_path) const;
    
    // Constants
    static constexpr size_t MAX_BUFFER_SIZE = 10000;
    static constexpr size_t CATEGORY_BUFFER_SIZE = 1000;
    static constexpr int FLUSH_INTERVAL_MS = 5000;
    static constexpr int MONITORING_INTERVAL_MS = 1000;
};

// Logging macros for convenient usage
#define PROF_LOG_TRACE(category, component, message) \
    ProfessionalLoggingSystem::instance()->log(professional_logging::LogLevel::TRACE, \
        category, component, __FUNCTION__, __LINE__, message)

#define PROF_LOG_DEBUG(category, component, message) \
    ProfessionalLoggingSystem::instance()->log(professional_logging::LogLevel::DEBUG, \
        category, component, __FUNCTION__, __LINE__, message)

#define PROF_LOG_INFO(category, component, message) \
    ProfessionalLoggingSystem::instance()->log(professional_logging::LogLevel::INFO, \
        category, component, __FUNCTION__, __LINE__, message)

#define PROF_LOG_WARNING(category, component, message) \
    ProfessionalLoggingSystem::instance()->log(professional_logging::LogLevel::WARNING, \
        category, component, __FUNCTION__, __LINE__, message)

#define PROF_LOG_ERROR(category, component, message) \
    ProfessionalLoggingSystem::instance()->log(professional_logging::LogLevel::ERROR, \
        category, component, __FUNCTION__, __LINE__, message)

#define PROF_LOG_CRITICAL(category, component, message) \
    ProfessionalLoggingSystem::instance()->log(professional_logging::LogLevel::CRITICAL, \
        category, component, __FUNCTION__, __LINE__, message)

// Singleton access (optional pattern for global logging)
class ProfessionalLogger {
public:
    static ProfessionalLoggingSystem* instance();
    static void initialize(QObject* parent = nullptr);
    static void shutdown();
    
private:
    static std::unique_ptr<ProfessionalLoggingSystem> s_instance;
};

} // namespace professional_logging