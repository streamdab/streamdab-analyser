/**
 * @file dls_plus_decoder.hpp
 * @brief DLS+ (Dynamic Label Plus) Decoder - ETSI TS 102 980
 *
 * Implements DLS+ structured metadata parsing for DAB/DAB+ "now playing"
 * information with support for multiple content tags.
 *
 * ETSI TS 102 980 Section 3: DLS+ provides structured metadata tags:
 * - ITEM (0x01): Track/Song title
 * - TITLE (0x02): Program title
 * - ALBUM (0x03): Album name
 * - ARTIST (0x04): Artist/Performer
 * - COMPOSITION (0x05): Composer
 * - COMMENT (0x06): General comment
 * - GENRE (0x07): Genre/Category
 * - NEWS (0x08): News content
 * - STOCK_MARKET (0x09): Stock ticker
 * - SPORT (0x0A): Sports info
 * - WEATHER (0x0B): Weather data
 * - URL (0x0C): Web link
 * - PHONE_NUMBER (0x0D): Phone hotline
 * - EMAIL (0x0E): Email address
 *
 * DLS+ Message Structure (ETSI TS 102 980 Section 4):
 * - Base DLS text (up to 128 characters, UTF-8)
 * - Content descriptors (4 bytes each):
 *   Byte 0: Tag type (6 bits) + Content ID MSB (2 bits)
 *   Byte 1: Content ID LSB (5 bits) + Start marker MSB (3 bits)
 *   Byte 2: Start marker LSB (3 bits) + Length marker (5 bits)
 *   Byte 3: Toggle bit + Reserved bits
 *
 * Example DLS+ Message:
 *   DLS Text: "Artist Name - Track Title [Album]"
 *   Descriptors:
 *   - ARTIST tag: start=0, length=11 -> "Artist Name"
 *   - ITEM tag: start=14, length=11 -> "Track Title"
 *   - ALBUM tag: start=27, length=5 -> "Album"
 *
 * Integration with Phase 3A:
 * - Works with DAB+ Audio Stream Validator (dabplus_stream_validator.cpp)
 * - Extracts DLS+ from PAD (Program Associated Data) in audio superframe
 * - Complements EPG/Journaline for complete metadata coverage
 *
 * @see ETSI TS 102 980 - Dynamic Label Plus (DLS+)
 * @see ETSI EN 300 401 - DAB System (DLS baseline)
 * @see ETSI TS 102 563 - DAB+ Audio (PAD structure)
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
#include <QVector>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>
#include <cstdint>
#include <optional>

namespace eti::dlsplus {

/**
 * @brief DLS+ Content Tag Types (ETSI TS 102 980 Section 3.1)
 *
 * Defines structured content types for DLS+ metadata.
 * Each tag identifies a specific type of information within the DLS text.
 */
enum class DLSPlusTag : uint8_t {
    DUMMY = 0x00,           ///< Dummy/Placeholder (no content)
    ITEM = 0x01,            ///< Track/Song title (now playing item)
    TITLE = 0x02,           ///< Program title (show/broadcast name)
    ALBUM = 0x03,           ///< Album name
    ARTIST = 0x04,          ///< Artist/Performer name
    COMPOSITION = 0x05,     ///< Composer name
    COMMENT = 0x06,         ///< General comment text
    GENRE = 0x07,           ///< Genre/Category
    NEWS = 0x08,            ///< News content
    STOCK_MARKET = 0x09,    ///< Stock market ticker
    SPORT = 0x0A,           ///< Sports information
    WEATHER = 0x0B,         ///< Weather information
    URL = 0x0C,             ///< Web URL
    PHONE_NUMBER = 0x0D,    ///< Phone hotline number
    EMAIL = 0x0E,           ///< Email address
    
    // Additional ETSI TS 102 980 Section 3.1 tags
    SMS_NUMBER = 0x0F,      ///< SMS number
    PURCHASE = 0x10,        ///< Purchase information
    GET_DATA = 0x11,        ///< Data retrieval info
    
    // Extended tags (0x20-0x3F reserved for future)
    DESCRIPTOR_PLACE = 0x20, ///< Place/location descriptor
    DESCRIPTOR_APPOINTMENT = 0x21, ///< Appointment/event time
    DESCRIPTOR_IDENTIFIER = 0x22,  ///< Identifier/reference
    
    UNKNOWN = 0xFF          ///< Unknown/unsupported tag
};

/**
 * @brief Convert DLS+ tag to human-readable string
 * @param tag DLS+ tag type
 * @return String representation (e.g., "ITEM", "ARTIST", "ALBUM")
 */
QString dlsTagToString(DLSPlusTag tag);

/**
 * @brief DLS+ Content Descriptor Structure
 *
 * ETSI TS 102 980 Section 4: Each tag has content identifier and positional markers.
 * The descriptor points to a substring within the DLS text using start/length markers.
 *
 * Content Descriptor Encoding (4 bytes):
 * - Byte 0: [Tag Type 6 bits][Content ID MSB 2 bits]
 * - Byte 1: [Content ID LSB 5 bits][Start Marker MSB 3 bits]
 * - Byte 2: [Start Marker LSB 3 bits][Length Marker 5 bits]
 * - Byte 3: [Toggle bit][Reserved 7 bits]
 *
 * Example:
 *   Tag=ARTIST (0x04), ContentID=0, Start=0, Length=11
 *   DLS Text: "Artist Name - Track Title"
 *   Extracted: "Artist Name" (characters 0-10)
 */
struct DLSPlusContentDescriptor {
    DLSPlusTag tag = DLSPlusTag::UNKNOWN;  ///< Content tag type
    uint8_t content_id = 0;                ///< Content identifier (0-127)
    uint8_t start_marker = 0;              ///< Start position in DLS text (0-127)
    uint8_t length_marker = 0;             ///< Length in DLS text (0-31)
    bool toggle_bit = false;               ///< Toggle bit for content change detection
    QString extracted_text;                 ///< Extracted text for this tag
    
    /**
     * @brief Check if descriptor is valid
     * @return true if tag is valid and length > 0
     */
    bool isValid() const {
        return tag != DLSPlusTag::UNKNOWN && 
               tag != DLSPlusTag::DUMMY &&
               length_marker > 0;
    }
    
    /**
     * @brief Get string representation of descriptor
     * @return Formatted descriptor info
     */
    QString toString() const;
};

/**
 * @brief Complete DLS+ Message Structure
 *
 * Represents a complete DLS+ message with base text and structured tags.
 * Each message contains the DLS text and multiple content descriptors
 * pointing to specific substrings within that text.
 *
 * Example:
 *   dls_text = "Rock Station - The Rolling Stones - Paint It Black"
 *   descriptors[0] = {TITLE, "Rock Station"}
 *   descriptors[1] = {ARTIST, "The Rolling Stones"}
 *   descriptors[2] = {ITEM, "Paint It Black"}
 */
struct DLSPlusMessage {
    QString dls_text;                               ///< Base DLS text (128 chars max)
    QVector<DLSPlusContentDescriptor> descriptors;  ///< Tag descriptors
    QDateTime timestamp;                            ///< Reception timestamp
    uint32_t service_id = 0;                        ///< DAB service ID
    
    /**
     * @brief Get ITEM tag content (track/song title)
     * @return Track title or empty string if not found
     */
    QString getItem() const;
    
    /**
     * @brief Get ARTIST tag content
     * @return Artist name or empty string if not found
     */
    QString getArtist() const;
    
    /**
     * @brief Get ALBUM tag content
     * @return Album name or empty string if not found
     */
    QString getAlbum() const;
    
    /**
     * @brief Get TITLE tag content (program title)
     * @return Program title or empty string if not found
     */
    QString getTitle() const;
    
    /**
     * @brief Get GENRE tag content
     * @return Genre string or empty string if not found
     */
    QString getGenre() const;
    
    /**
     * @brief Get COMMENT tag content
     * @return Comment text or empty string if not found
     */
    QString getComment() const;
    
    /**
     * @brief Get URL tag content
     * @return URL string or empty string if not found
     */
    QString getURL() const;
    
    /**
     * @brief Get text for specific tag type
     * @param tag Tag type to search for
     * @return Extracted text or empty string if not found
     */
    QString getTagText(DLSPlusTag tag) const;
    
    /**
     * @brief Check if message is valid
     * @return true if DLS text and descriptors are present
     */
    bool isValid() const {
        return !dls_text.isEmpty() && !descriptors.isEmpty();
    }
    
    /**
     * @brief Get string representation of entire message
     * @return Formatted message with all tags
     */
    QString toString() const;
};

/**
 * @brief DLS+ Decoder Class
 *
 * Professional DLS+ decoder with ETSI TS 102 980 compliance.
 *
 * Features:
 * - Complete tag parsing (15+ tag types per ETSI TS 102 980)
 * - Text extraction with marker-based positioning
 * - UTF-8 support (Thai, international characters)
 * - Thread-safe operation with QMutex
 * - Qt signal/slot integration for real-time updates
 * - Statistics tracking (messages, tags, errors)
 * - PAD (Program Associated Data) extraction
 * - Content change detection with toggle bits
 *
 * DLS+ Processing Flow:
 * 1. Receive PAD data from DAB+ audio superframe
 * 2. Extract DLS text (up to 128 characters)
 * 3. Extract DLS+ command with content descriptors
 * 4. Parse descriptors to get tag type and position markers
 * 5. Extract text substrings for each tag
 * 6. Emit signals with complete "now playing" information
 *
 * Usage Example:
 * @code
 *   DLSPlusDecoder decoder;
 *   
 *   // Connect to receive "now playing" updates
 *   connect(&decoder, &DLSPlusDecoder::dlsPlusMessageReceived,
 *           this, &MyClass::handleNowPlaying);
 *   
 *   connect(&decoder, &DLSPlusDecoder::nowPlayingUpdated,
 *           this, &MyClass::updateDisplay);
 *   
 *   // Process DLS+ from PAD (Program Associated Data)
 *   // PAD is extracted from DAB+ audio superframe
 *   decoder.processPADData(pad_bytes);
 *   
 *   // Access current "now playing" information
 *   auto current = decoder.getCurrentMessage();
 *   if (current.has_value()) {
 *       QString track = current->getItem();    // "Paint It Black"
 *       QString artist = current->getArtist(); // "The Rolling Stones"
 *       QString album = current->getAlbum();   // "Aftermath"
 *       qDebug() << "Now Playing:" << artist << "-" << track;
 *   }
 *   
 *   // Get statistics
 *   auto stats = decoder.getStatistics();
 *   qDebug() << "Processed" << stats.messages_processed << "DLS+ messages";
 *   qDebug() << "Extracted" << stats.tags_extracted << "tags total";
 * @endcode
 *
 * Thread Safety:
 * - All public methods are thread-safe (QMutex protection)
 * - Signals are emitted after mutex release (no deadlock risk)
 * - Safe for use from multiple threads (e.g., audio processing thread)
 *
 * Integration with Phase 3A:
 * - Works with dabplus_stream_validator.cpp for audio PAD extraction
 * - Complements EPG decoder for complete metadata coverage
 * - Integrates with MOT protocol for slideshow + "now playing" sync
 */
class DLSPlusDecoder : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool hasCurrentMessage READ hasCurrentMessage NOTIFY currentMessageChanged)
    Q_PROPERTY(QString currentTrack READ currentTrack NOTIFY currentMessageChanged)
    Q_PROPERTY(QString currentArtist READ currentArtist NOTIFY currentMessageChanged)
    Q_PROPERTY(QString currentAlbum READ currentAlbum NOTIFY currentMessageChanged)
    
public:
    /**
     * @brief Construct DLS+ decoder
     * @param parent Parent QObject (for Qt memory management)
     */
    explicit DLSPlusDecoder(QObject* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~DLSPlusDecoder() override;
    
    // Main processing interface
    
    /**
     * @brief Process PAD (Program Associated Data) containing DLS+
     * 
     * Extracts DLS text and DLS+ command from PAD bytes.
     * PAD is typically extracted from DAB+ audio superframe.
     * 
     * ETSI TS 102 563 Section 5: PAD structure in DAB+ audio
     * 
     * @param pad_data PAD bytes from audio superframe
     * @return true if DLS+ message successfully decoded
     */
    bool processPADData(const QByteArray& pad_data);
    
    /**
     * @brief Process DLS+ command with explicit DLS text and command data
     * 
     * Direct processing when DLS text and command are already extracted.
     * 
     * ETSI TS 102 980 Section 4: DLS+ command structure
     * 
     * @param dls_text Base DLS text (up to 128 characters)
     * @param command_data DLS+ command bytes (4 bytes per descriptor)
     * @return true if message successfully decoded
     */
    bool processDLSPlusCommand(const QByteArray& dls_text, const QByteArray& command_data);
    
    // Current message access
    
    /**
     * @brief Get current DLS+ message
     * @return Current message or std::nullopt if no message
     */
    std::optional<DLSPlusMessage> getCurrentMessage() const;
    
    /**
     * @brief Check if current message exists
     * @return true if current message is available
     */
    bool hasCurrentMessage() const;
    
    /**
     * @brief Get current track title (ITEM tag)
     * @return Track title or empty string
     */
    QString currentTrack() const;
    
    /**
     * @brief Get current artist name (ARTIST tag)
     * @return Artist name or empty string
     */
    QString currentArtist() const;
    
    /**
     * @brief Get current album name (ALBUM tag)
     * @return Album name or empty string
     */
    QString currentAlbum() const;
    
    /**
     * @brief Get current program title (TITLE tag)
     * @return Program title or empty string
     */
    QString currentProgramTitle() const;
    
    // Statistics
    
    /**
     * @brief DLS+ decoder statistics
     */
    struct Statistics {
        uint32_t messages_processed = 0;   ///< Total DLS+ messages processed
        uint32_t tags_extracted = 0;       ///< Total tags extracted
        uint32_t parsing_errors = 0;       ///< Parsing errors encountered
        uint32_t text_updates = 0;         ///< DLS text updates
        uint32_t pad_packets_processed = 0; ///< PAD packets processed
        uint32_t invalid_descriptors = 0;  ///< Invalid descriptors skipped
        
        /**
         * @brief Reset all statistics to zero
         */
        void reset() {
            messages_processed = 0;
            tags_extracted = 0;
            parsing_errors = 0;
            text_updates = 0;
            pad_packets_processed = 0;
            invalid_descriptors = 0;
        }
    };
    
    /**
     * @brief Get decoder statistics
     * @return Current statistics (thread-safe copy)
     */
    Statistics getStatistics() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStatistics();
    
    /**
     * @brief Clear current message
     */
    void clearCurrent();
    
signals:
    /**
     * @brief Emitted when complete DLS+ message is received
     * @param service_id DAB service ID
     * @param message Complete DLS+ message with all tags
     */
    void dlsPlusMessageReceived(uint32_t service_id, const DLSPlusMessage& message);
    
    /**
     * @brief Emitted when current message changes
     */
    void currentMessageChanged();
    
    /**
     * @brief Emitted when "now playing" information updates
     * @param track Track/item title
     * @param artist Artist name
     * @param album Album name
     */
    void nowPlayingUpdated(const QString& track, const QString& artist, const QString& album);
    
    /**
     * @brief Emitted when decoding error occurs
     * @param error Error description
     */
    void dlsPlusDecodingError(const QString& error);
    
private:
    // ETSI TS 102 980 parsing methods
    
    /**
     * @brief Parse DLS+ command structure
     * @param dls_text Base DLS text
     * @param command_data DLS+ command bytes
     * @param message Output message structure
     * @return true if parsing successful
     */
    bool parseDLSPlusCommand(const QByteArray& dls_text, const QByteArray& command_data, 
                             DLSPlusMessage& message);
    
    /**
     * @brief Parse content descriptors from command data
     * @param command_data DLS+ command bytes (4 bytes per descriptor)
     * @return Vector of parsed descriptors
     */
    QVector<DLSPlusContentDescriptor> parseContentDescriptors(const QByteArray& command_data);
    
    /**
     * @brief Parse single content descriptor (4 bytes)
     * @param data Pointer to descriptor bytes
     * @param offset Offset in buffer (updated after parsing)
     * @return Parsed descriptor
     */
    DLSPlusContentDescriptor parseDescriptor(const uint8_t* data, int& offset);
    
    /**
     * @brief Extract text for tag using position markers
     * @param dls_text Base DLS text
     * @param descriptor Descriptor with start/length markers (updated with text)
     */
    void extractTagText(const QString& dls_text, DLSPlusContentDescriptor& descriptor);
    
    /**
     * @brief Parse tag type from byte
     * @param tag_byte Tag byte (6 bits tag + 2 bits content ID MSB)
     * @return DLS+ tag type
     */
    DLSPlusTag parseTagType(uint8_t tag_byte) const;
    
    // PAD data extraction
    
    /**
     * @brief Extract DLS text from PAD data
     * @param pad_data PAD bytes
     * @param dls_text Output DLS text
     * @return true if DLS text extracted
     */
    bool extractDLSFromPAD(const QByteArray& pad_data, QByteArray& dls_text);
    
    /**
     * @brief Extract DLS+ command from PAD data
     * @param pad_data PAD bytes
     * @param command_data Output DLS+ command
     * @return true if command extracted
     */
    bool extractCommandFromPAD(const QByteArray& pad_data, QByteArray& command_data);
    
    // Validation
    
    /**
     * @brief Validate DLS text
     * @param text DLS text
     * @return true if valid (length, UTF-8)
     */
    bool validateDLSText(const QString& text) const;
    
    /**
     * @brief Validate content descriptor
     * @param descriptor Descriptor to validate
     * @return true if valid (tag, markers)
     */
    bool validateDescriptor(const DLSPlusContentDescriptor& descriptor) const;
    
    // Data members
    std::optional<DLSPlusMessage> m_current_message; ///< Current DLS+ message
    Statistics m_statistics;                         ///< Decoder statistics
    mutable QMutex m_mutex;                         ///< Thread safety mutex
};

} // namespace eti::dlsplus

// Qt metatype registration for signal/slot usage
Q_DECLARE_METATYPE(eti::dlsplus::DLSPlusMessage)
Q_DECLARE_METATYPE(eti::dlsplus::DLSPlusContentDescriptor)
Q_DECLARE_METATYPE(eti::dlsplus::DLSPlusTag)
