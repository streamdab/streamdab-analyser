/**
 * @file network_stream_receiver.h
 * @brief Professional Network Stream Receiver for ETI-over-IP multicast capture
 * 
 * Implements high-performance, real-time ETI stream reception from network sources
 * with professional broadcast-grade reliability and error handling.
 * Supports multicast UDP reception with automatic failover and quality monitoring.
 * 
 * @author Network/Stream Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 */

#ifndef NETWORK_STREAM_RECEIVER_H
#define NETWORK_STREAM_RECEIVER_H

#include <QObject>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QNetworkInterface>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <QQueue>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <vector>
#include <string>

#include "eti_types.hpp"

namespace eti {

/**
 * @brief Wire transport used to receive ETI-over-IP.
 *
 * The scheme in the user-facing URL selects the transport:
 *   - `udp://` or plain `A.B.C.D[:port]` -> UdpMulticast
 *   - `tcp://`                           -> Tcp  (raw 6144-byte ETI stream)
 *   - `zmq+tcp://`                       -> Zmq  (SUB to an ODR-DabMux publisher)
 */
enum class StreamTransport {
    UdpMulticast = 0,
    Tcp = 1,
    Zmq = 2
};

/**
 * @brief Human-readable transport name (for logs / error messages).
 */
const char* stream_transport_name(StreamTransport transport);

/**
 * @brief Result of validating/parsing an ETI-over-IP stream URL.
 *
 * `address`/`port` are only meaningful when `valid == true`. On failure
 * `error` carries a single, user-presentable reason.
 */
struct StreamUrlValidation {
    bool valid{false};
    StreamTransport transport{StreamTransport::UdpMulticast};
    QString address;             ///< IPv4 multicast address, or TCP/ZMQ host (valid only)
    quint16 port{9200};          ///< 1..65535, defaults to 9200 when omitted
    QString error;               ///< reason when `valid == false`
};

/**
 * @brief Validate an ETI-over-IP stream URL and resolve it to a transport.
 *
 * Accepted grammar (the ONLY accepted forms):
 *   - `udp://A.B.C.D[:port]`   IPv4 multicast (224.0.0.0/4)
 *   - `A.B.C.D[:port]`         IPv4 multicast (224.0.0.0/4)
 *   - `tcp://host[:port]`      raw ETI byte stream (hostname or IPv4 literal)
 *   - `zmq+tcp://host[:port]`  ZeroMQ SUB to an ODR-DabMux ETI publisher
 *
 * For every form the port is optional and defaults to 9200; when present it
 * must be 1..65535 with digits only. `address` receives the canonical
 * dotted-decimal multicast address (UDP) or the verbatim host (TCP/ZMQ).
 *
 * Rejected with a descriptive `error`: empty/blank input, `http://` and any
 * other scheme, hostnames without a transport scheme (ambiguous), unicast
 * UDP, IPv6, malformed paths and malformed addresses/ports.
 *
 * This is a pure free function (no I/O) so both the UI and the receiver can
 * call it before creating any worker thread.
 */
StreamUrlValidation validate_stream_url(const QString& url);

/**
 * @brief True when `address` is a valid IPv4 multicast address (224.0.0.0/4).
 */
bool is_valid_multicast_address(const QString& address);

/**
 * @brief Strict IPv4 + multicast canonicaliser.
 *
 * Unlike QHostAddress::setAddress (which accepts short/hex/octal forms such as
 * `224.0.0` or `0xE0.0.0.1`), this requires a strict 4-octet dotted-decimal
 * address (each octet 0-255, no leading zeros) inside 224.0.0.0/4. On success
 * `canonicalOut` receives `QHostAddress(raw).toString()`.
 */
bool canonical_multicast_address(const QString& input, QString& canonicalOut);

/**
 * @brief Inter-frame arrival latency tracker.
 *
 * Tracks the interval between consecutive received ETI frames (a real,
 * bounded latency proxy) instead of the old "now - stream start" uptime, which
 * grew without bound and tripped the latency threshold on every healthy
 * stream. The average is only considered valid after MIN_SAMPLES samples so
 * quality checks never fire before data flows.
 */
class InterFrameLatencyTracker {
public:
    static constexpr size_t MIN_SAMPLES = 10;

    void reset()
    {
        m_has_last = false;
        m_count = 0;
        m_sum_us = 0;
        m_max_us = 0;
    }

    /**
     * @brief Record a frame arrival. Returns true once a valid average exists.
     */
    bool addSample(std::chrono::steady_clock::time_point now)
    {
        bool validNow = false;
        if (m_has_last) {
            const int64_t interval_us = std::chrono::duration_cast<std::chrono::microseconds>(
                                            now - m_last).count();
            if (interval_us >= 0) {
                m_sum_us += interval_us;
                if (interval_us > m_max_us) {
                    m_max_us = interval_us;
                }
                ++m_count;
                validNow = m_count >= MIN_SAMPLES;
            }
        }
        m_last = now;
        m_has_last = true;
        return validNow;
    }

    bool valid() const { return m_count >= MIN_SAMPLES; }
    size_t sampleCount() const { return m_count; }
    int64_t averageUs() const { return m_count > 0 ? m_sum_us / static_cast<int64_t>(m_count) : 0; }
    int64_t maxUs() const { return m_max_us; }

private:
    std::chrono::steady_clock::time_point m_last{};
    bool m_has_last{false};
    size_t m_count{0};
    int64_t m_sum_us{0};
    int64_t m_max_us{0};
};

/**
 * @brief Network stream configuration for ETI-over-IP reception
 */
struct NetworkStreamConfig {
    /// Selected wire transport. Determines how `multicast_address` is used:
    /// UDP -> IPv4 multicast group; TCP/ZMQ -> target host name or IP literal.
    StreamTransport transport = StreamTransport::UdpMulticast;
    QString multicast_address = "239.192.0.1";  // Default DAB multicast (or TCP/ZMQ host)
    quint16 port = 9200;                        // Default ETI port
    QString interface_name;                      // Network interface (auto if empty)
    size_t buffer_size = 1000;                  // Frame buffer size
    std::chrono::milliseconds timeout{5000};    // Reception timeout
    bool enable_flow_control = true;            // Enable flow control
    bool auto_reconnect = true;                 // Automatic reconnection
    size_t max_reconnect_attempts = 10;         // Maximum reconnection attempts
    std::chrono::milliseconds reconnect_delay{1000}; // Delay between reconnections
    
    // Quality thresholds
    double min_frame_rate = 200.0;              // Minimum acceptable frame rate
    double max_frame_loss = 0.01;               // Maximum acceptable frame loss (1%)
    std::chrono::milliseconds max_latency{100}; // Maximum acceptable latency
};

/**
 * @brief Copyable network stream statistics snapshot
 */
struct NetworkStreamStatisticsSnapshot {
    size_t frames_received{0};
    size_t frames_processed{0};
    size_t frames_dropped{0};
    size_t bytes_received{0};
    size_t network_errors{0};
    size_t buffer_overflows{0};
    size_t buffer_underflows{0};
    
    double current_frame_rate{0.0};
    double average_frame_rate{0.0};
    double frame_loss_rate{0.0};
    int64_t average_latency_us{0};
    int64_t max_latency_us{0};
    
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point last_update;
};

/**
 * @brief Real-time network stream statistics (atomic for thread safety)
 */
struct NetworkStreamStatistics {
    std::atomic<size_t> frames_received{0};
    std::atomic<size_t> frames_processed{0};
    std::atomic<size_t> frames_dropped{0};
    std::atomic<size_t> bytes_received{0};
    std::atomic<size_t> network_errors{0};
    std::atomic<size_t> buffer_overflows{0};
    std::atomic<size_t> buffer_underflows{0};
    
    std::atomic<double> current_frame_rate{0.0};
    std::atomic<double> average_frame_rate{0.0};
    std::atomic<double> frame_loss_rate{0.0};
    std::atomic<int64_t> average_latency_us{0};
    std::atomic<int64_t> max_latency_us{0};
    
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point last_update;

    // Default-constructible + copyable VALUE type. The atomics make the
    // implicit copy operations deleted, but the worker->GUI `statistics_updated`
    // signal crosses a queued (cross-thread) connection, which REQUIRES a
    // copyable metatype. Without these, Qt delivers a null reference to
    // handle_worker_statistics() and the app segfaults on the first statistics
    // tick after reception starts. Copying loads each atomic (a consistent
    // per-field snapshot) — thread-safe because it never mutates the source.
    NetworkStreamStatistics() = default;
    NetworkStreamStatistics(const NetworkStreamStatistics& other)
        : frames_received(other.frames_received.load())
        , frames_processed(other.frames_processed.load())
        , frames_dropped(other.frames_dropped.load())
        , bytes_received(other.bytes_received.load())
        , network_errors(other.network_errors.load())
        , buffer_overflows(other.buffer_overflows.load())
        , buffer_underflows(other.buffer_underflows.load())
        , current_frame_rate(other.current_frame_rate.load())
        , average_frame_rate(other.average_frame_rate.load())
        , frame_loss_rate(other.frame_loss_rate.load())
        , average_latency_us(other.average_latency_us.load())
        , max_latency_us(other.max_latency_us.load())
        , start_time(other.start_time)
        , last_update(other.last_update)
    {}
    NetworkStreamStatistics& operator=(const NetworkStreamStatistics& other)
    {
        if (this == &other) {
            return *this;
        }
        frames_received.store(other.frames_received.load());
        frames_processed.store(other.frames_processed.load());
        frames_dropped.store(other.frames_dropped.load());
        bytes_received.store(other.bytes_received.load());
        network_errors.store(other.network_errors.load());
        buffer_overflows.store(other.buffer_overflows.load());
        buffer_underflows.store(other.buffer_underflows.load());
        current_frame_rate.store(other.current_frame_rate.load());
        average_frame_rate.store(other.average_frame_rate.load());
        frame_loss_rate.store(other.frame_loss_rate.load());
        average_latency_us.store(other.average_latency_us.load());
        max_latency_us.store(other.max_latency_us.load());
        start_time = other.start_time;
        last_update = other.last_update;
        return *this;
    }
    
    void reset() {
        frames_received = 0;
        frames_processed = 0;
        frames_dropped = 0;
        bytes_received = 0;
        network_errors = 0;
        buffer_overflows = 0;
        buffer_underflows = 0;
        current_frame_rate = 0.0;
        average_frame_rate = 0.0;
        frame_loss_rate = 0.0;
        average_latency_us = 0;
        max_latency_us = 0;
        start_time = std::chrono::steady_clock::now();
        last_update = start_time;
    }
};

/**
 * @brief Network packet container for ETI frame data
 */
struct NetworkPacket {
    std::vector<uint8_t> data;
    std::chrono::steady_clock::time_point timestamp;
    size_t sequence_number = 0;
    QString source_address;
    quint16 source_port = 0;
    bool is_valid = true;
    size_t expected_size = 6144;  // ETI frame size constant
    
    bool contains_eti_frame() const {
        return data.size() >= 6144 && is_valid;
    }
};

/**
 * @brief High-performance network stream receiver worker thread
 */
class NetworkStreamWorker : public QObject {
    Q_OBJECT

public:
    explicit NetworkStreamWorker(const NetworkStreamConfig& config, QObject* parent = nullptr);
    ~NetworkStreamWorker();

    // --- Test seams (T22 review F1) ----------------------------------------
    // Drive the fatal-socket-error path deterministically without real
    // multicast hardware: force the "connected" state, then deliver a fatal
    // socket error and observe the single connection_status_changed(false).
    void forceConnectedForTest()
    {
        m_receiving = true;
        m_connected = true;
    }
    void simulateFatalSocketErrorForTest()
    {
        handle_socket_error(QAbstractSocket::NetworkError);
    }
    bool isReceivingForTest() const { return m_receiving.load(); }
    bool isConnectedForTest() const { return m_connected.load(); }
    /// The frame-buffer size the worker snapshots at construction. Diagnostic /
    /// test read: the worker's config is written once at construction and never
    /// mutated afterwards, so this is race-free.
    size_t buffer_size_for_test() const { return m_config.buffer_size; }

public slots:
    void start_reception();
    void stop_reception();
    void process_pending_datagrams();
    void update_statistics();
    /// Tear down every transport socket in the worker's OWN thread. Called by
    /// the receiver with a bounded queued invocation before the thread quits
    /// because ZMQ sockets are not thread-safe: creating and destroying them
    /// must happen on the same thread.
    ///
    /// LOSS-SILENT: unlike `stop_reception()` this does NOT emit
    /// `connection_status_changed(false)` — the receiver initiated the teardown
    /// and must not mistake it for a stream loss (review F1).
    void shutdown();

signals:
    void frame_received(const QByteArray& frameData);
    void packet_received(const eti::NetworkPacket& packet);
    void error_occurred(const QString& error);
    void statistics_updated(const eti::NetworkStreamStatistics& stats);
    void connection_status_changed(bool connected);

private slots:
    void handle_socket_error(QAbstractSocket::SocketError error);
    void handle_datagram_ready();
    // TCP
    void handle_tcp_connected();
    void handle_tcp_disconnected();
    void handle_tcp_ready_read();
    void handle_tcp_error(QAbstractSocket::SocketError error);
    // ZeroMQ
    void poll_zmq();

private:
    /// Shared stop path. `emitStatus == false` is used by the intentional
    /// teardown (`shutdown()`): the receiver owns the connection lifecycle, and
    /// a stop it deliberately initiated must never be relayed as a stream loss
    /// (that used to retrigger the reconnect policy — review F1).
    void stop_reception_internal(bool emitStatus);
    bool initialize_socket();
    bool join_multicast_group();
    void leave_multicast_group();
    bool initialize_tcp_socket();
    bool initialize_zmq_socket();
    void reframe_tcp_buffer();
    void deliver_eti_frame(const QByteArray& frame, const QString& sourceAddress,
                           quint16 sourcePort);
    void report_socket_error(QAbstractSocket::SocketError error,
                             const QString& errorString, bool fatal);
    bool validate_eti_packet(const QByteArray& data) const;
    void update_frame_rate();
    void check_quality_thresholds();

    // ZeroMQ state is hidden behind a pimpl so <zmq.hpp> stays out of the
    // header (and therefore out of every translation unit that includes it).
    struct ZmqState;
    
    NetworkStreamConfig m_config;
    QString m_last_error;  ///< last start/init failure (worker thread only)
    std::unique_ptr<QUdpSocket> m_socket;
    std::unique_ptr<QTcpSocket> m_tcp_socket;   ///< TCP raw ETI stream
    QByteArray m_tcp_buffer;                    ///< TCP re-framing accumulator
    std::unique_ptr<ZmqState> m_zmq;            ///< ZMQ context + SUB socket
    std::unique_ptr<QTimer> m_zmq_poll_timer;   ///< drives zmq_poll (never blocks)
    std::unique_ptr<QTimer> m_statistics_timer;
    NetworkStreamStatistics m_statistics;
    QMutex m_statistics_mutex;
    static constexpr int ZMQ_POLL_INTERVAL_MS = 5;
    
    std::atomic<bool> m_receiving{false};
    std::atomic<bool> m_connected{false};
    std::atomic<size_t> m_sequence_number{0};
    
    QQueue<NetworkPacket> m_packet_queue;
    QMutex m_queue_mutex;
    QWaitCondition m_queue_condition;
    
    std::chrono::steady_clock::time_point m_last_frame_time;
    std::vector<double> m_frame_rate_history;
    InterFrameLatencyTracker m_latency;
    static constexpr size_t FRAME_RATE_HISTORY_SIZE = 100;
};

/**
 * @brief Professional Network Stream Receiver for ETI-over-IP
 * 
 * Thread-safe, high-performance receiver for UDP multicast, TCP and ZeroMQ
 * ETI streams with:
 * - Real-time frame processing (>900 frames/second capability)
 * - Automatic failover and reconnection
 * - Quality monitoring and threshold enforcement
 * - Buffer management with overflow/underflow detection
 * - Professional broadcast-grade error handling
 */
class NetworkStreamReceiver : public QObject {
    Q_OBJECT

public:
    explicit NetworkStreamReceiver(QObject* parent = nullptr);
    explicit NetworkStreamReceiver(const NetworkStreamConfig& config, QObject* parent = nullptr);
    ~NetworkStreamReceiver();

    // Connection management
    bool connect_to_stream(const QString& multicast_address, quint16 port);
    bool connect_with_config(const NetworkStreamConfig& config);
    void disconnect_from_stream();
    bool is_connected() const;
    bool is_receiving() const;

    // Configuration
    void set_config(const NetworkStreamConfig& config);
    NetworkStreamConfig get_config() const;
    void set_buffer_size(size_t buffer_frames);
    void set_quality_thresholds(double min_frame_rate, double max_frame_loss, 
                               std::chrono::milliseconds max_latency);

    // Stream control
    void start_reception();
    void stop_reception();
    void pause_reception();
    void resume_reception();

    // Statistics and monitoring
    NetworkStreamStatisticsSnapshot get_statistics() const;
    double get_current_frame_rate() const;
    double get_average_frame_rate() const;
    double get_frame_loss_rate() const;
    std::chrono::microseconds get_average_latency() const;
    std::chrono::microseconds get_max_latency() const;
    bool is_quality_acceptable() const;

    // Error handling
    QString get_last_error() const;

    // --- Test seams (T22 review F1) ----------------------------------------
    // Drive the receiver's connection-loss/reconnect policy without a live
    // worker: force the "receiving" flag, then feed a connection-status event.
    void setReceivingForTest(bool receiving) { m_receiving = receiving; }
    void simulateConnectionLostForTest() { handle_connection_status(false); }
    // T43: drive the confirmed-connection transition (and the GUI button state
    // it owns) without real multicast traffic.
    void simulateConnectionEstablishedForTest() { handle_connection_status(true); }
    size_t reconnectionAttemptsForTest() const { return m_reconnection_attempts.load(); }
    /// The frame-buffer size the live worker was constructed with (0 when no
    /// worker exists). See NetworkStreamWorker::buffer_size_for_test().
    size_t worker_buffer_size_for_test() const
    {
        return m_worker ? m_worker->buffer_size_for_test() : 0;
    }

    // Callback registration for real-time processing
    using FrameCallback = std::function<void(const QByteArray&)>;
    using PacketCallback = std::function<void(const NetworkPacket&)>;
    using ErrorCallback = std::function<void(const QString&)>;
    using StatisticsCallback = std::function<void(const NetworkStreamStatistics&)>;

    void set_frame_callback(FrameCallback callback);
    void set_packet_callback(PacketCallback callback);
    void set_error_callback(ErrorCallback callback);
    void set_statistics_callback(StatisticsCallback callback);

    // Advanced features
    void enable_automatic_reconnection(bool enabled, size_t max_attempts = 10);

signals:
    void frame_received(const QByteArray& frameData);
    void packet_received(const eti::NetworkPacket& packet);
    void error_occurred(const QString& error);
    void statistics_updated(const eti::NetworkStreamStatistics& stats);
    void connection_status_changed(bool connected);
    void quality_threshold_violated(const QString& violation);
    void reconnection_attempted(int attempt_number);
    void buffer_overflow_detected();
    void buffer_underflow_detected();

private slots:
    void handle_worker_frame(const QByteArray& frameData);
    void handle_worker_packet(const eti::NetworkPacket& packet);
    void handle_worker_error(const QString& error);
    void handle_worker_statistics(const eti::NetworkStreamStatistics& stats);
    void handle_connection_status(bool connected);
    void attempt_reconnection();

private:
    void initialize_worker_thread();
    void cleanup_worker_thread();
    void validate_config(const NetworkStreamConfig& config);
    void setup_reconnection_timer();
    void check_quality_violations(const NetworkStreamStatistics& stats);

    NetworkStreamConfig m_config;
    std::unique_ptr<QThread> m_worker_thread;
    std::unique_ptr<NetworkStreamWorker> m_worker;
    
    mutable QMutex m_mutex;
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_receiving{false};
    std::atomic<bool> m_paused{false};
    
    NetworkStreamStatistics m_current_statistics;
    QStringList m_error_messages;
    
    std::unique_ptr<QTimer> m_reconnection_timer;
    std::atomic<size_t> m_reconnection_attempts{0};
    
    // Frame and packet queues for buffering
    // Note: Frame queue disabled to avoid EtiFrame type visibility issues with MOC
    // Frames are delivered via signals instead
    // QQueue<EtiFrame> m_frame_queue;
    QQueue<NetworkPacket> m_packet_queue;
    QMutex m_queue_mutex;
    
    // Callbacks for real-time processing
    FrameCallback m_frame_callback;
    PacketCallback m_packet_callback;
    ErrorCallback m_error_callback;
    StatisticsCallback m_statistics_callback;
    
    static constexpr size_t DEFAULT_BUFFER_SIZE = 1000;
    static constexpr size_t MAX_BUFFER_SIZE = 10000;
    static constexpr size_t MAX_ERROR_MESSAGES = 100;
    /// Bounded wait for the worker-thread teardown invoke (review F3).
    static constexpr int SHUTDOWN_TIMEOUT_MS = 5000;
};

/**
 * @brief Factory for creating pre-configured network receivers
 */
class NetworkStreamReceiverFactory {
public:
    /**
     * @brief Create receiver for standard DAB ETI multicast
     */
    static std::unique_ptr<NetworkStreamReceiver> create_dab_multicast_receiver(
        const QString& multicast_address = "239.192.0.1",
        quint16 port = 9200);

    /**
     * @brief Create high-performance receiver for professional broadcasting
     */
    static std::unique_ptr<NetworkStreamReceiver> create_professional_receiver(
        const NetworkStreamConfig& config);

    /**
     * @brief Create receiver with automatic failover configuration
     */
    static std::unique_ptr<NetworkStreamReceiver> create_failover_receiver(
        const QStringList& multicast_addresses,
        quint16 port = 9200);

    /**
     * @brief Create receiver optimized for low-latency applications
     */
    static std::unique_ptr<NetworkStreamReceiver> create_low_latency_receiver(
        const QString& multicast_address,
        quint16 port = 9200);
};

} // namespace eti

// Queued (cross-thread) signal transports for the receiver: these MUST be
// registered metatypes or Qt delivers a null reference and the GUI segfaults.
// NetworkStreamStatistics is now a copyable snapshot value type (above).
Q_DECLARE_METATYPE(eti::NetworkStreamStatistics)
Q_DECLARE_METATYPE(eti::NetworkPacket)
Q_DECLARE_METATYPE(eti::NetworkStreamStatisticsSnapshot)

#endif // NETWORK_STREAM_RECEIVER_H
