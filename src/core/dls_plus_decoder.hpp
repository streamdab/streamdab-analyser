// dls_plus_decoder.hpp - DLS+ (Dynamic Label Plus) Decoder
// ETSI TS 102 980 - Dynamic Label Plus (DLS+) Specification
//
// This component implements DLS+ message parsing for structured "now playing"
// metadata extraction from DAB/DAB+ Program Associated Data (PAD).
//
// Phase 3B: Week 1 - Core Parser Implementation
// Timeline: October 23-29, 2025
// Status: Implementation in progress

#ifndef ETI_DLS_PLUS_DECODER_HPP
#define ETI_DLS_PLUS_DECODER_HPP

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QVector>
#include <QMap>
#include <QDateTime>
#include <QMutex>
#include <QPair>
#include <cstdint>
#include <optional>

namespace eti {
namespace dls_plus {

// DLS+ Content Types (ETSI TS 102 980 Section 4.1)
// These are actually content type tags (combining type + subtype)
enum class ContentType : uint8_t {
    DUMMY = 0,          // No data
    ITEM = 1,           // Item (ITEM.TITLE, ITEM.ARTIST, ITEM.ALBUM, etc.)
    INFO = 2,           // Information (INFO.NEWS, INFO.SPORT, etc.)
    PROGRAMME = 3,      // Programme (PROGRAMME.NOW, PROGRAMME.NEXT, etc.)
    INTERACTIVITY = 4,  // Interactivity (phone, SMS, email)
    RESERVED_5 = 5,     // Reserved
    RESERVED_6 = 6,     // Reserved
    DESCRIPTOR = 7,     // Content descriptor
    STATIONNAME = 8,    // Station name (SHORT, LONG)
    PROGRAMME_TYPE = 9, // Programme type
    VALIDITY = 10,      // Validity information
    RESERVED_11 = 11,   // Reserved
    RESERVED_12 = 12,   // Reserved
    RESERVED_13 = 13,   // Reserved
    RESERVED_14 = 14,   // Reserved
    RESERVED_15 = 15,   // Reserved
    
    // Convenience aliases for common tags (ITEM subtypes)
    TITLE = 1,          // Alias for ITEM (subtype 0: ITEM.TITLE)
    ALBUM = 1,          // Alias for ITEM (subtype 1: ITEM.ALBUM)
    ARTIST = 1,         // Alias for ITEM (subtype 3: ITEM.ARTIST)
    GENRE = 1           // Alias for ITEM (subtype 10: ITEM.GENRE)
};

// DLS+ tag structure
struct DLSPlusTag {
    uint8_t content_type;       // 4-bit: ITEM, INFO, PROGRAMME, etc.
    uint8_t content_subtype;    // 4-bit: TITLE, ARTIST, ALBUM, etc.
    uint16_t content_id;        // 12-bit content identifier
    uint8_t start_marker;       // Start position in DLS text (0-127)
    uint8_t length_marker;      // Length of tagged text (0-127)
    QString text;               // Extracted text (UTF-8)
    
    // Helper methods
    QString getTypeString() const;       // "ITEM", "INFO", "PROGRAMME", etc.
    QString getSubtypeString() const;    // "TITLE", "ARTIST", "ALBUM", etc.
    QString getFullTagName() const;      // "ITEM.TITLE", "ITEM.ARTIST", etc.
    bool isValid() const;                // Validate tag structure
    
    DLSPlusTag()
        : content_type(0)
        , content_subtype(0)
        , content_id(0)
        , start_marker(0)
        , length_marker(0)
    {}
};

// DLS+ message structure
struct DLSPlusMessage {
    QString dls_text;                    // Complete DLS text (UTF-8)
    QVector<DLSPlusTag> tags;            // All tags in message
    QVector<DLSPlusTag> descriptors;     // Alias for tags (for test compatibility)
    QDateTime timestamp;                 // Reception timestamp
    bool is_valid;                       // Validation status
    uint8_t charset;                     // Character set (0=ISO 8859-1, 15=UTF-8)
    
    // Helper methods
    QString getTag(const QString& tag_name) const;     // Get tag by name
    bool hasTag(const QString& tag_name) const;        // Check tag exists
    QMap<QString, QString> getAllTags() const;         // All tags as map
    QString getNowPlaying() const;                     // "ARTIST - TITLE" format
    QString getArtist() const;                         // Get ITEM.ARTIST
    QString getItem() const;                           // Get ITEM.TITLE (track/item)
    QString getTitle() const { return getTag("ITEM.TITLE"); }  // Alias for getItem
    QString getAlbum() const { return getTag("ITEM.ALBUM"); }
    QString getGenre() const { return getTag("ITEM.GENRE"); }
    
    DLSPlusMessage()
        : is_valid(false)
        , charset(15)  // Default to UTF-8
    {}
};

// Main DLS+ decoder class
class DLSPlusDecoder : public QObject {
    Q_OBJECT

public:
    explicit DLSPlusDecoder(QObject* parent = nullptr);
    ~DLSPlusDecoder() override;
    
    // Parse DLS+ message from PAD data
    // Returns true if DLS+ message successfully parsed
    bool processPADData(const QByteArray& pad_data);
    
    // Get current DLS+ message (thread-safe)
    DLSPlusMessage getCurrentMessage() const;
    std::optional<DLSPlusMessage> getCurrentMessageOptional() const;
    bool hasCurrentMessage() const;
    
    // Get specific tags (convenience methods)
    QString getNowPlaying() const;          // ITEM.TITLE - ITEM.ARTIST
    QString getArtist() const;              // ITEM.ARTIST
    QString getTitle() const;               // ITEM.TITLE
    QString getAlbum() const;               // ITEM.ALBUM
    QString getTrackNumber() const;         // ITEM.TRACKNUMBER
    QString getProgrammeName() const;       // PROGRAMME.NOW
    QString getNextProgramme() const;       // PROGRAMME.NEXT
    QString getStationName() const;         // STATIONNAME.LONG
    QString getNewsHeadline() const;        // INFO.NEWS
    
    // Convenience accessors (for test compatibility)
    QString currentArtist() const { return getArtist(); }
    QString currentTrack() const { return getTitle(); }
    QString currentAlbum() const { return getAlbum(); }
    
    // Alternative processing method for testing (processes DLS text + command directly)
    bool processDLSPlusCommand(const QByteArray& dls_text, const QByteArray& command_data);
    
    // Statistics structure
    struct Statistics {
        uint32_t messages_processed = 0;
        uint32_t tags_extracted = 0;
        uint32_t parsing_errors = 0;
    };
    
    // Statistics methods
    uint64_t getTotalMessagesProcessed() const;
    uint64_t getValidMessagesCount() const;
    uint64_t getInvalidMessagesCount() const;
    Statistics getStatistics() const;
    void resetStatistics();
    
signals:
    // Emitted when new DLS+ message received (service_id, message)
    void dlsPlusMessageReceived(uint32_t service_id, const DLSPlusMessage& message);
    
    // Emitted when specific tags change (main.cpp expects these exact signal names)
    void nowPlayingUpdated(const QString& track, const QString& artist, const QString& album);
    void nowPlayingChanged(const QString& artist, const QString& title);
    void albumChanged(const QString& album);
    void programmeChanged(const QString& programme);
    void stationNameChanged(const QString& station_name);
    
    // Error signals
    void parseError(const QString& error_msg);
    void dlsPlusDecodingError(const QString& error_msg);

private:
    // Internal parsing methods
    bool parseDLSPlusObject(const QByteArray& data, DLSPlusMessage& message);
    bool extractDLSText(const QByteArray& data, QString& dls_text, uint8_t& charset);
    bool extractTags(const QByteArray& data, QVector<DLSPlusTag>& tags);
    bool validateTag(const DLSPlusTag& tag, const QString& dls_text) const;
    bool extractTagText(const DLSPlusTag& tag, const QString& dls_text, QString& text) const;
    bool parseCommandData(const QByteArray& command_data, const QString& dls_text, QVector<DLSPlusTag>& tags);
    
    // PAD processing
    bool isPADContainingDLSPlus(const QByteArray& pad_data) const;
    QByteArray extractDLSPlusData(const QByteArray& pad_data) const;
    
    // Tag type lookup
    QString getTagTypeName(uint8_t type) const;
    QString getTagSubtypeName(uint8_t type, uint8_t subtype) const;
    void initializeTagLookupTables();
    
    // State management
    DLSPlusMessage current_message_;
    bool has_current_message_;
    mutable QMutex mutex_;
    
    // Statistics
    uint64_t total_messages_processed_;
    uint64_t valid_messages_count_;
    uint64_t invalid_messages_count_;
    uint32_t tags_extracted_count_;
    
    // Tag lookup tables (ETSI TS 102 980 compliant)
    QMap<uint8_t, QString> tag_types_;                              // Type 4-bit -> "ITEM"
    QMap<QPair<uint8_t, uint8_t>, QString> tag_subtypes_;          // (type, subtype) -> "TITLE"
    
    // Previous state for change detection
    QString previous_artist_;
    QString previous_title_;
    QString previous_album_;
    QString previous_programme_;
    QString previous_station_name_;
};

} // namespace dls_plus
} // namespace eti

#endif // ETI_DLS_PLUS_DECODER_HPP
