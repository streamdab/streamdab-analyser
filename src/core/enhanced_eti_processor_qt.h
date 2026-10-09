#ifndef ENHANCED_ETI_PROCESSOR_QT_H
#define ENHANCED_ETI_PROCESSOR_QT_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QMutex>
#include <vector>
#include <cstdint>
#include <chrono>
#include <memory>

// Phase 1.2: Qt-Compatible ETI Data Structures (Legacy - kept for compatibility)
enum class ETIFormat {
    ETI_NI,      // Network Independent (0x49931E03)
    ETI_LI_A,    // Linear Interface Pattern A (0xfff8c549)
    ETI_LI_B,    // Linear Interface Pattern B (0xff073ab6)
    UNKNOWN
};

struct ProcessedFrame {
    uint64_t frame_number = 0;
    ETIFormat format = ETIFormat::UNKNOWN;
    uint32_t sync_pattern = 0;
    uint32_t lidata = 0;
    QByteArray fic_data;
    // T24: total concatenated MSC size (bytes). The MSC bytes themselves are
    // owned exactly once, by the per-subchannel slices below — this replaces
    // the old duplicate `msc_data` copy and is only used for byte-count
    // metrics. Defaults to 0 (no MSC).
    std::size_t msc_size = 0;
    // Per-subchannel MSC slices already extracted by the frame parser, so
    // downstream consumers (e.g. DLS+ PAD parsing) can reuse them instead of
    // re-running parseHeader()/extractMSC() on the raw frame.
    struct SubChannelSlice {
        uint8_t sub_channel_id = 0;
        QByteArray data;
    };
    // T24: held behind a shared_ptr<const vector> so the frameProcessed()
    // signal hand-off (by-value / queued connections) is an O(1) refcount bump
    // instead of a deep copy of every ~5-6 KB slice each frame. Null means the
    // frame carries no MSC slices.
    std::shared_ptr<const std::vector<SubChannelSlice>> sub_channels;
    uint32_t crc = 0;
    QDateTime timestamp;
    bool is_valid = false;

    // Default-constructible/copyable/movable: required by Q_DECLARE_METATYPE
    // and Qt::QueuedConnection. The shared_ptr makes copies cheap and keeps the
    // slice bytes alive across a queued hand-off (refcounted, immutable).
    ProcessedFrame() = default;
    ProcessedFrame(const ProcessedFrame&) = default;
    ProcessedFrame& operator=(const ProcessedFrame&) = default;
    ProcessedFrame(ProcessedFrame&&) noexcept = default;
    ProcessedFrame& operator=(ProcessedFrame&&) noexcept = default;

    // Convenience accessors that never dereference the null shared_ptr.
    bool hasSubChannels() const {
        return sub_channels && !sub_channels->empty();
    }
    const std::vector<SubChannelSlice>& subChannels() const {
        static const std::vector<SubChannelSlice> kNoSubChannels;
        return sub_channels ? *sub_channels : kNoSubChannels;
    }
};

struct FIGInfo {
    uint8_t type;
    uint8_t length;
    QByteArray data;
    bool continuation;
    bool other_ensemble;
};

// Register custom types with Qt meta-object system
Q_DECLARE_METATYPE(ProcessedFrame)
Q_DECLARE_METATYPE(ETIFormat)

// ============================================================================
// Phase 1 Week 1: Real ETI Frame Structures (ETSI EN 300 799 Compliant)
// ============================================================================

// ETI Frame Header Structure (ETSI EN 300 799 Section 5.1)
struct ETIHeader {
    uint32_t sync;           // SYNC pattern (0xFF1F491F, 0xFF1FC4FF, or 0x491FC4FF)
    uint8_t err;             // Error field (byte 4)
    uint8_t frame_counter;   // FCT - Frame Counter (0-249, cycles every 5 seconds)
    uint8_t ficf;            // FIC flag (0=no FIC, 1=FIC present)
    uint8_t nst;             // Number of streams (0-64)
    uint8_t fp;              // Frame phase (0-7)
    uint8_t mid;             // Mode identity (1-4: Mode I-IV)
    uint16_t fl;             // Frame length in words

    // Helper methods
    bool isValid() const;
    QString toString() const;
};

// Sub-channel stream data (MSC component)
struct SubChannelData {
    uint8_t sub_channel_id;      // Sub-channel identifier (0-63)
    uint16_t start_address;      // Start address in CUs
    uint8_t table_switch;        // Table switch flag
    uint8_t table_index;         // Protection table index
    uint16_t size_in_bytes;      // Stream size in bytes
    QByteArray data;             // Actual MSC data

    QString toString() const;
};

// Complete ETI Frame (parsed representation)
struct ETIFrameData {
    ETIHeader header;                             // Parsed header
    QByteArray fic_data;                          // FIC data (96 bytes if present)
    std::vector<SubChannelData> sub_channels;     // MSC sub-channels
    uint16_t frame_crc;                           // Frame CRC (last 2 bytes)
    bool is_valid;                                // Overall validity

    QString summary() const;
};

// ============================================================================
// ETI Frame Parser Class (Real 6144-byte Frame Processing)
// ============================================================================

class ETIFrameParser {
public:
    ETIFrameParser();

    // Main parsing function
    bool parseFrame(const QByteArray& frame_data, ETIFrameData& result);

    // Header parsing
    ETIHeader parseHeader(const uint8_t* data);
    bool validateSyncPattern(uint32_t sync);
    bool validateFrameCounter(uint8_t fct);

    // FIC extraction (96 bytes) - CRITICAL-001 FIX: Added nst parameter
    QByteArray extractFIC(const uint8_t* data, bool ficf_flag, uint8_t nst, uint8_t mid);

    // MSC extraction (sub-channels)
    std::vector<SubChannelData> extractMSC(const uint8_t* data, uint8_t nst,
                                           bool ficf_flag, const ETIHeader& header);

    // Statistics
    uint64_t getFramesParsed() const { return m_frames_parsed; }
    uint64_t getFramesValid() const { return m_frames_valid; }
    uint64_t getFramesInvalid() const { return m_frames_invalid; }
    uint64_t getFrameCounterErrors() const { return m_frame_counter_errors; }

    // Reset statistics
    void resetStatistics();

private:
    // State tracking
    uint8_t m_last_frame_counter;
    bool m_first_frame;
    uint64_t m_frames_parsed;
    uint64_t m_frames_valid;
    uint64_t m_frames_invalid;
    uint64_t m_frame_counter_errors;

    // Helper methods: extract multi-byte values (big-endian)
    uint16_t extractUInt16(const uint8_t* data, size_t offset) const;
    uint32_t extractUInt32(const uint8_t* data, size_t offset) const;

    // Stream info parsing
    struct StreamInfo {
        uint8_t sub_channel_id;
        uint16_t start_address;
        uint8_t table_switch;
        uint8_t table_index;
        uint16_t stream_length;  // in 8-byte words
    };

    StreamInfo parseStreamInfo(const uint8_t* data, size_t offset) const;
};

// ============================================================================
// Enhanced ETI Processor Qt (Main Processor Class)
// ============================================================================

class EnhancedETIProcessorQt : public QObject
{
    Q_OBJECT

    // Friend class for unit testing (CRITICAL-003 test access)
    friend class TestETILICRCValidation;

public:
    explicit EnhancedETIProcessorQt(QObject *parent = nullptr);

    // File processing methods
    bool processETIFile(const QString& filePath);
    void stopProcessing();

    // Statistics accessors
    uint64_t getFrameCount() const { return frame_count_; }
    int getETINICount() const { return eti_ni_count_; }
    int getETILIACount() const { return eti_li_a_count_; }
    int getETILIBCount() const { return eti_li_b_count_; }

    // Format utilities
    QString formatToString(ETIFormat format) const;
    QString syncPatternToString(uint32_t pattern) const;

    // Hex viewer integration: Get raw frame data
    /**
     * @brief Get raw ETI frame data for hex viewer display
     * @param frame_index Frame index (0-based)
     * @return 6144-byte raw frame data, or empty QByteArray if invalid index
     *
     * This method extracts raw frame data from the loaded ETI file for
     * display in the hex viewer. Returns exactly 6144 bytes per frame.
     *
     * CRITICAL-002 FIX: Thread-safe implementation with mutex protection
     * and explicit detach to avoid Qt's copy-on-write race conditions.
     */
    QByteArray getRawFrameData(int frame_index) const;

    /**
     * @brief Get total number of frames available
     * @return Total frames loaded from file
     */
    int getTotalFrames() const { return total_frames_; }

    // Phase 4: Live streaming support - public API for real-time frame processing
    /**
     * @brief Process a single ETI frame received from live stream
     * @param frameData 6144-byte ETI frame data
     * @return true if frame processed successfully, false on error
     * 
     * This method enables real-time ETI frame processing from network streams
     * (ETI-over-IP multicast). Processes frame and extracts FIC for FIG analysis.
     */
    bool processLiveFrame(const QByteArray& frameData);

    /**
     * @brief Drop all file-backed/counter state for a fresh capture.
     *
     * Clears the loaded file bytes, total/frame counters and stored frames so a
     * later capture (e.g. a live network stream after a file load) starts from
     * zero and cannot continue numbering/reading a stale file. Safe to call at
     * any time on the owning (GUI) thread.
     */
    void resetStatistics();

public slots:
    void processNextFrame();

signals:
    // Processing signals
    void frameProcessed(const ProcessedFrame& frame);
    void processingProgress(int percentage, int currentFrame, int totalFrames);
    void processingComplete(int totalFrames, int processingTimeMs);
    void processingError(const QString& error);
    void frameError(int frameNumber, const QString& error, const QString& details);

    // Analysis signals
    void formatDetected(ETIFormat format, const QString& formatName);
    void figDiscovered(const FIGInfo& fig);
    void processingStarted(const QString& fileName, int estimatedFrames);

private:
    // Core processing methods
    ETIFormat detectETIFormat(uint32_t syncPattern) const;
    bool processEtiFrame(const QByteArray& frameData);
    void processFicData(const QByteArray& ficData);
    void processAllFramesImmediate();  // Phase 1.4: Fast batch processing
    void storeFrame(const ProcessedFrame& frame);  // Frame storage with FIFO cleanup

    // CRITICAL-007: CRC validation methods
    static const uint16_t CRC16_TABLE[256];
    uint16_t calculateCRC16(const uint8_t* data, size_t length) const;
    bool validateFrameCRC(const QByteArray& frameData) const;
    bool validateFICCRC(const QByteArray& ficData) const;

    // ETSI EN 300 799 sync patterns
    static const uint32_t ETI_NI_SYNC = 0x49931E03;     // Standard ETI-NI
    static const uint32_t ETI_LI_SYNC_A = 0xfff8c549;   // ETI-LI Pattern A
    static const uint32_t ETI_LI_SYNC_B = 0xff073ab6;   // ETI-LI Pattern B

    // Resource limits (HIGH-007)
    static constexpr size_t MAX_STORED_FRAMES = 10000;
    static constexpr size_t FRAME_CLEANUP_THRESHOLD = 1000;
    static constexpr qint64 MAX_FILE_SIZE = 500 * 1024 * 1024; // 500 MB

    // ETI frame size constant
    static constexpr size_t ETI_FRAME_SIZE = 6144;

    // Processing state
    uint64_t frame_count_;
    int eti_ni_count_;
    int eti_li_a_count_;
    int eti_li_b_count_;

    // File processing
    QByteArray file_data_;
    int current_frame_index_;
    int total_frames_;
    QTimer* processing_timer_;
    QString current_file_path_;

    // Performance tracking
    QDateTime processing_start_time_;
    QString current_file_name_;

    // Processed frames storage
    std::vector<ProcessedFrame> processed_frames_;

    // Phase 1 Week 1: Real ETI Frame Parser
    std::unique_ptr<ETIFrameParser> m_frame_parser;

    // CRITICAL-002 FIX: Thread safety for getRawFrameData()
    // Protects file_data_ from concurrent access and Qt's COW race conditions
    mutable QMutex m_file_data_mutex;
};

#endif // ENHANCED_ETI_PROCESSOR_QT_H
