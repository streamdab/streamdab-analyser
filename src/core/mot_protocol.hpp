/**
 * @file mot_protocol.hpp
 * @brief MOT (Multimedia Object Transfer) Protocol Parser
 *
 * Implements MOT Protocol per ETSI EN 301 234 for DAB data services.
 * Supports MOT SlideShow, MOT Broadcast Web Site, and general MOT objects.
 *
 * Standard-layout contract (T26)
 * -----------------------------
 * processMOTData() consumes the *standard* ETSI EN 300 401 §5.3.3 MSC data
 * group exactly as it arrives from the PAD → data-group assembly, i.e.:
 * @verbatim
 *   [0]      Data group header byte 0
 *              extension(0x80) crc(0x40) segment(0x20) user_access(0x10) type(0x0F)
 *   [1]      Data group header byte 1 (continuity index | repetition index)
 *   [2..3]   Data group extension (only if the extension flag is set)
 *   [..]     MOT session header (EN 300 401 §5.3.3.1; on DAB+ audio the
 *            data group is carried in X-PAD CI 12/13, EN 300 401 §7.4.2)
 *              last-segment flag (1) | segment number (15)
 *              transport-id flag (1) | length indicator (4)
 *              transport id (length-indicator bytes; 0/2/4)
 *   [..]     Segmentation header (EN 301 234 §5.1.1): 13-bit segment size
 *            (top 3 bits = RepetitionCount, parsed but ignored)
 *   [..]     Segment data (segment-size bytes)
 *   [..]     CRC-16 (2 bytes) = one's complement of CRC-16/CCITT-FALSE over the
 *            data group up to but excluding the CRC
 * @endverbatim
 * Data-group type 3 carries a MOT *header* entity, type 4 a MOT *body* entity.
 * Both entities are reassembled independently (order-independent, gap tolerant)
 * and, once complete, the standard 7-byte header core plus its extension
 * parameters are parsed and the object is emitted. No re-framing or synthesized
 * legacy layout is involved.
 *
 * Key Features:
 * - Standard MSC Data Group parsing (ETSI EN 300 401 §5.3.3)
 * - Standard MOT header/directory parsing (ETSI EN 301 234 §5)
 * - Header entity/body entity reassembly with duplicate detection
 * - Content type detection (JPEG, PNG, HTML, etc.)
 * - Header extension parameter extraction (name, trigger time, category, URL)
 * - Full 16-bit transport id (0–65535)
 * - Object carousel tracking with versioning
 * - CRC-16/CCITT-FALSE (one's complement) validation via core/crc16.hpp
 * - UTF-8 support for Thai language content
 *
 * Thread-safe with Qt signal/slot integration for GUI updates.
 *
 * @see ETSI EN 301 234 - MOT Protocol Specification
 * @see ETSI TS 101 499 - MOT SlideShow Application
 * @see ETSI EN 300 401 - DAB System (MSC Data Groups)
 *
 * @author StreamDAB Development Team
 * @date October 2026
 * @version 2.0
 * @copyright Copyright (c) 2025 StreamDAB Technologies
 */

#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <vector>
#include <map>
#include <optional>
#include <functional>
#include <cstdint>
#include <memory>

namespace eti::mot {

/**
 * @brief MOT Content Type Enumeration
 *
 * ETSI EN 301 234 §6.1: Content type field (6 bits) of the MOT header core.
 */
enum class ContentType : uint8_t {
    GENERAL_DATA = 0x00,     ///< General data object
    TEXT = 0x01,             ///< Text content
    IMAGE_JPEG = 0x02,       ///< JPEG image (SlideShow)
    IMAGE_PNG = 0x03,        ///< PNG image
    IMAGE_BMP = 0x04,        ///< BMP image
    HTML = 0x05,             ///< HTML document (Broadcast Web Site)
    AUDIO_MPEG = 0x06,       ///< MPEG audio
    VIDEO_MPEG = 0x07,       ///< MPEG video
    UNKNOWN = 0xFF           ///< Unknown/invalid type
};

/**
 * @brief MOT Header Parameter IDs
 *
 * ETSI EN 301 234 §6.2.2.2, Table 5: header extension parameter types
 * (6-bit ParamId; the preceding 2 bits are the Parameter Length Indicator —
 * see §6.2 Fig.22).
 */
enum class HeaderParameterId : uint8_t {
    TRIGGER_TIME = 0x05,             ///< Trigger time (MJD + UTC)
    RETRANSMISSION_DISTANCE = 0x07,  ///< Retransmission distance
    EXPIRATION_TIME = 0x09,          ///< Expiration time (MJD + UTC)
    COMPRESSION_TYPE = 0x0A,         ///< Compression type
    CONTENT_NAME = 0x0C,             ///< Content name (charset + text)
    UNIQUE_BODY_VERSION = 0x0D,      ///< Unique body version (was CONTENT_VERSION)
    MIME_TYPE = 0x10,                ///< MIME type string
    CATEGORY_TITLE = 0x26,           ///< Category title (SlideShow)
    CLICK_THROUGH_URL = 0x27         ///< Click-through URL (SlideShow)
};

/**
 * @brief MOT Header Structure
 *
 * ETSI EN 301 234 §6.1: MOT header core (7 bytes, 56 bits):
 * - Body size (28 bits): size of the MOT body in bytes
 * - Header size (13 bits): size of the MOT header (core + extension params)
 * - Content type (6 bits): JPEG, PNG, HTML, …
 * - Content subtype (9 bits): subtype identifier
 *
 * followed by the variable-length header extension parameter list (§6.2,
 * Fig.22 coding; each parameter = PLI(2) | ParamId(6) | DataFieldLength |
 * DataField, parameter types per §6.2.2.2 Table 5).
 */
struct MOTHeader {
    uint32_t body_size = 0;              ///< Body size in bytes (28 bits)
    uint16_t header_size = 0;            ///< Header size in bytes (13 bits)
    ContentType content_type = ContentType::GENERAL_DATA;  ///< Content type
    uint16_t content_subtype = 0;        ///< Content subtype (9 bits)

    // Header extension parameters
    struct HeaderParameter {
        uint8_t param_id = 0;            ///< Parameter ID
        std::vector<uint8_t> param_data; ///< Parameter data
    };
    std::map<uint8_t, HeaderParameter> parameters; ///< Parameter map

    // Extracted common parameters (for convenience). Text parameters carry a
    // leading charset/Rfa byte (EN 301 234 §6.2.2.1.1 Fig.24) which is stripped
    // before decoding; the raw ParamId is in `parameters`.
    QString content_name;                ///< Content name (ParamId 0x0C)
    uint32_t trigger_time = 0;           ///< Trigger time in seconds (ParamId 0x05)
    QString category_title;              ///< Category title (ParamId 0x26 - SlideShow)
    QString click_through_url;           ///< Click-through URL (ParamId 0x27 - SlideShow)
    uint8_t compression_type = 0;        ///< Compression type (ParamId 0x0A)
    uint16_t unique_body_version = 0;    ///< Unique body version (ParamId 0x0D)

    /**
     * @brief Get content type as string
     */
    QString getContentTypeString() const;

    /**
     * @brief Check if header has a specific parameter
     */
    bool hasParameter(uint8_t param_id) const {
        return parameters.find(param_id) != parameters.end();
    }

    /**
     * @brief Get parameter data if exists
     */
    std::optional<std::vector<uint8_t>> getParameterData(uint8_t param_id) const {
        auto it = parameters.find(param_id);
        if (it != parameters.end()) {
            return it->second.param_data;
        }
        return std::nullopt;
    }

    QString toString() const;
};

/**
 * @brief MOT Directory Entry
 *
 * ETSI EN 301 234: MOT directory structure
 */
struct MOTDirectoryEntry {
    uint32_t transport_id = 0;           ///< Transport ID (16 or 32-bit)
    uint16_t header_size = 0;            ///< Header size in bytes
    uint32_t body_size = 0;              ///< Body size in bytes
    uint8_t repetition_count = 0;        ///< Repetition counter

    QString toString() const;
};

/**
 * @brief MOT Directory Structure
 *
 * ETSI EN 301 234: MOT directory structure
 */
struct MOTDirectory {
    uint16_t directory_size = 0;         ///< Directory size in bytes
    uint16_t num_objects = 0;            ///< Number of objects in carousel
    uint32_t carousel_period_ms = 0;     ///< Carousel period in milliseconds
    uint16_t segment_size = 0;           ///< Segment size in bytes

    std::vector<MOTDirectoryEntry> entries; ///< Directory entries

    // CRC validation
    bool has_valid_crc = false;          ///< CRC validation result
    uint16_t crc_calculated = 0;         ///< Calculated CRC
    uint16_t crc_received = 0;           ///< Received CRC

    QString toString() const;
};

/**
 * @brief Complete MOT Object
 *
 * Represents a fully reassembled MOT object with header and body.
 */
struct MOTObject {
    uint32_t transport_id = 0;           ///< Transport ID (full 16-bit)
    MOTHeader header;                    ///< Parsed standard MOT header
    std::vector<uint8_t> body;           ///< MOT body (complete object data)

    QDateTime received_time;             ///< Reception timestamp
    uint16_t carousel_id = 0;            ///< Carousel identifier

    /**
     * @brief Check if object is complete and valid
     */
    bool isValid() const {
        return !body.empty() &&
               body.size() == header.body_size &&
               header.header_size > 0;
    }

    QString toString() const;
};

/**
 * @brief MSC Data Group Structure
 *
 * ETSI EN 300 401 Section 5.3.3: MSC Data Groups
 */
struct MSCDataGroup {
    bool extension_flag = false;         ///< Extension flag
    bool crc_flag = false;               ///< CRC flag
    bool segment_flag = false;           ///< Segment flag
    bool user_access_flag = false;       ///< User access flag
    uint8_t data_group_type = 0;         ///< Data group type (4 bits)
    uint8_t continuity_index = 0;        ///< Continuity index (4 bits)
    uint8_t repetition_index = 0;        ///< Repetition index (4 bits)

    std::vector<uint8_t> extension_data; ///< Data group extension (if flagged)
    std::vector<uint8_t> data_field;     ///< Data field (after header/extension)
    uint16_t crc_received = 0;           ///< Received CRC
    bool crc_valid = false;              ///< CRC validation result

    QString toString() const;
};

/**
 * @brief MOT Protocol Parser Class
 *
 * Standard-layout MOT decoder. Feed it complete EN 300 401 §5.3.3 MSC data
 * groups (as produced by the DAB+ X-PAD CI 12/13 assembly or any other MSC
 * source); it parses the DG header, MOT session header and segmentation
 * header, reassembles the header entity (type 3) and body entity (type 4),
 * parses the standard MOT header core + extension parameters, and emits
 * objectComplete() with a MOTObject carrying the full 16-bit transport id,
 * the real header fields and the exact body bytes.
 *
 * Features:
 * - Standard MSC data group parsing with CRC-16/CCITT-FALSE validation
 * - Standard MOT header/directory parsing (28-bit body size, 13-bit header size)
 * - Header/body entity reassembly with progress tracking
 * - Header extension parameter extraction (name, URL, category, trigger time)
 * - Full 16-bit transport id (0–65535)
 * - Object carousel version tracking (changed header → reset)
 * - Bounded reassembly (2 MiB/object, assembly cap, stale eviction)
 * - Thread-safe operation, Qt signal emission for object completion
 *
 * Usage:
 * @code
 *   MOTProtocol mot_parser;
 *   connect(&mot_parser, &MOTProtocol::objectComplete,
 *           this, &MyClass::handleMOTObject);
 *   mot_parser.processMOTData(service_id, msc_data_group);
 * @endcode
 */
class MOTProtocol : public QObject {
    Q_OBJECT

public:
    explicit MOTProtocol(QObject* parent = nullptr);
    ~MOTProtocol() override;

    // ========================================================================
    // Reassembly limits
    // ========================================================================

    /// Upper bounds guarding against a malicious/garbled stream.
    struct Limits {
        uint32_t maxBodyBytes = 2u * 1024u * 1024u;  ///< 2 MiB per object
        uint32_t maxAssemblies = 64;                 ///< concurrent transport ids
        uint32_t staleTicks = 8192;                  ///< data groups w/o activity
        uint32_t maxSegments = 4096;                 ///< segments per entity
    };
    void setLimits(const Limits& limits);
    const Limits& limits() const { return m_limits; }

    // ========================================================================
    // Main Processing Interface
    // ========================================================================

    /**
     * @brief Process one standard EN 300 401 §5.3.3 MSC data group.
     * @param service_id DAB service id carrying MOT (0 if unknown)
     * @param msc_data Complete MSC data group (header..CRC)
     * @return true if the data group was structurally valid and accepted
     *
     * Parses the data-group header, validates the CRC (one's complement
     * CRC-16/CCITT-FALSE), parses the MOT session header and segmentation
     * header, then reassembles the MOT header/body entities. Emits
     * objectComplete() when a full object is available.
     */
    bool processMOTData(uint32_t service_id, const QByteArray& msc_data);

    /**
     * @brief Parse a standard 7-byte MOT header core (ETSI EN 301 234 §6.1).
     * @param data Header buffer (at least 7 bytes)
     * @param size Buffer size
     * @param bodySize Output: 28-bit body size
     * @param headerSize Output: 13-bit header size
     * @param contentType Output: 6-bit content type
     * @param contentSubType Output: 9-bit content subtype
     * @return true if the core could be parsed
     *
     * Public/static so the PAD adapter and tests can share one implementation.
     */
    static bool parseHeaderCore(const uint8_t* data, size_t size,
                                uint32_t& bodySize, uint16_t& headerSize,
                                uint8_t& contentType, uint16_t& contentSubType);

    /**
     * @brief Parse MSC data group from raw data
     * @param msc_data Raw MSC data
     * @return Parsed data group or nullopt on error
     *
     * ETSI EN 300 401 Section 5.3.3: MSC Data Group Structure
     */
    std::optional<MSCDataGroup> parseMSCDataGroup(const QByteArray& msc_data);

    /**
     * @brief Validate data group CRC
     * @param data_group Data group to validate
     * @return true if CRC is valid
     *
     * ETSI EN 300 401: the transmitted CRC is the one's complement of
     * CRC-16/CCITT-FALSE over the data group up to but excluding the CRC.
     */
    bool validateDataGroupCRC(const MSCDataGroup& data_group);

    // ========================================================================
    // MOT Header/Directory Parsing (ETSI EN 301 234 §6)
    // ========================================================================

    /**
     * @brief Parse standard MOT header (core + extension parameters).
     * @param data Raw MOT header data
     * @param header Output: parsed header structure
     * @return true if parsing successful
     */
    bool parseMOTHeader(const std::vector<uint8_t>& data, MOTHeader& header);

    /**
     * @brief Parse header extension parameters
     * @param data Parameter data
     * @param offset Start offset in data
     * @param length Parameter length
     * @param header Output: header to populate with parameters
     * @return true if parsing successful
     *
     * Extracts parameters: content name, trigger time, category, URL, etc.
     */
    bool parseHeaderParameters(const std::vector<uint8_t>& data,
                               size_t offset,
                               size_t length,
                               MOTHeader& header);

    /**
     * @brief Parse MOT directory
     * @param data Raw directory data
     * @param directory Output: parsed directory structure
     * @return true if parsing successful
     *
     * ETSI EN 301 234: MOT directory structure
     */
    bool parseMOTDirectory(const std::vector<uint8_t>& data, MOTDirectory& directory);

    // ========================================================================
    // Transport ID Management
    // ========================================================================

    /**
     * @brief Validate transport ID range
     * @param transport_id Transport ID to validate
     * @return true if valid (0-65535)
     *
     * The MOT transport id is a 16-bit field (ETSI EN 301 234 §6.2); real DAB
     * multiplexes use the full range (e.g. the Bangkok capture carries ids in
     * the 14000s).
     */
    bool validateTransportId(uint32_t transport_id) const {
        return transport_id <= 0xFFFF;
    }

    /**
     * @brief Get reassembly progress for a transport ID
     * @param transport_id Transport ID
     * @return Progress 0.0-1.0, or -1.0 if not found
     */
    double getProgress(uint32_t transport_id) const;

    /**
     * @brief Check if object is complete
     * @param transport_id Transport ID
     * @return true if all header and body segments received
     */
    bool isComplete(uint32_t transport_id) const;

    /**
     * @brief Get list of active transport IDs
     * @return Vector of active transport IDs
     */
    std::vector<uint32_t> getActiveTransportIds() const;

    /**
     * @brief Clear object from reassembly buffer
     * @param transport_id Transport ID to clear
     */
    void clearObject(uint32_t transport_id);

    /**
     * @brief Clear all objects
     */
    void clearAll();

    /**
     * @brief Reset the processing statistics (keeps active reassembly state).
     */
    void resetStatistics();

    // ========================================================================
    // Statistics & Status
    // ========================================================================

    /**
     * @brief Get MOT processing statistics
     */
    struct Statistics {
        uint64_t data_groups_processed = 0;  ///< Structurally parsed DGs
        uint64_t data_groups_valid = 0;      ///< DGs that passed CRC checks
        uint64_t segments_received = 0;      ///< Accepted entity segments
        uint64_t objects_completed = 0;      ///< Header+body fully reassembled
        uint64_t crc_errors = 0;             ///< CRC validation failures
        uint32_t active_objects = 0;         ///< In-flight assemblies

        // Detailed counters (shared with the PAD adapter's report)
        uint64_t rejected = 0;               ///< Structurally invalid DGs
        uint64_t header_segments = 0;        ///< DG type 3 segments
        uint64_t body_segments = 0;          ///< DG type 4 segments
        uint64_t objects_emitted = 0;        ///< objectComplete() emissions
        uint64_t bytes_received = 0;         ///< Entity payload bytes buffered
        uint64_t oversized = 0;              ///< Objects/segments over the cap
        uint64_t stale_evictions = 0;        ///< Incomplete assemblies evicted
        uint64_t object_resets = 0;          ///< Carousel restarts
        uint64_t max_object_bytes = 0;       ///< Largest emitted object body
    };
    Statistics getStatistics() const;

    /**
     * @brief Install a progress callback (in addition to the Qt signal).
     *
     * Used by the thin PAD adapter shim so existing GUI wiring keeps working.
     */
    void setProgressCallback(std::function<void(uint32_t, double)> cb);

signals:
    /**
     * @brief Emitted when MOT object is completely reassembled
     * @param transport_id Transport ID
     * @param object Complete MOT object
     */
    void objectComplete(uint32_t transport_id, const MOTObject& object);

    /**
     * @brief Emitted when reassembly progress updates
     * @param transport_id Transport ID
     * @param progress Progress 0.0-1.0
     */
    void reassemblyProgress(uint32_t transport_id, double progress);

    /**
     * @brief Emitted on MOT parsing error
     * @param transport_id Transport ID
     * @param error Error message
     */
    void parseError(uint32_t transport_id, const QString& error);

    /**
     * @brief Emitted when object is cleared (timeout or manual)
     * @param transport_id Transport ID
     */
    void objectCleared(uint32_t transport_id);

private:
    // ========================================================================
    // Internal reassembly state
    // ========================================================================

    struct Assembly {
        std::map<uint16_t, QByteArray> headerSegs;  ///< DG type 3 segments
        std::map<uint16_t, QByteArray> bodySegs;    ///< DG type 4 segments
        int lastHeaderSeg = -1;                     ///< last-segment flag, header
        int lastBodySeg = -1;                       ///< last-segment flag, body
        size_t headerBytes = 0;                     ///< header payload bytes
        size_t bodyBytes = 0;                       ///< body payload bytes (progress)
        bool headerParsed = false;                  ///< standard header core parsed
        MOTHeader header;                           ///< parsed standard header
        uint64_t lastActivityTick = 0;              ///< for stale/LRU eviction
    };

    static bool entityComplete(const std::map<uint16_t, QByteArray>& segs, int lastSeg);
    static QByteArray concatEntity(const std::map<uint16_t, QByteArray>& segs, int lastSeg);

    /// Evict stale incomplete assemblies and cap the map before a new key is
    /// created (bounded memory).
    void evictStaleAndCap();

    void emitObject(uint32_t serviceId, uint32_t transportId, Assembly& assembly);

    /**
     * @brief Decode a MOT text parameter (ContentName/CategoryTitle/
     *        ClickThroughURL) whose DataField starts with a charset/Rfa byte.
     *
     * EN 301 234 §6.2.2.1.1 Fig.24: the first byte holds the 4-bit character
     * set indicator (high nibble) and 4-bit Rfa (low nibble), followed by the
     * character field. Charset 0x0 = complete EBU Latin repertoire, 0xF = UTF-8.
     */
    QString decodeTextParameter(const std::vector<uint8_t>& data);

    /**
     * @brief Extract trigger time from parameter (MJD + UTC)
     */
    uint32_t extractTriggerTime(const std::vector<uint8_t>& data);

    // ========================================================================
    // Private Data Members
    // ========================================================================

    // Header/body entity reassembly buffers, keyed by transport id.
    std::map<uint32_t, Assembly> m_assemblies;

    // Statistics
    Statistics m_statistics;

    // Reassembly limits
    Limits m_limits;

    // Monotonic data-group counter (staleness / LRU ordering).
    uint64_t m_tick = 0;

    // Optional progress callback (thin-adapter compatibility).
    std::function<void(uint32_t, double)> m_progressCallback;

    // Mutex for thread-safe access. Recursive because processMOTData() holds
    // the lock while calling getProgress()/clearObject()/eviction helpers
    // (which lock again).
    mutable QRecursiveMutex m_mutex;
};

} // namespace eti::mot

// Qt metatype registration for signals/slots
Q_DECLARE_METATYPE(eti::mot::MOTHeader)
Q_DECLARE_METATYPE(eti::mot::MOTObject)
Q_DECLARE_METATYPE(eti::mot::MSCDataGroup)
Q_DECLARE_METATYPE(eti::mot::MOTDirectory)
