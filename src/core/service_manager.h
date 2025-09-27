#pragma once

#include <QObject>
#include <QList>
#include <QMap>
#include <QString>
#include <QDateTime>
#include <QTimer>
#include <memory>
#include "eti_types.hpp"

/**
 * @class ServiceManager
 * @brief Manages DAB service discovery and organization
 * 
 * Handles service discovery, ensemble management, and service quality tracking
 * for ETI stream analysis with ETSI compliance.
 */
class ServiceManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Service status enumeration
     */
    enum class ServiceStatus {
        Unknown,        // Service status not determined
        Available,      // Service is available and active
        Unavailable,    // Service is not available
        Error          // Service has errors
    };

    /**
     * @brief Service quality metrics
     */
    struct ServiceQuality {
        double signalStrength;      // Signal strength (0.0-100.0)
        double errorRate;           // Error rate (0.0-100.0)
        double bitRate;             // Bit rate in kbps
        bool isAudioService;        // True if audio service
        bool isDataService;         // True if data service
        QDateTime lastUpdate;       // Last quality update
        
        ServiceQuality() : signalStrength(0.0), errorRate(0.0), bitRate(0.0),
                          isAudioService(false), isDataService(false) {}
    };

    /**
     * @brief Extended service information
     */
    struct ServiceInfo {
        eti::DabService service;           // Core service data
        ServiceStatus status;              // Current status
        ServiceQuality quality;           // Quality metrics
        QList<eti::SubChannelInfo> subchannels; // Associated subchannels
        QDateTime discoveryTime;           // When service was discovered
        QDateTime lastSeen;               // Last time service was seen
        
        ServiceInfo() : status(ServiceStatus::Unknown) {}
    };

    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit ServiceManager(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~ServiceManager();

    /**
     * @brief Initialize the service manager
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * @brief Shutdown the service manager
     */
    void shutdown();

    /**
     * @brief Update ensemble information
     * @param ensemble Ensemble data
     */
    void updateEnsemble(const eti::Ensemble& ensemble);

    /**
     * @brief Add or update a service
     * @param service Service data
     */
    void addOrUpdateService(const eti::DabService& service);

    /**
     * @brief Update service quality metrics
     * @param serviceId Service ID
     * @param quality Quality metrics
     */
    void updateServiceQuality(uint16_t serviceId, const ServiceQuality& quality);

    /**
     * @brief Update subchannel information
     * @param subchannels List of subchannels
     */
    void updateSubchannels(const QList<eti::SubChannelInfo>& subchannels);

    /**
     * @brief Get current ensemble information
     * @return Current ensemble
     */
    eti::Ensemble getCurrentEnsemble() const { return m_currentEnsemble; }

    /**
     * @brief Get all discovered services
     * @return List of services
     */
    QList<ServiceInfo> getAllServices() const { return m_services.values(); }

    /**
     * @brief Get service by ID
     * @param serviceId Service ID
     * @return Service info or empty if not found
     */
    ServiceInfo getService(uint16_t serviceId) const;

    /**
     * @brief Check if service exists
     * @param serviceId Service ID
     * @return true if service exists
     */
    bool hasService(uint16_t serviceId) const { return m_services.contains(serviceId); }

    /**
     * @brief Get service count
     * @return Number of discovered services
     */
    int getServiceCount() const { return m_services.size(); }

    /**
     * @brief Get audio services only
     * @return List of audio services
     */
    QList<ServiceInfo> getAudioServices() const;

    /**
     * @brief Get data services only
     * @return List of data services
     */
    QList<ServiceInfo> getDataServices() const;

    /**
     * @brief Get services by status
     * @param status Desired service status
     * @return List of services with specified status
     */
    QList<ServiceInfo> getServicesByStatus(ServiceStatus status) const;

    /**
     * @brief Clear all services and ensemble data
     */
    void clearAll();

    /**
     * @brief Get ensemble statistics
     * @return Statistics as key-value pairs
     */
    QMap<QString, QVariant> getEnsembleStatistics() const;

signals:
    /**
     * @brief Emitted when ensemble information is updated
     * @param ensemble Updated ensemble
     */
    void ensembleUpdated(const eti::Ensemble& ensemble);

    /**
     * @brief Emitted when a new service is discovered
     * @param serviceInfo New service information
     */
    void serviceDiscovered(const ServiceInfo& serviceInfo);

    /**
     * @brief Emitted when service information is updated
     * @param serviceInfo Updated service information
     */
    void serviceUpdated(const ServiceInfo& serviceInfo);

    /**
     * @brief Emitted when service quality changes
     * @param serviceId Service ID
     * @param quality New quality metrics
     */
    void serviceQualityUpdated(uint16_t serviceId, const ServiceQuality& quality);

    /**
     * @brief Emitted when service status changes
     * @param serviceId Service ID
     * @param oldStatus Previous status
     * @param newStatus New status
     */
    void serviceStatusChanged(uint16_t serviceId, ServiceStatus oldStatus, ServiceStatus newStatus);

    /**
     * @brief Emitted when subchannel organization changes
     * @param subchannels Updated subchannel list
     */
    void subchannelsUpdated(const QList<eti::SubChannelInfo>& subchannels);

private slots:
    /**
     * @brief Check for stale services periodically
     */
    void checkStaleServices();

private:
    /**
     * @brief Validate service against ETSI standards
     * @param service Service to validate
     * @return true if valid
     */
    bool validateService(const eti::DabService& service) const;

    /**
     * @brief Update service status based on quality and activity
     * @param serviceId Service ID
     */
    void updateServiceStatus(uint16_t serviceId);

    /**
     * @brief Associate subchannels with services
     */
    void associateSubchannelsWithServices();

    /**
     * @brief Calculate service quality score
     * @param quality Quality metrics
     * @return Quality score (0.0-100.0)
     */
    double calculateQualityScore(const ServiceQuality& quality) const;

private:
    bool m_initialized;
    eti::Ensemble m_currentEnsemble;
    QMap<uint16_t, ServiceInfo> m_services;
    QList<eti::SubChannelInfo> m_subchannels;
    QTimer* m_staleCheckTimer;
    
    // Configuration
    static constexpr int STALE_CHECK_INTERVAL_MS = 30000;  // 30 seconds
    static constexpr int SERVICE_TIMEOUT_MS = 60000;       // 1 minute
};

Q_DECLARE_METATYPE(ServiceManager::ServiceStatus)
Q_DECLARE_METATYPE(ServiceManager::ServiceQuality)
Q_DECLARE_METATYPE(ServiceManager::ServiceInfo)