#pragma once

#include <QObject>
#include <QMutex>
#include <QMutexLocker>
#include <QTimer>

/**
 * @class RealTimeController
 * @brief Thread-safe real-time mode management for ETI processing
 */
class RealTimeController : public QObject
{
    Q_OBJECT

public:
    explicit RealTimeController(QObject* parent = nullptr);

    /**
     * @brief Enable/disable real-time mode (thread-safe)
     * @param enabled Real-time mode state
     */
    void setRealTimeMode(bool enabled);

    /**
     * @brief Check if real-time mode is active
     * @return true if real-time mode is enabled
     */
    bool isRealTimeModeEnabled() const;

signals:
    /**
     * @brief Signal emitted when real-time mode changes
     * @param enabled New real-time mode state
     */
    void realTimeModeChanged(bool enabled);

    /**
     * @brief Signal for real-time processing updates
     * @param frameRate Current processing frame rate
     */
    void realTimeProcessingUpdate(double frameRate);

private slots:
    void updateProcessingMetrics();

private:
    mutable QMutex m_modeMutex;
    bool m_realTimeModeEnabled;
    QTimer* m_metricsTimer;
    double m_currentFrameRate;
};