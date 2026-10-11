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
#include <QSignalSpy>

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
    // Pull-mode behaviour through the simulated engine (no sound card needed).
    void testResampler();
    void testPlaybackAdvancesLevelsAndEnds();
    void testPauseHoldsPositionAndResumeContinues();
    void testSeekRepositions();
    void testStopReleasesBuffer();
    void testControlCallsFromSignalHandlersDoNotBlock();
    void testSecondStartWithOtherFormat();
};

namespace {
// Interleaved S16 PCM with a constant-amplitude square wave.
std::shared_ptr<const QByteArray> makePcm(int sampleRate, int channels, double seconds,
                                          qint16 amplitude = 16000)
{
    const qint64 frames = static_cast<qint64>(sampleRate * seconds);
    auto buf = std::make_shared<QByteArray>();
    buf->resize(static_cast<qsizetype>(frames * channels * 2));
    qint16* p = reinterpret_cast<qint16*>(buf->data());
    for (qint64 i = 0; i < frames; ++i) {
        const qint16 v = ((i / 24) % 2) ? amplitude : static_cast<qint16>(-amplitude);
        for (int c = 0; c < channels; ++c) {
            p[i * channels + c] = v;
        }
    }
    return buf;
}

// RAII: simulated engine on for the scope of a test.
struct SimulatedEngine {
    SimulatedEngine() { QtAudioOutputSink::setSimulatedBackendForTest(true, 0); }
    ~SimulatedEngine() { QtAudioOutputSink::setSimulatedBackendForTest(false, 0); }
};
} // namespace

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
    // A "stalled" device cannot pin close() any more (pull mode has no writer to
    // join); kept as a guard that teardown stays immediate while playing.
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

void TestQtAudioOutputSink::testResampler()
{
    // Pure helper: always compiled, no module needed.
    QByteArray ramp;
    ramp.resize(48000 * 2);  // 1 s mono
    qint16* r = reinterpret_cast<qint16*>(ramp.data());
    for (int i = 0; i < 48000; ++i) {
        r[i] = static_cast<qint16>(i / 2);  // slow ramp 0..23999
    }
    // same rate: unchanged
    QCOMPARE(QtAudioOutputSink::resampleLinearS16(ramp, 1, 48000, 48000), ramp);
    // 48k -> 24k halves the frame count and keeps the shape
    const QByteArray half = QtAudioOutputSink::resampleLinearS16(ramp, 1, 48000, 24000);
    QCOMPARE(half.size(), 24000 * 2);
    const qint16* h = reinterpret_cast<const qint16*>(half.constData());
    QCOMPARE(int(h[0]), 0);
    QVERIFY(std::abs(int(h[12000]) - 12000) <= 2);
    // 24k -> 48k doubles it
    const QByteArray twice = QtAudioOutputSink::resampleLinearS16(half, 1, 24000, 48000);
    QCOMPARE(twice.size(), 48000 * 2);
    // stereo keeps channel interleaving
    QByteArray st;
    st.resize(100 * 2 * 2);
    qint16* sp = reinterpret_cast<qint16*>(st.data());
    for (int i = 0; i < 100; ++i) { sp[i * 2] = 1000; sp[i * 2 + 1] = -1000; }
    const QByteArray st2 = QtAudioOutputSink::resampleLinearS16(st, 2, 32000, 48000);
    const qint16* q = reinterpret_cast<const qint16*>(st2.constData());
    QCOMPARE(int(q[0]), 1000);
    QCOMPARE(int(q[1]), -1000);
    QCOMPARE(int(q[st2.size() / 2 - 2]), 1000);
    // invalid arguments
    QVERIFY(QtAudioOutputSink::resampleLinearS16(QByteArray(), 2, 48000, 24000).isEmpty());
    QVERIFY(QtAudioOutputSink::resampleLinearS16(ramp, 0, 48000, 24000).isEmpty());
    QVERIFY(QtAudioOutputSink::resampleLinearS16(ramp, 1, 0, 24000).isEmpty());
}

void TestQtAudioOutputSink::testPlaybackAdvancesLevelsAndEnds()
{
#ifndef HAVE_QMULTIMEDIA
    QSKIP("Qt Multimedia not built in: simulated pull engine unavailable");
#else
    SimulatedEngine engine;
    QtAudioOutputSink sink;
    QVERIFY(sink.open());
    const auto pcm = makePcm(48000, 2, 0.5);
    QSignalSpy ended(&sink, &QtAudioOutputSink::playbackEnded);
    QSignalSpy levels(&sink, &QtAudioOutputSink::levelsChanged);
    QVERIFY(sink.start(pcm, 48000, 2, 0));
    QVERIFY(sink.isPlaying());
    QCOMPARE(sink.totalFrames(), qint64(24000));
    QTRY_VERIFY_WITH_TIMEOUT(ended.count() == 1, 3000);
    QVERIFY(!sink.isPlaying());
    QCOMPARE(sink.positionFrames(), qint64(24000));
    QVERIFY2(levels.count() >= 3, "level meters must keep updating during playback");
    float maxPeak = 0.0f;
    for (const QList<QVariant>& args : levels) {
        maxPeak = std::max(maxPeak, args.at(0).toFloat());
    }
    QVERIFY2(maxPeak > 0.3f, qPrintable(QString("peak %1 too low").arg(maxPeak)));
#endif
}

void TestQtAudioOutputSink::testPauseHoldsPositionAndResumeContinues()
{
#ifndef HAVE_QMULTIMEDIA
    QSKIP("Qt Multimedia not built in: simulated pull engine unavailable");
#else
    SimulatedEngine engine;
    QtAudioOutputSink sink;
    QVERIFY(sink.open());
    QVERIFY(sink.start(makePcm(48000, 2, 5.0), 48000, 2, 0));
    QTRY_VERIFY_WITH_TIMEOUT(sink.positionFrames() > 2000, 2000);
    sink.pause();
    QVERIFY(!sink.isPlaying());
    const qint64 held = sink.positionFrames();
    QTest::qWait(150);
    QCOMPARE(sink.positionFrames(), held);
    sink.resume();
    QVERIFY(sink.isPlaying());
    QTRY_VERIFY_WITH_TIMEOUT(sink.positionFrames() > held + 2000, 2000);
    sink.stop();
#endif
}

void TestQtAudioOutputSink::testSeekRepositions()
{
#ifndef HAVE_QMULTIMEDIA
    QSKIP("Qt Multimedia not built in: simulated pull engine unavailable");
#else
    SimulatedEngine engine;
    QtAudioOutputSink sink;
    QVERIFY(sink.open());
    QVERIFY(sink.start(makePcm(48000, 2, 5.0), 48000, 2, 0));
    sink.seek(200000);
    QVERIFY(sink.positionFrames() >= 200000);
    QVERIFY(sink.positionFrames() < 205000);
    sink.seek(10);
    QVERIFY(sink.positionFrames() < 5000);
    sink.stop();
#endif
}

void TestQtAudioOutputSink::testStopReleasesBuffer()
{
#ifndef HAVE_QMULTIMEDIA
    QSKIP("Qt Multimedia not built in: simulated pull engine unavailable");
#else
    SimulatedEngine engine;
    QtAudioOutputSink sink;
    QVERIFY(sink.open());
    auto pcm = makePcm(48000, 2, 1.0);
    std::weak_ptr<const QByteArray> weak = pcm;
    QVERIFY(sink.start(pcm, 48000, 2, 0));
    pcm.reset();
    QVERIFY2(!weak.expired(), "the sink must keep the buffer alive while playing");
    sink.stop();
    QVERIFY(!sink.isPlaying());
    QCOMPARE(sink.positionFrames(), qint64(0));
    QVERIFY2(weak.expired(), "stop() must release the PCM buffer");
    sink.resume();  // nothing loaded: must be a harmless no-op
    QVERIFY(!sink.isPlaying());
#endif
}

void TestQtAudioOutputSink::testControlCallsFromSignalHandlersDoNotBlock()
{
#ifndef HAVE_QMULTIMEDIA
    QSKIP("Qt Multimedia not built in: simulated pull engine unavailable");
#else
    // Regression for the Windows/macOS Play freeze: the old sink held a mutex
    // across QAudioSink::start() while its stateChanged handler took the same
    // mutex, so a control call made from inside a signal handler deadlocked the
    // GUI thread. Control calls from handlers must now return immediately.
    SimulatedEngine engine;
    QtAudioOutputSink sink;
    QVERIFY(sink.open());
    int calls = 0;
    QObject::connect(&sink, &QtAudioOutputSink::levelsChanged, &sink,
                     [&sink, &calls](float, float) {
                         ++calls;
                         sink.setVolume(0.5);
                         sink.pause();
                         sink.resume();
                         sink.seek(100);
                     });
    QElapsedTimer watchdog;
    watchdog.start();
    QVERIFY(sink.start(makePcm(48000, 2, 2.0), 48000, 2, 0));
    QTRY_VERIFY_WITH_TIMEOUT(calls >= 5, 3000);
    sink.stop();
    sink.close();
    QVERIFY2(watchdog.elapsed() < 3500, "control calls from handlers must not block");
#endif
}

void TestQtAudioOutputSink::testSecondStartWithOtherFormat()
{
#ifndef HAVE_QMULTIMEDIA
    QSKIP("Qt Multimedia not built in: simulated pull engine unavailable");
#else
    // Another service can have another sample rate / channel count: the second
    // start() must adopt it instead of reusing the first stream's parameters.
    SimulatedEngine engine;
    QtAudioOutputSink sink;
    QVERIFY(sink.open());
    QVERIFY(sink.start(makePcm(48000, 2, 1.0), 48000, 2, 0));
    QCOMPARE(sink.totalFrames(), qint64(48000));
    QVERIFY(sink.start(makePcm(24000, 1, 1.0), 24000, 1, 0));
    QCOMPARE(sink.totalFrames(), qint64(24000));
    QSignalSpy ended(&sink, &QtAudioOutputSink::playbackEnded);
    QTRY_VERIFY_WITH_TIMEOUT(ended.count() == 1, 3000);
    QCOMPARE(sink.positionFrames(), qint64(24000));
#endif
}

QTEST_MAIN(TestQtAudioOutputSink)
#include "test_qt_audio_output_sink.moc"