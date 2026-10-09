#include "eti_file_worker.hpp"
#include "eti_processor.hpp"
#include "utils/logger.h"

#include <QFileInfo>
#include <QTimer>
#include <QElapsedTimer>

EtiFileWorker::EtiFileWorker(EtiProcessor* processor, QObject *parent)
    : QObject(parent)
    , m_etiProcessor(processor)
    , m_processing(false)
    , m_cancelled(false)
    , m_progress(0)
    , m_frameCount(0)
{
    // Connect ETI processor signals for progress updates
    if (m_etiProcessor) {
        connect(m_etiProcessor, &EtiProcessor::frameProcessed,
                this, [this](const eti::ProcessedFrame& frame) {
                    QMutexLocker locker(&m_mutex);
                    m_frameCount = frame.frame_number;
                    
                    // Calculate progress (rough estimate based on file size processing)
                    // This will be more accurate when we know total frames
                    m_progress = qMin(99, static_cast<int>((frame.frame_number * 100) / qMax(1ULL, frame.frame_number + 1000)));
                    
                    emit processingProgress(m_progress, m_frameCount);
                });
    }
}

EtiFileWorker::~EtiFileWorker()
{
    QMutexLocker locker(&m_mutex);
    if (m_processing) {
        m_cancelled = true;
    }
}

bool EtiFileWorker::isProcessing() const
{
    QMutexLocker locker(&m_mutex);
    return m_processing;
}

int EtiFileWorker::getProgress() const
{
    QMutexLocker locker(&m_mutex);
    return m_progress;
}

void EtiFileWorker::processEtiFile(const QString& fileName)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_processing) {
        Logger::instance().log(Logger::Warning, "EtiFileWorker", 
                              "Processing already in progress, ignoring new request");
        return;
    }
    
    if (!m_etiProcessor || !m_etiProcessor->is_initialized()) {
        emit processingFailed(fileName, "ETI processor not initialized");
        return;
    }
    
    QFileInfo fileInfo(fileName);
    if (!fileInfo.exists() || !fileInfo.isReadable()) {
        emit processingFailed(fileName, "File not found or not readable");
        return;
    }
    
    // Setup for processing
    m_currentFile = fileName;
    m_processing = true;
    m_cancelled = false;
    m_progress = 0;
    m_frameCount = 0;
    
    locker.unlock();
    
    Logger::instance().log(Logger::Info, "EtiFileWorker", 
                          QString("Starting background processing of: %1").arg(fileName));
    
    emit processingStarted(fileName);
    
    // Start processing in next event loop iteration (asynchronous)
    QTimer::singleShot(0, this, &EtiFileWorker::doProcessing);
}

void EtiFileWorker::cancelProcessing()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_processing) {
        m_cancelled = true;
        Logger::instance().log(Logger::Info, "EtiFileWorker", 
                              QString("Cancelling processing of: %1").arg(m_currentFile));
    }
}

void EtiFileWorker::doProcessing()
{
    QString fileName;
    {
        QMutexLocker locker(&m_mutex);
        fileName = m_currentFile;
        
        if (m_cancelled) {
            m_processing = false;
            emit processingCancelled(fileName);
            return;
        }
    }
    
    QElapsedTimer timer;
    timer.start();
    
    Logger::instance().log(Logger::Info, "EtiFileWorker", 
                          QString("Processing ETI file in background thread: %1").arg(fileName));
    
    try {
        // Process the file using ETI processor with enhanced error handling
        bool success = false;
        quint64 finalFrameCount = 0;
        
        try {
            success = m_etiProcessor->process_file(fileName);
            
            {
                QMutexLocker locker(&m_mutex);
                if (m_cancelled) {
                    m_processing = false;
                    emit processingCancelled(fileName);
                    return;
                }
            }
            
            // Safely get frame count with error protection
            try {
                finalFrameCount = m_etiProcessor->get_frame_count();
            } catch (...) {
                // If frame count retrieval fails, estimate from progress
                finalFrameCount = m_frameCount;
                Logger::instance().log(Logger::Warning, "EtiFileWorker", 
                                      "Frame count retrieval failed, using estimated count");
            }
            
        } catch (const std::exception& processingError) {
            // Handle processing errors but don't crash
            Logger::instance().log(Logger::Warning, "EtiFileWorker", 
                                  QString("Processing completed with errors: %1").arg(processingError.what()));
            
            // Consider it partially successful if we processed some frames
            success = (m_frameCount > 0);
            finalFrameCount = m_frameCount;
        } catch (...) {
            Logger::instance().log(Logger::Warning, "EtiFileWorker", 
                                  "Processing completed with unknown errors");
            success = (m_frameCount > 0);
            finalFrameCount = m_frameCount;
        }
        
        if (success || finalFrameCount > 0) {
            {
                QMutexLocker locker(&m_mutex);
                m_processing = false;
                m_progress = 100;
                m_frameCount = finalFrameCount;
            }
            
            qint64 processingTime = timer.elapsed();
            
            Logger::instance().log(Logger::Info, "EtiFileWorker", 
                                  QString("ETI file processing completed: %1 frames in %2ms")
                                  .arg(finalFrameCount).arg(processingTime));
            
            emit processingProgress(100, finalFrameCount);
            emit processingCompleted(fileName, finalFrameCount);
            
        } else {
            QString errorMsg = "Processing failed - no frames processed";
            try {
                errorMsg = m_etiProcessor->get_status();
            } catch (...) {
                // Ignore status retrieval errors
            }
            
            {
                QMutexLocker locker(&m_mutex);
                m_processing = false;
            }
            
            Logger::instance().log(Logger::Error, "EtiFileWorker", 
                                  QString("ETI file processing failed: %1").arg(errorMsg));
            
            emit processingFailed(fileName, errorMsg);
        }
        
    } catch (const std::exception& e) {
        {
            QMutexLocker locker(&m_mutex);
            m_processing = false;
        }
        
        QString errorMsg = QString("Processing exception: %1").arg(e.what());
        
        Logger::instance().log(Logger::Error, "EtiFileWorker", errorMsg);
        emit processingFailed(fileName, errorMsg);
    }
}