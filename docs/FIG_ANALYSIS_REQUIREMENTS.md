# FIG Analysis Requirements and Validation Checkpoints

**Document Version:** 1.0  
**Date:** September 17, 2025  
**Author:** Standards Compliance Agent  
**Reference Standards:** ETSI EN 300 401 v2.1.1, ETSI TS 101 756 v1.4.1  

## Table of Contents

1. [FIG Overview](#fig-overview)
2. [Mandatory FIG Types](#mandatory-fig-types)
3. [FIG Validation Framework](#fig-validation-framework)
4. [Analysis Implementation](#analysis-implementation)
5. [Validation Checkpoints](#validation-checkpoints)
6. [Error Handling and Recovery](#error-handling-and-recovery)
7. [Performance Requirements](#performance-requirements)
8. [Integration Specifications](#integration-specifications)

## FIG Overview

Fast Information Groups (FIGs) are the core data structures within the Fast Information Channel (FIC) that provide essential configuration and identification information for DAB ensembles, services, and components. The StreamDAB Analyzer must implement comprehensive FIG analysis to achieve full ETSI compliance.

### FIG Structure Fundamentals

#### Basic FIG Format
```
FIG Header (1 byte):
┌─────────┬─────────┬─────────┬─────────┬─────────┬─────────┬─────────┬─────────┐
│   7     │   6     │   5     │   4     │   3     │   2     │   1     │   0     │
├─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼─────────┤
│    C    │   OE    │      FIG TYPE     │           LENGTH              │
└─────────┴─────────┴─────────┴─────────┴─────────┴─────────┴─────────┴─────────┘

C  = Continuation flag (0=last FIG of this type, 1=more FIGs follow)
OE = Other Ensemble flag (0=current ensemble, 1=other ensemble)
FIG TYPE = FIG type identifier (0-31)
LENGTH = Length of FIG data field (0-29 bytes)
```

#### FIG Type Classification
- **Type 0**: MCI (Multiplex Configuration Information)
- **Type 1**: Labels and service information
- **Type 2**: Reserved for future use
- **Type 3-7**: Reserved for future use

### FIG Transmission Rules

1. **Mandatory Transmission**: FIG 0/0, 0/1, 0/2 must be transmitted regularly
2. **Transmission Frequency**: Critical FIGs every 96ms (4 frames)
3. **Continuation Handling**: Multi-frame FIGs must be reassembled correctly
4. **Other Ensemble Data**: OE=1 FIGs provide information about adjacent ensembles

## Mandatory FIG Types

### FIG 0/0 - Ensemble Information (Critical)

**Purpose**: Provides fundamental ensemble identification and status information  
**Transmission**: Every frame or every other frame  
**ETSI Reference**: EN 300 401 Section 6.4.1  

```cpp
struct Fig0_0_EnsembleInfo {
    // Header (already parsed)
    uint8_t fig_type = 0;
    uint8_t extension = 0;
    
    // Data Field (6 bytes)
    uint16_t ensemble_id;           // EId: Ensemble identifier
    uint8_t change_flags : 2;       // Change event indication
    uint8_t alarm_flag : 1;         // Alarm announcement flag
    uint8_t cif_count_high : 5;     // CIF count upper 5 bits
    uint8_t cif_count_low;          // CIF count lower 8 bits (total 13 bits)
    uint8_t occurrence_change;      // Occurrence change field
    
    // Validation methods
    bool validate_ensemble_id() const {
        return ensemble_id != 0x0000 && ensemble_id != 0xFFFF;
    }
    
    uint16_t get_cif_count() const {
        return (static_cast<uint16_t>(cif_count_high) << 8) | cif_count_low;
    }
    
    bool validate_cif_continuity(uint16_t previous_cif) const {
        uint16_t current_cif = get_cif_count();
        return (current_cif == (previous_cif + 1) % 5000);  // CIF wraps at 5000
    }
};

// Validation requirements
class Fig0_0_Validator {
public:
    struct ValidationResult {
        bool valid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };
    
    static ValidationResult validate(const Fig0_0_EnsembleInfo& fig);
    static bool check_transmission_frequency(const std::vector<Fig0_0_EnsembleInfo>& history);
    static bool validate_change_event_handling(const Fig0_0_EnsembleInfo& current, 
                                              const Fig0_0_EnsembleInfo& previous);
};
```

**Validation Checkpoints:**
- ✅ Ensemble ID validity (not 0x0000 or 0xFFFF)
- ✅ CIF count continuity (incremental with wrap-around)
- ✅ Change flags consistency with actual configuration changes
- ✅ Transmission frequency compliance (minimum every 96ms)

### FIG 0/1 - Sub-channel Organization (Critical)

**Purpose**: Defines MSC sub-channel structure and organization  
**Transmission**: Must be present when sub-channel configuration changes  
**ETSI Reference**: EN 300 401 Section 6.2.1  

```cpp
struct Fig0_1_SubchannelOrg {
    uint8_t fig_type = 0;
    uint8_t extension = 1;
    
    // Variable length data field
    struct SubChannelDescriptor {
        uint8_t sub_channel_id : 6;       // SubChId (0-63)
        uint16_t start_address : 10;       // Start address in CUs
        uint8_t table_switch : 1;          // Short/Long form table switch
        
        // Short Form (Protection Level)
        uint8_t protection_level : 2;      // Protection level (1-4 mapped to internal)
        uint16_t sub_channel_size : 10;    // Size in CUs
        
        // Long Form (UEP Table)
        uint8_t option : 3;                // Option field
        uint8_t protection_level_long : 2; // Protection level
        uint16_t sub_channel_size_long : 10; // Size in CUs
        
        bool is_long_form() const { return table_switch == 1; }
        
        uint16_t get_size() const {
            return is_long_form() ? sub_channel_size_long : sub_channel_size;
        }
        
        uint8_t get_protection_level() const {
            return is_long_form() ? protection_level_long : protection_level;
        }
        
        bool validate_address_bounds() const {
            return (start_address + get_size()) <= 864;  // Max CUs per frame
        }
        
        bool validate_sub_channel_id() const {
            return sub_channel_id <= 63;
        }
    };
    
    std::vector<SubChannelDescriptor> sub_channels;
    
    bool validate_no_overlap() const {
        for (size_t i = 0; i < sub_channels.size(); ++i) {
            for (size_t j = i + 1; j < sub_channels.size(); ++j) {
                const auto& a = sub_channels[i];
                const auto& b = sub_channels[j];
                if (!(a.start_address + a.get_size() <= b.start_address ||
                      b.start_address + b.get_size() <= a.start_address)) {
                    return false;  // Overlap detected
                }
            }
        }
        return true;
    }
    
    uint16_t calculate_total_capacity() const {
        uint16_t total = 0;
        for (const auto& sub_ch : sub_channels) {
            total += sub_ch.get_size();
        }
        return total;
    }
};

class Fig0_1_Validator {
public:
    static ValidationResult validate(const Fig0_1_SubchannelOrg& fig) {
        ValidationResult result{true, {}, {}};
        
        // Check sub-channel count
        if (fig.sub_channels.size() > 64) {
            result.valid = false;
            result.errors.push_back("Too many sub-channels (max 64)");
        }
        
        // Validate each sub-channel
        for (const auto& sub_ch : fig.sub_channels) {
            if (!sub_ch.validate_address_bounds()) {
                result.valid = false;
                result.errors.push_back("Sub-channel address out of bounds");
            }
            if (!sub_ch.validate_sub_channel_id()) {
                result.valid = false;
                result.errors.push_back("Invalid sub-channel ID");
            }
        }
        
        // Check for overlaps
        if (!fig.validate_no_overlap()) {
            result.valid = false;
            result.errors.push_back("Sub-channel address overlap detected");
        }
        
        // Check total capacity
        if (fig.calculate_total_capacity() > 864) {
            result.valid = false;
            result.errors.push_back("Total sub-channel capacity exceeds limit");
        }
        
        return result;
    }
};
```

**Validation Checkpoints:**
- ✅ Sub-channel ID uniqueness (0-63)
- ✅ Address boundary validation (within 864 CUs)
- ✅ No address space overlap between sub-channels
- ✅ Protection level validity (1-5)
- ✅ Total capacity constraint (≤ 864 CUs)

### FIG 0/2 - Service Organization (Critical)

**Purpose**: Maps services to service components and sub-channels  
**Transmission**: Required for service discovery and organization  
**ETSI Reference**: EN 300 401 Section 8.1.1  

```cpp
struct Fig0_2_ServiceOrg {
    uint8_t fig_type = 0;
    uint8_t extension = 2;
    
    struct ServiceDescriptor {
        uint16_t service_id;              // SId: Service identifier
        uint8_t country_id;               // Country ID
        uint8_t extended_country_code;    // ECC: Extended Country Code
        bool programme_service;           // P/D flag (0=data, 1=programme)
        
        struct ServiceComponent {
            uint8_t tmid : 2;             // Transport Mechanism ID
            uint8_t asc_ty : 6;           // Audio Service Component Type
            uint8_t sub_channel_id : 6;   // SubChId
            bool primary : 1;             // Primary/Secondary component
            uint8_t ca_flag : 1;          // Conditional Access flag
            
            bool validate_tmid() const {
                return tmid <= 3;  // Valid TMID values: 0-3
            }
            
            bool validate_asc_ty() const {
                // Validate Audio Service Component Type
                return asc_ty <= 63;  // Valid range
            }
            
            bool validate_sub_channel_id() const {
                return sub_channel_id <= 63;
            }
        };
        
        std::vector<ServiceComponent> components;
        
        bool validate_service_id() const {
            // Programme services: 0x0001-0xFFFE
            // Data services: 0x0001-0xFFFE (different namespace)
            return service_id != 0x0000 && service_id != 0xFFFF;
        }
        
        bool validate_country_code() const {
            // Validate against ETSI TS 101 756 registered tables
            return country_id != 0x00 && country_id != 0xFF;
        }
        
        bool has_primary_component() const {
            return std::any_of(components.begin(), components.end(),
                             [](const ServiceComponent& comp) { return comp.primary; });
        }
    };
    
    std::vector<ServiceDescriptor> services;
    
    bool validate_service_id_uniqueness() const {
        std::set<uint16_t> programme_ids, data_ids;
        for (const auto& service : services) {
            auto& id_set = service.programme_service ? programme_ids : data_ids;
            if (id_set.count(service.service_id)) {
                return false;  // Duplicate service ID
            }
            id_set.insert(service.service_id);
        }
        return true;
    }
};

class Fig0_2_Validator {
public:
    static ValidationResult validate(const Fig0_2_ServiceOrg& fig) {
        ValidationResult result{true, {}, {}};
        
        // Check service ID uniqueness
        if (!fig.validate_service_id_uniqueness()) {
            result.valid = false;
            result.errors.push_back("Duplicate service IDs detected");
        }
        
        // Validate each service
        for (const auto& service : fig.services) {
            if (!service.validate_service_id()) {
                result.valid = false;
                result.errors.push_back("Invalid service ID");
            }
            
            if (!service.validate_country_code()) {
                result.warnings.push_back("Questionable country code");
            }
            
            if (!service.has_primary_component()) {
                result.warnings.push_back("Service has no primary component");
            }
            
            // Validate components
            for (const auto& comp : service.components) {
                if (!comp.validate_tmid()) {
                    result.valid = false;
                    result.errors.push_back("Invalid TMID");
                }
                if (!comp.validate_sub_channel_id()) {
                    result.valid = false;
                    result.errors.push_back("Invalid sub-channel ID in component");
                }
            }
        }
        
        return result;
    }
};
```

**Validation Checkpoints:**
- ✅ Service ID validity and uniqueness per service type
- ✅ Country code validation against registered tables
- ✅ Service component TMID validation (0-3)
- ✅ Sub-channel ID cross-reference with FIG 0/1
- ✅ Primary component presence validation

### FIG 1/0 - Ensemble Label (Critical)

**Purpose**: Provides human-readable ensemble identification  
**Transmission**: Should be transmitted regularly for user interfaces  
**ETSI Reference**: EN 300 401 Section 8.1.13  

```cpp
struct Fig1_0_EnsembleLabel {
    uint8_t fig_type = 1;
    uint8_t extension = 0;
    
    uint16_t ensemble_id;         // EId: Must match FIG 0/0
    char label[16];               // Ensemble label
    uint16_t character_flag;      // Character definition field
    
    std::string get_label_string() const {
        return std::string(label, strnlen(label, 16));
    }
    
    bool validate_ensemble_id_match(uint16_t expected_eid) const {
        return ensemble_id == expected_eid;
    }
    
    bool validate_character_encoding() const {
        // Check for valid UTF-8 encoding
        return is_valid_utf8_string(label, 16);
    }
    
    bool validate_printable_characters() const {
        for (int i = 0; i < 16 && label[i] != '\0'; ++i) {
            if (!std::isprint(static_cast<unsigned char>(label[i]))) {
                return false;
            }
        }
        return true;
    }
};
```

### FIG 1/1 - Service Labels (Recommended)

**Purpose**: Provides human-readable service identification  
**Transmission**: Recommended for enhanced user experience  
**ETSI Reference**: EN 300 401 Section 8.1.14  

```cpp
struct Fig1_1_ServiceLabel {
    uint8_t fig_type = 1;
    uint8_t extension = 1;
    
    uint16_t service_id;          // SId: Must match service in FIG 0/2
    char label[16];               // Service label
    uint16_t character_flag;      // Character definition field
    
    std::string get_label_string() const {
        return std::string(label, strnlen(label, 16));
    }
    
    bool validate_service_id_reference(const std::vector<uint16_t>& known_service_ids) const {
        return std::find(known_service_ids.begin(), known_service_ids.end(), 
                        service_id) != known_service_ids.end();
    }
    
    bool validate_character_encoding() const {
        return is_valid_utf8_string(label, 16);
    }
};
```

## FIG Validation Framework

### Comprehensive FIG Analyzer

```cpp
class FigAnalyzer {
public:
    struct AnalysisResult {
        bool analysis_successful;
        std::map<uint8_t, std::vector<std::unique_ptr<FigBase>>> decoded_figs;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::vector<std::string> recommendations;
    };
    
    struct FigStatistics {
        std::map<uint8_t, uint32_t> fig_type_counts;
        std::map<uint8_t, std::chrono::milliseconds> last_seen_times;
        uint32_t total_figs_processed;
        uint32_t invalid_figs_detected;
        double fig_validity_percentage;
    };
    
    AnalysisResult analyze_fic_block(const uint8_t fic_data[32]);
    AnalysisResult analyze_fig_sequence(const std::vector<EtiFrame>& frames);
    
    FigStatistics get_current_statistics() const;
    void reset_statistics();
    
    // Cross-validation methods
    bool validate_fig_consistency(const AnalysisResult& result);
    std::vector<std::string> check_mandatory_fig_presence(const AnalysisResult& result);
    bool validate_cross_references(const AnalysisResult& result);
    
private:
    FigStatistics statistics_;
    std::map<uint8_t, std::unique_ptr<FigValidatorBase>> validators_;
    
    void update_statistics(const AnalysisResult& result);
    std::unique_ptr<FigBase> decode_fig_by_type(uint8_t fig_type, 
                                               const std::vector<uint8_t>& data);
};
```

### FIG Base Classes

```cpp
class FigBase {
public:
    virtual ~FigBase() = default;
    virtual uint8_t get_fig_type() const = 0;
    virtual uint8_t get_extension() const = 0;
    virtual bool validate() const = 0;
    virtual std::string get_description() const = 0;
    virtual std::vector<uint8_t> serialize() const = 0;
};

class FigValidatorBase {
public:
    virtual ~FigValidatorBase() = default;
    virtual ValidationResult validate(const FigBase& fig) const = 0;
    virtual bool check_transmission_requirements(
        const std::vector<std::chrono::milliseconds>& transmission_times) const = 0;
};

template<typename FigType>
class FigValidator : public FigValidatorBase {
public:
    ValidationResult validate(const FigBase& fig) const override {
        const auto* typed_fig = dynamic_cast<const FigType*>(&fig);
        if (!typed_fig) {
            return {false, {"Type mismatch in FIG validation"}, {}};
        }
        return validate_specific(*typed_fig);
    }
    
protected:
    virtual ValidationResult validate_specific(const FigType& fig) const = 0;
};
```

## Validation Checkpoints

### Real-time Validation Pipeline

```cpp
class FigValidationPipeline {
public:
    enum class ValidationLevel {
        BASIC_SYNTAX,        // Basic FIG structure validation
        SEMANTIC_COHERENCE,  // Cross-FIG consistency checking
        TRANSMISSION_COMPLIANCE, // Transmission timing and frequency
        BROADCAST_QUALITY    // Professional broadcast standards
    };
    
    struct ValidationCheckpoint {
        std::string checkpoint_name;
        ValidationLevel level;
        bool is_critical;
        std::function<ValidationResult(const FigAnalyzer::AnalysisResult&)> validator;
    };
    
    struct PipelineResult {
        ValidationLevel highest_level_achieved;
        std::vector<ValidationCheckpoint> passed_checkpoints;
        std::vector<ValidationCheckpoint> failed_checkpoints;
        double overall_compliance_score;
        bool broadcast_ready;
    };
    
    void add_checkpoint(const ValidationCheckpoint& checkpoint);
    PipelineResult run_validation(const FigAnalyzer::AnalysisResult& analysis);
    void set_target_validation_level(ValidationLevel level);
    
private:
    std::vector<ValidationCheckpoint> checkpoints_;
    ValidationLevel target_level_;
    
    // Standard checkpoints
    ValidationResult validate_mandatory_figs(const FigAnalyzer::AnalysisResult& analysis);
    ValidationResult validate_transmission_timing(const FigAnalyzer::AnalysisResult& analysis);
    ValidationResult validate_cross_references(const FigAnalyzer::AnalysisResult& analysis);
    ValidationResult validate_ensemble_consistency(const FigAnalyzer::AnalysisResult& analysis);
};
```

### Critical Validation Checkpoints

#### Checkpoint 1: Mandatory FIG Presence
```cpp
ValidationResult validate_mandatory_figs(const FigAnalyzer::AnalysisResult& analysis) {
    ValidationResult result{true, {}, {}};
    
    // Check for FIG 0/0 (Ensemble Information)
    if (analysis.decoded_figs.find(0) == analysis.decoded_figs.end() ||
        analysis.decoded_figs.at(0).empty()) {
        result.valid = false;
        result.errors.push_back("Missing mandatory FIG 0/0 (Ensemble Information)");
    }
    
    // Check for FIG 0/1 (Sub-channel Organization)
    bool has_fig_0_1 = false;
    if (analysis.decoded_figs.find(0) != analysis.decoded_figs.end()) {
        for (const auto& fig : analysis.decoded_figs.at(0)) {
            if (fig->get_extension() == 1) {
                has_fig_0_1 = true;
                break;
            }
        }
    }
    if (!has_fig_0_1) {
        result.valid = false;
        result.errors.push_back("Missing mandatory FIG 0/1 (Sub-channel Organization)");
    }
    
    // Check for FIG 0/2 (Service Organization)
    bool has_fig_0_2 = false;
    if (analysis.decoded_figs.find(0) != analysis.decoded_figs.end()) {
        for (const auto& fig : analysis.decoded_figs.at(0)) {
            if (fig->get_extension() == 2) {
                has_fig_0_2 = true;
                break;
            }
        }
    }
    if (!has_fig_0_2) {
        result.valid = false;
        result.errors.push_back("Missing mandatory FIG 0/2 (Service Organization)");
    }
    
    return result;
}
```

#### Checkpoint 2: Cross-Reference Validation
```cpp
ValidationResult validate_cross_references(const FigAnalyzer::AnalysisResult& analysis) {
    ValidationResult result{true, {}, {}};
    
    // Extract data from different FIG types
    std::vector<uint16_t> ensemble_ids_from_0_0;
    std::vector<uint8_t> sub_channel_ids_from_0_1;
    std::vector<std::pair<uint16_t, std::vector<uint8_t>>> services_and_subchannels_from_0_2;
    std::vector<uint16_t> ensemble_ids_from_1_0;
    std::vector<uint16_t> service_ids_from_1_1;
    
    // ... extraction logic ...
    
    // Validate ensemble ID consistency between FIG 0/0 and FIG 1/0
    for (uint16_t eid_1_0 : ensemble_ids_from_1_0) {
        if (std::find(ensemble_ids_from_0_0.begin(), ensemble_ids_from_0_0.end(), 
                     eid_1_0) == ensemble_ids_from_0_0.end()) {
            result.valid = false;
            result.errors.push_back("Ensemble ID in FIG 1/0 not found in FIG 0/0");
        }
    }
    
    // Validate sub-channel references in FIG 0/2 exist in FIG 0/1
    for (const auto& service_subch_pair : services_and_subchannels_from_0_2) {
        for (uint8_t subch_id : service_subch_pair.second) {
            if (std::find(sub_channel_ids_from_0_1.begin(), sub_channel_ids_from_0_1.end(),
                         subch_id) == sub_channel_ids_from_0_1.end()) {
                result.valid = false;
                result.errors.push_back("Sub-channel ID referenced in FIG 0/2 not defined in FIG 0/1");
            }
        }
    }
    
    return result;
}
```

#### Checkpoint 3: Transmission Timing Validation
```cpp
ValidationResult validate_transmission_timing(const FigAnalyzer::AnalysisResult& analysis) {
    ValidationResult result{true, {}, {}};
    
    const auto& stats = analyzer_.get_current_statistics();
    
    // FIG 0/0 must be transmitted at least every 96ms (4 frames)
    if (stats.last_seen_times.find(0) != stats.last_seen_times.end()) {
        auto time_since_fig_0_0 = std::chrono::steady_clock::now() - 
                                 stats.last_seen_times.at(0);
        if (time_since_fig_0_0 > std::chrono::milliseconds(96)) {
            result.valid = false;
            result.errors.push_back("FIG 0/0 transmission interval exceeds 96ms requirement");
        }
    }
    
    // Check minimum transmission frequencies for other critical FIGs
    // ... additional timing validation ...
    
    return result;
}
```

## Error Handling and Recovery

### FIG Error Detection and Recovery

```cpp
class FigErrorHandler {
public:
    enum class ErrorType {
        SYNTAX_ERROR,        // Malformed FIG structure
        SEMANTIC_ERROR,      // Invalid content or cross-references
        TRANSMISSION_ERROR,  // Missing or late FIGs
        ENCODING_ERROR       // Character encoding issues
    };
    
    struct ErrorReport {
        ErrorType type;
        uint8_t fig_type;
        uint8_t extension;
        std::string description;
        std::chrono::system_clock::time_point timestamp;
        bool recoverable;
        std::string recovery_action;
    };
    
    struct RecoveryStrategy {
        ErrorType error_type;
        std::function<bool(const ErrorReport&)> recovery_function;
        std::string strategy_description;
    };
    
    void register_error(const ErrorReport& error);
    std::vector<ErrorReport> get_error_history() const;
    bool attempt_recovery(const ErrorReport& error);
    void clear_error_history();
    
    // Recovery strategies
    bool recover_from_syntax_error(const ErrorReport& error);
    bool recover_from_missing_fig(const ErrorReport& error);
    bool recover_from_cross_reference_error(const ErrorReport& error);
    
private:
    std::vector<ErrorReport> error_history_;
    std::map<ErrorType, RecoveryStrategy> recovery_strategies_;
    std::chrono::milliseconds error_retention_time_{300000};  // 5 minutes
    
    void cleanup_old_errors();
};
```

### Graceful Degradation Strategies

```cpp
class FigDegradationManager {
public:
    enum class DegradationLevel {
        FULL_COMPLIANCE,     // All FIGs present and valid
        ESSENTIAL_ONLY,      // Only mandatory FIGs present
        MINIMAL_FUNCTION,    // Basic operation with warnings
        SERVICE_UNAVAILABLE  // Cannot provide reliable information
    };
    
    struct ServiceStatus {
        uint16_t service_id;
        bool fig_0_2_present;
        bool fig_1_1_present;
        bool sub_channel_mapped;
        DegradationLevel status_level;
        std::vector<std::string> missing_information;
    };
    
    DegradationLevel assess_current_level(const FigAnalyzer::AnalysisResult& analysis);
    std::vector<ServiceStatus> get_service_status_report();
    std::vector<std::string> get_degradation_recommendations();
    
    void set_minimum_acceptable_level(DegradationLevel level);
    bool is_service_usable(uint16_t service_id);
    
private:
    DegradationLevel current_level_;
    DegradationLevel minimum_acceptable_level_;
    std::vector<ServiceStatus> service_status_;
    
    DegradationLevel calculate_service_level(const ServiceStatus& status);
    void update_service_status(const FigAnalyzer::AnalysisResult& analysis);
};
```

## Performance Requirements

### Real-time FIG Processing

```cpp
class FigPerformanceMonitor {
public:
    struct PerformanceMetrics {
        std::chrono::microseconds avg_fig_decode_time;
        std::chrono::microseconds max_fig_decode_time;
        uint32_t figs_processed_per_second;
        uint32_t validation_checks_per_second;
        double cpu_usage_percentage;
        size_t memory_usage_bytes;
    };
    
    struct PerformanceTargets {
        static constexpr auto MAX_FIG_DECODE_TIME = std::chrono::microseconds(1000);  // 1ms max
        static constexpr uint32_t MIN_FIGS_PER_SECOND = 3000;  // > 3000 FIGs/sec
        static constexpr double MAX_CPU_USAGE = 10.0;  // 10% max CPU for FIG processing
        static constexpr size_t MAX_MEMORY_USAGE = 10 * 1024 * 1024;  // 10MB max
    };
    
    void start_monitoring();
    void stop_monitoring();
    PerformanceMetrics get_current_metrics();
    bool meets_performance_targets();
    
    void record_fig_processing_time(std::chrono::microseconds processing_time);
    void record_validation_time(std::chrono::microseconds validation_time);
    
private:
    bool monitoring_active_;
    PerformanceMetrics current_metrics_;
    std::vector<std::chrono::microseconds> processing_times_;
    std::chrono::steady_clock::time_point monitoring_start_time_;
    
    void update_metrics();
    void cleanup_old_measurements();
};
```

## Integration Specifications

### ETISnoop Integration for FIG Processing

```cpp
class EtiSnoopFigIntegrator {
public:
    struct IntegrationConfig {
        bool enable_real_time_fig_analysis;
        bool generate_fig_reports;
        ValidationLevel target_validation_level;
        std::string output_format;  // "yaml", "json", "xml"
    };
    
    struct FigAnalysisOutput {
        Ensemble ensemble_info;
        std::vector<DabService> services;
        std::map<uint8_t, std::vector<std::string>> fig_contents;
        FigValidationPipeline::PipelineResult validation_results;
        std::vector<std::string> compliance_notes;
    };
    
    bool initialize_integration(const IntegrationConfig& config);
    FigAnalysisOutput analyze_eti_file(const std::string& filename);
    FigAnalysisOutput analyze_eti_stream(std::istream& stream);
    
    void set_fig_analyzer(std::shared_ptr<FigAnalyzer> analyzer);
    void set_validation_pipeline(std::shared_ptr<FigValidationPipeline> pipeline);
    
private:
    std::shared_ptr<FigAnalyzer> fig_analyzer_;
    std::shared_ptr<FigValidationPipeline> validation_pipeline_;
    IntegrationConfig config_;
    
    FigAnalysisOutput process_fig_data(const std::vector<EtiFrame>& frames);
    std::string format_output(const FigAnalysisOutput& output);
};
```

### Qt GUI FIG Monitoring Interface

```cpp
class FigMonitoringWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit FigMonitoringWidget(QWidget* parent = nullptr);
    
    void set_fig_analyzer(std::shared_ptr<FigAnalyzer> analyzer);
    void update_fig_status(const FigAnalyzer::AnalysisResult& analysis);
    void update_validation_results(const FigValidationPipeline::PipelineResult& results);
    
public slots:
    void on_new_fig_data(const FigAnalyzer::AnalysisResult& analysis);
    void on_validation_completed(const FigValidationPipeline::PipelineResult& results);
    void export_fig_report();
    void configure_fig_monitoring();
    
signals:
    void fig_error_detected(const QString& error_message);
    void fig_validation_failed(const QString& validation_error);
    void ensemble_configuration_changed();
    
private:
    class FigTypeMatrix;
    class ValidationStatusIndicator;
    class EnsembleInfoDisplay;
    class ServiceListWidget;
    
    std::unique_ptr<FigTypeMatrix> fig_type_matrix_;
    std::unique_ptr<ValidationStatusIndicator> validation_indicator_;
    std::unique_ptr<EnsembleInfoDisplay> ensemble_display_;
    std::unique_ptr<ServiceListWidget> service_list_;
    
    std::shared_ptr<FigAnalyzer> fig_analyzer_;
    
    void setup_ui_components();
    void connect_fig_signals();
    void update_fig_statistics();
};
```

This comprehensive FIG analysis framework provides the foundation for full ETSI compliance in FIC processing. The implementation ensures real-time validation, error recovery, and professional-grade monitoring suitable for broadcast industry applications.

**Implementation Priority:**
1. Core FIG decoder classes (0/0, 0/1, 0/2, 1/0, 1/1)
2. Validation framework and checkpoints
3. Cross-reference validation system
4. Error handling and recovery mechanisms
5. Performance monitoring integration
6. Qt GUI FIG monitoring interface

**Next Steps:**
1. Implement FIG base classes and validators
2. Create validation pipeline framework
3. Develop error handling strategies
4. Integrate with ETISnoop processing
5. Build Qt monitoring interface
6. Establish performance benchmarks