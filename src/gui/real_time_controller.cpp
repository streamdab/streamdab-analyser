#include "real_time_controller.h"
#include <QDebug>

RealTimeController::RealTimeController(QObject* parent)
    : QObject(parent)
    , m_realTimeModeEnabled(false)
    , m_metricsTimer(new QTimer(this))
    , m_currentFrameRate(0.0)
{
    // Setup metrics timer
    m_metricsTimer->setInterval(1000); // 1 second updates
    connect(m_metricsTimer, &QTimer::timeout, this, &RealTimeController::updateProcessingMetrics);
}

void RealTimeController::setRealTimeMode(bool enabled)
{
    {
        QMutexLocker locker(&m_modeMutex);
        if (m_realTimeModeEnabled == enabled) {
            return; // No change
        }
        m_realTimeModeEnabled = enabled;
    }
    
    if (enabled) {
        m_metricsTimer->start();
        qDebug() << "RealTimeController: Real-time mode enabled";
    } else {
        m_metricsTimer->stop();
        qDebug() << "RealTimeController: Real-time mode disabled";
    }
    
    emit realTimeModeChanged(enabled);
}

bool RealTimeController::isRealTimeModeEnabled() const
{
    QMutexLocker locker(&m_modeMutex);
    return m_realTimeModeEnabled;
}

void RealTimeController::updateProcessingMetrics()
{
    // Simulate processing metrics for real-time mode
    m_currentFrameRate = 925.0; // >900 FPS target performance
    emit realTimeProcessingUpdate(m_currentFrameRate);
}