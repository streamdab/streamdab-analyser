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
// Stream-mode display-text extraction (capture-backed, base coverage)
// ============================================================================

namespace {

// A-H1 plausibility gate: on the real HR capture ~71 % of the ≥6-byte
// printable runs are binary noise (`'- B~+'`, `hˀY^{')` …), so a run is only
// accepted as a display-text item when it looks like human text.
//
//   (a) UTF-8: at most ONE U+FFFD replacement character (a run split at a
//       multi-byte boundary may legitimately lose its last byte);
//   (b) printability: ≥ kPrintableScoreNum/kPrintableScoreDen of the
//       characters are letters, digits, space or common text punctuation —
//       German umlauts and Thai both count as letters, so real carousel
//       text passes and binary does not;
//   (c) a word of at least kMinWordLetters letters exists (rejects
//       `'- B~+'` / `hˀY^{')`, which are all "printable" but wordless).
constexpr int kPrintableScoreNum = 7;
constexpr int kPrintableScoreDen = 10;
constexpr int kMinWordLetters = 4;
constexpr int kMaxReplacementChars = 1;

bool isPlausibleStreamText(const QString& text)
{
    if (text.size() < JournalineDecoder::kStreamMinRunBytes) {
        return false;
    }
    // Characters allowed inside a plausible text run. Kept deliberately
    // conservative: symbols that dominate binary payloads (^ { } ~ | \ < >)
    // are NOT allowed, while the punctuation of real prose is.
    static const QString allowedPunct =
        QStringLiteral(".,:;!?\"'()[]/-\u2013\u2014&%+#\u2026");
    int replacement = 0;
    int printable = 0;
    int letterRun = 0;
    int longestLetterRun = 0;
    for (const QChar c : text) {
        if (c.unicode() == 0xFFFD) {
            if (++replacement > kMaxReplacementChars) {
                return false;
            }
        }
        if (c.isLetter()) {
            if (++letterRun > longestLetterRun) {
                longestLetterRun = letterRun;
            }
        } else {
            letterRun = 0;
        }
        if (c.isLetterOrNumber() || c.isSpace() || allowedPunct.contains(c)) {
            ++printable;
        }
    }
    if (longestLetterRun < kMinWordLetters) {
        return false;
    }
    return printable * kPrintableScoreDen >= text.size() * kPrintableScoreNum;
}

} // namespace

bool JournalineDecoder::processStreamData(const QByteArray& stream_chunk)
{
    if (stream_chunk.isEmpty()) {
        return false;
    }
    bool changed = false;
    {
        QMutexLocker locker(&m_mutex);

        const int generationBefore = m_streamItemGeneration;
        const int candidatesBefore = m_streamCandidateCount;

        const qsizetype sizeBefore = m_streamBuffer.size();
        m_streamBuffer.append(stream_chunk);

        // A-H2: hard cap on the raw buffer. Drop-oldest keeps the most recent
        // window; offsets shift, so the tail scan restarts from byte 0.
        if (m_streamBuffer.size() > kStreamBufferCap) {
            const qsizetype overflow = m_streamBuffer.size() - kStreamBufferCap;
            m_streamBuffer.remove(0, overflow);
            if (!m_streamBufferTrimWarned) {
                m_streamBufferTrimWarned = true;
                qWarning() << "[JournalineDecoder] stream buffer cap"
                           << kStreamBufferCap << "bytes reached — dropping the"
                           << "oldest bytes (one-shot warning)";
            }
            m_streamScanOffset = 0;
            m_streamSnapshot = StreamScanSnapshot{};
            m_streamItems.clear();
            m_streamCandidateCount = 0;
            // Every other stream stat is rebased here, and this one must be
            // too: thai_content_count = m_motThaiCount + m_streamThaiCount
            // (see scanStreamTailLocked()) would stay inflated after a trim
            // otherwise.
            m_streamThaiCount = 0;
            m_streamScanned = false;
        }
        Q_UNUSED(sizeBefore);

        // A-H2: rescan gate. The very first pass runs on ANY data (the
        // extraction has to start somewhere); afterwards the rescan is only
        // skipped while the buffer holds fewer than 24 bytes —
        // (size - size%24) > 0 is simply size >= 24 ("at least one whole
        // 24-byte unit is buffered"). It is NOT a cadence guarantee: the walk
        // below is an incremental tail rescan from m_streamScanOffset, it is
        // not aligned to 24-byte boundaries and does not depend on the buffer
        // size being a multiple of 24.
        const bool gate = m_streamScanned
            ? (m_streamBuffer.size() - m_streamBuffer.size() % 24) > 0
            : m_streamBuffer.size() > 0;
        if (gate) {
            m_streamScanned = true;
            scanStreamTailLocked();
            changed = (m_streamItemGeneration != generationBefore)
                || (m_streamCandidateCount != candidatesBefore);
        }
    }
    return changed;
}

void JournalineDecoder::scanStreamTailLocked()
{
    // Invariant (enforced by the offset update at the end of this function):
    // already-committed bytes are NEVER re-walked, while a run that is still
    // pending when the fed chunk ends IS re-walked on the next call — so a
    // run split across two fed chunks merges exactly like a single-shot feed.
    //
    // Concretely, two feed-boundary shapes must be told apart:
    //  - the feed ends MID-run: the trailing run has not been committed yet
    //    (the snapshot below is taken before committing it), so the offset is
    //    left on the run's first byte and the next call re-walks + extends it;
    //  - the feed ends ON a control byte: the loop already committed that last
    //    run, so it is part of the snapshot below and the offset must jump to
    //    the buffer end — leaving it on the run's first byte (the old
    //    behaviour) made the next call re-walk and RE-COMMIT it, minting a
    //    duplicate item with a fresh object id and inflating the candidate
    //    count (and with it the routing ratio).
    QString pendingTitle;
    uint16_t objectId = kStreamObjectIdBase;
    if (m_streamSnapshot.valid) {
        m_streamItems.resize(static_cast<int>(m_streamSnapshot.items));
        m_streamCandidateCount = m_streamSnapshot.candidates;
        m_streamThaiCount = m_streamSnapshot.thai;
        pendingTitle = m_streamSnapshot.pendingTitle;
        objectId = m_streamSnapshot.nextObjectId;
    } else {
        m_streamItems.clear();
        m_streamCandidateCount = 0;
    }

    QByteArray rawRun;
    qsizetype runStart = m_streamScanOffset;

    // Real flush (kept separate so `text` stays in scope for readability).
    auto commit = [&](const QString& text) {
        if (text.size() < kStreamMinRunBytes) {
            return;
        }
        ++m_streamCandidateCount;               // pre-gate candidate
        if (!isPlausibleStreamText(text)) {     // A-H1 plausibility gate
            return;
        }
        if (m_streamItems.size() >= kMaxStreamItems) {  // bounded inventory
            return;
        }
        // Heuristic based on the observed HR carousel: short "teaser/title"
        // runs ("hr4 - Britta am Vormittag", "ARD - Hitnacht") vs long
        // article bodies ("Die ARD-Hitnacht ist …").
        JournalineObject obj;
        obj.object_id = objectId;
        obj.category = 0; // stream mode carries no per-item category here
        if (text.size() <= 26) {
            obj.title = text;
            pendingTitle = text;   // attach to a following body if any
            obj.text_content = QString();
        } else {
            obj.title = pendingTitle;
            obj.text_content = text;
            pendingTitle.clear();
        }
        if (containsThaiText(obj.title) || containsThaiText(obj.text_content)) {
            ++m_streamThaiCount;
        }
        m_streamItems.append(obj);
        ++objectId;
    };

    for (qsizetype i = m_streamScanOffset; i < m_streamBuffer.size(); ++i) {
        const uchar c = static_cast<uchar>(m_streamBuffer.at(i));
        // Collect printable ASCII plus UTF-8 continuation-capable high bytes
        // (Thai text is first-class). Control bytes split the runs.
        if ((c >= 0x20 && c != 0x7F)) {
            if (rawRun.isEmpty()) {
                runStart = i;
            }
            rawRun.append(static_cast<char>(c));
        } else if (!rawRun.isEmpty()) {
            commit(QString::fromUtf8(rawRun).trimmed());
            rawRun.clear();
        }
    }

    // Snapshot BEFORE the trailing run is committed: it captures everything
    // that is committed at this point (including runs the loop closed on a
    // control byte), while the still-pending trailing run — if the buffer
    // ends mid-run — is deliberately excluded so the next call can restore
    // this state and re-walk that run from its first byte.
    m_streamSnapshot.valid = true;
    m_streamSnapshot.items = m_streamItems.size();
    m_streamSnapshot.candidates = m_streamCandidateCount;
    m_streamSnapshot.thai = m_streamThaiCount;
    m_streamSnapshot.pendingTitle = pendingTitle;
    m_streamSnapshot.nextObjectId = objectId;
    if (!rawRun.isEmpty()) {
        commit(QString::fromUtf8(rawRun).trimmed());
        rawRun.clear();
        // Feed ended MID-run: the snapshot above excludes this run, so point
        // the offset at its first byte and let the next call re-walk it.
        m_streamScanOffset = runStart;
    } else {
        // Feed ended ON a control byte (or with no printable byte): every
        // walkable byte is already committed AND included in the snapshot
        // above, so the next call must start at the buffer end. Pointing at
        // runStart here would re-walk the last committed run and commit it a
        // second time (duplicate item / inflated candidate count).
        m_streamScanOffset = m_streamBuffer.size();
    }

    m_streamItemGeneration = m_streamItems.size();

    // A-M3: recomputed from scratch (assign, never ++) — a rescan runs once
    // per fed chunk, so accumulating multiplied the same items per call.
    m_statistics.thai_content_count = m_motThaiCount + m_streamThaiCount;

    // Expose the items through the same object store so the GUI data model
    // can consume them uniformly; ids are disjoint from MOT-decoded objects.
    // A single "Display Carousel" menu holds the titles.
    m_streamMenuId = 0x7FFF;
    if (!m_streamItems.isEmpty()) {
        JournalineMenu carousel;
        carousel.menu_id = m_streamMenuId;
        carousel.parent_menu_id = 0;
        carousel.menu_title = QStringLiteral("Display Carousel");
        carousel.items = m_streamItems;
        m_menus[m_streamMenuId] = carousel;
    } else {
        m_menus.remove(m_streamMenuId);
    }
}

int JournalineDecoder::streamItemCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_streamItems.size();
}

int JournalineDecoder::streamCandidateCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_streamCandidateCount;
}

QVector<JournalineObject> JournalineDecoder::getStreamItems() const
{
    QMutexLocker locker(&m_mutex);
    return m_streamItems;
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

    // Detect Thai content (counted against the MOT source base; the stream
    // rescans recompute their own contribution — see scanStreamTailLocked).
    if (containsThaiText(object.title) || containsThaiText(object.text_content)) {
        ++m_motThaiCount;
        m_statistics.thai_content_count = m_motThaiCount + m_streamThaiCount;
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
    {
        QMutexLocker locker(&m_mutex);

        m_objects.clear();
        m_menus.clear();
        m_streamItems.clear();
        m_streamBuffer.clear();
        m_streamScanned = false;
        m_streamItemGeneration = 0;
        m_streamCandidateCount = 0;
        m_streamScanOffset = 0;
        m_streamSnapshot = StreamScanSnapshot{};
        m_streamBufferTrimWarned = false;
        m_motThaiCount = 0;
        m_streamThaiCount = 0;
        m_statistics = Statistics();
        // BUG-001 invariant ("emit signals OUTSIDE the mutex lock"): the lock
        // is released at the end of this scope; the emits below run unlocked,
        // mirroring TpegDecoder::clearAll().
    }

    emit objectCountChanged();
    emit menuCountChanged();

    qDebug() << "[JournalineDecoder] All Journaline data cleared";
}

void JournalineDecoder::clearObject(uint16_t object_id) {
    bool removed = false;
    {
        QMutexLocker locker(&m_mutex);

        auto it = m_objects.find(object_id);
        if (it != m_objects.end()) {
            m_objects.erase(it);
            removed = true;
        }
    }

    // BUG-001: emit and log after the lock is released.
    if (removed) {
        emit objectCountChanged();

        qDebug() << "[JournalineDecoder] Cleared object" << object_id;
    }
}

void JournalineDecoder::clearMenu(uint16_t menu_id) {
    bool removed = false;
    {
        QMutexLocker locker(&m_mutex);

        auto it = m_menus.find(menu_id);
        if (it != m_menus.end()) {
            m_menus.erase(it);
            removed = true;
        }
    }

    // BUG-001: emit and log after the lock is released.
    if (removed) {
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

    // A-M1: m_menus.clear() above also dropped the stream-mode "Display
    // Carousel" menu (it is not derived from m_objects), so re-insert it
    // whenever stream items exist.
    if (!m_streamItems.isEmpty()) {
        JournalineMenu carousel;
        carousel.menu_id = m_streamMenuId;
        carousel.parent_menu_id = 0;
        carousel.menu_title = QStringLiteral("Display Carousel");
        carousel.items = m_streamItems;
        m_menus[m_streamMenuId] = carousel;
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
