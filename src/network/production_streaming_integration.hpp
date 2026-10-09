/**
 * @file production_streaming_integration.hpp
 * @brief Production Streaming Platform Integration Manager
 * 
 * Complete integration manager for Phase 3 production streaming platform:
 * - Professional multicast receiver with zero packet loss
 * - Enhanced stream recording with comprehensive metadata
 * - Advanced SAP/SDP discovery with real-time updates
 * - Professional streaming analytics with sub-millisecond precision
 * - Unified platform management with performance validation
 * - Enterprise-grade monitoring and quality assurance
 * 
 * @author Network/Stream Agent - Phase 3 Production Platform Integration
 * @date 2025-09-28
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 */

#ifndef PRODUCTION_STREAMING_INTEGRATION_HPP
#define PRODUCTION_STREAMING_INTEGRATION_HPP

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QThread>
#include <QJsonObject>
#include <QDateTime>
#include <QVariantMap>
#include <memory>
#include <atomic>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <functional>

#include "professional_multicast_receiver.hpp"
#include "professional_stream_recorder.hpp"
#include "sap_sdp_discovery_engine.hpp"
#include "professional_streaming_analytics.hpp"
#include "../core/eti_types.hpp"

namespace eti_network {

/**
 * @brief Production streaming platform configuration
 */
struct ProductionPlatformConfig {
    // Platform identification
    QString platform_name = "StreamDAB Production Platform";
    QString platform_version = "3.0.0";
    QString operator_name;
    QString site_identifier;
    
    // Multicast streaming configuration
    MulticastStreamConfig multicast_config;
    bool enable_multicast_streaming = true;
    bool enable_redundancy = false;
    QString backup_multicast_address;
    
    // Recording configuration
    ProfessionalSegmentationConfig recording_config;
    bool enable_automatic_recording = false;
    QString default_recording_path = "/var/recordings/streamdab";
    ProfessionalRecordingFormat default_recording_format = ProfessionalRecordingFormat::ETI_NI_NATIVE;
    
    // Discovery configuration
    DiscoveryConfiguration discovery_config;
    bool enable_automatic_discovery = true;
    std::chrono::seconds discovery_refresh_interval{60};
    bool enable_service_verification = true;
    
    // Analytics configuration
    ProfessionalAlertConfiguration alert_config;
    bool enable_analytics = true;
    std::chrono::milliseconds analytics_update_interval{1000};
    bool enable_predictive_analytics = true;
    bool enable_compliance_monitoring = true;
    
    // Performance targets
    double target_fps = 1000.0;                            // >900 FPS requirement
    std::chrono::microseconds target_latency{17000};       // <17ms requirement
    double target_quality = 0.95;                          // 95% quality target
    size_t max_memory_usage_mb = 512;                       // 512MB memory limit
    
    // Integration behavior
    bool enable_auto_stream_switching = true;              // Auto-switch to best stream
    bool enable_quality_adaptation = true;                 // Adaptive quality control
    bool enable_performance_optimization = true;           // Auto-optimization
    bool enable_comprehensive_logging = true;              // Detailed logging
    
    // Alert and monitoring
    bool enable_health_monitoring = true;                  // Platform health monitoring
    std::chrono::seconds health_check_interval{30};       // Health check frequency
    bool enable_performance_alerts = true;                 // Performance alerts
    bool enable_dashboard_integration = true;              // Dashboard updates
    
    // Advanced features
    bool enable_load_balancing = false;                    // Multi-stream load balancing
    bool enable_failover = false;                          // Automatic failover
    QString failover_strategy = "quality_based";           // "quality_based", "latency_based"
    std::chrono::seconds failover_timeout{10};            // Failover decision timeout
    
    // Quality assurance
    bool enable_integrity_validation = true;               // Data integrity checks
    bool enable_benchmark_testing = false;                 // Continuous benchmarking
    bool enable_compliance_validation = true;              // ETSI compliance validation
    std::chrono::minutes validation_interval{15};         // Validation frequency
    
    QJsonObject to_json() const;
    void from_json(const QJsonObject& json);
    bool is_valid() const;
};

/**
 * @brief Platform operational status
 */
struct PlatformOperationalStatus {
    // Overall platform status
    QString platform_status = "initializing";              // "initializing", "active", "degraded", "failed"
    QString primary_issue;                                  // Most critical issue description
    double platform_health_score = 1.0;                   // Overall health (0.0-1.0)
    std::chrono::steady_clock::time_point last_update;
    
    // Component status
    struct ComponentStatus {
        bool is_active = false;
        bool is_healthy = true;
        QString status_message;
        std::chrono::steady_clock::time_point last_heartbeat;
        QVariantMap component_metrics;
    };
    
    ComponentStatus multicast_receiver_status;
    ComponentStatus stream_recorder_status;
    ComponentStatus discovery_engine_status;
    ComponentStatus analytics_engine_status;
    
    // Performance metrics
    ProfessionalStreamingMetrics current_performance;
    bool performance_targets_met = true;
    QStringList performance_issues;
    
    // Active streams and services
    size_t active_stream_count = 0;
    size_t discovered_service_count = 0;
    size_t recording_session_count = 0;
    QStringList active_alerts;
    
    // Resource utilization
    double cpu_utilization = 0.0;
    double memory_utilization_mb = 0.0;
    double network_utilization = 0.0;
    double disk_utilization = 0.0;
    
    // Operational statistics
    std::chrono::steady_clock::time_point platform_start_time;
    std::chrono::microseconds total_uptime{0};
    double uptime_percentage = 100.0;
    size_t total_restarts = 0;
    size_t total_failures = 0;
    
    QJsonObject to_json() const;
    void from_json(const QJsonObject& json);
    QString to_string() const;
};

/**
 * @brief Performance validation results
 */
struct PerformanceValidationResults {
    // Test configuration
    std::chrono::steady_clock::time_point test_start_time;
    std::chrono::steady_clock::time_point test_end_time;
    std::chrono::milliseconds test_duration{0};
    QString test_configuration;
    
    // Performance measurements
    double measured_fps = 0.0;
    std::chrono::microseconds measured_latency{0};
    double measured_quality = 0.0;
    size_t measured_memory_mb = 0;
    
    // Target comparisons
    bool fps_target_met = false;
    bool latency_target_met = false;
    bool quality_target_met = false;
    bool memory_target_met = false;
    
    // Detailed metrics
    struct DetailedMetrics {
        double minimum_fps = 0.0;
        double maximum_fps = 0.0;
        double fps_standard_deviation = 0.0;
        std::chrono::microseconds minimum_latency{0};
        std::chrono::microseconds maximum_latency{0};
        std::chrono::microseconds latency_standard_deviation{0};
        double minimum_quality = 0.0;
        double maximum_quality = 0.0;
        double quality_standard_deviation = 0.0;
    } detailed_metrics;
    
    // Test results
    bool overall_pass = false;
    QString performance_grade;                              // "A+", "A", "B+", "B", "C", "F"
    QStringList performance_issues;
    QStringList optimization_recommendations;
    
    // Benchmark comparisons
    double performance_index = 1.0;                        // Relative to baseline
    QString benchmark_comparison;
    bool exceeds_requirements = false;
    
    QJsonObject to_json() const;
    void from_json(const QJsonObject& json);
    QString generate_report() const;
};

/**
 * @brief Professional platform health monitor
 */
class PlatformHealthMonitor : public QObject {
    Q_OBJECT
    
public:
    explicit PlatformHealthMonitor(QObject* parent = nullptr);
    ~PlatformHealthMonitor() override;
    
    // Health monitoring
    void start_monitoring(std::chrono::seconds interval = std::chrono::seconds{30});
    void stop_monitoring();
    bool is_monitoring_active() const;
    
    // Component registration
    void register_component(const QString& component_name, QObject* component);
    void unregister_component(const QString& component_name);
    QStringList get_registered_components() const;
    
    // Health assessment
    PlatformOperationalStatus get_platform_status() const;
    double get_platform_health_score() const;
    QString get_primary_issue() const;
    bool is_platform_healthy() const;
    
    // Component health
    bool is_component_healthy(const QString& component_name) const;
    QString get_component_status(const QString& component_name) const;
    QVariantMap get_component_metrics(const QString& component_name) const;
    
    // Performance validation
    PerformanceValidationResults validate_performance(
        std::chrono::seconds test_duration = std::chrono::seconds{60}) const;
    bool meets_performance_targets() const;
    QStringList get_performance_issues() const;

signals:
    void platform_status_changed(const QString& new_status, const QString& previous_status);
    void component_status_changed(const QString& component_name, bool is_healthy);
    void health_score_changed(double new_score, double previous_score);
    void performance_target_missed(const QString& target, double current, double expected);
    void critical_issue_detected(const QString& issue, const QString& component);

private slots:
    void perform_health_check();
    void validate_component_health();
    void assess_platform_performance();

private:
    void calculate_health_score();
    void identify_primary_issue();
    void check_performance_targets();
    void update_operational_status();
    
    struct ComponentInfo {
        QString component_name;
        QObject* component_ptr;
        std::chrono::steady_clock::time_point last_heartbeat;
        bool is_healthy = true;
        QString last_status;
        QVariantMap metrics;
    };
    
    std::unordered_map<std::string, ComponentInfo> m_components;
    PlatformOperationalStatus m_platform_status;
    
    std::unique_ptr<QTimer> m_health_timer;
    std::unique_ptr<QTimer> m_validation_timer;
    
    mutable std::mutex m_status_mutex;
    std::atomic<bool> m_monitoring_active{false};
    
    static constexpr std::chrono::seconds DEFAULT_HEALTH_INTERVAL{30};
    static constexpr std::chrono::seconds VALIDATION_INTERVAL{300}; // 5 minutes
    static constexpr double HEALTH_THRESHOLD = 0.8;
};

/**
 * @brief Production-Grade Streaming Platform Integration Manager
 * 
 * Enterprise-class integration manager implementing:
 * - Unified management of all streaming platform components
 * - Zero packet loss multicast streaming with redundancy
 * - Comprehensive recording with professional metadata
 * - Real-time service discovery and verification
 * - Advanced analytics with predictive capabilities
 * - Performance validation and compliance monitoring
 */
class ProductionStreamingIntegration : public QObject {
    Q_OBJECT
    
public:
    explicit ProductionStreamingIntegration(QObject* parent = nullptr);
    ~ProductionStreamingIntegration() override;
    
    // Platform lifecycle management
    bool initialize_platform(const ProductionPlatformConfig& config);
    bool start_platform();
    void stop_platform();
    void restart_platform();
    bool is_platform_active() const;
    
    // Configuration management
    void set_platform_config(const ProductionPlatformConfig& config);
    ProductionPlatformConfig get_platform_config() const;
    void update_performance_targets(double fps, std::chrono::microseconds latency, double quality);
    void enable_advanced_features(bool redundancy, bool analytics, bool auto_optimization);
    
    // Stream management
    QString add_stream_source(const MulticastStreamConfig& stream_config);
    bool remove_stream_source(const QString& stream_id);
    QStringList get_active_streams() const;
    StreamQualityMetrics get_stream_quality(const QString& stream_id) const;
    bool switch_primary_stream(const QString& stream_id);
    
    // Recording management
    QString start_recording_session(const QString& output_directory,
                                   ProfessionalRecordingFormat format = ProfessionalRecordingFormat::ETI_NI_NATIVE);
    bool stop_recording_session(const QString& session_id);
    QStringList get_recording_sessions() const;
    RecordingMetadata get_recording_metadata(const QString& session_id) const;
    
    // Service discovery
    QList<DiscoveredBroadcastService> get_discovered_services() const;
    bool connect_to_discovered_service(const QString& service_id);
    void refresh_service_discovery();
    void enable_continuous_discovery(bool enabled);
    
    // Analytics and monitoring
    ProfessionalStreamingMetrics get_current_metrics() const;
    StreamingDashboardData get_dashboard_data() const;
    TrendAnalysis get_trend_analysis(std::chrono::hours period = std::chrono::hours{1}) const;
    QStringList get_active_alerts() const;
    QString get_performance_report(std::chrono::hours period = std::chrono::hours{24}) const;
    
    // Platform status and health
    PlatformOperationalStatus get_platform_status() const;
    double get_platform_health_score() const;
    bool is_platform_healthy() const;
    QString get_diagnostic_report() const;
    
    // Performance validation
    PerformanceValidationResults validate_platform_performance(
        std::chrono::seconds test_duration = std::chrono::seconds{60}) const;
    bool meets_performance_requirements() const;
    QStringList get_optimization_recommendations() const;
    
    // Advanced operations
    void enable_automatic_optimization(bool enabled);
    void enable_failover_protection(bool enabled, const QString& strategy = "quality_based");
    void perform_load_balancing();
    void export_platform_configuration(const QString& filename) const;
    void import_platform_configuration(const QString& filename);
    
    // Quality assurance
    bool validate_etsi_compliance() const;
    QString get_compliance_report() const;
    void enable_continuous_validation(bool enabled);
    void perform_integrity_check();
    
    // Callback registration
    using PlatformCallback = std::function<void(const PlatformOperationalStatus&)>;
    using PerformanceCallback = std::function<void(const ProfessionalStreamingMetrics&)>;
    using AlertCallback = std::function<void(const QString&, const QString&, const QString&)>;
    using ServiceCallback = std::function<void(const DiscoveredBroadcastService&)>;
    
    void set_platform_callback(PlatformCallback callback);
    void set_performance_callback(PerformanceCallback callback);
    void set_alert_callback(AlertCallback callback);
    void set_service_callback(ServiceCallback callback);

signals:
    void platform_initialized();
    void platform_started();
    void platform_stopped();
    void platform_status_changed(const PlatformOperationalStatus& status);
    
    void stream_added(const QString& stream_id);
    void stream_removed(const QString& stream_id);
    void primary_stream_changed(const QString& new_primary_id, const QString& previous_primary_id);
    void stream_quality_changed(const QString& stream_id, const StreamQualityMetrics& quality);
    
    void recording_started(const QString& session_id);
    void recording_stopped(const QString& session_id);
    void recording_error(const QString& session_id, const QString& error);
    
    void service_discovered(const DiscoveredBroadcastService& service);
    void service_connected(const QString& service_id);
    void service_disconnected(const QString& service_id);
    
    void performance_metrics_updated(const ProfessionalStreamingMetrics& metrics);
    void performance_target_missed(const QString& target, double current, double expected);
    void performance_degradation_detected(const QString& component, double severity);
    
    void alert_triggered(const QString& alert_id, const QString& type, const QString& message);
    void alert_cleared(const QString& alert_id);
    void critical_failure_detected(const QString& component, const QString& reason);
    
    void optimization_applied(const QString& optimization, double improvement);
    void failover_initiated(const QString& from_stream, const QString& to_stream, const QString& reason);
    void compliance_issue_detected(const QString& issue, const QString& standard);

private slots:
    void handle_multicast_frame(const QString& stream_id, const eti::EtiFrame& frame, 
                               const StreamQualityMetrics& quality);
    void handle_service_discovery(const DiscoveredBroadcastService& service);
    void handle_analytics_update(const ProfessionalStreamingMetrics& metrics);
    void handle_alert_triggered(const QString& alert_id, const QString& type, const QString& message);
    void handle_platform_status_change(const QString& new_status, const QString& previous_status);
    void perform_platform_maintenance();
    void monitor_performance_targets();
    void check_failover_conditions();

private:
    // Component initialization
    void initialize_components();
    void cleanup_components();
    void setup_component_connections();
    void configure_components();
    
    // Stream management
    void update_primary_stream();
    void handle_stream_failure(const QString& stream_id);
    void optimize_stream_configuration();
    
    // Performance optimization
    void apply_automatic_optimizations();
    void analyze_performance_bottlenecks();
    void adjust_quality_settings();
    void balance_resource_utilization();
    
    // Platform coordination
    void coordinate_component_lifecycle();
    void synchronize_component_states();
    void handle_component_failure(const QString& component_name);
    void initiate_recovery_procedures();
    
    // Configuration and state
    ProductionPlatformConfig m_platform_config;
    PlatformOperationalStatus m_platform_status;
    
    // Core components
    std::unique_ptr<ProfessionalMulticastReceiver> m_multicast_receiver;
    std::unique_ptr<ProfessionalStreamRecorder> m_stream_recorder;
    std::unique_ptr<SapSdpDiscoveryEngine> m_discovery_engine;
    std::unique_ptr<ProfessionalStreamingAnalytics> m_streaming_analytics;
    std::unique_ptr<PlatformHealthMonitor> m_health_monitor;
    
    // Stream management
    QString m_primary_stream_id;
    QStringList m_backup_stream_ids;
    std::unordered_map<std::string, StreamQualityMetrics> m_stream_quality_cache;
    
    // Recording sessions
    QStringList m_active_recording_sessions;
    std::unordered_map<std::string, RecordingMetadata> m_recording_metadata_cache;
    
    // Platform state
    std::atomic<bool> m_platform_initialized{false};
    std::atomic<bool> m_platform_active{false};
    std::atomic<bool> m_automatic_optimization{true};
    std::atomic<bool> m_failover_enabled{false};
    
    // Maintenance and monitoring
    std::unique_ptr<QTimer> m_maintenance_timer;
    std::unique_ptr<QTimer> m_performance_timer;
    std::unique_ptr<QTimer> m_failover_timer;
    
    // Performance tracking
    std::chrono::steady_clock::time_point m_platform_start_time;
    mutable std::mutex m_performance_mutex;
    mutable std::mutex m_status_mutex;
    
    // Callbacks
    PlatformCallback m_platform_callback;
    PerformanceCallback m_performance_callback;
    AlertCallback m_alert_callback;
    ServiceCallback m_service_callback;
    mutable std::mutex m_callback_mutex;
    
    static constexpr std::chrono::milliseconds MAINTENANCE_INTERVAL{60000};     // 1 minute
    static constexpr std::chrono::milliseconds PERFORMANCE_INTERVAL{5000};     // 5 seconds
    static constexpr std::chrono::milliseconds FAILOVER_CHECK_INTERVAL{1000};  // 1 second
    static constexpr double FAILOVER_QUALITY_THRESHOLD = 0.8;
    static constexpr std::chrono::milliseconds FAILOVER_DECISION_TIMEOUT{5000}; // 5 seconds
};

/**
 * @brief Factory for creating production streaming integrations
 */
class ProductionStreamingIntegrationFactory {
public:
    /**
     * @brief Create integration optimized for broadcast operations
     */
    static std::unique_ptr<ProductionStreamingIntegration> create_broadcast_integration();
    
    /**
     * @brief Create integration optimized for monitoring applications
     */
    static std::unique_ptr<ProductionStreamingIntegration> create_monitoring_integration();
    
    /**
     * @brief Create integration optimized for recording and archival
     */
    static std::unique_ptr<ProductionStreamingIntegration> create_archival_integration();
    
    /**
     * @brief Create integration optimized for development and testing
     */
    static std::unique_ptr<ProductionStreamingIntegration> create_development_integration();
    
    /**
     * @brief Create custom integration with specific configuration
     */
    static std::unique_ptr<ProductionStreamingIntegration> create_custom_integration(
        const ProductionPlatformConfig& config);
};

/**
 * @brief Performance benchmark utilities
 */
namespace PlatformBenchmarking {
    
    /**
     * @brief Run comprehensive platform performance benchmark
     */
    PerformanceValidationResults run_comprehensive_benchmark(
        ProductionStreamingIntegration* platform,
        std::chrono::seconds test_duration = std::chrono::seconds{300});
    
    /**
     * @brief Run targeted latency benchmark
     */
    PerformanceValidationResults run_latency_benchmark(
        ProductionStreamingIntegration* platform,
        std::chrono::seconds test_duration = std::chrono::seconds{60});
    
    /**
     * @brief Run throughput stress test
     */
    PerformanceValidationResults run_throughput_stress_test(
        ProductionStreamingIntegration* platform,
        std::chrono::seconds test_duration = std::chrono::seconds{120});
    
    /**
     * @brief Run quality stability test
     */
    PerformanceValidationResults run_quality_stability_test(
        ProductionStreamingIntegration* platform,
        std::chrono::seconds test_duration = std::chrono::seconds{180});
    
    /**
     * @brief Generate benchmark comparison report
     */
    QString generate_benchmark_report(const QList<PerformanceValidationResults>& results);
    
    /**
     * @brief Export benchmark results
     */
    void export_benchmark_results(const PerformanceValidationResults& results, 
                                 const QString& filename, const QString& format = "json");
}

} // namespace eti_network

#endif // PRODUCTION_STREAMING_INTEGRATION_HPP