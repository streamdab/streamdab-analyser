/**
 * @file eti_file_scanner.hpp
 * @brief Synchronous ETI(NI) file scanner: per-subchannel byte streams and
 *        per-frame FIC, with the same EN 300 799 layout used by the
 *        production ETI frame parser.
 *
 * Used by the capture-backed (PRIVATE) tests and by the data-path wave to
 * extract the raw bytes of data subchannels (EPG, TEPG, Journaline) without
 * depending on the GUI load path or timers.
 *
 * @author StreamDAB Development Team
 * @date October 2026
 */

#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>
#include <cstdint>

namespace eti {

/**
 * @brief One scanned subchannel: identifier + concatenated 24 ms slices.
 */
struct ScannedSubchannel {
    uint8_t sub_channel_id = 0;
    uint16_t start_address = 0;     ///< Sub-channel Start Address (SAD) in CUs
    uint16_t stream_length_cu = 0;  ///< STL (capacity units, 8 bytes each)
    QByteArray merged_bytes;        ///< concatenated per-frame slices
    quint64 frame_count = 0;        ///< number of ETI frames that carried it
};

/**
 * @brief Result of scanning an ETI(NI) file.
 */
struct EtiScanResult {
    QString file_path;
    quint64 total_frames = 0;   ///< file size / 6144 (including trailing partial)
    quint64 valid_frames = 0;   ///< frames whose 4-byte SYNC was accepted
    quint64 rejected_frames = 0;///< frames skipped because of an invalid SYNC
    QVector<ScannedSubchannel> subchannels;
    QByteArray first_fic;           ///< FIC bytes of the first FIC-bearing frame
    bool has_fic = false;

    /**
     * @brief Find a subchannel by id (nullptr when absent).
     */
    const ScannedSubchannel* find(uint8_t sub_channel_id) const;
};

/**
 * @brief Synchronously scan an ETI(NI) file (6144-byte frames, EN 300 799).
 *
 * Mirrors the production subchannel slicing: the STC is parsed from every
 * frame (so an nst/SAD/STL reconfiguration mid-capture is handled instead of
 * slicing with a stale descriptor), one capacity unit = 8 bytes, and frames
 * are accepted only when the full 4-byte SYNC matches the production
 * acceptance list (ETI(LI)/ETI(NI) patterns). Bytes of a subchannel that is
 * reconfigured are concatenated in first-seen order (the test captures are
 * single-ensemble recordings).
 *
 * Caveat: NST is read as (byte5 & 0x7F) exactly — the legacy-mux correction
 * AnalyserSettings::nst_offset_1 (NST = (byte5 & 0x7F) + 1) is a GUI/CLI
 * setting and is deliberately NOT applied here, so an ensemble that needs it
 * would be mis-sliced by this test helper. The production path takes it from
 * AnalyserSettings.
 *
 * @param file_path Path to the .eti file.
 * @param result    Filled result structure.
 * @param frame_limit Optional early-stop (0 = whole file).
 * @return true when at least one valid frame was scanned.
 */
bool scanEtiFile(const QString& file_path, EtiScanResult& result,
                 quint64 frame_limit = 0);

} // namespace eti