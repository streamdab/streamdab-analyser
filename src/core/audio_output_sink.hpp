/**
 * @file audio_output_sink.hpp
 * @brief T34: minimal ALSA playback sink for the decoded DAB+ PCM stream.
 *
 * The analyser intentionally does not depend on Qt Multimedia (its dev package
 * is not required by this project); on Linux the sink talks to ALSA directly.
 * When ALSA is unavailable at build time (`HAVE_ALSA` unset) the class still
 * compiles and degrades gracefully: it reports the missing backend via
 * `backendAvailable()` / `lastError()` and never opens a device.
 *
 * Lifecycle / threading contract (T34 review F1/F2/F3/F4)
 *  - Exactly one writer thread ever touches the ALSA handle, and every
 *    `snd_pcm_*` call is additionally serialized by a single handle mutex, so
 *    the GUI thread's control calls (stop/pause/resume/seek/close) can never
 *    race a concurrent `snd_pcm_writei`.
 *  - The handle is opened in **non-blocking** mode; the writer writes and, on
 *    EAGAIN, waits with `poll()` for a bounded timeout (≤20 ms) *without*
 *    holding the handle mutex. A stalled/unplugged device therefore can never
 *    pin the writer, and `close()`/the destructor always returns promptly: it
 *    clears the running flag, drops+closes the handle (which also wakes any
 *    poll) and only then joins.
 *  - The PCM buffer is held as an immutable `shared_ptr<const QByteArray>` for
 *    the whole playback, so the owner may release/replace its reference at any
 *    time without a use-after-free.
 *
 * @author C++ Qt Developer Agent (T34)
 */

#pragma once

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QString>
#include <memory>

namespace streamdab::audio {

/**
 * @brief ALSA playback sink for a pre-decoded PCM buffer.
 */
class AudioOutputSink : public QObject {
    Q_OBJECT

public:
    /// A selectable output device as reported by the backend.
    struct DeviceInfo {
        QString name;         ///< backend device identifier (e.g. "default")
        QString description;  ///< human-readable description
        bool isDefault = false;
    };

    explicit AudioOutputSink(QObject* parent = nullptr);
    ~AudioOutputSink() override;

    /// Enumerate playback devices via the chosen backend (ALSA hints).
    static QList<DeviceInfo> enumerateOutputDevices();
    /// True when an audio backend was compiled in.
    static bool backendAvailable();
    /// The backend's default device identifier.
    static QString defaultDeviceName();

    /// Open @p device (empty = default). Safe to call repeatedly.
    bool open(const QString& device = QString());
    /// Stop playback, join the writer and close the device. Never blocks
    /// indefinitely even if the device has stalled.
    void close();
    bool isOpen() const;

    /**
     * @brief Start playing @p pcm (interleaved S16) from @p startFrame.
     *
     * The shared pointer is retained for the playback duration; the caller may
     * drop its own reference immediately afterwards.
     *
     * @param pcm          Interleaved signed-16 little-endian PCM buffer.
     * @param sampleRate   Output sample rate in Hz.
     * @param channels     1 (mono) or 2 (stereo).
     * @param startFrame   0-based interleaved sample frame to start at.
     * @return true if the device is open and playback started.
     */
    bool start(std::shared_ptr<const QByteArray> pcm, int sampleRate, int channels,
               qint64 startFrame);

    /// Stop and rewind to frame 0 (drops the device buffer -> silence).
    void stop();
    /// Pause (drops the device buffer -> silence) keeping the position.
    void pause();
    /// Resume from the paused position.
    void resume();
    /// Seek the 0-based interleaved sample-frame position.
    void seek(qint64 frameIndex);

    void setVolume(double volume);  ///< 0.0 .. 1.0
    double volume() const;
    void setMuted(bool muted);
    bool isMuted() const;

    bool isPlaying() const;
    qint64 positionFrames() const;
    qint64 totalFrames() const;
    QString lastError() const;

    /**
     * @brief TEST-ONLY: replace the ALSA device with a deterministic simulated
     * backend.
     *
     * When enabled, `open()` succeeds without hardware and the writer's write
     * step simulates a device that can block for @p stallMs per call (checked
     * against the cancellation flag every ~5 ms). This is used by the headless
     * test to prove that `close()`/the destructor cannot hang on a stalled
     * device. Real runs never enable it.
     */
    static void setSimulatedBackendForTest(bool enabled, int stallMs = 0);

signals:
    /// Peak levels of the most recently written chunk (0.0 .. 1.0).
    void levelsChanged(float leftPeak, float rightPeak);
    /// Emitted (queued) when the buffer reaches its end.
    void playbackEnded();
    /// Emitted on a backend error (open / set-params / unrecoverable write).
    void errorOccurred(const QString& message);

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace streamdab::audio
