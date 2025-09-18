#include "fic_decoder.h"
#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <cstring>

FicDecoder::FicDecoder(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_status(DecoderStatus::Idle)
    , m_ficCount(0)
    , m_errorRate(0.0)
    , m_errorThreshold(DEFAULT_ERROR_THRESHOLD)
    , m_errorCount(0)
    , m_hasEnsembleInfo(false)
    , m_processingActive(false)
{
    qDebug() << "FIC Decoder initialized";
}

FicDecoder::~FicDecoder()
{
    shutdown();
}

bool FicDecoder::initialize()
{
    qDebug() << "FIC Decoder initialization started";
    
    if (m_initialized) {
        qDebug() << "FIC Decoder already initialized";
        return true;
    }
    
    resetDecoder();
    m_initialized = true;
    m_status = DecoderStatus::Ready;
    emit statusChanged(m_status);
    
    qDebug() << "FIC Decoder initialization completed";
    return true;
}

void FicDecoder::shutdown()
{
    if (!m_initialized) {
        return;
    }
    
    m_initialized = false;
    m_status = DecoderStatus::Idle;
    resetDecoder();
    emit statusChanged(m_status);
    
    qDebug() << "FIC Decoder shutdown completed";
}

bool FicDecoder::processFicData(const QByteArray& ficData)
{
    if (!m_initialized) {
        qWarning() << "FIC Decoder not initialized";
        emit ficError("Decoder not initialized");
        return false;
    }
    
    if (ficData.isEmpty()) {
        qWarning() << "Empty FIC data";
        updateErrorStatistics(true);
        emit ficError("Empty FIC data");
        return false;
    }
    
    m_status = DecoderStatus::Processing;
    emit statusChanged(m_status);
    
    bool success = false;
    
    // Add to buffer
    m_ficBuffer.append(ficData);
    
    // Process complete FIG blocks
    while (m_ficBuffer.size() >= FIG_HEADER_SIZE) {
        // Extract FIG length from header
        quint8 figLength = calculateFigLength(m_ficBuffer);
        
        if (figLength == 0 || figLength > m_ficBuffer.size()) {
            // Not enough data for complete FIG
            break;
        }
        
        QByteArray figData = m_ficBuffer.left(figLength);
        m_ficBuffer.remove(0, figLength);
        
        if (validateFigHeader(figData)) {
            // Determine FIG type and process
            quint8 figType = (figData[0] >> 5) & 0x07;
            
            switch (figType) {
                case 0:
                    success = processFig0(figData) || success;
                    break;
                case 1:
                    success = processFig1(figData) || success;
                    break;
                default:
                    qDebug() << "Unsupported FIG type:" << figType;
                    break;
            }
            
            m_ficCount++;
        } else {
            updateErrorStatistics(true);
            emit ficError("Invalid FIG header");
        }
    }
    
    // Limit buffer size
    if (m_ficBuffer.size() > MAX_FIC_BUFFER_SIZE) {
        m_ficBuffer.clear();
        qWarning() << "FIC buffer overflow, clearing";
    }
    
    updateErrorStatistics(!success);
    m_lastProcessTime = QDateTime::currentDateTime();
    
    m_status = DecoderStatus::Ready;
    emit statusChanged(m_status);
    
    return success;
}

bool FicDecoder::processEtiFrame(const EtiFrameData& frame)
{
    // Extract FIC data from ETI frame and process
    // ETI frame contains FIC in specific location
    QByteArray ficData; // Extract from frame.data based on ETI structure
    
    // For now, assume FIC data is available in the frame
    if (frame.data.size() >= 32) {
        ficData = frame.data.left(32); // Simplified extraction
    }
    
    return processFicData(ficData);
}

bool FicDecoder::processFigBlock(const FigBlock& figBlock)
{
    // Convert std::vector<uint8_t> to QByteArray
    QByteArray ficData(reinterpret_cast<const char*>(figBlock.data.data()), figBlock.data.size());
    return processFicData(ficData);
}

// Information retrieval methods
QList<FicDecoder::ServiceInfo> FicDecoder::getDiscoveredServices() const
{
    return m_services.values();
}

FicDecoder::ServiceInfo FicDecoder::getServiceInfo(quint32 serviceId) const
{
    return m_services.value(serviceId, ServiceInfo());
}

FicDecoder::EnsembleInfo FicDecoder::getEnsembleInfo() const
{
    return m_ensembleInfo;
}

QList<FicDecoder::ComponentInfo> FicDecoder::getServiceComponents(quint32 serviceId) const
{
    return m_serviceComponents.value(serviceId, QList<ComponentInfo>());
}

// Service management methods
bool FicDecoder::hasService(quint32 serviceId) const
{
    return m_services.contains(serviceId);
}

bool FicDecoder::hasEnsembleInfo() const
{
    return m_hasEnsembleInfo;
}

QString FicDecoder::getServiceLabel(quint32 serviceId) const
{
    ServiceInfo service = getServiceInfo(serviceId);
    return service.serviceLabel;
}

QString FicDecoder::getEnsembleLabel() const
{
    return m_ensembleInfo.ensembleLabel;
}

quint32 FicDecoder::getFigTypeCount(FigType type) const
{
    return m_figTypeStats.value(type, 0);
}

// Private slots
void FicDecoder::handleFicProcessed()
{
    // Handle FIC processing completion
    qDebug() << "FIC processing completed";
}

void FicDecoder::handleFicError(const QString& error)
{
    qWarning() << "FIC processing error:" << error;
    updateErrorStatistics(true);
}

// Private FIG processing methods
bool FicDecoder::processFig0(const QByteArray& figData)
{
    if (figData.size() < 2) {
        return false;
    }
    
    quint8 extension = figData[1] & 0x1F;
    
    switch (extension) {
        case 1:
            return processFig0Type1(figData.mid(2)); // Service info
        case 2:
            return processFig0Type2(figData.mid(2)); // Service components
        default:
            qDebug() << "Unsupported FIG 0 extension:" << extension;
            return false;
    }
}

bool FicDecoder::processFig1(const QByteArray& figData)
{
    if (figData.size() < 2) {
        return false;
    }
    
    quint8 extension = figData[1] & 0x1F;
    
    switch (extension) {
        case 1:
            return processFig1Type1(figData.mid(2)); // Service labels
        default:
            qDebug() << "Unsupported FIG 1 extension:" << extension;
            return false;
    }
}

bool FicDecoder::processFig0Type1(const QByteArray& data)
{
    // Process basic service information (FIG 0/1)
    if (data.size() < 4) {
        return false;
    }
    
    ServiceInfo service;
    service.serviceId = (data[0] << 8) | data[1];
    service.programmeType = (data[2] >> 3) & 0x1F;
    service.isDataService = (data[2] >> 2) & 0x01;
    service.isLocal = (data[2] >> 1) & 0x01;
    service.lastUpdated = QDateTime::currentDateTime();
    
    addOrUpdateService(service);
    
    m_figTypeStats[FigType::BasicServiceInfo]++;
    
    qDebug() << "Processed service info for ID:" << Qt::hex << service.serviceId;
    return true;
}

bool FicDecoder::processFig0Type2(const QByteArray& data)
{
    // Process service component information (FIG 0/2)
    if (data.size() < 6) {
        return false;
    }
    
    ComponentInfo component;
    component.serviceId = (data[0] << 8) | data[1];
    component.subChannelId = (data[2] >> 2) & 0x3F;
    component.startAddress = ((data[2] & 0x03) << 8) | data[3];
    component.length = (data[4] << 8) | data[5];
    component.protectionLevel = (data[6] >> 2) & 0x03;
    component.isProtected = (data[6] >> 1) & 0x01;
    
    addOrUpdateComponent(component);
    
    m_figTypeStats[FigType::ServiceComponent]++;
    
    qDebug() << "Processed service component for service ID:" << Qt::hex << component.serviceId;
    return true;
}

bool FicDecoder::processFig1Type1(const QByteArray& data)
{
    // Process service labels (FIG 1/1)
    if (data.size() < 18) { // 2 bytes service ID + 16 bytes label
        return false;
    }
    
    quint32 serviceId = (data[0] << 8) | data[1];
    QString label = extractLabel(data, 2, 16);
    
    if (m_services.contains(serviceId)) {
        m_services[serviceId].serviceLabel = label;
        m_services[serviceId].shortLabel = extractShortLabel(label);
        m_services[serviceId].lastUpdated = QDateTime::currentDateTime();
        
        emit serviceLabelUpdated(serviceId, label);
        emit servicesUpdated(getDiscoveredServices());
    }
    
    m_figTypeStats[FigType::ServiceLabel]++;
    
    qDebug() << "Processed service label for ID:" << Qt::hex << serviceId << "Label:" << label;
    return true;
}

// Helper methods
QString FicDecoder::extractLabel(const QByteArray& data, int offset, int length) const
{
    if (offset + length > data.size()) {
        return QString();
    }
    
    QByteArray labelData = data.mid(offset, length);
    
    // Remove null termination and trim
    int nullIndex = labelData.indexOf('\0');
    if (nullIndex >= 0) {
        labelData = labelData.left(nullIndex);
    }
    
    return QString::fromLatin1(labelData).trimmed();
}

QString FicDecoder::extractShortLabel(const QString& fullLabel) const
{
    // Extract short label based on DAB standard
    // This is a simplified implementation
    return fullLabel.left(8).trimmed();
}

bool FicDecoder::validateFigHeader(const QByteArray& data) const
{
    if (data.isEmpty()) {
        return false;
    }
    
    quint8 figType = (data[0] >> 5) & 0x07;
    quint8 figLength = data[0] & 0x1F;
    
    // Basic validation
    return figType <= 7 && figLength > 0 && figLength <= data.size();
}

quint8 FicDecoder::calculateFigLength(const QByteArray& data) const
{
    if (data.isEmpty()) {
        return 0;
    }
    
    return (data[0] & 0x1F) + 1; // Length field + header byte
}

// Service management
void FicDecoder::addOrUpdateService(const ServiceInfo& service)
{
    bool isNew = !m_services.contains(service.serviceId);
    m_services[service.serviceId] = service;
    
    if (isNew) {
        qDebug() << "New service discovered:" << Qt::hex << service.serviceId;
    }
    
    emit servicesUpdated(getDiscoveredServices());
}

void FicDecoder::addOrUpdateComponent(const ComponentInfo& component)
{
    QList<ComponentInfo>& components = m_serviceComponents[component.serviceId];
    
    // Find existing component or add new one
    bool found = false;
    for (int i = 0; i < components.size(); ++i) {
        if (components[i].subChannelId == component.subChannelId) {
            components[i] = component;
            found = true;
            break;
        }
    }
    
    if (!found) {
        components.append(component);
    }
    
    emit serviceComponentsUpdated(component.serviceId, components);
}

void FicDecoder::updateEnsembleInfo(const EnsembleInfo& ensemble)
{
    bool wasEmpty = !m_hasEnsembleInfo;
    m_ensembleInfo = ensemble;
    m_hasEnsembleInfo = true;
    
    if (wasEmpty) {
        qDebug() << "Ensemble info discovered:" << ensemble.ensembleLabel;
    }
    
    emit ensembleInfoUpdated(m_ensembleInfo);
}

// Error handling and statistics
void FicDecoder::updateErrorStatistics(bool hasError)
{
    if (hasError) {
        m_errorCount++;
    }
    
    if (m_ficCount > 0) {
        m_errorRate = static_cast<double>(m_errorCount) / m_ficCount;
    }
    
    if (m_errorRate > m_errorThreshold) {
        m_status = DecoderStatus::Error;
        emit statusChanged(m_status);
        emit ficError(QString("Error rate exceeded threshold: %1%").arg(m_errorRate * 100, 0, 'f', 1));
    }
}

void FicDecoder::resetDecoder()
{
    m_services.clear();
    m_serviceComponents.clear();
    m_ensembleInfo = EnsembleInfo();
    m_hasEnsembleInfo = false;
    m_ficCount = 0;
    m_errorCount = 0;
    m_errorRate = 0.0;
    m_figTypeStats.clear();
    m_ficBuffer.clear();
    m_processingActive = false;
    
    qDebug() << "FIC Decoder reset completed";
}