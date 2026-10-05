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
        case ContentType::STATIONNAME: return "STATIONNAME";
        case ContentType::PROGRAMME_TYPE: return "PROGRAMME_TYPE";
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
    , has_current_message_(false)
    , total_messages_processed_(0)
    , valid_messages_count_(0)
    , invalid_messages_count_(0)
    , tags_extracted_count_(0)
{
    initializeTagLookupTables();
}

DLSPlusDecoder::~DLSPlusDecoder() = default;

void DLSPlusDecoder::initializeTagLookupTables() {
    // Initialize content type names (internal type values, not tag codes)
    tag_types_[static_cast<uint8_t>(ContentType::DUMMY)] = "DUMMY";
    tag_types_[static_cast<uint8_t>(ContentType::ITEM)] = "ITEM";
    tag_types_[static_cast<uint8_t>(ContentType::INFO)] = "INFO";
    tag_types_[static_cast<uint8_t>(ContentType::PROGRAMME)] = "PROGRAMME";
    tag_types_[static_cast<uint8_t>(ContentType::INTERACTIVITY)] = "INTERACTIVITY";
    tag_types_[static_cast<uint8_t>(ContentType::STATIONNAME)] = "STATIONNAME";
    tag_types_[static_cast<uint8_t>(ContentType::PROGRAMME_TYPE)] = "PROGRAMME_TYPE";
    
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
        has_current_message_ = true;
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
    
    // Emit main message signal (with service_id = 0 for compatibility)
    emit dlsPlusMessageReceived(0, message);
    
    qInfo() << "DLS+: Message parsed successfully -" << message.getNowPlaying();
    
    return true;
}

bool DLSPlusDecoder::processDLSPlusCommand(const QByteArray& dls_text, const QByteArray& command_data) {
    // Alternative processing method for testing
    // Combines DLS text and command data into a single message
    
    total_messages_processed_++;
    
    DLSPlusMessage message;
    message.timestamp = QDateTime::currentDateTime();
    message.charset = 15; // UTF-8
    
    // Extract DLS text
    message.dls_text = QString::fromUtf8(dls_text);
    if (message.dls_text.isEmpty()) {
        emit dlsPlusDecodingError("Empty DLS text");
        invalid_messages_count_++;
        return false;
    }
    
    // Parse tags from command data
    if (!command_data.isEmpty()) {
        if (!parseCommandData(command_data, message.dls_text, message.tags)) {
            emit dlsPlusDecodingError("Failed to parse command data");
            invalid_messages_count_++;
            return false;
        }
    }
    
    // Copy tags to descriptors for test compatibility
    message.descriptors = message.tags;
    message.is_valid = true;
    valid_messages_count_++;
    tags_extracted_count_ += message.tags.size();
    
    // Update current message (thread-safe)
    {
        QMutexLocker locker(&mutex_);
        current_message_ = message;
        has_current_message_ = true;
    }
    
    // Emit signals for tag changes
    QString artist = message.getTag("ITEM.ARTIST");
    QString title = message.getTag("ITEM.TITLE");
    QString album = message.getTag("ITEM.ALBUM");
    QString programme = message.getTag("PROGRAMME.NOW");
    QString station_name = message.getTag("STATIONNAME.LONG");
    
    // Detect changes and emit signals
    if (artist != previous_artist_ || title != previous_title_ || album != previous_album_) {
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
    
    // Emit main message signal (with service_id = 0 for compatibility)
    emit dlsPlusMessageReceived(0, message);
    
    return true;
}

bool DLSPlusDecoder::parseCommandData(const QByteArray& command_data, const QString& dls_text, QVector<DLSPlusTag>& tags) {
    // Parse DLS+ command data (4 bytes per tag)
    // ETSI TS 102 980 Section 4: Content Descriptor Format
    
    tags.clear();
    
    if (command_data.isEmpty()) {
        return true; // No tags, but not an error
    }
    
    if (command_data.size() % 4 != 0) {
        qWarning() << "DLS+: Command data size not multiple of 4:" << command_data.size();
        return false;
    }
    
    int num_tags = command_data.size() / 4;
    if (num_tags > 8) {  // ETSI TS 102 980 allows up to 8 content descriptors
        qWarning() << "DLS+: Too many tags:" << num_tags;
        return false;
    }
    
    for (int i = 0; i < num_tags; i++) {
        int offset = i * 4;
        DLSPlusTag tag;
        
        // Parse 4-byte descriptor
        // Byte 0: Content type (6 bits MSB) | Content ID MSB (2 bits)
        uint8_t byte0 = static_cast<uint8_t>(command_data[offset]);
        tag.content_type = (byte0 >> 2) & 0x3F;
        uint16_t content_id_msb = byte0 & 0x03;
        
        // Byte 1: Content ID LSB (5 bits) | Start marker MSB (3 bits)
        uint8_t byte1 = static_cast<uint8_t>(command_data[offset + 1]);
        uint16_t content_id_lsb = (byte1 >> 3) & 0x1F;
        tag.content_id = (content_id_msb << 5) | content_id_lsb;
        uint8_t start_msb = byte1 & 0x07;
        
        // Byte 2: Start marker LSB (3 bits) | Length marker (5 bits)
        uint8_t byte2 = static_cast<uint8_t>(command_data[offset + 2]);
        uint8_t start_lsb = (byte2 >> 5) & 0x07;
        tag.start_marker = (start_msb << 3) | start_lsb;
        tag.length_marker = byte2 & 0x1F;
        
        // Byte 3: Toggle bit (1 bit) | Reserved (7 bits)
        // uint8_t byte3 = static_cast<uint8_t>(command_data[offset + 3]);
        // Not used for basic parsing
        
        // Map 6-bit tag code to content_type and content_subtype per ETSI TS 102 980 Table 2
        // Tag codes 1-31: ITEM (content_type=1, subtypes 0-30)
        // Tag codes 32-47: INFO (content_type=2, subtypes 0-15)
        // Tag codes 48-55: PROGRAMME (content_type=3, subtypes 0-7)
        // Tag codes 56-59: INTERACTIVITY (content_type=4, subtypes 0-3)
        // Tag codes 60-61: STATIONNAME (content_type=8, subtypes 0-1)
        // Tag codes 62: PROGRAMME_TYPE (content_type=9)
        uint8_t tag_code = tag.content_type;  // 6-bit value
        
        if (tag_code >= 1 && tag_code <= 31) {
            tag.content_type = static_cast<uint8_t>(ContentType::ITEM);
            tag.content_subtype = tag_code - 1;  // 0-30 (TITLE=0, ALBUM=1, TRACKNUMBER=2, ARTIST=3, etc.)
        } else if (tag_code >= 32 && tag_code <= 47) {
            tag.content_type = static_cast<uint8_t>(ContentType::INFO);
            tag.content_subtype = tag_code - 32;  // 0-15 (NEWS=0, NEWS_LOCAL=1, etc.)
        } else if (tag_code >= 48 && tag_code <= 55) {
            tag.content_type = static_cast<uint8_t>(ContentType::PROGRAMME);
            tag.content_subtype = tag_code - 48;  // 0-7 (NOW=0, NEXT=1, etc.)
        } else if (tag_code >= 56 && tag_code <= 59) {
            tag.content_type = static_cast<uint8_t>(ContentType::INTERACTIVITY);
            tag.content_subtype = tag_code - 56;  // 0-3 (SMS=0, MMS=1, etc.)
        } else if (tag_code >= 60 && tag_code <= 61) {
            tag.content_type = static_cast<uint8_t>(ContentType::STATIONNAME);
            tag.content_subtype = tag_code - 60;  // 0-1 (SHORT=0, LONG=1)
        } else if (tag_code == 62) {
            tag.content_type = static_cast<uint8_t>(ContentType::PROGRAMME_TYPE);
            tag.content_subtype = 0;
        } else {
            // Unknown tag code - treat as DUMMY
            tag.content_type = static_cast<uint8_t>(ContentType::DUMMY);
            tag.content_subtype = 0;
        }
        
        // Extract text from DLS using markers
        if (extractTagText(tag, dls_text, tag.text)) {
            tags.append(tag);
            qDebug() << "DLS+: Parsed tag" << tag.getFullTagName() 
                     << "start=" << tag.start_marker << "len=" << tag.length_marker
                     << "text=" << tag.text;
        } else {
            qWarning() << "DLS+: Failed to extract text for tag" << tag.getFullTagName();
        }
    }
    
    return true;
}

bool DLSPlusDecoder::parseDLSPlusObject(const QByteArray& data, DLSPlusMessage& message) {
    // ETSI TS 102 980 Section 3: DLS+ Object Structure
    // Minimum size check (at least DLS text + command flag + tag count)
    if (data.size() < 3) {
        emit parseError("DLS+ object too short");
        emit dlsPlusDecodingError("DLS+ object too short");
        return false;
    }
    
    // Extract DLS text and charset
    QString dls_text;
    uint8_t charset;
    if (!extractDLSText(data, dls_text, charset)) {
        emit parseError("Failed to extract DLS text");
        emit dlsPlusDecodingError("Failed to extract DLS text");
        return false;
    }
    
    message.dls_text = dls_text;
    message.charset = charset;
    message.timestamp = QDateTime::currentDateTime();
    
    // Extract tags
    if (!extractTags(data, message.tags)) {
        emit parseError("Failed to extract DLS+ tags");
        emit dlsPlusDecodingError("Failed to extract DLS+ tags");
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
    
    // Copy tags to descriptors for compatibility
    message.descriptors = message.tags;
    message.is_valid = true;
    tags_extracted_count_ += message.tags.size();
    
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
    // ETSI TS 102 980: Markers are BYTE offsets in UTF-8, not character positions
    QByteArray utf8_bytes = dls_text.toUtf8();
    
    // FIX MEDIUM-004: Prevent integer overflow by using size_t for calculation
    // Cast to size_t BEFORE addition to prevent uint8_t overflow
    const size_t start_pos = static_cast<size_t>(tag.start_marker);
    const size_t length = static_cast<size_t>(tag.length_marker);
    const size_t end_pos = start_pos + length;
    const size_t text_size = static_cast<size_t>(utf8_bytes.size());
    
    // Validate byte-level bounds (now overflow-safe)
    if (end_pos > text_size) {
        qWarning() << "DLS+: Tag bounds exceed text size:"
                   << "start=" << tag.start_marker
                   << "len=" << tag.length_marker
                   << "end=" << end_pos
                   << "text_bytes=" << utf8_bytes.size();
        return false;
    }
    
    // Additional check: ensure start position is valid
    if (start_pos >= text_size) {
        qWarning() << "DLS+: Tag start position beyond text:"
                   << "start=" << tag.start_marker
                   << "text_bytes=" << utf8_bytes.size();
        return false;
    }
    
    // Extract bytes and convert to QString
    QByteArray extracted_bytes = utf8_bytes.mid(static_cast<int>(start_pos), 
                                                static_cast<int>(length));
    text = QString::fromUtf8(extracted_bytes);
    
    return !text.isEmpty();
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

std::optional<DLSPlusMessage> DLSPlusDecoder::getCurrentMessageOptional() const {
    QMutexLocker locker(&mutex_);
    if (has_current_message_) {
        return current_message_;
    }
    return std::nullopt;
}

bool DLSPlusDecoder::hasCurrentMessage() const {
    QMutexLocker locker(&mutex_);
    return has_current_message_;
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

DLSPlusDecoder::Statistics DLSPlusDecoder::getStatistics() const {
    Statistics stats;
    stats.messages_processed = static_cast<uint32_t>(total_messages_processed_);
    stats.tags_extracted = tags_extracted_count_;
    stats.parsing_errors = static_cast<uint32_t>(invalid_messages_count_);
    return stats;
}

void DLSPlusDecoder::resetStatistics() {
    QMutexLocker locker(&mutex_);
    total_messages_processed_ = 0;
    valid_messages_count_ = 0;
    invalid_messages_count_ = 0;
    tags_extracted_count_ = 0;
    has_current_message_ = false;
}

void DLSPlusDecoder::resetHistory() {
    QMutexLocker locker(&mutex_);
    // Clear the change-detection baselines AND the current-message flag so the
    // next decoded label always emits (even if byte-identical to the previous
    // capture's last values). current_message_'s contents are left in place but
    // are no longer reported as current via hasCurrentMessage().
    previous_artist_.clear();
    previous_title_.clear();
    previous_album_.clear();
    previous_programme_.clear();
    previous_station_name_.clear();
    has_current_message_ = false;
}

} // namespace dls_plus
} // namespace eti
