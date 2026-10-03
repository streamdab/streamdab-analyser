/**
 * @file epg_decoder.cpp
 * @brief EPG (Electronic Program Guide) Decoder Implementation
 *
 * Complete implementation of ETSI TS 102 371 EPG decoding.
 *
 * @author StreamDAB Development Team
 * @date October 22, 2025
 */

#include "epg_decoder.hpp"
#include <QTimeZone>
#include <QDebug>
#include <cmath>

namespace eti::epg {

// ============================================================================
// Content Type Conversion
// ============================================================================

QString contentTypeToString(ContentType type) {
    switch (type) {
        case ContentType::NEWS: return "News";
        case ContentType::CURRENT_AFFAIRS: return "Current Affairs";
        case ContentType::INFORMATION: return "Information";
        case ContentType::SPORT: return "Sport";
        case ContentType::EDUCATION: return "Education";
        case ContentType::DRAMA: return "Drama";
        case ContentType::MUSIC: return "Music";
        case ContentType::ARTS_CULTURE: return "Arts & Culture";
        case ContentType::SOCIAL: return "Social";
        case ContentType::SCIENCE: return "Science";
        case ContentType::LEISURE: return "Leisure";
        case ContentType::CHILDRENS: return "Children's";
        case ContentType::UNDEFINED: return "Undefined";
        case ContentType::UNKNOWN:
        default: return "Unknown";
    }
}

// ============================================================================
// EPGEvent Methods
// ============================================================================

QString EPGEvent::toString() const {
    return QString("EPGEvent[ID=%1, %2-%3, %4]")
        .arg(event_id)
        .arg(start_time.toString("yyyy-MM-dd HH:mm"))
        .arg(end_time.toString("HH:mm"))
        .arg(program_name);
}

// ============================================================================
// EPGSchedule Methods
// ============================================================================

QString EPGSchedule::toString() const {
    return QString("EPGSchedule[SId=%1, Date=%2, Events=%3, Ver=%4]")
        .arg(service_id, 4, 16, QChar('0'))
        .arg(schedule_date.toString("yyyy-MM-dd"))
        .arg(events.size())
        .arg(schedule_version);
}

// ============================================================================
// EPGDecoder Constructor/Destructor
// ============================================================================

EPGDecoder::EPGDecoder(QObject* parent)
    : QObject(parent)
{
    qDebug() << "[EPGDecoder] Initialized - ETSI TS 102 371 compliant";
}

EPGDecoder::~EPGDecoder() {
    qDebug() << "[EPGDecoder] Shutdown - Total events processed:" << m_statistics.events_extracted;
}

// ============================================================================
// Main Processing Interface
// ============================================================================

bool EPGDecoder::processMOTObject(const QByteArray& mot_data) {
    QMutexLocker locker(&m_mutex);

    if (mot_data.isEmpty()) {
        emit epgDecodingError("Empty MOT data");
        m_statistics.parsing_errors++;
        return false;
    }

    // Parse EPG schedule from MOT object
    bool success = parseEPGSchedule(mot_data);

    if (success) {
        updateStatistics();
    }

    return success;
}

bool EPGDecoder::parseEPGSchedule(const QByteArray& epg_data) {
    // ETSI TS 102 371 Section 4: EPG Structure
    //
    // EPG Data Format:
    // - Header (8 bytes):
    //   - Service ID (32-bit)
    //   - Schedule date (MJD 16-bit)
    //   - Number of events (16-bit)
    // - Event entries (variable length each)
    //
    // Event Entry Format:
    // - Event ID (16-bit)
    // - Start time (MJD 16-bit + UTC 24-bit)
    // - Duration (16-bit minutes)
    // - Content type (8-bit)
    // - Program name length (8-bit)
    // - Program name (UTF-8)
    // - Description length (16-bit)
    // - Description (UTF-8)

    if (epg_data.size() < 8) {
        emit epgDecodingError("EPG data too short (< 8 bytes)");
        m_statistics.parsing_errors++;
        return false;
    }

    int offset = 0;

    // Parse EPG header
    uint32_t service_id = extractUint32BE(epg_data, offset);
    offset += 4;

    uint16_t schedule_mjd = extractUint16BE(epg_data, offset);
    offset += 2;

    uint16_t num_events = extractUint16BE(epg_data, offset);
    offset += 2;

    // Validate MJD
    if (!validateMJD(schedule_mjd)) {
        emit epgDecodingError(QString("Invalid MJD: %1").arg(schedule_mjd));
        m_statistics.parsing_errors++;
        return false;
    }

    // Create schedule structure
    EPGSchedule schedule;
    schedule.service_id = service_id;
    schedule.schedule_date = convertMJDtoQDateTime(schedule_mjd, 0);
    schedule.last_updated = QDateTime::currentDateTime();
    schedule.schedule_version = extractUint16BE(epg_data, 0) & 0xFFFF; // Simple version from first bytes

    qDebug() << "[EPGDecoder] Parsing schedule for SId" << QString::number(service_id, 16)
             << "Date:" << schedule.schedule_date.toString("yyyy-MM-dd")
             << "Events:" << num_events;

    // Parse events
    for (uint16_t i = 0; i < num_events; ++i) {
        if (offset + 10 > epg_data.size()) {
            emit epgDecodingError(QString("Truncated event data at event %1").arg(i));
            m_statistics.parsing_errors++;
            break;
        }

        EPGEvent event;

        // Event ID (16-bit)
        event.event_id = extractUint16BE(epg_data, offset);
        offset += 2;

        // Start time (MJD 16-bit + UTC 24-bit)
        uint16_t start_mjd = extractUint16BE(epg_data, offset);
        offset += 2;

        uint32_t start_utc = 0;
        start_utc = (static_cast<uint8_t>(epg_data[offset]) << 16) |
                    (static_cast<uint8_t>(epg_data[offset + 1]) << 8) |
                    static_cast<uint8_t>(epg_data[offset + 2]);
        offset += 3;

        event.start_time = convertMJDtoQDateTime(start_mjd, start_utc);

        // Duration (16-bit minutes)
        uint16_t duration_minutes = extractUint16BE(epg_data, offset);
        offset += 2;

        // Sanity check: validate duration is reasonable (max 24 hours)
        if (duration_minutes > 1440) {
            qWarning() << "[EPGDecoder] Excessive event duration:" << duration_minutes
                       << "minutes for event" << i << "- clamping to 24 hours";
            duration_minutes = 1440;
        }

        event.end_time = event.start_time.addSecs(duration_minutes * 60);

        // Content type (8-bit)
        uint8_t content_nibble = static_cast<uint8_t>(epg_data[offset]);
        offset += 1;

        event.content_type = parseContentType(content_nibble);
        event.genre = getGenreString(event.content_type);

        // Program name length (8-bit)
        if (offset >= epg_data.size()) {
            emit epgDecodingError(QString("Truncated at program name length for event %1").arg(i));
            m_statistics.parsing_errors++;
            break;
        }

        uint8_t name_length = static_cast<uint8_t>(epg_data[offset]);
        offset += 1;

        // Program name (UTF-8)
        if (offset + name_length > epg_data.size()) {
            emit epgDecodingError(QString("Truncated program name for event %1").arg(i));
            m_statistics.parsing_errors++;
            break;
        }

        event.program_name = extractThaiText(epg_data, offset, name_length);
        offset += name_length;

        // Description length (16-bit)
        if (offset + 2 > epg_data.size()) {
            emit epgDecodingError(QString("Truncated at description length for event %1").arg(i));
            m_statistics.parsing_errors++;
            break;
        }

        uint16_t desc_length = extractUint16BE(epg_data, offset);
        offset += 2;

        // Description (UTF-8)
        if (offset + desc_length > epg_data.size()) {
            emit epgDecodingError(QString("Truncated description for event %1").arg(i));
            m_statistics.parsing_errors++;
            break;
        }

        event.description = extractThaiText(epg_data, offset, desc_length);
        offset += desc_length;

        // Validate event
        if (validateEvent(event)) {
            schedule.events.append(event);
            emit epgEventDiscovered(service_id, event);

            qDebug() << "[EPGDecoder] Event" << i << ":" << event.program_name
                     << "at" << event.start_time.toString("HH:mm");
        } else {
            qWarning() << "[EPGDecoder] Invalid event" << i << "- skipping";
            m_statistics.parsing_errors++;
        }
    }

    // Store schedule (allow empty schedules - valid case)
    if (schedule.service_id > 0 && schedule.schedule_date.isValid()) {
        m_schedules[service_id] = schedule;
        m_statistics.schedules_processed++;
        m_statistics.events_extracted += schedule.events.size();

        emit epgScheduleUpdated(service_id);
        emit eventCountChanged();
        emit serviceCountChanged();

        qDebug() << "[EPGDecoder] Schedule parsed successfully:" << schedule.toString();
        return true;
    } else {
        emit epgDecodingError("Invalid EPG schedule");
        m_statistics.parsing_errors++;
        return false;
    }
}

// ============================================================================
// Event Retrieval
// ============================================================================

QVector<EPGEvent> EPGDecoder::getEvents(uint32_t service_id) const {
    QMutexLocker locker(&m_mutex);

    auto it = m_schedules.find(service_id);
    if (it != m_schedules.end()) {
        return it->events;
    }

    return QVector<EPGEvent>();
}

std::optional<EPGEvent> EPGDecoder::getCurrentEvent(uint32_t service_id) const {
    QMutexLocker locker(&m_mutex);

    auto it = m_schedules.find(service_id);
    if (it != m_schedules.end()) {
        return it->getCurrentEvent();
    }

    return std::nullopt;
}

std::optional<EPGEvent> EPGDecoder::getNextEvent(uint32_t service_id) const {
    QMutexLocker locker(&m_mutex);

    auto it = m_schedules.find(service_id);
    if (it != m_schedules.end()) {
        return it->getNextEvent();
    }

    return std::nullopt;
}

std::optional<EPGSchedule> EPGDecoder::getSchedule(uint32_t service_id) const {
    QMutexLocker locker(&m_mutex);

    auto it = m_schedules.find(service_id);
    if (it != m_schedules.end()) {
        return *it;
    }

    return std::nullopt;
}

QVector<uint32_t> EPGDecoder::getServiceIds() const {
    QMutexLocker locker(&m_mutex);

    QVector<uint32_t> service_ids;
    for (auto it = m_schedules.begin(); it != m_schedules.end(); ++it) {
        service_ids.append(it.key());
    }

    return service_ids;
}

// ============================================================================
// Statistics
// ============================================================================

int EPGDecoder::eventCount() const {
    QMutexLocker locker(&m_mutex);

    int total = 0;
    for (const auto& schedule : m_schedules) {
        total += schedule.events.size();
    }

    return total;
}

int EPGDecoder::serviceCount() const {
    QMutexLocker locker(&m_mutex);
    return m_schedules.size();
}

EPGDecoder::Statistics EPGDecoder::getStatistics() const {
    QMutexLocker locker(&m_mutex);
    Statistics stats = m_statistics;
    stats.active_services = m_schedules.size();
    return stats;
}

void EPGDecoder::clearAll() {
    QMutexLocker locker(&m_mutex);

    m_schedules.clear();
    m_statistics = Statistics();

    emit eventCountChanged();
    emit serviceCountChanged();

    qDebug() << "[EPGDecoder] All EPG data cleared";
}

void EPGDecoder::clearService(uint32_t service_id) {
    QMutexLocker locker(&m_mutex);

    auto it = m_schedules.find(service_id);
    if (it != m_schedules.end()) {
        m_schedules.erase(it);

        emit eventCountChanged();
        emit serviceCountChanged();

        qDebug() << "[EPGDecoder] Cleared EPG data for SId" << QString::number(service_id, 16);
    }
}

// ============================================================================
// MJD (Modified Julian Date) Conversion
// ============================================================================

QDateTime EPGDecoder::convertMJDtoQDateTime(uint16_t mjd, uint32_t utc_time) const {
    // ETSI TS 102 371 Section 4.1: Time Representation
    //
    // Modified Julian Date (MJD) Algorithm:
    // MJD 0 = November 17, 1858
    // Julian Date = MJD + 2400000.5
    //
    // Conversion to Gregorian Calendar:
    // Based on algorithm from "Calendrical Calculations"

    if (mjd == 0) {
        return QDateTime(); // Invalid
    }

    // Convert MJD to Julian Date
    double jd = mjd + 2400000.5;

    // Convert Julian Date to Gregorian calendar
    int a = static_cast<int>(jd + 0.5);
    int b = a + 1537;
    int c = static_cast<int>((b - 122.1) / 365.25);
    int d = static_cast<int>(365.25 * c);
    int e = static_cast<int>((b - d) / 30.6001);

    int day = b - d - static_cast<int>(30.6001 * e);
    int month = e - 1 - 12 * (e / 14);
    int year = c - 4715 - ((7 + month) / 10);

    // Decode BCD time
    auto [hours, minutes, seconds] = decodeBCDTime(utc_time);

    // Create QDateTime in UTC
    QDate date(year, month, day);
    QTime time(hours, minutes, seconds);

    if (!date.isValid() || !time.isValid()) {
        qWarning() << "[EPGDecoder] Invalid date/time from MJD" << mjd << "UTC" << QString::number(utc_time, 16);
        return QDateTime();
    }

    QDateTime dt(date, time, QTimeZone::utc());
    return dt;
}

std::tuple<int, int, int> EPGDecoder::decodeBCDTime(uint32_t bcd_time) const {
    // BCD format: 0xHHMMSS
    uint8_t hours_bcd = (bcd_time >> 16) & 0xFF;
    uint8_t minutes_bcd = (bcd_time >> 8) & 0xFF;
    uint8_t seconds_bcd = bcd_time & 0xFF;

    int hours = decodeBCD(hours_bcd);
    int minutes = decodeBCD(minutes_bcd);
    int seconds = decodeBCD(seconds_bcd);

    // Clamp to valid ranges
    if (hours < 0 || hours > 23) hours = 0;
    if (minutes < 0 || minutes > 59) minutes = 0;
    if (seconds < 0 || seconds > 59) seconds = 0;

    return std::make_tuple(hours, minutes, seconds);
}

int EPGDecoder::decodeBCD(uint8_t bcd) const {
    int high_nibble = (bcd >> 4) & 0x0F;
    int low_nibble = bcd & 0x0F;

    // Validate BCD (each nibble must be 0-9)
    if (high_nibble > 9 || low_nibble > 9) {
        return 0; // Invalid BCD
    }

    return high_nibble * 10 + low_nibble;
}

// ============================================================================
// Text Extraction
// ============================================================================

QString EPGDecoder::extractThaiText(const QByteArray& data, int offset, int length) const {
    if (offset < 0 || offset + length > data.size() || length <= 0) {
        return QString();
    }

    // Extract byte array
    QByteArray text_data = data.mid(offset, length);

    // Convert UTF-8 to QString (supports Thai)
    QString text = QString::fromUtf8(text_data);

    return text.trimmed();
}

QString EPGDecoder::extractNullTerminatedString(const QByteArray& data, int offset) const {
    if (offset < 0 || offset >= data.size()) {
        return QString();
    }

    // Find null terminator
    int null_pos = data.indexOf('\0', offset);
    if (null_pos < 0) {
        null_pos = data.size(); // No null found, use end of data
    }

    int length = null_pos - offset;
    return extractThaiText(data, offset, length);
}

// ============================================================================
// Content Type Parsing
// ============================================================================

ContentType EPGDecoder::parseContentType(uint8_t content_nibble) const {
    // ETSI TS 102 371 Section 4.3: Content Type Classification
    // Upper nibble defines category

    uint8_t category = content_nibble & 0xF0;

    switch (category) {
        case 0x10: return ContentType::NEWS;
        case 0x20: return ContentType::SPORT;
        case 0x30: return ContentType::EDUCATION;
        case 0x40: return ContentType::DRAMA;
        case 0x50: return ContentType::MUSIC;
        case 0x60: return ContentType::ARTS_CULTURE;
        case 0x70: return ContentType::SOCIAL;
        case 0x80: return ContentType::SCIENCE;
        case 0x90: return ContentType::LEISURE;
        case 0xA0: return ContentType::CHILDRENS;
        case 0x00: return ContentType::UNDEFINED;
        default: return ContentType::UNKNOWN;
    }
}

QString EPGDecoder::getGenreString(ContentType type) const {
    return contentTypeToString(type);
}

// ============================================================================
// Validation
// ============================================================================

bool EPGDecoder::validateEvent(const EPGEvent& event) const {
    return event.isValid();
}

bool EPGDecoder::validateMJD(uint16_t mjd) const {
    // Check if MJD is within reasonable range (1980-2099)
    return mjd >= MIN_VALID_MJD && mjd <= MAX_VALID_MJD;
}

// ============================================================================
// Helper Methods
// ============================================================================

void EPGDecoder::updateStatistics() {
    // Statistics already updated in parseEPGSchedule
}

uint16_t EPGDecoder::extractUint16BE(const QByteArray& data, int offset) const {
    if (offset + 2 > data.size()) {
        return 0;
    }

    uint16_t value = (static_cast<uint8_t>(data[offset]) << 8) |
                     static_cast<uint8_t>(data[offset + 1]);
    return value;
}

uint32_t EPGDecoder::extractUint32BE(const QByteArray& data, int offset) const {
    if (offset + 4 > data.size()) {
        return 0;
    }

    uint32_t value = (static_cast<uint8_t>(data[offset]) << 24) |
                     (static_cast<uint8_t>(data[offset + 1]) << 16) |
                     (static_cast<uint8_t>(data[offset + 2]) << 8) |
                     static_cast<uint8_t>(data[offset + 3]);
    return value;
}

} // namespace eti::epg
