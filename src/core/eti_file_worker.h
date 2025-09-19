#pragma once

#include <QObject>
#include <QThread>
#include <QString>
#include <QMutex>

// Forward declaration
class EtiProcessor;

/**
 * @class EtiFileWorker
 * @brief Background worker for ETI file processing
 * 
 * Processes ETI files in a separate thread to avoid blocking the main UI.
 * Provides progress updates and completion signals.
 */
class EtiFileWorker : public QObject
{
    Q_OBJECT

public:
    explicit EtiFileWorker(EtiProcessor* processor, QObject *parent = nullptr);
    ~EtiFileWorker();

    /**
     * @brief Check if worker is currently processing
     * @return true if processing in progress
     */
    bool isProcessing() const;

    /**
     * @brief Get current processing progress (0-100)
     * @return Processing percentage
     */
    int getProgress() const;

public slots:
    /**
     * @brief Start processing an ETI file
     * @param fileName Path to ETI file to process
     */
    void processEtiFile(const QString& fileName);

    /**
     * @brief Cancel current processing operation
     */
    void cancelProcessing();

signals:
    /**
     * @brief Emitted when processing starts
     * @param fileName File being processed
     */
    void processingStarted(const QString& fileName);

    /**
     * @brief Emitted during processing to show progress
     * @param progress Percentage complete (0-100)
     * @param frameCount Current frame count processed
     */
    void processingProgress(int progress, quint64 frameCount);

    /**
     * @brief Emitted when processing completes successfully
     * @param fileName Processed file name
     * @param frameCount Total frames processed
     */
    void processingCompleted(const QString& fileName, quint64 frameCount);

    /**
     * @brief Emitted when processing fails
     * @param fileName File that failed to process
     * @param errorMessage Error description
     */
    void processingFailed(const QString& fileName, const QString& errorMessage);

    /**
     * @brief Emitted when processing is cancelled
     * @param fileName File that was being processed
     */
    void processingCancelled(const QString& fileName);

private slots:
    /**
     * @brief Internal processing method (runs in background thread)
     */
    void doProcessing();

private:
    EtiProcessor* m_etiProcessor;
    QString m_currentFile;
    mutable QMutex m_mutex;
    bool m_processing;
    bool m_cancelled;
    int m_progress;
    quint64 m_frameCount;
};