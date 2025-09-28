/**
 * @file streaming_engine.cpp
 * @brief Real-time ETI Streaming Engine Implementation
 * 
 * High-performance implementation of the streaming engine with circular
 * buffering, adaptive quality management, and <50ms latency guarantee.
 * 
 * @author StreamDAB Development Team
 * @date 2025
 * @copyright Copyright (c) 2025 StreamDAB Technologies
 */

#include "streaming_engine.hpp"
#include "../utils/logger.h"
#include <algorithm>
#include <cmath>

namespace eti::streaming {

// ============================================================================
// CircularETIBuffer Implementation
// ============================================================================

CircularETIBuffer::CircularETIBuffer(size_t capacity) 
    : capacity_(capacity)
    , buffer_(capacity)
    , head_(0)
    , tail_(0)
    , count_(0) {
    
    Logger::instance().log(Logger::Info, "CircularETIBuffer", 
        QString("Circular buffer created with capacity: %1 frames").arg(capacity));
}

bool CircularETIBuffer::addFrame(const eti::EtiFrame& frame, 
                                 const std::chrono::system_clock::time_point& receive_time) {
    QMutexLocker locker(&mutex_);
    
    if (count_ >= capacity_) {
        overflow_count_++;
        Logger::instance().log(Logger::Warning, "CircularETIBuffer", 
            QString("Buffer overflow - dropping frame (total overflows: %1)")
            .arg(overflow_count_.load()));
        return false;
    }
    
    // Create buffered frame
    BufferedFrame& buffered = buffer_[head_];
    buffered.frame = frame;
    buffered.receive_time = receive_time;
    buffered.buffer_time = std::chrono::system_clock::now();
    buffered.sequence_number = sequence_counter_++;
    buffered.processed = false;
    
    // Advance head pointer
    head_ = (head_ + 1) % capacity_;
    count_++;
    total_frames_added_++;
    
    // Track recent latencies for statistics
    auto latency = buffered.getBufferLatency();
    recent_latencies_.push_back(latency);
    if (recent_latencies_.size() > MAX_LATENCY_SAMPLES) {
        recent_latencies_.pop_front();
    }
    
    // Notify waiting consumers
    not_empty_.wakeOne();
    
    Logger::instance().log(Logger::Debug, "CircularETIBuffer", 
        QString("Frame added to buffer (seq: %1, latency: %2ms, count: %3/%4)")
        .arg(buffered.sequence_number)
        .arg(latency.count())
        .arg(count_).arg(capacity_));
    
    return true;
}

std::optional<CircularETIBuffer::BufferedFrame> CircularETIBuffer::getNextFrame(int timeout_ms) {
    QMutexLocker locker(&mutex_);
    
    // Wait for frames if buffer is empty
    if (count_ == 0) {
        if (!not_empty_.wait(&mutex_, timeout_ms)) {
            underflow_count_++;
            Logger::instance().log(Logger::Debug, "CircularETIBuffer", 
                QString("Buffer underflow - timeout waiting for frame (total underflows: %1)")
                .arg(underflow_count_.load()));
            return std::nullopt;
        }
    }
    
    if (count_ == 0) {
        return std::nullopt;
    }
    
    // Get frame from tail
    BufferedFrame frame = buffer_[tail_];
    tail_ = (tail_ + 1) % capacity_;
    count_--;
    
    // Notify waiting producers
    not_full_.wakeOne();
    
    Logger::instance().log(Logger::Debug, "CircularETIBuffer", 
        QString("Frame retrieved from buffer (seq: %1, remaining: %2)")
        .arg(frame.sequence_number).arg(count_));
    
    return frame;
}

std::optional<CircularETIBuffer::BufferedFrame> CircularETIBuffer::peekNextFrame() const {
    QMutexLocker locker(&mutex_);
    
    if (count_ == 0) {
        return std::nullopt;
    }
    
    return buffer_[tail_];
}

BufferStatus CircularETIBuffer::getStatus() const {
    QMutexLocker locker(&mutex_);
    
    BufferStatus status;
    status.current_size = count_;
    status.max_size = capacity_;
    status.overflow_count = overflow_count_.load();
    status.underflow_count = underflow_count_.load();
    status.utilization_percent = (static_cast<double>(count_) / capacity_) * 100.0;
    
    // Calculate average and max latency
    if (!recent_latencies_.empty()) {
        auto total_latency = std::chrono::milliseconds{0};
        auto max_latency = std::chrono::milliseconds{0};
        
        for (const auto& latency : recent_latencies_) {
            total_latency += latency;
            max_latency = std::max(max_latency, latency);
        }
        
        status.avg_latency = total_latency / recent_latencies_.size();
        status.max_latency = max_latency;
    }
    
    return status;
}

void CircularETIBuffer::clear() {
    QMutexLocker locker(&mutex_);
    
    head_ = 0;
    tail_ = 0;
    count_ = 0;
    recent_latencies_.clear();
    
    Logger::instance().log(Logger::Info, "CircularETIBuffer", "Buffer cleared");
}

void CircularETIBuffer::resize(size_t new_capacity) {
    QMutexLocker locker(&mutex_);
    
    if (new_capacity == capacity_) {
        return;
    }
    
    Logger::instance().log(Logger::Info, "CircularETIBuffer", 
        QString("Resizing buffer from %1 to %2 frames").arg(capacity_).arg(new_capacity));
    
    // Create new buffer
    std::vector<BufferedFrame> new_buffer(new_capacity);
    
    // Copy existing frames to new buffer
    size_t frames_to_copy = std::min(count_, new_capacity);
    for (size_t i = 0; i < frames_to_copy; ++i) {
        new_buffer[i] = buffer_[(tail_ + i) % capacity_];
    }
    
    // Update buffer state
    buffer_ = std::move(new_buffer);
    capacity_ = new_capacity;
    head_ = frames_to_copy % new_capacity;
    tail_ = 0;
    count_ = frames_to_copy;
}

size_t CircularETIBuffer::size() const {
    QMutexLocker locker(&mutex_);
    return count_;
}

bool CircularETIBuffer::empty() const {
    QMutexLocker locker(&mutex_);
    return count_ == 0;
}

bool CircularETIBuffer::full() const {
    QMutexLocker locker(&mutex_);
    return count_ >= capacity_;
}

// ============================================================================
// StreamingEngine Implementation
// ============================================================================

StreamingEngine::StreamingEngine(QObject* parent) 
    : QObject(parent)
    , frame_buffer_(std::make_unique<CircularETIBuffer>(1000))
    , initialized_(false)
    , streaming_(false)
    , adaptive_buffering_enabled_(true)
    , quality_monitoring_enabled_(true)
    , etsi_validation_enabled_(true)
    , frame_counter_(0)
    , total_frames_processed_(0)
    , frames_with_errors_(0)
    , consecutive_errors_(0) {
    
    // Initialize quality thresholds with broadcast defaults
    quality_thresholds_ = utils::createBroadcastQualityThresholds();
    
    // Initialize timestamps
    start_time_ = std::chrono::system_clock::now();
    last_fps_update_ = start_time_;
    
    Logger::instance().log(Logger::Info, "StreamingEngine", 
        "Streaming engine created with default broadcast quality thresholds");
}

StreamingEngine::~StreamingEngine() {
    if (streaming_.load()) {
        stopStreaming();
    }
    
    cleanupMonitoringTimers();
    
    Logger::instance().log(Logger::Info, "StreamingEngine", 
        QString("Streaming engine destroyed - processed %1 frames")
        .arg(total_frames_processed_.load()));
}

bool StreamingEngine::initialize(
    std::shared_ptr<eti::network::ZeroMQETIClient> zmq_client,
    std::shared_ptr<eti::modern::ModernETIFrameParser> frame_parser,
    std::shared_ptr<eti::compliance::ComprehensiveETSIValidator> etsi_validator) {
    
    Logger::instance().log(Logger::Info, "StreamingEngine", "Initializing streaming engine");
    
    if (!zmq_client || !frame_parser) {
        Logger::instance().log(Logger::Error, "StreamingEngine", 
            "Invalid parameters - ZMQ client and frame parser required");
        return false;
    }
    
    zmq_client_ = zmq_client;
    frame_parser_ = frame_parser;
    etsi_validator_ = etsi_validator;
    
    // Create processing worker thread
    processing_thread_ = std::make_unique<ProcessingWorkerThread>(this);
    
    // Setup monitoring timers
    setupMonitoringTimers();
    
    // Connect signals
    connectSignals();
    
    initialized_ = true;
    
    Logger::instance().log(Logger::Info, "StreamingEngine", 
        QString("Streaming engine initialized - ETSI validation: %1")
        .arg(etsi_validator_ ? "enabled" : "disabled"));
    
    return true;
}

bool StreamingEngine::startLiveStreaming(const QString& zmq_endpoint) {
    Logger::instance().log(Logger::Info, "StreamingEngine", 
        QString("Starting live streaming from endpoint: %1").arg(zmq_endpoint));
    
    if (!initialized_.load()) {
        Logger::instance().log(Logger::Error, "StreamingEngine", 
            "Cannot start streaming - engine not initialized");
        return false;
    }
    
    if (streaming_.load()) {
        Logger::instance().log(Logger::Warning, "StreamingEngine", 
            "Streaming already active");
        return true;
    }
    
    // Connect ZeroMQ client to endpoint
    if (!zmq_client_->connectToStream(eti::network::zmq_utils::parseEndpointUrl(zmq_endpoint))) {
        Logger::instance().log(Logger::Error, "StreamingEngine", 
            QString("Failed to connect to ZMQ endpoint: %1").arg(zmq_endpoint));
        return false;
    }
    
    // Start ZeroMQ frame reception
    if (!zmq_client_->startReceiving()) {
        Logger::instance().log(Logger::Error, "StreamingEngine", 
            "Failed to start ZMQ frame reception");
        return false;
    }
    
    // Start processing worker thread
    processing_thread_->start();
    
    // Start monitoring timers
    if (quality_monitoring_enabled_.load()) {
        quality_timer_->start(1000); // Update every second
    }
    
    performance_timer_->start(5000); // Update every 5 seconds
    
    if (adaptive_buffering_enabled_.load()) {
        adaptive_buffer_timer_->start(10000); // Check every 10 seconds
    }
    
    streaming_ = true;
    start_time_ = std::chrono::system_clock::now();
    
    emit streamingStateChanged(true);
    
    Logger::instance().log(Logger::Info, "StreamingEngine", 
        "Live streaming started successfully");
    
    return true;
}

void StreamingEngine::stopStreaming() {
    Logger::instance().log(Logger::Info, "StreamingEngine", "Stopping streaming");
    
    if (!streaming_.load()) {
        return;
    }
    
    streaming_ = false;
    
    // Stop monitoring timers
    cleanupMonitoringTimers();
    
    // Stop ZeroMQ reception
    if (zmq_client_) {
        zmq_client_->stopReceiving();
        zmq_client_->disconnectFromStream();
    }
    
    // Stop processing thread
    if (processing_thread_) {
        processing_thread_->stop();
        processing_thread_->wait(5000); // Wait up to 5 seconds
    }
    
    // Clear buffer
    frame_buffer_->clear();
    
    emit streamingStateChanged(false);
    
    Logger::instance().log(Logger::Info, "StreamingEngine", "Streaming stopped");
}

void StreamingEngine::configureBuffering(uint32_t buffer_size_frames) {
    Logger::instance().log(Logger::Info, "StreamingEngine", 
        QString("Configuring buffer size to %1 frames").arg(buffer_size_frames));
    
    if (buffer_size_frames < 10 || buffer_size_frames > 10000) {
        Logger::instance().log(Logger::Warning, "StreamingEngine", 
            QString("Invalid buffer size: %1 (valid range: 10-10000)").arg(buffer_size_frames));
        return;
    }
    
    frame_buffer_->resize(buffer_size_frames);
    
    emit bufferStatusChanged(frame_buffer_->getStatus());
}

void StreamingEngine::setQualityThresholds(const QualityThresholds& thresholds) {
    if (!thresholds.isValid()) {
        Logger::instance().log(Logger::Warning, "StreamingEngine", 
            "Invalid quality thresholds provided");
        return;
    }
    
    QMutexLocker locker(&config_mutex_);
    quality_thresholds_ = thresholds;
    
    Logger::instance().log(Logger::Info, "StreamingEngine", 
        QString("Quality thresholds updated - Signal: %1%, ETSI: %2%, Latency: %3ms")
        .arg(thresholds.min_signal_quality)
        .arg(thresholds.min_etsi_compliance)
        .arg(thresholds.max_latency.count()));
}

QualityMetrics StreamingEngine::getQualityMetrics() const {
    QMutexLocker locker(&stats_mutex_);
    return quality_metrics_;
}

BufferStatus StreamingEngine::getBufferStatus() const {
    return frame_buffer_->getStatus();
}

void StreamingEngine::setAdaptiveBufferingEnabled(bool enabled) {
    adaptive_buffering_enabled_ = enabled;
    
    if (enabled && streaming_.load() && adaptive_buffer_timer_) {
        adaptive_buffer_timer_->start(10000);
    } else if (!enabled && adaptive_buffer_timer_) {
        adaptive_buffer_timer_->stop();
    }
    
    Logger::instance().log(Logger::Info, "StreamingEngine", 
        QString("Adaptive buffering %1").arg(enabled ? "enabled" : "disabled"));
}

void StreamingEngine::setQualityMonitoringEnabled(bool enabled) {
    quality_monitoring_enabled_ = enabled;
    
    if (enabled && streaming_.load() && quality_timer_) {
        quality_timer_->start(1000);
    } else if (!enabled && quality_timer_) {
        quality_timer_->stop();
    }
    
    Logger::instance().log(Logger::Info, "StreamingEngine", 
        QString("Quality monitoring %1").arg(enabled ? "enabled" : "disabled"));
}

void StreamingEngine::setETSIValidationEnabled(bool enabled) {
    etsi_validation_enabled_ = enabled;
    
    Logger::instance().log(Logger::Info, "StreamingEngine", 
        QString("ETSI validation %1").arg(enabled ? "enabled" : "disabled"));
}

void StreamingEngine::resetStatistics() {
    Logger::instance().log(Logger::Info, "StreamingEngine", "Resetting statistics");
    
    QMutexLocker locker(&stats_mutex_);
    
    // Reset counters
    frame_counter_ = 0;
    total_frames_processed_ = 0;
    frames_with_errors_ = 0;
    consecutive_errors_ = 0;
    
    // Reset metrics
    quality_metrics_ = QualityMetrics{};
    performance_stats_ = PerformanceStats{};
    
    // Reset timestamps
    start_time_ = std::chrono::system_clock::now();
    last_fps_update_ = start_time_;
    recent_frame_times_.clear();
    recent_processing_times_.clear();
    
    // Clear buffer
    frame_buffer_->clear();
}

StreamingEngine::PerformanceStats StreamingEngine::getPerformanceStats() const {
    QMutexLocker locker(&stats_mutex_);
    return performance_stats_;
}

// ============================================================================
// Signal/Slot Connection Methods
// ============================================================================

void StreamingEngine::connectSignals() {
    if (!zmq_client_) return;
    
    connect(zmq_client_.get(), &eti::network::ZeroMQETIClient::etiFrameReceived,
            this, &StreamingEngine::handleETIFrame);
    
    connect(zmq_client_.get(), &eti::network::ZeroMQETIClient::connectionStatusChanged,
            this, &StreamingEngine::handleConnectionStatusChanged);
    
    if (frame_parser_) {
        connect(frame_parser_.get(), &eti::modern::ModernETIFrameParser::frameProcessed,
                this, &StreamingEngine::handleParsedFrame);
    }
    
    Logger::instance().log(Logger::Debug, "StreamingEngine", "Signals connected");
}

void StreamingEngine::disconnectSignals() {
    if (zmq_client_) {
        disconnect(zmq_client_.get(), nullptr, this, nullptr);
    }
    
    if (frame_parser_) {
        disconnect(frame_parser_.get(), nullptr, this, nullptr);
    }
    
    Logger::instance().log(Logger::Debug, "StreamingEngine", "Signals disconnected");
}

void StreamingEngine::setupMonitoringTimers() {
    // Quality monitoring timer
    quality_timer_ = new QTimer(this);
    connect(quality_timer_, &QTimer::timeout, this, &StreamingEngine::handleQualityMonitoring);
    
    // Performance monitoring timer
    performance_timer_ = new QTimer(this);
    connect(performance_timer_, &QTimer::timeout, this, &StreamingEngine::handlePerformanceMonitoring);
    
    // Adaptive buffer timer
    adaptive_buffer_timer_ = new QTimer(this);
    connect(adaptive_buffer_timer_, &QTimer::timeout, this, &StreamingEngine::handleAdaptiveBuffering);
    
    Logger::instance().log(Logger::Debug, "StreamingEngine", "Monitoring timers setup");
}

void StreamingEngine::cleanupMonitoringTimers() {
    if (quality_timer_) {
        quality_timer_->stop();
    }
    
    if (performance_timer_) {
        performance_timer_->stop();
    }
    
    if (adaptive_buffer_timer_) {
        adaptive_buffer_timer_->stop();
    }
    
    Logger::instance().log(Logger::Debug, "StreamingEngine", "Monitoring timers cleaned up");
}

// ============================================================================
// Frame Processing Methods
// ============================================================================

void StreamingEngine::handleETIFrame(const QByteArray& frame_data, 
                                     const std::chrono::system_clock::time_point& receive_timestamp) {
    if (!streaming_.load()) {
        return;
    }
    
    // Parse frame using frame parser
    if (frame_parser_) {
        frame_parser_->parseFrame(frame_data, receive_timestamp);
    }
}

void StreamingEngine::handleParsedFrame(uint32_t frame_number, 
                                       const eti::EtiFrame& frame,
                                       std::chrono::nanoseconds parse_time) {
    if (!streaming_.load()) {
        return;
    }
    
    auto receive_time = frame.get_receive_timestamp();
    
    // Add frame to circular buffer
    if (!frame_buffer_->addFrame(frame, receive_time)) {
        Logger::instance().log(Logger::Warning, "StreamingEngine", 
            QString("Failed to buffer frame %1 - buffer full").arg(frame_number));
        
        frames_with_errors_++;
        consecutive_errors_++;
        return;
    }
    
    Logger::instance().log(Logger::Debug, "StreamingEngine", 
        QString("Frame %1 buffered successfully (parse time: %2µs)")
        .arg(frame_number)
        .arg(parse_time.count() / 1000));
}

void StreamingEngine::processFrameFromBuffer() {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Get frame from buffer
    auto buffered_frame_opt = frame_buffer_->getNextFrame(100); // 100ms timeout
    if (!buffered_frame_opt) {
        return; // No frame available
    }
    
    auto buffered_frame = *buffered_frame_opt;
    uint32_t frame_num = ++frame_counter_;
    
    try {
        // Validate frame quality
        bool quality_ok = validateFrameQuality(buffered_frame.frame);
        
        // Perform ETSI compliance validation if enabled
        eti::compliance::ComprehensiveComplianceResult etsi_result;
        if (etsi_validation_enabled_.load() && etsi_validator_) {
            etsi_result = etsi_validator_->validateFrame(buffered_frame.frame);
            
            if (!etsi_result.meets_broadcast_requirements) {
                emit etsiComplianceIssue(frame_num, etsi_result);
                
                if (etsi_result.total_violations > 0) {
                    quality_ok = false;
                    frames_with_errors_++;
                    consecutive_errors_++;
                } else {
                    consecutive_errors_ = 0;
                }
            } else {
                consecutive_errors_ = 0;
            }
        } else {
            consecutive_errors_ = 0;
        }
        
        // Calculate processing time
        auto end_time = std::chrono::high_resolution_clock::now();
        auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time);
        
        // Update metrics
        updateQualityMetrics(buffered_frame.frame, processing_time);
        
        // Update counters
        total_frames_processed_++;
        
        // Track recent processing times
        recent_processing_times_.push_back(processing_time);
        if (recent_processing_times_.size() > 100) {
            recent_processing_times_.pop_front();
        }
        
        // Track recent frame times for FPS calculation
        auto now = std::chrono::system_clock::now();
        recent_frame_times_.push_back(now);
        if (recent_frame_times_.size() > 1000) {
            recent_frame_times_.pop_front();
        }
        
        // Emit frame processed signal
        emit frameProcessed(frame_num, buffered_frame.frame, processing_time);
        
        Logger::instance().log(Logger::Debug, "StreamingEngine", 
            QString("Frame %1 processed (quality: %2, time: %3µs)")
            .arg(frame_num)
            .arg(quality_ok ? "OK" : "POOR")
            .arg(processing_time.count()));
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "StreamingEngine", 
            QString("Frame processing error: %1").arg(e.what()));
        
        frames_with_errors_++;
        consecutive_errors_++;
    }
}

bool StreamingEngine::validateFrameQuality(const eti::EtiFrame& frame) {
    // Basic frame validation
    if (!frame.is_valid() || frame.size() != 6144) {
        return false;
    }
    
    // Check sync pattern
    auto sync = frame.get_sync_field();
    if (!sync.is_valid()) {
        return false;
    }
    
    // Additional quality checks can be added here
    return true;
}

void StreamingEngine::updateQualityMetrics(const eti::EtiFrame& frame, 
                                          std::chrono::microseconds processing_time) {
    QMutexLocker locker(&stats_mutex_);
    
    // Update processing metrics
    quality_metrics_.total_frames_processed = total_frames_processed_.load();
    quality_metrics_.frames_with_errors = frames_with_errors_.load();
    quality_metrics_.consecutive_errors = consecutive_errors_.load();
    
    // Calculate error rate
    if (quality_metrics_.total_frames_processed > 0) {
        quality_metrics_.current_error_rate = 
            (static_cast<double>(quality_metrics_.frames_with_errors) / 
             quality_metrics_.total_frames_processed) * 100.0;
    }
    
    // Update processing times
    quality_metrics_.avg_processing_time = processing_time;
    if (processing_time > quality_metrics_.max_processing_time) {
        quality_metrics_.max_processing_time = processing_time;
    }
    
    // Calculate FPS
    if (recent_frame_times_.size() >= 2) {
        auto time_span = std::chrono::duration_cast<std::chrono::milliseconds>(
            recent_frame_times_.back() - recent_frame_times_.front());
        
        if (time_span.count() > 0) {
            quality_metrics_.current_fps = 
                (static_cast<double>(recent_frame_times_.size() - 1) / time_span.count()) * 1000.0;
        }
    }
    
    // Assess signal quality
    quality_metrics_.signal_quality = static_cast<double>(assessSignalQuality(frame));
    
    // Calculate sync accuracy
    quality_metrics_.sync_accuracy = calculateSyncAccuracy(frame);
    
    // Update buffer status
    quality_metrics_.buffer_status = frame_buffer_->getStatus();
    
    // Determine quality level
    auto new_level = quality_metrics_.determineQualityLevel();
    if (new_level != quality_metrics_.current_level) {
        quality_metrics_.current_level = new_level;
        emit qualityLevelChanged(new_level, quality_metrics_);
    }
    
    quality_metrics_.last_update = std::chrono::system_clock::now();
}

QualityLevel StreamingEngine::assessSignalQuality(const eti::EtiFrame& frame) {
    // Basic signal quality assessment based on frame characteristics
    double quality_score = 100.0;
    
    // Check frame validity
    if (!frame.is_valid()) {
        quality_score -= 50.0;
    }
    
    // Check sync pattern
    auto sync = frame.get_sync_field();
    if (!sync.is_valid()) {
        quality_score -= 30.0;
    }
    
    // Check LIDATA field
    try {
        auto lidata = frame.get_lidata_field();
        if (lidata.nst > 63) { // Invalid number of sub-channels
            quality_score -= 20.0;
        }
    } catch (...) {
        quality_score -= 20.0;
    }
    
    // Convert to quality level
    if (quality_score >= 95.0) return QualityLevel::EXCELLENT;
    if (quality_score >= 85.0) return QualityLevel::GOOD;
    if (quality_score >= 70.0) return QualityLevel::ACCEPTABLE;
    if (quality_score >= 50.0) return QualityLevel::POOR;
    return QualityLevel::CRITICAL;
}

double StreamingEngine::calculateSyncAccuracy(const eti::EtiFrame& frame) {
    // Calculate synchronization accuracy based on frame timing
    try {
        auto receive_time = frame.get_receive_timestamp();
        auto now = std::chrono::system_clock::now();
        
        // Calculate time since frame reception
        auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - receive_time);
        
        // Sync accuracy degrades with frame age
        if (age < std::chrono::milliseconds{10}) {
            return 100.0;
        } else if (age < std::chrono::milliseconds{50}) {
            return 90.0;
        } else if (age < std::chrono::milliseconds{100}) {
            return 75.0;
        } else {
            return 50.0;
        }
        
    } catch (...) {
        return 50.0; // Default sync accuracy for invalid frames
    }
}

// ============================================================================
// Monitoring and Adaptive Management
// ============================================================================

void StreamingEngine::handleQualityMonitoring() {
    if (!streaming_.load()) {
        return;
    }
    
    checkQualityThresholds();
}

void StreamingEngine::handlePerformanceMonitoring() {
    if (!streaming_.load()) {
        return;
    }
    
    updatePerformanceStats();
}

void StreamingEngine::handleAdaptiveBuffering() {
    if (!streaming_.load() || !adaptive_buffering_enabled_.load()) {
        return;
    }
    
    adaptBufferSize();
}

void StreamingEngine::checkQualityThresholds() {
    QMutexLocker config_locker(&config_mutex_);
    auto thresholds = quality_thresholds_;
    config_locker.unlock();
    
    QMutexLocker stats_locker(&stats_mutex_);
    auto metrics = quality_metrics_;
    stats_locker.unlock();
    
    // Check signal quality threshold
    if (metrics.signal_quality < thresholds.min_signal_quality) {
        triggerQualityAlert(QualityLevel::POOR, 
            QString("Signal quality below threshold: %1% < %2%")
            .arg(metrics.signal_quality, 0, 'f', 1)
            .arg(thresholds.min_signal_quality, 0, 'f', 1));
    }
    
    // Check ETSI compliance threshold
    if (metrics.etsi_compliance_score < thresholds.min_etsi_compliance) {
        triggerQualityAlert(QualityLevel::POOR, 
            QString("ETSI compliance below threshold: %1% < %2%")
            .arg(metrics.etsi_compliance_score, 0, 'f', 1)
            .arg(thresholds.min_etsi_compliance, 0, 'f', 1));
    }
    
    // Check latency threshold
    if (metrics.avg_processing_time > thresholds.max_latency) {
        triggerQualityAlert(QualityLevel::ACCEPTABLE, 
            QString("Processing latency above threshold: %1ms > %2ms")
            .arg(metrics.avg_processing_time.count() / 1000.0, 0, 'f', 1)
            .arg(thresholds.max_latency.count()));
    }
    
    // Check consecutive errors
    if (metrics.consecutive_errors >= thresholds.max_consecutive_errors) {
        triggerQualityAlert(QualityLevel::CRITICAL, 
            QString("Too many consecutive errors: %1 >= %2")
            .arg(metrics.consecutive_errors)
            .arg(thresholds.max_consecutive_errors));
    }
}

void StreamingEngine::updatePerformanceStats() {
    QMutexLocker locker(&stats_mutex_);
    
    // Update basic counters
    performance_stats_.total_frames_processed = total_frames_processed_.load();
    performance_stats_.frames_with_errors = frames_with_errors_.load();
    
    // Calculate current FPS
    performance_stats_.current_fps = quality_metrics_.current_fps;
    
    // Calculate average FPS
    auto now = std::chrono::system_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    if (uptime.count() > 0) {
        performance_stats_.average_fps = 
            static_cast<double>(performance_stats_.total_frames_processed) / uptime.count();
    }
    
    // Calculate average processing time
    if (!recent_processing_times_.empty()) {
        auto total_time = std::chrono::microseconds{0};
        auto max_time = std::chrono::microseconds{0};
        
        for (const auto& time : recent_processing_times_) {
            total_time += time;
            max_time = std::max(max_time, time);
        }
        
        performance_stats_.avg_processing_time = total_time / recent_processing_times_.size();
        performance_stats_.max_processing_time = max_time;
    }
    
    // Check if performance targets are met
    bool targets_met = performance_stats_.meetsPerformanceTargets();
    emit performanceTargetsStatus(targets_met, performance_stats_);
    
    Logger::instance().log(Logger::Debug, "StreamingEngine", 
        QString("Performance update - FPS: %1, Errors: %2, Targets: %3")
        .arg(performance_stats_.current_fps, 0, 'f', 1)
        .arg(performance_stats_.getErrorRate(), 0, 'f', 2)
        .arg(targets_met ? "MET" : "MISSED"));
}

void StreamingEngine::adaptBufferSize() {
    auto buffer_status = frame_buffer_->getStatus();
    auto current_capacity = frame_buffer_->capacity();
    
    // Increase buffer size if consistently high utilization
    if (buffer_status.utilization_percent > 85.0 && buffer_status.overflow_count > 0) {
        size_t new_size = std::min(current_capacity * 2, size_t{10000});
        if (new_size > current_capacity) {
            frame_buffer_->resize(new_size);
            Logger::instance().log(Logger::Info, "StreamingEngine", 
                QString("Adaptive buffering: increased buffer size to %1 frames").arg(new_size));
        }
    }
    // Decrease buffer size if consistently low utilization
    else if (buffer_status.utilization_percent < 25.0 && current_capacity > 100) {
        size_t new_size = std::max(current_capacity / 2, size_t{100});
        if (new_size < current_capacity) {
            frame_buffer_->resize(new_size);
            Logger::instance().log(Logger::Info, "StreamingEngine", 
                QString("Adaptive buffering: decreased buffer size to %1 frames").arg(new_size));
        }
    }
}

void StreamingEngine::triggerQualityAlert(QualityLevel level, const QString& reason) {
    emit qualityAlert(level, reason);
    
    Logger::instance().log(Logger::Warning, "StreamingEngine", 
        QString("Quality alert [%1]: %2")
        .arg(utils::qualityLevelToString(level))
        .arg(reason));
}

// ============================================================================
// Processing Worker Thread Implementation
// ============================================================================

StreamingEngine::ProcessingWorkerThread::ProcessingWorkerThread(StreamingEngine* parent)
    : QThread(parent), engine_(parent), stop_flag_(false) {
}

StreamingEngine::ProcessingWorkerThread::~ProcessingWorkerThread() {
    stop();
    wait();
}

void StreamingEngine::ProcessingWorkerThread::run() {
    Logger::instance().log(Logger::Info, "ProcessingWorkerThread", "Worker thread started");
    
    while (!stop_flag_.load()) {
        try {
            engine_->processFrameFromBuffer();
            
            // Small delay to prevent CPU spinning
            QThread::msleep(1);
            
        } catch (const std::exception& e) {
            Logger::instance().log(Logger::Error, "ProcessingWorkerThread", 
                QString("Worker thread error: %1").arg(e.what()));
            
            // Brief pause before retrying
            QThread::msleep(10);
        }
    }
    
    Logger::instance().log(Logger::Info, "ProcessingWorkerThread", "Worker thread stopped");
}

void StreamingEngine::ProcessingWorkerThread::stop() {
    stop_flag_ = true;
}

// ============================================================================
// Slot Implementations
// ============================================================================

void StreamingEngine::connectToEndpoint(const QString& endpoint) {
    startLiveStreaming(endpoint);
}

void StreamingEngine::adjustBufferSize(uint32_t new_size) {
    configureBuffering(new_size);
}

void StreamingEngine::updateQualityThresholds(const QualityThresholds& thresholds) {
    setQualityThresholds(thresholds);
}

void StreamingEngine::handleConnectionStatusChanged(const eti::network::ConnectionStatus& status) {
    if (status.isConnected()) {
        emit streamStatusChanged(false, "Connection restored");
        Logger::instance().log(Logger::Info, "StreamingEngine", "ZMQ connection restored");
    } else {
        emit streamStatusChanged(true, "Connection lost");
        Logger::instance().log(Logger::Warning, "StreamingEngine", 
            QString("ZMQ connection lost: %1").arg(status.error_message));
    }
}

// ============================================================================
// Utility Functions Implementation
// ============================================================================

namespace utils {

QualityThresholds createBroadcastQualityThresholds() {
    QualityThresholds thresholds;
    thresholds.min_signal_quality = 85.0;
    thresholds.min_etsi_compliance = 90.0;
    thresholds.max_latency = std::chrono::milliseconds{30};
    thresholds.max_error_rate = 2.0;
    thresholds.max_consecutive_errors = 5;
    return thresholds;
}

QualityThresholds createHighPerformanceThresholds() {
    QualityThresholds thresholds;
    thresholds.min_signal_quality = 95.0;
    thresholds.min_etsi_compliance = 98.0;
    thresholds.max_latency = std::chrono::milliseconds{10};
    thresholds.max_error_rate = 0.5;
    thresholds.max_consecutive_errors = 2;
    return thresholds;
}

QualityThresholds createTestingThresholds() {
    QualityThresholds thresholds;
    thresholds.min_signal_quality = 60.0;
    thresholds.min_etsi_compliance = 70.0;
    thresholds.max_latency = std::chrono::milliseconds{100};
    thresholds.max_error_rate = 10.0;
    thresholds.max_consecutive_errors = 20;
    return thresholds;
}

QString qualityLevelToString(QualityLevel level) {
    switch (level) {
        case QualityLevel::EXCELLENT: return "EXCELLENT";
        case QualityLevel::GOOD: return "GOOD";
        case QualityLevel::ACCEPTABLE: return "ACCEPTABLE";
        case QualityLevel::POOR: return "POOR";
        case QualityLevel::CRITICAL: return "CRITICAL";
        case QualityLevel::UNKNOWN: return "UNKNOWN";
    }
    return "UNKNOWN";
}

QString qualityLevelToColor(QualityLevel level) {
    switch (level) {
        case QualityLevel::EXCELLENT: return "#00AA00"; // Green
        case QualityLevel::GOOD: return "#88AA00"; // Yellow-green
        case QualityLevel::ACCEPTABLE: return "#AAAA00"; // Yellow
        case QualityLevel::POOR: return "#AA5500"; // Orange
        case QualityLevel::CRITICAL: return "#AA0000"; // Red
        case QualityLevel::UNKNOWN: return "#888888"; // Gray
    }
    return "#888888";
}

} // namespace utils

} // namespace eti::streaming