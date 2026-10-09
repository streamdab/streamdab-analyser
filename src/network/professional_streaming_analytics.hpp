/**
 * @file professional_streaming_analytics.hpp
 * @brief Production-Grade Streaming Analytics Engine
 * 
 * Professional broadcast analytics system implementing:
 * - Real-time performance monitoring with sub-millisecond precision
 * - Advanced quality metrics and trend analysis
 * - Network troubleshooting tools and diagnostics
 * - Bandwidth optimization and QoS management
 * - Professional streaming dashboard with configurable alerts
 * - Comprehensive reporting for broadcast compliance
 * 
 * @author Network/Stream Agent - Phase 3 Production Platform
 * @date 2025-09-28
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 */

#ifndef PROFESSIONAL_STREAMING_ANALYTICS_HPP
#define PROFESSIONAL_STREAMING_ANALYTICS_HPP

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QThread>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QVariantMap>
#include <memory>
#include <atomic>
#include <chrono>
#include <vector>
#include <array>
#include <queue>
#include <deque>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <numeric>

#include "../core/eti_types.hpp"

namespace eti_network {

/**
 * @brief Comprehensive streaming performance metrics with statistical analysis
 */
struct ProfessionalStreamingMetrics {
    // Timestamp information
    std::chrono::steady_clock::time_point measurement_time;
    std::chrono::milliseconds measurement_interval{1000};
    
    // Frame-level metrics
    std::atomic<size_t> frames_received{0};
    std::atomic<size_t> frames_processed{0};
    std::atomic<size_t> frames_dropped{0};
    std::atomic<size_t> frames_corrupted{0};
    std::atomic<size_t> frames_duplicate{0};
    std::atomic<size_t> frames_out_of_order{0};
    
    // Packet-level metrics  
    std::atomic<size_t> packets_received{0};
    std::atomic<size_t> packets_lost{0};
    std::atomic<size_t> packets_retransmitted{0};
    std::atomic<size_t> packets_out_of_order{0};
    std::atomic<size_t> packets_duplicate{0};
    
    // Latency metrics (microseconds)
    std::atomic<int64_t> current_latency_us{0};
    std::atomic<int64_t> minimum_latency_us{LLONG_MAX};
    std::atomic<int64_t> maximum_latency_us{0};
    std::atomic<int64_t> average_latency_us{0};
    std::atomic<int64_t> median_latency_us{0};
    std::atomic<int64_t> p95_latency_us{0};        // 95th percentile
    std::atomic<int64_t> p99_latency_us{0};        // 99th percentile
    
    // Jitter metrics (microseconds)
    std::atomic<int64_t> current_jitter_us{0};
    std::atomic<int64_t> minimum_jitter_us{LLONG_MAX};
    std::atomic<int64_t> maximum_jitter_us{0};
    std::atomic<int64_t> average_jitter_us{0};
    std::atomic<int64_t> jitter_variance_us{0};
    
    // Throughput metrics
    std::atomic<double> current_fps{0.0};
    std::atomic<double> average_fps{0.0};
    std::atomic<double> peak_fps{0.0};
    std::atomic<double> minimum_fps{DBL_MAX};
    std::atomic<double> target_fps{1000.0};
    std::atomic<double> fps_stability{1.0};        // Coefficient of variation
    
    // Bandwidth metrics
    std::atomic<double> current_bitrate_mbps{0.0};
    std::atomic<double> average_bitrate_mbps{0.0};
    std::atomic<double> peak_bitrate_mbps{0.0};
    std::atomic<double> minimum_bitrate_mbps{DBL_MAX};
    std::atomic<double> bandwidth_utilization{0.0}; // 0.0-1.0
    std::atomic<double> bandwidth_efficiency{1.0};   // Useful vs total
    
    // Quality metrics
    std::atomic<double> signal_quality{1.0};       // 0.0-1.0
    std::atomic<double> stream_quality{1.0};       // 0.0-1.0
    std::atomic<double> audio_quality{1.0};        // 0.0-1.0
    std::atomic<double> overall_quality{1.0};      // Composite quality
    std::atomic<double> quality_variance{0.0};     // Quality stability
    
    // Error rates
    std::atomic<double> packet_loss_rate{0.0};     // 0.0-1.0
    std::atomic<double> frame_error_rate{0.0};     // 0.0-1.0
    std::atomic<double> corruption_rate{0.0};      // 0.0-1.0
    std::atomic<double> duplicate_rate{0.0};       // 0.0-1.0
    std::atomic<double> out_of_order_rate{0.0};    // 0.0-1.0
    
    // Buffer metrics
    std::atomic<double> buffer_utilization{0.0};   // 0.0-1.0
    std::atomic<size_t> buffer_overflows{0};
    std::atomic<size_t> buffer_underflows{0};
    std::atomic<double> buffer_health{1.0};        // Overall buffer health
    std::atomic<size_t> buffer_size_frames{1000};
    
    // Resource utilization
    std::atomic<double> cpu_usage{0.0};            // 0.0-1.0
    std::atomic<double> memory_usage_mb{0.0};
    std::atomic<double> network_usage{0.0};        // 0.0-1.0
    std::atomic<double> disk_io_mbps{0.0};
    std::atomic<double> system_load{0.0};
    
    // Network health indicators
    std::atomic<double> network_congestion{0.0};   // 0.0-1.0
    std::atomic<double> route_stability{1.0};      // 0.0-1.0
    std::atomic<int64_t> round_trip_time_us{0};
    std::atomic<size_t> network_errors{0};
    std::atomic<bool> is_network_healthy{true};
    
    // Session statistics
    std::chrono::steady_clock::time_point session_start;
    std::atomic<size_t> total_session_frames{0};
    std::atomic<size_t> total_session_bytes{0};
    std::atomic<size_t> total_session_errors{0};
    std::atomic<double> session_uptime_percent{100.0};
    
    // Alert and threshold states
    std::atomic<bool> latency_alert{false};
    std::atomic<bool> quality_alert{false};
    std::atomic<bool> bandwidth_alert{false};
    std::atomic<bool> error_rate_alert{false};
    std::atomic<bool> buffer_alert{false};
    std::atomic<bool> resource_alert{false};
    
    void reset() {
        measurement_time = std::chrono::steady_clock::now();
        frames_received = 0;
        frames_processed = 0;
        frames_dropped = 0;
        frames_corrupted = 0;
        frames_duplicate = 0;
        frames_out_of_order = 0;
        packets_received = 0;
        packets_lost = 0;
        packets_retransmitted = 0;
        packets_out_of_order = 0;
        packets_duplicate = 0;
        current_latency_us = 0;
        minimum_latency_us = LLONG_MAX;
        maximum_latency_us = 0;
        average_latency_us = 0;
        median_latency_us = 0;
        p95_latency_us = 0;
        p99_latency_us = 0;
        current_jitter_us = 0;
        minimum_jitter_us = LLONG_MAX;
        maximum_jitter_us = 0;
        average_jitter_us = 0;
        jitter_variance_us = 0;
        current_fps = 0.0;
        average_fps = 0.0;
        peak_fps = 0.0;
        minimum_fps = DBL_MAX;
        fps_stability = 1.0;
        current_bitrate_mbps = 0.0;
        average_bitrate_mbps = 0.0;
        peak_bitrate_mbps = 0.0;
        minimum_bitrate_mbps = DBL_MAX;
        bandwidth_utilization = 0.0;
        bandwidth_efficiency = 1.0;
        signal_quality = 1.0;
        stream_quality = 1.0;
        audio_quality = 1.0;
        overall_quality = 1.0;
        quality_variance = 0.0;
        packet_loss_rate = 0.0;
        frame_error_rate = 0.0;
        corruption_rate = 0.0;
        duplicate_rate = 0.0;
        out_of_order_rate = 0.0;
        buffer_utilization = 0.0;
        buffer_overflows = 0;
        buffer_underflows = 0;
        buffer_health = 1.0;
        cpu_usage = 0.0;
        memory_usage_mb = 0.0;
        network_usage = 0.0;
        disk_io_mbps = 0.0;
        system_load = 0.0;
        network_congestion = 0.0;
        route_stability = 1.0;
        round_trip_time_us = 0;
        network_errors = 0;
        is_network_healthy = true;
        session_start = std::chrono::steady_clock::now();
        total_session_frames = 0;
        total_session_bytes = 0;
        total_session_errors = 0;
        session_uptime_percent = 100.0;
        latency_alert = false;
        quality_alert = false;
        bandwidth_alert = false;
        error_rate_alert = false;
        buffer_alert = false;
        resource_alert = false;
    }
    
    QJsonObject to_json() const;
    void from_json(const QJsonObject& json);
};

/**
 * @brief Professional alert configuration and thresholds
 */
struct ProfessionalAlertConfiguration {
    // Latency thresholds (microseconds)
    int64_t latency_warning_us = 15000;             // 15ms warning
    int64_t latency_critical_us = 20000;            // 20ms critical
    int64_t jitter_warning_us = 5000;               // 5ms jitter warning
    int64_t jitter_critical_us = 10000;             // 10ms jitter critical
    
    // Quality thresholds (0.0-1.0)
    double quality_warning = 0.9;                   // 90% quality warning
    double quality_critical = 0.8;                  // 80% quality critical
    double signal_strength_warning = 0.8;           // 80% signal warning
    double signal_strength_critical = 0.6;          // 60% signal critical
    
    // Throughput thresholds
    double fps_warning_min = 950.0;                 // Minimum FPS warning
    double fps_critical_min = 900.0;                // Minimum FPS critical
    double fps_warning_max = 1100.0;                // Maximum FPS warning
    double fps_critical_max = 1200.0;               // Maximum FPS critical
    
    // Error rate thresholds (0.0-1.0)
    double packet_loss_warning = 0.001;             // 0.1% packet loss warning
    double packet_loss_critical = 0.01;             // 1% packet loss critical
    double frame_error_warning = 0.005;             // 0.5% frame error warning
    double frame_error_critical = 0.02;             // 2% frame error critical
    
    // Buffer thresholds (0.0-1.0)
    double buffer_utilization_warning = 0.8;        // 80% buffer warning
    double buffer_utilization_critical = 0.95;      // 95% buffer critical
    size_t buffer_overflow_warning = 10;            // 10 overflows warning
    size_t buffer_overflow_critical = 50;           // 50 overflows critical
    
    // Resource thresholds (0.0-1.0)
    double cpu_usage_warning = 0.8;                 // 80% CPU warning
    double cpu_usage_critical = 0.95;               // 95% CPU critical
    double memory_usage_warning_mb = 500.0;         // 500MB memory warning
    double memory_usage_critical_mb = 1000.0;       // 1GB memory critical
    double network_usage_warning = 0.8;             // 80% network warning
    double network_usage_critical = 0.95;           // 95% network critical
    
    // Alert behavior
    std::chrono::seconds alert_hysteresis{30};      // 30s alert hysteresis
    std::chrono::seconds alert_max_frequency{60};   // Max 1 alert per minute
    bool enable_email_alerts = false;               // Email notifications
    bool enable_dashboard_alerts = true;            // Dashboard notifications
    bool enable_log_alerts = true;                  // Log file alerts
    bool enable_sound_alerts = false;               // Audio alerts
    
    // Escalation settings
    std::chrono::minutes escalation_timeout{5};     // 5 min escalation
    size_t max_consecutive_alerts = 5;              // Max consecutive alerts
    bool enable_automatic_recovery = true;          // Auto-recovery actions
    
    QString alert_email_address;                    // Alert email
    QString alert_webhook_url;                      // Webhook URL
    QStringList alert_keywords;                     // Custom alert keywords
    
    bool is_valid() const {
        return latency_warning_us > 0 && latency_critical_us > latency_warning_us &&
               quality_warning > quality_critical && quality_critical > 0.0 &&
               fps_warning_min > fps_critical_min && fps_critical_min > 0.0;
    }
    
    QJsonObject to_json() const;
    void from_json(const QJsonObject& json);
};

/**
 * @brief Historical analytics data point
 */
struct AnalyticsDataPoint {
    std::chrono::steady_clock::time_point timestamp;
    ProfessionalStreamingMetrics metrics;
    QString alert_status;                           // "normal", "warning", "critical"
    QStringList active_alerts;                      // List of active alerts
    QString notes;                                  // Optional notes
    
    QJsonObject to_json() const;
    void from_json(const QJsonObject& json);
};

/**
 * @brief Statistical trend analysis
 */
struct TrendAnalysis {
    // Trend direction
    enum TrendDirection { IMPROVING, STABLE, DEGRADING, VOLATILE };
    
    TrendDirection latency_trend = STABLE;
    TrendDirection quality_trend = STABLE;
    TrendDirection throughput_trend = STABLE;
    TrendDirection error_rate_trend = STABLE;
    
    // Confidence levels (0.0-1.0)
    double latency_confidence = 1.0;
    double quality_confidence = 1.0;
    double throughput_confidence = 1.0;
    double error_rate_confidence = 1.0;
    
    // Trend slopes (rate of change per hour)
    double latency_slope_us_per_hour = 0.0;
    double quality_slope_per_hour = 0.0;
    double throughput_slope_fps_per_hour = 0.0;
    double error_rate_slope_per_hour = 0.0;
    
    // Predictions (next hour if trend continues)
    int64_t predicted_latency_us = 0;
    double predicted_quality = 1.0;
    double predicted_throughput_fps = 1000.0;
    double predicted_error_rate = 0.0;
    
    // Anomaly detection
    bool latency_anomaly_detected = false;
    bool quality_anomaly_detected = false;
    bool throughput_anomaly_detected = false;
    bool pattern_anomaly_detected = false;
    
    std::chrono::steady_clock::time_point analysis_time;
    size_t sample_count = 0;
    std::chrono::milliseconds analysis_period{3600000}; // 1 hour
    
    QString to_string() const;
    QJsonObject to_json() const;
    void from_json(const QJsonObject& json);
};

/**
 * @brief Professional streaming dashboard data
 */
struct StreamingDashboardData {
    // Current status
    ProfessionalStreamingMetrics current_metrics;
    QString overall_status;                         // "healthy", "warning", "critical"
    QStringList active_alerts;
    QString primary_issue;                          // Most critical issue
    
    // Historical trends
    TrendAnalysis trend_analysis;
    std::vector<AnalyticsDataPoint> recent_history; // Last 24 hours
    
    // Performance summaries
    struct PerformanceSummary {
        double uptime_percent = 100.0;
        double average_quality = 1.0;
        int64_t average_latency_us = 0;
        double average_fps = 1000.0;
        size_t total_frames = 0;
        size_t total_errors = 0;
        std::chrono::hours summary_period{24};
    } performance_summary;
    
    // Network diagnostics
    struct NetworkDiagnostics {
        QString network_interface;
        QString connection_status;
        double bandwidth_utilization = 0.0;
        size_t active_connections = 0;
        QString route_status;
        QStringList detected_issues;
    } network_diagnostics;
    
    // Resource utilization
    struct ResourceUtilization {
        double cpu_percent = 0.0;
        double memory_mb = 0.0;
        double disk_io_mbps = 0.0;
        double network_io_mbps = 0.0;
        QString bottleneck_component;
    } resource_utilization;
    
    std::chrono::steady_clock::time_point last_update;
    
    QJsonObject to_json() const;
    void from_json(const QJsonObject& json);
};

/**
 * @brief High-performance metrics collector with statistical analysis
 */
class ProfessionalMetricsCollector : public QObject {
    Q_OBJECT
    
public:
    explicit ProfessionalMetricsCollector(QObject* parent = nullptr);
    ~ProfessionalMetricsCollector() override;
    
    // Data collection
    void record_frame_metrics(const eti::EtiFrame& frame, 
                             std::chrono::steady_clock::time_point receive_time,
                             std::chrono::steady_clock::time_point process_time);
    void record_packet_metrics(size_t packet_count, size_t lost_packets, 
                              std::chrono::microseconds rtt);
    void record_quality_metrics(double signal_quality, double stream_quality, 
                               double audio_quality);
    void record_resource_metrics(double cpu_usage, double memory_mb, 
                                double network_usage);
    
    // Statistical analysis
    ProfessionalStreamingMetrics get_current_metrics() const;
    ProfessionalStreamingMetrics get_averaged_metrics(std::chrono::minutes window) const;
    TrendAnalysis analyze_trends(std::chrono::hours analysis_period) const;
    std::vector<AnalyticsDataPoint> get_historical_data(std::chrono::hours period) const;
    
    // Performance queries
    double get_percentile_latency(double percentile) const; // 0.0-1.0
    double get_fps_stability_coefficient() const;
    double get_quality_variance(std::chrono::minutes window) const;
    bool is_performance_stable(std::chrono::minutes window) const;
    
    // Configuration
    void set_collection_interval(std::chrono::milliseconds interval);
    void set_history_retention(std::chrono::hours retention);
    void enable_advanced_statistics(bool enabled);
    void set_smoothing_factor(double factor); // 0.0-1.0

signals:
    void metrics_updated(const ProfessionalStreamingMetrics& metrics);
    void trend_detected(const TrendAnalysis& trend);
    void anomaly_detected(const QString& type, double severity);
    void performance_degradation(const QString& metric, double current, double expected);

private slots:
    void calculate_statistics();
    void detect_anomalies();
    void cleanup_old_data();

private:
    void update_latency_statistics(std::chrono::microseconds latency);
    void update_jitter_statistics(std::chrono::microseconds jitter);
    void update_fps_statistics(double fps);
    void update_quality_statistics(double quality);
    void calculate_percentiles();
    void detect_trend_changes();
    bool is_anomalous_value(double value, const std::vector<double>& history, 
                           double threshold = 3.0) const;
    
    ProfessionalStreamingMetrics m_current_metrics;
    std::deque<AnalyticsDataPoint> m_historical_data;
    
    // Statistical data
    std::vector<int64_t> m_latency_samples;
    std::vector<int64_t> m_jitter_samples;
    std::vector<double> m_fps_samples;
    std::vector<double> m_quality_samples;
    
    std::unique_ptr<QTimer> m_statistics_timer;
    std::unique_ptr<QTimer> m_anomaly_timer;
    std::unique_ptr<QTimer> m_cleanup_timer;
    
    mutable std::mutex m_metrics_mutex;
    mutable std::mutex m_history_mutex;
    
    std::chrono::milliseconds m_collection_interval{100};
    std::chrono::hours m_history_retention{24};
    std::atomic<bool> m_advanced_statistics{true};
    std::atomic<double> m_smoothing_factor{0.1};
    
    static constexpr size_t MAX_SAMPLE_SIZE = 10000;
    static constexpr std::chrono::milliseconds STATISTICS_INTERVAL{1000};
    static constexpr std::chrono::milliseconds ANOMALY_INTERVAL{5000};
    static constexpr std::chrono::hours CLEANUP_INTERVAL{1};
};

/**
 * @brief Professional alert manager with escalation
 */
class ProfessionalAlertManager : public QObject {
    Q_OBJECT
    
public:
    explicit ProfessionalAlertManager(QObject* parent = nullptr);
    ~ProfessionalAlertManager() override;
    
    // Alert configuration
    void set_alert_configuration(const ProfessionalAlertConfiguration& config);
    ProfessionalAlertConfiguration get_alert_configuration() const;
    void update_thresholds(const QString& metric, double warning, double critical);
    
    // Alert processing
    void process_metrics(const ProfessionalStreamingMetrics& metrics);
    void trigger_custom_alert(const QString& type, const QString& message, 
                             const QString& severity = "warning");
    void clear_alert(const QString& alert_id);
    void acknowledge_alert(const QString& alert_id, const QString& operator_name);
    
    // Alert queries
    QStringList get_active_alerts() const;
    QStringList get_alert_history(std::chrono::hours period) const;
    QString get_alert_status() const; // "normal", "warning", "critical"
    size_t get_alert_count(const QString& severity) const;
    
    // Alert actions
    void enable_alert_notifications(bool enabled);
    void enable_automatic_recovery(bool enabled);
    void set_escalation_contacts(const QStringList& contacts);
    void test_alert_system();

signals:
    void alert_triggered(const QString& alert_id, const QString& type, 
                        const QString& message, const QString& severity);
    void alert_cleared(const QString& alert_id);
    void alert_escalated(const QString& alert_id, const QString& contact);
    void alert_acknowledged(const QString& alert_id, const QString& operator);
    void alert_status_changed(const QString& new_status);

private slots:
    void check_alert_escalation();
    void cleanup_old_alerts();

private:
    struct AlertInfo {
        QString alert_id;
        QString type;
        QString message;
        QString severity;
        std::chrono::steady_clock::time_point trigger_time;
        std::chrono::steady_clock::time_point last_notification;
        bool acknowledged = false;
        QString acknowledged_by;
        size_t escalation_count = 0;
        bool auto_cleared = false;
    };
    
    void check_latency_thresholds(const ProfessionalStreamingMetrics& metrics);
    void check_quality_thresholds(const ProfessionalStreamingMetrics& metrics);
    void check_throughput_thresholds(const ProfessionalStreamingMetrics& metrics);
    void check_error_rate_thresholds(const ProfessionalStreamingMetrics& metrics);
    void check_buffer_thresholds(const ProfessionalStreamingMetrics& metrics);
    void check_resource_thresholds(const ProfessionalStreamingMetrics& metrics);
    
    void send_alert_notification(const AlertInfo& alert);
    void escalate_alert(const QString& alert_id);
    QString generate_alert_id() const;
    bool should_trigger_alert(const QString& type, const QString& severity) const;
    
    ProfessionalAlertConfiguration m_config;
    std::unordered_map<std::string, AlertInfo> m_active_alerts;
    std::vector<AlertInfo> m_alert_history;
    
    std::unique_ptr<QTimer> m_escalation_timer;
    std::unique_ptr<QTimer> m_cleanup_timer;
    
    mutable std::mutex m_alerts_mutex;
    std::atomic<bool> m_notifications_enabled{true};
    std::atomic<bool> m_auto_recovery_enabled{true};
    
    static constexpr std::chrono::milliseconds ESCALATION_CHECK_INTERVAL{30000}; // 30 seconds
    static constexpr std::chrono::hours ALERT_HISTORY_RETENTION{168}; // 7 days
};

/**
 * @brief Production-Grade Professional Streaming Analytics Engine
 * 
 * Enterprise-class analytics system implementing:
 * - Real-time performance monitoring with sub-millisecond precision
 * - Advanced statistical analysis and trend detection
 * - Professional alerting with escalation and recovery
 * - Comprehensive network diagnostics and troubleshooting
 * - Bandwidth optimization and QoS recommendations
 * - Broadcasting compliance reporting and analytics
 */
class ProfessionalStreamingAnalytics : public QObject {
    Q_OBJECT
    
public:
    explicit ProfessionalStreamingAnalytics(QObject* parent = nullptr);
    ~ProfessionalStreamingAnalytics() override;
    
    // Analytics engine control
    void start_analytics(std::chrono::milliseconds update_interval = std::chrono::milliseconds{1000});
    void stop_analytics();
    bool is_analytics_active() const;
    void reset_analytics();
    
    // Data input
    void process_frame_data(const eti::EtiFrame& frame, 
                           std::chrono::steady_clock::time_point receive_time,
                           std::chrono::steady_clock::time_point process_time);
    void process_network_data(const StreamQualityMetrics& quality_metrics);
    void process_resource_data(double cpu_usage, double memory_mb, double network_usage);
    
    // Metrics and analysis
    ProfessionalStreamingMetrics get_current_metrics() const;
    ProfessionalStreamingMetrics get_averaged_metrics(std::chrono::minutes window) const;
    TrendAnalysis get_trend_analysis(std::chrono::hours period = std::chrono::hours{1}) const;
    StreamingDashboardData get_dashboard_data() const;
    
    // Performance insights
    QStringList get_performance_insights() const;
    QStringList get_optimization_recommendations() const;
    QString get_bottleneck_analysis() const;
    double get_overall_health_score() const;
    
    // Alert management
    void set_alert_configuration(const ProfessionalAlertConfiguration& config);
    ProfessionalAlertConfiguration get_alert_configuration() const;
    QStringList get_active_alerts() const;
    QString get_alert_status() const;
    
    // Historical data
    std::vector<AnalyticsDataPoint> get_historical_data(std::chrono::hours period) const;
    void export_analytics_data(const QString& filename, const QString& format = "json") const;
    QJsonObject export_analytics_session() const;
    void import_analytics_session(const QJsonObject& session_data);
    
    // Network diagnostics
    QString perform_network_diagnostics() const;
    QStringList get_network_issues() const;
    QStringList get_bandwidth_recommendations() const;
    QString get_qos_analysis() const;
    
    // Reporting
    QString generate_performance_report(std::chrono::hours period = std::chrono::hours{24}) const;
    QString generate_compliance_report() const;
    QString generate_trend_report(std::chrono::hours period = std::chrono::hours{168}) const;
    QString generate_alert_summary(std::chrono::hours period = std::chrono::hours{24}) const;
    
    // Configuration
    void set_performance_targets(double target_fps, std::chrono::microseconds target_latency,
                                double target_quality);
    void enable_advanced_analytics(bool enabled);
    void set_analytics_precision(const QString& precision); // "high", "medium", "low"
    void enable_predictive_analysis(bool enabled);
    
    // Callback registration
    using MetricsCallback = std::function<void(const ProfessionalStreamingMetrics&)>;
    using AlertCallback = std::function<void(const QString&, const QString&, const QString&)>;
    using TrendCallback = std::function<void(const TrendAnalysis&)>;
    using DiagnosticsCallback = std::function<void(const QStringList&)>;
    
    void set_metrics_callback(MetricsCallback callback);
    void set_alert_callback(AlertCallback callback);
    void set_trend_callback(TrendCallback callback);
    void set_diagnostics_callback(DiagnosticsCallback callback);

signals:
    void analytics_started();
    void analytics_stopped();
    void metrics_updated(const ProfessionalStreamingMetrics& metrics);
    void dashboard_updated(const StreamingDashboardData& dashboard);
    
    void alert_triggered(const QString& alert_id, const QString& type, const QString& message);
    void alert_cleared(const QString& alert_id);
    void alert_status_changed(const QString& new_status);
    
    void trend_detected(const TrendAnalysis& trend);
    void anomaly_detected(const QString& type, double severity);
    void performance_degradation(const QString& metric, double current, double expected);
    
    void bottleneck_identified(const QString& component, double impact);
    void optimization_opportunity(const QString& recommendation, double potential_improvement);
    void compliance_issue_detected(const QString& issue, const QString& standard);

private slots:
    void update_analytics();
    void update_dashboard();
    void perform_trend_analysis();
    void check_performance_targets();
    void diagnose_network_issues();

private:
    void initialize_analytics();
    void cleanup_analytics();
    QString analyze_bottlenecks() const;
    QStringList generate_recommendations() const;
    QString assess_compliance_status() const;
    double calculate_health_score() const;
    void detect_performance_patterns();
    void update_predictive_models();
    
    std::unique_ptr<ProfessionalMetricsCollector> m_metrics_collector;
    std::unique_ptr<ProfessionalAlertManager> m_alert_manager;
    
    std::unique_ptr<QTimer> m_analytics_timer;
    std::unique_ptr<QTimer> m_dashboard_timer;
    std::unique_ptr<QTimer> m_trend_timer;
    std::unique_ptr<QTimer> m_diagnostics_timer;
    
    StreamingDashboardData m_dashboard_data;
    
    // Performance targets
    std::atomic<double> m_target_fps{1000.0};
    std::atomic<int64_t> m_target_latency_us{17000};
    std::atomic<double> m_target_quality{0.95};
    
    // Configuration
    std::atomic<bool> m_analytics_active{false};
    std::atomic<bool> m_advanced_analytics{true};
    std::atomic<bool> m_predictive_analysis{true};
    QString m_analytics_precision = "high";
    
    // Callbacks
    MetricsCallback m_metrics_callback;
    AlertCallback m_alert_callback;
    TrendCallback m_trend_callback;
    DiagnosticsCallback m_diagnostics_callback;
    mutable std::mutex m_callback_mutex;
    
    mutable std::mutex m_dashboard_mutex;
    
    static constexpr std::chrono::milliseconds ANALYTICS_INTERVAL{1000};    // 1 second
    static constexpr std::chrono::milliseconds DASHBOARD_INTERVAL{5000};    // 5 seconds
    static constexpr std::chrono::milliseconds TREND_INTERVAL{60000};       // 1 minute
    static constexpr std::chrono::milliseconds DIAGNOSTICS_INTERVAL{30000}; // 30 seconds
};

/**
 * @brief Factory for creating professional streaming analytics
 */
class ProfessionalStreamingAnalyticsFactory {
public:
    /**
     * @brief Create analytics optimized for broadcast monitoring
     */
    static std::unique_ptr<ProfessionalStreamingAnalytics> create_broadcast_analytics();
    
    /**
     * @brief Create analytics optimized for performance monitoring
     */
    static std::unique_ptr<ProfessionalStreamingAnalytics> create_performance_analytics();
    
    /**
     * @brief Create analytics optimized for compliance monitoring
     */
    static std::unique_ptr<ProfessionalStreamingAnalytics> create_compliance_analytics();
    
    /**
     * @brief Create analytics optimized for development and testing
     */
    static std::unique_ptr<ProfessionalStreamingAnalytics> create_development_analytics();
    
    /**
     * @brief Create custom analytics with specific configuration
     */
    static std::unique_ptr<ProfessionalStreamingAnalytics> create_custom_analytics(
        const ProfessionalAlertConfiguration& alert_config,
        std::chrono::milliseconds update_interval = std::chrono::milliseconds{1000});
};

} // namespace eti_network

#endif // PROFESSIONAL_STREAMING_ANALYTICS_HPP