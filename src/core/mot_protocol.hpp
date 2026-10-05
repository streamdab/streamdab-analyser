/**
 * @file mot_protocol.hpp
 * @brief MOT (Multimedia Object Transfer) Protocol Parser
 *
 * Implements MOT Protocol per ETSI EN 301 234 for DAB data services.
 * Supports MOT SlideShow, MOT Broadcast Web Site, and general MOT objects.
 *
 * Key Features:
 * - MOT header/directory parsing (ETSI EN 301 234 Section 5)
 * - Segment reassembly with duplicate detection
 * - Content type detection (JPEG, PNG, HTML, etc.)
 * - Header parameter extraction (name, trigger time, category, URL)
 * - Transport ID management (0-65535)
 * - Object carousel tracking with versioning
 * - MSC Data Group parsing (ETSI EN 300 401 Section 5.3.3)
 * - CRC validation for data groups
 * - UTF-8 support for Thai language content
 *
 * MOT Protocol Flow:
 * 1. Extract MSC Data Groups from ETI frames
 * 2. Parse data group headers and segment information
 * 3. Reassemble fragmented MOT objects
 * 4. Parse MOT header and extract parameters
 * 5. Emit signals for complete objects
 *
 * Thread-safe with Qt signal/slot integration for GUI updates.
 *
 * @see ETSI EN 301 234 - MOT Protocol Specification
 * @see ETSI TS 101 499 - MOT SlideShow Application
 * @see ETSI EN 300 401 - DAB System (MSC Data Groups)
 *
 * @author StreamDAB Development Team
 * @date October 20, 2025
 * @version 1.0
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
#include <cstdint>
#include <memory>

namespace eti::mot {

/**
 * @brief MOT Content Type Enumeration
 *
 * ETSI EN 301 234 Section 5.1: Content Type Field (6 bits)
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
 * ETSI EN 301 234 Section 5.1.1: Header Extension Parameters
 */
enum class HeaderParameterId : uint8_t {
    CONTENT_NAME = 12,           ///< Content name (UTF-8)
    TRIGGER_TIME = 5,            ///< Trigger time (MJD + UTC)
    CATEGORY_TITLE = 25,         ///< Category title (SlideShow)
    CLICK_THROUGH_URL = 27,      ///< Click-through URL (SlideShow)
    COMPRESSION_TYPE = 10,       ///< Compression type
    CONTENT_VERSION = 7,         ///< Content version number
    EXPIRATION_TIME = 6,         ///< Expiration time (MJD + UTC)
    PRIORITY = 8,                ///< Priority level
    MIME_TYPE = 20               ///< MIME type string
};

/**
 * @brief MOT Header Structure
 *
 * ETSI EN 301 234 Section 5.1: MOT Header
 *
 * Header Format:
 * - Body size (13 bits): Size of MOT body in bytes
 * - Header size (13 bits): Size of header including extensions
 * - Content type (6 bits): Type of content (JPEG, PNG, HTML, etc.)
 * - Content subtype (9 bits): Subtype identifier
 * - Header extension parameters: Variable-length parameter list
 */
struct MOTHeader {
    uint16_t body_size = 0;              ///< Body size in bytes (13 bits)
    uint16_t header_size = 0;            ///< Header size in bytes (13 bits)
    ContentType content_type = ContentType::GENERAL_DATA;  ///< Content type
    uint16_t content_subtype = 0;        ///< Content subtype (9 bits)

    // Header extension parameters
    struct HeaderParameter {
        uint8_t param_id = 0;            ///< Parameter ID
        std::vector<uint8_t> param_data; ///< Parameter data
    };
    std::map<uint8_t, HeaderParameter> parameters; ///< Parameter map

    // Extracted common parameters (for convenience)
    QString content_name;                ///< Content name (ParamId 12)
    uint32_t trigger_time = 0;           ///< Trigger time in seconds (ParamId 5)
    QString category_title;              ///< Category title (ParamId 25 - SlideShow)
    QString click_through_url;           ///< Click-through URL (ParamId 27 - SlideShow)
    uint8_t compression_type = 0;        ///< Compression type (ParamId 10)
    uint16_t content_version = 0;        ///< Content version (ParamId 7)

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
 * ETSI EN 301 234 Section 5.2: MOT Directory
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
 * ETSI EN 301 234 Section 5.2: MOT Directory
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
 * @brief MOT Segment Header
 *
 * ETSI EN 301 234 Section 6: Segmentation
 */
struct MOTSegmentHeader {
    uint16_t repetition_count = 0;       ///< Repetition counter
    uint16_t segment_number = 0;         ///< Current segment number
    uint16_t total_segments = 0;         ///< Total number of segments
    uint32_t transport_id = 0;           ///< Transport ID
    bool last_segment = false;           ///< Last segment flag

    QString toString() const;
};

/**
 * @brief Complete MOT Object
 *
 * Represents a fully reassembled MOT object with header and body
 */
struct MOTObject {
    uint32_t transport_id = 0;           ///< Transport ID
    MOTHeader header;                    ///< MOT header
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
 *
 * Data Group Header (2 bytes):
 * - Extension flag (1 bit)
 * - CRC flag (1 bit)
 * - Segment flag (1 bit)
 * - User access flag (1 bit)
 * - Data group type (4 bits)
 * - Continuity index (4 bits)
 * - Repetition index (4 bits)
 */
struct MSCDataGroup {
    bool extension_flag = false;         ///< Extension flag
    bool crc_flag = false;               ///< CRC flag
    bool segment_flag = false;           ///< Segment flag
    bool user_access_flag = false;       ///< User access flag
    uint8_t data_group_type = 0;         ///< Data group type (4 bits)
    uint8_t continuity_index = 0;        ///< Continuity index (4 bits)
    uint8_t repetition_index = 0;        ///< Repetition index (4 bits)

    std::vector<uint8_t> data_field;     ///< Data field
    uint16_t crc_received = 0;           ///< Received CRC
    bool crc_valid = false;              ///< CRC validation result

    QString toString() const;
};

/**
 * @brief Segment Buffer for Reassembly
 *
 * Internal structure for tracking segment reassembly progress
 */
struct SegmentBuffer {
    uint16_t total_segments = 0;         ///< Total expected segments
    std::map<uint16_t, std::vector<uint8_t>> segments; ///< Segment data map
    uint32_t last_update_time_ms = 0;    ///< Last update timestamp
    bool is_complete = false;            ///< Completion flag
    MOTHeader header;                    ///< Parsed header (when available)

    /**
     * @brief Get reassembly progress (0.0 - 1.0)
     */
    double getProgress() const {
        if (total_segments == 0) return 0.0;
        return static_cast<double>(segments.size()) / total_segments;
    }

    /**
     * @brief Check if all segments received
     */
    bool hasAllSegments() const {
        return total_segments > 0 && segments.size() == total_segments;
    }
};

/**
 * @brief MOT Protocol Parser Class
 *
 * Professional MOT protocol implementation with complete ETSI EN 301 234 support.
 *
 * Features:
 * - MSC Data Group parsing with CRC validation
 * - MOT header/directory parsing
 * - Segment reassembly with progress tracking
 * - Header parameter extraction
 * - Transport ID management (0-65535)
 * - Object carousel tracking
 * - Timeout-based stale object cleanup
 * - Thread-safe operation
 * - Qt signal emission for object completion
 *
 * Usage:
 * @code
 *   MOTProtocol mot_parser;
 *
 *   // Connect signals
 *   connect(&mot_parser, &MOTProtocol::objectComplete,
 *           this, &MyClass::handleMOTObject);
 *
 *   // Process MSC data
 *   mot_parser.processMOTData(service_id, msc_data);
 *
 *   // Get active objects
 *   auto active = mot_parser.getActiveTransportIds();
 * @endcode
 */
class MOTProtocol : public QObject {
    Q_OBJECT

public:
    explicit MOTProtocol(QObject* parent = nullptr);
    ~MOTProtocol() override;

    // ========================================================================
    // Main Processing Interface
    // ========================================================================

    /**
     * @brief Process MOT data from MSC (Main Service Channel)
     * @param service_id DAB service ID carrying MOT
     * @param msc_data Raw MSC data containing MOT segments
     * @return true if processing successful, false on error
     *
     * Extracts MSC data groups and processes MOT segments.
     * Emits objectComplete signal when reassembly finishes.
     */
    bool processMOTData(uint32_t service_id, const QByteArray& msc_data);

    // ========================================================================
    // MSC Data Group Parsing (ETSI EN 300 401 Section 5.3.3)
    // ========================================================================

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
     * ETSI EN 300 401: CRC-CCITT validation
     */
    bool validateDataGroupCRC(const MSCDataGroup& data_group);

    // ========================================================================
    // MOT Header/Directory Parsing (ETSI EN 301 234 Section 5)
    // ========================================================================

    /**
     * @brief Parse MOT header from data
     * @param data Raw MOT header data
     * @param header Output: parsed header structure
     * @return true if parsing successful
     *
     * ETSI EN 301 234 Section 5.1: MOT Header Structure
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
     * ETSI EN 301 234 Section 5.2: MOT Directory
     */
    bool parseMOTDirectory(const std::vector<uint8_t>& data, MOTDirectory& directory);

    // ========================================================================
    // Segment Reassembly (ETSI EN 301 234 Section 6)
    // ========================================================================

    /**
     * @brief Parse MOT segment header
     * @param data_group MSC data group containing segment
     * @return Parsed segment header or nullopt on error
     */
    std::optional<MOTSegmentHeader> parseMOTSegmentHeader(const MSCDataGroup& data_group);

    /**
     * @brief Add segment to reassembly buffer
     * @param transport_id MOT transport ID
     * @param segment_number Current segment number
     * @param total_segments Total segment count
     * @param segment_data Segment payload data
     * @return Reassembly status
     *
     * Manages segment buffering and duplicate detection.
     */
    enum class ReassemblyStatus {
        INCOMPLETE,              ///< More segments needed
        COMPLETE,                ///< Object fully reassembled
        ERROR_CRC,               ///< CRC validation failed
        ERROR_MISSING_SEGMENTS,  ///< Missing segments detected
        ERROR_DUPLICATE          ///< Duplicate segment received
    };
    ReassemblyStatus addSegment(uint32_t transport_id,
                                uint16_t segment_number,
                                uint16_t total_segments,
                                const std::vector<uint8_t>& segment_data);

    /**
     * @brief Reassemble complete MOT object from segments
     * @param transport_id Transport ID
     * @return Complete reassembled object or nullopt if incomplete
     *
     * Combines all segments and parses header/body.
     */
    std::optional<MOTObject> reassembleObject(uint32_t transport_id);

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
     * the 14000s). The earlier 0-1023 limit was too small and rejected every
     * real object.
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
     * @return true if all segments received
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

    // ========================================================================
    // Statistics & Status
    // ========================================================================

    /**
     * @brief Get MOT processing statistics
     */
    struct Statistics {
        uint64_t data_groups_processed = 0;
        uint64_t data_groups_valid = 0;
        uint64_t segments_received = 0;
        uint64_t objects_completed = 0;
        uint64_t crc_errors = 0;
        uint32_t active_objects = 0;
    };
    Statistics getStatistics() const;

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
    // CRC Calculation
    // ========================================================================

    /**
     * @brief Calculate CRC-CCITT for data group validation
     * @param data Input data
     * @return 16-bit CRC
     *
     * ETSI EN 300 401: CRC-CCITT polynomial
     */
    uint16_t calculateCRC_CCITT(const std::vector<uint8_t>& data);

    /**
     * @brief Initialize CRC lookup table
     */
    void initializeCRCTable();

    // ========================================================================
    // Stale Object Cleanup
    // ========================================================================

    /**
     * @brief Clean up stale objects (no updates for 60 seconds)
     */
    void cleanupStaleObjects();

    /**
     * @brief Check if object is stale
     * @param last_update_ms Last update timestamp
     * @return true if stale (>60 seconds old)
     */
    bool isStale(uint32_t last_update_ms) const;

    // ========================================================================
    // Helper Methods
    // ========================================================================

    /**
     * @brief Extract UTF-8 string from parameter data
     */
    QString extractUTF8String(const std::vector<uint8_t>& data);

    /**
     * @brief Extract trigger time from parameter (MJD + UTC)
     */
    uint32_t extractTriggerTime(const std::vector<uint8_t>& data);

    /**
     * @brief Get current timestamp in milliseconds
     */
    uint32_t getCurrentTimeMs() const;

    // ========================================================================
    // Private Data Members
    // ========================================================================

    // Segment reassembly buffers
    std::map<uint32_t, SegmentBuffer> m_segment_buffers; // transport_id → buffer

    // CRC lookup table
    std::array<uint16_t, 256> m_crc_table;

    // Statistics
    Statistics m_statistics;

    // Mutex for thread-safe access. Recursive because processMOTData() already
    // holds the lock while calling getProgress()/clearObject()/cleanupStale
    // Objects() (which lock again); a plain QMutex deadlocked the whole
    // pipeline as soon as a complete object was reassembled.
    mutable QRecursiveMutex m_mutex;

    // Stale object timeout (milliseconds)
    static constexpr uint32_t STALE_TIMEOUT_MS = 60000; // 60 seconds
};

} // namespace eti::mot

// Qt metatype registration for signals/slots
Q_DECLARE_METATYPE(eti::mot::MOTHeader)
Q_DECLARE_METATYPE(eti::mot::MOTObject)
Q_DECLARE_METATYPE(eti::mot::MSCDataGroup)
Q_DECLARE_METATYPE(eti::mot::MOTDirectory)
