/**
 * @file audio_playback_controller.hpp
 * @brief T34: orchestrates DAB+ audio capture, on-demand decode and playback.
 *
 * The controller is fed the same per-sub-channel MSC slices the GUI already
 * extracts for the DLS+/MOT feed (one call per ETI frame). During a capture it
 * retains the (small) encoded Access-Unit stream per DAB+ sub-channel; when a
 * service is selected it decodes that service's stream once (libfaad2 is far
 * faster than real time) and hands the finished PCM buffer to the ALSA sink.
 *
 * Transport is aligned to the existing 24 ms/frame playback playhead: the frame
 * index is converted to an interleaved sample-frame offset. Play/pause/stop map
 * directly to the sink; scrubbing re-seeks the sink, so a jump is exact to the
 * 24 ms frame boundary (plus the sink's output latency — documented limitation).
 *
 * The controller lives on the GUI thread. Decoding is synchronous on service
 * selection / first play (≈<1 s for the real fixture's ≈120 s service) and the
 * audio sink runs its own writer thread.
 *
 * @author C++ Qt Developer Agent (T34)
 */

#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QStringList>

#include <map>
#include <memory>
#include <vector>

#include "dabplus_audio_decoder.hpp"

namespace streamdab::audio {

class AudioOutputSink;

/**
 * @brief Selected-service DAB+ audio capture + decode + playback.
 */
class AudioPlaybackController : public QObject {
    Q_OBJECT

public:
    explicit AudioPlaybackController(QObject* parent = nullptr);
    ~AudioPlaybackController() override;

    /// Reset all captured streams and stop playback (start of a new load).
    void beginCapture();
    /// Feed one ETI frame's CIF chunk for @p subChannelId (load-time capture).
    bool feedSubchannelChunk(int subChannelId, const QByteArray& chunk);

    /// Switch the active audio source. Stops playback; decoding is deferred.
    void setActiveService(quint32 serviceId, int subChannelId);
    quint32 activeServiceId() const { return m_activeService; }
    int activeSubChannelId() const { return m_activeSubChannel; }
    /// True when the active service has a captured DAB+ AU stream.
    bool hasActiveAudio() const;
    /// True when the active service's PCM is decoded and ready to play.
    bool isDecoded() const { return m_activePcm && !m_activePcm->isEmpty(); }
    /// Format of the active stream (valid() false when none).
    DabPlusFormat activeFormat() const;
    /// True after a decode was attempted for the active service (success or not).
    bool decodeAttempted() const { return m_decodeAttempted; }

    // --- Transport (24 ms/frame playhead alignment) ---
    bool playFromFrame(int frameIndex);
    void pause();
    void stopAudio();
    void seekToFrame(int frameIndex);

    void setVolume(double volume);   ///< 0.0 .. 1.0
    void setMuted(bool muted);
    void setOutputDevice(const QString& device);
    QString outputDevice() const;
    static QStringList availableOutputDevices();
    static bool audioBackendAvailable();

    float peakLeft() const { return m_peakLeft; }
    float peakRight() const { return m_peakRight; }
    qint64 decodedTotalFrames() const { return m_pcmFrames; }
    int decodedSampleRate() const { return m_pcmSampleRate; }
    int decodedChannels() const { return m_pcmChannels; }
    bool isPlaying() const;
    QString statusText() const { return m_status; }
    QString lastError() const;

    /// Frames captured (AUs) for a sub-channel (0 when unknown).
    int capturedAccessUnitCount(int subChannelId) const;
    /// Number of sub-channels with a captured stream.
    int capturedSubChannelCount() const { return static_cast<int>(m_streams.size()); }

signals:
    void levelsChanged(float leftPeak, float rightPeak);
    void transportChanged();
    void activeServiceChanged();
    void statusChanged();

private:
    struct Stream {
        DabPlusFormat format;
        std::vector<QByteArray> accessUnits;
    };

    bool ensureDecoded();
    qint64 sampleForFrame(int frameIndex) const;
    void setStatus(const QString& status);

    std::map<int, std::unique_ptr<DabPlusAudioDecoder>> m_extractors;
    std::map<int, Stream> m_streams;

    quint32 m_activeService = 0;
    int m_activeSubChannel = -1;

    // Immutable, ref-counted PCM buffer for the active service. The sink
    // retains a reference for the playback duration, so the controller may
    // reset/replace this pointer at any time without a use-after-free.
    std::shared_ptr<const QByteArray> m_activePcm;
    int m_pcmSampleRate = 0;
    int m_pcmChannels = 0;
    qint64 m_pcmFrames = 0;
    bool m_decodeAttempted = false;
    AudioOutputSink* m_sink = nullptr;
    QString m_device;
    QString m_lastError;
    float m_peakLeft = 0.0f;
    float m_peakRight = 0.0f;
    QString m_status;
};

} // namespace streamdab::audio
