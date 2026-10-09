/**
 * @file dab_service_discovery_engine.h
 * @brief High-Performance DAB Service Discovery Engine
 * 
 * Implements comprehensive DAB service discovery and metadata extraction
 * according to ETSI EN 300 401 with optimized real-time performance.
 * 
 * Features:
 * - Complete ensemble discovery and service enumeration
 * - Real-time service metadata extraction
 * - Thai NBTC compliance for regional broadcasting
 * - High-performance processing (>900 FPS target)
 * - Service quality monitoring and statistics
 * 
 * Performance Target: Complete service discovery within 2 seconds
 * Processing Rate: >900 ETI frames/second during discovery
 * 
 * @author Standards Compliance Agent
 * @date September 22, 2025
 * @copyright StreamDAB Analyser Project
 */

#pragma once

#include "eti_types.hpp"
#include "fig_parser.hpp"
#include <QObject>
#include <QString>
#include <QDateTime>
#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <atomic>

namespace eti::discovery {

/**
 * @brief Enhanced DAB Service Information Structure
 */
struct EnhancedDabService {
    // Basic service information
    uint32_t service_id;                    // Service identifier
    std::string service_label;              // Service name/label
    uint8_t service_type;                   // Service type (audio/data)
    uint8_t country_id;                     // Country identifier
    uint8_t extended_country_code;          // Extended country code
    
    // Service components
    struct ServiceComponent {
        uint8_t component_id;               // Component identifier
        uint8_t subchannel_id;              // Associated sub-channel
        uint8_t transport_mechanism;        // Transport mechanism
        uint8_t audio_service_type;         // Audio service type
        uint16_t packet_address;            // Packet address (if applicable)
        bool primary_component;             // Primary component flag
        bool conditional_access;            // CA flag
        
        // Quality metrics
        double signal_strength;             // Signal strength (0.0-1.0)
        double error_rate;                  // Error rate (0.0-1.0)
        std::chrono::system_clock::time_point last_update;
    };
    
    std::vector<ServiceComponent> components;
    
    // Service characteristics
    bool is_audio_service;                  // Audio service flag
    bool is_data_service;                   // Data service flag
    bool is_dabplus_service;                // DAB+ service flag
    bool supports_thai_content;             // Thai content support
    bool emergency_service;                 // Emergency service flag
    bool emergency_alert_capable;           // Emergency alert capability
    bool continuous_operation;              // 24/7 operation capability
    
    // Thai NBTC specific fields
    std::string service_type_string;        // Service type string (Commercial, Public Service, etc.)
    std::string content_rating;             // Content rating (G, PG, etc.)
    std::string service_description;        // Service description
    std::string license_number;             // NBTC license number
    std::string government_authorization;   // Government authorization
    bool community_license_valid;           // Community license validity
    bool royal_authorization;               // Royal/monarchy authorization
    double signal_strength;                 // Signal strength (0.0-1.0)
    uint32_t financial_guarantee_amount;    // Financial guarantee in Baht
    uint32_t insurance_coverage_amount;     // Insurance coverage in Baht
    double transmit_power_watts;            // Transmit power in watts
    bool has_thai_content;                  // Contains Thai content flag
    
    // Service quality and status
    bool is_active;                         // Currently active
    bool is_valid;                          // Service configuration valid
    double overall_quality;                 // Overall service quality (0.0-1.0)
    std::chrono::system_clock::time_point first_discovered;
    std::chrono::system_clock::time_point last_updated;
    
    // ETSI compliance status
    bool etsi_compliant;                    // ETSI standards compliant
    bool thai_nbtc_compliant;               // Thai NBTC compliant
    std::vector<std::string> compliance_issues;
    
    EnhancedDabService() : service_id(0), service_type(0), country_id(0), 
                          extended_country_code(0), is_audio_service(false),
                          is_data_service(false), is_dabplus_service(false),
                          supports_thai_content(false), emergency_service(false),
                          emergency_alert_capable(false), continuous_operation(false),
                          community_license_valid(false), royal_authorization(false),
                          signal_strength(0.0), financial_guarantee_amount(0),
                          insurance_coverage_amount(0), transmit_power_watts(0.0),
                          has_thai_content(false), is_active(false), is_valid(false), 
                          overall_quality(0.0), etsi_compliant(false), thai_nbtc_compliant(false) {}
    
    bool hasAudioComponent() const {
        return std::any_of(components.begin(), components.end(),
            [](const ServiceComponent& comp) {
                return comp.transport_mechanism == 0; // Stream mode
            });
    }
    
    double getAverageSignalStrength() const {
        if (components.empty()) return 0.0;
        double total = 0.0;
        for (const auto& comp : components) {
            total += comp.signal_strength;
        }
        return total / components.size();
    }
};

/**
 * @brief Enhanced Ensemble Information Structure
 */
struct EnhancedEnsemble {
    uint16_t ensemble_id;                   // Ensemble identifier
    std::string ensemble_label;             // Ensemble name
    uint8_t country_id;                     // Country identifier
    uint8_t extended_country_code;          // Extended country code
    uint8_t alarm_flag;                     // Alarm flag
    uint16_t cif_count;                     // Common Interleaved Frame count
    
    // Sub-channel organization
    std::vector<::eti::SubChannelInfo> subchannels;
    
    // Services in this ensemble
    std::vector<EnhancedDabService> services;
    
    // Ensemble statistics
    uint32_t total_services;                // Total discovered services
    uint32_t audio_services;                // Audio services count
    uint32_t data_services;                 // Data services count
    uint32_t dabplus_services;              // DAB+ services count
    uint32_t thai_services;                 // Thai content services
    
    // Quality metrics
    double ensemble_quality;                // Overall ensemble quality
    std::chrono::system_clock::time_point discovery_time;
    std::chrono::system_clock::time_point last_update;
    
    // ETSI compliance
    bool etsi_compliant;                    // ETSI standards compliant
    bool thai_nbtc_compliant;               // Thai NBTC compliant
    std::vector<std::string> compliance_issues;
    
    EnhancedEnsemble() : ensemble_id(0), country_id(0), extended_country_code(0),
                        alarm_flag(0), cif_count(0), total_services(0),
                        audio_services(0), data_services(0), dabplus_services(0),
                        thai_services(0), ensemble_quality(0.0),
                        etsi_compliant(false), thai_nbtc_compliant(false) {}
    
    bool isThaiEnsemble() const {
        return country_id == 0x0E && extended_country_code == 0xE1;
    }
    
    void updateServiceStatistics() {
        total_services = static_cast<uint32_t>(services.size());
        audio_services = 0;
        data_services = 0;
        dabplus_services = 0;
        thai_services = 0;
        
        for (const auto& service : services) {
            if (service.is_audio_service) audio_services++;
            if (service.is_data_service) data_services++;
            if (service.is_dabplus_service) dabplus_services++;
            if (service.supports_thai_content) thai_services++;
        }
    }
};

/**
 * @brief Service Discovery Result Structure
 */
struct ServiceDiscoveryResult {
    bool discovery_successful;              // Discovery completed successfully
    std::chrono::milliseconds discovery_time; // Time taken for discovery
    uint32_t frames_processed;              // ETI frames processed
    uint32_t services_discovered;           // Total services discovered
    
    // Discovered data
    EnhancedEnsemble ensemble;              // Complete ensemble information
    std::vector<EnhancedDabService> services; // All discovered services
    
    // Discovery quality metrics
    double discovery_completeness;          // Discovery completeness (0.0-1.0)
    double service_quality_average;         // Average service quality
    uint32_t discovery_errors;              // Errors during discovery
    uint32_t compliance_violations;         // ETSI compliance violations
    
    // Performance metrics
    double frames_per_second;               // Processing rate during discovery
    std::chrono::nanoseconds average_frame_time; // Average frame processing time
    
    void reset() {
        discovery_successful = false;
        discovery_time = std::chrono::milliseconds::zero();
        frames_processed = 0;
        services_discovered = 0;
        ensemble = EnhancedEnsemble();
        services.clear();
        discovery_completeness = 0.0;
        service_quality_average = 0.0;
        discovery_errors = 0;
        compliance_violations = 0;
        frames_per_second = 0.0;
        average_frame_time = std::chrono::nanoseconds::zero();
    }
};

/**
 * @brief High-Performance DAB Service Discovery Engine
 * 
 * Implements comprehensive service discovery with real-time performance
 * optimizations and ETSI compliance validation.
 */
class DabServiceDiscoveryEngine : public QObject {
    Q_OBJECT

public:
    explicit DabServiceDiscoveryEngine(QObject* parent = nullptr);
    ~DabServiceDiscoveryEngine();

    /**
     * @brief Initialize the service discovery engine
     * @param enable_thai_support Enable Thai NBTC compliance
     * @param realtime_mode Enable real-time discovery mode
     * @return true if initialization successful
     */
    bool initialize(bool enable_thai_support = true, bool realtime_mode = false);

    /**
     * @brief Discover complete ensemble and services from ETI stream
     * @param eti_frames Vector of ETI frames to process
     * @return Complete service discovery result
     */
    ServiceDiscoveryResult discoverCompleteEnsemble(const std::vector<::eti::EtiFrame>& eti_frames);

    /**
     * @brief Process single ETI frame for incremental discovery
     * @param frame ETI frame to process
     * @return true if frame processed successfully
     */
    bool processFrameForDiscovery(const ::eti::EtiFrame& frame);

    /**
     * @brief Get current ensemble information
     * @return Current ensemble structure
     */
    EnhancedEnsemble getCurrentEnsemble() const { return current_ensemble_; }

    /**
     * @brief Get all discovered services
     * @return Vector of discovered services
     */
    std::vector<EnhancedDabService> getDiscoveredServices() const { return discovered_services_; }

    /**
     * @brief Get specific service by ID
     * @param service_id Service identifier
     * @return Service information or empty service if not found
     */
    EnhancedDabService getServiceById(uint32_t service_id) const;

    /**
     * @brief Check if discovery is complete
     * @return true if ensemble discovery is complete
     */
    bool isDiscoveryComplete() const { return discovery_complete_; }

    /**
     * @brief Get discovery progress percentage
     * @return Discovery progress (0.0-100.0)
     */
    double getDiscoveryProgress() const;

    /**
     * @brief Enable/disable real-time discovery mode
     * @param enabled Real-time mode flag
     */
    void setRealTimeMode(bool enabled);

    /**
     * @brief Reset discovery engine for new analysis
     */
    void resetDiscovery();

    /**
     * @brief Get discovery performance statistics
     */
    struct DiscoveryStatistics {
        uint64_t total_frames_processed;
        uint64_t total_services_discovered;
        uint64_t total_ensembles_discovered;
        std::chrono::milliseconds total_discovery_time;
        double average_discovery_fps;
        double average_discovery_completeness;
        uint32_t discovery_errors;
        uint32_t compliance_violations;
        
        double getDiscoveryRate() const {
            if (total_discovery_time.count() == 0) return 0.0;
            return static_cast<double>(total_frames_processed) / 
                   (static_cast<double>(total_discovery_time.count()) / 1000.0);
        }
    };
    
    DiscoveryStatistics getStatistics() const { return statistics_; }
    void resetStatistics();

signals:
    /**
     * @brief Emitted when ensemble information is discovered
     * @param ensemble Discovered ensemble information
     */
    void ensembleDiscovered(const EnhancedEnsemble& ensemble);

    /**
     * @brief Emitted when new service is discovered
     * @param service Discovered service information
     */
    void serviceDiscovered(const EnhancedDabService& service);

    /**
     * @brief Emitted when service information is updated
     * @param service_id Service identifier
     * @param service Updated service information
     */
    void serviceUpdated(uint32_t service_id, const EnhancedDabService& service);

    /**
     * @brief Emitted when discovery progress changes
     * @param progress Discovery progress percentage (0.0-100.0)
     */
    void discoveryProgressChanged(double progress);

    /**
     * @brief Emitted when discovery is complete
     * @param result Complete discovery result
     */
    void discoveryComplete(const ServiceDiscoveryResult& result);

    /**
     * @brief Emitted when service quality changes
     * @param service_id Service identifier
     * @param quality New quality level (0.0-1.0)
     */
    void serviceQualityChanged(uint32_t service_id, double quality);

    /**
     * @brief Emitted when discovery error occurs
     * @param error_description Error description
     * @param frame_number Frame number where error occurred
     */
    void discoveryError(const QString& error_description, uint32_t frame_number);

private slots:
    // FIG parser signal handlers
    void onEnsembleInfoDiscovered(const ::eti::fig::Fig00EnsembleInfo& ensemble_info);
    void onSubchannelOrganizationUpdated(const std::vector<::eti::fig::Fig01SubchannelInfo>& subchannel_info);
    void onServiceDiscovered(const ::eti::fig::Fig02ServiceInfo& service_info);
    void onServiceLabelDiscovered(const ::eti::fig::ServiceLabel& service_label);
    void onComplianceViolationDetected(const QString& violation_description, uint8_t fig_type);

private:
    // Core discovery methods
    bool processEnsembleInformation(const ::eti::fig::Fig00EnsembleInfo& ensemble_info);
    bool processSubchannelOrganization(const std::vector<::eti::fig::Fig01SubchannelInfo>& subchannel_info);
    bool processServiceInformation(const ::eti::fig::Fig02ServiceInfo& service_info);
    bool processServiceLabel(const ::eti::fig::ServiceLabel& service_label);
    
    // Service analysis and validation
    bool analyzeServiceComponents(EnhancedDabService& service);
    bool validateServiceConfiguration(const EnhancedDabService& service);
    bool checkThaiNbtcCompliance(const EnhancedDabService& service);
    
    // Discovery optimization
    void optimizeDiscoveryStrategy();
    void updateDiscoveryProgress();
    void updateServiceQuality(EnhancedDabService& service);
    
    // Performance monitoring
    void updateStatistics(std::chrono::milliseconds processing_time, uint32_t frames_processed);
    bool shouldOptimizeForPerformance() const;
    double calculateAverageServiceQuality() const;

private:
    bool initialized_;
    bool thai_support_enabled_;
    bool realtime_mode_;
    bool discovery_complete_;
    
    // Discovery state
    EnhancedEnsemble current_ensemble_;
    std::vector<EnhancedDabService> discovered_services_;
    std::unordered_map<uint32_t, size_t> service_index_map_;
    
    // FIG parser integration
    std::unique_ptr<::eti::fig::FigParser> fig_parser_;
    
    // Discovery progress tracking
    std::atomic<double> discovery_progress_;
    std::atomic<uint32_t> frames_processed_count_;
    std::chrono::steady_clock::time_point discovery_start_time_;
    
    // Performance tracking
    DiscoveryStatistics statistics_;
    std::chrono::steady_clock::time_point last_stats_update_;
    
    // Discovery configuration
    static constexpr double TARGET_DISCOVERY_FPS = 900.0;
    static constexpr std::chrono::seconds MAX_DISCOVERY_TIME{2};
    static constexpr double DISCOVERY_COMPLETE_THRESHOLD = 90.0; // 90% completeness
};

/**
 * @brief Factory function for creating optimized service discovery engine
 * @param enable_thai_support Enable Thai NBTC compliance
 * @param realtime_mode Enable real-time discovery mode
 * @return Unique pointer to configured discovery engine
 */
std::unique_ptr<DabServiceDiscoveryEngine> createOptimizedServiceDiscoveryEngine(
    bool enable_thai_support = true,
    bool realtime_mode = false);

} // namespace eti::discovery