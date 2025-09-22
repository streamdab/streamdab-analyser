/**
 * @file eti_network_integration.h
 * @brief ETI Network Integration Layer for Real-time Processing
 * 
 * Implements comprehensive integration between network streaming components
 * and the existing ETI processor for seamless real-time analysis:
 * - Real-time ETI-over-IP stream processing integration
 * - Performance optimization for >7,482 FPS processing
 * - ETSI compliance validation in streaming context
 * - Professional broadcast workflow integration
 * - Quality monitoring and adaptive control
 * 
 * @author Network/Stream Agent
 * @date 2025-09-21
 * @copyright Copyright (c) 2025 ETI Stream Analyzer Team
 */

#ifndef ETI_NETWORK_INTEGRATION_H
#define ETI_NETWORK_INTEGRATION_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QThread>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <queue>

#include "../core/eti_processor.h"
#include "../core/eti_types.h"
#include "eti_over_ip_receiver.h"
#include "streaming_processor.h"
#include "network_discovery.h"
#include "../core/stream_recording_manager.h"
#include "stream_playback.h"
// #include "broadcast_interface.h" // Removed - stub implementation

namespace eti_network {

/**
 * @brief Integration mode for different operational contexts
 */
enum class IntegrationMode {
    REAL_TIME_ANALYSIS,     // Real-time stream analysis with <50ms latency
    BATCH_PROCESSING,       // Batch file processing for archived content
    HYBRID_MODE,            // Combined real-time and file processing
    BROADCAST_MONITORING,   // Professional broadcast monitoring mode
    ANALYSIS_PLAYBACK,      // Playback with detailed analysis
    QUALITY_ASSESSMENT      // Quality assessment and compliance testing
};

/**
 * @brief Integration configuration for network processing
 */
struct IntegrationConfig {
    IntegrationMode mode = IntegrationMode::REAL_TIME_ANALYSIS;
    
    // Processing configuration
    bool enable_real_time_processing = true;          // Enable real-time processing
    bool enable_etsi_compliance_checking = true;      // Enable ETSI compliance
    bool enable_quality_monitoring = true;            // Enable quality monitoring
    bool enable_performance_optimization = true;      // Enable performance optimization
    
    // Target performance parameters
    std::chrono::milliseconds target_latency{50};     // Target processing latency
    double target_fps = 7482.0;                       // Target processing FPS
    double target_memory_mb = 100.0;                  // Target memory usage
    double quality_threshold = 0.8;                   // Quality threshold
    
    // Network configuration
    bool enable_network_discovery = true;             // Enable network discovery
    bool enable_automatic_failover = true;            // Enable automatic failover
    bool enable_redundancy_monitoring = true;         // Enable redundancy monitoring
    std::chrono::seconds discovery_interval{30};      // Discovery interval
    
    // Recording configuration
    bool enable_automatic_recording = false;          // Enable automatic recording
    eti::RecordingFormat recording_format = eti::RecordingFormat::ETI_NI;
    std::chrono::minutes recording_segment_duration{60}; // Recording segment duration
    QString recording_directory;                       // Recording output directory
    
    // Broadcast integration
    bool enable_broadcast_integration = false;        // Enable broadcast equipment integration
    bool enable_equipment_monitoring = false;         // Enable equipment monitoring
    std::chrono::seconds equipment_discovery_interval{60}; // Equipment discovery interval
    
    // Advanced features
    bool enable_adaptive_quality = true;              // Enable adaptive quality control
    bool enable_predictive_buffering = true;          // Enable predictive buffering
    bool enable_error_recovery = true;                // Enable error recovery
    bool enable_performance_profiling = false;        // Enable detailed profiling
};

/**
 * @brief Comprehensive integration statistics
 */
struct IntegrationStatistics {
    // Processing performance
    std::atomic<double> current_fps{0.0};             // Current processing FPS
    std::atomic<double> average_fps{0.0};             // Average processing FPS
    std::atomic<double> peak_fps{0.0};                // Peak processing FPS
    std::atomic<int64_t> current_latency_us{0};       // Current latency in microseconds
    std::atomic<int64_t> average_latency_us{0};       // Average latency in microseconds
    
    // Network performance
    std::atomic<size_t> network_frames_received{0};   // Network frames received
    std::atomic<size_t> network_frames_processed{0};  // Network frames processed
    std::atomic<double> network_quality_score{1.0};   // Network quality score
    std::atomic<size_t> network_errors{0};            // Network error count
    
    // Processing pipeline
    std::atomic<size_t> frames_processed{0};          // Total frames processed
    std::atomic<size_t> frames_dropped{0};            // Frames dropped due to errors
    std::atomic<size_t> etsi_compliance_violations{0}; // ETSI compliance violations
    std::atomic<size_t> quality_degradations{0};      // Quality degradation events
    
    // Resource utilization
    std::atomic<double> cpu_usage{0.0};               // CPU usage percentage
    std::atomic<double> memory_usage_mb{0.0};         // Memory usage in MB
    std::atomic<double> disk_io_mbps{0.0};            // Disk I/O rate in MB/s
    std::atomic<double> network_bandwidth_mbps{0.0};  // Network bandwidth in MB/s
    
    // Integration health
    std::atomic<double> integration_health{1.0};      // Overall integration health
    std::atomic<bool> real_time_constraints_met{true}; // Real-time constraints status
    std::atomic<bool> quality_targets_met{true};      // Quality targets status
    std::atomic<bool> performance_targets_met{true};  // Performance targets status
    
    void reset() {
        current_fps = 0.0;
        average_fps = 0.0;
        peak_fps = 0.0;
        current_latency_us = 0;
        average_latency_us = 0;
        network_frames_received = 0;
        network_frames_processed = 0;
        network_quality_score = 1.0;
        network_errors = 0;
        frames_processed = 0;
        frames_dropped = 0;
        etsi_compliance_violations = 0;
        quality_degradations = 0;
        cpu_usage = 0.0;
        memory_usage_mb = 0.0;
        disk_io_mbps = 0.0;
        network_bandwidth_mbps = 0.0;
        integration_health = 1.0;
        real_time_constraints_met = true;
        quality_targets_met = true;
        performance_targets_met = true;
    }
};

/**
 * @brief Integration event for system notifications
 */
struct IntegrationEvent {
    QString event_id;                                  // Unique event identifier
    QString event_type;                                // Event type classification
    QString source_component;                          // Source component
    QString description;                               // Event description
    QDateTime timestamp;                               // Event timestamp
    QString severity;                                  // Event severity (INFO, WARNING, ERROR, CRITICAL)
    QJsonObject metadata;                              // Additional event metadata
    
    bool is_critical() const {
        return severity == "CRITICAL" || severity == "ERROR";
    }
    
    bool requires_action() const {
        return severity == "CRITICAL" || severity == "ERROR" || severity == "WARNING";
    }
};

/**
 * @brief ETI Network Integration Engine
 * 
 * Comprehensive integration layer implementing:
 * - Real-time ETI stream processing with network components
 * - Performance optimization maintaining >7,482 FPS processing
 * - ETSI compliance validation in streaming context
 * - Professional broadcast workflow automation
 * - Quality monitoring and adaptive control
 * - Equipment integration and redundancy management
 */
class EtiNetworkIntegration : public QObject {
    Q_OBJECT
    
public:
    explicit EtiNetworkIntegration(QObject* parent = nullptr);
    explicit EtiNetworkIntegration(const IntegrationConfig& config, QObject* parent = nullptr);
    ~EtiNetworkIntegration();
    
    // Integration lifecycle
    bool initialize();
    bool initialize(const IntegrationConfig& config);
    void shutdown();
    bool isInitialized() const;
    bool isRunning() const;
    
    // Configuration management
    void setConfiguration(const IntegrationConfig& config);
    IntegrationConfig getConfiguration() const;
    void setIntegrationMode(IntegrationMode mode);
    IntegrationMode getIntegrationMode() const;
    
    // Real-time processing control
    bool startRealTimeProcessing(const QString& multicastAddress, quint16 port);
    bool startRealTimeProcessing(const ETIStreamInfo& streamInfo);
    void stopRealTimeProcessing();
    bool isRealTimeProcessingActive() const;
    
    // File processing integration
    bool processFile(const QString& filename);
    bool processFileWithRecording(const QString& inputFilename, const QString& outputDirectory);
    void stopFileProcessing();
    bool isFileProcessingActive() const;
    
    // Stream management
    QList<ETIStreamInfo> getAvailableStreams() const;
    ETIStreamInfo getCurrentStream() const;
    bool switchToStream(const QString& streamId);
    void enableStreamRecording(bool enabled);
    void enableStreamPlayback(const QString& recordingFile);
    
    // Performance monitoring
    IntegrationStatistics getStatistics() const;
    double getCurrentProcessingFPS() const;
    std::chrono::microseconds getCurrentLatency() const;
    double getSystemHealth() const;
    bool arePerformanceTargetsMet() const;
    
    // Quality management
    double getCurrentQualityScore() const;
    bool isETSICompliant() const;
    QStringList getComplianceViolations() const;
    void setQualityThreshold(double threshold);
    void enableAdaptiveQuality(bool enabled);
    
    // Network discovery and management
    void startNetworkDiscovery();
    void stopNetworkDiscovery();
    QList<NetworkInterfaceInfo> getNetworkInterfaces() const;
    void setPreferredNetworkInterface(const QString& interfaceName);
    
    // Broadcast equipment integration (removed - stub implementation)
    // bool connectToBroadcastEquipment(const EquipmentConfig& config);
    // void disconnectFromBroadcastEquipment(const QString& equipmentId);
    // QStringList getConnectedEquipment() const;
    // void enableEquipmentRedundancy(bool enabled);
    
    // Error handling and recovery
    QList<IntegrationEvent> getRecentEvents() const;
    QStringList getActiveAlerts() const;
    void acknowledgeAlert(const QString& alertId);
    void enableAutoRecovery(bool enabled);
    
    // Diagnostics and reporting
    QString getSystemDiagnostics() const;
    QString getPerformanceReport() const;
    QString getComplianceReport() const;
    void exportDiagnostics(const QString& filename) const;
    void resetStatistics();
    
    // Callback registration for real-time notifications
    using FrameCallback = std::function<void(const eti::EtiFrame&, const NetworkQualityMetrics&)>;
    using EventCallback = std::function<void(const IntegrationEvent&)>;
    using PerformanceCallback = std::function<void(const IntegrationStatistics&)>;
    using QualityCallback = std::function<void(double qualityScore, bool compliant)>;
    
    void setFrameCallback(FrameCallback callback);
    void setEventCallback(EventCallback callback);
    void setPerformanceCallback(PerformanceCallback callback);
    void setQualityCallback(QualityCallback callback);

signals:
    // Integration lifecycle signals
    void integrationInitialized();
    void integrationShutdown();
    void integrationModeChanged(IntegrationMode mode);
    
    // Processing signals
    void realTimeProcessingStarted(const QString& streamId);
    void realTimeProcessingStopped();
    void fileProcessingStarted(const QString& filename);
    void fileProcessingCompleted(const QString& filename);
    
    // Performance signals
    void performanceUpdate(const IntegrationStatistics& statistics);
    void performanceTargetViolated(const QString& target, double currentValue, double targetValue);
    void latencyThresholdExceeded(std::chrono::microseconds currentLatency, std::chrono::microseconds threshold);
    void fpsDropDetected(double currentFPS, double targetFPS);
    
    // Quality signals
    void qualityUpdate(double qualityScore, bool etsiCompliant);
    void qualityDegradation(double qualityScore, const QString& reason);
    void etsiComplianceViolation(const QString& violation, const QString& standard);
    void qualityRecovered(double qualityScore);
    
    // Network signals
    void streamDiscovered(const ETIStreamInfo& streamInfo);
    void streamLost(const QString& streamId);
    void networkQualityChanged(const NetworkQualityMetrics& metrics);
    void failoverTriggered(const QString& fromStreamId, const QString& toStreamId);
    
    // Equipment signals
    void equipmentConnected(const QString& equipmentId);
    void equipmentDisconnected(const QString& equipmentId);
    void equipmentError(const QString& equipmentId, const QString& error);
    void redundancyStatusChanged(bool active);
    
    // Event signals
    void integrationEvent(const IntegrationEvent& event);
    void criticalAlert(const QString& alertId, const QString& message);
    void systemHealthChanged(double healthScore);
    void autoRecoveryTriggered(const QString& component, const QString& action);

private slots:
    // ETI Processor integration
    void handleEtiFrame(quint64 frameNumber, const QByteArray& data);
    void handleEtiError(const QString& error);
    void handleEnsembleDiscovered(const eti::Ensemble& ensemble);
    void handleServiceDiscovered(const eti::DabService& service);
    void handleServiceQualityChanged(uint16_t serviceId, double signalStrength, double errorRate);
    
    // Network receiver integration
    void handleNetworkFrame(const eti::EtiFrame& frame, const NetworkQualityMetrics& quality);
    void handleNetworkError(const QString& error, int severity);
    void handleNetworkQualityUpdate(const NetworkQualityMetrics& metrics);
    
    // Streaming processor integration
    void handleProcessedFrame(const ProcessedFrame& frame);
    void handleStreamingPerformanceUpdate(const StreamingPerformance& performance);
    void handleStreamingError(const QString& error, int severity);
    void handleQualityDegradation(const QString& reason, double quality);
    
    // Discovery integration
    void handleStreamDiscovered(const ETIStreamInfo& streamInfo);
    void handleStreamLost(const QString& streamId);
    void handleNetworkInterfaceChanged(const QString& interfaceName);
    
    // Broadcast interface integration
    void handleEquipmentConnected(const QString& equipmentId);
    void handleEquipmentDisconnected(const QString& equipmentId);
    // void handleBroadcastParametersChanged(const BroadcastParams& params); // Removed - stub implementation
    void handleRedundancyStatusChanged(bool active);
    
    // Monitoring and maintenance
    void updateStatistics();
    void monitorPerformance();
    void checkSystemHealth();
    void performMaintenance();
    void handleAutoRecovery();

private:
    // Initialization and cleanup
    void initializeComponents();
    void cleanupComponents();
    void setupSignalConnections();
    void cleanupSignalConnections();
    void validateConfiguration(const IntegrationConfig& config);
    
    // Component integration
    void integrateEtiProcessor();
    void integrateNetworkReceiver();
    void integrateStreamingProcessor();
    void integrateNetworkDiscovery();
    void integrateRecordingManager();
    void integratePlaybackEngine();
    // void integrateBroadcastInterface(); // Removed - stub implementation
    
    // Performance optimization
    void optimizeProcessingPipeline();
    void adaptToPerformanceConstraints();
    void handlePerformanceDegradation();
    void optimizeResourceUsage();
    
    // Quality management
    void validateETSICompliance(const eti::EtiFrame& frame);
    void updateQualityMetrics(const eti::EtiFrame& frame, const NetworkQualityMetrics& networkQuality);
    void handleQualityThresholdViolation(double currentQuality);
    void adaptQualitySettings();
    
    // Event management
    void generateIntegrationEvent(const QString& type, const QString& source, 
                                 const QString& description, const QString& severity);
    void processIntegrationEvent(const IntegrationEvent& event);
    void manageEventHistory();
    
    // Error handling and recovery
    void handleComponentError(const QString& component, const QString& error);
    void attemptErrorRecovery(const QString& component);
    bool validateSystemHealth();
    void performSystemRecovery();
    
    // Core components
    std::unique_ptr<EtiProcessor> m_etiProcessor;
    std::unique_ptr<EtiOverIpReceiver> m_networkReceiver;
    std::unique_ptr<StreamingProcessor> m_streamingProcessor;
    std::unique_ptr<NetworkDiscovery> m_networkDiscovery;
    std::unique_ptr<eti::StreamRecordingManager> m_recordingManager;
    std::unique_ptr<StreamPlayback> m_playbackEngine;
    // std::unique_ptr<BroadcastInterface> m_broadcastInterface; // Removed - stub implementation
    
    // Integration configuration and state
    IntegrationConfig m_config;
    IntegrationStatistics m_statistics;
    std::atomic<IntegrationMode> m_currentMode{IntegrationMode::REAL_TIME_ANALYSIS};
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_realTimeActive{false};
    std::atomic<bool> m_fileProcessingActive{false};
    
    // Current stream information
    ETIStreamInfo m_currentStream;
    QString m_currentRecordingSession;
    QString m_currentPlaybackFile;
    
    // Performance monitoring
    std::unique_ptr<QTimer> m_statisticsTimer;
    std::unique_ptr<QTimer> m_performanceTimer;
    std::unique_ptr<QTimer> m_healthTimer;
    std::unique_ptr<QTimer> m_maintenanceTimer;
    
    // Quality and compliance tracking
    std::atomic<double> m_currentQualityScore{1.0};
    std::atomic<bool> m_etsiCompliant{true};
    QStringList m_complianceViolations;
    mutable QMutex m_complianceMutex;
    
    // Event management
    std::queue<IntegrationEvent> m_eventHistory;
    QStringList m_activeAlerts;
    mutable QMutex m_eventMutex;
    std::atomic<bool> m_autoRecoveryEnabled{true};
    
    // Callbacks
    FrameCallback m_frameCallback;
    EventCallback m_eventCallback;
    PerformanceCallback m_performanceCallback;
    QualityCallback m_qualityCallback;
    mutable QMutex m_callbackMutex;
    
    // Threading and synchronization
    mutable QMutex m_integrationMutex;
    std::unique_ptr<QThread> m_processingThread;
    
    // Performance tracking
    std::chrono::steady_clock::time_point m_integrationStartTime;
    std::chrono::steady_clock::time_point m_lastFrameTime;
    std::vector<double> m_fpsHistory;
    std::vector<std::chrono::microseconds> m_latencyHistory;
    
    static constexpr size_t HISTORY_SIZE = 100;
    static constexpr size_t MAX_EVENT_HISTORY = 1000;
    static constexpr std::chrono::milliseconds STATISTICS_UPDATE_INTERVAL{1000};
    static constexpr std::chrono::milliseconds PERFORMANCE_CHECK_INTERVAL{500};
    static constexpr std::chrono::milliseconds HEALTH_CHECK_INTERVAL{5000};
    static constexpr std::chrono::milliseconds MAINTENANCE_INTERVAL{30000};
};

/**
 * @brief Factory for creating specialized integration configurations
 */
class EtiNetworkIntegrationFactory {
public:
    /**
     * @brief Create real-time analysis integration
     */
    static std::unique_ptr<EtiNetworkIntegration> createRealTimeAnalysisIntegration();
    
    /**
     * @brief Create broadcast monitoring integration
     */
    static std::unique_ptr<EtiNetworkIntegration> createBroadcastMonitoringIntegration();
    
    /**
     * @brief Create quality assessment integration
     */
    static std::unique_ptr<EtiNetworkIntegration> createQualityAssessmentIntegration();
    
    /**
     * @brief Create hybrid processing integration
     */
    static std::unique_ptr<EtiNetworkIntegration> createHybridProcessingIntegration();
    
    /**
     * @brief Create custom integration with specific configuration
     */
    static std::unique_ptr<EtiNetworkIntegration> createCustomIntegration(const IntegrationConfig& config);
    
    /**
     * @brief Create performance-optimized integration for maximum throughput
     */
    static std::unique_ptr<EtiNetworkIntegration> createPerformanceOptimizedIntegration();
};

} // namespace eti_network

#endif // ETI_NETWORK_INTEGRATION_H