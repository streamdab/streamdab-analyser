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
#include <QTimer>
#include <algorithm>

// Static member initialization
Logger* Logger::s_instance = nullptr;
QMutex Logger::s_instanceMutex;

Logger::Logger(QObject *parent)
    : QObject(parent)
    , m_startTime(QDateTime::currentDateTime())
{
    // CRITICAL: Minimal constructor - avoid any operations that could hang
    
    // Setup delayed initialization timer (safe)
    m_initTimer = new QTimer(this);
    m_initTimer->setSingleShot(false);
    m_initTimer->setInterval(INIT_RETRY_INTERVAL_MS);
    
    connect(m_initTimer, &QTimer::timeout, this, &Logger::attemptDelayedInitialization);
    
    // Start timer to attempt initialization when Qt is ready
    QTimer::singleShot(100, this, [this]() {
        if (m_initTimer) {
            m_initTimer->start();
        }
    });
    
    // Log using fallback that we're starting up
    fallbackLog(LogLevel::Info, "Logger created with hang-safe implementation", "Logger");
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
    // Use minimal locking to prevent hangs
    QMutexLocker locker(&s_instanceMutex);
    if (!s_instance) {
        s_instance = new Logger();
        // Note: Do NOT call initialization here - use delayed initialization
    }
    return *s_instance;
}

bool Logger::isQtApplicationReady()
{
    // Check if Qt application context is available
    QCoreApplication* app = QCoreApplication::instance();
    if (!app) {
        return false;
    }
    
    // Check if application is in a state where file operations are safe
    if (app->applicationName().isEmpty()) {
        return false;
    }
    
    return true;
}

void Logger::attemptDelayedInitialization()
{
    if (m_fullyInitialized.load()) {
        // Already initialized, stop timer
        if (m_initTimer) {
            m_initTimer->stop();
        }
        return;
    }
    
    if (!isQtApplicationReady()) {
        // Qt not ready yet, try again later
        return;
    }
    
    // Attempt safe initialization
    if (initializeFileLogging()) {
        // Success - stop timer
        if (m_initTimer) {
            m_initTimer->stop();
        }
        m_fullyInitialized.store(true);
        m_fileLoggingAvailable.store(true);
        
        // Log successful initialization
        logInfo("Logger fully initialized with file logging", "Logger");
    }
}

bool Logger::initialize()
{
    if (m_fullyInitialized.load()) {
        return true;
    }
    
    if (!isQtApplicationReady()) {
        return false;
    }
    
    // Attempt immediate initialization
    return initializeFileLogging();
}

bool Logger::initializeFileLogging()
{
    QMutexLocker locker(&m_fileMutex);
    
    if (m_fileLoggingAvailable.load()) {
        return true;
    }
    
    try {
        // Get safe log directory
        QString appName = QCoreApplication::applicationName();
        if (appName.isEmpty()) {
            appName = "ETIStreamAnalyser";
        }
        
        // Try to get writable location, with fallback
        QString logDir;
        try {
            logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            if (logDir.isEmpty()) {
                // Fallback to temp directory
                logDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
                if (!logDir.isEmpty()) {
                    logDir = logDir + "/" + appName;
                }
            }
        }
        catch (...) {
            // QStandardPaths failed, use current directory
            logDir = QDir::currentPath() + "/logs";
        }
        
        if (logDir.isEmpty()) {
            // Last resort fallback
            logDir = "./logs";
        }
        
        // Ensure logs subdirectory
        if (!logDir.endsWith("/logs")) {
            logDir = logDir + "/logs";
        }
        
        m_logDirectory = logDir;
        
        // Create log directory if it doesn't exist (with timeout protection)
        QDir dir;
        if (!dir.mkpath(m_logDirectory)) {
            // Directory creation failed, try current directory fallback
            m_logDirectory = "./logs";
            if (!dir.mkpath(m_logDirectory)) {
                return false;
            }
        }
        
        // Create log file with timestamp
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        m_currentLogFile = m_logDirectory + QString("/eti_analyser_safe_%1.log").arg(timestamp);
        
        // Attempt to open log file
        m_logFile = std::make_unique<QFile>(m_currentLogFile);
        if (!m_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
            return false;
        }
        
        m_logStream = std::make_unique<QTextStream>(m_logFile.get());
        
        // Write initialization message to file
        *m_logStream << QString("[%1] [INFO] [Logger] File logging initialized: %2")
                        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                        .arg(m_currentLogFile) << Qt::endl;
        m_logStream->flush();
        
        return true;
    }
    catch (...) {
        // Any exception during file logging setup
        return false;
    }
}

bool Logger::enableLogging(bool enabled)
{
    m_loggingEnabled.store(enabled);
    
    QString message = QString("Logging %1").arg(enabled ? "enabled" : "disabled");
    fallbackLog(LogLevel::Info, message, "Logger");
    
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
    if (!m_loggingEnabled.load()) {
        return;
    }
    
    // Check if message meets minimum log level
    if (level < static_cast<LogLevel>(m_logLevel.load())) {
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

bool Logger::waitForAsyncCompletion(int timeoutMs)
{
    if (m_fullyInitialized.load()) {
        return true;
    }
    
    // Wait for delayed initialization with timeout
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(timeoutMs);
    
    while (!m_fullyInitialized.load() && timeoutTimer.isActive()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QThread::msleep(10);
    }
    
    return m_fullyInitialized.load();
}

qint64 Logger::getMemoryUsage() const
{
    QMutexLocker locker(&m_messagesMutex);
    return m_recentMessages.size() * sizeof(LogMessage);
}

QVariantHash Logger::getPerformanceStatistics() const
{
    QMutexLocker locker(&m_statsMutex);
    
    QVariantHash stats;
    stats["message_count"] = m_messageCount;
    stats["uptime_seconds"] = m_startTime.secsTo(QDateTime::currentDateTime());
    stats["file_logging_available"] = m_fileLoggingAvailable.load();
    stats["fully_initialized"] = m_fullyInitialized.load();
    stats["log_file"] = m_currentLogFile;
    stats["memory_usage"] = getMemoryUsage();
    
    {
        QMutexLocker msgLocker(&m_messagesMutex);
        stats["recent_message_count"] = m_recentMessages.size();
    }
    
    return stats;
}

void Logger::flush()
{
    if (m_fileLoggingAvailable.load()) {
        QMutexLocker locker(&m_fileMutex);
        if (m_logStream) {
            m_logStream->flush();
        }
        if (m_logFile) {
            m_logFile->flush();
        }
    }
}

bool Logger::setLogDirectory(const QString& directory)
{
    QMutexLocker locker(&m_fileMutex);
    
    QDir dir(directory);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            return false;
        }
    }
    
    m_logDirectory = directory;
    return true;
}

QString Logger::getLogDirectory() const
{
    QMutexLocker locker(&m_fileMutex);
    if (!m_logDirectory.isEmpty()) {
        return m_logDirectory;
    }
    
    // Fallback directory
    return "./logs";
}

QString Logger::getCurrentLogFilePath() const
{
    QMutexLocker locker(&m_fileMutex);
    return m_currentLogFile;
}

QStringList Logger::getLogFileList() const
{
    QString logDir = getLogDirectory();
    QDir dir(logDir);
    QStringList filters;
    filters << "eti_analyser_*.log";
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Time);
    QStringList filePaths;
    
    for (const QFileInfo& fileInfo : files) {
        filePaths << fileInfo.absoluteFilePath();
    }
    
    return filePaths;
}

QList<Logger::LogMessage> Logger::getRecentMessages(int maxCount) const
{
    QMutexLocker locker(&m_messagesMutex);
    
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
    // Always use fallback logging for immediate output (never hangs)
    fallbackLog(level, message, category);
    
    // Create log message for storage
    LogMessage logMsg(level, message, category);
    
    // Add to recent messages (thread-safe)
    {
        QMutexLocker locker(&m_messagesMutex);
        m_recentMessages.append(logMsg);
        if (m_recentMessages.size() > MAX_RECENT_MESSAGES) {
            m_recentMessages.removeFirst();
        }
    }
    
    // Write to file if available (non-blocking)
    if (m_fileLoggingAvailable.load()) {
        writeLogMessage(logMsg);
    }
    
    // Update statistics
    {
        QMutexLocker locker(&m_statsMutex);
        m_messageCount++;
    }
    
    // Emit signal for GUI integration (safe)
    QTimer::singleShot(0, this, [this, level, message, category, logMsg]() {
        emit logMessageAdded(level, message, category, logMsg.timestamp);
    });
}

void Logger::fallbackLog(LogLevel level, const QString& message, const QString& category)
{
    // Always safe fallback using qDebug() - never hangs
    QString formattedMessage = QString("[%1] [%2] %3: %4")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
        .arg(getLevelString(level))
        .arg(category.isEmpty() ? "General" : category)
        .arg(message);
    
    // Use appropriate Qt logging based on level
    switch (level) {
        case LogLevel::Debug:
            qDebug().noquote() << formattedMessage;
            break;
        case LogLevel::Info:
            qInfo().noquote() << formattedMessage;
            break;
        case LogLevel::Warning:
            qWarning().noquote() << formattedMessage;
            break;
        case LogLevel::Error:
        case LogLevel::Critical:
            qCritical().noquote() << formattedMessage;
            break;
    }
}

void Logger::writeLogMessage(const LogMessage& message)
{
    if (!m_fileLoggingAvailable.load()) {
        return;
    }
    
    try {
        QMutexLocker locker(&m_fileMutex);
        if (m_logStream && m_logFile && m_logFile->isOpen()) {
            QString formattedMessage = formatLogMessage(message);
            *m_logStream << formattedMessage << Qt::endl;
            m_logStream->flush();
        }
    }
    catch (...) {
        // File write failed, but don't hang the application
        // Continue with fallback logging only
        m_fileLoggingAvailable.store(false);
    }
}

QString Logger::formatLogMessage(const LogMessage& message) const
{
    QString formatted = QString("[%1] [%2] %3: %4")
        .arg(message.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz"))
        .arg(getLevelString(message.level))
        .arg(message.category.isEmpty() ? "General" : message.category)
        .arg(message.message);
    
    return formatted;
}

QString Logger::getLevelString(LogLevel level) const
{
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARNING";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Critical: return "CRITICAL";
        default: return "UNKNOWN";
    }
}