// ==============================================================================
// Mock Performance Metrics Collector Header - TDD Testing Support
// ==============================================================================

#pragma once

#include <QObject>

class MockPerformanceMetricsCollector : public QObject
{
    Q_OBJECT

public:
    explicit MockPerformanceMetricsCollector(QObject *parent = nullptr);
    ~MockPerformanceMetricsCollector() override;

    void recordFrameProcessing(quint64 frameNumber, qint64 processingTime);
    quint64 getTotalFrames() const;
    qint64 getTotalProcessingTime() const;
    double getAverageProcessingTime() const;

signals:
    void metricsUpdated();

private:
    quint64 m_frameCount;
    qint64 m_processingTime;
};