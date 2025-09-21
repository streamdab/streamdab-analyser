/**
 * @file eti_processing_engine.h
 * @brief Unified ETI Processing Engine with Migration Support
 * 
 * This engine provides a unified interface that supports both the legacy
 * ETISnoop wrapper and the new integrated C++20 architecture. Feature flags
 * allow seamless switching between implementations during Phase 2 migration.
 * 
 * Key Features:
 * - Backward compatibility with existing ETISnoop wrapper
 * - Progressive migration to integrated C++20 architecture  
 * - Runtime switching between implementations
 * - Performance comparison and validation
 * - Gradual component replacement
 */

#pragma once

#include "eti_types.h"
#include "modern_eti_frame_parser.h"
#include "enhanced_fig_analyzer.h"
#include "etisnoop_wrapper.h"
#include <QObject>
#include <memory>
#include <atomic>
#include <variant>

namespace eti::processing {

/**
 * @brief Engine configuration with feature flags
 */
struct EngineConfiguration {
    enum class ProcessingMode {
        LEGACY_WRAPPER,     // Use ETISnoop wrapper only
        INTEGRATED_ENGINE,  // Use new C++20 integrated engine only
        HYBRID_MODE,        // Use both and compare results
        GRADUAL_MIGRATION   // Progressive component replacement
    };
    
    ProcessingMode processing_mode{ProcessingMode::LEGACY_WRAPPER};
    
    // Component-specific feature flags
    struct ComponentFlags {
        bool use_modern_frame_parser{false};
        bool use_enhanced_fig_analyzer{false};
        bool use_integrated_audio_extractor{false};
        bool use_modern_error_detector{false};
        bool enable_performance_comparison{true};
        bool enable_result_validation{true};
    } components;
    
    // Performance configuration
    eti::modern::ProcessingConfig performance_config;
    
    // Migration settings
    struct MigrationSettings {
        bool enable_fallback_on_error{true};
        bool log_performance_differences{true};
        bool validate_result_consistency{true};
        double performance_threshold_percent{5.0}; // Switch if >5% difference
        uint32_t validation_sample_size{100}; // Validate every N frames
    } migration;
    
    // Debugging and monitoring
    bool enable_detailed_logging{false};
    bool enable_migration_metrics{true};
    bool enable_compatibility_checks{true};
};

/**
 * @brief Processing result that can contain data from either engine
 */
struct UnifiedProcessingResult {
    bool success{false};
    uint32_t frame_number{0};
    std::chrono::nanoseconds processing_time{0};
    
    // Legacy wrapper result (when available)
    std::optional<EtisnoopWrapper::FrameAnalysis> wrapper_result;
    
    // Integrated engine result (when available)  
    std::optional<eti::modern::ETIParseResult> integrated_result;
    
    // Comparison metrics (in hybrid mode)
    struct ComparisonMetrics {
        bool results_match{true};
        double performance_difference_percent{0.0};
        std::vector<std::string> differences;
        bool integrated_faster{false};
        
        ComparisonMetrics() = default;
    } comparison;
    
    // Unified access methods
    bool isValid() const { return success; }
    const eti::EtiFrame* getFrame() const;
    std::vector<eti::FigBlock> getFigBlocks() const;
    eti::Ensemble getEnsemble() const;
    std::vector<eti::DabService> getServices() const;
};

/**
 * @brief Migration progress tracking
 */
struct MigrationProgress {
    std::atomic<uint64_t> total_frames_processed{0};
    std::atomic<uint64_t> wrapper_processed{0};
    std::atomic<uint64_t> integrated_processed{0};
    std::atomic<uint64_t> hybrid_processed{0};
    
    std::atomic<uint64_t> performance_wins_integrated{0};
    std::atomic<uint64_t> performance_wins_wrapper{0};
    std::atomic<uint64_t> result_mismatches{0};
    std::atomic<uint64_t> fallback_activations{0};
    
    struct ComponentMigrationStatus {
        std::atomic<bool> frame_parser_migrated{false};
        std::atomic<bool> fig_analyzer_migrated{false};
        std::atomic<bool> audio_extractor_migrated{false};
        std::atomic<bool> error_detector_migrated{false};
        std::atomic<double> overall_migration_percent{0.0};
    } components;
    
    double getOverallProgress() const;
    std::string generateProgressReport() const;
};

/**
 * @brief Unified ETI Processing Engine
 * 
 * This engine provides a unified interface for ETI processing that supports
 * both legacy ETISnoop wrapper and new integrated C++20 architecture.
 * Enables gradual migration with performance validation and fallback support.
 */
class UnifiedETIProcessingEngine : public QObject {
    Q_OBJECT

public:
    explicit UnifiedETIProcessingEngine(QObject* parent = nullptr);
    ~UnifiedETIProcessingEngine();

    /**
     * @brief Initialize the engine with configuration
     * @param config Engine configuration with feature flags
     * @return true if initialization successful
     */
    bool initialize(const EngineConfiguration& config = {});

    /**
     * @brief Check if engine is ready for processing
     */
    bool isInitialized() const { return initialized_.load(); }

    /**
     * @brief Process single ETI frame using configured engine
     * @param frame_data Raw ETI frame data
     * @return Unified processing result
     */
    UnifiedProcessingResult processFrame(const QByteArray& frame_data);

    /**
     * @brief Process ETI file using configured engine
     * @param filename Path to ETI file
     * @return true if processing successful
     */
    bool processFile(const QString& filename);

    /**
     * @brief Get current engine configuration
     */
    EngineConfiguration getConfiguration() const { return config_; }

    /**
     * @brief Update configuration at runtime
     * @param config New configuration
     * @return true if update successful
     */
    bool updateConfiguration(const EngineConfiguration& config);

    /**
     * @brief Switch processing mode at runtime
     * @param mode New processing mode
     * @return true if switch successful
     */
    bool switchProcessingMode(EngineConfiguration::ProcessingMode mode);

    /**
     * @brief Enable/disable specific components
     */
    void enableModernFrameParser(bool enable);
    void enableEnhancedFIGAnalyzer(bool enable);
    void enableIntegratedAudioExtractor(bool enable);
    void enableModernErrorDetector(bool enable);

    /**
     * @brief Get migration progress information
     */
    MigrationProgress getMigrationProgress() const { return migration_progress_; }

    /**
     * @brief Get performance comparison between engines
     */
    struct PerformanceComparison {
        double wrapper_avg_fps{0.0};
        double integrated_avg_fps{0.0};
        double speed_improvement_percent{0.0};
        
        double wrapper_avg_memory_mb{0.0};
        double integrated_avg_memory_mb{0.0};
        double memory_reduction_percent{0.0};
        
        bool meets_performance_targets{false};
        std::vector<std::string> recommendations;
    };
    
    PerformanceComparison getPerformanceComparison() const;

    /**
     * @brief Validate result consistency between engines
     * @param wrapper_result Result from ETISnoop wrapper
     * @param integrated_result Result from integrated engine
     * @return Validation result with differences
     */
    struct ValidationResult {
        bool results_consistent{true};
        std::vector<std::string> differences;
        double similarity_score{100.0};
    };
    
    ValidationResult validateResultConsistency(
        const EtisnoopWrapper::FrameAnalysis& wrapper_result,
        const eti::modern::ETIParseResult& integrated_result) const;

    /**
     * @brief Generate migration recommendation based on current metrics
     * @return Migration recommendation with rationale
     */
    struct MigrationRecommendation {
        bool recommend_migration{false};
        std::string rationale;
        std::vector<std::string> next_steps;
        std::vector<std::string> risks;
        double confidence_score{0.0};
    };
    
    MigrationRecommendation generateMigrationRecommendation() const;

    /**
     * @brief Force fallback to legacy wrapper (emergency use)
     */
    void forceFallbackToWrapper();

    /**
     * @brief Reset migration statistics
     */
    void resetMigrationStats();

signals:
    /**
     * @brief Emitted when frame is processed (unified interface)
     */
    void frameProcessed(uint32_t frame_number, const UnifiedProcessingResult& result);

    /**
     * @brief Emitted when ensemble is discovered
     */
    void ensembleDiscovered(const eti::Ensemble& ensemble);

    /**
     * @brief Emitted when service is discovered
     */
    void serviceDiscovered(const eti::DabService& service);

    /**
     * @brief Emitted when performance comparison is available
     */
    void performanceComparisonUpdate(const PerformanceComparison& comparison);

    /**
     * @brief Emitted when migration milestone is reached
     */
    void migrationMilestone(const QString& milestone, double progress_percent);

    /**
     * @brief Emitted when fallback is activated
     */
    void fallbackActivated(const QString& reason);

    /**
     * @brief Emitted when result consistency issue is detected
     */
    void consistencyIssue(const QString& issue, double similarity_score);

private slots:
    void onWrapperFrameProcessed(const EtisnoopWrapper::FrameAnalysis& analysis);
    void onIntegratedFrameProcessed(uint32_t frame_number, const eti::modern::ETIParseResult& result);
    void updateMigrationMetrics();

private:
    // Core processing methods
    UnifiedProcessingResult processWithWrapper(const QByteArray& frame_data);
    UnifiedProcessingResult processWithIntegratedEngine(const QByteArray& frame_data);
    UnifiedProcessingResult processWithHybridMode(const QByteArray& frame_data);
    UnifiedProcessingResult processWithGradualMigration(const QByteArray& frame_data);

    // Component management
    bool initializeWrapper();
    bool initializeIntegratedEngine();
    bool shouldUseIntegratedComponent(const QString& component_name) const;
    
    // Performance tracking
    void trackPerformance(const UnifiedProcessingResult& result);
    void updatePerformanceComparison();
    
    // Migration logic
    void evaluateMigrationOpportunity();
    bool shouldMigrateComponent(const QString& component_name) const;
    void updateMigrationProgress();
    
    // Validation and comparison
    UnifiedProcessingResult::ComparisonMetrics compareResults(
        const EtisnoopWrapper::FrameAnalysis& wrapper_result,
        const eti::modern::ETIParseResult& integrated_result) const;
    
    bool validateFrameConsistency(const eti::EtiFrame& frame1, const eti::EtiFrame& frame2) const;
    bool validateEnsembleConsistency(const eti::Ensemble& ensemble1, const eti::Ensemble& ensemble2) const;
    
    // Error handling
    void handleProcessingError(const QString& error, const QString& component);
    bool activateFallback(const QString& reason);

private:
    std::atomic<bool> initialized_{false};
    EngineConfiguration config_;
    
    // Engine components
    std::unique_ptr<EtisnoopWrapper> wrapper_engine_;
    std::unique_ptr<eti::modern::ModernETIFrameParser> integrated_parser_;
    std::unique_ptr<eti::modern::EnhancedFIGAnalyzer> enhanced_fig_analyzer_;
    
    // Migration tracking
    MigrationProgress migration_progress_;
    mutable std::shared_mutex migration_mutex_;
    
    // Performance tracking
    std::vector<std::chrono::nanoseconds> wrapper_processing_times_;
    std::vector<std::chrono::nanoseconds> integrated_processing_times_;
    std::chrono::steady_clock::time_point last_performance_update_;
    
    // State management
    std::atomic<uint32_t> frame_sequence_{0};
    std::atomic<bool> fallback_active_{false};
    std::string fallback_reason_;
    
    // Caching for consistency validation
    struct ValidationCache {
        std::optional<eti::Ensemble> last_ensemble;
        std::vector<eti::DabService> last_services;
        uint32_t last_frame_number{0};
    } validation_cache_;
    
    // Performance constants
    static constexpr double PERFORMANCE_IMPROVEMENT_THRESHOLD = 30.0; // 30% improvement target
    static constexpr double MEMORY_REDUCTION_THRESHOLD = 40.0; // 40% memory reduction target
    static constexpr size_t PERFORMANCE_SAMPLE_SIZE = 100; // Sample size for comparisons
};

/**
 * @brief Factory function for creating configured engine
 */
std::unique_ptr<UnifiedETIProcessingEngine> createConfiguredEngine(
    const EngineConfiguration& config = {});

/**
 * @brief Migration utilities
 */
namespace migration {
    /**
     * @brief Analyze current system for migration readiness
     */
    struct ReadinessAssessment {
        bool ready_for_migration{false};
        std::vector<std::string> blockers;
        std::vector<std::string> recommendations;
        double readiness_score{0.0};
    };
    
    ReadinessAssessment assessMigrationReadiness();
    
    /**
     * @brief Generate migration plan based on current configuration
     */
    struct MigrationPlan {
        std::vector<std::string> phases;
        std::vector<std::string> validation_steps;
        std::vector<std::string> rollback_procedures;
        std::chrono::duration<int, std::ratio<3600>> estimated_duration;
    };
    
    MigrationPlan generateMigrationPlan(const EngineConfiguration& current_config);
}

} // namespace eti::processing