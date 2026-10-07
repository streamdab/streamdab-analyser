/**
 * @file network_stream_receiver.cpp
 * @brief Implementation of Professional Network Stream Receiver
 * 
 * High-performance, thread-safe ETI-over-IP multicast receiver with
 * professional broadcast-grade reliability and real-time processing.
 */

#include "network_stream_receiver.hpp"
#include <QNetworkDatagram>
#include <QHostAddress>
#include <QNetworkProxy>
#include <QCoreApplication>
#include <QDebug>
#include <QSemaphore>
#include <algorithm>
#include <cstring>
#include <numeric>
#include <optional>

// ZeroMQ (cppzmq) is OPTIONAL: HAVE_ZMQ is defined by CMake only when both
// <zmq.hpp> and libzmq were found (see the CMakeLists.txt detection ladder).
// Without it the zmq+tcp:// transport is compiled out entirely.
#ifdef HAVE_ZMQ
#include <zmq.hpp>
#endif

namespace eti {

// ============================================================================
// Stream URL validation (shared by the UI pre-check and the receiver)
// ============================================================================
namespace {

// Strict 4-octet dotted-decimal IPv4 parser. Rejects short/hex/octal forms
// (e.g. "224.0.0", "0xE0.0.0.1", "224.0.0.010") that QHostAddress accepts.
bool parse_strict_ipv4(const QString& text, quint32& rawOut) {
    const QStringList parts = text.split(QLatin1Char('.'));
    if (parts.size() != 4) {
        return false;
    }
    quint32 raw = 0;
    for (const QString& part : parts) {
        if (part.isEmpty() || part.size() > 3) {
            return false;
        }
        for (const QChar c : part) {
            if (!c.isDigit()) {
                return false;  // digits only: rejects "+1", "0x..", spaces
            }
        }
        if (part.size() > 1 && part.at(0) == QLatin1Char('0')) {
            return false;  // no leading zeros ("010" must not become octal 8)
        }
        bool ok = false;
        const int octet = part.toInt(&ok);
        if (!ok || octet < 0 || octet > 255) {
            return false;
        }
        raw = (raw << 8) | static_cast<quint32>(octet);
    }
    rawOut = raw;
    return true;
}

// TCP / ZMQ host: a DNS name or IPv4 literal. Deliberately ASCII-only and
// conservative (letters/digits plus `.`, `-`, `_`) so shell/URL metacharacters
// and whitespace can never reach QTcpSocket / libzmq.
bool is_valid_host_name(const QString& host) {
    if (host.isEmpty() || host.size() > 253) {
        return false;
    }
    if (host.startsWith(QLatin1Char('.')) || host.endsWith(QLatin1Char('.'))) {
        return false;
    }
    for (const QChar c : host) {
        const ushort u = c.unicode();
        const bool asciiAlnum =
            (u >= '0' && u <= '9') || (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z');
        if (!asciiAlnum && u != '.' && u != '-' && u != '_') {
            return false;
        }
    }
    return true;
}

// Strict digits-only port in 1..65535. On failure `error` receives the reason.
bool parse_port(const QString& portText, quint16& portOut, QString& error) {
    if (portText.size() > 5) {
        error = QStringLiteral("Invalid port '%1'. Use a number between 1 and "
                               "65535 (default 9200).").arg(portText);
        return false;
    }
    for (const QChar c : portText) {
        if (!c.isDigit()) {
            error = QStringLiteral("Invalid port '%1'. Use digits only, between "
                                   "1 and 65535 (default 9200).").arg(portText);
            return false;
        }
    }
    if (portText.size() > 1 && portText.at(0) == QLatin1Char('0')) {
        error = QStringLiteral("Invalid port '%1'. Leading zeros are not allowed "
                               "(use e.g. '5', not '0005').").arg(portText);
        return false;
    }
    bool ok = false;
    const uint value = portText.toUInt(&ok);
    if (!ok || value == 0 || value > 65535) {
        error = QStringLiteral("Invalid port '%1'. Use a number between 1 and "
                               "65535 (default 9200).").arg(portText);
        return false;
    }
    portOut = static_cast<quint16>(value);
    return true;
}

// True when the 4 bytes at `offset` are one of the accepted ETI sync words.
bool is_eti_sync_at(const QByteArray& data, int offset) {
    if (offset < 0 || offset + 4 > data.size()) {
        return false;
    }
    const uint8_t* p = reinterpret_cast<const uint8_t*>(data.constData()) + offset;
    return std::memcmp(p, ETI_SYNC_PATTERN.data(), 4) == 0
        || std::memcmp(p, ETI_SYNC_LI_A.data(), 4) == 0
        || std::memcmp(p, ETI_SYNC_LI_B.data(), 4) == 0;
}

}  // namespace

const char* stream_transport_name(StreamTransport transport) {
    switch (transport) {
        case StreamTransport::UdpMulticast: return "UDP multicast";
        case StreamTransport::Tcp:          return "TCP";
        case StreamTransport::Zmq:          return "ZeroMQ";
    }
    return "unknown";
}

bool canonical_multicast_address(const QString& input, QString& canonicalOut) {
    quint32 raw = 0;
    if (!parse_strict_ipv4(input, raw)) {
        return false;
    }
    // 224.0.0.0/4 -> top nibble 0xE.
    if ((raw & 0xF0000000u) != 0xE0000000u) {
        return false;
    }
    canonicalOut = QHostAddress(raw).toString();
    return true;
}

bool is_valid_multicast_address(const QString& address) {
    QString canonical;
    return canonical_multicast_address(address, canonical);
}

StreamUrlValidation validate_stream_url(const QString& rawUrl) {
    StreamUrlValidation result;
    const QString url = rawUrl.trimmed();
    if (url.isEmpty()) {
        result.error = QStringLiteral(
            "Please enter a stream URL (e.g. udp://239.192.0.1:9200, "
            "tcp://host:9200 or zmq+tcp://host:9201).");
        return result;
    }

    // Optional scheme prefix selects the transport. A bare address (no scheme)
    // is only valid as UDP multicast, as before.
    QString hostPort = url;
    StreamTransport transport = StreamTransport::UdpMulticast;
    const int schemeSep = url.indexOf(QStringLiteral("://"));
    if (schemeSep >= 0) {
        const QString scheme = url.left(schemeSep).toLower();
        if (scheme == QLatin1String("udp")) {
            transport = StreamTransport::UdpMulticast;
        } else if (scheme == QLatin1String("tcp")) {
            transport = StreamTransport::Tcp;
        } else if (scheme == QLatin1String("zmq+tcp")) {
#ifdef HAVE_ZMQ
            transport = StreamTransport::Zmq;
#else
            // Reject early and clearly: silently accepting would create a
            // receiver whose start_reception() can only fail later.
            result.error = QStringLiteral(
                "ZeroMQ support not built in: this build does not include the "
                "zmq+tcp:// transport. Rebuild with the ZeroMQ/cppzmq "
                "libraries, or use udp:// (multicast) or tcp:// (raw ETI).");
            return result;
#endif
        } else {
            result.error = QStringLiteral(
                "Unsupported URL scheme '%1://'. Supported: udp:// (IPv4 "
                "multicast), tcp:// (raw ETI stream), zmq+tcp:// (ODR-DabMux "
                "ZeroMQ).").arg(scheme);
            return result;
        }
        hostPort = url.mid(schemeSep + 3);
    } else if (url.contains(QLatin1Char('/'))) {
        result.error = QStringLiteral(
            "Malformed URL '%1'. Supported formats: udp://239.192.0.1:9200, "
            "tcp://host:9200 or zmq+tcp://host:9201.").arg(url);
        return result;
    }

    // A path component is never part of an ETI endpoint.
    if (hostPort.contains(QLatin1Char('/'))) {
        result.error = QStringLiteral(
            "Malformed URL '%1'. A host:port is expected, not a path.").arg(url);
        return result;
    }

    // Split host[:port]. More than one colon means IPv6 or malformed input,
    // which this IPv4-only receiver does not support.
    QString host = hostPort;
    QString portText;
    const int colon = hostPort.lastIndexOf(QLatin1Char(':'));
    if (colon >= 0) {
        if (hostPort.indexOf(QLatin1Char(':')) != colon) {
            result.error = QStringLiteral(
                "IPv6 addresses are not supported by this receiver. Use an IPv4 "
                "address (e.g. udp://239.192.0.1:9200).");
            return result;
        }
        host = hostPort.left(colon).trimmed();
        portText = hostPort.mid(colon + 1).trimmed();
        if (portText.isEmpty()) {
            result.error = QStringLiteral(
                "Missing port after ':' in '%1'. Use 1-65535 "
                "(e.g. udp://239.192.0.1:9200).").arg(hostPort);
            return result;
        }
    }

    if (host.isEmpty()) {
        result.error = (transport == StreamTransport::UdpMulticast)
            ? QStringLiteral("Missing multicast address. Use an IPv4 multicast "
                             "address in 224.0.0.0/4 (e.g. udp://239.192.0.1:9200).")
            : QStringLiteral("Missing host. Use tcp://host:9200 or "
                             "zmq+tcp://host:9201.");
        return result;
    }

    QString canonicalHost;
    if (transport == StreamTransport::UdpMulticast) {
        if (!canonical_multicast_address(host, canonicalHost)) {
            result.error = QStringLiteral(
                "'%1' is not a supported UDP destination. Use a 4-octet IPv4 "
                "multicast address in 224.0.0.0/4 (e.g. udp://239.192.0.1:9200); "
                "for a unicast host use tcp://host:9200.").arg(host);
            return result;
        }
    } else {
        if (!is_valid_host_name(host)) {
            result.error = QStringLiteral(
                "'%1' is not a valid host. Use a host name or IPv4 address "
                "(e.g. tcp://localhost:9200).").arg(host);
            return result;
        }
        canonicalHost = host;
    }

    quint16 port = 9200;  // default ETI port
    if (!portText.isEmpty()) {
        if (!parse_port(portText, port, result.error)) {
            return result;
        }
    }

    result.valid = true;
    result.transport = transport;
    result.address = canonicalHost;
    result.port = port;
    return result;
}

// ---------------------------------------------------------------------------
// ZeroMQ state (pimpl). Defined here -- BEFORE ~NetworkStreamWorker() and
// shutdown(), which reset/destroy the unique_ptr<ZmqState> -- because MSVC is
// stricter than GCC/Clang about std::unique_ptr<T> with an incomplete T (the
// destructor/default_delete instantiation is diagnosed even when the type is
// completed later in the same TU). Without HAVE_ZMQ the type stays a complete
// (empty) stub so the unique_ptr member's destructor and reset() remain valid.
// ---------------------------------------------------------------------------
#ifdef HAVE_ZMQ
struct NetworkStreamWorker::ZmqState {
    ZmqState() : context(1), subscriber(context, zmq::socket_type::sub) {}
    zmq::context_t context;
    zmq::socket_t subscriber;  // declared after context: destroyed first
};
#else
struct NetworkStreamWorker::ZmqState {};
#endif

// NetworkStreamWorker Implementation
NetworkStreamWorker::NetworkStreamWorker(const NetworkStreamConfig& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_socket(std::make_unique<QUdpSocket>(this))
    , m_zmq_poll_timer(std::make_unique<QTimer>(this))
    , m_statistics_timer(std::make_unique<QTimer>(this))
    , m_last_frame_time(std::chrono::steady_clock::now())
{
    m_statistics.reset();
    m_frame_rate_history.reserve(FRAME_RATE_HISTORY_SIZE);
    
    // Connect socket signals (UDP; TCP signals are connected in
    // initialize_tcp_socket() because that socket is created there).
    connect(m_socket.get(), &QUdpSocket::readyRead,
            this, &NetworkStreamWorker::handle_datagram_ready);
    connect(m_socket.get(), &QUdpSocket::errorOccurred,
            this, &NetworkStreamWorker::handle_socket_error);

    // ZeroMQ is polled from the worker's own event loop: a short, non-blocking
    // zmq_poll keeps teardown prompt (T22/T34 lesson: shutdown must never hang).
    connect(m_zmq_poll_timer.get(), &QTimer::timeout,
            this, &NetworkStreamWorker::poll_zmq);
    m_zmq_poll_timer->setInterval(ZMQ_POLL_INTERVAL_MS);
    
    // Setup statistics timer
    connect(m_statistics_timer.get(), &QTimer::timeout,
            this, &NetworkStreamWorker::update_statistics);
    m_statistics_timer->setInterval(1000); // Update every second
}

NetworkStreamWorker::~NetworkStreamWorker() {
    shutdown();
}

void NetworkStreamWorker::start_reception() {
    if (m_receiving.load()) {
        return; // Already receiving
    }
    
    m_last_error.clear();
    
    // No exception may escape this worker slot: a throw across the Qt event
    // loop / thread boundary terminates the process. The helpers below return
    // bool and set m_last_error; this catch is belt-and-suspenders.
    bool ok = false;
    try {
        switch (m_config.transport) {
            case StreamTransport::UdpMulticast:
                ok = initialize_socket() && join_multicast_group();
                break;
            case StreamTransport::Tcp:
                ok = initialize_tcp_socket();
                break;
            case StreamTransport::Zmq:
                ok = initialize_zmq_socket();
                break;
        }
    } catch (const std::exception& e) {
        m_last_error = QString("Failed to start reception: %1").arg(e.what());
    } catch (...) {
        m_last_error = QStringLiteral("Failed to start reception: unknown error");
    }
    
    if (!ok) {
        m_receiving = false;
        m_connected = false;
        if (m_last_error.isEmpty()) {
            m_last_error = QStringLiteral("Failed to start reception");
        }
        emit error_occurred(m_last_error);
        emit connection_status_changed(false);
        return;
    }
    
    m_receiving = true;
    m_connected = false;
    m_statistics.reset();
    m_frame_rate_history.clear();
    m_latency.reset();
    m_last_frame_time = std::chrono::steady_clock::now();
    m_statistics_timer->start();
    
    // UDP is connectionless (bind + join IS the connection) and a ZMQ SUB
    // connect() is accepted synchronously. Only TCP must wait for its
    // connected() signal before the session is confirmed.
    if (m_config.transport != StreamTransport::Tcp) {
        m_connected = true;
        emit connection_status_changed(true);
    }
    
    qDebug() << "NetworkStreamWorker: Started reception ("
             << stream_transport_name(m_config.transport) << ") on"
             << m_config.multicast_address << ":" << m_config.port;
}

void NetworkStreamWorker::stop_reception() {
    stop_reception_internal(/*emitStatus=*/true);
}

void NetworkStreamWorker::stop_reception_internal(bool emitStatus) {
    if (!m_receiving.load()) {
        return; // Not receiving
    }
    
    m_receiving = false;
    m_statistics_timer->stop();
    if (m_zmq_poll_timer) {
        m_zmq_poll_timer->stop();
    }
    
    switch (m_config.transport) {
        case StreamTransport::UdpMulticast:
            if (m_socket && m_socket->state() != QAbstractSocket::UnconnectedState) {
                leave_multicast_group();
                m_socket->close();
            }
            break;
        case StreamTransport::Tcp:
            // Never delete the socket from inside one of its own signal
            // handlers (disconnected()/errorOccurred() reach this slot): abort
            // and leave the unique_ptr for shutdown()/initialize_tcp_socket().
            if (m_tcp_socket) {
                m_tcp_socket->disconnect(this);
                m_tcp_socket->abort();
            }
            m_tcp_buffer.clear();
            break;
        case StreamTransport::Zmq:
            // Keep the socket for shutdown()/destructor; it is destroyed on
            // the worker thread where it was created.
            break;
    }
    
    m_connected = false;
    if (emitStatus) {
        emit connection_status_changed(false);
    }
    
    qDebug() << "NetworkStreamWorker: Stopped reception";
}

void NetworkStreamWorker::shutdown() {
    // Loss-silent teardown: the receiver initiated this, so no
    // connection_status_changed(false) may be emitted (review F1).
    stop_reception_internal(/*emitStatus=*/false);
    if (m_zmq_poll_timer) {
        m_zmq_poll_timer->stop();
    }
    // Destroy the ZMQ socket + context on the worker thread. ZMQ sockets are
    // NOT thread-safe; linger==0 (set at init) guarantees this never blocks.
    m_zmq.reset();
    if (m_tcp_socket) {
        m_tcp_socket->disconnect(this);
        m_tcp_socket->abort();
        m_tcp_socket.reset();
    }
    m_tcp_buffer.clear();
    if (m_socket && m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->close();
    }
}

bool NetworkStreamWorker::initialize_socket() {
    if (!m_socket) {
        m_last_error = QStringLiteral("Socket unavailable");
        return false;
    }
    
    m_socket->setProxy(QNetworkProxy::NoProxy);
    
    // Bind to the multicast port
    if (!m_socket->bind(QHostAddress::AnyIPv4, m_config.port, 
                       QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        m_last_error = QString("Failed to bind to port %1: %2")
                           .arg(m_config.port)
                           .arg(m_socket->errorString());
        return false;
    }
    
    // Set socket buffer sizes for high-throughput
    const int buffer_size = static_cast<int>(m_config.buffer_size * ETI_FRAME_SIZE * 2);
    m_socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, buffer_size);
    m_socket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, buffer_size);
    
    qDebug() << "NetworkStreamWorker: Socket initialized on port" << m_config.port;
    return true;
}

bool NetworkStreamWorker::join_multicast_group() {
    QHostAddress group_address;
    if (!group_address.setAddress(m_config.multicast_address) ||
        group_address.protocol() != QAbstractSocket::IPv4Protocol ||
        !group_address.isMulticast()) {
        m_last_error = QString("Invalid multicast address: %1")
                           .arg(m_config.multicast_address);
        return false;
    }
    
    // Find appropriate network interface
    QNetworkInterface interface;
    if (!m_config.interface_name.isEmpty()) {
        interface = QNetworkInterface::interfaceFromName(m_config.interface_name);
        if (!interface.isValid()) {
            m_last_error = QString("Network interface not found: %1")
                               .arg(m_config.interface_name);
            return false;
        }
    } else {
        // Use first multicast-capable interface
        const auto interfaces = QNetworkInterface::allInterfaces();
        for (const auto& iface : interfaces) {
            if (iface.flags() & QNetworkInterface::CanMulticast &&
                iface.flags() & QNetworkInterface::IsUp &&
                iface.flags() & QNetworkInterface::IsRunning) {
                interface = iface;
                break;
            }
        }
    }
    
    if (!interface.isValid()) {
        m_last_error = QStringLiteral("No suitable network interface found for multicast");
        return false;
    }
    
    if (!m_socket->joinMulticastGroup(group_address, interface)) {
        m_last_error = QString("Failed to join multicast group %1: %2")
                           .arg(m_config.multicast_address)
                           .arg(m_socket->errorString());
        return false;
    }
    
    qDebug() << "NetworkStreamWorker: Joined multicast group" 
             << m_config.multicast_address << "on interface" << interface.name();
    return true;
}

void NetworkStreamWorker::leave_multicast_group() {
    QHostAddress group_address(m_config.multicast_address);
    if (group_address.isMulticast() && m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->leaveMulticastGroup(group_address);
        qDebug() << "NetworkStreamWorker: Left multicast group" << m_config.multicast_address;
    }
}

// ---------------------------------------------------------------------------
// TCP raw ETI stream (tcp://host:port)
// ---------------------------------------------------------------------------
bool NetworkStreamWorker::initialize_tcp_socket() {
    // A previous aborted socket is deleted here (never from inside its own
    // signal handler). Safe: start_reception() runs as a queued slot.
    if (m_tcp_socket) {
        m_tcp_socket->disconnect(this);
        m_tcp_socket->abort();
        m_tcp_socket.reset();
    }
    m_tcp_buffer.clear();

    m_tcp_socket = std::make_unique<QTcpSocket>(this);
    m_tcp_socket->setProxy(QNetworkProxy::NoProxy);
    connect(m_tcp_socket.get(), &QTcpSocket::readyRead,
            this, &NetworkStreamWorker::handle_tcp_ready_read);
    connect(m_tcp_socket.get(), &QTcpSocket::connected,
            this, &NetworkStreamWorker::handle_tcp_connected);
    connect(m_tcp_socket.get(), &QTcpSocket::disconnected,
            this, &NetworkStreamWorker::handle_tcp_disconnected);
    connect(m_tcp_socket.get(), &QTcpSocket::errorOccurred,
            this, &NetworkStreamWorker::handle_tcp_error);

    qDebug() << "NetworkStreamWorker: TCP connecting to"
             << m_config.multicast_address << ":" << m_config.port;
    m_tcp_socket->connectToHost(m_config.multicast_address, m_config.port);
    return true;  // asynchronous: the outcome arrives via signals
}

void NetworkStreamWorker::handle_tcp_connected() {
    if (!m_receiving.load()) {
        return;
    }
    m_connected = true;
    emit connection_status_changed(true);
    qDebug() << "NetworkStreamWorker: TCP connected to"
             << m_config.multicast_address << ":" << m_config.port;
}

void NetworkStreamWorker::handle_tcp_disconnected() {
    if (m_receiving.load()) {
        qDebug() << "NetworkStreamWorker: TCP peer closed the stream";
        stop_reception();  // emits connection_status_changed(false)
    }
}

void NetworkStreamWorker::handle_tcp_error(QAbstractSocket::SocketError error) {
    // Connect failures and mid-stream errors are fatal to this session; the
    // receiver (and only the receiver) owns the reconnect policy.
    report_socket_error(error,
                        m_tcp_socket ? m_tcp_socket->errorString() : QString(), true);
}

void NetworkStreamWorker::handle_tcp_ready_read() {
    if (!m_tcp_socket || !m_receiving.load()) {
        return;
    }
    const QByteArray chunk = m_tcp_socket->readAll();
    if (chunk.isEmpty()) {
        return;
    }
    m_statistics.bytes_received += static_cast<size_t>(chunk.size());
    m_tcp_buffer.append(chunk);
    reframe_tcp_buffer();
}

void NetworkStreamWorker::reframe_tcp_buffer() {
    // Find the first valid 4-byte ETI sync word at or after `from`. Returns -1
    // when the accumulator holds no complete sync word.
    const auto findSync = [this](int from) {
        const int limit = m_tcp_buffer.size() - 3;  // need 4 bytes to compare
        for (int i = std::max(0, from); i < limit; ++i) {
            if (is_eti_sync_at(m_tcp_buffer, i)) {
                return i;
            }
        }
        return -1;
    };

    while (m_tcp_buffer.size() >= static_cast<int>(ETI_FRAME_SIZE)) {
        const int sync = findSync(0);
        if (sync < 0) {
            // No sync word anywhere: drop the un-frameable prefix but keep the
            // trailing 3 bytes, since a sync word may straddle the next read.
            if (m_tcp_buffer.size() > 3) {
                m_tcp_buffer.remove(0, m_tcp_buffer.size() - 3);
            }
            return;
        }
        if (sync > 0) {
            // Desynchronised: drop bytes until the first valid sync word.
            m_tcp_buffer.remove(0, sync);
        }
        if (m_tcp_buffer.size() < static_cast<int>(ETI_FRAME_SIZE)) {
            return;  // wait for the rest of the frame
        }
        // The frame starts with a sync word we just matched, so it is valid by
        // construction (validate_eti_packet only re-checks size + the same sync
        // words). No second validation / fallback branch is reachable.
        const QByteArray frame = m_tcp_buffer.left(static_cast<int>(ETI_FRAME_SIZE));
        const QString peer = m_tcp_socket ? m_tcp_socket->peerAddress().toString() : QString();
        const quint16 peerPort = m_tcp_socket ? m_tcp_socket->peerPort() : 0;
        deliver_eti_frame(frame, peer, peerPort);
        m_tcp_buffer.remove(0, static_cast<int>(ETI_FRAME_SIZE));
    }
}

// ---------------------------------------------------------------------------
// ZeroMQ SUB (zmq+tcp://host:port) — ODR-DabMux ETI publisher
// (NetworkStreamWorker::ZmqState is defined earlier, above the destructor.)
// ---------------------------------------------------------------------------
#ifdef HAVE_ZMQ
bool NetworkStreamWorker::initialize_zmq_socket() {
    try {
        m_zmq = std::make_unique<ZmqState>();
    } catch (const std::exception& e) {
        m_last_error = QString("Failed to create ZeroMQ context: %1").arg(e.what());
        return false;
    } catch (...) {
        m_last_error = QStringLiteral("Failed to create ZeroMQ context");
        return false;
    }

    try {
        // Never block on close (teardown must be prompt) and receive every
        // message the publisher sends (ODR-DabMux ETI output is unfiltered).
        m_zmq->subscriber.set(zmq::sockopt::linger, 0);
        m_zmq->subscriber.set(zmq::sockopt::subscribe, "");
        m_zmq->subscriber.set(zmq::sockopt::rcvhwm, 1000);
        const std::string endpoint =
            QStringLiteral("tcp://%1:%2")
                .arg(m_config.multicast_address)
                .arg(m_config.port)
                .toStdString();
        m_zmq->subscriber.connect(endpoint);
    } catch (const zmq::error_t& e) {
        m_last_error = QString("ZeroMQ connect failed: %1").arg(e.what());
        m_zmq.reset();
        return false;
    } catch (const std::exception& e) {
        m_last_error = QString("ZeroMQ setup failed: %1").arg(e.what());
        m_zmq.reset();
        return false;
    }

    if (m_zmq_poll_timer) {
        m_zmq_poll_timer->start();
    }
    qDebug() << "NetworkStreamWorker: ZMQ SUB connected to"
             << m_config.multicast_address << ":" << m_config.port;
    return true;
}

void NetworkStreamWorker::poll_zmq() {
    if (!m_zmq || !m_receiving.load()) {
        return;
    }

    try {
        zmq::pollitem_t items[] = {
            {m_zmq->subscriber.handle(), 0, ZMQ_POLLIN, 0}
        };
        // Short (1 ms) timeout: the worker event loop must stay responsive so a
        // queued shutdown is honoured promptly (teardown never hangs).
        zmq::poll(items, 1, std::chrono::milliseconds(1));
        if (!(items[0].revents & ZMQ_POLLIN)) {
            return;
        }
    } catch (const zmq::error_t& e) {
        emit error_occurred(QString("ZeroMQ poll error: %1").arg(e.what()));
        return;
    }

    // Drain pending messages, bounded per tick so a flood cannot starve the
    // event loop (and therefore shutdown).
    constexpr int MAX_MESSAGES_PER_POLL = 2000;
    for (int i = 0; i < MAX_MESSAGES_PER_POLL && m_receiving.load(); ++i) {
        zmq::message_t message;
        std::optional<size_t> received;
        try {
            received = m_zmq->subscriber.recv(message, zmq::recv_flags::dontwait);
        } catch (const zmq::error_t& e) {
            emit error_occurred(QString("ZeroMQ receive error: %1").arg(e.what()));
            break;
        }
        if (!received.has_value()) {
            break;  // no more messages pending
        }

        const QByteArray payload(static_cast<const char*>(message.data()),
                                 static_cast<int>(message.size()));
        m_statistics.bytes_received += static_cast<size_t>(payload.size());
        if (payload.size() >= static_cast<int>(ETI_FRAME_SIZE)) {
            const QByteArray frame = payload.left(static_cast<int>(ETI_FRAME_SIZE));
            if (validate_eti_packet(frame)) {
                deliver_eti_frame(frame, m_config.multicast_address, m_config.port);
            } else {
                m_statistics.frames_dropped++;
            }
        } else {
            m_statistics.frames_dropped++;
        }
    }
}
#else  // !HAVE_ZMQ
// ZeroMQ compiled out (the empty ZmqState stub is defined above). The transport
// methods are no-op/failing stubs so the switch in start_reception() and the
// poll timer connection still compile. UDP and TCP remain fully functional.
bool NetworkStreamWorker::initialize_zmq_socket() {
    m_last_error = QStringLiteral(
        "ZeroMQ support not built in: this build does not include the "
        "zmq+tcp:// transport.");
    return false;
}

void NetworkStreamWorker::poll_zmq() {}
#endif  // HAVE_ZMQ

void NetworkStreamWorker::deliver_eti_frame(const QByteArray& frame,
                                            const QString& sourceAddress,
                                            quint16 sourcePort) {
    if (!validate_eti_packet(frame)) {
        return;
    }
    NetworkPacket packet;
    packet.data.reserve(static_cast<size_t>(frame.size()));
    packet.data.insert(packet.data.end(), frame.begin(), frame.end());
    packet.timestamp = std::chrono::steady_clock::now();
    packet.sequence_number = m_sequence_number.fetch_add(1);
    packet.source_address = sourceAddress;
    packet.source_port = sourcePort;
    packet.is_valid = true;

    m_statistics.frames_received++;
    update_frame_rate();

    emit packet_received(packet);
    emit frame_received(frame);
}

void NetworkStreamWorker::handle_datagram_ready() {
    while (m_socket->hasPendingDatagrams() && m_receiving.load()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        
        if (datagram.isValid()) {
            const auto timestamp = std::chrono::steady_clock::now();
            const QByteArray& data = datagram.data();
            
            m_statistics.bytes_received += data.size();
            
            if (validate_eti_packet(data)) {
                // Create network packet
                NetworkPacket packet;
                packet.data.reserve(data.size());
                packet.data.insert(packet.data.end(), data.begin(), data.end());
                packet.timestamp = timestamp;
                packet.sequence_number = m_sequence_number.fetch_add(1);
                packet.source_address = datagram.senderAddress().toString();
                packet.source_port = datagram.senderPort();
                packet.is_valid = true;
                
                // Extract ETI frame data
                if (packet.contains_eti_frame()) {
                    m_statistics.frames_received++;
                    
                    // Update timing statistics
                    update_frame_rate();
                    
                    // Convert to QByteArray for signal emission
                    QByteArray frameData(reinterpret_cast<const char*>(packet.data.data()), 
                                        static_cast<int>(std::min(packet.data.size(), static_cast<size_t>(6144))));
                    
                    // Emit signals for real-time processing
                    emit packet_received(packet);
                    emit frame_received(frameData);
                } else {
                    m_statistics.frames_dropped++;
                    emit error_occurred("Invalid ETI frame received");
                }
            } else {
                m_statistics.network_errors++;
                emit error_occurred(QString("Invalid packet received: size=%1").arg(data.size()));
            }
        }
    }
}

void NetworkStreamWorker::process_pending_datagrams() {
    // Process all pending datagrams in the socket buffer
    while (m_socket && m_socket->hasPendingDatagrams() && m_receiving.load()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        
        if (datagram.isValid()) {
            const auto timestamp = std::chrono::steady_clock::now();
            const QByteArray& data = datagram.data();
            
            m_statistics.bytes_received += data.size();
            
            // Validate ETI packet format
            if (validate_eti_packet(data)) {
                // Create network packet
                NetworkPacket packet;
                packet.data.reserve(data.size());
                packet.data.insert(packet.data.end(), data.begin(), data.end());
                packet.timestamp = timestamp;
                packet.sequence_number = m_sequence_number.fetch_add(1);
                packet.source_address = datagram.senderAddress().toString();
                packet.source_port = datagram.senderPort();
                packet.is_valid = true;
                
                // Add to buffer with thread safety
                {
                    QMutexLocker locker(&m_queue_mutex);
                    
                    // Check buffer capacity
                    if (m_packet_queue.size() >= static_cast<int>(m_config.buffer_size)) {
                        // Buffer overflow - remove oldest frame
                        m_packet_queue.dequeue();
                        m_statistics.buffer_overflows++;
                    }
                    
                    m_packet_queue.enqueue(packet);
                    m_queue_condition.wakeOne();
                }
                
                // Update statistics
                m_statistics.frames_received++;
                m_last_frame_time = timestamp;
                
                // Emit signal for new packet
                emit packet_received(packet);
                
                // Update frame rate tracking
                update_frame_rate();
                
            } else {
                // Invalid packet
                m_statistics.frames_dropped++;
                qWarning() << "Invalid ETI packet received from" 
                          << datagram.senderAddress().toString()
                          << ":" << datagram.senderPort();
            }
        } else {
            // Invalid datagram
            m_statistics.network_errors++;
            qWarning() << "Invalid datagram received";
        }
    }
    
    // Check for quality issues
    check_quality_thresholds();
}

void NetworkStreamWorker::report_socket_error(QAbstractSocket::SocketError error,
                                              const QString& errorString,
                                              bool fatal) {
    m_statistics.network_errors++;
    
    QString error_string;
    switch (error) {
        case QAbstractSocket::ConnectionRefusedError:
            error_string = "Connection refused";
            break;
        case QAbstractSocket::HostNotFoundError:
            error_string = "Host not found";
            break;
        case QAbstractSocket::NetworkError:
            error_string = "Network error";
            break;
        case QAbstractSocket::SocketTimeoutError:
            error_string = "Socket timeout";
            break;
        case QAbstractSocket::RemoteHostClosedError:
            error_string = "Remote host closed the connection";
            break;
        default:
            error_string = errorString;
            break;
    }
    
    emit error_occurred(QString("Socket error: %1").arg(error_string));
    
    // A socket error is FATAL to the current reception. Stop and report the
    // connection as lost (exactly one connection_status_changed(false)) so the
    // receiver — which solely owns the reconnect policy — can decide whether
    // to retry. Fatal errors flow ONLY through here; quality/latency and
    // invalid-frame errors are non-fatal and must never trigger a reconnect.
    if (fatal && m_receiving.load()) {
        qDebug() << "NetworkStreamWorker: Fatal socket error, stopping reception";
        stop_reception();  // emits connection_status_changed(false) once
    }
}

void NetworkStreamWorker::handle_socket_error(QAbstractSocket::SocketError error) {
    report_socket_error(error, m_socket ? m_socket->errorString() : QString(), true);
}

bool NetworkStreamWorker::validate_eti_packet(const QByteArray& data) const {
    // Check minimum size for ETI frame
    if (data.size() < static_cast<int>(ETI_FRAME_SIZE)) {
        return false;
    }
    
    // Validate ETI sync pattern. All three standard ETI sync words are valid
    // on the wire (ETI-NI plus ETI-LI patterns A and B); the core parser
    // auto-detects which one it is.
    //
    // F8 assumption: the receiver accepts the ETI-LI sync words so an
    // ETI-LI-over-IP stream is not dropped. Genuine ETSI EN 300 799 LI framing
    // is accepted at this gate but is then parsed with the NI/ODR frame layout
    // by the downstream processor; that is correct for the current captures
    // (which use the LI sync word with ODR framing) and is intentionally left
    // unchanged here — no LI-specific de-framing is implemented.
    const uint8_t* packet_data = reinterpret_cast<const uint8_t*>(data.constData());
    return std::memcmp(packet_data, ETI_SYNC_PATTERN.data(), 4) == 0
           || std::memcmp(packet_data, ETI_SYNC_LI_A.data(), 4) == 0
           || std::memcmp(packet_data, ETI_SYNC_LI_B.data(), 4) == 0;
}

void NetworkStreamWorker::update_frame_rate() {
    const auto now = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        now - m_last_frame_time).count();
    
    if (duration > 0) {
        const double instantaneous_rate = 1000000.0 / static_cast<double>(duration); // frames per second
        
        // Update frame rate history
        m_frame_rate_history.push_back(instantaneous_rate);
        if (m_frame_rate_history.size() > FRAME_RATE_HISTORY_SIZE) {
            m_frame_rate_history.erase(m_frame_rate_history.begin());
        }
        
        // Calculate average frame rate
        const double average_rate = std::accumulate(
            m_frame_rate_history.begin(), m_frame_rate_history.end(), 0.0) / 
            static_cast<double>(m_frame_rate_history.size());
        
        m_statistics.current_frame_rate = instantaneous_rate;
        m_statistics.average_frame_rate = average_rate;
    }
    
    // Real per-frame arrival latency (interval between consecutive frames), NOT
    // "now - stream start" (which grew without bound and tripped the latency
    // threshold on a healthy stream). Publish the average/max only once the
    // tracker has enough samples, so quality checks never fire before data.
    if (m_latency.addSample(now)) {
        m_statistics.average_latency_us = m_latency.averageUs();
        m_statistics.max_latency_us = m_latency.maxUs();
    }
    
    m_last_frame_time = now;
}

void NetworkStreamWorker::update_statistics() {
    QMutexLocker locker(&m_statistics_mutex);
    
    // Calculate frame loss rate
    const size_t total_expected = m_statistics.frames_received + m_statistics.frames_dropped;
    if (total_expected > 0) {
        m_statistics.frame_loss_rate = static_cast<double>(m_statistics.frames_dropped) / static_cast<double>(total_expected);
    }
    
    m_statistics.last_update = std::chrono::steady_clock::now();
    
    // Check quality thresholds
    check_quality_thresholds();
    
    emit statistics_updated(m_statistics);
}

void NetworkStreamWorker::check_quality_thresholds() {
    // Never emit quality errors before any data has flowed: a freshly started
    // socket has a 0 frame rate / 0 latency and would otherwise emit
    // "Frame rate below threshold" (and, pre-fix, "Latency above threshold")
    // on a perfectly healthy idle stream.
    if (m_statistics.frames_received.load() == 0) {
        return;
    }
    
    // Check minimum frame rate
    if (m_statistics.average_frame_rate < m_config.min_frame_rate) {
        emit error_occurred(QString("Frame rate below threshold: %1 < %2")
                           .arg(m_statistics.average_frame_rate.load())
                           .arg(m_config.min_frame_rate));
    }
    
    // Check maximum frame loss
    if (m_statistics.frame_loss_rate > m_config.max_frame_loss) {
        emit error_occurred(QString("Frame loss above threshold: %1 > %2")
                           .arg(m_statistics.frame_loss_rate.load())
                           .arg(m_config.max_frame_loss));
    }
    
    // Check maximum latency — only with a valid (>= MIN_SAMPLES) real
    // inter-frame average.
    const auto max_latency_us = m_config.max_latency.count() * 1000;
    if (m_latency.valid() && m_latency.averageUs() > max_latency_us) {
        emit error_occurred(QString("Latency above threshold: %1µs > %2µs")
                           .arg(m_latency.averageUs())
                           .arg(max_latency_us));
    }
}

// NetworkStreamReceiver Implementation
NetworkStreamReceiver::NetworkStreamReceiver(QObject* parent)
    : QObject(parent)
    , m_reconnection_timer(std::make_unique<QTimer>(this))
{
    qRegisterMetaType<eti::NetworkStreamStatistics>("eti::NetworkStreamStatistics");
    qRegisterMetaType<eti::NetworkPacket>("eti::NetworkPacket");
    setup_reconnection_timer();
}

NetworkStreamReceiver::NetworkStreamReceiver(const NetworkStreamConfig& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_reconnection_timer(std::make_unique<QTimer>(this))
{
    qRegisterMetaType<eti::NetworkStreamStatistics>("eti::NetworkStreamStatistics");
    qRegisterMetaType<eti::NetworkPacket>("eti::NetworkPacket");
    validate_config(config);
    setup_reconnection_timer();
}

NetworkStreamReceiver::~NetworkStreamReceiver() {
    disconnect_from_stream();
    cleanup_worker_thread();
}

bool NetworkStreamReceiver::connect_to_stream(const QString& multicast_address, quint16 port) {
    NetworkStreamConfig config;
    config.multicast_address = multicast_address;
    config.port = port;
    return connect_with_config(config);
}

bool NetworkStreamReceiver::connect_with_config(const NetworkStreamConfig& config) {
    // Pre-validate the target BEFORE creating a worker. UDP requires a valid
    // multicast address; TCP/ZMQ require a non-empty host. A rejected target
    // must fail fast with exactly one error and no worker thread (and therefore
    // no retry loop).
    if (config.transport == StreamTransport::UdpMulticast) {
        if (!is_valid_multicast_address(config.multicast_address)) {
            const QString error = QString("Invalid multicast address: %1")
                                      .arg(config.multicast_address);
            {
                QMutexLocker locker(&m_mutex);
                m_error_messages.append(error);
                while (m_error_messages.size() > MAX_ERROR_MESSAGES) {
                    m_error_messages.removeFirst();
                }
            }
            emit error_occurred(error);
            return false;
        }
    } else if (config.multicast_address.trimmed().isEmpty()) {
        const QString error = QString("Invalid %1 host: address cannot be empty")
                                  .arg(stream_transport_name(config.transport));
        {
            QMutexLocker locker(&m_mutex);
            m_error_messages.append(error);
            while (m_error_messages.size() > MAX_ERROR_MESSAGES) {
                m_error_messages.removeFirst();
            }
        }
        emit error_occurred(error);
        return false;
    }
    
    try {
        validate_config(config);
        m_config = config;
        
        // Tear down any previous session SILENTLY (no status emit, no reconnect
        // — review F1). initialize_worker_thread() also cleans up first, but
        // the explicit call severs the old worker's signal connections before a
        // new one is created.
        cleanup_worker_thread();
        
        // Initialize worker thread
        initialize_worker_thread();
        
        return true;
    } catch (const std::exception& e) {
        QMutexLocker locker(&m_mutex);
        m_error_messages.append(QString("Connection failed: %1").arg(e.what()));
        return false;
    }
}

void NetworkStreamReceiver::disconnect_from_stream() {
    // Intentional teardown. cleanup_worker_thread() is loss-silent, so the stop
    // it performs can never be mistaken for a stream loss and retrigger the
    // reconnect policy (review F1). Exactly one user-facing disconnected event
    // is emitted below.
    cleanup_worker_thread();
    
    m_receiving = false;
    m_connected = false;
    m_reconnection_attempts = 0;
    m_reconnection_timer->stop();
    
    emit connection_status_changed(false);
}

bool NetworkStreamReceiver::is_connected() const {
    return m_connected.load();
}

bool NetworkStreamReceiver::is_receiving() const {
    return m_receiving.load() && !m_paused.load();
}

void NetworkStreamReceiver::set_config(const NetworkStreamConfig& config) {
    validate_config(config);

    if (!m_worker) {
        // No worker yet (nothing connected): just update the stored config;
        // the next connect_with_config()/initialize_worker_thread() snapshots it.
        m_config = config;
        return;
    }

    // LIVE config change. The worker snapshots its config at construction, so
    // a running worker must be rebuilt for the new config to take effect:
    // tear the worker+thread down SILENTLY, recreate it with the new config,
    // restart reception.
    //
    // Documented semantics:
    //  - The socket is transparently re-created with the new config
    //    (buffer sizes are applied in NetworkStreamWorker::initialize_socket()).
    //  - Intentional rebuild: NO connection_status_changed is emitted and the
    //    reconnect policy is NOT triggered (review F1). The receiver keeps its
    //    connected state; no user disconnect is required.
    //  - If reception was paused, it stays stopped until resumed.
    const bool was_receiving = is_receiving();

    cleanup_worker_thread();
    m_config = config;
    initialize_worker_thread();

    if (was_receiving) {
        start_reception();
    }
}

NetworkStreamConfig NetworkStreamReceiver::get_config() const {
    QMutexLocker locker(&m_mutex);
    return m_config;
}

void NetworkStreamReceiver::set_buffer_size(size_t buffer_frames) {
    // Public setter declared in the header but previously unimplemented; the
    // Settings dialog wires analysis/bufferSize through it. Delegate to
    // set_config() so validation + a reception restart stay in one place.
    NetworkStreamConfig config = get_config();
    config.buffer_size = buffer_frames;
    set_config(config);
}

void NetworkStreamReceiver::start_reception() {
    if (!m_connected.load()) {
        QMutexLocker locker(&m_mutex);
        m_error_messages.append("Cannot start reception: not connected");
        return;
    }
    
    if (m_worker) {
        m_receiving = true;
        m_paused = false;
        QMetaObject::invokeMethod(m_worker.get(), "start_reception", Qt::QueuedConnection);
    }
}

void NetworkStreamReceiver::stop_reception() {
    m_receiving = false;
    m_paused = false;
    
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker.get(), "stop_reception", Qt::QueuedConnection);
    }
}

void NetworkStreamReceiver::pause_reception() {
    m_paused = true;
}

void NetworkStreamReceiver::resume_reception() {
    if (m_receiving.load()) {
        m_paused = false;
    }
}

NetworkStreamStatisticsSnapshot NetworkStreamReceiver::get_statistics() const {
    QMutexLocker locker(&m_mutex);
    
    // Create a copyable snapshot by loading atomic values
    NetworkStreamStatisticsSnapshot snapshot;
    snapshot.frames_received = m_current_statistics.frames_received.load();
    snapshot.frames_processed = m_current_statistics.frames_processed.load();
    snapshot.frames_dropped = m_current_statistics.frames_dropped.load();
    snapshot.bytes_received = m_current_statistics.bytes_received.load();
    snapshot.network_errors = m_current_statistics.network_errors.load();
    snapshot.buffer_overflows = m_current_statistics.buffer_overflows.load();
    snapshot.buffer_underflows = m_current_statistics.buffer_underflows.load();
    snapshot.current_frame_rate = m_current_statistics.current_frame_rate.load();
    snapshot.average_frame_rate = m_current_statistics.average_frame_rate.load();
    snapshot.frame_loss_rate = m_current_statistics.frame_loss_rate.load();
    snapshot.average_latency_us = m_current_statistics.average_latency_us.load();
    snapshot.max_latency_us = m_current_statistics.max_latency_us.load();
    snapshot.start_time = m_current_statistics.start_time;
    snapshot.last_update = m_current_statistics.last_update;
    
    return snapshot;
}

double NetworkStreamReceiver::get_current_frame_rate() const {
    return m_current_statistics.current_frame_rate.load();
}

double NetworkStreamReceiver::get_average_frame_rate() const {
    return m_current_statistics.average_frame_rate.load();
}

double NetworkStreamReceiver::get_frame_loss_rate() const {
    return m_current_statistics.frame_loss_rate.load();
}

std::chrono::microseconds NetworkStreamReceiver::get_average_latency() const {
    return std::chrono::microseconds(m_current_statistics.average_latency_us.load());
}

std::chrono::microseconds NetworkStreamReceiver::get_max_latency() const {
    return std::chrono::microseconds(m_current_statistics.max_latency_us.load());
}

bool NetworkStreamReceiver::is_quality_acceptable() const {
    const auto& stats = m_current_statistics;
    return stats.average_frame_rate >= m_config.min_frame_rate &&
           stats.frame_loss_rate <= m_config.max_frame_loss &&
           stats.average_latency_us <= m_config.max_latency.count() * 1000;
}

void NetworkStreamReceiver::set_frame_callback(FrameCallback callback) {
    QMutexLocker locker(&m_mutex);
    m_frame_callback = std::move(callback);
}

void NetworkStreamReceiver::set_packet_callback(PacketCallback callback) {
    QMutexLocker locker(&m_mutex);
    m_packet_callback = std::move(callback);
}

void NetworkStreamReceiver::set_error_callback(ErrorCallback callback) {
    QMutexLocker locker(&m_mutex);
    m_error_callback = std::move(callback);
}

void NetworkStreamReceiver::set_statistics_callback(StatisticsCallback callback) {
    QMutexLocker locker(&m_mutex);
    m_statistics_callback = std::move(callback);
}

void NetworkStreamReceiver::initialize_worker_thread() {
    cleanup_worker_thread();
    
    m_worker_thread = std::make_unique<QThread>();
    
    // The worker must never schedule its OWN retries: reconnection policy is
    // owned solely by the receiver (attempt_reconnection). Disabling the
    // worker-level retry here prevents the old worker+receiver double-retry
    // and any tight loop on a failed start.
    NetworkStreamConfig worker_config = m_config;
    worker_config.auto_reconnect = false;
    m_worker = std::make_unique<NetworkStreamWorker>(worker_config);
    
    m_worker->moveToThread(m_worker_thread.get());
    
    // Connect worker signals
    connect(m_worker.get(), &NetworkStreamWorker::frame_received,
            this, &NetworkStreamReceiver::handle_worker_frame);
    connect(m_worker.get(), &NetworkStreamWorker::packet_received,
            this, &NetworkStreamReceiver::handle_worker_packet);
    connect(m_worker.get(), &NetworkStreamWorker::error_occurred,
            this, &NetworkStreamReceiver::handle_worker_error);
    connect(m_worker.get(), &NetworkStreamWorker::statistics_updated,
            this, &NetworkStreamReceiver::handle_worker_statistics);
    connect(m_worker.get(), &NetworkStreamWorker::connection_status_changed,
            this, &NetworkStreamReceiver::handle_connection_status);
    
    m_worker_thread->start();
    m_connected = true;
}

void NetworkStreamReceiver::cleanup_worker_thread() {
    // Intentional teardown is LOSS-SILENT (review F1). Sever the worker's
    // signal connections first so a stop reported while shutting down can never
    // reach handle_connection_status() and retrigger the reconnect policy. The
    // shutdown() call below is a queued functor metacall, not a signal
    // connection, so it still reaches the worker.
    if (m_worker) {
        QObject::disconnect(m_worker.get(), nullptr, this, nullptr);
    }

    if (m_worker && m_worker_thread && m_worker_thread->isRunning()) {
        // Run shutdown() on the worker's OWN thread: ZMQ sockets are not
        // thread-safe and must be destroyed on the thread that created them.
        // Bounded wait (review F3): the completion semaphore is held by a
        // shared_ptr captured in the queued lambda, so it stays valid even if
        // the timeout path abandons the call — an over-running worker can never
        // hang the caller.
        auto done = std::make_shared<QSemaphore>();
        QMetaObject::invokeMethod(
            m_worker.get(),
            [worker = m_worker.get(), done]() {
                worker->shutdown();
                done->release();
            },
            Qt::QueuedConnection);
        if (!done->tryAcquire(1, std::chrono::milliseconds(SHUTDOWN_TIMEOUT_MS))) {
            qWarning() << "NetworkStreamReceiver: worker shutdown timed out after"
                       << SHUTDOWN_TIMEOUT_MS << "ms; terminating worker thread";
            m_worker_thread->terminate();
            m_worker_thread->wait(1000);
        }
    } else if (m_worker) {
        // Review F4: there is no worker event loop to run shutdown() on. This
        // only happens when the thread never started, in which case the worker
        // never executed a transport init and holds no ZMQ context/socket, so
        // destroying it here cannot violate libzmq's create/destroy-on-the-same
        // -thread rule. Documented explicitly rather than silently relying on
        // the empty-ZMQ-state invariant.
        qWarning() << "NetworkStreamReceiver: worker thread not running; "
                      "shutting the worker down on the caller thread";
        m_worker->shutdown();
    }

    if (m_worker_thread) {
        m_worker_thread->quit();
        if (!m_worker_thread->wait(5000)) {
            m_worker_thread->terminate();
            m_worker_thread->wait(1000);
        }
    }
    
    m_worker.reset();
    m_worker_thread.reset();
}

void NetworkStreamReceiver::validate_config(const NetworkStreamConfig& config) {
    if (config.multicast_address.isEmpty()) {
        throw std::invalid_argument("Multicast address cannot be empty");
    }
    
    if (config.port == 0) {
        throw std::invalid_argument("Port cannot be zero");
    }
    
    if (config.buffer_size == 0 || config.buffer_size > MAX_BUFFER_SIZE) {
        throw std::invalid_argument(QString("Buffer size must be between 1 and %1")
                                   .arg(MAX_BUFFER_SIZE).toStdString());
    }
}

void NetworkStreamReceiver::handle_worker_frame(const QByteArray& frameData) {
    if (m_paused.load()) {
        return;
    }
    
    // Note: Callback handling removed as it expects EtiFrame type
    // Direct frame processing can be done in connected slots instead
    
    // Note: Frame queue disabled - frames delivered via signals only
    // Buffering can be done in the connected slot if needed
    
    emit frame_received(frameData);
}

void NetworkStreamReceiver::handle_worker_packet(const eti::NetworkPacket& packet) {
    if (m_paused.load()) {
        return;
    }
    
    // Call registered callback
    if (m_packet_callback) {
        m_packet_callback(packet);
    }
    
    emit packet_received(packet);
}

void NetworkStreamReceiver::handle_worker_error(const QString& error) {
    {
        QMutexLocker locker(&m_mutex);
        m_error_messages.append(error);
        
        // Limit error message history
        while (m_error_messages.size() > MAX_ERROR_MESSAGES) {
            m_error_messages.removeFirst();
        }
    }
    
    // Call registered callback
    if (m_error_callback) {
        m_error_callback(error);
    }
    
    emit error_occurred(error);
}

void NetworkStreamReceiver::handle_worker_statistics(const eti::NetworkStreamStatistics& stats) {
    {
        QMutexLocker locker(&m_mutex);
        // Snapshot copy (NetworkStreamStatistics now has a copy-assignment that
        // loads each atomic) — a consistent per-field view of the worker's
        // counters under the lock.
        m_current_statistics = stats;
    }
    
    // Check quality violations
    check_quality_violations(stats);
    
    // Call registered callback
    if (m_statistics_callback) {
        m_statistics_callback(stats);
    }
    
    emit statistics_updated(stats);
}

void NetworkStreamReceiver::handle_connection_status(bool connected) {
    m_connected = connected;
    
    if (connected) {
        // A confirmed connection resets the reconnection budget. Used to be
        // reset on mere worker creation, which caused an unbounded retry loop.
        m_reconnection_attempts = 0;
    } else if (m_config.auto_reconnect && m_receiving.load()) {
        // Review F2: loss-driven retries must honour reconnect_delay. The first
        // loss (no attempt spent yet) retries immediately for a fast recovery;
        // every later attempt is scheduled through the single-shot timer so
        // async/TCP failures cannot hammer the network back-to-back.
        if (m_reconnection_attempts.load() == 0) {
            attempt_reconnection();
        } else {
            m_reconnection_timer->start(static_cast<int>(m_config.reconnect_delay.count()));
        }
    }
    
    emit connection_status_changed(connected);
}

void NetworkStreamReceiver::setup_reconnection_timer() {
    connect(m_reconnection_timer.get(), &QTimer::timeout,
            this, &NetworkStreamReceiver::attempt_reconnection);
    m_reconnection_timer->setSingleShot(true);
}

void NetworkStreamReceiver::attempt_reconnection() {
    const size_t attempt = m_reconnection_attempts.fetch_add(1) + 1;
    
    if (attempt > m_config.max_reconnect_attempts) {
        handle_worker_error(QString("Maximum reconnection attempts (%1) exceeded")
                           .arg(m_config.max_reconnect_attempts));
        return;
    }
    
    emit reconnection_attempted(static_cast<int>(attempt));
    
    // Attempt to reconnect. NOTE: the attempt counter is NOT reset here — a
    // successful worker creation is not a successful connection; it is reset
    // only once connection_status_changed(true) is observed.
    if (connect_with_config(m_config)) {
        start_reception();
    } else {
        // Schedule next attempt
        m_reconnection_timer->start(static_cast<int>(m_config.reconnect_delay.count()));
    }
}

void NetworkStreamReceiver::enable_automatic_reconnection(bool enabled, size_t max_attempts) {
    QMutexLocker locker(&m_mutex);
    m_config.auto_reconnect = enabled;
    if (max_attempts > 0) {
        m_config.max_reconnect_attempts = max_attempts;
    }
}

void NetworkStreamReceiver::check_quality_violations(const NetworkStreamStatistics& stats) {
    // Frame rate violation
    if (stats.average_frame_rate < m_config.min_frame_rate) {
        emit quality_threshold_violated(
            QString("Frame rate below threshold: %1 < %2")
            .arg(stats.average_frame_rate.load())
            .arg(m_config.min_frame_rate));
    }
    
    // Frame loss violation
    if (stats.frame_loss_rate > m_config.max_frame_loss) {
        emit quality_threshold_violated(
            QString("Frame loss above threshold: %1% > %2%")
            .arg(stats.frame_loss_rate.load() * 100, 0, 'f', 2)
            .arg(m_config.max_frame_loss * 100, 0, 'f', 2));
    }
    
    // Latency violation
    const auto max_latency_us = m_config.max_latency.count() * 1000;
    if (stats.average_latency_us > max_latency_us) {
        emit quality_threshold_violated(
            QString("Latency above threshold: %1µs > %2µs")
            .arg(stats.average_latency_us.load())
            .arg(max_latency_us));
    }
    
    // Buffer violations
    if (stats.buffer_overflows > 0) {
        emit buffer_overflow_detected();
    }
    
    if (stats.buffer_underflows > 0) {
        emit buffer_underflow_detected();
    }
}

// NetworkStreamReceiverFactory Implementation
std::unique_ptr<NetworkStreamReceiver> NetworkStreamReceiverFactory::create_dab_multicast_receiver(
    const QString& multicast_address, quint16 port) {
    
    NetworkStreamConfig config;
    config.multicast_address = multicast_address;
    config.port = port;
    config.buffer_size = 1000;
    config.auto_reconnect = true;
    config.enable_flow_control = true;
    
    return std::make_unique<NetworkStreamReceiver>(config);
}

std::unique_ptr<NetworkStreamReceiver> NetworkStreamReceiverFactory::create_professional_receiver(
    const NetworkStreamConfig& config) {
    
    return std::make_unique<NetworkStreamReceiver>(config);
}

std::unique_ptr<NetworkStreamReceiver> NetworkStreamReceiverFactory::create_low_latency_receiver(
    const QString& multicast_address, quint16 port) {
    
    NetworkStreamConfig config;
    config.multicast_address = multicast_address;
    config.port = port;
    config.buffer_size = 100;  // Smaller buffer for lower latency
    config.max_latency = std::chrono::milliseconds(50);  // Strict latency requirement
    config.enable_flow_control = false;  // Disable for minimum latency
    config.auto_reconnect = true;
    
    return std::make_unique<NetworkStreamReceiver>(config);
}

QString NetworkStreamReceiver::get_last_error() const {
    // m_error_messages is written under m_mutex (handle_worker_error,
    // connect_with_config, start_reception), so read it under the same lock.
    // m_mutex is mutable, so no const_cast is needed here.
    QMutexLocker locker(&m_mutex);
    if (!m_error_messages.empty()) {
        return m_error_messages.back();
    }
    return QString();
}

} // namespace eti

// MOC file inclusion handled automatically by CMake
