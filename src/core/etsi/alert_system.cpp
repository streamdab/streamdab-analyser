/**
 * @file alert_system.cpp
 * @brief ETSI Standards-Compliant Professional Alert System Implementation
 * 
 * Implementation of professional broadcast alert system with ETSI compliance
 * monitoring, audio quality validation, and emergency alert capabilities.
 * 
 * @author Standards Compliance Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyzer Team
 * @version 1.0
 */

#include "alert_system.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace etsi {
namespace alerts {

// Helper classes for alert processing
class AlertProcessor {
public:
    explicit AlertProcessor(const AlertSystemConfig& config) : config_(config) {}
    
    void process_compliance_violations(const compliance::ComplianceResult& result,
                                     std::vector<Alert>& alerts) {
        for (const auto& violation : result.violations) {
            AlertSeverity severity = map_validation_to_alert_severity(violation.severity);
            AlertCategory category = AlertCategory::ETSI_COMPLIANCE;
            
            Alert alert(severity, category, 
                       "ETSI Compliance Violation",
                       violation.description,
                       violation.standard);
            
            alert.location = violation.location;
            alert.recommendation = violation.recommendation;
            alert.metadata["frame_number"] = std::to_string(violation.frame_number);
            alert.metadata["standard"] = get_standard_name(violation.standard);
            
            alerts.push_back(std::move(alert));
        }
    }
    
    void process_audio_quality_issues(const std::vector<float>& audio_data,
                                    uint32_t sample_rate, uint32_t channels,
                                    uint32_t service_id, std::vector<Alert>& alerts) {
        // Calculate LUFS loudness
        double lufs = calculate_lufs_loudness(audio_data, sample_rate, channels);
        double lufs_deviation = std::abs(lufs - config_.audio_thresholds.lufs_target);
        
        if (lufs_deviation > config_.audio_thresholds.lufs_critical_deviation) {
            Alert alert(AlertSeverity::CRITICAL, AlertCategory::AUDIO_QUALITY,
                       "Critical Audio Loudness Deviation",
                       "Audio loudness deviates significantly from ITU-R BS.1770-4 target");
            alert.location = "Service " + std::to_string(service_id);
            alert.metadata["lufs_measured"] = std::to_string(lufs);
            alert.metadata["lufs_target"] = std::to_string(config_.audio_thresholds.lufs_target);
            alert.metadata["deviation"] = std::to_string(lufs_deviation);
            alerts.push_back(std::move(alert));
        }
        else if (lufs_deviation > config_.audio_thresholds.lufs_warning_deviation) {
            Alert alert(AlertSeverity::WARNING, AlertCategory::AUDIO_QUALITY,
                       "Audio Loudness Warning",
                       "Audio loudness outside recommended tolerance");
            alert.location = "Service " + std::to_string(service_id);
            alert.metadata["lufs_measured"] = std::to_string(lufs);
            alert.metadata["lufs_target"] = std::to_string(config_.audio_thresholds.lufs_target);
            alerts.push_back(std::move(alert));
        }
        
        // Check peak levels
        double peak_level = calculate_peak_level(audio_data);
        if (peak_level > config_.audio_thresholds.peak_level_critical) {
            Alert alert(AlertSeverity::CRITICAL, AlertCategory::AUDIO_QUALITY,
                       "Critical Audio Peak Level",
                       "Audio peak level exceeds safe broadcast limits");
            alert.location = "Service " + std::to_string(service_id);
            alert.metadata["peak_level_db"] = std::to_string(peak_level);
            alert.metadata["peak_threshold"] = std::to_string(config_.audio_thresholds.peak_level_critical);
            alerts.push_back(std::move(alert));
        }
        
        // Check for silence
        if (detect_silence(audio_data)) {
            Alert alert(AlertSeverity::WARNING, AlertCategory::AUDIO_QUALITY,
                       "Audio Silence Detected",
                       "Extended period of audio silence detected");
            alert.location = "Service " + std::to_string(service_id);
            alert.metadata["duration_estimate"] = "5+ seconds";
            alerts.push_back(std::move(alert));
        }
        
        // Check for overmodulation
        if (detect_overmodulation(audio_data)) {
            Alert alert(AlertSeverity::CRITICAL, AlertCategory::AUDIO_QUALITY,
                       "Audio Overmodulation Detected",
                       "Digital clipping/overmodulation detected in audio signal");
            alert.location = "Service " + std::to_string(service_id);
            alerts.push_back(std::move(alert));
        }
    }

private:
    const AlertSystemConfig& config_;
    
    AlertSeverity map_validation_to_alert_severity(compliance::ValidationSeverity severity) {
        switch (severity) {
            case compliance::ValidationSeverity::CRITICAL:
                return AlertSeverity::CRITICAL;
            case compliance::ValidationSeverity::ERROR:
                return AlertSeverity::WARNING;
            case compliance::ValidationSeverity::WARNING:
                return AlertSeverity::NOTICE;
            case compliance::ValidationSeverity::INFO:
                return AlertSeverity::INFO;
            default:
                return AlertSeverity::INFO;
        }
    }
    
    std::string get_standard_name(compliance::EtsiStandard standard) {
        switch (standard) {
            case compliance::EtsiStandard::EN_300_401: return "EN 300 401";
            case compliance::EtsiStandard::EN_302_077: return "EN 302 077";
            case compliance::EtsiStandard::EN_300_799: return "EN 300 799";
            case compliance::EtsiStandard::TS_102_563: return "TS 102 563";
            case compliance::EtsiStandard::TS_101_756: return "TS 101 756";
            default: return "Unknown";
        }
    }
    
    double calculate_lufs_loudness(const std::vector<float>& audio_data,
                                 uint32_t sample_rate, uint32_t channels) {
        // Simplified LUFS calculation - real implementation would use proper K-weighting
        if (audio_data.empty()) return -std::numeric_limits<double>::infinity();
        
        double sum_squares = 0.0;
        for (float sample : audio_data) {
            sum_squares += sample * sample;
        }
        
        double rms = std::sqrt(sum_squares / audio_data.size());
        double lufs = -0.691 + 10.0 * std::log10(rms * rms + 1e-10);
        
        return lufs;
    }
    
    double calculate_peak_level(const std::vector<float>& audio_data) {
        if (audio_data.empty()) return -std::numeric_limits<double>::infinity();
        
        float peak = *std::max_element(audio_data.begin(), audio_data.end(),
                                     [](float a, float b) {
                                         return std::abs(a) < std::abs(b);
                                     });
        
        return 20.0 * std::log10(std::abs(peak) + 1e-10);
    }
    
    bool detect_silence(const std::vector<float>& audio_data, double threshold_db = -60.0) {
        if (audio_data.empty()) return true;
        
        double threshold_linear = std::pow(10.0, threshold_db / 20.0);
        
        size_t silent_samples = 0;
        for (float sample : audio_data) {
            if (std::abs(sample) < threshold_linear) {
                silent_samples++;
            }
        }
        
        return (static_cast<double>(silent_samples) / audio_data.size()) > 0.95; // 95% silence
    }
    
    bool detect_overmodulation(const std::vector<float>& audio_data) {
        if (audio_data.empty()) return false;
        
        const float clip_threshold = 0.99f;
        size_t clipped_samples = 0;
        
        for (float sample : audio_data) {
            if (std::abs(sample) > clip_threshold) {
                clipped_samples++;
            }
        }
        
        return (static_cast<double>(clipped_samples) / audio_data.size()) > 0.01; // 1% clipping
    }
};

class AlertNotifier {
public:
    explicit AlertNotifier(EtsiAlertSystem::AlertCallback callback)
        : callback_(std::move(callback)) {}
    
    void notify(const Alert& alert) {
        if (callback_) {
            try {
                callback_(alert);
            } catch (const std::exception& e) {
                // Log notification error but don't throw
            }
        }
    }
    
private:
    EtsiAlertSystem::AlertCallback callback_;
};

class AlertAggregator {
public:
    explicit AlertAggregator(const AlertSystemConfig& config) : config_(config) {}
    
    std::vector<Alert> aggregate_similar_alerts(const std::vector<Alert>& alerts) {
        std::vector<Alert> aggregated;
        std::map<std::string, std::vector<Alert>> grouped_alerts;
        
        // Group similar alerts by title and category
        for (const auto& alert : alerts) {
            std::string key = std::to_string(static_cast<int>(alert.category)) + ":" + alert.title;
            grouped_alerts[key].push_back(alert);
        }
        
        // Aggregate groups with multiple alerts
        for (const auto& group : grouped_alerts) {
            if (group.second.size() == 1) {
                aggregated.push_back(group.second[0]);
            } else {
                // Create aggregated alert
                Alert aggregated_alert = group.second[0];
                aggregated_alert.description += " (+" + std::to_string(group.second.size() - 1) + " similar)";
                aggregated_alert.metadata["aggregated_count"] = std::to_string(group.second.size());
                aggregated.push_back(std::move(aggregated_alert));
            }
        }
        
        return aggregated;
    }
    
private:
    const AlertSystemConfig& config_;
};

// EtsiAlertSystem implementation
EtsiAlertSystem::EtsiAlertSystem(const AlertSystemConfig& config)
    : config_(config) {
    
    // Initialize enabled categories
    enabled_categories_[AlertCategory::ETSI_COMPLIANCE] = true;
    enabled_categories_[AlertCategory::AUDIO_QUALITY] = config_.enable_audio_monitoring;
    enabled_categories_[AlertCategory::SERVICE_AVAILABILITY] = config_.enable_service_monitoring;
    enabled_categories_[AlertCategory::ERROR_CORRECTION] = true;
    enabled_categories_[AlertCategory::SYNCHRONIZATION] = true;
    enabled_categories_[AlertCategory::EMERGENCY_ALERT] = config_.enable_emergency_alerts;
    enabled_categories_[AlertCategory::PERFORMANCE] = config_.enable_performance_monitoring;
    enabled_categories_[AlertCategory::REGULATORY] = config_.enable_regulatory_monitoring;
    
    // Initialize statistics
    statistics_ = {};
}

EtsiAlertSystem::~EtsiAlertSystem() {
    shutdown();
}

bool EtsiAlertSystem::initialize() {
    if (initialized_.load()) {
        return true;
    }
    
    try {
        // Initialize processing components
        processor_ = std::make_unique<AlertProcessor>(config_);
        notifier_ = std::make_unique<AlertNotifier>(alert_callback_);
        aggregator_ = std::make_unique<AlertAggregator>(config_);
        
        // Start background processing thread
        processing_thread_ = std::make_unique<std::thread>(&EtsiAlertSystem::processing_loop, this);
        
        initialized_.store(true);
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

void EtsiAlertSystem::shutdown() {
    if (!initialized_.load()) {
        return;
    }
    
    shutdown_requested_.store(true);
    
    // Notify processing thread to wake up and exit
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_condition_.notify_all();
    }
    
    // Wait for processing thread to finish
    if (processing_thread_ && processing_thread_->joinable()) {
        processing_thread_->join();
    }
    
    initialized_.store(false);
}

void EtsiAlertSystem::process_compliance_result(const compliance::ComplianceResult& result) {
    if (!initialized_.load()) return;
    
    std::lock_guard<std::mutex> lock(queue_mutex_);
    processing_queue_.push([this, result]() {
        check_etsi_compliance_thresholds(result);
    });
    queue_condition_.notify_one();
}

void EtsiAlertSystem::process_eti_frame(const EtiFrame& frame, const Ensemble& ensemble) {
    if (!initialized_.load()) return;
    
    std::lock_guard<std::mutex> lock(queue_mutex_);
    processing_queue_.push([this, frame, ensemble]() {
        check_service_availability_thresholds(frame, ensemble);
        check_error_correction_thresholds(frame);
        check_synchronization_thresholds(frame);
    });
    queue_condition_.notify_one();
}

void EtsiAlertSystem::process_audio_data(const std::vector<float>& audio_data,
                                       uint32_t sample_rate, uint32_t channels,
                                       uint32_t service_id) {
    if (!initialized_.load() || !enabled_categories_[AlertCategory::AUDIO_QUALITY]) return;
    
    std::lock_guard<std::mutex> lock(queue_mutex_);
    processing_queue_.push([this, audio_data, sample_rate, channels, service_id]() {
        check_audio_quality_thresholds(audio_data, sample_rate, channels, service_id);
    });
    queue_condition_.notify_one();
}

void EtsiAlertSystem::generate_emergency_alert(const std::string& title,
                                             const std::string& message,
                                             const std::string& location,
                                             const std::map<std::string, std::string>& metadata) {
    if (!enabled_categories_[AlertCategory::EMERGENCY_ALERT]) return;
    
    Alert alert(AlertSeverity::EMERGENCY, AlertCategory::EMERGENCY_ALERT, title, message);
    alert.location = location;
    alert.metadata = metadata;
    alert.metadata["emergency"] = "true";
    
    add_alert(std::move(alert));
}

std::vector<Alert> EtsiAlertSystem::get_active_alerts() const {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    std::vector<Alert> active;
    std::copy_if(active_alerts_.begin(), active_alerts_.end(),
                std::back_inserter(active),
                [](const Alert& alert) { return alert.is_active(); });
    return active;
}

std::vector<Alert> EtsiAlertSystem::get_alerts_by_category(AlertCategory category) const {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    std::vector<Alert> filtered;
    std::copy_if(active_alerts_.begin(), active_alerts_.end(),
                std::back_inserter(filtered),
                [category](const Alert& alert) { 
                    return alert.category == category && alert.is_active(); 
                });
    return filtered;
}

std::vector<Alert> EtsiAlertSystem::get_alerts_by_severity(AlertSeverity severity) const {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    std::vector<Alert> filtered;
    std::copy_if(active_alerts_.begin(), active_alerts_.end(),
                std::back_inserter(filtered),
                [severity](const Alert& alert) { 
                    return alert.severity == severity && alert.is_active(); 
                });
    return filtered;
}

bool EtsiAlertSystem::acknowledge_alert(uint64_t alert_id) {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    auto it = std::find_if(active_alerts_.begin(), active_alerts_.end(),
                          [alert_id](Alert& alert) { return alert.alert_id == alert_id; });
    
    if (it != active_alerts_.end() && !it->acknowledged) {
        it->acknowledged = true;
        it->acknowledged_time = std::chrono::system_clock::now();
        return true;
    }
    return false;
}

bool EtsiAlertSystem::resolve_alert(uint64_t alert_id) {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    auto it = std::find_if(active_alerts_.begin(), active_alerts_.end(),
                          [alert_id](Alert& alert) { return alert.alert_id == alert_id; });
    
    if (it != active_alerts_.end() && !it->resolved) {
        it->resolved = true;
        it->resolved_time = std::chrono::system_clock::now();
        return true;
    }
    return false;
}

void EtsiAlertSystem::clear_old_alerts() {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    
    // Move resolved and acknowledged alerts to history
    auto partition_point = std::stable_partition(active_alerts_.begin(), active_alerts_.end(),
                                               [](const Alert& alert) { return alert.is_active(); });
    
    // Add cleared alerts to history
    alert_history_.insert(alert_history_.end(), partition_point, active_alerts_.end());
    
    // Remove from active alerts
    active_alerts_.erase(partition_point, active_alerts_.end());
    
    // Limit history size
    if (alert_history_.size() > config_.max_alert_history) {
        alert_history_.erase(alert_history_.begin(),
                           alert_history_.begin() + (alert_history_.size() - config_.max_alert_history));
    }
}

void EtsiAlertSystem::update_configuration(const AlertSystemConfig& config) {
    config_ = config;
    
    // Update enabled categories
    enabled_categories_[AlertCategory::AUDIO_QUALITY] = config_.enable_audio_monitoring;
    enabled_categories_[AlertCategory::SERVICE_AVAILABILITY] = config_.enable_service_monitoring;
    enabled_categories_[AlertCategory::EMERGENCY_ALERT] = config_.enable_emergency_alerts;
    enabled_categories_[AlertCategory::PERFORMANCE] = config_.enable_performance_monitoring;
    enabled_categories_[AlertCategory::REGULATORY] = config_.enable_regulatory_monitoring;
    
    // Update processor configuration
    if (processor_) {
        processor_ = std::make_unique<AlertProcessor>(config_);
    }
}

EtsiAlertSystem::AlertStatistics EtsiAlertSystem::get_alert_statistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return statistics_;
}

void EtsiAlertSystem::register_alert_callback(AlertCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    alert_callback_ = std::move(callback);
    
    // Update notifier with new callback
    if (notifier_) {
        notifier_ = std::make_unique<AlertNotifier>(alert_callback_);
    }
}

void EtsiAlertSystem::unregister_alert_callback() {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    alert_callback_ = nullptr;
    
    if (notifier_) {
        notifier_ = std::make_unique<AlertNotifier>(nullptr);
    }
}

void EtsiAlertSystem::enable_alert_category(AlertCategory category, bool enabled) {
    enabled_categories_[category] = enabled;
}

bool EtsiAlertSystem::is_alert_category_enabled(AlertCategory category) const {
    auto it = enabled_categories_.find(category);
    return it != enabled_categories_.end() ? it->second : false;
}

// Private implementation methods
void EtsiAlertSystem::processing_loop() {
    while (!shutdown_requested_.load()) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        // Wait for work or shutdown
        queue_condition_.wait(lock, [this] {
            return !processing_queue_.empty() || shutdown_requested_.load();
        });
        
        if (shutdown_requested_.load()) {
            break;
        }
        
        // Process all queued tasks
        while (!processing_queue_.empty()) {
            auto task = std::move(processing_queue_.front());
            processing_queue_.pop();
            lock.unlock();
            
            try {
                task();
            } catch (const std::exception& e) {
                // Log error but continue processing
            }
            
            lock.lock();
        }
    }
}

void EtsiAlertSystem::add_alert(Alert&& alert) {
    {
        std::lock_guard<std::mutex> lock(alerts_mutex_);
        
        // Check if we have too many active alerts
        if (active_alerts_.size() >= config_.max_active_alerts) {
            clear_old_alerts();
        }
        
        active_alerts_.push_back(std::move(alert));
    }
    
    update_statistics(active_alerts_.back());
    notify_alert_generated(active_alerts_.back());
}

void EtsiAlertSystem::update_statistics(const Alert& alert) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    statistics_.total_alerts_generated++;
    statistics_.active_alerts = static_cast<uint32_t>(get_active_alerts().size());
    statistics_.alerts_by_severity[alert.severity]++;
    statistics_.alerts_by_category[alert.category]++;
    statistics_.last_alert_time = alert.timestamp;
}

void EtsiAlertSystem::check_etsi_compliance_thresholds(const compliance::ComplianceResult& result) {
    if (!enabled_categories_[AlertCategory::ETSI_COMPLIANCE]) return;
    
    std::vector<Alert> new_alerts;
    if (processor_) {
        processor_->process_compliance_violations(result, new_alerts);
    }
    
    for (auto& alert : new_alerts) {
        add_alert(std::move(alert));
    }
}

void EtsiAlertSystem::check_audio_quality_thresholds(const std::vector<float>& audio_data,
                                                    uint32_t sample_rate, uint32_t channels,
                                                    uint32_t service_id) {
    if (!enabled_categories_[AlertCategory::AUDIO_QUALITY]) return;
    
    std::vector<Alert> new_alerts;
    if (processor_) {
        processor_->process_audio_quality_issues(audio_data, sample_rate, channels, service_id, new_alerts);
    }
    
    for (auto& alert : new_alerts) {
        add_alert(std::move(alert));
    }
}

void EtsiAlertSystem::check_service_availability_thresholds(const EtiFrame& frame,
                                                          const Ensemble& ensemble) {
    if (!enabled_categories_[AlertCategory::SERVICE_AVAILABILITY]) return;
    
    // Implementation would check service availability metrics
    // This is a placeholder for the actual implementation
}

void EtsiAlertSystem::check_error_correction_thresholds(const EtiFrame& frame) {
    if (!enabled_categories_[AlertCategory::ERROR_CORRECTION]) return;
    
    // Check Reed-Solomon error correction status
    if (frame.error_flags & ETI_ERROR_FLAG_FIC_CRC_ERROR) {
        Alert alert(AlertSeverity::WARNING, AlertCategory::ERROR_CORRECTION,
                   "FIC CRC Error Detected",
                   "FIC (Fast Information Channel) CRC error detected");
        alert.location = "Frame " + std::to_string(frame.frame_number);
        alert.metadata["error_type"] = "FIC_CRC";
        add_alert(std::move(alert));
    }
    
    if (frame.error_flags & ETI_ERROR_FLAG_MSC_CRC_ERROR) {
        Alert alert(AlertSeverity::CRITICAL, AlertCategory::ERROR_CORRECTION,
                   "MSC CRC Error Detected", 
                   "MSC (Main Service Channel) CRC error detected");
        alert.location = "Frame " + std::to_string(frame.frame_number);
        alert.metadata["error_type"] = "MSC_CRC";
        add_alert(std::move(alert));
    }
}

void EtsiAlertSystem::check_synchronization_thresholds(const EtiFrame& frame) {
    if (!enabled_categories_[AlertCategory::SYNCHRONIZATION]) return;
    
    // Check frame synchronization
    if (frame.error_flags & ETI_ERROR_FLAG_SYNC_LOST) {
        Alert alert(AlertSeverity::CRITICAL, AlertCategory::SYNCHRONIZATION,
                   "Frame Synchronization Lost",
                   "ETI frame synchronization has been lost");
        alert.location = "Frame " + std::to_string(frame.frame_number);
        add_alert(std::move(alert));
    }
}

void EtsiAlertSystem::notify_alert_generated(const Alert& alert) {
    if (notifier_) {
        notifier_->notify(alert);
    }
}

std::string EtsiAlertSystem::get_severity_name(AlertSeverity severity) {
    switch (severity) {
        case AlertSeverity::EMERGENCY: return "EMERGENCY";
        case AlertSeverity::CRITICAL: return "CRITICAL";
        case AlertSeverity::WARNING: return "WARNING";
        case AlertSeverity::NOTICE: return "NOTICE";
        case AlertSeverity::INFO: return "INFO";
        default: return "UNKNOWN";
    }
}

std::string EtsiAlertSystem::get_category_name(AlertCategory category) {
    switch (category) {
        case AlertCategory::ETSI_COMPLIANCE: return "ETSI Compliance";
        case AlertCategory::AUDIO_QUALITY: return "Audio Quality";
        case AlertCategory::SERVICE_AVAILABILITY: return "Service Availability";
        case AlertCategory::ERROR_CORRECTION: return "Error Correction";
        case AlertCategory::SYNCHRONIZATION: return "Synchronization";
        case AlertCategory::EMERGENCY_ALERT: return "Emergency Alert";
        case AlertCategory::PERFORMANCE: return "Performance";
        case AlertCategory::REGULATORY: return "Regulatory";
        default: return "Unknown";
    }
}

std::string EtsiAlertSystem::format_alert_summary(const Alert& alert) {
    std::ostringstream oss;
    oss << "[" << get_severity_name(alert.severity) << "] "
        << get_category_name(alert.category) << ": "
        << alert.title;
    return oss.str();
}

// Utility function implementations
namespace utils {

std::string format_alert(const Alert& alert) {
    std::ostringstream oss;
    oss << "Alert ID: " << alert.alert_id << "\n"
        << "Severity: " << EtsiAlertSystem::get_severity_name(alert.severity) << "\n"
        << "Category: " << EtsiAlertSystem::get_category_name(alert.category) << "\n"
        << "Title: " << alert.title << "\n"
        << "Description: " << alert.description << "\n"
        << "Location: " << alert.location << "\n"
        << "Timestamp: " << std::put_time(std::localtime(&(std::chrono::system_clock::to_time_t(alert.timestamp))), "%Y-%m-%d %H:%M:%S") << "\n"
        << "Status: " << (alert.is_active() ? "Active" : (alert.resolved ? "Resolved" : "Acknowledged")) << "\n";
    
    if (!alert.metadata.empty()) {
        oss << "Metadata:\n";
        for (const auto& pair : alert.metadata) {
            oss << "  " << pair.first << ": " << pair.second << "\n";
        }
    }
    
    return oss.str();
}

std::string generate_alert_dashboard(const std::vector<Alert>& alerts) {
    std::ostringstream oss;
    
    // Count alerts by severity
    std::map<AlertSeverity, int> severity_counts;
    std::map<AlertCategory, int> category_counts;
    
    for (const auto& alert : alerts) {
        if (alert.is_active()) {
            severity_counts[alert.severity]++;
            category_counts[alert.category]++;
        }
    }
    
    oss << "=== ALERT DASHBOARD ===\n";
    oss << "Total Active Alerts: " << alerts.size() << "\n\n";
    
    oss << "By Severity:\n";
    for (const auto& pair : severity_counts) {
        oss << "  " << EtsiAlertSystem::get_severity_name(pair.first) << ": " << pair.second << "\n";
    }
    
    oss << "\nBy Category:\n";
    for (const auto& pair : category_counts) {
        oss << "  " << EtsiAlertSystem::get_category_name(pair.first) << ": " << pair.second << "\n";
    }
    
    return oss.str();
}

bool requires_immediate_attention(AlertSeverity severity) {
    return severity >= AlertSeverity::CRITICAL;
}

uint32_t calculate_alert_priority(const Alert& alert) {
    uint32_t base_priority = static_cast<uint32_t>(alert.severity) * 1000;
    
    // Add urgency based on age
    auto age_seconds = alert.get_age().count();
    if (age_seconds > 300) { // 5 minutes
        base_priority += 100;
    }
    
    // Higher priority for certain categories
    if (alert.category == AlertCategory::EMERGENCY_ALERT) {
        base_priority += 500;
    } else if (alert.category == AlertCategory::ETSI_COMPLIANCE) {
        base_priority += 200;
    }
    
    return base_priority;
}

std::string export_alerts_to_csv(const std::vector<Alert>& alerts) {
    std::ostringstream oss;
    oss << "Alert ID,Severity,Category,Title,Description,Location,Timestamp,Status\n";
    
    for (const auto& alert : alerts) {
        oss << alert.alert_id << ","
            << EtsiAlertSystem::get_severity_name(alert.severity) << ","
            << EtsiAlertSystem::get_category_name(alert.category) << ","
            << "\"" << alert.title << "\","
            << "\"" << alert.description << "\","
            << "\"" << alert.location << "\","
            << std::put_time(std::localtime(&(std::chrono::system_clock::to_time_t(alert.timestamp))), "%Y-%m-%d %H:%M:%S") << ","
            << (alert.is_active() ? "Active" : (alert.resolved ? "Resolved" : "Acknowledged")) << "\n";
    }
    
    return oss.str();
}

std::string export_alerts_to_json(const std::vector<Alert>& alerts) {
    std::ostringstream oss;
    oss << "{\n  \"alerts\": [\n";
    
    for (size_t i = 0; i < alerts.size(); ++i) {
        const auto& alert = alerts[i];
        oss << "    {\n"
            << "      \"id\": " << alert.alert_id << ",\n"
            << "      \"severity\": \"" << EtsiAlertSystem::get_severity_name(alert.severity) << "\",\n"
            << "      \"category\": \"" << EtsiAlertSystem::get_category_name(alert.category) << "\",\n"
            << "      \"title\": \"" << alert.title << "\",\n"
            << "      \"description\": \"" << alert.description << "\",\n"
            << "      \"location\": \"" << alert.location << "\",\n"
            << "      \"timestamp\": \"" << std::put_time(std::localtime(&(std::chrono::system_clock::to_time_t(alert.timestamp))), "%Y-%m-%d %H:%M:%S") << "\",\n"
            << "      \"status\": \"" << (alert.is_active() ? "Active" : (alert.resolved ? "Resolved" : "Acknowledged")) << "\"\n"
            << "    }";
        
        if (i < alerts.size() - 1) {
            oss << ",";
        }
        oss << "\n";
    }
    
    oss << "  ]\n}";
    return oss.str();
}

} // namespace utils

} // namespace alerts
} // namespace etsi