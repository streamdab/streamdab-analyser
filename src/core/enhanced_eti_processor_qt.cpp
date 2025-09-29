#include "enhanced_eti_processor_qt.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>

EnhancedETIProcessorQt::EnhancedETIProcessorQt(QObject *parent)
    : QObject(parent)
    , frame_count_(0)
    , eti_ni_count_(0)
    , eti_li_a_count_(0)
    , eti_li_b_count_(0)
    , current_frame_index_(0)
    , total_frames_(0)
    , processing_timer_(new QTimer(this))
{
    // Setup processing timer for real-time updates
    processing_timer_->setSingleShot(false);
    processing_timer_->setInterval(10); // 10ms intervals for smooth updates
    connect(processing_timer_, &QTimer::timeout, this, &EnhancedETIProcessorQt::processNextFrame);
}

bool EnhancedETIProcessorQt::processETIFile(const QString& filePath)
{
    // Reset state
    resetStatistics();
    
    // Load file
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit processingError(QString("Cannot open file: %1").arg(filePath));
        return false;
    }
    
    file_data_ = file.readAll();
    file.close();
    
    // Validate file size
    if (file_data_.size() % 6144 != 0) {
        emit processingError(QString("Invalid ETI file size: %1 bytes (not multiple of 6144)").arg(file_data_.size()));
        return false;
    }
    
    // Calculate frame count
    total_frames_ = file_data_.size() / 6144;
    current_frame_index_ = 0;
    
    // Setup processing
    QFileInfo fileInfo(filePath);
    current_file_name_ = fileInfo.fileName();
    processing_start_time_ = QDateTime::currentDateTime();
    
    // Start processing
    emit processingStarted(current_file_name_, total_frames_);
    
    // PHASE 1.4 FIX: Process all frames immediately for file analysis
    processAllFramesImmediate();
    
    return true;
}

void EnhancedETIProcessorQt::processAllFramesImmediate()
{
    // Process all frames immediately for file analysis (Phase 1.4)
    for (int i = 0; i < total_frames_; ++i) {
        current_frame_index_ = i;
        
        // Extract frame data
        int frameOffset = i * 6144;
        QByteArray frameData = file_data_.mid(frameOffset, 6144);
        
        // Process frame
        if (processEtiFrame(frameData)) {
            // Calculate progress and emit every 100 frames to avoid UI flooding
            if (i % 100 == 0 || i == total_frames_ - 1) {
                int percentage = (i * 100) / total_frames_;
                emit processingProgress(percentage, i + 1, total_frames_);
            }
        }
    }
    
    // Processing complete
    auto endTime = QDateTime::currentDateTime();
    int processingTimeMs = processing_start_time_.msecsTo(endTime);
    
    emit processingComplete(total_frames_, processingTimeMs);
}

void EnhancedETIProcessorQt::processNextFrame()
{
    if (current_frame_index_ >= total_frames_) {
        // Processing complete
        processing_timer_->stop();
        
        auto endTime = QDateTime::currentDateTime();
        int processingTimeMs = processing_start_time_.msecsTo(endTime);
        
        emit processingComplete(total_frames_, processingTimeMs);
        return;
    }
    
    // Extract frame data
    int frameOffset = current_frame_index_ * 6144;
    QByteArray frameData = file_data_.mid(frameOffset, 6144);
    
    // Process frame
    if (processEtiFrame(frameData)) {
        // Calculate progress
        int percentage = (current_frame_index_ * 100) / total_frames_;
        emit processingProgress(percentage, current_frame_index_ + 1, total_frames_);
    }
    
    current_frame_index_++;
    
    // Check if all frames processed (Phase 1.4 - process all frames)
    if (current_frame_index_ >= total_frames_) {
        processing_timer_->stop();
        
        auto endTime = QDateTime::currentDateTime();
        int processingTimeMs = processing_start_time_.msecsTo(endTime);
        
        emit processingComplete(total_frames_, processingTimeMs);
    }
}

bool EnhancedETIProcessorQt::processEtiFrame(const QByteArray& frameData)
{
    if (frameData.size() != 6144) {
        emit processingError(QString("Invalid frame size: %1 bytes").arg(frameData.size()));
        return false;
    }
    
    ProcessedFrame frame;
    frame.frame_number = ++frame_count_;
    frame.timestamp = QDateTime::currentDateTime();
    
    // Extract and detect SYNC pattern (first 4 bytes)
    const uint8_t* data = reinterpret_cast<const uint8_t*>(frameData.constData());
    frame.sync_pattern = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    frame.format = detectETIFormat(frame.sync_pattern);
    
    if (frame.format == ETIFormat::UNKNOWN) {
        emit processingError(QString("Unknown ETI format with SYNC pattern: 0x%1")
                           .arg(frame.sync_pattern, 8, 16, QChar('0')));
        frame.is_valid = false;
        return false;
    }
    
    // Update format counts
    switch (frame.format) {
        case ETIFormat::ETI_NI:   eti_ni_count_++; break;
        case ETIFormat::ETI_LI_A: eti_li_a_count_++; break;
        case ETIFormat::ETI_LI_B: eti_li_b_count_++; break;
        default: break;
    }
    
    // Extract frame components
    frame.lidata = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
    frame.fic_data = frameData.mid(8, 32);
    frame.msc_data = frameData.mid(40, 6096);
    frame.crc = (data[6140] << 24) | (data[6141] << 16) | (data[6142] << 8) | data[6143];
    frame.is_valid = true;
    
    // Store frame
    processed_frames_.push_back(frame);
    
    // Emit signals
    emit formatDetected(frame.format, formatToString(frame.format));
    emit frameProcessed(frame);
    
    // Process FIC data
    processFicData(frame.fic_data);
    
    return true;
}

void EnhancedETIProcessorQt::processFicData(const QByteArray& ficData)
{
    if (ficData.size() < 32) return;
    
    // Enhanced FIG detection
    for (int i = 0; i < ficData.size() - 1; i++) {
        uint8_t figHeader = ficData[i];
        uint8_t figType = (figHeader >> 3) & 0x1F;
        uint8_t figLength = ficData[i + 1] & 0x1F;
        
        if (figType <= 31 && figLength > 0 && (i + figLength + 1) < ficData.size()) {
            FIGInfo fig;
            fig.type = figType;
            fig.length = figLength;
            fig.continuation = (figHeader & 0x80) != 0;
            fig.other_ensemble = (figHeader & 0x40) != 0;
            fig.data = ficData.mid(i + 2, figLength);
            
            emit figDiscovered(fig);
            
            i += figLength + 1; // Skip processed FIG
        }
    }
}

ETIFormat EnhancedETIProcessorQt::detectETIFormat(uint32_t syncPattern) const
{
    switch (syncPattern) {
        case ETI_NI_SYNC:   return ETIFormat::ETI_NI;
        case ETI_LI_SYNC_A: return ETIFormat::ETI_LI_A;
        case ETI_LI_SYNC_B: return ETIFormat::ETI_LI_B;
        default:            return ETIFormat::UNKNOWN;
    }
}

QString EnhancedETIProcessorQt::formatToString(ETIFormat format) const
{
    switch (format) {
        case ETIFormat::ETI_NI:   return "ETI-NI (Network Independent)";
        case ETIFormat::ETI_LI_A: return "ETI-LI Pattern A (Linear Interface)";
        case ETIFormat::ETI_LI_B: return "ETI-LI Pattern B (Linear Interface)";
        default:                  return "Unknown Format";
    }
}

QString EnhancedETIProcessorQt::syncPatternToString(uint32_t pattern) const
{
    return QString("0x%1").arg(pattern, 8, 16, QChar('0'));
}

void EnhancedETIProcessorQt::stopProcessing()
{
    processing_timer_->stop();
}

void EnhancedETIProcessorQt::resetStatistics()
{
    frame_count_ = 0;
    eti_ni_count_ = 0;
    eti_li_a_count_ = 0;
    eti_li_b_count_ = 0;
    current_frame_index_ = 0;
    total_frames_ = 0;
    processed_frames_.clear();
    file_data_.clear();
}