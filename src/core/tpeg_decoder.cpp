/**
 * @file tpeg_decoder.cpp
 * @brief TPEG / TEPG base/partial decoder implementation.
 *
 * See tpeg_decoder.hpp for the coverage statement: this module implements
 * transport-framing identification plus text-inventory extraction for the
 * TPEG2-style carousel observed in the Bangkok TEPG capture. Full TS 102 894-2
 * binary TTI decoding is deferred to a later wave.
 *
 * @author StreamDAB Development Team
 * @date October 2026
 */

#include "tpeg_decoder.hpp"
#include <QDebug>

namespace eti::tpeg {

// Observed 24-byte-frame phase markers for the Bangkok TEPG carousel.
// The phase byte can carry a service-group sub-field (0x1x, 0x2x, 0x3x, 0x0x
// with the low nibble varying), so detection matches the high-nibble group.

bool TpegDecoder::containsSyncPair(const QByteArray& stream)
{
    return stream.indexOf(QByteArray::fromHex("0a46")) >= 0;
}

bool TpegDecoder::detectsCarouselFraming(const QByteArray& stream)
{
    // At least two 24-byte-offset markers from the phase set must occur.
    const int n = stream.size();
    if (n < 48) {
        return false;
    }
    int phaseHits = 0;
    for (int off = 0; off + 3 <= n; off += 24) {
        const QByteArray b = stream.mid(off, 3);
        const quint8 first = static_cast<quint8>(b.at(0));
        // phase-byte groups: 0x10/0x14/.. 0x1f, 0x20.., 0x30.., 0x00..
        const quint8 group = static_cast<quint8>(first & 0xF0);
        if ((group == 0x10 || group == 0x20 || group == 0x30 || group == 0x00) &&
            b.mid(1, 2) == QByteArray::fromHex("0113")) {
            ++phaseHits;
        }
    }
    return phaseHits >= 2;
}

TpegDecoder::TpegDecoder(QObject* parent)
    : QObject(parent)
{
    qDebug() << "[TPEGDecoder] Initialized - base/partial TPEG coverage (TS 102 894 text-inventory level)";
}

TpegDecoder::~TpegDecoder()
{
    qDebug() << "[TPEGDecoder] Shutdown - frames:" << m_frames
             << "unique messages:" << m_messages.size();
}

void TpegDecoder::processStreamData(const QByteArray& chunk)
{
    if (chunk.isEmpty()) {
        return;
    }
    QString framingToEmit;
    {
        QMutexLocker locker(&m_mutex);

        // A-M4e: bytes arriving after finish() would append without ever
        // being rescanned — drop them loudly instead of silently growing an
        // inventory nobody rebuilds. clearAll() re-opens the decoder.
        if (m_finished) {
            if (!m_postFinishWarned) {
                m_postFinishWarned = true;
                qWarning() << "[TPEGDecoder] data fed after finish() ignored "
                              "(one-shot warning); call clearAll() to re-open";
            }
            return;
        }

        // A-H3: bounded raw buffer. The tail beyond the cap is refused (no
        // drop-oldest re-basing), so 24-byte frame/marker accounting stays
        // exact; a live carousel repeats, so the first kBufferCap bytes hold
        // the full message set anyway.
        if (m_buffer.size() >= kBufferCap) {
            m_droppedBytes += static_cast<quint64>(chunk.size());
            m_appended += static_cast<quint64>(chunk.size());
            if (!m_bufferCapped) {
                m_bufferCapped = true;
                qWarning() << "[TPEGDecoder] stream buffer cap" << kBufferCap
                           << "bytes reached — refusing further bytes "
                              "(one-shot warning)";
            }
            return;
        }
        const qsizetype room = kBufferCap - m_buffer.size();
        const qsizetype take = qMin<qsizetype>(room, chunk.size());
        m_buffer.append(chunk.constData(), take);
        if (take < chunk.size()) {
            m_droppedBytes += static_cast<quint64>(chunk.size() - take);
            if (!m_bufferCapped) {
                m_bufferCapped = true;
                qWarning() << "[TPEGDecoder] stream buffer cap" << kBufferCap
                           << "bytes reached — refusing the remaining "
                           << (chunk.size() - take) << "bytes (one-shot warning)";
            }
        }
        m_appended += static_cast<quint64>(chunk.size());

        // 24-byte frame accounting: the buffer may receive arbitrary chunk sizes
        // (normally one 24-byte CIF per DAB frame). Count whole 24-byte units.
        const qint64 whole = m_buffer.size() / 24;
        const quint64 hadFrames = m_frames;
        m_frames = static_cast<quint64>(whole);

        // Marker distribution: first byte of each complete 24-byte frame.
        // qsizetype offset: `f * 24` overflows int beyond 2 GB.
        for (quint64 f = hadFrames; f < m_frames; ++f) {
            const qsizetype off = static_cast<qsizetype>(f) * 24;
            ++m_markers[static_cast<quint8>(m_buffer.at(off))];
        }

        // A-M4d: the framing decision is re-evaluated on every call until
        // kFramingDecideFrames frames are in (or finish() commits it) instead
        // of being frozen from the first 96 bytes alone.
        if (!m_framingReported && m_frames >= kFramingDecideFrames) {
            decideFramingLocked();
        }

        // Copy out while still locked; emit after the unlock (A-M4a — the
        // BUG-001 pattern: never emit while holding m_mutex).
        framingToEmit = m_pendingFraming;
        m_pendingFraming.clear();
    }
    if (!framingToEmit.isEmpty()) {
        emit tpegFramingDetected(framingToEmit);
    }
    emit frameCountChanged();
}

void TpegDecoder::decideFramingLocked()
{
    const bool sync = containsSyncPair(m_buffer);
    const bool carousel = detectsCarouselFraming(m_buffer.left(96));
    QString desc;
    if (sync) {
        desc = QString(
            "TPEG transport framing (TS 102 894-1): 0x0A 0x46 sync present");
    } else if (carousel) {
        desc = QString(
            "TPEG2-style carousel framing (no TS 102 894-1 0x0A 0x46 sync): "
            "24-byte frames with 4-phase markers (0x10/0x20/0x30/0x00 01 13)");
    } else {
        desc = QString(
            "Unrecognised TPEG framing: no 0x0A 0x46 sync and no carousel "
            "marker phases observed (raw stream analysis)");
    }
    m_framingReported = true;
    m_detectedFraming = desc;
    m_pendingFraming = desc;
}

void TpegDecoder::finish()
{
    bool inventoryChanged = false;
    QString framingToEmit;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_finished) {
            m_finished = true;
            if (!m_framingReported) {
                decideFramingLocked();   // A-M4d: commit on finish()
            }
            rescanInventoryLocked();
            inventoryChanged = true;
        }
        framingToEmit = m_pendingFraming;
        m_pendingFraming.clear();
    }
    if (inventoryChanged) {
        emit messageCountChanged();
    }
    if (!framingToEmit.isEmpty()) {
        emit tpegFramingDetected(framingToEmit);
    }
}

int TpegDecoder::refreshInventory()
{
    int count = 0;
    {
        QMutexLocker locker(&m_mutex);
        rescanInventoryLocked();
        count = static_cast<int>(m_messages.size());
    }
    // BUG-001 / A-M4a: emit after the unlock. The inventory CONTENT (repeat
    // counts, first/last frame) may have changed even when the count did not,
    // so the signal is emitted unconditionally — callers coalesce it anyway.
    emit messageCountChanged();
    return count;
}

void TpegDecoder::rescanInventoryLocked()
{
    // Extract printable ASCII runs (>= 6 chars); these are the visible
    // TPEG message texts. The carousel repeats the same small message set, so
    // first-seen order defines the inventory and each entry counts its repeats
    // (observed occurrences across the whole stream).
    //
    // A-H3: the inventory is BOUNDED (kMaxUniqueMessages unique texts). The
    // rolling version marker `R-3.1-wip|000078113` would otherwise mint a new
    // unique entry per update and grow without bound on a live stream; texts
    // that do not fit are counted per occurrence in m_droppedMessages.
    struct Agg {
        uint32_t count = 0;
        quint64 first = 0;
        quint64 last = 0;
    };
    QVector<QString> order;
    QMap<QString, Agg> agg;
    m_droppedMessages = 0;

    QString run;
    qsizetype runStart = -1;
    auto flushAt = [&](qsizetype endPos) {
        const qsizetype start = runStart;
        runStart = -1;
        if (run.size() < 6) {
            run.clear();
            return;
        }
        const QString text = run.trimmed();
        run.clear();
        if (text.isEmpty()) {
            return;
        }
        const quint64 firstFrame = static_cast<quint64>(start) / 24;
        const quint64 lastFrame =
            static_cast<quint64>(endPos > 0 ? endPos - 1 : 0) / 24;
        auto it = agg.find(text);
        if (it == agg.end()) {
            if (agg.size() >= kMaxUniqueMessages) {
                ++m_droppedMessages;
                return;
            }
            Agg a;
            a.count = 1;
            a.first = firstFrame;
            a.last = lastFrame;
            agg.insert(text, a);
            order.append(text);
            return;
        }
        ++it->count;
        it->last = lastFrame;
    };

    for (qsizetype i = 0; i < m_buffer.size(); ++i) {
        const uchar c = static_cast<uchar>(m_buffer.at(i));
        if (c >= 0x20 && c < 0x7F) {
            if (runStart < 0) {
                runStart = i;
            }
            run.append(static_cast<char>(c));
        } else if (runStart >= 0) {
            flushAt(i);
        }
    }
    if (runStart >= 0) {
        flushAt(m_buffer.size());
    }

    QVector<TpegMessage> fresh;
    fresh.reserve(order.size());
    uint32_t repeats = 0;
    for (uint32_t i = 0; i < static_cast<uint32_t>(order.size()); ++i) {
        const QString& text = order.at(static_cast<int>(i));
        const Agg& a = agg.value(text);
        TpegMessage m;
        m.index = i + 1;
        m.text = text;
        m.category = classifyText(text);
        m.repeat_count = a.count;
        m.first_frame = a.first;
        m.last_frame = a.last;
        repeats += a.count - 1;
        fresh.append(m);
    }
    m_repeatTotal = repeats;
    m_messages = fresh;
}

QString TpegDecoder::classifyText(const QString& text)
{
    if (text.contains("TEC") && text.contains("TFP") && text.contains("PKI")) {
        return "banner";
    }
    if (text.contains("R-3.1") || text.contains("wip")) {
        return "version";
    }
    if (text.contains("TEC", Qt::CaseInsensitive)) {
        return "TEC";
    }
    if (text.contains("TFP", Qt::CaseInsensitive)) {
        return "TFP";
    }
    if (text.contains("PKI", Qt::CaseInsensitive)) {
        return "PKI";
    }
    if (text.contains("EMI", Qt::CaseInsensitive)) {
        return "EMI";
    }
    if (text.contains("Parking", Qt::CaseInsensitive) ||
        text.contains("charge", Qt::CaseInsensitive)) {
        return "PKI";
    }
    if (text.contains("Rd", Qt::CaseInsensitive) ||
        text.contains("Bangkok", Qt::CaseInsensitive) ||
        text.contains("Khwaeng", Qt::CaseInsensitive)) {
        return "TEC";
    }
    return "unknown";
}

quint64 TpegDecoder::frameCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_frames;
}

bool TpegDecoder::sawSyncBytes() const
{
    QMutexLocker locker(&m_mutex);
    return containsSyncPair(m_buffer);
}

QMap<quint8, quint32> TpegDecoder::markerDistribution() const
{
    QMutexLocker locker(&m_mutex);
    return m_markers;
}

QVector<TpegMessage> TpegDecoder::messages() const
{
    // Returned BY VALUE (A-M4c): the reference must not outlive the lock.
    QMutexLocker locker(&m_mutex);
    return m_messages;
}

int TpegDecoder::messageCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_messages.size();
}

QString TpegDecoder::framingDescription() const
{
    QMutexLocker locker(&m_mutex);
    if (!m_framingReported) {
        return QString("framing not yet determined (insufficient data)");
    }
    return m_detectedFraming;
}

TpegDecoder::Statistics TpegDecoder::getStatistics() const
{
    QMutexLocker locker(&m_mutex);
    Statistics st;
    st.frames = m_frames;
    st.bytes_appended = m_appended;
    st.unique_messages = m_messages.size();
    st.repeat_count = static_cast<int>(m_repeatTotal);
    st.sync_0a46 = containsSyncPair(m_buffer);
    st.dropped_messages = m_droppedMessages;
    st.dropped_bytes = m_droppedBytes;
    return st;
}

void TpegDecoder::clearAll()
{
    QString pendingFraming;
    {
        QMutexLocker locker(&m_mutex);
        m_buffer.clear();
        m_appended = 0;
        m_frames = 0;
        m_finished = false;
        m_framingReported = false;
        m_bufferCapped = false;
        m_postFinishWarned = false;
        m_markers.clear();
        m_messages.clear();
        m_droppedMessages = 0;
        m_droppedBytes = 0;
        m_detectedFraming.clear();
        m_pendingFraming.clear();
        m_repeatTotal = 0;
        // A-M4b: never emit while holding m_mutex (the BUG-001 pattern
        // documented in journaline_decoder.cpp) — copy out and emit below.
        pendingFraming.clear();
    }
    Q_UNUSED(pendingFraming);
    emit frameCountChanged();
    emit messageCountChanged();
    qDebug() << "[TPEGDecoder] All TPEG state cleared";
}

} // namespace eti::tpeg