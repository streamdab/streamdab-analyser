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
    // ETSI TS 102 980 Section 3.2: DLS Object
    // DLS text is carried in PAD as a segment with charset indicator
    
    if (data.isEmpty() || data.size() < 2) {
        return false;
    }
    
    // First byte typically contains flags and charset
    // Bit 0-3: Charset (0=ISO 8859-1, 15=UTF-8, etc.)
    // ETSI EN 300 401 Table 4 - Character field
    charset = data[0] & 0x0F;
    
    // Extract text starting from byte 1
    QByteArray textData = data.mid(1);
    
    // Decode based on charset
    if (charset == 15) {
        // UTF-8
        dls_text = QString::fromUtf8(textData);
    } else if (charset == 0) {
        // ISO 8859-1 (Latin-1)
        dls_text = QString::fromLatin1(textData);
    } else if (charset == 6) {
        // ISO 8859-9 (Turkish)
        dls_text = QString::fromLatin1(textData);  // Close enough
    } else {
        // Default to UTF-8 for unknown charsets
        qWarning() << "DLS+: Unknown charset" << charset << "- defaulting to UTF-8";
        dls_text = QString::fromUtf8(textData);
    }
    
    // Remove null terminators and trim
    dls_text = dls_text.trimmed();
    int nullPos = dls_text.indexOf(QChar('\0'));
    if (nullPos >= 0) {
        dls_text = dls_text.left(nullPos);
    }
    
    return !dls_text.isEmpty();
}

bool DLSPlusDecoder::extractTags(const QByteArray& data, QVector<DLSPlusTag>& tags) {
    // ETSI TS 102 980 Section 3.3: DLS+ Command
    // DLS+ tags are encoded after the DLS text in a command structure
    
    tags.clear();
    
    if (data.size() < 4) {
        // Minimum: 1 byte charset + 1 byte text + 2 bytes for tag header
        return false;  // No tags, just DLS text
    }
    
    // Find DLS+ command marker
    // After DLS text, look for command byte
    // Command byte format: bit 7-5 = link bit, bit 4 = CRC flag, bit 3-0 = num tags
    
    // Skip DLS text to find command (simplified: look for non-text bytes)
    int commandPos = 1;  // Start after charset byte
    
    // Find end of text (null terminator or specific pattern)
    while (commandPos < data.size() && data[commandPos] != 0x00) {
        commandPos++;
    }
    
    // Check if we have command data after text
    if (commandPos + 2 >= data.size()) {
        return false;  // No command data
    }
    
    // Move past null terminator
    commandPos++;
    
    // Read command byte
    uint8_t commandByte = static_cast<uint8_t>(data[commandPos]);
    uint8_t numTags = commandByte & 0x0F;  // Bits 0-3: number of tags
    bool hasCRC = (commandByte & 0x10) != 0;  // Bit 4: CRC present
    
    if (numTags == 0 || numTags > 4) {
        // ETSI TS 102 980: Maximum 4 tags per message
        qDebug() << "DLS+: Invalid tag count:" << numTags;
        return false;
    }
    
    commandPos++;  // Move to first tag
    
    // Parse each tag (each tag is 4 bytes)
    for (int i = 0; i < numTags && commandPos + 4 <= data.size(); i++) {
        DLSPlusTag tag;
        
        // Byte 0-1: Content type (4 bits) + subtype (4 bits) + content ID (12 bits)
        uint16_t word1 = (static_cast<uint8_t>(data[commandPos]) << 8) | 
                         static_cast<uint8_t>(data[commandPos + 1]);
        
        tag.content_type = (word1 >> 12) & 0x0F;  // Bits 12-15
        tag.content_subtype = (word1 >> 8) & 0x0F;  // Bits 8-11
        tag.content_id = word1 & 0x0FFF;  // Bits 0-11 (12-bit content ID)
        
        // Byte 2: Start marker (7 bits) + length marker high bit
        uint8_t byte2 = static_cast<uint8_t>(data[commandPos + 2]);
        tag.start_marker = (byte2 >> 1) & 0x7F;  // Bits 1-7
        uint8_t lengthHigh = byte2 & 0x01;  // Bit 0
        
        // Byte 3: Length marker low bits (6 bits)
        uint8_t byte3 = static_cast<uint8_t>(data[commandPos + 3]);
        tag.length_marker = ((lengthHigh << 6) | ((byte3 >> 2) & 0x3F));  // 7-bit length
        
        // Validate tag
        if (tag.isValid()) {
            tags.append(tag);
            qDebug() << "DLS+: Parsed tag" << i << ":" << tag.getFullTagName()
                     << "start=" << tag.start_marker << "len=" << tag.length_marker;
        } else {
            qWarning() << "DLS+: Invalid tag" << i << "- skipping";
        }
        
        commandPos += 4;  // Move to next tag (4 bytes per tag)
    }
    
    return !tags.isEmpty();
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
    // ETSI EN 300 401 Section 7.4.2.1: PAD Application Type
    // Check PAD Application Type Identifier for DLS/DLS+
    
    if (pad_data.size() < 2) {
        return false;
    }
    
    // PAD starts with CI (Content Indicator) byte
    // For X-PAD: Check for DLS application type
    // DLS/DLS+ uses Application Type = 2 or 3
    
    uint8_t firstByte = static_cast<uint8_t>(pad_data[0]);
    
    // Check for X-PAD indicator (bit 7 = 0 for F-PAD, 1 for X-PAD)
    bool isXPAD = (firstByte & 0x80) != 0;
    
    if (!isXPAD) {
        // F-PAD (Fixed PAD) - simple case
        // Check if it looks like DLS text (printable characters)
        return true;  // Assume F-PAD can contain DLS
    }
    
    // X-PAD (Extended PAD) - check application type
    // Byte 0: CI flag + length indicator
    // Following bytes: Data Group or PAD data
    
    // Look for DLS+ command structure
    // DLS+ has specific markers: text followed by command byte
    
    // Check if data contains null-terminated text followed by command
    int nullPos = pad_data.indexOf('\0');
    if (nullPos > 0 && nullPos + 2 < pad_data.size()) {
        // Found potential text + command structure
        uint8_t commandByte = static_cast<uint8_t>(pad_data[nullPos + 1]);
        uint8_t numTags = commandByte & 0x0F;
        
        // DLS+ typically has 1-4 tags
        if (numTags >= 1 && numTags <= 4) {
            return true;  // Likely DLS+
        }
    }
    
    // Check for printable text (indicates DLS/DLS+)
    int printableCount = 0;
    for (int i = 0; i < qMin(20, pad_data.size()); i++) {
        uint8_t byte = static_cast<uint8_t>(pad_data[i]);
        if ((byte >= 0x20 && byte <= 0x7E) || byte >= 0x80) {
            printableCount++;
        }
    }
    
    // If > 50% printable, likely contains DLS text
    return printableCount > (qMin(20, pad_data.size()) / 2);
}

QByteArray DLSPlusDecoder::extractDLSPlusData(const QByteArray& pad_data) const {
    // ETSI EN 300 401 Section 7.4: PAD Data Extraction
    // Extract DLS+ data from PAD based on Application Type
    
    if (pad_data.isEmpty()) {
        return QByteArray();
    }
    
    uint8_t firstByte = static_cast<uint8_t>(pad_data[0]);
    bool isXPAD = (firstByte & 0x80) != 0;
    
    if (!isXPAD) {
        // F-PAD (Fixed PAD) - return as-is
        return pad_data;
    }
    
    // X-PAD (Extended PAD) - need to extract payload
    // Byte 0: CI (Content Indicator)
    //   Bit 7: X-PAD indicator (1)
    //   Bit 6-4: Length indicator
    //   Bit 3-0: Application type / first nibble
    
    uint8_t lengthIndicator = (firstByte >> 4) & 0x07;
    
    // Skip CI byte and extract payload
    if (pad_data.size() > 1) {
        // Return payload (skip CI byte)
        return pad_data.mid(1);
    }
    
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
