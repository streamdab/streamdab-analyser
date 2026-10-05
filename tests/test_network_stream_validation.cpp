/**
 * @file test_network_stream_validation.cpp
 * @brief T22 network-stream robustness tests.
 *
 * Covers two independent layers:
 *   1. `eti::validate_stream_url()` — the pure URL validator shared by the UI
 *      pre-check and the receiver. Only IPv4 multicast (`udp://A.B.C.D[:port]`
 *      or `A.B.C.D[:port]`) is accepted; hostnames, unicast, TCP/ZMQ and bad
 *      ports are rejected with an error.
 *   2. `NetworkStreamReceiver` / `NetworkStreamWorker` hardening — a
 *      non-multicast target must fail fast: return false / emit
 *      `error_occurred` exactly once, WITHOUT throwing and WITHOUT scheduling
 *      reconnect attempts (the reported tight loop).
 *
 * @date October 2026
 */

#include <QtTest/QtTest>
#include <QSignalSpy>

#include "core/network_stream_receiver.hpp"

class TestNetworkStreamValidation : public QObject
{
    Q_OBJECT

private slots:
    // --- URL validator -----------------------------------------------------
    void testAcceptsSupportedMulticastUrls();
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

void TestNetworkStreamValidation::testRejectsUnsupportedUrls_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("why");

    QTest::newRow("hostname")          << QStringLiteral("localhost")              << "hostname";
    QTest::newRow("zmq+tcp")           << QStringLiteral("zmq+tcp://localhost:9201") << "zmq scheme";
    QTest::newRow("tcp")               << QStringLiteral("tcp://192.168.1.100:9200") << "tcp scheme";
    QTest::newRow("unicast")           << QStringLiteral("192.168.1.100:9200")    << "unicast";
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
    config.reconnect_delay = std::chrono::milliseconds(10000);  // timer never fires
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

    // Simulate the loss reported by the worker's fatal error path.
    receiver.simulateConnectionLostForTest();
    QCOMPARE(reconnectSpy.count(), 1);  // attempt #1 within budget

    // The new worker fails to join; its lost-connection report must not
    // exceed the configured budget (no unbounded retry storm).
    QTest::qWait(500);
    qInfo() << "[F1] reconnection_attempted signals:" << reconnectSpy.count()
            << "receiver attempt counter:" << receiver.reconnectionAttemptsForTest()
            << "(max budget = 1) errors:" << errorSpy.count();
    QCOMPARE(reconnectSpy.count(), 1);
    QCOMPARE(receiver.reconnectionAttemptsForTest(), size_t(2));  // #2 rejected
    QVERIFY2(errorSpy.count() >= 1, "the exceeded-budget error must be reported");

    receiver.stop_reception();
    receiver.disconnect_from_stream();
}

QTEST_MAIN(TestNetworkStreamValidation)
#include "test_network_stream_validation.moc"
