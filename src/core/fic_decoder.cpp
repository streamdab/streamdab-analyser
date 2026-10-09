#include "fic_decoder.hpp"
#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <cstring>

fic_decoder::fic_decoder(QObject *parent)
    : QObject(parent)
    , initialized_(false)
    , status_(DecoderStatus::Idle)
    , fic_count_(0)
    , error_rate_(0.0)
    , error_threshold_(DEFAULT_ERROR_THRESHOLD)
    , error_count_(0)
    , sync_level_(0)
    , has_ensemble_info_(false)
    , processing_active_(false)
{
    qDebug() << "FIC Decoder initialized";
}

fic_decoder::~fic_decoder()
{
    shutdown();
}

bool fic_decoder::initialize()
{
    qDebug() << "FIC Decoder initialization started";
    
    if (initialized_) {
        qDebug() << "FIC Decoder already initialized";
        return true;
    }
    
    resetDecoder();
    initialized_ = true;
    status_ = DecoderStatus::Ready;
    emit statusChanged(status_);
    
    qDebug() << "FIC Decoder initialization completed";
    return true;
}

void fic_decoder::shutdown()
{
    if (!initialized_) {
        return;
    }
    
    initialized_ = false;
    status_ = DecoderStatus::Idle;
    resetDecoder();
    emit statusChanged(status_);
    
    qDebug() << "FIC Decoder shutdown completed";
}

bool fic_decoder::processFicData(const QByteArray& ficData)
{
    if (!initialized_) {
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
    
    status_ = DecoderStatus::Processing;
    emit statusChanged(status_);
    
    bool success = false;
    
    // Add to buffer
    fic_buffer_.append(ficData);
    
    // Process complete FIG blocks
    while (fic_buffer_.size() >= FIG_HEADER_SIZE) {
        // Extract FIG length from header
        quint8 figLength = calculateFigLength(fic_buffer_);
        
        if (figLength == 0 || figLength > fic_buffer_.size()) {
            // Not enough data for complete FIG
            break;
        }
        
        QByteArray figData = fic_buffer_.left(figLength);
        fic_buffer_.remove(0, figLength);
        
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
            
            fic_count_++;
        } else {
            updateErrorStatistics(true);
            emit ficError("Invalid FIG header");
        }
    }
    
    // Limit buffer size
    if (fic_buffer_.size() > MAX_FIC_BUFFER_SIZE) {
        fic_buffer_.clear();
        qWarning() << "FIC buffer overflow, clearing";
    }
    
    updateErrorStatistics(!success);
    last_process_time_ = QDateTime::currentDateTime();
    
    // Update sync level based on processing success
    if (success) {
        sync_level_ = qMin(100, sync_level_ + 2);
        if (sync_level_ >= 75 && status_ != DecoderStatus::Synchronized) {
            status_ = DecoderStatus::Synchronized;
            emit statusChanged(status_);
        } else if (status_ == DecoderStatus::Processing) {
            status_ = DecoderStatus::Ready;
            emit statusChanged(status_);
        }
    } else {
        sync_level_ = qMax(0, sync_level_ - 1);
        if (sync_level_ < 25 && status_ == DecoderStatus::Synchronized) {
            status_ = DecoderStatus::Ready;
            emit statusChanged(status_);
        }
    }
    
    // Emit ficDataProcessed signal
    emit ficDataProcessed();
    
    return success;
}

bool fic_decoder::processEtiFrame(const eti::EtiFrameData& frame)
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

bool fic_decoder::processFigBlock(const eti::FigBlock& figBlock)
{
    // Convert std::vector<uint8_t> to QByteArray
    QByteArray ficData(reinterpret_cast<const char*>(figBlock.data.data()), figBlock.data.size());
    return processFicData(ficData);
}

// Information retrieval methods
QList<fic_decoder::ServiceInfo> fic_decoder::getDiscoveredServices() const
{
    return services_.values();
}

fic_decoder::ServiceInfo fic_decoder::getServiceInfo(quint32 serviceId) const
{
    return services_.value(serviceId, ServiceInfo());
}

fic_decoder::EnsembleInfo fic_decoder::getEnsembleInfo() const
{
    return ensemble_info_;
}

QList<fic_decoder::ComponentInfo> fic_decoder::getServiceComponents(quint32 serviceId) const
{
    return service_components_.value(serviceId, QList<ComponentInfo>());
}

// Service management methods
bool fic_decoder::has_service(quint32 serviceId) const
{
    return services_.contains(serviceId);
}

bool fic_decoder::has_ensemble_info() const
{
    return has_ensemble_info_;
}

QString fic_decoder::get_service_label(quint32 serviceId) const
{
    ServiceInfo service = getServiceInfo(serviceId);
    return service.serviceLabel;
}

QString fic_decoder::get_ensemble_label() const
{
    return ensemble_info_.ensembleLabel;
}



// Private slots
void fic_decoder::handle_fic_processed()
{
    // Handle FIC processing completion
    qDebug() << "FIC processing completed";
}

void fic_decoder::handle_fic_error(const QString& error)
{
    qWarning() << "FIC processing error:" << error;
    updateErrorStatistics(true);
}

// Private FIG processing methods
bool fic_decoder::processFig0(const QByteArray& figData)
{
    if (figData.size() < 2) {
        return false;
    }
    
    quint8 extension = figData[1] & 0x1F;
    
    switch (extension) {
        case 0:
            // Process ensemble information (FIG 0/0)
            if (figData.size() >= 6) {
                ensemble_data_.ensembleId = (figData[2] << 8) | figData[3];
                if (figData.size() > 5) {
                    ensemble_data_.cifCount = figData[5];
                }
                
                // Update main EnsembleInfo structure
                ensemble_info_.ensembleId = ensemble_data_.ensembleId;
                ensemble_info_.cifCount = ensemble_data_.cifCount;
                has_ensemble_info_ = true;
                
                emit ensembleInfoUpdated(ensemble_info_);
                return true;
            }
            return false;
        case 1:
            return process_fig0_type1(figData.mid(2)); // Service info
        case 2:
            return process_fig0_type2(figData.mid(2)); // Service components
        default:
            qDebug() << "Unsupported FIG 0 extension:" << extension;
            return false;
    }
}

bool fic_decoder::processFig1(const QByteArray& figData)
{
    if (figData.size() < 2) {
        return false;
    }
    
    quint8 extension = figData[1] & 0x1F;
    
    switch (extension) {
        case 0:
            // Process ensemble label (FIG 1/0)
            if (figData.size() >= 18) {
                quint16 ensembleId = (figData[2] << 8) | figData[3];
                QString label = extract_label(figData, 4, qMin(figData.size() - 4, 16));
                ensemble_data_.ensembleId = ensembleId;
                ensemble_data_.label = label;
                
                // Update main EnsembleInfo structure
                ensemble_info_.ensembleId = ensembleId;
                ensemble_info_.ensembleLabel = label;
                ensemble_info_.label = label; // Test-compatible alias
                ensemble_info_.shortLabel = extract_short_label(label);
                has_ensemble_info_ = true;
                
                emit ensembleInfoUpdated(ensemble_info_);
                return true;
            }
            return false;
        case 1:
            return process_fig1_type1(figData.mid(2)); // Service labels
        default:
            qDebug() << "Unsupported FIG 1 extension:" << extension;
            return false;
    }
}

bool fic_decoder::process_fig0_type1(const QByteArray& data)
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
    
    add_or_update_service(service);
    
    // Also update test-compatible service structure
    Service testService;
    testService.serviceId = service.serviceId;
    testService.nbServiceComp = 1; // Default to 1 component
    test_services_[service.serviceId] = testService;
    
    emit serviceInfoUpdated(service.serviceId);
    
    fig_type_stats_[FigType::BasicServiceInfo]++;
    
    qDebug() << "Processed service info for ID:" << Qt::hex << service.serviceId;
    return true;
}

bool fic_decoder::process_fig0_type2(const QByteArray& data)
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
    if (data.size() > 6) {
        component.protectionLevel = (data[6] >> 2) & 0x03;
        component.isProtected = (data[6] >> 1) & 0x01;
    }
    
    add_or_update_component(component);
    
    // Also update test-compatible sub-channel structure
    SubChannel subChannel;
    subChannel.subChannelId = component.subChannelId;
    subChannel.startAddress = component.startAddress;
    subChannel.subChannelSize = component.length;
    subChannel.protectionLevel = component.protectionLevel;
    subChannel.isValid = true;
    sub_channels_[component.subChannelId] = subChannel;
    
    emit subchannelInfoUpdated(component.subChannelId);
    
    fig_type_stats_[FigType::ServiceComponent]++;
    
    qDebug() << "Processed service component for service ID:" << Qt::hex << component.serviceId;
    return true;
}

bool fic_decoder::process_fig1_type1(const QByteArray& data)
{
    // Process service labels (FIG 1/1)
    if (data.size() < 18) { // 2 bytes service ID + 16 bytes label
        return false;
    }
    
    quint32 serviceId = (data[0] << 8) | data[1];
    QString label = extract_label(data, 2, 16);
    
    if (services_.contains(serviceId)) {
        services_[serviceId].serviceLabel = label;
        services_[serviceId].shortLabel = extract_short_label(label);
        services_[serviceId].lastUpdated = QDateTime::currentDateTime();
        
        emit serviceLabelUpdated(serviceId, label);
        emit servicesUpdated(getDiscoveredServices());
    }
    
    // Update test-compatible service structure
    if (test_services_.contains(serviceId)) {
        test_services_[serviceId].label = label;
    }
    
    fig_type_stats_[FigType::ServiceLabel]++;
    
    qDebug() << "Processed service label for ID:" << Qt::hex << serviceId << "Label:" << label;
    return true;
}

// Helper methods
QString fic_decoder::extract_label(const QByteArray& data, int offset, int length) const
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

QString fic_decoder::extract_short_label(const QString& fullLabel) const
{
    // Extract short label based on DAB standard
    // This is a simplified implementation
    return fullLabel.left(8).trimmed();
}

bool fic_decoder::validateFigHeader(const QByteArray& data) const
{
    if (data.isEmpty()) {
        return false;
    }
    
    quint8 figType = (data[0] >> 5) & 0x07;
    quint8 figLength = data[0] & 0x1F;
    
    // Basic validation
    return figType <= 7 && figLength > 0 && figLength <= data.size();
}

quint8 fic_decoder::calculateFigLength(const QByteArray& data) const
{
    if (data.isEmpty()) {
        return 0;
    }
    
    return (data[0] & 0x1F) + 1; // Length field + header byte
}

// Service management
void fic_decoder::add_or_update_service(const ServiceInfo& service)
{
    bool isNew = !services_.contains(service.serviceId);
    services_[service.serviceId] = service;
    
    if (isNew) {
        qDebug() << "New service discovered:" << Qt::hex << service.serviceId;
    }
    
    emit servicesUpdated(getDiscoveredServices());
}

void fic_decoder::add_or_update_component(const ComponentInfo& component)
{
    QList<ComponentInfo>& components = service_components_[component.serviceId];
    
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

void fic_decoder::update_ensemble_info(const EnsembleInfo& ensemble)
{
    bool wasEmpty = !has_ensemble_info_;
    ensemble_info_ = ensemble;
    has_ensemble_info_ = true;
    
    if (wasEmpty) {
        qDebug() << "Ensemble info discovered:" << ensemble.ensembleLabel;
    }
    
    emit ensembleInfoUpdated(ensemble_info_);
}

// Error handling and statistics
void fic_decoder::updateErrorStatistics(bool hasError)
{
    if (hasError) {
        error_count_++;
    }
    
    if (fic_count_ > 0) {
        error_rate_ = static_cast<double>(error_count_) / fic_count_;
    }
    
    if (error_rate_ > error_threshold_) {
        status_ = DecoderStatus::Error;
        emit statusChanged(status_);
        emit ficError(QString("Error rate exceeded threshold: %1%").arg(error_rate_ * 100, 0, 'f', 1));
    }
}

void fic_decoder::resetDecoder()
{
    services_.clear();
    service_components_.clear();
    ensemble_info_ = EnsembleInfo();
    has_ensemble_info_ = false;
    fic_count_ = 0;
    error_count_ = 0;
    error_rate_ = 0.0;
    sync_level_ = 0;
    fig_type_stats_.clear();
    fic_buffer_.clear();
    processing_active_ = false;
    
    // Reset test-compatible data structures
    test_services_.clear();
    sub_channels_.clear();
    ensemble_data_ = EnsembleData();
    
    qDebug() << "FIC Decoder reset completed";
}

// Test-compatible interface methods implementation
void fic_decoder::reset()
{
    resetDecoder();
    status_ = DecoderStatus::Idle;
    emit statusChanged(status_);
}

QList<fic_decoder::Service> fic_decoder::get_services() const
{
    return test_services_.values();
}

QList<fic_decoder::SubChannel> fic_decoder::get_sub_channels() const
{
    return sub_channels_.values();
}

fic_decoder::EnsembleData fic_decoder::get_ensemble_data() const
{
    return ensemble_data_;
}

// Note: Several test-compatibility methods were removed as they were not declared 
// in the header file. These included getServiceById, getSubChannelById, 
// isServiceAvailable, and isSubChannelConfigured.