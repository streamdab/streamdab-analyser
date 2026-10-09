/**
 * @file test_qt_audio_output_sink.cpp
 * @brief T34-ext: Qt Multimedia (QAudioSink) sink + backend factory tests.
 *
 * Runs on all three platforms and adapts to the compile-time configuration:
 *  - Always: `backendAvailable()` matches the build, enumerate/default queries
 *    never touch hardware, and the controller's backend preference is checked
 *    per-platform via #ifdef.
 *  - With HAVE_QMULTIMEDIA UNSET (this dev host): the graceful-degradation
 *    stubs are proven (`open()`/`start()` return false with a non-empty
 *    `lastError()`, the simulated hook is a no-op stub).
 *  - With HAVE_QMULTIMEDIA SET (Windows/macOS): the QAudioFormat mapping helper
 *    is validated and the same stalled-device close/destructor no-hang proof as
 *    test_audio_output_sink is run against the Qt backend's simulated write.
 *
 * @date October 2026
 */

#include <QtTest/QtTest>

#include <QByteArray>
#include <QElapsedTimer>
#include <memory>

#ifdef HAVE_QMULTIMEDIA
#include <QAudioFormat>
#endif

#include "core/qt_audio_output_sink.hpp"
#include "core/audio_playback_controller.hpp"

using streamdab::audio::QtAudioOutputSink;
using streamdab::audio::AudioPlaybackController;

class TestQtAudioOutputSink : public QObject
{
    Q_OBJECT

private slots:
    void testBackendAvailabilityMatchesCompileTimeConfig();
    void testGracefulDegradationWithoutModule();
    void testEnumerateAndDefaultNeverTouchHardware();
    void testControllerPreferredBackend();
    void testSinkSelectionInvariant();
    void testFormatMapping();          // QSKIPs without the module
    void testStalledShutdownIsBounded();  // QSKIPs without the module
};

void TestQtAudioOutputSink::testBackendAvailabilityMatchesCompileTimeConfig()
{
#ifdef HAVE_QMULTIMEDIA
    QVERIFY2(QtAudioOutputSink::backendAvailable(),
             "HAVE_QMULTIMEDIA is set: the Qt backend must be available");
#else
    QVERIFY2(!QtAudioOutputSink::backendAvailable(),
             "HAVE_QMULTIMEDIA unset: the Qt backend must report unavailable");
#endif
    qInfo() << "[Qt-sink] backendAvailable:" << QtAudioOutputSink::backendAvailable();
}

void TestQtAudioOutputSink::testEnumerateAndDefaultNeverTouchHardware()
{
    // enumerate()/defaultDeviceName() never open a device and must always yield
    // at least the default fallback.
    const QList<QtAudioOutputSink::DeviceInfo> devices =
        QtAudioOutputSink::enumerateOutputDevices();
    QVERIFY2(!devices.isEmpty(), "enumerate must always yield >=1 device");
    bool hasDefault = false;
    for (const QtAudioOutputSink::DeviceInfo& d : devices) {
        QVERIFY(!d.name.isEmpty());
        if (d.isDefault) {
            hasDefault = true;
        }
    }
    QVERIFY2(hasDefault, "the default device must be present");
    QVERIFY(!QtAudioOutputSink::defaultDeviceName().isEmpty());
}

void TestQtAudioOutputSink::testGracefulDegradationWithoutModule()
{
#ifdef HAVE_QMULTIMEDIA
    QSKIP("Qt Multimedia is built in: the degradation stubs are not compiled");
#endif
    QtAudioOutputSink sink;
    QVERIFY(!QtAudioOutputSink::backendAvailable());

    QVERIFY2(!sink.open(), "open() must fail when the module is not built in");
    QVERIFY2(!sink.lastError().isEmpty(),
             "a failed open() must set a non-empty lastError()");
    QVERIFY(!sink.isOpen());
    QVERIFY(!sink.isPlaying());

    auto pcm = std::make_shared<QByteArray>(48000 * 2 * 2 * 10, '\0');  // 10 s
    QVERIFY2(!sink.start(pcm, 48000, 2, 0),
             "start() must fail when the module is not built in");
    QVERIFY2(!sink.lastError().isEmpty(),
             "a failed start() must leave a non-empty lastError()");
    QVERIFY(!sink.isPlaying());
    QCOMPARE(sink.positionFrames(), qint64(0));
    QCOMPARE(sink.totalFrames(), qint64(0));

    sink.close();  // must be a safe no-op

    // The simulated hook is a no-op stub when the module is unset: enabling it
    // must NOT make open()/start() succeed.
    QtAudioOutputSink::setSimulatedBackendForTest(true, 3000);
    QVERIFY2(!sink.open(), "the no-op simulated hook must not resurrect open()");
    QtAudioOutputSink::setSimulatedBackendForTest(false, 0);
}

void TestQtAudioOutputSink::testControllerPreferredBackend()
{
#if defined(HAVE_ALSA)
    QCOMPARE(AudioPlaybackController::preferredBackendName(), QStringLiteral("alsa"));
#elif defined(HAVE_QMULTIMEDIA)
    QCOMPARE(AudioPlaybackController::preferredBackendName(),
             QStringLiteral("qt-multimedia"));
#else
    QCOMPARE(AudioPlaybackController::preferredBackendName(), QStringLiteral("none"));
#endif
    qInfo() << "[Qt-sink] controller preferred backend:"
            << AudioPlaybackController::preferredBackendName();
}

void TestQtAudioOutputSink::testSinkSelectionInvariant()
{
    // Constructing the controller selects the preferred sink; it must never be
    // null and its backendName() must match the build-time preference.
    AudioPlaybackController controller;
    QCOMPARE(controller.backendName(), AudioPlaybackController::preferredBackendName());
    // audioBackendAvailable() iff a real backend was compiled in.
    QCOMPARE(AudioPlaybackController::audioBackendAvailable(),
             AudioPlaybackController::preferredBackendName() != QStringLiteral("none"));
    // Static queries must never empty-crash on any configuration.
    QVERIFY(!AudioPlaybackController::availableOutputDevices().isEmpty());
}

void TestQtAudioOutputSink::testFormatMapping()
{
#ifdef HAVE_QMULTIMEDIA
    const QAudioFormat stereo = QtAudioOutputSink::qtAudioFormatFor(48000, 2);
    QCOMPARE(stereo.sampleRate(), 48000);
    QCOMPARE(stereo.channelCount(), 2);
    QCOMPARE(stereo.sampleFormat(), QAudioFormat::Int16);
    QCOMPARE(stereo.byteOrder(), QAudioFormat::LittleEndian);
    QVERIFY2(stereo.isValid(), "the sink format must be valid");
    const QAudioFormat mono = QtAudioOutputSink::qtAudioFormatFor(24000, 1);
    QCOMPARE(mono.sampleRate(), 24000);
    QCOMPARE(mono.channelCount(), 1);
    QVERIFY2(mono.isValid(), "the mono sink format must be valid");
#else
    QSKIP("Qt Multimedia not built in: format mapping is unavailable");
#endif
}

void TestQtAudioOutputSink::testStalledShutdownIsBounded()
{
#ifndef HAVE_QMULTIMEDIA
    QSKIP("Qt Multimedia not built in: simulated Qt sink backend unavailable");
#endif
    // Simulate a device whose write blocks 3 s per call. Without the bounded,
    // cancellable write path close()/the destructor would hang here.
    QtAudioOutputSink::setSimulatedBackendForTest(true, 3000);
    {
        QtAudioOutputSink sink;
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
        qInfo() << "[Qt-sink] close() on a stalled device took" << elapsed << "ms";
        QVERIFY2(elapsed < 500, qPrintable(QString("close() hung for %1 ms").arg(elapsed)));
        QVERIFY(!sink.isOpen());
    }
    QtAudioOutputSink::setSimulatedBackendForTest(false, 0);
}

QTEST_MAIN(TestQtAudioOutputSink)
#include "test_qt_audio_output_sink.moc"