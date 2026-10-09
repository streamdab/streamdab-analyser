#pragma once

#include "../core/eti_types.h"
#include <QObject>
#include <QString>
#include <QDateTime>
#include <chrono>
#include <memory>

/**
 * @file performance_validator.h
 * @brief Network performance validation and monitoring
 * 
 * This module provides comprehensive performance validation for network
 * streaming operations, ensuring optimal ETI-over-IP performance.
 */

namespace network {

/**
 * @brief Performance metrics structure
 */
struct PerformanceMetrics {
    double throughput_mbps = 0.0;
    double latency_ms = 0.0;
    double jitter_ms = 0.0;
    double packet_loss_percent = 0.0;
    size_t frames_processed = 0;
    std::chrono::steady_clock::time_point timestamp;
};

/**
 * @brief Network performance validator
 * 
 * Validates and monitors network streaming performance to ensure
 * optimal ETI-over-IP operation meeting broadcast industry standards.
 */
class PerformanceValidator : public QObject {
    Q_OBJECT

public:
    explicit PerformanceValidator(QObject* parent = nullptr);
    virtual ~PerformanceValidator() = default;

    /**
     * @brief Start performance monitoring
     */
    void startMonitoring();

    /**
     * @brief Stop performance monitoring
     */
    void stopMonitoring();

    /**
     * @brief Record frame processing time
     * @param processing_time_ms Processing time in milliseconds
     */
    void recordFrameProcessingTime(double processing_time_ms);

    /**
     * @brief Get current performance metrics
     * @return Current performance metrics
     */
    PerformanceMetrics getCurrentMetrics() const;

    /**
     * @brief Check if performance meets requirements
     * @return true if performance is acceptable
     */
    bool isPerformanceAcceptable() const;

signals:
    /**
     * @brief Emitted when performance metrics update
     * @param metrics Current performance metrics
     */
    void performanceMetricsUpdated(const PerformanceMetrics& metrics);

    /**
     * @brief Emitted when performance degrades below threshold
     * @param issue Description of performance issue
     */
    void performanceIssueDetected(const QString& issue);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace network