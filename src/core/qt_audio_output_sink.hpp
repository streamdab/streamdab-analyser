/**
 * @file qt_audio_output_sink.hpp
 * @brief T34-ext: Qt Multimedia (QAudioSink) playback sink for Windows/macOS.
 *
 * Windows and macOS have no ALSA. When the Qt Multimedia module is built in
 * (`HAVE_QMULTIMEDIA`, i.e. Qt Multimedia was found at configure time) this
 * sink plays a pre-decoded DAB+ PCM buffer through QAudioSink (WASAPI on
 * Windows, CoreAudio on macOS). When the module is absent the class still
 * compiles and degrades gracefully, exactly like `AudioOutputSink` without
 * ALSA: `backendAvailable()` returns false, `open()`/`start()` fail cleanly
 * with a `lastError()` explaining that the module is not built in, and every
 * other query returns the inactive default.
 *
 * The public API mirrors `AudioOutputSink` 1:1 so the playback controller can
 * treat both backends identically (the preferred sink is selected at build
 * time: ALSA on Linux, Qt Multimedia elsewhere).
 *
 * Lifecycle / threading contract (port of the T34 ALSA contract, F1/F2/F3/F4)
 *  - Exactly one writer thread ever touches the QAudioSink / the internal push
 *    mode QIODevice, and every call is additionally serialized by a single sink
 *    mutex, so the GUI thread's control calls (stop/pause/resume/seek/close)
 *    can never race a concurrent `write()`.
 *  - The writer writes at most `bytesFree()` bytes per call (which never
 *    blocks) and, when the device ring buffer is full, waits with a bounded
 *    sleep while polling the cancellation flag. A stalled/unplugged device
 *    therefore can never pin the writer, and `close()`/the destructor always
 *    return promptly: they reset+destroy the sink (immediate teardown, no
 *    synchronous drain) and only then join.
 *  - The PCM buffer is held as an immutable `shared_ptr<const QByteArray>` for
 *    the whole playback, so the owner may release/replace its reference at any
 *    time without a use-after-free.
 *
 * @author C++ Qt Developer Agent (T34-ext)
 */

#pragma once

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QString>
#include <memory>

#ifdef HAVE_QMULTIMEDIA
class QAudioFormat;
#endif

namespace streamdab::audio {

/**
 * @brief Qt Multimedia (QAudioSink) playback sink for a pre-decoded PCM buffer.
 */
class QtAudioOutputSink : public QObject {
    Q_OBJECT

public:
    /// A selectable output device as reported by the backend.
    struct DeviceInfo {
        QString name;         ///< backend device identifier (e.g. "default")
        QString description;  ///< human-readable description
        bool isDefault = false;
    };

    explicit QtAudioOutputSink(QObject* parent = nullptr);
    ~QtAudioOutputSink() override;

    /// Enumerate playback devices via Qt Multimedia (QMediaDevices).
    static QList<DeviceInfo> enumerateOutputDevices();
    /// True when the Qt Multimedia module was compiled in.
    static bool backendAvailable();
    /// The backend's default device identifier.
    static QString defaultDeviceName();

    /// Open @p device (empty = default). Safe to call repeatedly. The device is
    /// only resolved here; the QAudioSink itself is created lazily in `start()`
    /// (the output format is not known until then), so an idle open never
    /// grabs the hardware.
    bool open(const QString& device = QString());
    /// Stop playback, join the writer and release the sink. Never blocks
    /// indefinitely even if the device has stalled.
    void close();
    bool isOpen() const;

    /**
     * @brief Start playing @p pcm (interleaved S16LE) from @p startFrame.
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
    /// Pause (suspend the device) keeping the position.
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

#ifdef HAVE_QMULTIMEDIA
    /**
     * @brief Build the QAudioFormat (S16LE, @p channels, @p sampleRate) used by
     * the sink. Pure helper — no device required — unit-testable.
     */
    static QAudioFormat qtAudioFormatFor(int sampleRate, int channels);
#endif

    /**
     * @brief TEST-ONLY: replace the Qt audio device with a deterministic
     * simulated backend.
     *
     * Mirrors `AudioOutputSink::setSimulatedBackendForTest` for the same
     * headless stall/close test. When enabled, `open()`/`start()` succeed
     * without hardware and the writer's write step simulates a device that can
     * block for @p stallMs per call (checked against the cancellation flag
     * every ~5 ms). Real runs never enable it. No-op when the module is not
     * built in.
     */
    static void setSimulatedBackendForTest(bool enabled, int stallMs = 0);

signals:
    /// Peak levels of the most recently written chunk (0.0 .. 1.0).
    void levelsChanged(float leftPeak, float rightPeak);
    /// Emitted (queued) when the buffer reaches its end.
    void playbackEnded();
    /// Emitted on a backend error (open / start / unrecoverable write).
    void errorOccurred(const QString& message);

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace streamdab::audio