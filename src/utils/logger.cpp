#include "logger.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>
#include <QThread>
#include <algorithm>

// Static member initialization
Logger* Logger::s_instance = nullptr;
QMutex Logger::s_instanceMutex;

Logger::Logger(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_loggingEnabled(true)
    , m_asyncLoggingEnabled(false)
    , m_logLevel(LogLevel::Info)
    , m_maxLogFileSize(DEFAULT_MAX_FILE_SIZE)
    , m_maxMemoryUsage(DEFAULT_MAX_MEMORY)
    , m_memoryUsage(0)
    , m_messageCount(0)
    , m_startTime(QDateTime::currentDateTime())
{
    // Set default log directory
    m_logDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    
    // Ensure log directory exists
    QDir().mkpath(m_logDirectory);
}

Logger::~Logger()
{
    if (m_logStream) {
        m_logStream->flush();
    }
    if (m_logFile) {
        m_logFile->close();
    }
}

Logger& Logger::instance()
{
    QMutexLocker locker(&s_instanceMutex);
    if (!s_instance) {
        s_instance = new Logger();
        s_instance->initialize();
    }
    return *s_instance;
}

bool Logger::initialize()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_initialized) {
        return true;
    }
    
    // Create log directory if it doesn't exist
    QDir logDir(m_logDirectory);
    if (!logDir.exists()) {
        if (!logDir.mkpath(".")) {
            qDebug() << "Failed to create log directory:" << m_logDirectory;
            return false;
        }
    }
    
    // Create initial log file
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    m_currentLogFile = m_logDirectory + QString("/eti_analyzer_%1.log").arg(timestamp);
    
    m_logFile = std::make_unique<QFile>(m_currentLogFile);
    if (!m_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        qDebug() << "Failed to open log file:" << m_currentLogFile;
        return false;
    }
    
    m_logStream = std::make_unique<QTextStream>(m_logFile.get());
    // Note: setCodec() is deprecated in Qt6, UTF-8 is default
    
    m_initialized = true;
    
    // Log initialization message
    logInfo("Logger initialized", "Logger");
    
    return true;
}

bool Logger::enableLogging(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_loggingEnabled = enabled;
    return true;
}

void Logger::log(LegacyLogLevel level, const QString& category, const QString& message)
{
    LogLevel modernLevel = convertLegacyLevel(level);
    log(modernLevel, message, category);
}

void Logger::setLogLevel(LegacyLogLevel level)
{
    setLogLevel(convertLegacyLevel(level));
}

void Logger::log(LogLevel level, const QString& message, const QString& category, const QVariantHash& /*metadata*/)
{
    if (!m_initialized || !m_loggingEnabled) {
        return;
    }
    
    // Check if message meets minimum log level
    if (level < m_logLevel) {
        return;
    }
    
    logInternal(level, message, category);
}

void Logger::logDebug(const QString& message, const QString& category)
{
    log(LogLevel::Debug, message, category);
}

void Logger::logInfo(const QString& message, const QString& category)
{
    log(LogLevel::Info, message, category);
}

void Logger::logWarning(const QString& message, const QString& category)
{
    log(LogLevel::Warning, message, category);
}

void Logger::logError(const QString& message, const QString& category)
{
    log(LogLevel::Error, message, category);
    emit errorMessageAdded(message, QDateTime::currentDateTime());
}

void Logger::logCritical(const QString& message, const QString& category)
{
    log(LogLevel::Critical, message, category);
    emit errorMessageAdded(message, QDateTime::currentDateTime());
}

bool Logger::waitForAsyncCompletion(int /*timeoutMs*/)
{
    // For now, return immediately since we're not using async processing yet
    return true;
}

QVariantHash Logger::getPerformanceStatistics() const
{
    QMutexLocker locker(&m_mutex);
    
    QVariantHash stats;
    stats["message_count"] = m_messageCount;
    stats["memory_usage"] = m_memoryUsage;
    stats["uptime_seconds"] = m_startTime.secsTo(QDateTime::currentDateTime());
    stats["log_file"] = m_currentLogFile;
    stats["recent_message_count"] = m_recentMessages.size();
    
    return stats;
}

void Logger::flush()
{
    QMutexLocker locker(&m_mutex);
    if (m_logStream) {
        m_logStream->flush();
    }
    if (m_logFile) {
        m_logFile->flush();
    }
}

bool Logger::setLogDirectory(const QString& directory)
{
    QMutexLocker locker(&m_mutex);
    
    QDir dir(directory);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            return false;
        }
    }
    
    m_logDirectory = directory;
    return true;
}

QStringList Logger::getLogFileList() const
{
    QDir logDir(m_logDirectory);
    QStringList filters;
    filters << "eti_analyzer_*.log";
    
    QFileInfoList files = logDir.entryInfoList(filters, QDir::Files, QDir::Time);
    QStringList filePaths;
    
    for (const QFileInfo& fileInfo : files) {
        filePaths << fileInfo.absoluteFilePath();
    }
    
    return filePaths;
}

QList<Logger::LogMessage> Logger::getRecentMessages(int maxCount) const
{
    QMutexLocker locker(&m_mutex);
    
    if (m_recentMessages.size() <= maxCount) {
        return m_recentMessages;
    }
    
    // Return the most recent messages
    return m_recentMessages.mid(m_recentMessages.size() - maxCount);
}

Logger::LogLevel Logger::convertLegacyLevel(LegacyLogLevel level) const
{
    switch (level) {
        case Debug: return LogLevel::Debug;
        case Info: return LogLevel::Info;
        case Warning: return LogLevel::Warning;
        case Error: return LogLevel::Error;
        case Critical: return LogLevel::Critical;
        default: return LogLevel::Info;
    }
}

void Logger::logInternal(LogLevel level, const QString& message, const QString& category)
{
    QMutexLocker locker(&m_mutex);
    
    // Create log message
    LogMessage logMsg(level, message, category);
    
    // Add to recent messages (with size limit)
    m_recentMessages.append(logMsg);
    if (m_recentMessages.size() > MAX_RECENT_MESSAGES) {
        m_recentMessages.removeFirst();
    }
    
    // Write to file and console
    writeLogMessage(logMsg);
    
    // Update statistics
    m_messageCount++;
    m_memoryUsage = m_recentMessages.size() * sizeof(LogMessage);
    
    // Emit signal for GUI integration
    emit logMessageAdded(level, message, category, logMsg.timestamp);
}

void Logger::writeLogMessage(const LogMessage& message)
{
    QString formattedMessage = formatLogMessage(message);
    
    // Write to console for immediate feedback
    qDebug().noquote() << formattedMessage;
    
    // Write to file if available
    if (m_logStream) {
        *m_logStream << formattedMessage << Qt::endl;
        m_logStream->flush();
    }
}

QString Logger::formatLogMessage(const LogMessage& message) const
{
    QString levelStr;
    switch (message.level) {
        case LogLevel::Debug: levelStr = "DEBUG"; break;
        case LogLevel::Info: levelStr = "INFO"; break;
        case LogLevel::Warning: levelStr = "WARNING"; break;
        case LogLevel::Error: levelStr = "ERROR"; break;
        case LogLevel::Critical: levelStr = "CRITICAL"; break;
    }
    
    QString formatted = QString("[%1] [%2] %3: %4")
        .arg(message.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz"))
        .arg(levelStr)
        .arg(message.category.isEmpty() ? "General" : message.category)
        .arg(message.message);
    
    return formatted;
}

// Note: moc is handled automatically by CMake