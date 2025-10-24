// /home/seksan/workspace/streamdab-analyzer/src/core/eti_reader_thread.cpp
#include "eti_reader_thread.hpp"
#include "enhanced_eti_processor_qt.h"
#include "advanced_fig_analyzer.h"
#include <QElapsedTimer>
#include <QDebug>

ETIReaderThread::ETIReaderThread(QObject *parent)
    : QThread(parent)
    , m_running(false)
    , m_paused(false)
    , m_inputMode(InputMode::FileMode)
    , m_udpPort(12000)
    , m_currentFrame(0)
    , m_framesProcessed(0)
    , m_processingTimeMs(0)
{
    // Register metatypes for signal/slot across threads
    qRegisterMetaType<QSharedPointer<FICDataSignal>>("QSharedPointer<FICDataSignal>");
    qRegisterMetaType<QSharedPointer<MSCData>>("QSharedPointer<MSCData>");
    qRegisterMetaType<ErrorStats>("ErrorStats");
}

ETIReaderThread::~ETIReaderThread()
{
    stopProcessing();
    wait(5000);
}

void ETIReaderThread::setFilePath(const QString &filePath)
{
    QMutexLocker locker(&m_controlMutex);
    m_filePath = filePath;
}

void ETIReaderThread::setInputMode(InputMode mode)
{
    QMutexLocker locker(&m_controlMutex);
    m_inputMode = mode;
}

void ETIReaderThread::startProcessing()
{
    QMutexLocker locker(&m_controlMutex);
    if (!m_running) {
        m_running = true;
        m_paused = false;
        start();
    }
}

void ETIReaderThread::stopProcessing()
{
    QMutexLocker locker(&m_controlMutex);
    if (m_running) {
        m_running = false;
        quit();
    }
}

void ETIReaderThread::pauseProcessing()
{
    QMutexLocker locker(&m_controlMutex);
    m_paused = true;
}

void ETIReaderThread::resumeProcessing()
{
    QMutexLocker locker(&m_controlMutex);
    m_paused = false;
}

void ETIReaderThread::run()
{
    // Initialize in worker thread context
    m_processor = std::make_unique<EnhancedETIProcessorQt>();
    m_figAnalyzer = std::make_unique<AdvancedFIGAnalyzer>();
    
    m_currentFrame = 0;
    m_framesProcessed = 0;
    m_elapsedTimer.start();
    
    // Open input source
    bool opened = false;
    {
        QMutexLocker locker(&m_controlMutex);
        if (m_inputMode == InputMode::FileMode) {
            opened = openFile();
        } else {
            opened = openUDPSocket();
        }
    }
    
    if (!opened) {
        emit processingError("Failed to open input source");
        return;
    }
    
    emit processingStarted();
    
    // Create QTimer in worker thread context (Pattern 3)
    QTimer timer;
    timer.setInterval(FRAME_INTERVAL_MS);  // 24ms = 41 fps
    timer.setTimerType(Qt::PreciseTimer);
    
    connect(&timer, &QTimer::timeout, this, &ETIReaderThread::processNextFrame);
    
    timer.start();
    
    // Enter event loop (blocks until quit())
    exec();
    
    // Cleanup
    timer.stop();
    
    {
        QMutexLocker locker(&m_controlMutex);
        if (m_inputMode == InputMode::FileMode) {
            closeFile();
        } else {
            closeUDPSocket();
        }
    }
    
    m_processingTimeMs = m_elapsedTimer.elapsed();
    emit processingComplete(m_framesProcessed, m_processingTimeMs);
}

void ETIReaderThread::processNextFrame()
{
    // Check if paused
    {
        QMutexLocker locker(&m_controlMutex);
        if (m_paused) {
            return;
        }
        if (!m_running) {
            quit();
            return;
        }
    }
    
    // Read frame
    QByteArray frameData;
    bool success = false;
    
    {
        QMutexLocker locker(&m_controlMutex);
        if (m_inputMode == InputMode::FileMode) {
            success = readFrame(frameData);
        } else {
            success = receiveFrame(frameData);
        }
    }
    
    if (!success) {
        // End of file or error
        quit();
        return;
    }
    
    // Process frame with existing ETI processor
    if (m_processor && frameData.size() == ETI_FRAME_SIZE) {
        // Use public processLiveFrame method (Phase 4 API)
        bool success = m_processor->processLiveFrame(frameData);
        
        if (success) {
            m_currentFrame++;
            m_framesProcessed++;
            
            // Emit frame received signal
            emit frameReceived(m_currentFrame);
            
            // Note: The EnhancedETIProcessorQt emits its own signals:
            // - frameProcessed(ProcessedFrame)
            // - figDiscovered(FIGInfo)
            // The widgets connect to those signals directly
            
            // For Phase A, we just forward basic frame count
            // Phase B will add DLS+ extraction
            // Phase C will add live streaming
        }
    }
}

bool ETIReaderThread::openFile()
{
    m_file.setFileName(m_filePath);
    if (!m_file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open ETI file:" << m_filePath;
        return false;
    }
    qDebug() << "Opened ETI file:" << m_filePath << "Size:" << m_file.size() << "bytes";
    return true;
}

void ETIReaderThread::closeFile()
{
    if (m_file.isOpen()) {
        m_file.close();
    }
}

bool ETIReaderThread::readFrame(QByteArray &buffer)
{
    if (!m_file.isOpen()) {
        return false;
    }
    
    buffer = m_file.read(ETI_FRAME_SIZE);
    
    if (buffer.size() < ETI_FRAME_SIZE) {
        // End of file or error
        return false;
    }
    
    return true;
}

bool ETIReaderThread::openUDPSocket()
{
    // Stub for Phase C (Live Stream Input)
    qDebug() << "UDP mode not yet implemented (Phase C)";
    return false;
}

void ETIReaderThread::closeUDPSocket()
{
    // Stub for Phase C
}

bool ETIReaderThread::receiveFrame(QByteArray &buffer)
{
    // Stub for Phase C
    return false;
}

// Phase A Stub Methods - Future phases will implement these
// For now, ETIReaderThread delegates to EnhancedETIProcessorQt signals

QSharedPointer<FICDataSignal> ETIReaderThread::extractFICData()
{
    // Stub for Phase A
    // Future: Extract from processor signals
    return nullptr;
}

QSharedPointer<MSCData> ETIReaderThread::extractMSCData()
{
    // Stub for Phase A
    // Future: Extract from processor signals
    return nullptr;
}

ErrorStats ETIReaderThread::extractErrorStats()
{
    // Stub for Phase A
    // Future: Aggregate from processor
    return ErrorStats();
}

EnsembleInfo ETIReaderThread::extractEnsembleInfo()
{
    // Stub for Phase A
    // Future: Extract from FIG analyzer via signals
    return EnsembleInfo();
}

QList<ServiceInfo> ETIReaderThread::extractServiceList()
{
    // Stub for Phase A
    // Future: Extract from FIG analyzer via signals
    return QList<ServiceInfo>();
}
