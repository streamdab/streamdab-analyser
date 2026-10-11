/**
 * @file qt_audio_output_sink.cpp
 * @brief Qt Multimedia (QAudioSink) playback sink — pull-mode implementation.
 *
 * See the header for the threading contract. In short: every public call and
 * every QAudioSink call happens on the thread the sink lives in (the GUI
 * thread). The audio engine PULLS PCM through `PcmPullDevice::readData()`,
 * which only copies bytes out of an immutable buffer and never calls back into
 * Qt Multimedia, so there is no writer thread, no sink mutex and nothing that
 * can block or re-enter.
 *
 * Why not push mode: the previous design had a writer thread feeding the
 * QAudioSink and a mutex held around `QAudioSink::start()` while the
 * `stateChanged` handler took the same mutex. On WASAPI/CoreAudio the state
 * change is emitted synchronously inside `start()`, so pressing Play deadlocked
 * the GUI thread (and Pause/Stop/seek could do the same).
 *
 * When the module is absent (`HAVE_QMULTIMEDIA` unset) every QAudio and QMedia
 * symbol is compiled out and the class provides the same graceful-degradation
 * stubs as `AudioOutputSink` without ALSA.
 */

#include "qt_audio_output_sink.hpp"

#include <QElapsedTimer>
#include <QIODevice>
#include <QTimer>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>

#ifdef HAVE_QMULTIMEDIA
#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
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

// Test-only simulated backend: no QAudioSink, a timer pulls from the device the
// way the audio engine would. `stallMs` is accepted for API compatibility; a
// pull-mode sink has nothing that can block, so it is ignored.
std::atomic<bool> g_simulated{false};

constexpr int kTickMs = 40;          // level updates + end-of-buffer detection (25 Hz)
constexpr int kSimulatedTickMs = 10;  // simulated engine pull cadence

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

/**
 * @brief Read-only sequential device that serves a pre-decoded PCM buffer.
 *
 * `readData()` may be called from the audio engine's own thread (CoreAudio) or
 * from the GUI thread (WASAPI), and may even run synchronously inside
 * `QAudioSink::start()`. It therefore only touches atomics, a short critical
 * section around the buffer pointer, and the destination memory: it never
 * calls into QAudioSink and never blocks.
 */
class PcmPullDevice : public QIODevice {
public:
    explicit PcmPullDevice(QObject* parent = nullptr) : QIODevice(parent) {}

    /// Install a new buffer (GUI thread). @p startByte is where reading starts.
    void setSource(std::shared_ptr<const QByteArray> pcm, int channels, qint64 startByte)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_pcm = std::move(pcm);
        }
        m_frameBytes = std::max(1, channels) * 2;
        m_pos = alignToFrame(startByte);
        m_ended = false;
        m_peakL = 0.0f;
        m_peakR = 0.0f;
    }

    void clearSource()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_pcm.reset();
        }
        m_pos = 0;
        m_ended = false;
        m_peakL = 0.0f;
        m_peakR = 0.0f;
    }

    void setPositionBytes(qint64 byte)
    {
        m_pos = alignToFrame(byte);
        m_ended = false;
    }
    qint64 positionBytes() const { return m_pos.load(); }
    qint64 totalBytes() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pcm ? m_pcm->size() : 0;
    }
    bool reachedEnd() const { return m_ended.load(); }

    /// Effective output gain used only for the level meters.
    void setGain(float gain) { m_gain = gain; }

    /// Largest peaks since the last call (then reset). 0..1.
    void takePeaks(float* left, float* right)
    {
        *left = m_peakL.exchange(0.0f);
        *right = m_peakR.exchange(0.0f);
    }

    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override
    {
        const qint64 total = totalBytes();
        return std::max<qint64>(0, total - m_pos.load()) + QIODevice::bytesAvailable();
    }

protected:
    qint64 readData(char* data, qint64 maxlen) override
    {
        std::shared_ptr<const QByteArray> pcm;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            pcm = m_pcm;
        }
        if (!pcm || maxlen <= 0) {
            return 0;
        }
        const int frameBytes = m_frameBytes.load();
        const qint64 pos = m_pos.load();
        const qint64 remaining = pcm->size() - pos;
        if (remaining < frameBytes) {
            m_ended = true;
            return 0;
        }
        qint64 n = std::min<qint64>(maxlen, remaining);
        n -= n % frameBytes;
        if (n <= 0) {
            return 0;
        }
        std::memcpy(data, pcm->constData() + pos, static_cast<size_t>(n));
        m_pos = pos + n;

        // Level meters (cheap: one pass over the chunk just handed out).
        const int channels = frameBytes / 2;
        const int16_t* s = reinterpret_cast<const int16_t*>(data);
        const qint64 frames = n / frameBytes;
        const float gain = m_gain.load();
        float pl = 0.0f;
        float pr = 0.0f;
        for (qint64 i = 0; i < frames; ++i) {
            const float l = std::fabs(static_cast<float>(s[i * channels])) * gain / 32768.0f;
            const float r = channels > 1
                ? std::fabs(static_cast<float>(s[i * channels + 1])) * gain / 32768.0f
                : l;
            pl = std::max(pl, l);
            pr = std::max(pr, r);
        }
        float cur = m_peakL.load();
        while (pl > cur && !m_peakL.compare_exchange_weak(cur, pl)) {
        }
        cur = m_peakR.load();
        while (pr > cur && !m_peakR.compare_exchange_weak(cur, pr)) {
        }
        return n;
    }

    qint64 writeData(const char*, qint64) override { return -1; }

private:
    qint64 alignToFrame(qint64 byte) const
    {
        const int fb = std::max(1, m_frameBytes.load());
        const qint64 clamped = std::max<qint64>(0, byte);
        return clamped - (clamped % fb);
    }

    mutable std::mutex m_mutex;  // guards only the buffer pointer
    std::shared_ptr<const QByteArray> m_pcm;
    std::atomic<int> m_frameBytes{4};
    std::atomic<qint64> m_pos{0};
    std::atomic<bool> m_ended{false};
    std::atomic<float> m_gain{1.0f};
    std::atomic<float> m_peakL{0.0f};
    std::atomic<float> m_peakR{0.0f};
};

} // namespace

QByteArray QtAudioOutputSink::resampleLinearS16(const QByteArray& src, int channels,
                                                int srcRate, int dstRate)
{
    if (channels <= 0 || srcRate <= 0 || dstRate <= 0 || src.isEmpty()) {
        return QByteArray();
    }
    if (srcRate == dstRate) {
        return src;
    }
    const int16_t* in = reinterpret_cast<const int16_t*>(src.constData());
    const qint64 inFrames = src.size() / (channels * 2);
    if (inFrames <= 0) {
        return QByteArray();
    }
    const qint64 outFrames = std::max<qint64>(1, inFrames * dstRate / srcRate);
    QByteArray out;
    out.resize(static_cast<qsizetype>(outFrames * channels * 2));
    int16_t* o = reinterpret_cast<int16_t*>(out.data());
    const double step = static_cast<double>(srcRate) / static_cast<double>(dstRate);
    for (qint64 i = 0; i < outFrames; ++i) {
        const double p = static_cast<double>(i) * step;
        const qint64 i0 = std::min<qint64>(inFrames - 1, static_cast<qint64>(p));
        const qint64 i1 = std::min<qint64>(inFrames - 1, i0 + 1);
        const double frac = p - static_cast<double>(i0);
        for (int c = 0; c < channels; ++c) {
            const double a = in[i0 * channels + c];
            const double b = in[i1 * channels + c];
            o[i * channels + c] = static_cast<int16_t>(std::lround(a + (b - a) * frac));
        }
    }
    return out;
}

class QtAudioOutputSink::Impl {
public:
    explicit Impl(QtAudioOutputSink* owner) : m_owner(owner)
    {
        m_device_io = new PcmPullDevice(owner);
        m_device_io->open(QIODevice::ReadOnly);
        m_timer = new QTimer(owner);
        m_timer->setInterval(kTickMs);
        QObject::connect(m_timer, &QTimer::timeout, owner, [this]() { onTick(); });
    }

    ~Impl() { teardownSink(); }

    // ------------------------------------------------------------------
    // Sink ownership (GUI thread only)
    // ------------------------------------------------------------------
    void teardownSink()
    {
#ifdef HAVE_QMULTIMEDIA
        if (m_sink) {
            // Immediate teardown (no synchronous drain).
            m_sink->stop();
            m_sink.reset();
        }
#endif
    }

    void setError(const QString& error) { m_lastError = error; }

    void fail(const QString& message)
    {
        setError(message);
        m_playing = false;
        m_timer->stop();
        emit m_owner->errorOccurred(message);
    }

    /// Source frame -> byte offset in the (possibly resampled) device buffer.
    qint64 byteForSourceFrame(qint64 frame) const
    {
        const qint64 clamped = std::clamp<qint64>(frame, 0, std::max<qint64>(0, m_totalFrames));
        return clamped * m_outRate / std::max(1, m_srcRate) * m_channels * 2;
    }

    qint64 sourceFrameForByte(qint64 byte) const
    {
        const qint64 outFrame = byte / std::max(1, m_channels * 2);
        return outFrame * std::max(1, m_srcRate) / std::max(1, m_outRate);
    }

    void applyGain()
    {
        const float gain = m_muted ? 0.0f : m_volume;
        m_device_io->setGain(gain);
#ifdef HAVE_QMULTIMEDIA
        if (m_sink) {
            // QAudioSink has no setMuted() in Qt 6 (that is QAudioOutput);
            // mute by driving the sink volume to zero.
            m_sink->setVolume(static_cast<qreal>(gain));
        }
#endif
    }

    // ------------------------------------------------------------------
    // 25 Hz housekeeping tick (GUI thread): level meters + end of buffer.
    // In simulated mode the tick also plays the part of the audio engine.
    // ------------------------------------------------------------------
    void onTick()
    {
        if (!m_playing) {
            return;
        }
        if (g_simulated.load()) {
            simulatePull();
        }
        float l = 0.0f;
        float r = 0.0f;
        m_device_io->takePeaks(&l, &r);
        emit m_owner->levelsChanged(l, r);

        if (m_device_io->reachedEnd()) {
            bool drained = true;
#ifdef HAVE_QMULTIMEDIA
            if (!g_simulated.load() && m_sink) {
                // Everything handed to the device has been played once the
                // device buffer is entirely free again (or the sink went idle).
                drained = m_sink->bytesFree() >= m_sink->bufferSize()
                          || m_sink->state() != qtns::State::ActiveState;
            }
#endif
            if (drained) {
                m_playing = false;
                m_timer->stop();
                emit m_owner->playbackEnded();
            }
        }
    }

    void simulatePull()
    {
        const qint64 ms = std::max<qint64>(1, m_simClock.restart());
        qint64 bytes = ms * m_outRate / 1000 * m_channels * 2;
        bytes -= bytes % (m_channels * 2);
        if (bytes <= 0) {
            return;
        }
        QByteArray sink;
        sink.resize(static_cast<qsizetype>(bytes));
        m_device_io->read(sink.data(), bytes);
    }

    QtAudioOutputSink* m_owner = nullptr;
    PcmPullDevice* m_device_io = nullptr;
    QTimer* m_timer = nullptr;
    QElapsedTimer m_simClock;

#ifdef HAVE_QMULTIMEDIA
    std::unique_ptr<QAudioSink> m_sink;
    QAudioFormat m_sinkFormat;
    QAudioDevice m_deviceInfo;
#endif
    QString m_device;
    QString m_lastError;

    std::shared_ptr<const QByteArray> m_pcm;  // keeps the caller's buffer alive
    std::shared_ptr<const QByteArray> m_converted;  // resampled copy, if needed
    int m_srcRate = 0;
    int m_outRate = 0;
    int m_channels = 0;
    qint64 m_totalFrames = 0;  // in SOURCE frames

    bool m_open = false;
    std::atomic<bool> m_playing{false};
    float m_volume = 1.0f;
    bool m_muted = false;
};

QtAudioOutputSink::QtAudioOutputSink(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>(this))
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
    Q_UNUSED(stallMs);  // pull mode: nothing can block, so nothing to stall
    g_simulated = enabled;
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
    for (const QAudioDevice& dev : outputs) {
        DeviceInfo info;
        info.name = dev.id();
        info.description = dev.description();
        info.isDefault = !def.isNull() && (dev.id() == def.id());
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
    // device (immediate teardown, no drain).
    d->m_playing = false;
    d->m_timer->stop();
    d->m_device_io->clearSource();
    d->m_pcm.reset();
    d->m_converted.reset();
    d->teardownSink();
    d->m_open = false;

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
    d->m_playing = false;
    d->m_timer->stop();
    d->m_device_io->clearSource();
    d->m_pcm.reset();
    d->m_converted.reset();
    d->teardownSink();
    d->m_open = false;
}

bool QtAudioOutputSink::isOpen() const
{
    return d->m_open;
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
    if (!d->m_open && !open(d->m_device.isEmpty() ? defaultDeviceName() : d->m_device)) {
        return false;
    }

    d->m_playing = false;
    d->m_timer->stop();

    int outRate = sampleRate;
#ifdef HAVE_QMULTIMEDIA
    if (!g_simulated.load()) {
        QAudioFormat format = qtAudioFormatFor(sampleRate, channels);
        if (!d->m_deviceInfo.isFormatSupported(format)) {
            // Ask the device what it wants and resample to that rate. A device
            // that cannot take 16-bit PCM at any rate is reported, not played.
            const int preferred = d->m_deviceInfo.preferredFormat().sampleRate();
            QAudioFormat alt = qtAudioFormatFor(preferred > 0 ? preferred : 48000, channels);
            if (!d->m_deviceInfo.isFormatSupported(alt)) {
                alt = qtAudioFormatFor(48000, channels);
            }
            if (!d->m_deviceInfo.isFormatSupported(alt)) {
                d->fail(QStringLiteral("audio device '%1' does not support 16-bit PCM at %2 Hz")
                            .arg(d->m_deviceInfo.description())
                            .arg(sampleRate));
                return false;
            }
            format = alt;
            outRate = alt.sampleRate();
        }
        // (Re)create the sink when the format changed (e.g. another service with
        // a different sample rate); an unchanged format reuses the sink.
        if (!d->m_sink || d->m_sinkFormat != format) {
            d->teardownSink();
            d->m_sink = std::make_unique<QAudioSink>(d->m_deviceInfo, format);
            d->m_sinkFormat = format;
            // Reported on the GUI thread (direct connection, same thread). The
            // handler takes no lock and never calls back into the sink.
            QObject::connect(d->m_sink.get(), &QAudioSink::stateChanged, this,
                             [this](qtns::State state) {
                                 if (state == qtns::State::StoppedState && d->m_sink
                                     && d->m_sink->error() != qtns::Error::NoError) {
                                     d->fail(QStringLiteral("Qt audio device error (code %1)")
                                                 .arg(static_cast<int>(d->m_sink->error())));
                                 }
                             });
        }
    }
#endif

    std::shared_ptr<const QByteArray> source = pcm;
    if (outRate != sampleRate) {
        d->m_converted = std::make_shared<const QByteArray>(
            resampleLinearS16(*pcm, channels, sampleRate, outRate));
        source = d->m_converted;
    } else {
        d->m_converted.reset();
    }

    d->m_pcm = pcm;
    d->m_srcRate = sampleRate;
    d->m_outRate = outRate;
    d->m_channels = channels;
    d->m_totalFrames = pcm->size() / (channels * 2);
    d->m_device_io->setSource(source, channels,
                              d->byteForSourceFrame(std::clamp<qint64>(
                                  startFrame, 0, std::max<qint64>(0, d->m_totalFrames - 1))));
    d->applyGain();

#ifdef HAVE_QMULTIMEDIA
    if (!g_simulated.load()) {
        // A previous play may have left the sink running or idle: restart it
        // on the freshly positioned device.
        if (d->m_sink->state() != qtns::State::StoppedState) {
            d->m_sink->stop();
        }
        d->m_sink->start(d->m_device_io);  // pull mode: the engine reads from us
        d->applyGain();
        if (d->m_sink->error() != qtns::Error::NoError) {
            d->fail(QStringLiteral("QAudioSink start failed: %1")
                        .arg(d->m_deviceInfo.description()));
            return false;
        }
    } else {
        d->m_simClock.start();
    }
#endif
#ifndef HAVE_QMULTIMEDIA
    d->m_simClock.start();
#endif

    d->m_playing = true;
    d->m_timer->setInterval(g_simulated.load() ? kSimulatedTickMs : kTickMs);
    d->m_timer->start();
    return true;
}

void QtAudioOutputSink::stop()
{
    d->m_playing = false;
    d->m_timer->stop();
#ifdef HAVE_QMULTIMEDIA
    if (d->m_sink) {
        d->m_sink->stop();
    }
#endif
    d->m_device_io->clearSource();
    d->m_pcm.reset();
    d->m_converted.reset();
}

void QtAudioOutputSink::pause()
{
    d->m_playing = false;
    d->m_timer->stop();
#ifdef HAVE_QMULTIMEDIA
    if (d->m_sink) {
        d->m_sink->suspend();
    }
#endif
}

void QtAudioOutputSink::resume()
{
    if (d->m_device_io->totalBytes() <= 0) {
        return;  // nothing loaded (stop() released the buffer): start() again
    }
#ifdef HAVE_QMULTIMEDIA
    if (d->m_sink) {
        if (d->m_sink->state() == qtns::State::SuspendedState) {
            d->m_sink->resume();
        } else if (d->m_sink->state() == qtns::State::StoppedState) {
            d->m_sink->start(d->m_device_io);
        }
    }
#endif
    d->m_simClock.start();
    d->m_playing = true;
    d->m_timer->setInterval(g_simulated.load() ? kSimulatedTickMs : kTickMs);
    d->m_timer->start();
}

void QtAudioOutputSink::seek(qint64 frameIndex)
{
    if (d->m_device_io->totalBytes() <= 0) {
        return;
    }
    d->m_device_io->setPositionBytes(d->byteForSourceFrame(frameIndex));
#ifdef HAVE_QMULTIMEDIA
    if (d->m_sink && d->m_open) {
        // Discard stale pre-seek buffered audio. Some backends fall back to
        // StoppedState after reset(); re-arm pull mode in that case.
        d->m_sink->reset();
        if (d->m_sink->state() == qtns::State::StoppedState && d->m_playing) {
            d->m_sink->start(d->m_device_io);
        }
    }
#endif
}

void QtAudioOutputSink::setVolume(double volume)
{
    d->m_volume = static_cast<float>(std::clamp(volume, 0.0, 1.0));
    d->applyGain();
}

double QtAudioOutputSink::volume() const
{
    return static_cast<double>(d->m_volume);
}

void QtAudioOutputSink::setMuted(bool muted)
{
    d->m_muted = muted;
    d->applyGain();
}

bool QtAudioOutputSink::isMuted() const
{
    return d->m_muted;
}

bool QtAudioOutputSink::isPlaying() const
{
    return d->m_playing.load();
}

qint64 QtAudioOutputSink::positionFrames() const
{
    if (d->m_device_io->totalBytes() <= 0) {
        return 0;
    }
    return std::min(d->sourceFrameForByte(d->m_device_io->positionBytes()),
                    d->m_totalFrames);
}

qint64 QtAudioOutputSink::totalFrames() const
{
    return d->m_pcm ? d->m_totalFrames : 0;
}

QString QtAudioOutputSink::lastError() const
{
    return d->m_lastError;
}

#ifdef HAVE_QMULTIMEDIA
QAudioFormat QtAudioOutputSink::qtAudioFormatFor(int sampleRate, int channels)
{
    QAudioFormat format;
    format.setSampleRate(sampleRate);
    format.setChannelCount(channels);
    format.setSampleFormat(QAudioFormat::Int16);
    return format;
}
#endif

} // namespace streamdab::audio
