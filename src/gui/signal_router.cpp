#include "signal_router.h"
#include "progress_manager.h"
#include "status_manager.h"
#include "error_indicator_system.h"
#include "real_time_controller.h"
#include <QDebug>

SignalRouter::SignalRouter(QObject* parent)
    : QObject(parent)
    , m_progressManager(nullptr)
    , m_statusManager(nullptr)
    , m_errorSystem(nullptr)
    , m_realTimeController(nullptr)
    , m_initialized(false)
{
}

void SignalRouter::initializeConnections(ProgressManager* progressManager,
                                        StatusManager* statusManager,
                                        ErrorIndicatorSystem* errorSystem,
                                        RealTimeController* realTimeController)
{
    QMutexLocker locker(&m_connectionMutex);
    
    m_progressManager = progressManager;
    m_statusManager = statusManager;
    m_errorSystem = errorSystem;
    m_realTimeController = realTimeController;
    
    if (m_progressManager && m_statusManager && m_errorSystem && m_realTimeController) {
        // Connect cross-component signals for professional integration
        
        // Progress to Status integration
        connect(this, &SignalRouter::frameProcessed,
                m_progressManager, &ProgressManager::onFrameProcessed,
                Qt::QueuedConnection);
        
        connect(this, &SignalRouter::frameProcessed,
                m_statusManager, &StatusManager::onFrameProcessed,
                Qt::QueuedConnection);
        
        // Status to Error integration
        connect(this, &SignalRouter::errorOccurred,
                m_statusManager, &StatusManager::onErrorOccurred,
                Qt::QueuedConnection);
        
        connect(this, &SignalRouter::errorOccurred,
                m_errorSystem, &ErrorIndicatorSystem::onErrorOccurred,
                Qt::QueuedConnection);
        
        // Real-time mode integration
        connect(this, &SignalRouter::realTimeModeChanged,
                m_progressManager, &ProgressManager::onRealTimeModeChanged,
                Qt::QueuedConnection);
        
        connect(this, &SignalRouter::realTimeModeChanged,
                m_realTimeController, &RealTimeController::setRealTimeMode,
                Qt::QueuedConnection);
        
        m_initialized = true;
        qDebug() << "SignalRouter: Professional signal connections established";
    }
}

void SignalRouter::connectETIProcessingSignals(QObject* source)
{
    if (!source || !m_initialized) {
        return;
    }
    
    // Connect ETI processing signals with thread-safe queued connections
    connect(source, SIGNAL(frameProcessed(quint64, quint64)),
            this, SIGNAL(frameProcessed(quint64, quint64)),
            Qt::QueuedConnection);
    
    connect(source, SIGNAL(statusChanged(QString)),
            this, SIGNAL(statusChanged(QString)),
            Qt::QueuedConnection);
    
    connect(source, SIGNAL(errorOccurred(QString)),
            this, SIGNAL(errorOccurred(QString)),
            Qt::QueuedConnection);
}

void SignalRouter::connectUIComponentSignals(QObject* uiComponent)
{
    if (!uiComponent || !m_initialized) {
        return;
    }
    
    // Connect UI component signals for professional integration
    connect(uiComponent, SIGNAL(realTimeModeChanged(bool)),
            this, SIGNAL(realTimeModeChanged(bool)),
            Qt::QueuedConnection);
}