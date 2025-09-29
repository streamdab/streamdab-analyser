#ifndef ENHANCED_ETI_PROCESSOR_QT_H
#define ENHANCED_ETI_PROCESSOR_QT_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <vector>
#include <cstdint>
#include <chrono>

// Phase 1.2: Qt-Compatible ETI Data Structures
enum class ETIFormat {
    ETI_NI,      // Network Independent (0x49931E03)
    ETI_LI_A,    // Linear Interface Pattern A (0xfff8c549) 
    ETI_LI_B,    // Linear Interface Pattern B (0xff073ab6)
    UNKNOWN
};

struct ProcessedFrame {
    uint64_t frame_number;
    ETIFormat format;
    uint32_t sync_pattern;
    uint32_t lidata;
    QByteArray fic_data;
    QByteArray msc_data;
    uint32_t crc;
    QDateTime timestamp;
    bool is_valid;
};

struct FIGInfo {
    uint8_t type;
    uint8_t length;
    QByteArray data;
    bool continuation;
    bool other_ensemble;
};

class EnhancedETIProcessorQt : public QObject
{
    Q_OBJECT

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

public slots:
    void processNextFrame();

signals:
    // Processing signals
    void frameProcessed(const ProcessedFrame& frame);
    void processingProgress(int percentage, int currentFrame, int totalFrames);
    void processingComplete(int totalFrames, int processingTimeMs);
    void processingError(const QString& error);
    
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
    void resetStatistics();
    
    // ETSI EN 300 799 sync patterns
    static const uint32_t ETI_NI_SYNC = 0x49931E03;     // Standard ETI-NI
    static const uint32_t ETI_LI_SYNC_A = 0xfff8c549;   // ETI-LI Pattern A
    static const uint32_t ETI_LI_SYNC_B = 0xff073ab6;   // ETI-LI Pattern B
    
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
    
    // Performance tracking
    QDateTime processing_start_time_;
    QString current_file_name_;
    
    // Processed frames storage
    std::vector<ProcessedFrame> processed_frames_;
};

#endif // ENHANCED_ETI_PROCESSOR_QT_H