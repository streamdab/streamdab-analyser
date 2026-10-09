/**
 * @file comprehensive_etsi_validator.h
 * @brief Comprehensive ETSI Standards Compliance Validator
 *
 * Validates ETI streams against all 9 ETSI standards for complete broadcast
 * industry compliance. Part of the Modern ETI Core Engine migration.
 *
 * Supported ETSI Standards:
 * 1. ETSI EN 302 077 - Harmonized Radio Standard
 * 2. ETSI EN 300 401 - Digital Audio Broadcasting (DAB)
 * 3. ETSI TS 102 563 - Digital Audio Broadcasting Plus (DAB+)
 * 4. ETSI TS 101 756 - Registered Tables for DAB
 * 5. ETSI TR 101 496 - Network Guidelines for DAB/DAB+
 * 6. ETSI TS 101 499 - SlideShow Application
 * 7. ETSI TS 102 818 - Service Programme Information (SPI)
 * 8. ETSI TS 103 551 - Transport Protocol Experts Group (TPEG)
 * 9. ETSI TS 103 176 - Service and Programme Information (SI)
 *
 * @author Standards Compliance Agent
 * @date 2025
 * @copyright StreamDAB Analyser Project
 */

#pragma once

#include "eti_types.hpp"
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <memory>
#include <vector>
#include <chrono>
#include <atomic>

namespace eti::compliance {

/**
 * @brief Individual ETSI standard compliance result
 */
struct StandardComplianceResult {
    QString standard_name;
    QString standard_number;
    bool is_compliant{false};
    double compliance_score{0.0}; // 0-100%
    std::vector<std::string> violations;
    std::vector<std::string> warnings;
    std::vector<std::string> recommendations;
    std::chrono::steady_clock::time_point validation_time;

    StandardComplianceResult() = default;
    // Fixed: Use explicit parameter names to prevent swapping
    explicit StandardComplianceResult(const QString& standard_name_param, const QString& standard_number_param)
        : standard_name(standard_name_param), standard_number(standard_number_param) {}
};

/**
 * @brief Overall ETSI compliance assessment
 */
struct ComprehensiveComplianceResult {
    std::vector<StandardComplianceResult> standard_results;
    double overall_compliance_score{0.0}; // Weighted average
    bool meets_broadcast_requirements{false};
    bool ready_for_production{false};

    // Performance impact
    std::chrono::nanoseconds validation_time{0};
    bool validation_affects_performance{false};

    // Summary statistics
    static constexpr uint32_t TOTAL_ETSI_STANDARDS = 9; // EN 300 401, EN 302 077, TS 102 563, etc.
    uint32_t total_violations{0};
    uint32_t total_warnings{0};
    uint32_t compliant_standards{0};
    uint32_t total_standards{TOTAL_ETSI_STANDARDS};

    // Government compliance (Thai NBTC requirements)
    bool thai_government_compliant{false};
    QStringList thai_compliance_issues;

    QString get_compliance_summary() const;
    QStringList get_all_violations() const;
    bool is_production_ready() const { return ready_for_production; }
};

/**
 * @brief Thai DAB Government Compliance Validator
 */
class ThaiGovernmentComplianceValidator {
public:
    struct ThaiComplianceResult {
        bool nbtc_approved{false};
        bool frequency_plan_compliant{false};
        bool content_guidelines_met{false};
        bool character_encoding_valid{false};
        bool emergency_broadcast_capable{false};

        QStringList compliance_issues;
        QStringList recommendations;
        double overall_score{0.0};

        bool isFullyCompliant() const {
            return nbtc_approved && frequency_plan_compliant &&
                   content_guidelines_met && character_encoding_valid;
        }
    };

    // TODO: Methods with eti:: types disabled (no validated caller; Ensemble/DabService
    // schema no longer carries frequency/capability fields — see E-0.1 report)
    /*
    static ThaiComplianceResult validateThaiRequirements(
        const eti::Ensemble& ensemble,
        const std::vector<eti::DabService>& services);
    */

    static bool validateThaiCharacterEncoding(const std::string& text);
    static bool validateThaiFrequencyPlan(uint32_t frequency);

    // static bool validateThaiContentGuidelines(const eti::DabService& service);  // disabled: see above
};

/**
 * @brief Comprehensive ETSI Standards Compliance Validator
 *
 * Validates ETI streams against all 9 ETSI standards with optimized
 * performance to maintain >7,482 FPS processing rate.
 */
class ComprehensiveETSIValidator : public QObject {
    Q_OBJECT

public:
    explicit ComprehensiveETSIValidator(QObject* parent = nullptr);
    ~ComprehensiveETSIValidator();

    /**
     * @brief Initialize validator with configuration
     * @param enable_thai_compliance Enable Thai government compliance checks
     * @param strict_mode Enable strict validation (may impact performance)
     * @return true if initialization successful
     */
    bool initialize(bool enable_thai_compliance = true, bool strict_mode = false);

    /**
     * @brief Validate single ETI frame against all standards
     * @param frame ETI frame to validate
     * @return Comprehensive compliance result
     */
    ComprehensiveComplianceResult validateFrame(const eti::EtiFrame& frame);

    /**
     * @brief Validate ensemble configuration
     * @param ensemble Ensemble information
     * @return Compliance result for ensemble
     *
     * TODO(MOC FIX): Method with eti:: types commented out to avoid MOC compilation errors
     */
    // StandardComplianceResult validateEnsemble(const eti::Ensemble& ensemble);

    /**
     * @brief Validate service configuration
     * @param service DAB service information
     * @return Compliance result for service
     *
     * TODO(MOC FIX): Method with eti:: types commented out to avoid MOC compilation errors
     */
    // StandardComplianceResult validateService(const eti::DabService& service);

    /**
     * @brief Validate FIG block compliance
     * @param fig_block FIG block to validate
     * @return Compliance result for FIG
     *
     * TODO(MOC FIX): Method with eti:: types commented out to avoid MOC compilation errors
     */
    // StandardComplianceResult validateFIGBlock(const eti::FigBlock& fig_block);

    /**
     * @brief Get cumulative compliance statistics
     */
    struct ComplianceStatistics {
        uint64_t frames_validated{0};
        uint64_t compliant_frames{0};
        uint64_t total_violations{0};
        double average_compliance_score{0.0};
        std::chrono::milliseconds total_validation_time{0};

        // Per-standard statistics
        std::array<uint64_t, 9> standard_violations{};
        std::array<double, 9> standard_scores{};

        double getComplianceRate() const {
            return frames_validated > 0 ?
                   (static_cast<double>(compliant_frames) / frames_validated) * 100.0 : 0.0;
        }
    };

    ComplianceStatistics getStatistics() const { return statistics_; }

    /**
     * @brief Reset compliance statistics
     */
    void resetStatistics();

    /**
     * @brief Enable/disable specific standards validation
     */
    void enableStandard(const QString& standard_number, bool enable);
    void enableThaiCompliance(bool enable);
    void enableStrictMode(bool enable);

    /**
     * @brief Get detailed compliance report
     * @return Comprehensive compliance report as formatted text
     */
    QString generateComplianceReport() const;

    /**
     * @brief Export compliance data for regulatory submission
     * @param format Export format ("xml", "json", "csv")
     * @return Formatted compliance data
     */
    QByteArray exportComplianceData(const QString& format = "xml") const;

signals:
    /**
     * @brief Emitted when compliance violation is detected
     * @param standard ETSI standard number
     * @param violation Violation description
     * @param severity Severity level (Critical, Major, Minor, Warning)
     */
    void complianceViolation(const QString& standard, const QString& violation,
                           const QString& severity);

    /**
     * @brief Emitted when frame fails critical compliance check
     * @param frame_number Frame number
     * @param violations List of critical violations
     */
    void criticalComplianceFailure(uint32_t frame_number, const QStringList& violations);

    /**
     * @brief Emitted when Thai government compliance status changes
     * @param compliant Current compliance status
     * @param issues List of compliance issues
     */
    void thaiComplianceUpdate(bool compliant, const QStringList& issues);

private:
    // Private validation helpers using eti:: types

    // Individual standard validators
    StandardComplianceResult validateEN302077_Harmonized(const eti::EtiFrame& frame);
    StandardComplianceResult validateEN300401_DAB(const eti::EtiFrame& frame);
    StandardComplianceResult validateTS102563_DABPlus(const eti::EtiFrame& frame);
    StandardComplianceResult validateTS101756_RegisteredTables(const eti::EtiFrame& frame);
    StandardComplianceResult validateTR101496_NetworkGuidelines(const eti::EtiFrame& frame);
    StandardComplianceResult validateTS101499_SlideShow(const eti::EtiFrame& frame);
    StandardComplianceResult validateTS102818_SPI_XML(const eti::EtiFrame& frame);
    StandardComplianceResult validateTS103551_TPEG(const eti::EtiFrame& frame);
    StandardComplianceResult validateTS103176_ServiceInfo(const eti::EtiFrame& frame);

    // Detailed validation methods
    bool validateETIFrameStructure(const eti::EtiFrame& frame);
    bool validateSyncPattern(const eti::EtiFrame& frame);
    bool validateLIDATAField(const eti::EtiFrame& frame);
    bool validateFICStructure(const eti::EtiFrame& frame);
    bool validateMSCStructure(const eti::EtiFrame& frame);
    bool validateCRCIntegrity(const eti::EtiFrame& frame);

    // FIG-specific validation (not yet implemented)
    /*
    bool validateFIGTiming(uint8_t fig_type, uint8_t extension);
    bool validateFIGCarousel(const std::vector<eti::FigBlock>& figs);
    bool validateFIGContent(const eti::FigBlock& fig);
    */

    // Specific FIG type validators
    bool validateFIG0_3_ServiceComponent(const eti::FigBlock& fig);
    bool validateFIG0_4_ServiceComponentLinking(const eti::FigBlock& fig);
    bool validateFIG0_5_ServiceComponentLanguage(const eti::FigBlock& fig);

    // Service validation (not yet implemented)
    /*
    bool validateServiceConfiguration(const eti::DabService& service);
    bool validateAudioServiceCompliance(const eti::DabService& service);
    bool validateDataServiceCompliance(const eti::DabService& service);
    */

    // Performance optimization
    void updateStatistics(const ComprehensiveComplianceResult& result);
    bool shouldSkipValidation(const eti::EtiFrame& frame) const;
    // void optimizeValidationSequence();  // Not implemented

    // CRC calculation utilities (no eti:: types)
    uint16_t calculateCRC16(const uint8_t* data, size_t length);

private:
    bool initialized_{false};
    bool thai_compliance_enabled_{true};
    bool strict_mode_{false};

    // Standard enablement flags
    std::array<bool, 9> standards_enabled_;

    // Performance tracking
    ComplianceStatistics statistics_;
    std::chrono::steady_clock::time_point last_stats_update_;

    // Thai compliance validator
    std::unique_ptr<ThaiGovernmentComplianceValidator> thai_validator_;

    // Optimization data
    std::atomic<uint64_t> frames_processed_{0};
    std::atomic<uint64_t> validation_time_ns_{0};

    // Cached validation results for performance
    struct ValidationCache {
        std::unordered_map<uint32_t, StandardComplianceResult> frame_cache;
        std::chrono::steady_clock::time_point last_cleanup;
        size_t max_cache_size{1000};

        void cleanup();
        bool has_result(uint32_t frame_hash) const;
        void store_result(uint32_t frame_hash, const StandardComplianceResult& result);
    } validation_cache_;

    // Standard weights for overall score calculation
    static constexpr std::array<double, 9> STANDARD_WEIGHTS = {
        0.20, // EN 302 077 - Harmonized (Critical)
        0.25, // EN 300 401 - DAB Core (Critical)
        0.15, // TS 102 563 - DAB+ (Important)
        0.10, // TS 101 756 - Registered Tables
        0.10, // TR 101 496 - Network Guidelines
        0.05, // TS 101 499 - SlideShow
        0.05, // TS 102 818 - SPI
        0.05, // TS 103 551 - TPEG
        0.05  // TS 103 176 - Service Info
    };
};

/**
 * @brief Factory function for creating optimized validator
 */
std::unique_ptr<ComprehensiveETSIValidator> createETSIValidator(
    bool enable_thai_compliance = true,
    bool strict_mode = false);

} // namespace eti::compliance
