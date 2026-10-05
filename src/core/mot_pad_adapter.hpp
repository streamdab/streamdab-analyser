/**
 * @file mot_pad_adapter.hpp
 * @brief Thin compatibility shim: standard EN 300 401 MSC data groups into
 *        MOTProtocol (no re-framing).
 *
 * Background (T26)
 * ----------------
 * The MOT SlideShow user application (ETSI TS 101 499) transports each MOT
 * object's header entity and body entity as a sequence of MOT *segments*, one
 * per MSC Data Group (ETSI EN 300 401 §5.3.3). On DAB+ audio services those
 * data groups are fragmented across X-PAD CI type 12 (MOT start) / 13 (MOT
 * continuation) sub-fields, with the total data-group length announced by a
 * preceding X-PAD CI type 1 (Data Group Length Indicator, DGLI).
 *
 * MOTProtocol now consumes the standard data group directly (DG header, MOT
 * session header, segmentation header, header/body entity reassembly, standard
 * header core + extension parameters). This class therefore no longer parses or
 * re-frames anything: processDataGroup() simply forwards the complete standard
 * data group to MOTProtocol::processMOTData(). It remains as the stable façade
 * used by the GUI and the existing tests (statistics, limits, progress
 * callback, limits constants).
 *
 * @author StreamDAB Development Team
 * @date October 2026
 */

#pragma once

#include <QByteArray>
#include <cstdint>
#include <functional>

#include "mot_protocol.hpp"

namespace eti::mot {

/**
 * @brief Façade over MOTProtocol for the PAD → DG transport path.
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
        uint64_t objectsEmitted = 0;  ///< Objects handed to objectComplete
        uint64_t bytesReceived = 0;   ///< Segment payload bytes buffered
        uint64_t oversized = 0;       ///< Objects/segments dropped over the cap
        uint64_t staleEvictions = 0;  ///< Incomplete assemblies evicted
        uint64_t objResets = 0;       ///< Carousel restarts (changed header)
        uint64_t maxObjectBytes = 0;  ///< Largest emitted object body
    };

    /**
     * @param mot MOTProtocol to feed (may be null, set later). Not owned.
     */
    explicit MotPadAdapter(MOTProtocol* mot = nullptr);

    /// Set/replace the MOTProtocol sink (not owned).
    void setMOTProtocol(MOTProtocol* mot);

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
     * Forwarded to MOTProtocol: fraction = received bytes / announced body
     * size (0.0..1.0).
     */
    void setProgressCallback(std::function<void(uint32_t, double)> cb);

    /// Drop all in-flight reassembly state (keeps counters).
    void reset();

    /// Counters snapshot (mapped from MOTProtocol's detailed statistics).
    Statistics statistics() const;

    /// Parse the standard MOT header core (ETSI EN 301 234 §5.1), exposed for
    /// tests: body size(28) header size(13) content type(6) subtype(9).
    static bool parseStandardHeader(const QByteArray& header, uint32_t& bodySize,
                                    uint16_t& headerSize, uint8_t& contentType,
                                    uint16_t& contentSubType);

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
    void setLimits(const Limits& limits);
    const Limits& limits() const { return m_limits; }

private:
    MOTProtocol* m_mot = nullptr;
    Limits m_limits;
    uint64_t m_dataGroups = 0;  ///< Data groups offered (this adapter)
    std::function<void(uint32_t, double)> m_progressCallback;
};

} // namespace eti::mot
