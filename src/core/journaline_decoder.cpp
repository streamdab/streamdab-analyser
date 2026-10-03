/**
 * @file journaline_decoder.cpp
 * @brief Journaline Text Service Decoder Implementation
 *
 * Complete implementation of ETSI TS 102 979 Journaline decoding.
 *
 * @author StreamDAB Development Team
 * @date October 22, 2025
 */

#include "journaline_decoder.hpp"
#include <QTimeZone>
#include <QDebug>
#include <cmath>

namespace eti::journaline {

// ============================================================================
// Category Conversion
// ============================================================================

QString categoryToString(JournalineCategory category) {
    switch (category) {
        case JournalineCategory::NEWS_GENERAL: return "News - General";
        case JournalineCategory::NEWS_REGIONAL: return "News - Regional";
        case JournalineCategory::NEWS_INTERNATIONAL: return "News - International";
        case JournalineCategory::SPORT_GENERAL: return "Sport - General";
        case JournalineCategory::SPORT_FOOTBALL: return "Sport - Football";
        case JournalineCategory::SPORT_OTHER: return "Sport - Other";
        case JournalineCategory::WEATHER_GENERAL: return "Weather - General";
        case JournalineCategory::WEATHER_FORECAST: return "Weather - Forecast";
        case JournalineCategory::WEATHER_WARNING: return "Weather - Warning";
        case JournalineCategory::TRAFFIC: return "Traffic";
        case JournalineCategory::FINANCIAL: return "Financial";
        case JournalineCategory::ENTERTAINMENT: return "Entertainment";
        case JournalineCategory::POLITICS: return "Politics";
        case JournalineCategory::CULTURE: return "Culture";
        case JournalineCategory::TECHNOLOGY: return "Technology";
        case JournalineCategory::HEALTH: return "Health";
        case JournalineCategory::UNDEFINED: return "Undefined";
        case JournalineCategory::UNKNOWN:
        default: return "Unknown";
    }
}

// ============================================================================
// JournalineObject Methods
// ============================================================================

QString JournalineObject::getCategoryString() const {
    JournalineCategory cat = static_cast<JournalineCategory>(category);
    return categoryToString(cat);
}

// ============================================================================
// JournalineDecoder Constructor/Destructor
// ============================================================================

JournalineDecoder::JournalineDecoder(QObject* parent)
    : QObject(parent)
{
    qDebug() << "[JournalineDecoder] Initialized - ETSI TS 102 979 compliant";
}

JournalineDecoder::~JournalineDecoder() {
    qDebug() << "[JournalineDecoder] Shutdown - Total objects processed:" << m_statistics.objects_processed;
}

// ============================================================================
// Main Processing Interface
// ============================================================================

bool JournalineDecoder::processMOTObject(const QByteArray& mot_data) {
    // BUG-001-CRITICAL FIX: Emit signals OUTSIDE mutex lock to prevent deadlock
    // Signal emission while holding lock can cause deadlock if slots call back into decoder
    
    QString error_msg;
    bool success = false;
    uint16_t object_id = 0;
    JournalineObject parsed_object;
    QVector<uint16_t> updated_menu_ids;
    
    {
        QMutexLocker locker(&m_mutex);
        
        if (mot_data.isEmpty()) {
            error_msg = "Empty MOT data";
            m_statistics.parsing_errors++;
        } else {
            // Parse Journaline object from MOT data (internal, no signal emission)
            success = parseJournalineObjectInternal(mot_data, object_id, parsed_object);
            
            if (success) {
                // Store object in internal state
                m_objects[object_id] = parsed_object;
                m_statistics.objects_processed++;
                
                updateStatistics();
                
                // Build menu hierarchy and collect updated menu IDs
                buildMenuHierarchyInternal(updated_menu_ids);
            } else {
                // Parsing failed - set generic error message
                if (mot_data.size() < 10) {
                    error_msg = "Journaline data too short (< 10 bytes)";
                } else {
                    error_msg = "Failed to parse Journaline object";
                }
            }
        }
    } // Mutex released HERE
    
    // Emit signals AFTER mutex release
    if (!error_msg.isEmpty()) {
        emit journalineDecodingError(error_msg);
        return false;
    }
    
    if (success) {
        // Emit object received signal
        emit journalineObjectReceived(object_id, parsed_object);
        emit objectCountChanged();
        
        // Emit menu update signals
        for (uint16_t menu_id : updated_menu_ids) {
            emit journalineMenuUpdated(menu_id);
        }
        
        emit menuCountChanged();
    }
    
    return success;
}

// ============================================================================
// ETSI TS 102 979 Parsing Methods
// ============================================================================

bool JournalineDecoder::parseJournalineObjectInternal(const QByteArray& data, 
                                                      uint16_t& out_object_id,
                                                      JournalineObject& out_object) {
    // BUG-001-CRITICAL FIX: Internal parsing function that does NOT emit signals
    // Called while mutex is held - signals emitted by caller after mutex release
    //
    // ETSI TS 102 979 Section 5: Journaline Object Structure
    //
    // Object Format:
    // - Object ID (16-bit)
    // - Category (8-bit)
    // - Timestamp (40-bit): MJD (16-bit) + UTC (24-bit BCD)
    // - Title length (8-bit)
    // - Title text (UTF-8)
    // - Content length (16-bit)
    // - Content text (UTF-8)
    // - Link flag (1-bit)
    // - Link target (16-bit, if link flag set)

    if (data.size() < 10) {
        // Note: Error will be reported by caller via error_msg
        qWarning() << "[JournalineDecoder] Journaline data too short (< 10 bytes)";
        m_statistics.parsing_errors++;
        return false;
    }

    int offset = 0;
    JournalineObject object;

    // Parse object header
    offset = parseObjectHeader(data, offset, object);
    if (offset < 0) {
        qWarning() << "[JournalineDecoder] Failed to parse Journaline object header";
        m_statistics.parsing_errors++;
        return false;
    }

    // Title length (8-bit)
    if (offset >= data.size()) {
        qWarning() << "[JournalineDecoder] Truncated at title length";
        m_statistics.parsing_errors++;
        return false;
    }

    uint8_t title_length = static_cast<uint8_t>(data[offset]);
    offset += 1;

    // BUG-002-HIGH FIX: Overflow-safe bounds checking for title_length
    // Check if title_length exceeds remaining data size
    if (title_length > static_cast<uint8_t>(data.size() - offset) ||
        offset + static_cast<int>(title_length) < offset) {  // Detect overflow
        qWarning() << "[JournalineDecoder] Invalid title length:" << title_length
                   << "at offset:" << offset << "data size:" << data.size();
        m_statistics.parsing_errors++;
        return false;
    }

    // Sanity check: title should be reasonable size (< 256 bytes, already enforced by uint8_t)
    object.title = extractText(data, offset, title_length);
    offset += title_length;

    // Content length (16-bit)
    if (offset + 2 > data.size()) {
        qWarning() << "[JournalineDecoder] Truncated at content length";
        m_statistics.parsing_errors++;
        return false;
    }

    // BUG-003-HIGH FIX: Handle std::optional return from extractUint16BE
    auto content_length_opt = extractUint16BE(data, offset);
    if (!content_length_opt.has_value()) {
        qWarning() << "[JournalineDecoder] Failed to extract content length at offset" << offset;
        m_statistics.parsing_errors++;
        return false;
    }
    uint16_t content_length = *content_length_opt;
    offset += 2;

    // BUG-002-HIGH FIX: Overflow-safe bounds checking for content_length
    // Check if content_length exceeds remaining data size
    if (content_length > static_cast<uint16_t>(data.size() - offset) ||
        offset + static_cast<int>(content_length) < offset) {  // Detect overflow
        qWarning() << "[JournalineDecoder] Invalid content length:" << content_length
                   << "at offset:" << offset << "data size:" << data.size();
        m_statistics.parsing_errors++;
        return false;
    }

    // Sanity check: content should be reasonable size (< 64KB for Journaline)
    if (content_length > 65000) {
        qWarning() << "[JournalineDecoder] Excessive content length:" << content_length;
        m_statistics.parsing_errors++;
        return false;
    }

    object.text_content = extractText(data, offset, content_length);
    offset += content_length;

    // Link flag (1 bit, stored in byte)
    if (offset >= data.size()) {
        // Link is optional - not an error if missing
        qDebug() << "[JournalineDecoder] No link data for object" << object.object_id;
    } else {
        uint8_t link_byte = static_cast<uint8_t>(data[offset]);
        offset += 1;

        object.is_link = (link_byte & 0x80) != 0;

        // Link target (16-bit, if link flag set)
        if (object.is_link) {
            if (offset + 2 > data.size()) {
                qWarning() << "[JournalineDecoder] Truncated link target";
                object.is_link = false;
            } else {
                // BUG-003-HIGH FIX: Handle std::optional return from extractUint16BE
                auto link_target_opt = extractUint16BE(data, offset);
                if (!link_target_opt.has_value()) {
                    qWarning() << "[JournalineDecoder] Failed to extract link target at offset" << offset;
                    object.is_link = false;
                } else {
                    object.link_target = *link_target_opt;
                    offset += 2;
                    qDebug() << "[JournalineDecoder] Object" << object.object_id << "links to" << object.link_target;
                    m_statistics.links_resolved++;
                }
            }
        }
    }

    // Detect Thai content
    if (containsThaiText(object.title) || containsThaiText(object.text_content)) {
        m_statistics.thai_content_count++;
        qDebug() << "[JournalineDecoder] Thai content detected in object" << object.object_id;
    }

    // Validate object
    if (!validateObject(object)) {
        qWarning() << "[JournalineDecoder] Invalid object - skipping";
        m_statistics.parsing_errors++;
        return false;
    }

    // Return parsed object and ID to caller
    out_object_id = object.object_id;
    out_object = object;

    qDebug() << "[JournalineDecoder] Object parsed:" << object.toString();
    qDebug() << "[JournalineDecoder] Title:" << object.title;
    qDebug() << "[JournalineDecoder] Content preview:" << object.text_content.left(50);

    return true;
}

int JournalineDecoder::parseObjectHeader(const QByteArray& data, int offset, JournalineObject& object) {
    if (offset + 8 > data.size()) {
        qWarning() << "[JournalineDecoder] Insufficient data for object header";
        return -1;
    }

    // Object ID (16-bit)
    // BUG-003-HIGH FIX: Handle std::optional return from extractUint16BE
    auto object_id_opt = extractUint16BE(data, offset);
    if (!object_id_opt.has_value()) {
        qWarning() << "[JournalineDecoder] Failed to extract object ID at offset" << offset;
        return -1;
    }
    object.object_id = *object_id_opt;
    offset += 2;

    if (!validateObjectId(object.object_id)) {
        qWarning() << "[JournalineDecoder] Invalid object ID:" << object.object_id;
        return -1;
    }

    // Category (8-bit)
    uint8_t category_byte = static_cast<uint8_t>(data[offset]);
    offset += 1;

    object.category = parseCategory(category_byte);

    // Timestamp (40-bit): MJD (16-bit) + UTC (24-bit)
    // BUG-003-HIGH FIX: Handle std::optional return from extractUint16BE
    auto mjd_opt = extractUint16BE(data, offset);
    if (!mjd_opt.has_value()) {
        qWarning() << "[JournalineDecoder] Failed to extract MJD at offset" << offset;
        return -1;
    }
    uint16_t mjd = *mjd_opt;
    offset += 2;

    uint32_t utc_time = 0;
    utc_time = (static_cast<uint8_t>(data[offset]) << 16) |
               (static_cast<uint8_t>(data[offset + 1]) << 8) |
               static_cast<uint8_t>(data[offset + 2]);
    offset += 3;

    // Validate and convert timestamp
    if (validateMJD(mjd)) {
        object.timestamp = convertMJDtoQDateTime(mjd, utc_time);
    } else {
        qWarning() << "[JournalineDecoder] Invalid MJD:" << mjd << "- using current time";
        object.timestamp = QDateTime::currentDateTime();
    }

    qDebug() << "[JournalineDecoder] Object ID:" << object.object_id
             << "Category:" << object.getCategoryString()
             << "Timestamp:" << object.timestamp.toString("yyyy-MM-dd HH:mm:ss");

    return offset;
}

uint8_t JournalineDecoder::parseCategory(uint8_t category_byte) const {
    // ETSI TS 102 979 Section 5.3: Category Encoding
    // Categories are defined in spec, directly return byte value
    return category_byte;
}

JournalineCategory JournalineDecoder::getCategoryEnum(uint8_t category_byte) const {
    // Map byte to enum
    switch (category_byte) {
        case 0x01: return JournalineCategory::NEWS_GENERAL;
        case 0x02: return JournalineCategory::NEWS_REGIONAL;
        case 0x03: return JournalineCategory::NEWS_INTERNATIONAL;
        case 0x10: return JournalineCategory::SPORT_GENERAL;
        case 0x11: return JournalineCategory::SPORT_FOOTBALL;
        case 0x12: return JournalineCategory::SPORT_OTHER;
        case 0x20: return JournalineCategory::WEATHER_GENERAL;
        case 0x21: return JournalineCategory::WEATHER_FORECAST;
        case 0x22: return JournalineCategory::WEATHER_WARNING;
        case 0x30: return JournalineCategory::TRAFFIC;
        case 0x40: return JournalineCategory::FINANCIAL;
        case 0x50: return JournalineCategory::ENTERTAINMENT;
        case 0x60: return JournalineCategory::POLITICS;
        case 0x70: return JournalineCategory::CULTURE;
        case 0x80: return JournalineCategory::TECHNOLOGY;
        case 0x90: return JournalineCategory::HEALTH;
        case 0x00: return JournalineCategory::UNDEFINED;
        default: return JournalineCategory::UNKNOWN;
    }
}

// ============================================================================
// Object Retrieval
// ============================================================================

QVector<JournalineObject> JournalineDecoder::getObjects() const {
    QMutexLocker locker(&m_mutex);

    QVector<JournalineObject> objects;
    objects.reserve(m_objects.size());

    for (auto it = m_objects.begin(); it != m_objects.end(); ++it) {
        objects.append(it.value());
    }

    return objects;
}

std::optional<JournalineObject> JournalineDecoder::getObject(uint16_t object_id) const {
    QMutexLocker locker(&m_mutex);

    auto it = m_objects.find(object_id);
    if (it != m_objects.end()) {
        return it.value();
    }

    return std::nullopt;
}

QVector<JournalineObject> JournalineDecoder::getObjectsByCategory(uint8_t category) const {
    QMutexLocker locker(&m_mutex);

    QVector<JournalineObject> filtered;

    for (auto it = m_objects.begin(); it != m_objects.end(); ++it) {
        if (it.value().category == category) {
            filtered.append(it.value());
        }
    }

    return filtered;
}

std::optional<JournalineObject> JournalineDecoder::resolveLinkTarget(const JournalineObject& source_object) const {
    if (!source_object.is_link) {
        return std::nullopt;
    }

    return getObject(source_object.link_target);
}

// ============================================================================
// Menu Retrieval
// ============================================================================

std::optional<JournalineMenu> JournalineDecoder::getMenu(uint16_t menu_id) const {
    QMutexLocker locker(&m_mutex);

    auto it = m_menus.find(menu_id);
    if (it != m_menus.end()) {
        return it.value();
    }

    return std::nullopt;
}

QVector<uint16_t> JournalineDecoder::getMenuIds() const {
    QMutexLocker locker(&m_mutex);

    QVector<uint16_t> menu_ids;
    menu_ids.reserve(m_menus.size());

    for (auto it = m_menus.begin(); it != m_menus.end(); ++it) {
        menu_ids.append(it.key());
    }

    return menu_ids;
}

QVector<JournalineMenu> JournalineDecoder::getRootMenus() const {
    QMutexLocker locker(&m_mutex);

    QVector<JournalineMenu> root_menus;

    for (auto it = m_menus.begin(); it != m_menus.end(); ++it) {
        if (it.value().isRootMenu()) {
            root_menus.append(it.value());
        }
    }

    return root_menus;
}

QVector<JournalineMenu> JournalineDecoder::getSubMenus(uint16_t parent_menu_id) const {
    QMutexLocker locker(&m_mutex);

    QVector<JournalineMenu> submenus;

    for (auto it = m_menus.begin(); it != m_menus.end(); ++it) {
        if (it.value().parent_menu_id == parent_menu_id && !it.value().isRootMenu()) {
            submenus.append(it.value());
        }
    }

    return submenus;
}

// ============================================================================
// Statistics
// ============================================================================

int JournalineDecoder::objectCount() const {
    QMutexLocker locker(&m_mutex);
    return m_objects.size();
}

int JournalineDecoder::menuCount() const {
    QMutexLocker locker(&m_mutex);
    return m_menus.size();
}

JournalineDecoder::Statistics JournalineDecoder::getStatistics() const {
    QMutexLocker locker(&m_mutex);
    return m_statistics;
}

void JournalineDecoder::clearAll() {
    QMutexLocker locker(&m_mutex);

    m_objects.clear();
    m_menus.clear();
    m_statistics = Statistics();

    emit objectCountChanged();
    emit menuCountChanged();

    qDebug() << "[JournalineDecoder] All Journaline data cleared";
}

void JournalineDecoder::clearObject(uint16_t object_id) {
    QMutexLocker locker(&m_mutex);

    auto it = m_objects.find(object_id);
    if (it != m_objects.end()) {
        m_objects.erase(it);
        emit objectCountChanged();

        qDebug() << "[JournalineDecoder] Cleared object" << object_id;
    }
}

void JournalineDecoder::clearMenu(uint16_t menu_id) {
    QMutexLocker locker(&m_mutex);

    auto it = m_menus.find(menu_id);
    if (it != m_menus.end()) {
        m_menus.erase(it);
        emit menuCountChanged();

        qDebug() << "[JournalineDecoder] Cleared menu" << menu_id;
    }
}

// ============================================================================
// Text Extraction
// ============================================================================

QString JournalineDecoder::extractText(const QByteArray& data, int offset, int length) const {
    if (offset < 0 || offset + length > data.size() || length <= 0) {
        return QString();
    }

    // Extract byte array
    QByteArray text_data = data.mid(offset, length);

    // Convert UTF-8 to QString (supports Thai)
    QString text = QString::fromUtf8(text_data);

    return text.trimmed();
}

QString JournalineDecoder::extractNullTerminatedString(const QByteArray& data, int offset) const {
    if (offset < 0 || offset >= data.size()) {
        return QString();
    }

    // Find null terminator
    int null_pos = data.indexOf('\0', offset);
    if (null_pos < 0) {
        null_pos = data.size(); // No null found, use end of data
    }

    int length = null_pos - offset;
    return extractText(data, offset, length);
}

bool JournalineDecoder::containsThaiText(const QString& text) const {
    // Thai Unicode range: U+0E00 to U+0E7F
    for (const QChar& ch : text) {
        ushort unicode = ch.unicode();
        if (unicode >= 0x0E00 && unicode <= 0x0E7F) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// Timestamp Parsing (MJD + UTC)
// ============================================================================

QDateTime JournalineDecoder::parseTimestamp(uint32_t mjd, uint32_t utc_time) const {
    return convertMJDtoQDateTime(static_cast<uint16_t>(mjd), utc_time);
}

QDateTime JournalineDecoder::convertMJDtoQDateTime(uint16_t mjd, uint32_t utc_time) const {
    // ETSI TS 102 979 Section 5.2: Timestamp Format
    // Same algorithm as EPG decoder (ETSI TS 102 371)
    //
    // Modified Julian Date (MJD) Algorithm:
    // MJD 0 = November 17, 1858
    // Julian Date = MJD + 2400000.5

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
        qWarning() << "[JournalineDecoder] Invalid date/time from MJD" << mjd << "UTC" << QString::number(utc_time, 16);
        return QDateTime();
    }

    QDateTime dt(date, time, QTimeZone::utc());
    return dt;
}

std::tuple<int, int, int> JournalineDecoder::decodeBCDTime(uint32_t bcd_time) const {
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

int JournalineDecoder::decodeBCD(uint8_t bcd) const {
    int high_nibble = (bcd >> 4) & 0x0F;
    int low_nibble = bcd & 0x0F;

    // Validate BCD (each nibble must be 0-9)
    if (high_nibble > 9 || low_nibble > 9) {
        return 0; // Invalid BCD
    }

    return high_nibble * 10 + low_nibble;
}

// ============================================================================
// Menu Construction
// ============================================================================

void JournalineDecoder::buildMenuHierarchyInternal(QVector<uint16_t>& out_updated_menu_ids) {
    // BUG-001-CRITICAL FIX: Internal menu building that does NOT emit signals
    // Called while mutex is held - caller emits signals after mutex release
    //
    // ETSI TS 102 979 Section 6: Menu Hierarchy
    //
    // Simple menu construction strategy:
    // - Group objects by category
    // - Create menu for each category
    // - Root menu (ID=1) contains category submenus

    out_updated_menu_ids.clear();

    if (m_objects.isEmpty()) {
        return;
    }

    // Clear existing menus
    m_menus.clear();

    // Group objects by category
    QMap<uint8_t, QVector<JournalineObject>> category_groups;

    for (auto it = m_objects.begin(); it != m_objects.end(); ++it) {
        const JournalineObject& obj = it.value();
        category_groups[obj.category].append(obj);
    }

    // Create root menu
    JournalineMenu root_menu;
    root_menu.menu_id = 1;
    root_menu.parent_menu_id = 0;
    root_menu.menu_title = "Main Menu";
    m_menus[root_menu.menu_id] = root_menu;
    out_updated_menu_ids.append(root_menu.menu_id);

    // Create category menus
    uint16_t menu_id_counter = 10; // Start category menus at ID 10

    for (auto it = category_groups.begin(); it != category_groups.end(); ++it) {
        uint8_t category = it.key();
        const QVector<JournalineObject>& objects = it.value();

        if (objects.isEmpty()) {
            continue;
        }

        JournalineMenu category_menu;
        category_menu.menu_id = menu_id_counter++;
        category_menu.parent_menu_id = 1; // Parent is root menu
        category_menu.menu_title = objects.first().getCategoryString();
        category_menu.items = objects;

        m_menus[category_menu.menu_id] = category_menu;
        out_updated_menu_ids.append(category_menu.menu_id);

        qDebug() << "[JournalineDecoder] Created menu:" << category_menu.toString();
    }

    m_statistics.menus_processed = m_menus.size();

    qDebug() << "[JournalineDecoder] Menu hierarchy built:" << m_menus.size() << "menus";
}

void JournalineDecoder::addObjectToMenu(const JournalineObject& object) {
    // Add object to appropriate category menu
    // This is called during menu construction

    for (auto it = m_menus.begin(); it != m_menus.end(); ++it) {
        JournalineMenu& menu = it.value();

        // Check if object belongs to this menu's category
        if (!menu.items.isEmpty() && menu.items.first().category == object.category) {
            menu.items.append(object);
            return;
        }
    }
}

void JournalineDecoder::createMenu(uint16_t menu_id, uint16_t parent_id,
                                  const QString& title, const QVector<JournalineObject>& objects) {
    JournalineMenu menu;
    menu.menu_id = menu_id;
    menu.parent_menu_id = parent_id;
    menu.menu_title = title;
    menu.items = objects;

    m_menus[menu_id] = menu;

    emit journalineMenuUpdated(menu_id);

    qDebug() << "[JournalineDecoder] Created menu:" << menu.toString();
}

// ============================================================================
// Validation
// ============================================================================

bool JournalineDecoder::validateObject(const JournalineObject& object) const {
    return object.isValid();
}

bool JournalineDecoder::validateMJD(uint16_t mjd) const {
    // Check if MJD is within reasonable range (1980-2099)
    return mjd >= MIN_VALID_MJD && mjd <= MAX_VALID_MJD;
}

// ============================================================================
// Helper Methods
// ============================================================================

void JournalineDecoder::updateStatistics() {
    // Statistics updated inline during parsing
}

std::optional<uint16_t> JournalineDecoder::extractUint16BE(const QByteArray& data, int offset) const {
    // BUG-003-HIGH FIX: Return std::optional for explicit error handling
    // Check for negative offset and bounds violation
    if (offset < 0 || offset + 2 > data.size()) {
        return std::nullopt;  // Explicit error indication
    }

    uint16_t value = (static_cast<uint8_t>(data[offset]) << 8) |
                     static_cast<uint8_t>(data[offset + 1]);
    return value;
}

std::optional<uint32_t> JournalineDecoder::extractUint32BE(const QByteArray& data, int offset) const {
    // BUG-003-HIGH FIX: Return std::optional for explicit error handling
    // Check for negative offset and bounds violation
    if (offset < 0 || offset + 4 > data.size()) {
        return std::nullopt;  // Explicit error indication
    }

    uint32_t value = (static_cast<uint8_t>(data[offset]) << 24) |
                     (static_cast<uint8_t>(data[offset + 1]) << 16) |
                     (static_cast<uint8_t>(data[offset + 2]) << 8) |
                     static_cast<uint8_t>(data[offset + 3]);
    return value;
}

} // namespace eti::journaline
