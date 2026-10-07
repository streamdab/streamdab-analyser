/**
 * @file modern_eti_frame_parser.h
 * @brief Modern C++20 ETI Frame Parser - Phase 2 Migration Core Component
 *
 * This is the foundation of the integrated ETI processing engine, replacing
 * the ETISnoop wrapper approach with a modern, optimized, and Qt-integrated
 * C++20 implementation. Designed for >30% speed improvement and >40% memory reduction.
 *
 * Key Features:
 * - Zero-copy processing where possible
 * - SIMD optimization support
 * - Qt threading integration
 * - Modern C++20 concepts and constexpr evaluation
 * - Memory-efficient pooling system
 * - Real-time processing optimizations
 * - ETI-LI and ETI-NI format support (Agent 44)
 *
 * @author Standards Compliance Agent (Phase 2 Migration Lead)
 * @date 2025
 * @copyright ETI Stream Analyser Project
 */

#pragma once

#include "eti_types.hpp"
#include "analyser_settings.hpp"
#include <QObject>
#include <QByteArray>
#include <QString>
#include <QThreadPool>
#include <memory>
#include <span>

#include <concepts>
#include <vector>
#include <atomic>
#include <chrono>
#include <optional>
#include <functional>

// Forward declarations
#include "eti_memory_pool.hpp"
#include "performance_profiler.hpp"

namespace eti::modern {

/**
 * @brief Concepts for ETI processing type safety (C++20)
 */
template<typename T>
concept ETIFrameData = requires(T t) {
    { t.data() } -> std::convertible_to<const uint8_t*>;
    { t.size() } -> std::convertible_to<size_t>;
    requires t.size() == ETI_FRAME_SIZE;
};

template<typename T>
concept ETIProcessor = requires(T t, const ::eti::EtiFrame& frame) {
    { t.process(frame) } -> std::convertible_to<bool>;
    { t.validate(frame) } -> std::convertible_to<::eti::ValidationResult>;
};

/**
 * @brief ETI Parse Error Codes
 */
enum class ETIParseErrorCode {
    SUCCESS = 0,
    INVALID_SYNC,
    INVALID_SIZE,
    INVALID_NST,
    INVALID_FICF,
    INVALID_LIDATA,
    UNKNOWN_FORMAT
};

/**
 * @brief Modern ETI Frame parsing result with move semantics
 */
struct ETIParseResult {
    bool success{false};
    ::eti::EtiFrame frame;
    ::eti::ValidationResult validation;
    std::chrono::nanoseconds parse_time{0};
    uint32_t frame_number{0};
    ETIParseErrorCode error_code{ETIParseErrorCode::SUCCESS};

    // ETI-NI subchannel data (from STC fields, not FIC)
    std::vector<::eti::SubChannelInfo> subchannels_from_stc;

    // CRITICAL FIX: Store FICF flag and format from actual parse
    // This fixes service discovery for ETI-NI format where get_lidata_field()
    // assumes ETI-LI offsets and reads FICF incorrectly
    bool has_fic{false};           // FICF flag from actual parse (not get_lidata_field())
    ::eti::ETIFormat detected_format{::eti::ETIFormat::UNKNOWN};

    // Performance metrics
    struct {
        uint64_t cycles_used{0};
        uint32_t cache_hits{0};
        uint32_t cache_misses{0};
        double cpu_efficiency{0.0};
    } performance;

    ETIParseResult() = default;
    ETIParseResult(const ETIParseResult&) = delete;
    ETIParseResult(ETIParseResult&&) = default;
    ETIParseResult& operator=(const ETIParseResult&) = delete;
    ETIParseResult& operator=(ETIParseResult&&) = default;
};

/**
 * @brief Frame processing configuration for performance tuning
 */
struct ProcessingConfig {
    enum class OptimizationLevel : std::uint8_t {
        MINIMAL,     // Basic parsing only
        STANDARD,    // Full parsing with validation
        AGGRESSIVE,  // All optimizations enabled
        REALTIME     // Real-time optimized with reduced validation
    };

    OptimizationLevel optimization_level{OptimizationLevel::STANDARD};
    bool enable_simd{true};
    bool enable_caching{true};
    bool enable_threading{true};
    bool enable_zero_copy{true};
    bool enable_memory_pooling{true};
    uint32_t thread_count{0}; // 0 = auto-detect
    uint32_t cache_size_mb{64};

    // Real-time processing parameters
    std::chrono::microseconds max_frame_time{20000}; // 20ms max per frame
    double target_fps{1000.0}; // Target >1000fps for 30% improvement
    uint32_t memory_limit_mb{40}; // 40% reduction from 100MB target
};

/**
 * @brief One observed FIG instance in a frame (CLI detail wave, P1).
 *
 * Chronological FIG trace backbone: which FIG (type/ext), how long, which
 * FIB it came from and whether that FIB's CRC validated. Consumers combine
 * the entries with the parallel decoded data (labels, ensemble, subchannel /
 * component counts) to print a readable per-frame FIG trace.
 */
struct FIGTraceEntry {
    uint8_t fig_type{0};         // FIG type (0-7)
    uint8_t extension{0};        // Extension (FIG 0/1-data byte & 0x1F; label charset low bits for FIG 1)
    uint8_t length{0};           // FIG data length in bytes
    uint8_t fib_index{0};        // 1-based FIB index (1..4); 0 = raw walk / no FIB structure
    bool crc_ok{true};           // True when the carrying FIB CRC validated (or no per-FIB CRC exists)
};

/**
 * @brief One decoded FIG 0/21 (frequency information) entry (backlog P2/P3).
 *
 * RegionId/cei are kept for diagnostics; the readable trace value is the
 * carrier frequency in MHz (DAB R&M 0/1 entries, EN 300 401 8.1.8). freq_khz
 * is 0 for frequency-information entries whose R&M is not a DAB frequency
 * (FM/AM/DRM entries carry their own units — see fig0_21.cpp).
 */
struct FrequencyInfoEntry {
    uint16_t region_id{0};       // RegionId (13 bits)
    uint16_t id_field{0};        // FI Id field (EId / RDS PI / service id)
    uint8_t randm{0};            // R&M: 0/1 = DAB ensemble, 8/9 = FM RDS...
    uint32_t freq_khz{0};        // Carrier frequency in kHz (0 = n/a)
};

/**
 * @brief One decoded FIG 0/22 (geographical location) TII field (backlog P2/P3).
 *
 * Main identifiers (is_main) carry an absolute lat/lon from the coarse/fine
 * fields; sub identifiers carry TD + lat/lon offsets (EN 300 401 8.1.9). The
 * absolute position of a sub identifier is only resolvable when the matching
 * main identifier was seen earlier in the same FIG (the CLI reports the
 * offsets as-is; referential resolution is a GUI/DB concern).
 */
struct GeoCoordEntry {
    bool is_main{true};          // TII Main identifier vs Sub identifier
    uint8_t main_id{0};          // MainId / MainId of the sub field (7 bits)
    uint8_t sub_id{0};           // SubId (5 bits; 0 for main fields)
    uint16_t td{0};              // Sub identifier time delay (us)
    double latitude{0.0};        // Degrees, north positive (absolute or base)
    double longitude{0.0};       // Degrees, east positive
    int16_t lat_offset{0};       // Sub identifier latitude offset (raw)
    int16_t lon_offset{0};       // Sub identifier longitude offset (raw)
};

/**
 * @brief One decoded FIG 1/2 or FIG 1/3 label (backlog P2/P3).
 *
 * FIG 1/2 = service component label ([charset|OE|ext][SCIdS 6b|Rfu|P/D]
 * [SId 16/32][label 16][flag 2]); FIG 1/3 = data service label ([charset|OE|
 * ext][SId 32][label 16][flag 2]). Labels are charset-decoded with the same
 * pipeline as FIG 1/0/1/1 (EBU Latin / UTF-8 / TIS-620, force_charset and
 * fallback policy honoured).
 */
struct ComponentLabelInfo {
    uint8_t extension{2};        // 2 = service component label, 3 = data label
    uint32_t service_id{0};      // Full SId as read (16-bit P/D=0, 32-bit P/D=1)
    uint16_t scids{0};           // FIG 1/2 SCIdS (6 bits); 0 for FIG 1/3
    std::string label;           // Full label, UTF-8, trailing spaces trimmed
    std::string short_label;     // Mask-derived short label (when flag present)
    uint16_t character_flag{0};  // 16-bit character flag (0 when absent)
};

/**
 * @brief Minimal decode of one FIG type 2 (OTH) instance (backlog P2/P3).
 *
 * FIG type 2 carries segmented UTF-8/UCS-2 labels (ETSI TS 101 756 /
 * EN 300 401 8.1.13): [toggle|segment|rfu|ext] [identifier] [enc|segcount|rfa]
 * [char flag] [label segment bytes]. Only the first segment of UTF-8 labels
 * is surfaced readably; UCS-2 segments are marked but not converted.
 */
struct FigType2Info {
    uint8_t extension{0};        // 0 ensemble, 1 programme, 4 comp, 5 data...
    uint8_t toggle{0};           // Toggle flag
    uint8_t segment_index{0};    // Segment index (0-based)
    uint8_t rfu{0};              // Rfu / text-control
    uint32_t service_id{0};      // Identifier (EId / SId as carried)
    std::string label;           // First-segment label bytes (UTF-8 passthrough)
    bool ucs2{false};            // True when the segment is UCS-2 encoded
};

/**
 * @brief Number of backlog-decoder entries produced by one FIG block.
 *
 * Parallel to fig_blocks: the headless trace/inventory walk pairs each
 * fig_trace entry with the readable values its block's decoder appended to
 * the result vectors (frequency_infos / geo_coords / component_labels /
 * fig2_infos) via running consumed-counters.
 */
struct FigDecodeCounts {
    uint8_t frequencies{0};
    uint8_t geos{0};
    uint8_t component_labels{0};
    uint8_t fig2{0};
};

/**
 * @brief Advanced FIG processing result
 */
struct FIGAnalysisResult {
    std::vector<eti::FigBlock> fig_blocks;
    ::eti::Ensemble ensemble_info;
    std::vector<::eti::DabService> discovered_services;
    std::vector<::eti::SubChannelInfo> subchannels;

    // Backlog P2/P3 decoders (appended in FIG block order, so the headless
    // trace walk can pair them with the matching fig_trace entries via
    // running consumed-counters).
    std::vector<FrequencyInfoEntry> frequency_infos;      // FIG 0/21
    std::vector<GeoCoordEntry> geo_coords;                // FIG 0/22
    std::vector<ComponentLabelInfo> component_labels;     // FIG 1/2 + 1/3
    std::vector<FigType2Info> fig2_infos;                 // FIG type 2

    // Per-block entry counts (parallel to fig_blocks) — pairing metadata for
    // the trace walk above.
    std::vector<FigDecodeCounts> fig_decode_counts;

    // Per-frame FIG trace (P1): parallel to fig_blocks — one entry per block
    // with FIB provenance + CRC health for the chronological trace output.
    std::vector<FIGTraceEntry> fig_trace;

    // Per-frame FIC health (P1): FIBs walked, FIB CRC failures, FIG headers
    // skipped for length/padding anomalies, and whether the raw auto-fallback
    // produced the blocks (all-FIB-CRC-fail CRC-less dump heuristic).
    uint32_t total_fibs{0};
    uint32_t fib_crc_failures{0};
    uint32_t erroneous_figs{0};
    bool raw_fallback_used{false};

    struct ComplianceMetrics {
        double etsi_en_300_799_score{100.0};
        double etsi_en_300_401_score{100.0};
        std::vector<std::string> compliance_issues;
        std::vector<std::string> performance_warnings;
    } compliance;

    bool has_ensemble_info() const { return ensemble_info.validate_ensemble_id(); }
    bool has_services() const { return !discovered_services.empty(); }
    bool has_subchannels() const { return !subchannels.empty(); }
};

/**
 * @brief Modern C++20 ETI Frame Parser with Qt Integration
 *
 * This class provides high-performance, memory-efficient ETI frame parsing
 * with modern C++20 features and Qt signal/slot integration. Designed to
 * replace the ETISnoop wrapper with superior performance and functionality.
 *
 * Supports both ETI-LI and ETI-NI formats (Agent 44 enhancement).
 */
class ModernETIFrameParser : public QObject {
    Q_OBJECT

public:
    explicit ModernETIFrameParser(QObject* parent = nullptr);
    ~ModernETIFrameParser();

    /**
     * @brief Initialize the parser with configuration
     * @param config Processing configuration
     * @return true if initialization successful
     */
    bool initialize(const ProcessingConfig& config = {});

    /**
     * @brief Check if parser is ready for processing
     */
    bool isInitialized() const noexcept { return initialized_.load(); }

    /**
     * @brief Set the user-selected analyser decode options (row 1-14).
     *
     * Applied from the next parseFrame()/analyzeFIGData() call on; has no
     * effect on frames already parsed. Defaults to AnalyserSettings::defaults()
     * (byte parity with the pre-settings behavior).
     *
     * @param settings Analyser decode options to use
     */
    void setAnalyserSettings(const AnalyserSettings& settings) { settings_ = settings; }

    /**
     * @brief Current analyser decode options.
     */
    const AnalyserSettings& analyserSettings() const noexcept { return settings_; }

    /**
     * @brief Parse single ETI frame with optimal performance (auto-detects format)
     * @param frame_data Raw ETI frame data (must be exactly 6144 bytes)
     * @return Parse result with frame data and metrics
     */
    [[nodiscard]] ETIParseResult parseFrame(std::span<const uint8_t, ETI_FRAME_SIZE> frame_data);

    /**
     * @brief Parse ETI frame from QByteArray (Qt compatibility)
     * @param frame_data Qt byte array containing ETI frame
     * @return Parse result
     */
    [[nodiscard]] ETIParseResult parseFrame(const QByteArray& frame_data);

    /**
     * @brief Batch process multiple frames with threading
     * @param frames Vector of frame data spans
     * @return Vector of parse results
     */
    [[nodiscard]] std::vector<ETIParseResult> parseFramesBatch(
        const std::vector<std::span<const uint8_t, ETI_FRAME_SIZE>>& frames);

    /**
     * @brief Parse and analyze FIG data from frame
     * @param frame Parsed ETI frame
     * @param parseResult Parse result containing FICF and format info (CRITICAL for ETI-NI)
     * @return Comprehensive FIG analysis result
     */
    [[nodiscard]] FIGAnalysisResult analyzeFIGData(
        const ::eti::EtiFrame& frame,
        const ETIParseResult& parseResult);

    /**
     * @brief Validate frame structure and compliance
     * @param frame ETI frame to validate
     * @return Detailed validation result
     */
    [[nodiscard]] ::eti::ValidationResult validateFrame(const ::eti::EtiFrame& frame) const;

    /**
     * @brief Get current performance statistics
     */
    struct PerformanceStats {
        uint64_t frames_processed{0};
        double average_fps{0.0};
        double average_frame_time_us{0.0};
        uint64_t total_processing_time_ns{0};
        double memory_usage_mb{0.0};
        double cpu_efficiency{0.0};
        uint32_t cache_hit_rate_percent{0};

        // Performance vs targets
        double speed_improvement_percent{0.0}; // vs ETISnoop wrapper
        double memory_reduction_percent{0.0};  // vs baseline
        bool meets_performance_targets{false};
    };

    [[nodiscard]] PerformanceStats getPerformanceStats() const;

    /**
     * @brief Calculate speed improvement percentage vs baseline
     * @return Speed improvement percentage
     */
    [[nodiscard]] double calculateSpeedImprovement() const;

    /**
     * @brief Calculate memory reduction percentage vs baseline
     * @return Memory reduction percentage
     */
    [[nodiscard]] double calculateMemoryReduction() const;

    /**
     * @brief Reset performance counters
     */
    void resetStats();

    /**
     * @brief Update processing configuration at runtime
     * @param config New configuration
     */
    void updateConfiguration(const ProcessingConfig& config);

    /**
     * @brief Enable/disable specific optimizations
     */
    void enableSIMD(bool enable);
    void enableCaching(bool enable);
    void enableThreading(bool enable);
    void enableMemoryPooling(bool enable);

    /**
     * @brief Memory management
     */
    void clearCaches();
    void optimizeMemoryUsage();
    [[nodiscard]] size_t getMemoryFootprint() const;

signals:
    // NOTE: Custom eti:: type signals commented out to avoid MOC compilation errors
    // TODO: Refactor to use QByteArray serialization when needed

    // void frameProcessed(uint32_t frame_number, const ::eti::EtiFrame& frame,
    //                    std::chrono::nanoseconds parse_time);
    // void figAnalysisComplete(uint32_t frame_number, const FIGAnalysisResult& analysis);
    // void ensembleDiscovered(const ::eti::Ensemble& ensemble);
    // void serviceDiscovered(const ::eti::DabService& service);

    /**
     * @brief Emitted when compliance issue is detected
     * @param standard ETSI standard reference
     * @param issue Issue description
     * @param severity Issue severity level
     */
    void complianceIssue(const QString& standard, const QString& issue,
                        const QString& severity);

    /**
     * @brief Emitted when performance targets are achieved/missed
     * @param target_met Whether performance target was met
     * @param current_fps Current processing rate
     * @param target_fps Target processing rate
     */
    void performanceTarget(bool target_met, double current_fps, double target_fps);

private slots:
    void onThreadPoolFinished();
    void updatePerformanceMetrics();

private:
    // Core processing methods
    [[nodiscard]] bool validateFrameHeader(std::span<const uint8_t, ETI_FRAME_SIZE> data) const noexcept;
    [[nodiscard]] ::eti::EtiFrame parseFrameStructure(std::span<const uint8_t, ETI_FRAME_SIZE> data);
    [[nodiscard]] ::eti::FicDecodeResult extractFIGBlocks(const eti::EtiFicField& fic);

    // ETI-LI parsing (original implementation)
    [[nodiscard]] ETIParseResult parseETI_LI_Frame(std::span<const uint8_t, ETI_FRAME_SIZE> frame_data);

    // ETI-NI parsing (Agent 44 implementation)
    [[nodiscard]] ETIParseResult parseETI_NI_Frame(std::span<const uint8_t, ETI_FRAME_SIZE> frame_data);

    // FIG processing methods
    void processFIG00_EnsembleInfo(const eti::FigBlock& fig, FIGAnalysisResult& result);
    void processFIG01_SubchannelOrg(const eti::FigBlock& fig, FIGAnalysisResult& result);
    void processFIG02_ServiceOrg(const eti::FigBlock& fig, FIGAnalysisResult& result);
    void processFIG03_ServiceComponent(const eti::FigBlock& fig, FIGAnalysisResult& result);
    void processFIG09_CountryLTO(const eti::FigBlock& fig, FIGAnalysisResult& result);
    void processFIG021_FrequencyInfo(const eti::FigBlock& fig, FIGAnalysisResult& result);
    void processFIG022_GeoLocation(const eti::FigBlock& fig, FIGAnalysisResult& result);
    void processFIG10_EnsembleLabel(const eti::FigBlock& fig, FIGAnalysisResult& result);
    void processFIG11_ServiceLabel(const eti::FigBlock& fig, FIGAnalysisResult& result);
    void processFIG12_ServiceComponentLabel(const eti::FigBlock& fig, FIGAnalysisResult& result);
    void processFIG13_DataServiceLabel(const eti::FigBlock& fig, FIGAnalysisResult& result);
    void processFIG2_Other(const eti::FigBlock& fig, FIGAnalysisResult& result);

    // FIG 1 label charset decoding (EN 300 401 8.1.13): EBU Latin / UTF-8 /
    // TIS-620; honours force_charset and charset_fallback_policy settings.
    [[nodiscard]] std::string decodeLabelCharacters(uint8_t charset,
                                                    const uint8_t* bytes,
                                                    size_t count);

    // Short label from the 16-char label + 16-bit mask (EN 300 401 8.1.14,
    // etisnoop label_t::shortlabel() parity): include label byte i when
    // (mask & (0x8000 >> i)); then charset-convert the selected bytes. No
    // trailing-space trimming (matches etisnoop).
    [[nodiscard]] std::string decodeShortLabel(uint8_t charset,
                                               const uint8_t* bytes,
                                               size_t count,
                                               uint16_t mask);

    // Effective label charset after the force_charset setting (row 1):
    // force_charset >= 0 wins, else the stream charset flag.
    [[nodiscard]] uint8_t effectiveLabelCharset(uint8_t streamCharset) const noexcept {
        return settings_.force_charset >= 0
            ? static_cast<uint8_t>(settings_.force_charset & 0x0F)
            : streamCharset;
    }

    // Optimization implementations
    [[nodiscard]] bool validateFrameHeaderSIMD(std::span<const uint8_t, ETI_FRAME_SIZE> data) const noexcept;
    [[nodiscard]] ETIParseResult parseFrameOptimized(std::span<const uint8_t, ETI_FRAME_SIZE> data);

    // Performance tracking
    void updateFrameStats(std::chrono::nanoseconds parse_time);
    void checkPerformanceTargets();

    // Compliance calculation methods
    double calculateETSI799Compliance(const ::eti::EtiFrame& frame);
    double calculateETSI401Compliance(const FIGAnalysisResult& result);

    // Performance calculation methods
    double calculateCPUEfficiency(std::chrono::nanoseconds parse_time) const;

    // Validation helper methods
    void validateFICStructure(const ::eti::EtiFrame& frame, ::eti::ValidationResult& result) const;
    void validateMSCStructure(const ::eti::EtiFrame& frame, ::eti::ValidationResult& result) const;

    // Member variables
    std::atomic<bool> initialized_{false};
    ProcessingConfig config_;

    // Performance components
    std::unique_ptr<eti::modern::ETIFrameMemoryPool> memory_pool_;
    std::unique_ptr<eti::modern::ETIProcessingCache<uint32_t, ::eti::EtiFrame>> cache_;
    std::unique_ptr<eti::modern::performance_profiler> profiler_;

    // Threading
    QThreadPool* thread_pool_;

    // Statistics (atomic for thread safety)
    std::atomic<uint64_t> frames_processed_{0};
    std::atomic<uint64_t> total_processing_time_ns_{0};
    std::atomic<uint32_t> frame_sequence_{0};

    // Performance tracking
    mutable std::chrono::high_resolution_clock::time_point last_stats_update_;
    std::vector<std::chrono::nanoseconds> recent_frame_times_;

    // Cache for ensemble/service information
    std::optional<::eti::Ensemble> current_ensemble_;
    std::vector<::eti::DabService> discovered_services_;
    std::vector<::eti::SubChannelInfo> current_subchannels_;

    // True once the raw-FIC fallback warning (all FIB CRCs failed) has been
    // logged, so it appears once per stream instead of once per frame.
    std::atomic<bool> raw_fic_fallback_warned_{false};

    // User-selected analyser decode options (option-variant matrix rows 1-14).
    // Defaults == current etisnoop-parity behavior; populated by the CLI from
    // command-line flags or by the GUI from QSettings.
    AnalyserSettings settings_;

    // Charset values for which the "raw passthrough" warning was already logged
    // (FIG 1 labels, F4). Indexed by the 4-bit charset flag.
    std::array<bool, 16> label_charset_warned_{};

    // Compliance tracking
    struct {
        std::atomic<uint32_t> etsi_799_violations{0};
        std::atomic<uint32_t> etsi_401_violations{0};
        std::atomic<double> overall_compliance_score{100.0};
    } compliance_stats_;
};

/**
 * @brief Factory function for creating optimized parser instances
 */
[[nodiscard]] std::unique_ptr<ModernETIFrameParser> createOptimizedETIParser(
    const ProcessingConfig& config = {});

/**
 * @brief Utility functions for performance benchmarking
 */
namespace benchmark {
    /**
     * @brief Compare parser performance against ETISnoop wrapper
     * @param parser Modern parser instance
     * @param test_frames Vector of test frame data
     * @return Performance comparison metrics
     */
    struct ComparisonResult {
        double speed_improvement_percent;
        double memory_reduction_percent;
        bool meets_targets;
        std::vector<std::string> performance_notes;
    };

    [[nodiscard]] ComparisonResult compareWithETISnoop(
        ModernETIFrameParser& parser,
        const std::vector<std::span<const uint8_t, ETI_FRAME_SIZE>>& test_frames);
}

} // namespace eti::modern
