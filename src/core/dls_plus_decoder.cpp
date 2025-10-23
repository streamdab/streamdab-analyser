// dls_plus_decoder.cpp - DLS+ (Dynamic Label Plus) Decoder Implementation
// ETSI TS 102 980 - Dynamic Label Plus (DLS+) Specification
//
// This component implements DLS+ message parsing for structured "now playing"
// metadata extraction from DAB/DAB+ Program Associated Data (PAD).
//
// Phase 3B: Week 1 - Core Parser Implementation
// Timeline: October 23-29, 2025
// Status: Implementation in progress

#include "dls_plus_decoder.hpp"
#include <QMutexLocker>
#include <QDebug>

namespace eti {
namespace dls_plus {

// ============================================================================
// DLSPlusTag Implementation
// ============================================================================

QString DLSPlusTag::getTypeString() const {
    switch (static_cast<ContentType>(content_type)) {
        case ContentType::DUMMY: return "DUMMY";
        case ContentType::ITEM: return "ITEM";
        case ContentType::INFO: return "INFO";
        case ContentType::PROGRAMME: return "PROGRAMME";
        case ContentType::INTERACTIVITY: return "INTERACTIVITY";
        case ContentType::DESCRIPTOR: return "DESCRIPTOR";
        case ContentType::STATIONNAME: return "STATIONNAME";
        case ContentType::PROGRAMME_TYPE: return "PROGRAMME_TYPE";
        case ContentType::VALIDITY: return "VALIDITY";
        default: return QString("RESERVED_%1").arg(content_type);
    }
}

QString DLSPlusTag::getSubtypeString() const {
    // Subtype interpretation depends on content_type
    // Full implementation based on ETSI TS 102 980 Table 2
    
    switch (static_cast<ContentType>(content_type)) {
        case ContentType::ITEM:
            switch (content_subtype) {
                case 0: return "TITLE";
                case 1: return "ALBUM";
                case 2: return "TRACKNUMBER";
                case 3: return "ARTIST";
                case 4: return "COMPOSITION";
                case 5: return "MOVEMENT";
                case 6: return "CONDUCTOR";
                case 7: return "COMPOSER";
                case 8: return "BAND";
                case 9: return "COMMENT";
                case 10: return "GENRE";
                default: return QString("SUBTYPE_%1").arg(content_subtype);
            }
        
        case ContentType::INFO:
            switch (content_subtype) {
                case 0: return "NEWS";
                case 1: return "NEWS_LOCAL";
                case 2: return "STOCKMARKET";
                case 3: return "SPORT";
                case 4: return "LOTTERY";
                case 5: return "HOROSCOPE";
                case 6: return "DAILY_DIVERSION";
                case 7: return "HEALTH";
                case 8: return "EVENT";
                case 9: return "SCENE";
                case 10: return "CINEMA";
                case 11: return "TV";
                case 12: return "DATE_TIME";
                case 13: return "WEATHER";
                case 14: return "TRAFFIC";
                case 15: return "ALARM";
                default: return QString("SUBTYPE_%1").arg(content_subtype);
            }
        
        case ContentType::PROGRAMME:
            switch (content_subtype) {
                case 0: return "NOW";
                case 1: return "NEXT";
                case 2: return "PART";
                case 3: return "HOST";
                case 4: return "EDITORIAL_STAFF";
                case 5: return "FREQUENCY";
                case 6: return "HOMEPAGE";
                case 7: return "SUBCHANNEL";
                default: return QString("SUBTYPE_%1").arg(content_subtype);
            }
        
        case ContentType::INTERACTIVITY:
            switch (content_subtype) {
                case 0: return "SMS";
                case 1: return "MMS";
                case 2: return "EMAIL";
                case 3: return "PHONE_HOTLINE";
                case 4: return "PHONE_STUDIO";
                case 5: return "PHONE_OTHER";
                case 6: return "FAX";
                case 7: return "MMS_STUDIO";
                case 8: return "SMS_STUDIO";
                default: return QString("SUBTYPE_%1").arg(content_subtype);
            }
        
        case ContentType::STATIONNAME:
            switch (content_subtype) {
                case 0: return "SHORT";
                case 1: return "LONG";
                default: return QString("SUBTYPE_%1").arg(content_subtype);
            }
        
        case ContentType::PROGRAMME_TYPE:
            return QString("PTY_%1").arg(content_subtype);
        
        default:
            return QString("SUBTYPE_%1").arg(content_subtype);
    }
}

QString DLSPlusTag::getFullTagName() const {
    return QString("%1.%2").arg(getTypeString(), getSubtypeString());
}

bool DLSPlusTag::isValid() const {
    // Basic validation
    if (content_type > 15) return false;  // 4-bit max
    if (content_subtype > 15) return false;  // 4-bit max
    if (start_marker > 127) return false;  // 7-bit max
    if (length_marker == 0) return false;  // Must have content
    if (start_marker + length_marker > 128) return false;  // Bounds check
    
    return true;
}

// ============================================================================
// DLSPlusMessage Implementation
// ============================================================================

QString DLSPlusMessage::getTag(const QString& tag_name) const {
    for (const auto& tag : tags) {
        if (tag.getFullTagName() == tag_name) {
            return tag.text;
        }
    }
    return QString();
}

bool DLSPlusMessage::hasTag(const QString& tag_name) const {
    for (const auto& tag : tags) {
        if (tag.getFullTagName() == tag_name) {
            return true;
        }
    }
    return false;
}

QMap<QString, QString> DLSPlusMessage::getAllTags() const {
    QMap<QString, QString> result;
    for (const auto& tag : tags) {
        result[tag.getFullTagName()] = tag.text;
    }
    return result;
}

QString DLSPlusMessage::getNowPlaying() const {
    QString artist = getTag("ITEM.ARTIST");
    QString title = getTag("ITEM.TITLE");
    
    if (!artist.isEmpty() && !title.isEmpty()) {
        return QString("%1 - %2").arg(artist, title);
    } else if (!title.isEmpty()) {
        return title;
    } else if (!artist.isEmpty()) {
        return artist;
    }
    
    return dls_text;  // Fallback to raw DLS text
}

QString DLSPlusMessage::getArtist() const {
    return getTag("ITEM.ARTIST");
}

QString DLSPlusMessage::getItem() const {
    return getTag("ITEM.TITLE");
}

// ============================================================================
// DLSPlusDecoder Implementation
// ============================================================================

DLSPlusDecoder::DLSPlusDecoder(QObject* parent)
    : QObject(parent)
    , total_messages_processed_(0)
    , valid_messages_count_(0)
    , invalid_messages_count_(0)
{
    initializeTagLookupTables();
}

DLSPlusDecoder::~DLSPlusDecoder() = default;

void DLSPlusDecoder::initializeTagLookupTables() {
    // Initialize content type names
    tag_types_[static_cast<uint8_t>(ContentType::DUMMY)] = "DUMMY";
    tag_types_[static_cast<uint8_t>(ContentType::ITEM)] = "ITEM";
    tag_types_[static_cast<uint8_t>(ContentType::INFO)] = "INFO";
    tag_types_[static_cast<uint8_t>(ContentType::PROGRAMME)] = "PROGRAMME";
    tag_types_[static_cast<uint8_t>(ContentType::INTERACTIVITY)] = "INTERACTIVITY";
    tag_types_[static_cast<uint8_t>(ContentType::DESCRIPTOR)] = "DESCRIPTOR";
    tag_types_[static_cast<uint8_t>(ContentType::STATIONNAME)] = "STATIONNAME";
    tag_types_[static_cast<uint8_t>(ContentType::PROGRAMME_TYPE)] = "PROGRAMME_TYPE";
    tag_types_[static_cast<uint8_t>(ContentType::VALIDITY)] = "VALIDITY";
    
    // Initialize ITEM subtypes (ETSI TS 102 980 Table 2)
    auto item_type = static_cast<uint8_t>(ContentType::ITEM);
    tag_subtypes_[{item_type, 0}] = "TITLE";
    tag_subtypes_[{item_type, 1}] = "ALBUM";
    tag_subtypes_[{item_type, 2}] = "TRACKNUMBER";
    tag_subtypes_[{item_type, 3}] = "ARTIST";
    tag_subtypes_[{item_type, 4}] = "COMPOSITION";
    tag_subtypes_[{item_type, 5}] = "MOVEMENT";
    tag_subtypes_[{item_type, 6}] = "CONDUCTOR";
    tag_subtypes_[{item_type, 7}] = "COMPOSER";
    tag_subtypes_[{item_type, 8}] = "BAND";
    tag_subtypes_[{item_type, 9}] = "COMMENT";
    tag_subtypes_[{item_type, 10}] = "GENRE";
    
    // Initialize INFO subtypes
    auto info_type = static_cast<uint8_t>(ContentType::INFO);
    tag_subtypes_[{info_type, 0}] = "NEWS";
    tag_subtypes_[{info_type, 1}] = "NEWS_LOCAL";
    tag_subtypes_[{info_type, 2}] = "STOCKMARKET";
    tag_subtypes_[{info_type, 3}] = "SPORT";
    tag_subtypes_[{info_type, 4}] = "LOTTERY";
    tag_subtypes_[{info_type, 5}] = "HOROSCOPE";
    tag_subtypes_[{info_type, 6}] = "DAILY_DIVERSION";
    tag_subtypes_[{info_type, 7}] = "HEALTH";
    tag_subtypes_[{info_type, 8}] = "EVENT";
    tag_subtypes_[{info_type, 9}] = "SCENE";
    tag_subtypes_[{info_type, 10}] = "CINEMA";
    tag_subtypes_[{info_type, 11}] = "TV";
    tag_subtypes_[{info_type, 12}] = "DATE_TIME";
    tag_subtypes_[{info_type, 13}] = "WEATHER";
    tag_subtypes_[{info_type, 14}] = "TRAFFIC";
    tag_subtypes_[{info_type, 15}] = "ALARM";
    
    // Initialize PROGRAMME subtypes
    auto prog_type = static_cast<uint8_t>(ContentType::PROGRAMME);
    tag_subtypes_[{prog_type, 0}] = "NOW";
    tag_subtypes_[{prog_type, 1}] = "NEXT";
    tag_subtypes_[{prog_type, 2}] = "PART";
    tag_subtypes_[{prog_type, 3}] = "HOST";
    tag_subtypes_[{prog_type, 4}] = "EDITORIAL_STAFF";
    tag_subtypes_[{prog_type, 5}] = "FREQUENCY";
    tag_subtypes_[{prog_type, 6}] = "HOMEPAGE";
    tag_subtypes_[{prog_type, 7}] = "SUBCHANNEL";
    
    // Initialize INTERACTIVITY subtypes
    auto inter_type = static_cast<uint8_t>(ContentType::INTERACTIVITY);
    tag_subtypes_[{inter_type, 0}] = "SMS";
    tag_subtypes_[{inter_type, 1}] = "MMS";
    tag_subtypes_[{inter_type, 2}] = "EMAIL";
    tag_subtypes_[{inter_type, 3}] = "PHONE_HOTLINE";
    tag_subtypes_[{inter_type, 4}] = "PHONE_STUDIO";
    tag_subtypes_[{inter_type, 5}] = "PHONE_OTHER";
    tag_subtypes_[{inter_type, 6}] = "FAX";
    tag_subtypes_[{inter_type, 7}] = "MMS_STUDIO";
    tag_subtypes_[{inter_type, 8}] = "SMS_STUDIO";
    
    // Initialize STATIONNAME subtypes
    auto station_type = static_cast<uint8_t>(ContentType::STATIONNAME);
    tag_subtypes_[{station_type, 0}] = "SHORT";
    tag_subtypes_[{station_type, 1}] = "LONG";
}

bool DLSPlusDecoder::processPADData(const QByteArray& pad_data) {
    total_messages_processed_++;
    
    if (pad_data.isEmpty()) {
        qWarning() << "DLS+: Empty PAD data";
        invalid_messages_count_++;
        return false;
    }
    
    // Check if PAD contains DLS+ data
    if (!isPADContainingDLSPlus(pad_data)) {
        // Not an error - just regular PAD without DLS+
        return false;
    }
    
    // Extract DLS+ data from PAD
    QByteArray dls_plus_data = extractDLSPlusData(pad_data);
    if (dls_plus_data.isEmpty()) {
        qWarning() << "DLS+: Failed to extract DLS+ data from PAD";
        invalid_messages_count_++;
        return false;
    }
    
    // Parse DLS+ message
    DLSPlusMessage message;
    if (!parseDLSPlusObject(dls_plus_data, message)) {
        qWarning() << "DLS+: Failed to parse DLS+ message";
        invalid_messages_count_++;
        return false;
    }
    
    // Update statistics
    valid_messages_count_++;
    
    // Update current message (thread-safe)
    {
        QMutexLocker locker(&mutex_);
        current_message_ = message;
    }
    
    // Emit signals for tag changes
    QString artist = message.getTag("ITEM.ARTIST");
    QString title = message.getTag("ITEM.TITLE");
    QString album = message.getTag("ITEM.ALBUM");
    QString programme = message.getTag("PROGRAMME.NOW");
    QString station_name = message.getTag("STATIONNAME.LONG");
    
    // Detect changes and emit signals
    if (artist != previous_artist_ || title != previous_title_ || album != previous_album_) {
        // Emit nowPlayingUpdated for main.cpp compatibility (track, artist, album)
        emit nowPlayingUpdated(title, artist, album);
        emit nowPlayingChanged(artist, title);
        previous_artist_ = artist;
        previous_title_ = title;
    }
    
    if (album != previous_album_) {
        emit albumChanged(album);
        previous_album_ = album;
    }
    
    if (programme != previous_programme_) {
        emit programmeChanged(programme);
        previous_programme_ = programme;
    }
    
    if (station_name != previous_station_name_) {
        emit stationNameChanged(station_name);
        previous_station_name_ = station_name;
    }
    
    // Emit main message signal
    emit dlsPlusMessageReceived(message);
    
    qInfo() << "DLS+: Message parsed successfully -" << message.getNowPlaying();
    
    return true;
}

bool DLSPlusDecoder::parseDLSPlusObject(const QByteArray& data, DLSPlusMessage& message) {
    // ETSI TS 102 980 Section 3: DLS+ Object Structure
    // Minimum size check (at least DLS text + command flag + tag count)
    if (data.size() < 3) {
        emit parseError("DLS+ object too short");
        return false;
    }
    
    // Extract DLS text and charset
    QString dls_text;
    uint8_t charset;
    if (!extractDLSText(data, dls_text, charset)) {
        emit parseError("Failed to extract DLS text");
        return false;
    }
    
    message.dls_text = dls_text;
    message.charset = charset;
    message.timestamp = QDateTime::currentDateTime();
    
    // Extract tags
    if (!extractTags(data, message.tags)) {
        emit parseError("Failed to extract DLS+ tags");
        return false;
    }
    
    // Validate and extract text for each tag
    for (auto& tag : message.tags) {
        QString text;
        if (extractTagText(tag, dls_text, text)) {
            tag.text = text;
        } else {
            qWarning() << "DLS+: Invalid tag bounds for" << tag.getFullTagName();
        }
    }
    
    message.is_valid = true;
    return true;
}

bool DLSPlusDecoder::extractDLSText(const QByteArray& data, QString& dls_text, uint8_t& charset) {
    // TODO: Implement DLS text extraction from PAD data
    // ETSI TS 102 980 Section 3.2: DLS Object
    // For now, stub implementation
    
    if (data.isEmpty()) {
        return false;
    }
    
    // Assume UTF-8 charset for now
    charset = 15;  // UTF-8
    dls_text = QString::fromUtf8(data);
    
    return true;
}

bool DLSPlusDecoder::extractTags(const QByteArray& data, QVector<DLSPlusTag>& tags) {
    // TODO: Implement DLS+ tag extraction
    // ETSI TS 102 980 Section 3.3: DLS+ Command
    // For now, stub implementation
    
    tags.clear();
    
    // Stub: Create sample tag for testing
    DLSPlusTag sample_tag;
    sample_tag.content_type = static_cast<uint8_t>(ContentType::ITEM);
    sample_tag.content_subtype = 3;  // ARTIST
    sample_tag.content_id = 0;
    sample_tag.start_marker = 0;
    sample_tag.length_marker = 10;
    
    tags.append(sample_tag);
    
    return true;
}

bool DLSPlusDecoder::validateTag(const DLSPlusTag& tag, const QString& dls_text) const {
    if (!tag.isValid()) {
        return false;
    }
    
    // Check bounds against DLS text
    if (tag.start_marker + tag.length_marker > dls_text.length()) {
        return false;
    }
    
    return true;
}

bool DLSPlusDecoder::extractTagText(const DLSPlusTag& tag, const QString& dls_text, QString& text) const {
    if (!validateTag(tag, dls_text)) {
        return false;
    }
    
    text = dls_text.mid(tag.start_marker, tag.length_marker);
    return true;
}

bool DLSPlusDecoder::isPADContainingDLSPlus(const QByteArray& pad_data) const {
    // TODO: Implement PAD type detection
    // Check PAD Application Type Identifier for DLS+
    // ETSI EN 300 401 Section 7.4.2.1
    
    if (pad_data.size() < 2) {
        return false;
    }
    
    // Stub: For now, assume any non-empty PAD might contain DLS+
    return true;
}

QByteArray DLSPlusDecoder::extractDLSPlusData(const QByteArray& pad_data) const {
    // TODO: Implement PAD data extraction
    // Extract DLS+ data from PAD based on Application Type
    // ETSI EN 300 401 Section 7.4
    
    // Stub: Return raw PAD data for now
    return pad_data;
}

QString DLSPlusDecoder::getTagTypeName(uint8_t type) const {
    return tag_types_.value(type, QString("UNKNOWN_%1").arg(type));
}

QString DLSPlusDecoder::getTagSubtypeName(uint8_t type, uint8_t subtype) const {
    return tag_subtypes_.value({type, subtype}, QString("UNKNOWN_%1_%2").arg(type).arg(subtype));
}

// Convenience methods
DLSPlusMessage DLSPlusDecoder::getCurrentMessage() const {
    QMutexLocker locker(&mutex_);
    return current_message_;
}

QString DLSPlusDecoder::getNowPlaying() const {
    QMutexLocker locker(&mutex_);
    return current_message_.getNowPlaying();
}

QString DLSPlusDecoder::getArtist() const {
    QMutexLocker locker(&mutex_);
    return current_message_.getTag("ITEM.ARTIST");
}

QString DLSPlusDecoder::getTitle() const {
    QMutexLocker locker(&mutex_);
    return current_message_.getTag("ITEM.TITLE");
}

QString DLSPlusDecoder::getAlbum() const {
    QMutexLocker locker(&mutex_);
    return current_message_.getTag("ITEM.ALBUM");
}

QString DLSPlusDecoder::getTrackNumber() const {
    QMutexLocker locker(&mutex_);
    return current_message_.getTag("ITEM.TRACKNUMBER");
}

QString DLSPlusDecoder::getProgrammeName() const {
    QMutexLocker locker(&mutex_);
    return current_message_.getTag("PROGRAMME.NOW");
}

QString DLSPlusDecoder::getNextProgramme() const {
    QMutexLocker locker(&mutex_);
    return current_message_.getTag("PROGRAMME.NEXT");
}

QString DLSPlusDecoder::getStationName() const {
    QMutexLocker locker(&mutex_);
    return current_message_.getTag("STATIONNAME.LONG");
}

QString DLSPlusDecoder::getNewsHeadline() const {
    QMutexLocker locker(&mutex_);
    return current_message_.getTag("INFO.NEWS");
}

uint64_t DLSPlusDecoder::getTotalMessagesProcessed() const {
    return total_messages_processed_;
}

uint64_t DLSPlusDecoder::getValidMessagesCount() const {
    return valid_messages_count_;
}

uint64_t DLSPlusDecoder::getInvalidMessagesCount() const {
    return invalid_messages_count_;
}

} // namespace dls_plus
} // namespace eti
