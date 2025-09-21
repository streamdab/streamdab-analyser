/**
 * @file eti_processing_engine.cpp
 * @brief Unified ETI Processing Engine Implementation
 */

#include "eti_processing_engine.h"
#include "utils/logger.h"
#include <QTimer>
#include <algorithm>
#include <numeric>

namespace eti::processing {

UnifiedETIProcessingEngine::UnifiedETIProcessingEngine(QObject* parent)
    : QObject(parent)
    , last_performance_update_(std::chrono::steady_clock::now())
{
    // Reserve space for performance tracking
    wrapper_processing_times_.reserve(PERFORMANCE_SAMPLE_SIZE);
    integrated_processing_times_.reserve(PERFORMANCE_SAMPLE_SIZE);
    
    Logger::instance().log(Logger::Info, "UnifiedETIProcessingEngine", 
                          "Unified ETI Processing Engine initialized for Phase 2 migration");
}

UnifiedETIProcessingEngine::~UnifiedETIProcessingEngine() {
    if (initialized_.load()) {
        auto progress = getMigrationProgress();
        Logger::instance().log(Logger::Info, "UnifiedETIProcessingEngine", 
                              QString("Engine destroyed - Migration progress: %.1f%%, Frames processed: %2")
                              .arg(progress.getOverallProgress())
                              .arg(progress.total_frames_processed.load()));
    }
}

bool UnifiedETIProcessingEngine::initialize(const EngineConfiguration& config) {
    Logger::instance().log(Logger::Info, "UnifiedETIProcessingEngine", 
                          "Initializing Unified ETI Processing Engine");
    
    try {
        config_ = config;
        
        // Initialize components based on configuration
        bool wrapper_needed = (config_.processing_mode == EngineConfiguration::ProcessingMode::LEGACY_WRAPPER ||
                              config_.processing_mode == EngineConfiguration::ProcessingMode::HYBRID_MODE ||
                              config_.processing_mode == EngineConfiguration::ProcessingMode::GRADUAL_MIGRATION);
        
        bool integrated_needed = (config_.processing_mode == EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE ||
                                 config_.processing_mode == EngineConfiguration::ProcessingMode::HYBRID_MODE ||
                                 config_.processing_mode == EngineConfiguration::ProcessingMode::GRADUAL_MIGRATION);
        
        // Initialize ETISnoop wrapper if needed
        if (wrapper_needed && !initializeWrapper()) {
            Logger::instance().log(Logger::Error, "UnifiedETIProcessingEngine", 
                                  "Failed to initialize ETISnoop wrapper");
            return false;
        }
        
        // Initialize integrated engine if needed
        if (integrated_needed && !initializeIntegratedEngine()) {
            Logger::instance().log(Logger::Error, "UnifiedETIProcessingEngine", 
                                  "Failed to initialize integrated engine");
            
            // Fall back to wrapper only if available
            if (wrapper_engine_) {
                Logger::instance().log(Logger::Warning, "UnifiedETIProcessingEngine", 
                                      "Falling back to ETISnoop wrapper only");
                config_.processing_mode = EngineConfiguration::ProcessingMode::LEGACY_WRAPPER;
            } else {
                return false;
            }
        }
        
        // Setup performance monitoring timer
        if (config_.migration.enable_performance_comparison) {
            QTimer* perf_timer = new QTimer(this);
            connect(perf_timer, &QTimer::timeout, this, &UnifiedETIProcessingEngine::updateMigrationMetrics);
            perf_timer->start(5000); // Update every 5 seconds
        }
        
        // Reset migration statistics
        resetMigrationStats();
        
        initialized_.store(true);
        
        Logger::instance().log(Logger::Info, "UnifiedETIProcessingEngine", 
                              QString("Engine initialized successfully - Mode: %1, Components: %2")
                              .arg(static_cast<int>(config_.processing_mode))
                              .arg(QString("FP:%1 FA:%2 AE:%3 ED:%4")
                                   .arg(config_.components.use_modern_frame_parser ? "I" : "W")
                                   .arg(config_.components.use_enhanced_fig_analyzer ? "I" : "W")
                                   .arg(config_.components.use_integrated_audio_extractor ? "I" : "W")
                                   .arg(config_.components.use_modern_error_detector ? "I" : "W")));
        
        return true;
    }
    catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "UnifiedETIProcessingEngine", 
                              QString("Initialization failed: %1").arg(e.what()));
        return false;
    }
}

UnifiedProcessingResult UnifiedETIProcessingEngine::processFrame(const QByteArray& frame_data) {
    if (!initialized_.load()) {
        UnifiedProcessingResult result;
        result.success = false;
        return result;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    UnifiedProcessingResult result;
    result.frame_number = frame_sequence_.fetch_add(1);
    
    try {
        // Route to appropriate processing method based on mode
        switch (config_.processing_mode) {
            case EngineConfiguration::ProcessingMode::LEGACY_WRAPPER:
                result = processWithWrapper(frame_data);
                migration_progress_.wrapper_processed.fetch_add(1);
                break;
                
            case EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE:
                result = processWithIntegratedEngine(frame_data);
                migration_progress_.integrated_processed.fetch_add(1);
                break;
                
            case EngineConfiguration::ProcessingMode::HYBRID_MODE:
                result = processWithHybridMode(frame_data);
                migration_progress_.hybrid_processed.fetch_add(1);
                break;
                
            case EngineConfiguration::ProcessingMode::GRADUAL_MIGRATION:
                result = processWithGradualMigration(frame_data);
                break;
        }
        
        // Update processing time
        auto end_time = std::chrono::high_resolution_clock::now();
        result.processing_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        
        // Track performance
        trackPerformance(result);
        
        // Update migration progress
        migration_progress_.total_frames_processed.fetch_add(1);
        updateMigrationProgress();
        
        // Emit unified signal
        if (result.success) {
            emit frameProcessed(result.frame_number, result);
        }
        
        Logger::instance().log(Logger::Debug, "UnifiedETIProcessingEngine", 
                              QString("Frame %1 processed in %2 ns (mode: %3, success: %4)")
                              .arg(result.frame_number)
                              .arg(result.processing_time.count())
                              .arg(static_cast<int>(config_.processing_mode))
                              .arg(result.success));
        
        return result;
    }
    catch (const std::exception& e) {
        auto end_time = std::chrono::high_resolution_clock::now();
        result.processing_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        result.success = false;
        
        handleProcessingError(QString("Frame processing exception: %1").arg(e.what()), "Engine");
        return result;
    }
}

bool UnifiedETIProcessingEngine::processFile(const QString& filename) {
    if (!initialized_.load()) {
        Logger::instance().log(Logger::Error, "UnifiedETIProcessingEngine", 
                              "Engine not initialized");
        return false;
    }
    
    Logger::instance().log(Logger::Info, "UnifiedETIProcessingEngine", 
                          QString("Processing ETI file: %1").arg(filename));
    
    // Route to appropriate engine based on configuration
    switch (config_.processing_mode) {
        case EngineConfiguration::ProcessingMode::LEGACY_WRAPPER:
            if (wrapper_engine_) {
                return wrapper_engine_->processEtiFile(filename);
            }
            break;
            
        case EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE:
            // File processing would need to be implemented in the integrated parser
            Logger::instance().log(Logger::Warning, "UnifiedETIProcessingEngine", 
                                  "File processing not yet implemented in integrated engine");
            return false;
            
        case EngineConfiguration::ProcessingMode::HYBRID_MODE:
        case EngineConfiguration::ProcessingMode::GRADUAL_MIGRATION:
            // Use wrapper for file processing, compare with frame-by-frame integrated processing
            if (wrapper_engine_) {
                return wrapper_engine_->processEtiFile(filename);
            }
            break;
    }
    
    return false;
}

bool UnifiedETIProcessingEngine::updateConfiguration(const EngineConfiguration& config) {
    Logger::instance().log(Logger::Info, "UnifiedETIProcessingEngine", 
                          "Updating engine configuration");
    
    try {
        EngineConfiguration old_config = config_;
        config_ = config;
        
        // Check if we need to initialize new components
        bool needs_wrapper = (config_.processing_mode != EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE);
        bool needs_integrated = (config_.processing_mode != EngineConfiguration::ProcessingMode::LEGACY_WRAPPER);
        
        if (needs_wrapper && !wrapper_engine_) {
            if (!initializeWrapper()) {
                config_ = old_config; // Rollback
                return false;
            }
        }
        
        if (needs_integrated && !integrated_parser_) {
            if (!initializeIntegratedEngine()) {
                config_ = old_config; // Rollback
                return false;
            }
        }
        
        Logger::instance().log(Logger::Info, "UnifiedETIProcessingEngine", 
                              "Configuration updated successfully");
        return true;
    }
    catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "UnifiedETIProcessingEngine", 
                              QString("Configuration update failed: %1").arg(e.what()));
        return false;
    }
}

bool UnifiedETIProcessingEngine::switchProcessingMode(EngineConfiguration::ProcessingMode mode) {
    EngineConfiguration new_config = config_;
    new_config.processing_mode = mode;
    
    bool success = updateConfiguration(new_config);
    
    if (success) {
        Logger::instance().log(Logger::Info, "UnifiedETIProcessingEngine", 
                              QString("Switched to processing mode: %1").arg(static_cast<int>(mode)));
        
        emit migrationMilestone(QString("Switched to mode %1").arg(static_cast<int>(mode)), 
                               getMigrationProgress().getOverallProgress());
    }
    
    return success;
}

void UnifiedETIProcessingEngine::enableModernFrameParser(bool enable) {
    config_.components.use_modern_frame_parser = enable;
    updateMigrationProgress();
    
    Logger::instance().log(Logger::Info, "UnifiedETIProcessingEngine", 
                          QString("Modern frame parser %1").arg(enable ? "enabled" : "disabled"));
}

void UnifiedETIProcessingEngine::enableEnhancedFIGAnalyzer(bool enable) {
    config_.components.use_enhanced_fig_analyzer = enable;
    updateMigrationProgress();
    
    Logger::instance().log(Logger::Info, "UnifiedETIProcessingEngine", 
                          QString("Enhanced FIG analyzer %1").arg(enable ? "enabled" : "disabled"));
}

void UnifiedETIProcessingEngine::enableIntegratedAudioExtractor(bool enable) {
    config_.components.use_integrated_audio_extractor = enable;
    updateMigrationProgress();
    
    Logger::instance().log(Logger::Info, "UnifiedETIProcessingEngine", 
                          QString("Integrated audio extractor %1").arg(enable ? "enabled" : "disabled"));
}

void UnifiedETIProcessingEngine::enableModernErrorDetector(bool enable) {
    config_.components.use_modern_error_detector = enable;
    updateMigrationProgress();
    
    Logger::instance().log(Logger::Info, "UnifiedETIProcessingEngine", 
                          QString("Modern error detector %1").arg(enable ? "enabled" : "disabled"));
}

// Private implementation methods

bool UnifiedETIProcessingEngine::initializeWrapper() {
    try {
        wrapper_engine_ = std::make_unique<EtisnoopWrapper>(this);
        
        if (!wrapper_engine_->initialize(true, true)) { // Enable Thai and StreamDAB support
            Logger::instance().log(Logger::Error, "UnifiedETIProcessingEngine", 
                                  "ETISnoop wrapper initialization failed");
            wrapper_engine_.reset();
            return false;
        }
        
        // Connect signals
        connect(wrapper_engine_.get(), &EtisnoopWrapper::frameProcessed,
                this, &UnifiedETIProcessingEngine::onWrapperFrameProcessed);
        
        Logger::instance().log(Logger::Info, "UnifiedETIProcessingEngine", 
                              "ETISnoop wrapper initialized successfully");
        return true;
    }
    catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "UnifiedETIProcessingEngine", 
                              QString("Wrapper initialization exception: %1").arg(e.what()));
        return false;
    }
}

bool UnifiedETIProcessingEngine::initializeIntegratedEngine() {
    try {
        // Initialize modern frame parser
        integrated_parser_ = std::make_unique<eti::modern::ModernETIFrameParser>(this);
        
        if (!integrated_parser_->initialize(config_.performance_config)) {
            Logger::instance().log(Logger::Error, "UnifiedETIProcessingEngine", 
                                  "Modern frame parser initialization failed");
            integrated_parser_.reset();
            return false;
        }
        
        // Initialize enhanced FIG analyzer
        enhanced_fig_analyzer_ = std::make_unique<eti::modern::EnhancedFIGAnalyzer>(this);
        
        if (!enhanced_fig_analyzer_->initialize(true, true)) { // Enable carousel analysis and Thai support
            Logger::instance().log(Logger::Error, "UnifiedETIProcessingEngine", 
                                  "Enhanced FIG analyzer initialization failed");
            enhanced_fig_analyzer_.reset();
            return false;
        }
        
        // Connect signals
        connect(integrated_parser_.get(), &eti::modern::ModernETIFrameParser::frameProcessed,
                this, &UnifiedETIProcessingEngine::onIntegratedFrameProcessed);
        
        connect(enhanced_fig_analyzer_.get(), &eti::modern::EnhancedFIGAnalyzer::ensembleDiscovered,
                this, &UnifiedETIProcessingEngine::ensembleDiscovered);
        
        connect(enhanced_fig_analyzer_.get(), &eti::modern::EnhancedFIGAnalyzer::serviceDiscovered,
                [this](const eti::modern::EnhancedServiceInfo& enhanced_service) {
                    emit serviceDiscovered(enhanced_service.base_service);
                });
        
        Logger::instance().log(Logger::Info, "UnifiedETIProcessingEngine", 
                              "Integrated engine components initialized successfully");
        return true;
    }
    catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "UnifiedETIProcessingEngine", 
                              QString("Integrated engine initialization exception: %1").arg(e.what()));
        return false;
    }
}

UnifiedProcessingResult UnifiedETIProcessingEngine::processWithWrapper(const QByteArray& frame_data) {
    UnifiedProcessingResult result;
    
    if (!wrapper_engine_) {
        result.success = false;
        return result;
    }
    
    try {
        // Process frame with ETISnoop wrapper
        auto wrapper_result = wrapper_engine_->processEtiFrame(frame_data);
        
        result.success = wrapper_result.isValid;
        result.wrapper_result = wrapper_result;
        
        Logger::instance().log(Logger::Debug, "UnifiedETIProcessingEngine", 
                              QString("Wrapper processing: success=%1, time=%2ms")
                              .arg(result.success)
                              .arg(wrapper_result.processingTime, 0, 'f', 2));
        
        return result;
    }
    catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "UnifiedETIProcessingEngine", 
                              QString("Wrapper processing exception: %1").arg(e.what()));
        result.success = false;
        return result;
    }
}

UnifiedProcessingResult UnifiedETIProcessingEngine::processWithIntegratedEngine(const QByteArray& frame_data) {
    UnifiedProcessingResult result;
    
    if (!integrated_parser_) {
        result.success = false;
        return result;
    }
    
    try {
        // Process frame with integrated engine
        auto integrated_result = integrated_parser_->parseFrame(frame_data);
        
        result.success = integrated_result.success;
        result.integrated_result = integrated_result;
        
        // Analyze FIG data if parsing was successful
        if (result.success && enhanced_fig_analyzer_) {
            auto fig_analysis = enhanced_fig_analyzer_->analyzeFIGData(integrated_result.frame);
            // FIG analysis results are emitted via signals
        }
        
        Logger::instance().log(Logger::Debug, "UnifiedETIProcessingEngine", 
                              QString("Integrated processing: success=%1, time=%2ns")
                              .arg(result.success)
                              .arg(integrated_result.parse_time.count()));
        
        return result;
    }
    catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "UnifiedETIProcessingEngine", 
                              QString("Integrated processing exception: %1").arg(e.what()));
        result.success = false;
        return result;
    }
}

UnifiedProcessingResult UnifiedETIProcessingEngine::processWithHybridMode(const QByteArray& frame_data) {
    UnifiedProcessingResult result;
    
    try {
        // Process with both engines
        auto wrapper_result = processWithWrapper(frame_data);
        auto integrated_result = processWithIntegratedEngine(frame_data);
        
        // Combine results
        result.success = wrapper_result.success || integrated_result.success;
        result.wrapper_result = wrapper_result.wrapper_result;
        result.integrated_result = integrated_result.integrated_result;
        
        // Compare results if both succeeded
        if (wrapper_result.success && integrated_result.success && 
            config_.migration.validate_result_consistency) {
            
            result.comparison = compareResults(
                wrapper_result.wrapper_result.value(),
                integrated_result.integrated_result.value());
            
            if (!result.comparison.results_match) {
                migration_progress_.result_mismatches.fetch_add(1);
                emit consistencyIssue("Result mismatch detected", result.comparison.performance_difference_percent);
            }
            
            // Track performance comparison
            if (result.comparison.integrated_faster) {
                migration_progress_.performance_wins_integrated.fetch_add(1);
            } else {
                migration_progress_.performance_wins_wrapper.fetch_add(1);
            }
        }
        
        Logger::instance().log(Logger::Debug, "UnifiedETIProcessingEngine", 
                              QString("Hybrid processing: wrapper=%1, integrated=%2, match=%3")
                              .arg(wrapper_result.success)
                              .arg(integrated_result.success)
                              .arg(result.comparison.results_match));
        
        return result;
    }
    catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "UnifiedETIProcessingEngine", 
                              QString("Hybrid processing exception: %1").arg(e.what()));
        result.success = false;
        return result;
    }
}

UnifiedProcessingResult UnifiedETIProcessingEngine::processWithGradualMigration(const QByteArray& frame_data) {
    UnifiedProcessingResult result;
    
    try {
        // Determine which components to use based on migration flags
        bool use_integrated_parser = config_.components.use_modern_frame_parser;
        bool use_enhanced_analyzer = config_.components.use_enhanced_fig_analyzer;
        
        if (use_integrated_parser && integrated_parser_) {
            // Use integrated frame parser
            result = processWithIntegratedEngine(frame_data);
            migration_progress_.integrated_processed.fetch_add(1);
        } else if (wrapper_engine_) {
            // Use wrapper parser
            result = processWithWrapper(frame_data);
            migration_progress_.wrapper_processed.fetch_add(1);
        } else {
            result.success = false;
            return result;
        }
        
        // Enhanced FIG analysis if enabled
        if (use_enhanced_analyzer && enhanced_fig_analyzer_ && result.success) {
            if (result.integrated_result.has_value()) {
                auto fig_analysis = enhanced_fig_analyzer_->analyzeFIGData(result.integrated_result->frame);
            }
        }
        
        Logger::instance().log(Logger::Debug, "UnifiedETIProcessingEngine", 
                              QString("Gradual migration: parser=%1, analyzer=%2")
                              .arg(use_integrated_parser ? "integrated" : "wrapper")
                              .arg(use_enhanced_analyzer ? "enhanced" : "legacy"));
        
        return result;
    }
    catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "UnifiedETIProcessingEngine", 
                              QString("Gradual migration processing exception: %1").arg(e.what()));
        
        // Try fallback to wrapper
        if (activateFallback("Gradual migration processing failed")) {
            return processWithWrapper(frame_data);
        }
        
        result.success = false;
        return result;
    }
}

void UnifiedETIProcessingEngine::trackPerformance(const UnifiedProcessingResult& result) {
    // Track processing times for performance comparison
    if (result.wrapper_result.has_value()) {
        auto time_ns = std::chrono::nanoseconds(static_cast<uint64_t>(result.wrapper_result->processingTime * 1000000));
        wrapper_processing_times_.push_back(time_ns);
        
        if (wrapper_processing_times_.size() > PERFORMANCE_SAMPLE_SIZE) {
            wrapper_processing_times_.erase(wrapper_processing_times_.begin());
        }
    }
    
    if (result.integrated_result.has_value()) {
        integrated_processing_times_.push_back(result.integrated_result->parse_time);
        
        if (integrated_processing_times_.size() > PERFORMANCE_SAMPLE_SIZE) {
            integrated_processing_times_.erase(integrated_processing_times_.begin());
        }
    }
}

void UnifiedETIProcessingEngine::updateMigrationProgress() {
    // Update component migration status
    migration_progress_.components.frame_parser_migrated.store(config_.components.use_modern_frame_parser);
    migration_progress_.components.fig_analyzer_migrated.store(config_.components.use_enhanced_fig_analyzer);
    migration_progress_.components.audio_extractor_migrated.store(config_.components.use_integrated_audio_extractor);
    migration_progress_.components.error_detector_migrated.store(config_.components.use_modern_error_detector);
    
    // Calculate overall migration percentage
    int migrated_components = 0;
    migrated_components += config_.components.use_modern_frame_parser ? 1 : 0;
    migrated_components += config_.components.use_enhanced_fig_analyzer ? 1 : 0;
    migrated_components += config_.components.use_integrated_audio_extractor ? 1 : 0;
    migrated_components += config_.components.use_modern_error_detector ? 1 : 0;
    
    double progress = (migrated_components / 4.0) * 100.0;
    migration_progress_.components.overall_migration_percent.store(progress);
}

UnifiedProcessingResult::ComparisonMetrics UnifiedETIProcessingEngine::compareResults(
    const EtisnoopWrapper::FrameAnalysis& wrapper_result,
    const eti::modern::ETIParseResult& integrated_result) const {
    
    UnifiedProcessingResult::ComparisonMetrics metrics;
    
    // Compare processing times
    auto wrapper_time_ns = static_cast<uint64_t>(wrapper_result.processingTime * 1000000);
    auto integrated_time_ns = integrated_result.parse_time.count();
    
    if (wrapper_time_ns > 0) {
        metrics.performance_difference_percent = 
            ((static_cast<double>(wrapper_time_ns) - integrated_time_ns) / wrapper_time_ns) * 100.0;
        metrics.integrated_faster = integrated_time_ns < wrapper_time_ns;
    }
    
    // Basic frame validation comparison
    metrics.results_match = (wrapper_result.isValid == integrated_result.success);
    
    if (!metrics.results_match) {
        metrics.differences.push_back("Processing success status differs");
    }
    
    // Add more detailed comparisons as needed
    
    return metrics;
}

bool UnifiedETIProcessingEngine::activateFallback(const QString& reason) {
    if (fallback_active_.load()) {
        return true; // Already in fallback mode
    }
    
    if (!wrapper_engine_) {
        return false; // No fallback available
    }
    
    fallback_active_.store(true);
    fallback_reason_ = reason;
    migration_progress_.fallback_activations.fetch_add(1);
    
    emit fallbackActivated(reason);
    
    Logger::instance().log(Logger::Warning, "UnifiedETIProcessingEngine", 
                          QString("Fallback activated: %1").arg(reason));
    
    return true;
}

void UnifiedETIProcessingEngine::handleProcessingError(const QString& error, const QString& component) {
    Logger::instance().log(Logger::Error, "UnifiedETIProcessingEngine", 
                          QString("Processing error in %1: %2").arg(component, error));
    
    // Consider activating fallback if in integrated mode
    if (config_.processing_mode == EngineConfiguration::ProcessingMode::INTEGRATED_ENGINE &&
        config_.migration.enable_fallback_on_error) {
        activateFallback(QString("Error in %1: %2").arg(component, error));
    }
}

void UnifiedETIProcessingEngine::resetMigrationStats() {
    migration_progress_ = MigrationProgress{};
    wrapper_processing_times_.clear();
    integrated_processing_times_.clear();
    
    fallback_active_.store(false);
    fallback_reason_.clear();
    
    Logger::instance().log(Logger::Info, "UnifiedETIProcessingEngine", 
                          "Migration statistics reset");
}

void UnifiedETIProcessingEngine::forceFallbackToWrapper() {
    activateFallback("Forced fallback to wrapper requested");
    config_.processing_mode = EngineConfiguration::ProcessingMode::LEGACY_WRAPPER;
    
    Logger::instance().log(Logger::Warning, "UnifiedETIProcessingEngine", 
                          "Forced fallback to ETISnoop wrapper");
}

UnifiedETIProcessingEngine::PerformanceComparison UnifiedETIProcessingEngine::getPerformanceComparison() const {
    PerformanceComparison comparison;
    
    // Calculate wrapper performance
    if (!wrapper_processing_times_.empty()) {
        auto avg_time_ns = std::accumulate(wrapper_processing_times_.begin(), wrapper_processing_times_.end(), 
                                          std::chrono::nanoseconds{0}) / wrapper_processing_times_.size();
        comparison.wrapper_avg_fps = 1e9 / avg_time_ns.count(); // Convert ns to fps
    }
    
    // Calculate integrated performance
    if (!integrated_processing_times_.empty()) {
        auto avg_time_ns = std::accumulate(integrated_processing_times_.begin(), integrated_processing_times_.end(), 
                                          std::chrono::nanoseconds{0}) / integrated_processing_times_.size();
        comparison.integrated_avg_fps = 1e9 / avg_time_ns.count(); // Convert ns to fps
    }
    
    // Calculate improvement
    if (comparison.wrapper_avg_fps > 0) {
        comparison.speed_improvement_percent = 
            ((comparison.integrated_avg_fps - comparison.wrapper_avg_fps) / comparison.wrapper_avg_fps) * 100.0;
    }
    
    // Check if targets are met
    comparison.meets_performance_targets = 
        (comparison.speed_improvement_percent >= PERFORMANCE_IMPROVEMENT_THRESHOLD);
    
    // Generate recommendations
    if (!comparison.meets_performance_targets) {
        comparison.recommendations.push_back("Enable SIMD optimizations in integrated engine");
        comparison.recommendations.push_back("Increase cache size for better performance");
        comparison.recommendations.push_back("Optimize FIG parsing algorithms");
    }
    
    return comparison;
}

// Slot implementations
void UnifiedETIProcessingEngine::onWrapperFrameProcessed(const EtisnoopWrapper::FrameAnalysis& analysis) {
    // Handle wrapper frame processing completion
    Logger::instance().log(Logger::Debug, "UnifiedETIProcessingEngine", 
                          QString("Wrapper frame processed: valid=%1, time=%2ms")
                          .arg(analysis.isValid)
                          .arg(analysis.processingTime, 0, 'f', 2));
}

void UnifiedETIProcessingEngine::onIntegratedFrameProcessed(uint32_t frame_number, 
                                                           const eti::modern::ETIParseResult& result) {
    // Handle integrated frame processing completion
    Logger::instance().log(Logger::Debug, "UnifiedETIProcessingEngine", 
                          QString("Integrated frame processed: #%1, success=%2, time=%3ns")
                          .arg(frame_number)
                          .arg(result.success)
                          .arg(result.parse_time.count()));
}

void UnifiedETIProcessingEngine::updateMigrationMetrics() {
    updatePerformanceComparison();
    
    auto progress = getMigrationProgress().getOverallProgress();
    
    // Check for migration milestones
    static double last_milestone = 0.0;
    if (progress >= last_milestone + 25.0) { // Every 25% progress
        emit migrationMilestone(QString("Migration %1% complete").arg(progress, 0, 'f', 1), progress);
        last_milestone = progress;
    }
}

void UnifiedETIProcessingEngine::updatePerformanceComparison() {
    auto comparison = getPerformanceComparison();
    emit performanceComparisonUpdate(comparison);
    
    Logger::instance().log(Logger::Debug, "UnifiedETIProcessingEngine", 
                          QString("Performance comparison: wrapper=%.1f fps, integrated=%.1f fps, improvement=%.1f%%")
                          .arg(comparison.wrapper_avg_fps)
                          .arg(comparison.integrated_avg_fps)
                          .arg(comparison.speed_improvement_percent));
}

// Helper implementations for other methods
double MigrationProgress::getOverallProgress() const {
    return components.overall_migration_percent.load();
}

std::string MigrationProgress::generateProgressReport() const {
    std::ostringstream report;
    report << "Migration Progress Report:\n";
    report << "  Overall Progress: " << getOverallProgress() << "%\n";
    report << "  Frames Processed: " << total_frames_processed.load() << "\n";
    report << "  Performance Wins - Integrated: " << performance_wins_integrated.load() 
           << ", Wrapper: " << performance_wins_wrapper.load() << "\n";
    report << "  Result Mismatches: " << result_mismatches.load() << "\n";
    report << "  Fallback Activations: " << fallback_activations.load() << "\n";
    return report.str();
}

// Factory function
std::unique_ptr<UnifiedETIProcessingEngine> createConfiguredEngine(const EngineConfiguration& config) {
    auto engine = std::make_unique<UnifiedETIProcessingEngine>();
    
    if (!engine->initialize(config)) {
        Logger::instance().log(Logger::Error, "createConfiguredEngine", 
                              "Failed to create configured engine");
        return nullptr;
    }
    
    return engine;
}

} // namespace eti::processing