/**
 * @file audio_output_sink.cpp
 * @brief T34: ALSA playback sink implementation.
 *
 * See the header for the lifecycle/threading contract. The writer thread is the
 * only user of the ALSA handle; every `snd_pcm_*` call is serialized by
 * `m_handleMutex`, the handle is opened non-blocking and all waits are bounded,
 * so `close()` and the destructor can never hang on a stalled device.
 *
 * @author C++ Qt Developer Agent (T34)
 */

#include "audio_output_sink.hpp"

#include <QDebug>
#include <QMetaObject>
#include <QStringList>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#ifdef HAVE_ALSA
#include <alsa/asoundlib.h>
#include <cerrno>
#include <poll.h>
#endif

namespace streamdab::audio {

namespace {
// Test-only simulated backend state (see setSimulatedBackendForTest()).
std::atomic<bool> g_simulated{false};
std::atomic<int> g_simStallMs{0};
} // namespace

class AudioOutputSink::Impl {
public:
    ~Impl() { shutdown(); }

    // ------------------------------------------------------------------
    // Handle ownership. All ALSA handle access happens under m_handleMutex.
    // ------------------------------------------------------------------
    void closeHandle()
    {
        std::lock_guard<std::mutex> lock(m_handleMutex);
#ifdef HAVE_ALSA
        if (m_handle) {
            snd_pcm_drop(m_handle);
            snd_pcm_close(m_handle);
            m_handle = nullptr;
        }
#endif
        m_open = false;
    }

    void dropDevice()
    {
        std::lock_guard<std::mutex> lock(m_handleMutex);
#ifdef HAVE_ALSA
        if (m_handle) {
            snd_pcm_drop(m_handle);
        }
#endif
    }

    void prepareDevice()
    {
        std::lock_guard<std::mutex> lock(m_handleMutex);
#ifdef HAVE_ALSA
        if (m_handle) {
            snd_pcm_prepare(m_handle);
        }
#endif
    }

    /**
     * @brief Stop, close and join. Bounded by construction.
     *
     * Order matters: clear the running flag, then drop+close the handle (this
     * wakes a writer parked in poll() and prevents any further write), THEN
     * join. The writer holds the handle mutex only for the (non-blocking) write,
     * never during a wait, so closeHandle() cannot block on it for long.
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
        closeHandle();
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    void ensureThread(AudioOutputSink* owner)
    {
        if (!m_thread.joinable()) {
            m_running = true;
            m_thread = std::thread(&Impl::run, this, owner);
        }
    }

    // ------------------------------------------------------------------
    // Writer thread
    // ------------------------------------------------------------------
    void run(AudioOutputSink* owner)
    {
        using namespace std::chrono;
        qint64 lastLevelUs = 0;
        std::vector<int16_t> scratch;
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
            scratch.resize(static_cast<size_t>(frames) * static_cast<size_t>(channels));

            const int16_t* src = reinterpret_cast<const int16_t*>(pcm->constData())
                                 + pos * channels;
            const float vol = m_muted.load() ? 0.0f : static_cast<float>(m_volume.load());
            float peakL = 0.0f;
            float peakR = 0.0f;
            for (qint64 i = 0; i < frames; ++i) {
                for (int c = 0; c < channels; ++c) {
                    const int16_t in = src[i * channels + c];
                    float s = static_cast<float>(in) * vol;
                    s = std::clamp(s, -32768.0f, 32767.0f);
                    scratch[static_cast<size_t>(i) * channels + c] =
                        static_cast<int16_t>(std::lround(s));
                    const float mag = std::fabs(s) / 32768.0f;
                    if (c == 0) {
                        peakL = std::max(peakL, mag);
                    }
                    if (c == 1 || channels == 1) {
                        peakR = std::max(peakR, mag);
                    }
                }
            }

            const WriteResult result = writeChunk(scratch.data(), frames);
            if (result.status == WriteStatus::Interrupted) {
                continue;  // shutting down / stopped
            }
            if (result.status == WriteStatus::WouldBlock) {
                waitWritable();
                continue;
            }
            if (result.status == WriteStatus::Error) {
                if (!recover(result.errorCode)) {
                    const QString message = QString("audio device error (recover failed): %1")
                                                .arg(result.errorText);
                    setError(message);
                    m_playing = false;  // never spin; wait on the CV
                    emitQueued(owner, [owner, message]() {
                        emit owner->errorOccurred(message);
                    });
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_cv.wait_for(lock, std::chrono::milliseconds(100));
                }
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

    enum class WriteStatus { Ok, WouldBlock, Interrupted, Error };
    struct WriteResult {
        WriteStatus status = WriteStatus::Ok;
        int frames = 0;
        int errorCode = 0;
        QString errorText;
    };

    WriteResult writeChunk(const int16_t* data, qint64 frames)
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
#ifdef HAVE_ALSA
        std::lock_guard<std::mutex> lock(m_handleMutex);
        if (!m_handle) {
            r.status = WriteStatus::Interrupted;
            return r;
        }
        const snd_pcm_sframes_t w =
            snd_pcm_writei(m_handle, data, static_cast<snd_pcm_uframes_t>(frames));
        if (w == -EAGAIN) {
            r.status = WriteStatus::WouldBlock;
            return r;
        }
        if (w < 0) {
            r.status = WriteStatus::Error;
            r.errorCode = static_cast<int>(w);
            r.errorText = QString::fromLatin1(snd_strerror(static_cast<int>(w)));
            return r;
        }
        r.status = WriteStatus::Ok;
        r.frames = static_cast<int>(w);
        return r;
#else
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
#ifdef HAVE_ALSA
        struct pollfd fds[8];
        int count = 0;
        {
            std::lock_guard<std::mutex> lock(m_handleMutex);
            if (!m_handle) {
                return;
            }
            count = snd_pcm_poll_descriptors(m_handle, fds, 8);
        }
        if (count > 0) {
            ::poll(fds, static_cast<nfds_t>(count), 20);  // bounded wait
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
    }

    bool recover(int errorCode)
    {
        if (g_simulated.load()) {
            return false;
        }
#ifdef HAVE_ALSA
        std::lock_guard<std::mutex> lock(m_handleMutex);
        if (!m_handle) {
            return false;
        }
        return snd_pcm_recover(m_handle, errorCode, 0) >= 0;
#else
        Q_UNUSED(errorCode);
        return false;
#endif
    }

    template <typename Fn>
    static void emitQueued(AudioOutputSink* owner, Fn&& fn)
    {
        QMetaObject::invokeMethod(owner, std::forward<Fn>(fn), Qt::QueuedConnection);
    }

    void setError(const QString& error)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastError = error;
    }

#ifdef HAVE_ALSA
    snd_pcm_t* m_handle = nullptr;
#endif
    QString m_device;
    QString m_lastError;

    std::thread m_thread;
    // m_mutex guards the writer state (running/playing/pcm/params/cv);
    // m_handleMutex serializes all ALSA handle access. Lock order, when both
    // are needed, is m_handleMutex -> m_mutex (never the reverse).
    std::mutex m_mutex;
    std::mutex m_handleMutex;
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

AudioOutputSink::AudioOutputSink(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
}

AudioOutputSink::~AudioOutputSink()
{
    close();
}

bool AudioOutputSink::backendAvailable()
{
#ifdef HAVE_ALSA
    return true;
#else
    return false;
#endif
}

QString AudioOutputSink::defaultDeviceName()
{
    return QStringLiteral("default");
}

void AudioOutputSink::setSimulatedBackendForTest(bool enabled, int stallMs)
{
    g_simulated = enabled;
    g_simStallMs = enabled ? std::max(0, stallMs) : 0;
}

QList<AudioOutputSink::DeviceInfo> AudioOutputSink::enumerateOutputDevices()
{
    QList<DeviceInfo> devices;
#ifdef HAVE_ALSA
    void** hints = nullptr;
    const int err = snd_device_name_hint(-1, "pcm", &hints);
    if (err == 0 && hints) {
        for (void** h = hints; *h != nullptr; ++h) {
            char* name = snd_device_name_get_hint(*h, "NAME");
            char* desc = snd_device_name_get_hint(*h, "DESC");
            char* ioid = snd_device_name_get_hint(*h, "IOID");
            const QString deviceName = name ? QString::fromLatin1(name) : QString();
            const bool isOutput = (ioid == nullptr) || (std::strcmp(ioid, "Output") == 0);
            if (!deviceName.isEmpty() && isOutput
                && !deviceName.contains(QLatin1String("null"))) {
                DeviceInfo info;
                info.name = deviceName;
                info.description = desc ? QString::fromLatin1(desc).section('\n', 0, 0)
                                        : deviceName;
                info.isDefault = (deviceName == QLatin1String("default"));
                devices.append(info);
            }
            std::free(name);
            std::free(desc);
            std::free(ioid);
        }
        snd_device_name_free_hint(hints);
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

bool AudioOutputSink::open(const QString& device)
{
    const QString wanted = device.isEmpty() ? defaultDeviceName() : device;

    // Stop any playback and release the previous handle before opening a new
    // one. closeHandle() is bounded (non-blocking write + bounded poll) and
    // serialized with the writer.
    {
        std::lock_guard<std::mutex> lock(d->m_mutex);
        d->m_playing = false;
        d->m_pcm.reset();
    }
    d->m_cv.notify_all();
    d->closeHandle();

    if (g_simulated.load()) {
        d->m_device = wanted;
        d->m_open = true;
        d->setError(QString());
        return true;
    }

#ifdef HAVE_ALSA
    snd_pcm_t* handle = nullptr;
    const QByteArray dev = wanted.toUtf8();
    const int err = snd_pcm_open(&handle, dev.constData(), SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        const QString message = QString("snd_pcm_open(%1) failed: %2")
                                    .arg(wanted, QString::fromLatin1(snd_strerror(err)));
        d->setError(message);
        emit errorOccurred(message);
        return false;
    }
    // Non-blocking: the writer polls instead of blocking, so a stalled device
    // can never pin the writer thread.
    snd_pcm_nonblock(handle, 1);
    {
        std::lock_guard<std::mutex> lock(d->m_handleMutex);
        d->m_handle = handle;
    }
    d->m_device = wanted;
    d->m_open = true;
    d->setError(QString());
    return true;
#else
    const QString message = QStringLiteral("ALSA backend not available in this build");
    d->setError(message);
    emit errorOccurred(message);
    return false;
#endif
}

void AudioOutputSink::close()
{
    d->shutdown();
}

bool AudioOutputSink::isOpen() const
{
    return d->m_open.load();
}

bool AudioOutputSink::start(std::shared_ptr<const QByteArray> pcm, int sampleRate, int channels,
                            qint64 startFrame)
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

    // Configure the (non-blocking) handle. Serialized with the writer.
    int paramError = 0;
    bool noHandle = false;
    if (!g_simulated.load()) {
#ifdef HAVE_ALSA
        {
            std::lock_guard<std::mutex> lock(d->m_handleMutex);
            if (!d->m_handle) {
                noHandle = true;
            } else {
                paramError = snd_pcm_set_params(d->m_handle, SND_PCM_FORMAT_S16_LE,
                                                SND_PCM_ACCESS_RW_INTERLEAVED,
                                                static_cast<unsigned int>(channels),
                                                static_cast<unsigned int>(sampleRate),
                                                1,        // allow software resampling
                                                100000);  // 100 ms latency
                if (paramError == 0) {
                    snd_pcm_prepare(d->m_handle);
                }
            }
        }
#else
        paramError = -1;
#endif
    }
    if (noHandle || paramError < 0) {
        const QString message = noHandle
            ? QStringLiteral("no ALSA handle")
            : QStringLiteral("snd_pcm_set_params failed");
        d->setError(message);
        emit errorOccurred(message);
        return false;
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

void AudioOutputSink::stop()
{
    {
        std::lock_guard<std::mutex> lock(d->m_mutex);
        d->m_playing = false;
        d->m_position = 0;
        d->m_pcm.reset();
    }
    d->dropDevice();
    d->m_cv.notify_all();
}

void AudioOutputSink::pause()
{
    d->m_playing = false;
    d->dropDevice();
    d->m_cv.notify_all();
}

void AudioOutputSink::resume()
{
    d->prepareDevice();
    d->m_playing = true;
    d->ensureThread(this);
    d->m_cv.notify_all();
}

void AudioOutputSink::seek(qint64 frameIndex)
{
    d->m_position = std::max<qint64>(0, frameIndex);
    // Flush the buffered (stale) audio so playback continues from the new
    // position instead of the pre-seek tail.
    d->dropDevice();
    d->prepareDevice();
    d->m_cv.notify_all();
}

void AudioOutputSink::setVolume(double volume)
{
    d->m_volume = static_cast<float>(std::clamp(volume, 0.0, 1.0));
}

double AudioOutputSink::volume() const
{
    return static_cast<double>(d->m_volume.load());
}

void AudioOutputSink::setMuted(bool muted)
{
    d->m_muted = muted;
}

bool AudioOutputSink::isMuted() const
{
    return d->m_muted.load();
}

bool AudioOutputSink::isPlaying() const
{
    return d->m_playing.load();
}

qint64 AudioOutputSink::positionFrames() const
{
    return d->m_position.load();
}

qint64 AudioOutputSink::totalFrames() const
{
    std::lock_guard<std::mutex> lock(d->m_mutex);
    return d->m_totalFrames;
}

QString AudioOutputSink::lastError() const
{
    std::lock_guard<std::mutex> lock(d->m_mutex);
    return d->m_lastError;
}

} // namespace streamdab::audio
