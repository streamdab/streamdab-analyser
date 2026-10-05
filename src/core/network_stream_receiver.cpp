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
#include <algorithm>
#include <numeric>

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

}  // namespace

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
            "Please enter a stream URL (e.g. udp://239.192.0.1:9200).");
        return result;
    }

    // Optional scheme prefix: only udp:// reaches the multicast receiver.
    QString hostPort = url;
    const int schemeSep = url.indexOf(QStringLiteral("://"));
    if (schemeSep >= 0) {
        const QString scheme = url.left(schemeSep).toLower();
        if (scheme != QLatin1String("udp")) {
            result.error = QStringLiteral(
                "Unsupported URL scheme '%1://'. Only UDP multicast is supported "
                "(e.g. udp://239.192.0.1:9200).").arg(scheme);
            return result;
        }
        hostPort = url.mid(schemeSep + 3);
    } else if (url.contains(QLatin1Char('/'))) {
        result.error = QStringLiteral(
            "Malformed URL '%1'. Supported format: udp://239.192.0.1:9200 "
            "or 239.192.0.1:9200.").arg(url);
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
                "multicast address (e.g. udp://239.192.0.1:9200).");
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
        result.error = QStringLiteral(
            "Missing multicast address. Use an IPv4 multicast address in "
            "224.0.0.0/4 (e.g. udp://239.192.0.1:9200).");
        return result;
    }

    QString canonical;
    if (!canonical_multicast_address(host, canonical)) {
        result.error = QStringLiteral(
            "'%1' is not a supported destination. This receiver is multicast-only: "
            "use a 4-octet IPv4 multicast address in 224.0.0.0/4 "
            "(e.g. udp://239.192.0.1:9200).").arg(host);
        return result;
    }

    quint16 port = 9200;  // default ETI port
    if (!portText.isEmpty()) {
        // Strict digits-only port: rejects "+5", "0005", " 5" and "5.0".
        if (portText.size() > 5) {
            result.error = QStringLiteral(
                "Invalid port '%1'. Use a number between 1 and 65535 "
                "(default 9200).").arg(portText);
            return result;
        }
        for (const QChar c : portText) {
            if (!c.isDigit()) {
                result.error = QStringLiteral(
                    "Invalid port '%1'. Use digits only, between 1 and 65535 "
                    "(default 9200).").arg(portText);
                return result;
            }
        }
        if (portText.size() > 1 && portText.at(0) == QLatin1Char('0')) {
            result.error = QStringLiteral(
                "Invalid port '%1'. Leading zeros are not allowed "
                "(use e.g. '5', not '0005').").arg(portText);
            return result;
        }
        bool ok = false;
        const uint value = portText.toUInt(&ok);
        if (!ok || value == 0 || value > 65535) {
            result.error = QStringLiteral(
                "Invalid port '%1'. Use a number between 1 and 65535 "
                "(default 9200).").arg(portText);
            return result;
        }
        port = static_cast<quint16>(value);
    }

    result.valid = true;
    result.address = canonical;  // canonical 4-octet dotted-decimal
    result.port = port;
    return result;
}

// NetworkStreamWorker Implementation
NetworkStreamWorker::NetworkStreamWorker(const NetworkStreamConfig& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_socket(std::make_unique<QUdpSocket>(this))
    , m_statistics_timer(std::make_unique<QTimer>(this))
    , m_last_frame_time(std::chrono::steady_clock::now())
{
    m_statistics.reset();
    m_frame_rate_history.reserve(FRAME_RATE_HISTORY_SIZE);
    
    // Connect socket signals
    connect(m_socket.get(), &QUdpSocket::readyRead,
            this, &NetworkStreamWorker::handle_datagram_ready);
    connect(m_socket.get(), &QUdpSocket::errorOccurred,
            this, &NetworkStreamWorker::handle_socket_error);
    
    // Setup statistics timer
    connect(m_statistics_timer.get(), &QTimer::timeout,
            this, &NetworkStreamWorker::update_statistics);
    m_statistics_timer->setInterval(1000); // Update every second
}

NetworkStreamWorker::~NetworkStreamWorker() {
    stop_reception();
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
        ok = initialize_socket() && join_multicast_group();
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
    m_connected = true;
    m_statistics.reset();
    m_frame_rate_history.clear();
    m_latency.reset();
    m_last_frame_time = std::chrono::steady_clock::now();
    m_statistics_timer->start();
    
    emit connection_status_changed(true);
    qDebug() << "NetworkStreamWorker: Started reception on" 
             << m_config.multicast_address << ":" << m_config.port;
}

void NetworkStreamWorker::stop_reception() {
    if (!m_receiving.load()) {
        return; // Not receiving
    }
    
    m_receiving = false;
    m_statistics_timer->stop();
    
    if (m_socket && m_socket->state() != QAbstractSocket::UnconnectedState) {
        leave_multicast_group();
        m_socket->close();
    }
    
    m_connected = false;
    emit connection_status_changed(false);
    
    qDebug() << "NetworkStreamWorker: Stopped reception";
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

void NetworkStreamWorker::handle_socket_error(QAbstractSocket::SocketError error) {
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
        default:
            error_string = m_socket->errorString();
            break;
    }
    
    emit error_occurred(QString("Socket error: %1").arg(error_string));
    
    // A socket error is FATAL to the current reception. Stop and report the
    // connection as lost (exactly one connection_status_changed(false)) so the
    // receiver — which solely owns the reconnect policy — can decide whether
    // to retry. Fatal errors flow ONLY through here; quality/latency and
    // invalid-frame errors are non-fatal and must never trigger a reconnect.
    if (m_receiving.load()) {
        qDebug() << "NetworkStreamWorker: Fatal socket error, stopping reception";
        stop_reception();  // emits connection_status_changed(false) once
    }
}

bool NetworkStreamWorker::validate_eti_packet(const QByteArray& data) const {
    // Check minimum size for ETI frame
    if (data.size() < static_cast<int>(ETI_FRAME_SIZE)) {
        return false;
    }
    
    // Validate ETI sync pattern
    const uint8_t* packet_data = reinterpret_cast<const uint8_t*>(data.constData());
    return std::memcmp(packet_data, ETI_SYNC_PATTERN.data(), 4) == 0;
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
    setup_reconnection_timer();
}

NetworkStreamReceiver::NetworkStreamReceiver(const NetworkStreamConfig& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_reconnection_timer(std::make_unique<QTimer>(this))
{
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
    // Pre-validate the target BEFORE creating a worker: this receiver is
    // multicast-only, so a host name / unicast / TCP target must fail fast with
    // exactly one error and no worker thread (and therefore no retry loop).
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
    
    try {
        validate_config(config);
        m_config = config;
        
        // Cleanup existing connection
        if (m_connected.load()) {
            disconnect_from_stream();
        }
        
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
    if (m_receiving.load()) {
        stop_reception();
    }
    
    cleanup_worker_thread();
    
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
    
    const bool was_receiving = is_receiving();
    if (was_receiving) {
        stop_reception();
    }
    
    m_config = config;
    
    if (was_receiving) {
        start_reception();
    }
}

NetworkStreamConfig NetworkStreamReceiver::get_config() const {
    QMutexLocker locker(&m_mutex);
    return m_config;
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
    if (m_worker_thread && m_worker_thread->isRunning()) {
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
        // Manually assign atomic values since operator= is deleted
        m_current_statistics.frames_received = stats.frames_received.load();
        m_current_statistics.frames_processed = stats.frames_processed.load();
        m_current_statistics.frames_dropped = stats.frames_dropped.load();
        m_current_statistics.bytes_received = stats.bytes_received.load();
        m_current_statistics.network_errors = stats.network_errors.load();
        m_current_statistics.buffer_overflows = stats.buffer_overflows.load();
        m_current_statistics.buffer_underflows = stats.buffer_underflows.load();
        m_current_statistics.current_frame_rate = stats.current_frame_rate.load();
        m_current_statistics.average_frame_rate = stats.average_frame_rate.load();
        m_current_statistics.frame_loss_rate = stats.frame_loss_rate.load();
        m_current_statistics.average_latency_us = stats.average_latency_us.load();
        m_current_statistics.max_latency_us = stats.max_latency_us.load();
        m_current_statistics.start_time = stats.start_time;
        m_current_statistics.last_update = stats.last_update;
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
        attempt_reconnection();
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