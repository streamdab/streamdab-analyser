// src/core/eti_reader_thread.cpp
#include "eti_reader_thread.hpp"
#include "enhanced_eti_processor_qt.h"
#include "advanced_fig_analyser.h"
#include <QElapsedTimer>
#include <QDebug>

ETIReaderThread::ETIReaderThread(QObject *parent)
    : QThread(parent)
    , m_running(false)
    , m_paused(false)
    , m_inputMode(InputMode::FileMode)
    , m_udpSocket(nullptr)
    , m_udpPort(12000)
    , m_useMulticast(false)
    , m_packetsReceived(0)
    , m_packetsLost(0)
    , m_currentFrame(0)
    , m_framesProcessed(0)
    , m_processingTimeMs(0)
{
    // Register metatypes for signal/slot across threads
    qRegisterMetaType<QSharedPointer<FICDataSignal>>("QSharedPointer<FICDataSignal>");
    qRegisterMetaType<QSharedPointer<MSCData>>("QSharedPointer<MSCData>");
    qRegisterMetaType<ErrorStats>("ErrorStats");
    
    // Phase D: Start throttling timers
    m_frameCounterThrottle.start();
    m_figDataThrottle.start();
    m_errorStatsThrottle.start();
    m_mscDataThrottle.start();
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

// Phase C: UDP configuration methods
void ETIReaderThread::setUDPPort(quint16 port)
{
    QMutexLocker locker(&m_controlMutex);
    m_udpPort = port;
}

void ETIReaderThread::setMulticastAddress(const QString& address)
{
    QMutexLocker locker(&m_controlMutex);
    m_multicastAddress = address;
}

void ETIReaderThread::setUseMulticast(bool enable)
{
    QMutexLocker locker(&m_controlMutex);
    m_useMulticast = enable;
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
    m_figAnalyser = std::make_unique<AdvancedFIGAnalyser>();
    
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
            
            // Phase B: Emit legacy frame received signal (for backward compatibility)
            emit frameReceived(m_currentFrame);
            
            // Phase D: Emit throttled specialized signals (Pattern 7 + Pattern 8)
            
            // Frame counter update (5 fps throttle)
            if (m_frameCounterThrottle.elapsed() >= FRAME_COUNTER_INTERVAL_MS) {
                emit frameCounterUpdated(m_currentFrame);
                m_frameCounterThrottle.restart();
            }
            
            // FIG data update (1 fps throttle)
            if (m_figDataThrottle.elapsed() >= FIG_DATA_INTERVAL_MS) {
                auto figData = extractFICData();
                if (figData) {
                    emit figDataReady(figData);
                }
                m_figDataThrottle.restart();
            }
            
            // Error statistics update (0.5 fps throttle)
            if (m_errorStatsThrottle.elapsed() >= ERROR_STATS_INTERVAL_MS) {
                emit errorStatsReady(m_errorStats);
                m_errorStatsThrottle.restart();
            }
            
            // MSC data update (2 fps throttle)
            if (m_mscDataThrottle.elapsed() >= MSC_DATA_INTERVAL_MS) {
                auto mscData = extractMSCData();
                if (mscData) {
                    emit mscDataReady(mscData);
                }
                m_mscDataThrottle.restart();
            }
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
    // Phase C: Create UDP socket in worker thread context
    m_udpSocket = new QUdpSocket(this);
    
    // Bind to port (allow sharing for multicast)
    if (!m_udpSocket->bind(QHostAddress::AnyIPv4, m_udpPort, 
                           QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        qCritical() << "Phase C: Failed to bind UDP socket to port" << m_udpPort
                    << "Error:" << m_udpSocket->errorString();
        delete m_udpSocket;
        m_udpSocket = nullptr;
        return false;
    }
    
    qDebug() << "Phase C: UDP socket bound to port" << m_udpPort;
    
    // Join multicast group if enabled
    if (m_useMulticast && !m_multicastAddress.isEmpty()) {
        QHostAddress groupAddress(m_multicastAddress);
        if (m_udpSocket->joinMulticastGroup(groupAddress)) {
            qDebug() << "Phase C: Joined multicast group" << m_multicastAddress;
        } else {
            qWarning() << "Phase C: Failed to join multicast group" 
                       << m_multicastAddress
                       << "Error:" << m_udpSocket->errorString();
            // Continue anyway - unicast might still work
        }
    }
    
    // Set socket buffer size for high throughput (2 Mbps = ~256 KB/sec)
    m_udpSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 
                                  256 * 1024); // 256 KB buffer
    
    m_packetsReceived = 0;
    m_packetsLost = 0;
    m_networkBuffer.clear();
    
    qDebug() << "Phase C: UDP streaming ready - waiting for ETI frames...";
    return true;
}

void ETIReaderThread::closeUDPSocket()
{
    // Phase C: Clean shutdown of UDP socket
    if (m_udpSocket) {
        // Leave multicast group if joined
        if (m_useMulticast && !m_multicastAddress.isEmpty()) {
            m_udpSocket->leaveMulticastGroup(QHostAddress(m_multicastAddress));
            qDebug() << "Phase C: Left multicast group" << m_multicastAddress;
        }
        
        m_udpSocket->close();
        delete m_udpSocket;
        m_udpSocket = nullptr;
        
        qDebug() << "Phase C: UDP socket closed. Stats - Received:" 
                 << m_packetsReceived << "Lost:" << m_packetsLost;
    }
}

bool ETIReaderThread::receiveFrame(QByteArray &buffer)
{
    // Phase C: Receive ETI frame from UDP socket
    if (!m_udpSocket || !m_udpSocket->hasPendingDatagrams()) {
        // No data available (normal - QTimer fires faster than network sometimes)
        return false;
    }
    
    // Read all pending datagrams (process latest data)
    while (m_udpSocket->hasPendingDatagrams()) {
        qint64 datagramSize = m_udpSocket->pendingDatagramSize();
        QByteArray datagram;
        datagram.resize(datagramSize);
        
        qint64 bytesRead = m_udpSocket->readDatagram(datagram.data(), datagram.size());
        
        if (bytesRead < 0) {
            qWarning() << "Phase C: UDP read error:" << m_udpSocket->errorString();
            m_packetsLost++;
            continue;
        }
        
        m_packetsReceived++;
        
        // Check if datagram is exactly one ETI frame (most common case)
        if (bytesRead == ETI_FRAME_SIZE) {
            buffer = datagram;
            return true;
        }
        
        // Handle fragmented frames or concatenated frames
        m_networkBuffer.append(datagram);
        
        if (m_networkBuffer.size() >= ETI_FRAME_SIZE) {
            buffer = m_networkBuffer.left(ETI_FRAME_SIZE);
            m_networkBuffer.remove(0, ETI_FRAME_SIZE);
            return true;
        }
    }
    
    // No complete frame available yet
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
    // Future: Extract from FIG analyser via signals
    return EnsembleInfo();
}

QList<ServiceInfo> ETIReaderThread::extractServiceList()
{
    // Stub for Phase A
    // Future: Extract from FIG analyser via signals
    return QList<ServiceInfo>();
}
