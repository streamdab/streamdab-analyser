/**
 * @file stream_resilience.hpp
 * @brief Stream Resilience and Connection Health Monitoring
 * 
 * Advanced connection health monitoring, automatic reconnection, and
 * graceful degradation for real-time ETI streaming applications.
 * Provides <1s automatic reconnection and exponential backoff.
 * 
 * @author StreamDAB Development Team
 * @date 2025
 * @copyright Copyright (c) 2025 StreamDAB Technologies
 */

#pragma once

#include "zeromq_eti_client.hpp"
#include "streaming_engine.hpp"
#include "eti_types.hpp"

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <atomic>
#include <memory>
#include <vector>
#include <chrono>
#include <functional>

namespace eti::resilience {

/**
 * @brief Connection health status enumeration
 */
enum class HealthStatus {
    EXCELLENT = 5,    // Perfect connection, no issues
    GOOD = 4,         // Minor fluctuations, stable
    DEGRADED = 3,     // Noticeable issues, still functional
    POOR = 2,         // Significant problems, unstable
    CRITICAL = 1,     // Severe issues, barely functional
    OFFLINE = 0       // No connection
};

/**
 * @brief Reconnection strategy configuration
 */
struct ReconnectionStrategy {
    bool auto_reconnect_enabled{true};     // Enable automatic reconnection
    int max_reconnect_attempts{10};        // Maximum reconnection attempts
    std::chrono::milliseconds initial_delay{1000};     // Initial backoff delay
    std::chrono::milliseconds max_delay{30000};        // Maximum backoff delay
    double backoff_multiplier{2.0};        // Exponential backoff multiplier
    std::chrono::milliseconds health_check_interval{5000}; // Health check frequency
    
    // Advanced settings
    bool adaptive_strategy{true};          // Adapt strategy based on failure patterns
    int consecutive_failures_for_adaptation{3}; // Failures before strategy adaptation
    bool fallback_endpoints_enabled{true}; // Enable fallback endpoint switching
    std::chrono::seconds connection_timeout{10}; // Connection attempt timeout
    
    /**
     * @brief Validate strategy configuration
     */
    [[nodiscard]] bool isValid() const {
        return max_reconnect_attempts > 0 &&
               initial_delay.count() > 0 &&
               max_delay >= initial_delay &&
               backoff_multiplier > 1.0 &&
               health_check_interval.count() > 0 &&
               consecutive_failures_for_adaptation > 0 &&
               connection_timeout.count() > 0;
    }
    
    /**
     * @brief Calculate next reconnection delay
     * @param attempt_number Current attempt number (0-based)
     */
    [[nodiscard]] std::chrono::milliseconds calculateDelay(int attempt_number) const {
        auto delay = initial_delay;
        for (int i = 0; i < attempt_number; ++i) {
            delay = std::chrono::milliseconds(
                static_cast<long long>(delay.count() * backoff_multiplier));
        }
        return std::min(delay, max_delay);
    }
};

/**
 * @brief Connection health metrics
 */
struct ConnectionHealthMetrics {
    // Basic connection metrics
    bool is_connected{false};
    std::chrono::system_clock::time_point last_successful_connection;
    std::chrono::system_clock::time_point last_failure;
    std::chrono::milliseconds uptime{0};
    std::chrono::milliseconds downtime{0};
    
    // Connection quality metrics
    double connection_stability{100.0};    // Connection stability percentage
    std::chrono::milliseconds avg_latency{0};       // Average connection latency
    std::chrono::milliseconds max_latency{0};       // Maximum latency observed
    double packet_loss_rate{0.0};          // Packet loss percentage
    
    // Failure tracking
    uint64_t total_connection_attempts{0};
    uint64_t successful_connections{0};
    uint64_t failed_connections{0};
    uint64_t reconnection_attempts{0};
    uint64_t consecutive_failures{0};
    
    // Quality indicators
    HealthStatus current_health{HealthStatus::OFFLINE};
    double health_score{0.0};              // Overall health score (0-100)
    std::vector<std::string> active_issues; // Current connection issues
    
    // Performance impact
    bool affecting_stream_quality{false};   // Whether connection issues affect streaming
    double stream_impact_score{0.0};       // Impact on stream quality (0-100)
    
    /**
     * @brief Calculate connection success rate
     */
    [[nodiscard]] double getSuccessRate() const {
        return total_connection_attempts > 0 ? 
               (static_cast<double>(successful_connections) / total_connection_attempts) * 100.0 : 0.0;
    }
    
    /**
     * @brief Get connection availability percentage
     */
    [[nodiscard]] double getAvailability() const {
        auto total_time = uptime + downtime;
        return total_time.count() > 0 ? 
               (static_cast<double>(uptime.count()) / total_time.count()) * 100.0 : 0.0;
    }
    
    /**
     * @brief Determine health status from metrics
     */
    [[nodiscard]] HealthStatus calculateHealthStatus() const {
        if (!is_connected) return HealthStatus::OFFLINE;
        
        double composite_score = 
            (connection_stability * 0.4) +
            (getSuccessRate() * 0.3) +
            (getAvailability() * 0.2) +
            ((100.0 - packet_loss_rate) * 0.1);
        
        if (composite_score >= 95.0) return HealthStatus::EXCELLENT;
        if (composite_score >= 85.0) return HealthStatus::GOOD;
        if (composite_score >= 70.0) return HealthStatus::DEGRADED;
        if (composite_score >= 50.0) return HealthStatus::POOR;
        return HealthStatus::CRITICAL;
    }
};

/**
 * @brief Endpoint configuration for failover
 */
struct EndpointConfig {
    QString url;                           // ZeroMQ endpoint URL
    QString description;                   // Human-readable description
    int priority{1};                       // Priority (1=highest, higher numbers=lower priority)
    bool enabled{true};                    // Whether endpoint is enabled
    std::chrono::milliseconds timeout{5000}; // Connection timeout
    
    // Health check settings
    bool health_check_enabled{true};       // Enable periodic health checks
    QString health_check_url;              // Optional HTTP health check URL
    std::chrono::milliseconds health_check_timeout{3000}; // Health check timeout
    
    // Statistics
    uint64_t connection_attempts{0};
    uint64_t successful_connections{0};
    std::chrono::system_clock::time_point last_attempt;
    std::chrono::system_clock::time_point last_success;
    bool currently_healthy{true};
    
    /**
     * @brief Calculate endpoint success rate
     */
    [[nodiscard]] double getSuccessRate() const {
        return connection_attempts > 0 ? 
               (static_cast<double>(successful_connections) / connection_attempts) * 100.0 : 0.0;
    }
    
    /**
     * @brief Check if endpoint should be used
     */
    [[nodiscard]] bool isUsable() const {
        return enabled && currently_healthy;
    }
};

/**
 * @brief Stream Resilience Manager
 * 
 * Comprehensive connection health monitoring and automatic recovery
 * system for maintaining continuous ETI streaming with minimal
 * interruption and <1s reconnection times.
 */
class StreamResilience : public QObject {
    Q_OBJECT
    
public:
    explicit StreamResilience(QObject* parent = nullptr);
    ~StreamResilience() override;
    
    // Disable copy/move for proper resource management
    StreamResilience(const StreamResilience&) = delete;
    StreamResilience& operator=(const StreamResilience&) = delete;
    StreamResilience(StreamResilience&&) = delete;
    StreamResilience& operator=(StreamResilience&&) = delete;
    
    /**
     * @brief Initialize resilience manager
     * @param zmq_client ZeroMQ ETI client to monitor
     * @param streaming_engine Streaming engine for quality feedback
     * @return true if initialization successful
     */
    bool initialize(std::shared_ptr<eti::network::ZeroMQETIClient> zmq_client,
                   std::shared_ptr<eti::streaming::StreamingEngine> streaming_engine = nullptr);
    
    /**
     * @brief Check if resilience manager is initialized
     */
    [[nodiscard]] bool isInitialized() const noexcept {
        return initialized_.load();
    }
    
    /**
     * @brief Start connection health monitoring
     * @return true if monitoring started successfully
     */
    bool startMonitoring();
    
    /**
     * @brief Stop connection health monitoring
     */
    void stopMonitoring();
    
    /**
     * @brief Check if monitoring is active
     */
    [[nodiscard]] bool isMonitoring() const noexcept {
        return monitoring_.load();
    }
    
    /**
     * @brief Configure reconnection strategy
     * @param strategy Reconnection strategy parameters
     */
    void setReconnectionStrategy(const ReconnectionStrategy& strategy);
    
    /**
     * @brief Get current reconnection strategy
     */
    [[nodiscard]] ReconnectionStrategy getReconnectionStrategy() const {
        QMutexLocker locker(&config_mutex_);
        return strategy_;
    }
    
    /**
     * @brief Enable/disable automatic reconnection
     * @param enabled true to enable auto-reconnect
     * @param max_retries Maximum number of retry attempts
     */
    void enableAutoReconnect(bool enabled, int max_retries = 10);
    
    /**
     * @brief Add fallback endpoint for failover
     * @param config Endpoint configuration
     */
    void addFallbackEndpoint(const EndpointConfig& config);
    
    /**
     * @brief Remove fallback endpoint
     * @param url Endpoint URL to remove
     */
    void removeFallbackEndpoint(const QString& url);
    
    /**
     * @brief Get list of configured endpoints
     */
    [[nodiscard]] std::vector<EndpointConfig> getFallbackEndpoints() const;
    
    /**
     * @brief Clear all fallback endpoints
     */
    void clearFallbackEndpoints();
    
    /**
     * @brief Handle connection loss (call when connection fails)
     */
    void handleConnectionLoss();
    
    /**
     * @brief Attempt manual reconnection
     * @param retry_count Current retry attempt number
     * @return true if reconnection attempt was initiated
     */
    bool attemptReconnection(int retry_count = 0);
    
    /**
     * @brief Force immediate reconnection attempt
     * @return true if reconnection successful
     */
    bool forceReconnection();
    
    /**
     * @brief Get current connection health metrics
     */
    [[nodiscard]] ConnectionHealthMetrics getHealthMetrics() const;
    
    /**
     * @brief Get current health status
     */
    [[nodiscard]] HealthStatus getCurrentHealthStatus() const;
    
    /**
     * @brief Reset all statistics and counters
     */
    void resetStatistics();
    
    /**
     * @brief Enable/disable health checks for endpoints
     * @param enabled true to enable health checks
     */
    void setHealthChecksEnabled(bool enabled);
    
    /**
     * @brief Test endpoint connectivity
     * @param endpoint_url Endpoint URL to test
     * @return true if endpoint is reachable
     */
    bool testEndpointConnectivity(const QString& endpoint_url);
    
    /**
     * @brief Get recommended action for current connection state
     */
    struct RecommendedAction {
        QString action;                    // Recommended action description
        QString reason;                    // Reason for recommendation
        int priority{1};                   // Action priority (1=highest)
        std::chrono::milliseconds eta{0};  // Estimated time to complete
    };
    
    [[nodiscard]] std::vector<RecommendedAction> getRecommendedActions() const;

public slots:
    /**
     * @brief Manually trigger reconnection
     */
    void reconnect();
    
    /**
     * @brief Switch to next available endpoint
     */
    void switchToNextEndpoint();
    
    /**
     * @brief Switch to specific endpoint
     * @param endpoint_url Target endpoint URL
     */
    void switchToEndpoint(const QString& endpoint_url);
    
    /**
     * @brief Update reconnection strategy at runtime
     * @param strategy New strategy configuration
     */
    void updateReconnectionStrategy(const ReconnectionStrategy& strategy);

signals:
    /**
     * @brief Emitted when reconnection attempt starts
     * @param attempt Current attempt number (1-based)
     * @param max_attempts Maximum attempts configured
     * @param delay Delay before this attempt
     */
    void reconnecting(int attempt, int max_attempts, std::chrono::milliseconds delay);
    
    /**
     * @brief Emitted when reconnection fails
     * @param attempt Failed attempt number
     * @param error_message Failure reason
     * @param will_retry true if more attempts will be made
     */
    void reconnectionFailed(int attempt, const QString& error_message, bool will_retry);
    
    /**
     * @brief Emitted when connection is restored
     * @param endpoint Endpoint that was successfully connected
     * @param attempt_count Number of attempts needed
     * @param total_downtime Total time offline
     */
    void connectionRestored(const QString& endpoint, int attempt_count, 
                           std::chrono::milliseconds total_downtime);
    
    /**
     * @brief Emitted when all reconnection attempts are exhausted
     * @param total_attempts Total attempts made
     * @param last_error Last error encountered
     */
    void reconnectionExhausted(int total_attempts, const QString& last_error);
    
    /**
     * @brief Emitted when health status changes
     * @param status New health status
     * @param metrics Current health metrics
     */
    void healthStatusChanged(HealthStatus status, const ConnectionHealthMetrics& metrics);
    
    /**
     * @brief Emitted when switching to fallback endpoint
     * @param from_endpoint Previous endpoint (may be empty)
     * @param to_endpoint New endpoint
     * @param reason Reason for switch
     */
    void endpointSwitch(const QString& from_endpoint, const QString& to_endpoint, 
                       const QString& reason);
    
    /**
     * @brief Emitted when connection quality degrades
     * @param level Degradation severity
     * @param metrics Current metrics
     * @param recommended_actions Suggested remediation actions
     */
    void connectionQualityAlert(HealthStatus level, const ConnectionHealthMetrics& metrics,
                               const std::vector<RecommendedAction>& recommended_actions);
    
    /**
     * @brief Emitted when endpoint health check completes
     * @param endpoint Endpoint that was checked
     * @param healthy true if endpoint is healthy
     * @param response_time Health check response time
     */
    void endpointHealthCheck(const QString& endpoint, bool healthy, 
                            std::chrono::milliseconds response_time);

private slots:
    /**
     * @brief Handle ZeroMQ connection status changes
     */
    void handleZMQConnectionStatus(const eti::network::ConnectionStatus& status);
    
    /**
     * @brief Handle periodic health monitoring
     */
    void handleHealthMonitoring();
    
    /**
     * @brief Handle reconnection timer timeout
     */
    void handleReconnectionTimer();
    
    /**
     * @brief Handle endpoint health check timer
     */
    void handleEndpointHealthCheck();
    
    /**
     * @brief Handle HTTP health check response
     */
    void handleHealthCheckResponse();
    
    /**
     * @brief Handle streaming engine quality updates
     */
    void handleStreamQualityUpdate(const eti::streaming::QualityMetrics& metrics);

private:
    // Core components
    std::shared_ptr<eti::network::ZeroMQETIClient> zmq_client_;
    std::shared_ptr<eti::streaming::StreamingEngine> streaming_engine_;
    
    // Configuration and state
    mutable QMutex config_mutex_;
    ReconnectionStrategy strategy_;
    std::vector<EndpointConfig> fallback_endpoints_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> monitoring_{false};
    
    // Reconnection state
    std::atomic<bool> reconnection_in_progress_{false};
    std::atomic<int> current_attempt_{0};
    std::atomic<int> consecutive_failures_{0};
    QString current_endpoint_;
    QString last_error_;
    std::chrono::system_clock::time_point last_connection_attempt_;
    std::chrono::system_clock::time_point connection_lost_time_;
    
    // Health monitoring
    mutable QMutex metrics_mutex_;
    ConnectionHealthMetrics health_metrics_;
    HealthStatus last_health_status_{HealthStatus::OFFLINE};
    
    // Timers
    QTimer* health_monitor_timer_{nullptr};
    QTimer* reconnection_timer_{nullptr};
    QTimer* endpoint_health_timer_{nullptr};
    
    // Network manager for HTTP health checks
    QNetworkAccessManager* network_manager_{nullptr};
    std::map<QNetworkReply*, QString> pending_health_checks_;
    
    // Feature flags
    std::atomic<bool> health_checks_enabled_{true};
    std::atomic<bool> adaptive_strategy_enabled_{true};
    
    // Statistics tracking
    std::chrono::system_clock::time_point start_time_;
    std::chrono::system_clock::time_point last_successful_connection_;
    std::deque<std::chrono::milliseconds> recent_latencies_;
    std::deque<bool> recent_connection_attempts_;
    
    // Helper methods
    void connectSignals();
    void disconnectSignals();
    void setupMonitoringTimers();
    void cleanupMonitoringTimers();
    
    // Connection management
    bool attemptConnectionToEndpoint(const QString& endpoint_url);
    EndpointConfig* findBestAvailableEndpoint();
    void updateEndpointStatistics(const QString& endpoint_url, bool success);
    bool isEndpointHealthy(const QString& endpoint_url);
    
    // Health monitoring
    void updateHealthMetrics();
    void calculateConnectionStability();
    void detectQualityDegradation();
    void performEndpointHealthChecks();
    void startHTTPHealthCheck(const EndpointConfig& endpoint);
    
    // Reconnection logic
    void scheduleReconnection(std::chrono::milliseconds delay);
    void executeReconnectionAttempt();
    bool shouldAdaptStrategy();
    void adaptReconnectionStrategy();
    
    // Quality assessment
    double assessConnectionQuality();
    bool isConnectionAffectingStreamQuality();
    std::vector<std::string> identifyActiveIssues();
    std::vector<RecommendedAction> generateRecommendations() const;
    
    // Utility methods
    std::chrono::milliseconds calculateUptime() const;
    std::chrono::milliseconds calculateDowntime() const;
    void logConnectionEvent(const QString& event, const QString& details = {});
};

/**
 * @brief Utility functions for stream resilience
 */
namespace utils {
    /**
     * @brief Create default reconnection strategy for broadcast applications
     */
    [[nodiscard]] ReconnectionStrategy createBroadcastStrategy();
    
    /**
     * @brief Create aggressive reconnection strategy for critical applications
     */
    [[nodiscard]] ReconnectionStrategy createAggressiveStrategy();
    
    /**
     * @brief Create conservative reconnection strategy for stable networks
     */
    [[nodiscard]] ReconnectionStrategy createConservativeStrategy();
    
    /**
     * @brief Convert health status to human-readable string
     */
    [[nodiscard]] QString healthStatusToString(HealthStatus status);
    
    /**
     * @brief Get health status color for UI display
     */
    [[nodiscard]] QString healthStatusToColor(HealthStatus status);
    
    /**
     * @brief Parse ZeroMQ endpoint URL to extract host and port
     */
    struct ParsedEndpoint {
        QString protocol;
        QString host;
        quint16 port{0};
        bool valid{false};
    };
    
    [[nodiscard]] ParsedEndpoint parseEndpointURL(const QString& url);
    
    /**
     * @brief Validate endpoint URL format
     */
    [[nodiscard]] bool validateEndpointURL(const QString& url);
    
    /**
     * @brief Generate health check URL from ZeroMQ endpoint
     */
    [[nodiscard]] QString generateHealthCheckURL(const QString& zmq_endpoint, 
                                                quint16 http_port = 8080);
}

} // namespace eti::resilience