/**
 * @file eti_network_integration.cpp
 * @brief ETI Network Integration Manager Implementation
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

#include "eti_network_integration.h"
#include "../utils/logger.h"
#include <QTimer>
#include <QVariantMap>
#include <algorithm>

namespace eti_network {

// =============================================================================
// EtiNetworkIntegration Implementation
// =============================================================================

EtiNetworkIntegration::EtiNetworkIntegration(QObject* parent)
    : QObject(parent)
    , m_statusTimer(std::make_unique<QTimer>(this))
    , m_performanceTimer(std::make_unique<QTimer>(this))
    , m_healthTimer(std::make_unique<QTimer>(this))
    , m_maintenanceTimer(std::make_unique<QTimer>(this)) {
    
    initializeIntegration();
}

EtiNetworkIntegration::EtiNetworkIntegration(EtiProcessor* etiProcessor, QObject* parent)
    : QObject(parent)
    , m_etiProcessor(etiProcessor)
    , m_statusTimer(std::make_unique<QTimer>(this))
    , m_performanceTimer(std::make_unique<QTimer>(this))
    , m_healthTimer(std::make_unique<QTimer>(this))
    , m_maintenanceTimer(std::make_unique<QTimer>(this)) {
    
    initializeIntegration();
}

EtiNetworkIntegration::~EtiNetworkIntegration() {
    cleanupIntegration();
}

void EtiNetworkIntegration::initializeIntegration() {
    Logger::instance().log(Logger::Info, "EtiNetworkIntegration", "Initializing ETI network integration");
    
    // Initialize default configuration
    m_config = NetworkStreamingConfig();
    m_config.target_fps = 1000.0;                          // >900 FPS requirement
    m_config.target_latency = std::chrono::microseconds{17000}; // <17ms requirement
    m_config.memory_limit_mb = 30;                         // <30MB requirement
    
    // Initialize status
    m_status = IntegrationStatus();
    m_status.last_update = std::chrono::steady_clock::now();
    m_startTime = std::chrono::steady_clock::now();
    
    // Setup timers
    setupConnections();
    
    // Set initial mode
    m_currentMode = IntegrationMode::FILE_ONLY;
    
    m_initialized = true;
    
    Logger::instance().log(Logger::Info, "EtiNetworkIntegration", "Integration initialized successfully");
}

void EtiNetworkIntegration::cleanupIntegration() {
    Logger::instance().log(Logger::Info, "EtiNetworkIntegration", "Cleaning up ETI network integration");
    
    // Stop network streaming
    stopNetworkStreaming();
    
    // Stop timers
    if (m_statusTimer && m_statusTimer->isActive()) {
        m_statusTimer->stop();
    }
    if (m_performanceTimer && m_performanceTimer->isActive()) {
        m_performanceTimer->stop();
    }
    if (m_healthTimer && m_healthTimer->isActive()) {
        m_healthTimer->stop();
    }
    if (m_maintenanceTimer && m_maintenanceTimer->isActive()) {
        m_maintenanceTimer->stop();
    }
    
    // Destroy network components
    destroyNetworkComponents();
}

void EtiNetworkIntegration::setupConnections() {
    // Setup timers
    m_statusTimer->setInterval(STATUS_UPDATE_INTERVAL.count());
    m_statusTimer->setSingleShot(false);
    connect(m_statusTimer.get(), &QTimer::timeout, this, &EtiNetworkIntegration::updateStatus);
    
    m_performanceTimer->setInterval(PERFORMANCE_UPDATE_INTERVAL.count());
    m_performanceTimer->setSingleShot(false);
    connect(m_performanceTimer.get(), &QTimer::timeout, this, &EtiNetworkIntegration::monitorPerformance);
    
    m_healthTimer->setInterval(HEALTH_CHECK_INTERVAL.count());
    m_healthTimer->setSingleShot(false);
    connect(m_healthTimer.get(), &QTimer::timeout, this, &EtiNetworkIntegration::checkHealth);
    
    m_maintenanceTimer->setInterval(MAINTENANCE_INTERVAL.count());
    m_maintenanceTimer->setSingleShot(false);
    connect(m_maintenanceTimer.get(), &QTimer::timeout, this, &EtiNetworkIntegration::performMaintenance);
    
    // Start monitoring timers
    m_statusTimer->start();
    m_performanceTimer->start();
    m_healthTimer->start();
    m_maintenanceTimer->start();
}

void EtiNetworkIntegration::setConfiguration(const NetworkStreamingConfig& config) {
    QMutexLocker locker(&m_configMutex);
    
    Logger::instance().log(Logger::Info, "EtiNetworkIntegration", 
                          QString("Updating configuration: FPS=%1, Latency=%2μs, Memory=%3MB")
                          .arg(config.target_fps)
                          .arg(config.target_latency.count())
                          .arg(config.memory_limit_mb));
    
    NetworkStreamingConfig oldConfig = m_config;
    m_config = config;
    
    // Reconfigure components if they exist
    configureComponents();
    
    // Handle mode changes
    if (oldConfig.mode != config.mode) {
        handleModeSwitch(config.mode);
    }
}

NetworkStreamingConfig EtiNetworkIntegration::getConfiguration() const {
    QMutexLocker locker(&m_configMutex);
    return m_config;
}

void EtiNetworkIntegration::setEtiProcessor(EtiProcessor* processor) {
    m_etiProcessor = processor;
    
    if (processor) {
        Logger::instance().log(Logger::Info, "EtiNetworkIntegration", "ETI processor connected");
    } else {
        Logger::instance().log(Logger::Warning, "EtiNetworkIntegration", "ETI processor disconnected");
    }
}

EtiProcessor* EtiNetworkIntegration::getEtiProcessor() const {
    return m_etiProcessor;
}

bool EtiNetworkIntegration::startNetworkStreaming() {
    return startNetworkStreaming(m_config.multicast_address, m_config.port);
}

bool EtiNetworkIntegration::startNetworkStreaming(const QString& address, quint16 port) {
    if (m_networkActive.load()) {
        Logger::instance().log(Logger::Warning, "EtiNetworkIntegration", "Network streaming already active");
        return false;
    }
    
    Logger::instance().log(Logger::Info, "EtiNetworkIntegration", 
                          QString("Starting network streaming on %1:%2").arg(address).arg(port));
    
    try {
        // Create network components if needed
        createNetworkComponents();
        
        // Configure components
        configureComponents();
        
        // Start network receiver
        if (!m_networkReceiver->startListening(address, port, m_config.network_interface)) {
            Logger::instance().log(Logger::Error, "EtiNetworkIntegration", "Failed to start network receiver");
            return false;
        }
        
        // Start streaming processor
        if (!m_streamingProcessor->startProcessing()) {
            Logger::instance().log(Logger::Error, "EtiNetworkIntegration", "Failed to start streaming processor");
            m_networkReceiver->stopListening();
            return false;
        }
        
        // Update state
        m_networkActive = true;
        m_processingActive = true;
        
        // Update status
        {
            QMutexLocker locker(&m_statusMutex);
            m_status.network_connected = true;
            m_status.processing_active = true;
            m_status.status_message = "Network streaming active";
        }
        
        emit networkStreamingStarted();
        
        Logger::instance().log(Logger::Info, "EtiNetworkIntegration", "Network streaming started successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "EtiNetworkIntegration", 
                              QString("Exception starting network streaming: %1").arg(e.what()));
        return false;
    }
}

void EtiNetworkIntegration::stopNetworkStreaming() {
    if (!m_networkActive.load()) {
        return;
    }
    
    Logger::instance().log(Logger::Info, "EtiNetworkIntegration", "Stopping network streaming");
    
    // Stop components
    if (m_streamingProcessor) {
        m_streamingProcessor->stopProcessing();
    }
    
    if (m_networkReceiver) {
        m_networkReceiver->stopListening();
    }
    
    // Update state
    m_networkActive = false;
    m_processingActive = false;
    
    // Update status
    {
        QMutexLocker locker(&m_statusMutex);
        m_status.network_connected = false;
        m_status.processing_active = false;
        m_status.status_message = "Network streaming stopped";
    }
    
    emit networkStreamingStopped();
    
    Logger::instance().log(Logger::Info, "EtiNetworkIntegration", "Network streaming stopped");
}

bool EtiNetworkIntegration::isNetworkStreamingActive() const {
    return m_networkActive.load();
}

void EtiNetworkIntegration::setIntegrationMode(IntegrationMode mode) {
    if (m_currentMode == mode) {
        return;
    }
    
    Logger::instance().log(Logger::Info, "EtiNetworkIntegration", 
                          QString("Switching integration mode from %1 to %2")
                          .arg(static_cast<int>(m_currentMode))
                          .arg(static_cast<int>(mode)));
    
    IntegrationMode oldMode = m_currentMode;
    handleModeSwitch(mode);
    m_currentMode = mode;
    
    emit integrationModeChanged(oldMode, mode);
}

IntegrationMode EtiNetworkIntegration::getIntegrationMode() const {
    return m_currentMode;
}

bool EtiNetworkIntegration::switchToLiveMode() {
    setIntegrationMode(IntegrationMode::LIVE_ONLY);
    return startNetworkStreaming();
}

bool EtiNetworkIntegration::switchToFileMode() {
    stopNetworkStreaming();
    setIntegrationMode(IntegrationMode::FILE_ONLY);
    return true;
}

bool EtiNetworkIntegration::enableDualMode() {
    setIntegrationMode(IntegrationMode::DUAL_MODE);
    return startNetworkStreaming();
}

void EtiNetworkIntegration::setPerformanceTargets(double fps, std::chrono::microseconds latency, size_t memoryMB) {
    QMutexLocker locker(&m_configMutex);
    
    m_config.target_fps = fps;
    m_config.target_latency = latency;
    m_config.memory_limit_mb = memoryMB;
    
    // Update components
    configureComponents();
    
    Logger::instance().log(Logger::Info, "EtiNetworkIntegration", 
                          QString("Performance targets updated: FPS=%1, Latency=%2μs, Memory=%3MB")
                          .arg(fps).arg(latency.count()).arg(memoryMB));
}

void EtiNetworkIntegration::enableProfessionalMode(bool enabled) {
    QMutexLocker locker(&m_configMutex);
    
    m_config.enable_professional_mode = enabled;
    
    if (m_networkReceiver) {
        m_networkReceiver->enableProfessionalMode(enabled);
    }
    
    Logger::instance().log(Logger::Info, "EtiNetworkIntegration", 
                          QString("Professional mode %1").arg(enabled ? "enabled" : "disabled"));
}

void EtiNetworkIntegration::enableRealTimeAnalysis(bool enabled) {
    QMutexLocker locker(&m_configMutex);
    
    m_config.enable_real_time_analysis = enabled;
    
    Logger::instance().log(Logger::Info, "EtiNetworkIntegration", 
                          QString("Real-time analysis %1").arg(enabled ? "enabled" : "disabled"));
}

void EtiNetworkIntegration::setProcessingMode(ProcessingMode mode) {
    QMutexLocker locker(&m_configMutex);
    
    m_config.processing_mode = mode;
    
    if (m_streamingProcessor) {
        m_streamingProcessor->setProcessingMode(mode);
    }
    
    Logger::instance().log(Logger::Info, "EtiNetworkIntegration", 
                          QString("Processing mode set to %1").arg(static_cast<int>(mode)));
}

void EtiNetworkIntegration::setQualityThresholds(double minSignalQuality, double maxPacketLoss, double maxJitter) {
    QMutexLocker locker(&m_configMutex);
    
    m_config.min_signal_quality = minSignalQuality;
    m_config.max_packet_loss = maxPacketLoss;
    m_config.max_jitter_ms = maxJitter;
    
    if (m_networkReceiver) {
        m_networkReceiver->setQualityThresholds(minSignalQuality, maxPacketLoss, maxJitter);
    }
    
    if (m_streamingProcessor) {
        m_streamingProcessor->setQualityThresholds(0.95, maxJitter, maxJitter);
    }
}

void EtiNetworkIntegration::enableRedundancy(bool enabled, const QString& backupAddress) {
    QMutexLocker locker(&m_configMutex);
    
    m_config.enable_redundancy = enabled;
    m_config.backup_address = backupAddress;
    
    if (m_networkReceiver && enabled && !backupAddress.isEmpty()) {
        m_networkReceiver->setRedundancyMode(RedundancyMode::DUAL_STREAM);
        m_networkReceiver->addBackupStream(backupAddress, m_config.port);
    }
}

void EtiNetworkIntegration::enableAdaptiveQuality(bool enabled) {
    QMutexLocker locker(&m_configMutex);
    
    if (m_networkReceiver) {
        m_networkReceiver->setAdaptiveBuffering(enabled);
    }
    
    if (m_streamingProcessor) {
        m_streamingProcessor->enableAdaptiveQuality(enabled);
    }
    
    if (m_bufferManager) {
        m_bufferManager->enableAdaptiveSizing(enabled);
    }
}

IntegrationStatus EtiNetworkIntegration::getStatus() const {
    QMutexLocker locker(&m_statusMutex);
    return m_status;
}

bool EtiNetworkIntegration::isHealthy() const {
    return m_healthy.load() && m_status.is_healthy;
}

QString EtiNetworkIntegration::getStatusMessage() const {
    QMutexLocker locker(&m_statusMutex);
    return m_status.status_message;
}

bool EtiNetworkIntegration::arePerformanceTargetsMet() const {
    QMutexLocker locker(&m_statusMutex);
    
    return m_status.current_fps >= m_config.target_fps &&
           m_status.current_latency <= m_config.target_latency &&
           m_status.memory_usage_mb <= m_config.memory_limit_mb;
}

void EtiNetworkIntegration::enableGuiIntegration(bool enabled) {
    m_guiIntegrationEnabled = enabled;
    
    if (enabled) {
        Logger::instance().log(Logger::Info, "EtiNetworkIntegration", "GUI integration enabled");
    } else {
        Logger::instance().log(Logger::Info, "EtiNetworkIntegration", "GUI integration disabled");
    }
}

void EtiNetworkIntegration::connectToMainWindow(QObject* mainWindow) {
    m_mainWindow = mainWindow;
    
    if (mainWindow) {
        Logger::instance().log(Logger::Info, "EtiNetworkIntegration", "Connected to main window");
    }
}

void EtiNetworkIntegration::updateGuiStatus() {
    if (m_guiIntegrationEnabled && m_mainWindow) {
        // Create GUI update data
        QVariantMap data;
        
        {
            QMutexLocker locker(&m_statusMutex);
            data["network_connected"] = m_status.network_connected;
            data["processing_active"] = m_status.processing_active;
            data["current_fps"] = m_status.current_fps;
            data["current_latency"] = static_cast<qint64>(m_status.current_latency.count());
            data["memory_usage"] = static_cast<double>(m_status.memory_usage_mb);
            data["signal_quality"] = m_status.signal_quality;
            data["packet_loss"] = m_status.packet_loss_rate;
            data["jitter"] = m_status.jitter_ms;
            data["frames_received"] = static_cast<qulonglong>(m_status.frames_received);
            data["frames_processed"] = static_cast<qulonglong>(m_status.frames_processed);
            data["is_healthy"] = m_status.is_healthy;
            data["status_message"] = m_status.status_message;
        }
        
        emit realTimeDataAvailable(data);
        emit guiUpdateRequired();
    }
}

void EtiNetworkIntegration::enableNetworkRecording(bool enabled, const QString& outputPath) {
    m_recordingEnabled = enabled;
    m_recordingPath = outputPath;
    
    Logger::instance().log(Logger::Info, "EtiNetworkIntegration", 
                          QString("Network recording %1").arg(enabled ? "enabled" : "disabled"));
}

void EtiNetworkIntegration::enableStreamComparison(bool enabled) {
    m_comparisonEnabled = enabled;
    
    Logger::instance().log(Logger::Info, "EtiNetworkIntegration", 
                          QString("Stream comparison %1").arg(enabled ? "enabled" : "disabled"));
}

void EtiNetworkIntegration::exportNetworkMetrics(const QString& filename) {
    // Implementation would export detailed metrics to file
    Logger::instance().log(Logger::Info, "EtiNetworkIntegration", 
                          QString("Network metrics export requested: %1").arg(filename));
}

void EtiNetworkIntegration::generateIntegrationReport() {
    // Implementation would generate comprehensive integration report
    Logger::instance().log(Logger::Info, "EtiNetworkIntegration", "Integration report generation requested");
}

void EtiNetworkIntegration::setFrameCallback(FrameCallback callback) {
    QMutexLocker locker(&m_callbackMutex);
    m_frameCallback = callback;
}

void EtiNetworkIntegration::setStatusCallback(StatusCallback callback) {
    QMutexLocker locker(&m_callbackMutex);
    m_statusCallback = callback;
}

void EtiNetworkIntegration::setErrorCallback(ErrorCallback callback) {
    QMutexLocker locker(&m_callbackMutex);
    m_errorCallback = callback;
}

// Private slot implementations

void EtiNetworkIntegration::handleNetworkFrame(const eti::EtiFrame& frame, const NetworkQualityMetrics& quality) {
    // Update statistics
    m_totalFramesReceived.fetch_add(1);
    m_lastFrameTime = std::chrono::steady_clock::now();
    
    // Process frame
    processNetworkFrame(frame, quality);
    
    // Forward to ETI processor if connected
    if (m_etiProcessor && (m_currentMode == IntegrationMode::LIVE_ONLY || 
                          m_currentMode == IntegrationMode::DUAL_MODE ||
                          m_currentMode == IntegrationMode::HYBRID_MODE)) {
        forwardToEtiProcessor(frame);
    }
    
    // Call frame callback
    {
        QMutexLocker locker(&m_callbackMutex);
        if (m_frameCallback) {
            m_frameCallback(frame, quality);
        }
    }
    
    // Emit signals
    emit liveFrameReceived(frame, quality);
    emit frameProcessed(frame, true);
}

void EtiNetworkIntegration::handleNetworkError(const QString& error, int severity) {
    Logger::instance().log(Logger::Error, "EtiNetworkIntegration", 
                          QString("Network error (severity %1): %2").arg(severity).arg(error));
    
    handleIntegrationError(error, severity);
    
    emit networkStreamingError(error);
    emit integrationError(error, severity);
    
    // Call error callback
    {
        QMutexLocker locker(&m_callbackMutex);
        if (m_errorCallback) {
            m_errorCallback(error, severity);
        }
    }
}

void EtiNetworkIntegration::handleNetworkQualityUpdate(const NetworkQualityMetrics& metrics) {
    // Update status with network quality
    {
        QMutexLocker locker(&m_statusMutex);
        m_status.signal_quality = metrics.signal_quality.load();
        m_status.packet_loss_rate = metrics.packet_loss_rate.load();
        m_status.jitter_ms = metrics.jitter_ms.load();
    }
    
    // Validate quality
    validateQualityMetrics(metrics);
}

void EtiNetworkIntegration::handleNetworkConnection(bool connected) {
    {
        QMutexLocker locker(&m_statusMutex);
        m_status.network_connected = connected;
        m_status.status_message = connected ? "Network connected" : "Network disconnected";
    }
    
    if (!connected) {
        Logger::instance().log(Logger::Warning, "EtiNetworkIntegration", "Network connection lost");
        performErrorRecovery();
    }
}

void EtiNetworkIntegration::handleProcessedFrame(const ProcessedFrame& frame) {
    m_totalFramesProcessed.fetch_add(1);
    
    // Update latency metrics
    {
        QMutexLocker locker(&m_statusMutex);
        m_status.current_latency = frame.get_total_latency();
        
        // Check latency target
        if (frame.get_total_latency() > m_config.target_latency) {
            emit latencyThresholdViolated(frame.get_total_latency(), m_config.target_latency);
        }
    }
}

void EtiNetworkIntegration::handleProcessingError(const QString& error, int severity) {
    Logger::instance().log(Logger::Error, "EtiNetworkIntegration", 
                          QString("Processing error (severity %1): %2").arg(severity).arg(error));
    
    handleIntegrationError(error, severity);
}

void EtiNetworkIntegration::handlePerformanceUpdate(const StreamingPerformance& performance) {
    // Update status with performance metrics
    {
        QMutexLocker locker(&m_statusMutex);
        m_status.current_fps = performance.current_fps.load();
        m_status.cpu_usage = performance.cpu_usage.load();
        m_status.memory_usage_mb = performance.memory_usage_mb.load();
        
        // Check performance targets
        if (performance.current_fps.load() < m_config.target_fps) {
            emit performanceTargetMissed("FPS", performance.current_fps.load(), m_config.target_fps);
        }
        
        if (performance.memory_usage_mb.load() > m_config.memory_limit_mb) {
            emit memoryLimitExceeded(static_cast<size_t>(performance.memory_usage_mb.load()), 
                                   m_config.memory_limit_mb);
        }
    }
}

void EtiNetworkIntegration::handleBufferOverflow(size_t droppedFrames) {
    m_totalFramesDropped.fetch_add(droppedFrames);
    
    Logger::instance().log(Logger::Warning, "EtiNetworkIntegration", 
                          QString("Buffer overflow: %1 frames dropped").arg(droppedFrames));
}

void EtiNetworkIntegration::handleBufferUnderflow() {
    Logger::instance().log(Logger::Warning, "EtiNetworkIntegration", "Buffer underflow detected");
}

void EtiNetworkIntegration::handleBufferMetrics(const BufferMetricsSnapshot& metrics) {
    // Update buffer utilization in status
    {
        QMutexLocker locker(&m_statusMutex);
        // Buffer metrics would be integrated into overall status
    }
}

void EtiNetworkIntegration::updateStatus() {
    updateIntegrationStatus();
    
    IntegrationStatus current_status = getStatus();
    emit statusUpdated(current_status);
    
    // Call status callback
    {
        QMutexLocker locker(&m_callbackMutex);
        if (m_statusCallback) {
            m_statusCallback(current_status);
        }
    }
    
    // Update GUI if enabled
    updateGuiStatus();
}

void EtiNetworkIntegration::monitorPerformance() {
    calculatePerformanceMetrics();
    
    // Check if performance targets are met
    if (!arePerformanceTargetsMet()) {
        optimizePerformance();
    }
}

void EtiNetworkIntegration::checkHealth() {
    updateHealthStatus();
    
    bool current_health = isHealthy();
    static bool last_health = true;
    
    if (current_health != last_health) {
        QString reason = current_health ? "Health restored" : "Health degraded";
        emit healthStatusChanged(current_health, reason);
        last_health = current_health;
    }
}

void EtiNetworkIntegration::performMaintenance() {
    // Perform periodic maintenance tasks
    generateDiagnostics();
    
    // Clean up old data if needed
    // Reset counters if they get too large
    static constexpr size_t COUNTER_RESET_THRESHOLD = 1000000;
    
    if (m_totalFramesReceived.load() > COUNTER_RESET_THRESHOLD) {
        m_totalFramesReceived = 0;
        m_totalFramesProcessed = 0;
        m_totalFramesDropped = 0;
    }
}

// Private helper methods

void EtiNetworkIntegration::createNetworkComponents() {
    if (!m_networkReceiver) {
        m_networkReceiver = std::make_unique<EtiOverIpReceiver>(this);
        
        // Connect signals
        connect(m_networkReceiver.get(), &EtiOverIpReceiver::etiFrameReceived,
                this, &EtiNetworkIntegration::handleNetworkFrame);
        connect(m_networkReceiver.get(), &EtiOverIpReceiver::networkError,
                this, &EtiNetworkIntegration::handleNetworkError);
        connect(m_networkReceiver.get(), &EtiOverIpReceiver::qualityMetricsUpdated,
                this, &EtiNetworkIntegration::handleNetworkQualityUpdate);
        connect(m_networkReceiver.get(), &EtiOverIpReceiver::connectionStatusChanged,
                this, &EtiNetworkIntegration::handleNetworkConnection);
    }
    
    if (!m_streamingProcessor) {
        StreamingConfig config;
        config.mode = m_config.processing_mode;
        config.target_latency = std::chrono::duration_cast<std::chrono::milliseconds>(m_config.target_latency);
        
        m_streamingProcessor = std::make_unique<StreamingProcessor>(config, this);
        
        // Connect signals
        connect(m_streamingProcessor.get(), &StreamingProcessor::frameProcessed,
                this, &EtiNetworkIntegration::handleProcessedFrame);
        connect(m_streamingProcessor.get(), &StreamingProcessor::processingError,
                this, &EtiNetworkIntegration::handleProcessingError);
        connect(m_streamingProcessor.get(), &StreamingProcessor::performanceUpdate,
                this, &EtiNetworkIntegration::handlePerformanceUpdate);
    }
    
    if (!m_bufferManager) {
        CircularBufferConfig config;
        config.memory_limit_mb = m_config.memory_limit_mb;
        config.target_latency = m_config.target_latency;
        config.target_throughput_fps = m_config.target_fps;
        
        m_bufferManager = std::make_unique<CircularBufferManager>(config, this);
        
        // Connect signals
        connect(m_bufferManager.get(), &CircularBufferManager::bufferOverflow,
                this, &EtiNetworkIntegration::handleBufferOverflow);
        connect(m_bufferManager.get(), &CircularBufferManager::bufferUnderflow,
                this, &EtiNetworkIntegration::handleBufferUnderflow);
        connect(m_bufferManager.get(), &CircularBufferManager::metricsUpdated,
                this, &EtiNetworkIntegration::handleBufferMetrics);
    }
}

void EtiNetworkIntegration::destroyNetworkComponents() {
    m_networkReceiver.reset();
    m_streamingProcessor.reset();
    m_bufferManager.reset();
}

void EtiNetworkIntegration::configureComponents() {
    if (m_networkReceiver) {
        m_networkReceiver->enableProfessionalMode(m_config.enable_professional_mode);
        m_networkReceiver->setQualityThresholds(m_config.min_signal_quality, 
                                               m_config.max_packet_loss, 
                                               m_config.max_jitter_ms);
        m_networkReceiver->setAdaptiveBuffering(m_config.adaptive_buffering);
    }
    
    if (m_streamingProcessor) {
        m_streamingProcessor->setProcessingMode(m_config.processing_mode);
        m_streamingProcessor->setTargetLatency(
            std::chrono::duration_cast<std::chrono::milliseconds>(m_config.target_latency));
    }
    
    if (m_bufferManager) {
        m_bufferManager->setMemoryLimit(m_config.memory_limit_mb);
        m_bufferManager->setTargetLatency(m_config.target_latency);
        m_bufferManager->setTargetThroughput(m_config.target_fps);
    }
}

void EtiNetworkIntegration::optimizeForMode(IntegrationMode mode) {
    switch (mode) {
        case IntegrationMode::LIVE_ONLY:
            // Optimize for real-time processing
            if (m_streamingProcessor) {
                m_streamingProcessor->setProcessingMode(ProcessingMode::LOW_LATENCY);
            }
            break;
            
        case IntegrationMode::DUAL_MODE:
            // Balance between file and live processing
            if (m_streamingProcessor) {
                m_streamingProcessor->setProcessingMode(ProcessingMode::BALANCED);
            }
            break;
            
        case IntegrationMode::FILE_ONLY:
            // No network optimization needed
            break;
            
        case IntegrationMode::HYBRID_MODE:
            // Adaptive optimization
            if (m_streamingProcessor) {
                m_streamingProcessor->enableAdaptiveQuality(true);
            }
            break;
    }
}

void EtiNetworkIntegration::processNetworkFrame(const eti::EtiFrame& frame, const NetworkQualityMetrics& quality) {
    // Buffer the frame if buffer manager is available
    if (m_bufferManager) {
        m_bufferManager->writeFrame(frame, quality);
    }
    
    // Process the frame if streaming processor is available
    if (m_streamingProcessor) {
        m_streamingProcessor->processFrame(frame, quality);
    }
}

void EtiNetworkIntegration::forwardToEtiProcessor(const eti::EtiFrame& frame) {
    if (m_etiProcessor) {
        // Convert EtiFrame to QByteArray for existing processor
        QByteArray frameData(reinterpret_cast<const char*>(frame.data()), frame.size());
        m_etiProcessor->processEtiFrame(frameData);
    }
}

void EtiNetworkIntegration::updateIntegrationStatus() {
    QMutexLocker locker(&m_statusMutex);
    
    // Update basic status
    m_status.frames_received = m_totalFramesReceived.load();
    m_status.frames_processed = m_totalFramesProcessed.load();
    m_status.frames_dropped = m_totalFramesDropped.load();
    m_status.last_update = std::chrono::steady_clock::now();
    
    // Update health status based on performance
    m_status.is_healthy = arePerformanceTargetsMet() && 
                         (m_status.frames_dropped < m_status.frames_received * 0.01); // <1% drop rate
}

void EtiNetworkIntegration::handleModeSwitch(IntegrationMode newMode) {
    Logger::instance().log(Logger::Info, "EtiNetworkIntegration", 
                          QString("Handling mode switch to %1").arg(static_cast<int>(newMode)));
    
    optimizeForMode(newMode);
    
    // Update configuration
    {
        QMutexLocker locker(&m_configMutex);
        m_config.mode = newMode;
    }
}

void EtiNetworkIntegration::optimizePerformance() {
    // Implement performance optimization strategies
    if (m_streamingProcessor) {
        // Enable adaptive quality if performance is below target
        m_streamingProcessor->enableAdaptiveQuality(true);
    }
    
    if (m_bufferManager) {
        // Optimize buffer settings
        m_bufferManager->optimizeMemoryUsage();
    }
}

void EtiNetworkIntegration::adaptToNetworkConditions() {
    // Implement network adaptation strategies based on quality metrics
    NetworkQualityMetrics quality;
    if (m_networkReceiver) {
        quality = m_networkReceiver->getQualityMetrics();
    }
    
    // Adapt buffer settings based on network conditions
    if (m_bufferManager && quality.jitter_ms.load() > m_config.max_jitter_ms) {
        // Increase buffer size to handle jitter
        size_t current_capacity = m_bufferManager->capacity();
        m_bufferManager->resize(std::min(current_capacity * 12 / 10, size_t{2000})); // 20% increase, max 2000
    }
}

void EtiNetworkIntegration::balanceLatencyThroughput() {
    // Implement latency-throughput balancing
    double current_fps = m_status.current_fps;
    auto current_latency = m_status.current_latency;
    
    if (current_latency > m_config.target_latency && current_fps >= m_config.target_fps) {
        // Prioritize latency
        if (m_streamingProcessor) {
            m_streamingProcessor->setProcessingMode(ProcessingMode::LOW_LATENCY);
        }
    } else if (current_fps < m_config.target_fps && current_latency <= m_config.target_latency) {
        // Prioritize throughput
        if (m_streamingProcessor) {
            m_streamingProcessor->setProcessingMode(ProcessingMode::BALANCED);
        }
    }
}

void EtiNetworkIntegration::handleResourceConstraints() {
    // Handle resource constraints by reducing quality if needed
    if (m_status.memory_usage_mb > m_config.memory_limit_mb) {
        if (m_bufferManager) {
            m_bufferManager->optimizeMemoryUsage();
        }
    }
    
    if (m_status.cpu_usage > 0.8) {
        if (m_streamingProcessor) {
            m_streamingProcessor->enableFrameDropping(true);
        }
    }
}

void EtiNetworkIntegration::validateQualityMetrics(const NetworkQualityMetrics& metrics) {
    bool quality_ok = true;
    QString issues;
    
    if (metrics.signal_quality.load() < m_config.min_signal_quality) {
        quality_ok = false;
        issues += QString("Signal quality %1 below threshold %2; ")
                 .arg(metrics.signal_quality.load()).arg(m_config.min_signal_quality);
    }
    
    if (metrics.packet_loss_rate.load() > m_config.max_packet_loss) {
        quality_ok = false;
        issues += QString("Packet loss %1% above threshold %2%; ")
                 .arg(metrics.packet_loss_rate.load() * 100).arg(m_config.max_packet_loss * 100);
    }
    
    if (metrics.jitter_ms.load() > m_config.max_jitter_ms) {
        quality_ok = false;
        issues += QString("Jitter %1ms above threshold %2ms; ")
                 .arg(metrics.jitter_ms.load()).arg(m_config.max_jitter_ms);
    }
    
    if (!quality_ok) {
        emit qualityDegradation(issues, metrics.signal_quality.load());
        handleQualityDegradation();
    }
}

void EtiNetworkIntegration::handleQualityDegradation() {
    Logger::instance().log(Logger::Warning, "EtiNetworkIntegration", "Quality degradation detected");
    
    // Apply quality recovery measures
    applyQualityAdaptations();
}

void EtiNetworkIntegration::applyQualityAdaptations() {
    // Enable adaptive buffering
    if (m_networkReceiver) {
        m_networkReceiver->setAdaptiveBuffering(true);
    }
    
    // Increase buffer sizes temporarily
    adaptToNetworkConditions();
}

void EtiNetworkIntegration::handleIntegrationError(const QString& error, int severity) {
    static size_t consecutive_errors = 0;
    consecutive_errors++;
    
    if (consecutive_errors >= MAX_CONSECUTIVE_ERRORS) {
        Logger::instance().log(Logger::Error, "EtiNetworkIntegration", "Too many consecutive errors, performing recovery");
        performErrorRecovery();
        consecutive_errors = 0;
    }
    
    // Update health status
    if (severity >= 3) {
        m_healthy = false;
        
        QMutexLocker locker(&m_statusMutex);
        m_status.is_healthy = false;
        m_status.status_message = QString("Error: %1").arg(error);
    }
}

void EtiNetworkIntegration::performErrorRecovery() {
    Logger::instance().log(Logger::Info, "EtiNetworkIntegration", "Performing error recovery");
    
    // Reset components
    if (m_networkActive.load()) {
        stopNetworkStreaming();
        QTimer::singleShot(1000, this, [this]() {
            startNetworkStreaming();
        });
    }
    
    // Reset buffers
    if (m_bufferManager) {
        m_bufferManager->reset();
    }
    
    // Reset health status
    m_healthy = true;
    
    {
        QMutexLocker locker(&m_statusMutex);
        m_status.is_healthy = true;
        m_status.status_message = "Recovery in progress";
    }
}

void EtiNetworkIntegration::resetIntegrationState() {
    // Reset all counters and state
    m_totalFramesReceived = 0;
    m_totalFramesProcessed = 0;
    m_totalFramesDropped = 0;
    
    m_healthy = true;
    
    {
        QMutexLocker locker(&m_statusMutex);
        m_status = IntegrationStatus();
        m_status.last_update = std::chrono::steady_clock::now();
    }
}

void EtiNetworkIntegration::updateGuiComponents() {
    if (m_guiIntegrationEnabled) {
        updateGuiStatus();
    }
}

void EtiNetworkIntegration::sendGuiUpdates() {
    updateGuiComponents();
}

void EtiNetworkIntegration::handleGuiRequests() {
    // Handle GUI-specific requests
}

void EtiNetworkIntegration::calculatePerformanceMetrics() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_startTime);
    
    if (elapsed.count() > 0) {
        QMutexLocker locker(&m_statusMutex);
        
        // Calculate FPS
        m_status.current_fps = static_cast<double>(m_totalFramesProcessed.load()) / elapsed.count();
    }
}

void EtiNetworkIntegration::updateHealthStatus() {
    bool healthy = arePerformanceTargetsMet() && 
                  m_status.signal_quality > m_config.min_signal_quality * HEALTH_SCORE_THRESHOLD;
    
    m_healthy = healthy;
    
    {
        QMutexLocker locker(&m_statusMutex);
        m_status.is_healthy = healthy;
    }
}

void EtiNetworkIntegration::generateDiagnostics() {
    // Generate diagnostic information for debugging
    Logger::instance().log(Logger::Debug, "EtiNetworkIntegration", 
                          QString("Diagnostics: FPS=%1, Latency=%2μs, Memory=%3MB, Health=%4")
                          .arg(m_status.current_fps)
                          .arg(m_status.current_latency.count())
                          .arg(m_status.memory_usage_mb)
                          .arg(m_status.is_healthy ? "Good" : "Poor"));
}

// =============================================================================
// EtiNetworkIntegrationFactory Implementation
// =============================================================================

std::unique_ptr<EtiNetworkIntegration> EtiNetworkIntegrationFactory::createBroadcastIntegration(EtiProcessor* processor) {
    auto integration = std::make_unique<EtiNetworkIntegration>(processor);
    
    NetworkStreamingConfig config;
    config.target_fps = 1000.0;
    config.target_latency = std::chrono::microseconds{20000}; // 20ms for broadcast
    config.memory_limit_mb = 30;
    config.enable_professional_mode = true;
    config.enable_etsi_validation = true;
    config.min_signal_quality = 0.98;
    config.max_packet_loss = 0.0001;
    config.max_jitter_ms = 2.0;
    config.processing_mode = ProcessingMode::HIGH_QUALITY;
    
    integration->setConfiguration(config);
    integration->enableProfessionalMode(true);
    integration->enableRealTimeAnalysis(true);
    
    return integration;
}

std::unique_ptr<EtiNetworkIntegration> EtiNetworkIntegrationFactory::createLowLatencyIntegration(EtiProcessor* processor) {
    auto integration = std::make_unique<EtiNetworkIntegration>(processor);
    
    NetworkStreamingConfig config;
    config.target_fps = 900.0;
    config.target_latency = std::chrono::microseconds{17000}; // <17ms requirement
    config.memory_limit_mb = 25;
    config.processing_mode = ProcessingMode::LOW_LATENCY;
    config.buffer_size = 100; // Small buffer for low latency
    config.adaptive_buffering = false; // Disable for consistent latency
    
    integration->setConfiguration(config);
    
    return integration;
}

std::unique_ptr<EtiNetworkIntegration> EtiNetworkIntegrationFactory::createHighThroughputIntegration(EtiProcessor* processor) {
    auto integration = std::make_unique<EtiNetworkIntegration>(processor);
    
    NetworkStreamingConfig config;
    config.target_fps = 1500.0; // High throughput
    config.target_latency = std::chrono::microseconds{50000}; // 50ms acceptable
    config.memory_limit_mb = 30; // Use full allocation
    config.processing_mode = ProcessingMode::BALANCED;
    config.buffer_size = 2000; // Large buffer for throughput
    config.adaptive_buffering = true;
    
    integration->setConfiguration(config);
    
    return integration;
}

std::unique_ptr<EtiNetworkIntegration> EtiNetworkIntegrationFactory::createDevelopmentIntegration(EtiProcessor* processor) {
    auto integration = std::make_unique<EtiNetworkIntegration>(processor);
    
    NetworkStreamingConfig config;
    config.target_fps = 500.0; // Moderate for development
    config.target_latency = std::chrono::microseconds{100000}; // 100ms for development
    config.memory_limit_mb = 50; // More generous for development
    config.enable_professional_mode = false;
    config.processing_mode = ProcessingMode::BALANCED;
    
    integration->setConfiguration(config);
    integration->enableGuiIntegration(true);
    
    return integration;
}

std::unique_ptr<EtiNetworkIntegration> EtiNetworkIntegrationFactory::createCustomIntegration(
    EtiProcessor* processor, const NetworkStreamingConfig& config) {
    
    auto integration = std::make_unique<EtiNetworkIntegration>(processor);
    integration->setConfiguration(config);
    
    return integration;
}

} // namespace eti_network