/**
 * @file mot_pad_adapter.hpp
 * @brief Adapter from EN 300 401 MSC data groups (MOT SlideShow, carried in
 *        DAB+ X-PAD CI 12/13) into the segment layout consumed by MOTProtocol.
 *
 * Background
 * ----------
 * The MOT SlideShow user application (ETSI TS 101 499) transports each MOT
 * object's header entity and body entity as a sequence of MOT *segments*, one
 * per MSC Data Group (ETSI EN 300 401 §5.3.3). On DAB+ audio services those
 * data groups are fragmented across X-PAD CI type 12 (MOT start) / 13 (MOT
 * continuation) sub-fields, with the total data-group length announced by a
 * preceding X-PAD CI type 1 (Data Group Length Indicator, DGLI).
 *
 * The on-air MSC data group layout is:
 * @verbatim
 *   [0]      Data group header byte 0
 *              extension(0x80) crc(0x40) segment(0x20) user_access(0x10) type(0x0F)
 *   [1]      Data group header byte 1 (continuity index | repetition index)
 *   [2..3]   Data group extension (only if extension flag set)
 *   [..]     Session header (user address, MOT):
 *              last-segment flag (1 bit) | segment number (15 bits)
 *              transport-id flag (1 bit) | length indicator (4 bits)
 *              transport id (length-indicator bytes; normally 2)
 *            Segmentation header: segment size (13 bits)
 *   [..]     Segment data (segment-size bytes)
 *   [..]     CRC-16 (2 bytes, if crc flag) = ~CRC-16/CCITT-FALSE over the
 *            data group up to but excluding the CRC
 * @endverbatim
 *
 * MOTProtocol (the existing decoder) consumes a *simplified* internal form: a
 * single segment carrying a 7-byte MOT header core immediately followed by the
 * object body, framed as an MSC data group whose data field is
 * [repetition(2) | segment(2) | transport id(2) | last+total(2) | payload].
 * This adapter collects the standard header/body entities per transport id and
 * re-frames a completed object into that internal form (see
 * buildReframedDataGroup()).
 *
 * @author StreamDAB Development Team
 * @date October 2026
 */

#pragma once

#include <QByteArray>
#include <QString>
#include <cstdint>
#include <functional>
#include <map>

#include "mot_protocol.hpp"

namespace eti::mot {

/**
 * @brief Adapts standard EN 300 401 MOT MSC data groups to MOTProtocol input.
 *
 * Plain class (no QObject): the owning GUI wires progress reporting through
 * setProgressCallback(). One instance may serve one or more services; the
 * transport id alone keys the reassembly state (MOT transport ids are unique
 * within an ensemble's MOT carousel).
 */
class MotPadAdapter {
public:
    /// Aggregated adapter counters (honest evidence for the load report).
    struct Statistics {
        uint64_t dataGroups = 0;      ///< Standard data groups offered
        uint64_t crcErrors = 0;       ///< Data groups with a bad CRC
        uint64_t rejected = 0;        ///< Structurally invalid data groups
        uint64_t headerSegments = 0;  ///< dg_type 3 (MOT header) segments
        uint64_t bodySegments = 0;    ///< dg_type 4 (MOT body) segments
        uint64_t objectsCompleted = 0;///< Header+body both fully reassembled
        uint64_t objectsEmitted = 0;  ///< Re-framed + handed to MOTProtocol
        uint64_t bytesReceived = 0;   ///< Segment payload bytes buffered
        uint64_t oversized = 0;       ///< Objects/segments dropped over the cap
        uint64_t staleEvictions = 0;  ///< Incomplete assemblies evicted
        uint64_t objResets = 0;       ///< Carousel restarts (repeat segment 0)
        uint64_t maxObjectBytes = 0;  ///< Largest emitted object body
    };

    /**
     * @param mot MOTProtocol to feed (may be null, set later). Not owned.
     */
    explicit MotPadAdapter(MOTProtocol* mot = nullptr);

    /// Set/replace the MOTProtocol sink (not owned).
    void setMOTProtocol(MOTProtocol* mot) { m_mot = mot; }
    /// Current MOTProtocol sink.
    MOTProtocol* motProtocol() const { return m_mot; }

    /**
     * @brief Process one complete standard MOT MSC data group.
     * @param serviceId DAB service id carrying MOT (0 if unknown)
     * @param dataGroup Complete EN 300 401 MSC data group (header..CRC)
     * @return true when the data group was structurally valid and accepted
     */
    bool processDataGroup(uint32_t serviceId, const QByteArray& dataGroup);

    /**
     * @brief Best-effort reassembly progress for a body entity.
     *
     * Fired on every accepted body segment with fraction = received bytes /
     * announced body size (0.0..1.0). This gives the UI a live "x %" value;
     * MOTProtocol's own reassembly is instantaneous for the re-framed
     * single-segment object, so its reassemblyProgress only reports 100 %.
     */
    void setProgressCallback(std::function<void(uint32_t, double)> cb)
    {
        m_onProgress = std::move(cb);
    }

    /// Drop all in-flight reassembly state (keeps counters).
    void reset();

    /// Counters snapshot.
    Statistics statistics() const { return m_stats; }

    /// Parse the standard MOT header core (ETSI EN 301 234 §5.1), exposed for
    /// tests: body size(28) header size(13) content type(6) subtype(9).
    static bool parseStandardHeader(const QByteArray& header, uint32_t& bodySize,
                                    uint16_t& headerSize, uint8_t& contentType,
                                    uint16_t& contentSubType);

    /// Build the MOTProtocol-internal MSC data group for one complete object.
    /// Exposed for tests. @p contentType is the standard 6-bit MOT content type.
    static QByteArray buildReframedDataGroup(uint32_t transportId, uint8_t contentType,
                                             const QByteArray& body);

    /// Upper bounds guarding against a malicious/garbled stream.
    static constexpr uint32_t MAX_BODY_SIZE = 2u * 1024u * 1024u;  // 2 MiB
    static constexpr int MAX_SEGMENTS = 4096;
    static constexpr uint32_t MAX_ASSEMBLIES = 64;   // concurrent transport ids
    static constexpr uint32_t STALE_TICKS = 8192;    // data groups w/o activity

    /// Tunable reassembly limits (defaults = the constants above). Settable so
    /// tests can exercise the size/staleness boundaries cheaply.
    struct Limits {
        uint32_t maxBodyBytes = MAX_BODY_SIZE;
        uint32_t maxAssemblies = MAX_ASSEMBLIES;
        uint32_t staleTicks = STALE_TICKS;
    };
    void setLimits(const Limits& limits) { m_limits = limits; }
    const Limits& limits() const { return m_limits; }

private:
    struct Assembly {
        std::map<int, QByteArray> headerSegs;  ///< dg_type 3 segments
        std::map<int, QByteArray> bodySegs;    ///< dg_type 4 segments
        int lastHeaderSeg = -1;
        int lastBodySeg = -1;
        uint32_t bodySize = 0;                 ///< announced by the header core
        uint8_t contentType = 0;               ///< standard 6-bit content type
        uint16_t contentSubType = 0;
        size_t headerBytes = 0;                ///< header payload bytes
        size_t bodyBytes = 0;                  ///< body payload bytes (progress)
        uint64_t lastActivityTick = 0;         ///< for stale/LRU eviction
    };

    static bool entityComplete(const std::map<int, QByteArray>& segs, int lastSeg);
    static QByteArray concatEntity(const std::map<int, QByteArray>& segs, int lastSeg);
    static bool parseStandardDataGroupHeader(const QByteArray& dg, int& offset,
                                             int& dgType, bool& crcFlag);
    static bool parseSessionHeader(const QByteArray& dg, int& offset, bool& lastSeg,
                                   int& segNumber, uint32_t& transportId);
    static bool parseSegmentationHeader(const QByteArray& dg, int& offset, int& segSize);

    /// Evict stale incomplete assemblies and cap the map before a new key is
    /// created (bounded memory).
    void evictStaleAndCap();

    void emitObject(uint32_t serviceId, uint32_t transportId, Assembly& assembly);

    MOTProtocol* m_mot = nullptr;
    std::map<uint32_t, Assembly> m_assemblies;
    Limits m_limits;
    uint64_t m_tick = 0;  ///< monotonic data-group counter
    Statistics m_stats;
    std::function<void(uint32_t, double)> m_onProgress;
};

} // namespace eti::mot
