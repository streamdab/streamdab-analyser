/**
 * @file zeromq_eti_client.cpp
 * @brief ZeroMQ ETI Client Implementation
 * 
 * Real-time ETI-NI stream processing via ZeroMQ transport for ODR-DabMux integration.
 * Provides high-performance ETI frame reception with Qt6 integration.
 * 
 * @author StreamDAB Development Team
 * @date 2025
 */

#include "zeromq_eti_client.hpp"
#include "utils/logger.h"

#include <zmq.h>
#include <QMutexLocker>
#include <QUrl>
#include <QRegularExpression>
#include <QApplication>
#include <algorithm>
#include <cstring>

namespace eti::network {

// ============================================================================
// ZeroMQETIClient Implementation
// ============================================================================

ZeroMQETIClient::ZeroMQETIClient(QObject* parent)
    : QObject(parent)
    , start_time_(std::chrono::system_clock::now())
{
    // Initialize timers
    reconnect_timer_ = new QTimer(this);
    reconnect_timer_->setSingleShot(true);
    connect(reconnect_timer_, &QTimer::timeout, 
            this, &ZeroMQETIClient::handleReconnectTimer);
    
    stats_timer_ = new QTimer(this);
    stats_timer_->setInterval(1000); // Update stats every second
    connect(stats_timer_, &QTimer::timeout,
            this, &ZeroMQETIClient::handleStatsTimer);
    
    // Initialize connection status
    connection_status_.state = ConnectionStatus::State::DISCONNECTED;
    connection_status_.last_update = std::chrono::system_clock::now();
    
    // Reserve space for frame time tracking
    recent_frame_times_.reserve(1000);
    
    Logger::instance().log(Logger::Info, "ZeroMQETIClient", 
                          "ZeroMQ ETI client created");
}

ZeroMQETIClient::~ZeroMQETIClient()
{
    // Ensure clean shutdown
    stopReceiving();
    disconnectFromStream();
    cleanupZMQContext();
    
    Logger::instance().log(Logger::Info, "ZeroMQETIClient", 
                          "ZeroMQ ETI client destroyed");
}

bool ZeroMQETIClient::initialize()
{
    if (initialized_.load()) {
        return true; // Already initialized
    }
    
    if (!initializeZMQContext()) {
        Logger::instance().log(Logger::Error, "ZeroMQETIClient", 
                              "Failed to initialize ZeroMQ context");
        return false;
    }
    
    // Start statistics timer
    stats_timer_->start();
    
    initialized_.store(true);
    
    Logger::instance().log(Logger::Info, "ZeroMQETIClient", 
                          "ZeroMQ ETI client initialized successfully");
    return true;
}

bool ZeroMQETIClient::connectToStream(const ZMQConnectionConfig& config)
{
    if (!initialized_.load()) {
        Logger::instance().log(Logger::Error, "ZeroMQETIClient", 
                              "Client not initialized");
        return false;
    }
    
    if (!config.isValid()) {
        Logger::instance().log(Logger::Error, "ZeroMQETIClient", 
                              "Invalid connection configuration");
        return false;
    }
    
    // Disconnect from current stream if connected
    if (connection_status_.isConnected()) {
        disconnectFromStream();
    }
    
    // Update configuration
    {
        QMutexLocker locker(&config_mutex_);
        current_config_ = config;
    }
    
    updateConnectionStatus(ConnectionStatus::State::CONNECTING);
    
    // Create and configure socket
    if (!createZMQSocket()) {
        updateConnectionStatus(ConnectionStatus::State::ERROR, 
                              "Failed to create ZeroMQ socket");
        return false;
    }
    
    if (!configureSocket(config)) {
        closeZMQSocket();
        updateConnectionStatus(ConnectionStatus::State::ERROR, 
                              "Failed to configure ZeroMQ socket");
        return false;
    }
    
    // Attempt connection
    QString endpoint = config.getEndpointUrl();
    int result = zmq_connect(zmq_socket_, endpoint.toUtf8().constData());
    
    if (result != 0) {
        int error_code = zmq_errno();
        QString error_msg = QString("Connection failed: %1 (code: %2)")
                           .arg(zmq_utils::getZMQErrorString(error_code))
                           .arg(error_code);
        
        closeZMQSocket();
        updateConnectionStatus(ConnectionStatus::State::ERROR, error_msg);
        
        Logger::instance().log(Logger::Error, "ZeroMQETIClient", 
                              QString("Failed to connect to %1: %2")
                              .arg(endpoint).arg(error_msg));
        return false;
    }
    
    updateConnectionStatus(ConnectionStatus::State::CONNECTED);
    connection_status_.connect_time = std::chrono::system_clock::now();
    
    Logger::instance().log(Logger::Info, "ZeroMQETIClient", 
                          QString("Successfully connected to %1").arg(endpoint));
    
    return true;
}

void ZeroMQETIClient::disconnectFromStream()
{
    stopReceiving();
    closeZMQSocket();
    updateConnectionStatus(ConnectionStatus::State::DISCONNECTED);
    
    Logger::instance().log(Logger::Info, "ZeroMQETIClient", 
                          "Disconnected from ETI stream");
}

bool ZeroMQETIClient::startReceiving()
{
    if (!connection_status_.isConnected()) {
        Logger::instance().log(Logger::Error, "ZeroMQETIClient", 
                              "Cannot start receiving: not connected");
        return false;
    }
    
    if (receiving_.load()) {
        return true; // Already receiving
    }
    
    // Create and start worker thread
    worker_thread_ = std::make_unique<ZMQWorkerThread>(this);
    worker_thread_->setSocket(zmq_socket_);
    worker_thread_->start();
    
    receiving_.store(true);
    
    Logger::instance().log(Logger::Info, "ZeroMQETIClient", 
                          "Started receiving ETI frames");
    return true;
}

void ZeroMQETIClient::stopReceiving()
{
    if (!receiving_.load()) {
        return; // Not receiving
    }
    
    receiving_.store(false);
    
    // Stop worker thread
    if (worker_thread_) {
        worker_thread_->stop();
        worker_thread_->wait(5000); // Wait up to 5 seconds
        if (worker_thread_->isRunning()) {
            worker_thread_->terminate();
            worker_thread_->wait(1000);
        }
        worker_thread_.reset();
    }
    
    Logger::instance().log(Logger::Info, "ZeroMQETIClient", 
                          "Stopped receiving ETI frames");
}

ConnectionStatus ZeroMQETIClient::getConnectionStatus() const
{
    QMutexLocker locker(&stats_mutex_);
    return connection_status_;
}

void ZeroMQETIClient::setAutoReconnect(bool enable, int interval_ms)
{
    auto_reconnect_enabled_ = enable;
    reconnect_interval_ms_ = interval_ms;
    
    if (!enable) {
        stopReconnectTimer();
    }
    
    Logger::instance().log(Logger::Info, "ZeroMQETIClient", 
                          QString("Auto-reconnect %1 (interval: %2ms)")
                          .arg(enable ? "enabled" : "disabled")
                          .arg(interval_ms));
}

void ZeroMQETIClient::setFrameParser(std::shared_ptr<eti::modern::ModernETIFrameParser> parser)
{
    frame_parser_ = parser;
    
    if (frame_parser_) {
        // Connect parser signals
        connect(frame_parser_.get(), &eti::modern::ModernETIFrameParser::frameProcessed,
                this, &ZeroMQETIClient::handleFrameProcessed);
        
        Logger::instance().log(Logger::Info, "ZeroMQETIClient", 
                              "ETI frame parser connected");
    }
}

ZeroMQETIClient::PerformanceStats ZeroMQETIClient::getPerformanceStats() const
{
    QMutexLocker locker(&stats_mutex_);
    return performance_stats_;
}

void ZeroMQETIClient::resetStats()
{
    QMutexLocker locker(&stats_mutex_);
    
    performance_stats_ = PerformanceStats{};
    connection_status_.frames_received = 0;
    connection_status_.bytes_received = 0;
    connection_status_.connection_errors = 0;
    connection_status_.frame_errors = 0;
    
    total_frames_received_.store(0);
    total_bytes_received_.store(0);
    frame_sequence_.store(0);
    
    start_time_ = std::chrono::system_clock::now();
    recent_frame_times_.clear();
    
    Logger::instance().log(Logger::Info, "ZeroMQETIClient", 
                          "Performance statistics reset");
}

// ============================================================================
// Public Slots
// ============================================================================

void ZeroMQETIClient::connectToEndpoint(const QString& endpoint)
{
    auto config = zmq_utils::parseEndpointUrl(endpoint);
    if (!config.isValid()) {
        Logger::instance().log(Logger::Error, "ZeroMQETIClient", 
                              QString("Invalid endpoint URL: %1").arg(endpoint));
        return;
    }
    
    connectToStream(config);
}

void ZeroMQETIClient::reconnect()
{
    QMutexLocker locker(&config_mutex_);
    ZMQConnectionConfig config = current_config_;
    locker.unlock();
    
    if (config.isValid()) {
        Logger::instance().log(Logger::Info, "ZeroMQETIClient", 
                              "Attempting reconnection...");
        connectToStream(config);
    }
}

// ============================================================================
// Private Slots
// ============================================================================

void ZeroMQETIClient::handleReconnectTimer()
{
    if (auto_reconnect_enabled_ && 
        connection_status_.state == ConnectionStatus::State::ERROR) {
        
        updateConnectionStatus(ConnectionStatus::State::RECONNECTING);
        reconnect();
    }
}

void ZeroMQETIClient::handleStatsTimer()
{
    updatePerformanceMetrics();
}

void ZeroMQETIClient::handleFrameProcessed(uint32_t frame_number, 
                                          const eti::EtiFrame& frame,
                                          std::chrono::nanoseconds parse_time)
{
    // Create parse result for signal emission
    eti::modern::ETIParseResult parse_result;
    parse_result.success = true;
    parse_result.frame = frame;
    parse_result.frame_number = frame_number;
    parse_result.parse_time = parse_time;
    
    emit etiFrameParsed(frame, parse_result);
}

// ============================================================================
// Private Implementation
// ============================================================================

bool ZeroMQETIClient::initializeZMQContext()
{
    // Create ZeroMQ context
    zmq_context_ = zmq_ctx_new();
    if (!zmq_context_) {
        int error_code = zmq_errno();
        Logger::instance().log(Logger::Error, "ZeroMQETIClient", 
                              QString("Failed to create ZMQ context: %1")
                              .arg(zmq_utils::getZMQErrorString(error_code)));
        return false;
    }
    
    // Configure context
    zmq_ctx_set(zmq_context_, ZMQ_IO_THREADS, 1);
    zmq_ctx_set(zmq_context_, ZMQ_MAX_SOCKETS, 1024);
    
    return true;
}

void ZeroMQETIClient::cleanupZMQContext()
{
    if (zmq_context_) {
        zmq_ctx_destroy(zmq_context_);
        zmq_context_ = nullptr;
    }
}

bool ZeroMQETIClient::createZMQSocket()
{
    if (!zmq_context_) {
        return false;
    }
    
    // Create SUB socket for receiving ETI streams
    zmq_socket_ = zmq_socket(zmq_context_, ZMQ_SUB);
    if (!zmq_socket_) {
        int error_code = zmq_errno();
        Logger::instance().log(Logger::Error, "ZeroMQETIClient", 
                              QString("Failed to create ZMQ socket: %1")
                              .arg(zmq_utils::getZMQErrorString(error_code)));
        return false;
    }
    
    // Subscribe to all messages (empty filter)
    int result = zmq_setsockopt(zmq_socket_, ZMQ_SUBSCRIBE, "", 0);
    if (result != 0) {
        int error_code = zmq_errno();
        Logger::instance().log(Logger::Error, "ZeroMQETIClient", 
                              QString("Failed to set subscription: %1")
                              .arg(zmq_utils::getZMQErrorString(error_code)));
        closeZMQSocket();
        return false;
    }
    
    return true;
}

void ZeroMQETIClient::closeZMQSocket()
{
    if (zmq_socket_) {
        zmq_close(zmq_socket_);
        zmq_socket_ = nullptr;
    }
}

bool ZeroMQETIClient::configureSocket(const ZMQConnectionConfig& config)
{
    if (!zmq_socket_) {
        return false;
    }
    
    // Set socket options
    int result = 0;
    
    // Receive timeout
    result = zmq_setsockopt(zmq_socket_, ZMQ_RCVTIMEO, 
                           &config.recv_timeout_ms, sizeof(config.recv_timeout_ms));
    if (result != 0) {
        Logger::instance().log(Logger::Warning, "ZeroMQETIClient", 
                              "Failed to set receive timeout");
    }
    
    // High water mark
    result = zmq_setsockopt(zmq_socket_, ZMQ_RCVHWM, 
                           &config.recv_hwm, sizeof(config.recv_hwm));
    if (result != 0) {
        Logger::instance().log(Logger::Warning, "ZeroMQETIClient", 
                              "Failed to set receive high water mark");
    }
    
    // Linger time
    result = zmq_setsockopt(zmq_socket_, ZMQ_LINGER, 
                           &config.linger_ms, sizeof(config.linger_ms));
    if (result != 0) {
        Logger::instance().log(Logger::Warning, "ZeroMQETIClient", 
                              "Failed to set linger time");
    }
    
    // Immediate mode
    int immediate = config.immediate ? 1 : 0;
    result = zmq_setsockopt(zmq_socket_, ZMQ_IMMEDIATE, 
                           &immediate, sizeof(immediate));
    if (result != 0) {
        Logger::instance().log(Logger::Warning, "ZeroMQETIClient", 
                              "Failed to set immediate mode");
    }
    
    return true;
}

void ZeroMQETIClient::processReceivedFrame(const QByteArray& frame_data)
{
    auto receive_timestamp = std::chrono::system_clock::now();
    
    // Update statistics
    total_frames_received_.fetch_add(1);
    total_bytes_received_.fetch_add(frame_data.size());
    
    // Track frame timing
    {
        QMutexLocker locker(&stats_mutex_);
        connection_status_.frames_received++;
        connection_status_.bytes_received += frame_data.size();
        
        recent_frame_times_.push_back(receive_timestamp);
        if (recent_frame_times_.size() > 1000) {
            recent_frame_times_.erase(recent_frame_times_.begin(), 
                                     recent_frame_times_.begin() + 100);
        }
        
        last_frame_time_ = receive_timestamp;
    }
    
    // Validate ETI frame
    if (!validateETIFrame(frame_data)) {
        connection_status_.frame_errors++;
        emit frameError("Invalid ETI frame received", frame_sequence_.load());
        return;
    }
    
    // Emit raw frame signal
    emit etiFrameReceived(frame_data, receive_timestamp);
    
    // Parse frame if parser is available
    if (frame_parser_) {
        try {
            auto parse_result = frame_parser_->parseFrame(frame_data);
            if (parse_result.success) {
                emit etiFrameParsed(parse_result.frame, parse_result);
            } else {
                emit frameError("Frame parsing failed", parse_result.frame_number);
            }
        } catch (const std::exception& e) {
            Logger::instance().log(Logger::Error, "ZeroMQETIClient", 
                                  QString("Frame parsing exception: %1").arg(e.what()));
            emit frameError(QString("Parsing exception: %1").arg(e.what()), 
                           frame_sequence_.load());
        }
    }
    
    frame_sequence_.fetch_add(1);
}

bool ZeroMQETIClient::validateETIFrame(const QByteArray& frame_data) const
{
    // Basic validation: ETI frame must be exactly 6144 bytes
    if (frame_data.size() != static_cast<int>(eti::ETI_FRAME_SIZE)) {
        return false;
    }
    
    // Check ETI sync pattern
    const uint8_t* data = reinterpret_cast<const uint8_t*>(frame_data.constData());
    const uint8_t expected_sync[4] = {0x49, 0x93, 0x1E, 0x03};
    
    return std::memcmp(data, expected_sync, 4) == 0;
}

void ZeroMQETIClient::updateConnectionStatus(ConnectionStatus::State new_state, 
                                            const QString& error_message)
{
    QMutexLocker locker(&stats_mutex_);
    
    ConnectionStatus::State old_state = connection_status_.state;
    connection_status_.state = new_state;
    connection_status_.error_message = error_message;
    connection_status_.last_update = std::chrono::system_clock::now();
    
    if (new_state == ConnectionStatus::State::ERROR) {
        connection_status_.connection_errors++;
        
        // Start auto-reconnect if enabled
        if (auto_reconnect_enabled_) {
            startReconnectTimer();
        }
    }
    
    locker.unlock();
    
    // Emit signal if state changed
    if (old_state != new_state) {
        emit connectionStatusChanged(connection_status_);
        
        Logger::instance().log(Logger::Info, "ZeroMQETIClient", 
                              QString("Connection state changed: %1 -> %2%3")
                              .arg(static_cast<int>(old_state))
                              .arg(static_cast<int>(new_state))
                              .arg(error_message.isEmpty() ? "" : QString(" (%1)").arg(error_message)));
    }
}

void ZeroMQETIClient::startReconnectTimer()
{
    if (reconnect_timer_ && !reconnect_timer_->isActive()) {
        reconnect_timer_->start(reconnect_interval_ms_);
    }
}

void ZeroMQETIClient::stopReconnectTimer()
{
    if (reconnect_timer_) {
        reconnect_timer_->stop();
    }
}

void ZeroMQETIClient::updateReceiveStats()
{
    // This is called periodically to update receive rate calculations
    QMutexLocker locker(&stats_mutex_);
    
    auto now = std::chrono::system_clock::now();
    
    // Calculate receive rate over last second
    auto one_second_ago = now - std::chrono::seconds(1);
    
    auto recent_count = std::count_if(recent_frame_times_.begin(), 
                                     recent_frame_times_.end(),
                                     [one_second_ago](const auto& time) {
                                         return time >= one_second_ago;
                                     });
    
    connection_status_.receive_rate_fps = static_cast<double>(recent_count);
    
    // Calculate average latency (simplified - would need more sophisticated measurement)
    if (!recent_frame_times_.empty()) {
        auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(
            recent_frame_times_.back() - recent_frame_times_.front());
        
        if (recent_frame_times_.size() > 1) {
            connection_status_.avg_frame_latency = 
                std::chrono::microseconds(total_duration.count() / (recent_frame_times_.size() - 1));
        }
    }
}

void ZeroMQETIClient::updatePerformanceMetrics()
{
    updateReceiveStats();
    
    QMutexLocker locker(&stats_mutex_);
    
    // Update performance stats structure
    performance_stats_.total_frames_received = total_frames_received_.load();
    performance_stats_.total_bytes_received = total_bytes_received_.load();
    performance_stats_.current_fps = connection_status_.receive_rate_fps;
    performance_stats_.avg_latency = connection_status_.avg_frame_latency;
    performance_stats_.frame_errors = connection_status_.frame_errors;
    performance_stats_.connection_errors = connection_status_.connection_errors;
    
    // Calculate average FPS over entire session
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now() - start_time_);
    
    if (elapsed.count() > 0) {
        performance_stats_.average_fps = 
            static_cast<double>(performance_stats_.total_frames_received) / elapsed.count();
    }
    
    // Calculate uptime percentage
    auto total_time = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now() - start_time_);
    auto connected_time = connection_status_.getConnectionDuration();
    
    if (total_time.count() > 0) {
        performance_stats_.uptime_percentage = 
            static_cast<double>(connected_time.count()) / total_time.count() * 100.0;
    }
    
    locker.unlock();
    
    // Emit performance update
    emit performanceUpdate(performance_stats_);
}

// ============================================================================
// ZMQWorkerThread Implementation
// ============================================================================

ZeroMQETIClient::ZMQWorkerThread::ZMQWorkerThread(ZeroMQETIClient* parent)
    : QThread(parent), client_(parent)
{
}

ZeroMQETIClient::ZMQWorkerThread::~ZMQWorkerThread()
{
    stop();
    wait();
}

void ZeroMQETIClient::ZMQWorkerThread::run()
{
    if (!socket_) {
        Logger::instance().log(Logger::Error, "ZMQWorkerThread", 
                              "No socket available for reception");
        return;
    }
    
    Logger::instance().log(Logger::Info, "ZMQWorkerThread", 
                          "Worker thread started");
    
    while (!stop_flag_.load()) {
        zmq_msg_t message;
        int result = zmq_msg_init(&message);
        if (result != 0) {
            continue;
        }
        
        // Receive message with timeout
        result = zmq_msg_recv(&message, socket_, ZMQ_DONTWAIT);
        
        if (result == -1) {
            int error_code = zmq_errno();
            if (error_code == EAGAIN) {
                // No message available, continue polling
                zmq_msg_close(&message);
                msleep(1); // Sleep 1ms to avoid busy waiting
                continue;
            } else {
                // Real error occurred
                Logger::instance().log(Logger::Error, "ZMQWorkerThread", 
                                      QString("Receive error: %1")
                                      .arg(zmq_utils::getZMQErrorString(error_code)));
                zmq_msg_close(&message);
                break;
            }
        }
        
        // Process received message
        size_t msg_size = zmq_msg_size(&message);
        void* msg_data = zmq_msg_data(&message);
        
        if (msg_size == eti::ETI_FRAME_SIZE) {
            QByteArray frame_data(static_cast<const char*>(msg_data), 
                                 static_cast<int>(msg_size));
            
            // Process frame in main thread context
            QMetaObject::invokeMethod(client_, [this, frame_data]() {
                client_->processReceivedFrame(frame_data);
            }, Qt::QueuedConnection);
        } else {
            Logger::instance().log(Logger::Warning, "ZMQWorkerThread", 
                                  QString("Received frame with invalid size: %1 bytes (expected %2)")
                                  .arg(msg_size).arg(eti::ETI_FRAME_SIZE));
        }
        
        zmq_msg_close(&message);
    }
    
    Logger::instance().log(Logger::Info, "ZMQWorkerThread", 
                          "Worker thread stopped");
}

void ZeroMQETIClient::ZMQWorkerThread::stop()
{
    stop_flag_.store(true);
}

// ============================================================================
// Utility Functions
// ============================================================================

namespace zmq_utils {

ZMQConnectionConfig parseEndpointUrl(const QString& endpoint)
{
    ZMQConnectionConfig config;
    
    QUrl url(endpoint);
    QString scheme = url.scheme().toLower();
    
    if (scheme == "tcp") {
        config.transport_type = ZMQConnectionConfig::TransportType::TCP;
        config.host = url.host();
        config.port = static_cast<quint16>(url.port(9200)); // Default port
    } else if (scheme == "ipc") {
        config.transport_type = ZMQConnectionConfig::TransportType::IPC;
        config.socket_path = url.path();
    } else if (scheme == "inproc") {
        config.transport_type = ZMQConnectionConfig::TransportType::INPROC;
        config.endpoint_name = url.path().mid(1); // Remove leading '/'
    }
    
    return config;
}

bool validateEndpointUrl(const QString& endpoint)
{
    static const QRegularExpression tcp_pattern(R"(^tcp://[\w\.-]+:\d+$)");
    static const QRegularExpression ipc_pattern(R"(^ipc://[/\w\.-]+$)");
    static const QRegularExpression inproc_pattern(R"(^inproc://\w+$)");
    
    return tcp_pattern.match(endpoint).hasMatch() ||
           ipc_pattern.match(endpoint).hasMatch() ||
           inproc_pattern.match(endpoint).hasMatch();
}

QString getZMQErrorString(int error_code)
{
    return QString::fromUtf8(zmq_strerror(error_code));
}

bool testEndpointConnectivity(const QString& endpoint, int timeout_ms)
{
    // Create temporary context and socket for testing
    void* test_context = zmq_ctx_new();
    if (!test_context) {
        return false;
    }
    
    void* test_socket = zmq_socket(test_context, ZMQ_SUB);
    if (!test_socket) {
        zmq_ctx_destroy(test_context);
        return false;
    }
    
    // Set short timeout for testing
    zmq_setsockopt(test_socket, ZMQ_RCVTIMEO, &timeout_ms, sizeof(timeout_ms));
    zmq_setsockopt(test_socket, ZMQ_SUBSCRIBE, "", 0);
    
    // Attempt connection
    int result = zmq_connect(test_socket, endpoint.toUtf8().constData());
    bool success = (result == 0);
    
    // Cleanup
    zmq_close(test_socket);
    zmq_ctx_destroy(test_context);
    
    return success;
}

} // namespace zmq_utils

} // namespace eti::network