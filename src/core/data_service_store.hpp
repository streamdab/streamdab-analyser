/**
 * @file data_service_store.hpp
 * @brief GUI-agnostic per-service data model for decoded EPG / Journaline /
 *        TPEG content.
 *
 * v1.4 data-path wave: the decoders (EPGDecoder, JournalineDecoder,
 * TpegDecoder) own their parsing state; this store exposes a plain-struct
 * per-service view suitable for the upcoming GUI wave (no widgets here).
 *
 * A service entry is created the first time a data subchannel / service is
 * observed (its decoded content may arrive later — check
 * ServiceDataSnapshot::hasContent()); the accessors are plain Qt value types
 * so a GUI controller can register them with QML / widget models without
 * coupling.
 *
 * @author StreamDAB Development Team
 * @date October 2026
 */

#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QMap>
#include <cstdint>

#include "epg_decoder.hpp"
#include "journaline_decoder.hpp"
#include "tpeg_decoder.hpp"

namespace eti::data {

/**
 * @brief Per-service decoded-data summary (plain struct for the GUI wave).
 */
struct ServiceDataSnapshot {
    uint32_t sub_channel_id = 0;    ///< DAB subchannel carrying the data service
    uint32_t service_id = 0;        ///< DAB service id (0 when unknown)
    QString service_label;          ///< e.g. "EPG", "TEPG"

    quint64 stream_bytes = 0;       ///< raw stream bytes fed to the decoders
    quint64 stream_frames = 0;      ///< stream frames attributed to THIS
                                    ///< subchannel (TPEG: whole 24-byte frames
                                    ///< its bytes added to the shared decoder)

    // EPG (ETSI TS 102 371)
    int epg_schedules = 0;          ///< schedules stored
    int epg_events = 0;             ///< events discovered

    // Journaline (ETSI TS 102 979)
    int journaline_objects = 0;     ///< MOT objects decoded
    int journaline_stream_items = 0;///< display-text items from stream mode
    QString journaline_sample_title;///< first decoded title (for UI preview)

    // TPEG / TEPG (TS 102 894, base/partial)
    //
    // Attribution: the COUNTERS are per-subchannel deltas — what this
    // subchannel's own bytes contributed to the one shared TpegDecoder
    // (summing them over all TPEG rows equals the decoder's cumulative
    // total). The framing fields are shared-decoder ATTRIBUTES of the TPEG
    // stream (framing/sync are detected over the combined bytes), stamped
    // after the single end-of-feed finish(); they are identical on every
    // TPEG row by construction and must not be read as per-subchannel data.
    int tpeg_messages = 0;          ///< unique texts this subchannel ADDED
                                    ///< (delta on the shared inventory)
    QString tpeg_framing;           ///< shared decoder's framing verdict
    bool tpeg_sync_0a46 = false;    ///< TS 102 894-1 sync pair observed
                                    ///< (shared decoder)

    bool hasContent() const {
        return epg_events > 0 || journaline_objects > 0 ||
               journaline_stream_items > 0 || tpeg_messages > 0;
    }
};

/**
 * @brief Aggregates per-service decoded-data snapshots.
 *
 * The store is intentionally passive (no parsing): the data-path wave fills it
 * after running the decoders, and the GUI wave reads it. Q_OBJECT so a
 * controller can connect to its change signals.
 *
 * Thread affinity: like every other QObject in this application the store is
 * a GUI-thread-only object — updateService()/clear() are called from the GUI
 * thread (after the capture has been fed to the decoders) and the signals are
 * therefore delivered on the GUI thread. Do not touch it from a worker thread;
 * marshal the snapshot to the GUI thread first (Qt::QueuedConnection).
 */
class DataServiceStore : public QObject {
    Q_OBJECT

public:
    explicit DataServiceStore(QObject* parent = nullptr);

    /**
     * @brief Upsert a snapshot for a subchannel (replaces the old entry).
     */
    void updateService(const ServiceDataSnapshot& snapshot);

    /**
     * @brief Snapshot for a subchannel, or false when absent.
     */
    bool getService(uint32_t sub_channel_id, ServiceDataSnapshot& out) const;

    /**
     * @brief All stored snapshots (sorted by subchannel id).
     */
    QVector<ServiceDataSnapshot> getServices() const;

    /**
     * @brief Total number of tracked services.
     *
     * Counts every upsert regardless of whether the entry carries decoded
     * content yet (an entry is created as soon as a subchannel/service is
     * seen); filter with ServiceDataSnapshot::hasContent() when only
     * content-bearing services matter.
     */
    int serviceCount() const;

    /**
     * @brief Clear every entry and emit servicesCleared().
     */
    void clear();

signals:
    /// Emitted whenever a service snapshot is added or replaced.
    void serviceUpdated(uint32_t sub_channel_id);
    /// Emitted after clear() emptied the store (GUI: drop its rows too).
    void servicesCleared();

private:
    QMap<uint32_t, ServiceDataSnapshot> m_bySubChannel;
};

} // namespace eti::data