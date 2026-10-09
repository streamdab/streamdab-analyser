#include "eti_processing_worker.h"
#include "enhanced_eti_processor_qt.h"
#include <QFile>
#include <QDebug>
#include <QMutexLocker>
#include <QCoreApplication>

ETIProcessingWorker::ETIProcessingWorker(QObject* parent)
    : QThread(parent)
    , m_stopRequested(false)
    , m_processing(false)
    , m_totalFrames(0)
    , m_currentFrameIndex(0)
    , m_processedFrameCount(0)
    , m_errorCount(0)
{
    // Create batch timer for UI updates
    m_batchTimer = new QTimer();
    m_batchTimer->setInterval(BATCH_INTERVAL_MS);
    m_batchTimer->setSingleShot(false);
    
    // Connect timer to batch processing
    connect(m_batchTimer, &QTimer::timeout, this, &ETIProcessingWorker::onBatchTimer);
    
    // Move timer to this thread
    m_batchTimer->moveToThread(this);
    
    qDebug() << "🧵 ETI Processing Worker created";
}

ETIProcessingWorker::~ETIProcessingWorker()
{
    stopProcessing();
    quit();
    wait(5000);  // Wait up to 5 seconds for thread to finish
    
    if (m_batchTimer) {
        delete m_batchTimer;
    }
    
    qDebug() << "🧵 ETI Processing Worker destroyed";
}

void ETIProcessingWorker::processFile(const QString& filePath)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_processing) {
        qDebug() << "⚠️ Worker already processing, stopping current job";
        stopProcessing();
    }
    
    m_currentFilePath = filePath;
    m_stopRequested = false;
    
    if (!isRunning()) {
        start();
    } else {
        // Trigger processing on the worker thread
        QMetaObject::invokeMethod(this, "startFileProcessing", Qt::QueuedConnection);
    }
    
    qDebug() << "🚀 Worker scheduled to process:" << filePath;
}

void ETIProcessingWorker::stopProcessing()
{
    QMutexLocker locker(&m_mutex);
    m_stopRequested = true;
    m_condition.wakeOne();
    
    if (m_batchTimer && m_batchTimer->isActive()) {
        m_batchTimer->stop();
    }
}

bool ETIProcessingWorker::isProcessing() const
{
    QMutexLocker locker(&m_mutex);
    return m_processing;
}

void ETIProcessingWorker::run()
{
    qDebug() << "🧵 ETI Processing Worker thread started";
    
    // Process any pending file when thread starts
    QTimer::singleShot(0, this, [this]() {
        QMutexLocker locker(&m_mutex);
        if (!m_currentFilePath.isEmpty() && !m_stopRequested) {
            qDebug() << "🔄 Starting processing for:" << m_currentFilePath;
            locker.unlock();
            processFramesBatch();  // Start the actual processing
        }
    });
    
    exec();  // Start event loop for timer
    
    qDebug() << "🧵 ETI Processing Worker thread finished";
}

bool ETIProcessingWorker::loadETIFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit processingError(QString("Cannot open file: %1").arg(filePath));
        return false;
    }
    
    m_fileData = file.readAll();
    file.close();
    
    if (m_fileData.isEmpty()) {
        emit processingError("File is empty or cannot be read");
        return false;
    }
    
    // Calculate total frames (ETI frame size is 6144 bytes)
    m_totalFrames = m_fileData.size() / 6144;
    m_currentFrameIndex = 0;
    m_processedFrameCount = 0;
    m_errorCount = 0;
    
    if (m_totalFrames == 0) {
        emit processingError("No valid ETI frames found in file");
        return false;
    }
    
    qDebug() << "📁 Loaded ETI file:" << filePath;
    qDebug() << "📊 File size:" << m_fileData.size() << "bytes";
    qDebug() << "🎯 Total frames:" << m_totalFrames;
    
    return true;
}

void ETIProcessingWorker::processFramesBatch()
{
    if (m_stopRequested || m_currentFrameIndex >= m_totalFrames) {
        return;
    }
    
    m_processing = true;
    
    // Load file if needed
    if (m_fileData.isEmpty() && !loadETIFile(m_currentFilePath)) {
        m_processing = false;
        return;
    }
    
    // Emit processing started signal
    if (m_currentFrameIndex == 0) {
        emit processingStarted(m_totalFrames);
        m_processingStartTime = QDateTime::currentDateTime();
        
        // Start batch timer
        m_batchTimer->start();
        qDebug() << "⚡ Started batch processing with" << BATCH_INTERVAL_MS << "ms intervals";
    }
    
    // Process batch of frames
    int batchEnd = qMin(m_currentFrameIndex + BATCH_SIZE, m_totalFrames);
    
    for (int i = m_currentFrameIndex; i < batchEnd && !m_stopRequested; ++i) {
        int frameOffset = i * 6144;
        QByteArray frameData = m_fileData.mid(frameOffset, 6144);
        
        if (processEtiFrame(frameData, i + 1)) {
            m_processedFrameCount++;
        } else {
            m_errorCount++;
        }
        
        // Allow other events to be processed
        QCoreApplication::processEvents();
    }
    
    m_currentFrameIndex = batchEnd;
    
    // Emit progress
    int percentage = (m_currentFrameIndex * 100) / m_totalFrames;
    emit progressUpdated(percentage, m_currentFrameIndex, m_totalFrames);
    
    qDebug() << "📈 Batch processed:" << m_currentFrameIndex << "/" << m_totalFrames 
             << "(" << percentage << "%)";
    
    // Check if processing complete
    if (m_currentFrameIndex >= m_totalFrames) {
        m_batchTimer->stop();
        emitBatchedResults();  // Emit any remaining frames
        
        auto endTime = QDateTime::currentDateTime();
        int processingTimeMs = m_processingStartTime.msecsTo(endTime);
        
        emit processingCompleted(m_totalFrames, processingTimeMs);
        m_processing = false;
        
        qDebug() << "✅ Processing completed:" << m_totalFrames << "frames in" << processingTimeMs << "ms";
    }
}

bool ETIProcessingWorker::processEtiFrame(const QByteArray& frameData, int frameNumber)
{
    if (frameData.size() != 6144) {
        qDebug() << "❌ Invalid frame size:" << frameData.size() << "bytes for frame" << frameNumber;
        return false;
    }
    
    // Create ProcessedFrame structure
    ProcessedFrame frame;
    frame.frame_number = frameNumber;
    frame.format = ETIFormat::ETI_NI;  // Default to ETI-NI
    frame.sync_pattern = 0x49931E03;   // Standard ETI sync
    frame.is_valid = true;
    frame.timestamp = QDateTime::currentDateTime();
    // frame.frame_size = frameData.size();  // Not needed in ProcessedFrame structure
    
    // Extract basic ETI header information
    if (frameData.size() >= 4) {
        const uint8_t* data = reinterpret_cast<const uint8_t*>(frameData.constData());
        uint32_t sync = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
        frame.sync_pattern = sync;
        
        // Determine ETI format based on sync pattern
        if (sync == 0x49931E03) {
            frame.format = ETIFormat::ETI_NI;
        } else if (sync == 0xFF1F49C5) {
            frame.format = ETIFormat::ETI_LI_A;
        } else if (sync == 0x99B5C7CC) {
            frame.format = ETIFormat::ETI_LI_B;
        } else {
            frame.is_valid = false;
            qDebug() << "⚠️ Unknown sync pattern:" << QString::number(sync, 16) << "in frame" << frameNumber;
        }
    }
    
    // Add to pending batch
    m_pendingFrames.append(frame);
    
    // Emit format detection for first frame
    if (frameNumber == 1) {
        emit formatDetected(frame.format, formatToString(frame.format));
        qDebug() << "🎯 ETI Format detected:" << formatToString(frame.format);
    }
    
    return frame.is_valid;
}

void ETIProcessingWorker::onBatchTimer()
{
    // Continue processing next batch
    processFramesBatch();
    
    // Emit batched results if we have enough frames
    if (m_pendingFrames.size() >= BATCH_SIZE || m_currentFrameIndex >= m_totalFrames) {
        emitBatchedResults();
    }
}

void ETIProcessingWorker::startFileProcessing()
{
    QMutexLocker locker(&m_mutex);
    if (!m_currentFilePath.isEmpty() && !m_stopRequested && !m_processing) {
        qDebug() << "🔄 Worker thread starting file processing:" << m_currentFilePath;
        locker.unlock();
        processFramesBatch();  // Start the actual processing
    }
}

void ETIProcessingWorker::emitBatchedResults()
{
    if (!m_pendingFrames.isEmpty()) {
        qDebug() << "📤 Emitting batch of" << m_pendingFrames.size() << "frames to UI";
        emit framesBatchProcessed(m_pendingFrames);
        m_pendingFrames.clear();
    }
}

QString ETIProcessingWorker::formatToString(ETIFormat format) const
{
    switch (format) {
        case ETIFormat::ETI_NI:    return "ETI-NI (Network Independent)";
        case ETIFormat::ETI_LI_A:  return "ETI-LI-A (Linear Independent A)";
        case ETIFormat::ETI_LI_B:  return "ETI-LI-B (Linear Independent B)";
        default:                   return "Unknown ETI Format";
    }
}