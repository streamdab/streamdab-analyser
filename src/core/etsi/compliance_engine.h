/**
 * @file compliance_engine.h
 * @brief ETSI Standards Compliance Engine - Core Implementation
 * 
 * This file contains the main ETSI compliance engine that validates DAB/ETI streams
 * against all relevant ETSI standards including EN 300 401, EN 302 077, EN 300 799,
 * TS 102 563, and TS 101 756.
 * 
 * Provides real-time compliance monitoring, validation, and professional reporting
 * capabilities for broadcast industry applications.
 * 
 * @author Standards Compliance Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 * @version 1.0
 */

#ifndef ETSI_COMPLIANCE_ENGINE_H
#define ETSI_COMPLIANCE_ENGINE_H

#include "../eti_types.hpp"
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>

namespace etsi {
namespace compliance {

/**
 * @brief ETSI Standard identifiers for validation
 */
enum class EtsiStandard : uint8_t {
    EN_300_401 = 1,     // DAB Radio Broadcasting
    EN_302_077 = 2,     // DAB+ Audio Coding (HE-AAC v2)
    EN_300_799 = 3,     // ETI Distribution Interface
    TS_102_563 = 4,     // DAB+ Audio Encoding Guidelines
    TS_101_756 = 5      // Registered Tables for DAB
};

/**
 * @brief Compliance validation levels
 */
enum class ComplianceLevel : uint8_t {
    STRICT_ETSI = 3,        // Exact ETSI specification compliance (99.9% tolerance)
    BROADCAST_QUALITY = 2,  // Professional broadcast requirements (99.0% tolerance)
    BASIC_INTEROP = 1       // Basic interoperability (95.0% tolerance)
};

/**
 * @brief Validation severity levels for issues
 */
enum class ValidationSeverity : uint8_t {
    CRITICAL = 4,    // Standard violation, breaks compatibility
    ERROR = 3,       // Significant issue, may affect operation
    WARNING = 2,     // Minor issue, should be addressed
    INFO = 1         // Informational, no action required
};

/**
 * @brief Individual validation issue
 */
struct ValidationIssue {
    EtsiStandard standard;
    ValidationSeverity severity;
    std::string description;
    std::string location;           // Frame number, FIG type, etc.
    std::string recommendation;
    uint32_t frame_number;
    std::chrono::system_clock::time_point timestamp;
    
    ValidationIssue(EtsiStandard std, ValidationSeverity sev, 
                   const std::string& desc, const std::string& loc = "",
                   uint32_t frame_num = 0)
        : standard(std), severity(sev), description(desc), 
          location(loc), frame_number(frame_num),
          timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief Comprehensive compliance validation result
 */
struct ComplianceResult {
    bool is_compliant;
    ComplianceLevel achieved_level;
    std::map<EtsiStandard, bool> standard_compliance;
    std::vector<ValidationIssue> violations;
    std::vector<ValidationIssue> warnings;
    std::map<EtsiStandard, double> compliance_percentages;
    std::chrono::microseconds validation_time;
    uint32_t frames_analyzed;
    
    ComplianceResult() 
        : is_compliant(false), achieved_level(ComplianceLevel::BASIC_INTEROP),
          validation_time(0), frames_analyzed(0) {
        // Initialize all standards as non-compliant
        standard_compliance[EtsiStandard::EN_300_401] = false;
        standard_compliance[EtsiStandard::EN_302_077] = false;
        standard_compliance[EtsiStandard::EN_300_799] = false;
        standard_compliance[EtsiStandard::TS_102_563] = false;
        standard_compliance[EtsiStandard::TS_101_756] = false;
        
        // Initialize compliance percentages
        compliance_percentages[EtsiStandard::EN_300_401] = 0.0;
        compliance_percentages[EtsiStandard::EN_302_077] = 0.0;
        compliance_percentages[EtsiStandard::EN_300_799] = 0.0;
        compliance_percentages[EtsiStandard::TS_102_563] = 0.0;
        compliance_percentages[EtsiStandard::TS_101_756] = 0.0;
    }
    
    void add_violation(const ValidationIssue& issue) {
        if (issue.severity >= ValidationSeverity::ERROR) {
            violations.push_back(issue);
            is_compliant = false;
            standard_compliance[issue.standard] = false;
        } else {
            warnings.push_back(issue);
        }
    }
    
    void set_compliance_percentage(EtsiStandard standard, double percentage) {
        compliance_percentages[standard] = percentage;
        
        // Update standard compliance based on achieved level thresholds
        switch (achieved_level) {
            case ComplianceLevel::STRICT_ETSI:
                standard_compliance[standard] = (percentage >= 99.9);
                break;
            case ComplianceLevel::BROADCAST_QUALITY:
                standard_compliance[standard] = (percentage >= 99.0);
                break;
            case ComplianceLevel::BASIC_INTEROP:
                standard_compliance[standard] = (percentage >= 95.0);
                break;
        }
    }
    
    double get_overall_compliance() const {
        double total = 0.0;
        for (const auto& pair : compliance_percentages) {
            total += pair.second;
        }
        return total / compliance_percentages.size();
    }
    
    size_t get_critical_issues() const {
        return std::count_if(violations.begin(), violations.end(),
                           [](const ValidationIssue& issue) {
                               return issue.severity == ValidationSeverity::CRITICAL;
                           });
    }
};

// Forward declarations
class StandardValidator;
class EN300401Validator;
class EN302077Validator;
class EN300799Validator;
class TS102563Validator;
class TS101756Validator;

/**
 * @brief Main ETSI Compliance Engine
 * 
 * Central engine for validating ETI streams against ETSI standards.
 * Supports real-time validation with configurable compliance levels.
 * Thread-safe for concurrent validation operations.
 */
class EtsiComplianceEngine {
public:
    /**
     * @brief Engine configuration options
     */
    struct Config {
        ComplianceLevel target_level = ComplianceLevel::BROADCAST_QUALITY;
        bool enable_real_time_monitoring = true;
        bool enable_performance_monitoring = true;
        bool enable_cross_standard_validation = true;
        std::chrono::milliseconds validation_timeout{100}; // 100ms max per frame
        size_t max_validation_history = 10000; // Keep last 10k validations
        bool generate_detailed_reports = true;
        
        Config() = default;
    };
    
    EtsiComplianceEngine(); // Default constructor with default config
    explicit EtsiComplianceEngine(const Config& config);
    ~EtsiComplianceEngine();
    
    // Disable copy/move for thread safety
    EtsiComplianceEngine(const EtsiComplianceEngine&) = delete;
    EtsiComplianceEngine& operator=(const EtsiComplianceEngine&) = delete;
    EtsiComplianceEngine(EtsiComplianceEngine&&) = delete;
    EtsiComplianceEngine& operator=(EtsiComplianceEngine&&) = delete;
    
    /**
     * @brief Initialize the compliance engine
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Shutdown the compliance engine
     */
    void shutdown();
    
    /**
     * @brief Check if engine supports a specific ETSI standard
     * @param standard The ETSI standard to check
     * @return true if standard is supported
     */
    bool supports_standard(EtsiStandard standard) const;
    
    /**
     * @brief Validate a single ETI frame against all standards
     * @param frame The ETI frame to validate
     * @return Compliance validation result
     */
    ComplianceResult validate_eti_frame(const eti::EtiFrame& frame);
    
    /**
     * @brief Validate ensemble configuration against standards
     * @param ensemble The ensemble to validate
     * @return Compliance validation result
     */
    ComplianceResult validate_ensemble_configuration(const eti::Ensemble& ensemble);
    
    /**
     * @brief Validate DAB service against standards
     * @param service The DAB service to validate
     * @return Compliance validation result
     */
    ComplianceResult validate_service_configuration(const eti::DabService& service);
    
    /**
     * @brief Validate DAB+ audio data against standards
     * @param audio_data Raw audio data from MSC
     * @param sub_channel_info Associated sub-channel information
     * @return Compliance validation result
     */
    ComplianceResult validate_dabplus_audio(const std::vector<uint8_t>& audio_data,
                                          const eti::SubChannelInfo& sub_channel_info);
    
    /**
     * @brief Perform cross-standard consistency validation
     * @param frame ETI frame
     * @param ensemble Extracted ensemble information
     * @return Compliance validation result
     */
    ComplianceResult validate_cross_standard_consistency(const eti::EtiFrame& frame,
                                                       const eti::Ensemble& ensemble);
    
    /**
     * @brief Set the target compliance level
     * @param level Target compliance level
     */
    void set_compliance_level(ComplianceLevel level);
    
    /**
     * @brief Get current compliance level
     * @return Current compliance level
     */
    ComplianceLevel get_compliance_level() const;
    
    /**
     * @brief Enable/disable specific standard validation
     * @param standard ETSI standard to enable/disable
     * @param enabled true to enable, false to disable
     */
    void enable_standard_validation(EtsiStandard standard, bool enabled);
    
    /**
     * @brief Check if a standard validation is enabled
     * @param standard ETSI standard to check
     * @return true if enabled
     */
    bool is_standard_enabled(EtsiStandard standard) const;
    
    /**
     * @brief Get validation performance metrics
     */
    struct PerformanceMetrics {
        std::chrono::microseconds avg_validation_time;
        std::chrono::microseconds max_validation_time;
        std::chrono::microseconds min_validation_time;
        uint32_t validations_per_second;
        uint32_t total_validations;
        uint32_t successful_validations;
        uint32_t failed_validations;
        double success_rate;
        size_t memory_usage_mb;
    };
    
    PerformanceMetrics get_performance_metrics() const;
    
    /**
     * @brief Reset performance metrics
     */
    void reset_performance_metrics();
    
    /**
     * @brief Get validation history
     * @param count Maximum number of recent validations to return
     * @return Vector of recent compliance results
     */
    std::vector<ComplianceResult> get_validation_history(size_t count = 100) const;
    
    /**
     * @brief Clear validation history
     */
    void clear_validation_history();
    
    /**
     * @brief Register a callback for validation events
     * @param callback Function to call on validation completion
     */
    using ValidationCallback = std::function<void(const ComplianceResult&)>;
    void register_validation_callback(ValidationCallback callback);
    
    /**
     * @brief Unregister validation callback
     */
    void unregister_validation_callback();

private:
    Config config_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> shutdown_requested_{false};
    
    // Standard validators
    std::unique_ptr<EN300401Validator> en300401_validator_;
    std::unique_ptr<EN302077Validator> en302077_validator_;
    std::unique_ptr<EN300799Validator> en300799_validator_;
    std::unique_ptr<TS102563Validator> ts102563_validator_;
    std::unique_ptr<TS101756Validator> ts101756_validator_;
    
    // Enabled standards mask
    std::map<EtsiStandard, bool> enabled_standards_;
    
    // Performance tracking
    mutable std::mutex metrics_mutex_;
    PerformanceMetrics performance_metrics_;
    std::vector<std::chrono::microseconds> validation_times_;
    std::chrono::high_resolution_clock::time_point start_time_;
    
    // Validation history
    mutable std::mutex history_mutex_;
    std::vector<ComplianceResult> validation_history_;
    
    // Callback handling
    std::mutex callback_mutex_;
    ValidationCallback validation_callback_;
    
    // Internal methods
    void initialize_validators();
    void update_performance_metrics(const std::chrono::microseconds& validation_time,
                                  bool success);
    void add_to_history(const ComplianceResult& result);
    void notify_validation_complete(const ComplianceResult& result);
    
    // Standard-specific validation methods
    ComplianceResult validate_en300401(const eti::EtiFrame& frame, const eti::Ensemble& ensemble);
    ComplianceResult validate_en302077(const std::vector<uint8_t>& audio_data,
                                     const eti::SubChannelInfo& sub_channel_info);
    ComplianceResult validate_en300799(const eti::EtiFrame& frame);
    ComplianceResult validate_ts102563(const std::vector<uint8_t>& audio_data);
    ComplianceResult validate_ts101756(const eti::Ensemble& ensemble);
    
    // Utility methods
    static std::string get_standard_name(EtsiStandard standard);
    static std::string get_severity_name(ValidationSeverity severity);
    static std::string get_compliance_level_name(ComplianceLevel level);
};

/**
 * @brief Utility functions for ETSI compliance
 */
namespace utils {
    /**
     * @brief Convert compliance result to human-readable string
     * @param result Compliance result to format
     * @return Formatted string representation
     */
    std::string format_compliance_result(const ComplianceResult& result);
    
    /**
     * @brief Generate compliance summary statistics
     * @param results Vector of compliance results
     * @return Summary statistics string
     */
    std::string generate_compliance_summary(const std::vector<ComplianceResult>& results);
    
    /**
     * @brief Check if compliance level meets professional broadcast requirements
     * @param level Compliance level to check
     * @return true if meets broadcast requirements
     */
    bool meets_broadcast_requirements(ComplianceLevel level);
    
    /**
     * @brief Calculate compliance trend over time
     * @param results Historical compliance results
     * @return Trend indication (-1: declining, 0: stable, 1: improving)
     */
    int calculate_compliance_trend(const std::vector<ComplianceResult>& results);
}

} // namespace compliance
} // namespace etsi

#endif // ETSI_COMPLIANCE_ENGINE_H