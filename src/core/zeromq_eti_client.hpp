/**
 * @file zeromq_eti_client.hpp
 * @brief ZeroMQ ETI Client for ODR-DabMux Integration
 * 
 * Implementation of real-time ETI-NI stream processing via ZeroMQ transport.
 * Supports TCP transport for ODR-DabMux connectivity with error handling
 * and connection management.
 * 
 * @author StreamDAB Development Team
 * @date 2025
 * @copyright Copyright (c) 2025 StreamDAB Technologies
 */

#pragma once

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QString>
#include <QByteArray>
#include <QMutex>
#include <QWaitCondition>
#include <atomic>
#include <memory>
#include <chrono>
#include <functional>

#include "eti_types.hpp"
#include "modern_eti_frame_parser.hpp"

// Simple forward declarations for ZeroMQ types
typedef void* zmq_ctx_t;
typedef void* zmq_socket_t;
struct zmq_msg_t;

namespace eti::network {

/**
 * @brief Connection configuration for ZeroMQ ETI transport
 */
struct ZMQConnectionConfig {
    enum class TransportType {
        TCP,        // tcp://host:port 
        IPC,        // ipc:///path/to/socket
        INPROC      // inproc://name
    };
    
    TransportType transport_type{TransportType::TCP};
    QString host{"localhost"};
    quint16 port{9200};
    QString socket_path{"/tmp/eti_stream"};
    QString endpoint_name{"eti"};
    
    // Connection parameters
    int recv_timeout_ms{1000};      // ZMQ_RCVTIMEO
    int recv_hwm{1000};             // ZMQ_RCVHWM (high water mark)
    int linger_ms{0};               // ZMQ_LINGER
    bool immediate{true};           // ZMQ_IMMEDIATE
    
    // Performance settings
    int io_threads{1};              // ZMQ context IO threads
    int max_sockets{1024};          // ZMQ context max sockets
    
    /**
     * @brief Generate full ZeroMQ endpoint URL
     * @return Complete endpoint string for zmq_connect()
     */
    [[nodiscard]] QString getEndpointUrl() const {
        switch (transport_type) {
            case TransportType::TCP:
                return QString("tcp://%1:%2").arg(host).arg(port);
            case TransportType::IPC:
                return QString("ipc://%1").arg(socket_path);
            case TransportType::INPROC:
                return QString("inproc://%1").arg(endpoint_name);
        }
        return {};
    }
    
    /**
     * @brief Validate configuration parameters
     * @return true if configuration is valid
     */
    [[nodiscard]] bool isValid() const {
        if (transport_type == TransportType::TCP) {
            return !host.isEmpty() && port > 0 && port < 65536;
        } else if (transport_type == TransportType::IPC) {
            return !socket_path.isEmpty();
        } else if (transport_type == TransportType::INPROC) {
            return !endpoint_name.isEmpty();
        }
        return false;
    }
};

/**
 * @brief Connection status information
 */
struct ConnectionStatus {
    enum class State {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        ERROR,
        RECONNECTING
    };
    
    State state{State::DISCONNECTED};
    QString error_message;
    std::chrono::system_clock::time_point last_update;
    std::chrono::system_clock::time_point connect_time;
    
    // Statistics
    uint64_t frames_received{0};
    uint64_t bytes_received{0};
    uint64_t connection_errors{0};
    uint64_t frame_errors{0};
    
    // Performance metrics
    double receive_rate_fps{0.0};
    std::chrono::microseconds avg_frame_latency{0};
    
    /**
     * @brief Check if currently connected and operational
     */
    [[nodiscard]] bool isConnected() const {
        return state == State::CONNECTED;
    }
    
    /**
     * @brief Get connection duration
     */
    [[nodiscard]] std::chrono::milliseconds getConnectionDuration() const {
        if (state == State::CONNECTED) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - connect_time);
        }
        return std::chrono::milliseconds{0};
    }
};

/**
 * @brief ZeroMQ ETI Client for ODR-DabMux Integration
 * 
 * High-performance ZeroMQ client for receiving real-time ETI-NI streams
 * from ODR-DabMux multiplexer. Supports TCP transport with automatic
 * reconnection and comprehensive error handling.
 */
class ZeroMQETIClient : public QObject {
    Q_OBJECT
    
public:
    explicit ZeroMQETIClient(QObject* parent = nullptr);
    ~ZeroMQETIClient() override;
    
    // Disable copy/move to ensure proper ZMQ resource management
    ZeroMQETIClient(const ZeroMQETIClient&) = delete;
    ZeroMQETIClient& operator=(const ZeroMQETIClient&) = delete;
    ZeroMQETIClient(ZeroMQETIClient&&) = delete;
    ZeroMQETIClient& operator=(ZeroMQETIClient&&) = delete;
    
    /**
     * @brief Initialize ZeroMQ context and prepare for connections
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Check if client is initialized and ready for use
     */
    [[nodiscard]] bool isInitialized() const noexcept {
        return initialized_.load();
    }
    
    /**
     * @brief Connect to ODR-DabMux ETI stream
     * @param config Connection configuration
     * @return true if connection initiated successfully
     */
    bool connectToStream(const ZMQConnectionConfig& config);
    
    /**
     * @brief Disconnect from current stream
     */
    void disconnectFromStream();
    
    /**
     * @brief Start receiving ETI frames
     * @return true if receiving started successfully
     */
    bool startReceiving();
    
    /**
     * @brief Stop receiving ETI frames
     */
    void stopReceiving();
    
    /**
     * @brief Get current connection status
     */
    [[nodiscard]] ConnectionStatus getConnectionStatus() const;
    
    /**
     * @brief Get current configuration
     */
    [[nodiscard]] ZMQConnectionConfig getCurrentConfig() const {
        QMutexLocker locker(&config_mutex_);
        return current_config_;
    }
    
    /**
     * @brief Enable/disable automatic reconnection
     * @param enable true to enable auto-reconnect
     * @param interval_ms reconnection interval in milliseconds
     */
    void setAutoReconnect(bool enable, int interval_ms = 5000);
    
    /**
     * @brief Set ETI frame parser for processing received frames
     * @param parser Modern ETI frame parser instance
     */
    void setFrameParser(std::shared_ptr<eti::modern::ModernETIFrameParser> parser);
    
    /**
     * @brief Get performance statistics
     */
    struct PerformanceStats {
        uint64_t total_frames_received{0};
        uint64_t total_bytes_received{0};
        double current_fps{0.0};
        double average_fps{0.0};
        std::chrono::microseconds avg_latency{0};
        std::chrono::microseconds max_latency{0};
        uint64_t frame_errors{0};
        uint64_t connection_errors{0};
        double uptime_percentage{0.0};
    };
    
    [[nodiscard]] PerformanceStats getPerformanceStats() const;
    
    /**
     * @brief Reset performance counters
     */
    void resetStats();

public slots:
    /**
     * @brief Connect using endpoint URL string
     * @param endpoint ZeroMQ endpoint (e.g., "tcp://localhost:9200")
     */
    void connectToEndpoint(const QString& endpoint);
    
    /**
     * @brief Reconnect to last known configuration
     */
    void reconnect();

signals:
    /**
     * @brief Emitted when connection state changes
     * @param status Current connection status
     */
    void connectionStatusChanged(const ConnectionStatus& status);
    
    /**
     * @brief Emitted when ETI frame is received and validated
     * @param frame_data Raw ETI frame data (6144 bytes)
     * @param receive_timestamp Frame reception timestamp
     */
    void etiFrameReceived(const QByteArray& frame_data, 
                         const std::chrono::system_clock::time_point& receive_timestamp);
    
    /**
     * @brief Emitted when ETI frame is successfully parsed
     * @param frame Parsed ETI frame structure
     * @param parse_result Parse result with validation info
     */
    void etiFrameParsed(const eti::EtiFrame& frame, 
                       const eti::modern::ETIParseResult& parse_result);
    
    /**
     * @brief Emitted when connection error occurs
     * @param error_message Human-readable error description
     * @param error_code ZeroMQ error code (if applicable)
     */
    void connectionError(const QString& error_message, int error_code = 0);
    
    /**
     * @brief Emitted when frame processing error occurs
     * @param error_message Human-readable error description
     * @param frame_number Frame sequence number (if known)
     */
    void frameError(const QString& error_message, uint32_t frame_number = 0);
    
    /**
     * @brief Emitted periodically with performance statistics
     * @param stats Current performance metrics
     */
    void performanceUpdate(const PerformanceStats& stats);

private slots:
    /**
     * @brief Handle reconnection timer timeout
     */
    void handleReconnectTimer();
    
    /**
     * @brief Handle statistics update timer
     */
    void handleStatsTimer();
    
    /**
     * @brief Handle frame processing completion
     */
    void handleFrameProcessed(uint32_t frame_number, const eti::EtiFrame& frame,
                             std::chrono::nanoseconds parse_time);

private:
    /**
     * @brief Worker thread for ZeroMQ reception
     */
    class ZMQWorkerThread : public QThread {
    public:
        explicit ZMQWorkerThread(ZeroMQETIClient* parent);
        ~ZMQWorkerThread() override;
        
        void run() override;
        void stop();
        
        void setSocket(zmq_socket_t socket) { socket_ = socket; }
        
    private:
        ZeroMQETIClient* client_;
        zmq_socket_t socket_{nullptr};
        std::atomic<bool> stop_flag_{false};
    };
    
    // Core ZeroMQ functions
    bool initializeZMQContext();
    void cleanupZMQContext();
    bool createZMQSocket();
    void closeZMQSocket();
    bool configureSocket(const ZMQConnectionConfig& config);
    
    // Frame processing
    void processReceivedFrame(const QByteArray& frame_data);
    bool validateETIFrame(const QByteArray& frame_data) const;
    
    // Connection management
    void updateConnectionStatus(ConnectionStatus::State new_state, 
                               const QString& error_message = {});
    void startReconnectTimer();
    void stopReconnectTimer();
    
    // Statistics and monitoring
    void updateReceiveStats();
    void updatePerformanceMetrics();
    
    // Member variables
    std::atomic<bool> initialized_{false};
    std::atomic<bool> receiving_{false};
    
    // ZeroMQ context and socket
    zmq_ctx_t zmq_context_{nullptr};
    zmq_socket_t zmq_socket_{nullptr};
    
    // Configuration and status
    mutable QMutex config_mutex_;
    ZMQConnectionConfig current_config_;
    ConnectionStatus connection_status_;
    
    // Worker thread for reception
    std::unique_ptr<ZMQWorkerThread> worker_thread_;
    
    // Frame parser
    std::shared_ptr<eti::modern::ModernETIFrameParser> frame_parser_;
    
    // Timers
    QTimer* reconnect_timer_{nullptr};
    QTimer* stats_timer_{nullptr};
    
    // Auto-reconnect settings
    bool auto_reconnect_enabled_{false};
    int reconnect_interval_ms_{5000};
    
    // Performance tracking
    mutable QMutex stats_mutex_;
    PerformanceStats performance_stats_;
    std::chrono::system_clock::time_point start_time_;
    std::chrono::system_clock::time_point last_frame_time_;
    std::vector<std::chrono::system_clock::time_point> recent_frame_times_;
    
    // Frame sequence tracking
    std::atomic<uint32_t> frame_sequence_{0};
    std::atomic<uint64_t> total_frames_received_{0};
    std::atomic<uint64_t> total_bytes_received_{0};
};

/**
 * @brief Utility functions for ZeroMQ ETI transport
 */
namespace zmq_utils {
    /**
     * @brief Parse ZeroMQ endpoint URL into configuration
     * @param endpoint Endpoint URL (e.g., "tcp://localhost:9200")
     * @return Configuration structure
     */
    [[nodiscard]] ZMQConnectionConfig parseEndpointUrl(const QString& endpoint);
    
    /**
     * @brief Validate ZeroMQ endpoint URL format
     * @param endpoint Endpoint URL to validate
     * @return true if URL format is valid
     */
    [[nodiscard]] bool validateEndpointUrl(const QString& endpoint);
    
    /**
     * @brief Get human-readable ZeroMQ error string
     * @param error_code ZeroMQ error code
     * @return Error description string
     */
    [[nodiscard]] QString getZMQErrorString(int error_code);
    
    /**
     * @brief Test ZeroMQ endpoint connectivity
     * @param endpoint Endpoint URL to test
     * @param timeout_ms Connection timeout in milliseconds
     * @return true if endpoint is reachable
     */
    [[nodiscard]] bool testEndpointConnectivity(const QString& endpoint, 
                                               int timeout_ms = 5000);
}

} // namespace eti::network