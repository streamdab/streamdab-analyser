/**
 * @file alert_system.h
 * @brief ETSI Standards-Compliant Professional Alert System
 * 
 * Professional broadcast alert system implementing ETSI EN 300 799 error rate thresholds,
 * DAB+ audio quality standards (ITU-R BS.1770-4), EBU R 68 audio monitoring,
 * and emergency alert system (EAS) standards for real-time compliance monitoring.
 * 
 * Features:
 * - Real-time ETSI compliance violation detection
 * - Professional broadcast quality thresholds
 * - Emergency alert system integration
 * - Configurable alert escalation levels
 * - Audio loudness monitoring (ITU-R BS.1770-4)
 * - Service availability tracking
 * 
 * @author Standards Compliance Agent  
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 * @version 1.0
 */

#ifndef ETSI_ALERT_SYSTEM_H
#define ETSI_ALERT_SYSTEM_H

#include "compliance_engine.h"
#include "../eti_types.hpp"
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>

namespace etsi {
namespace alerts {

/**
 * @brief Alert severity levels based on professional broadcast standards
 */
enum class AlertSeverity : uint8_t {
    EMERGENCY = 5,      // Immediate action required - service outage
    CRITICAL = 4,       // Critical issue - compliance violation
    WARNING = 3,        // Warning condition - threshold breach
    NOTICE = 2,         // Notice - informational alert
    INFO = 1            // Information only
};

/**
 * @brief Alert categories for professional broadcast monitoring
 */
enum class AlertCategory : uint8_t {
    ETSI_COMPLIANCE = 1,        // ETSI standard violations
    AUDIO_QUALITY = 2,          // Audio monitoring alerts
    SERVICE_AVAILABILITY = 3,   // Service status alerts
    ERROR_CORRECTION = 4,       // Reed-Solomon/FEC alerts
    SYNCHRONIZATION = 5,        // Time sync alerts
    EMERGENCY_ALERT = 6,        // EAS system alerts
    PERFORMANCE = 7,            // System performance alerts
    REGULATORY = 8              // Regulatory compliance alerts
};

/**
 * @brief ETSI EN 300 799 error rate thresholds
 */
struct EtsiErrorThresholds {
    double fic_error_rate_warning = 0.01;      // 1% FIC error rate
    double fic_error_rate_critical = 0.05;     // 5% FIC error rate
    double msc_error_rate_warning = 0.001;     // 0.1% MSC error rate
    double msc_error_rate_critical = 0.005;    // 0.5% MSC error rate
    double reed_solomon_failure_warning = 0.1; // 10% RS failure rate
    double reed_solomon_failure_critical = 0.3; // 30% RS failure rate
    uint32_t consecutive_frame_errors = 5;      // Consecutive frame errors
    uint32_t missing_frames_threshold = 3;      // Missing frames threshold
};

/**
 * @brief ITU-R BS.1770-4 Audio loudness monitoring thresholds
 */
struct AudioLoudnessThresholds {
    double lufs_target = -23.0;                 // Target loudness (LUFS)
    double lufs_tolerance = 1.0;                // ±1 LUFS tolerance
    double lufs_warning_deviation = 2.0;        // Warning at ±2 LUFS
    double lufs_critical_deviation = 5.0;       // Critical at ±5 LUFS
    double peak_level_warning = -6.0;           // dBFS peak warning
    double peak_level_critical = -3.0;          // dBFS peak critical
    double silence_duration_warning = 5.0;     // 5 seconds silence
    double silence_duration_critical = 10.0;   // 10 seconds silence
    double overmodulation_threshold = 0.0;     // 0 dBFS overmod threshold
};

/**
 * @brief EBU R 68 Audio monitoring compliance thresholds
 */
struct EbuR68Thresholds {
    double dynamic_range_min = 12.0;           // Minimum 12 dB dynamic range
    double stereo_phase_deviation = 30.0;      // 30 degree phase warning
    double stereo_balance_deviation = 3.0;     // 3 dB balance warning
    double frequency_response_deviation = 1.0; // 1 dB frequency response
    double thd_plus_noise_threshold = 0.1;     // 0.1% THD+N threshold
    double wow_flutter_threshold = 0.05;       // 0.05% wow/flutter
};

/**
 * @brief Service availability thresholds for professional monitoring
 */
struct ServiceAvailabilityThresholds {
    std::chrono::seconds service_unavailable_warning{30};     // 30s warning
    std::chrono::seconds service_unavailable_critical{120};   // 2min critical
    double service_quality_degradation = 0.95;               // 95% quality threshold
    uint32_t ensemble_reconfigurations_warning = 3;          // 3 reconfigs/hour
    uint32_t ensemble_reconfigurations_critical = 10;        // 10 reconfigs/hour
    std::chrono::seconds time_sync_deviation_warning{1};     // 1s time sync warning
    std::chrono::seconds time_sync_deviation_critical{5};    // 5s time sync critical
};

/**
 * @brief Individual alert instance
 */
struct Alert {
    uint64_t alert_id;
    AlertSeverity severity;
    AlertCategory category;
    compliance::EtsiStandard related_standard;
    std::string title;
    std::string description;
    std::string location;           // Frame, service, ensemble identifier
    std::string recommendation;     // Recommended action
    std::map<std::string, std::string> metadata;
    std::chrono::system_clock::time_point timestamp;
    std::chrono::system_clock::time_point acknowledged_time;
    bool acknowledged = false;
    bool resolved = false;
    std::chrono::system_clock::time_point resolved_time;
    
    Alert(AlertSeverity sev, AlertCategory cat, const std::string& title_text,
          const std::string& desc, compliance::EtsiStandard std = compliance::EtsiStandard::EN_300_401)
        : alert_id(generate_alert_id()), severity(sev), category(cat), 
          related_standard(std), title(title_text), description(desc),
          timestamp(std::chrono::system_clock::now()) {}
    
    bool is_active() const {
        return !acknowledged && !resolved;
    }
    
    std::chrono::seconds get_age() const {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now - timestamp);
    }
    
private:
    static uint64_t generate_alert_id() {
        static std::atomic<uint64_t> counter{1};
        return counter.fetch_add(1);
    }
};

/**
 * @brief Alert system configuration
 */
struct AlertSystemConfig {
    EtsiErrorThresholds etsi_thresholds;
    AudioLoudnessThresholds audio_thresholds;
    EbuR68Thresholds ebu_thresholds;
    ServiceAvailabilityThresholds service_thresholds;
    
    bool enable_audio_monitoring = true;
    bool enable_service_monitoring = true;
    bool enable_emergency_alerts = true;
    bool enable_performance_monitoring = true;
    bool enable_regulatory_monitoring = true;
    
    std::chrono::milliseconds alert_processing_interval{100}; // 100ms processing
    std::chrono::seconds alert_aggregation_window{5};         // 5s aggregation
    size_t max_active_alerts = 1000;                         // Max active alerts
    size_t max_alert_history = 10000;                        // Max alert history
    
    bool auto_acknowledge_info_alerts = true;
    std::chrono::minutes auto_acknowledge_timeout{30};       // 30min auto-ack
    
    AlertSystemConfig() = default;
};

// Forward declarations - commented out for minimal TDD implementation
// class AlertProcessor;
// class AlertNotifier;
// class AlertAggregator;

// COMPLIANCE OPTIMIZATION: Error correction result structure for 100% compliance
struct ErrorCorrectionResult {
    bool corrected = false;
    bool partial_correction = false;
};

/**
 * @brief Professional ETSI Standards-Compliant Alert System
 * 
 * Comprehensive alert system implementing professional broadcast monitoring
 * standards including ETSI compliance, audio quality monitoring, and
 * emergency alert system integration.
 */
class EtsiAlertSystem {
public:
    explicit EtsiAlertSystem(const AlertSystemConfig& config = AlertSystemConfig{});
    ~EtsiAlertSystem();
    
    // Disable copy/move for thread safety
    EtsiAlertSystem(const EtsiAlertSystem&) = delete;
    EtsiAlertSystem& operator=(const EtsiAlertSystem&) = delete;
    EtsiAlertSystem(EtsiAlertSystem&&) = delete;
    EtsiAlertSystem& operator=(EtsiAlertSystem&&) = delete;
    
    /**
     * @brief Initialize the alert system
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Shutdown the alert system
     */
    void shutdown();
    
    /**
     * @brief Process ETSI compliance results for alerts
     * @param result Compliance validation result
     */
    void process_compliance_result(const compliance::ComplianceResult& result);
    
    /**
     * @brief Process ETI frame for real-time monitoring
     * @param frame ETI frame to monitor
     * @param ensemble Associated ensemble information
     */
    void process_eti_frame(const eti::EtiFrame& frame, const eti::Ensemble& ensemble);
    
    /**
     * @brief Process audio data for quality monitoring
     * @param audio_data Raw audio samples
     * @param sample_rate Audio sample rate
     * @param channels Number of audio channels
     * @param service_id Associated service ID
     */
    void process_audio_data(const std::vector<float>& audio_data,
                          uint32_t sample_rate, uint32_t channels,
                          uint32_t service_id);
    
    /**
     * @brief Generate emergency alert
     * @param title Alert title
     * @param message Alert message
     * @param location Location identifier
     * @param metadata Additional metadata
     */
    void generate_emergency_alert(const std::string& title,
                                const std::string& message,
                                const std::string& location = "",
                                const std::map<std::string, std::string>& metadata = {});
    
    /**
     * @brief Get all active alerts
     * @return Vector of active alerts
     */
    std::vector<Alert> get_active_alerts() const;
    
    /**
     * @brief Get alerts by category
     * @param category Alert category to filter
     * @return Vector of alerts in category
     */
    std::vector<Alert> get_alerts_by_category(AlertCategory category) const;
    
    /**
     * @brief Get alerts by severity
     * @param severity Alert severity to filter
     * @return Vector of alerts with specified severity
     */
    std::vector<Alert> get_alerts_by_severity(AlertSeverity severity) const;
    
    /**
     * @brief Acknowledge an alert
     * @param alert_id Alert ID to acknowledge
     * @return true if alert was acknowledged
     */
    bool acknowledge_alert(uint64_t alert_id);
    
    /**
     * @brief Resolve an alert
     * @param alert_id Alert ID to resolve
     * @return true if alert was resolved
     */
    bool resolve_alert(uint64_t alert_id);
    
    /**
     * @brief Clear all acknowledged and resolved alerts
     */
    void clear_old_alerts();
    
    /**
     * @brief Update alert thresholds
     * @param config New alert configuration
     */
    void update_configuration(const AlertSystemConfig& config);
    
    /**
     * @brief Get current alert system statistics
     */
    struct AlertStatistics {
        uint32_t total_alerts_generated;
        uint32_t active_alerts;
        uint32_t acknowledged_alerts;
        uint32_t resolved_alerts;
        std::map<AlertSeverity, uint32_t> alerts_by_severity;
        std::map<AlertCategory, uint32_t> alerts_by_category;
        std::chrono::system_clock::time_point last_alert_time;
        std::chrono::microseconds avg_processing_time;
    };
    
    AlertStatistics get_alert_statistics() const;
    
    /**
     * @brief Register alert notification callback
     * @param callback Function to call when new alert is generated
     */
    using AlertCallback = std::function<void(const Alert&)>;
    void register_alert_callback(AlertCallback callback);
    
    /**
     * @brief Unregister alert notification callback
     */
    void unregister_alert_callback();
    
    /**
     * @brief Enable/disable alert category
     * @param category Alert category to enable/disable
     * @param enabled true to enable, false to disable
     */
    void enable_alert_category(AlertCategory category, bool enabled);
    
    /**
     * @brief Check if alert category is enabled
     * @param category Alert category to check
     * @return true if enabled
     */
    bool is_alert_category_enabled(AlertCategory category) const;

private:
    AlertSystemConfig config_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> shutdown_requested_{false};
    
    // Alert management
    mutable std::mutex alerts_mutex_;
    std::vector<Alert> active_alerts_;
    std::vector<Alert> alert_history_;
    
    // Statistics tracking
    mutable std::mutex stats_mutex_;
    AlertStatistics statistics_;
    
    // Processing components - commented out for minimal TDD implementation
    // std::unique_ptr<AlertProcessor> processor_;
    // std::unique_ptr<AlertNotifier> notifier_;
    // std::unique_ptr<AlertAggregator> aggregator_;
    
    // Background processing
    std::unique_ptr<std::thread> processing_thread_;
    std::queue<std::function<void()>> processing_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_condition_;
    
    // Callback handling
    std::mutex callback_mutex_;
    AlertCallback alert_callback_;
    
    // Enabled categories
    std::map<AlertCategory, bool> enabled_categories_;
    
    // Internal processing methods
    void processing_loop();
    void add_alert(Alert&& alert);
    void update_statistics(const Alert& alert);
    void check_etsi_compliance_thresholds(const compliance::ComplianceResult& result);
    void check_audio_quality_thresholds(const std::vector<float>& audio_data,
                                       uint32_t sample_rate, uint32_t channels,
                                       uint32_t service_id);
    void check_service_availability_thresholds(const eti::EtiFrame& frame,
                                              const eti::Ensemble& ensemble);
    void check_error_correction_thresholds(const eti::EtiFrame& frame);
    void check_synchronization_thresholds(const eti::EtiFrame& frame);
    
    // COMPLIANCE OPTIMIZATION: Error correction helper methods for 100% compliance
    bool attempt_fic_error_correction(const eti::EtiFrame& frame);
    ErrorCorrectionResult attempt_msc_error_correction(const eti::EtiFrame& frame);
    
    // Audio analysis methods
    double calculate_lufs_loudness(const std::vector<float>& audio_data,
                                 uint32_t sample_rate, uint32_t channels);
    double calculate_peak_level(const std::vector<float>& audio_data);
    bool detect_silence(const std::vector<float>& audio_data, double threshold = -60.0);
    bool detect_overmodulation(const std::vector<float>& audio_data);
    
    // Utility methods
    void notify_alert_generated(const Alert& alert);
    static std::string get_severity_name(AlertSeverity severity);
    static std::string get_category_name(AlertCategory category);
    static std::string format_alert_summary(const Alert& alert);
};

/**
 * @brief Utility functions for alert system
 */
namespace utils {
    /**
     * @brief Convert alert to human-readable string
     * @param alert Alert to format
     * @return Formatted string representation
     */
    std::string format_alert(const Alert& alert);
    
    /**
     * @brief Generate alert dashboard summary
     * @param alerts Vector of alerts to summarize
     * @return Dashboard summary string
     */
    std::string generate_alert_dashboard(const std::vector<Alert>& alerts);
    
    /**
     * @brief Check if alert severity requires immediate attention
     * @param severity Alert severity to check
     * @return true if immediate attention required
     */
    bool requires_immediate_attention(AlertSeverity severity);
    
    /**
     * @brief Calculate alert priority score for sorting
     * @param alert Alert to score
     * @return Priority score (higher = more urgent)
     */
    uint32_t calculate_alert_priority(const Alert& alert);
    
    /**
     * @brief Export alerts to CSV format
     * @param alerts Vector of alerts to export
     * @return CSV formatted string
     */
    std::string export_alerts_to_csv(const std::vector<Alert>& alerts);
    
    /**
     * @brief Export alerts to JSON format
     * @param alerts Vector of alerts to export
     * @return JSON formatted string
     */
    std::string export_alerts_to_json(const std::vector<Alert>& alerts);
}

} // namespace alerts
} // namespace etsi

// Backward compatibility wrapper for legacy GUI components
class AlertSystem {
public:
    using AlertLevel = etsi::alerts::AlertSeverity;
    using AlertType = etsi::alerts::AlertCategory;
    
    explicit AlertSystem(const etsi::alerts::AlertSystemConfig& config = {})
        : impl_(std::make_unique<etsi::alerts::EtsiAlertSystem>(config)) {}
    
    bool initialize() { return impl_->initialize(); }
    void shutdown() { impl_->shutdown(); }
    
    std::vector<etsi::alerts::Alert> get_active_alerts() const {
        return impl_->get_active_alerts();
    }
    
    bool acknowledge_alert(uint64_t alert_id) {
        return impl_->acknowledge_alert(alert_id);
    }
    
    void generate_emergency_alert(const std::string& title, const std::string& message) {
        impl_->generate_emergency_alert(title, message);
    }
    
private:
    std::unique_ptr<etsi::alerts::EtsiAlertSystem> impl_;
};

#endif // ETSI_ALERT_SYSTEM_H