/**
 * @file streaming_processor.h
 * @brief Real-time ETI Streaming Processor with <50ms Latency
 *
 * Implements high-performance real-time ETI stream processing with:
 * - Ultra-low latency <50ms processing pipeline
 * - Lock-free multi-threaded architecture
 * - Adaptive quality management
 * - Professional broadcast workflow integration
 * - Real-time performance monitoring and optimization
 *
 * @author Network/Stream Agent
 * @date 2025-09-21
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 */

#ifndef STREAMING_PROCESSOR_H
#define STREAMING_PROCESSOR_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>

#include "../core/eti_types.hpp"
#include "eti_over_ip_receiver.h"

namespace eti_network {

/**
 * @brief Processing mode configuration for different performance targets
 */
enum class ProcessingMode {
    ULTRA_LOW_LATENCY,      // <20ms latency, minimal processing
    LOW_LATENCY,            // <50ms latency, standard processing
    BALANCED,               // <100ms latency, full feature processing
    HIGH_QUALITY,           // <200ms latency, maximum quality analysis
    BATCH_PROCESSING        // No latency constraints, maximum throughput
};

/**
 * @brief Real-time streaming configuration for performance optimization
 */
struct StreamingConfig {
    ProcessingMode mode = ProcessingMode::LOW_LATENCY;
    std::chrono::milliseconds target_latency{50};
    std::chrono::milliseconds max_jitter{10};

    // Thread configuration
    size_t processing_threads = std::thread::hardware_concurrency();
    size_t io_threads = 2;
    bool use_thread_affinity = false;
    std::vector<int> cpu_cores;

    // Buffer configuration
    size_t input_buffer_size = 1000;        // Input frame buffer
    size_t output_buffer_size = 1000;       // Output frame buffer
    size_t processing_queue_size = 100;     // Processing queue size
    bool lock_free_queues = true;           // Use lock-free data structures

    // Quality management
    bool adaptive_quality = true;           // Adaptive quality adjustment
    bool frame_dropping = true;             // Enable frame dropping under load
    bool priority_scheduling = true;        // Use priority scheduling
    double cpu_threshold = 0.8;            // CPU usage threshold for adaptation

    // Performance monitoring
    bool enable_metrics = true;             // Enable performance metrics
    std::chrono::milliseconds metrics_interval{100}; // Metrics update interval
    bool enable_profiling = false;          // Enable detailed profiling

    // Error handling
    bool continue_on_error = true;          // Continue processing on errors
    size_t max_consecutive_errors = 10;     // Maximum consecutive errors
    std::chrono::milliseconds error_recovery_delay{100}; // Error recovery delay
};

/**
 * @brief Copyable streaming performance metrics (for returning values)
 */
struct StreamingPerformanceSnapshot {
    // Latency metrics (microseconds)
    int64_t current_latency_us{0};
    int64_t average_latency_us{0};
    int64_t min_latency_us{LLONG_MAX};
    int64_t max_latency_us{0};
    int64_t jitter_us{0};

    // Throughput metrics
    double current_fps{0.0};
    double average_fps{0.0};
    double peak_fps{0.0};
    size_t frames_processed{0};
    size_t frames_dropped{0};

    // Resource utilization
    double cpu_usage{0.0};
    double memory_usage_mb{0.0};
    double network_bandwidth_mbps{0.0};
    double buffer_utilization{0.0};

    // Quality metrics
    double processing_efficiency{1.0};  // 0.0-1.0
    double quality_score{1.0};          // 0.0-1.0
    size_t processing_errors{0};
    size_t quality_degradations{0};

    // Timing breakdown (microseconds)
    int64_t input_time_us{0};           // Input processing time
    int64_t decode_time_us{0};          // Decoding time
    int64_t analysis_time_us{0};        // Analysis time
    int64_t output_time_us{0};          // Output processing time
    int64_t queue_wait_time_us{0};      // Queue waiting time
};

/**
 * @brief Real-time streaming performance metrics (atomic for thread safety)
 */
struct StreamingPerformance {
    // Latency metrics (microseconds)
    std::atomic<int64_t> current_latency_us{0};
    std::atomic<int64_t> average_latency_us{0};
    std::atomic<int64_t> min_latency_us{LLONG_MAX};
    std::atomic<int64_t> max_latency_us{0};
    std::atomic<int64_t> jitter_us{0};

    // Throughput metrics
    std::atomic<double> current_fps{0.0};
    std::atomic<double> average_fps{0.0};
    std::atomic<double> peak_fps{0.0};
    std::atomic<size_t> frames_processed{0};
    std::atomic<size_t> frames_dropped{0};

    // Resource utilization
    std::atomic<double> cpu_usage{0.0};
    std::atomic<double> memory_usage_mb{0.0};
    std::atomic<double> network_bandwidth_mbps{0.0};
    std::atomic<double> buffer_utilization{0.0};

    // Quality metrics
    std::atomic<double> processing_efficiency{1.0};  // 0.0-1.0
    std::atomic<double> quality_score{1.0};          // 0.0-1.0
    std::atomic<size_t> processing_errors{0};
    std::atomic<size_t> quality_degradations{0};

    // Timing breakdown (microseconds)
    std::atomic<int64_t> input_time_us{0};           // Input processing time
    std::atomic<int64_t> decode_time_us{0};          // Decoding time
    std::atomic<int64_t> analysis_time_us{0};        // Analysis time
    std::atomic<int64_t> output_time_us{0};          // Output processing time
    std::atomic<int64_t> queue_wait_time_us{0};      // Queue waiting time

    // Method to create a copyable snapshot
    StreamingPerformanceSnapshot getSnapshot() const {
        StreamingPerformanceSnapshot snapshot;
        snapshot.current_latency_us = current_latency_us.load();
        snapshot.average_latency_us = average_latency_us.load();
        snapshot.min_latency_us = min_latency_us.load();
        snapshot.max_latency_us = max_latency_us.load();
        snapshot.jitter_us = jitter_us.load();
        snapshot.current_fps = current_fps.load();
        snapshot.average_fps = average_fps.load();
        snapshot.peak_fps = peak_fps.load();
        snapshot.frames_processed = frames_processed.load();
        snapshot.frames_dropped = frames_dropped.load();
        snapshot.cpu_usage = cpu_usage.load();
        snapshot.memory_usage_mb = memory_usage_mb.load();
        snapshot.network_bandwidth_mbps = network_bandwidth_mbps.load();
        snapshot.buffer_utilization = buffer_utilization.load();
        snapshot.processing_efficiency = processing_efficiency.load();
        snapshot.quality_score = quality_score.load();
        snapshot.processing_errors = processing_errors.load();
        snapshot.quality_degradations = quality_degradations.load();
        snapshot.input_time_us = input_time_us.load();
        snapshot.decode_time_us = decode_time_us.load();
        snapshot.analysis_time_us = analysis_time_us.load();
        snapshot.output_time_us = output_time_us.load();
        snapshot.queue_wait_time_us = queue_wait_time_us.load();
        return snapshot;
    }

    void reset() {
        current_latency_us = 0;
        average_latency_us = 0;
        min_latency_us = LLONG_MAX;
        max_latency_us = 0;
        jitter_us = 0;
        current_fps = 0.0;
        average_fps = 0.0;
        peak_fps = 0.0;
        frames_processed = 0;
        frames_dropped = 0;
        cpu_usage = 0.0;
        memory_usage_mb = 0.0;
        network_bandwidth_mbps = 0.0;
        buffer_utilization = 0.0;
        processing_efficiency = 1.0;
        quality_score = 1.0;
        processing_errors = 0;
        quality_degradations = 0;
        input_time_us = 0;
        decode_time_us = 0;
        analysis_time_us = 0;
        output_time_us = 0;
        queue_wait_time_us = 0;
    }
};

/**
 * @brief Processed frame with timing and quality information
 */
struct ProcessedFrame {
    eti::EtiFrame frame;
    NetworkQualityMetrics network_quality;

    // Timing information
    std::chrono::steady_clock::time_point receive_time;
    std::chrono::steady_clock::time_point process_start_time;
    std::chrono::steady_clock::time_point process_end_time;
    std::chrono::microseconds total_latency{0};

    // Processing results
    bool processing_successful = true;
    QString error_message;
    double quality_score = 1.0;
    size_t processing_stage = 0;

    // Analysis results (populated based on processing mode)
    struct AnalysisResults {
        bool fic_decoded = false;
        bool services_discovered = false;
        bool audio_extracted = false;
        bool etsi_validated = false;
        size_t discovered_services = 0;
        double signal_quality = 0.0;
    } analysis;

    std::chrono::microseconds get_processing_latency() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            process_end_time - process_start_time);
    }

    std::chrono::microseconds get_total_latency() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            process_end_time - receive_time);
    }

    bool meets_latency_target(std::chrono::microseconds target) const {
        return get_total_latency() <= target;
    }
};

/**
 * @brief Lock-free queue for high-performance inter-thread communication
 *
 * FIXED: Memory leak and ABA problem resolved with:
 * - Proper destructor for cleanup
 * - Epoch-based memory reclamation
 * - CAS-based pop() to avoid ABA problem
 */
template<typename T>
class LockFreeQueue {
public:
    explicit LockFreeQueue(size_t capacity = 1024);
    ~LockFreeQueue();

    bool push(T&& item);
    bool push(const T& item);
    bool pop(T& item);
    bool empty() const;
    size_t size() const;
    size_t capacity() const;

    // Performance metrics
    size_t get_push_count() const;
    size_t get_pop_count() const;
    size_t get_contention_count() const;

private:
    struct Node {
        std::atomic<T*> data{nullptr};
        std::atomic<Node*> next{nullptr};
    };

    std::atomic<Node*> m_head{nullptr};
    std::atomic<Node*> m_tail{nullptr};
    std::atomic<size_t> m_size{0};
    size_t m_capacity;

    // Performance counters
    mutable std::atomic<size_t> m_push_count{0};
    mutable std::atomic<size_t> m_pop_count{0};
    mutable std::atomic<size_t> m_contention_count{0};

    // Epoch-based memory reclamation (FIXED: Memory leak)
    std::atomic<uint64_t> m_pop_epoch{0};
    std::vector<Node*> m_retired_nodes;
    std::mutex m_retired_mutex;

    void initialize_queue();
    void retire_node(Node* node, uint64_t epoch);
    void cleanup_retired_nodes(uint64_t current_epoch);
};

/**
 * @brief High-performance streaming worker for real-time processing
 */
class StreamingWorker : public QObject {
    Q_OBJECT

public:
    explicit StreamingWorker(const StreamingConfig& config, QObject* parent = nullptr);
    ~StreamingWorker();

public slots:
    void startProcessing();
    void stopProcessing();
    void processFrame(const eti::EtiFrame& frame, const NetworkQualityMetrics& quality);
    void updateConfiguration(const StreamingConfig& config);

signals:
    void frameProcessed(const ProcessedFrame& frame);
    void performanceUpdate(const StreamingPerformance& performance);
    void processingError(const QString& error, int severity);
    void qualityDegradation(const QString& reason, double current_quality);

private slots:
    void updatePerformanceMetrics();
    void checkResourceUsage();
    void performAdaptiveOptimization();

private:
    void initializeWorker();
    void cleanupWorker();
    void processFrameInternal(const eti::EtiFrame& frame, const NetworkQualityMetrics& quality);
    void updateLatencyStatistics(const ProcessedFrame& frame);
    void handleProcessingError(const QString& error);
    void adaptProcessingQuality();
    void optimizeThreadAffinity();

    // Processing pipeline stages
    ProcessedFrame inputStage(const eti::EtiFrame& frame, const NetworkQualityMetrics& quality);
    ProcessedFrame decodeStage(ProcessedFrame frame);
    ProcessedFrame analysisStage(ProcessedFrame frame);
    ProcessedFrame outputStage(ProcessedFrame frame);

    StreamingConfig m_config;
    StreamingPerformance m_performance;

    std::unique_ptr<QTimer> m_metricsTimer;
    std::unique_ptr<QTimer> m_resourceTimer;
    std::unique_ptr<QTimer> m_adaptationTimer;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_processing{false};
    std::atomic<size_t> m_consecutive_errors{0};

    // Lock-free processing queues
    std::unique_ptr<LockFreeQueue<std::pair<eti::EtiFrame, NetworkQualityMetrics>>> m_inputQueue;
    std::unique_ptr<LockFreeQueue<ProcessedFrame>> m_outputQueue;

    // Performance monitoring
    std::chrono::steady_clock::time_point m_start_time;
    std::chrono::steady_clock::time_point m_last_frame_time;
    std::vector<std::chrono::microseconds> m_latency_history;
    std::vector<double> m_fps_history;

    static constexpr size_t HISTORY_SIZE = 100;
    static constexpr std::chrono::milliseconds METRICS_UPDATE_INTERVAL{100};
    static constexpr std::chrono::milliseconds RESOURCE_CHECK_INTERVAL{500};
    static constexpr std::chrono::milliseconds ADAPTATION_INTERVAL{2000};
};

/**
 * @brief Real-time ETI Streaming Processor
 *
 * High-performance streaming processor implementing:
 * - Ultra-low latency processing pipeline (<50ms target)
 * - Lock-free multi-threaded architecture
 * - Adaptive quality management
 * - Real-time performance monitoring
 * - Professional broadcast integration
 */
class StreamingProcessor : public QObject {
    Q_OBJECT

public:
    explicit StreamingProcessor(QObject* parent = nullptr);
    explicit StreamingProcessor(const StreamingConfig& config, QObject* parent = nullptr);
    ~StreamingProcessor();

    // Processing control
    bool startProcessing(const StreamingConfig& config);
    bool startProcessing(); // Use existing configuration
    void stopProcessing();
    bool isProcessing() const;

    // Configuration management
    void setConfiguration(const StreamingConfig& config);
    StreamingConfig getConfiguration() const;
    void setTargetLatency(std::chrono::milliseconds latency);
    void setProcessingMode(ProcessingMode mode);
    void setThreadConfiguration(size_t processingThreads, size_t ioThreads);
    void enableAdaptiveQuality(bool enabled);

    // Frame processing
    void processFrame(const eti::EtiFrame& frame, const NetworkQualityMetrics& quality);
    void processFrameBatch(const std::vector<std::pair<eti::EtiFrame, NetworkQualityMetrics>>& frames);

    // Performance monitoring
    StreamingPerformanceSnapshot getPerformanceMetrics() const;
    std::chrono::microseconds getCurrentLatency() const;
    double getCurrentFPS() const;
    double getProcessingEfficiency() const;
    bool isLatencyTargetMet() const;

    // Quality management
    void setQualityThresholds(double minEfficiency, double maxLatency, double maxJitter);
    bool isQualityAcceptable() const;
    double getCurrentQualityScore() const;
    void enableFrameDropping(bool enabled);
    void setFrameDropThreshold(double cpuThreshold);

    // Resource monitoring
    double getCPUUsage() const;
    double getMemoryUsage() const;
    double getBufferUtilization() const;
    void setResourceLimits(double maxCPU, double maxMemory);

    // Diagnostics
    QString getDiagnosticInfo() const;
    QStringList getPerformanceReport() const;
    void exportPerformanceData(const QString& filename) const;
    void resetPerformanceCounters();

    // Callback registration
    using FrameCallback = std::function<void(const ProcessedFrame&)>;
    using PerformanceCallback = std::function<void(const StreamingPerformance&)>;
    using ErrorCallback = std::function<void(const QString&, int severity)>;

    void setFrameCallback(FrameCallback callback);
    void setPerformanceCallback(PerformanceCallback callback);
    void setErrorCallback(ErrorCallback callback);

signals:
    void frameProcessed(const ProcessedFrame& frame);
    void performanceUpdate(const StreamingPerformance& performance);
    void processingStarted();
    void processingStopped();
    void processingError(const QString& error, int severity);
    void qualityDegradation(const QString& reason, double currentQuality);
    void latencyTargetViolated(std::chrono::microseconds currentLatency,
                              std::chrono::microseconds targetLatency);
    void resourceLimitExceeded(const QString& resource, double currentUsage, double limit);
    void adaptiveOptimizationApplied(const QString& optimization, double improvement);

private slots:
    void handleWorkerFrame(const ProcessedFrame& frame);
    void handleWorkerPerformance(const StreamingPerformance& performance);
    void handleWorkerError(const QString& error, int severity);
    void handleWorkerQualityDegradation(const QString& reason, double quality);
    void performMaintenanceTasks();
    void checkSystemResources();
    void optimizePerformance();

private:
    // Initialization and cleanup
    void initializeProcessor();
    void cleanupProcessor();
    void setupWorkerThreads();
    void cleanupWorkerThreads();
    void validateConfiguration(const StreamingConfig& config);

    // Performance optimization
    void applyProcessingModeOptimizations();
    void optimizeBufferSizes();
    void optimizeThreadConfiguration();
    void handleResourceConstraints();

    // Quality management
    void monitorQualityMetrics();
    void adjustProcessingQuality();
    void handleQualityDegradation();

    StreamingConfig m_config;
    StreamingPerformance m_aggregatedPerformance;

    // Worker thread management
    std::vector<std::unique_ptr<QThread>> m_workerThreads;
    std::vector<std::unique_ptr<StreamingWorker>> m_workers;
    std::atomic<size_t> m_nextWorkerIndex{0};

    // Timers for maintenance and monitoring
    std::unique_ptr<QTimer> m_maintenanceTimer;
    std::unique_ptr<QTimer> m_resourceTimer;
    std::unique_ptr<QTimer> m_optimizationTimer;

    // State management
    std::atomic<bool> m_processing{false};
    std::atomic<bool> m_initialized{false};

    // Quality thresholds
    double m_minEfficiency = 0.8;
    double m_maxLatency = 50.0; // milliseconds
    double m_maxJitter = 10.0;  // milliseconds

    // Resource limits
    double m_maxCPU = 0.8;      // 80%
    double m_maxMemory = 1024.0; // MB

    // Performance tracking
    mutable QMutex m_performanceMutex;
    std::chrono::steady_clock::time_point m_processingStartTime;

    // Callbacks
    FrameCallback m_frameCallback;
    PerformanceCallback m_performanceCallback;
    ErrorCallback m_errorCallback;
    mutable QMutex m_callbackMutex;

    // Error handling
    QStringList m_errorHistory;
    mutable QMutex m_errorMutex;
    static constexpr size_t MAX_ERROR_HISTORY = 100;

    // Diagnostics
    std::chrono::steady_clock::time_point m_lastDiagnosticTime;
    mutable QMutex m_diagnosticMutex;

    static constexpr std::chrono::milliseconds MAINTENANCE_INTERVAL{1000};
    static constexpr std::chrono::milliseconds RESOURCE_CHECK_INTERVAL{500};
    static constexpr std::chrono::milliseconds OPTIMIZATION_INTERVAL{5000};
};

/**
 * @brief Factory for creating optimized streaming processors
 */
class StreamingProcessorFactory {
public:
    /**
     * @brief Create ultra-low latency processor (<20ms)
     */
    static std::unique_ptr<StreamingProcessor> createUltraLowLatencyProcessor();

    /**
     * @brief Create low latency processor (<50ms)
     */
    static std::unique_ptr<StreamingProcessor> createLowLatencyProcessor();

    /**
     * @brief Create balanced processor (<100ms)
     */
    static std::unique_ptr<StreamingProcessor> createBalancedProcessor();

    /**
     * @brief Create high-quality processor (<200ms)
     */
    static std::unique_ptr<StreamingProcessor> createHighQualityProcessor();

    /**
     * @brief Create custom processor with specific configuration
     */
    static std::unique_ptr<StreamingProcessor> createCustomProcessor(const StreamingConfig& config);

    /**
     * @brief Create processor optimized for current hardware
     */
    static std::unique_ptr<StreamingProcessor> createOptimizedProcessor(ProcessingMode mode);
};

} // namespace eti_network

#endif // STREAMING_PROCESSOR_H
