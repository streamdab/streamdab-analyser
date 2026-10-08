/**
 * @file journaline_decoder.hpp
 * @brief Journaline Text Service Decoder
 *
 * Implements Journaline text service decoding per ETSI TS 102 979.
 * Journaline provides structured text information (news, weather, sports)
 * with hierarchical menu navigation for DAB/DAB+ services.
 *
 * Key Features:
 * - Journaline object parsing (ETSI TS 102 979 Section 5)
 * - Menu hierarchy construction (Section 6)
 * - Text content extraction (UTF-8, Thai support)
 * - Category classification (news, sport, weather, etc.)
 * - Link navigation and resolution
 * - Object carousel tracking with versioning
 * - Timestamp parsing (MJD + UTC)
 * - Thread-safe operation with QMutex
 * - Qt signal/slot integration for GUI updates
 *
 * Journaline Protocol Flow:
 * 1. Receive MOT object containing Journaline data
 * 2. Parse Journaline object header (ID, category, timestamp)
 * 3. Extract text content (UTF-8 decoding)
 * 4. Build menu hierarchy from object relationships
 * 5. Resolve links between objects
 * 6. Emit signals for GUI updates
 *
 * Journaline Object Structure (ETSI TS 102 979 Section 5):
 * - Object ID (16 bits): Unique identifier
 * - Category (8 bits): Content category (news, sport, weather)
 * - Timestamp (40 bits): MJD + UTC time
 * - Title length + Title text (UTF-8)
 * - Content length + Content text (UTF-8)
 * - Link flag + Link target (optional)
 *
 * Thread-safe with Qt signal/slot integration for real-time updates.
 *
 * @see ETSI TS 102 979 - Journaline Specification
 * @see ETSI EN 301 234 - MOT Protocol (carrier for Journaline)
 * @see ETSI EN 300 401 - DAB System Specification
 *
 * @author StreamDAB Development Team
 * @date October 22, 2025
 * @version 1.0
 * @copyright Copyright (c) 2025 StreamDAB Technologies
 */

#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QVector>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <cstdint>
#include <optional>

namespace eti::journaline {

/**
 * @brief Journaline Object Structure
 *
 * Represents a single Journaline object (text item) within the service.
 * ETSI TS 102 979 Section 5: Journaline Object Format
 */
struct JournalineObject {
    uint16_t object_id = 0;              ///< Unique object identifier (16 bits)
    QString text_content;                ///< Main text content (UTF-8, Thai support)
    QString title;                       ///< Object title/headline (UTF-8)
    QDateTime timestamp;                 ///< Publication timestamp (MJD + UTC)
    uint8_t category = 0;                ///< Category code (news, sport, weather, etc.)
    bool is_link = false;                ///< Link flag (true if object contains link)
    uint16_t link_target = 0;            ///< Link target object ID (if is_link=true)

    /**
     * @brief Check if object is valid
     * @return true if object has valid ID and non-empty content
     */
    bool isValid() const {
        return object_id > 0 && (!text_content.isEmpty() || !title.isEmpty());
    }

    /**
     * @brief Get category as human-readable string
     * @return Category string (e.g., "News", "Sport", "Weather")
     */
    QString getCategoryString() const;

    /**
     * @brief Convert to debug string
     * @return String representation for logging
     */
    QString toString() const {
        return QString("JournalineObject[ID=%1, category=%2, title='%3', link=%4]")
            .arg(object_id)
            .arg(getCategoryString())
            .arg(title)
            .arg(is_link ? QString("→%1").arg(link_target) : "no");
    }
};

/**
 * @brief Journaline Menu Structure
 *
 * Represents a hierarchical menu containing Journaline objects.
 * ETSI TS 102 979 Section 6: Menu Hierarchy
 */
struct JournalineMenu {
    QString menu_title;                  ///< Menu title (UTF-8)
    QVector<JournalineObject> items;     ///< Menu items (objects)
    uint16_t menu_id = 0;                ///< Menu identifier
    uint16_t parent_menu_id = 0;         ///< Parent menu ID (0 = root menu)

    /**
     * @brief Check if menu is valid
     * @return true if menu has valid ID
     */
    bool isValid() const {
        return menu_id > 0;
    }

    /**
     * @brief Check if this is a root menu
     * @return true if parent_menu_id is 0
     */
    bool isRootMenu() const {
        return parent_menu_id == 0;
    }

    /**
     * @brief Get number of items in menu
     * @return Item count
     */
    int itemCount() const {
        return items.size();
    }

    /**
     * @brief Convert to debug string
     * @return String representation for logging
     */
    QString toString() const {
        return QString("JournalineMenu[ID=%1, parent=%2, title='%3', items=%4]")
            .arg(menu_id)
            .arg(parent_menu_id)
            .arg(menu_title)
            .arg(items.size());
    }
};

/**
 * @brief Journaline Category Enumeration
 *
 * ETSI TS 102 979 Section 5.3: Category Classification
 */
enum class JournalineCategory : uint8_t {
    UNDEFINED = 0x00,          ///< Undefined category
    NEWS_GENERAL = 0x01,       ///< General news
    NEWS_REGIONAL = 0x02,      ///< Regional news
    NEWS_INTERNATIONAL = 0x03, ///< International news
    SPORT_GENERAL = 0x10,      ///< General sport
    SPORT_FOOTBALL = 0x11,     ///< Football/Soccer
    SPORT_OTHER = 0x12,        ///< Other sports
    WEATHER_GENERAL = 0x20,    ///< General weather
    WEATHER_FORECAST = 0x21,   ///< Weather forecast
    WEATHER_WARNING = 0x22,    ///< Weather warnings
    TRAFFIC = 0x30,            ///< Traffic information
    FINANCIAL = 0x40,          ///< Financial news
    ENTERTAINMENT = 0x50,      ///< Entertainment
    POLITICS = 0x60,           ///< Politics
    CULTURE = 0x70,            ///< Culture/Arts
    TECHNOLOGY = 0x80,         ///< Technology/Science
    HEALTH = 0x90,             ///< Health
    UNKNOWN = 0xFF             ///< Unknown category
};

/**
 * @brief Convert category to human-readable string
 * @param category Category code
 * @return Category string
 */
QString categoryToString(JournalineCategory category);

/**
 * @brief Journaline Decoder Class
 *
 * Professional Journaline decoder implementation with complete ETSI TS 102 979 support.
 *
 * Features:
 * - MOT object Journaline data parsing
 * - Object header extraction (ID, category, timestamp)
 * - Text content extraction (UTF-8, Thai support)
 * - Menu hierarchy construction and navigation
 * - Link resolution between objects
 * - Timestamp conversion (MJD + UTC to QDateTime)
 * - Category classification
 * - Thread-safe operation with QMutex
 * - Qt signal emission for real-time GUI updates
 * - Object versioning and duplicate detection
 *
 * Usage:
 * @code
 *   JournalineDecoder decoder;
 *
 *   // Connect signals
 *   connect(&decoder, &JournalineDecoder::journalineObjectReceived,
 *           this, &MyClass::handleJournalineObject);
 *   connect(&decoder, &JournalineDecoder::journalineMenuUpdated,
 *           this, &MyClass::refreshMenuDisplay);
 *
 *   // Process MOT object containing Journaline
 *   if (decoder.processMOTObject(mot_object.body)) {
 *       auto objects = decoder.getObjects();
 *       auto menus = decoder.getMenuIds();
 *   }
 * @endcode
 */
class JournalineDecoder : public QObject {
    Q_OBJECT
    Q_PROPERTY(int objectCount READ objectCount NOTIFY objectCountChanged)
    Q_PROPERTY(int menuCount READ menuCount NOTIFY menuCountChanged)

public:
    explicit JournalineDecoder(QObject* parent = nullptr);
    ~JournalineDecoder() override;

    // ========================================================================
    // Main Processing Interface
    // ========================================================================

    /**
     * @brief Process MOT object containing Journaline data
     * @param mot_data MOT object body containing Journaline content
     * @return true if processing successful, false on error
     *
     * Extracts Journaline object from MOT data and emits signals
     * for object discovery and menu updates.
     */
    bool processMOTObject(const QByteArray& mot_data);

    /**
     * @brief Process raw data-subchannel stream bytes (stream-mode Journaline).
     * @param stream_chunk Raw bytes of the data subchannel (concatenated
     *                     per-frame subchannel slices).
     * @return true when THIS call changed the extracted item set (item count
     *         or candidate count differ from the previous scan). A repeat of
     *         an unchanged buffer therefore returns false; feed the whole
     *         stream and check streamItemCount() for "did we decode anything".
     *
     * Capture-backed stream-mode path: the Hessischen Rundfunk Warntag 2024
     * capture (hr_20240912T105801_EWS_Start.eti, SCId 8) carries a Journaline
     * display-text carousel directly in the data subchannel (not wrapped in
     * MOT). This method extracts the human-visible title/content runs and
     * stores them as JournalineObject entries. It does NOT alter the MOT path
     * or the existing processMOTObject() contract.
     *
     * Bounds (never unbounded on a live stream):
     *  - the raw buffer is capped at kStreamBufferCap bytes (drop-oldest,
     *    one-shot qWarning);
     *  - the extracted inventory is capped at kMaxStreamItems entries;
     *  - work per call is O(new bytes) — only the tail that has not been
     *    scanned yet (plus the run still in progress) is re-walked.
     *
     * Plausibility gate (A-H1): a candidate run is only accepted when it
     * decodes as UTF-8 (≤1 U+FFFD), is mostly printable text (German
     * umlauts and Thai pass, binary does not) and contains a ≥4-letter
     * word — so binary traffic no longer floods the inventory with junk.
     *
     * Coverage note: this is a display-text extraction (base coverage) — the
     * full TS 102 979 Object-Element bit parsing (LevelId/Object headers,
     * category links) is deferred; see docs/EPG_JOURNALINE_TPEG_VALIDATION.md.
     */
    bool processStreamData(const QByteArray& stream_chunk);

    /**
     * @brief Number of display-text items that PASSED the plausibility gate.
     */
    int streamItemCount() const;

    /**
     * @brief Number of ≥6-character candidate runs seen in the LAST scan
     *        (before the plausibility gate). streamItemCount() /
     *        streamCandidateCount() is the clean ratio the data-service
     *        router uses to tell a real carousel from binary traffic.
     */
    int streamCandidateCount() const;

    /**
     * @brief Items extracted from stream data (titles/texts).
     */
    QVector<JournalineObject> getStreamItems() const;

    // ========================================================================
    // Stream-mode bounds (documented in the class contract, see
    // processStreamData()). Public so tests / the calibration probe can
    // reference the same numbers.
    // ========================================================================

    /// Raw stream buffer cap (drop-oldest, one-shot qWarning on first trim).
    static constexpr qsizetype kStreamBufferCap = 4 * 1024 * 1024;
    /// Extracted inventory cap (bounded even for a pathological stream).
    static constexpr int kMaxStreamItems = 4096;
    /// Minimum raw run length (bytes) considered a text candidate.
    static constexpr int kStreamMinRunBytes = 6;
    /// Stream items use an object-id base disjoint from the MOT id space so a
    /// stream item can never collide with a MOT-decoded object id.
    static constexpr uint16_t kStreamObjectIdBase = 0x8000;

    // ========================================================================
    // Object Retrieval
    // ========================================================================

    /**
     * @brief Get all Journaline objects
     * @return Vector of all objects
     */
    QVector<JournalineObject> getObjects() const;

    /**
     * @brief Get specific Journaline object by ID
     * @param object_id Object identifier
     * @return Object or nullopt if not found
     */
    std::optional<JournalineObject> getObject(uint16_t object_id) const;

    /**
     * @brief Get objects by category
     * @param category Category filter
     * @return Vector of objects matching category
     */
    QVector<JournalineObject> getObjectsByCategory(uint8_t category) const;

    /**
     * @brief Resolve link target (get linked object)
     * @param source_object Object containing link
     * @return Linked object or nullopt if not found or no link
     */
    std::optional<JournalineObject> resolveLinkTarget(const JournalineObject& source_object) const;

    // ========================================================================
    // Menu Retrieval
    // ========================================================================

    /**
     * @brief Get specific menu by ID
     * @param menu_id Menu identifier
     * @return Menu or nullopt if not found
     */
    std::optional<JournalineMenu> getMenu(uint16_t menu_id) const;

    /**
     * @brief Get list of all menu IDs
     * @return Vector of menu IDs
     */
    QVector<uint16_t> getMenuIds() const;

    /**
     * @brief Get root menus (parent_id = 0)
     * @return Vector of root menus
     */
    QVector<JournalineMenu> getRootMenus() const;

    /**
     * @brief Get submenus of a specific menu
     * @param parent_menu_id Parent menu ID
     * @return Vector of submenus
     */
    QVector<JournalineMenu> getSubMenus(uint16_t parent_menu_id) const;

    // ========================================================================
    // Statistics
    // ========================================================================

    /**
     * @brief Get total number of objects
     * @return Object count
     */
    int objectCount() const;

    /**
     * @brief Get total number of menus
     * @return Menu count
     */
    int menuCount() const;

    /**
     * @brief Get Journaline processing statistics
     */
    struct Statistics {
        uint32_t objects_processed = 0;   ///< Total objects processed
        uint32_t menus_processed = 0;     ///< Total menus constructed
        uint32_t parsing_errors = 0;      ///< Parsing error count
        uint32_t links_resolved = 0;      ///< Successfully resolved links
        uint32_t thai_content_count = 0;  ///< Objects with Thai content
    };
    Statistics getStatistics() const;

    /**
     * @brief Clear all Journaline data
     */
    void clearAll();

    /**
     * @brief Clear specific object
     * @param object_id Object ID to clear
     */
    void clearObject(uint16_t object_id);

    /**
     * @brief Clear specific menu
     * @param menu_id Menu ID to clear
     */
    void clearMenu(uint16_t menu_id);

signals:
    /**
     * @brief Emitted when Journaline object is received
     * @param object_id Object identifier
     * @param object Journaline object
     */
    void journalineObjectReceived(uint16_t object_id, const JournalineObject& object);

    /**
     * @brief Emitted when menu is updated
     * @param menu_id Menu identifier
     */
    void journalineMenuUpdated(uint16_t menu_id);

    /**
     * @brief Emitted on Journaline decoding error
     * @param error Error message
     */
    void journalineDecodingError(const QString& error);

    /**
     * @brief Emitted when object count changes
     */
    void objectCountChanged();

    /**
     * @brief Emitted when menu count changes
     */
    void menuCountChanged();

private:
    // ========================================================================
    // ETSI TS 102 979 Parsing Methods
    // ========================================================================

    /**
     * @brief Scan the not-yet-scanned tail of m_streamBuffer and (re)build the
     *        stream-mode display-text inventory (capture-backed, base
     *        coverage). Caller must hold m_mutex.
     *
     * Invariant: already-committed bytes are never re-walked. A run still in
     * progress when the previous call ended is re-walked from its first byte
     * (and re-committed only once it is finally closed), so a run split
     * across two fed chunks merges exactly like a single-shot feed; a run
     * that was closed by a control byte inside the previous chunk is
     * committed, so the scan resumes at the buffer end instead of walking it
     * again.
     */
    void scanStreamTailLocked();

    /**
     * @brief Parse Journaline object from raw data (internal, no signal emission)
     * @param data Raw Journaline data from MOT object
     * @param out_object_id Output: parsed object ID
     * @param out_object Output: parsed object data
     * @return true if parsing successful
     *
     * BUG-001-CRITICAL FIX: Internal method that does NOT emit signals
     * Signals are emitted by caller after mutex release to prevent deadlock
     *
     * ETSI TS 102 979 Section 5: Journaline Object Structure
     */
    bool parseJournalineObjectInternal(const QByteArray& data,
                                       uint16_t& out_object_id,
                                       JournalineObject& out_object);

    /**
     * @brief Parse object header
     * @param data Raw data
     * @param offset Current offset in data
     * @param object Output: object to populate
     * @return New offset after parsing header, or -1 on error
     */
    int parseObjectHeader(const QByteArray& data, int offset, JournalineObject& object);

    /**
     * @brief Parse category field
     * @param category_byte Category byte from object
     * @return Category code
     *
     * ETSI TS 102 979 Section 5.3: Category Encoding
     */
    uint8_t parseCategory(uint8_t category_byte) const;

    /**
     * @brief Get category enumeration from byte
     * @param category_byte Category byte
     * @return Category enum
     */
    JournalineCategory getCategoryEnum(uint8_t category_byte) const;

    // ========================================================================
    // Text Extraction
    // ========================================================================

    /**
     * @brief Extract UTF-8 text from data
     * @param data Raw data containing UTF-8 text
     * @param offset Start offset in data
     * @param length Text length in bytes
     * @return Extracted QString (supports Thai characters)
     */
    QString extractText(const QByteArray& data, int offset, int length) const;

    /**
     * @brief Extract null-terminated UTF-8 string
     * @param data Raw data
     * @param offset Start offset
     * @return Extracted QString
     */
    QString extractNullTerminatedString(const QByteArray& data, int offset) const;

    /**
     * @brief Detect if text contains Thai characters
     * @param text Text to check
     * @return true if Thai characters detected
     */
    bool containsThaiText(const QString& text) const;

    // ========================================================================
    // Timestamp Parsing (MJD + UTC)
    // ========================================================================

    /**
     * @brief Parse Journaline timestamp to QDateTime
     * @param timestamp 40-bit timestamp (MJD 16-bit + UTC 24-bit)
     * @return QDateTime in UTC timezone
     *
     * ETSI TS 102 979 Section 5.2: Timestamp Format
     * - MJD (Modified Julian Date): 16 bits
     * - UTC time (BCD HH:MM:SS): 24 bits
     */
    QDateTime parseTimestamp(uint32_t mjd, uint32_t utc_time) const;

    /**
     * @brief Convert MJD and UTC time to QDateTime
     * @param mjd Modified Julian Date (16-bit)
     * @param utc_time UTC time in BCD format (24-bit: HH:MM:SS)
     * @return QDateTime in UTC timezone
     */
    QDateTime convertMJDtoQDateTime(uint16_t mjd, uint32_t utc_time) const;

    /**
     * @brief Decode BCD (Binary-Coded Decimal) time
     * @param bcd_time BCD-encoded time (0xHHMMSS)
     * @return Tuple of (hours, minutes, seconds)
     */
    std::tuple<int, int, int> decodeBCDTime(uint32_t bcd_time) const;

    /**
     * @brief Decode single BCD byte
     * @param bcd BCD byte (e.g., 0x23 -> 23)
     * @return Decimal value
     */
    int decodeBCD(uint8_t bcd) const;

    // ========================================================================
    // Menu Construction
    // ========================================================================

    /**
     * @brief Build menu hierarchy from objects (internal, no signal emission)
     * @param out_updated_menu_ids Output: vector of updated menu IDs
     *
     * BUG-001-CRITICAL FIX: Internal method that does NOT emit signals
     * Signals are emitted by caller after mutex release to prevent deadlock
     *
     * Constructs menu structure by analyzing object relationships
     * and link patterns.
     */
    void buildMenuHierarchyInternal(QVector<uint16_t>& out_updated_menu_ids);

    /**
     * @brief Add object to appropriate menu
     * @param object Object to add
     */
    void addObjectToMenu(const JournalineObject& object);

    /**
     * @brief Create menu from objects
     * @param menu_id Menu identifier
     * @param parent_id Parent menu ID
     * @param title Menu title
     * @param objects Objects to include
     */
    void createMenu(uint16_t menu_id, uint16_t parent_id,
                   const QString& title, const QVector<JournalineObject>& objects);

    // ========================================================================
    // Validation
    // ========================================================================

    /**
     * @brief Validate Journaline object
     * @param object Object to validate
     * @return true if valid
     */
    bool validateObject(const JournalineObject& object) const;

    /**
     * @brief Validate object ID
     * @param object_id Object ID to validate
     * @return true if valid (non-zero)
     */
    bool validateObjectId(uint16_t object_id) const {
        return object_id > 0;
    }

    /**
     * @brief Validate MJD value
     * @param mjd Modified Julian Date
     * @return true if valid (reasonable date range)
     */
    bool validateMJD(uint16_t mjd) const;

    // ========================================================================
    // Helper Methods
    // ========================================================================

    /**
     * @brief Update statistics counters
     */
    void updateStatistics();

    /**
     * @brief Extract 16-bit big-endian value
     * @param data Data buffer
     * @param offset Offset in buffer
     * @return Extracted value or std::nullopt if bounds check fails
     *
     * BUG-003-HIGH FIX: Return std::optional for explicit error handling
     * Silent failures (returning 0) are indistinguishable from valid data
     */
    std::optional<uint16_t> extractUint16BE(const QByteArray& data, int offset) const;

    /**
     * @brief Extract 32-bit big-endian value
     * @param data Data buffer
     * @param offset Offset in buffer
     * @return Extracted value or std::nullopt if bounds check fails
     *
     * BUG-003-HIGH FIX: Return std::optional for explicit error handling
     * Silent failures (returning 0) are indistinguishable from valid data
     */
    std::optional<uint32_t> extractUint32BE(const QByteArray& data, int offset) const;

    // ========================================================================
    // Private Data Members
    // ========================================================================

    // Journaline objects indexed by object ID
    QMap<uint16_t, JournalineObject> m_objects;

    // Journaline menus indexed by menu ID
    QMap<uint16_t, JournalineMenu> m_menus;

    // Stream-mode display-text items (capture-backed carousel extraction).
    QVector<JournalineObject> m_streamItems;
    QByteArray m_streamBuffer;
    bool m_streamScanned = false;          ///< first scan has run (rescan gate)
    int m_streamItemGeneration = 0;        ///< == m_streamItems.size() after a scan
    uint16_t m_streamMenuId = 0x7FFF;
    /// ≥6-byte candidate runs seen in the last scan (pre-gate).
    int m_streamCandidateCount = 0;
    /// Tail-scan bookkeeping: bytes of m_streamBuffer already committed.
    qsizetype m_streamScanOffset = 0;
    /// Scan state as of the end of the previous scan: everything committed up
    /// to that point (item count / candidates / Thai count / pending title /
    /// next object id), taken BEFORE the trailing run — if the buffer ended
    /// mid-run — is committed, so restoring it and re-walking that run from
    /// m_streamScanOffset merges the split run exactly once.
    struct StreamScanSnapshot {
        bool valid = false;
        qsizetype items = 0;                ///< m_streamItems.size() to restore
        int candidates = 0;
        uint32_t thai = 0;
        QString pendingTitle;
        uint16_t nextObjectId = kStreamObjectIdBase;
    };
    StreamScanSnapshot m_streamSnapshot;
    bool m_streamBufferTrimWarned = false;  ///< one-shot drop-oldest warning
    /// Thai-content counters split by source so a stream rescan can recompute
    /// its own contribution without wiping the MOT path's (A-M3).
    uint32_t m_motThaiCount = 0;
    uint32_t m_streamThaiCount = 0;

    // Statistics
    Statistics m_statistics;

    // Mutex for thread-safe access
    mutable QMutex m_mutex;

    // MJD epoch reference (November 17, 1858)
    static constexpr int MJD_EPOCH_OFFSET = 2400001; // Julian Date offset

    // Valid MJD range (1980-2089 approximately)
    static constexpr uint16_t MIN_VALID_MJD = 44239; // Jan 1, 1980
    static constexpr uint16_t MAX_VALID_MJD = 65500; // ~May 2089 (safety margin)
};

} // namespace eti::journaline

// Qt metatype registration for signals/slots
Q_DECLARE_METATYPE(eti::journaline::JournalineObject)
Q_DECLARE_METATYPE(eti::journaline::JournalineMenu)
Q_DECLARE_METATYPE(eti::journaline::JournalineCategory)
