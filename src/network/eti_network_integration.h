/**
 * @file eti_network_integration.h
 * @brief ETI Network Integration Manager for Real-time Streaming
 * 
 * Integrates ETI-over-IP network streaming with existing ETI processor and GUI:
 * - Real-time ETI-over-IP reception and processing
 * - Seamless integration with existing EtiProcessor
 * - GUI integration for live stream monitoring
 * - Professional broadcast workflow support
 * - Performance optimization for >900 FPS throughput
 * - Latency optimization for <17ms end-to-end processing
 * 
 * @author Network/Stream Agent
 * @date 2025-09-22
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 */

#ifndef ETI_NETWORK_INTEGRATION_H
#define ETI_NETWORK_INTEGRATION_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>

#include "eti_over_ip_receiver.h"
#include "streaming_processor.h"
#include "circular_buffer_manager.h"
#include "../core/eti_processor.hpp"
#include "../core/eti_types.hpp"

namespace eti_network {

/**
 * @brief Integration mode for different use cases
 */
enum class IntegrationMode {
    FILE_ONLY,              // File-based processing only
    LIVE_ONLY,              // Live streaming only
    DUAL_MODE,              // Both file and live streaming
    HYBRID_MODE             // Seamless switching between file and live
};

/**
 * @brief Network streaming configuration
 */
struct NetworkStreamingConfig {
    // Network settings
    QString multicast_address = "239.192.0.1";
    quint16 port = 9200;
    QString network_interface;
    
    // Performance targets
    double target_fps = 1000.0;                            // >900 FPS requirement
    std::chrono::microseconds target_latency{17000};       // <17ms requirement
    size_t memory_limit_mb = 30;                           // <30MB requirement
    
    // Integration settings
    IntegrationMode mode = IntegrationMode::DUAL_MODE;
    bool enable_gui_integration = true;
    bool enable_real_time_analysis = true;
    bool enable_professional_mode = true;
    
    // Buffer configuration
    size_t buffer_size = 1000;
    bool adaptive_buffering = true;
    bool zero_copy_mode = true;
    
    // Quality settings
    bool enable_redundancy = false;
    QString backup_address;
    double min_signal_quality = 0.95;
    double max_packet_loss = 0.001;
    double max_jitter_ms = 5.0;
    
    // Processing configuration
    ProcessingMode processing_mode = ProcessingMode::LOW_LATENCY;
    bool enable_etsi_validation = true;
    bool enable_service_discovery = true;
    bool enable_audio_monitoring = false;
};

/**
 * @brief Integration status and metrics
 */
struct IntegrationStatus {
    // Connection status
    bool network_connected = false;
    bool processing_active = false;
    bool gui_connected = false;
    
    // Performance metrics
    double current_fps = 0.0;
    std::chrono::microseconds current_latency{0};
    size_t memory_usage_mb = 0;
    double cpu_usage = 0.0;
    
    // Quality metrics
    double signal_quality = 0.0;
    double packet_loss_rate = 0.0;
    double jitter_ms = 0.0;
    
    // Processing statistics
    size_t frames_received = 0;
    size_t frames_processed = 0;
    size_t frames_dropped = 0;
    size_t processing_errors = 0;
    
    // Integration health
    bool is_healthy = true;
    QString status_message;
    std::chrono::steady_clock::time_point last_update;
};

/**
 * @brief ETI Network Integration Manager
 * 
 * Manages the integration between ETI-over-IP network streaming
 * and the existing ETI processor and GUI components.
 */
class EtiNetworkIntegration : public QObject {
    Q_OBJECT
    
public:
    explicit EtiNetworkIntegration(QObject* parent = nullptr);
    explicit EtiNetworkIntegration(EtiProcessor* etiProcessor, QObject* parent = nullptr);
    ~EtiNetworkIntegration();
    
    // Configuration management
    void setConfiguration(const NetworkStreamingConfig& config);
    NetworkStreamingConfig getConfiguration() const;
    void setEtiProcessor(EtiProcessor* processor);
    EtiProcessor* getEtiProcessor() const;
    
    // Network streaming control
    bool startNetworkStreaming();
    bool startNetworkStreaming(const QString& address, quint16 port);
    void stopNetworkStreaming();
    bool isNetworkStreamingActive() const;
    
    // Integration mode management
    void setIntegrationMode(IntegrationMode mode);
    IntegrationMode getIntegrationMode() const;
    bool switchToLiveMode();
    bool switchToFileMode();
    bool enableDualMode();
    
    // Performance configuration
    void setPerformanceTargets(double fps, std::chrono::microseconds latency, size_t memoryMB);
    void enableProfessionalMode(bool enabled);
    void enableRealTimeAnalysis(bool enabled);
    void setProcessingMode(ProcessingMode mode);
    
    // Quality management
    void setQualityThresholds(double minSignalQuality, double maxPacketLoss, double maxJitter);
    void enableRedundancy(bool enabled, const QString& backupAddress = QString());
    void enableAdaptiveQuality(bool enabled);
    
    // Status and monitoring
    IntegrationStatus getStatus() const;
    bool isHealthy() const;
    QString getStatusMessage() const;
    bool arePerformanceTargetsMet() const;
    
    // GUI integration
    void enableGuiIntegration(bool enabled);
    void connectToMainWindow(QObject* mainWindow);
    void updateGuiStatus();
    
    // Advanced features
    void enableNetworkRecording(bool enabled, const QString& outputPath = QString());
    void enableStreamComparison(bool enabled);
    void exportNetworkMetrics(const QString& filename);
    void generateIntegrationReport();
    
    // Callback registration
    using FrameCallback = std::function<void(const eti::EtiFrame&, const NetworkQualityMetrics&)>;
    using StatusCallback = std::function<void(const IntegrationStatus&)>;
    using ErrorCallback = std::function<void(const QString&, int severity)>;
    
    void setFrameCallback(FrameCallback callback);
    void setStatusCallback(StatusCallback callback);
    void setErrorCallback(ErrorCallback callback);

signals:
    // Network streaming signals
    void networkStreamingStarted();
    void networkStreamingStopped();
    void networkStreamingError(const QString& error);
    
    // Frame processing signals
    void liveFrameReceived(const eti::EtiFrame& frame, const NetworkQualityMetrics& quality);
    void frameProcessed(const eti::EtiFrame& frame, bool fromNetwork);
    void batchProcessed(size_t frameCount, double avgLatency);
    
    // Status and monitoring signals
    void statusUpdated(const IntegrationStatus& status);
    void performanceTargetMissed(const QString& target, double current, double expected);
    void qualityDegradation(const QString& reason, double quality);
    void integrationModeChanged(IntegrationMode oldMode, IntegrationMode newMode);
    
    // Health and error signals
    void healthStatusChanged(bool healthy, const QString& reason);
    void integrationError(const QString& error, int severity);
    void memoryLimitExceeded(size_t current, size_t limit);
    void latencyThresholdViolated(std::chrono::microseconds current, std::chrono::microseconds target);
    
    // GUI integration signals
    void guiUpdateRequired();
    void realTimeDataAvailable(const QVariantMap& data);

private slots:
    // Network receiver slots
    void handleNetworkFrame(const eti::EtiFrame& frame, const NetworkQualityMetrics& quality);
    void handleNetworkError(const QString& error, int severity);
    void handleNetworkQualityUpdate(const NetworkQualityMetrics& metrics);
    void handleNetworkConnection(bool connected);
    
    // Streaming processor slots
    void handleProcessedFrame(const ProcessedFrame& frame);
    void handleProcessingError(const QString& error, int severity);
    void handlePerformanceUpdate(const StreamingPerformance& performance);
    
    // Buffer manager slots
    void handleBufferOverflow(size_t droppedFrames);
    void handleBufferUnderflow();
    void handleBufferMetrics(const BufferMetricsSnapshot& metrics);
    
    // System monitoring slots
    void updateStatus();
    void monitorPerformance();
    void checkHealth();
    void performMaintenance();

private:
    // Initialization and cleanup
    void initializeIntegration();
    void cleanupIntegration();
    void setupConnections();
    void configureComponents();
    
    // Component management
    void createNetworkComponents();
    void destroyNetworkComponents();
    void optimizeForMode(IntegrationMode mode);
    
    // Integration logic
    void processNetworkFrame(const eti::EtiFrame& frame, const NetworkQualityMetrics& quality);
    void forwardToEtiProcessor(const eti::EtiFrame& frame);
    void updateIntegrationStatus();
    void handleModeSwitch(IntegrationMode newMode);
    
    // Performance optimization
    void optimizePerformance();
    void adaptToNetworkConditions();
    void balanceLatencyThroughput();
    void handleResourceConstraints();
    
    // Quality management
    void validateQualityMetrics(const NetworkQualityMetrics& metrics);
    void handleQualityDegradation();
    void applyQualityAdaptations();
    
    // Error handling and recovery
    void handleIntegrationError(const QString& error, int severity);
    void performErrorRecovery();
    void resetIntegrationState();
    
    // GUI integration helpers
    void updateGuiComponents();
    void sendGuiUpdates();
    void handleGuiRequests();
    
    // Monitoring and diagnostics
    void calculatePerformanceMetrics();
    void updateHealthStatus();
    void generateDiagnostics();
    
    // Configuration and state
    NetworkStreamingConfig m_config;
    IntegrationStatus m_status;
    IntegrationMode m_currentMode = IntegrationMode::FILE_ONLY;
    
    // Core components
    EtiProcessor* m_etiProcessor = nullptr; // External reference, not owned
    std::unique_ptr<EtiOverIpReceiver> m_networkReceiver;
    std::unique_ptr<StreamingProcessor> m_streamingProcessor;
    std::unique_ptr<CircularBufferManager> m_bufferManager; // Removed - stub implementation
    
    // Timers for monitoring and maintenance
    std::unique_ptr<QTimer> m_statusTimer;
    std::unique_ptr<QTimer> m_performanceTimer;
    std::unique_ptr<QTimer> m_healthTimer;
    std::unique_ptr<QTimer> m_maintenanceTimer;
    
    // State management
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_networkActive{false};
    std::atomic<bool> m_processingActive{false};
    std::atomic<bool> m_healthy{true};
    
    // Performance tracking
    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_lastFrameTime;
    std::atomic<size_t> m_totalFramesReceived{0};
    std::atomic<size_t> m_totalFramesProcessed{0};
    std::atomic<size_t> m_totalFramesDropped{0};
    
    // Thread safety
    mutable QMutex m_statusMutex;
    mutable QMutex m_configMutex;
    mutable QMutex m_callbackMutex;
    
    // Callbacks
    FrameCallback m_frameCallback;
    StatusCallback m_statusCallback;
    ErrorCallback m_errorCallback;
    
    // GUI integration
    QObject* m_mainWindow = nullptr;
    bool m_guiIntegrationEnabled = false;
    
    // Recording and analysis
    bool m_recordingEnabled = false;
    QString m_recordingPath;
    bool m_comparisonEnabled = false;
    
    // Constants
    static constexpr std::chrono::milliseconds STATUS_UPDATE_INTERVAL{1000};
    static constexpr std::chrono::milliseconds PERFORMANCE_UPDATE_INTERVAL{500};
    static constexpr std::chrono::milliseconds HEALTH_CHECK_INTERVAL{2000};
    static constexpr std::chrono::milliseconds MAINTENANCE_INTERVAL{10000};
    
    static constexpr size_t MAX_CONSECUTIVE_ERRORS = 10;
    static constexpr double HEALTH_SCORE_THRESHOLD = 0.8;
};

/**
 * @brief Factory for creating ETI network integrations
 */
class EtiNetworkIntegrationFactory {
public:
    /**
     * @brief Create integration for professional broadcast use
     */
    static std::unique_ptr<EtiNetworkIntegration> createBroadcastIntegration(EtiProcessor* processor);
    
    /**
     * @brief Create integration for low-latency applications
     */
    static std::unique_ptr<EtiNetworkIntegration> createLowLatencyIntegration(EtiProcessor* processor);
    
    /**
     * @brief Create integration for high-throughput applications
     */
    static std::unique_ptr<EtiNetworkIntegration> createHighThroughputIntegration(EtiProcessor* processor);
    
    /**
     * @brief Create integration for development and testing
     */
    static std::unique_ptr<EtiNetworkIntegration> createDevelopmentIntegration(EtiProcessor* processor);
    
    /**
     * @brief Create custom integration with specific configuration
     */
    static std::unique_ptr<EtiNetworkIntegration> createCustomIntegration(
        EtiProcessor* processor, const NetworkStreamingConfig& config);
};

} // namespace eti_network

#endif // ETI_NETWORK_INTEGRATION_H