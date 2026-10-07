/**
 * @file audio_playback_controller.cpp
 * @brief T34: selected-service DAB+ audio capture + playback controller.
 * @author C++ Qt Developer Agent (T34)
 */

#include "audio_playback_controller.hpp"
#include "audio_output_sink.hpp"
#ifdef HAVE_QMULTIMEDIA
#include "qt_audio_output_sink.hpp"
#endif

#include <QDebug>

#include <algorithm>

namespace streamdab::audio {

namespace {
constexpr int kDabFrameMs = 24;  // DAB Mode I transmission frame (ETSI EN 300 401)

// Build-time backend preference: ALSA is the Linux primary; Qt Multimedia is
// the Windows/macOS backend; "none" only when neither is compiled in. On a
// Linux build that also has Qt Multimedia installed ALSA still wins (kept in
// byte-for-byte parity with the historical behaviour).
constexpr const char* kBackendName =
#if defined(HAVE_ALSA)
    "alsa";
#elif defined(HAVE_QMULTIMEDIA)
    "qt-multimedia";
#else
    "none";
#endif
} // namespace

QString AudioPlaybackController::preferredBackendName()
{
    return QLatin1String(kBackendName);
}

void AudioPlaybackController::onSinkLevels(float left, float right)
{
    m_peakLeft = left;
    m_peakRight = right;
    emit levelsChanged(left, right);
}

void AudioPlaybackController::onSinkPlaybackEnded()
{
    setStatus(QStringLiteral("Stopped"));
    m_peakLeft = 0.0f;
    m_peakRight = 0.0f;
    emit levelsChanged(0.0f, 0.0f);
    emit transportChanged();
}

void AudioPlaybackController::onSinkError(const QString& message)
{
    m_lastError = message;
    setStatus(message);
}

AudioPlaybackController::AudioPlaybackController(QObject* parent)
    : QObject(parent)
{
#if defined(HAVE_ALSA)
    m_sink = new AudioOutputSink(this);
#elif defined(HAVE_QMULTIMEDIA)
    m_sink = new QtAudioOutputSink(this);
#else
    m_sink = new AudioOutputSink(this);  // graceful no-backend stub
#endif
    m_backendName = preferredBackendName();
#if defined(HAVE_ALSA)
    connect(m_sink, &AudioOutputSink::levelsChanged, this,
            &AudioPlaybackController::onSinkLevels);
    connect(m_sink, &AudioOutputSink::playbackEnded, this,
            &AudioPlaybackController::onSinkPlaybackEnded);
    connect(m_sink, &AudioOutputSink::errorOccurred, this,
            &AudioPlaybackController::onSinkError);
#elif defined(HAVE_QMULTIMEDIA)
    connect(m_sink, &QtAudioOutputSink::levelsChanged, this,
            &AudioPlaybackController::onSinkLevels);
    connect(m_sink, &QtAudioOutputSink::playbackEnded, this,
            &AudioPlaybackController::onSinkPlaybackEnded);
    connect(m_sink, &QtAudioOutputSink::errorOccurred, this,
            &AudioPlaybackController::onSinkError);
#else
    connect(m_sink, &AudioOutputSink::levelsChanged, this,
            &AudioPlaybackController::onSinkLevels);
    connect(m_sink, &AudioOutputSink::playbackEnded, this,
            &AudioPlaybackController::onSinkPlaybackEnded);
    connect(m_sink, &AudioOutputSink::errorOccurred, this,
            &AudioPlaybackController::onSinkError);
#endif
}

AudioPlaybackController::~AudioPlaybackController()
{
    if (m_sink) {
        m_sink->stop();
        m_sink->close();
    }
}

void AudioPlaybackController::beginCapture()
{
    if (m_sink) {
        m_sink->stop();
    }
    m_extractors.clear();
    m_streams.clear();
    m_activeService = 0;
    m_activeSubChannel = -1;
    m_activePcm.reset();
    m_pcmSampleRate = 0;
    m_pcmChannels = 0;
    m_pcmFrames = 0;
    m_decodeAttempted = false;
    m_peakLeft = 0.0f;
    m_peakRight = 0.0f;
    m_lastError.clear();
    setStatus(QStringLiteral("Stopped"));
}

bool AudioPlaybackController::feedSubchannelChunk(int subChannelId, const QByteArray& chunk)
{
    auto it = m_extractors.find(subChannelId);
    if (it == m_extractors.end()) {
        it = m_extractors.emplace(subChannelId,
                                  std::make_unique<DabPlusAudioDecoder>()).first;
    }
    Stream& stream = m_streams[subChannelId];
    DabPlusAudioDecoder* decoder = it->second.get();
    const bool produced = decoder->feedChunk(
        chunk, [&stream](const QByteArray& au) { stream.accessUnits.push_back(au); });
    stream.format = decoder->format();
    stream.format.subChannelId = subChannelId;
    return produced;
}

void AudioPlaybackController::setActiveService(quint32 serviceId, int subChannelId)
{
    if (serviceId == m_activeService && subChannelId == m_activeSubChannel) {
        return;
    }
    if (m_sink) {
        m_sink->stop();
    }
    m_activeService = serviceId;
    m_activeSubChannel = subChannelId;
    m_activePcm.reset();
    m_pcmSampleRate = 0;
    m_pcmChannels = 0;
    m_pcmFrames = 0;
    m_decodeAttempted = false;
    m_peakLeft = 0.0f;
    m_peakRight = 0.0f;
    m_lastError.clear();
    setStatus(hasActiveAudio() ? QStringLiteral("Ready") : QStringLiteral("No audio"));
    emit levelsChanged(0.0f, 0.0f);
    emit activeServiceChanged();
}

bool AudioPlaybackController::hasActiveAudio() const
{
    if (m_activeSubChannel < 0) {
        return false;
    }
    const auto it = m_streams.find(m_activeSubChannel);
    return it != m_streams.end() && !it->second.accessUnits.empty();
}

DabPlusFormat AudioPlaybackController::activeFormat() const
{
    if (m_activeSubChannel >= 0) {
        const auto it = m_streams.find(m_activeSubChannel);
        if (it != m_streams.end()) {
            return it->second.format;
        }
    }
    return DabPlusFormat{};
}

int AudioPlaybackController::capturedAccessUnitCount(int subChannelId) const
{
    const auto it = m_streams.find(subChannelId);
    return it == m_streams.end() ? 0 : static_cast<int>(it->second.accessUnits.size());
}

bool AudioPlaybackController::ensureDecoded()
{
    if (m_activePcm && !m_activePcm->isEmpty()) {
        return true;
    }
    if (m_decodeAttempted) {
        return false;  // already failed; avoid repeated decode attempts
    }
    m_decodeAttempted = true;
    m_lastError.clear();

    const auto it = m_streams.find(m_activeSubChannel);
    if (it == m_streams.end() || it->second.accessUnits.empty()) {
        m_lastError = QStringLiteral("no captured DAB+ audio for this service");
        return false;
    }
    if (!DabPlusAudioDecoder::faadAvailable()) {
        m_lastError = QStringLiteral("libfaad2 not available in this build");
        return false;
    }

    DabPlusAudioDecoder decoder;
    if (!decoder.beginDecode(it->second.format)) {
        m_lastError = decoder.lastError();
        return false;
    }
    QByteArray pcm;
    for (const QByteArray& au : it->second.accessUnits) {
        decoder.decodeAu(au, pcm);
    }
    decoder.endDecode();
    if (pcm.isEmpty()) {
        m_lastError = QStringLiteral("decoder produced no PCM");
        return false;
    }

    m_activePcm = std::make_shared<QByteArray>(std::move(pcm));
    m_pcmSampleRate = decoder.outputSampleRate() > 0
                          ? decoder.outputSampleRate()
                          : it->second.format.coreSampleRateKHz() * 1000;
    m_pcmChannels = decoder.outputChannels() > 0
                        ? decoder.outputChannels()
                        : it->second.format.coreChConfig();
    if (m_pcmChannels <= 0) {
        m_pcmChannels = 2;
    }
    m_pcmFrames = m_activePcm->size() / (m_pcmChannels * 2);
    m_lastError.clear();
    return true;
}

qint64 AudioPlaybackController::sampleForFrame(int frameIndex) const
{
    if (m_pcmSampleRate <= 0 || frameIndex <= 0) {
        return 0;
    }
    return static_cast<qint64>(frameIndex) * m_pcmSampleRate * kDabFrameMs / 1000;
}

void AudioPlaybackController::setStatus(const QString& status)
{
    if (m_status == status) {
        return;
    }
    m_status = status;
    emit statusChanged();
}

bool AudioPlaybackController::playFromFrame(int frameIndex)
{
    if (!hasActiveAudio()) {
        setStatus(QStringLiteral("No DAB+ audio service selected"));
        return false;
    }
    if (!ensureDecoded()) {
        setStatus(m_lastError.isEmpty() ? QStringLiteral("Decode failed") : m_lastError);
        return false;
    }
    if (m_pcmFrames <= 0) {
        setStatus(QStringLiteral("No audio samples"));
        return false;
    }
    if (!m_sink->isOpen()) {
        if (!m_sink->open(m_device)) {
            setStatus(m_sink->lastError());
            return false;
        }
    }
    qint64 frame = sampleForFrame(frameIndex);
    frame = std::clamp<qint64>(frame, 0, std::max<qint64>(0, m_pcmFrames - 1));
    if (!m_sink->start(m_activePcm, m_pcmSampleRate, m_pcmChannels, frame)) {
        setStatus(m_sink->lastError());
        return false;
    }
    setStatus(QStringLiteral("Playing"));
    emit transportChanged();
    return true;
}

void AudioPlaybackController::pause()
{
    if (m_sink) {
        m_sink->pause();
    }
    setStatus(QStringLiteral("Paused"));
    emit transportChanged();
}

void AudioPlaybackController::stopAudio()
{
    if (m_sink) {
        m_sink->stop();
    }
    m_peakLeft = 0.0f;
    m_peakRight = 0.0f;
    emit levelsChanged(0.0f, 0.0f);
    setStatus(QStringLiteral("Stopped"));
    emit transportChanged();
}

void AudioPlaybackController::seekToFrame(int frameIndex)
{
    if (m_sink && m_sink->isPlaying()) {
        m_sink->seek(sampleForFrame(frameIndex));
    }
}

void AudioPlaybackController::setVolume(double volume)
{
    if (m_sink) {
        m_sink->setVolume(volume);
    }
}

void AudioPlaybackController::setMuted(bool muted)
{
    if (m_sink) {
        m_sink->setMuted(muted);
    }
}

void AudioPlaybackController::setOutputDevice(const QString& device)
{
    m_device = device;
    if (!m_sink) {
        return;
    }
    // Open lazily: selecting a device at UI-build time must not grab the
    // hardware. If a device is already open, switch it now (restarting
    // playback from the current position when it was playing).
    if (!m_sink->isOpen()) {
        return;
    }
    const bool wasPlaying = m_sink->isPlaying();
    m_sink->stop();
    if (!m_sink->open(device)) {
        m_lastError = m_sink->lastError();
    }
    if (wasPlaying && hasActiveAudio()) {
        m_sink->start(m_activePcm, m_pcmSampleRate, m_pcmChannels,
                      m_sink->positionFrames());
    }
}

QString AudioPlaybackController::outputDevice() const
{
    return m_device;
}

QStringList AudioPlaybackController::availableOutputDevices()
{
    QStringList names;
#if defined(HAVE_ALSA)
    const QList<AudioOutputSink::DeviceInfo> devices =
        AudioOutputSink::enumerateOutputDevices();
    for (const AudioOutputSink::DeviceInfo& info : devices) {
        names.append(info.name);
    }
#elif defined(HAVE_QMULTIMEDIA)
    const QList<QtAudioOutputSink::DeviceInfo> devices =
        QtAudioOutputSink::enumerateOutputDevices();
    for (const QtAudioOutputSink::DeviceInfo& info : devices) {
        names.append(info.name);
    }
#else
    const QList<AudioOutputSink::DeviceInfo> devices =
        AudioOutputSink::enumerateOutputDevices();
    for (const AudioOutputSink::DeviceInfo& info : devices) {
        names.append(info.name);
    }
#endif
    return names;
}

QList<AudioPlaybackController::OutputDeviceInfo>
AudioPlaybackController::enumerateOutputDevices()
{
    QList<OutputDeviceInfo> out;
#if defined(HAVE_ALSA)
    const QList<AudioOutputSink::DeviceInfo> devices =
        AudioOutputSink::enumerateOutputDevices();
    for (const AudioOutputSink::DeviceInfo& info : devices) {
        out.append(OutputDeviceInfo{info.name, info.description, info.isDefault});
    }
#elif defined(HAVE_QMULTIMEDIA)
    const QList<QtAudioOutputSink::DeviceInfo> devices =
        QtAudioOutputSink::enumerateOutputDevices();
    for (const QtAudioOutputSink::DeviceInfo& info : devices) {
        out.append(OutputDeviceInfo{info.name, info.description, info.isDefault});
    }
#else
    const QList<AudioOutputSink::DeviceInfo> devices =
        AudioOutputSink::enumerateOutputDevices();  // degraded stub
    for (const AudioOutputSink::DeviceInfo& info : devices) {
        out.append(OutputDeviceInfo{info.name, info.description, info.isDefault});
    }
#endif
    return out;
}

QString AudioPlaybackController::defaultOutputDevice()
{
#if defined(HAVE_ALSA)
    return AudioOutputSink::defaultDeviceName();
#elif defined(HAVE_QMULTIMEDIA)
    return QtAudioOutputSink::defaultDeviceName();
#else
    return AudioOutputSink::defaultDeviceName();  // degraded stub
#endif
}

bool AudioPlaybackController::audioBackendAvailable()
{
#if defined(HAVE_ALSA)
    return AudioOutputSink::backendAvailable();
#elif defined(HAVE_QMULTIMEDIA)
    return QtAudioOutputSink::backendAvailable();
#else
    return false;
#endif
}

bool AudioPlaybackController::isPlaying() const
{
    return m_sink && m_sink->isPlaying();
}

QString AudioPlaybackController::lastError() const
{
    return m_lastError;
}

} // namespace streamdab::audio
