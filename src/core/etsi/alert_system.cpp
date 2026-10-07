/**
 * @file alert_system.cpp
 * @brief ETSI Standards-Compliant Alert System - Minimal Implementation for TDD Foundation
 * 
 * This is a minimal clean implementation for the TDD cleanup phase.
 * Full ETSI compliance features will be implemented via TDD methodology.
 * 
 * @author Build Manager Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 */

#include "alert_system.h"
#include <sstream>

namespace etsi {
namespace alerts {

// EtsiAlertSystem Implementation
EtsiAlertSystem::EtsiAlertSystem(const AlertSystemConfig& config)
    : config_(config), initialized_(false), shutdown_requested_(false) {
}

EtsiAlertSystem::~EtsiAlertSystem() {
    if (initialized_.load()) {
        shutdown();
    }
}

bool EtsiAlertSystem::initialize() {
    bool expected = false;
    if (initialized_.compare_exchange_strong(expected, true)) {
        // Initialize components
        return true;
    }
    return false;
}

void EtsiAlertSystem::shutdown() {
    shutdown_requested_.store(true);
    initialized_.store(false);
}

void EtsiAlertSystem::process_compliance_result(const compliance::ComplianceResult& result) {
    // Minimal implementation - will be expanded via TDD
    (void)result; // Suppress unused parameter warning
}

void EtsiAlertSystem::process_eti_frame(const eti::EtiFrame& frame, const eti::Ensemble& ensemble) {
    // Minimal implementation - will be expanded via TDD
    (void)frame; (void)ensemble; // Suppress unused parameter warnings
}

void EtsiAlertSystem::process_audio_data(const std::vector<float>& audio_data,
                                        uint32_t sample_rate, uint32_t channels,
                                        uint32_t service_id) {
    // Minimal implementation - will be expanded via TDD
    (void)audio_data; (void)sample_rate; (void)channels; (void)service_id;
}

void EtsiAlertSystem::generate_emergency_alert(const std::string& title,
                                              const std::string& message,
                                              const std::string& location,
                                              const std::map<std::string, std::string>& metadata) {
    // Create basic emergency alert
    Alert alert(AlertSeverity::EMERGENCY, AlertCategory::EMERGENCY_ALERT, title, message);
    alert.location = location;
    alert.metadata = metadata;
    
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    active_alerts_.push_back(std::move(alert));
}

std::vector<Alert> EtsiAlertSystem::get_active_alerts() const {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    return active_alerts_;
}

std::vector<Alert> EtsiAlertSystem::get_alerts_by_category(AlertCategory category) const {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    std::vector<Alert> result;
    for (const auto& alert : active_alerts_) {
        if (alert.category == category) {
            result.push_back(alert);
        }
    }
    return result;
}

std::vector<Alert> EtsiAlertSystem::get_alerts_by_severity(AlertSeverity severity) const {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    std::vector<Alert> result;
    for (const auto& alert : active_alerts_) {
        if (alert.severity == severity) {
            result.push_back(alert);
        }
    }
    return result;
}

bool EtsiAlertSystem::acknowledge_alert(uint64_t alert_id) {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    for (auto& alert : active_alerts_) {
        if (alert.alert_id == alert_id && !alert.acknowledged) {
            alert.acknowledged = true;
            alert.acknowledged_time = std::chrono::system_clock::now();
            return true;
        }
    }
    return false;
}

bool EtsiAlertSystem::resolve_alert(uint64_t alert_id) {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    for (auto& alert : active_alerts_) {
        if (alert.alert_id == alert_id && !alert.resolved) {
            alert.resolved = true;
            alert.resolved_time = std::chrono::system_clock::now();
            return true;
        }
    }
    return false;
}

void EtsiAlertSystem::clear_old_alerts() {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    auto it = std::remove_if(active_alerts_.begin(), active_alerts_.end(),
                            [](const Alert& alert) {
                                return alert.acknowledged && alert.resolved;
                            });
    active_alerts_.erase(it, active_alerts_.end());
}

void EtsiAlertSystem::update_configuration(const AlertSystemConfig& config) {
    config_ = config;
}

EtsiAlertSystem::AlertStatistics EtsiAlertSystem::get_alert_statistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return statistics_;
}

void EtsiAlertSystem::register_alert_callback(AlertCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    alert_callback_ = std::move(callback);
}

void EtsiAlertSystem::unregister_alert_callback() {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    alert_callback_ = nullptr;
}

void EtsiAlertSystem::enable_alert_category(AlertCategory category, bool enabled) {
    enabled_categories_[category] = enabled;
}

bool EtsiAlertSystem::is_alert_category_enabled(AlertCategory category) const {
    auto it = enabled_categories_.find(category);
    return (it != enabled_categories_.end()) ? it->second : true;
}

// Utility namespace functions
namespace utils {

std::string format_alert(const Alert& alert) {
    std::ostringstream oss;
    oss << "Alert ID: " << alert.alert_id << "\n"
        << "Title: " << alert.title << "\n"
        << "Description: " << alert.description << "\n"
        << "Active: " << (alert.is_active() ? "Yes" : "No") << "\n";
    return oss.str();
}

std::string generate_alert_dashboard(const std::vector<Alert>& alerts) {
    std::ostringstream oss;
    oss << "Alert Dashboard Summary:\n";
    oss << "Total Alerts: " << alerts.size() << "\n";
    
    size_t active_count = 0;
    for (const auto& alert : alerts) {
        if (alert.is_active()) {
            active_count++;
        }
    }
    oss << "Active Alerts: " << active_count << "\n";
    return oss.str();
}

bool requires_immediate_attention(AlertSeverity severity) {
    return severity == AlertSeverity::EMERGENCY || severity == AlertSeverity::CRITICAL;
}

uint32_t calculate_alert_priority(const Alert& alert) {
    uint32_t priority = static_cast<uint32_t>(alert.severity) * 1000;
    if (alert.is_active()) {
        priority += 500;
    }
    return priority;
}

std::string export_alerts_to_csv(const std::vector<Alert>& alerts) {
    std::ostringstream oss;
    oss << "AlertID,Title,Description,Severity,Category\n";
    for (const auto& alert : alerts) {
        oss << alert.alert_id << ","
            << alert.title << ","
            << alert.description << ","
            << static_cast<int>(alert.severity) << ","
            << static_cast<int>(alert.category) << "\n";
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
            << "      \"title\": \"" << alert.title << "\",\n"
            << "      \"description\": \"" << alert.description << "\",\n"
            << "      \"severity\": " << static_cast<int>(alert.severity) << ",\n"
            << "      \"category\": " << static_cast<int>(alert.category) << "\n"
            << "    }";
        if (i < alerts.size() - 1) oss << ",";
        oss << "\n";
    }
    oss << "  ]\n}";
    return oss.str();
}

} // namespace utils

} // namespace alerts
} // namespace etsi