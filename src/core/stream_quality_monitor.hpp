/**
 * @file stream_quality_monitor.h
 * @brief Professional Stream Quality Monitor for broadcast stream health assessment
 * 
 * Implements comprehensive real-time quality monitoring and assessment for ETI streams
 * with professional broadcast-grade metrics, threshold management, and alert systems
 * compliant with ETSI standards and broadcasting industry requirements.
 * 
 * @author Network/Stream Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 */

#ifndef STREAM_QUALITY_MONITOR_H
#define STREAM_QUALITY_MONITOR_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QReadWriteLock>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <vector>
#include <map>
#include <string>

#include "eti_types.hpp"

namespace eti {

/**
 * @brief Quality assessment level enumeration
 */
enum class QualityLevel {
    EXCELLENT,              // 95-100% quality
    GOOD,                   // 85-94% quality
    ACCEPTABLE,             // 70-84% quality
    POOR,                   // 50-69% quality
    CRITICAL,               // 25-49% quality
    FAILING                 // 0-24% quality
};

/**
 * @brief Alert severity enumeration
 */
enum class AlertSeverity {
    INFO,                   // Informational message
    WARNING,                // Warning condition
    MINOR,                  // Minor alarm
    MAJOR,                  // Major alarm
    CRITICAL,               // Critical alarm
    EMERGENCY               // Emergency condition
};

/**
 * @brief Quality metric type enumeration
 */
enum class QualityMetricType {
    SIGNAL_STRENGTH,        // RF signal strength
    BIT_ERROR_RATE,         // Bit error rate
    FRAME_ERROR_RATE,       // Frame error rate
    SYNC_QUALITY,           // Synchronization quality
    ENSEMBLE_CONFIDENCE,    // Ensemble decode confidence
    SERVICE_AVAILABILITY,   // Service availability percentage
    AUDIO_QUALITY,          // Audio quality metrics
    TIMING_ACCURACY,        // Timing accuracy
    SPECTRAL_PURITY,        // Spectral purity
    CONSTELLATION_QUALITY,  // Constellation diagram quality
    INTERFERENCE_LEVEL,     // Interference level
    MULTIPATH_DISTORTION   // Multipath distortion
};

/**
 * @brief Quality threshold configuration
 */
struct QualityThreshold {
    QualityMetricType metric_type;
    double warning_threshold;               // Warning level threshold
    double critical_threshold;              // Critical level threshold
    double emergency_threshold;             // Emergency level threshold
    std::chrono::seconds evaluation_period{30}; // Evaluation period
    size_t consecutive_violations = 3;      // Consecutive violations for alert
    bool enabled = true;                    // Threshold enabled
    QString custom_message;                 // Custom alert message
};

/**
 * @brief Quality measurement data
 */
struct QualityMeasurement {
    QualityMetricType metric_type;
    double value;                          // Measured value
    double normalized_value;               // Normalized value (0.0-1.0)
    QualityLevel quality_level;            // Assessed quality level
    std::chrono::system_clock::time_point timestamp; // Measurement timestamp
    bool is_valid = true;                  // Measurement validity
    QString unit;                          // Unit of measurement
    QString description;                   // Metric description
    
    // Statistical data
    double min_value = 0.0;                // Minimum value in period
    double max_value = 0.0;                // Maximum value in period
    double average_value = 0.0;            // Average value in period
    double standard_deviation = 0.0;       // Standard deviation
    
    // Trend analysis
    double trend_slope = 0.0;              // Trend slope (positive = improving)
    bool is_trending_up = false;           // Trending upward
    bool is_stable = true;                 // Value is stable
    
    QualityMeasurement() {
        timestamp = std::chrono::system_clock::now();
    }
};

/**
 * @brief Quality alert information
 */
struct QualityAlert {
    QString alert_id;                      // Unique alert identifier
    QString stream_id;                     // Associated stream ID
    QualityMetricType metric_type;         // Metric that triggered alert
    AlertSeverity severity;                // Alert severity level
    QString message;                       // Alert message
    std::chrono::system_clock::time_point timestamp; // Alert timestamp
    std::chrono::system_clock::time_point clear_time; // Alert clear time
    
    bool is_active = true;                 // Alert is active
    bool is_acknowledged = false;          // Alert acknowledged by operator
    size_t occurrence_count = 1;           // Number of occurrences
    
    QualityMeasurement measurement;        // Associated measurement
    QString recommended_action;            // Recommended action
    
    QualityAlert() {
        timestamp = std::chrono::system_clock::now();
    }
};

/**
 * @brief Comprehensive quality assessment report
 */
struct QualityAssessment {
    QString stream_id;                     // Stream identifier
    QualityLevel overall_quality;          // Overall quality assessment
    double overall_score;                  // Overall quality score (0.0-1.0)
    std::chrono::system_clock::time_point assessment_time; // Assessment timestamp
    
    std::map<QualityMetricType, QualityMeasurement> measurements; // Individual measurements
    std::vector<QualityAlert> active_alerts; // Active alerts
    
    // Statistical summary
    size_t total_measurements = 0;         // Total measurements taken
    size_t valid_measurements = 0;         // Valid measurements
    size_t error_measurements = 0;         // Measurements with errors
    double measurement_success_rate = 0.0; // Success rate
    
    // Trend analysis
    bool quality_improving = false;        // Quality is improving
    bool quality_degrading = false;        // Quality is degrading
    bool quality_stable = true;            // Quality is stable
    std::chrono::minutes stability_duration{0}; // Duration of current stability
    
    // Recommendations
    QStringList recommendations;           // Quality improvement recommendations
    QString priority_action;               // Highest priority action needed
    
    QualityAssessment() {
        assessment_time = std::chrono::system_clock::now();
    }
};

/**
 * @brief Quality monitoring configuration
 */
struct QualityMonitorConfig {
    // Monitoring intervals
    std::chrono::milliseconds measurement_interval{1000}; // Measurement interval
    std::chrono::seconds assessment_interval{30}; // Assessment interval
    std::chrono::minutes report_interval{5}; // Report generation interval
    
    // Threshold management
    std::vector<QualityThreshold> thresholds; // Quality thresholds
    bool auto_adjust_thresholds = false;   // Automatically adjust thresholds
    double threshold_sensitivity = 1.0;    // Threshold sensitivity factor
    
    // Alert configuration
    bool enable_alerts = true;             // Enable quality alerts
    AlertSeverity min_alert_severity = AlertSeverity::WARNING; // Minimum alert severity
    std::chrono::seconds alert_debounce{10}; // Alert debounce period
    size_t max_active_alerts = 100;       // Maximum active alerts
    
    // Data retention
    std::chrono::hours measurement_retention{24}; // Measurement retention period
    std::chrono::hours alert_retention{168}; // Alert retention period (7 days)
    size_t max_measurements_per_metric = 1000; // Maximum measurements per metric
    
    // Analysis configuration
    bool enable_trend_analysis = true;     // Enable trend analysis
    bool enable_statistical_analysis = true; // Enable statistical analysis
    bool enable_prediction = false;        // Enable quality prediction
    size_t trend_analysis_window = 100;    // Trend analysis window size
    
    // Performance optimization
    bool enable_parallel_processing = true; // Enable parallel processing
    size_t processing_thread_count = 2;    // Number of processing threads
    bool enable_caching = true;            // Enable measurement caching
    
    // ETSI compliance
    bool enforce_etsi_standards = true;    // Enforce ETSI standard compliance
    bool generate_etsi_reports = false;    // Generate ETSI-compliant reports
    QString etsi_measurement_standard = "ETSI EN 300 401"; // ETSI standard reference
};

/**
 * @brief Quality metric calculator interface
 */
class QualityMetricCalculator {
public:
    virtual ~QualityMetricCalculator() = default;
    
    virtual QualityMeasurement calculate_metric(const EtiFrame& frame) = 0;
    virtual QualityMeasurement calculate_metric_batch(const std::vector<EtiFrame>& frames) = 0;
    virtual QualityMetricType get_metric_type() const = 0;
    virtual QString get_metric_name() const = 0;
    virtual QString get_metric_unit() const = 0;
    virtual QString get_metric_description() const = 0;
    virtual bool is_real_time_capable() const = 0;
    
    // Calibration and configuration
    virtual void calibrate(const std::vector<EtiFrame>& reference_frames) {}
    virtual void set_reference_values(const std::map<QString, double>& values) {}
    virtual void enable_advanced_analysis(bool enabled) {}
};

/**
 * @brief Signal strength metric calculator
 */
class SignalStrengthCalculator : public QualityMetricCalculator {
public:
    QualityMeasurement calculate_metric(const EtiFrame& frame) override;
    QualityMeasurement calculate_metric_batch(const std::vector<EtiFrame>& frames) override;
    QualityMetricType get_metric_type() const override { return QualityMetricType::SIGNAL_STRENGTH; }
    QString get_metric_name() const override { return "Signal Strength"; }
    QString get_metric_unit() const override { return "dBm"; }
    QString get_metric_description() const override { return "RF signal strength measurement"; }
    bool is_real_time_capable() const override { return true; }
};

/**
 * @brief Bit error rate metric calculator
 */
class BitErrorRateCalculator : public QualityMetricCalculator {
public:
    QualityMeasurement calculate_metric(const EtiFrame& frame) override;
    QualityMeasurement calculate_metric_batch(const std::vector<EtiFrame>& frames) override;
    QualityMetricType get_metric_type() const override { return QualityMetricType::BIT_ERROR_RATE; }
    QString get_metric_name() const override { return "Bit Error Rate"; }
    QString get_metric_unit() const override { return "BER"; }
    QString get_metric_description() const override { return "Bit error rate measurement"; }
    bool is_real_time_capable() const override { return true; }
    
private:
    size_t count_bit_errors(const EtiFrame& frame);
    double calculate_ber(size_t errors, size_t total_bits);
};

/**
 * @brief Sync quality metric calculator
 */
class SyncQualityCalculator : public QualityMetricCalculator {
public:
    QualityMeasurement calculate_metric(const EtiFrame& frame) override;
    QualityMeasurement calculate_metric_batch(const std::vector<EtiFrame>& frames) override;
    QualityMetricType get_metric_type() const override { return QualityMetricType::SYNC_QUALITY; }
    QString get_metric_name() const override { return "Sync Quality"; }
    QString get_metric_unit() const override { return "%"; }
    QString get_metric_description() const override { return "Synchronization quality assessment"; }
    bool is_real_time_capable() const override { return true; }
    
private:
    double assess_sync_pattern_quality(const EtiFrame& frame);
    double assess_timing_accuracy(const EtiFrame& frame);
};

/**
 * @brief Audio quality metric calculator
 */
class AudioQualityCalculator : public QualityMetricCalculator {
public:
    QualityMeasurement calculate_metric(const EtiFrame& frame) override;
    QualityMeasurement calculate_metric_batch(const std::vector<EtiFrame>& frames) override;
    QualityMetricType get_metric_type() const override { return QualityMetricType::AUDIO_QUALITY; }
    QString get_metric_name() const override { return "Audio Quality"; }
    QString get_metric_unit() const override { return "MOS"; }
    QString get_metric_description() const override { return "Audio quality assessment (MOS)"; }
    bool is_real_time_capable() const override { return false; }
    
private:
    double calculate_mos_score(const std::vector<uint8_t>& audio_data);
    double analyze_thd_plus_noise(const std::vector<int16_t>& samples);
    double analyze_dynamic_range(const std::vector<int16_t>& samples);
};

/**
 * @brief Professional Stream Quality Monitor
 * 
 * Comprehensive real-time quality monitoring system for ETI streams with:
 * - Multi-metric quality assessment (signal, sync, audio, timing)
 * - Professional threshold management with configurable alerts
 * - ETSI standard compliance monitoring and reporting
 * - Real-time trend analysis and quality prediction
 * - Broadcast-grade alert system with severity levels
 * - Comprehensive quality reporting and analytics
 */
class StreamQualityMonitor : public QObject {
    Q_OBJECT

public:
    explicit StreamQualityMonitor(QObject* parent = nullptr);
    explicit StreamQualityMonitor(const QualityMonitorConfig& config, QObject* parent = nullptr);
    ~StreamQualityMonitor();

    // Configuration management
    void set_config(const QualityMonitorConfig& config);
    QualityMonitorConfig get_config() const;
    void update_measurement_interval(std::chrono::milliseconds interval);
    void update_assessment_interval(std::chrono::seconds interval);
    
    // Monitoring control
    void start_monitoring(const QString& stream_id);
    void stop_monitoring(const QString& stream_id);
    void pause_monitoring(const QString& stream_id);
    void resume_monitoring(const QString& stream_id);
    void start_all_monitoring();
    void stop_all_monitoring();
    
    // Stream management
    bool add_stream(const QString& stream_id);
    bool remove_stream(const QString& stream_id);
    QStringList get_monitored_streams() const;
    bool is_monitoring_stream(const QString& stream_id) const;
    
    // Frame processing
    void process_frame(const QString& stream_id, const EtiFrame& frame);
    void process_frame_batch(const QString& stream_id, const std::vector<EtiFrame>& frames);
    
    // Quality assessment
    QualityAssessment get_current_assessment(const QString& stream_id) const;
    QList<QualityAssessment> get_all_assessments() const;
    QualityLevel get_overall_quality(const QString& stream_id) const;
    double get_quality_score(const QString& stream_id) const;
    
    // Metric access
    QualityMeasurement get_latest_measurement(const QString& stream_id, QualityMetricType metric) const;
    std::vector<QualityMeasurement> get_measurement_history(const QString& stream_id, 
                                                           QualityMetricType metric, 
                                                           size_t max_count = 100) const;
    std::map<QualityMetricType, QualityMeasurement> get_all_latest_measurements(const QString& stream_id) const;
    
    // Threshold management
    void add_threshold(const QString& stream_id, const QualityThreshold& threshold);
    void update_threshold(const QString& stream_id, QualityMetricType metric, const QualityThreshold& threshold);
    void remove_threshold(const QString& stream_id, QualityMetricType metric);
    std::vector<QualityThreshold> get_thresholds(const QString& stream_id) const;
    void set_global_thresholds(const std::vector<QualityThreshold>& thresholds);
    
    // Alert management
    std::vector<QualityAlert> get_active_alerts(const QString& stream_id = QString()) const;
    std::vector<QualityAlert> get_alert_history(const QString& stream_id, 
                                               std::chrono::hours period = std::chrono::hours{24}) const;
    bool acknowledge_alert(const QString& alert_id);
    bool clear_alert(const QString& alert_id);
    void clear_all_alerts(const QString& stream_id = QString());
    size_t get_active_alert_count(const QString& stream_id = QString()) const;
    
    // Quality statistics
    double get_average_quality_score(const QString& stream_id, 
                                    std::chrono::hours period = std::chrono::hours{1}) const;
    double get_quality_trend(const QString& stream_id) const;
    std::chrono::minutes get_stability_duration(const QString& stream_id) const;
    size_t get_measurement_count(const QString& stream_id, QualityMetricType metric) const;
    
    // ETSI compliance
    bool is_etsi_compliant(const QString& stream_id) const;
    QStringList get_etsi_violations(const QString& stream_id) const;
    void enable_etsi_compliance_monitoring(bool enabled);
    QJsonObject generate_etsi_compliance_report(const QString& stream_id) const;
    
    // Reporting
    QJsonObject generate_quality_report(const QString& stream_id) const;
    QJsonObject generate_comprehensive_report() const;
    QStringList generate_text_report(const QString& stream_id) const;
    bool export_measurements_to_csv(const QString& stream_id, const QString& filename) const;
    
    // Performance analysis
    QStringList get_quality_recommendations(const QString& stream_id) const;
    QString get_priority_action(const QString& stream_id) const;
    double predict_quality_trend(const QString& stream_id, std::chrono::minutes horizon) const;
    
    // Advanced features
    void enable_automatic_threshold_adjustment(bool enabled);
    void calibrate_metrics(const QString& stream_id, const std::vector<EtiFrame>& reference_frames);
    void set_reference_quality_parameters(const std::map<QString, double>& parameters);
    
    // Callback registration
    using QualityCallback = std::function<void(const QString&, const QualityAssessment&)>;
    using AlertCallback = std::function<void(const QualityAlert&)>;
    using MeasurementCallback = std::function<void(const QString&, const QualityMeasurement&)>;
    using ThresholdCallback = std::function<void(const QString&, QualityMetricType, double)>;
    
    void set_quality_callback(QualityCallback callback);
    void set_alert_callback(AlertCallback callback);
    void set_measurement_callback(MeasurementCallback callback);
    void set_threshold_callback(ThresholdCallback callback);

signals:
    void quality_assessment_updated(const QString& stream_id, const eti::QualityAssessment& assessment);
    void quality_alert_triggered(const eti::QualityAlert& alert);
    void quality_alert_cleared(const QString& alert_id);
    void measurement_completed(const QString& stream_id, const eti::QualityMeasurement& measurement);
    void threshold_violated(const QString& stream_id, eti::QualityMetricType metric, double value);
    void quality_trend_changed(const QString& stream_id, double trend_slope);
    void etsi_compliance_violation(const QString& stream_id, const QString& violation);
    void monitoring_started(const QString& stream_id);
    void monitoring_stopped(const QString& stream_id);

private slots:
    void perform_measurements();
    void perform_assessments();
    void check_thresholds();
    void cleanup_old_data();
    void generate_periodic_reports();

private:
    void initialize_metric_calculators();
    void cleanup_metric_calculators();
    void validate_config(const QualityMonitorConfig& config);
    void setup_timers();
    void cleanup_timers();
    
    // Metric calculation
    void calculate_all_metrics(const QString& stream_id, const EtiFrame& frame);
    void calculate_metric_batch(const QString& stream_id, QualityMetricType metric, 
                               const std::vector<EtiFrame>& frames);
    
    // Assessment and analysis
    QualityAssessment perform_quality_assessment(const QString& stream_id);
    void analyze_quality_trends(const QString& stream_id, QualityAssessment& assessment);
    void generate_recommendations(const QString& stream_id, QualityAssessment& assessment);
    
    // Threshold management
    void check_metric_thresholds(const QString& stream_id, const QualityMeasurement& measurement);
    QualityAlert create_threshold_alert(const QString& stream_id, const QualityThreshold& threshold,
                                       const QualityMeasurement& measurement);
    
    // Data management
    void store_measurement(const QString& stream_id, const QualityMeasurement& measurement);
    void cleanup_measurements(const QString& stream_id);
    void cleanup_alerts(const QString& stream_id);
    
    // ETSI compliance checking
    void check_etsi_compliance(const QString& stream_id, const QualityAssessment& assessment);
    QStringList validate_etsi_requirements(const QString& stream_id) const;
    
    QualityMonitorConfig m_config;
    mutable QReadWriteLock m_config_lock;
    
    // Metric calculators
    std::map<QualityMetricType, std::unique_ptr<QualityMetricCalculator>> m_calculators;
    
    // Stream data
    std::map<QString, std::map<QualityMetricType, std::vector<QualityMeasurement>>> m_measurements;
    std::map<QString, std::vector<QualityThreshold>> m_stream_thresholds;
    std::map<QString, QualityAssessment> m_current_assessments;
    mutable QReadWriteLock m_data_lock;
    
    // Alert management
    std::vector<QualityAlert> m_active_alerts;
    std::vector<QualityAlert> m_alert_history;
    mutable QMutex m_alerts_mutex;
    std::atomic<size_t> m_alert_counter{0};
    
    // Monitoring state
    std::map<QString, bool> m_monitoring_streams;
    std::map<QString, std::chrono::system_clock::time_point> m_monitoring_start_times;
    mutable QMutex m_monitoring_mutex;
    
    // Timers
    std::unique_ptr<QTimer> m_measurement_timer;
    std::unique_ptr<QTimer> m_assessment_timer;
    std::unique_ptr<QTimer> m_threshold_timer;
    std::unique_ptr<QTimer> m_cleanup_timer;
    std::unique_ptr<QTimer> m_report_timer;
    
    // Global thresholds
    std::vector<QualityThreshold> m_global_thresholds;
    QMutex m_global_thresholds_mutex;
    
    // Callbacks
    QualityCallback m_quality_callback;
    AlertCallback m_alert_callback;
    MeasurementCallback m_measurement_callback;
    ThresholdCallback m_threshold_callback;
    QMutex m_callbacks_mutex;
    
    // Performance optimization
    std::atomic<bool> m_parallel_processing_enabled{true};
    std::atomic<bool> m_caching_enabled{true};
    std::atomic<bool> m_etsi_compliance_enabled{true};
    
    static constexpr size_t MAX_MEASUREMENTS_PER_METRIC = 10000;
    static constexpr size_t MAX_ACTIVE_ALERTS = 1000;
    static constexpr size_t MAX_ALERT_HISTORY = 10000;
};

/**
 * @brief Factory for creating pre-configured quality monitors
 */
class StreamQualityMonitorFactory {
public:
    /**
     * @brief Create monitor for broadcast operations with standard thresholds
     */
    static std::unique_ptr<StreamQualityMonitor> create_broadcast_monitor();

    /**
     * @brief Create monitor for critical broadcast applications
     */
    static std::unique_ptr<StreamQualityMonitor> create_critical_monitor();

    /**
     * @brief Create monitor for ETSI compliance testing
     */
    static std::unique_ptr<StreamQualityMonitor> create_etsi_compliance_monitor();

    /**
     * @brief Create monitor for research and development
     */
    static std::unique_ptr<StreamQualityMonitor> create_research_monitor();
};

/**
 * @brief Utility functions for quality analysis
 */
namespace QualityUtils {
    
    /**
     * @brief Quality level conversion utilities
     */
    QString quality_level_to_string(QualityLevel level);
    QualityLevel quality_score_to_level(double score);
    QString alert_severity_to_string(AlertSeverity severity);
    
    /**
     * @brief Threshold calculation utilities
     */
    QualityThreshold create_standard_threshold(QualityMetricType metric, double warning, double critical);
    std::vector<QualityThreshold> create_broadcast_thresholds();
    std::vector<QualityThreshold> create_etsi_compliant_thresholds();
    
    /**
     * @brief Statistical analysis utilities
     */
    double calculate_quality_trend(const std::vector<QualityMeasurement>& measurements);
    double calculate_quality_stability(const std::vector<QualityMeasurement>& measurements);
    bool is_quality_degrading(const std::vector<QualityMeasurement>& measurements, double threshold = 0.05);
    
    /**
     * @brief Reporting utilities
     */
    QJsonObject measurements_to_json(const std::vector<QualityMeasurement>& measurements);
    QString generate_quality_summary(const QualityAssessment& assessment);
    QStringList generate_improvement_recommendations(const QualityAssessment& assessment);
}

} // namespace eti

#endif // STREAM_QUALITY_MONITOR_H