/**
 * @file eti_file_scanner.cpp
 * @brief Synchronous ETI(NI) file scanner implementation.
 *
 * Layout (ETSI EN 300 799, RAW ETI):
 *   SYNC(4) + FC(4) + STC(4*NST) + EOH(4) + FIC(96/128) + MSC + EOF(4) + TIST(4)
 * Subchannels are stored contiguously in STC (== SAD) order; each subchannel
 * occupies STL capacity units of 8 bytes per 24 ms frame.
 *
 * @author StreamDAB Development Team
 * @date October 2026
 */

#include "eti_file_scanner.hpp"

#include <QFile>
#include <QHash>
#include <QDebug>

namespace eti {

namespace {
constexpr qint64 kEtiFrameSize = 6144;
constexpr int kFicSizeMode1 = 96;
constexpr int kFicSizeMode3 = 128;
/// NST is 7 bits (EN 300 799 §5.1) -> at most 127 stream descriptors.
constexpr int kMaxStcEntries = 128;

/// One Stream Organisation entry of the STC (4 bytes).
struct StcEntry {
    uint8_t sub_channel_id = 0;
    uint16_t start_address = 0;     ///< SAD (capacity units)
    uint16_t stream_length_cu = 0;  ///< STL (capacity units, 8 bytes each)
};

/**
 * @brief Full 4-byte SYNC validation (EN 300 799 §5.1.1).
 *
 * Mirrors the production acceptance list exactly
 * (ETIFrameParser::validateSyncPattern / ETIHeader::isValid): the ETI(NI)
 * byte-order variants plus both ETI(LI) patterns (0xFFF8C549 / 0xFF073AB6 and
 * their byte-reversed readings). The previous check only looked at byte 0
 * == 0xFF, which accepted arbitrary data that merely starts with 0xFF.
 */
bool isValidEtiSync(const uchar* b)
{
    const quint32 sync = (static_cast<quint32>(b[0]) << 24)
                       | (static_cast<quint32>(b[1]) << 16)
                       | (static_cast<quint32>(b[2]) << 8)
                       |  static_cast<quint32>(b[3]);
    // ETI(NI)
    if (sync == 0xFF1F491F || sync == 0x491F1FFF || sync == 0xFF1FC4FF ||
        sync == 0xC4FF1FFF || sync == 0x491FC4FF || sync == 0xC4FF491F ||
        sync == 0x49931E03) {
        return true;
    }
    // ETI(LI) — patterns A and B (incl. byte-reversed readings)
    return sync == 0xFFF8C549 || sync == 0x49C5F8FF ||
           sync == 0xFF073AB6 || sync == 0xB63A07FF;
}
} // namespace

const ScannedSubchannel* EtiScanResult::find(uint8_t sub_channel_id) const
{
    for (const ScannedSubchannel& sc : subchannels) {
        if (sc.sub_channel_id == sub_channel_id) {
            return &sc;
        }
    }
    return nullptr;
}

bool scanEtiFile(const QString& file_path, EtiScanResult& result,
                 quint64 frame_limit)
{
    result = EtiScanResult();
    result.file_path = file_path;

    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[EtiFileScanner] Cannot open" << file_path;
        return false;
    }

    const qint64 file_size = file.size();
    result.total_frames = static_cast<quint64>(file_size / kEtiFrameSize);

    // Accumulators keyed by SubChId: the STC of every frame is parsed fresh
    // (A-M7 — nst/SAD/STL may change mid-capture), while the bytes of a
    // subchannel are concatenated across the whole file in first-seen order.
    QVector<ScannedSubchannel> acc;
    QHash<int, int> acc_index;  // SubChId -> index into acc

    QByteArray frame(static_cast<int>(kEtiFrameSize), Qt::Uninitialized);
    quint64 frame_index = 0;

    while (true) {
        if (frame_limit > 0 && frame_index >= frame_limit) {
            break;
        }
        const qint64 got = file.read(frame.data(), kEtiFrameSize);
        if (got < kEtiFrameSize) {
            break;
        }

        const uchar* b = reinterpret_cast<const uchar*>(frame.constData());
        if (!isValidEtiSync(b)) {
            ++result.rejected_frames;
            ++frame_index;
            continue; // invalid SYNC: skip (same acceptance list as production)
        }
        const uint8_t byte5 = b[5];
        const bool ficf = (byte5 & 0x80) != 0;
        const int nst = byte5 & 0x7F;
        const uint8_t mid = static_cast<uint8_t>((b[6] >> 3) & 0x03);
        const int fic_len = (mid == 3) ? kFicSizeMode3 : kFicSizeMode1;
        const int msc_offset = 12 + 4 * nst + (ficf ? fic_len : 0);

        // STC is re-read from THIS frame (4 bytes per stream):
        //   byte 0: SubChId (bits 7-2) + SAD high (bits 1-0)
        //   byte 1: SAD low
        //   byte 2: TPL (bit 7) + STL high (bits 1-0)
        //   byte 3: STL low
        StcEntry entries[kMaxStcEntries];
        for (int i = 0; i < nst; ++i) {
            const int stc = 8 + 4 * i;
            entries[i].sub_channel_id = (b[stc] >> 2) & 0x3F;
            entries[i].start_address =
                static_cast<uint16_t>(((b[stc] & 0x03) << 8) | b[stc + 1]);
            entries[i].stream_length_cu =
                static_cast<uint16_t>(((b[stc + 2] & 0x03) << 8) | b[stc + 3]);
        }

        ++result.valid_frames;

        // FIC capture (first FIC-bearing frame).
        if (ficf && !result.has_fic) {
            const int fic_offset = 12 + 4 * nst;
            result.first_fic = frame.mid(fic_offset, fic_len);
            result.has_fic = true;
        }

        // Slice each subchannel's bytes for this frame (contiguous STC order).
        int offset = msc_offset;
        for (int i = 0; i < nst; ++i) {
            const StcEntry& e = entries[i];
            const int size = static_cast<int>(e.stream_length_cu) * 8;
            if (size <= 0 || offset + size > static_cast<int>(kEtiFrameSize)) {
                break;  // frame would run past the 6144-byte ETI frame
            }
            int idx = acc_index.value(e.sub_channel_id, -1);
            if (idx < 0) {
                idx = acc.size();
                ScannedSubchannel sc;
                sc.sub_channel_id = e.sub_channel_id;
                sc.start_address = e.start_address;
                sc.stream_length_cu = e.stream_length_cu;
                acc.append(sc);
                acc_index.insert(e.sub_channel_id, idx);
            }
            acc[idx].merged_bytes.append(frame.constData() + offset, size);
            acc[idx].frame_count++;
            offset += size;
        }

        ++frame_index;
    }

    for (ScannedSubchannel& sc : acc) {
        if (sc.frame_count > 0) {
            result.subchannels.append(sc);
        }
    }

    if (result.valid_frames == 0) {
        qWarning() << "[EtiFileScanner] No valid ETI frames in" << file_path
                   << "(" << result.rejected_frames << "frames rejected)";
        return false;
    }
    return true;
}

} // namespace eti