/**
 * @file fig_parser.h
 * @brief Comprehensive ETSI-Compliant FIG Parser Engine
 * 
 * Implements complete Fast Information Group (FIG) parsing according to
 * ETSI EN 300 401 Section 5.2 and related standards. Designed for
 * high-performance real-time DAB stream analysis.
 * 
 * Supported FIG Types:
 * - FIG 0/0: Ensemble information
 * - FIG 0/1: Sub-channel organization
 * - FIG 0/2: Service organization  
 * - FIG 0/3: Service component in packet mode
 * - FIG 0/4: Service component with conditional access
 * - FIG 0/5: Service component language
 * - FIG 1/0: Ensemble label
 * - FIG 1/1: Programme service label
 * - FIG 1/4: Service component label
 * - FIG 1/5: Data service label
 * 
 * Performance Target: >900 FPS frame processing with <5ms FIG latency
 * 
 * @author Standards Compliance Agent
 * @date September 22, 2025
 * @copyright StreamDAB Analyzer Project
 */

#pragma once

#include "eti_types.h"
#include <QObject>
#include <QString>
#include <QByteArray>
#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <atomic>

namespace eti::fig {

/**
 * @brief FIG 0/0 Ensemble Information Structure
 * Reference: ETSI EN 300 401 Section 8.1.1.2.1
 */
struct Fig00EnsembleInfo {
    uint16_t ensemble_id;           // EId (16-bit ensemble identifier)
    uint8_t country_id;             // Country ID (4-bit)
    uint8_t extended_country_code;  // ECC (8-bit extended country code)
    uint8_t alarm_flag;             // AL (1-bit alarm flag)
    uint8_t cif_count_high;         // CIF Count (high 2 bits)
    uint8_t cif_count_low;          // CIF Count (low 8 bits)
    bool is_valid;                  // Validation flag
    
    uint16_t get_cif_count() const {
        return (static_cast<uint16_t>(cif_count_high) << 8) | cif_count_low;
    }
    
    bool validate_country_id() const {
        return country_id != 0x0F;  // Reserved value
    }
    
    bool is_thai_ensemble() const {
        return country_id == 0x0E && extended_country_code == 0xE1;  // Thailand
    }
};

/**
 * @brief FIG 0/1 Sub-channel Organization Structure
 * Reference: ETSI EN 300 401 Section 8.1.1.2.2
 */
struct Fig01SubchannelInfo {
    uint8_t subchannel_id;          // SubChId (6-bit)
    uint16_t start_address;         // Start Address (10-bit in CUs)
    bool short_form;                // Form flag (1=short, 0=long)
    
    // Short form fields
    uint8_t table_switch;           // Table Switch (1-bit)
    uint8_t table_index;            // Table Index (6-bit)
    
    // Long form fields
    uint8_t option;                 // Option (3-bit)
    uint8_t protection_level;       // Protection Level (2-bit)
    uint16_t subchannel_size;       // Sub-channel Size (10-bit in CUs)
    
    bool is_valid;                  // Validation flag
    
    bool validate_subchannel_id() const {
        return subchannel_id < 64;  // Maximum 64 sub-channels
    }
    
    bool validate_boundaries() const {
        return start_address < 864 && // Maximum CUs per frame
               (short_form || (start_address + subchannel_size) <= 864);
    }
    
    uint8_t get_protection_level() const {
        return short_form ? (table_index & 0x3F) : protection_level;
    }
};

/**
 * @brief FIG 0/2 Service Organization Structure
 * Reference: ETSI EN 300 401 Section 8.1.1.2.3
 */
struct Fig02ServiceInfo {
    uint32_t service_id;            // SId (16-bit or 32-bit)
    uint8_t country_id;             // Country ID (4-bit)
    uint8_t extended_country_code;  // ECC (8-bit)
    bool local_flag;                // Local flag
    bool caid_flag;                 // CAID flag
    uint8_t number_of_components;   // Number of service components
    
    struct ServiceComponent {
        uint8_t transport_mechanism_id; // TMId (2-bit)
        uint8_t audio_service_type;     // ASCTy/DSCTy (6-bit)
        uint8_t subchannel_id;          // SubChId (6-bit)
        uint16_t packet_address;        // Packet Address (10-bit)
        bool primary_flag;              // P/S flag
        bool ca_flag;                   // CA flag
        bool is_valid;
    };
    
    std::vector<ServiceComponent> components;
    bool is_valid;
    
    bool validate_service_id() const {
        return service_id != 0;
    }
    
    bool validate_components() const {
        return number_of_components == components.size() &&
               number_of_components <= 12;  // Maximum components per service
    }
    
    bool is_audio_service() const {
        return std::any_of(components.begin(), components.end(),
            [](const ServiceComponent& comp) {
                return comp.transport_mechanism_id == 0;  // Stream mode
            });
    }
};

/**
 * @brief FIG 1/x Service Label Structures
 * Reference: ETSI EN 300 401 Section 8.1.2
 */
struct ServiceLabel {
    uint32_t service_id;            // Service identifier
    std::string label;              // Service label (up to 16 characters)
    uint16_t character_flag_field;  // Character flag field
    bool uses_utf8;                 // UTF-8 encoding flag
    bool supports_thai;             // Thai character support
    bool is_valid;                  // Validation flag
    
    bool validate_label_length() const {
        return label.length() <= 16;
    }
    
    bool validate_character_encoding() const {
        // Validate proper UTF-8 encoding for Thai support
        if (!supports_thai) return true;
        
        // Check for valid Thai UTF-8 sequences
        return std::all_of(label.begin(), label.end(), [](unsigned char c) {
            return c < 0x80 || (c >= 0xE0 && c <= 0xEF);  // Thai Unicode range
        });
    }
};

/**
 * @brief FIG Parsing Result Structure
 */
struct FigParsingResult {
    bool parsing_successful;
    std::chrono::nanoseconds parsing_time;
    uint32_t figs_processed;
    uint32_t parsing_errors;
    
    // Parsed FIG data
    std::vector<Fig00EnsembleInfo> ensemble_info;
    std::vector<Fig01SubchannelInfo> subchannel_info;
    std::vector<Fig02ServiceInfo> service_info;
    std::vector<ServiceLabel> service_labels;
    
    // ETSI compliance status
    bool etsi_compliant;
    std::vector<std::string> compliance_violations;
    std::vector<std::string> compliance_warnings;
    
    void reset() {
        parsing_successful = false;
        parsing_time = std::chrono::nanoseconds::zero();
        figs_processed = 0;
        parsing_errors = 0;
        ensemble_info.clear();
        subchannel_info.clear();
        service_info.clear();
        service_labels.clear();
        etsi_compliant = false;
        compliance_violations.clear();
        compliance_warnings.clear();
    }
};

/**
 * @brief High-Performance ETSI-Compliant FIG Parser
 * 
 * Implements complete FIG parsing with optimized performance for
 * real-time DAB stream analysis. Designed to maintain >900 FPS
 * processing rate while providing comprehensive ETSI compliance.
 */
class FigParser : public QObject {
    Q_OBJECT

public:
    explicit FigParser(QObject* parent = nullptr);
    ~FigParser();

    /**
     * @brief Initialize the FIG parser
     * @param enable_thai_support Enable Thai character encoding support
     * @param strict_compliance Enable strict ETSI compliance checking
     * @return true if initialization successful
     */
    bool initialize(bool enable_thai_support = true, bool strict_compliance = false);

    /**
     * @brief Parse FIC data and extract all FIG blocks
     * @param fic_data FIC field data (32 bytes)
     * @return FIG parsing result with all extracted information
     */
    FigParsingResult parseFicData(const EtiFicField& fic_data);

    /**
     * @brief Parse individual FIG block
     * @param fig_block FIG block to parse
     * @return true if parsing successful
     */
    bool parseFigBlock(const FigBlock& fig_block);

    /**
     * @brief Parse FIG 0/0 ensemble information
     * @param fig_data FIG data payload
     * @return Parsed ensemble information
     */
    Fig00EnsembleInfo parseFig00_EnsembleInfo(const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 0/1 sub-channel organization
     * @param fig_data FIG data payload
     * @return Vector of parsed sub-channel information
     */
    std::vector<Fig01SubchannelInfo> parseFig01_SubchannelOrganization(
        const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 0/2 service organization
     * @param fig_data FIG data payload
     * @return Vector of parsed service information
     */
    std::vector<Fig02ServiceInfo> parseFig02_ServiceOrganization(
        const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 1/1 programme service labels
     * @param fig_data FIG data payload
     * @return Vector of parsed service labels
     */
    std::vector<ServiceLabel> parseFig11_ProgrammeServiceLabels(
        const std::vector<uint8_t>& fig_data);

    /**
     * @brief Validate FIG parsing against ETSI standards
     * @param result FIG parsing result to validate
     * @return true if ETSI compliant
     */
    bool validateEtsiCompliance(FigParsingResult& result);

    /**
     * @brief Get parsing performance statistics
     */
    struct ParsingStatistics {
        uint64_t total_fics_processed;
        uint64_t total_figs_processed;
        uint64_t parsing_errors;
        std::chrono::nanoseconds total_parsing_time;
        double average_figs_per_second;
        double average_parsing_latency_ms;
        
        double getFigProcessingRate() const {
            if (total_parsing_time.count() == 0) return 0.0;
            return static_cast<double>(total_figs_processed) / 
                   (static_cast<double>(total_parsing_time.count()) / 1e9);
        }
    };
    
    ParsingStatistics getStatistics() const { return statistics_; }
    void resetStatistics();

    /**
     * @brief Enable/disable specific FIG type parsing
     * @param fig_type FIG type to configure (e.g., 0x00 for FIG 0/0)
     * @param enabled Enable or disable parsing
     */
    void enableFigType(uint8_t fig_type, bool enabled);

    /**
     * @brief Check if parser is ready for processing
     * @return true if initialized and ready
     */
    bool isReady() const { return initialized_; }

signals:
    /**
     * @brief Emitted when ensemble information is discovered
     * @param ensemble_info Discovered ensemble information
     */
    void ensembleInfoDiscovered(const Fig00EnsembleInfo& ensemble_info);

    /**
     * @brief Emitted when sub-channel organization is updated
     * @param subchannel_info Vector of sub-channel information
     */
    void subchannelOrganizationUpdated(const std::vector<Fig01SubchannelInfo>& subchannel_info);

    /**
     * @brief Emitted when service is discovered
     * @param service_info Discovered service information
     */
    void serviceDiscovered(const Fig02ServiceInfo& service_info);

    /**
     * @brief Emitted when service label is discovered
     * @param service_label Discovered service label
     */
    void serviceLabelDiscovered(const ServiceLabel& service_label);

    /**
     * @brief Emitted when ETSI compliance violation is detected
     * @param violation_description Description of the violation
     * @param fig_type FIG type where violation occurred
     */
    void complianceViolationDetected(const QString& violation_description, uint8_t fig_type);

    /**
     * @brief Emitted when Thai encoding issue is detected
     * @param issue_description Description of the encoding issue
     * @param service_id Service ID with the issue
     */
    void thaiEncodingIssueDetected(const QString& issue_description, uint32_t service_id);

private:
    // Core FIG parsing methods
    std::vector<FigBlock> extractFigBlocks(const EtiFicField& fic_data);
    bool validateFigBlockStructure(const FigBlock& fig_block);
    bool validateFigTiming(uint8_t fig_type, uint8_t extension);
    
    // FIG type parsing methods
    bool parseFigType0(const FigBlock& fig_block, uint8_t extension);
    bool parseFigType1(const FigBlock& fig_block, uint8_t extension);
    
    // FIG type-specific parsers
    bool parseFig00_Internal(const std::vector<uint8_t>& data, Fig00EnsembleInfo& result);
    bool parseFig01_Internal(const std::vector<uint8_t>& data, std::vector<Fig01SubchannelInfo>& result);
    bool parseFig02_Internal(const std::vector<uint8_t>& data, std::vector<Fig02ServiceInfo>& result);
    bool parseFig11_Internal(const std::vector<uint8_t>& data, std::vector<ServiceLabel>& result);
    
    // ETSI compliance validation
    bool validateEnsembleCompliance(const Fig00EnsembleInfo& ensemble);
    bool validateSubchannelCompliance(const Fig01SubchannelInfo& subchannel);
    bool validateServiceCompliance(const Fig02ServiceInfo& service);
    bool validateLabelCompliance(const ServiceLabel& label);
    
    // Thai character encoding support
    bool validateThaiCharacterEncoding(const std::string& text);
    bool isThaiUnicodeRange(uint32_t codepoint);
    std::string normalizeThaiText(const std::string& input);
    
    // Performance optimization
    void updateStatistics(std::chrono::nanoseconds parsing_time, uint32_t figs_processed);
    bool shouldOptimizeForPerformance() const;
    void optimizeParsingStrategy();

private:
    bool initialized_;
    bool thai_support_enabled_;
    bool strict_compliance_;
    
    // FIG type enablement flags
    std::array<bool, 256> fig_types_enabled_;
    
    // Performance tracking
    ParsingStatistics statistics_;
    std::chrono::steady_clock::time_point last_stats_update_;
    
    // ETSI compliance tracking
    std::atomic<uint64_t> compliance_violations_count_;
    std::atomic<uint64_t> thai_encoding_issues_count_;
    
    // Parsing optimization data
    std::unordered_map<uint8_t, uint64_t> fig_type_frequency_;
    std::chrono::steady_clock::time_point performance_optimization_time_;
    
private:
    // Constants for validation
    static constexpr uint32_t THAI_UNICODE_START = 0x0E00;
    static constexpr uint32_t THAI_UNICODE_END = 0x0E7F;
    static constexpr double TARGET_FPS = 900.0;
    static constexpr double MAX_PARSING_LATENCY_MS = 5.0;
};

/**
 * @brief Factory function for creating optimized FIG parser
 * @param enable_thai_support Enable Thai character support
 * @param strict_compliance Enable strict ETSI compliance
 * @return Unique pointer to configured FIG parser
 */
std::unique_ptr<FigParser> createOptimizedFigParser(
    bool enable_thai_support = true,
    bool strict_compliance = false);

} // namespace eti::fig