/**
 * @file test_audio_output_sink.cpp
 * @brief T34 review gate 5: ALSA sink lifecycle + no-hang regression tests.
 *
 * Headless-safe: the hardware is not required. A deterministic simulated
 * backend (see AudioOutputSink::setSimulatedBackendForTest) simulates a device
 * whose write blocks for seconds; the test proves `close()` / destruction still
 * returns promptly (the review F1 class of teardown hang). The real backend is
 * exercised only for the graceful-failure paths (bad device name), never for a
 * blocking playback that CI cannot provide.
 *
 * @date October 2026
 */

#include <QtTest/QtTest>

#include <QByteArray>
#include <QElapsedTimer>
#include <memory>

#include "core/audio_output_sink.hpp"

using streamdab::audio::AudioOutputSink;

class TestAudioOutputSink : public QObject
{
    Q_OBJECT

private slots:
    void testEnumerateAndBackendQuery();
    void testOpenBadDeviceFailsGracefully();
    void testStalledDeviceShutdownIsBounded();
    void testDestructorWhileStalledIsBounded();
};

void TestAudioOutputSink::testEnumerateAndBackendQuery()
{
    // backendAvailable() is a build capability flag; enumerate never opens a
    // device and must always yield at least the default fallback.
    const bool available = AudioOutputSink::backendAvailable();
    qInfo() << "[T34-sink] backendAvailable:" << available;
    const QList<AudioOutputSink::DeviceInfo> devices =
        AudioOutputSink::enumerateOutputDevices();
    QVERIFY2(!devices.isEmpty(), "enumerate must always yield >=1 device");
    bool hasDefault = false;
    for (const AudioOutputSink::DeviceInfo& d : devices) {
        QVERIFY(!d.name.isEmpty());
        if (d.isDefault) {
            hasDefault = true;
        }
    }
    QVERIFY2(hasDefault, "the default device must be present");
}

void TestAudioOutputSink::testOpenBadDeviceFailsGracefully()
{
    if (!AudioOutputSink::backendAvailable()) {
        QSKIP("no backend: open() must fail gracefully, covered by the other path");
    }
    AudioOutputSink sink;
    QVERIFY2(!sink.open(QStringLiteral("streamdab-does-not-exist-xyz")),
             "opening a non-existent device must fail, not crash");
    QVERIFY2(!sink.lastError().isEmpty(), "a failed open must set lastError()");
    QVERIFY(!sink.isOpen());
    QVERIFY(!sink.isPlaying());
    sink.close();  // must be a safe no-op
}

void TestAudioOutputSink::testStalledDeviceShutdownIsBounded()
{
    // Simulate a device whose write blocks 3 s per call. Without the
    // non-blocking/cancellable write path the destructor would hang here.
    AudioOutputSink::setSimulatedBackendForTest(true, 3000);
    {
        AudioOutputSink sink;
        QVERIFY(sink.open());
        QVERIFY(sink.isOpen());
        auto pcm = std::make_shared<QByteArray>(48000 * 2 * 2 * 10, '\0');  // 10 s
        QVERIFY(sink.start(pcm, 48000, 2, 0));
        QVERIFY(sink.isPlaying());
        // Let the writer enter the simulated 3-s blocking write before we close.
        QTest::qWait(50);
        QVERIFY(sink.isPlaying());

        QElapsedTimer timer;
        timer.start();
        sink.close();  // join must not wait for the stalled write
        const qint64 elapsed = timer.elapsed();
        qInfo() << "[T34-sink] close() on a stalled device took" << elapsed << "ms";
        QVERIFY2(elapsed < 500, qPrintable(QString("close() hung for %1 ms").arg(elapsed)));
        QVERIFY(!sink.isOpen());
    }
    AudioOutputSink::setSimulatedBackendForTest(false, 0);
}

void TestAudioOutputSink::testDestructorWhileStalledIsBounded()
{
    AudioOutputSink::setSimulatedBackendForTest(true, 3000);
    QElapsedTimer timer;
    timer.start();
    {
        // Heap-allocated so the destructor runs at the closing brace.
        auto sink = std::make_unique<AudioOutputSink>();
        QVERIFY(sink->open());
        auto pcm = std::make_shared<QByteArray>(48000 * 2 * 2 * 10, '\1');
        QVERIFY(sink->start(pcm, 48000, 2, 0));
        QVERIFY(sink->isPlaying());
        // Ensure the writer is inside the simulated 3-s blocked write.
        QTest::qWait(50);
        QVERIFY(sink->isPlaying());
    }
    const qint64 elapsed = timer.elapsed();
    qInfo() << "[T34-sink] destructor on a stalled device took" << elapsed << "ms";
    QVERIFY2(elapsed < 500,
             qPrintable(QString("destructor hung for %1 ms").arg(elapsed)));
    AudioOutputSink::setSimulatedBackendForTest(false, 0);
}

QTEST_MAIN(TestAudioOutputSink)
#include "test_audio_output_sink.moc"
