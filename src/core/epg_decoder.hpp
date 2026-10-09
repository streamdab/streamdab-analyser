/**
 * @file epg_decoder.hpp
 * @brief EPG (Electronic Program Guide) Decoder
 *
 * Implements EPG data extraction per ETSI TS 102 371 for DAB/DAB+ services.
 * Supports schedule information, program metadata, and Thai UTF-8 content.
 *
 * Key Features:
 * - EPG schedule parsing (ETSI TS 102 371 Section 4)
 * - MJD (Modified Julian Date) to QDateTime conversion
 * - Event timing extraction (start/end times)
 * - Program metadata (name, description, genre)
 * - Content type classification (news, sports, music, etc.)
 * - Thai UTF-8 text support
 * - Qt signal/slot integration for real-time updates
 * - Thread-safe operation with QMutex
 *
 * EPG Data Flow:
 * 1. Receive MOT object containing EPG data
 * 2. Parse EPG schedule structure
 * 3. Extract events with timing and metadata
 * 4. Convert MJD/UTC to QDateTime
 * 5. Emit signals for GUI updates
 *
 * @see ETSI TS 102 371 - Electronic Program Guide (EPG)
 * @see ETSI EN 301 234 - MOT Protocol (carrier for EPG)
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

namespace eti::epg {

/**
 * @brief EPG Content Type Enumeration
 *
 * ETSI TS 102 371 Section 4.3: Content Type Classification
 */
enum class ContentType : uint8_t {
    UNDEFINED = 0x00,           ///< Undefined content
    NEWS = 0x10,                ///< News programs
    CURRENT_AFFAIRS = 0x11,     ///< Current affairs
    INFORMATION = 0x12,         ///< Information programs
    SPORT = 0x20,               ///< Sports events
    EDUCATION = 0x30,           ///< Educational programs
    DRAMA = 0x40,               ///< Drama/series
    MUSIC = 0x50,               ///< Music programs
    ARTS_CULTURE = 0x60,        ///< Arts and culture
    SOCIAL = 0x70,              ///< Social/political issues
    SCIENCE = 0x80,             ///< Science/technology
    LEISURE = 0x90,             ///< Leisure hobbies
    CHILDRENS = 0xA0,           ///< Children's programs
    UNKNOWN = 0xFF              ///< Unknown type
};

/**
 * @brief Convert content type to human-readable string
 * @param type Content type
 * @return String representation
 */
QString contentTypeToString(ContentType type);

/**
 * @brief EPG Event Structure
 *
 * Represents a single program event in the EPG schedule.
 * Contains timing, metadata, and content classification.
 */
struct EPGEvent {
    uint32_t event_id = 0;          ///< Unique event identifier
    QDateTime start_time;           ///< Event start time (UTC)
    QDateTime end_time;             ///< Event end time (UTC)
    QString program_name;           ///< Program name (UTF-8, Thai support)
    QString description;            ///< Program description (UTF-8)
    QString genre;                  ///< Genre/category string
    ContentType content_type = ContentType::UNDEFINED;  ///< Content type
    uint8_t parental_rating = 0;    ///< Parental rating (0-18+)
    bool is_live = false;           ///< Live broadcast flag
    bool has_subtitles = false;     ///< Subtitle availability
    bool has_audio_description = false; ///< Audio description flag

    /**
     * @brief Get event duration in minutes
     */
    int durationMinutes() const {
        if (!start_time.isValid() || !end_time.isValid()) {
            return 0;
        }
        return start_time.secsTo(end_time) / 60;
    }

    /**
     * @brief Check if event is currently active
     */
    bool isActive() const {
        QDateTime now = QDateTime::currentDateTime();
        return start_time <= now && now < end_time;
    }

    /**
     * @brief Check if event is valid
     */
    bool isValid() const {
        return event_id > 0 &&
               start_time.isValid() &&
               end_time.isValid() &&
               start_time < end_time &&
               !program_name.isEmpty();
    }

    /**
     * @brief Convert to debug string
     */
    QString toString() const;
};

/**
 * @brief EPG Schedule Structure
 *
 * Contains all events for a specific DAB service on a given date.
 */
struct EPGSchedule {
    uint32_t service_id = 0;        ///< DAB service ID (SId)
    QDateTime schedule_date;        ///< Schedule date (typically midnight UTC)
    QVector<EPGEvent> events;       ///< List of events for this service/date
    QDateTime last_updated;         ///< Last update timestamp
    uint16_t schedule_version = 0;  ///< Version number for change detection

    /**
     * @brief Get total number of events
     */
    int eventCount() const {
        return events.size();
    }

    /**
     * @brief Get currently active event (if any)
     */
    std::optional<EPGEvent> getCurrentEvent() const {
        QDateTime now = QDateTime::currentDateTime();
        for (const auto& event : events) {
            if (event.start_time <= now && now < event.end_time) {
                return event;
            }
        }
        return std::nullopt;
    }

    /**
     * @brief Get next upcoming event (if any)
     */
    std::optional<EPGEvent> getNextEvent() const {
        QDateTime now = QDateTime::currentDateTime();
        for (const auto& event : events) {
            if (event.start_time > now) {
                return event;
            }
        }
        return std::nullopt;
    }

    /**
     * @brief Check if schedule is valid
     */
    bool isValid() const {
        return service_id > 0 &&
               schedule_date.isValid() &&
               !events.isEmpty();
    }

    QString toString() const;
};

/**
 * @brief EPG Decoder Class
 *
 * Professional EPG decoder implementation with complete ETSI TS 102 371 support.
 *
 * Features:
 * - MOT object EPG data parsing
 * - MJD (Modified Julian Date) to QDateTime conversion
 * - Event timing extraction (start/end times)
 * - Program metadata extraction (UTF-8, Thai support)
 * - Content type classification
 * - Schedule versioning for change detection
 * - Thread-safe operation
 * - Qt signal emission for real-time GUI updates
 *
 * Usage:
 * @code
 *   EPGDecoder epg;
 *
 *   // Connect signals
 *   connect(&epg, &EPGDecoder::epgEventDiscovered,
 *           this, &MyClass::handleEPGEvent);
 *   connect(&epg, &EPGDecoder::epgScheduleUpdated,
 *           this, &MyClass::refreshScheduleDisplay);
 *
 *   // Process MOT object containing EPG
 *   if (epg.processMOTObject(mot_object.body)) {
 *       auto events = epg.getEvents(service_id);
 *   }
 * @endcode
 */
class EPGDecoder : public QObject {
    Q_OBJECT
    Q_PROPERTY(int eventCount READ eventCount NOTIFY eventCountChanged)
    Q_PROPERTY(int serviceCount READ serviceCount NOTIFY serviceCountChanged)

public:
    explicit EPGDecoder(QObject* parent = nullptr);
    ~EPGDecoder() override;

    // ========================================================================
    // Main Processing Interface
    // ========================================================================

    /**
     * @brief Process MOT object containing EPG data
     * @param mot_data MOT object body containing EPG schedule
     * @return true if processing successful, false on error
     *
     * Extracts EPG schedule from MOT object and emits signals
     * for each discovered event.
     */
    bool processMOTObject(const QByteArray& mot_data);

    /**
     * @brief Parse EPG schedule structure
     * @param epg_data Raw EPG data from MOT object
     * @return true if parsing successful, false on error
     *
     * ETSI TS 102 371 Section 4: EPG Structure
     */
    bool parseEPGSchedule(const QByteArray& epg_data);

    // ========================================================================
    // Event Retrieval
    // ========================================================================

    /**
     * @brief Get all events for a specific service
     * @param service_id DAB service ID
     * @return Vector of events (empty if none found)
     */
    QVector<EPGEvent> getEvents(uint32_t service_id) const;

    /**
     * @brief Get currently active event for a service
     * @param service_id DAB service ID
     * @return Current event or nullopt if none active
     */
    std::optional<EPGEvent> getCurrentEvent(uint32_t service_id) const;

    /**
     * @brief Get next upcoming event for a service
     * @param service_id DAB service ID
     * @return Next event or nullopt if none found
     */
    std::optional<EPGEvent> getNextEvent(uint32_t service_id) const;

    /**
     * @brief Get complete schedule for a service
     * @param service_id DAB service ID
     * @return Schedule structure or nullopt if not found
     */
    std::optional<EPGSchedule> getSchedule(uint32_t service_id) const;

    /**
     * @brief Get list of all services with EPG data
     * @return Vector of service IDs
     */
    QVector<uint32_t> getServiceIds() const;

    // ========================================================================
    // Statistics
    // ========================================================================

    /**
     * @brief Get total number of events across all services
     */
    int eventCount() const;

    /**
     * @brief Get number of services with EPG data
     */
    int serviceCount() const;

    /**
     * @brief Get EPG processing statistics
     */
    struct Statistics {
        uint32_t schedules_processed = 0;
        uint32_t events_extracted = 0;
        uint32_t parsing_errors = 0;
        uint32_t active_services = 0;
    };
    Statistics getStatistics() const;

    /**
     * @brief Clear all EPG data
     */
    void clearAll();

    /**
     * @brief Clear EPG data for a specific service
     * @param service_id Service ID to clear
     */
    void clearService(uint32_t service_id);

signals:
    /**
     * @brief Emitted when EPG event is discovered
     * @param service_id DAB service ID
     * @param event Discovered EPG event
     */
    void epgEventDiscovered(uint32_t service_id, const EPGEvent& event);

    /**
     * @brief Emitted when EPG schedule is updated
     * @param service_id DAB service ID
     */
    void epgScheduleUpdated(uint32_t service_id);

    /**
     * @brief Emitted on EPG decoding error
     * @param error Error message
     */
    void epgDecodingError(const QString& error);

    /**
     * @brief Emitted when event count changes
     */
    void eventCountChanged();

    /**
     * @brief Emitted when service count changes
     */
    void serviceCountChanged();

private:
    // ========================================================================
    // MJD (Modified Julian Date) Conversion
    // ========================================================================

    /**
     * @brief Convert MJD and UTC time to QDateTime
     * @param mjd Modified Julian Date (16-bit)
     * @param utc_time UTC time in BCD format (24-bit: HH:MM:SS)
     * @return QDateTime in UTC timezone
     *
     * ETSI TS 102 371 Section 4.1: Time Representation
     *
     * MJD Conversion Algorithm:
     * - MJD 0 = November 17, 1858
     * - Julian Date = MJD + 2400000.5
     * - Convert to Gregorian calendar using standard algorithm
     *
     * UTC Time is BCD-encoded: 0xHHMMSS
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
    // Text Extraction
    // ========================================================================

    /**
     * @brief Extract UTF-8 text from EPG data
     * @param data Raw data containing UTF-8 text
     * @param offset Start offset in data
     * @param length Text length in bytes
     * @return Extracted QString (supports Thai characters)
     */
    QString extractThaiText(const QByteArray& data, int offset, int length) const;

    /**
     * @brief Extract null-terminated UTF-8 string
     * @param data Raw data
     * @param offset Start offset
     * @return Extracted QString
     */
    QString extractNullTerminatedString(const QByteArray& data, int offset) const;

    // ========================================================================
    // Content Type Parsing
    // ========================================================================

    /**
     * @brief Parse content type field
     * @param content_nibble Content nibble from EPG data
     * @return Content type enum
     */
    ContentType parseContentType(uint8_t content_nibble) const;

    /**
     * @brief Extract genre string from content type
     * @param type Content type
     * @return Genre string
     */
    QString getGenreString(ContentType type) const;

    // ========================================================================
    // Validation
    // ========================================================================

    /**
     * @brief Validate EPG event data
     * @param event Event to validate
     * @return true if valid
     */
    bool validateEvent(const EPGEvent& event) const;

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
     */
    uint16_t extractUint16BE(const QByteArray& data, int offset) const;

    /**
     * @brief Extract 32-bit big-endian value
     */
    uint32_t extractUint32BE(const QByteArray& data, int offset) const;

    // ========================================================================
    // Private Data Members
    // ========================================================================

    // EPG schedules indexed by service ID
    QMap<uint32_t, EPGSchedule> m_schedules;

    // Statistics
    Statistics m_statistics;

    // Mutex for thread-safe access
    mutable QMutex m_mutex;

    // MJD epoch reference (November 17, 1858)
    static constexpr int MJD_EPOCH_OFFSET = 2400001; // Julian Date offset

    // Valid MJD range (1980-2089 approximately)
    // Note: uint16_t max is 65535, limiting date range to ~2089
    static constexpr uint16_t MIN_VALID_MJD = 44239; // Jan 1, 1980
    static constexpr uint16_t MAX_VALID_MJD = 65500; // ~May 2089 (safety margin)
};

} // namespace eti::epg

// Qt metatype registration for signals/slots
Q_DECLARE_METATYPE(eti::epg::EPGEvent)
Q_DECLARE_METATYPE(eti::epg::EPGSchedule)
Q_DECLARE_METATYPE(eti::epg::ContentType)
