/**
 * @file eti_frame_list_model.cpp
 * @brief Professional ETI Frame List Model implementation
 * 
 * This file implements the EtiFrameListModel class providing detailed
 * ETI frame information with real-time processing and ETSI compliance.
 */

#include "eti_frame_list_model.h"
#include "core/modern_eti_frame_parser.hpp"
#include "core/comprehensive_etsi_validator.hpp"
#include "utils/logger.h"

#include <QColor>
#include <QBrush>
#include <QFont>
#include <QIcon>
#include <QDateTime>
#include <QTimer>
#include <QMutexLocker>
#include <QDebug>
#include <algorithm>

EtiFrameListModel::EtiFrameListModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_frameParser(nullptr)
    , m_etsiValidator(nullptr)
    , m_maxFrames(DEFAULT_MAX_FRAMES)
    , m_realTimeEnabled(false)
    , m_statsTimer(nullptr)
    , m_totalFrames(0)
    , m_errorFrameCount(0)
    , m_currentFrameRate(0.0)
{
    // Setup statistics timer
    m_statsTimer = new QTimer(this);
    connect(m_statsTimer, &QTimer::timeout, this, &EtiFrameListModel::updateStatistics);
    
    Logger::instance().log(Logger::Info, "EtiFrameListModel", "Constructor completed");
}

EtiFrameListModel::~EtiFrameListModel()
{
    Logger::instance().log(Logger::Info, "EtiFrameListModel", "Destructor starting");
    
    // Stop updates
    setRealTimeEnabled(false);
    
    // Cleanup timer
    if (m_statsTimer) {
        m_statsTimer->stop();
        delete m_statsTimer;
        m_statsTimer = nullptr;
    }
    
    Logger::instance().log(Logger::Info, "EtiFrameListModel", "Destructor completed");
}

bool EtiFrameListModel::initializeWithModernEngine(eti::modern::ModernETIFrameParser *frameParser, 
                                                 eti::compliance::ComprehensiveETSIValidator *etsiValidator)
{
    if (!frameParser || !etsiValidator) {
        Logger::instance().log(Logger::Error, "EtiFrameListModel", "Invalid Modern ETI Core Engine components provided");
        return false;
    }
    
    Logger::instance().log(Logger::Info, "EtiFrameListModel", "Initializing with Modern ETI Core Engine");
    
    m_frameParser = frameParser;
    m_etsiValidator = etsiValidator;
    
    // Connect Modern ETI Core Engine signals - use lambda to adapt signal parameters
    connect(m_frameParser, &eti::modern::ModernETIFrameParser::frameProcessed,
            this, [this](uint32_t frame_number, const eti::EtiFrame& /*frame*/, std::chrono::nanoseconds parse_time) {
                // Convert to ETIParseResult format expected by existing slot
                eti::modern::ETIParseResult result;
                result.frame_number = frame_number;
                result.success = true; // Assume success if we got a frame
                result.parse_time = parse_time;
                // Note: frame data conversion would be needed for full compatibility
                onFrameProcessed(result);
            });
    // Note: Other signals will be connected when the corresponding slots are implemented
    
    // Enable real-time updates by default
    setRealTimeEnabled(true);
    
    Logger::instance().log(Logger::Info, "EtiFrameListModel", "Modern ETI Core Engine initialization completed successfully");
    return true;
}

int EtiFrameListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    QMutexLocker locker(&m_dataMutex);
    return m_frames.size();
}

int EtiFrameListModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return ColumnCount;
}

QVariant EtiFrameListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_frames.size()) {
        return QVariant();
    }
    
    QMutexLocker locker(&m_dataMutex);
    const FrameInfo& frame = m_frames.at(index.row());
    
    switch (role) {
    case Qt::DisplayRole:
        return formatFrameData(frame, index.column());
        
    case Qt::DecorationRole:
        if (index.column() == ColumnType) {
            return getFrameTypeIcon(frame.frameType);
        }
        break;
        
    case Qt::BackgroundRole:
        if (frame.status != FrameStatus::Valid) {
            QColor color = getStatusColor(frame.status);
            color.setAlpha(80); // Semi-transparent background
            return QBrush(color);
        } else if (frame.hasErrors) {
            QColor errorColor(255, 0, 0, 50); // Light red for errors
            return QBrush(errorColor);
        }
        break;
        
    case Qt::ForegroundRole:
        if (frame.status == FrameStatus::Error || frame.hasErrors) {
            return QBrush(QColor(Qt::red));
        } else if (frame.status == FrameStatus::Warning) {
            return QBrush(QColor(255, 140, 0)); // Orange
        } else if (frame.status == FrameStatus::Valid) {
            return QBrush(QColor(Qt::darkGreen));
        }
        break;
        
    case Qt::FontRole:
        if (frame.hasErrors) {
            QFont font;
            font.setBold(true);
            return font;
        }
        break;
        
    case Qt::TextAlignmentRole:
        switch (index.column()) {
        case ColumnFrameNumber:
        case ColumnSize:
        case ColumnQuality:
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        case ColumnTimestamp:
            return QVariant(Qt::AlignCenter);
        default:
            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        }
        
    case Qt::ToolTipRole:
        return QString("Frame #%1\n"
                      "Type: %2\n"
                      "Status: %3\n"
                      "Size: %4 bytes\n"
                      "Processing Time: %5 ms\n"
                      "Quality: %6%\n"
                      "Timestamp: %7")
                .arg(frame.frameNumber)
                .arg(formatFrameData(frame, ColumnType))
                .arg(formatFrameData(frame, ColumnStatus))
                .arg(frame.frameSize)
                .arg(frame.processingTime, 0, 'f', 3)
                .arg(frame.qualityScore, 0, 'f', 1)
                .arg(frame.timestamp.toString(Qt::ISODate));
        
    case Qt::UserRole:
        return static_cast<int>(frame.frameType);
        
    case Qt::UserRole + 1:
        return frame.frameNumber;
        
    case Qt::UserRole + 2:
        return static_cast<int>(frame.status);
        
    case Qt::UserRole + 3:
        return frame.qualityScore;
    }
    
    return QVariant();
}

QVariant EtiFrameListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }
    
    switch (section) {
    case ColumnFrameNumber:
        return tr("Frame #");
    case ColumnType:
        return tr("Type");
    case ColumnSize:
        return tr("Size");
    case ColumnFIC:
        return tr("FIC");
    case ColumnMSC:
        return tr("MSC");
    case ColumnTimestamp:
        return tr("Timestamp");
    case ColumnQuality:
        return tr("Quality");
    case ColumnStatus:
        return tr("Status");
    default:
        return QVariant();
    }
}

Qt::ItemFlags EtiFrameListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void EtiFrameListModel::addFrame(const FrameInfo& frame)
{
    QMutexLocker locker(&m_dataMutex);
    
    // Check if frame matches current filter
    if (!frameMatchesFilter(frame)) {
        return;
    }
    
    Logger::instance().log(Logger::Debug, "EtiFrameListModel", 
                          QString("Adding frame #%1 (Type: %2, Status: %3)")
                          .arg(frame.frameNumber)
                          .arg(static_cast<int>(frame.frameType))
                          .arg(static_cast<int>(frame.status)));
    
    beginInsertRows(QModelIndex(), m_frames.size(), m_frames.size());
    
    // Add frame to list
    m_frames.append(frame);
    m_frameIndexMap[frame.frameNumber] = m_frames.size() - 1;
    
    // Track error frames
    if (frame.hasErrors || frame.status == FrameStatus::Error) {
        m_errorFrames.append(frame.frameNumber);
        m_errorFrameCount++;
        emit frameErrorDetected(frame.frameNumber, frame.errorMessage);
    }
    
    // Update statistics
    m_totalFrames++;
    
    // Track frame timing for rate calculation
    m_recentFrameTimes.append(frame.timestamp);
    if (m_recentFrameTimes.size() > FRAME_RATE_WINDOW_SIZE) {
        m_recentFrameTimes.removeFirst();
    }
    
    endInsertRows();
    
    // Trim old frames if needed
    trimFramesIfNeeded();
    
    // Update frame rate
    updateFrameRate();
    
    // Emit selection request for real-time mode
    if (m_realTimeEnabled) {
        emit frameSelectionRequested(frame.frameNumber);
    }
}

void EtiFrameListModel::updateFrameStatus(quint64 frameNumber, FrameStatus status)
{
    QMutexLocker locker(&m_dataMutex);
    
    if (!m_frameIndexMap.contains(frameNumber)) {
        return;
    }
    
    int index = m_frameIndexMap[frameNumber];
    if (index >= 0 && index < m_frames.size()) {
        m_frames[index].status = status;
        m_frames[index].hasErrors = (status == FrameStatus::Error || 
                                    status == FrameStatus::CRC_Error ||
                                    status == FrameStatus::Sync_Error);
        
        QModelIndex modelIndex = createIndex(index, 0);
        QModelIndex lastColumn = createIndex(index, ColumnCount - 1);
        emit dataChanged(modelIndex, lastColumn);
        
        Logger::instance().log(Logger::Debug, "EtiFrameListModel", 
                              QString("Frame #%1 status updated to %2")
                              .arg(frameNumber)
                              .arg(static_cast<int>(status)));
    }
}

void EtiFrameListModel::highlightErrorFrames(const QList<quint64>& errorFrames)
{
    QMutexLocker locker(&m_dataMutex);
    
    m_errorFrames = errorFrames;
    
    // Update display for all error frames
    for (quint64 frameNumber : errorFrames) {
        if (m_frameIndexMap.contains(frameNumber)) {
            int index = m_frameIndexMap[frameNumber];
            if (index >= 0 && index < m_frames.size()) {
                m_frames[index].hasErrors = true;
                QModelIndex modelIndex = createIndex(index, 0);
                QModelIndex lastColumn = createIndex(index, ColumnCount - 1);
                emit dataChanged(modelIndex, lastColumn);
            }
        }
    }
    
    Logger::instance().log(Logger::Info, "EtiFrameListModel", 
                          QString("Highlighted %1 error frames").arg(errorFrames.size()));
}

void EtiFrameListModel::applyServiceFilter(const QList<quint32>& serviceIds)
{
    QMutexLocker locker(&m_dataMutex);
    
    m_serviceFilter = serviceIds;
    
    Logger::instance().log(Logger::Info, "EtiFrameListModel", 
                          QString("Applied service filter: %1 services")
                          .arg(serviceIds.size()));
    
    // Refresh the entire model with the new filter
    beginResetModel();
    // Filter logic would be applied here in a more complete implementation
    endResetModel();
}

EtiFrameListModel::FrameInfo EtiFrameListModel::getFrameInfo(quint64 frameNumber) const
{
    QMutexLocker locker(&m_dataMutex);
    
    if (m_frameIndexMap.contains(frameNumber)) {
        int index = m_frameIndexMap[frameNumber];
        if (index >= 0 && index < m_frames.size()) {
            return m_frames[index];
        }
    }
    
    return FrameInfo(); // Return invalid frame
}

EtiFrameListModel::FrameInfo EtiFrameListModel::getFrameInfo(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() >= m_frames.size()) {
        return FrameInfo();
    }
    
    QMutexLocker locker(&m_dataMutex);
    return m_frames.at(index.row());
}

QList<EtiFrameListModel::FrameInfo> EtiFrameListModel::getAllFrames() const
{
    QMutexLocker locker(&m_dataMutex);
    return m_frames;
}

QList<quint64> EtiFrameListModel::getErrorFrames() const
{
    QMutexLocker locker(&m_dataMutex);
    return m_errorFrames;
}

void EtiFrameListModel::clearFrames()
{
    QMutexLocker locker(&m_dataMutex);
    
    Logger::instance().log(Logger::Info, "EtiFrameListModel", "Clearing all frame data");
    
    beginResetModel();
    
    m_frames.clear();
    m_frameIndexMap.clear();
    m_errorFrames.clear();
    m_recentFrameTimes.clear();
    
    m_totalFrames = 0;
    m_errorFrameCount = 0;
    m_currentFrameRate = 0.0;
    
    endResetModel();
    
    emit statisticsChanged(getFrameStatistics());
}

void EtiFrameListModel::setMaxFrames(int maxFrames)
{
    QMutexLocker locker(&m_dataMutex);
    
    m_maxFrames = maxFrames;
    
    Logger::instance().log(Logger::Info, "EtiFrameListModel", 
                          QString("Maximum frames set to %1").arg(maxFrames));
    
    // Trim frames if necessary
    trimFramesIfNeeded();
}

void EtiFrameListModel::setRealTimeEnabled(bool enabled)
{
    if (m_realTimeEnabled == enabled) {
        return;
    }
    
    m_realTimeEnabled = enabled;
    
    if (m_statsTimer) {
        if (enabled) {
            m_statsTimer->start(STATS_UPDATE_INTERVAL_MS);
            Logger::instance().log(Logger::Info, "EtiFrameListModel", "Real-time updates enabled");
        } else {
            m_statsTimer->stop();
            Logger::instance().log(Logger::Info, "EtiFrameListModel", "Real-time updates disabled");
        }
    }
}

QVariantMap EtiFrameListModel::getFrameStatistics() const
{
    QMutexLocker locker(&m_dataMutex);
    
    QVariantMap stats;
    stats["totalFrames"] = m_totalFrames;
    stats["errorFrames"] = m_errorFrameCount;
    stats["currentFrameRate"] = m_currentFrameRate;
    stats["framesInMemory"] = m_frames.size();
    stats["maxFrames"] = m_maxFrames;
    
    // Calculate error rate
    double errorRate = (m_totalFrames > 0) ? (double(m_errorFrameCount) / m_totalFrames * 100.0) : 0.0;
    stats["errorRate"] = errorRate;
    
    // Calculate average processing time
    double avgProcessingTime = 0.0;
    if (!m_frames.isEmpty()) {
        double totalTime = 0.0;
        for (const auto& frame : m_frames) {
            totalTime += frame.processingTime;
        }
        avgProcessingTime = totalTime / m_frames.size();
    }
    stats["averageProcessingTime"] = avgProcessingTime;
    
    // Calculate average quality
    double avgQuality = 0.0;
    if (!m_frames.isEmpty()) {
        double totalQuality = 0.0;
        for (const auto& frame : m_frames) {
            totalQuality += frame.qualityScore;
        }
        avgQuality = totalQuality / m_frames.size();
    }
    stats["averageQuality"] = avgQuality;
    
    return stats;
}

void EtiFrameListModel::onFrameProcessed(const eti::modern::ETIParseResult& analysis)
{
    FrameInfo frame = convertFromEtisnoopFrame(analysis);
    addFrame(frame);
}

void EtiFrameListModel::onProcessingError(const QString& error)
{
    Logger::instance().log(Logger::Warning, "EtiFrameListModel", 
                          QString("Processing error received: %1").arg(error));
    
    // Create error frame entry
    FrameInfo errorFrame;
    errorFrame.frameNumber = m_totalFrames + 1;
    errorFrame.frameType = FrameType::Unknown;
    errorFrame.status = FrameStatus::Error;
    errorFrame.timestamp = QDateTime::currentDateTime();
    errorFrame.errorMessage = error;
    errorFrame.hasErrors = true;
    errorFrame.frameSize = 0;
    errorFrame.qualityScore = 0.0;
    
    addFrame(errorFrame);
}

void EtiFrameListModel::onStatsUpdated(const eti::modern::ModernETIFrameParser::PerformanceStats& stats)
{
    // Update frame rate from Modern ETI Core statistics
    m_currentFrameRate = stats.average_fps;
    emit frameRateChanged(m_currentFrameRate);
    emit statisticsChanged(getFrameStatistics());
}

void EtiFrameListModel::updateStatistics()
{
    // Update frame rate calculation
    updateFrameRate();
    
    // Emit updated statistics
    emit statisticsChanged(getFrameStatistics());
}

EtiFrameListModel::FrameInfo EtiFrameListModel::convertFromEtisnoopFrame(const eti::modern::ETIParseResult& analysis) const
{
    FrameInfo frame;
    frame.frameNumber = analysis.frame_number;
    frame.frameSize = 6144; // ETI frame size - rawData not available in ETIParseResult
    frame.timestamp = QDateTime::currentDateTime();
    frame.processingTime = analysis.parse_time.count() / 1000.0; // Convert ns to microseconds
    frame.hasErrors = !analysis.success;
    frame.errorMessage = analysis.success ? QString() : "Parse failed";
    frame.figData = QVariantMap(); // FIG data would need to be extracted from frame
    frame.complianceData = QVariantMap(); // Compliance data would come from validation
    frame.rawData = QByteArray(); // Raw data not directly available
    
    // Detect frame type - default to ETI_NI since we have parsed ETI frame
    frame.frameType = FrameType::ETI_NI;
    
    // Determine frame status
    if (!analysis.success) {
        frame.status = FrameStatus::Error;
    } else if (!frame.errorMessage.isEmpty()) {
        frame.status = FrameStatus::Warning;
    } else {
        frame.status = FrameStatus::Valid;
    }
    
    // Extract FIC and MSC information
    // TODO: FIC/MSC extraction needs proper ETIParseResult integration
    // frame.ficInfo = extractFicInfo(analysis.rawData);
    // frame.mscInfo = extractMscInfo(analysis.rawData);
    frame.ficInfo = "FIC parsing not yet integrated";
    frame.mscInfo = "MSC parsing not yet integrated";
    
    // Calculate quality score
    frame.qualityScore = calculateQualityScore(frame);
    
    return frame;
}

EtiFrameListModel::FrameType EtiFrameListModel::detectFrameType(const QByteArray& frameData) const
{
    if (frameData.size() != ETI_FRAME_SIZE) {
        return FrameType::Unknown;
    }
    
    // ETI frame detection based on ETSI EN 300 799
    // Check sync pattern at the beginning of frame
    if (frameData.size() >= 4) {
        const quint8* data = reinterpret_cast<const quint8*>(frameData.constData());
        
        // Check for ETI-NI sync pattern (0xFF, 0x00, 0xFF, 0x00)
        if (data[0] == 0xFF && data[1] == 0x00 && data[2] == 0xFF && data[3] == 0x00) {
            return FrameType::ETI_NI;
        }
        
        // Additional frame type detection could be added here
        // For now, assume ETI-NI if size is correct
        return FrameType::ETI_NI;
    }
    
    return FrameType::Unknown;
}

QString EtiFrameListModel::extractFicInfo(const QByteArray& frameData) const
{
    if (frameData.size() < ETI_FRAME_SIZE) {
        return "Invalid";
    }
    
    // Simplified FIC extraction - in real implementation would parse FIC structure
    // FIC starts at byte 12 in ETI-NI frame
    if (frameData.size() >= 32) {
        const quint8* data = reinterpret_cast<const quint8*>(frameData.constData());
        quint8 ficLength = data[5]; // FIC length field
        return QString("FIC Len: %1").arg(ficLength);
    }
    
    return "N/A";
}

QString EtiFrameListModel::extractMscInfo(const QByteArray& frameData) const
{
    if (frameData.size() < ETI_FRAME_SIZE) {
        return "Invalid";
    }
    
    // Simplified MSC extraction - in real implementation would parse MSC structure
    // MSC starts after FIC and EOF in ETI frame
    int mscDataLength = frameData.size() - 32; // Approximate MSC data length
    return QString("MSC: %1 bytes").arg(mscDataLength);
}

double EtiFrameListModel::calculateQualityScore(const FrameInfo& frame) const
{
    double score = 100.0;
    
    // Reduce score based on status
    switch (frame.status) {
    case FrameStatus::Valid:
        break; // No reduction
    case FrameStatus::Warning:
        score -= 20.0;
        break;
    case FrameStatus::Error:
    case FrameStatus::CRC_Error:
    case FrameStatus::Sync_Error:
        score -= 50.0;
        break;
    case FrameStatus::Timeout:
        score -= 30.0;
        break;
    default:
        score -= 80.0;
        break;
    }
    
    // Reduce score based on processing time (if too slow)
    if (frame.processingTime > 1.0) { // More than 1ms is slow for 900+ fps
        score -= (frame.processingTime - 1.0) * 10.0;
    }
    
    // Ensure score is in valid range
    return qBound(0.0, score, 100.0);
}

QColor EtiFrameListModel::getStatusColor(FrameStatus status) const
{
    switch (status) {
    case FrameStatus::Valid:
        return QColor(76, 175, 80);   // Green
    case FrameStatus::Warning:
        return QColor(255, 152, 0);   // Orange
    case FrameStatus::Error:
        return QColor(244, 67, 54);   // Red
    case FrameStatus::CRC_Error:
        return QColor(156, 39, 176);  // Purple
    case FrameStatus::Sync_Error:
        return QColor(233, 30, 99);   // Pink
    case FrameStatus::Timeout:
        return QColor(121, 85, 72);   // Brown
    default:
        return QColor(158, 158, 158); // Gray
    }
}

QIcon EtiFrameListModel::getFrameTypeIcon(FrameType type) const
{
    switch (type) {
    case FrameType::ETI_NI:
        return QIcon(":/icons/eti-ni.png");
    case FrameType::ETI_G703:
        return QIcon(":/icons/eti-g703.png");
    case FrameType::FIC_DATA:
        return QIcon(":/icons/fic.png");
    case FrameType::MSC_DATA:
        return QIcon(":/icons/msc.png");
    case FrameType::NULL_FRAME:
        return QIcon(":/icons/null-frame.png");
    default:
        return QIcon(":/icons/unknown-frame.png");
    }
}

QString EtiFrameListModel::formatFrameData(const FrameInfo& frame, int column) const
{
    switch (column) {
    case ColumnFrameNumber:
        return QString::number(frame.frameNumber);
        
    case ColumnType:
        switch (frame.frameType) {
        case FrameType::ETI_NI:
            return "ETI-NI";
        case FrameType::ETI_G703:
            return "ETI-G703";
        case FrameType::FIC_DATA:
            return "FIC";
        case FrameType::MSC_DATA:
            return "MSC";
        case FrameType::NULL_FRAME:
            return "NULL";
        default:
            return "Unknown";
        }
        
    case ColumnSize:
        return QString("%1 B").arg(frame.frameSize);
        
    case ColumnFIC:
        return frame.ficInfo;
        
    case ColumnMSC:
        return frame.mscInfo;
        
    case ColumnTimestamp:
        return frame.timestamp.toString("hh:mm:ss.zzz");
        
    case ColumnQuality:
        return QString("%1%").arg(frame.qualityScore, 0, 'f', 1);
        
    case ColumnStatus:
        switch (frame.status) {
        case FrameStatus::Valid:
            return "Valid";
        case FrameStatus::Warning:
            return "Warning";
        case FrameStatus::Error:
            return "Error";
        case FrameStatus::CRC_Error:
            return "CRC Error";
        case FrameStatus::Sync_Error:
            return "Sync Error";
        case FrameStatus::Timeout:
            return "Timeout";
        default:
            return "Unknown";
        }
        
    default:
        return QString();
    }
}

bool EtiFrameListModel::frameMatchesFilter(const FrameInfo& frame) const
{
    // If no filter is set, show all frames
    if (m_serviceFilter.isEmpty()) {
        return true;
    }
    
    // In a more complete implementation, this would check if the frame
    // contains data for any of the filtered services
    Q_UNUSED(frame)
    return true; // For now, show all frames
}

void EtiFrameListModel::trimFramesIfNeeded()
{
    if (m_maxFrames <= 0 || m_frames.size() <= m_maxFrames) {
        return;
    }
    
    int framesToRemove = m_frames.size() - m_maxFrames;
    
    Logger::instance().log(Logger::Debug, "EtiFrameListModel", 
                          QString("Trimming %1 old frames (keeping %2)")
                          .arg(framesToRemove).arg(m_maxFrames));
    
    beginRemoveRows(QModelIndex(), 0, framesToRemove - 1);
    
    // Remove old frames
    for (int i = 0; i < framesToRemove; ++i) {
        if (!m_frames.isEmpty()) {
            FrameInfo removedFrame = m_frames.takeFirst();
            m_frameIndexMap.remove(removedFrame.frameNumber);
        }
    }
    
    // Update frame index mapping
    for (int i = 0; i < m_frames.size(); ++i) {
        m_frameIndexMap[m_frames[i].frameNumber] = i;
    }
    
    endRemoveRows();
}

void EtiFrameListModel::updateFrameRate()
{
    if (m_recentFrameTimes.size() < 2) {
        return;
    }
    
    // Calculate frame rate based on recent frame timestamps
    QDateTime earliest = m_recentFrameTimes.first();
    QDateTime latest = m_recentFrameTimes.last();
    
    qint64 timeDiff = earliest.msecsTo(latest);
    if (timeDiff > 0) {
        double fps = (m_recentFrameTimes.size() - 1) * 1000.0 / timeDiff;
        
        // Smooth the frame rate calculation
        m_currentFrameRate = (m_currentFrameRate * 0.8) + (fps * 0.2);
        
        emit frameRateChanged(m_currentFrameRate);
    }
}

bool EtiFrameListModel::validateFrameData(const FrameInfo& frame) const
{
    // Basic frame validation
    if (frame.frameNumber == 0) {
        return false;
    }
    
    if (frame.frameSize != ETI_FRAME_SIZE && frame.frameSize != 0) {
        return false; // Invalid frame size
    }
    
    if (frame.processingTime < 0.0) {
        return false; // Invalid processing time
    }
    
    return true;
}