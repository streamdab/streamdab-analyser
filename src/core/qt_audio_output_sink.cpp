/**
 * @file qt_audio_output_sink.cpp
 * @brief T34-ext: Qt Multimedia (QAudioSink) playback sink implementation.
 *
 * See the header for the lifecycle/threading contract. The writer thread is the
 * only user of the QAudioSink / the internal push-mode QIODevice; every call is
 * serialized by `m_sinkMutex`. In push mode (`QAudioSink::start()` without a
 * device) the writer writes at most `bytesFree()` bytes per call — which never
 * blocks — and when the ring buffer is full it waits with a bounded sleep,
 * checking the cancellation flag, so `close()` and the destructor can never
 * hang on a stalled device.
 *
 * When the module is absent (`HAVE_QMULTIMEDIA` unset) every QAudio and QMedia
 * symbol is compiled out and the class provides the same graceful-degradation
 * stubs as `AudioOutputSink` without ALSA.
 *
 * @author C++ Qt Developer Agent (T34-ext)
 */

#include "qt_audio_output_sink.hpp"

#include <QMetaObject>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>   // must precede <chrono> (GCC 13: <chrono> pulls <format>)
#include <thread>

#ifdef HAVE_QMULTIMEDIA
#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QIODevice>
#include <QMediaDevices>
#endif

namespace streamdab::audio {

#ifdef HAVE_QMULTIMEDIA
// The audio enums lived in the QAudio namespace up to and including Qt 6.6 and
// moved to QtAudio (as scoped enums) in Qt 6.7. A single alias + C++11
// `EnumName::Enumerator` spelling keeps both families source-compatible.
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
namespace qtns = QtAudio;
#else
namespace qtns = QAudio;
#endif
#endif

namespace {
// Test-only simulated backend state (see setSimulatedBackendForTest()).
std::atomic<bool> g_simulated{false};
std::atomic<int> g_simStallMs{0};

#ifdef HAVE_QMULTIMEDIA
/**
 * @brief Resolve a device requested by id/description (or the system default
 * when @p wanted is empty or "default"). Returns false when no such device
 * exists — mirrors ALSA's failed snd_pcm_open.
 */
bool resolveDevice(const QString& wanted, QAudioDevice* out)
{
    const QList<QAudioDevice> outputs = QMediaDevices::audioOutputs();
    if (wanted.isEmpty() || wanted == QLatin1String("default")) {
        QAudioDevice def = QMediaDevices::defaultAudioOutput();
        if (!def.isNull()) {
            *out = def;
            return true;
        }
        if (!outputs.isEmpty()) {
            *out = outputs.first();
            return true;
        }
        return false;
    }
    for (const QAudioDevice& d : outputs) {
        if (d.id() == wanted || d.description() == wanted) {
            *out = d;
            return true;
        }
    }
    return false;
}
#endif
} // namespace

class QtAudioOutputSink::Impl {
public:
    ~Impl() { shutdown(); }

    // ------------------------------------------------------------------
    // Sink ownership. All QAudioSink/QIODevice access happens under
    // m_sinkMutex (the writer locks it only for a non-blocking write, the
    // GUI-thread control paths for short control calls).
    // ------------------------------------------------------------------
    void closeSink()
    {
#ifdef HAVE_QMULTIMEDIA
        std::lock_guard<std::mutex> lock(m_sinkMutex);
        // The QAudioSink destructor releases the device resources immediately
        // (no synchronous drain); deleting before joining guarantees a stalled
        // device can never pin close()/the destructor.
        m_sink.reset();
        m_io = nullptr;
#endif
        m_open = false;
    }

    void dropSink()
    {
#ifdef HAVE_QMULTIMEDIA
        std::lock_guard<std::mutex> lock(m_sinkMutex);
        if (m_sink) {
            m_sink->stop();
            m_io = nullptr;  // start() must be called again before the next write
        }
#endif
    }

    void suspendSink()
    {
#ifdef HAVE_QMULTIMEDIA
        std::lock_guard<std::mutex> lock(m_sinkMutex);
        if (m_sink) {
            m_sink->suspend();
        }
#endif
    }

    void resumeSink()
    {
#ifdef HAVE_QMULTIMEDIA
        std::lock_guard<std::mutex> lock(m_sinkMutex);
        if (m_sink) {
            m_sink->resume();
        }
#endif
    }

    void seekSink()
    {
#ifdef HAVE_QMULTIMEDIA
        std::lock_guard<std::mutex> lock(m_sinkMutex);
        if (m_sink && m_open.load()) {
            // Discard stale pre-seek buffered audio immediately. reset() (unlike
            // stop()) does not synchronously drain, so seek stays prompt even on
            // a stalled device. Some backends fall back to StoppedState after
            // reset(); re-arm push mode in that case so the writer can continue.
            m_sink->reset();
            if (m_sink->state() == qtns::State::StoppedState) {
                m_io = m_sink->start();
            }
        }
#endif
    }

    void applyVolumeMute()
    {
#ifdef HAVE_QMULTIMEDIA
        std::lock_guard<std::mutex> lock(m_sinkMutex);
        if (m_sink) {
            m_sink->setMuted(m_muted.load());
            m_sink->setVolume(static_cast<qreal>(m_volume.load()));
        }
#else
        // Volume/mute are stored in the atomics even without the backend so the
        // query API behaves identically.
#endif
    }

    /**
     * @brief Stop, destroy and join. Bounded by construction.
     *
     * Order matters: clear the running flag, then drop the PCM reference, then
     * destroy the sink (which wakes/aborts any writer parked waiting on the
     * device), THEN join. The writer holds m_sinkMutex only for the
     * (non-blocking, <= bytesFree) write, never during a wait, so closeSink()
     * cannot block on it for long.
     */
    void shutdown()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_running = false;
            m_playing = false;
            m_pcm.reset();
        }
        m_cv.notify_all();
        closeSink();
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    void ensureThread(QtAudioOutputSink* owner)
    {
        if (!m_thread.joinable()) {
            m_running = true;
            m_thread = std::thread(&Impl::run, this, owner);
        }
    }

    // ------------------------------------------------------------------
    // Writer thread
    // ------------------------------------------------------------------
    void run(QtAudioOutputSink* owner)
    {
        using namespace std::chrono;
        qint64 lastLevelUs = 0;
        while (true) {
            std::shared_ptr<const QByteArray> pcm;
            int sampleRate = 0;
            int channels = 0;
            qint64 total = 0;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                if (!m_running) {
                    break;
                }
                if (!m_playing || !m_pcm || m_sampleRate <= 0 || m_channels <= 0) {
                    m_cv.wait_for(lock, std::chrono::milliseconds(20));
                    continue;
                }
                // Snapshot: the shared pointer keeps the buffer alive for the
                // whole iteration even if the owner resets its reference.
                pcm = m_pcm;
                sampleRate = m_sampleRate;
                channels = m_channels;
                total = m_totalFrames;
            }

            const qint64 pos = m_position.load();
            if (pos >= total) {
                m_playing = false;
                emitQueued(owner, [owner]() { emit owner->playbackEnded(); });
                continue;
            }
            const qint64 want = std::max<qint64>(1, sampleRate / 100);  // ~10 ms
            const qint64 frames = std::min(want, total - pos);

            // Peaks reflect the effective output level (the Qt sink applies the
            // volume/mute it was given); the bytes written are NOT rescaled so
            // the device-side gain is applied exactly once.
            const int16_t* src = reinterpret_cast<const int16_t*>(pcm->constData())
                                 + pos * channels;
            const float vol = m_muted.load() ? 0.0f : static_cast<float>(m_volume.load());
            float peakL = 0.0f;
            float peakR = 0.0f;
            for (qint64 i = 0; i < frames; ++i) {
                for (int c = 0; c < channels; ++c) {
                    const float s = static_cast<float>(src[i * channels + c]) * vol;
                    const float mag = std::fabs(s) / 32768.0f;
                    if (c == 0) {
                        peakL = std::max(peakL, mag);
                    }
                    if (c == 1 || channels == 1) {
                        peakR = std::max(peakR, mag);
                    }
                }
            }

            const WriteResult result = writeChunk(src, frames, channels);
            if (result.status == WriteStatus::Interrupted) {
                // Control-path stop()/pause() clear m_playing (prompt return);
                // a spontaneous device stop while still playing is converted to
                // an Error inside writeChunk. Reaching this with m_playing true
                // is therefore transient (state race) — wait bounded, never spin.
                if (m_running.load() && m_playing.load()) {
                    waitWritable();
                } else {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_cv.wait_for(lock, std::chrono::milliseconds(20));
                }
                continue;
            }
            if (result.status == WriteStatus::WriteLater) {
                waitWritable();
                continue;
            }
            if (result.status == WriteStatus::Error) {
                const QString message = QStringLiteral("Qt audio device error: %1")
                                            .arg(result.errorText);
                setError(message);
                m_playing = false;  // never spin; wait on the CV
                emitQueued(owner, [owner, message]() {
                    emit owner->errorOccurred(message);
                });
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait_for(lock, std::chrono::milliseconds(100));
                continue;
            }
            if (result.frames <= 0) {
                continue;
            }
            m_position.fetch_add(result.frames);

            const qint64 nowUs = duration_cast<microseconds>(
                steady_clock::now().time_since_epoch()).count();
            if (nowUs - lastLevelUs >= 50000) {  // ~20 Hz level updates
                lastLevelUs = nowUs;
                emitQueued(owner, [owner, peakL, peakR]() {
                    emit owner->levelsChanged(peakL, peakR);
                });
            }
        }
    }

    enum class WriteStatus { Ok, WriteLater, Interrupted, Error };
    struct WriteResult {
        WriteStatus status = WriteStatus::Ok;
        int frames = 0;
        QString errorText;
    };

    WriteResult writeChunk(const int16_t* data, qint64 frames, int channels)
    {
        WriteResult r;
        if (!m_running.load() || !m_playing.load()) {
            r.status = WriteStatus::Interrupted;
            return r;
        }
        if (g_simulated.load()) {
            // Simulate a device that may block for a long time per write; check
            // the cancellation flag every ~5 ms so shutdown stays bounded.
            const int stallMs = g_simStallMs.load();
            for (int waited = 0; waited < stallMs; waited += 5) {
                if (!m_running.load() || !m_playing.load()) {
                    r.status = WriteStatus::Interrupted;
                    return r;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            r.status = WriteStatus::Ok;
            r.frames = static_cast<int>(frames);
            return r;
        }
#ifdef HAVE_QMULTIMEDIA
        std::lock_guard<std::mutex> lock(m_sinkMutex);
        if (!m_sink || !m_io || !m_running.load() || !m_playing.load()) {
            r.status = WriteStatus::Interrupted;
            return r;
        }
        const qtns::State state = m_sink->state();
        if (state == qtns::State::StoppedState) {
            // Control-path stop()/pause() already cleared m_playing, so the
            // guard above returns first. Reaching this line with m_playing
            // still true means the device stopped on its own (unplug / backend
            // error) — report it as a terminal error; looping Interrupted here
            // would busy-spin with m_playing never cleared.
            if (m_playing.load()) {
                r.status = WriteStatus::Error;
                r.errorText = QStringLiteral("audio device stopped unexpectedly");
                return r;
            }
            r.status = WriteStatus::Interrupted;  // stopped by the control path
            return r;
        }
        if (state == qtns::State::SuspendedState) {
            r.status = WriteStatus::WriteLater;  // paused; wait and retry
            return r;
        }
        // ActiveState or IdleState: bytesFree() is valid. Write at most the
        // free capacity so this call can never block on the device.
        const qint64 bytesNeeded = frames * channels * 2;
        const qint64 freeBytes = static_cast<qint64>(m_sink->bytesFree());
        if (freeBytes <= 0) {
            r.status = WriteStatus::WriteLater;
            return r;
        }
        const qint64 toWrite = std::min(bytesNeeded, freeBytes);
        const qint64 written =
            m_io->write(reinterpret_cast<const char*>(data), toWrite);
        if (written < 0) {
            r.status = WriteStatus::Error;
            r.errorText = QStringLiteral("QIODevice::write failed");
            return r;
        }
        if (written == 0) {
            r.status = WriteStatus::WriteLater;
            return r;
        }
        r.status = WriteStatus::Ok;
        r.frames = static_cast<int>(written / (channels * 2));
        return r;
#else
        Q_UNUSED(data);
        Q_UNUSED(frames);
        Q_UNUSED(channels);
        r.status = WriteStatus::Interrupted;
        return r;
#endif
    }

    void waitWritable()
    {
        if (g_simulated.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return;
        }
        // QAudio has no poll(); the ring buffer frees as the device consumes.
        // Sleep briefly and re-check the cancellation flag — always bounded.
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    template <typename Fn>
    static void emitQueued(QtAudioOutputSink* owner, Fn&& fn)
    {
        QMetaObject::invokeMethod(owner, std::forward<Fn>(fn), Qt::QueuedConnection);
    }

    void setError(const QString& error)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastError = error;
    }

#ifdef HAVE_QMULTIMEDIA
    // Owned sink + the internal push-mode device returned by QAudioSink::start().
    // It becomes invalid after stop()/reset()+start(); both are only mutated
    // under m_sinkMutex, so the writer and the control paths never race it.
    std::unique_ptr<QAudioSink> m_sink;
    QIODevice* m_io = nullptr;
    QAudioDevice m_deviceInfo;
#endif
    QString m_device;
    QString m_lastError;

    std::thread m_thread;
    // m_mutex guards the writer state (running/playing/pcm/params/cv);
    // m_sinkMutex serializes all QAudioSink/the internal device access. Lock
    // order, when both are needed, is m_sinkMutex -> m_mutex (never reverse).
    std::mutex m_mutex;
    std::mutex m_sinkMutex;
    std::condition_variable m_cv;

    std::shared_ptr<const QByteArray> m_pcm;
    int m_sampleRate = 0;
    int m_channels = 0;
    qint64 m_totalFrames = 0;

    std::atomic<bool> m_open{false};
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_playing{false};
    std::atomic<qint64> m_position{0};
    std::atomic<float> m_volume{1.0f};
    std::atomic<bool> m_muted{false};
};

QtAudioOutputSink::QtAudioOutputSink(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
}

QtAudioOutputSink::~QtAudioOutputSink()
{
    close();
}

bool QtAudioOutputSink::backendAvailable()
{
#ifdef HAVE_QMULTIMEDIA
    return true;
#else
    return false;
#endif
}

QString QtAudioOutputSink::defaultDeviceName()
{
#ifdef HAVE_QMULTIMEDIA
    const QAudioDevice dev = QMediaDevices::defaultAudioOutput();
    if (!dev.isNull()) {
        return dev.id();
    }
#endif
    return QStringLiteral("default");
}

void QtAudioOutputSink::setSimulatedBackendForTest(bool enabled, int stallMs)
{
#ifdef HAVE_QMULTIMEDIA
    g_simulated = enabled;
    g_simStallMs = enabled ? std::max(0, stallMs) : 0;
#else
    // No-op stub: without the module there is no simulated sink backend.
    Q_UNUSED(enabled);
    Q_UNUSED(stallMs);
#endif
}

QList<QtAudioOutputSink::DeviceInfo> QtAudioOutputSink::enumerateOutputDevices()
{
    QList<DeviceInfo> devices;
#ifdef HAVE_QMULTIMEDIA
    const QList<QAudioDevice> outputs = QMediaDevices::audioOutputs();
    const QAudioDevice def = QMediaDevices::defaultAudioOutput();
    for (const QAudioDevice& d : outputs) {
        DeviceInfo info;
        info.name = d.id();
        info.description = d.description();
        info.isDefault = !def.isNull() && (d.id() == def.id());
        devices.append(info);
    }
#endif
    if (devices.isEmpty()) {
        DeviceInfo fallback;
        fallback.name = defaultDeviceName();
        fallback.description = QStringLiteral("System default");
        fallback.isDefault = true;
        devices.append(fallback);
    }
    return devices;
}

bool QtAudioOutputSink::open(const QString& device)
{
    const QString wanted = device.isEmpty() ? defaultDeviceName() : device;

    // Stop any playback and release the previous sink before opening a new
    // device. closeSink() is bounded (immediate teardown, no drain) and
    // serialized with the writer.
    {
        std::lock_guard<std::mutex> lock(d->m_mutex);
        d->m_playing = false;
        d->m_pcm.reset();
    }
    d->m_cv.notify_all();
    d->closeSink();

    if (g_simulated.load()) {
        d->m_device = wanted;
        d->m_open = true;
        d->setError(QString());
        return true;
    }

#ifdef HAVE_QMULTIMEDIA
    QAudioDevice resolved;
    if (!resolveDevice(wanted, &resolved)) {
        const QString message =
            QStringLiteral("no Qt audio output device matching '%1'").arg(wanted);
        d->setError(message);
        emit errorOccurred(message);
        return false;
    }
    d->m_device = wanted;
    d->m_deviceInfo = resolved;
    d->m_open = true;
    d->setError(QString());
    return true;
#else
    const QString message =
        QStringLiteral("Qt Multimedia backend not available in this build");
    d->setError(message);
    emit errorOccurred(message);
    return false;
#endif
}

void QtAudioOutputSink::close()
{
    d->shutdown();
}

bool QtAudioOutputSink::isOpen() const
{
    return d->m_open.load();
}

bool QtAudioOutputSink::start(std::shared_ptr<const QByteArray> pcm, int sampleRate,
                              int channels, qint64 startFrame)
{
    if (!pcm || pcm->isEmpty() || sampleRate <= 0 || channels <= 0 || channels > 2) {
        const QString message = QStringLiteral("invalid PCM buffer / format");
        d->setError(message);
        emit errorOccurred(message);
        return false;
    }
    if (!d->m_open.load() && !open(d->m_device.isEmpty() ? defaultDeviceName() : d->m_device)) {
        return false;
    }

    if (g_simulated.load()) {
        // Simulated backend: no real QAudioSink is created; the writer feeds
        // the stall simulation below.
    } else {
#ifdef HAVE_QMULTIMEDIA
        const QAudioFormat format = qtAudioFormatFor(sampleRate, channels);
        std::lock_guard<std::mutex> lock(d->m_sinkMutex);
        if (!d->m_sink) {
            d->m_sink = std::make_unique<QAudioSink>(d->m_deviceInfo, format);
            if (d->m_sink->error() != qtns::Error::NoError) {
                const QString message =
                    QStringLiteral("QAudioSink open failed: %1")
                        .arg(d->m_deviceInfo.description());
                d->setError(message);
                emit errorOccurred(message);
                return false;
            }
            // The audio engine may report state changes from its own thread;
            // the context (this) functor connection queues them to the GUI
            // thread. Report every non-NoError state as an errorOccurred.
            QObject::connect(d->m_sink.get(), &QAudioSink::stateChanged, this,
                             [this](qtns::State state) {
                                 Q_UNUSED(state);
                                 qtns::Error err = qtns::Error::NoError;
                                 {
                                     std::lock_guard<std::mutex> lock(d->m_sinkMutex);
                                     if (!d->m_sink) {
                                         return;
                                     }
                                     err = d->m_sink->error();
                                 }
                                 if (err != qtns::Error::NoError) {
                                     const QString message =
                                         QStringLiteral("Qt audio device error");
                                     d->setError(message);
                                     emit errorOccurred(message);
                                 }
                             });
        }
        d->m_io = d->m_sink->start();  // push mode: write() into the returned device
        if (d->m_sink->error() != qtns::Error::NoError || !d->m_io) {
            const QString message =
                QStringLiteral("QAudioSink start failed: %1")
                    .arg(d->m_deviceInfo.description());
            d->setError(message);
            emit errorOccurred(message);
            return false;
        }
        d->m_sink->setVolume(static_cast<qreal>(d->m_volume.load()));
        d->m_sink->setMuted(d->m_muted.load());
#else
        const QString message =
            QStringLiteral("Qt Multimedia backend not available in this build");
        d->setError(message);
        emit errorOccurred(message);
        return false;
#endif
    }

    const qint64 totalFrames = pcm->size() / (channels * 2);
    {
        std::lock_guard<std::mutex> lock(d->m_mutex);
        d->m_pcm = std::move(pcm);
        d->m_sampleRate = sampleRate;
        d->m_channels = channels;
        d->m_totalFrames = totalFrames;
    }
    d->m_position = std::clamp<qint64>(startFrame, 0, std::max<qint64>(0, totalFrames - 1));
    d->m_playing = true;
    d->ensureThread(this);
    d->m_cv.notify_all();
    return true;
}

void QtAudioOutputSink::stop()
{
    {
        std::lock_guard<std::mutex> lock(d->m_mutex);
        d->m_playing = false;
        d->m_position = 0;
        d->m_pcm.reset();
    }
    d->dropSink();
    d->m_cv.notify_all();
}

void QtAudioOutputSink::pause()
{
    {
        std::lock_guard<std::mutex> lock(d->m_mutex);
        d->m_playing = false;
    }
    d->suspendSink();
    d->m_cv.notify_all();
}

void QtAudioOutputSink::resume()
{
#ifdef HAVE_QMULTIMEDIA
    d->resumeSink();
    {
        std::lock_guard<std::mutex> lock(d->m_sinkMutex);
        // stop()/dropSink() nulled the internal push-mode device; re-arm it so
        // the writer has a device to write into again (mirrors ALSA resume()
        // re-prepping the handle).
        if (d->m_sink && d->m_io == nullptr) {
            d->m_io = d->m_sink->start();
        }
    }
#else
    d->resumeSink();
#endif
    d->m_playing = true;
    d->ensureThread(this);
    d->m_cv.notify_all();
}

void QtAudioOutputSink::seek(qint64 frameIndex)
{
    {
        std::lock_guard<std::mutex> lock(d->m_mutex);
        d->m_position = std::max<qint64>(0, frameIndex);
    }
    // Drop the pre-seek buffered audio so playback continues from the new
    // position instead of the stale tail.
    d->seekSink();
    d->m_cv.notify_all();
}

void QtAudioOutputSink::setVolume(double volume)
{
    d->m_volume = static_cast<float>(std::clamp(volume, 0.0, 1.0));
    d->applyVolumeMute();
}

double QtAudioOutputSink::volume() const
{
    return static_cast<double>(d->m_volume.load());
}

void QtAudioOutputSink::setMuted(bool muted)
{
    d->m_muted = muted;
    d->applyVolumeMute();
}

bool QtAudioOutputSink::isMuted() const
{
    return d->m_muted.load();
}

bool QtAudioOutputSink::isPlaying() const
{
    return d->m_playing.load();
}

qint64 QtAudioOutputSink::positionFrames() const
{
    return d->m_position.load();
}

qint64 QtAudioOutputSink::totalFrames() const
{
    std::lock_guard<std::mutex> lock(d->m_mutex);
    return d->m_totalFrames;
}

QString QtAudioOutputSink::lastError() const
{
    std::lock_guard<std::mutex> lock(d->m_mutex);
    return d->m_lastError;
}

#ifdef HAVE_QMULTIMEDIA
QAudioFormat QtAudioOutputSink::qtAudioFormatFor(int sampleRate, int channels)
{
    QAudioFormat format;
    format.setSampleRate(sampleRate);
    format.setChannelCount(channels);
    format.setSampleFormat(QAudioFormat::Int16);
    format.setByteOrder(QAudioFormat::LittleEndian);
    return format;
}
#endif

} // namespace streamdab::audio