/**
 * @file test_network_stream_validation.cpp
 * @brief T22 + item-4 network-stream robustness tests.
 *
 * Covers three independent layers:
 *   1. `eti::validate_stream_url()` — the pure URL validator shared by the UI
 *      pre-check and the receiver. Exactly four forms are accepted:
 *      `udp://A.B.C.D[:port]`, plain `A.B.C.D[:port]` (IPv4 multicast),
 *      `tcp://host[:port]` (raw ETI) and `zmq+tcp://host[:port]` (ZeroMQ SUB).
 *      Unicast UDP, scheme-less hostnames, IPv6, `http://`, the old `zmq://`
 *      alias and bad ports are rejected with an error.
 *   2. `NetworkStreamReceiver` / `NetworkStreamWorker` hardening — a
 *      non-multicast UDP target must fail fast: return false / emit
 *      `error_occurred` exactly once, WITHOUT throwing and WITHOUT scheduling
 *      reconnect attempts (the reported tight loop).
 *   3. The TCP/ZMQ transports over loopback (local QTcpServer / ZMQ PUB) and
 *      the review W2 regression that an intentional live reconfiguration
 *      (`set_config`/`set_buffer_size`) with auto-reconnect armed triggers NO
 *      reconnection and no status flap (F1).
 *
 * @date October 2026
 */

#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QFile>
#include <QTcpServer>
#include <QTcpSocket>

#include <cstring>

// ZeroMQ (cppzmq) is optional; HAVE_ZMQ is defined by CMake only when found.
#ifdef HAVE_ZMQ
#include <zmq.hpp>
#endif

#include "core/network_stream_receiver.hpp"

class TestNetworkStreamValidation : public QObject
{
    Q_OBJECT

private slots:
    // --- URL validator -----------------------------------------------------
    void testAcceptsSupportedMulticastUrls();
    void testAcceptsTcpAndZmqUrls();
    void testRejectsUnsupportedUrls_data();
    void testRejectsUnsupportedUrls();
    void testValidatorNeverThrows();
    void testCanonicalMulticastAddressStrict();

    // --- latency metric (F3b) ---------------------------------------------
    void testInterFrameLatencyTracker();

    // --- receiver / worker hardening --------------------------------------
    void testReceiverRejectsNonMulticastWithoutThrowOrRetry();
    void testReceiverRejectsUnicastWithoutRetry();
    void testWorkerStartNoThrowOnInvalidAddress();
    // --- F1: fatal-error -> stop -> reconnect -----------------------------
    void testWorkerFatalErrorStopsReceptionOnce();
    void testReceiverReconnectBoundedByMaxAttempts();

    // --- item 4: TCP + ZMQ transports (loopback, no external services) -----
    void testTcpLoopbackReframesFixture();
    void testZmqLoopbackDelivery();
    // --- W2 F1: intentional reconfiguration must be loss-silent ------------
    void testReconfigureWithAutoReconnectIsQuiet();

private:
    /// Load up to `count` real 6144-byte ETI frames from the Bangkok fixture.
    /// Empty when the fixture cannot be found (caller QSKIPs).
    QList<QByteArray> loadFixtureFrames(int count) const;
};

// ============================================================================
// URL validator
// ============================================================================
void TestNetworkStreamValidation::testAcceptsSupportedMulticastUrls()
{
    // udp:// with explicit port.
    {
        const eti::StreamUrlValidation r =
            eti::validate_stream_url(QStringLiteral("udp://239.192.0.1:9200"));
        QVERIFY2(r.valid, qPrintable(r.error));
        QCOMPARE(r.address, QStringLiteral("239.192.0.1"));
        QCOMPARE(r.port, quint16(9200));
    }
    // Plain address:port.
    {
        const eti::StreamUrlValidation r =
            eti::validate_stream_url(QStringLiteral("239.192.0.1:9200"));
        QVERIFY2(r.valid, qPrintable(r.error));
        QCOMPARE(r.address, QStringLiteral("239.192.0.1"));
        QCOMPARE(r.port, quint16(9200));
    }
    // Missing port defaults to 9200.
    {
        const eti::StreamUrlValidation r =
            eti::validate_stream_url(QStringLiteral("udp://224.0.0.1"));
        QVERIFY2(r.valid, qPrintable(r.error));
        QCOMPARE(r.address, QStringLiteral("224.0.0.1"));
        QCOMPARE(r.port, quint16(9200));
    }
    // Plain address without port also defaults.
    {
        const eti::StreamUrlValidation r =
            eti::validate_stream_url(QStringLiteral("239.192.0.1"));
        QVERIFY2(r.valid, qPrintable(r.error));
        QCOMPARE(r.port, quint16(9200));
    }
    // Surrounding whitespace is trimmed.
    {
        const eti::StreamUrlValidation r =
            eti::validate_stream_url(QStringLiteral("  udp://239.192.0.1:9200 \n"));
        QVERIFY2(r.valid, qPrintable(r.error));
    }
    // Upper boundary of the multicast /4 range and the port range.
    {
        QVERIFY(eti::validate_stream_url(QStringLiteral("224.0.0.1:1")).valid);
        QVERIFY(eti::validate_stream_url(QStringLiteral("239.255.255.255:65535")).valid);
    }
}

void TestNetworkStreamValidation::testAcceptsTcpAndZmqUrls()
{
    // TCP raw ETI stream: host name or IPv4 literal, explicit or default port.
    {
        const eti::StreamUrlValidation r =
            eti::validate_stream_url(QStringLiteral("tcp://localhost:9200"));
        QVERIFY2(r.valid, qPrintable(r.error));
        QCOMPARE(r.transport, eti::StreamTransport::Tcp);
        QCOMPARE(r.address, QStringLiteral("localhost"));
        QCOMPARE(r.port, quint16(9200));
    }
    {
        const eti::StreamUrlValidation r =
            eti::validate_stream_url(QStringLiteral("tcp://192.168.1.100:9200"));
        QVERIFY2(r.valid, qPrintable(r.error));
        QCOMPARE(r.transport, eti::StreamTransport::Tcp);
        QCOMPARE(r.address, QStringLiteral("192.168.1.100"));
    }
    {
        // Port is optional and defaults to 9200.
        const eti::StreamUrlValidation r =
            eti::validate_stream_url(QStringLiteral("tcp://example.org"));
        QVERIFY2(r.valid, qPrintable(r.error));
        QCOMPARE(r.transport, eti::StreamTransport::Tcp);
        QCOMPARE(r.address, QStringLiteral("example.org"));
        QCOMPARE(r.port, quint16(9200));
    }
    // ZeroMQ SUB (ODR-DabMux publisher) when the transport is built in; when it
    // is compiled out the validator must reject zmq+tcp:// with a clear reason.
#ifdef HAVE_ZMQ
    {
        const eti::StreamUrlValidation r =
            eti::validate_stream_url(QStringLiteral("zmq+tcp://localhost:9201"));
        QVERIFY2(r.valid, qPrintable(r.error));
        QCOMPARE(r.transport, eti::StreamTransport::Zmq);
        QCOMPARE(r.address, QStringLiteral("localhost"));
        QCOMPARE(r.port, quint16(9201));
    }
    {
        const eti::StreamUrlValidation r =
            eti::validate_stream_url(QStringLiteral("zmq+tcp://10.0.0.5"));
        QVERIFY2(r.valid, qPrintable(r.error));
        QCOMPARE(r.transport, eti::StreamTransport::Zmq);
        QCOMPARE(r.port, quint16(9200));
    }
#else
    {
        const eti::StreamUrlValidation r =
            eti::validate_stream_url(QStringLiteral("zmq+tcp://localhost:9201"));
        QVERIFY2(!r.valid, "zmq+tcp:// must be rejected when ZeroMQ is not built in");
        QVERIFY2(r.error.contains(QStringLiteral("ZeroMQ support not built in")),
                 qPrintable(QStringLiteral("unexpected rejection reason: %1").arg(r.error)));
    }
#endif
    // Case-insensitive scheme.
    {
        const eti::StreamUrlValidation r =
            eti::validate_stream_url(QStringLiteral("TCP://Host.Example:9200"));
        QVERIFY2(r.valid, qPrintable(r.error));
        QCOMPARE(r.transport, eti::StreamTransport::Tcp);
        QCOMPARE(r.address, QStringLiteral("Host.Example"));
    }
    // UDP still resolves to the multicast transport.
    QCOMPARE(eti::validate_stream_url(QStringLiteral("udp://239.192.0.1:9200")).transport,
             eti::StreamTransport::UdpMulticast);
    QCOMPARE(eti::validate_stream_url(QStringLiteral("239.192.0.1:9200")).transport,
             eti::StreamTransport::UdpMulticast);
}

void TestNetworkStreamValidation::testRejectsUnsupportedUrls_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("why");

    QTest::newRow("hostname")          << QStringLiteral("localhost")              << "hostname without scheme";
    QTest::newRow("unicast")           << QStringLiteral("192.168.1.100:9200")    << "unicast without scheme";
    QTest::newRow("loopback")          << QStringLiteral("udp://127.0.0.1:9200")  << "loopback unicast";
    QTest::newRow("empty")             << QString()                               << "empty";
    QTest::newRow("whitespace")        << QStringLiteral("   ")                    << "blank";
    QTest::newRow("port0")             << QStringLiteral("239.192.0.1:0")          << "port 0";
    QTest::newRow("port_too_big")      << QStringLiteral("239.192.0.1:99999")      << "port > 65535";
    QTest::newRow("port_not_a_number") << QStringLiteral("239.192.0.1:abc")        << "non-numeric port";
    QTest::newRow("missing_port")      << QStringLiteral("udp://239.192.0.1:")     << "trailing colon";
    QTest::newRow("ipv6")              << QStringLiteral("udp://[ff02::1]:9200")   << "ipv6 unsupported";
    QTest::newRow("http")              << QStringLiteral("http://239.192.0.1:9200") << "http scheme";
    QTest::newRow("path")              << QStringLiteral("239.192.0.1/stream")     << "malformed path";
    // F4: strict dotted-decimal / digits-only port (QHostAddress is lenient).
    QTest::newRow("short_ipv4")        << QStringLiteral("224.0.0")               << "short form";
    QTest::newRow("hex_octet")         << QStringLiteral("0xE0.0.0.1")            << "hex octet";
    QTest::newRow("octal_octet")       << QStringLiteral("224.0.0.010")           << "leading-zero octet";
    QTest::newRow("hex_dword")         << QStringLiteral("0xE0000001")            << "hex dword";
    QTest::newRow("port_plus")         << QStringLiteral("239.192.0.1:+5")        << "signed port";
    QTest::newRow("port_leading_zero") << QStringLiteral("239.192.0.1:0005")      << "leading-zero port";
    QTest::newRow("octet_overflow")    << QStringLiteral("239.192.0.256")         << "octet > 255";
    QTest::newRow("octet_space")       << QStringLiteral("239.192. 0.1")          << "space in octet";
    // item 4: malformed TCP / ZMQ forms stay rejected.
    QTest::newRow("tcp_missing_host")  << QStringLiteral("tcp://")                << "tcp empty host";
    QTest::newRow("tcp_bad_port")      << QStringLiteral("tcp://host:abc")        << "tcp non-numeric port";
    QTest::newRow("tcp_port_zero")     << QStringLiteral("tcp://host:0")          << "tcp port 0";
    QTest::newRow("tcp_port_big")      << QStringLiteral("tcp://host:99999")      << "tcp port > 65535";
    QTest::newRow("tcp_ipv6")          << QStringLiteral("tcp://[ff02::1]:9200")  << "tcp ipv6";
    QTest::newRow("tcp_path")          << QStringLiteral("tcp://host/stream")     << "tcp path";
    QTest::newRow("tcp_space_host")    << QStringLiteral("tcp://ho st:9200")      << "tcp space in host";
    QTest::newRow("zmq_missing_host")  << QStringLiteral("zmq+tcp://")            << "zmq empty host";
    QTest::newRow("zmq_bad_port")      << QStringLiteral("zmq+tcp://host:70000")  << "zmq port > 65535";
    // F5: the undocumented `zmq://` alias is intentionally NOT accepted.
    QTest::newRow("zmq_alias")         << QStringLiteral("zmq://localhost:9200")  << "undocumented zmq alias";
}

void TestNetworkStreamValidation::testRejectsUnsupportedUrls()
{
    QFETCH(QString, url);
    QFETCH(QString, why);

    const eti::StreamUrlValidation r = eti::validate_stream_url(url);
    QVERIFY2(!r.valid, qPrintable(QStringLiteral("must reject %1 (%2)").arg(url, why)));
    QVERIFY2(!r.error.isEmpty(),
             qPrintable(QStringLiteral("rejection must carry a reason (%1)").arg(why)));
    QCOMPARE(r.address, QString());
    QCOMPARE(r.port, quint16(9200));  // default remains
}

void TestNetworkStreamValidation::testValidatorNeverThrows()
{
    // Malformed / hostile inputs must never escape as an exception: the UI
    // calls the validator on raw user text.
    const QStringList hostile = {
        QStringLiteral(""), QStringLiteral(":::::"), QStringLiteral("udp://"),
        QStringLiteral("udp://:"), QStringLiteral("999.999.999.999"),
        QStringLiteral("udp://239.192.0.1:99999999999999999999"),
        QStringLiteral("\x01\x02"), QStringLiteral("://"),
    };
    for (const QString& h : hostile) {
        bool threw = false;
        try {
            const eti::StreamUrlValidation r = eti::validate_stream_url(h);
            QVERIFY(!r.valid);
        } catch (...) {
            threw = true;
        }
        QVERIFY2(!threw, qPrintable(QStringLiteral("validator threw on '%1'").arg(h)));
    }
}

void TestNetworkStreamValidation::testCanonicalMulticastAddressStrict()
{
    // Strict parser: accept exact 4-octet dotted-decimal only.
    QString canonical;
    QVERIFY(eti::canonical_multicast_address(QStringLiteral("239.192.0.1"), canonical));
    QCOMPARE(canonical, QStringLiteral("239.192.0.1"));
    QVERIFY(eti::canonical_multicast_address(QStringLiteral("224.0.0.0"), canonical));
    QCOMPARE(canonical, QStringLiteral("224.0.0.0"));

    // Lenient QHostAddress forms must now be rejected.
    QVERIFY(!eti::canonical_multicast_address(QStringLiteral("224.0.0"), canonical));
    QVERIFY(!eti::canonical_multicast_address(QStringLiteral("224.0.0.010"), canonical));
    QVERIFY(!eti::canonical_multicast_address(QStringLiteral("0xE0.0.0.1"), canonical));
    QVERIFY(!eti::canonical_multicast_address(QStringLiteral("0xE0000001"), canonical));
    QVERIFY(!eti::canonical_multicast_address(QStringLiteral("239.192.0.256"), canonical));
    QVERIFY(!eti::canonical_multicast_address(QStringLiteral("239.192.0.01"), canonical));
    // Outside 224.0.0.0/4.
    QVERIFY(!eti::canonical_multicast_address(QStringLiteral("223.255.255.255"), canonical));
    QVERIFY(!eti::canonical_multicast_address(QStringLiteral("240.0.0.1"), canonical));
    QVERIFY(!eti::canonical_multicast_address(QStringLiteral("192.168.1.1"), canonical));
    QVERIFY(!eti::canonical_multicast_address(QString(), canonical));

    // The validator surfaces the canonical form as `address`.
    const eti::StreamUrlValidation r =
        eti::validate_stream_url(QStringLiteral("udp://239.192.0.1:9200"));
    QVERIFY(r.valid);
    QCOMPARE(r.address, QStringLiteral("239.192.0.1"));
}

// ============================================================================
// F3b: inter-frame latency metric
// ============================================================================
void TestNetworkStreamValidation::testInterFrameLatencyTracker()
{
    using namespace std::chrono;

    eti::InterFrameLatencyTracker tracker;
    QVERIFY(!tracker.valid());
    QCOMPARE(tracker.sampleCount(), size_t(0));

    // The very first arrival has no predecessor -> not a sample.
    auto t = steady_clock::time_point{};
    tracker.addSample(t);
    QVERIFY(!tracker.valid());
    QCOMPARE(tracker.sampleCount(), size_t(0));

    // Synthetic regular ~1 ms cadence (≈1000 fps): valid only after
    // MIN_SAMPLES, and far below the 100 ms default threshold.
    for (size_t i = 1; i <= eti::InterFrameLatencyTracker::MIN_SAMPLES; ++i) {
        t += microseconds(1000);
        tracker.addSample(t);
    }
    QVERIFY(tracker.valid());
    QCOMPARE(tracker.sampleCount(), eti::InterFrameLatencyTracker::MIN_SAMPLES);
    QCOMPARE(tracker.averageUs(), int64_t(1000));
    QCOMPARE(tracker.maxUs(), int64_t(1000));
    QVERIFY2(tracker.averageUs() < 100000,
             "healthy 1 ms cadence must stay below the 100 ms threshold");

    // The old metric (now - stream_start) grew without bound; a long session
    // must NOT inflate the inter-frame average.
    for (size_t i = 0; i < 5000; ++i) {
        t += microseconds(1000);
        tracker.addSample(t);
    }
    QCOMPARE(tracker.averageUs(), int64_t(1000));
    QVERIFY(tracker.averageUs() < 100000);

    // Before MIN_SAMPLES the tracker reports invalid, so quality checks are
    // suppressed until real data has flowed.
    tracker.reset();
    QVERIFY(!tracker.valid());
    for (size_t i = 1; i < eti::InterFrameLatencyTracker::MIN_SAMPLES; ++i) {
        t += microseconds(1000);
        tracker.addSample(t);
    }
    QVERIFY(!tracker.valid());
}

// ============================================================================
// Receiver / worker hardening
// ============================================================================
void TestNetworkStreamValidation::testReceiverRejectsNonMulticastWithoutThrowOrRetry()
{
    eti::NetworkStreamConfig config;
    config.multicast_address = QStringLiteral("localhost");
    config.port = 9200;
    // Even with reconnection explicitly enabled, an invalid target must not
    // produce a retry storm: validation happens before any worker exists.
    config.auto_reconnect = true;
    config.max_reconnect_attempts = 10;
    config.reconnect_delay = std::chrono::milliseconds(10);

    eti::NetworkStreamReceiver receiver(config);  // must not throw
    QSignalSpy errorSpy(&receiver, &eti::NetworkStreamReceiver::error_occurred);
    QSignalSpy reconnectSpy(&receiver, &eti::NetworkStreamReceiver::reconnection_attempted);
    QVERIFY(errorSpy.isValid());
    QVERIFY(reconnectSpy.isValid());

    bool threw = false;
    bool connected = true;
    try {
        connected = receiver.connect_to_stream(QStringLiteral("localhost"), 9200);
        receiver.start_reception();
    } catch (...) {
        threw = true;
    }

    QVERIFY2(!threw, "connect_to_stream/start_reception must not throw on a hostname");
    QVERIFY2(!connected, "connect_to_stream must return false for a hostname");
    QVERIFY(!receiver.is_connected());
    QVERIFY(!receiver.get_last_error().isEmpty());
    QCOMPARE(errorSpy.count(), 1);  // exactly one clear error, no loop

    // No reconnect/error signals may arrive after the failed attempt.
    errorSpy.clear();
    reconnectSpy.clear();
    QTest::qWait(150);  // > configured reconnect_delay
    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(reconnectSpy.count(), 0);
}

void TestNetworkStreamValidation::testReceiverRejectsUnicastWithoutRetry()
{
    eti::NetworkStreamConfig config;
    config.multicast_address = QStringLiteral("192.168.1.100");
    config.port = 9200;
    config.auto_reconnect = true;
    config.reconnect_delay = std::chrono::milliseconds(10);

    eti::NetworkStreamReceiver receiver(config);
    QSignalSpy errorSpy(&receiver, &eti::NetworkStreamReceiver::error_occurred);
    QSignalSpy reconnectSpy(&receiver, &eti::NetworkStreamReceiver::reconnection_attempted);

    bool threw = false;
    bool connected = true;
    try {
        connected = receiver.connect_to_stream(QStringLiteral("192.168.1.100"), 9200);
        receiver.start_reception();
    } catch (...) {
        threw = true;
    }
    QVERIFY(!threw);
    QVERIFY(!connected);
    QCOMPARE(errorSpy.count(), 1);

    errorSpy.clear();
    reconnectSpy.clear();
    QTest::qWait(120);
    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(reconnectSpy.count(), 0);
}

void TestNetworkStreamValidation::testWorkerStartNoThrowOnInvalidAddress()
{
    // The worker slot runs on its own thread; a throw crossing that boundary
    // would terminate the process. Port 0 binds an ephemeral port so the
    // failure is deterministically the address rejection (not a busy port).
    eti::NetworkStreamConfig config;
    config.multicast_address = QStringLiteral("localhost");
    config.port = 0;

    eti::NetworkStreamWorker worker(config);
    QSignalSpy errorSpy(&worker, &eti::NetworkStreamWorker::error_occurred);
    QSignalSpy statusSpy(&worker, &eti::NetworkStreamWorker::connection_status_changed);
    QVERIFY(errorSpy.isValid());
    QVERIFY(statusSpy.isValid());

    bool threw = false;
    try {
        worker.start_reception();
    } catch (...) {
        threw = true;
    }

    QVERIFY2(!threw, "NetworkStreamWorker::start_reception must never throw");
    QCOMPARE(errorSpy.count(), 1);
    const QString message = errorSpy.first().at(0).toString();
    QVERIFY2(message.contains(QStringLiteral("multicast"), Qt::CaseInsensitive),
             qPrintable(QStringLiteral("error must name the multicast problem: %1").arg(message)));
    // A failed start must report the disconnected status exactly once.
    QCOMPARE(statusSpy.count(), 1);
    QCOMPARE(statusSpy.first().at(0).toBool(), false);

    // No retry: the worker has no auto-reconnect for a failed start.
    errorSpy.clear();
    statusSpy.clear();
    QTest::qWait(80);
    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(statusSpy.count(), 0);

    worker.stop_reception();
}

// ============================================================================
// F1: a connected worker receiving a fatal socket error must stop exactly
// once and report connection_status_changed(false) so the receiver's
// reconnect policy (and only the receiver's) can act.
// ============================================================================
void TestNetworkStreamValidation::testWorkerFatalErrorStopsReceptionOnce()
{
    eti::NetworkStreamConfig config;
    config.multicast_address = QStringLiteral("239.192.0.1");
    config.port = 0;  // never actually bound in this test

    eti::NetworkStreamWorker worker(config);
    QSignalSpy statusSpy(&worker, &eti::NetworkStreamWorker::connection_status_changed);
    QSignalSpy errorSpy(&worker, &eti::NetworkStreamWorker::error_occurred);
    QVERIFY(statusSpy.isValid());
    QVERIFY(errorSpy.isValid());

    worker.forceConnectedForTest();
    QVERIFY(worker.isConnectedForTest());
    QVERIFY(worker.isReceivingForTest());

    bool threw = false;
    try {
        worker.simulateFatalSocketErrorForTest();
    } catch (...) {
        threw = true;
    }
    QVERIFY2(!threw, "fatal socket error handling must never throw");
    QCOMPARE(errorSpy.count(), 1);    // the socket error itself
    QCOMPARE(statusSpy.count(), 1);   // exactly one lost-connection report
    QCOMPARE(statusSpy.first().at(0).toBool(), false);
    QVERIFY2(!worker.isConnectedForTest(), "fatal error must clear connected");
    QVERIFY2(!worker.isReceivingForTest(), "fatal error must stop reception");

    // Already stopped: a second fatal error surfaces the error but must not
    // emit another connection_status_changed(false).
    errorSpy.clear();
    statusSpy.clear();
    worker.simulateFatalSocketErrorForTest();
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(statusSpy.count(), 0);
}

void TestNetworkStreamValidation::testReceiverReconnectBoundedByMaxAttempts()
{
    eti::NetworkStreamConfig config;
    config.multicast_address = QStringLiteral("239.192.0.1");
    config.port = 9200;
    config.auto_reconnect = true;
    config.max_reconnect_attempts = 1;                 // one attempt budget
    // W2 F2: loss-driven retries are scheduled through the timer, so the delay
    // must be short enough for the test to observe the (rejected) second
    // attempt within the wait below.
    config.reconnect_delay = std::chrono::milliseconds(50);
    // Force the worker's join to fail fast and deterministically so the
    // reconnection attempt produces a second (rejected) loss event.
    config.interface_name = QStringLiteral("nonexistent_iface_zzz");

    eti::NetworkStreamReceiver receiver(config);
    QSignalSpy reconnectSpy(&receiver, &eti::NetworkStreamReceiver::reconnection_attempted);
    QSignalSpy errorSpy(&receiver, &eti::NetworkStreamReceiver::error_occurred);
    QVERIFY(reconnectSpy.isValid());
    QVERIFY(errorSpy.isValid());

    QVERIFY(receiver.connect_with_config(config));
    receiver.setReceivingForTest(true);

    // Simulate the loss reported by the worker's fatal error path. The first
    // loss (no attempt spent) retries immediately.
    receiver.simulateConnectionLostForTest();
    QCOMPARE(reconnectSpy.count(), 1);  // attempt #1 within budget

    // The new worker fails to join; its lost-connection report schedules the
    // next attempt via the timer. That attempt exceeds the budget and must not
    // exceed it (no unbounded retry storm).
    QTest::qWait(500);
    qInfo() << "[F1/F2] reconnection_attempted signals:" << reconnectSpy.count()
            << "receiver attempt counter:" << receiver.reconnectionAttemptsForTest()
            << "(max budget = 1) errors:" << errorSpy.count();
    QCOMPARE(reconnectSpy.count(), 1);
    QCOMPARE(receiver.reconnectionAttemptsForTest(), size_t(2));  // #2 rejected
    QVERIFY2(errorSpy.count() >= 1, "the exceeded-budget error must be reported");

    receiver.disconnect_from_stream();
}

// ============================================================================
// item 4: TCP + ZeroMQ transports over loopback (no external services)
// ============================================================================
QList<QByteArray> TestNetworkStreamValidation::loadFixtureFrames(int count) const
{
    const QStringList candidates = {
        QStringLiteral("../../eti/bkk_20062022_141637.eti"),
        QStringLiteral("../eti/bkk_20062022_141637.eti"),
        QStringLiteral("eti/bkk_20062022_141637.eti"),
        QStringLiteral(QT_TESTCASE_SOURCEDIR) + QStringLiteral("/../eti/bkk_20062022_141637.eti"),
    };
    QString path;
    for (const QString& c : candidates) {
        if (QFile::exists(c)) {
            path = c;
            break;
        }
    }
    QList<QByteArray> frames;
    if (path.isEmpty()) {
        return frames;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return frames;
    }
    const QByteArray blob = file.readAll();
    file.close();

    // The fixture may carry a leading header: start at the first ETI sync word.
    int start = -1;
    for (int i = 0; i + 4 <= blob.size(); ++i) {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(blob.constData()) + i;
        if (std::memcmp(p, eti::ETI_SYNC_PATTERN.data(), 4) == 0
            || std::memcmp(p, eti::ETI_SYNC_LI_A.data(), 4) == 0
            || std::memcmp(p, eti::ETI_SYNC_LI_B.data(), 4) == 0) {
            start = i;
            break;
        }
    }
    if (start < 0) {
        return frames;
    }
    for (int n = 0; n < count; ++n) {
        const int off = start + n * static_cast<int>(eti::ETI_FRAME_SIZE);
        if (off + static_cast<int>(eti::ETI_FRAME_SIZE) > blob.size()) {
            break;
        }
        frames.append(blob.mid(off, static_cast<int>(eti::ETI_FRAME_SIZE)));
    }
    return frames;
}

void TestNetworkStreamValidation::testTcpLoopbackReframesFixture()
{
    const QList<QByteArray> frames = loadFixtureFrames(3);
    if (frames.isEmpty()) {
        QSKIP("Real ETI fixture eti/bkk_20062022_141637.eti not found.");
    }

    QTcpServer server;
    QVERIFY2(server.listen(QHostAddress::LocalHost, 0), qPrintable(server.errorString()));
    const quint16 port = server.serverPort();

    eti::NetworkStreamConfig config;
    config.transport = eti::StreamTransport::Tcp;
    config.multicast_address = QStringLiteral("127.0.0.1");
    config.port = port;
    config.auto_reconnect = false;

    eti::NetworkStreamReceiver receiver(config);
    QSignalSpy frameSpy(&receiver, &eti::NetworkStreamReceiver::frame_received);
    QSignalSpy statusSpy(&receiver, &eti::NetworkStreamReceiver::connection_status_changed);
    QVERIFY(frameSpy.isValid());
    QVERIFY(statusSpy.isValid());

    QVERIFY2(receiver.connect_with_config(config), qPrintable(receiver.get_last_error()));
    receiver.start_reception();

    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 5000);
    QTcpSocket* client = server.nextPendingConnection();
    QVERIFY(client);
    QTRY_VERIFY_WITH_TIMEOUT(client->state() == QAbstractSocket::ConnectedState, 5000);
    // TCP is asynchronous: the worker confirms only after connected().
    QTRY_VERIFY_WITH_TIMEOUT(statusSpy.count() >= 1, 5000);
    QCOMPARE(statusSpy.first().at(0).toBool(), true);

    // A desynchronising prefix + the real frames, fed in awkward chunks so the
    // receiver must accumulate partial frames AND resync past the garbage.
    QByteArray stream;
    stream.append(QByteArrayLiteral("\x01\x02\x03GARBAGE!?"), 12);
    for (const QByteArray& f : frames) {
        stream.append(f);
    }

    const int total = stream.size();
    int written = 0;
    const QList<int> chunkSizes = {7, 100, 5000, total};
    for (int chunk : chunkSizes) {
        if (written >= total) {
            break;
        }
        const int n = qMin(chunk, total - written);
        client->write(stream.constData() + written, n);
        client->flush();
        written += n;
        QTest::qWait(15);
    }

    QTRY_COMPARE_WITH_TIMEOUT(frameSpy.count(), frames.size(), 5000);
    for (int i = 0; i < frames.size(); ++i) {
        QCOMPARE(frameSpy.at(i).at(0).toByteArray(), frames.at(i));
    }
    qInfo() << "[TCP] received" << frameSpy.count() << "fixture frames after a"
            << "12-byte desync prefix + 4 partial writes; status connected:"
            << statusSpy.first().at(0).toBool();

    receiver.stop_reception();
    receiver.disconnect_from_stream();
    client->disconnectFromHost();
    if (client->state() != QAbstractSocket::UnconnectedState) {
        client->waitForDisconnected(2000);
    }
}

void TestNetworkStreamValidation::testZmqLoopbackDelivery()
{
#ifdef HAVE_ZMQ
    const QList<QByteArray> frames = loadFixtureFrames(2);
    if (frames.isEmpty()) {
        QSKIP("Real ETI fixture eti/bkk_20062022_141637.eti not found.");
    }

    // In-process PUB socket (the ODR-DabMux role) on an ephemeral loopback port.
    zmq::context_t pubContext(1);
    zmq::socket_t pub(pubContext, zmq::socket_type::pub);
    pub.set(zmq::sockopt::linger, 0);
    quint16 port = 0;
    try {
        pub.bind("tcp://127.0.0.1:*");
        const std::string last = pub.get(zmq::sockopt::last_endpoint);
        const QString endpoint = QString::fromStdString(last);
        port = static_cast<quint16>(endpoint.section(QLatin1Char(':'), -1).toUInt());
    } catch (const zmq::error_t& e) {
        QSKIP(qPrintable(QStringLiteral("ZMQ loopback bind unavailable: %1").arg(e.what())));
    }
    if (port == 0) {
        QSKIP("ZMQ loopback endpoint had no usable port.");
    }

    eti::NetworkStreamConfig config;
    config.transport = eti::StreamTransport::Zmq;
    config.multicast_address = QStringLiteral("127.0.0.1");
    config.port = port;
    config.auto_reconnect = false;

    eti::NetworkStreamReceiver receiver(config);
    QSignalSpy frameSpy(&receiver, &eti::NetworkStreamReceiver::frame_received);
    QSignalSpy statusSpy(&receiver, &eti::NetworkStreamReceiver::connection_status_changed);
    QVERIFY(frameSpy.isValid());
    QVERIFY(statusSpy.isValid());

    QVERIFY2(receiver.connect_with_config(config), qPrintable(receiver.get_last_error()));
    receiver.start_reception();
    QTRY_VERIFY_WITH_TIMEOUT(statusSpy.count() >= 1, 2000);
    QCOMPARE(statusSpy.first().at(0).toBool(), true);

    // ZMQ PUB/SUB slow-joiner: keep publishing until the SUB has subscribed.
    int attempts = 0;
    while (frameSpy.count() < frames.size() && attempts < 400) {
        for (const QByteArray& f : frames) {
            pub.send(zmq::buffer(f.constData(), static_cast<size_t>(f.size())),
                     zmq::send_flags::none);
        }
        QTest::qWait(5);
        ++attempts;
    }

    QVERIFY2(frameSpy.count() >= frames.size(),
             qPrintable(QStringLiteral("ZMQ delivery failed after %1 attempts (got %2 frames)")
                            .arg(attempts).arg(frameSpy.count())));
    bool matched = false;
    for (int i = 0; i < frames.size(); ++i) {
        if (frameSpy.first().at(0).toByteArray() == frames.at(i)) {
            matched = true;
            break;
        }
    }
    QVERIFY2(matched, "received ZMQ frame does not match a published fixture frame");
    qInfo() << "[ZMQ] received" << frameSpy.count() << "frames from the loopback PUB after"
            << attempts << "publish attempts (slow-joiner); first frame matches a fixture frame";

    receiver.stop_reception();
    receiver.disconnect_from_stream();
#else
    QSKIP("ZeroMQ transport not built in (HAVE_ZMQ undefined); loopback test skipped.");
#endif
}

// ============================================================================
// W2 F1 regression: an INTENTIONAL live reconfiguration (the GUI's Settings ->
// buffer size -> Apply path) with auto-reconnect armed must be loss-silent.
// Previously cleanup_worker_thread() routed the worker's stop through the
// reconnect policy, producing a self-sustaining reconnect storm.
// ============================================================================
void TestNetworkStreamValidation::testReconfigureWithAutoReconnectIsQuiet()
{
    const QList<QByteArray> frames = loadFixtureFrames(2);
    if (frames.isEmpty()) {
        QSKIP("Real ETI fixture eti/bkk_20062022_141637.eti not found.");
    }

    QTcpServer server;
    QVERIFY2(server.listen(QHostAddress::LocalHost, 0), qPrintable(server.errorString()));
    const quint16 port = server.serverPort();

    eti::NetworkStreamConfig config;
    config.transport = eti::StreamTransport::Tcp;
    config.multicast_address = QStringLiteral("127.0.0.1");
    config.port = port;
    config.auto_reconnect = false;   // initial attempt: like the GUI

    eti::NetworkStreamReceiver receiver(config);
    QSignalSpy frameSpy(&receiver, &eti::NetworkStreamReceiver::frame_received);
    QSignalSpy reconnectSpy(&receiver, &eti::NetworkStreamReceiver::reconnection_attempted);
    QSignalSpy statusSpy(&receiver, &eti::NetworkStreamReceiver::connection_status_changed);
    QVERIFY(frameSpy.isValid());
    QVERIFY(reconnectSpy.isValid());
    QVERIFY(statusSpy.isValid());

    QVERIFY2(receiver.connect_with_config(config), qPrintable(receiver.get_last_error()));
    receiver.start_reception();

    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 5000);
    QTcpSocket* client = server.nextPendingConnection();
    QVERIFY(client);
    QTRY_VERIFY_WITH_TIMEOUT(statusSpy.count() >= 1, 5000);
    QCOMPARE(statusSpy.first().at(0).toBool(), true);

    // A confirmed connection arms auto-reconnect, exactly like the GUI.
    receiver.enable_automatic_reconnection(true, 10);

    // Make the session live.
    for (const QByteArray& f : frames) {
        client->write(f);
    }
    client->flush();
    QTRY_VERIFY_WITH_TIMEOUT(frameSpy.count() >= frames.size(), 5000);

    const int statusBefore = statusSpy.count();
    reconnectSpy.clear();

    // The reproduced GUI chain: Settings -> buffer size -> Apply.
    receiver.set_buffer_size(500);
    QCOMPARE(receiver.worker_buffer_size_for_test(), size_t(500));

    // The rebuilt session reconnects (new TCP accept) and re-confirms (true).
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 5000);
    QTcpSocket* client2 = server.nextPendingConnection();
    QVERIFY(client2);
    QTRY_VERIFY_WITH_TIMEOUT(statusSpy.count() > statusBefore, 5000);

    // Give a stale queued loss event every chance to arrive — this is the exact
    // window in which the old code produced ~566 attempts / 3 s.
    QTest::qWait(1000);

    bool sawFalse = false;
    for (int i = statusBefore; i < statusSpy.count(); ++i) {
        if (!statusSpy.at(i).at(0).toBool()) {
            sawFalse = true;
        }
    }
    QVERIFY2(!sawFalse,
             "intentional reconfiguration must not emit connection_status_changed(false)");
    QCOMPARE(reconnectSpy.count(), 0);

    // The rebuilt stream still delivers frames.
    const int framesBefore = frameSpy.count();
    for (const QByteArray& f : frames) {
        client2->write(f);
    }
    client2->flush();
    QTRY_VERIFY_WITH_TIMEOUT(frameSpy.count() >= framesBefore + frames.size(), 5000);

    qInfo() << "[F1] set_buffer_size rebuild: reconnection_attempted ="
            << reconnectSpy.count()
            << "false status events =" << (sawFalse ? 1 : 0)
            << "frames after rebuild =" << (frameSpy.count() - framesBefore);

    receiver.disconnect_from_stream();
    client->disconnectFromHost();
    client2->disconnectFromHost();
}

QTEST_MAIN(TestNetworkStreamValidation)
#include "test_network_stream_validation.moc"
