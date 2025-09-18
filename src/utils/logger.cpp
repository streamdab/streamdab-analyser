#include "logger.h"
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextDocument>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QWaitCondition>
#include <QPrinter>
#include <QPainter>
#include <algorithm>

/**
 * @brief Logging Worker for Asynchronous Operation
 * 
 * Separate thread worker for handling log message processing
 * without blocking the main application thread.
 */
class Logger::LoggingWorker : public QObject
{
    Q_OBJECT

public:
    explicit LoggingWorker(Logger* logger) 
        : QObject(nullptr), m_logger(logger), m_shutdown(false) {}

    void shutdown() { m_shutdown = true; }

public slots:
    void processMessages() {
        while (!m_shutdown) {
            QMutexLocker locker(&m_logger->m_mutex);
            
            if (m_logger->m_messageQueue.isEmpty()) {
                m_logger->m_waitCondition.wait(&m_logger->m_mutex, 100);
                continue;
            }
            
            LogMessage message = m_logger->m_messageQueue.dequeue();
            locker.unlock();
            
            m_logger->writeLogMessage(message);
        }
    }

private:
    Logger* m_logger;
    bool m_shutdown;
};

/**
 * @brief Logger Implementation
 * 
 * Professional logging framework with TDD-validated functionality.
 */

Logger::Logger(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_loggingEnabled(false)
    , m_asyncLoggingEnabled(false)
    , m_logLevel(LogLevel::Info)
    , m_outputFormat(OutputFormat::PlainText)
    , m_maxLogFileSize(DEFAULT_MAX_FILE_SIZE)
    , m_maxLogFileCount(DEFAULT_MAX_FILE_COUNT)
    , m_maxMemoryUsage(DEFAULT_MAX_MEMORY)
    , m_bufferSize(DEFAULT_BUFFER_SIZE)
    , m_messageCount(0)
    , m_bytesWritten(0)
    , m_startTime(QDateTime::currentDateTime())
{
    // Set default log directory
    m_logDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    
    // Initialize timers
    m_maintenanceTimer = new QTimer(this);
    m_maintenanceTimer->setInterval(MAINTENANCE_INTERVAL_MS);
    connect(m_maintenanceTimer, &QTimer::timeout, this, &Logger::performMaintenance);
    
    m_flushTimer = new QTimer(this);
    m_flushTimer->setInterval(FLUSH_INTERVAL_MS);
    connect(m_flushTimer, &QTimer::timeout, this, &Logger::flush);
    
    // Load default configuration
    loadDefaultConfiguration();
    
    // Initialize the system
    initialize();
}

Logger::~Logger()
{
    if (m_loggingEnabled) {
        enableLogging(false);
    }
    
    // Cleanup worker thread
    if (m_worker && m_workerThread) {
        m_worker->shutdown();
        m_workerThread->quit();
        m_workerThread->wait(5000);
    }
}

bool Logger::initialize()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_initialized) {
        return true;
    }
    
    // Create log directory if it doesn't exist
    QDir logDir;
    if (!logDir.mkpath(m_logDirectory)) {
        qWarning() << "Logger: Failed to create log directory:" << m_logDirectory;
        return false;
    }
    
    // Verify directory is writable
    QFileInfo dirInfo(m_logDirectory);
    if (!dirInfo.isWritable()) {
        qWarning() << "Logger: Log directory is not writable:" << m_logDirectory;
        return false;
    }
    
    m_initialized = true;
    qDebug() << "Logger: Initialized successfully, log directory:" << m_logDirectory;
    return true;
}

bool Logger::enableLogging(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        qWarning() << "Logger: Cannot enable logging - system not initialized";
        return false;
    }
    
    if (m_loggingEnabled == enabled) {
        return true; // Already in desired state
    }
    
    m_loggingEnabled = enabled;
    
    if (enabled) {
        // Create new log file
        if (!createNewLogFile()) {
            qWarning() << "Logger: Failed to create log file";
            m_loggingEnabled = false;
            return false;
        }
        
        // Start timers
        m_maintenanceTimer->start();
        m_flushTimer->start();
        
        // Setup async worker if enabled
        if (m_asyncLoggingEnabled) {
            m_worker = std::make_unique<LoggingWorker>(this);
            m_workerThread = std::make_unique<QThread>();
            m_worker->moveToThread(m_workerThread.get());
            
            connect(m_workerThread.get(), &QThread::started, 
                    m_worker.get(), &LoggingWorker::processMessages);
            
            m_workerThread->start();
        }
        
        emit loggingStarted();
        qDebug() << "Logger: Logging enabled, file:" << m_currentLogFile;
    } else {
        // Stop timers
        m_maintenanceTimer->stop();
        m_flushTimer->stop();
        
        // Flush remaining data
        flush();
        
        // Shutdown async worker
        if (m_worker && m_workerThread) {
            m_worker->shutdown();
            m_workerThread->quit();
            m_workerThread->wait();
            m_worker.reset();
            m_workerThread.reset();
        }
        
        // Close log file
        if (m_logStream) {
            m_logStream.reset();
        }
        if (m_logFile) {
            m_logFile->close();
            m_logFile.reset();
        }
        
        emit loggingStopped();
        qDebug() << "Logger: Logging disabled";
    }
    
    return true;
}

bool Logger::setLogDirectory(const QString& directory)
{
    QMutexLocker locker(&m_mutex);
    
    QDir dir(directory);
    if (!dir.exists() && !dir.mkpath(directory)) {
        qWarning() << "Logger: Cannot create directory:" << directory;
        return false;
    }
    
    QFileInfo dirInfo(directory);
    if (!dirInfo.isWritable()) {
        qWarning() << "Logger: Directory is not writable:" << directory;
        return false;
    }
    
    m_logDirectory = directory;
    emit configurationChanged();
    return true;
}

void Logger::enableAsyncLogging(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_asyncLoggingEnabled = enabled;
    
    if (m_loggingEnabled) {
        // Restart logging to apply async mode
        bool wasEnabled = m_loggingEnabled;
        locker.unlock();
        enableLogging(false);
        enableLogging(wasEnabled);
    }
}

// ============================================================================
// LOGGING METHODS
// ============================================================================

void Logger::logDebug(const QString& message, const QString& category, 
                      const QVariantHash& metadata)
{
    log(LogLevel::Debug, message, category, metadata);
}

void Logger::logInfo(const QString& message, const QString& category, 
                     const QVariantHash& metadata)
{
    log(LogLevel::Info, message, category, metadata);
}

void Logger::logWarning(const QString& message, const QString& category, 
                        const QVariantHash& metadata)
{
    log(LogLevel::Warning, message, category, metadata);
}

void Logger::logError(const QString& message, const QString& category, 
                      const QVariantHash& metadata)
{
    log(LogLevel::Error, message, category, metadata);
    emit errorMessageAdded(message, QDateTime::currentDateTime());
}

void Logger::logCritical(const QString& message, const QString& category, 
                         const QVariantHash& metadata)
{
    log(LogLevel::Critical, message, category, metadata);
    emit errorMessageAdded(message, QDateTime::currentDateTime());
}

void Logger::log(LogLevel level, const QString& message, const QString& category, 
                 const QVariantHash& metadata)
{
    if (!m_loggingEnabled || level < m_logLevel) {
        return;
    }
    
    LogMessage logMsg(level, message, category);
    logMsg.metadata = metadata;
    
    QMutexLocker locker(&m_mutex);
    
    // Add to recent messages
    m_recentMessages.append(logMsg);
    if (m_recentMessages.size() > MAX_RECENT_MESSAGES) {
        m_recentMessages.removeFirst();
    }
    
    m_messageCount++;
    
    // Emit signal for GUI updates
    emit logMessageAdded(level, message, category, logMsg.timestamp);
    
    if (m_asyncLoggingEnabled) {
        // Queue for async processing
        m_messageQueue.enqueue(logMsg);
        m_waitCondition.wakeOne();
    } else {
        // Process immediately
        locker.unlock();
        writeLogMessage(logMsg);
    }
}

// ============================================================================
// FILE MANAGEMENT
// ============================================================================

QString Logger::getCurrentLogFilePath() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentLogFile;
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

bool Logger::waitForAsyncCompletion(int timeoutMs)
{
    if (!m_asyncLoggingEnabled) {
        return true;
    }
    
    QTime startTime = QTime::currentTime();
    while (startTime.msecsTo(QTime::currentTime()) < timeoutMs) {
        QMutexLocker locker(&m_mutex);
        if (m_messageQueue.isEmpty()) {
            return true;
        }
        locker.unlock();
        QThread::msleep(10);
    }
    
    return false;
}

bool Logger::rotateLogFile()
{
    QMutexLocker locker(&m_mutex);
    performRotation();
    return createNewLogFile();
}

QStringList Logger::getLogFileList() const
{
    QDir logDir(m_logDirectory);
    QStringList filters;
    filters << "*.log";
    
    QStringList files = logDir.entryList(filters, QDir::Files, QDir::Time);
    
    // Convert to full paths
    QStringList fullPaths;
    for (const QString& file : files) {
        fullPaths.append(logDir.absoluteFilePath(file));
    }
    
    return fullPaths;
}

// ============================================================================
// EXPORT FUNCTIONALITY
// ============================================================================

bool Logger::exportToPdf(const QString& filePath, 
                         const QDateTime& startTime,
                         const QDateTime& endTime) const
{
    QMutexLocker locker(&m_mutex);
    
    // Create HTML content for PDF generation
    QString htmlContent = "<html><head><title>ETI Analyzer Log Export</title></head><body>";
    htmlContent += "<h1>ETI Stream Analyzer - Log Export</h1>";
    htmlContent += QString("<p>Export Date: %1</p>").arg(QDateTime::currentDateTime().toString());
    
    if (!startTime.isNull() || !endTime.isNull()) {
        htmlContent += QString("<p>Time Range: %1 to %2</p>")
                      .arg(startTime.isNull() ? "Beginning" : startTime.toString())
                      .arg(endTime.isNull() ? "End" : endTime.toString());
    }
    
    htmlContent += "<table border='1' cellpadding='3' cellspacing='0'>";
    htmlContent += "<tr><th>Timestamp</th><th>Level</th><th>Category</th><th>Message</th></tr>";
    
    // Add log messages within time range
    for (const LogMessage& msg : m_recentMessages) {
        if (!startTime.isNull() && msg.timestamp < startTime) continue;
        if (!endTime.isNull() && msg.timestamp > endTime) continue;
        
        htmlContent += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td></tr>")
                      .arg(msg.timestamp.toString())
                      .arg(logLevelToString(msg.level))
                      .arg(msg.category)
                      .arg(msg.message.toHtmlEscaped());
    }
    
    htmlContent += "</table></body></html>";
    
    // Generate PDF
    QTextDocument document;
    document.setHtml(htmlContent);
    
    QPrinter printer;
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    printer.setPageSize(QPageSize::A4);
    
    document.print(&printer);
    
    return QFile::exists(filePath);
}

bool Logger::exportToHtml(const QString& filePath,
                          const QDateTime& startTime,
                          const QDateTime& endTime) const
{
    QMutexLocker locker(&m_mutex);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream stream(&file);
    
    stream << "<!DOCTYPE html>\n<html>\n<head>\n";
    stream << "<title>ETI Analyzer Log Export</title>\n";
    stream << "<style>\n";
    stream << "body { font-family: Arial, sans-serif; }\n";
    stream << "table { border-collapse: collapse; width: 100%; }\n";
    stream << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
    stream << "th { background-color: #f2f2f2; }\n";
    stream << ".error { color: #d32f2f; }\n";
    stream << ".warning { color: #f57c00; }\n";
    stream << ".info { color: #1976d2; }\n";
    stream << "</style>\n</head>\n<body>\n";
    
    stream << "<h1>ETI Stream Analyzer - Log Export</h1>\n";
    stream << "<p>Export Date: " << QDateTime::currentDateTime().toString() << "</p>\n";
    
    stream << "<table>\n";
    stream << "<tr><th>Timestamp</th><th>Level</th><th>Category</th><th>Message</th></tr>\n";
    
    for (const LogMessage& msg : m_recentMessages) {
        if (!startTime.isNull() && msg.timestamp < startTime) continue;
        if (!endTime.isNull() && msg.timestamp > endTime) continue;
        
        QString levelClass;
        switch (msg.level) {
            case LogLevel::Error:
            case LogLevel::Critical:
                levelClass = "error";
                break;
            case LogLevel::Warning:
                levelClass = "warning";
                break;
            default:
                levelClass = "info";
                break;
        }
        
        stream << QString("<tr class='%1'><td>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>\n")
                  .arg(levelClass)
                  .arg(msg.timestamp.toString())
                  .arg(logLevelToString(msg.level))
                  .arg(msg.category.toHtmlEscaped())
                  .arg(msg.message.toHtmlEscaped());
    }
    
    stream << "</table>\n</body>\n</html>\n";
    
    return true;
}

bool Logger::exportToCsv(const QString& filePath,
                         const QDateTime& startTime,
                         const QDateTime& endTime) const
{
    QMutexLocker locker(&m_mutex);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream stream(&file);
    
    // Write CSV header
    stream << "Timestamp,Level,Category,Thread,Message\n";
    
    // Write log messages
    for (const LogMessage& msg : m_recentMessages) {
        if (!startTime.isNull() && msg.timestamp < startTime) continue;
        if (!endTime.isNull() && msg.timestamp > endTime) continue;
        
        stream << QString("\"%1\",\"%2\",\"%3\",\"%4\",\"%5\"\n")
                  .arg(msg.timestamp.toString())
                  .arg(logLevelToString(msg.level))
                  .arg(msg.category)
                  .arg(msg.threadId)
                  .arg(msg.message.replace("\"", "\"\""));  // Escape quotes
    }
    
    return true;
}

// ============================================================================
// CONFIGURATION PERSISTENCE
// ============================================================================

bool Logger::saveConfiguration(const QString& filePath) const
{
    QJsonObject config;
    config["logLevel"] = static_cast<int>(m_logLevel);
    config["outputFormat"] = static_cast<int>(m_outputFormat);
    config["logDirectory"] = m_logDirectory;
    config["maxLogFileSize"] = m_maxLogFileSize;
    config["maxLogFileCount"] = m_maxLogFileCount;
    config["maxMemoryUsage"] = m_maxMemoryUsage;
    config["bufferSize"] = m_bufferSize;
    config["asyncLoggingEnabled"] = m_asyncLoggingEnabled;
    
    QJsonDocument doc(config);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(doc.toJson());
    return true;
}

bool Logger::loadConfiguration(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) {
        return false;
    }
    
    QJsonObject config = doc.object();
    
    if (config.contains("logLevel")) {
        m_logLevel = static_cast<LogLevel>(config["logLevel"].toInt());
    }
    if (config.contains("outputFormat")) {
        m_outputFormat = static_cast<OutputFormat>(config["outputFormat"].toInt());
    }
    if (config.contains("logDirectory")) {
        setLogDirectory(config["logDirectory"].toString());
    }
    if (config.contains("maxLogFileSize")) {
        m_maxLogFileSize = config["maxLogFileSize"].toDouble();
    }
    if (config.contains("maxLogFileCount")) {
        m_maxLogFileCount = config["maxLogFileCount"].toInt();
    }
    if (config.contains("maxMemoryUsage")) {
        m_maxMemoryUsage = config["maxMemoryUsage"].toDouble();
    }
    if (config.contains("bufferSize")) {
        m_bufferSize = config["bufferSize"].toInt();
    }
    if (config.contains("asyncLoggingEnabled")) {
        enableAsyncLogging(config["asyncLoggingEnabled"].toBool());
    }
    
    emit configurationChanged();
    return true;
}

// ============================================================================
// PERFORMANCE MONITORING
// ============================================================================

qint64 Logger::getMemoryUsage() const
{
    QMutexLocker locker(&m_mutex);
    
    qint64 usage = sizeof(Logger);
    usage += m_messageQueue.size() * sizeof(LogMessage);
    usage += m_recentMessages.size() * sizeof(LogMessage);
    
    // Estimate string memory usage
    for (const LogMessage& msg : m_recentMessages) {
        usage += msg.message.size() * sizeof(QChar);
        usage += msg.category.size() * sizeof(QChar);
        usage += msg.threadId.size() * sizeof(QChar);
    }
    
    return usage;
}

QVariantHash Logger::getPerformanceStatistics() const
{
    QMutexLocker locker(&m_mutex);
    
    QVariantHash stats;
    stats["messageCount"] = m_messageCount;
    stats["bytesWritten"] = m_bytesWritten;
    stats["memoryUsage"] = getMemoryUsage();
    stats["loggingEnabled"] = m_loggingEnabled;
    stats["asyncLoggingEnabled"] = m_asyncLoggingEnabled;
    stats["queueSize"] = m_messageQueue.size();
    stats["recentMessageCount"] = m_recentMessages.size();
    stats["uptimeSeconds"] = m_startTime.secsTo(QDateTime::currentDateTime());
    
    return stats;
}

QList<Logger::LogMessage> Logger::getRecentMessages(int maxCount) const
{
    QMutexLocker locker(&m_mutex);
    
    if (maxCount <= 0 || maxCount >= m_recentMessages.size()) {
        return m_recentMessages;
    }
    
    return m_recentMessages.mid(m_recentMessages.size() - maxCount);
}

// ============================================================================
// PRIVATE IMPLEMENTATION
// ============================================================================

void Logger::writeLogMessage(const LogMessage& message)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_logStream) {
        return;
    }
    
    QString formattedMessage = formatLogMessage(message);
    *m_logStream << formattedMessage << Qt::endl;
    
    m_bytesWritten += formattedMessage.toUtf8().size();
    
    // Check if rotation is needed
    if (needsRotation()) {
        locker.unlock();
        performRotation();
        createNewLogFile();
    }
}

QString Logger::formatLogMessage(const LogMessage& message) const
{
    QString formatted;
    
    switch (m_outputFormat) {
        case OutputFormat::Json: {
            QJsonObject obj;
            obj["timestamp"] = message.timestamp.toString(Qt::ISODate);
            obj["level"] = logLevelToString(message.level);
            obj["category"] = message.category;
            obj["thread"] = message.threadId;
            obj["message"] = message.message;
            
            if (!message.metadata.isEmpty()) {
                QJsonObject metadata;
                for (auto it = message.metadata.begin(); it != message.metadata.end(); ++it) {
                    metadata[it.key()] = QJsonValue::fromVariant(it.value());
                }
                obj["metadata"] = metadata;
            }
            
            formatted = QJsonDocument(obj).toJson(QJsonDocument::Compact);
            break;
        }
        
        case OutputFormat::Xml: {
            formatted = QString("<log><timestamp>%1</timestamp><level>%2</level>"
                              "<category>%3</category><thread>%4</thread>"
                              "<message>%5</message></log>")
                       .arg(message.timestamp.toString(Qt::ISODate))
                       .arg(logLevelToString(message.level))
                       .arg(message.category.toHtmlEscaped())
                       .arg(message.threadId)
                       .arg(message.message.toHtmlEscaped());
            break;
        }
        
        default: // PlainText
            formatted = QString("[%1] %2 [%3] %4: %5")
                       .arg(message.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz"))
                       .arg(logLevelToString(message.level).toUpper())
                       .arg(message.threadId)
                       .arg(message.category.isEmpty() ? "General" : message.category)
                       .arg(message.message);
            break;
    }
    
    return formatted;
}

bool Logger::needsRotation() const
{
    if (!m_logFile) {
        return false;
    }
    
    return m_logFile->size() >= m_maxLogFileSize;
}

void Logger::performRotation()
{
    if (m_logStream) {
        m_logStream.reset();
    }
    if (m_logFile) {
        QString oldFile = m_logFile->fileName();
        m_logFile->close();
        m_logFile.reset();
        
        emit logFileRotated(oldFile);
    }
    
    cleanupOldLogFiles();
}

void Logger::cleanupOldLogFiles()
{
    QDir logDir(m_logDirectory);
    QStringList filters;
    filters << "*.log";
    
    QFileInfoList files = logDir.entryInfoList(filters, QDir::Files, QDir::Time);
    
    // Remove oldest files if we exceed the limit
    while (files.size() >= m_maxLogFileCount) {
        QFile::remove(files.last().absoluteFilePath());
        files.removeLast();
    }
}

bool Logger::createNewLogFile()
{
    QString timestamp = generateLogFileTimestamp();
    m_currentLogFile = QDir(m_logDirectory).absoluteFilePath(
        QString("eti_analyzer_%1.log").arg(timestamp));
    
    m_logFile = std::make_unique<QFile>(m_currentLogFile);
    if (!m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Logger: Failed to create log file:" << m_currentLogFile;
        m_logFile.reset();
        return false;
    }
    
    m_logStream = std::make_unique<QTextStream>(m_logFile.get());
    m_logStream->setCodec("UTF-8");
    
    return true;
}

QString Logger::generateLogFileTimestamp() const
{
    return QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
}

void Logger::loadDefaultConfiguration()
{
    // Default configuration is already set in constructor
}

QString Logger::logLevelToString(LogLevel level) const
{
    switch (level) {
        case LogLevel::Debug: return "Debug";
        case LogLevel::Info: return "Info";
        case LogLevel::Warning: return "Warning";
        case LogLevel::Error: return "Error";
        case LogLevel::Critical: return "Critical";
        default: return "Unknown";
    }
}

void Logger::processPendingMessages()
{
    // This method is intentionally empty - processing is handled by the worker thread
}

void Logger::performMaintenance()
{
    QMutexLocker locker(&m_mutex);
    
    // Cleanup old messages if memory usage is too high
    qint64 memUsage = getMemoryUsage();
    if (memUsage > m_maxMemoryUsage) {
        int removeCount = m_recentMessages.size() / 4; // Remove 25%
        for (int i = 0; i < removeCount && !m_recentMessages.isEmpty(); ++i) {
            m_recentMessages.removeFirst();
        }
    }
    
    // Check for log file rotation
    if (needsRotation()) {
        locker.unlock();
        performRotation();
        createNewLogFile();
    }
}

#include "logger.moc"