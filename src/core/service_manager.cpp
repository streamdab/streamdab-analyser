#include "service_manager.hpp"
#include "utils/logger.h"

#include <QDebug>
#include <QVariant>
#include <algorithm>

ServiceManager::ServiceManager(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_staleCheckTimer(new QTimer(this))
{
    Logger::instance().log(Logger::Info, "ServiceManager", "Service Manager constructed");

    // Setup stale service checking
    m_staleCheckTimer->setInterval(STALE_CHECK_INTERVAL_MS);
    connect(m_staleCheckTimer, &QTimer::timeout, this, &ServiceManager::checkStaleServices);
}

ServiceManager::~ServiceManager()
{
    shutdown();
    Logger::instance().log(Logger::Info, "ServiceManager", "Service Manager destroyed");
}

bool ServiceManager::initialize()
{
    if (m_initialized) {
        Logger::instance().log(Logger::Warning, "ServiceManager", "Already initialized");
        return true;
    }

    Logger::instance().log(Logger::Info, "ServiceManager", "Initializing Service Manager");

    try {
        // Clear any existing data
        clearAll();

        // Start stale service checking
        m_staleCheckTimer->start();

        m_initialized = true;
        
        Logger::instance().log(Logger::Info, "ServiceManager", "Service Manager initialized successfully");
        return true;

    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "ServiceManager", 
                              QString("Initialization failed: %1").arg(e.what()));
        return false;
    }
}

void ServiceManager::shutdown()
{
    if (!m_initialized) {
        return;
    }

    Logger::instance().log(Logger::Info, "ServiceManager", "Shutting down Service Manager");

    // Stop timers
    m_staleCheckTimer->stop();

    // Clear data
    clearAll();

    m_initialized = false;
    
    Logger::instance().log(Logger::Info, "ServiceManager", "Service Manager shutdown complete");
}

void ServiceManager::updateEnsemble(const eti::Ensemble& ensemble)
{
    if (!m_initialized) {
        Logger::instance().log(Logger::Warning, "ServiceManager", "Not initialized");
        return;
    }

    bool isNewEnsemble = (m_currentEnsemble.ensemble_id != ensemble.ensemble_id);

    if (isNewEnsemble) {
        Logger::instance().log(Logger::Info, "ServiceManager", 
                              QString("New ensemble detected: ID=0x%1, Label='%2'")
                              .arg(ensemble.ensemble_id, 4, 16, QChar('0'))
                              .arg(QString::fromStdString(ensemble.label)));

        // Clear existing services for new ensemble
        m_services.clear();
    }

    m_currentEnsemble = ensemble;

    // Update subchannels from ensemble
    if (!ensemble.sub_channels.empty()) {
        QList<eti::SubChannelInfo> subchannels;
        for (const auto& subchannel : ensemble.sub_channels) {
            subchannels.append(subchannel);
        }
        updateSubchannels(subchannels);
    }

    // Add/update services from ensemble
    for (const auto& service : ensemble.services) {
        addOrUpdateService(service);
    }

    emit ensembleUpdated(m_currentEnsemble);

    Logger::instance().log(Logger::Debug, "ServiceManager", 
                          QString("Ensemble updated: %1 services, %2 subchannels")
                          .arg(ensemble.services.size())
                          .arg(ensemble.sub_channels.size()));
}

void ServiceManager::addOrUpdateService(const eti::DabService& service)
{
    if (!m_initialized) {
        Logger::instance().log(Logger::Warning, "ServiceManager", "Not initialized");
        return;
    }

    if (!validateService(service)) {
        Logger::instance().log(Logger::Warning, "ServiceManager", 
                              QString("Invalid service data: ID=0x%1")
                              .arg(service.service_id, 4, 16, QChar('0')));
        return;
    }

    bool isNewService = !m_services.contains(service.service_id);
    QDateTime currentTime = QDateTime::currentDateTime();

    if (isNewService) {
        ServiceInfo serviceInfo;
        serviceInfo.service = service;
        serviceInfo.status = ServiceStatus::Available;
        serviceInfo.discoveryTime = currentTime;
        serviceInfo.lastSeen = currentTime;

        // Initialize quality metrics
        serviceInfo.quality.isAudioService = service.is_programme;
        serviceInfo.quality.isDataService = !service.is_programme;
        serviceInfo.quality.lastUpdate = currentTime;

        m_services[service.service_id] = serviceInfo;

        Logger::instance().log(Logger::Info, "ServiceManager", 
                              QString("New service discovered: ID=0x%1, Label='%2', Type=%3")
                              .arg(service.service_id, 4, 16, QChar('0'))
                              .arg(QString::fromStdString(service.label))
                              .arg(service.is_programme ? "Programme" : "Data"));

        emit serviceDiscovered(serviceInfo);
    } else {
        // Update existing service
        ServiceInfo& existingService = m_services[service.service_id];
        ServiceStatus oldStatus = existingService.status;

        existingService.service = service;
        existingService.lastSeen = currentTime;

        // Update status
        updateServiceStatus(service.service_id);

        if (existingService.status != oldStatus) {
            emit serviceStatusChanged(service.service_id, oldStatus, existingService.status);
        }

        emit serviceUpdated(existingService);

        Logger::instance().log(Logger::Debug, "ServiceManager", 
                              QString("Service updated: ID=0x%1, Status=%2")
                              .arg(service.service_id, 4, 16, QChar('0'))
                              .arg(static_cast<int>(existingService.status)));
    }

    // Associate subchannels with services
    associateSubchannelsWithServices();
}

void ServiceManager::updateServiceQuality(uint16_t serviceId, const ServiceQuality& quality)
{
    if (!m_initialized) {
        return;
    }

    if (!m_services.contains(serviceId)) {
        Logger::instance().log(Logger::Warning, "ServiceManager", 
                              QString("Quality update for unknown service: ID=0x%1")
                              .arg(serviceId, 4, 16, QChar('0')));
        return;
    }

    ServiceInfo& serviceInfo = m_services[serviceId];
    ServiceStatus oldStatus = serviceInfo.status;

    // Update quality metrics
    serviceInfo.quality = quality;
    serviceInfo.quality.lastUpdate = QDateTime::currentDateTime();
    serviceInfo.lastSeen = QDateTime::currentDateTime();

    // Update service status based on quality
    updateServiceStatus(serviceId);

    emit serviceQualityUpdated(serviceId, quality);

    if (serviceInfo.status != oldStatus) {
        emit serviceStatusChanged(serviceId, oldStatus, serviceInfo.status);
    }

    Logger::instance().log(Logger::Debug, "ServiceManager", 
                          QString("Service quality updated: ID=0x%1, Signal=%2%, Error=%3%")
                          .arg(serviceId, 4, 16, QChar('0'))
                          .arg(quality.signalStrength, 0, 'f', 1)
                          .arg(quality.errorRate, 0, 'f', 2));
}

void ServiceManager::updateSubchannels(const QList<eti::SubChannelInfo>& subchannels)
{
    if (!m_initialized) {
        return;
    }

    m_subchannels = subchannels;

    // Associate subchannels with services
    associateSubchannelsWithServices();

    emit subchannelsUpdated(subchannels);

    Logger::instance().log(Logger::Debug, "ServiceManager", 
                          QString("Subchannels updated: %1 subchannels").arg(subchannels.size()));
}

ServiceManager::ServiceInfo ServiceManager::getService(uint16_t serviceId) const
{
    return m_services.value(serviceId, ServiceInfo());
}

QList<ServiceManager::ServiceInfo> ServiceManager::getAudioServices() const
{
    QList<ServiceInfo> audioServices;
    
    for (const auto& service : m_services.values()) {
        if (service.service.is_programme || service.quality.isAudioService) {
            audioServices.append(service);
        }
    }
    
    return audioServices;
}

QList<ServiceManager::ServiceInfo> ServiceManager::getDataServices() const
{
    QList<ServiceInfo> dataServices;
    
    for (const auto& service : m_services.values()) {
        if (!service.service.is_programme || service.quality.isDataService) {
            dataServices.append(service);
        }
    }
    
    return dataServices;
}

QList<ServiceManager::ServiceInfo> ServiceManager::getServicesByStatus(ServiceStatus status) const
{
    QList<ServiceInfo> filteredServices;
    
    for (const auto& service : m_services.values()) {
        if (service.status == status) {
            filteredServices.append(service);
        }
    }
    
    return filteredServices;
}

void ServiceManager::clearAll()
{
    m_services.clear();
    m_subchannels.clear();
    m_currentEnsemble = eti::Ensemble{};
    
    Logger::instance().log(Logger::Info, "ServiceManager", "All data cleared");
}

QMap<QString, QVariant> ServiceManager::getEnsembleStatistics() const
{
    QMap<QString, QVariant> stats;
    
    stats["ensemble_id"] = QString("0x%1").arg(m_currentEnsemble.ensemble_id, 4, 16, QChar('0'));
    stats["ensemble_label"] = QString::fromStdString(m_currentEnsemble.label);
    stats["country_id"] = m_currentEnsemble.country_id;
    stats["total_services"] = m_services.size();
    stats["audio_services"] = getAudioServices().size();
    stats["data_services"] = getDataServices().size();
    stats["available_services"] = getServicesByStatus(ServiceStatus::Available).size();
    stats["unavailable_services"] = getServicesByStatus(ServiceStatus::Unavailable).size();
    stats["error_services"] = getServicesByStatus(ServiceStatus::Error).size();
    stats["total_subchannels"] = m_subchannels.size();
    
    // Calculate average quality metrics
    double avgSignalStrength = 0.0;
    double avgErrorRate = 0.0;
    int qualityCount = 0;
    
    for (const auto& service : m_services.values()) {
        if (service.quality.lastUpdate.isValid()) {
            avgSignalStrength += service.quality.signalStrength;
            avgErrorRate += service.quality.errorRate;
            qualityCount++;
        }
    }
    
    if (qualityCount > 0) {
        stats["avg_signal_strength"] = avgSignalStrength / qualityCount;
        stats["avg_error_rate"] = avgErrorRate / qualityCount;
    } else {
        stats["avg_signal_strength"] = 0.0;
        stats["avg_error_rate"] = 0.0;
    }
    
    return stats;
}

void ServiceManager::checkStaleServices()
{
    if (!m_initialized) {
        return;
    }

    QDateTime currentTime = QDateTime::currentDateTime();
    QList<uint16_t> staleServices;

    for (auto it = m_services.begin(); it != m_services.end(); ++it) {
        qint64 timeSinceLastSeen = it->lastSeen.msecsTo(currentTime);
        
        if (timeSinceLastSeen > SERVICE_TIMEOUT_MS) {
            if (it->status != ServiceStatus::Unavailable) {
                ServiceStatus oldStatus = it->status;
                it->status = ServiceStatus::Unavailable;
                
                emit serviceStatusChanged(it.key(), oldStatus, ServiceStatus::Unavailable);
                
                Logger::instance().log(Logger::Warning, "ServiceManager", 
                                      QString("Service marked as unavailable: ID=0x%1 (stale for %2ms)")
                                      .arg(it.key(), 4, 16, QChar('0'))
                                      .arg(timeSinceLastSeen));
            }
        }
    }
}

bool ServiceManager::validateService(const eti::DabService& service) const
{
    // Validate according to ETSI EN 300 401
    
    // Service ID validation
    if (!service.validate_service_id()) {
        Logger::instance().log(Logger::Debug, "ServiceManager", 
                              QString("Invalid service ID: 0x%1").arg(service.service_id, 4, 16, QChar('0')));
        return false;
    }

    // Country code validation
    if (!service.validate_country_code()) {
        Logger::instance().log(Logger::Debug, "ServiceManager", 
                              QString("Invalid country code: 0x%1").arg(service.country_id, 2, 16, QChar('0')));
        return false;
    }

    // Service components validation
    for (const auto& component : service.components) {
        if (!component.validate_sub_channel_id() || !component.validate_tmid()) {
            Logger::instance().log(Logger::Debug, "ServiceManager", 
                                  QString("Invalid service component: SubCh=%1, TMID=%2")
                                  .arg(component.sub_channel_id).arg(component.tmid));
            return false;
        }
    }

    return true;
}

void ServiceManager::updateServiceStatus(uint16_t serviceId)
{
    if (!m_services.contains(serviceId)) {
        return;
    }

    ServiceInfo& serviceInfo = m_services[serviceId];
    ServiceStatus newStatus = ServiceStatus::Unknown;

    // Determine status based on quality metrics and activity
    QDateTime currentTime = QDateTime::currentDateTime();
    qint64 timeSinceLastSeen = serviceInfo.lastSeen.msecsTo(currentTime);

    if (timeSinceLastSeen > SERVICE_TIMEOUT_MS) {
        newStatus = ServiceStatus::Unavailable;
    } else if (serviceInfo.quality.lastUpdate.isValid()) {
        double qualityScore = calculateQualityScore(serviceInfo.quality);
        
        if (qualityScore >= 80.0) {
            newStatus = ServiceStatus::Available;
        } else if (qualityScore >= 50.0) {
            newStatus = ServiceStatus::Available; // Still available but lower quality
        } else {
            newStatus = ServiceStatus::Error; // Poor quality
        }
    } else {
        newStatus = ServiceStatus::Available; // Assume available if no quality data
    }

    serviceInfo.status = newStatus;
}

void ServiceManager::associateSubchannelsWithServices()
{
    // Clear existing subchannel associations
    for (auto& service : m_services) {
        service.subchannels.clear();
    }

    // Associate subchannels with services based on service components
    for (auto& service : m_services) {
        for (const auto& component : service.service.components) {
            // Find the corresponding subchannel
            auto subchannelIt = std::find_if(m_subchannels.begin(), m_subchannels.end(),
                                           [&component](const eti::SubChannelInfo& sub) {
                                               return sub.sub_channel_id == component.sub_channel_id;
                                           });
            
            if (subchannelIt != m_subchannels.end()) {
                service.subchannels.append(*subchannelIt);
                
                // Update quality information based on subchannel
                service.quality.bitRate = subchannelIt->size * 8.0; // Rough calculation
            }
        }
    }
}

double ServiceManager::calculateQualityScore(const ServiceQuality& quality) const
{
    // Calculate overall quality score (0.0-100.0)
    double signalWeight = 0.6;
    double errorWeight = 0.4;
    
    double signalScore = quality.signalStrength;
    double errorScore = 100.0 - quality.errorRate; // Lower error rate = higher score
    
    return (signalScore * signalWeight) + (errorScore * errorWeight);
}