#include "error_indicator_system.h"
#include <QWidget>
#include <QApplication>
#include <QThread>
#include <QDateTime>
#include <QDebug>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QColor>

ErrorIndicatorSystem::ErrorIndicatorSystem(QWidget* parent)
    : QObject(parent)
    , m_statusLabel(nullptr)
    , m_progressBar(nullptr)
    , m_processTimer(new QTimer(this))
    , m_cleanupTimer(new QTimer(this))
    , m_lastUpdateTime(0)
    , m_errorCounter(0)
{
    // Initialize performance metrics
    m_performanceMetrics.errorsPerSecond = 0;
    m_performanceMetrics.activeErrors = 0;
    m_performanceMetrics.averageLatency = 0.0;
    m_performanceMetrics.componentsManaged = 0;
    
    // Setup error processing timer
    m_processTimer->setSingleShot(false);
    m_processTimer->setInterval(PROCESS_INTERVAL_MS);
    connect(m_processTimer, &QTimer::timeout, 
            this, &ErrorIndicatorSystem::processErrorQueue);
    
    // Setup cleanup timer
    m_cleanupTimer->setSingleShot(false);
    m_cleanupTimer->setInterval(CLEANUP_INTERVAL_MS);
    connect(m_cleanupTimer, &QTimer::timeout,
            this, &ErrorIndicatorSystem::cleanupExpiredErrors);
    
    // Start timers
    m_processTimer->start();
    m_cleanupTimer->start();
    
    // Initialize timing
    m_lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
    
    qDebug() << "ErrorIndicatorSystem: Professional error system initialized";
}

ErrorIndicatorSystem::~ErrorIndicatorSystem()
{
    // Stop timers
    if (m_processTimer) {
        m_processTimer->stop();
    }
    if (m_cleanupTimer) {
        m_cleanupTimer->stop();
    }
    
    // Clean up animations
    for (auto& component : m_managedComponents) {
        if (component.animation) {
            component.animation->stop();
            delete component.animation;
        }
    }
    
    // Clear error data
    QMutexLocker locker(&m_errorMutex);
    m_errorQueue.clear();
    m_activeErrors.clear();
    
    qDebug() << "ErrorIndicatorSystem: Professional cleanup completed";
}

void ErrorIndicatorSystem::registerComponent(QWidget* component, IndicatorType type, const QString& name)
{
    ensureUIThread("registerComponent");
    
    if (!component) {
        qWarning() << "ErrorIndicatorSystem: Cannot register null component";
        return;
    }
    
    ComponentInfo info;
    info.widget = component;
    info.type = type;
    info.name = name;
    info.originalStyleSheet = component->styleSheet();
    info.animation = nullptr;
    
    m_managedComponents.append(info);
    
    // Update performance metrics
    QMutexLocker locker(&m_performanceMutex);
    m_performanceMetrics.componentsManaged = m_managedComponents.size();
    
    qDebug() << "ErrorIndicatorSystem: Registered component" << name << "type" << static_cast<int>(type);
}

void ErrorIndicatorSystem::registerStatusLabel(QLabel* statusLabel)
{
    ensureUIThread("registerStatusLabel");
    
    m_statusLabel = statusLabel;
    if (m_statusLabel) {
        registerComponent(m_statusLabel, IndicatorType::StatusLabel, "StatusLabel");
    }
}

void ErrorIndicatorSystem::registerProgressBar(QProgressBar* progressBar)
{
    ensureUIThread("registerProgressBar");
    
    m_progressBar = progressBar;
    if (m_progressBar) {
        registerComponent(m_progressBar, IndicatorType::ProgressBar, "ProgressBar");
    }
}

void ErrorIndicatorSystem::showError(const ErrorInfo& errorInfo)
{
    enqueueError(errorInfo);
}

void ErrorIndicatorSystem::showError(const QString& message, ErrorSeverity severity, const QString& context)
{
    ErrorInfo info(message, severity, context);
    enqueueError(info);
}

void ErrorIndicatorSystem::showETSIError(const QString& message, const QString& etsiStandard, 
                                        const QString& section, const QString& recoveryAction)
{
    ErrorInfo info(message, ErrorSeverity::ETSICompliance, "ETSI Compliance");
    
    // Format ETSI reference
    if (!section.isEmpty()) {
        info.etsiReference = QString("%1 Section %2").arg(etsiStandard, section);
    } else {
        info.etsiReference = etsiStandard;
    }
    
    info.recoveryAction = recoveryAction;
    info.requiresAttention = true;
    info.autoRecover = false;
    
    enqueueError(info);
    
    // Emit specific ETSI compliance signal
    emit etsiComplianceViolation(info);
}

void ErrorIndicatorSystem::clearAllErrors()
{
    QMutexLocker locker(&m_errorMutex);
    m_errorQueue.clear();
    m_activeErrors.clear();
    
    // Clear all component indicators
    QMetaObject::invokeMethod(this, "updateComponentIndicators", Qt::QueuedConnection);
    
    // Emit state change
    emit errorStateChanged(false, ErrorSeverity::Info);
}

void ErrorIndicatorSystem::clearErrorsBySeverity(ErrorSeverity severity)
{
    QMutexLocker locker(&m_errorMutex);
    
    // Remove from queue
    QQueue<ErrorInfo> filteredQueue;
    while (!m_errorQueue.isEmpty()) {
        ErrorInfo error = m_errorQueue.dequeue();
        if (error.severity != severity) {
            filteredQueue.enqueue(error);
        }
    }
    m_errorQueue = filteredQueue;
    
    // Remove from active errors
    auto it = m_activeErrors.begin();
    while (it != m_activeErrors.end()) {
        if (it->severity == severity) {
            it = m_activeErrors.erase(it);
        } else {
            ++it;
        }
    }
    
    // Update UI
    QMetaObject::invokeMethod(this, "updateComponentIndicators", Qt::QueuedConnection);
    
    // Emit state change
    ErrorSeverity highestSeverity = getHighestSeverity();
    emit errorStateChanged(!m_activeErrors.isEmpty(), highestSeverity);
}

void ErrorIndicatorSystem::clearErrorsByContext(const QString& context)
{
    if (context.isEmpty()) {
        return;
    }
    
    QMutexLocker locker(&m_errorMutex);
    
    // Remove from queue
    QQueue<ErrorInfo> filteredQueue;
    while (!m_errorQueue.isEmpty()) {
        ErrorInfo error = m_errorQueue.dequeue();
        if (error.context != context) {
            filteredQueue.enqueue(error);
        }
    }
    m_errorQueue = filteredQueue;
    
    // Remove from active errors
    auto it = m_activeErrors.begin();
    while (it != m_activeErrors.end()) {
        if (it->context == context) {
            it = m_activeErrors.erase(it);
        } else {
            ++it;
        }
    }
    
    // Update UI
    QMetaObject::invokeMethod(this, "updateComponentIndicators", Qt::QueuedConnection);
    
    // Emit state change
    ErrorSeverity highestSeverity = getHighestSeverity();
    emit errorStateChanged(!m_activeErrors.isEmpty(), highestSeverity);
}

ErrorIndicatorSystem::ErrorInfo ErrorIndicatorSystem::getCurrentError() const
{
    QMutexLocker locker(&m_errorMutex);
    
    if (m_activeErrors.isEmpty()) {
        return ErrorInfo();
    }
    
    // Return highest severity error
    ErrorInfo highest = m_activeErrors.first();
    for (const auto& error : m_activeErrors) {
        if (error.severity > highest.severity) {
            highest = error;
        }
    }
    
    return highest;
}

bool ErrorIndicatorSystem::hasActiveErrors() const
{
    QMutexLocker locker(&m_errorMutex);
    return !m_activeErrors.isEmpty();
}

int ErrorIndicatorSystem::getErrorCount(ErrorSeverity severity) const
{
    QMutexLocker locker(&m_errorMutex);
    
    int count = 0;
    for (const auto& error : m_activeErrors) {
        if (error.severity == severity) {
            count++;
        }
    }
    return count;
}

ErrorIndicatorSystem::ErrorPerformanceMetrics ErrorIndicatorSystem::getPerformanceMetrics() const
{
    QMutexLocker locker(&m_performanceMutex);
    return m_performanceMetrics;
}

void ErrorIndicatorSystem::onErrorOccurred(const QString& errorMessage)
{
    showError(errorMessage, ErrorSeverity::Error, "System");
}

void ErrorIndicatorSystem::onETSIComplianceError(const QString& complianceError, const QString& standardReference)
{
    showETSIError(complianceError, standardReference, QString(), 
                  "Review ETSI compliance requirements and adjust configuration");
}

void ErrorIndicatorSystem::onWarningOccurred(const QString& warningMessage, const QString& context)
{
    QString fullContext = context.isEmpty() ? "System" : context;
    showError(warningMessage, ErrorSeverity::Warning, fullContext);
}

void ErrorIndicatorSystem::onCriticalError(const QString& criticalError)
{
    ErrorInfo info(criticalError, ErrorSeverity::Critical, "System");
    info.requiresAttention = true;
    info.autoRecover = false;
    info.recoveryAction = "Restart application or contact support";
    
    showError(info);
    
    // Emit critical error signal
    emit criticalErrorRequiresAttention(info);
}

void ErrorIndicatorSystem::processErrorQueue()
{
    QMutexLocker locker(&m_errorMutex);
    
    // Process queued errors
    while (!m_errorQueue.isEmpty() && m_activeErrors.size() < MAX_ACTIVE_ERRORS) {
        ErrorInfo error = m_errorQueue.dequeue();
        m_activeErrors.append(error);
        
        // Update performance metrics
        updatePerformanceMetrics();
        
        // Emit state change
        ErrorSeverity highestSeverity = getHighestSeverity();
        emit errorStateChanged(true, highestSeverity);
    }
    
    // Update UI components
    if (!m_activeErrors.isEmpty()) {
        locker.unlock();
        QMetaObject::invokeMethod(this, "updateComponentIndicators", Qt::QueuedConnection);
    }
}

void ErrorIndicatorSystem::updateComponentIndicators()
{
    // Ensure this runs on UI thread only
    if (!isUIThread()) {
        QMetaObject::invokeMethod(this, "updateComponentIndicators", Qt::QueuedConnection);
        return;
    }
    
    ErrorInfo currentError = getCurrentError();
    
    // Update all registered components
    for (auto& component : m_managedComponents) {
        if (!component.widget) {
            continue;
        }
        
        if (currentError.message.isEmpty()) {
            clearErrorIndication(component);
        } else {
            applyErrorIndication(component, currentError);
        }
    }
}

void ErrorIndicatorSystem::cleanupExpiredErrors()
{
    QMutexLocker locker(&m_errorMutex);
    removeExpiredErrors();
    
    // Update performance metrics
    QMutexLocker perfLocker(&m_performanceMutex);
    m_performanceMetrics.activeErrors = m_activeErrors.size();
}

void ErrorIndicatorSystem::onAnimationFinished()
{
    // Animation completed - can be extended for complex effects
    qDebug() << "ErrorIndicatorSystem: Animation completed";
}

void ErrorIndicatorSystem::enqueueError(const ErrorInfo& errorInfo)
{
    QMutexLocker locker(&m_errorMutex);
    m_errorQueue.enqueue(errorInfo);
    m_errorCounter++;
}

void ErrorIndicatorSystem::applyErrorIndication(const ComponentInfo& component, const ErrorInfo& error)
{
    if (!component.widget) {
        return;
    }
    
    QString styleSheet = getStyleSheetForSeverity(error.severity);
    
    switch (component.type) {
        case IndicatorType::StatusLabel:
            if (QLabel* label = qobject_cast<QLabel*>(component.widget)) {
                label->setText(QString("Error: %1").arg(error.message));
                label->setStyleSheet(styleSheet);
            }
            break;
            
        case IndicatorType::ProgressBar:
            if (QProgressBar* progressBar = qobject_cast<QProgressBar*>(component.widget)) {
                progressBar->setStyleSheet(QString("QProgressBar::chunk { %1 }").arg(styleSheet));
            }
            break;
            
        case IndicatorType::Widget:
            component.widget->setStyleSheet(QString("QWidget { border: 2px solid %1; }")
                                          .arg(getColorForSeverity(error.severity).name()));
            break;
            
        case IndicatorType::Animation:
            startErrorAnimation(component.widget, error.severity);
            break;
            
        default:
            component.widget->setStyleSheet(styleSheet);
            break;
    }
}

void ErrorIndicatorSystem::clearErrorIndication(const ComponentInfo& component)
{
    if (!component.widget) {
        return;
    }
    
    // Stop any running animation
    if (component.animation) {
        component.animation->stop();
    }
    
    // Restore original style
    component.widget->setStyleSheet(component.originalStyleSheet);
    
    // Clear status label text
    if (component.type == IndicatorType::StatusLabel) {
        if (QLabel* label = qobject_cast<QLabel*>(component.widget)) {
            label->setText("Ready");
        }
    }
}

QString ErrorIndicatorSystem::getStyleSheetForSeverity(ErrorSeverity severity) const
{
    switch (severity) {
        case ErrorSeverity::Info:
            return "color: #0078d4; font-weight: normal;";
        case ErrorSeverity::Warning:
            return "color: #ff8c00; font-weight: bold;";
        case ErrorSeverity::Error:
            return "color: #cc0000; font-weight: bold;";
        case ErrorSeverity::Critical:
        case ErrorSeverity::Fatal:
            return "color: #990000; font-weight: bold; background-color: #ffeeee;";
        case ErrorSeverity::ETSICompliance:
            return "color: #663399; font-weight: bold;";
        default:
            return "color: #333333;";
    }
}

QColor ErrorIndicatorSystem::getColorForSeverity(ErrorSeverity severity) const
{
    switch (severity) {
        case ErrorSeverity::Info:
            return QColor(0, 120, 212);      // Blue
        case ErrorSeverity::Warning:
            return QColor(255, 140, 0);      // Orange
        case ErrorSeverity::Error:
            return QColor(204, 0, 0);        // Red
        case ErrorSeverity::Critical:
        case ErrorSeverity::Fatal:
            return QColor(153, 0, 0);        // Dark Red
        case ErrorSeverity::ETSICompliance:
            return QColor(102, 51, 153);     // Purple
        default:
            return QColor(51, 51, 51);       // Dark Gray
    }
}

void ErrorIndicatorSystem::startErrorAnimation(QWidget* widget, ErrorSeverity severity)
{
    if (!widget) {
        return;
    }
    
    // Create opacity effect if needed
    QGraphicsOpacityEffect* effect = qobject_cast<QGraphicsOpacityEffect*>(widget->graphicsEffect());
    if (!effect) {
        effect = new QGraphicsOpacityEffect(widget);
        widget->setGraphicsEffect(effect);
    }
    
    // Create animation
    QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity", this);
    animation->setDuration(severity >= ErrorSeverity::Critical ? 500 : 1000);
    animation->setStartValue(1.0);
    animation->setEndValue(0.3);
    animation->setLoopCount(severity >= ErrorSeverity::Critical ? -1 : 3); // Infinite for critical
    
    connect(animation, &QPropertyAnimation::finished, 
            this, &ErrorIndicatorSystem::onAnimationFinished);
    
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void ErrorIndicatorSystem::updatePerformanceMetrics()
{
    QMutexLocker locker(&m_performanceMutex);
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 timeDelta = currentTime - m_lastUpdateTime;
    
    if (timeDelta >= 1000) { // Update every second
        m_performanceMetrics.errorsPerSecond = 
            static_cast<int>((m_errorCounter * 1000.0) / timeDelta);
        m_performanceMetrics.averageLatency = 
            static_cast<double>(timeDelta) / qMax(m_errorCounter, 1);
        
        // Reset counters
        m_errorCounter = 0;
        m_lastUpdateTime = currentTime;
        
        // Emit performance update
        emit performanceUpdate(m_performanceMetrics);
    }
}

bool ErrorIndicatorSystem::isUIThread() const
{
    return QThread::currentThread() == QApplication::instance()->thread();
}

void ErrorIndicatorSystem::ensureUIThread(const QString& functionName) const
{
    if (!isUIThread()) {
        qWarning() << "ErrorIndicatorSystem::" << functionName 
                   << "called from non-UI thread - this may cause issues";
    }
}

ErrorIndicatorSystem::ErrorSeverity ErrorIndicatorSystem::getHighestSeverity() const
{
    if (m_activeErrors.isEmpty()) {
        return ErrorSeverity::Info;
    }
    
    ErrorSeverity highest = ErrorSeverity::Info;
    for (const auto& error : m_activeErrors) {
        if (error.severity > highest) {
            highest = error.severity;
        }
    }
    return highest;
}

void ErrorIndicatorSystem::removeExpiredErrors()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    const qint64 ERROR_LIFETIME_MS = 30000; // 30 seconds
    
    auto it = m_activeErrors.begin();
    while (it != m_activeErrors.end()) {
        if (!it->requiresAttention && 
            (currentTime - it->timestamp) > ERROR_LIFETIME_MS) {
            it = m_activeErrors.erase(it);
        } else {
            ++it;
        }
    }
}

QString errorSeverityToString(ErrorIndicatorSystem::ErrorSeverity severity)
{
    switch (severity) {
        case ErrorIndicatorSystem::ErrorSeverity::Info:
            return QObject::tr("Info");
        case ErrorIndicatorSystem::ErrorSeverity::Warning:
            return QObject::tr("Warning");
        case ErrorIndicatorSystem::ErrorSeverity::Error:
            return QObject::tr("Error");
        case ErrorIndicatorSystem::ErrorSeverity::Critical:
            return QObject::tr("Critical");
        case ErrorIndicatorSystem::ErrorSeverity::ETSICompliance:
            return QObject::tr("ETSI Compliance");
        case ErrorIndicatorSystem::ErrorSeverity::Fatal:
            return QObject::tr("Fatal");
        default:
            return QObject::tr("Unknown");
    }
}

QString getETSIStandardURL(const QString& standard)
{
    // ETSI standards are available at: https://www.etsi.org/standards/
    QString baseUrl = "https://www.etsi.org/deliver/etsi_en/";
    
    if (standard.contains("300 799")) {
        return baseUrl + "300799/300799v010101p.pdf";
    } else if (standard.contains("300 401")) {
        return baseUrl + "300401/300401v020101p.pdf";
    }
    
    // Generic ETSI standards search
    return "https://www.etsi.org/standards/standard?search=" + standard.simplified().replace(" ", "%20");
}