#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <memory>

/**
 * @class EtiProcessor
 * @brief Core ETI stream processing engine
 * 
 * Handles ETI frame parsing, FIG analysis, and audio extraction
 * following ETSI standards for broadcast industry compliance.
 */
class EtiProcessor : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit EtiProcessor(QObject *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~EtiProcessor();

    /**
     * @brief Initialize the ETI processor
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * @brief Process ETI data from file
     * @param filename Path to ETI file
     * @return true if processing successful
     */
    bool processFile(const QString& filename);

    /**
     * @brief Process ETI data from buffer
     * @param data ETI data buffer
     * @return true if processing successful
     */
    bool processData(const QByteArray& data);

    /**
     * @brief Check if processor is initialized
     * @return true if ready for processing
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Get current frame count
     * @return Number of processed frames
     */
    quint64 getFrameCount() const { return m_frameCount; }

    /**
     * @brief Get processing status
     * @return Current status string
     */
    QString getStatus() const { return m_status; }

signals:
    /**
     * @brief Emitted when frame is processed
     * @param frameNumber Frame number
     * @param data Frame data
     */
    void frameProcessed(quint64 frameNumber, const QByteArray& data);

    /**
     * @brief Emitted when error occurs
     * @param error Error message
     */
    void errorOccurred(const QString& error);

    /**
     * @brief Emitted when processing status changes
     * @param status New status
     */
    void statusChanged(const QString& status);

private:
    bool m_initialized;
    quint64 m_frameCount;
    QString m_status;
    QString m_currentFile;
};