/**
 * @file modern_eti_controller.cpp
 * @brief Modern ETI Processing Controller Implementation
 * 
 * Central controller implementation that orchestrates the complete ETI
 * processing pipeline with Qt6 signals/slots integration.
 * 
 * @author StreamDAB Development Team
 * @date 2025
 */

#include "modern_eti_controller.hpp"
#include "utils/logger.h"

#include <QMutexLocker>
#include <QTimer>
#include <algorithm>
#include <numeric>

namespace eti::controller {

// ============================================================================
// ModernETIController Implementation
// ============================================================================

ModernETIController::ModernETIController(QObject* parent)
    : QObject(parent)
    , start_time_(std::chrono::system_clock::now())
{
    // Initialize statistics timer
    stats_timer_ = new QTimer(this);
    stats_timer_->setInterval(1000); // Update every second
    connect(stats_timer_, &QTimer::timeout, this, &ModernETIController::updateStatistics);
    
    // Reserve space for performance tracking
    recent_processing_times_.reserve(1000);
    
    Logger::instance().log(Logger::Info, "ModernETIController", 
                          "Modern ETI controller created");
}

ModernETIController::~ModernETIController()
{
    stopProcessing();
    
    Logger::instance().log(Logger::Info, "ModernETIController", 
                          "Modern ETI controller destroyed");
}

bool ModernETIController::initialize(const ProcessingConfig& config)
{
    if (initialized_.load()) {
        Logger::instance().log(Logger::Warning, "ModernETIController", 
                              "Controller already initialized");
        return true;
    }
    
    if (!config.isValid()) {
        Logger::instance().log(Logger::Error, "ModernETIController", 
                              "Invalid configuration provided");
        return false;
    }
    
    try {
        // Store configuration
        {
            QMutexLocker locker(&config_mutex_);
            config_ = config;
        }
        
        // Create ZeroMQ ETI client
        zmq_client_ = std::make_shared<eti::network::ZeroMQETIClient>(this);
        if (!zmq_client_->initialize()) {
            Logger::instance().log(Logger::Error, "ModernETIController", 
                                  "Failed to initialize ZeroMQ client");
            return false;
        }
        
        // Create modern ETI frame parser
        frame_parser_ = std::make_shared<eti::modern::ModernETIFrameParser>(this);
        if (!frame_parser_->initialize(config.frame_parser_config)) {
            Logger::instance().log(Logger::Error, "ModernETIController", 
                                  "Failed to initialize frame parser");
            return false;
        }
        
        // Create ETI-NI processor
        if (config.enable_eti_ni_processing) {
            eti_ni_processor_ = std::make_shared<eti::ni::ETINIProcessor>(this);
            if (!eti_ni_processor_->initialize()) {
                Logger::instance().log(Logger::Error, "ModernETIController", 
                                      "Failed to initialize ETI-NI processor");
                return false;
            }
        }
        
        // Create Reed-Solomon processor
        if (config.enable_reed_solomon_correction) {
            reed_solomon_processor_ = std::make_shared<eti::codec::ETIReedSolomonProcessor>(this);
            if (!reed_solomon_processor_->initialize()) {
                Logger::instance().log(Logger::Error, "ModernETIController", 
                                      "Failed to initialize Reed-Solomon processor");
                return false;
            }
        }
        
        // Set up component relationships
        zmq_client_->setFrameParser(frame_parser_);
        zmq_client_->setAutoReconnect(config.auto_reconnect, config.reconnect_interval_ms);
        
        // Connect all signals and slots
        connectSignals();
        
        // Initialize statistics
        resetStatistics();
        
        // Set component state
        eti_ni_enabled_ = config.enable_eti_ni_processing;
        reed_solomon_enabled_ = config.enable_reed_solomon_correction;
        etsi_compliance_enabled_ = config.enable_etsi_compliance_checking;
        performance_monitoring_enabled_ = config.enable_performance_monitoring;
        
        if (performance_monitoring_enabled_) {
            stats_timer_->start();
        }
        
        initialized_.store(true);
        
        Logger::instance().log(Logger::Info, "ModernETIController", 
                              QString("Controller initialized with target %1 FPS")
                              .arg(config.target_fps, 0, 'f', 1));
        
        emit initialized(true);
        return true;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "ModernETIController", 
                              QString("Initialization failed: %1").arg(e.what()));
        
        emit initialized(false);
        return false;
    }
}

bool ModernETIController::startProcessing()
{
    if (!initialized_.load()) {
        Logger::instance().log(Logger::Error, "ModernETIController", 
                              "Cannot start processing: not initialized");
        return false;
    }
    
    if (processing_.load()) {
        Logger::instance().log(Logger::Warning, "ModernETIController", 
                              "Processing already started");
        return true;
    }
    
    // Start ZeroMQ reception
    if (!zmq_client_->startReceiving()) {
        Logger::instance().log(Logger::Error, "ModernETIController", 
                              "Failed to start ZeroMQ reception");
        return false;
    }
    
    processing_.store(true);
    start_time_ = std::chrono::system_clock::now();
    
    Logger::instance().log(Logger::Info, "ModernETIController", 
                          "ETI processing started");
    
    emit processingStateChanged(true);
    return true;
}

void ModernETIController::stopProcessing()
{
    if (!processing_.load()) {
        return;
    }
    
    processing_.store(false);
    
    // Stop ZeroMQ reception
    if (zmq_client_) {
        zmq_client_->stopReceiving();
    }
    
    // Stop statistics timer
    if (stats_timer_) {
        stats_timer_->stop();
    }
    
    Logger::instance().log(Logger::Info, "ModernETIController", 
                          "ETI processing stopped");
    
    emit processingStateChanged(false);
}

bool ModernETIController::connectToStream(const QString& endpoint)
{
    if (!initialized_.load()) {
        Logger::instance().log(Logger::Error, "ModernETIController", 
                              "Cannot connect: not initialized");
        return false;
    }
    
    return zmq_client_->connectToEndpoint(endpoint);
}

void ModernETIController::disconnectFromStream()
{
    if (zmq_client_) {
        zmq_client_->disconnectFromStream();
    }
}

ProcessingStatistics ModernETIController::getStatistics() const
{
    QMutexLocker locker(&stats_mutex_);
    return statistics_;
}

bool ModernETIController::updateConfiguration(const ProcessingConfig& config)
{
    if (!config.isValid()) {
        Logger::instance().log(Logger::Error, "ModernETIController", 
                              "Invalid configuration update");
        return false;
    }
    
    {
        QMutexLocker locker(&config_mutex_);
        config_ = config;
    }
    
    // Update frame parser configuration
    if (frame_parser_) {
        frame_parser_->updateConfiguration(config.frame_parser_config);
    }
    
    // Update component states
    setETINIProcessingEnabled(config.enable_eti_ni_processing);
    setReedSolomonCorrectionEnabled(config.enable_reed_solomon_correction);
    setETSIComplianceCheckingEnabled(config.enable_etsi_compliance_checking);
    setPerformanceMonitoringEnabled(config.enable_performance_monitoring);
    
    Logger::instance().log(Logger::Info, "ModernETIController", 
                          "Configuration updated");
    
    return true;
}

void ModernETIController::resetStatistics()
{
    QMutexLocker locker(&stats_mutex_);
    
    statistics_ = ProcessingStatistics{};
    frame_counter_.store(0);
    recent_processing_times_.clear();
    start_time_ = std::chrono::system_clock::now();
    
    // Reset component statistics
    if (zmq_client_) {
        zmq_client_->resetStats();
    }
    
    if (frame_parser_) {
        frame_parser_->resetStats();
    }
    
    if (eti_ni_processor_) {
        eti_ni_processor_->resetSynchronization();
    }
    
    Logger::instance().log(Logger::Info, "ModernETIController", 
                          "Statistics reset");
}

void ModernETIController::setETINIProcessingEnabled(bool enabled)
{
    eti_ni_enabled_ = enabled;
    
    if (!enabled && eti_ni_processor_) {
        // Disconnect ETI-NI processor signals
        disconnect(eti_ni_processor_.get(), nullptr, this, nullptr);
    } else if (enabled && eti_ni_processor_) {
        // Reconnect ETI-NI processor signals
        connect(eti_ni_processor_.get(), &eti::ni::ETINIProcessor::frameProcessed,
                this, &ModernETIController::handleETINIFrame);
        connect(eti_ni_processor_.get(), &eti::ni::ETINIProcessor::tistExtracted,
                this, &ModernETIController::handleTISTExtracted);
        connect(eti_ni_processor_.get(), &eti::ni::ETINIProcessor::synchronizationStatusChanged,
                this, &ModernETIController::handleSynchronizationStatusChanged);
    }
    
    Logger::instance().log(Logger::Info, "ModernETIController", 
                          QString("ETI-NI processing %1").arg(enabled ? "enabled" : "disabled"));
}

void ModernETIController::setReedSolomonCorrectionEnabled(bool enabled)
{
    reed_solomon_enabled_ = enabled;
    
    if (!enabled && reed_solomon_processor_) {
        // Disconnect Reed-Solomon processor signals
        disconnect(reed_solomon_processor_.get(), nullptr, this, nullptr);
    } else if (enabled && reed_solomon_processor_) {
        // Reconnect Reed-Solomon processor signals
        connect(reed_solomon_processor_.get(), &eti::codec::ETIReedSolomonProcessor::etiFrameCorrected,
                this, &ModernETIController::handleErrorsCorrected);
        connect(reed_solomon_processor_.get(), &eti::codec::ETIReedSolomonProcessor::etiFrameUncorrectable,
                this, [this](uint32_t frame_number, const QString& error_description) {
                    emit uncorrectableErrors(frame_number, error_description);
                });
    }
    
    Logger::instance().log(Logger::Info, "ModernETIController", 
                          QString("Reed-Solomon correction %1").arg(enabled ? "enabled" : "disabled"));
}

void ModernETIController::setETSIComplianceCheckingEnabled(bool enabled)
{
    etsi_compliance_enabled_ = enabled;
    
    Logger::instance().log(Logger::Info, "ModernETIController", 
                          QString("ETSI compliance checking %1").arg(enabled ? "enabled" : "disabled"));
}

void ModernETIController::setPerformanceMonitoringEnabled(bool enabled)
{
    performance_monitoring_enabled_ = enabled;
    
    if (enabled && stats_timer_) {
        stats_timer_->start();
    } else if (!enabled && stats_timer_) {
        stats_timer_->stop();
    }
    
    Logger::instance().log(Logger::Info, "ModernETIController", 
                          QString("Performance monitoring %1").arg(enabled ? "enabled" : "disabled"));
}

// ============================================================================
// Public Slots
// ============================================================================

void ModernETIController::connectToEndpoint(const QString& endpoint)
{
    connectToStream(endpoint);
}

void ModernETIController::reconnect()
{
    if (zmq_client_) {
        zmq_client_->reconnect();
    }
}

void ModernETIController::setTargetFPS(double fps)
{
    QMutexLocker locker(&config_mutex_);
    config_.target_fps = fps;
    
    if (frame_parser_) {
        auto parser_config = config_.frame_parser_config;
        parser_config.target_fps = fps;
        frame_parser_->updateConfiguration(parser_config);
    }
}

void ModernETIController::setSignalQualityThreshold(double quality)
{
    QMutexLocker locker(&config_mutex_);
    config_.min_signal_quality = quality;
}

// ============================================================================
// Private Slots - Signal Handlers
// ============================================================================

void ModernETIController::handleRawETIFrame(const QByteArray& frame_data,
                                           const std::chrono::system_clock::time_point& receive_timestamp)
{
    if (!processing_.load()) {
        return;
    }
    
    auto processing_start = std::chrono::system_clock::now();
    uint32_t frame_number = frame_counter_.fetch_add(1) + 1;
    
    last_frame_time_ = receive_timestamp;
    
    // Process frame through pipeline
    processFrameThroughPipeline(frame_data, receive_timestamp);
    
    // Calculate processing time
    auto processing_end = std::chrono::system_clock::now();
    auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(
        processing_end - processing_start);
    
    // Update performance tracking
    recent_processing_times_.push_back(processing_time);
    if (recent_processing_times_.size() > 1000) {
        recent_processing_times_.erase(recent_processing_times_.begin(),
                                      recent_processing_times_.begin() + 100);
    }
    
    Logger::instance().log(Logger::Debug, "ModernETIController", 
                          QString("Processed frame #%1 in %2μs")
                          .arg(frame_number)
                          .arg(processing_time.count()));
}

void ModernETIController::handleParsedETIFrame(uint32_t frame_number,
                                              const eti::EtiFrame& frame,
                                              std::chrono::nanoseconds parse_time)
{
    auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(parse_time);
    
    // Update statistics
    {
        QMutexLocker locker(&stats_mutex_);
        statistics_.total_frames_processed++;
        
        if (processing_time > statistics_.max_processing_latency) {
            statistics_.max_processing_latency = processing_time;
        }
    }
    
    emit etiFrameProcessed(frame_number, frame, processing_time);
    
    // Process through Reed-Solomon if enabled
    if (reed_solomon_enabled_ && reed_solomon_processor_) {
        auto frame_span = std::span<const uint8_t, 6144>(frame.data(), 6144);
        auto correction_result = reed_solomon_processor_->processETIFrame(frame_span);
        
        if (correction_result.wasSuccessful() && correction_result.hasErrors()) {
            emit errorsCorrect(frame_number, correction_result);
        }
    }
}

void ModernETIController::handleETINIFrame(const eti::ni::ETINIFrame& frame)
{
    double sync_quality = frame.tist_info.getSyncQuality();
    
    emit etiNIFrameProcessed(frame, sync_quality);
    
    // Update synchronization statistics
    {
        QMutexLocker locker(&stats_mutex_);
        statistics_.tist_sync_status = frame.tist_info.type;
        statistics_.sync_accuracy = frame.tist_info.sync_accuracy;
    }
}

void ModernETIController::handleTISTExtracted(const eti::ni::TISTInfo& tist_info, uint32_t frame_number)
{
    // Update synchronization metrics
    {
        QMutexLocker locker(&stats_mutex_);
        statistics_.sync_accuracy = tist_info.sync_accuracy;
    }
    
    // Check synchronization status
    bool synchronized = tist_info.isValid() && 
                       tist_info.sync_accuracy < std::chrono::microseconds{2000};
    
    static bool last_sync_status = false;
    if (synchronized != last_sync_status) {
        emit synchronizationStatusChanged(synchronized, tist_info.sync_accuracy);
        last_sync_status = synchronized;
    }
}

void ModernETIController::handleEnsembleDiscovered(const eti::Ensemble& ensemble)
{
    // Update ensemble information
    {
        QMutexLocker locker(&stats_mutex_);
        statistics_.ensemble_label = QString::fromStdString(ensemble.label);
        statistics_.ensemble_id = ensemble.ensemble_id;
    }
    
    emit ensembleDiscovered(ensemble);
}

void ModernETIController::handleServiceDiscovered(const eti::DabService& service)
{
    // Update service count
    {
        QMutexLocker locker(&stats_mutex_);
        statistics_.discovered_services++;
    }
    
    emit serviceDiscovered(service);
}

void ModernETIController::handleErrorsCorrected(uint32_t frame_number,
                                               const eti::codec::ETIReedSolomonProcessor::ETIErrorStats& stats)
{
    // Update error statistics
    {
        QMutexLocker locker(&stats_mutex_);
        if (stats.total_errors_corrected > 0) {
            statistics_.frames_corrected++;
            statistics_.total_errors_corrected += stats.total_errors_corrected;
        }
        
        if (stats.hasSignificantErrors()) {
            statistics_.frames_with_errors++;
        }
        
        // Update signal integrity
        statistics_.overall_signal_quality = 
            (statistics_.overall_signal_quality * 0.95) + (stats.signal_integrity * 0.05);
    }
    
    eti::codec::CorrectionResult correction_result;
    correction_result.status = stats.overall_status;
    correction_result.errors_corrected = stats.total_errors_corrected;
    
    emit errorsCorrect(frame_number, correction_result);
}

void ModernETIController::handleConnectionStatusChanged(const eti::network::ConnectionStatus& status)
{
    // Update connection statistics
    {
        QMutexLocker locker(&stats_mutex_);
        statistics_.connection_status = status;
        
        if (status.isConnected()) {
            statistics_.connection_uptime = status.getConnectionDuration();
        }
    }
    
    emit connectionStatusChanged(status);
}

void ModernETIController::handlePerformanceUpdate(const eti::network::ZeroMQETIClient::PerformanceStats& stats)
{
    // Update performance statistics
    {
        QMutexLocker locker(&stats_mutex_);
        statistics_.current_fps = stats.current_fps;
        statistics_.average_fps = stats.average_fps;
    }
    
    // Check performance targets
    checkPerformanceTargets();
}

void ModernETIController::handleSynchronizationStatusChanged(const eti::ni::ETINIProcessor::SyncStatus& status)
{
    emit synchronizationStatusChanged(status.is_synchronized, status.sync_accuracy);
}

void ModernETIController::updateStatistics()
{
    if (!performance_monitoring_enabled_) {
        return;
    }
    
    updateProcessingStatistics();
    updateQualityMetrics();
    validateSignalQuality();
    
    // Emit updated statistics
    auto stats = getStatistics();
    emit statisticsUpdated(stats);
}

// ============================================================================
// Private Implementation Methods
// ============================================================================

void ModernETIController::connectSignals()
{
    if (!zmq_client_) return;
    
    // ZeroMQ client signals
    connect(zmq_client_.get(), &eti::network::ZeroMQETIClient::etiFrameReceived,
            this, &ModernETIController::handleRawETIFrame);
    connect(zmq_client_.get(), &eti::network::ZeroMQETIClient::connectionStatusChanged,
            this, &ModernETIController::handleConnectionStatusChanged);
    connect(zmq_client_.get(), &eti::network::ZeroMQETIClient::performanceUpdate,
            this, &ModernETIController::handlePerformanceUpdate);
    
    // Frame parser signals
    if (frame_parser_) {
        connect(frame_parser_.get(), &eti::modern::ModernETIFrameParser::frameProcessed,
                this, &ModernETIController::handleParsedETIFrame);
        connect(frame_parser_.get(), &eti::modern::ModernETIFrameParser::ensembleDiscovered,
                this, &ModernETIController::handleEnsembleDiscovered);
        connect(frame_parser_.get(), &eti::modern::ModernETIFrameParser::serviceDiscovered,
                this, &ModernETIController::handleServiceDiscovered);
    }
    
    // ETI-NI processor signals
    if (eti_ni_processor_) {
        connect(eti_ni_processor_.get(), &eti::ni::ETINIProcessor::frameProcessed,
                this, &ModernETIController::handleETINIFrame);
        connect(eti_ni_processor_.get(), &eti::ni::ETINIProcessor::tistExtracted,
                this, &ModernETIController::handleTISTExtracted);
        connect(eti_ni_processor_.get(), &eti::ni::ETINIProcessor::synchronizationStatusChanged,
                this, &ModernETIController::handleSynchronizationStatusChanged);
    }
    
    // Reed-Solomon processor signals
    if (reed_solomon_processor_) {
        connect(reed_solomon_processor_.get(), &eti::codec::ETIReedSolomonProcessor::etiFrameCorrected,
                this, &ModernETIController::handleErrorsCorrected);
        connect(reed_solomon_processor_.get(), &eti::codec::ETIReedSolomonProcessor::etiFrameUncorrectable,
                this, [this](uint32_t frame_number, const QString& error_description) {
                    emit uncorrectableErrors(frame_number, error_description);
                });
    }
}

void ModernETIController::disconnectSignals()
{
    if (zmq_client_) {
        disconnect(zmq_client_.get(), nullptr, this, nullptr);
    }
    if (frame_parser_) {
        disconnect(frame_parser_.get(), nullptr, this, nullptr);
    }
    if (eti_ni_processor_) {
        disconnect(eti_ni_processor_.get(), nullptr, this, nullptr);
    }
    if (reed_solomon_processor_) {
        disconnect(reed_solomon_processor_.get(), nullptr, this, nullptr);
    }
}

void ModernETIController::processFrameThroughPipeline(const QByteArray& frame_data,
                                                     const std::chrono::system_clock::time_point& receive_timestamp)
{
    try {
        // Process through ETI-NI processor if enabled
        if (eti_ni_enabled_ && eti_ni_processor_) {
            auto eti_ni_frame = eti_ni_processor_->processFrame(frame_data);
            
            // ETI-NI frame will be handled by signal
        }
        
        // Frame parsing is handled by the ZeroMQ client automatically
        // through the frame parser connection
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "ModernETIController", 
                              QString("Pipeline processing error: %1").arg(e.what()));
    }
}

void ModernETIController::updateProcessingStatistics()
{
    QMutexLocker locker(&stats_mutex_);
    
    // Calculate average processing latency
    if (!recent_processing_times_.empty()) {
        auto total_time = std::accumulate(recent_processing_times_.begin(),
                                         recent_processing_times_.end(),
                                         std::chrono::microseconds{0});
        statistics_.avg_processing_latency = total_time / recent_processing_times_.size();
    }
    
    // Calculate current FPS
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    
    if (elapsed.count() > 0) {
        statistics_.average_fps = static_cast<double>(statistics_.total_frames_processed) / elapsed.count();
    }
    
    // Calculate error correction efficiency
    if (statistics_.total_errors_detected > 0) {
        statistics_.error_correction_efficiency = 
            static_cast<double>(statistics_.total_errors_corrected) / 
            statistics_.total_errors_detected * 100.0;
    }
}

void ModernETIController::updateQualityMetrics()
{
    QMutexLocker locker(&stats_mutex_);
    
    // Calculate overall signal quality
    statistics_.overall_signal_quality = calculateOverallQuality();
    
    // Calculate ETSI compliance score
    statistics_.etsi_compliance_score = calculateETSIComplianceScore();
}

void ModernETIController::checkPerformanceTargets()
{
    QMutexLocker config_locker(&config_mutex_);
    double target_fps = config_.target_fps;
    config_locker.unlock();
    
    QMutexLocker stats_locker(&stats_mutex_);
    double current_fps = statistics_.current_fps;
    stats_locker.unlock();
    
    bool target_met = current_fps >= target_fps;
    emit performanceTargetStatus(target_met, current_fps, target_fps);
}

void ModernETIController::validateSignalQuality()
{
    QMutexLocker config_locker(&config_mutex_);
    double threshold = config_.min_signal_quality;
    config_locker.unlock();
    
    QMutexLocker stats_locker(&stats_mutex_);
    double quality = statistics_.overall_signal_quality;
    stats_locker.unlock();
    
    static double last_quality = 100.0;
    if (std::abs(quality - last_quality) > 5.0) { // 5% change threshold
        emit signalQualityChanged(quality, threshold);
        last_quality = quality;
    }
}

double ModernETIController::calculateOverallQuality() const
{
    // Simplified quality calculation based on error rates and sync status
    double quality = 100.0;
    
    if (statistics_.total_frames_processed > 0) {
        double error_rate = static_cast<double>(statistics_.frames_with_errors) / 
                           statistics_.total_frames_processed;
        quality -= error_rate * 30.0; // Up to 30% penalty for errors
    }
    
    if (statistics_.sync_accuracy > std::chrono::microseconds{2000}) {
        quality -= 20.0; // Penalty for poor synchronization
    }
    
    return std::max(0.0, quality);
}

double ModernETIController::calculateETSIComplianceScore() const
{
    // Simplified compliance calculation
    double score = 100.0;
    
    if (statistics_.frames_uncorrectable > 0) {
        score -= 50.0; // Major penalty for uncorrectable frames
    }
    
    if (statistics_.error_correction_efficiency < 95.0) {
        score -= (95.0 - statistics_.error_correction_efficiency);
    }
    
    return std::max(0.0, score);
}

// ============================================================================
// Factory and Utility Functions
// ============================================================================

std::unique_ptr<ModernETIController> createETIController(const ProcessingConfig& config,
                                                        QObject* parent)
{
    auto controller = std::make_unique<ModernETIController>(parent);
    
    if (!controller->initialize(config)) {
        Logger::instance().log(Logger::Error, "createETIController", 
                              "Failed to create ETI controller");
        return nullptr;
    }
    
    return controller;
}

namespace utils {

ProcessingConfig createDefaultConfig(const QString& endpoint)
{
    ProcessingConfig config;
    
    // ZeroMQ configuration
    config.zmq_config = eti::network::zmq_utils::parseEndpointUrl(endpoint);
    config.auto_reconnect = true;
    config.reconnect_interval_ms = 5000;
    
    // Processing configuration
    config.frame_parser_config = eti::modern::ProcessingConfig{};
    config.enable_eti_ni_processing = true;
    config.enable_reed_solomon_correction = true;
    
    // Performance settings
    config.target_fps = 1000.0;
    config.max_frame_buffer_size = 1000;
    config.enable_performance_monitoring = true;
    config.stats_update_interval_ms = 1000;
    
    // Quality settings
    config.enable_etsi_compliance_checking = true;
    config.enable_error_recovery = true;
    config.min_signal_quality = 80.0;
    
    return config;
}

ProcessingConfig createHighPerformanceConfig(const QString& endpoint)
{
    auto config = createDefaultConfig(endpoint);
    
    // Optimize for performance
    config.target_fps = 2000.0;
    config.frame_parser_config.optimization_level = eti::modern::ProcessingConfig::OptimizationLevel::AGGRESSIVE;
    config.frame_parser_config.enable_threading = true;
    config.frame_parser_config.enable_simd = true;
    config.frame_parser_config.enable_zero_copy = true;
    
    return config;
}

ProcessingConfig createQualityConfig(const QString& endpoint)
{
    auto config = createDefaultConfig(endpoint);
    
    // Optimize for quality
    config.target_fps = 500.0; // Lower target for more thorough processing
    config.frame_parser_config.optimization_level = eti::modern::ProcessingConfig::OptimizationLevel::STANDARD;
    config.enable_reed_solomon_correction = true;
    config.enable_etsi_compliance_checking = true;
    config.min_signal_quality = 95.0;
    
    return config;
}

HealthCheck checkControllerHealth(const ModernETIController& controller)
{
    HealthCheck health;
    
    if (!controller.isInitialized()) {
        health.issues.push_back("Controller not initialized");
        health.recommendations.push_back("Initialize controller before use");
        return health;
    }
    
    auto stats = controller.getStatistics();
    health.health_score = stats.getHealthScore();
    health.healthy = stats.isHealthy();
    
    // Check connection
    if (!stats.connection_status.isConnected()) {
        health.issues.push_back("Not connected to ETI stream");
        health.recommendations.push_back("Check ZeroMQ endpoint and network connectivity");
    }
    
    // Check performance
    if (stats.current_fps < 100.0) {
        health.issues.push_back("Low processing performance");
        health.recommendations.push_back("Check system resources and optimize configuration");
    }
    
    // Check signal quality
    if (stats.overall_signal_quality < 80.0) {
        health.issues.push_back("Poor signal quality");
        health.recommendations.push_back("Check signal source and transmission quality");
    }
    
    // Check synchronization
    if (stats.sync_accuracy > std::chrono::microseconds{5000}) {
        health.issues.push_back("Poor synchronization accuracy");
        health.recommendations.push_back("Check TIST configuration and network timing");
    }
    
    return health;
}

} // namespace utils

} // namespace eti::controller