#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <memory>
#include <concepts>
#include <ranges>
#include <algorithm>
#include "eti_types.hpp"
#include "modern_eti_frame_parser.hpp"
#include "enhanced_fig_analyser.hpp"
#include "performance_profiler.hpp"

/**
 * @class EtiProcessor
 * @brief Core ETI stream processing engine
 *
 * Handles ETI frame parsing, FIG analysis, and audio extraction
 * following ETSI standards for broadcast industry compliance.
 */
class EtiProcessor : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit EtiProcessor(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~EtiProcessor();

    /**
     * @brief Initialize the ETI processor
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * @brief Process ETI data from file using ETISnoop backend
     * @param filename Path to ETI file
     * @return true if processing successful
     */
    bool process_file(const QString& filename);

    /**
     * @brief Process ETI data from buffer using ETISnoop backend
     * @param data ETI data buffer
     * @return true if processing successful
     */
    bool process_data(const QByteArray& data);

    /**
     * @brief Get Modern ETI Frame Parser instance for direct access
     * @return Modern ETI parser instance
     */
    eti::modern::ModernETIFrameParser* get_eti_parser() const { return frame_parser_.get(); }

    /**
     * @brief Get Enhanced FIG Analyser instance for direct access
     * @return Enhanced FIG analyser instance
     */
    eti::modern::EnhancedFIGAnalyser* get_fig_analyser() const { return fig_analyser_.get(); }

    /**
     * @brief Check if processor is initialized
     * @return true if ready for processing
     */
    bool is_initialized() const { return initialized_; }

    /**
     * @brief Get current frame count
     * @return Number of processed frames
     */
    quint64 get_frame_count() const { return frame_count_; }

    /**
     * @brief Get processing status
     * @return Current status string
     */
    QString get_status() const { return status_; }

    /**
     * @brief Process a single ETI frame
     * @param frameData Raw ETI frame data (6144 bytes)
     * @return true if frame was processed successfully
     */
    bool process_eti_frame(const QByteArray& frameData);

    /**
     * @brief Get current ETSI compliance status
     * @return Current compliance status structure
     *
     * TODO: Temporarily commented out due to MOC compilation errors with custom eti:: types.
     * Refactor to use Qt-compatible types (QVariantMap) or serialize to QByteArray.
     */
    // eti::ETSIComplianceResult get_compliance_status() const;

    /**
     * @brief Process FIC data for service discovery
     * @param fic_data 32-byte FIC field from ETI frame
     * @return true if FIC processing successful
     */
    bool process_fic_data(const QByteArray& fic_data);

    /**
     * @brief Get discovered services
     * @return Vector of discovered DAB services
     *
     * TODO: Temporarily commented out due to MOC compilation errors with custom eti:: types.
     * Refactor to use Qt-compatible types or serialize to QByteArray.
     */
    // std::vector<eti::DabService> get_discovered_services() const { return discovered_services_; }

    // ============================================================================
    // C++20 RANGES-BASED DATA PROCESSING METHODS
    // ============================================================================

    /**
     * @brief Get active DAB services using C++20 ranges
     * @return Vector of active services filtered using ranges
     *
     * Uses ranges to filter services where is_programme is true and
     * service has at least one primary component.
     *
     * TODO: Temporarily commented out due to MOC compilation errors with custom eti:: types.
     * Refactor to use Qt-compatible types or serialize to QByteArray.
     */
    /*
    [[nodiscard]] auto get_active_services() -> std::vector<eti::DabService> {
        std::vector<eti::DabService> result;
        auto filtered = discovered_services_
            | std::views::filter([](const auto& service) {
                return service.is_programme && service.has_primary_component();
              });
        std::ranges::copy(filtered, std::back_inserter(result));
        return result;
    }
    */

    /**
     * @brief Get services by type using ranges and concepts
     * @tparam ServicePredicate Predicate type for filtering
     * @param predicate Function to test service type
     * @return Filtered services matching the predicate
     *
     * TODO: Temporarily commented out due to MOC compilation errors with custom eti:: types.
     * Refactor to use Qt-compatible types or serialize to QByteArray.
     */
    /*
    template<std::predicate<const eti::DabService&> ServicePredicate>
    [[nodiscard]] auto get_services_by_condition(ServicePredicate&& predicate)
        -> std::vector<eti::DabService> {
        std::vector<eti::DabService> result;
        auto filtered = discovered_services_
            | std::views::filter(std::forward<ServicePredicate>(predicate));
        std::ranges::copy(filtered, std::back_inserter(result));
        return result;
    }
    */

    /**
     * @brief Get service IDs using ranges transformation
     * @return Vector of service IDs from all discovered services
     *
     * TODO: Temporarily commented out due to MOC compilation errors with custom eti:: types.
     * Refactor to use Qt-compatible types or serialize to QByteArray.
     */
    /*
    [[nodiscard]] auto get_service_ids() -> std::vector<uint16_t> {
        std::vector<uint16_t> result;
        auto transformed = discovered_services_
            | std::views::transform([](const auto& service) { return service.service_id; });
        std::ranges::copy(transformed, std::back_inserter(result));
        return result;
    }
    */

    /**
     * @brief Get services with bitrate above threshold using ranges
     * @param min_bitrate Minimum bitrate threshold in kbps
     * @return Services with bitrate >= min_bitrate
     *
     * TODO: Temporarily commented out due to MOC compilation errors with custom eti:: types.
     * Refactor to use Qt-compatible types or serialize to QByteArray.
     */
    /*
    [[nodiscard]] auto get_high_bitrate_services(uint16_t min_bitrate)
        -> std::vector<eti::ServiceInfo> {
        std::vector<eti::ServiceInfo> result;
        // Convert to ServiceInfo for compatibility with existing tests
        auto pipeline = discovered_services_
            | std::views::filter([min_bitrate](const auto& service) {
                // Calculate actual bitrate from components
                uint16_t total_bitrate = 0;
                for (const auto& component : service.components) {
                    // Calculate bitrate based on component type and characteristics
                    if (component.primary) {
                        // Primary component typically uses higher bitrate
                        // Use component type to estimate bitrate (default 128 for primary)
                        total_bitrate += (component.component_type >= 3) ? 192 : 128;
                    } else {
                        // Secondary components use lower bitrate
                        total_bitrate += 64;
                    }
                }
                return total_bitrate >= min_bitrate;
              })
            | std::views::transform([](const auto& service) {
                return eti::ServiceInfo{
                    service.service_id,
                    service.label,
                    service.is_programme ? eti::ServiceType::DAB_PLUS_AUDIO : eti::ServiceType::DATA_SERVICE,
                    128, // Approximate bitrate
                    static_cast<uint8_t>(3) // Default protection level
                };
              });
        std::ranges::copy(pipeline, std::back_inserter(result));
        return result;
    }
    */

    /**
     * @brief Get subchannel usage statistics using ranges
     * @return [used_capacity_units, total_capacity_units] pair
     *
     * TODO: Temporarily commented out due to MOC compilation errors with custom eti:: types.
     * Refactor to use Qt-compatible types or serialize to QByteArray.
     */
    /*
    [[nodiscard]] auto get_subchannel_usage() -> std::pair<uint16_t, uint16_t> {
        uint16_t used_capacity = 0;
        auto sizes = subchannels_ | std::views::transform([](const auto& sc) { return sc.size; });
        for (const auto& size : sizes) {
            used_capacity += size;
        }
        return {used_capacity, eti::MAX_CAPACITY_UNITS};
    }
    */

    /**
     * @brief Validate all services using ranges and structured bindings
     * @return [valid_count, total_count, first_error] tuple
     *
     * TODO: Temporarily commented out due to MOC compilation errors with custom eti:: types.
     * Refactor to use Qt-compatible types or serialize to QByteArray.
     */
    /*
    [[nodiscard]] auto validate_all_services()
        -> std::tuple<size_t, size_t, std::string> {

        auto validation_results = discovered_services_
            | std::views::transform([](const auto& service) {
                return service.validate_service_id() &&
                       service.validate_country_code() &&
                       service.has_primary_component();
              });

        const size_t valid_count = std::ranges::count(validation_results, true);
        const size_t total_count = discovered_services_.size();

        // Find first invalid service for error reporting
        std::string first_error = "";
        auto invalid_service = std::ranges::find_if(discovered_services_,
            [](const auto& service) {
                return !service.validate_service_id() ||
                       !service.validate_country_code() ||
                       !service.has_primary_component();
            });

        if (invalid_service != discovered_services_.end()) {
            first_error = "Service " + std::to_string(invalid_service->service_id) + " validation failed";
        }

        return {valid_count, total_count, first_error};
    }
    */

    /**
     * @brief Get current ensemble information
     * @return Current ensemble structure
     *
     * TODO: Temporarily commented out due to MOC compilation errors with custom eti:: types.
     * Refactor to use Qt-compatible types or serialize to QByteArray.
     */
    // eti::Ensemble get_current_ensemble() const { return current_ensemble_; }

    /**
     * @brief Get subchannel organization
     * @return Vector of subchannel information
     *
     * TODO: Temporarily commented out due to MOC compilation errors with custom eti:: types.
     * Refactor to use Qt-compatible types or serialize to QByteArray.
     */
    // std::vector<eti::SubChannelInfo> get_subchannels() const { return subchannels_; }

    /**
     * @brief Get processing performance statistics
     * @return Current processing rate in FPS
     */
    double get_processing_rate() const { return average_frame_rate_; }

    /**
     * @brief Frame error statistics structure (minimal for TDD RED phase)
     */
    struct FrameErrorStatistics {
        quint64 totalFrames = 0;
        quint64 crcErrors = 0;
    };

    /**
     * @brief Reset processor state for new analysis
     */
    void reset();

    /**
     * @brief Validate ETI frame structure (exposed for TDD testing)
     * @param frameData Raw frame data to validate
     * @return true if frame is valid according to ETSI EN 300 799
     */
    bool validate_frame(const QByteArray& frameData);

private:
    // ETI frame validation methods
    bool validate_sync_pattern(const eti::EtiFrame& frame);
    bool validate_lidata_field(const eti::EtiLidataField& lidata);
    bool validate_frame_crc(const eti::EtiFrame& frame);

    // CRC-32 calculation according to ETSI EN 300 799
    uint32_t calculate_crc32(const uint8_t* data, size_t length);

    // FIC and MSC processing methods
    bool process_fic_data(const eti::EtiFicField& fic);
    bool process_msc_data(const eti::EtiMscField& msc, uint8_t subChannelCount);

    // Enhanced FIG processing methods
    void process_fig_type0(const eti::FigBlock& figBlock);
    void process_fig_type1(const eti::FigBlock& figBlock);
    void process_fig_type2(const eti::FigBlock& figBlock);

    // Specific FIG parsers for Phase 1 completion
    bool parse_fig00_ensemble_info(const eti::FigBlock& figBlock);
    bool parse_fig01_subchannel_organization(const eti::FigBlock& figBlock);
    bool parse_fig02_service_organization(const eti::FigBlock& figBlock);
    bool parse_fig03_service_component(const eti::FigBlock& figBlock);
    bool parse_fig10_ensemble_label(const eti::FigBlock& figBlock);
    bool parse_fig11_service_label(const eti::FigBlock& figBlock);

    // FIG Type 2 parsers for extended service information
    bool parse_fig20_service_component_global(const eti::FigBlock& figBlock);
    bool parse_fig21_mot_configuration(const eti::FigBlock& figBlock);
    bool parse_fig22_service_component_language(const eti::FigBlock& figBlock);
    bool parse_fig23_service_component_trigger(const eti::FigBlock& figBlock);

    // Enhanced service discovery methods
    void update_ensemble_information(const eti::Ensemble& ensemble);
    void update_service_information(const eti::DabService& service);
    void update_subchannel_information(const eti::SubChannelInfo& subchannel);

    // ETSI compliance validation methods
    bool validate_frame_compliance(const eti::EtiFrame& frame);
    bool validate_ensemble_compliance(const eti::Ensemble& ensemble);
    bool validate_service_compliance(const eti::DabService& service);

    // Performance optimization methods
    void optimize_processing_pipeline();
    void update_processing_statistics();

    // Enhanced service management methods are available in public section above

public:
    // Service quality and status indicators
    struct ServiceQuality {
        double signalStrength;
        double errorRate;
        bool isValid;
        std::chrono::system_clock::time_point lastUpdate;

        ServiceQuality() : signalStrength(0.0), errorRate(0.0), isValid(false) {}
    };

private:

    ServiceQuality get_service_quality(uint16_t serviceId) const;
    bool is_service_active(uint16_t serviceId) const;

signals:
    // Removed old frameProcessed signal - using Phase 1.1 version instead

    /**
     * @brief Emitted when error occurs
     * @param error Error message
     */
    void errorOccurred(const QString& error);

    /**
     * @brief Emitted when processing status changes
     * @param status New status
     */
    void statusChanged(const QString& status);

    // NOTE: Custom eti:: type signals commented out to avoid MOC compilation errors
    // TODO: Refactor to use QByteArray serialization when needed
    // void ensembleDiscovered(const eti::Ensemble& ensemble);
    // void serviceDiscovered(const eti::DabService& service);

    /**
     * @brief Emitted when service quality changes
     * @param serviceId Service ID
     * @param signalStrength Signal strength (0.0-1.0)
     * @param errorRate Error rate (0.0-1.0)
     */
    void serviceQualityChanged(uint16_t serviceId, double signalStrength, double errorRate);

    /**
     * @brief Signal emitted when frame processing error occurs (minimal TDD RED phase)
     * @param frameNumber Frame number where error occurred
     */
    void frameError(quint64 frameNumber);

    // NOTE: Custom eti:: type signals commented out to avoid MOC compilation errors
    // void frameProcessed(const eti::ProcessedFrame& frame);
    // void figDiscovered(const eti::FIGInfo& fig);

private slots:
    // NOTE: Modern ETI Core Engine signal handlers commented out to avoid MOC errors
    // TODO: Refactor to use QByteArray serialization for eti:: types
    // void on_modern_frame_processed(uint32_t frameNumber, const eti::EtiFrame& frame,
    //                               std::chrono::nanoseconds parseTime);
    // void on_fig_analysis_complete(uint32_t frameNumber, const eti::modern::FIGAnalysisResult& analysis);
    // void on_modern_ensemble_discovered(const eti::modern::EnhancedEnsembleInfo& ensemble);
    // void on_modern_service_discovered(const eti::modern::EnhancedServiceInfo& service);
    void on_compliance_issue_detected(const QString& standard, const QString& issue, const QString& severity);
    void on_performance_target_result(bool targetMet, double currentFps, double targetFps);

private:
    // Modern ETI Core Engine components
    std::unique_ptr<eti::modern::ModernETIFrameParser> frame_parser_;
    std::unique_ptr<eti::modern::EnhancedFIGAnalyser> fig_analyser_;
    std::unique_ptr<eti::modern::performance_profiler> profiler_;

    // Processing configuration
    eti::modern::ProcessingConfig processing_config_;

    bool initialized_;
    quint64 frame_count_;
    QString status_;
    QString current_file_;

    // Enhanced service discovery state
    // TODO: Commented out due to MOC compilation errors with custom eti:: types
    // These member variables prevent Qt MOC from properly processing this header
    // Solution: Refactor to use Qt-compatible types (QVariantMap, QByteArray serialization)
    // or move to implementation-only storage (pImpl pattern)
    // eti::Ensemble current_ensemble_;
    // std::vector<eti::DabService> discovered_services_;
    // std::vector<eti::SubChannelInfo> subchannels_;
    std::map<uint16_t, ServiceQuality> service_quality_;

    // Performance statistics
    std::chrono::high_resolution_clock::time_point processing_start_time_;
    std::chrono::high_resolution_clock::time_point last_frame_time_;
    double average_frame_rate_;
    uint64_t total_processing_time_;

    // Modern ETI Core Engine utilities
    double get_current_memory_usage() const;
    void validate_performance_targets(double fps, double memoryMB);
    void generate_performance_report(const eti::modern::ModernETIFrameParser::PerformanceStats& stats);

    // ETSI compliance tracking
    struct ComplianceStatus {
        bool etsi_en_300_799_compliant;
        bool etsi_en_300_401_compliant;
        std::vector<std::string> compliance_warnings;
        std::vector<std::string> compliance_errors;
    } compliance_status_;

    // Frame error handling and statistics
    FrameErrorStatistics error_stats_;

    // Minimal error reporting for TDD RED phase
    void report_frame_error(quint64 frameNumber);
};
