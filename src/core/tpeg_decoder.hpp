/**
 * @file tpeg_decoder.hpp
 * @brief TPEG / TEPG Decoder (base/partial coverage)
 *
 * Implements a base+partial TPEG/TEPG decoder for DAB data services such as
 * the "TEPG" subchannel observed in the Bangkok capture
 * (eti/bkk_20062022_141637.eti, SCId 22, 8 kb/s).
 *
 * Coverage (per the v1.4 user decision — base/partial only):
 *   TPEG: base — TS 102 894-1-style transport framing identification +
 *         the actual application observed in the capture (a TPEG2-style
 *         carousel carrying TEC/TFP/PKI text messages) decoded at the
 *         text-inventory level.
 *   Deferred (NOT covered in this wave):
 *     - full TS 102 894-2 TTI binary message-component decoding;
 *     - TS 102 894-1 transport-frame sync (0x0A 0x46) de-framing — critically,
 *       the Bangkok capture does NOT contain the 0x0A 0x46 sync pair anywhere
 *       in the TEPG subchannel (verified empirically over all captured bytes);
 *     - real-time navigation / multi-frame TMID reassembly semantics.
 *
 * Empirical framing observed in the Bangkok TEPG capture:
 *   - The subchannel is a plain stream (no DAB packet-mode headers) of
 *     24-byte capacity-unit frames (3 CUs x 8 bytes).
 *   - A 96-byte / 4-frame rotation repeats with marker prefixes
 *     `10 01 13`, `20 01 13`, `30 01 13`, `00 01 13` (phase = high nibble of
 *     the first byte; the sequence runs 1 -> 2 -> 3 -> 0).
 *   - The stream carries a small set of human-visible TPEG2-style text
 *     messages that repeat on the carousel, e.g. the application banner
 *     "TPEG TEC+TFP+PKI+E", the version marker "R-3.1-wip|…" and rolling
 *     parking / traffic texts ("BTS Mo Chit Parking", "PTT Pracha Uthit Rd,
 *     Khwaeng Samsri … Bangkok 10320, Thailand").
 *
 * This decoder therefore:
 *   1. counts 24-byte stream frames and the observed marker phases;
 *   2. reports whether the TS 102 894-1 sync pair 0x0A 0x46 is present;
 *   3. extracts the repeated printable message inventory (deduplicated);
 *   4. classifies each message into the observed application families
 *      (TEC / TFP / PKI / EMI / banner / version) when identifiable.
 *
 * The remaining binary TPEG2 message component layers (TS 102 894-2) are
 * explicitly out of scope for this wave; see
 * docs/EPG_JOURNALINE_TPEG_VALIDATION.md.
 *
 * @see ETSI TS 102 894-1 - TPEG transport and framing
 * @see ETSI TS 102 894-2 - TPEG2-TTI application
 * @author StreamDAB Development Team
 * @date October 2026
 */

#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QVector>
#include <QMap>
#include <QMutex>
#include <cstdint>

namespace eti::tpeg {

/**
 * @brief A single observable TPEG message (text-inventory level).
 *
 * Represents one human-visible text message extracted from the TEPG carousel.
 * Because this wave only implements base/partial coverage, `text` is the
 * raw displayed string; full TS 102 894-2 binary decoding is deferred.
 */
struct TpegMessage {
    uint32_t index = 0;         ///< First-seen order in the stream
    QString text;               ///< Human-visible text (UTF-8 / ASCII)
    QString category;           ///< "TEC" | "TFP" | "PKI" | "EMI" | "banner" | "version" | "unknown"
    uint32_t repeat_count = 1;  ///< How many times the same text was seen
    quint64 first_frame = 0;    ///< 24-byte frame index where the text first appeared
    quint64 last_frame = 0;     ///< last 24-byte frame index carrying the text
};

/**
 * @brief TPEG/TEPG base/partial decoder.
 *
 * Consumes raw DAB data-subchannel bytes (stream mode; the 24-byte frame
 * cadence observed in the Bangkok TEPG capture) and reports framing plus the
 * repeated message inventory. Thread-safe via QMutex (same pattern as the
 * EPG/Journaline decoders).
 */
class TpegDecoder : public QObject {
    Q_OBJECT
    Q_PROPERTY(quint64 frameCount READ frameCount NOTIFY frameCountChanged)
    Q_PROPERTY(int messageCount READ messageCount NOTIFY messageCountChanged)

public:
    explicit TpegDecoder(QObject* parent = nullptr);
    ~TpegDecoder() override;

    // ========================================================================
    // Stream Processing
    // ========================================================================

    /**
     * @brief Append raw stream bytes (concatenated subchannel bytes).
     * @param chunk Raw data-subchannel bytes (typically one 24-byte CIF chunk
     *              per DAB frame, but whole-stream concatenations are fine).
     *
     * Detection and text-inventory extraction run incrementally; call
     * finish() after the last chunk for a complete inventory (the inventory
     * is materialised by finish(), NOT by every chunk — call
     * refreshInventory() for an explicit mid-stream view without closing the
     * decoder; see docs/EPG_JOURNALINE_TPEG_VALIDATION.md).
     *
     * Bounds (documented contract, never unbounded on a live stream):
     *  - the raw buffer is capped at kBufferCap bytes (the tail beyond the cap
     *    is dropped and a ONE-SHOT qWarning is emitted; frame/marker
     *    accounting stays exact because nothing is re-based);
     *  - the unique-text inventory is capped at kMaxUniqueMessages entries
     *    (first-seen order); further texts are counted per occurrence in
     *    Statistics::dropped_messages instead of being stored — a rolling
     *    counter such as `R-3.1-wip|000078113` therefore cannot grow the
     *    inventory without bound;
     *  - calling processStreamData() AFTER finish() is ignored (one-shot
     *    qWarning): the stream is finalised, late bytes are dropped rather
     *    than silently appended without a rescan. clearAll() re-opens it.
     */
    void processStreamData(const QByteArray& chunk);

    /**
     * @brief Flush / finalise the inventory for the current stream.
     *
     * finish() CLOSES the decoder: processStreamData() called afterwards is
     * ignored (one-shot qWarning). Call it exactly once, after the last
     * subchannel/chunk has been fed — never per subchannel (see
     * MainWindow::feedDataServiceSubchannels()).
     */
    void finish();

    /**
     * @brief Rebuild the text inventory from the bytes fed so far, WITHOUT
     *        closing the decoder (finish() is still required at end of
     *        stream).
     *
     * The inventory is otherwise materialised only by finish(); a caller that
     * feeds several streams into one shared decoder (e.g. one per TPEG-routed
     * subchannel) needs a consistent mid-stream view of the inventory to
     * attribute messages to the subchannel currently being fed (delta =
     * countAfter - countBefore). Reading messageCount()/messages() without
     * this call would return the previous finish()'s (or empty) inventory.
     *
     * Idempotent and cheap enough for per-subchannel use: it rescans the
     * capped buffer (kBufferCap) and produces exactly what a following
     * finish() would produce for the same bytes.
     *
     * @return the number of unique messages now in the inventory.
     */
    int refreshInventory();

    // ========================================================================
    // Observations
    // ========================================================================

    /**
     * @brief Number of 24-byte stream frames consumed.
     */
    quint64 frameCount() const;

    /**
     * @brief True when the TS 102 894-1 sync byte pair (0x0A 0x46) occurs.
     */
    bool sawSyncBytes() const;

    /**
     * @brief Distribution of the first byte of every 24-byte stream frame.
     *
     * The Bangkok TEPG capture shows the phase bytes 0x10, 0x20, 0x30 and 0x00
     * (plus 0x14/0x24/0x34/0x04-style variants) as the dominant markers.
     */
    QMap<quint8, quint32> markerDistribution() const;

    /**
     * @brief First-seen message inventory (deduplicated), by VALUE.
     *
     * Returned by value (not const&) so the caller can never observe the
     * container after the mutex was released — the decoder may be fed from a
     * worker thread while the GUI reads (A-M4c).
     */
    QVector<TpegMessage> messages() const;

    /**
     * @brief Number of unique messages in the inventory.
     */
    int messageCount() const;

    /**
     * @brief Human-readable framing description for the observed stream.
     */
    QString framingDescription() const;

    /**
     * @brief Statistics snapshot.
     */
    struct Statistics {
        quint64 frames = 0;          ///< consumed 24-byte frames
        quint64 bytes_appended = 0;  ///< bytes fed into the decoder
        int unique_messages = 0;     ///< deduplicated text inventory size
        int repeat_count = 0;        ///< total repeated occurrences
        bool sync_0a46 = false;      ///< TS 102 894-1 sync seen
        quint64 dropped_messages = 0;///< texts beyond kMaxUniqueMessages
                                     ///< (counted per occurrence so the
                                     ///< counter itself stays bounded)
        quint64 dropped_bytes = 0;   ///< bytes beyond kBufferCap (capped feed)
    };
    Statistics getStatistics() const;

    // ========================================================================
    // Bounds (documented contract — see processStreamData())
    // ========================================================================

    /// Raw stream buffer cap (drop-oldest NOT used: the tail is refused so
    /// frame accounting stays exact; one-shot qWarning on first refusal).
    static constexpr qsizetype kBufferCap = 4 * 1024 * 1024;
    /// Unique-text inventory cap (first-seen order; rest → dropped_messages).
    static constexpr int kMaxUniqueMessages = 512;
    /// Frames required before the framing decision is committed when finish()
    /// is never called (A-M4d: the first 96 bytes alone were not enough).
    static constexpr quint64 kFramingDecideFrames = 40;

    /**
     * @brief Reset all state.
     */
    void clearAll();

    // ========================================================================
    // Framing helper (self-contained, PUBLIC-testable)
    // ========================================================================

    /**
     * @brief Detect the observed marker-phase rotation in a byte stream.
     *
     * Looks for at least two of the canonical phase markers (`10 01 13`,
     * `20 01 13`, `30 01 13`, `00 01 13`) at 24-byte offsets. Self-contained
     * so it can be unit-tested without any capture.
     *
     * Assumptions (documented, A-LOW): the scan starts at offset 0, i.e. the
     * caller must hand over bytes that begin on a 24-byte FRAME BOUNDARY —
     * there is no distinct-phase search, so a stream whose first byte is not
     * a frame start can be missed. At least TWO hits are required; the hits
     * need not be distinct phases (the same phase hit twice at 24-byte
     * spacing also satisfies the test, which is what a partial/rotated
     * carousel produces).
     * @param stream Candidate bytes.
     * @return true when the observed carousel framing is detected.
     */
    static bool detectsCarouselFraming(const QByteArray& stream);

    /**
     * @brief True when the stream contains the TS 102 894-1 sync pair.
     */
    static bool containsSyncPair(const QByteArray& stream);

signals:
    /**
     * @brief Emitted when a new unique message text is decoded.
     */
    void tpegMessageDecoded(const QString& text);

    /**
     * @brief Emitted once usable framing has been identified.
     */
    void tpegFramingDetected(const QString& description);

    /**
     * @brief Emitted on decoding trouble (e.g. malformed marker data).
     */
    void tpegDecodingError(const QString& error);

    /**
     * @brief Emitted when the frame count changes.
     */
    void frameCountChanged();

    /**
     * @brief Emitted when the message inventory changes.
     */
    void messageCountChanged();

private:
    void rescanInventoryLocked();
    /// Commit the framing verdict (m_detectedFraming/m_pendingFraming).
    /// Caller must hold m_mutex. Runs at kFramingDecideFrames or on finish().
    void decideFramingLocked();
    static QString classifyText(const QString& text);

    mutable QMutex m_mutex;
    QByteArray m_buffer;
    quint64 m_appended = 0;
    quint64 m_frames = 0;
    bool m_finished = false;
    bool m_framingReported = false;
    bool m_bufferCapped = false;      ///< one-shot buffer-cap warning emitted
    bool m_postFinishWarned = false;  ///< one-shot post-finish warning emitted
    uint32_t m_repeatTotal = 0;
    quint64 m_droppedMessages = 0;   ///< texts beyond kMaxUniqueMessages
    quint64 m_droppedBytes = 0;      ///< bytes refused beyond kBufferCap
    QString m_detectedFraming;
    QString m_pendingFraming;
    QMap<quint8, quint32> m_markers;
    QVector<TpegMessage> m_messages;
};

} // namespace eti::tpeg

Q_DECLARE_METATYPE(eti::tpeg::TpegMessage)