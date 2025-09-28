#pragma once

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QVariant>
#include <QTimer>
#include <QMutex>
#include <QDateTime>
#include <QColor>
#include <QIcon>
#include <QList>
#include <QHash>
#include <memory>
#include "core/modern_eti_frame_parser.hpp"

// Forward declarations for Modern ETI Core Engine
namespace eti {
namespace modern {
    struct FrameAnalysisResult;
}
namespace compliance {
    class ComprehensiveETSIValidator;
}
}

/**
 * @class EtiFrameListModel
 * @brief Professional ETI Frame List Model for frame-by-frame analysis
 * 
 * This model provides detailed ETI frame information with:
 * - 6144-byte frame structure display
 * - 24ms frame timing precision
 * - Real-time frame processing at >900 fps
 * - Color-coded frame types and status
 * - ETSI compliance validation per frame
 * - Professional broadcasting industry standards
 */
class EtiFrameListModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    /**
     * @brief ETI frame types based on ETSI standards
     */
    enum class FrameType {
        Unknown = 0,
        ETI_NI = 1,    // ETI Native Interface
        ETI_G703 = 2,  // ETI G.703 Interface
        FIC_DATA = 3,  // Fast Information Channel
        MSC_DATA = 4,  // Main Service Channel
        NULL_FRAME = 5 // Null/Padding frame
    };

    /**
     * @brief Frame status enumeration for quality indication
     */
    enum class FrameStatus {
        Unknown = 0,
        Valid = 1,
        Warning = 2,
        Error = 3,
        Sync_Error = 4,
        CRC_Error = 5,
        Timeout = 6
    };

    /**
     * @brief ETI frame information structure
     */
    struct FrameInfo {
        quint64 frameNumber;
        FrameType frameType;
        FrameStatus status;
        quint16 frameSize;
        QDateTime timestamp;
        double processingTime; // milliseconds
        QString ficInfo;
        QString mscInfo;
        double qualityScore;
        QVariantMap figData;
        QVariantMap complianceData;
        QByteArray rawData;
        QString errorMessage;
        bool hasErrors;
        
        FrameInfo() : frameNumber(0), frameType(FrameType::Unknown), 
                     status(FrameStatus::Unknown), frameSize(0),
                     processingTime(0.0), qualityScore(0.0), hasErrors(false) {}
    };

    /**
     * @brief Column definitions for the frame table
     */
    enum FrameColumns {
        ColumnFrameNumber = 0,
        ColumnType = 1,
        ColumnSize = 2,
        ColumnFIC = 3,
        ColumnMSC = 4,
        ColumnTimestamp = 5,
        ColumnQuality = 6,
        ColumnStatus = 7,
        ColumnCount = 8
    };

public:
    /**
     * @brief Constructor
     * @param parent Parent object
     */
    explicit EtiFrameListModel(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~EtiFrameListModel();

    /**
     * @brief Initialize model with ETISnoop wrapper
     * @param wrapper ETISnoop wrapper instance
     * @return true if successful
     */
    bool initializeWithModernEngine(eti::modern::ModernETIFrameParser *frameParser, 
                                   eti::compliance::ComprehensiveETSIValidator *etsiValidator);

    // QAbstractTableModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    /**
     * @brief Add new ETI frame to the model
     * @param frame Frame analysis result
     */
    void addFrame(const FrameInfo& frame);

    /**
     * @brief Update frame status
     * @param frameNumber Frame number to update
     * @param status New frame status
     */
    void updateFrameStatus(quint64 frameNumber, FrameStatus status);

    /**
     * @brief Highlight frames with errors
     * @param errorFrames List of frame numbers with errors
     */
    void highlightErrorFrames(const QList<quint64>& errorFrames);

    /**
     * @brief Apply service filter to frames
     * @param serviceIds List of service IDs to show (empty = show all)
     */
    void applyServiceFilter(const QList<quint32>& serviceIds);

    /**
     * @brief Get frame information by number
     * @param frameNumber Frame number
     * @return Frame information
     */
    FrameInfo getFrameInfo(quint64 frameNumber) const;

    /**
     * @brief Get frame information by model index
     * @param index Model index
     * @return Frame information
     */
    FrameInfo getFrameInfo(const QModelIndex &index) const;

    /**
     * @brief Get all frames
     * @return List of all frame information
     */
    QList<FrameInfo> getAllFrames() const;

    /**
     * @brief Get frames with errors
     * @return List of frame numbers with errors
     */
    QList<quint64> getErrorFrames() const;

    /**
     * @brief Clear all frame data
     */
    void clearFrames();

    /**
     * @brief Set maximum number of frames to keep in memory
     * @param maxFrames Maximum frame count (0 = unlimited)
     */
    void setMaxFrames(int maxFrames);

    /**
     * @brief Get maximum frame count
     * @return Maximum frames kept in memory
     */
    int getMaxFrames() const { return m_maxFrames; }

    /**
     * @brief Enable/disable real-time updates
     * @param enabled Update status
     */
    void setRealTimeEnabled(bool enabled);

    /**
     * @brief Check if real-time updates are enabled
     * @return true if enabled
     */
    bool isRealTimeEnabled() const { return m_realTimeEnabled; }

    /**
     * @brief Get frame statistics
     * @return Statistics map (total, errors, rate, etc.)
     */
    QVariantMap getFrameStatistics() const;

    /**
     * @brief Get current frame processing rate
     * @return Frames per second
     */
    double getCurrentFrameRate() const { return m_currentFrameRate; }

signals:
    /**
     * @brief Emitted when frame selection should change
     * @param frameNumber Frame number
     */
    void frameSelectionRequested(quint64 frameNumber);

    /**
     * @brief Emitted when frame error is detected
     * @param frameNumber Frame number
     * @param errorMessage Error description
     */
    void frameErrorDetected(quint64 frameNumber, const QString& errorMessage);

    /**
     * @brief Emitted when frame statistics update
     * @param stats Updated statistics
     */
    void statisticsChanged(const QVariantMap& stats);

    /**
     * @brief Emitted when frame processing rate changes
     * @param fps Current frames per second
     */
    void frameRateChanged(double fps);

private slots:
    /**
     * @brief Handle ETISnoop wrapper frame signals
     */
    void onFrameProcessed(const eti::modern::ETIParseResult& analysis);
    void onProcessingError(const QString& error);
    void onStatsUpdated(const eti::modern::ModernETIFrameParser::PerformanceStats& stats);

    /**
     * @brief Update frame statistics
     */
    void updateStatistics();

private:
    /**
     * @brief Convert ETISnoop frame analysis to internal format
     * @param analysis ETISnoop frame analysis
     * @return Internal frame info
     */
    FrameInfo convertFromEtisnoopFrame(const eti::modern::ETIParseResult& analysis) const;

    /**
     * @brief Get frame type from raw data
     * @param frameData Raw frame data
     * @return Detected frame type
     */
    FrameType detectFrameType(const QByteArray& frameData) const;

    /**
     * @brief Extract FIC information from frame
     * @param frameData Raw frame data
     * @return FIC information string
     */
    QString extractFicInfo(const QByteArray& frameData) const;

    /**
     * @brief Extract MSC information from frame
     * @param frameData Raw frame data
     * @return MSC information string
     */
    QString extractMscInfo(const QByteArray& frameData) const;

    /**
     * @brief Calculate frame quality score
     * @param frame Frame information
     * @return Quality score (0.0-100.0)
     */
    double calculateQualityScore(const FrameInfo& frame) const;

    /**
     * @brief Get color for frame status
     * @param status Frame status
     * @return Color for display
     */
    QColor getStatusColor(FrameStatus status) const;

    /**
     * @brief Get icon for frame type
     * @param type Frame type
     * @return Icon for display
     */
    QIcon getFrameTypeIcon(FrameType type) const;

    /**
     * @brief Format frame data for display
     * @param frame Frame information
     * @param column Column index
     * @return Formatted display text
     */
    QString formatFrameData(const FrameInfo& frame, int column) const;

    /**
     * @brief Check if frame matches service filter
     * @param frame Frame information
     * @return true if frame should be shown
     */
    bool frameMatchesFilter(const FrameInfo& frame) const;

    /**
     * @brief Trim old frames if over limit
     */
    void trimFramesIfNeeded();

    /**
     * @brief Update frame rate calculation
     */
    void updateFrameRate();

    /**
     * @brief Validate frame data integrity
     * @param frame Frame information
     * @return true if frame data is valid
     */
    bool validateFrameData(const FrameInfo& frame) const;

    // Core components
    // Modern ETI Core Engine components
    eti::modern::ModernETIFrameParser *m_frameParser;
    eti::compliance::ComprehensiveETSIValidator *m_etsiValidator;

    // Frame data storage
    QList<FrameInfo> m_frames;
    QHash<quint64, int> m_frameIndexMap; // frameNumber -> index mapping
    QList<quint64> m_errorFrames;
    QList<quint32> m_serviceFilter;

    // Configuration
    int m_maxFrames;
    bool m_realTimeEnabled;

    // Statistics and performance tracking
    mutable QMutex m_dataMutex;
    QTimer *m_statsTimer;
    int m_totalFrames;
    int m_errorFrameCount;
    double m_currentFrameRate;
    QDateTime m_lastFrameTime;
    QList<QDateTime> m_recentFrameTimes; // For rate calculation

    // Constants
    static constexpr int DEFAULT_MAX_FRAMES = 10000;
    static constexpr int STATS_UPDATE_INTERVAL_MS = 1000;
    static constexpr int FRAME_RATE_WINDOW_SIZE = 100;
    static constexpr int ETI_FRAME_SIZE = 6144; // Standard ETI frame size
};

Q_DECLARE_METATYPE(EtiFrameListModel::FrameType)
Q_DECLARE_METATYPE(EtiFrameListModel::FrameStatus)
Q_DECLARE_METATYPE(EtiFrameListModel::FrameInfo)