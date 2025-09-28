/**
 * @file ui_integration_bridge.cpp
 * @brief Implementation of UI Integration Bridge
 * 
 * @author StreamDAB Development Team
 * @date 2025
 * @copyright Copyright (c) 2025 StreamDAB Technologies
 */

#include "ui_integration_bridge.h"

#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <algorithm>

namespace streamdab::integration {

UIIntegrationBridge::UIIntegrationBridge(QObject* parent)
    : QObject(parent)
    , real_time_timer_(new QTimer(this))
{
    // Setup real-time update timer
    real_time_timer_->setSingleShot(false);
    real_time_timer_->setInterval(DEFAULT_UPDATE_INTERVAL);
    connect(real_time_timer_, &QTimer::timeout, this, &UIIntegrationBridge::onRealTimeUpdate);
    
    // Initialize timestamps
    metrics_last_update_ = std::chrono::system_clock::now();
    discovery_last_update_ = std::chrono::system_clock::now();
    connection_start_time_ = std::chrono::system_clock::now();
    last_error_time_ = std::chrono::system_clock::now();
    
    // Initialize status
    updateIntegrationStatus(IntegrationStatus::UNINITIALIZED);
}

UIIntegrationBridge::~UIIntegrationBridge() {
    if (real_time_timer_->isActive()) {
        real_time_timer_->stop();
    }
    
    // Clean up connections
    if (initialized_) {
        stopStreaming();
    }
}

bool UIIntegrationBridge::initialize(std::shared_ptr<eti::controller::ModernETIController> eti_controller,
                                    std::shared_ptr<eti::streaming::StreamingEngine> streaming_engine,
                                    std::shared_ptr<eti::resilience::StreamResilience> stream_resilience,
                                    std::shared_ptr<eti::compliance::ComprehensiveETSIValidator> etsi_validator) {
    if (!eti_controller || !streaming_engine) {
        handleIntegrationError("ETI Controller and Streaming Engine are required");
        return false;
    }
    
    // Validate backend compatibility
    if (!integration_utils::validateBackendCompatibility(eti_controller, streaming_engine)) {
        handleIntegrationError("Backend components are not compatible");
        return false;
    }
    
    updateIntegrationStatus(IntegrationStatus::INITIALIZING);
    
    // Store backend components
    eti_controller_ = eti_controller;
    streaming_engine_ = streaming_engine;
    stream_resilience_ = stream_resilience;
    etsi_validator_ = etsi_validator;
    
    // Setup backend signal connections
    setupBackendConnections();
    
    // Initialize with default values
    current_metrics_ = RealTimeMetrics{};
    discovery_data_ = ServiceDiscoveryData{};
    
    initialized_.store(true);
    updateIntegrationStatus(IntegrationStatus::CONNECTED);
    
    // Start real-time updates if enabled
    if (real_time_enabled_) {
        real_time_timer_->start();
    }
    
    qDebug() << "UI Integration Bridge initialized successfully";
    return true;
}

bool UIIntegrationBridge::connectUIComponents(ui::StreamingDashboard* dashboard,
                                             ui::ConnectionConfigDialog* config_dialog,
                                             config::ConfigurationManager* config_manager,
                                             ServiceBrowser* service_browser,
                                             EtiServiceTreeModel* service_tree_model) {
    if (!initialized_) {
        handleIntegrationError("Integration bridge must be initialized before connecting UI components");
        return false;
    }
    
    // Store UI component references
    dashboard_ = dashboard;
    config_dialog_ = config_dialog;
    config_manager_ = config_manager;
    service_browser_ = service_browser;
    service_tree_model_ = service_tree_model;
    
    // Setup UI signal connections
    setupUIConnections();
    
    // Initialize UI components with current backend state
    propagateDataToUI();
    
    qDebug() << "UI components connected successfully";
    return true;
}

RealTimeMetrics UIIntegrationBridge::getCurrentMetrics() const {
    QMutexLocker locker(&metrics_mutex_);
    return current_metrics_;
}

ServiceDiscoveryData UIIntegrationBridge::getServiceDiscoveryData() const {
    QMutexLocker locker(&discovery_mutex_);
    return discovery_data_;
}

void UIIntegrationBridge::setRealTimeUpdatesEnabled(bool enabled, int update_interval) {
    real_time_enabled_ = enabled;
    update_interval_ = update_interval;
    
    if (enabled && initialized_) {
        real_time_timer_->setInterval(update_interval);
        real_time_timer_->start();
    } else {
        real_time_timer_->stop();
    }
}

bool UIIntegrationBridge::startStreaming(const ui::ConnectionConfig& config) {
    if (!initialized_) {
        handleIntegrationError("Integration bridge not initialized");
        return false;
    }
    
    if (!validateConnectionConfig(config)) {
        handleIntegrationError("Invalid connection configuration");
        return false;
    }
    
    // Store configuration
    last_connection_config_ = config;
    current_endpoint_ = config.full_url;
    connection_start_time_ = std::chrono::system_clock::now();
    
    try {
        // Convert UI config to backend config
        auto backend_config = config.toProcessingConfig();
        
        // Update ETI controller configuration
        if (!eti_controller_->updateConfiguration(backend_config)) {
            handleIntegrationError("Failed to update ETI controller configuration");
            return false;
        }
        
        // Start ETI controller processing
        if (!eti_controller_->startProcessing()) {
            handleIntegrationError("Failed to start ETI processing");
            return false;
        }
        
        // Connect to stream
        if (!eti_controller_->connectToStream(config.full_url)) {
            handleIntegrationError("Failed to connect to stream");
            eti_controller_->stopProcessing();
            return false;
        }
        
        // Configure streaming engine
        eti::streaming::QualityThresholds quality_thresholds;
        quality_thresholds.min_signal_quality = config.min_signal_quality;
        quality_thresholds.min_etsi_compliance = 85.0; // Default ETSI compliance threshold
        quality_thresholds.max_latency = std::chrono::milliseconds{50};
        quality_thresholds.max_error_rate = 5.0;
        quality_thresholds.max_consecutive_errors = 10;
        
        streaming_engine_->setQualityThresholds(quality_thresholds);
        streaming_engine_->configureBuffering(config.buffer_frames);
        
        // Start streaming engine
        if (!streaming_engine_->startLiveStreaming(config.full_url)) {
            handleIntegrationError("Failed to start streaming engine");
            eti_controller_->stopProcessing();
            eti_controller_->disconnectFromStream();
            return false;
        }
        
        updateIntegrationStatus(IntegrationStatus::STREAMING);
        emit streamingStateChanged(true, current_endpoint_);
        
        qDebug() << "Streaming started successfully on" << config.full_url;
        return true;
        
    } catch (const std::exception& e) {
        handleIntegrationError(QString("Exception while starting streaming: %1").arg(e.what()));
        return false;
    }
}

void UIIntegrationBridge::stopStreaming() {
    if (!initialized_) {
        return;
    }
    
    try {
        // Stop streaming engine
        if (streaming_engine_) {
            streaming_engine_->stopStreaming();
        }
        
        // Stop ETI controller
        if (eti_controller_) {
            eti_controller_->stopProcessing();
            eti_controller_->disconnectFromStream();
        }
        
        updateIntegrationStatus(IntegrationStatus::CONNECTED);
        emit streamingStateChanged(false, current_endpoint_);
        
        qDebug() << "Streaming stopped successfully";
        
    } catch (const std::exception& e) {
        handleIntegrationError(QString("Exception while stopping streaming: %1").arg(e.what()));
    }
}

void UIIntegrationBridge::reconnectStreaming() {
    if (!initialized_ || last_connection_config_.full_url.isEmpty()) {
        handleIntegrationError("Cannot reconnect: no previous connection configuration");
        return;
    }
    
    qDebug() << "Attempting to reconnect to" << last_connection_config_.full_url;
    
    // Stop current streaming
    stopStreaming();
    
    // Wait a moment before reconnecting
    QTimer::singleShot(2000, this, [this]() {
        if (!startStreaming(last_connection_config_)) {
            handleIntegrationError("Reconnection failed");
        }
    });
}

void UIIntegrationBridge::onConnectionConfigChanged(const ui::ConnectionConfig& config) {
    last_connection_config_ = config;
    
    // Update backend configuration if currently streaming
    if (current_status_ == IntegrationStatus::STREAMING) {
        auto backend_config = config.toProcessingConfig();
        if (eti_controller_) {
            eti_controller_->updateConfiguration(backend_config);
        }
        
        if (streaming_engine_) {
            eti::streaming::QualityThresholds thresholds;
            thresholds.min_signal_quality = config.min_signal_quality;
            thresholds.min_etsi_compliance = 85.0;
            thresholds.max_latency = std::chrono::milliseconds{50};
            thresholds.max_error_rate = 5.0;
            thresholds.max_consecutive_errors = 10;
            
            streaming_engine_->setQualityThresholds(thresholds);
            streaming_engine_->configureBuffering(config.buffer_frames);
        }
    }
}

void UIIntegrationBridge::onStreamingStartRequested(const QString& endpoint) {
    ui::ConnectionConfig config = last_connection_config_;
    config.full_url = endpoint;
    config.updateFullURL();
    
    startStreaming(config);
}

void UIIntegrationBridge::onStreamingStopRequested() {
    stopStreaming();
}

void UIIntegrationBridge::onReconnectionRequested() {
    reconnectStreaming();
}

void UIIntegrationBridge::onConfigurationSaved() {
    if (config_manager_) {
        // Update stored configurations with current values
        auto app_config = config_manager_->getApplicationConfig();
        auto streaming_config = config_manager_->getStreamingConfig();
        streaming_config.connection = last_connection_config_;
        config_manager_->setStreamingConfig(streaming_config);
    }
}

void UIIntegrationBridge::onConfigurationLoaded() {
    if (config_manager_) {
        // Load configuration and update connection config
        auto streaming_config = config_manager_->getStreamingConfig();
        last_connection_config_ = streaming_config.connection;
        
        // Update UI components if connected
        if (config_dialog_) {
            config_dialog_->setConfiguration(last_connection_config_);
        }
    }
}

void UIIntegrationBridge::onServiceSelectionChanged(uint32_t service_id) {
    Q_UNUSED(service_id)
    
    // Update service tree model with selection
    if (service_tree_model_) {
        // Implementation would depend on service tree model interface
        qDebug() << "Service selection changed to ID:" << service_id;
    }
}

void UIIntegrationBridge::refreshServiceDiscovery() {
    if (!initialized_) {
        return;
    }
    
    updateServiceDiscoveryData();
    propagateDataToUI();
}

// Backend signal handlers
void UIIntegrationBridge::onETIControllerConnectionStatusChanged(const eti::network::ConnectionStatus& status) {
    {
        QMutexLocker locker(&metrics_mutex_);
        current_metrics_.is_connected = status.isConnected();
        current_metrics_.endpoint_url = current_endpoint_;
    }
    
    if (status.isConnected()) {
        updateIntegrationStatus(IntegrationStatus::CONNECTED);
    } else {
        updateIntegrationStatus(IntegrationStatus::DISCONNECTED);
    }
    
    aggregateRealTimeMetrics();
}

void UIIntegrationBridge::onETIControllerStatisticsUpdated(const eti::controller::ProcessingStatistics& stats) {
    {
        QMutexLocker locker(&metrics_mutex_);
        current_metrics_.current_fps = stats.current_fps;
        current_metrics_.total_frames = stats.total_frames_processed;
        current_metrics_.frames_with_errors = stats.frames_with_errors;
        current_metrics_.avg_processing_time = stats.avg_processing_latency;
        current_metrics_.signal_quality = stats.overall_signal_quality;
        current_metrics_.etsi_compliance = stats.etsi_compliance_score;
        
        if (stats.connection_status.isConnected()) {
            auto now = std::chrono::system_clock::now();
            current_metrics_.connection_uptime = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - connection_start_time_);
        }
    }
    
    aggregateRealTimeMetrics();
}

void UIIntegrationBridge::onETIControllerEnsembleDiscovered(const eti::Ensemble& ensemble) {
    {
        QMutexLocker locker(&discovery_mutex_);
        discovery_data_.ensemble_label = QString::fromStdString(ensemble.label);
        discovery_data_.ensemble_id = ensemble.id;
        discovery_data_.country_code = QString::fromStdString(ensemble.country_code);
        discovery_data_.discovery_time = QDateTime::currentDateTime();
    }
    
    emit ensembleUpdated(discovery_data_.ensemble_label, discovery_data_.ensemble_id);
    updateServiceDiscoveryData();
}

void UIIntegrationBridge::onETIControllerServiceDiscovered(const eti::DabService& service) {
    ServiceDiscoveryData::ServiceInfo service_info = convertServiceInfo(service);
    
    {
        QMutexLocker locker(&discovery_mutex_);
        
        // Check if service already exists
        bool found = false;
        for (auto& existing_service : discovery_data_.services) {
            if (existing_service.service_id == service_info.service_id) {
                existing_service = service_info; // Update existing
                found = true;
                break;
            }
        }
        
        if (!found) {
            discovery_data_.services.append(service_info);
        }
        
        discovery_data_.total_services = static_cast<uint32_t>(discovery_data_.services.size());
        
        // Update discovered services list
        discovery_data_.discovered_services.clear();
        for (const auto& svc : discovery_data_.services) {
            discovery_data_.discovered_services.append(svc.service_label);
        }
    }
    
    emit serviceDiscovered(service_info);
    updateServiceDiscoveryData();
}

void UIIntegrationBridge::onETIControllerETSIComplianceIssue(const QString& standard, 
                                                            const QString& issue, 
                                                            const QString& severity) {
    emit etsiComplianceIssue(standard, issue, severity);
    
    // Log the issue
    QString error_message = QString("ETSI Compliance Issue [%1]: %2 - %3").arg(severity, standard, issue);
    handleIntegrationError(error_message);
}

void UIIntegrationBridge::onStreamingEngineStateChanged(bool streaming) {
    {
        QMutexLocker locker(&metrics_mutex_);
        current_metrics_.is_streaming = streaming;
    }
    
    if (streaming) {
        updateIntegrationStatus(IntegrationStatus::STREAMING);
    } else if (current_metrics_.is_connected) {
        updateIntegrationStatus(IntegrationStatus::CONNECTED);
    } else {
        updateIntegrationStatus(IntegrationStatus::DISCONNECTED);
    }
    
    emit streamingStateChanged(streaming, current_endpoint_);
}

void UIIntegrationBridge::onStreamingEngineFrameProcessed(uint32_t frame_number, 
                                                         const eti::EtiFrame& frame,
                                                         std::chrono::microseconds processing_time) {
    Q_UNUSED(frame_number)
    Q_UNUSED(frame)
    
    {
        QMutexLocker locker(&metrics_mutex_);
        current_metrics_.total_frames++;
        current_metrics_.avg_processing_time = processing_time;
    }
    
    // Update metrics without triggering full aggregation for performance
    metrics_last_update_ = std::chrono::system_clock::now();
}

void UIIntegrationBridge::onStreamingEngineQualityLevelChanged(eti::streaming::QualityLevel level,
                                                              const eti::streaming::QualityMetrics& metrics) {
    {
        QMutexLocker locker(&metrics_mutex_);
        current_metrics_.signal_quality = metrics.getOverallQuality();
        current_metrics_.etsi_compliance = metrics.etsi_compliance_score;
    }
    
    aggregateRealTimeMetrics();
}

void UIIntegrationBridge::onStreamingEngineQualityAlert(eti::streaming::QualityLevel level, const QString& message) {
    ui::QualityIndicator ui_level = convertQualityLevel(level);
    emit qualityAlert(ui_level, message);
    
    // Log quality alert as potential error
    if (level == eti::streaming::QualityLevel::CRITICAL || level == eti::streaming::QualityLevel::POOR) {
        handleIntegrationError(QString("Quality Alert [%1]: %2")
                             .arg(static_cast<int>(level))
                             .arg(message));
    }
}

void UIIntegrationBridge::onStreamingEngineBufferStatusChanged(const eti::streaming::BufferStatus& status) {
    {
        QMutexLocker locker(&metrics_mutex_);
        current_metrics_.buffer_utilization = static_cast<uint32_t>(status.utilization_percent);
        current_metrics_.buffer_latency = status.avg_latency;
    }
    
    aggregateRealTimeMetrics();
}

void UIIntegrationBridge::onStreamResilienceConnectionRestored(const QString& endpoint) {
    qDebug() << "Stream resilience: connection restored to" << endpoint;
    updateIntegrationStatus(IntegrationStatus::STREAMING);
}

void UIIntegrationBridge::onStreamResilienceConnectionLost(const QString& reason) {
    qDebug() << "Stream resilience: connection lost -" << reason;
    updateIntegrationStatus(IntegrationStatus::ERROR);
    handleIntegrationError(QString("Connection lost: %1").arg(reason));
}

void UIIntegrationBridge::onStreamResilienceReconnectionAttempt(int attempt_number, const QString& endpoint) {
    qDebug() << "Stream resilience: reconnection attempt" << attempt_number << "to" << endpoint;
    updateIntegrationStatus(IntegrationStatus::INITIALIZING);
}

void UIIntegrationBridge::onETSIValidatorComplianceResult(const eti::compliance::ComprehensiveComplianceResult& result) {
    if (!result.is_compliant) {
        QString error_message = QString("ETSI Compliance Failure: %1").arg(QString::fromStdString(result.summary));
        emit etsiComplianceIssue("ETSI EN 300 799", error_message, "ERROR");
    }
}

void UIIntegrationBridge::onRealTimeUpdate() {
    if (!initialized_) {
        return;
    }
    
    // Aggregate current metrics from all backend components
    aggregateRealTimeMetrics();
    
    // Update service discovery data
    updateServiceDiscoveryData();
    
    // Propagate data to UI components
    propagateDataToUI();
    
    // Emit real-time metrics update
    emit realTimeMetricsUpdated(getCurrentMetrics());
}

void UIIntegrationBridge::setupBackendConnections() {
    if (!eti_controller_) {
        return;
    }
    
    // ETI Controller connections
    connect(eti_controller_.get(), &eti::controller::ModernETIController::connectionStatusChanged,
            this, &UIIntegrationBridge::onETIControllerConnectionStatusChanged);
    connect(eti_controller_.get(), &eti::controller::ModernETIController::statisticsUpdated,
            this, &UIIntegrationBridge::onETIControllerStatisticsUpdated);
    connect(eti_controller_.get(), &eti::controller::ModernETIController::ensembleDiscovered,
            this, &UIIntegrationBridge::onETIControllerEnsembleDiscovered);
    connect(eti_controller_.get(), &eti::controller::ModernETIController::serviceDiscovered,
            this, &UIIntegrationBridge::onETIControllerServiceDiscovered);
    connect(eti_controller_.get(), &eti::controller::ModernETIController::etsiComplianceIssue,
            this, &UIIntegrationBridge::onETIControllerETSIComplianceIssue);
    
    // Streaming Engine connections
    if (streaming_engine_) {
        connect(streaming_engine_.get(), &eti::streaming::StreamingEngine::streamingStateChanged,
                this, &UIIntegrationBridge::onStreamingEngineStateChanged);
        connect(streaming_engine_.get(), &eti::streaming::StreamingEngine::frameProcessed,
                this, &UIIntegrationBridge::onStreamingEngineFrameProcessed);
        connect(streaming_engine_.get(), &eti::streaming::StreamingEngine::qualityLevelChanged,
                this, &UIIntegrationBridge::onStreamingEngineQualityLevelChanged);
        connect(streaming_engine_.get(), &eti::streaming::StreamingEngine::qualityAlert,
                this, &UIIntegrationBridge::onStreamingEngineQualityAlert);
        connect(streaming_engine_.get(), &eti::streaming::StreamingEngine::bufferStatusChanged,
                this, &UIIntegrationBridge::onStreamingEngineBufferStatusChanged);
    }
    
    // Stream Resilience connections
    if (stream_resilience_) {
        connect(stream_resilience_.get(), &eti::resilience::StreamResilience::connectionRestored,
                this, &UIIntegrationBridge::onStreamResilienceConnectionRestored);
        connect(stream_resilience_.get(), &eti::resilience::StreamResilience::connectionLost,
                this, &UIIntegrationBridge::onStreamResilienceConnectionLost);
        connect(stream_resilience_.get(), &eti::resilience::StreamResilience::reconnectionAttempt,
                this, &UIIntegrationBridge::onStreamResilienceReconnectionAttempt);
    }
    
    // ETSI Validator connections
    if (etsi_validator_) {
        connect(etsi_validator_.get(), &eti::compliance::ComprehensiveETSIValidator::complianceResult,
                this, &UIIntegrationBridge::onETSIValidatorComplianceResult);
    }
}

void UIIntegrationBridge::setupUIConnections() {
    // Streaming Dashboard connections
    if (dashboard_) {
        connect(dashboard_, &ui::StreamingDashboard::streamingStartRequested,
                this, &UIIntegrationBridge::onStreamingStartRequested);
        connect(dashboard_, &ui::StreamingDashboard::streamingStopRequested,
                this, &UIIntegrationBridge::onStreamingStopRequested);
        connect(dashboard_, &ui::StreamingDashboard::reconnectionRequested,
                this, &UIIntegrationBridge::onReconnectionRequested);
        
        // Initialize dashboard with backend components
        dashboard_->initialize(streaming_engine_, eti_controller_);
    }
    
    // Connection Config Dialog connections
    if (config_dialog_) {
        connect(config_dialog_, &ui::ConnectionConfigDialog::configurationAccepted,
                this, &UIIntegrationBridge::onConnectionConfigChanged);
    }
    
    // Configuration Manager connections
    if (config_manager_) {
        connect(config_manager_, &config::ConfigurationManager::configurationSaved,
                this, &UIIntegrationBridge::onConfigurationSaved);
        connect(config_manager_, &config::ConfigurationManager::configurationLoaded,
                this, &UIIntegrationBridge::onConfigurationLoaded);
    }
    
    // Service Browser connections
    if (service_browser_) {
        // Connect service selection signals
        // Note: This would depend on the actual ServiceBrowser interface
        // connect(service_browser_, &ServiceBrowser::serviceSelectionChanged,
        //         this, &UIIntegrationBridge::onServiceSelectionChanged);
    }
}

void UIIntegrationBridge::updateIntegrationStatus(IntegrationStatus new_status) {
    IntegrationStatus old_status;
    {
        QMutexLocker locker(&status_mutex_);
        old_status = current_status_;
        current_status_ = new_status;
    }
    
    if (old_status != new_status) {
        emit integrationStatusChanged(new_status);
        qDebug() << "Integration status changed from" 
                 << integration_utils::integrationStatusToString(old_status)
                 << "to" << integration_utils::integrationStatusToString(new_status);
    }
}

void UIIntegrationBridge::aggregateRealTimeMetrics() {
    QMutexLocker locker(&metrics_mutex_);
    
    // Update timestamp
    metrics_last_update_ = std::chrono::system_clock::now();
    
    // Calculate FPS from frame count if controller is available
    if (eti_controller_) {
        auto stats = eti_controller_->getStatistics();
        current_metrics_.current_fps = stats.current_fps;
        current_metrics_.signal_quality = stats.overall_signal_quality;
        current_metrics_.etsi_compliance = stats.etsi_compliance_score;
    }
    
    // Get buffer status from streaming engine
    if (streaming_engine_) {
        auto buffer_status = streaming_engine_->getBufferStatus();
        current_metrics_.buffer_utilization = static_cast<uint32_t>(buffer_status.utilization_percent);
        current_metrics_.buffer_latency = buffer_status.avg_latency;
    }
    
    // Update connection uptime
    if (current_metrics_.is_connected) {
        auto now = std::chrono::system_clock::now();
        current_metrics_.connection_uptime = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - connection_start_time_);
    } else {
        current_metrics_.connection_uptime = std::chrono::milliseconds{0};
    }
}

void UIIntegrationBridge::updateServiceDiscoveryData() {
    QMutexLocker locker(&discovery_mutex_);
    
    discovery_last_update_ = std::chrono::system_clock::now();
    
    // Emit discovery completion if we have valid data
    if (discovery_data_.isValid()) {
        emit serviceDiscoveryCompleted(discovery_data_);
    }
}

void UIIntegrationBridge::propagateDataToUI() {
    // Update dashboard with current metrics
    if (dashboard_) {
        auto metrics = getCurrentMetrics();
        
        // Convert to dashboard-specific format
        ui::ConnectionStatus conn_status = ui::ConnectionStatus::DISCONNECTED;
        if (metrics.is_streaming) {
            conn_status = ui::ConnectionStatus::STREAMING;
        } else if (metrics.is_connected) {
            conn_status = ui::ConnectionStatus::CONNECTED;
        }
        
        dashboard_->updateConnectionStatus(conn_status);
        dashboard_->updateConnectionInfo(metrics.endpoint_url);
        
        // Update quality metrics
        eti::streaming::QualityMetrics quality_metrics;
        quality_metrics.signal_quality = metrics.signal_quality;
        quality_metrics.etsi_compliance_score = metrics.etsi_compliance;
        quality_metrics.total_frames_processed = metrics.total_frames;
        quality_metrics.frames_with_errors = metrics.frames_with_errors;
        quality_metrics.current_error_rate = metrics.total_frames > 0 ? 
            (static_cast<double>(metrics.frames_with_errors) / metrics.total_frames) * 100.0 : 0.0;
        
        dashboard_->updateQualityMetrics(quality_metrics);
        
        // Update buffer status
        eti::streaming::BufferStatus buffer_status;
        buffer_status.utilization_percent = static_cast<double>(metrics.buffer_utilization);
        buffer_status.avg_latency = metrics.buffer_latency;
        
        dashboard_->updateBufferStatus(buffer_status);
    }
    
    // Update service tree model with discovered services
    if (service_tree_model_) {
        auto discovery = getServiceDiscoveryData();
        // Implementation would depend on service tree model interface
        // service_tree_model_->updateServices(discovery.services);
    }
}

ui::QualityIndicator UIIntegrationBridge::convertQualityLevel(eti::streaming::QualityLevel level) {
    switch (level) {
        case eti::streaming::QualityLevel::EXCELLENT:
            return ui::QualityIndicator::EXCELLENT;
        case eti::streaming::QualityLevel::GOOD:
            return ui::QualityIndicator::GOOD;
        case eti::streaming::QualityLevel::ACCEPTABLE:
            return ui::QualityIndicator::ACCEPTABLE;
        case eti::streaming::QualityLevel::POOR:
            return ui::QualityIndicator::POOR;
        case eti::streaming::QualityLevel::CRITICAL:
            return ui::QualityIndicator::CRITICAL;
        case eti::streaming::QualityLevel::UNKNOWN:
        default:
            return ui::QualityIndicator::UNKNOWN;
    }
}

ServiceDiscoveryData::ServiceInfo UIIntegrationBridge::convertServiceInfo(const eti::DabService& service) {
    ServiceDiscoveryData::ServiceInfo info;
    info.service_id = service.id;
    info.service_label = QString::fromStdString(service.label);
    info.service_type = service.is_audio ? "Audio" : "Data";
    info.bitrate = service.bitrate;
    info.signal_quality = 95.0; // Default quality, would be calculated from actual signal
    info.is_dab_plus = service.is_dab_plus;
    info.codec_info = service.is_dab_plus ? "DAB+" : "DAB";
    
    return info;
}

bool UIIntegrationBridge::validateConnectionConfig(const ui::ConnectionConfig& config) {
    auto validation = ui::connection_utils::validateConfig(config);
    return validation.valid;
}

void UIIntegrationBridge::handleIntegrationError(const QString& error) {
    // Add to recent errors list
    recent_errors_.prepend(QString("[%1] %2")
                          .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                          .arg(error));
    
    // Limit error history
    while (recent_errors_.size() > MAX_RECENT_ERRORS) {
        recent_errors_.removeLast();
    }
    
    last_error_time_ = std::chrono::system_clock::now();
    
    // Update status if appropriate
    if (current_status_ != IntegrationStatus::ERROR && 
        (error.contains("failed", Qt::CaseInsensitive) || 
         error.contains("error", Qt::CaseInsensitive))) {
        updateIntegrationStatus(IntegrationStatus::ERROR);
    }
    
    // Emit error signal
    emit integrationError(error);
    
    // Log error
    qWarning() << "Integration error:" << error;
}

// Utility functions implementation
namespace integration_utils {

QString integrationStatusToString(IntegrationStatus status) {
    switch (status) {
        case IntegrationStatus::UNINITIALIZED:
            return "UNINITIALIZED";
        case IntegrationStatus::INITIALIZING:
            return "INITIALIZING";
        case IntegrationStatus::CONNECTED:
            return "CONNECTED";
        case IntegrationStatus::STREAMING:
            return "STREAMING";
        case IntegrationStatus::ERROR:
            return "ERROR";
        case IntegrationStatus::DISCONNECTED:
            return "DISCONNECTED";
        default:
            return "UNKNOWN";
    }
}

bool isStreamingStatus(IntegrationStatus status) {
    return status == IntegrationStatus::STREAMING;
}

bool isErrorStatus(IntegrationStatus status) {
    return status == IntegrationStatus::ERROR;
}

QColor getStatusColor(IntegrationStatus status) {
    switch (status) {
        case IntegrationStatus::STREAMING:
            return QColor(0, 180, 0); // Green
        case IntegrationStatus::CONNECTED:
            return QColor(100, 200, 0); // Light Green
        case IntegrationStatus::INITIALIZING:
            return QColor(255, 165, 0); // Orange
        case IntegrationStatus::ERROR:
            return QColor(220, 20, 20); // Red
        case IntegrationStatus::DISCONNECTED:
        case IntegrationStatus::UNINITIALIZED:
        default:
            return QColor(128, 128, 128); // Gray
    }
}

bool validateBackendCompatibility(std::shared_ptr<eti::controller::ModernETIController> eti_controller,
                                 std::shared_ptr<eti::streaming::StreamingEngine> streaming_engine) {
    // Basic validation - ensure both components are initialized
    if (!eti_controller || !streaming_engine) {
        return false;
    }
    
    // Check if components are properly initialized
    if (!eti_controller->isInitialized() || !streaming_engine->isInitialized()) {
        return false;
    }
    
    // Additional compatibility checks could be added here
    return true;
}

IntegrationConfig createDefaultIntegrationConfig() {
    IntegrationConfig config;
    config.enable_real_time_updates = true;
    config.update_interval_ms = 500;
    config.enable_service_discovery = true;
    config.enable_quality_monitoring = true;
    config.enable_etsi_validation = true;
    config.max_error_history = 10;
    
    return config;
}

} // namespace integration_utils

} // namespace streamdab::integration