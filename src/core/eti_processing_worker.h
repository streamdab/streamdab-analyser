#pragma once

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QString>
#include <QTimer>
#include <QDateTime>
#include <QByteArray>
#include <memory>

// Forward declarations
struct ProcessedFrame;
enum class ETIFormat;

/**
 * @brief Dedicated worker thread for ETI file processing
 * 
 * This class handles all heavy ETI processing operations on a separate thread
 * to keep the main UI thread responsive. It processes frames in batches and
 * emits periodic updates to prevent UI flooding.
 */
class ETIProcessingWorker : public QThread
{
    Q_OBJECT

public:
    explicit ETIProcessingWorker(QObject* parent = nullptr);
    ~ETIProcessingWorker();

    // Thread-safe methods to control processing
    void processFile(const QString& filePath);
    void stopProcessing();
    bool isProcessing() const;

protected:
    void run() override;

signals:
    // Batched signals to prevent UI flooding
    void framesBatchProcessed(const QVector<ProcessedFrame>& frames);
    void progressUpdated(int percentage, int currentFrame, int totalFrames);
    void processingStarted(int totalFrames);
    void processingCompleted(int totalFrames, int processingTimeMs);
    void processingError(const QString& error);
    void formatDetected(ETIFormat format, const QString& formatString);

private slots:
    void onBatchTimer();
    void startFileProcessing();

private:
    // Core processing methods
    bool loadETIFile(const QString& filePath);
    void processFramesBatch();
    bool processEtiFrame(const QByteArray& frameData, int frameNumber);
    void emitBatchedResults();
    QString formatToString(ETIFormat format) const;

    // Thread synchronization
    mutable QMutex m_mutex;
    QWaitCondition m_condition;
    bool m_stopRequested;
    bool m_processing;

    // File processing data
    QString m_currentFilePath;
    QByteArray m_fileData;
    int m_totalFrames;
    int m_currentFrameIndex;
    QDateTime m_processingStartTime;

    // Batched processing
    QTimer* m_batchTimer;
    QVector<ProcessedFrame> m_pendingFrames;
    static const int BATCH_SIZE = 50;  // Process 50 frames per batch
    static const int BATCH_INTERVAL_MS = 50;  // Update UI every 50ms

    // Processing statistics
    int m_processedFrameCount;
    int m_errorCount;
};