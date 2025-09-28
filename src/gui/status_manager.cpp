#include "status_manager.h"
#include <QWidget>
#include <QApplication>
#include <QThread>
#include <QDateTime>
#include <QDebug>

StatusManager::StatusManager(QWidget* parent)
    : QObject(parent)
    , m_statusLabel(nullptr)
    , m_parentWidget(parent)
    , m_processTimer(new QTimer(this))
    , m_cleanupTimer(new QTimer(this))
    , m_lastUpdateTime(0)
    , m_messageCounter(0)
    , m_droppedMessages(0)
{
    // Initialize performance metrics
    m_performanceMetrics.messagesPerSecond = 0;
    m_performanceMetrics.queueSize = 0;
    m_performanceMetrics.averageLatency = 0.0;
    m_performanceMetrics.droppedMessages = 0;
    
    // Setup message processing timer
    m_processTimer->setSingleShot(false);
    m_processTimer->setInterval(PROCESS_INTERVAL_MS);
    connect(m_processTimer, &QTimer::timeout, 
            this, &StatusManager::processMessageQueue);
    
    // Setup cleanup timer
    m_cleanupTimer->setSingleShot(false);
    m_cleanupTimer->setInterval(CLEANUP_INTERVAL_MS);
    connect(m_cleanupTimer, &QTimer::timeout,
            this, &StatusManager::cleanupExpiredMessages);
    
    // Start timers
    m_processTimer->start();
    m_cleanupTimer->start();
    
    // Initialize timing
    m_lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
    
    qDebug() << "StatusManager: Professional status system initialized";
}

StatusManager::~StatusManager()
{
    // Stop timers
    if (m_processTimer) {
        m_processTimer->stop();
    }
    if (m_cleanupTimer) {
        m_cleanupTimer->stop();
    }
    
    // Clear message queue
    QMutexLocker locker(&m_messageMutex);
    m_messageQueue.clear();
    
    qDebug() << "StatusManager: Professional cleanup completed";
}

void StatusManager::setStatusLabel(QLabel* statusLabel)
{
    ensureUIThread("setStatusLabel");
    
    m_statusLabel = statusLabel;
    
    if (m_statusLabel) {
        // Configure label for professional appearance
        m_statusLabel->setWordWrap(false);
        m_statusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        
        qDebug() << "StatusManager: Status label configured";
    }
}

void StatusManager::showStatus(const QString& message, StatusLevel level, 
                              const QString& context, int duration)
{
    if (message.isEmpty()) {
        return;
    }
    
    StatusMessage statusMsg(message, level, context, 
                           duration > 0 ? duration : 3000);
    enqueueMessage(statusMsg);
}

void StatusManager::showPersistentStatus(const QString& message, StatusLevel level, 
                                        const QString& context)
{
    if (message.isEmpty()) {
        return;
    }
    
    StatusMessage statusMsg(message, level, context, 0);
    statusMsg.persistent = true;
    enqueueMessage(statusMsg);
}

void StatusManager::clearStatus()
{
    QMutexLocker locker(&m_messageMutex);
    m_messageQueue.clear();
    m_currentMessage = StatusMessage();
    
    // Update UI
    QMetaObject::invokeMethod(this, "updateStatusLabelSafely", Qt::QueuedConnection);
}

void StatusManager::clearStatusByLevel(StatusLevel level)
{
    QMutexLocker locker(&m_messageMutex);
    
    // Remove from queue
    QQueue<StatusMessage> filteredQueue;
    while (!m_messageQueue.isEmpty()) {
        StatusMessage msg = m_messageQueue.dequeue();
        if (msg.level != level) {
            filteredQueue.enqueue(msg);
        }
    }
    m_messageQueue = filteredQueue;
    
    // Clear current if matches level
    if (m_currentMessage.level == level) {
        m_currentMessage = StatusMessage();
        QMetaObject::invokeMethod(this, "updateStatusLabelSafely", Qt::QueuedConnection);
    }
}

void StatusManager::clearStatusByContext(const QString& context)
{
    if (context.isEmpty()) {
        return;
    }
    
    QMutexLocker locker(&m_messageMutex);
    
    // Remove from queue
    QQueue<StatusMessage> filteredQueue;
    while (!m_messageQueue.isEmpty()) {
        StatusMessage msg = m_messageQueue.dequeue();
        if (msg.context != context) {
            filteredQueue.enqueue(msg);
        }
    }
    m_messageQueue = filteredQueue;
    
    // Clear current if matches context
    if (m_currentMessage.context == context) {
        m_currentMessage = StatusMessage();
        QMetaObject::invokeMethod(this, "updateStatusLabelSafely", Qt::QueuedConnection);
    }
}

StatusManager::StatusMessage StatusManager::getCurrentStatus() const
{
    QMutexLocker locker(&m_messageMutex);
    return m_currentMessage;
}

bool StatusManager::hasQueuedMessages() const
{
    QMutexLocker locker(&m_messageMutex);
    return !m_messageQueue.isEmpty();
}

StatusManager::StatusPerformanceMetrics StatusManager::getPerformanceMetrics() const
{
    QMutexLocker locker(&m_performanceMutex);
    return m_performanceMetrics;
}

void StatusManager::onFrameProcessed(quint64 frameNumber, quint64 totalFrames)
{
    QString statusText;
    if (totalFrames > 0) {
        double percentage = (static_cast<double>(frameNumber) / totalFrames) * 100.0;
        statusText = QString("Frame %1 processed (%2%)")
                    .arg(frameNumber)
                    .arg(QString::number(percentage, 'f', 1));
    } else {
        statusText = QString("Frame %1 processed").arg(frameNumber);
    }
    
    showStatus(statusText, StatusLevel::Info, "ETI Processing", 1000);
}

void StatusManager::onStatusChanged(const QString& status)
{
    showStatus(status, StatusLevel::Info, "System");
}

void StatusManager::onErrorOccurred(const QString& errorMessage, const QString& context)
{
    QString fullContext = context.isEmpty() ? "Error" : context;
    showStatus(errorMessage, StatusLevel::Error, fullContext, 5000);
    
    // Emit critical error signal for severe issues
    if (errorMessage.contains("critical", Qt::CaseInsensitive) ||
        errorMessage.contains("fatal", Qt::CaseInsensitive)) {
        emit criticalError(errorMessage);
    }
}

void StatusManager::onSuccessMessage(const QString& successMessage, const QString& context)
{
    QString fullContext = context.isEmpty() ? "Success" : context;
    showStatus(successMessage, StatusLevel::Success, fullContext, 2000);
}

void StatusManager::onComplianceStatus(const QString& complianceStatus, bool isValid)
{
    StatusLevel level = isValid ? StatusLevel::Success : StatusLevel::Warning;
    showStatus(complianceStatus, level, "ETSI Compliance", 3000);
}

void StatusManager::processMessageQueue()
{
    QMutexLocker locker(&m_messageMutex);
    
    // Check if current message should be replaced
    bool shouldUpdateCurrent = false;
    
    if (m_currentMessage.text.isEmpty()) {
        // No current message, show next if available
        shouldUpdateCurrent = true;
    } else if (!m_currentMessage.persistent) {
        // Check if current message has expired
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        qint64 messageAge = currentTime - m_currentMessage.timestamp;
        if (messageAge >= m_currentMessage.duration) {
            shouldUpdateCurrent = true;
        }
    }
    
    // Update to next message if needed
    if (shouldUpdateCurrent && !m_messageQueue.isEmpty()) {
        m_currentMessage = m_messageQueue.dequeue();
        
        // Update performance metrics
        updatePerformanceMetrics();
        
        // Emit status change signal
        emit statusChanged(m_currentMessage);
        
        // Update UI (thread-safe)
        locker.unlock();
        QMetaObject::invokeMethod(this, "updateStatusLabelSafely", Qt::QueuedConnection);
    }
}

void StatusManager::updateStatusLabelSafely()
{
    // Ensure this runs on UI thread only
    if (!isUIThread()) {
        QMetaObject::invokeMethod(this, "updateStatusLabelSafely", Qt::QueuedConnection);
        return;
    }
    
    if (!m_statusLabel) {
        return;
    }
    
    StatusMessage currentMsg;
    {
        QMutexLocker locker(&m_messageMutex);
        currentMsg = m_currentMessage;
    }
    
    if (currentMsg.text.isEmpty()) {
        m_statusLabel->setText("Ready");
        m_statusLabel->setStyleSheet("");
        return;
    }
    
    // Format and display message
    QString displayText = formatStatusText(currentMsg);
    QString styleSheet = getStyleSheetForLevel(currentMsg.level);
    
    m_statusLabel->setText(displayText);
    m_statusLabel->setStyleSheet(styleSheet);
}

void StatusManager::cleanupExpiredMessages()
{
    QMutexLocker locker(&m_messageMutex);
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    // Clean up expired messages from queue
    QQueue<StatusMessage> cleanQueue;
    while (!m_messageQueue.isEmpty()) {
        StatusMessage msg = m_messageQueue.dequeue();
        qint64 messageAge = currentTime - msg.timestamp;
        
        // Keep message if not expired or if persistent
        if (msg.persistent || messageAge < (msg.duration + 10000)) { // 10s grace period
            cleanQueue.enqueue(msg);
        }
    }
    m_messageQueue = cleanQueue;
    
    // Update performance metrics with queue size
    QMutexLocker perfLocker(&m_performanceMutex);
    m_performanceMetrics.queueSize = m_messageQueue.size();
    m_performanceMetrics.droppedMessages = m_droppedMessages;
}

void StatusManager::enqueueMessage(const StatusMessage& message)
{
    QMutexLocker locker(&m_messageMutex);
    
    // Check queue size limit
    if (m_messageQueue.size() >= MAX_QUEUE_SIZE) {
        dropOldestMessages();
    }
    
    m_messageQueue.enqueue(message);
    m_messageCounter++;
}

QString StatusManager::formatStatusText(const StatusMessage& message) const
{
    if (message.context.isEmpty()) {
        return message.text;
    } else {
        return QString("[%1] %2").arg(message.context, message.text);
    }
}

QString StatusManager::getStyleSheetForLevel(StatusLevel level) const
{
    switch (level) {
        case StatusLevel::Success:
            return "color: #008f00; font-weight: bold;";
        case StatusLevel::Warning:
            return "color: #ff8c00; font-weight: bold;";
        case StatusLevel::Error:
        case StatusLevel::Critical:
            return "color: #cc0000; font-weight: bold;";
        case StatusLevel::Info:
        default:
            return "color: #333333;";
    }
}

void StatusManager::updatePerformanceMetrics()
{
    QMutexLocker locker(&m_performanceMutex);
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 timeDelta = currentTime - m_lastUpdateTime;
    
    if (timeDelta >= 1000) { // Update every second
        m_performanceMetrics.messagesPerSecond = 
            static_cast<int>((m_messageCounter * 1000.0) / timeDelta);
        m_performanceMetrics.averageLatency = 
            static_cast<double>(timeDelta) / qMax(m_messageCounter, 1);
        
        // Reset counters
        m_messageCounter = 0;
        m_lastUpdateTime = currentTime;
        
        // Emit performance update
        emit performanceUpdate(m_performanceMetrics);
    }
}

bool StatusManager::isUIThread() const
{
    return QThread::currentThread() == QApplication::instance()->thread();
}

void StatusManager::ensureUIThread(const QString& functionName) const
{
    if (!isUIThread()) {
        qWarning() << "StatusManager::" << functionName 
                   << "called from non-UI thread - this may cause issues";
    }
}

void StatusManager::dropOldestMessages()
{
    // Drop oldest non-persistent messages first
    QQueue<StatusMessage> tempQueue;
    bool droppedAny = false;
    
    while (!m_messageQueue.isEmpty()) {
        StatusMessage msg = m_messageQueue.dequeue();
        if (!msg.persistent && !droppedAny) {
            // Drop this message
            droppedAny = true;
            m_droppedMessages++;
        } else {
            tempQueue.enqueue(msg);
        }
    }
    
    // If no non-persistent messages, drop oldest regardless
    if (!droppedAny && !tempQueue.isEmpty()) {
        tempQueue.dequeue(); // Drop oldest
        m_droppedMessages++;
    }
    
    m_messageQueue = tempQueue;
}

QString statusLevelToString(StatusManager::StatusLevel level)
{
    switch (level) {
        case StatusManager::StatusLevel::Info:
            return QObject::tr("Info");
        case StatusManager::StatusLevel::Success:
            return QObject::tr("Success");
        case StatusManager::StatusLevel::Warning:
            return QObject::tr("Warning");
        case StatusManager::StatusLevel::Error:
            return QObject::tr("Error");
        case StatusManager::StatusLevel::Critical:
            return QObject::tr("Critical");
        default:
            return QObject::tr("Unknown");
    }
}