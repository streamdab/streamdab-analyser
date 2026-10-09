/**
 * @file modern_eti_controller.hpp
 * @brief Modern ETI Processing Controller with Qt6 Integration
 * 
 * Central controller that integrates ZeroMQ ETI client, ETI-NI processor,
 * Reed-Solomon codec, and modern frame parser with Qt6 signals/slots
 * for real-time communication and comprehensive ETI processing.
 * 
 * @author StreamDAB Development Team
 * @date 2025
 * @copyright Copyright (c) 2025 StreamDAB Technologies
 */

#pragma once

#include "zeromq_eti_client.hpp"
#include "eti_ni_processor.hpp"
#include "modern_eti_frame_parser.hpp"
#include "reed_solomon_codec.hpp"
#include "eti_types.hpp"

#include <QObject>
#include <QTimer>
#include <QString>
#include <QMutex>
#include <memory>
#include <atomic>
#include <chrono>

namespace eti::controller {

/**
 * @brief Processing pipeline configuration
 */
struct ProcessingConfig {
    // ZeroMQ configuration
    eti::network::ZMQConnectionConfig zmq_config;
    bool auto_reconnect{true};
    int reconnect_interval_ms{5000};
    
    // Frame processing configuration
    eti::modern::ProcessingConfig frame_parser_config;
    bool enable_eti_ni_processing{true};
    bool enable_reed_solomon_correction{true};
    
    // Performance settings
    double target_fps{1000.0};        // Target processing rate
    int max_frame_buffer_size{1000};  // Maximum frames to buffer
    bool enable_performance_monitoring{true};
    int stats_update_interval_ms{1000};
    
    // Quality settings
    bool enable_etsi_compliance_checking{true};
    bool enable_error_recovery{true};
    double min_signal_quality{80.0};  // Minimum acceptable signal quality %
    
    /**
     * @brief Validate configuration parameters
     */
    [[nodiscard]] bool isValid() const {
        return zmq_config.isValid() && 
               target_fps > 0.0 && 
               max_frame_buffer_size > 0 &&
               min_signal_quality >= 0.0 && min_signal_quality <= 100.0;
    }
};

/**
 * @brief Comprehensive processing statistics
 */
struct ProcessingStatistics {
    // Connection statistics
    eti::network::ConnectionStatus connection_status;
    std::chrono::milliseconds connection_uptime{0};
    
    // Frame processing statistics
    uint64_t total_frames_processed{0};
    uint64_t frames_with_errors{0};
    uint64_t frames_corrected{0};
    uint64_t frames_uncorrectable{0};
    
    // Performance metrics
    double current_fps{0.0};
    double average_fps{0.0};
    std::chrono::microseconds avg_processing_latency{0};
    std::chrono::microseconds max_processing_latency{0};
    
    // Quality metrics
    double overall_signal_quality{100.0};
    double etsi_compliance_score{100.0};
    eti::ni::TISTInfo::TimestampType tist_sync_status{eti::ni::TISTInfo::TimestampType::INVALID};
    std::chrono::microseconds sync_accuracy{0};
    
    // Error correction statistics
    uint64_t total_errors_detected{0};
    uint64_t total_errors_corrected{0};
    double error_correction_efficiency{100.0};
    
    // Service discovery
    uint16_t discovered_services{0};
    QString ensemble_label;
    uint16_t ensemble_id{0};
    
    /**
     * @brief Check if processing is healthy
     */
    [[nodiscard]] bool isHealthy() const {
        return connection_status.isConnected() &&
               current_fps > 100.0 &&  // Minimum 100 FPS
               overall_signal_quality > 70.0 &&
               sync_accuracy < std::chrono::microseconds{5000}; // <5ms sync
    }
    
    /**
     * @brief Get overall health score (0-100)
     */
    [[nodiscard]] double getHealthScore() const {
        double score = 100.0;
        
        if (!connection_status.isConnected()) score -= 40.0;
        if (current_fps < 500.0) score -= 20.0;
        if (overall_signal_quality < 90.0) score -= 15.0;
        if (sync_accuracy > std::chrono::microseconds{2000}) score -= 15.0;
        if (error_correction_efficiency < 95.0) score -= 10.0;
        
        return std::max(0.0, score);
    }
};

/**
 * @brief Modern ETI Processing Controller
 * 
 * Orchestrates the complete ETI processing pipeline from ZeroMQ reception
 * through frame parsing, error correction, and service discovery with
 * Qt6 signals/slots for real-time communication.
 */
class ModernETIController : public QObject {
    Q_OBJECT
    
public:
    explicit ModernETIController(QObject* parent = nullptr);
    ~ModernETIController() override;
    
    // Disable copy/move for proper resource management
    ModernETIController(const ModernETIController&) = delete;
    ModernETIController& operator=(const ModernETIController&) = delete;
    ModernETIController(ModernETIController&&) = delete;
    ModernETIController& operator=(ModernETIController&&) = delete;
    
    /**
     * @brief Initialize controller with configuration
     * @param config Processing pipeline configuration
     * @return true if initialization successful
     */
    bool initialize(const ProcessingConfig& config);
    
    /**
     * @brief Check if controller is initialized
     */
    [[nodiscard]] bool isInitialized() const noexcept {
        return initialized_.load();
    }
    
    /**
     * @brief Start ETI processing pipeline
     * @return true if started successfully
     */
    bool startProcessing();
    
    /**
     * @brief Stop ETI processing pipeline
     */
    void stopProcessing();
    
    /**
     * @brief Check if processing is active
     */
    [[nodiscard]] bool isProcessing() const noexcept {
        return processing_.load();
    }
    
    /**
     * @brief Connect to ETI stream source
     * @param endpoint ZeroMQ endpoint URL
     * @return true if connection initiated successfully
     */
    bool connectToStream(const QString& endpoint);
    
    /**
     * @brief Disconnect from current stream
     */
    void disconnectFromStream();
    
    /**
     * @brief Get current processing statistics
     */
    [[nodiscard]] ProcessingStatistics getStatistics() const;
    
    /**
     * @brief Get current configuration
     */
    [[nodiscard]] ProcessingConfig getConfiguration() const {
        QMutexLocker locker(&config_mutex_);
        return config_;
    }
    
    /**
     * @brief Update configuration at runtime
     * @param config New configuration
     * @return true if update successful
     */
    bool updateConfiguration(const ProcessingConfig& config);
    
    /**
     * @brief Reset all statistics and counters
     */
    void resetStatistics();
    
    /**
     * @brief Enable/disable specific processing features
     */
    void setETINIProcessingEnabled(bool enabled);
    void setReedSolomonCorrectionEnabled(bool enabled);
    void setETSIComplianceCheckingEnabled(bool enabled);
    void setPerformanceMonitoringEnabled(bool enabled);
    
    /**
     * @brief Get individual component pointers for advanced configuration
     */
    [[nodiscard]] std::shared_ptr<eti::network::ZeroMQETIClient> getZMQClient() const {
        return zmq_client_;
    }
    
    [[nodiscard]] std::shared_ptr<eti::modern::ModernETIFrameParser> getFrameParser() const {
        return frame_parser_;
    }
    
    [[nodiscard]] std::shared_ptr<eti::ni::ETINIProcessor> getETINIProcessor() const {
        return eti_ni_processor_;
    }
    
    [[nodiscard]] std::shared_ptr<eti::codec::ETIReedSolomonProcessor> getRSProcessor() const {
        return reed_solomon_processor_;
    }

public slots:
    /**
     * @brief Connect using endpoint URL string
     * @param endpoint ZeroMQ endpoint URL
     */
    void connectToEndpoint(const QString& endpoint);
    
    /**
     * @brief Reconnect to last known endpoint
     */
    void reconnect();
    
    /**
     * @brief Update target FPS
     * @param fps Target frames per second
     */
    void setTargetFPS(double fps);
    
    /**
     * @brief Update signal quality threshold
     * @param quality Minimum signal quality percentage
     */
    void setSignalQualityThreshold(double quality);

signals:
    /**
     * @brief Emitted when controller is initialized
     * @param success true if initialization successful
     */
    void initialized(bool success);
    
    /**
     * @brief Emitted when processing starts/stops
     * @param processing true if processing started, false if stopped
     */
    void processingStateChanged(bool processing);
    
    /**
     * @brief Emitted when connection state changes
     * @param status Current connection status
     */
    void connectionStatusChanged(const eti::network::ConnectionStatus& status);
    
    /**
     * @brief Emitted when ETI frame is completely processed
     * @param frame_number Frame sequence number
     * @param frame Processed ETI frame
     * @param processing_time Total processing time
     */
    void etiFrameProcessed(uint32_t frame_number, 
                          const eti::EtiFrame& frame,
                          std::chrono::microseconds processing_time);
    
    /**
     * @brief Emitted when ETI-NI frame with TIST is processed
     * @param frame ETI-NI frame with timestamp info
     * @param sync_quality Synchronization quality (0-100)
     */
    void etiNIFrameProcessed(const eti::ni::ETINIFrame& frame, double sync_quality);
    
    /**
     * @brief Emitted when ensemble information is discovered/updated
     * @param ensemble Ensemble information
     */
    void ensembleDiscovered(const eti::Ensemble& ensemble);
    
    /**
     * @brief Emitted when DAB service is discovered
     * @param service Service information
     */
    void serviceDiscovered(const eti::DabService& service);
    
    /**
     * @brief Emitted when errors are detected and corrected
     * @param frame_number Frame number
     * @param correction_result Error correction details
     */
    void errorsCorrect(uint32_t frame_number, 
                        const eti::codec::CorrectionResult& correction_result);
    
    /**
     * @brief Emitted when uncorrectable errors are detected
     * @param frame_number Frame number
     * @param error_description Error description
     */
    void uncorrectableErrors(uint32_t frame_number, const QString& error_description);
    
    /**
     * @brief Emitted when ETSI compliance issue is detected
     * @param standard ETSI standard reference
     * @param issue Issue description
     * @param severity Severity level
     */
    void etsiComplianceIssue(const QString& standard, 
                            const QString& issue, 
                            const QString& severity);
    
    /**
     * @brief Emitted periodically with processing statistics
     * @param stats Current processing statistics
     */
    void statisticsUpdated(const ProcessingStatistics& stats);
    
    /**
     * @brief Emitted when performance target is achieved or missed
     * @param target_met true if target was met
     * @param current_fps Current processing rate
     * @param target_fps Target processing rate
     */
    void performanceTargetStatus(bool target_met, double current_fps, double target_fps);
    
    /**
     * @brief Emitted when synchronization status changes
     * @param synchronized true if TIST synchronization is locked
     * @param accuracy Synchronization accuracy in microseconds
     */
    void synchronizationStatusChanged(bool synchronized, 
                                     std::chrono::microseconds accuracy);
    
    /**
     * @brief Emitted when overall signal quality changes significantly
     * @param quality Current signal quality percentage
     * @param threshold Configured quality threshold
     */
    void signalQualityChanged(double quality, double threshold);

private slots:
    /**
     * @brief Handle raw ETI frame from ZeroMQ client
     */
    void handleRawETIFrame(const QByteArray& frame_data, 
                          const std::chrono::system_clock::time_point& receive_timestamp);
    
    /**
     * @brief Handle parsed ETI frame from frame parser
     */
    void handleParsedETIFrame(uint32_t frame_number, 
                             const eti::EtiFrame& frame,
                             std::chrono::nanoseconds parse_time);
    
    /**
     * @brief Handle ETI-NI frame processing
     */
    void handleETINIFrame(const eti::ni::ETINIFrame& frame);
    
    /**
     * @brief Handle TIST extraction
     */
    void handleTISTExtracted(const eti::ni::TISTInfo& tist_info, uint32_t frame_number);
    
    /**
     * @brief Handle ensemble discovery
     */
    void handleEnsembleDiscovered(const eti::Ensemble& ensemble);
    
    /**
     * @brief Handle service discovery
     */
    void handleServiceDiscovered(const eti::DabService& service);
    
    /**
     * @brief Handle error correction results
     */
    void handleErrorsCorrected(uint32_t frame_number, 
                              const eti::codec::ETIReedSolomonProcessor::ETIErrorStats& stats);
    
    /**
     * @brief Handle connection status changes
     */
    void handleConnectionStatusChanged(const eti::network::ConnectionStatus& status);
    
    /**
     * @brief Handle performance updates
     */
    void handlePerformanceUpdate(const eti::network::ZeroMQETIClient::PerformanceStats& stats);
    
    /**
     * @brief Handle synchronization status changes
     */
    void handleSynchronizationStatusChanged(const eti::ni::ETINIProcessor::SyncStatus& status);
    
    /**
     * @brief Update statistics timer handler
     */
    void updateStatistics();

private:
    // Core components
    std::shared_ptr<eti::network::ZeroMQETIClient> zmq_client_;
    std::shared_ptr<eti::modern::ModernETIFrameParser> frame_parser_;
    std::shared_ptr<eti::ni::ETINIProcessor> eti_ni_processor_;
    std::shared_ptr<eti::codec::ETIReedSolomonProcessor> reed_solomon_processor_;
    
    // Configuration and state
    mutable QMutex config_mutex_;
    ProcessingConfig config_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> processing_{false};
    
    // Statistics and monitoring
    mutable QMutex stats_mutex_;
    ProcessingStatistics statistics_;
    QTimer* stats_timer_{nullptr};
    std::chrono::system_clock::time_point start_time_;
    std::chrono::system_clock::time_point last_frame_time_;
    
    // Frame tracking
    std::atomic<uint32_t> frame_counter_{0};
    std::vector<std::chrono::microseconds> recent_processing_times_;
    
    // Component state tracking
    bool eti_ni_enabled_{true};
    bool reed_solomon_enabled_{true};
    bool etsi_compliance_enabled_{true};
    bool performance_monitoring_enabled_{true};
    
    // Helper methods
    void connectSignals();
    void disconnectSignals();
    void updateProcessingStatistics();
    void updateQualityMetrics();
    void checkPerformanceTargets();
    void validateSignalQuality();
    
    // Processing pipeline methods
    void processFrameThroughPipeline(const QByteArray& frame_data,
                                   const std::chrono::system_clock::time_point& receive_timestamp);
    
    // Statistics calculation helpers
    double calculateAverageFPS() const;
    std::chrono::microseconds calculateAverageLatency() const;
    double calculateOverallQuality() const;
    double calculateETSIComplianceScore() const;
};

/**
 * @brief Factory function for creating configured ETI controller
 */
[[nodiscard]] std::unique_ptr<ModernETIController> createETIController(
    const ProcessingConfig& config = {},
    QObject* parent = nullptr);

/**
 * @brief Utility functions for controller management
 */
namespace utils {
    /**
     * @brief Create default configuration for ODR-DabMux
     * @param endpoint ZeroMQ endpoint URL
     * @return Default configuration
     */
    [[nodiscard]] ProcessingConfig createDefaultConfig(const QString& endpoint = "tcp://localhost:9200");
    
    /**
     * @brief Create high-performance configuration
     * @param endpoint ZeroMQ endpoint URL
     * @return High-performance configuration
     */
    [[nodiscard]] ProcessingConfig createHighPerformanceConfig(const QString& endpoint = "tcp://localhost:9200");
    
    /**
     * @brief Create quality-focused configuration
     * @param endpoint ZeroMQ endpoint URL
     * @return Quality-focused configuration
     */
    [[nodiscard]] ProcessingConfig createQualityConfig(const QString& endpoint = "tcp://localhost:9200");
    
    /**
     * @brief Validate controller health
     * @param controller Controller to check
     * @return Health status and recommendations
     */
    struct HealthCheck {
        bool healthy{false};
        double health_score{0.0};
        std::vector<QString> issues;
        std::vector<QString> recommendations;
    };
    
    [[nodiscard]] HealthCheck checkControllerHealth(const ModernETIController& controller);
}

} // namespace eti::controller