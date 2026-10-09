/**
 * @file stream_resilience.cpp
 * @brief Stream Resilience and Connection Health Monitoring Implementation
 * 
 * Advanced implementation providing <1s reconnection times, exponential
 * backoff, and comprehensive health monitoring for broadcast applications.
 * 
 * @author StreamDAB Development Team
 * @date 2025
 * @copyright Copyright (c) 2025 StreamDAB Technologies
 */

#include "stream_resilience.hpp"
#include "../utils/logger.h"
#include <algorithm>
#include <cmath>
#include <QUrl>
#include <QHostInfo>
#include <QNetworkInterface>

namespace eti::resilience {

// ============================================================================
// StreamResilience Implementation
// ============================================================================

StreamResilience::StreamResilience(QObject* parent)
    : QObject(parent)
    , initialized_(false)
    , monitoring_(false)
    , reconnection_in_progress_(false)
    , current_attempt_(0)
    , consecutive_failures_(0)
    , last_health_status_(HealthStatus::OFFLINE)
    , health_checks_enabled_(true)
    , adaptive_strategy_enabled_(true) {
    
    // Initialize with default broadcast strategy
    strategy_ = utils::createBroadcastStrategy();
    
    // Initialize network manager
    network_manager_ = new QNetworkAccessManager(this);
    
    // Initialize timestamps
    start_time_ = std::chrono::system_clock::now();
    last_successful_connection_ = start_time_;
    
    Logger::instance().log(Logger::Info, "StreamResilience", 
        "Stream resilience manager created with broadcast strategy");
}

StreamResilience::~StreamResilience() {
    if (monitoring_.load()) {
        stopMonitoring();
    }
    
    cleanupMonitoringTimers();
    
    Logger::instance().log(Logger::Info, "StreamResilience", 
        QString("Stream resilience destroyed - %1 reconnection attempts made")
        .arg(health_metrics_.reconnection_attempts));
}

bool StreamResilience::initialize(
    std::shared_ptr<eti::network::ZeroMQETIClient> zmq_client,
    std::shared_ptr<eti::streaming::StreamingEngine> streaming_engine) {
    
    Logger::instance().log(Logger::Info, "StreamResilience", "Initializing stream resilience");
    
    if (!zmq_client) {
        Logger::instance().log(Logger::Error, "StreamResilience", 
            "Invalid parameters - ZMQ client required");
        return false;
    }
    
    zmq_client_ = zmq_client;
    streaming_engine_ = streaming_engine;
    
    // Setup monitoring timers
    setupMonitoringTimers();
    
    // Connect signals
    connectSignals();
    
    // Initialize health metrics
    {
        QMutexLocker locker(&metrics_mutex_);
        health_metrics_ = ConnectionHealthMetrics{};
        health_metrics_.current_health = HealthStatus::OFFLINE;
    }
    
    initialized_ = true;
    
    Logger::instance().log(Logger::Info, "StreamResilience", 
        QString("Stream resilience initialized - streaming engine: %1")
        .arg(streaming_engine_ ? "connected" : "not connected"));
    
    return true;
}

bool StreamResilience::startMonitoring() {
    Logger::instance().log(Logger::Info, "StreamResilience", "Starting connection health monitoring");
    
    if (!initialized_.load()) {
        Logger::instance().log(Logger::Error, "StreamResilience", 
            "Cannot start monitoring - not initialized");
        return false;
    }
    
    if (monitoring_.load()) {
        Logger::instance().log(Logger::Warning, "StreamResilience", 
            "Monitoring already active");
        return true;
    }
    
    // Start monitoring timers
    if (health_monitor_timer_) {
        health_monitor_timer_->start(strategy_.health_check_interval.count());
    }
    
    if (endpoint_health_timer_ && health_checks_enabled_.load()) {
        endpoint_health_timer_->start(30000); // Check endpoints every 30 seconds
    }
    
    monitoring_ = true;
    start_time_ = std::chrono::system_clock::now();
    
    Logger::instance().log(Logger::Info, "StreamResilience", 
        QString("Health monitoring started - check interval: %1ms")
        .arg(strategy_.health_check_interval.count()));
    
    return true;
}

void StreamResilience::stopMonitoring() {
    Logger::instance().log(Logger::Info, "StreamResilience", "Stopping health monitoring");
    
    if (!monitoring_.load()) {
        return;
    }
    
    monitoring_ = false;
    
    // Stop all timers
    cleanupMonitoringTimers();
    
    // Cancel any ongoing reconnection
    reconnection_in_progress_ = false;
    
    Logger::instance().log(Logger::Info, "StreamResilience", "Health monitoring stopped");
}

void StreamResilience::setReconnectionStrategy(const ReconnectionStrategy& strategy) {
    if (!strategy.isValid()) {
        Logger::instance().log(Logger::Warning, "StreamResilience", 
            "Invalid reconnection strategy provided");
        return;
    }
    
    QMutexLocker locker(&config_mutex_);
    strategy_ = strategy;
    
    // Update health check interval if monitoring is active
    if (monitoring_.load() && health_monitor_timer_) {
        health_monitor_timer_->setInterval(strategy.health_check_interval.count());
    }
    
    Logger::instance().log(Logger::Info, "StreamResilience", 
        QString("Reconnection strategy updated - max attempts: %1, initial delay: %2ms")
        .arg(strategy.max_reconnect_attempts)
        .arg(strategy.initial_delay.count()));
}

void StreamResilience::enableAutoReconnect(bool enabled, int max_retries) {
    QMutexLocker locker(&config_mutex_);
    strategy_.auto_reconnect_enabled = enabled;
    strategy_.max_reconnect_attempts = max_retries;
    
    Logger::instance().log(Logger::Info, "StreamResilience", 
        QString("Auto-reconnect %1 - max retries: %2")
        .arg(enabled ? "enabled" : "disabled")
        .arg(max_retries));
}

void StreamResilience::addFallbackEndpoint(const EndpointConfig& config) {
    QMutexLocker locker(&config_mutex_);
    
    // Check if endpoint already exists
    auto it = std::find_if(fallback_endpoints_.begin(), fallback_endpoints_.end(),
        [&config](const EndpointConfig& existing) {
            return existing.url == config.url;
        });
    
    if (it != fallback_endpoints_.end()) {
        // Update existing endpoint
        *it = config;
        Logger::instance().log(Logger::Info, "StreamResilience", 
            QString("Updated fallback endpoint: %1").arg(config.url));
    } else {
        // Add new endpoint
        fallback_endpoints_.push_back(config);
        Logger::instance().log(Logger::Info, "StreamResilience", 
            QString("Added fallback endpoint: %1 (priority: %2)")
            .arg(config.url).arg(config.priority));
    }
    
    // Sort endpoints by priority
    std::sort(fallback_endpoints_.begin(), fallback_endpoints_.end(),
        [](const EndpointConfig& a, const EndpointConfig& b) {
            return a.priority < b.priority; // Lower number = higher priority
        });
}

void StreamResilience::removeFallbackEndpoint(const QString& url) {
    QMutexLocker locker(&config_mutex_);
    
    auto it = std::find_if(fallback_endpoints_.begin(), fallback_endpoints_.end(),
        [&url](const EndpointConfig& config) {
            return config.url == url;
        });
    
    if (it != fallback_endpoints_.end()) {
        fallback_endpoints_.erase(it);
        Logger::instance().log(Logger::Info, "StreamResilience", 
            QString("Removed fallback endpoint: %1").arg(url));
    } else {
        Logger::instance().log(Logger::Warning, "StreamResilience", 
            QString("Fallback endpoint not found: %1").arg(url));
    }
}

std::vector<EndpointConfig> StreamResilience::getFallbackEndpoints() const {
    QMutexLocker locker(&config_mutex_);
    return fallback_endpoints_;
}

void StreamResilience::clearFallbackEndpoints() {
    QMutexLocker locker(&config_mutex_);
    size_t count = fallback_endpoints_.size();
    fallback_endpoints_.clear();
    
    Logger::instance().log(Logger::Info, "StreamResilience", 
        QString("Cleared %1 fallback endpoints").arg(count));
}

void StreamResilience::handleConnectionLoss() {
    Logger::instance().log(Logger::Warning, "StreamResilience", "Connection loss detected");
    
    // Record connection loss time
    connection_lost_time_ = std::chrono::system_clock::now();
    consecutive_failures_++;
    
    // Update health metrics
    {
        QMutexLocker locker(&metrics_mutex_);
        health_metrics_.is_connected = false;
        health_metrics_.last_failure = connection_lost_time_;
        health_metrics_.failed_connections++;
        health_metrics_.consecutive_failures = consecutive_failures_.load();
        health_metrics_.current_health = HealthStatus::OFFLINE;
    }
    
    // Trigger automatic reconnection if enabled
    if (strategy_.auto_reconnect_enabled && !reconnection_in_progress_.load()) {
        Logger::instance().log(Logger::Info, "StreamResilience", 
            "Initiating automatic reconnection");
        attemptReconnection(0);
    }
    
    updateHealthMetrics();
}

bool StreamResilience::attemptReconnection(int retry_count) {
    if (reconnection_in_progress_.load()) {
        Logger::instance().log(Logger::Debug, "StreamResilience", 
            "Reconnection already in progress");
        return false;
    }
    
    if (retry_count >= strategy_.max_reconnect_attempts) {
        Logger::instance().log(Logger::Error, "StreamResilience", 
            QString("Maximum reconnection attempts reached: %1").arg(retry_count));
        
        emit reconnectionExhausted(retry_count, last_error_);
        return false;
    }
    
    reconnection_in_progress_ = true;
    current_attempt_ = retry_count;
    
    // Calculate delay for this attempt
    auto delay = strategy_.calculateDelay(retry_count);
    
    Logger::instance().log(Logger::Info, "StreamResilience", 
        QString("Scheduling reconnection attempt %1/%2 in %3ms")
        .arg(retry_count + 1)
        .arg(strategy_.max_reconnect_attempts)
        .arg(delay.count()));
    
    emit reconnecting(retry_count + 1, strategy_.max_reconnect_attempts, delay);
    
    // Schedule reconnection attempt
    scheduleReconnection(delay);
    
    return true;
}

bool StreamResilience::forceReconnection() {
    Logger::instance().log(Logger::Info, "StreamResilience", "Force reconnection requested");
    
    // Stop any ongoing reconnection
    reconnection_in_progress_ = false;
    if (reconnection_timer_) {
        reconnection_timer_->stop();
    }
    
    // Reset attempt counter
    current_attempt_ = 0;
    consecutive_failures_ = 0;
    
    // Immediately attempt reconnection
    return attemptReconnection(0);
}

ConnectionHealthMetrics StreamResilience::getHealthMetrics() const {
    QMutexLocker locker(&metrics_mutex_);
    return health_metrics_;
}

HealthStatus StreamResilience::getCurrentHealthStatus() const {
    QMutexLocker locker(&metrics_mutex_);
    return health_metrics_.current_health;
}

void StreamResilience::resetStatistics() {
    Logger::instance().log(Logger::Info, "StreamResilience", "Resetting health statistics");
    
    QMutexLocker locker(&metrics_mutex_);
    
    // Reset counters
    health_metrics_.total_connection_attempts = 0;
    health_metrics_.successful_connections = 0;
    health_metrics_.failed_connections = 0;
    health_metrics_.reconnection_attempts = 0;
    health_metrics_.consecutive_failures = 0;
    consecutive_failures_ = 0;
    
    // Reset timestamps
    start_time_ = std::chrono::system_clock::now();
    last_successful_connection_ = start_time_;
    
    // Clear tracking data
    recent_latencies_.clear();
    recent_connection_attempts_.clear();
    
    // Reset endpoint statistics
    QMutexLocker config_locker(&config_mutex_);
    for (auto& endpoint : fallback_endpoints_) {
        endpoint.connection_attempts = 0;
        endpoint.successful_connections = 0;
        endpoint.currently_healthy = true;
    }
}

void StreamResilience::setHealthChecksEnabled(bool enabled) {
    health_checks_enabled_ = enabled;
    
    if (enabled && monitoring_.load() && endpoint_health_timer_) {
        endpoint_health_timer_->start(30000);
    } else if (!enabled && endpoint_health_timer_) {
        endpoint_health_timer_->stop();
    }
    
    Logger::instance().log(Logger::Info, "StreamResilience", 
        QString("Health checks %1").arg(enabled ? "enabled" : "disabled"));
}

bool StreamResilience::testEndpointConnectivity(const QString& endpoint_url) {
    Logger::instance().log(Logger::Debug, "StreamResilience", 
        QString("Testing connectivity to endpoint: %1").arg(endpoint_url));
    
    if (!zmq_client_) {
        return false;
    }
    
    // Use ZMQ utility function to test connectivity
    return eti::network::zmq_utils::testEndpointConnectivity(endpoint_url, 5000);
}

std::vector<StreamResilience::RecommendedAction> StreamResilience::getRecommendedActions() const {
    return generateRecommendations();
}

// ============================================================================
// Signal/Slot Connection Methods
// ============================================================================

void StreamResilience::connectSignals() {
    if (!zmq_client_) return;
    
    connect(zmq_client_.get(), &eti::network::ZeroMQETIClient::connectionStatusChanged,
            this, &StreamResilience::handleZMQConnectionStatus);
    
    if (streaming_engine_) {
        connect(streaming_engine_.get(), &eti::streaming::StreamingEngine::qualityLevelChanged,
                this, &StreamResilience::handleStreamQualityUpdate);
    }
    
    Logger::instance().log(Logger::Debug, "StreamResilience", "Signals connected");
}

void StreamResilience::disconnectSignals() {
    if (zmq_client_) {
        disconnect(zmq_client_.get(), nullptr, this, nullptr);
    }
    
    if (streaming_engine_) {
        disconnect(streaming_engine_.get(), nullptr, this, nullptr);
    }
    
    Logger::instance().log(Logger::Debug, "StreamResilience", "Signals disconnected");
}

void StreamResilience::setupMonitoringTimers() {
    // Health monitoring timer
    health_monitor_timer_ = new QTimer(this);
    connect(health_monitor_timer_, &QTimer::timeout, this, &StreamResilience::handleHealthMonitoring);
    
    // Reconnection timer
    reconnection_timer_ = new QTimer(this);
    reconnection_timer_->setSingleShot(true);
    connect(reconnection_timer_, &QTimer::timeout, this, &StreamResilience::handleReconnectionTimer);
    
    // Endpoint health check timer
    endpoint_health_timer_ = new QTimer(this);
    connect(endpoint_health_timer_, &QTimer::timeout, this, &StreamResilience::handleEndpointHealthCheck);
    
    Logger::instance().log(Logger::Debug, "StreamResilience", "Monitoring timers setup");
}

void StreamResilience::cleanupMonitoringTimers() {
    if (health_monitor_timer_) {
        health_monitor_timer_->stop();
    }
    
    if (reconnection_timer_) {
        reconnection_timer_->stop();
    }
    
    if (endpoint_health_timer_) {
        endpoint_health_timer_->stop();
    }
    
    Logger::instance().log(Logger::Debug, "StreamResilience", "Monitoring timers cleaned up");
}

// ============================================================================
// Connection Management Methods
// ============================================================================

bool StreamResilience::attemptConnectionToEndpoint(const QString& endpoint_url) {
    Logger::instance().log(Logger::Info, "StreamResilience", 
        QString("Attempting connection to endpoint: %1").arg(endpoint_url));
    
    if (!zmq_client_) {
        return false;
    }
    
    last_connection_attempt_ = std::chrono::system_clock::now();
    
    // Update statistics
    updateEndpointStatistics(endpoint_url, false); // Mark attempt
    
    {
        QMutexLocker locker(&metrics_mutex_);
        health_metrics_.total_connection_attempts++;
    }
    
    // Parse endpoint configuration
    auto config = eti::network::zmq_utils::parseEndpointUrl(endpoint_url);
    
    try {
        // Attempt connection
        if (zmq_client_->connectToStream(config)) {
            // Connection successful
            current_endpoint_ = endpoint_url;
            last_successful_connection_ = std::chrono::system_clock::now();
            consecutive_failures_ = 0;
            
            // Update statistics
            updateEndpointStatistics(endpoint_url, true);
            
            {
                QMutexLocker locker(&metrics_mutex_);
                health_metrics_.is_connected = true;
                health_metrics_.successful_connections++;
                health_metrics_.last_successful_connection = last_successful_connection_;
                health_metrics_.consecutive_failures = 0;
                health_metrics_.current_health = HealthStatus::GOOD;
            }
            
            Logger::instance().log(Logger::Info, "StreamResilience", 
                QString("Successfully connected to endpoint: %1").arg(endpoint_url));
            
            return true;
        } else {
            // Connection failed
            last_error_ = QString("Failed to connect to %1").arg(endpoint_url);
            consecutive_failures_++;
            
            {
                QMutexLocker locker(&metrics_mutex_);
                health_metrics_.is_connected = false;
                health_metrics_.failed_connections++;
                health_metrics_.consecutive_failures = consecutive_failures_.load();
                health_metrics_.last_failure = last_connection_attempt_;
            }
            
            Logger::instance().log(Logger::Warning, "StreamResilience", 
                QString("Failed to connect to endpoint: %1").arg(endpoint_url));
            
            return false;
        }
        
    } catch (const std::exception& e) {
        last_error_ = QString("Connection exception: %1").arg(e.what());
        consecutive_failures_++;
        
        Logger::instance().log(Logger::Error, "StreamResilience", 
            QString("Connection exception for %1: %2").arg(endpoint_url).arg(e.what()));
        
        return false;
    }
}

EndpointConfig* StreamResilience::findBestAvailableEndpoint() {
    QMutexLocker locker(&config_mutex_);
    
    if (fallback_endpoints_.empty()) {
        return nullptr;
    }
    
    // Find the highest priority available endpoint
    for (auto& endpoint : fallback_endpoints_) {
        if (endpoint.isUsable()) {
            Logger::instance().log(Logger::Debug, "StreamResilience", 
                QString("Selected endpoint: %1 (priority: %2)")
                .arg(endpoint.url).arg(endpoint.priority));
            return &endpoint;
        }
    }
    
    // If no endpoint is marked as healthy, try the first enabled one
    for (auto& endpoint : fallback_endpoints_) {
        if (endpoint.enabled) {
            Logger::instance().log(Logger::Warning, "StreamResilience", 
                QString("Using potentially unhealthy endpoint: %1").arg(endpoint.url));
            return &endpoint;
        }
    }
    
    return nullptr;
}

void StreamResilience::updateEndpointStatistics(const QString& endpoint_url, bool success) {
    QMutexLocker locker(&config_mutex_);
    
    auto it = std::find_if(fallback_endpoints_.begin(), fallback_endpoints_.end(),
        [&endpoint_url](const EndpointConfig& config) {
            return config.url == endpoint_url;
        });
    
    if (it != fallback_endpoints_.end()) {
        it->connection_attempts++;
        it->last_attempt = last_connection_attempt_;
        
        if (success) {
            it->successful_connections++;
            it->last_success = last_connection_attempt_;
            it->currently_healthy = true;
        } else {
            // Don't immediately mark as unhealthy on single failure
            // Wait for health check to confirm
        }
        
        Logger::instance().log(Logger::Debug, "StreamResilience", 
            QString("Updated endpoint stats for %1: %2/%3 successful")
            .arg(endpoint_url)
            .arg(it->successful_connections)
            .arg(it->connection_attempts));
    }
}

bool StreamResilience::isEndpointHealthy(const QString& endpoint_url) {
    QMutexLocker locker(&config_mutex_);
    
    auto it = std::find_if(fallback_endpoints_.begin(), fallback_endpoints_.end(),
        [&endpoint_url](const EndpointConfig& config) {
            return config.url == endpoint_url;
        });
    
    if (it != fallback_endpoints_.end()) {
        return it->currently_healthy;
    }
    
    // If endpoint not in fallback list, assume healthy for now
    return true;
}

// ============================================================================
// Health Monitoring Methods
// ============================================================================

void StreamResilience::updateHealthMetrics() {
    QMutexLocker locker(&metrics_mutex_);
    
    // Calculate uptime and downtime
    health_metrics_.uptime = calculateUptime();
    health_metrics_.downtime = calculateDowntime();
    
    // Calculate connection stability
    calculateConnectionStability();
    
    // Update latency metrics
    if (!recent_latencies_.empty()) {
        auto total_latency = std::chrono::milliseconds{0};
        auto max_latency = std::chrono::milliseconds{0};
        
        for (const auto& latency : recent_latencies_) {
            total_latency += latency;
            max_latency = std::max(max_latency, latency);
        }
        
        health_metrics_.avg_latency = total_latency / recent_latencies_.size();
        health_metrics_.max_latency = max_latency;
    }
    
    // Calculate health score
    health_metrics_.health_score = assessConnectionQuality();
    
    // Update health status
    auto new_status = health_metrics_.calculateHealthStatus();
    if (new_status != health_metrics_.current_health) {
        auto old_status = health_metrics_.current_health;
        health_metrics_.current_health = new_status;
        
        Logger::instance().log(Logger::Info, "StreamResilience", 
            QString("Health status changed: %1 -> %2")
            .arg(utils::healthStatusToString(old_status))
            .arg(utils::healthStatusToString(new_status)));
        
        emit healthStatusChanged(new_status, health_metrics_);
    }
    
    // Check for quality degradation
    detectQualityDegradation();
    
    // Update impact assessment
    health_metrics_.affecting_stream_quality = isConnectionAffectingStreamQuality();
    health_metrics_.active_issues = identifyActiveIssues();
}

void StreamResilience::calculateConnectionStability() {
    // Calculate stability based on recent connection attempts
    if (recent_connection_attempts_.empty()) {
        health_metrics_.connection_stability = 100.0;
        return;
    }
    
    int successful_attempts = std::count(recent_connection_attempts_.begin(),
                                       recent_connection_attempts_.end(), true);
    
    health_metrics_.connection_stability = 
        (static_cast<double>(successful_attempts) / recent_connection_attempts_.size()) * 100.0;
}

void StreamResilience::detectQualityDegradation() {
    auto current_health = health_metrics_.current_health;
    
    // Trigger alerts for significant quality degradation
    if (current_health <= HealthStatus::POOR && last_health_status_ > HealthStatus::POOR) {
        auto recommendations = generateRecommendations();
        emit connectionQualityAlert(current_health, health_metrics_, recommendations);
        
        Logger::instance().log(Logger::Warning, "StreamResilience", 
            QString("Connection quality degraded to %1")
            .arg(utils::healthStatusToString(current_health)));
    }
    
    last_health_status_ = current_health;
}

void StreamResilience::performEndpointHealthChecks() {
    if (!health_checks_enabled_.load()) {
        return;
    }
    
    QMutexLocker locker(&config_mutex_);
    
    for (const auto& endpoint : fallback_endpoints_) {
        if (endpoint.health_check_enabled && !endpoint.health_check_url.isEmpty()) {
            startHTTPHealthCheck(endpoint);
        } else {
            // Perform basic connectivity test
            bool healthy = testEndpointConnectivity(endpoint.url);
            
            // Update endpoint health status
            auto& mutable_endpoint = const_cast<EndpointConfig&>(endpoint);
            mutable_endpoint.currently_healthy = healthy;
            
            emit endpointHealthCheck(endpoint.url, healthy, std::chrono::milliseconds{0});
        }
    }
}

void StreamResilience::startHTTPHealthCheck(const EndpointConfig& endpoint) {
    QNetworkRequest request(QUrl(endpoint.health_check_url));
    request.setRawHeader("User-Agent", "StreamDAB-Analyser/1.0");
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    
    QNetworkReply* reply = network_manager_->get(request);
    reply->ignoreSslErrors();
    
    // Set timeout
    QTimer::singleShot(endpoint.health_check_timeout.count(), reply, [reply]() {
        if (reply->isRunning()) {
            reply->abort();
        }
    });
    
    // Store pending health check
    pending_health_checks_[reply] = endpoint.url;
    
    connect(reply, &QNetworkReply::finished, this, &StreamResilience::handleHealthCheckResponse);
    
    Logger::instance().log(Logger::Debug, "StreamResilience", 
        QString("Started HTTP health check for %1: %2")
        .arg(endpoint.url).arg(endpoint.health_check_url));
}

// ============================================================================
// Reconnection Logic Methods
// ============================================================================

void StreamResilience::scheduleReconnection(std::chrono::milliseconds delay) {
    if (reconnection_timer_) {
        reconnection_timer_->start(delay.count());
        
        Logger::instance().log(Logger::Debug, "StreamResilience", 
            QString("Reconnection scheduled in %1ms").arg(delay.count()));
    }
}

void StreamResilience::executeReconnectionAttempt() {
    if (!reconnection_in_progress_.load()) {
        return;
    }
    
    int attempt = current_attempt_.load();
    
    Logger::instance().log(Logger::Info, "StreamResilience", 
        QString("Executing reconnection attempt %1/%2")
        .arg(attempt + 1).arg(strategy_.max_reconnect_attempts));
    
    // Try current endpoint first if available
    bool success = false;
    if (!current_endpoint_.isEmpty()) {
        success = attemptConnectionToEndpoint(current_endpoint_);
    }
    
    // If current endpoint failed, try fallback endpoints
    if (!success) {
        auto* best_endpoint = findBestAvailableEndpoint();
        if (best_endpoint) {
            QString old_endpoint = current_endpoint_;
            success = attemptConnectionToEndpoint(best_endpoint->url);
            
            if (success && old_endpoint != best_endpoint->url) {
                emit endpointSwitch(old_endpoint, best_endpoint->url, "Failover during reconnection");
            }
        }
    }
    
    if (success) {
        // Reconnection successful
        reconnection_in_progress_ = false;
        
        auto downtime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - connection_lost_time_);
        
        emit connectionRestored(current_endpoint_, attempt + 1, downtime);
        
        Logger::instance().log(Logger::Info, "StreamResilience", 
            QString("Connection restored to %1 after %2 attempts (%3ms downtime)")
            .arg(current_endpoint_).arg(attempt + 1).arg(downtime.count()));
        
        // Adapt strategy if needed
        if (shouldAdaptStrategy()) {
            adaptReconnectionStrategy();
        }
        
    } else {
        // Reconnection failed
        emit reconnectionFailed(attempt + 1, last_error_, 
                               attempt + 1 < strategy_.max_reconnect_attempts);
        
        Logger::instance().log(Logger::Warning, "StreamResilience", 
            QString("Reconnection attempt %1 failed: %2")
            .arg(attempt + 1).arg(last_error_));
        
        // Try next attempt if under limit
        if (attempt + 1 < strategy_.max_reconnect_attempts) {
            attemptReconnection(attempt + 1);
        } else {
            // All attempts exhausted
            reconnection_in_progress_ = false;
            emit reconnectionExhausted(attempt + 1, last_error_);
            
            Logger::instance().log(Logger::Error, "StreamResilience", 
                QString("All %1 reconnection attempts exhausted").arg(attempt + 1));
        }
    }
    
    updateHealthMetrics();
}

bool StreamResilience::shouldAdaptStrategy() {
    return adaptive_strategy_enabled_.load() && 
           consecutive_failures_.load() >= strategy_.consecutive_failures_for_adaptation;
}

void StreamResilience::adaptReconnectionStrategy() {
    QMutexLocker locker(&config_mutex_);
    
    // Increase max attempts if consistently failing
    if (consecutive_failures_.load() > 5) {
        strategy_.max_reconnect_attempts = std::min(strategy_.max_reconnect_attempts + 2, 20);
    }
    
    // Reduce initial delay for faster recovery
    if (health_metrics_.getAvailability() < 95.0) {
        strategy_.initial_delay = std::max(strategy_.initial_delay / 2, std::chrono::milliseconds{500});
    }
    
    Logger::instance().log(Logger::Info, "StreamResilience", 
        QString("Adapted reconnection strategy - max attempts: %1, initial delay: %2ms")
        .arg(strategy_.max_reconnect_attempts)
        .arg(strategy_.initial_delay.count()));
}

// ============================================================================
// Quality Assessment Methods
// ============================================================================

double StreamResilience::assessConnectionQuality() {
    double quality_score = 100.0;
    
    // Factor in connection stability
    quality_score *= (health_metrics_.connection_stability / 100.0);
    
    // Factor in success rate
    double success_rate = health_metrics_.getSuccessRate();
    quality_score *= (success_rate / 100.0);
    
    // Factor in availability
    double availability = health_metrics_.getAvailability();
    quality_score *= (availability / 100.0);
    
    // Penalize for consecutive failures
    if (health_metrics_.consecutive_failures > 0) {
        quality_score *= std::max(0.1, 1.0 - (health_metrics_.consecutive_failures * 0.1));
    }
    
    // Penalize for high latency
    if (health_metrics_.avg_latency > std::chrono::milliseconds{100}) {
        quality_score *= 0.8;
    }
    
    return std::max(0.0, std::min(100.0, quality_score));
}

bool StreamResilience::isConnectionAffectingStreamQuality() {
    if (!streaming_engine_) {
        return false;
    }
    
    auto stream_metrics = streaming_engine_->getQualityMetrics();
    
    // Check if poor connection is affecting stream quality
    return health_metrics_.current_health <= HealthStatus::POOR &&
           stream_metrics.current_level <= eti::streaming::QualityLevel::ACCEPTABLE;
}

std::vector<std::string> StreamResilience::identifyActiveIssues() {
    std::vector<std::string> issues;
    
    if (!health_metrics_.is_connected) {
        issues.push_back("No active connection");
    }
    
    if (health_metrics_.consecutive_failures > 3) {
        issues.push_back("Multiple consecutive connection failures");
    }
    
    if (health_metrics_.avg_latency > std::chrono::milliseconds{100}) {
        issues.push_back("High connection latency");
    }
    
    if (health_metrics_.connection_stability < 80.0) {
        issues.push_back("Unstable connection");
    }
    
    if (health_metrics_.getAvailability() < 95.0) {
        issues.push_back("Poor connection availability");
    }
    
    return issues;
}

std::vector<StreamResilience::RecommendedAction> StreamResilience::generateRecommendations() const {
    std::vector<RecommendedAction> actions;
    
    // Recommend reconnection if offline
    if (!health_metrics_.is_connected) {
        actions.push_back({
            "Attempt manual reconnection",
            "No active connection detected",
            1,
            std::chrono::milliseconds{5000}
        });
    }
    
    // Recommend endpoint switch if current is problematic
    if (health_metrics_.consecutive_failures > 2 && !fallback_endpoints_.empty()) {
        actions.push_back({
            "Switch to fallback endpoint",
            "Current endpoint experiencing failures",
            2,
            std::chrono::milliseconds{2000}
        });
    }
    
    // Recommend strategy adaptation
    if (health_metrics_.getAvailability() < 90.0) {
        actions.push_back({
            "Adapt reconnection strategy",
            "Poor connection availability",
            3,
            std::chrono::milliseconds{1000}
        });
    }
    
    return actions;
}

// ============================================================================
// Utility Methods
// ============================================================================

std::chrono::milliseconds StreamResilience::calculateUptime() const {
    if (!health_metrics_.is_connected) {
        return std::chrono::milliseconds{0};
    }
    
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        now - health_metrics_.last_successful_connection);
}

std::chrono::milliseconds StreamResilience::calculateDowntime() const {
    if (health_metrics_.is_connected) {
        return std::chrono::milliseconds{0};
    }
    
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        now - health_metrics_.last_failure);
}

void StreamResilience::logConnectionEvent(const QString& event, const QString& details) {
    QString log_message = event;
    if (!details.isEmpty()) {
        log_message += QString(" - %1").arg(details);
    }
    
    Logger::instance().log(Logger::Info, "StreamResilience", log_message);
}

// ============================================================================
// Slot Implementations
// ============================================================================

void StreamResilience::reconnect() {
    forceReconnection();
}

void StreamResilience::switchToNextEndpoint() {
    auto* next_endpoint = findBestAvailableEndpoint();
    if (next_endpoint) {
        switchToEndpoint(next_endpoint->url);
    } else {
        Logger::instance().log(Logger::Warning, "StreamResilience", 
            "No available fallback endpoints for switching");
    }
}

void StreamResilience::switchToEndpoint(const QString& endpoint_url) {
    Logger::instance().log(Logger::Info, "StreamResilience", 
        QString("Manual endpoint switch requested: %1").arg(endpoint_url));
    
    QString old_endpoint = current_endpoint_;
    
    // Disconnect current connection
    if (zmq_client_) {
        zmq_client_->disconnectFromStream();
    }
    
    // Attempt connection to new endpoint
    if (attemptConnectionToEndpoint(endpoint_url)) {
        emit endpointSwitch(old_endpoint, endpoint_url, "Manual switch");
        Logger::instance().log(Logger::Info, "StreamResilience", 
            QString("Successfully switched to endpoint: %1").arg(endpoint_url));
    } else {
        Logger::instance().log(Logger::Error, "StreamResilience", 
            QString("Failed to switch to endpoint: %1").arg(endpoint_url));
        
        // Try to reconnect to original endpoint
        if (!old_endpoint.isEmpty()) {
            attemptConnectionToEndpoint(old_endpoint);
        }
    }
}

void StreamResilience::updateReconnectionStrategy(const ReconnectionStrategy& strategy) {
    setReconnectionStrategy(strategy);
}

void StreamResilience::handleZMQConnectionStatus(const eti::network::ConnectionStatus& status) {
    if (status.isConnected()) {
        // Connection established/restored
        consecutive_failures_ = 0;
        reconnection_in_progress_ = false;
        
        {
            QMutexLocker locker(&metrics_mutex_);
            health_metrics_.is_connected = true;
            health_metrics_.last_successful_connection = std::chrono::system_clock::now();
        }
        
        Logger::instance().log(Logger::Info, "StreamResilience", "ZMQ connection established");
        
    } else {
        // Connection lost
        handleConnectionLoss();
    }
    
    updateHealthMetrics();
}

void StreamResilience::handleHealthMonitoring() {
    if (!monitoring_.load()) {
        return;
    }
    
    updateHealthMetrics();
}

void StreamResilience::handleReconnectionTimer() {
    executeReconnectionAttempt();
}

void StreamResilience::handleEndpointHealthCheck() {
    if (!monitoring_.load()) {
        return;
    }
    
    performEndpointHealthChecks();
}

void StreamResilience::handleHealthCheckResponse() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    auto it = pending_health_checks_.find(reply);
    if (it == pending_health_checks_.end()) {
        reply->deleteLater();
        return;
    }
    
    QString endpoint_url = it->second;
    pending_health_checks_.erase(it);
    
    bool healthy = (reply->error() == QNetworkReply::NoError) && 
                   (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200);
    
    auto response_time = std::chrono::milliseconds{reply->property("responseTime").toLongLong()};
    
    // Update endpoint health status
    QMutexLocker locker(&config_mutex_);
    auto endpoint_it = std::find_if(fallback_endpoints_.begin(), fallback_endpoints_.end(),
        [&endpoint_url](const EndpointConfig& config) {
            return config.url == endpoint_url;
        });
    
    if (endpoint_it != fallback_endpoints_.end()) {
        endpoint_it->currently_healthy = healthy;
    }
    
    emit endpointHealthCheck(endpoint_url, healthy, response_time);
    
    Logger::instance().log(healthy ? Logger::Debug : Logger::Warning, "StreamResilience", 
        QString("Health check for %1: %2 (response time: %3ms)")
        .arg(endpoint_url)
        .arg(healthy ? "HEALTHY" : "UNHEALTHY")
        .arg(response_time.count()));
    
    reply->deleteLater();
}

void StreamResilience::handleStreamQualityUpdate(const eti::streaming::QualityMetrics& metrics) {
    // Update connection impact assessment based on stream quality
    QMutexLocker locker(&metrics_mutex_);
    
    health_metrics_.affecting_stream_quality = 
        (health_metrics_.current_health <= HealthStatus::POOR) &&
        (metrics.current_level <= eti::streaming::QualityLevel::ACCEPTABLE);
    
    health_metrics_.stream_impact_score = 
        std::max(0.0, 100.0 - metrics.getOverallQuality());
}

// ============================================================================
// Utility Functions Implementation
// ============================================================================

namespace utils {

ReconnectionStrategy createBroadcastStrategy() {
    ReconnectionStrategy strategy;
    strategy.auto_reconnect_enabled = true;
    strategy.max_reconnect_attempts = 10;
    strategy.initial_delay = std::chrono::milliseconds{1000};
    strategy.max_delay = std::chrono::milliseconds{30000};
    strategy.backoff_multiplier = 1.5;
    strategy.health_check_interval = std::chrono::milliseconds{5000};
    strategy.adaptive_strategy = true;
    strategy.consecutive_failures_for_adaptation = 3;
    strategy.fallback_endpoints_enabled = true;
    strategy.connection_timeout = std::chrono::seconds{10};
    return strategy;
}

ReconnectionStrategy createAggressiveStrategy() {
    ReconnectionStrategy strategy;
    strategy.auto_reconnect_enabled = true;
    strategy.max_reconnect_attempts = 20;
    strategy.initial_delay = std::chrono::milliseconds{500};
    strategy.max_delay = std::chrono::milliseconds{5000};
    strategy.backoff_multiplier = 1.2;
    strategy.health_check_interval = std::chrono::milliseconds{2000};
    strategy.adaptive_strategy = true;
    strategy.consecutive_failures_for_adaptation = 2;
    strategy.fallback_endpoints_enabled = true;
    strategy.connection_timeout = std::chrono::seconds{5};
    return strategy;
}

ReconnectionStrategy createConservativeStrategy() {
    ReconnectionStrategy strategy;
    strategy.auto_reconnect_enabled = true;
    strategy.max_reconnect_attempts = 5;
    strategy.initial_delay = std::chrono::milliseconds{5000};
    strategy.max_delay = std::chrono::milliseconds{60000};
    strategy.backoff_multiplier = 2.0;
    strategy.health_check_interval = std::chrono::milliseconds{10000};
    strategy.adaptive_strategy = false;
    strategy.consecutive_failures_for_adaptation = 5;
    strategy.fallback_endpoints_enabled = false;
    strategy.connection_timeout = std::chrono::seconds{30};
    return strategy;
}

QString healthStatusToString(HealthStatus status) {
    switch (status) {
        case HealthStatus::EXCELLENT: return "EXCELLENT";
        case HealthStatus::GOOD: return "GOOD";
        case HealthStatus::DEGRADED: return "DEGRADED";
        case HealthStatus::POOR: return "POOR";
        case HealthStatus::CRITICAL: return "CRITICAL";
        case HealthStatus::OFFLINE: return "OFFLINE";
    }
    return "UNKNOWN";
}

QString healthStatusToColor(HealthStatus status) {
    switch (status) {
        case HealthStatus::EXCELLENT: return "#00AA00"; // Green
        case HealthStatus::GOOD: return "#88AA00"; // Yellow-green
        case HealthStatus::DEGRADED: return "#AAAA00"; // Yellow
        case HealthStatus::POOR: return "#AA5500"; // Orange
        case HealthStatus::CRITICAL: return "#AA0000"; // Red
        case HealthStatus::OFFLINE: return "#666666"; // Gray
    }
    return "#888888";
}

ParsedEndpoint parseEndpointURL(const QString& url) {
    ParsedEndpoint result;
    
    QUrl qurl(url);
    if (!qurl.isValid()) {
        return result;
    }
    
    result.protocol = qurl.scheme();
    result.host = qurl.host();
    result.port = qurl.port();
    result.valid = !result.protocol.isEmpty() && !result.host.isEmpty() && result.port > 0;
    
    return result;
}

bool validateEndpointURL(const QString& url) {
    auto parsed = parseEndpointURL(url);
    return parsed.valid && 
           (parsed.protocol == "tcp" || parsed.protocol == "ipc" || parsed.protocol == "inproc");
}

QString generateHealthCheckURL(const QString& zmq_endpoint, quint16 http_port) {
    auto parsed = parseEndpointURL(zmq_endpoint);
    if (!parsed.valid) {
        return QString();
    }
    
    return QString("http://%1:%2/health").arg(parsed.host).arg(http_port);
}

} // namespace utils

} // namespace eti::resilience