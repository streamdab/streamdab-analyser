/**
 * @file thai_nbtc_validator.h
 * @brief Thai NBTC Compliance Validator for Regional Broadcasting Standards
 * 
 * Implements comprehensive validation for Thai National Broadcasting and
 * Telecommunications Commission (NBTC) requirements for DAB broadcasting.
 * 
 * Thai NBTC Requirements:
 * - Frequency plan compliance (Band III: 174-240 MHz)
 * - Thai character encoding support (UTF-8 with Thai Unicode)
 * - Content classification and guidelines
 * - Emergency alert system integration
 * - Service labeling in Thai language
 * - Broadcast license compliance
 * 
 * Reference Standards:
 * - NBTC Notification on DAB Broadcasting Standards
 * - Thai Frequency Allocation Table 2019
 * - ETSI EN 300 401 with Thai adaptations
 * 
 * @author Standards Compliance Agent
 * @date September 22, 2025
 * @copyright StreamDAB Analyser Project
 */

#pragma once

#include "eti_types.hpp"
#include "dab_service_discovery_engine.hpp"
#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace eti::thai {

/**
 * @brief Thai DAB Frequency Plan Information
 */
struct ThaiFrequencyInfo {
    double frequency_mhz;                   // Frequency in MHz
    uint8_t channel_number;                 // DAB channel number
    std::string channel_name;               // Channel name (e.g., "12C")
    bool nbtc_approved;                     // NBTC approved frequency
    std::string license_category;           // License category
    
    static const std::vector<ThaiFrequencyInfo> THAI_DAB_CHANNELS;
    
    bool isValidThaiFrequency() const {
        return frequency_mhz >= 174.0 && frequency_mhz <= 240.0 && nbtc_approved;
    }
};

/**
 * @brief Thai Content Classification
 */
enum class ThaiContentCategory {
    NEWS_INFORMATION = 0,                   // ข่าวสารและข้อมูล
    EDUCATION_CULTURE = 1,                  // การศึกษาและวัฒนธรรม
    ENTERTAINMENT = 2,                      // บันเทิง
    MUSIC = 3,                             // ดนตรี
    SPORTS = 4,                            // กีฬา
    RELIGIOUS = 5,                         // ศาสนา
    CHILDREN = 6,                          // เด็กและเยาวชน
    GOVERNMENT = 7,                        // ราชการ
    COMMERCIAL = 8,                        // พาณิชย์
    EMERGENCY = 9                          // ฉุกเฉิน
};

/**
 * @brief Thai Language Support Information
 */
struct ThaiLanguageSupport {
    bool supports_thai_characters;         // Thai Unicode support
    bool supports_thai_numerals;           // Thai numeral support
    bool supports_thai_punctuation;        // Thai punctuation
    bool supports_mixed_script;            // Thai-English mixed script
    std::string encoding_format;           // Character encoding (UTF-8)
    std::vector<std::string> font_requirements; // Font requirements
    
    bool isFullyCompliant() const {
        return supports_thai_characters && supports_thai_numerals && 
               supports_thai_punctuation && (encoding_format == "UTF-8");
    }
};

/**
 * @brief Thai NBTC Compliance Result
 */
struct ThaiNbtcComplianceResult {
    bool overall_compliant;                 // Overall NBTC compliance
    double compliance_score;                // Compliance score (0-100%)
    
    // Specific compliance areas
    bool frequency_plan_compliant;          // Frequency plan compliance
    bool character_encoding_compliant;      // Character encoding compliance
    bool content_guidelines_compliant;      // Content guidelines compliance
    bool emergency_system_compliant;        // Emergency system compliance
    bool license_requirements_compliant;    // License requirements compliance
    bool service_labeling_compliant;        // Service labeling compliance
    
    // Detailed results
    ThaiFrequencyInfo frequency_info;       // Frequency information
    ThaiLanguageSupport language_support;   // Language support details
    ThaiContentCategory content_category;   // Content classification
    
    // Issues and recommendations
    std::vector<std::string> compliance_violations;
    std::vector<std::string> compliance_warnings;
    std::vector<std::string> nbtc_recommendations;
    
    // Certification status
    bool ready_for_nbtc_submission;         // Ready for NBTC submission
    std::string certification_level;        // Certification level
    QDateTime last_validation;              // Last validation time
    
    ThaiNbtcComplianceResult() : overall_compliant(false), compliance_score(0.0),
                                frequency_plan_compliant(false), character_encoding_compliant(false),
                                content_guidelines_compliant(false), emergency_system_compliant(false),
                                license_requirements_compliant(false), service_labeling_compliant(false),
                                content_category(ThaiContentCategory::NEWS_INFORMATION),
                                ready_for_nbtc_submission(false), certification_level("None") {}
    
    QString getComplianceSummary() const;
    QStringList getAllViolations() const;
    bool isReadyForBroadcast() const { return overall_compliant && ready_for_nbtc_submission; }
};

/**
 * @brief Thai NBTC Service Requirements
 */
struct ThaiServiceRequirements {
    bool requires_thai_labeling;            // Thai language labeling required
    bool requires_content_classification;   // Content classification required
    bool requires_emergency_capability;     // Emergency broadcast capability
    bool requires_cultural_sensitivity;     // Cultural sensitivity compliance
    uint32_t minimum_thai_content_percentage; // Minimum Thai content percentage
    std::vector<std::string> prohibited_content; // Prohibited content types
    std::vector<std::string> required_disclaimers; // Required disclaimers
    
    static ThaiServiceRequirements getRequirementsForCategory(ThaiContentCategory category);
};

/**
 * @brief Comprehensive Thai NBTC Compliance Validator
 * 
 * Validates DAB services and ensembles against Thai NBTC requirements
 * with comprehensive coverage of all regulatory aspects.
 */
class ThaiNbtcValidator : public QObject {
    Q_OBJECT

public:
    explicit ThaiNbtcValidator(QObject* parent = nullptr);
    ~ThaiNbtcValidator();

    /**
     * @brief Initialize the Thai NBTC validator
     * @param strict_mode Enable strict NBTC compliance checking
     * @return true if initialization successful
     */
    bool initialize(bool strict_mode = false);

    /**
     * @brief Validate ensemble against Thai NBTC requirements
     * @param ensemble Ensemble to validate
     * @return Thai NBTC compliance result
     */
    ThaiNbtcComplianceResult validateEnsemble(const discovery::EnhancedEnsemble& ensemble);

    /**
     * @brief Validate service against Thai NBTC requirements
     * @param service Service to validate
     * @return Thai NBTC compliance result
     */
    ThaiNbtcComplianceResult validateService(const discovery::EnhancedDabService& service);

    /**
     * @brief Validate frequency plan compliance
     * @param frequency_mhz Frequency in MHz
     * @return true if frequency is NBTC approved
     */
    bool validateFrequencyPlan(double frequency_mhz);

    /**
     * @brief Validate Thai character encoding
     * @param text Text to validate
     * @return true if text properly supports Thai characters
     */
    bool validateThaiCharacterEncoding(const std::string& text);

    /**
     * @brief Validate content classification
     * @param service Service to classify
     * @return Content category and compliance status
     */
    std::pair<ThaiContentCategory, bool> validateContentClassification(
        const discovery::EnhancedDabService& service);

    /**
     * @brief Generate NBTC compliance report
     * @param results Vector of compliance results
     * @return Formatted NBTC compliance report
     */
    QString generateNbtcComplianceReport(const std::vector<ThaiNbtcComplianceResult>& results);

    /**
     * @brief Export compliance data for NBTC submission
     * @param results Vector of compliance results
     * @param format Export format ("xml", "json", "pdf")
     * @return Formatted compliance data for submission
     */
    QByteArray exportNbtcSubmissionData(const std::vector<ThaiNbtcComplianceResult>& results,
                                       const QString& format = "xml");

    /**
     * @brief Get Thai frequency plan information
     * @return Vector of all Thai DAB frequency information
     */
    std::vector<ThaiFrequencyInfo> getThaiFrequencyPlan() const;

    /**
     * @brief Check if validator is ready
     * @return true if initialized and ready for validation
     */
    bool isReady() const { return initialized_; }

    /**
     * @brief Enable/disable strict NBTC compliance mode
     * @param enabled Strict mode flag
     */
    void setStrictMode(bool enabled) { strict_mode_ = enabled; }

signals:
    /**
     * @brief Emitted when NBTC compliance violation is detected
     * @param violation_type Type of violation
     * @param description Violation description
     * @param service_id Service ID where violation occurred
     */
    void nbtcViolationDetected(const QString& violation_type, const QString& description, 
                              uint32_t service_id);

    /**
     * @brief Emitted when Thai character encoding issue is detected
     * @param issue_description Description of encoding issue
     * @param service_id Service ID with the issue
     */
    void thaiEncodingIssueDetected(const QString& issue_description, uint32_t service_id);

    /**
     * @brief Emitted when content classification is updated
     * @param service_id Service ID
     * @param category Content category
     * @param compliant Compliance status
     */
    void contentClassificationUpdated(uint32_t service_id, ThaiContentCategory category, 
                                    bool compliant);

private:
    // Core validation methods
    bool validateFrequencyCompliance(double frequency_mhz, ThaiNbtcComplianceResult& result);
    bool validateCharacterEncodingCompliance(const std::string& text, ThaiNbtcComplianceResult& result);
    bool validateContentGuidelinesCompliance(const discovery::EnhancedDabService& service, 
                                           ThaiNbtcComplianceResult& result);
    bool validateEmergencySystemCompliance(const discovery::EnhancedDabService& service, 
                                         ThaiNbtcComplianceResult& result);
    bool validateLicenseRequirements(const discovery::EnhancedDabService& service, 
                                   ThaiNbtcComplianceResult& result);
    bool validateServiceLabelingCompliance(const discovery::EnhancedDabService& service, 
                                         ThaiNbtcComplianceResult& result);
    
    // Thai character validation
    bool isThaiUnicodeCharacter(uint32_t codepoint);
    bool isThaiNumeral(uint32_t codepoint);
    bool isThaiPunctuation(uint32_t codepoint);
    std::string normalizeThaiText(const std::string& input);
    
    // Content classification
    ThaiContentCategory classifyServiceContent(const discovery::EnhancedDabService& service);
    bool validateContentForCategory(const discovery::EnhancedDabService& service, 
                                  ThaiContentCategory category);
    
    // NBTC specific validation
    bool checkNbtcFrequencyApproval(double frequency_mhz);
    bool checkNbtcLicenseRequirements(const discovery::EnhancedDabService& service);
    bool checkCulturalSensitivityCompliance(const discovery::EnhancedDabService& service);
    
    // Compliance scoring
    double calculateComplianceScore(const ThaiNbtcComplianceResult& result);
    void updateComplianceStatistics(const ThaiNbtcComplianceResult& result);

private:
    bool initialized_;
    bool strict_mode_;
    
    // Thai frequency plan data
    std::vector<ThaiFrequencyInfo> thai_frequency_plan_;
    std::unordered_map<double, ThaiFrequencyInfo> frequency_lookup_;
    
    // Content classification rules
    std::unordered_map<ThaiContentCategory, ThaiServiceRequirements> content_requirements_;
    
    // Validation statistics
    struct ValidationStatistics {
        uint64_t total_validations;
        uint64_t compliant_services;
        uint64_t frequency_violations;
        uint64_t encoding_violations;
        uint64_t content_violations;
        double average_compliance_score;
        
        double getComplianceRate() const {
            return total_validations > 0 ? 
                   (static_cast<double>(compliant_services) / total_validations) * 100.0 : 0.0;
        }
    } statistics_;
    
    // Thai Unicode ranges
    static constexpr uint32_t THAI_UNICODE_START = 0x0E00;
    static constexpr uint32_t THAI_UNICODE_END = 0x0E7F;
    static constexpr uint32_t THAI_EXTENDED_A_START = 0xAA80;
    static constexpr uint32_t THAI_EXTENDED_A_END = 0xAADF;
    
    // NBTC compliance thresholds
    static constexpr double NBTC_MINIMUM_COMPLIANCE_SCORE = 85.0;
    static constexpr double NBTC_CERTIFICATION_THRESHOLD = 95.0;
};

/**
 * @brief Factory function for creating Thai NBTC validator
 * @param strict_mode Enable strict NBTC compliance
 * @return Unique pointer to configured Thai NBTC validator
 */
std::unique_ptr<ThaiNbtcValidator> createThaiNbtcValidator(bool strict_mode = false);

/**
 * @brief Utility functions for Thai compliance
 */
namespace thai_utils {
    /**
     * @brief Convert Thai content category to string
     * @param category Content category
     * @return Thai and English category names
     */
    std::pair<QString, QString> contentCategoryToString(ThaiContentCategory category);
    
    /**
     * @brief Check if frequency is in Thai DAB band
     * @param frequency_mhz Frequency in MHz
     * @return true if in Thai DAB band (174-240 MHz)
     */
    bool isThaiDabFrequency(double frequency_mhz);
    
    /**
     * @brief Validate Thai text encoding
     * @param text Text to validate
     * @return true if properly encoded Thai text
     */
    bool isValidThaiText(const std::string& text);
    
    /**
     * @brief Get NBTC frequency information
     * @param frequency_mhz Frequency in MHz
     * @return Frequency information or empty if not found
     */
    ThaiFrequencyInfo getNbtcFrequencyInfo(double frequency_mhz);
}

} // namespace eti::thai