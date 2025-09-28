/**
 * @file streaming_dashboard.h
 * @brief Live Streaming Dashboard with Real-time Monitoring
 * 
 * Professional dashboard widget for monitoring live ETI streaming with
 * connection status, buffer management, quality metrics, and performance
 * indicators designed for broadcast industry standards.
 * 
 * @author StreamDAB Development Team
 * @date 2025
 * @copyright Copyright (c) 2025 StreamDAB Technologies
 */

#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLCDNumber>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QElapsedTimer>
#include <QFrame>
#include <QScrollArea>
#include <QTextEdit>
#include <QTabWidget>
#include <QPaintEvent>
#include <QPainter>
#include <QStaticText>
#include <QPropertyAnimation>
#include <QGraphicsEffect>
#include <memory>
#include <deque>
#include <chrono>

// Core streaming components
#include "../core/streaming_engine.hpp"
#include "../core/modern_eti_controller.hpp"
#include "connection_config_dialog.h"

namespace streamdab::ui {

/**
 * @brief Connection status enumeration
 */
enum class ConnectionStatus {
    DISCONNECTED = 0,
    CONNECTING = 1,
    CONNECTED = 2,
    STREAMING = 3,
    ERROR = 4,
    RECONNECTING = 5
};

/**
 * @brief Quality indicator levels
 */
enum class QualityIndicator {
    EXCELLENT = 0,  // Green - >95%
    GOOD = 1,       // Light Green - 85-95%
    ACCEPTABLE = 2, // Yellow - 70-85%
    POOR = 3,       // Orange - 50-70%
    CRITICAL = 4,   // Red - <50%
    UNKNOWN = 5     // Gray - No data
};

/**
 * @brief Live dashboard metrics structure
 */
struct DashboardMetrics {
    // Connection metrics
    ConnectionStatus connection_status{ConnectionStatus::DISCONNECTED};
    std::chrono::milliseconds connection_uptime{0};
    QString endpoint_url;
    
    // Streaming metrics
    double current_fps{0.0};
    double average_fps{0.0};
    uint64_t total_frames{0};
    uint64_t frames_with_errors{0};
    double error_rate{0.0};
    
    // Buffer metrics
    uint32_t buffer_size{0};
    uint32_t buffer_capacity{1000};
    double buffer_utilization{0.0};
    std::chrono::milliseconds buffer_latency{0};
    uint32_t buffer_overflows{0};
    uint32_t buffer_underflows{0};
    
    // Quality metrics
    double signal_quality{0.0};
    double etsi_compliance{0.0};
    QualityIndicator quality_level{QualityIndicator::UNKNOWN};
    std::chrono::microseconds sync_accuracy{0};
    
    // Performance metrics
    std::chrono::microseconds processing_latency{0};
    double cpu_usage{0.0};
    double memory_usage{0.0};
    double network_throughput{0.0}; // Mbps
    
    /**
     * @brief Check if streaming is healthy
     */
    [[nodiscard]] bool isHealthy() const {
        return connection_status == ConnectionStatus::STREAMING &&
               current_fps > 100.0 &&
               error_rate < 5.0 &&
               buffer_utilization < 80.0 &&
               signal_quality > 70.0;
    }
    
    /**
     * @brief Get overall health score (0-100)
     */
    [[nodiscard]] double getHealthScore() const {
        if (connection_status != ConnectionStatus::STREAMING) {
            return 0.0;
        }
        
        double score = 100.0;
        if (current_fps < 500.0) score -= 20.0;
        if (error_rate > 1.0) score -= 15.0;
        if (buffer_utilization > 75.0) score -= 15.0;
        if (signal_quality < 90.0) score -= 10.0;
        if (etsi_compliance < 95.0) score -= 10.0;
        
        return std::max(0.0, score);
    }
};

/**
 * @brief Custom status indicator widget
 */
class StatusIndicatorWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit StatusIndicatorWidget(QWidget* parent = nullptr);
    
    void setStatus(ConnectionStatus status);
    void setQuality(QualityIndicator quality);
    void setText(const QString& text);
    void setBlinking(bool enabled);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    
private slots:
    void onBlinkTimer();
    
private:
    ConnectionStatus status_{ConnectionStatus::DISCONNECTED};
    QualityIndicator quality_{QualityIndicator::UNKNOWN};
    QString text_;
    bool blinking_{false};
    bool blink_state_{false};
    QTimer* blink_timer_{nullptr};
    
    QColor getStatusColor() const;
    QColor getQualityColor() const;
};

/**
 * @brief Real-time chart widget for metrics visualization
 */
class RealTimeChartWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit RealTimeChartWidget(QWidget* parent = nullptr);
    
    void addDataPoint(double value);
    void setMaxDataPoints(int max_points);
    void setValueRange(double min_val, double max_val);
    void setTitle(const QString& title);
    void setUnit(const QString& unit);
    void setColor(const QColor& color);
    void clear();
    
protected:
    void paintEvent(QPaintEvent* event) override;
    
private:
    std::deque<double> data_points_;
    int max_data_points_{100};
    double min_value_{0.0};
    double max_value_{100.0};
    QString title_;
    QString unit_;
    QColor chart_color_{Qt::blue};
    
    void drawGrid(QPainter& painter, const QRect& chart_rect);
    void drawChart(QPainter& painter, const QRect& chart_rect);
    void drawLabels(QPainter& painter, const QRect& widget_rect);
};

/**
 * @brief Professional Live Streaming Dashboard
 * 
 * Comprehensive dashboard widget providing real-time monitoring of ETI
 * streaming with connection status, buffer management, quality metrics,
 * and performance indicators for professional broadcast applications.
 */
class StreamingDashboard : public QWidget {
    Q_OBJECT
    
public:
    explicit StreamingDashboard(QWidget* parent = nullptr);
    ~StreamingDashboard() override;
    
    // Disable copy/move for proper resource management
    StreamingDashboard(const StreamingDashboard&) = delete;
    StreamingDashboard& operator=(const StreamingDashboard&) = delete;
    StreamingDashboard(StreamingDashboard&&) = delete;
    StreamingDashboard& operator=(StreamingDashboard&&) = delete;
    
    /**
     * @brief Initialize dashboard with streaming engine
     * @param streaming_engine Streaming engine for monitoring
     * @param eti_controller ETI controller for status updates
     * @return true if initialization successful
     */
    bool initialize(std::shared_ptr<eti::streaming::StreamingEngine> streaming_engine,
                   std::shared_ptr<eti::controller::ModernETIController> eti_controller);
    
    /**
     * @brief Check if dashboard is initialized
     */
    [[nodiscard]] bool isInitialized() const noexcept {
        return initialized_;
    }
    
    /**
     * @brief Get current dashboard metrics
     */
    [[nodiscard]] DashboardMetrics getCurrentMetrics() const {
        return current_metrics_;
    }
    
    /**
     * @brief Enable/disable auto-refresh
     * @param enabled true to enable automatic updates
     * @param interval_ms Update interval in milliseconds
     */
    void setAutoRefreshEnabled(bool enabled, int interval_ms = 500);
    
    /**
     * @brief Set dashboard theme
     */
    void setDarkTheme(bool dark = true);
    
    /**
     * @brief Reset all statistics and charts
     */
    void resetStatistics();

public slots:
    /**
     * @brief Update connection status display
     * @param status Current connection status
     */
    void updateConnectionStatus(ConnectionStatus status);
    
    /**
     * @brief Update quality metrics display
     * @param metrics Current quality metrics from streaming engine
     */
    void updateQualityMetrics(const eti::streaming::QualityMetrics& metrics);
    
    /**
     * @brief Update buffer status display
     * @param status Current buffer status
     */
    void updateBufferStatus(const eti::streaming::BufferStatus& status);
    
    /**
     * @brief Update performance statistics
     * @param stats Current performance statistics
     */
    void updatePerformanceStats(const eti::streaming::StreamingEngine::PerformanceStats& stats);
    
    /**
     * @brief Update connection endpoint information
     * @param endpoint Current connection endpoint URL
     */
    void updateConnectionInfo(const QString& endpoint);
    
    /**
     * @brief Show connection configuration dialog
     */
    void showConnectionConfig();
    
    /**
     * @brief Start streaming connection
     */
    void startStreaming();
    
    /**
     * @brief Stop streaming connection
     */
    void stopStreaming();
    
    /**
     * @brief Reconnect to streaming source
     */
    void reconnectStream();
    
    /**
     * @brief Refresh dashboard display
     */
    void refreshDisplay();

signals:
    /**
     * @brief Emitted when connection configuration is requested
     */
    void connectionConfigRequested();
    
    /**
     * @brief Emitted when streaming start is requested
     * @param endpoint Connection endpoint URL
     */
    void streamingStartRequested(const QString& endpoint);
    
    /**
     * @brief Emitted when streaming stop is requested
     */
    void streamingStopRequested();
    
    /**
     * @brief Emitted when reconnection is requested
     */
    void reconnectionRequested();
    
    /**
     * @brief Emitted when dashboard metrics change significantly
     * @param metrics Current dashboard metrics
     */
    void metricsChanged(const DashboardMetrics& metrics);
    
    /**
     * @brief Emitted when quality alert threshold is triggered
     * @param level Alert severity level
     * @param message Alert message
     */
    void qualityAlert(QualityIndicator level, const QString& message);

private slots:
    /**
     * @brief Handle periodic dashboard updates
     */
    void onUpdateTimer();
    
    /**
     * @brief Handle streaming engine signals
     */
    void onStreamingStateChanged(bool streaming);
    void onFrameProcessed(uint32_t frame_number, const eti::EtiFrame& frame,
                         std::chrono::microseconds processing_time);
    void onBufferStatusChanged(const eti::streaming::BufferStatus& status);
    void onQualityLevelChanged(eti::streaming::QualityLevel level,
                              const eti::streaming::QualityMetrics& metrics);
    void onQualityAlert(eti::streaming::QualityLevel level, const QString& message);
    
    /**
     * @brief Handle ETI controller signals
     */
    void onConnectionStatusChanged(const eti::network::ConnectionStatus& status);
    void onStatisticsUpdated(const eti::controller::ProcessingStatistics& stats);
    void onPerformanceTargetStatus(bool target_met, double current_fps, double target_fps);

private:
    /**
     * @brief Setup UI components
     */
    void setupUI();
    
    /**
     * @brief Create connection status panel
     */
    QWidget* createConnectionStatusPanel();
    
    /**
     * @brief Create streaming metrics panel
     */
    QWidget* createStreamingMetricsPanel();
    
    /**
     * @brief Create buffer status panel
     */
    QWidget* createBufferStatusPanel();
    
    /**
     * @brief Create quality monitoring panel
     */
    QWidget* createQualityMonitoringPanel();
    
    /**
     * @brief Create performance charts panel
     */
    QWidget* createPerformanceChartsPanel();
    
    /**
     * @brief Create control buttons panel
     */
    QWidget* createControlButtonsPanel();
    
    /**
     * @brief Setup signal connections
     */
    void connectSignals();
    
    /**
     * @brief Apply professional styling
     */
    void applyProfessionalStyling();
    
    /**
     * @brief Update all dashboard displays
     */
    void updateAllDisplays();
    
    /**
     * @brief Update connection status indicators
     */
    void updateConnectionIndicators();
    
    /**
     * @brief Update streaming metrics displays
     */
    void updateStreamingDisplays();
    
    /**
     * @brief Update buffer status displays
     */
    void updateBufferDisplays();
    
    /**
     * @brief Update quality indicators
     */
    void updateQualityDisplays();
    
    /**
     * @brief Update performance charts
     */
    void updatePerformanceCharts();
    
    /**
     * @brief Check and trigger quality alerts
     */
    void checkQualityAlerts();
    
    /**
     * @brief Get connection status from engine state
     */
    ConnectionStatus getConnectionStatusFromEngine();
    
    /**
     * @brief Convert quality level to indicator
     */
    QualityIndicator convertQualityLevel(eti::streaming::QualityLevel level);
    
    /**
     * @brief Format uptime display
     */
    QString formatUptime(std::chrono::milliseconds uptime);
    
    /**
     * @brief Format data rate display
     */
    QString formatDataRate(double rate_mbps);
    
    /**
     * @brief Format latency display
     */
    QString formatLatency(std::chrono::microseconds latency);

    // Core components
    std::shared_ptr<eti::streaming::StreamingEngine> streaming_engine_;
    std::shared_ptr<eti::controller::ModernETIController> eti_controller_;
    std::unique_ptr<ConnectionConfigDialog> config_dialog_;
    
    // UI Layout
    QVBoxLayout* main_layout_{nullptr};
    QTabWidget* metrics_tabs_{nullptr};
    
    // Connection Status Panel
    QGroupBox* connection_group_{nullptr};
    StatusIndicatorWidget* connection_indicator_{nullptr};
    QLabel* endpoint_label_{nullptr};
    QLabel* uptime_label_{nullptr};
    QLabel* status_text_label_{nullptr};
    
    // Streaming Metrics Panel
    QGroupBox* streaming_group_{nullptr};
    QLCDNumber* fps_display_{nullptr};
    QLCDNumber* total_frames_display_{nullptr};
    QLabel* error_rate_label_{nullptr};
    QProgressBar* throughput_bar_{nullptr};
    
    // Buffer Status Panel
    QGroupBox* buffer_group_{nullptr};
    QProgressBar* buffer_utilization_bar_{nullptr};
    QLabel* buffer_size_label_{nullptr};
    QLabel* buffer_latency_label_{nullptr};
    QLabel* buffer_overflows_label_{nullptr};
    
    // Quality Monitoring Panel
    QGroupBox* quality_group_{nullptr};
    StatusIndicatorWidget* quality_indicator_{nullptr};
    QProgressBar* signal_quality_bar_{nullptr};
    QProgressBar* etsi_compliance_bar_{nullptr};
    QLabel* sync_accuracy_label_{nullptr};
    
    // Performance Charts Panel
    QGroupBox* charts_group_{nullptr};
    RealTimeChartWidget* fps_chart_{nullptr};
    RealTimeChartWidget* latency_chart_{nullptr};
    RealTimeChartWidget* quality_chart_{nullptr};
    RealTimeChartWidget* buffer_chart_{nullptr};
    
    // Control Buttons Panel
    QGroupBox* control_group_{nullptr};
    QPushButton* configure_btn_{nullptr};
    QPushButton* start_btn_{nullptr};
    QPushButton* stop_btn_{nullptr};
    QPushButton* reconnect_btn_{nullptr};
    QPushButton* reset_stats_btn_{nullptr};
    
    // Dashboard state
    bool initialized_{false};
    DashboardMetrics current_metrics_;
    QTimer* update_timer_{nullptr};
    QElapsedTimer uptime_timer_;
    bool auto_refresh_enabled_{true};
    bool dark_theme_{false};
    
    // Statistics tracking
    std::chrono::system_clock::time_point connection_start_time_;
    uint64_t last_frame_count_{0};
    std::chrono::system_clock::time_point last_fps_update_;
    std::deque<double> recent_fps_values_;
    
    // Quality thresholds for alerts
    double signal_quality_threshold_{80.0};
    double etsi_compliance_threshold_{85.0};
    double error_rate_threshold_{5.0};
    double buffer_utilization_threshold_{85.0};
    
    // Constants
    static constexpr int DEFAULT_UPDATE_INTERVAL = 500; // 500ms updates
    static constexpr int MAX_CHART_POINTS = 100;
    static constexpr int FPS_HISTORY_SIZE = 10;
};

/**
 * @brief Utility functions for dashboard operations
 */
namespace dashboard_utils {
    /**
     * @brief Convert connection status to display string
     */
    [[nodiscard]] QString connectionStatusToString(ConnectionStatus status);
    
    /**
     * @brief Convert quality indicator to display string
     */
    [[nodiscard]] QString qualityIndicatorToString(QualityIndicator indicator);
    
    /**
     * @brief Get status color for UI elements
     */
    [[nodiscard]] QColor getStatusColor(ConnectionStatus status);
    
    /**
     * @brief Get quality color for UI elements
     */
    [[nodiscard]] QColor getQualityColor(QualityIndicator indicator);
    
    /**
     * @brief Format bytes as human-readable string
     */
    [[nodiscard]] QString formatBytes(uint64_t bytes);
    
    /**
     * @brief Format duration as human-readable string
     */
    [[nodiscard]] QString formatDuration(std::chrono::milliseconds duration);
    
    /**
     * @brief Calculate moving average
     */
    [[nodiscard]] double calculateMovingAverage(const std::deque<double>& values, size_t window_size);
}

} // namespace streamdab::ui