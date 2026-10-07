#include "professional_logging_system.hpp"
#include <QApplication>
#include <QThread>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDirIterator>
#include <QStringConverter>
#include <QUuid>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace professional_logging {

// LoggingWorker implementation
LoggingWorker::LoggingWorker(QObject* parent)
    : QObject(parent)
{
    // Initialize timers - these will run in the worker thread
    m_rotation_timer = new QTimer(this);
    m_cleanup_timer = new QTimer(this);
    m_flush_timer = new QTimer(this);
    
    // Connect signals
    connect(m_rotation_timer, &QTimer::timeout, this, &LoggingWorker::performLogRotation);
    connect(m_cleanup_timer, &QTimer::timeout, this, &LoggingWorker::performLogCleanup);
    connect(m_flush_timer, &QTimer::timeout, this, &LoggingWorker::flushLogBuffers);
}

void LoggingWorker::startLogging()
{
    if (!m_logging_active) {
        m_logging_active = true;
        
        // Start timers with reasonable intervals (background thread can handle this)
        m_rotation_timer->start(3600000);  // 1 hour for log rotation
        m_cleanup_timer->start(86400000);  // 24 hours for cleanup
        m_flush_timer->start(5000);        // 5 seconds for buffer flush
        
        qDebug() << "LoggingWorker: Background logging started in thread" << QThread::currentThread();
    }
}

void LoggingWorker::stopLogging()
{
    m_logging_active = false;
    m_rotation_timer->stop();
    m_cleanup_timer->stop();
    m_flush_timer->stop();
    qDebug() << "LoggingWorker: Background logging stopped";
}

void LoggingWorker::performLogRotation()
{
    if (!m_logging_active) return;
    
    // Background processing - doesn't block UI
    // TODO: Add actual log rotation logic here
    emit logRotationCompleted();
}

void LoggingWorker::performLogCleanup()
{
    if (!m_logging_active) return;
    
    // Background processing - doesn't block UI
    // TODO: Add actual log cleanup logic here
    emit logCleanupCompleted();
}

void LoggingWorker::flushLogBuffers()
{
    if (!m_logging_active) return;
    
    // Background processing - doesn't block UI
    // TODO: Add actual buffer flush logic here
    emit logBuffersFlushed();
}

// Singleton implementation
std::unique_ptr<ProfessionalLoggingSystem> ProfessionalLogger::s_instance = nullptr;

ProfessionalLoggingSystem* ProfessionalLogger::instance()
{
    if (!s_instance) {
        initialize();
    }
    return s_instance.get();
}

void ProfessionalLogger::initialize(QObject* parent)
{
    if (!s_instance) {
        s_instance = std::make_unique<ProfessionalLoggingSystem>(parent);
    }
}

void ProfessionalLogger::shutdown()
{
    if (s_instance) {
        s_instance->shutdown();
        s_instance.reset();
    }
}

ProfessionalLoggingSystem::ProfessionalLoggingSystem(QObject* parent)
    : QObject(parent)
    , m_minimum_level(LogLevel::DEBUG)
    , m_session_id(QUuid::createUuid().toString().remove('{').remove('}'))
    , m_initialized(false)
    , m_real_time_monitoring(true)
    , m_shutdown_requested(false)
{
    // Initialize configuration with defaults
    m_output_config.log_directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    m_output_config.log_file_prefix = "streamdab_analyser";
    
    // Create background thread and worker
    m_worker_thread = new QThread(this);
    m_worker = new LoggingWorker();
    m_worker->moveToThread(m_worker_thread);
    
    // Connect worker signals to main thread (thread-safe)
    connect(m_worker, &LoggingWorker::logRotationCompleted,
            this, &ProfessionalLoggingSystem::onRotationTimerTimeout, Qt::QueuedConnection);
    connect(m_worker, &LoggingWorker::logBuffersFlushed,
            this, &ProfessionalLoggingSystem::onBufferFlushTimerTimeout, Qt::QueuedConnection);
    connect(m_worker, &LoggingWorker::loggingError,
            this, [this](const QString& error) {
                logError(LogCategory::SYSTEM, "LoggingWorker", error);
            }, Qt::QueuedConnection);
    
    // Connect thread lifecycle
    connect(m_worker_thread, &QThread::started, m_worker, &LoggingWorker::startLogging);
    connect(m_worker_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    
    // Initialize logging system
    ensureLogDirectory();
    initializeLogFile();
    
    // Start background processing after delay (non-blocking)
    QTimer::singleShot(3000, this, [this]() {
        if (!m_shutdown_requested.load()) {
            m_worker_thread->start();
            qDebug() << "ProfessionalLoggingSystem: Background thread started - UI will not freeze";
        }
    });
    
    m_initialized.store(true);
    
    // Log system initialization
    logInfo(LogCategory::SYSTEM, "ProfessionalLoggingSystem", 
           QString("Professional logging system initialized - Session: %1").arg(m_session_id));
    
    qDebug() << "Professional Logging System initialized with session ID:" << m_session_id;
}

ProfessionalLoggingSystem::~ProfessionalLoggingSystem()
{
    shutdown();
}

void ProfessionalLoggingSystem::setOutputConfiguration(const LogOutputConfig& config)
{
    QMutexLocker locker(&m_log_mutex);
    m_output_config = config;
    
    // Update minimum level
    m_minimum_level.store(config.minimum_level);
    
    // Reinitialize log file if directory changed
    if (m_current_log_filename != generateLogFileName()) {
        initializeLogFile();
    }
    
    logInfo(LogCategory::SYSTEM, "ProfessionalLoggingSystem", "Output configuration updated");
}

void ProfessionalLoggingSystem::setRotationConfiguration(const LogRotationConfig& config)
{
    QMutexLocker locker(&m_log_mutex);
    m_rotation_config = config;
    
    // Update worker configuration (timers managed in background thread)
    if (config.enabled && m_worker_thread && m_worker_thread->isRunning()) {
        qDebug() << "Log rotation configuration updated - worker will apply new settings";
    }
    
    logInfo(LogCategory::SYSTEM, "ProfessionalLoggingSystem", "Rotation configuration updated");
}

void ProfessionalLoggingSystem::setMinimumLogLevel(LogLevel level)
{
    m_minimum_level.store(level);
    m_output_config.minimum_level = level;
    
    logInfo(LogCategory::SYSTEM, "ProfessionalLoggingSystem", 
           QString("Minimum log level set to: %1").arg(logLevelToString(level)));
}

void ProfessionalLoggingSystem::setSessionId(const QString& session_id)
{
    QMutexLocker locker(&m_log_mutex);
    m_session_id = session_id;
    
    logInfo(LogCategory::SYSTEM, "ProfessionalLoggingSystem", 
           QString("Session ID updated: %1").arg(session_id));
}

void ProfessionalLoggingSystem::pauseTimers()
{
    // Pause worker processing temporarily for file dialog operations
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "stopLogging", Qt::QueuedConnection);
        qDebug() << "Professional Logging: Paused worker processing for file dialog";
    }
}

void ProfessionalLoggingSystem::resumeTimers()
{
    // Resume worker processing after file dialog operations
    if (m_worker && !m_shutdown_requested.load()) {
        QMetaObject::invokeMethod(m_worker, "startLogging", Qt::QueuedConnection);
        qDebug() << "Professional Logging: Resumed worker processing after file dialog";
    }
}

void ProfessionalLoggingSystem::log(LogLevel level, LogCategory category, const QString& component,
                                   const QString& function, int line, const QString& message,
                                   const QString& technical_details, const QString& context)
{
    if (!m_initialized.load() || m_shutdown_requested.load()) {
        return;
    }
    
    // Check minimum level
    if (level < m_minimum_level.load()) {
        return;
    }
    
    auto start_time = std::chrono::steady_clock::now();
    
    LogEntry entry;
    entry.log_id = generateLogEntryId();
    entry.timestamp = QDateTime::currentDateTime();
    entry.level = level;
    entry.category = category;
    entry.component = component;
    entry.function_name = function;
    entry.line_number = line;
    entry.message = message;
    entry.technical_details = technical_details;
    entry.context_data = context;
    entry.thread_id = reinterpret_cast<uint64_t>(QThread::currentThreadId());
    entry.session_id = m_session_id;
    
    auto end_time = std::chrono::steady_clock::now();
    entry.processing_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    if (!shouldLogEntry(entry)) {
        return;
    }
    
    // Thread-safe logging
    QMutexLocker locker(&m_log_mutex);
    
    // Add to main buffer
    m_log_buffer.push_back(entry);
    if (m_log_buffer.size() > MAX_BUFFER_SIZE) {
        m_log_buffer.pop_front();
    }
    
    // Add to category buffer
    m_category_buffers[category].push_back(entry);
    if (m_category_buffers[category].size() > CATEGORY_BUFFER_SIZE) {
        m_category_buffers[category].pop_front();
    }
    
    // Update statistics
    updateStatistics(entry);
    
    // Write log entry
    writeLogEntry(entry);
    
    // Emit signals
    emit logEntryAdded(entry);
    
    if (level >= LogLevel::CRITICAL) {
        emit criticalLogDetected(entry);
    }
    
    // Check buffer status
    if (isCategoryBufferFull(category)) {
        emit bufferFull(category);
    }
}

void ProfessionalLoggingSystem::logTrace(LogCategory category, const QString& component, const QString& message)
{
    log(LogLevel::TRACE, category, component, "", 0, message);
}

void ProfessionalLoggingSystem::logDebug(LogCategory category, const QString& component, const QString& message)
{
    log(LogLevel::DEBUG, category, component, "", 0, message);
}

void ProfessionalLoggingSystem::logInfo(LogCategory category, const QString& component, const QString& message)
{
    log(LogLevel::INFO, category, component, "", 0, message);
}

void ProfessionalLoggingSystem::logWarning(LogCategory category, const QString& component, const QString& message)
{
    log(LogLevel::WARNING, category, component, "", 0, message);
}

void ProfessionalLoggingSystem::logError(LogCategory category, const QString& component, const QString& message)
{
    log(LogLevel::ERROR, category, component, "", 0, message);
}

void ProfessionalLoggingSystem::logCritical(LogCategory category, const QString& component, const QString& message)
{
    log(LogLevel::CRITICAL, category, component, "", 0, message);
}

void ProfessionalLoggingSystem::logEmergency(LogCategory category, const QString& component, const QString& message)
{
    log(LogLevel::EMERGENCY, category, component, "", 0, message);
}

void ProfessionalLoggingSystem::logPerformance(const QString& operation, std::chrono::microseconds duration,
                                              const QString& details)
{
    QString message = QString("Performance: %1 completed in %2μs").arg(operation).arg(duration.count());
    log(LogLevel::INFO, LogCategory::PERFORMANCE, "PerformanceMonitor", "", 0, message, details);
}

void ProfessionalLoggingSystem::logETIFrame(uint32_t frame_number, const QString& operation, 
                                           const QString& details, LogLevel level)
{
    QString message = QString("ETI Frame %1: %2").arg(frame_number).arg(operation);
    log(level, LogCategory::ETI_PROCESSING, "ETIProcessor", "", 0, message, details);
}

void ProfessionalLoggingSystem::logServiceDiscovery(const QString& service_name, const QString& details)
{
    QString message = QString("Service discovered: %1").arg(service_name);
    log(LogLevel::INFO, LogCategory::SERVICE_DISCOVERY, "ServiceDiscovery", "", 0, message, details);
}

void ProfessionalLoggingSystem::logETSICompliance(const QString& test_name, bool passed, const QString& details)
{
    LogLevel level = passed ? LogLevel::INFO : LogLevel::WARNING;
    QString message = QString("ETSI Compliance Test: %1 - %2").arg(test_name).arg(passed ? "PASSED" : "FAILED");
    log(level, LogCategory::ETSI_COMPLIANCE, "ETSIValidator", "", 0, message, details);
}

std::vector<LogEntry> ProfessionalLoggingSystem::getLogEntries(const LogFilter& filter) const
{
    QMutexLocker locker(&m_log_mutex);
    
    std::vector<LogEntry> filtered_entries;
    
    for (const auto& entry : m_log_buffer) {
        // Apply filters
        if (entry.level < filter.min_level || entry.level > filter.max_level) {
            continue;
        }
        
        if (!filter.categories.empty() && 
            std::find(filter.categories.begin(), filter.categories.end(), entry.category) == filter.categories.end()) {
            continue;
        }
        
        if (filter.from_time.isValid() && entry.timestamp < filter.from_time) {
            continue;
        }
        
        if (filter.to_time.isValid() && entry.timestamp > filter.to_time) {
            continue;
        }
        
        if (!filter.component_filter.isEmpty() && !entry.component.contains(filter.component_filter, Qt::CaseInsensitive)) {
            continue;
        }
        
        if (!filter.message_filter.isEmpty() && !entry.message.contains(filter.message_filter, Qt::CaseInsensitive)) {
            continue;
        }
        
        if (!filter.session_filter.isEmpty() && entry.session_id != filter.session_filter) {
            continue;
        }
        
        filtered_entries.push_back(entry);
    }
    
    return filtered_entries;
}

std::vector<LogEntry> ProfessionalLoggingSystem::getRecentEntries(int count) const
{
    QMutexLocker locker(&m_log_mutex);
    
    std::vector<LogEntry> recent_entries;
    int start_index = std::max(0, static_cast<int>(m_log_buffer.size()) - count);
    
    for (int i = start_index; i < static_cast<int>(m_log_buffer.size()); i++) {
        recent_entries.push_back(m_log_buffer[i]);
    }
    
    return recent_entries;
}

LogStatistics ProfessionalLoggingSystem::getLogStatistics() const
{
    QMutexLocker locker(&m_stats_mutex);
    return m_statistics;
}

bool ProfessionalLoggingSystem::exportLogsToJSON(const QString& file_path, const LogFilter& filter) const
{
    std::vector<LogEntry> entries = getLogEntries(filter);
    
    QJsonArray logs_array;
    for (const auto& entry : entries) {
        QJsonObject log_obj;
        log_obj["id"] = entry.log_id;
        log_obj["timestamp"] = entry.timestamp.toString(Qt::ISODate);
        log_obj["level"] = logLevelToString(entry.level);
        log_obj["category"] = logCategoryToString(entry.category);
        log_obj["component"] = entry.component;
        log_obj["function"] = entry.function_name;
        log_obj["line"] = entry.line_number;
        log_obj["message"] = entry.message;
        log_obj["technical_details"] = entry.technical_details;
        log_obj["context"] = entry.context_data;
        log_obj["thread_id"] = static_cast<qint64>(entry.thread_id);
        log_obj["session_id"] = entry.session_id;
        log_obj["processing_time_us"] = static_cast<qint64>(entry.processing_time.count());
        
        logs_array.append(log_obj);
    }
    
    QJsonObject root;
    root["export_timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["total_entries"] = logs_array.size();
    root["session_id"] = m_session_id;
    root["logs"] = logs_array;
    
    QJsonDocument doc(root);
    
    QFile file(file_path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(doc.toJson());
    return true;
}

void ProfessionalLoggingSystem::rotateLogFiles()
{
    performLogRotation();
}

void ProfessionalLoggingSystem::startRealTimeMonitoring()
{
    m_real_time_monitoring.store(true);
    if (m_worker_thread && !m_worker_thread->isRunning()) {
        m_worker_thread->start();
    }
    
    logInfo(LogCategory::SYSTEM, "ProfessionalLoggingSystem", "Real-time monitoring started");
}

void ProfessionalLoggingSystem::stopRealTimeMonitoring()
{
    m_real_time_monitoring.store(false);
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "stopLogging", Qt::QueuedConnection);
    }
    
    logInfo(LogCategory::SYSTEM, "ProfessionalLoggingSystem", "Real-time monitoring stopped");
}

bool ProfessionalLoggingSystem::isRealTimeMonitoringActive() const
{
    return m_real_time_monitoring.load();
}

void ProfessionalLoggingSystem::flushAllBuffers()
{
    QMutexLocker locker(&m_log_mutex);
    
    if (m_log_stream) {
        m_log_stream->flush();
    }
    
    logDebug(LogCategory::SYSTEM, "ProfessionalLoggingSystem", "All buffers flushed");
}

void ProfessionalLoggingSystem::shutdown()
{
    if (m_shutdown_requested.load()) {
        return;
    }
    
    m_shutdown_requested.store(true);
    
    logInfo(LogCategory::SYSTEM, "ProfessionalLoggingSystem", "Logging system shutdown initiated");
    
    // Stop worker thread
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "stopLogging", Qt::QueuedConnection);
    }
    if (m_worker_thread && m_worker_thread->isRunning()) {
        m_worker_thread->quit();
        m_worker_thread->wait(3000); // Wait up to 3 seconds
    }
    
    // Flush all buffers
    flushAllBuffers();
    
    // Close log file
    QMutexLocker file_locker(&m_file_mutex);
    if (m_log_stream) {
        m_log_stream.reset();
    }
    if (m_current_log_file) {
        m_current_log_file->close();
        m_current_log_file.reset();
    }
    
    m_initialized.store(false);
}

bool ProfessionalLoggingSystem::isInitialized() const
{
    return m_initialized.load();
}

void ProfessionalLoggingSystem::onRotationTimerTimeout()
{
    performLogRotation();
}

void ProfessionalLoggingSystem::onMonitoringTimerTimeout()
{
    if (!m_real_time_monitoring.load()) {
        return;
    }
    
    // Update and emit statistics
    QMutexLocker locker(&m_stats_mutex);
    emit logStatisticsUpdated(m_statistics);
}

void ProfessionalLoggingSystem::onBufferFlushTimerTimeout()
{
    flushAllBuffers();
}

void ProfessionalLoggingSystem::initializeLogFile()
{
    QMutexLocker locker(&m_file_mutex);
    
    // Close existing file
    if (m_log_stream) {
        m_log_stream.reset();
    }
    if (m_current_log_file) {
        m_current_log_file->close();
        m_current_log_file.reset();
    }
    
    // Generate new filename
    m_current_log_filename = generateLogFileName();
    
    // Create new log file
    m_current_log_file = std::make_unique<QFile>(m_current_log_filename);
    if (!m_current_log_file->open(QIODevice::WriteOnly | QIODevice::Append)) {
        qCritical() << "Failed to open log file:" << m_current_log_filename;
        return;
    }
    
    m_log_stream = std::make_unique<QTextStream>(m_current_log_file.get());
    m_log_stream->setEncoding(QStringConverter::Utf8);
    
    // Write session header
    *m_log_stream << QString("=== StreamDAB Analyser Professional Logging Session Started ===\n");
    *m_log_stream << QString("Session ID: %1\n").arg(m_session_id);
    *m_log_stream << QString("Started: %1\n").arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    *m_log_stream << QString("================================================================\n\n");
    m_log_stream->flush();
}

void ProfessionalLoggingSystem::writeLogEntry(const LogEntry& entry)
{
    if (m_output_config.console_output) {
        writeToConsole(entry);
    }
    
    if (m_output_config.file_output) {
        writeToFile(entry);
    }
    
    // Check for file rotation
    if (m_rotation_config.enabled) {
        checkAndRotateFile();
    }
}

void ProfessionalLoggingSystem::writeToConsole(const LogEntry& entry)
{
    QString formatted = formatLogEntry(entry, "console");
    qDebug().noquote() << formatted;
}

void ProfessionalLoggingSystem::writeToFile(const LogEntry& entry)
{
    QMutexLocker locker(&m_file_mutex);
    
    if (!m_log_stream) {
        return;
    }
    
    QString formatted = formatLogEntry(entry, "file");
    *m_log_stream << formatted << "\n";
    
    // Flush critical and emergency logs immediately
    if (entry.level >= LogLevel::CRITICAL) {
        m_log_stream->flush();
    }
}

void ProfessionalLoggingSystem::updateStatistics(const LogEntry& entry)
{
    QMutexLocker locker(&m_stats_mutex);
    
    m_statistics.total_entries++;
    m_statistics.entries_by_level[static_cast<int>(entry.level)]++;
    m_statistics.entries_by_category[static_cast<int>(entry.category)]++;
    
    if (m_statistics.first_entry_time.isNull() || entry.timestamp < m_statistics.first_entry_time) {
        m_statistics.first_entry_time = entry.timestamp;
    }
    
    if (m_statistics.last_entry_time.isNull() || entry.timestamp > m_statistics.last_entry_time) {
        m_statistics.last_entry_time = entry.timestamp;
    }
    
    // Update component activity
    m_component_counts[entry.component]++;
    
    // Find most active component
    QString most_active;
    uint64_t max_count = 0;
    for (const auto& pair : m_component_counts) {
        if (pair.second > max_count) {
            max_count = pair.second;
            most_active = pair.first;
        }
    }
    m_statistics.most_active_component = most_active;
    
    // Update average processing time
    static uint64_t total_processing_time = 0;
    total_processing_time += entry.processing_time.count();
    m_statistics.average_processing_time_ms = 
        static_cast<double>(total_processing_time) / m_statistics.total_entries / 1000.0;
}

void ProfessionalLoggingSystem::checkAndRotateFile()
{
    if (!m_rotation_config.enabled || !m_current_log_file) {
        return;
    }
    
    uint64_t file_size = getFileSize(m_current_log_filename);
    uint64_t max_size_bytes = m_rotation_config.max_file_size_mb * 1024 * 1024;
    
    if (file_size >= max_size_bytes) {
        performLogRotation();
    }
}

void ProfessionalLoggingSystem::performLogRotation()
{
    QString old_filename = m_current_log_filename;
    
    // Create archived filename
    QFileInfo file_info(old_filename);
    QString archived_filename = QString("%1/%2_%3.%4")
        .arg(file_info.absolutePath())
        .arg(file_info.baseName())
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"))
        .arg(file_info.suffix());
    
    // Close current file
    {
        QMutexLocker locker(&m_file_mutex);
        if (m_log_stream) {
            m_log_stream.reset();
        }
        if (m_current_log_file) {
            m_current_log_file->close();
            m_current_log_file.reset();
        }
    }
    
    // Move current file to archived location
    if (copyLogFile(old_filename, archived_filename)) {
        deleteLogFile(old_filename);
        
        // Compress if enabled
        if (m_rotation_config.compress_archived_logs) {
            compressLogFile(archived_filename);
        }
        
        emit logFileRotated(old_filename, archived_filename);
    }
    
    // Initialize new log file
    initializeLogFile();
    
    logInfo(LogCategory::SYSTEM, "ProfessionalLoggingSystem", 
           QString("Log file rotated: %1 -> %2").arg(old_filename, archived_filename));
}

QString ProfessionalLoggingSystem::formatLogEntry(const LogEntry& entry, const QString& format) const
{
    if (format == "console") {
        return QString("[%1] %2 [%3] %4: %5")
            .arg(entry.timestamp.toString("hh:mm:ss.zzz"))
            .arg(logLevelToString(entry.level).leftJustified(8))
            .arg(logCategoryToString(entry.category).leftJustified(12))
            .arg(entry.component)
            .arg(entry.message);
    } else { // file format
        QString result = QString("%1|%2|%3|%4|%5|%6|%7|%8")
            .arg(entry.timestamp.toString(Qt::ISODate))
            .arg(logLevelToString(entry.level))
            .arg(logCategoryToString(entry.category))
            .arg(entry.component)
            .arg(entry.function_name)
            .arg(entry.line_number)
            .arg(entry.message)
            .arg(entry.session_id);
        
        if (!entry.technical_details.isEmpty()) {
            result += QString("|TECH:%1").arg(entry.technical_details);
        }
        
        if (!entry.context_data.isEmpty()) {
            result += QString("|CTX:%1").arg(entry.context_data);
        }
        
        return result;
    }
}

QString ProfessionalLoggingSystem::logLevelToString(LogLevel level) const
{
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        case LogLevel::EMERGENCY: return "EMERGENCY";
        default: return "UNKNOWN";
    }
}

QString ProfessionalLoggingSystem::logCategoryToString(LogCategory category) const
{
    switch (category) {
        case LogCategory::SYSTEM: return "SYSTEM";
        case LogCategory::ETI_PROCESSING: return "ETI_PROC";
        case LogCategory::SERVICE_DISCOVERY: return "SERVICE";
        case LogCategory::AUDIO_ANALYSIS: return "AUDIO";
        case LogCategory::NETWORK: return "NETWORK";
        case LogCategory::FILE_IO: return "FILE_IO";
        case LogCategory::USER_INTERFACE: return "UI";
        case LogCategory::ETSI_COMPLIANCE: return "ETSI";
        case LogCategory::ERROR_DETECTION: return "ERROR_DET";
        case LogCategory::PERFORMANCE: return "PERF";
        case LogCategory::SECURITY: return "SECURITY";
        case LogCategory::CONFIGURATION: return "CONFIG";
        default: return "UNKNOWN";
    }
}

QString ProfessionalLoggingSystem::generateLogEntryId() const
{
    QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(timestamp.toUtf8());
    hash.addData(QUuid::createUuid().toString().toUtf8());
    return hash.result().toHex().left(12);
}

QString ProfessionalLoggingSystem::generateLogFileName() const
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    return QString("%1/%2_%3.log")
        .arg(m_output_config.log_directory)
        .arg(m_output_config.log_file_prefix)
        .arg(timestamp);
}

bool ProfessionalLoggingSystem::shouldLogEntry(const LogEntry& entry) const
{
    return entry.level >= m_minimum_level.load();
}

void ProfessionalLoggingSystem::ensureLogDirectory() const
{
    QDir dir(m_output_config.log_directory);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

uint64_t ProfessionalLoggingSystem::getFileSize(const QString& file_path) const
{
    QFileInfo info(file_path);
    return info.size();
}

bool ProfessionalLoggingSystem::copyLogFile(const QString& source, const QString& destination) const
{
    return QFile::copy(source, destination);
}

bool ProfessionalLoggingSystem::deleteLogFile(const QString& file_path) const
{
    return QFile::remove(file_path);
}

void ProfessionalLoggingSystem::compressLogFile(const QString& file_path) const
{
    // Placeholder for compression implementation
    Q_UNUSED(file_path)
    // Could implement gzip compression here
}

bool ProfessionalLoggingSystem::isCategoryBufferFull(LogCategory category) const
{
    auto it = m_category_buffers.find(category);
    return (it != m_category_buffers.end() && it->second.size() >= CATEGORY_BUFFER_SIZE);
}

} // namespace professional_logging