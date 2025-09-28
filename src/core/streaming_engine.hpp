/**
 * @file streaming_engine.hpp
 * @brief Real-time ETI Streaming Engine with Circular Buffering
 * 
 * High-performance streaming engine for continuous ETI frame processing
 * with adaptive buffering, quality monitoring, and resilience features.
 * Designed for <50ms latency real-time applications.
 * 
 * @author StreamDAB Development Team
 * @date 2025 
 * @copyright Copyright (c) 2025 StreamDAB Technologies
 */

#pragma once

#include "eti_types.hpp"
#include "zeromq_eti_client.hpp"
#include "modern_eti_frame_parser.hpp"
#include "comprehensive_etsi_validator.hpp"

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include <QString>
#include <QThread>
#include <atomic>
#include <memory>
#include <vector>
#include <chrono>
#include <deque>
#include <functional>

namespace eti::streaming {

/**
 * @brief Quality level enumeration for monitoring
 */
enum class QualityLevel {
    EXCELLENT = 5,    // >95% quality
    GOOD = 4,         // 85-95% quality
    ACCEPTABLE = 3,   // 70-85% quality
    POOR = 2,         // 50-70% quality
    CRITICAL = 1,     // <50% quality
    UNKNOWN = 0
};

/**
 * @brief Buffer status information
 */
struct BufferStatus {
    size_t current_size{0};        // Current number of frames in buffer
    size_t max_size{1000};         // Maximum buffer capacity
    size_t overflow_count{0};      // Number of buffer overflows
    size_t underflow_count{0};     // Number of buffer underflows
    std::chrono::milliseconds avg_latency{0};  // Average buffering latency
    std::chrono::milliseconds max_latency{0};  // Maximum latency observed
    double utilization_percent{0.0};           // Buffer utilization percentage
    
    /**
     * @brief Check if buffer is healthy
     */
    [[nodiscard]] bool isHealthy() const {
        return utilization_percent < 80.0 && 
               avg_latency < std::chrono::milliseconds{30} &&
               overflow_count == 0;
    }
    
    /**
     * @brief Get buffer health score (0-100)
     */
    [[nodiscard]] double getHealthScore() const {
        double score = 100.0;
        if (utilization_percent > 90.0) score -= 30.0;
        else if (utilization_percent > 75.0) score -= 15.0;
        
        if (avg_latency > std::chrono::milliseconds{40}) score -= 25.0;
        else if (avg_latency > std::chrono::milliseconds{25}) score -= 10.0;
        
        if (overflow_count > 0) score -= 20.0;
        if (underflow_count > 0) score -= 15.0;
        
        return std::max(0.0, score);
    }
};

/**
 * @brief Quality thresholds for monitoring
 */
struct QualityThresholds {
    double min_signal_quality{80.0};        // Minimum acceptable signal quality %
    double min_etsi_compliance{85.0};       // Minimum ETSI compliance score %
    std::chrono::milliseconds max_latency{50}; // Maximum acceptable latency
    double max_error_rate{5.0};             // Maximum error rate %
    uint32_t max_consecutive_errors{10};    // Maximum consecutive errors
    
    /**
     * @brief Validate threshold configuration
     */
    [[nodiscard]] bool isValid() const {
        return min_signal_quality >= 0.0 && min_signal_quality <= 100.0 &&
               min_etsi_compliance >= 0.0 && min_etsi_compliance <= 100.0 &&
               max_latency.count() > 0 &&
               max_error_rate >= 0.0 && max_error_rate <= 100.0 &&
               max_consecutive_errors > 0;
    }
};

/**
 * @brief Quality metrics for real-time monitoring
 */
struct QualityMetrics {
    // Signal quality
    double signal_quality{100.0};           // Overall signal quality %
    double etsi_compliance_score{100.0};    // ETSI compliance score %
    double sync_accuracy{100.0};            // Synchronization accuracy %
    
    // Error metrics
    uint64_t total_frames_processed{0};
    uint64_t frames_with_errors{0};
    uint64_t consecutive_errors{0};
    double current_error_rate{0.0};         // Current error rate %
    
    // Performance metrics
    std::chrono::microseconds avg_processing_time{0};
    std::chrono::microseconds max_processing_time{0};
    double current_fps{0.0};                // Current processing rate
    
    // Buffer metrics
    BufferStatus buffer_status;
    
    // Quality assessment
    QualityLevel current_level{QualityLevel::EXCELLENT};
    std::chrono::system_clock::time_point last_update;
    
    /**
     * @brief Calculate overall quality score
     */
    [[nodiscard]] double getOverallQuality() const {
        double weighted_score = 
            (signal_quality * 0.4) +
            (etsi_compliance_score * 0.3) +
            (sync_accuracy * 0.2) +
            ((100.0 - current_error_rate) * 0.1);
        
        return std::min(100.0, std::max(0.0, weighted_score));
    }
    
    /**
     * @brief Determine quality level from metrics
     */
    [[nodiscard]] QualityLevel determineQualityLevel() const {
        double overall = getOverallQuality();
        
        if (overall >= 95.0) return QualityLevel::EXCELLENT;
        if (overall >= 85.0) return QualityLevel::GOOD;
        if (overall >= 70.0) return QualityLevel::ACCEPTABLE;
        if (overall >= 50.0) return QualityLevel::POOR;
        return QualityLevel::CRITICAL;
    }
};

/**
 * @brief Circular buffer for ETI frame storage
 */
class CircularETIBuffer {
public:
    /**
     * @brief Buffered ETI frame with metadata
     */
    struct BufferedFrame {
        eti::EtiFrame frame;
        std::chrono::system_clock::time_point receive_time;
        std::chrono::system_clock::time_point buffer_time;
        uint32_t sequence_number{0};
        bool processed{false};
        
        /**
         * @brief Get buffering latency
         */
        [[nodiscard]] std::chrono::milliseconds getBufferLatency() const {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                buffer_time - receive_time);
        }
    };
    
    explicit CircularETIBuffer(size_t capacity = 1000);
    ~CircularETIBuffer() = default;
    
    // Disable copy/move for thread safety
    CircularETIBuffer(const CircularETIBuffer&) = delete;
    CircularETIBuffer& operator=(const CircularETIBuffer&) = delete;
    CircularETIBuffer(CircularETIBuffer&&) = delete;
    CircularETIBuffer& operator=(CircularETIBuffer&&) = delete;
    
    /**
     * @brief Add frame to buffer (thread-safe)
     * @param frame ETI frame to buffer
     * @param receive_time Frame reception timestamp
     * @return true if frame was buffered, false if buffer full
     */
    bool addFrame(const eti::EtiFrame& frame, 
                  const std::chrono::system_clock::time_point& receive_time);
    
    /**
     * @brief Get next frame from buffer (thread-safe)
     * @param timeout_ms Maximum wait time for frame
     * @return Buffered frame, or empty optional if timeout
     */
    std::optional<BufferedFrame> getNextFrame(int timeout_ms = 100);
    
    /**
     * @brief Peek at next frame without removing it
     */
    std::optional<BufferedFrame> peekNextFrame() const;
    
    /**
     * @brief Get current buffer status
     */
    BufferStatus getStatus() const;
    
    /**
     * @brief Clear all buffered frames
     */
    void clear();
    
    /**
     * @brief Resize buffer capacity
     * @param new_capacity New buffer size
     */
    void resize(size_t new_capacity);
    
    /**
     * @brief Get buffer capacity
     */
    [[nodiscard]] size_t capacity() const { return capacity_; }
    
    /**
     * @brief Get current buffer size
     */
    [[nodiscard]] size_t size() const;
    
    /**
     * @brief Check if buffer is empty
     */
    [[nodiscard]] bool empty() const;
    
    /**
     * @brief Check if buffer is full
     */
    [[nodiscard]] bool full() const;

private:
    mutable QMutex mutex_;
    QWaitCondition not_empty_;
    QWaitCondition not_full_;
    
    std::vector<BufferedFrame> buffer_;
    size_t capacity_;
    size_t head_{0};                        // Next write position
    size_t tail_{0};                        // Next read position
    size_t count_{0};                       // Current number of frames
    
    // Statistics
    std::atomic<uint64_t> total_frames_added_{0};
    std::atomic<uint64_t> overflow_count_{0};
    std::atomic<uint64_t> underflow_count_{0};
    std::atomic<uint32_t> sequence_counter_{0};
    
    // Latency tracking
    std::deque<std::chrono::milliseconds> recent_latencies_;
    static constexpr size_t MAX_LATENCY_SAMPLES = 100;
};

/**
 * @brief Real-time ETI Streaming Engine
 * 
 * High-performance streaming engine that orchestrates ETI frame processing
 * with circular buffering, quality monitoring, and adaptive performance
 * management for real-time broadcast applications.
 */
class StreamingEngine : public QObject {
    Q_OBJECT
    
public:
    explicit StreamingEngine(QObject* parent = nullptr);
    ~StreamingEngine() override;
    
    // Disable copy/move for proper resource management
    StreamingEngine(const StreamingEngine&) = delete;
    StreamingEngine& operator=(const StreamingEngine&) = delete;
    StreamingEngine(StreamingEngine&&) = delete;
    StreamingEngine& operator=(StreamingEngine&&) = delete;
    
    /**
     * @brief Initialize streaming engine
     * @param zmq_client ZeroMQ ETI client for frame reception
     * @param frame_parser ETI frame parser
     * @param etsi_validator ETSI compliance validator (optional)
     * @return true if initialization successful
     */
    bool initialize(std::shared_ptr<eti::network::ZeroMQETIClient> zmq_client,
                   std::shared_ptr<eti::modern::ModernETIFrameParser> frame_parser,
                   std::shared_ptr<eti::compliance::ComprehensiveETSIValidator> etsi_validator = nullptr);
    
    /**
     * @brief Check if engine is initialized
     */
    [[nodiscard]] bool isInitialized() const noexcept {
        return initialized_.load();
    }
    
    /**
     * @brief Start live streaming from ZeroMQ endpoint
     * @param zmq_endpoint ZeroMQ endpoint URL (e.g., "tcp://localhost:9200")
     * @return true if streaming started successfully
     */
    bool startLiveStreaming(const QString& zmq_endpoint);
    
    /**
     * @brief Stop live streaming
     */
    void stopStreaming();
    
    /**
     * @brief Check if currently streaming
     */
    [[nodiscard]] bool isStreaming() const noexcept {
        return streaming_.load();
    }
    
    /**
     * @brief Configure circular buffering parameters
     * @param buffer_size_frames Maximum number of frames to buffer
     */
    void configureBuffering(uint32_t buffer_size_frames);
    
    /**
     * @brief Set quality monitoring thresholds
     * @param thresholds Quality threshold configuration
     */
    void setQualityThresholds(const QualityThresholds& thresholds);
    
    /**
     * @brief Get current quality thresholds
     */
    [[nodiscard]] QualityThresholds getQualityThresholds() const {
        QMutexLocker locker(&config_mutex_);
        return quality_thresholds_;
    }
    
    /**
     * @brief Get current quality metrics
     */
    [[nodiscard]] QualityMetrics getQualityMetrics() const;
    
    /**
     * @brief Get buffer status
     */
    [[nodiscard]] BufferStatus getBufferStatus() const;
    
    /**
     * @brief Enable/disable adaptive buffer management
     * @param enabled true to enable adaptive buffering
     */
    void setAdaptiveBufferingEnabled(bool enabled);
    
    /**
     * @brief Enable/disable quality monitoring
     * @param enabled true to enable quality monitoring
     */
    void setQualityMonitoringEnabled(bool enabled);
    
    /**
     * @brief Enable/disable ETSI compliance checking
     * @param enabled true to enable ETSI validation
     */
    void setETSIValidationEnabled(bool enabled);
    
    /**
     * @brief Reset all statistics and counters
     */
    void resetStatistics();
    
    /**
     * @brief Get processing performance statistics
     */
    struct PerformanceStats {
        uint64_t total_frames_processed{0};
        uint64_t frames_with_errors{0};
        double current_fps{0.0};
        double average_fps{0.0};
        std::chrono::microseconds avg_processing_time{0};
        std::chrono::microseconds max_processing_time{0};
        double cpu_usage_percent{0.0};
        double memory_usage_mb{0.0};
        
        [[nodiscard]] double getErrorRate() const {
            return total_frames_processed > 0 ? 
                   (static_cast<double>(frames_with_errors) / total_frames_processed) * 100.0 : 0.0;
        }
        
        [[nodiscard]] bool meetsPerformanceTargets() const {
            return current_fps >= 900.0 &&                    // >900 FPS target
                   avg_processing_time < std::chrono::microseconds{1000} && // <1ms processing
                   getErrorRate() < 1.0;                       // <1% error rate
        }
    };
    
    [[nodiscard]] PerformanceStats getPerformanceStats() const;

public slots:
    /**
     * @brief Connect to streaming endpoint
     * @param endpoint ZeroMQ endpoint URL
     */
    void connectToEndpoint(const QString& endpoint);
    
    /**
     * @brief Adjust buffer size dynamically
     * @param new_size New buffer size in frames
     */
    void adjustBufferSize(uint32_t new_size);
    
    /**
     * @brief Update quality thresholds at runtime
     * @param thresholds New quality thresholds
     */
    void updateQualityThresholds(const QualityThresholds& thresholds);

signals:
    /**
     * @brief Emitted when streaming state changes
     * @param streaming true if streaming started, false if stopped
     */
    void streamingStateChanged(bool streaming);
    
    /**
     * @brief Emitted when ETI frame is processed successfully
     * @param frame_number Frame sequence number
     * @param frame Processed ETI frame
     * @param processing_time Frame processing duration
     */
    void frameProcessed(uint32_t frame_number, 
                       const eti::EtiFrame& frame,
                       std::chrono::microseconds processing_time);
    
    /**
     * @brief Emitted when buffer status changes significantly
     * @param status Current buffer status
     */
    void bufferStatusChanged(const BufferStatus& status);
    
    /**
     * @brief Emitted when quality level changes
     * @param level New quality level
     * @param metrics Current quality metrics
     */
    void qualityLevelChanged(QualityLevel level, const QualityMetrics& metrics);
    
    /**
     * @brief Emitted when quality alert threshold is triggered
     * @param level Alert severity level
     * @param message Human-readable alert message
     */
    void qualityAlert(QualityLevel level, const QString& message);
    
    /**
     * @brief Emitted when stream is interrupted or restored
     * @param interrupted true if stream interrupted, false if restored
     * @param reason Reason for interruption/restoration
     */
    void streamStatusChanged(bool interrupted, const QString& reason);
    
    /**
     * @brief Emitted when performance targets are met or missed
     * @param targets_met true if all performance targets met
     * @param stats Current performance statistics
     */
    void performanceTargetsStatus(bool targets_met, const PerformanceStats& stats);
    
    /**
     * @brief Emitted when ETSI compliance issue is detected
     * @param frame_number Frame number
     * @param compliance_result ETSI compliance result
     */
    void etsiComplianceIssue(uint32_t frame_number, 
                            const eti::compliance::ComprehensiveComplianceResult& compliance_result);

private slots:
    /**
     * @brief Handle raw ETI frame from ZeroMQ client
     */
    void handleETIFrame(const QByteArray& frame_data, 
                       const std::chrono::system_clock::time_point& receive_timestamp);
    
    /**
     * @brief Handle parsed ETI frame
     */
    void handleParsedFrame(uint32_t frame_number, 
                          const eti::EtiFrame& frame,
                          std::chrono::nanoseconds parse_time);
    
    /**
     * @brief Handle ZeroMQ connection status changes
     */
    void handleConnectionStatusChanged(const eti::network::ConnectionStatus& status);
    
    /**
     * @brief Handle quality monitoring timer
     */
    void handleQualityMonitoring();
    
    /**
     * @brief Handle performance monitoring timer
     */
    void handlePerformanceMonitoring();
    
    /**
     * @brief Handle adaptive buffer management
     */
    void handleAdaptiveBuffering();

private:
    /**
     * @brief Worker thread for frame processing
     */
    class ProcessingWorkerThread : public QThread {
    public:
        explicit ProcessingWorkerThread(StreamingEngine* parent);
        ~ProcessingWorkerThread() override;
        
        void run() override;
        void stop();
        
    private:
        StreamingEngine* engine_;
        std::atomic<bool> stop_flag_{false};
    };
    
    // Core components
    std::shared_ptr<eti::network::ZeroMQETIClient> zmq_client_;
    std::shared_ptr<eti::modern::ModernETIFrameParser> frame_parser_;
    std::shared_ptr<eti::compliance::ComprehensiveETSIValidator> etsi_validator_;
    
    // Circular buffer
    std::unique_ptr<CircularETIBuffer> frame_buffer_;
    
    // Worker thread
    std::unique_ptr<ProcessingWorkerThread> processing_thread_;
    
    // Configuration and state
    mutable QMutex config_mutex_;
    QualityThresholds quality_thresholds_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> streaming_{false};
    
    // Feature flags
    std::atomic<bool> adaptive_buffering_enabled_{true};
    std::atomic<bool> quality_monitoring_enabled_{true};
    std::atomic<bool> etsi_validation_enabled_{true};
    
    // Monitoring timers
    QTimer* quality_timer_{nullptr};
    QTimer* performance_timer_{nullptr};
    QTimer* adaptive_buffer_timer_{nullptr};
    
    // Statistics and metrics
    mutable QMutex stats_mutex_;
    QualityMetrics quality_metrics_;
    PerformanceStats performance_stats_;
    
    // Frame tracking
    std::atomic<uint32_t> frame_counter_{0};
    std::atomic<uint64_t> total_frames_processed_{0};
    std::atomic<uint64_t> frames_with_errors_{0};
    std::atomic<uint64_t> consecutive_errors_{0};
    
    // Performance tracking
    std::chrono::system_clock::time_point start_time_;
    std::chrono::system_clock::time_point last_fps_update_;
    std::deque<std::chrono::system_clock::time_point> recent_frame_times_;
    std::deque<std::chrono::microseconds> recent_processing_times_;
    
    // Helper methods
    void connectSignals();
    void disconnectSignals();
    void setupMonitoringTimers();
    void cleanupMonitoringTimers();
    
    // Processing pipeline
    void processFrameFromBuffer();
    bool validateFrameQuality(const eti::EtiFrame& frame);
    void updateQualityMetrics(const eti::EtiFrame& frame, 
                             std::chrono::microseconds processing_time);
    void updatePerformanceStats();
    void checkQualityThresholds();
    void adaptBufferSize();
    
    // Quality assessment
    QualityLevel assessSignalQuality(const eti::EtiFrame& frame);
    double calculateSyncAccuracy(const eti::EtiFrame& frame);
    void triggerQualityAlert(QualityLevel level, const QString& reason);
};

/**
 * @brief Utility functions for streaming engine
 */
namespace utils {
    /**
     * @brief Create default quality thresholds for broadcast applications
     */
    [[nodiscard]] QualityThresholds createBroadcastQualityThresholds();
    
    /**
     * @brief Create high-performance quality thresholds
     */
    [[nodiscard]] QualityThresholds createHighPerformanceThresholds();
    
    /**
     * @brief Create relaxed quality thresholds for testing
     */
    [[nodiscard]] QualityThresholds createTestingThresholds();
    
    /**
     * @brief Convert quality level to human-readable string
     */
    [[nodiscard]] QString qualityLevelToString(QualityLevel level);
    
    /**
     * @brief Get quality level color for UI display
     */
    [[nodiscard]] QString qualityLevelToColor(QualityLevel level);
}

} // namespace eti::streaming