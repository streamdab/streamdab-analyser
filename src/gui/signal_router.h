#pragma once

#include <QObject>
#include <QMetaObject>
#include <QMutex>
#include <memory>

class ProgressManager;
class StatusManager;
class ErrorIndicatorSystem;
class RealTimeController;

/**
 * @class SignalRouter
 * @brief Professional signal/slot connection management for ETI analysis workflow
 * 
 * This class manages all signal/slot connections between professional components,
 * ensuring thread-safe operation and optimal performance for >900 FPS processing.
 */
class SignalRouter : public QObject
{
    Q_OBJECT

public:
    explicit SignalRouter(QObject* parent = nullptr);

    /**
     * @brief Initialize professional component connections
     * @param progressManager Progress management system
     * @param statusManager Status message system  
     * @param errorSystem Error indication system
     * @param realTimeController Real-time mode controller
     */
    void initializeConnections(ProgressManager* progressManager,
                              StatusManager* statusManager,
                              ErrorIndicatorSystem* errorSystem,
                              RealTimeController* realTimeController);

    /**
     * @brief Connect ETI processing signals (thread-safe)
     * @param source ETI processing object
     */
    void connectETIProcessingSignals(QObject* source);

    /**
     * @brief Connect UI component signals (UI thread only)
     * @param uiComponent UI widget or component
     */
    void connectUIComponentSignals(QObject* uiComponent);

signals:
    // Professional signal forwarding for system integration
    void frameProcessed(quint64 frameNumber, quint64 totalFrames);
    void statusChanged(const QString& status);
    void errorOccurred(const QString& errorMessage);
    void realTimeModeChanged(bool enabled);

private:
    // Professional component references
    ProgressManager* m_progressManager;
    StatusManager* m_statusManager;
    ErrorIndicatorSystem* m_errorSystem;
    RealTimeController* m_realTimeController;
    
    // Thread safety
    QMutex m_connectionMutex;
    bool m_initialized;
};