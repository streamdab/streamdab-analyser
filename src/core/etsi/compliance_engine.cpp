/**
 * @file compliance_engine.cpp
 * @brief ETSI Standards Compliance Engine - Core Implementation
 * 
 * Implementation of the main ETSI compliance engine for validating DAB/ETI streams
 * against all relevant ETSI standards with real-time monitoring capabilities.
 * 
 * @author Standards Compliance Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 * @version 1.0
 */

#include "compliance_engine.h"
#include "standard_validators.h"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>

namespace etsi {
namespace compliance {

EtsiComplianceEngine::EtsiComplianceEngine(const Config& config) 
    : config_(config), start_time_(std::chrono::high_resolution_clock::now()) {
    
    // Initialize enabled standards (all enabled by default)
    enabled_standards_[EtsiStandard::EN_300_401] = true;
    enabled_standards_[EtsiStandard::EN_302_077] = true;
    enabled_standards_[EtsiStandard::EN_300_799] = true;
    enabled_standards_[EtsiStandard::TS_102_563] = true;
    enabled_standards_[EtsiStandard::TS_101_756] = true;
    
    // Initialize performance metrics
    performance_metrics_ = {};
    performance_metrics_.avg_validation_time = std::chrono::microseconds(0);
    performance_metrics_.max_validation_time = std::chrono::microseconds(0);
    performance_metrics_.min_validation_time = std::chrono::microseconds(std::numeric_limits<long>::max());
    validation_times_.reserve(1000); // Pre-allocate for performance
}

EtsiComplianceEngine::~EtsiComplianceEngine() {
    if (initialized_) {
        shutdown();
    }
}

bool EtsiComplianceEngine::initialize() {
    if (initialized_) {
        return true;
    }
    
    try {
        initialize_validators();
        
        // Reset metrics
        reset_performance_metrics();
        
        initialized_ = true;
        shutdown_requested_ = false;
        
        return true;
    } catch (const std::exception& e) {
        // Log initialization error
        initialized_ = false;
        return false;
    }
}

void EtsiComplianceEngine::shutdown() {
    if (!initialized_) {
        return;
    }
    
    shutdown_requested_ = true;
    
    // Clean up validators
    en300401_validator_.reset();
    en302077_validator_.reset();
    en300799_validator_.reset();
    ts102563_validator_.reset();
    ts101756_validator_.reset();
    
    // Clear validation callback
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        validation_callback_ = nullptr;
    }
    
    initialized_ = false;
}

void EtsiComplianceEngine::initialize_validators() {
    en300401_validator_ = std::make_unique<EN300401Validator>();
    en302077_validator_ = std::make_unique<EN302077Validator>();
    en300799_validator_ = std::make_unique<EN300799Validator>();
    ts102563_validator_ = std::make_unique<TS102563Validator>();
    ts101756_validator_ = std::make_unique<TS101756Validator>();
}

bool EtsiComplianceEngine::supports_standard(EtsiStandard standard) const {
    switch (standard) {
        case EtsiStandard::EN_300_401:
        case EtsiStandard::EN_302_077:
        case EtsiStandard::EN_300_799:
        case EtsiStandard::TS_102_563:
        case EtsiStandard::TS_101_756:
            return true;
        default:
            return false;
    }
}

ComplianceResult EtsiComplianceEngine::validate_eti_frame(const EtiFrame& frame) {
    if (!initialized_) {
        ComplianceResult result;
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_799,
            ValidationSeverity::CRITICAL,
            "Compliance engine not initialized"
        ));
        return result;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    ComplianceResult combined_result;
    combined_result.achieved_level = config_.target_level;
    combined_result.frames_analyzed = 1;
    
    try {
        // Extract ensemble information for comprehensive validation
        Ensemble ensemble = frame.extract_ensemble_info();
        
        // Validate against EN 300 799 (ETI Distribution Interface)
        if (enabled_standards_[EtsiStandard::EN_300_799]) {
            auto eti_result = validate_en300799(frame);
            merge_compliance_results(combined_result, eti_result);
        }
        
        // Validate against EN 300 401 (DAB Radio Broadcasting)
        if (enabled_standards_[EtsiStandard::EN_300_401]) {
            auto dab_result = validate_en300401(frame, ensemble);
            merge_compliance_results(combined_result, dab_result);
        }
        
        // Validate against TS 101 756 (Registered Tables)
        if (enabled_standards_[EtsiStandard::TS_101_756]) {
            auto tables_result = validate_ts101756(ensemble);
            merge_compliance_results(combined_result, tables_result);
        }
        
        // Extract and validate DAB+ audio if present
        if (enabled_standards_[EtsiStandard::EN_302_077] || 
            enabled_standards_[EtsiStandard::TS_102_563]) {
            
            for (const auto& service : ensemble.services) {
                for (const auto& component : service.components) {
                    if (component.component_type == 0x3F) { // DAB+ audio
                        auto subchannel_it = std::find_if(
                            ensemble.sub_channels.begin(),
                            ensemble.sub_channels.end(),
                            [&component](const SubChannelInfo& sc) {
                                return sc.sub_channel_id == component.sub_channel_id;
                            });
                        
                        if (subchannel_it != ensemble.sub_channels.end()) {
                            auto msc = frame.get_msc_field();
                            auto audio_data = msc.extract_subchannel_data(*subchannel_it);
                            
                            if (enabled_standards_[EtsiStandard::EN_302_077]) {
                                auto audio_result = validate_en302077(audio_data, *subchannel_it);
                                merge_compliance_results(combined_result, audio_result);
                            }
                            
                            if (enabled_standards_[EtsiStandard::TS_102_563]) {
                                auto guidelines_result = validate_ts102563(audio_data);
                                merge_compliance_results(combined_result, guidelines_result);
                            }
                        }
                    }
                }
            }
        }
        
        // Perform cross-standard consistency validation
        if (config_.enable_cross_standard_validation) {
            auto consistency_result = validate_cross_standard_consistency(frame, ensemble);
            merge_compliance_results(combined_result, consistency_result);
        }
        
        // Finalize compliance assessment
        finalize_compliance_result(combined_result);
        
    } catch (const std::exception& e) {
        combined_result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_799,
            ValidationSeverity::CRITICAL,
            std::string("Validation exception: ") + e.what()
        ));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto validation_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    combined_result.validation_time = validation_time;
    
    // Update performance metrics
    update_performance_metrics(validation_time, combined_result.is_compliant);
    
    // Add to history
    add_to_history(combined_result);
    
    // Notify callback if registered
    notify_validation_complete(combined_result);
    
    return combined_result;
}

ComplianceResult EtsiComplianceEngine::validate_ensemble_configuration(const Ensemble& ensemble) {
    if (!initialized_) {
        ComplianceResult result;
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_401,
            ValidationSeverity::CRITICAL,
            "Compliance engine not initialized"
        ));
        return result;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    ComplianceResult combined_result;
    combined_result.achieved_level = config_.target_level;
    combined_result.frames_analyzed = 0; // Ensemble-level validation
    
    try {
        // Validate ensemble structure against EN 300 401
        if (enabled_standards_[EtsiStandard::EN_300_401]) {
            validate_ensemble_structure(ensemble, combined_result);
        }
        
        // Validate registered tables compliance
        if (enabled_standards_[EtsiStandard::TS_101_756]) {
            validate_registered_tables_compliance(ensemble, combined_result);
        }
        
        // Validate service organization
        validate_service_organization(ensemble, combined_result);
        
        // Finalize result
        finalize_compliance_result(combined_result);
        
    } catch (const std::exception& e) {
        combined_result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_401,
            ValidationSeverity::CRITICAL,
            std::string("Ensemble validation exception: ") + e.what()
        ));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto validation_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    combined_result.validation_time = validation_time;
    
    update_performance_metrics(validation_time, combined_result.is_compliant);
    add_to_history(combined_result);
    notify_validation_complete(combined_result);
    
    return combined_result;
}

ComplianceResult EtsiComplianceEngine::validate_service_configuration(const DabService& service) {
    ComplianceResult result;
    result.achieved_level = config_.target_level;
    result.frames_analyzed = 0;
    
    try {
        // Validate service ID
        if (!service.validate_service_id()) {
            result.add_violation(ValidationIssue(
                EtsiStandard::EN_300_401,
                ValidationSeverity::ERROR,
                "Invalid service ID: " + std::to_string(service.service_id),
                "Service validation"
            ));
        }
        
        // Validate country code
        if (!service.validate_country_code()) {
            result.add_violation(ValidationIssue(
                EtsiStandard::TS_101_756,
                ValidationSeverity::ERROR,
                "Invalid country code: " + std::to_string(service.country_id),
                "Service validation"
            ));
        }
        
        // Validate service has primary component
        if (!service.has_primary_component()) {
            result.add_violation(ValidationIssue(
                EtsiStandard::EN_300_401,
                ValidationSeverity::WARNING,
                "Service has no primary component",
                "Service ID: " + std::to_string(service.service_id)
            ));
        }
        
        // Validate service label (EN 300 401 - max 16 characters)
        if (service.label.length() > 16) {
            result.add_violation(ValidationIssue(
                EtsiStandard::EN_300_401,
                ValidationSeverity::ERROR,
                "Service label exceeds 16 characters: " + std::to_string(service.label.length()),
                "Service: " + service.label
            ));
        }
        
        // Validate service components
        for (const auto& component : service.components) {
            if (!component.validate_sub_channel_id()) {
                result.add_violation(ValidationIssue(
                    EtsiStandard::EN_300_401,
                    ValidationSeverity::ERROR,
                    "Invalid sub-channel ID in service component: " + std::to_string(component.sub_channel_id),
                    "Service: " + service.label
                ));
            }
            
            if (!component.validate_tmid()) {
                result.add_violation(ValidationIssue(
                    EtsiStandard::EN_300_401,
                    ValidationSeverity::ERROR,
                    "Invalid TMID in service component: " + std::to_string(component.tmid),
                    "Service: " + service.label
                ));
            }
        }
        
        finalize_compliance_result(result);
        
    } catch (const std::exception& e) {
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_401,
            ValidationSeverity::CRITICAL,
            std::string("Service validation exception: ") + e.what()
        ));
    }
    
    return result;
}

ComplianceResult EtsiComplianceEngine::validate_dabplus_audio(const std::vector<uint8_t>& audio_data,
                                                            const SubChannelInfo& sub_channel_info) {
    if (!initialized_) {
        ComplianceResult result;
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_302_077,
            ValidationSeverity::CRITICAL,
            "Compliance engine not initialized"
        ));
        return result;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    ComplianceResult combined_result;
    combined_result.achieved_level = config_.target_level;
    combined_result.frames_analyzed = 1;
    
    try {
        // Validate against EN 302 077 (DAB+ Audio Coding)
        if (enabled_standards_[EtsiStandard::EN_302_077]) {
            auto audio_result = validate_en302077(audio_data, sub_channel_info);
            merge_compliance_results(combined_result, audio_result);
        }
        
        // Validate against TS 102 563 (DAB+ Guidelines)
        if (enabled_standards_[EtsiStandard::TS_102_563]) {
            auto guidelines_result = validate_ts102563(audio_data);
            merge_compliance_results(combined_result, guidelines_result);
        }
        
        finalize_compliance_result(combined_result);
        
    } catch (const std::exception& e) {
        combined_result.add_violation(ValidationIssue(
            EtsiStandard::EN_302_077,
            ValidationSeverity::CRITICAL,
            std::string("DAB+ audio validation exception: ") + e.what()
        ));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto validation_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    combined_result.validation_time = validation_time;
    
    update_performance_metrics(validation_time, combined_result.is_compliant);
    add_to_history(combined_result);
    notify_validation_complete(combined_result);
    
    return combined_result;
}

ComplianceResult EtsiComplianceEngine::validate_cross_standard_consistency(const EtiFrame& frame,
                                                                         const Ensemble& ensemble) {
    ComplianceResult result;
    result.achieved_level = config_.target_level;
    result.frames_analyzed = 1;
    
    try {
        // Validate ETI-DAB consistency
        validate_eti_dab_consistency(frame, ensemble, result);
        
        // Validate service-subchannel consistency
        validate_service_subchannel_consistency(ensemble, result);
        
        // Validate FIG cross-references
        validate_fig_cross_references(frame, ensemble, result);
        
        finalize_compliance_result(result);
        
    } catch (const std::exception& e) {
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_401,
            ValidationSeverity::CRITICAL,
            std::string("Cross-standard validation exception: ") + e.what()
        ));
    }
    
    return result;
}

void EtsiComplianceEngine::set_compliance_level(ComplianceLevel level) {
    config_.target_level = level;
}

ComplianceLevel EtsiComplianceEngine::get_compliance_level() const {
    return config_.target_level;
}

void EtsiComplianceEngine::enable_standard_validation(EtsiStandard standard, bool enabled) {
    if (supports_standard(standard)) {
        enabled_standards_[standard] = enabled;
    }
}

bool EtsiComplianceEngine::is_standard_enabled(EtsiStandard standard) const {
    auto it = enabled_standards_.find(standard);
    return it != enabled_standards_.end() && it->second;
}

EtsiComplianceEngine::PerformanceMetrics EtsiComplianceEngine::get_performance_metrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return performance_metrics_;
}

void EtsiComplianceEngine::reset_performance_metrics() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    performance_metrics_ = {};
    performance_metrics_.min_validation_time = std::chrono::microseconds(std::numeric_limits<long>::max());
    validation_times_.clear();
    start_time_ = std::chrono::high_resolution_clock::now();
}

std::vector<ComplianceResult> EtsiComplianceEngine::get_validation_history(size_t count) const {
    std::lock_guard<std::mutex> lock(history_mutex_);
    if (validation_history_.size() <= count) {
        return validation_history_;
    }
    return std::vector<ComplianceResult>(
        validation_history_.end() - count,
        validation_history_.end()
    );
}

void EtsiComplianceEngine::clear_validation_history() {
    std::lock_guard<std::mutex> lock(history_mutex_);
    validation_history_.clear();
}

void EtsiComplianceEngine::register_validation_callback(ValidationCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    validation_callback_ = callback;
}

void EtsiComplianceEngine::unregister_validation_callback() {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    validation_callback_ = nullptr;
}

// Private helper methods

void EtsiComplianceEngine::update_performance_metrics(const std::chrono::microseconds& validation_time,
                                                     bool success) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    validation_times_.push_back(validation_time);
    if (validation_times_.size() > 1000) {
        validation_times_.erase(validation_times_.begin());
    }
    
    performance_metrics_.total_validations++;
    if (success) {
        performance_metrics_.successful_validations++;
    } else {
        performance_metrics_.failed_validations++;
    }
    
    performance_metrics_.success_rate = 
        static_cast<double>(performance_metrics_.successful_validations) / 
        performance_metrics_.total_validations;
    
    // Update min/max/avg times
    if (validation_time > performance_metrics_.max_validation_time) {
        performance_metrics_.max_validation_time = validation_time;
    }
    
    if (validation_time < performance_metrics_.min_validation_time) {
        performance_metrics_.min_validation_time = validation_time;
    }
    
    if (!validation_times_.empty()) {
        auto total_time = std::accumulate(validation_times_.begin(), validation_times_.end(),
                                        std::chrono::microseconds(0));
        performance_metrics_.avg_validation_time = total_time / validation_times_.size();
    }
    
    // Calculate validations per second
    auto elapsed = std::chrono::high_resolution_clock::now() - start_time_;
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    if (elapsed_seconds > 0) {
        performance_metrics_.validations_per_second = 
            performance_metrics_.total_validations / elapsed_seconds;
    }
    
    // Estimate memory usage (simplified)
    performance_metrics_.memory_usage_mb = 
        (validation_history_.size() * sizeof(ComplianceResult) + 
         validation_times_.size() * sizeof(std::chrono::microseconds)) / (1024 * 1024);
}

void EtsiComplianceEngine::add_to_history(const ComplianceResult& result) {
    std::lock_guard<std::mutex> lock(history_mutex_);
    validation_history_.push_back(result);
    
    // Limit history size
    if (validation_history_.size() > config_.max_validation_history) {
        validation_history_.erase(validation_history_.begin());
    }
}

void EtsiComplianceEngine::notify_validation_complete(const ComplianceResult& result) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (validation_callback_) {
        try {
            validation_callback_(result);
        } catch (...) {
            // Ignore callback exceptions
        }
    }
}

// ETSI EN 300 401 Comprehensive Compliance Validation Implementation
ComplianceResult EtsiComplianceEngine::validate_en300401(const EtiFrame& frame, const Ensemble& ensemble) {
    // Delegate to EN300401Validator if available
    if (en300401_validator_) {
        return en300401_validator_->validate(frame, ensemble);
    }
    
    // Comprehensive ETSI EN 300 401 validation implementation
    ComplianceResult result;
    result.achieved_level = config_.target_level;
    
    // 1. ETI Frame Structure Validation (ETSI EN 300 401 Section 7)
    if (!frame.validate_sync_pattern()) {
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_401,
            ValidationSeverity::CRITICAL,
            "Invalid ETI sync pattern for DAB frame - Section 7.1"
        ));
    }
    
    auto lidata = frame.get_lidata_field();
    
    // Validate Frame Count (Section 7.2.1)
    if (lidata.fc > 249) {
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_401,
            ValidationSeverity::ERROR,
            "Frame count exceeds maximum (249) - Section 7.2.1"
        ));
    }
    
    // Validate Mode Identity (Section 7.2.2)
    if (lidata.mid < 1 || lidata.mid > 4) {
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_401,
            ValidationSeverity::ERROR,
            "Invalid mode identity: " + std::to_string(lidata.mid) + " - Section 7.2.2"
        ));
    }
    
    // Validate Number of Sub-channels (Section 7.2.3)
    if (lidata.nst > 63) {
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_401,
            ValidationSeverity::ERROR,
            "Sub-channel count exceeds maximum (63) - Section 7.2.3"
        ));
    }
    
    // 2. Ensemble Validation (ETSI EN 300 401 Section 8.1)
    if (!ensemble.validate_ensemble_id()) {
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_401,
            ValidationSeverity::ERROR,
            "Invalid ensemble ID: 0x" + QString::number(ensemble.ensemble_id, 16).toStdString() + " - Section 8.1.1"
        ));
    }
    
    if (!ensemble.validate_country_code()) {
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_401,
            ValidationSeverity::ERROR,
            "Invalid country code: 0x" + QString::number(ensemble.country_id, 16).toStdString() + " - Section 8.1.1"
        ));
    }
    
    // 3. Sub-channel Organization Validation (Section 6.2)
    if (!ensemble.validate_subchannel_organization()) {
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_401,
            ValidationSeverity::ERROR,
            "Invalid subchannel organization - overlapping sub-channels detected - Section 6.2"
        ));
    }
    
    // Validate total capacity utilization
    uint16_t totalCapacity = ensemble.calculate_total_capacity();
    if (totalCapacity > 864) { // Maximum CUs per frame
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_401,
            ValidationSeverity::ERROR,
            "Total capacity exceeds maximum: " + std::to_string(totalCapacity) + " CUs - Section 6.2"
        ));
    }
    
    // 4. Service Validation (Section 8.1)
    for (const auto& service : ensemble.services) {
        if (!service.validate_service_id()) {
            result.add_violation(ValidationIssue(
                EtsiStandard::EN_300_401,
                ValidationSeverity::WARNING,
                "Service ID validation failed: 0x" + QString::number(service.service_id, 16).toStdString() + " - Section 8.1.2"
            ));
        }
        
        if (!service.has_primary_component()) {
            result.add_violation(ValidationIssue(
                EtsiStandard::EN_300_401,
                ValidationSeverity::WARNING,
                "Service lacks primary component: 0x" + QString::number(service.service_id, 16).toStdString() + " - Section 8.1.4"
            ));
        }
        
        // Validate service components
        for (const auto& component : service.components) {
            if (!component.validate_tmid()) {
                result.add_violation(ValidationIssue(
                    EtsiStandard::EN_300_401,
                    ValidationSeverity::WARNING,
                    "Invalid TMID for service component - Section 8.1.4"
                ));
            }
            
            if (!component.validate_sub_channel_id()) {
                result.add_violation(ValidationIssue(
                    EtsiStandard::EN_300_401,
                    ValidationSeverity::ERROR,
                    "Invalid sub-channel ID for service component - Section 8.1.4"
                ));
            }
        }
    }
    
    // 5. FIC Content Validation (Section 6.4)
    if (lidata.ficf) {
        auto ficBlocks = frame.decode_fic_blocks();
        bool hasEnsembleInfo = false;
        bool hasSubchannelOrg = false;
        
        for (const auto& fig : ficBlocks) {
            if (fig.fig_type == 0) {
                uint8_t extension = fig.get_extension();
                if (extension == 0) hasEnsembleInfo = true;
                if (extension == 1) hasSubchannelOrg = true;
            }
        }
        
        if (!hasEnsembleInfo) {
            result.add_violation(ValidationIssue(
                EtsiStandard::EN_300_401,
                ValidationSeverity::WARNING,
                "Missing FIG 0/0 (ensemble information) in FIC - Section 8.1.1"
            ));
        }
        
        if (!hasSubchannelOrg && lidata.nst > 0) {
            result.add_violation(ValidationIssue(
                EtsiStandard::EN_300_401,
                ValidationSeverity::WARNING,
                "Missing FIG 0/1 (sub-channel organization) with active sub-channels - Section 8.1.2"
            ));
        }
    }
    
    // Calculate compliance percentage based on violations
    double compliancePercentage = 100.0;
    int criticalCount = 0, errorCount = 0, warningCount = 0;
    
    for (const auto& violation : result.violations) {
        switch (violation.severity) {
            case ValidationSeverity::CRITICAL: 
                criticalCount++; 
                compliancePercentage -= 20.0;
                break;
            case ValidationSeverity::ERROR: 
                errorCount++; 
                compliancePercentage -= 10.0;
                break;
            case ValidationSeverity::WARNING: 
                warningCount++; 
                compliancePercentage -= 2.0;
                break;
            default: break;
        }
    }
    
    // Ensure compliance percentage doesn't go below 0
    compliancePercentage = std::max(0.0, compliancePercentage);
    
    result.set_compliance_percentage(EtsiStandard::EN_300_401, compliancePercentage);
    
    // Add summary information
    if (result.violations.empty()) {
        result.add_info("ETSI EN 300 401 validation passed - Full DAB compliance achieved");
    } else {
        result.add_info("ETSI EN 300 401 validation completed - " + 
                       std::to_string(criticalCount) + " critical, " +
                       std::to_string(errorCount) + " errors, " +
                       std::to_string(warningCount) + " warnings");
    }
    
    return result;
}

ComplianceResult EtsiComplianceEngine::validate_en302077(const std::vector<uint8_t>& audio_data,
                                                       const SubChannelInfo& sub_channel_info) {
    // Delegate to EN302077Validator if available
    if (en302077_validator_) {
        return en302077_validator_->validate(audio_data, sub_channel_info);
    }
    
    // Basic validation implementation
    ComplianceResult result;
    result.achieved_level = config_.target_level;
    
    // Basic DAB+ audio validation
    if (audio_data.empty()) {
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_302_077,
            ValidationSeverity::ERROR,
            "Empty DAB+ audio data"
        ));
    } else {
        // Check minimum audio superframe size (120 bytes typical for DAB+)
        if (audio_data.size() < 120) {
            result.add_violation(ValidationIssue(
                EtsiStandard::EN_302_077,
                ValidationSeverity::WARNING,
                "DAB+ audio data smaller than expected superframe size"
            ));
        }
    }
    
    result.set_compliance_percentage(EtsiStandard::EN_302_077,
                                   result.violations.empty() ? 100.0 : 90.0);
    
    return result;
}

ComplianceResult EtsiComplianceEngine::validate_en300799(const EtiFrame& frame) {
    // Delegate to EN300799Validator if available
    if (en300799_validator_) {
        return en300799_validator_->validate(frame);
    }
    
    // Comprehensive ETSI EN 300 799 ETI frame validation implementation
    ComplianceResult result;
    result.achieved_level = config_.target_level;
    
    // 1. ETI Frame Size Validation (Section 4)
    if (frame.size() != ETI_FRAME_SIZE) {
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_799,
            ValidationSeverity::CRITICAL,
            "Invalid ETI frame size: " + std::to_string(frame.size()) + " (expected: " + std::to_string(ETI_FRAME_SIZE) + ") - Section 4"
        ));
    }
    
    // 2. Sync Pattern Validation (Section 5.1)
    if (!frame.validate_sync_pattern()) {
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_799,
            ValidationSeverity::CRITICAL,
            "Invalid ETI sync pattern - Section 5.1"
        ));
    }
    
    // 3. LIDATA Field Validation (Section 5.2)
    auto lidata = frame.get_lidata_field();
    
    // Frame Count validation (FC)
    if (lidata.fc > 249) {
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_799,
            ValidationSeverity::ERROR,
            "Frame count exceeds maximum (249): " + std::to_string(lidata.fc) + " - Section 5.2.1"
        ));
    }
    
    // Number of Sub-channels validation (NST)
    if (!lidata.validate_nst()) {
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_799,
            ValidationSeverity::ERROR,
            "Invalid number of sub-channels: " + std::to_string(lidata.nst) + " (max 63) - Section 5.2.2"
        ));
    }
    
    // Frame Phase validation (FP)
    if (lidata.fp > 7) {
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_799,
            ValidationSeverity::ERROR,
            "Invalid frame phase: " + std::to_string(lidata.fp) + " (max 7) - Section 5.2.3"
        ));
    }
    
    // Mode Identity validation (MID)
    if (lidata.mid < 1 || lidata.mid > 4) {
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_799,
            ValidationSeverity::ERROR,
            "Invalid mode identity: " + std::to_string(lidata.mid) + " (valid: 1-4) - Section 5.2.4"
        ));
    }
    
    // Frame Length validation (FL)
    uint16_t expectedFrameLength = 6144 / 8; // Convert bytes to 8-byte units
    if (lidata.fl != expectedFrameLength) {
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_799,
            ValidationSeverity::WARNING,
            "Frame length mismatch: " + std::to_string(lidata.fl) + " (expected: " + std::to_string(expectedFrameLength) + ") - Section 5.2.5"
        ));
    }
    
    // 4. FIC Field Validation (Section 5.3)
    if (lidata.ficf) {
        auto fic = frame.get_fic_field();
        if (!fic.validate_fic_crc()) {
            result.add_violation(ValidationIssue(
                EtsiStandard::EN_300_799,
                ValidationSeverity::ERROR,
                "FIC CRC validation failed - Section 5.3"
            ));
        }
    }
    
    // 5. MSC Field Validation (Section 5.4)
    auto msc = frame.get_msc_field();
    size_t expectedMscSize = ETI_FRAME_SIZE - ETI_SYNC_SIZE - ETI_LIDATA_SIZE - ETI_FIC_SIZE - ETI_CRC_SIZE;
    if (msc.get_capacity_units_count() * CAPACITY_UNIT_SIZE != expectedMscSize) {
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_799,
            ValidationSeverity::WARNING,
            "MSC size validation inconsistency - Section 5.4"
        ));
    }
    
    // 6. CRC Field Validation (Section 5.5)
    auto crc = frame.get_crc_field();
    if (!crc.validate_frame(frame)) {
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_799,
            ValidationSeverity::WARNING,
            "ETI frame CRC validation failed - Section 5.5"
        ));
    }
    
    // 7. Timing and Synchronization Validation (Section 6)
    auto timestamp = lidata.get_timestamp_us();
    if (timestamp.count() < 0) {
        result.add_violation(ValidationIssue(
            EtsiStandard::EN_300_799,
            ValidationSeverity::WARNING,
            "Invalid timestamp in TIST field - Section 6"
        ));
    }
    
    // Calculate compliance percentage
    double compliancePercentage = 100.0;
    int criticalCount = 0, errorCount = 0, warningCount = 0;
    
    for (const auto& violation : result.violations) {
        switch (violation.severity) {
            case ValidationSeverity::CRITICAL: 
                criticalCount++; 
                compliancePercentage -= 25.0;
                break;
            case ValidationSeverity::ERROR: 
                errorCount++; 
                compliancePercentage -= 10.0;
                break;
            case ValidationSeverity::WARNING: 
                warningCount++; 
                compliancePercentage -= 2.0;
                break;
            default: break;
        }
    }
    
    // Ensure compliance percentage doesn't go below 0
    compliancePercentage = std::max(0.0, compliancePercentage);
    
    result.set_compliance_percentage(EtsiStandard::EN_300_799, compliancePercentage);
    
    // Add summary information
    if (result.violations.empty()) {
        result.add_info("ETSI EN 300 799 validation passed - Full ETI frame compliance achieved");
    } else {
        result.add_info("ETSI EN 300 799 validation completed - " + 
                       std::to_string(criticalCount) + " critical, " +
                       std::to_string(errorCount) + " errors, " +
                       std::to_string(warningCount) + " warnings");
    }
    
    return result;
}

ComplianceResult EtsiComplianceEngine::validate_ts102563(const std::vector<uint8_t>& audio_data) {
    // Delegate to TS102563Validator if available
    if (ts102563_validator_) {
        return ts102563_validator_->validate(audio_data);
    }
    
    // Basic validation implementation
    ComplianceResult result;
    result.achieved_level = config_.target_level;
    
    // Basic DAB+ guidelines validation
    if (!audio_data.empty()) {
        // Check for proper HE-AAC v2 superframe structure
        // This is a simplified check - full implementation would validate
        // SBR parameters, PS coding, etc.
        result.set_compliance_percentage(EtsiStandard::TS_102_563, 95.0);
    } else {
        result.add_violation(ValidationIssue(
            EtsiStandard::TS_102_563,
            ValidationSeverity::WARNING,
            "No audio data for DAB+ guidelines validation"
        ));
        result.set_compliance_percentage(EtsiStandard::TS_102_563, 0.0);
    }
    
    return result;
}

ComplianceResult EtsiComplianceEngine::validate_ts101756(const Ensemble& ensemble) {
    // Delegate to TS101756Validator if available
    if (ts101756_validator_) {
        return ts101756_validator_->validate(ensemble);
    }
    
    // Basic validation implementation
    ComplianceResult result;
    result.achieved_level = config_.target_level;
    
    // Validate registered tables compliance
    if (!ensemble.validate_country_code()) {
        result.add_violation(ValidationIssue(
            EtsiStandard::TS_101_756,
            ValidationSeverity::ERROR,
            "Invalid country code in ensemble: " + std::to_string(ensemble.country_id)
        ));
    }
    
    // Validate service country codes
    for (const auto& service : ensemble.services) {
        if (!service.validate_country_code()) {
            result.add_violation(ValidationIssue(
                EtsiStandard::TS_101_756,
                ValidationSeverity::ERROR,
                "Invalid country code in service: " + std::to_string(service.country_id),
                "Service: " + service.label
            ));
        }
    }
    
    result.set_compliance_percentage(EtsiStandard::TS_101_756,
                                   result.violations.empty() ? 100.0 : 85.0);
    
    return result;
}

// Helper methods for validation logic
void EtsiComplianceEngine::merge_compliance_results(ComplianceResult& target, const ComplianceResult& source) {
    // Merge violations and warnings
    target.violations.insert(target.violations.end(), source.violations.begin(), source.violations.end());
    target.warnings.insert(target.warnings.end(), source.warnings.begin(), source.warnings.end());
    
    // Merge standard compliance
    for (const auto& pair : source.standard_compliance) {
        target.standard_compliance[pair.first] = pair.second;
    }
    
    // Merge compliance percentages
    for (const auto& pair : source.compliance_percentages) {
        target.compliance_percentages[pair.first] = pair.second;
    }
    
    // Update overall compliance
    target.is_compliant = target.is_compliant && source.is_compliant;
}

void EtsiComplianceEngine::finalize_compliance_result(ComplianceResult& result) {
    // Determine final compliance status based on violations
    result.is_compliant = std::none_of(result.violations.begin(), result.violations.end(),
                                     [](const ValidationIssue& issue) {
                                         return issue.severity >= ValidationSeverity::ERROR;
                                     });
    
    // Set achieved level based on overall compliance
    double overall_compliance = result.get_overall_compliance();
    
    if (overall_compliance >= 99.9) {
        result.achieved_level = ComplianceLevel::STRICT_ETSI;
    } else if (overall_compliance >= 99.0) {
        result.achieved_level = ComplianceLevel::BROADCAST_QUALITY;
    } else if (overall_compliance >= 95.0) {
        result.achieved_level = ComplianceLevel::BASIC_INTEROP;
    } else {
        result.achieved_level = ComplianceLevel::BASIC_INTEROP;
        result.is_compliant = false;
    }
}

// Utility method implementations
std::string EtsiComplianceEngine::get_standard_name(EtsiStandard standard) {
    switch (standard) {
        case EtsiStandard::EN_300_401: return "ETSI EN 300 401 (DAB Radio Broadcasting)";
        case EtsiStandard::EN_302_077: return "ETSI EN 302 077 (DAB+ Audio Coding)";
        case EtsiStandard::EN_300_799: return "ETSI EN 300 799 (ETI Distribution Interface)";
        case EtsiStandard::TS_102_563: return "ETSI TS 102 563 (DAB+ Guidelines)";
        case EtsiStandard::TS_101_756: return "ETSI TS 101 756 (Registered Tables)";
        default: return "Unknown Standard";
    }
}

std::string EtsiComplianceEngine::get_severity_name(ValidationSeverity severity) {
    switch (severity) {
        case ValidationSeverity::CRITICAL: return "CRITICAL";
        case ValidationSeverity::ERROR: return "ERROR";
        case ValidationSeverity::WARNING: return "WARNING";
        case ValidationSeverity::INFO: return "INFO";
        default: return "UNKNOWN";
    }
}

std::string EtsiComplianceEngine::get_compliance_level_name(ComplianceLevel level) {
    switch (level) {
        case ComplianceLevel::STRICT_ETSI: return "Strict ETSI";
        case ComplianceLevel::BROADCAST_QUALITY: return "Broadcast Quality";
        case ComplianceLevel::BASIC_INTEROP: return "Basic Interoperability";
        default: return "Unknown Level";
    }
}

// Namespace utility functions
namespace utils {

std::string format_compliance_result(const ComplianceResult& result) {
    std::stringstream ss;
    
    ss << "ETSI Compliance Result:\n";
    ss << "  Status: " << (result.is_compliant ? "COMPLIANT" : "NON-COMPLIANT") << "\n";
    ss << "  Level: " << EtsiComplianceEngine::get_compliance_level_name(result.achieved_level) << "\n";
    ss << "  Overall: " << std::fixed << std::setprecision(1) << result.get_overall_compliance() << "%\n";
    ss << "  Violations: " << result.violations.size() << "\n";
    ss << "  Warnings: " << result.warnings.size() << "\n";
    ss << "  Validation Time: " << result.validation_time.count() << "μs\n";
    
    // Standard-specific compliance
    ss << "  Standards:\n";
    for (const auto& pair : result.compliance_percentages) {
        ss << "    " << EtsiComplianceEngine::get_standard_name(pair.first) 
           << ": " << std::fixed << std::setprecision(1) << pair.second << "%\n";
    }
    
    return ss.str();
}

std::string generate_compliance_summary(const std::vector<ComplianceResult>& results) {
    if (results.empty()) {
        return "No compliance results available";
    }
    
    std::stringstream ss;
    
    size_t compliant_count = std::count_if(results.begin(), results.end(),
                                         [](const ComplianceResult& r) { return r.is_compliant; });
    
    double compliance_rate = static_cast<double>(compliant_count) / results.size() * 100.0;
    
    ss << "ETSI Compliance Summary:\n";
    ss << "  Total Validations: " << results.size() << "\n";
    ss << "  Compliant: " << compliant_count << " (" << std::fixed << std::setprecision(1) 
       << compliance_rate << "%)\n";
    ss << "  Non-Compliant: " << (results.size() - compliant_count) << "\n";
    
    // Calculate average compliance percentage
    double total_compliance = 0.0;
    for (const auto& result : results) {
        total_compliance += result.get_overall_compliance();
    }
    double avg_compliance = total_compliance / results.size();
    ss << "  Average Compliance: " << std::fixed << std::setprecision(1) << avg_compliance << "%\n";
    
    return ss.str();
}

bool meets_broadcast_requirements(ComplianceLevel level) {
    return level >= ComplianceLevel::BROADCAST_QUALITY;
}

int calculate_compliance_trend(const std::vector<ComplianceResult>& results) {
    if (results.size() < 2) {
        return 0; // Insufficient data
    }
    
    // Simple trend calculation: compare first and second halves
    size_t mid = results.size() / 2;
    
    double first_half_avg = 0.0;
    for (size_t i = 0; i < mid; ++i) {
        first_half_avg += results[i].get_overall_compliance();
    }
    first_half_avg /= mid;
    
    double second_half_avg = 0.0;
    for (size_t i = mid; i < results.size(); ++i) {
        second_half_avg += results[i].get_overall_compliance();
    }
    second_half_avg /= (results.size() - mid);
    
    const double threshold = 1.0; // 1% threshold
    
    if (second_half_avg > first_half_avg + threshold) {
        return 1; // Improving
    } else if (second_half_avg < first_half_avg - threshold) {
        return -1; // Declining
    } else {
        return 0; // Stable
    }
}

} // namespace utils

} // namespace compliance
} // namespace etsi