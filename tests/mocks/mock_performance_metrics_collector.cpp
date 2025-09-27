// ==============================================================================
// Mock Performance Metrics Collector - TDD Testing Support
// ==============================================================================

#include "mock_performance_metrics_collector.h"

MockPerformanceMetricsCollector::MockPerformanceMetricsCollector(QObject *parent)
    : QObject(parent)
    , m_frameCount(0)
    , m_processingTime(0)
{
}

MockPerformanceMetricsCollector::~MockPerformanceMetricsCollector() = default;

void MockPerformanceMetricsCollector::recordFrameProcessing(quint64 frameNumber, qint64 processingTime)
{
    Q_UNUSED(frameNumber)
    m_frameCount++;
    m_processingTime += processingTime;
    emit metricsUpdated();
}

quint64 MockPerformanceMetricsCollector::getTotalFrames() const
{
    return m_frameCount;
}

qint64 MockPerformanceMetricsCollector::getTotalProcessingTime() const
{
    return m_processingTime;
}

double MockPerformanceMetricsCollector::getAverageProcessingTime() const
{
    return m_frameCount > 0 ? static_cast<double>(m_processingTime) / m_frameCount : 0.0;
}