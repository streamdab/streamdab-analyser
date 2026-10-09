/**
 * @file enhanced_fig_analyser.h
 * @brief Enhanced FIG Analyser with optimized algorithms
 * 
 * Advanced FIG (Fast Information Group) processing with improved performance
 * and comprehensive ETSI compliance validation. Part of Phase 2 migration
 * to integrated C++20 architecture.
 * 
 * Key improvements over ETISnoop FIGalyser:
 * - Modern C++20 algorithms and data structures
 * - Optimized FIG parsing with zero-copy where possible
 * - Enhanced error detection and recovery
 * - Real-time FIG carousel analysis
 * - Thai DAB standards support
 * - Comprehensive ETSI compliance validation
 */

#pragma once

#include "eti_types.hpp"
#include <QObject>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <shared_mutex>
#include <span>
#include <chrono>
#include <memory>
#include <atomic>

namespace eti::modern {

/**
 * @brief FIG processing statistics and metrics
 */
struct FIGProcessingStats {
    uint64_t total_figs_processed{0};
    uint64_t fig_type_0_count{0};
    uint64_t fig_type_1_count{0};
    uint64_t fig_type_2_count{0};
    uint64_t invalid_figs{0};
    uint64_t crc_errors{0};
    uint64_t parsing_errors{0};
    
    // Performance metrics
    uint64_t total_processing_time_ns{0};
    uint64_t peak_processing_time_ns{0};
    double average_processing_time_ns{0.0};
    
    // ETSI compliance
    uint32_t etsi_violations{0};
    double compliance_score{100.0};
};

/**
 * @brief FIG carousel analysis result
 */
struct FIGCarouselAnalysis {
    struct FIGOccurrence {
        uint8_t fig_type{0};
        uint8_t extension{0};
        std::chrono::steady_clock::time_point last_seen;
        uint32_t occurrence_count{0};
        std::chrono::milliseconds average_interval{std::chrono::milliseconds::zero()};
        std::chrono::milliseconds min_interval{std::chrono::milliseconds::max()};
        std::chrono::milliseconds max_interval{std::chrono::milliseconds::zero()};
        bool meets_etsi_timing{false};
    };
    
    std::unordered_map<uint16_t, FIGOccurrence> fig_occurrences; // key: (type << 8) | extension
    std::chrono::steady_clock::time_point analysis_start;
    std::chrono::milliseconds analysis_duration;
    double carousel_efficiency;
    std::vector<std::string> timing_violations;
    
    // Additional members required by implementation
    std::unordered_map<uint16_t, std::chrono::steady_clock::time_point> fig_intervals;
    std::chrono::steady_clock::time_point last_update;
    double fig_repetition_rate{0.0};
    uint64_t total_fig_count{0};
    
    bool is_carousel_compliant() const;
    std::vector<std::string> get_compliance_issues() const;
};

/**
 * @brief Service information with enhanced metadata
 */
struct EnhancedServiceInfo {
    ::eti::DabService base_service;
    
    // Enhanced metadata
    std::optional<std::string> thai_label;
    std::optional<std::string> programme_type_name;
    std::optional<std::string> language_name;
    std::chrono::steady_clock::time_point first_discovered;
    std::chrono::steady_clock::time_point last_updated;
    
    // Quality metrics
    double signal_quality{1.0};
    uint32_t error_count{0};
    bool is_stable{true};
    
    // Component analysis
    struct ComponentAnalysis {
        uint8_t sub_channel_id{0};
        std::string component_type_name;
        std::optional<std::string> audio_mode; // For audio components
        std::optional<uint32_t> bitrate; // For audio components
        bool is_protected{false};
        uint8_t protection_level{0};
    };
    std::vector<ComponentAnalysis> component_details;
    
    // Additional components member for implementation compatibility
    std::vector<ComponentAnalysis> components;
    
    bool has_audio_component() const;
    bool has_data_component() const;
    std::vector<std::string> get_audio_modes() const;
};

/**
 * @brief Enhanced ensemble information
 */
struct EnhancedEnsembleInfo {
    ::eti::Ensemble base_ensemble;
    
    // Enhanced metadata
    std::optional<std::string> thai_label;
    std::optional<std::string> provider_name;
    std::chrono::steady_clock::time_point first_discovered;
    std::chrono::steady_clock::time_point last_updated;
    
    // Analysis results
    uint32_t total_services{0};
    uint32_t audio_services{0};
    uint32_t data_services{0};
    uint32_t total_subchannels{0};
    uint32_t total_capacity_units{0};
    double capacity_utilization{0.0};
    
    // Quality metrics
    double overall_quality{1.0};
    uint32_t total_errors{0};
    bool is_stable{true};
    
    std::vector<std::string> get_quality_issues() const;
};

/**
 * @brief Enhanced FIG Analyser with improved algorithms
 * 
 * This class provides high-performance FIG processing with comprehensive
 * ETSI compliance validation and enhanced error detection capabilities.
 * Designed to replace ETISnoop FIGalyser with superior performance.
 */
class EnhancedFIGAnalyser : public QObject {
    Q_OBJECT

public:
    explicit EnhancedFIGAnalyser(QObject* parent = nullptr);
    ~EnhancedFIGAnalyser();

    /**
     * @brief Initialize the FIG analyser
     * @param enable_carousel_analysis Enable FIG carousel timing analysis
     * @param enable_thai_support Enable Thai DAB standards support
     * @return true if initialization successful
     */
    bool initialize(bool enable_carousel_analysis = true, bool enable_thai_support = true);

    /**
     * @brief Process FIG blocks from FIC data
     * @param fic_data FIC field data
     * @return Vector of processed FIG blocks
     */
    std::vector<eti::FigBlock> process_fic_data(const eti::EtiFicField& fic_data);

    /**
     * @brief Analyze single FIG block with enhanced processing
     * @param fig_block FIG block to analyze
     * @return true if processing successful
     */
    bool analyze_fig_block(const eti::FigBlock& fig_block);

    /**
     * @brief Get current ensemble information
     */
    std::optional<EnhancedEnsembleInfo> get_current_ensemble() const;

    /**
     * @brief Get all discovered services
     */
    std::vector<EnhancedServiceInfo> get_discovered_services() const;

    /**
     * @brief Get service by ID with enhanced information
     * @param service_id Service identifier
     */
    std::optional<EnhancedServiceInfo> get_service_by_id(uint16_t service_id) const;

    /**
     * @brief Get FIG carousel analysis results
     */
    FIGCarouselAnalysis getCarouselAnalysis() const;

    /**
     * @brief Get processing statistics (thread-safe copy)
     */
    FIGProcessingStats getProcessingStats() const;

    /**
     * @brief Reset analysis state and statistics
     */
    void reset();

    /**
     * @brief Enable/disable specific analysis features
     */
    void enableCarouselAnalysis(bool enable);
    void enableThaiSupport(bool enable);
    void enableErrorRecovery(bool enable);

    /**
     * @brief Get ETSI compliance score (0-100)
     */
    double getETSIComplianceScore() const;

    /**
     * @brief Get detailed compliance issues
     */
    std::vector<std::string> getComplianceIssues() const;

    /**
     * @brief Validate FIG timing according to ETSI standards
     * @param fig_type FIG type
     * @param extension FIG extension
     * @param interval Time since last occurrence
     * @return true if timing is compliant
     */
    bool validateFIGTiming(uint8_t fig_type, uint8_t extension, 
                          std::chrono::milliseconds interval) const;

signals:
    /**
     * @brief Emitted when new ensemble is discovered
     */
    void ensembleDiscovered(const EnhancedEnsembleInfo& ensemble);

    /**
     * @brief Emitted when new service is discovered
     */
    void serviceDiscovered(const EnhancedServiceInfo& service);

    /**
     * @brief Emitted when service information is updated
     */
    void serviceUpdated(const EnhancedServiceInfo& service);

    /**
     * @brief Emitted when FIG carousel analysis is complete
     */
    void carouselAnalysisComplete(const FIGCarouselAnalysis& analysis);

    /**
     * @brief Emitted when ETSI compliance issue is detected
     */
    void complianceIssue(const QString& standard, const QString& issue, 
                        const QString& severity);

    /**
     * @brief Emitted when FIG processing error occurs
     */
    void processingError(const QString& error, uint8_t fig_type, uint8_t extension);

private:
    // Core FIG processing methods
    bool processFIG0(const eti::FigBlock& fig);
    bool processFIG1(const eti::FigBlock& fig);
    bool processFIG2(const eti::FigBlock& fig);

    // FIG Type 0 processors (Multiplex Configuration Information - Complete Set)
    bool processFIG0_0_EnsembleInfo(const eti::FigBlock& fig);
    bool processFIG0_1_SubchannelOrg(const eti::FigBlock& fig);
    bool processFIG0_2_ServiceOrg(const eti::FigBlock& fig);
    bool processFIG0_3_ServiceComponent(const eti::FigBlock& fig);
    bool processFIG0_4_ServiceComponentLink(const eti::FigBlock& fig);
    bool processFIG0_5_ServiceComponentLang(const eti::FigBlock& fig);
    bool processFIG0_6_ServiceLinking(const eti::FigBlock& fig);
    bool processFIG0_7_ConfigurationInfo(const eti::FigBlock& fig);
    bool processFIG0_8_ServiceComponentGlobal(const eti::FigBlock& fig);
    bool processFIG0_9_CountryLTOInt(const eti::FigBlock& fig);
    bool processFIG0_10_DateTime(const eti::FigBlock& fig);
    bool processFIG0_11_RegionDefinition(const eti::FigBlock& fig);
    bool processFIG0_12_RESERVED(const eti::FigBlock& fig);
    bool processFIG0_13_UserAppInfo(const eti::FigBlock& fig);
    bool processFIG0_14_FECSubChannelOrg(const eti::FigBlock& fig);
    bool processFIG0_15_RESERVED(const eti::FigBlock& fig);
    bool processFIG0_16_ProgrammeNumber(const eti::FigBlock& fig);
    bool processFIG0_17_ProgrammeType(const eti::FigBlock& fig);
    bool processFIG0_18_Announcement(const eti::FigBlock& fig);
    bool processFIG0_19_AnnouncementSwitch(const eti::FigBlock& fig);
    bool processFIG0_20_ServiceComponentInfo(const eti::FigBlock& fig);
    bool processFIG0_21_FrequencyInfo(const eti::FigBlock& fig);
    bool processFIG0_22_TransmitterIdInfo(const eti::FigBlock& fig);
    bool processFIG0_23_RESERVED(const eti::FigBlock& fig);
    bool processFIG0_24_OtherEnsembleService(const eti::FigBlock& fig);
    bool processFIG0_25_OtherEnsembleAnnouncement(const eti::FigBlock& fig);
    bool processFIG0_26_OtherEnsembleFreq(const eti::FigBlock& fig);

    // FIG Type 1 processors (Labels - Complete Set)
    bool processFIG1_0_EnsembleLabel(const eti::FigBlock& fig);
    bool processFIG1_1_ServiceLabel(const eti::FigBlock& fig);
    bool processFIG1_2_RESERVED(const eti::FigBlock& fig);
    bool processFIG1_3_RESERVED(const eti::FigBlock& fig);
    bool processFIG1_4_ServiceComponentLabel(const eti::FigBlock& fig);
    bool processFIG1_5_DataServiceLabel(const eti::FigBlock& fig);
    bool processFIG1_6_XPADUserAppLabel(const eti::FigBlock& fig);
    bool processFIG1_7_RESERVED(const eti::FigBlock& fig);

    // FIG Type 2 processors (Extended Service Information)
    bool processFIG2_0_MOTChannel(const eti::FigBlock& fig);
    bool processFIG2_1_MOTService(const eti::FigBlock& fig);

    // Enhanced processing utilities
    bool validateFIGStructure(const eti::FigBlock& fig) const;
    bool validateFIGCRC(const eti::FigBlock& fig) const;
    std::string decodeDabText(std::span<const uint8_t> data, bool is_thai = false) const;
    std::string decodeThaiDabText(std::span<const uint8_t> data) const;
    
    // Carousel analysis
    void updateCarouselAnalysis(uint8_t fig_type, uint8_t extension);
    void validateCarouselTiming();
    
    // Error handling and recovery
    bool attemptErrorRecovery(const eti::FigBlock& fig);
    void reportComplianceViolation(const std::string& standard, 
                                  const std::string& issue,
                                  const std::string& severity);

    // Data management
    void updateServiceInfo(const ::eti::DabService& service);
    void updateEnsembleInfo(const ::eti::Ensemble& ensemble);
    void mergeServiceComponents(EnhancedServiceInfo& service, 
                               const ::eti::ServiceComponent& component);

    // Performance optimization
    void optimizeDataStructures();
    void cleanupStaleData();

    // Configuration (removed redundant private: label)
    bool m_carousel_analysis_enabled{true};
    bool m_thai_support_enabled{true};
    bool m_error_recovery_enabled{true};
    
    // Current state
    std::optional<EnhancedEnsembleInfo> current_ensemble_;
    std::unordered_map<uint16_t, EnhancedServiceInfo> discovered_services_;
    std::unordered_map<uint8_t, ::eti::SubChannelInfo> subchannels_;
    
    // Additional state tracking for ETSI compliance
    std::unordered_map<uint8_t, ::eti::ServiceComponent> m_serviceComponents;
    std::unordered_map<uint32_t, ::eti::DabService> m_dabServices;
    std::unordered_map<uint32_t, std::vector<std::string>> m_userApplications;
    
    // Carousel analysis data
    FIGCarouselAnalysis carousel_analysis_;
    std::chrono::steady_clock::time_point analysis_start_time_;
    
    // Performance tracking
    FIGProcessingStats stats_;
    std::chrono::steady_clock::time_point last_stats_update_;
    
    // ETSI compliance tracking
    std::vector<std::string> compliance_issues_;
    std::unordered_set<std::string> reported_issues_; // Avoid duplicate reports
    
    // Thread safety
    mutable std::shared_mutex data_mutex_;
    
    // Constants for ETSI timing validation
    static constexpr std::chrono::milliseconds FIG_0_0_MAX_INTERVAL{96}; // 4 transmission frames
    static constexpr std::chrono::milliseconds FIG_0_1_MAX_INTERVAL{96};
    static constexpr std::chrono::milliseconds FIG_0_2_MAX_INTERVAL{96};
    static constexpr std::chrono::milliseconds FIG_1_0_MAX_INTERVAL{5000}; // 5 seconds
    static constexpr std::chrono::milliseconds FIG_1_1_MAX_INTERVAL{5000};
};

} // namespace eti::modern