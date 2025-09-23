#include "dab_decoder.h"
#include "../utils/logger.h"
#include <QDebug>
#include <QDateTime>
#include <QtEndian>

using eti::EtiFrameData;

DabDecoder::DabDecoder(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_status(DecoderStatus::Idle)
    , m_currentServiceId(0)
    , m_frameCount(0)
    , m_errorRate(0.0)
    , m_errorCount(0)
    , m_maxBufferSize(DEFAULT_BUFFER_SIZE)
    , m_currentSampleRate(48000)
    , m_synchronized(false)
    , m_lastFrameTime(0)
{
    // Initialize sync pattern for DAB
    m_syncPattern[0] = 0xFF;
    m_syncPattern[1] = 0x1F;
    m_syncPattern[2] = 0xEC;
    m_syncPattern[3] = 0xD6;

    // Initialize statistics
    memset(&m_stats, 0, sizeof(m_stats));

    // Connect internal signals
    connect(this, &DabDecoder::statusChanged, this, &DabDecoder::handleFrameProcessed);
}

DabDecoder::~DabDecoder()
{
    shutdown();
}

bool DabDecoder::initialize()
{
    if (m_initialized) {
        return true;
    }

    qDebug() << "DabDecoder: Initializing DAB decoder";

    // Reset all internal state
    resetDecoder();

    // Initialize audio buffer
    m_audioBuffer.reserve(MAX_AUDIO_BUFFER_SIZE);
    m_tempBuffer.reserve(DEFAULT_BUFFER_SIZE);

    // Set initial status
    m_status = DecoderStatus::Idle;
    m_initialized = true;

    emit statusChanged(m_status);
    qDebug() << "DabDecoder: Initialization complete";

    return true;
}

void DabDecoder::shutdown()
{
    if (!m_initialized) {
        return;
    }

    qDebug() << "DabDecoder: Shutting down";

    // Clear all buffers
    m_audioBuffer.clear();
    m_tempBuffer.clear();
    m_services.clear();
    m_discoveredServices.clear();

    // Reset state
    m_status = DecoderStatus::Idle;
    m_synchronized = false;
    m_currentServiceId = 0;
    m_initialized = false;

    emit statusChanged(m_status);
}

bool DabDecoder::processEtiFrame(const eti::EtiFrameData& frame)
{
    if (!m_initialized) {
        qWarning() << "DabDecoder: Cannot process frame - decoder not initialized";
        return false;
    }

    // Update frame counter
    m_frameCount++;
    m_stats.totalFrames++;

    // Check frame validity
    if (frame.data.isEmpty() || frame.data.size() < 8) {
        m_errorCount++;
        updateErrorStatistics(true);
        emit decodingError("Invalid ETI frame - insufficient data");
        return false;
    }

    // Verify ETI frame sync
    if (!synchronizeToStream(frame)) {
        m_errorCount++;
        updateErrorStatistics(true);
        emit decodingError("ETI frame synchronization lost");
        return false;
    }

    // Update processing status
    if (m_status != DecoderStatus::Processing) {
        m_status = DecoderStatus::Processing;
        emit statusChanged(m_status);
    }

    // Parse service information from FIC
    if (!parseServiceInformation(frame)) {
        qWarning() << "DabDecoder: Failed to parse service information";
        // Don't return false here - frame might still contain valid audio data
    }

    // Process MSC (Main Service Channel) data if service is selected
    if (m_currentServiceId != 0 && m_services.contains(m_currentServiceId)) {
        const ServiceComponent& service = m_services[m_currentServiceId];
        
        // Extract subchannel data based on service configuration
        QByteArray subchannelData = extractSubchannelData(frame, service.subChannelId);
        
        if (!subchannelData.isEmpty()) {
            if (!processSubChannel(service.subChannelId, subchannelData)) {
                m_errorCount++;
                updateErrorStatistics(true);
                return false;
            }
        }
    }

    updateErrorStatistics(false);
    return true;
}

bool DabDecoder::processSubChannel(quint8 subChannelId, const QByteArray& data)
{
    if (!m_initialized || data.isEmpty()) {
        return false;
    }

    // Find the service associated with this subchannel
    ServiceComponent* service = nullptr;
    for (auto& svc : m_services) {
        if (svc.subChannelId == subChannelId) {
            service = &svc;
            break;
        }
    }

    if (!service) {
        qWarning() << "DabDecoder: No service found for subchannel" << subChannelId;
        return false;
    }

    // Decode audio based on codec type
    bool success = false;
    switch (service->codec) {
        case AudioCodec::DAB_MP2:
            success = decodeDabMp2(data);
            break;
        case AudioCodec::DAB_PLUS:
            success = decodeDabPlus(data);
            break;
        default:
            qWarning() << "DabDecoder: Unsupported codec for service" << service->serviceId;
            return false;
    }

    if (success && hasDecodedAudio()) {
        emit audioDataReady(m_audioBuffer, m_currentSampleRate);
    }

    return success;
}

QList<DabDecoder::ServiceComponent> DabDecoder::getAvailableServices() const
{
    return m_discoveredServices;
}

DabDecoder::ServiceComponent DabDecoder::getServiceInfo(quint16 serviceId) const
{
    return m_services.value(serviceId, ServiceComponent());
}

bool DabDecoder::selectService(quint16 serviceId)
{
    if (!m_services.contains(serviceId)) {
        qWarning() << "DabDecoder: Service" << serviceId << "not available";
        return false;
    }

    m_currentServiceId = serviceId;
    const ServiceComponent& service = m_services[serviceId];
    
    qDebug() << "DabDecoder: Selected service" << serviceId 
             << "(" << service.label << ")" 
             << "codec:" << static_cast<int>(service.codec);

    emit serviceSelected(serviceId, service);
    return true;
}

QByteArray DabDecoder::getDecodedAudio()
{
    QByteArray result = m_audioBuffer;
    m_audioBuffer.clear();
    return result;
}

bool DabDecoder::hasDecodedAudio() const
{
    return !m_audioBuffer.isEmpty();
}

void DabDecoder::clearAudioBuffer()
{
    m_audioBuffer.clear();
}

quint32 DabDecoder::getBitrate() const
{
    if (m_currentServiceId != 0 && m_services.contains(m_currentServiceId)) {
        return m_services[m_currentServiceId].bitrate;
    }
    return 0;
}

// Private implementation methods

bool DabDecoder::parseServiceInformation(const eti::EtiFrameData& frame)
{
    // Simplified FIC parsing - in real implementation this would be much more complex
    // This is a basic version that demonstrates the concept
    
    if (frame.data.size() < 12) {
        return false;
    }

    // Look for FIG Type 0/1 (Basic service and service component definition)
    // Note: data and offset will be used in future implementation for actual FIG parsing
    Q_UNUSED(frame.data) // Suppress unused warning until full implementation
    
    // Basic service discovery simulation
    static bool servicesInitialized = false;
    if (!servicesInitialized) {
        // Add some mock services for testing
        ServiceComponent service1;
        service1.serviceId = 0xD001;
        service1.subChannelId = 0;
        service1.codec = AudioCodec::DAB_PLUS;
        service1.bitrate = 128;
        service1.label = "Test Radio 1";
        service1.isProtected = true;
        
        // Add the test service to our component list
        addServiceComponent(service1);
        
        // Create and add a second test service
        ServiceComponent service2;
        service2.serviceId = 0xD002;
        service2.subChannelId = 1;
        service2.codec = AudioCodec::DAB_MP2;
        service2.bitrate = 64;
        service2.label = "Test Radio 2";
        service2.isProtected = false;
        addServiceComponent(service2);
        
        // Mark services as initialized
        servicesInitialized = true;
        
        // Emit the discovered services
        emit servicesDiscovered(m_discoveredServices);
        Logger::instance().log(Logger::Info, "DabDecoder", 
                              QString("Initialized %1 mock services for testing")
                              .arg(m_discoveredServices.size()));
    }
    
    return true;
}

bool DabDecoder::decodeAudioData(const QByteArray& data, AudioCodec codec)
{
    switch (codec) {
        case AudioCodec::DAB_MP2:
            return decodeDabMp2(data);
        case AudioCodec::DAB_PLUS:
            return decodeDabPlus(data);
        default:
            return false;
    }
}

bool DabDecoder::synchronizeToStream(const eti::EtiFrameData& frame)
{
    if (frame.data.size() < 4) {
        return false;
    }

    // Check for ETI sync pattern at the beginning of frame
    const quint8* data = reinterpret_cast<const quint8*>(frame.data.constData());
    
    bool syncFound = (data[0] == m_syncPattern[0] && 
                     data[1] == m_syncPattern[1]);
    
    if (syncFound && !m_synchronized) {
        m_synchronized = true;
        m_status = DecoderStatus::Synchronized;
        qDebug() << "DabDecoder: Stream synchronized";
        emit statusChanged(m_status);
    } else if (!syncFound && m_synchronized) {
        m_synchronized = false;
        m_stats.syncLoss++;
        qWarning() << "DabDecoder: Synchronization lost";
    }

    return syncFound;
}

void DabDecoder::updateErrorStatistics(bool hasError)
{
    if (hasError) {
        m_stats.errorFrames++;
    }
    
    if (m_stats.totalFrames > 0) {
        m_errorRate = static_cast<double>(m_stats.errorFrames) / m_stats.totalFrames;
    }
}

void DabDecoder::resetDecoder()
{
    m_frameCount = 0;
    m_errorCount = 0;
    m_errorRate = 0.0;
    m_synchronized = false;
    m_currentServiceId = 0;
    m_lastFrameTime = 0;
    
    memset(&m_stats, 0, sizeof(m_stats));
    
    m_services.clear();
    m_discoveredServices.clear();
    m_audioBuffer.clear();
    m_tempBuffer.clear();
}

bool DabDecoder::decodeDabMp2(const QByteArray& data)
{
    // Simplified DAB MP2 decoding simulation
    // In a real implementation, this would use a proper MPEG Layer 2 decoder
    
    if (data.size() < 32) {
        return false;
    }
    
    // Generate simulated PCM audio data
    QByteArray audioData;
    audioData.resize(data.size() * 4); // Rough PCM expansion
    
    // Fill with pattern that simulates decoded audio
    qint16* samples = reinterpret_cast<qint16*>(audioData.data());
    for (int i = 0; i < audioData.size() / 2; ++i) {
        samples[i] = qint16((qSin(i * 0.1) * 16384)); // Simple sine wave
    }
    
    m_audioBuffer.append(audioData);
    m_currentSampleRate = 48000;
    
    return true;
}

bool DabDecoder::decodeDabPlus(const QByteArray& data)
{
    // Simplified DAB+ (HE-AAC) decoding simulation
    // In a real implementation, this would use FAAD2 or similar AAC decoder
    
    if (data.size() < 16) {
        return false;
    }
    
    // Generate simulated PCM audio data
    QByteArray audioData;
    audioData.resize(data.size() * 6); // DAB+ typically has better compression
    
    // Fill with pattern that simulates decoded audio
    qint16* samples = reinterpret_cast<qint16*>(audioData.data());
    for (int i = 0; i < audioData.size() / 2; ++i) {
        samples[i] = qint16((qSin(i * 0.05) * 12288)); // Different pattern for DAB+
    }
    
    m_audioBuffer.append(audioData);
    m_currentSampleRate = 48000;
    
    return true;
}

void DabDecoder::addServiceComponent(const ServiceComponent& component)
{
    m_services[component.serviceId] = component;
    
    // Add to discovered services if not already present
    bool found = false;
    for (const auto& existing : m_discoveredServices) {
        if (existing.serviceId == component.serviceId) {
            found = true;
            break;
        }
    }
    
    if (!found) {
        m_discoveredServices.append(component);
    }
}

void DabDecoder::removeServiceComponent(quint16 serviceId)
{
    m_services.remove(serviceId);
    
    // Remove from discovered services
    for (int i = 0; i < m_discoveredServices.size(); ++i) {
        if (m_discoveredServices[i].serviceId == serviceId) {
            m_discoveredServices.removeAt(i);
            break;
        }
    }
}

void DabDecoder::updateServiceComponent(const ServiceComponent& component)
{
    if (m_services.contains(component.serviceId)) {
        m_services[component.serviceId] = component;
        
        // Update in discovered services list
        for (auto& existing : m_discoveredServices) {
            if (existing.serviceId == component.serviceId) {
                existing = component;
                break;
            }
        }
    }
}

QByteArray DabDecoder::extractSubchannelData(const eti::EtiFrameData& frame, quint8 subChannelId)
{
    // Simplified subchannel extraction
    // In real implementation, this would parse the ETI frame structure properly
    
    if (frame.data.size() < 64) {
        return QByteArray();
    }
    
    // Calculate subchannel offset (simplified)
    int offset = 32 + (subChannelId * 64); // Basic offset calculation
    
    if (offset + 64 > frame.data.size()) {
        return QByteArray();
    }
    
    return frame.data.mid(offset, 64);
}

void DabDecoder::handleFrameProcessed()
{
    // Update processing statistics
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (m_lastFrameTime > 0) {
        qint64 processingTime = currentTime - m_lastFrameTime;
        m_stats.processingTime += processingTime;
        
        if (m_stats.totalFrames > 0) {
            m_stats.averageLatency = static_cast<double>(m_stats.processingTime) / m_stats.totalFrames;
        }
    }
    m_lastFrameTime = currentTime;
}

void DabDecoder::handleDecodingError(const QString& error)
{
    qWarning() << "DabDecoder: Decoding error:" << error;
    
    if (m_errorRate > MAX_ERROR_RATE) {
        m_status = DecoderStatus::Error;
        emit statusChanged(m_status);
    }
}