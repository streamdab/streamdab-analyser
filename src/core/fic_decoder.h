#pragma once

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QHash>
#include <QMap>
#include <QString>
#include <QDateTime>
#include "eti_types.h"

// Forward declarations for ETI types

/**
 * @class FicDecoder
 * @brief Professional FIC (Fast Information Channel) decoder following ETSI EN 300 401
 * 
 * Decodes FIC data from ETI frames to extract service information, ensemble data,
 * and service component details according to ETSI specifications.
 * 
 * Features:
 * - ETSI EN 300 401 compliant FIC decoding
 * - FIG (Fast Information Group) parsing for all standard types
 * - Service and ensemble information extraction
 * - Real-time FIC processing with error detection
 * - Service label decoding (including extended character sets)
 * - Programme Type (PTy) classification
 */
class FicDecoder : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief FIG (Fast Information Group) types supported
     */
    enum class FigType {
        Unknown = -1,
        BasicServiceInfo = 0,      // FIG 0/1 - Basic service and service component definition
        ServiceLabel = 1,          // FIG 1/1 - Programme service label
        ServiceComponent = 2,      // FIG 0/2 - Basic service component definition
        PacketData = 3,           // FIG 0/3 - Service component with packet mode
        StreamData = 4,           // FIG 0/4 - Service component with stream mode
        LanguageInfo = 5,         // FIG 0/5 - Service component language
        ServiceLinking = 6,       // FIG 0/6 - Service linking information
        ConfigInfo = 7            // FIG 0/7 - Configuration information
    };
    Q_ENUM(FigType)

    /**
     * @brief Decoder processing status
     */
    enum class DecoderStatus {
        Idle = 0,
        Processing = 1,
        Error = 2,
        Ready = 3,
        Synchronized = 4
    };
    Q_ENUM(DecoderStatus)

    /**
     * @brief Service information extracted from FIC
     */
    struct ServiceInfo {
        quint32 serviceId;
        QString serviceLabel;
        QString shortLabel;
        quint8 programmeType;
        quint8 languageCode;
        bool isLocal;
        bool isDataService;
        QDateTime lastUpdated;
        
        ServiceInfo() : serviceId(0), programmeType(0), languageCode(0), 
                       isLocal(false), isDataService(false) {}
    };

    /**
     * @brief Ensemble information from FIC
     */
    struct EnsembleInfo {
        quint16 ensembleId;
        QString ensembleLabel;
        QString shortLabel;
        quint8 countryId;
        quint8 extendedCountryCode;
        quint8 cifCount;
        QString label;  // Test-compatible alias for ensembleLabel
        QDateTime lastUpdated;
        
        EnsembleInfo() : ensembleId(0), countryId(0), extendedCountryCode(0), cifCount(0) {}
    };

    /**
     * @brief Service component information
     */
    struct ComponentInfo {
        quint32 serviceId;
        quint8 subChannelId;
        quint16 startAddress;
        quint16 length;
        quint8 protectionLevel;
        bool isProtected;
        QString label;
        
        ComponentInfo() : serviceId(0), subChannelId(0), startAddress(0), 
                         length(0), protectionLevel(0), isProtected(false) {}
    };

    /**
     * @brief Service information structure (test-compatible)
     */
    struct Service {
        quint32 serviceId;
        QString label;
        quint8 nbServiceComp;
        QList<ComponentInfo> components;
        
        Service() : serviceId(0), nbServiceComp(0) {}
    };

    /**
     * @brief Sub-channel information structure (test-compatible)
     */
    struct SubChannel {
        quint8 subChannelId;
        quint16 startAddress;
        quint16 subChannelSize;
        quint8 protectionLevel;
        bool isValid;
        
        SubChannel() : subChannelId(0), startAddress(0), subChannelSize(0), 
                      protectionLevel(0), isValid(false) {}
    };

    /**
     * @brief Ensemble information structure (test-compatible)
     */
    struct EnsembleData {
        quint16 ensembleId;
        QString label;
        quint8 cifCount;
        
        EnsembleData() : ensembleId(0), cifCount(0) {}
    };

    explicit FicDecoder(QObject *parent = nullptr);
    virtual ~FicDecoder();

    // Core decoding operations
    bool initialize();
    void shutdown();
    bool isInitialized() const { return m_initialized; }

    // FIC processing
    bool processFicData(const QByteArray& ficData);
    bool processEtiFrame(const eti::EtiFrameData& frame);
    bool processFigBlock(const eti::FigBlock& figBlock);

    // Information retrieval
    QList<ServiceInfo> getDiscoveredServices() const;
    ServiceInfo getServiceInfo(quint32 serviceId) const;
    EnsembleInfo getEnsembleInfo() const;
    QList<ComponentInfo> getServiceComponents(quint32 serviceId) const;

    // Service management
    bool hasService(quint32 serviceId) const;
    bool hasEnsembleInfo() const;
    QString getServiceLabel(quint32 serviceId) const;
    QString getEnsembleLabel() const;

    // Status and statistics
    DecoderStatus getStatus() const { return m_status; }
    quint32 getProcessedFicCount() const { return m_ficCount; }
    double getErrorRate() const { return m_errorRate; }
    quint32 getFigTypeCount(FigType type) const;
    
    // Test-compatible interface methods
    quint32 getFibCount() const { return m_ficCount; }
    quint32 getErrorCount() const { return m_errorCount; }
    int getSyncLevel() const { return m_syncLevel; }
    void reset();
    QList<Service> getServices() const;
    QList<SubChannel> getSubChannels() const;
    EnsembleData getEnsembleData() const;
    Service getServiceById(quint32 serviceId) const;
    SubChannel getSubChannelById(quint8 subChannelId) const;
    bool isServiceAvailable(quint32 serviceId) const;
    bool isSubChannelConfigured(quint8 subChannelId) const;

    // Configuration
    void setErrorThreshold(double threshold) { m_errorThreshold = threshold; }
    double getErrorThreshold() const { return m_errorThreshold; }

signals:
    /**
     * @brief Emitted when new services are discovered
     */
    void servicesUpdated(const QList<ServiceInfo>& services);

    /**
     * @brief Emitted when ensemble information is updated
     */
    void ensembleInfoUpdated(const EnsembleInfo& ensemble);

    /**
     * @brief Emitted when service components are discovered
     */
    void serviceComponentsUpdated(quint32 serviceId, const QList<ComponentInfo>& components);

    /**
     * @brief Emitted when decoder status changes
     */
    void statusChanged(DecoderStatus status);

    /**
     * @brief Emitted on FIC decoding errors
     */
    void ficError(const QString& error);

    /**
     * @brief Emitted when service label is updated
     */
    void serviceLabelUpdated(quint32 serviceId, const QString& label);

    /**
     * @brief Emitted when FIC data is processed
     */
    void ficDataProcessed();

    /**
     * @brief Emitted when service information is updated
     */
    void serviceInfoUpdated(quint32 serviceId);

    /**
     * @brief Emitted when sub-channel information is updated
     */
    void subchannelInfoUpdated(quint8 subChannelId);

private slots:
    void handleFicProcessed();
    void handleFicError(const QString& error);

private:
    // FIG processing methods
    bool processFig0(const QByteArray& figData);
    bool processFig1(const QByteArray& figData);
    bool processFig0Type1(const QByteArray& data); // Service info
    bool processFig0Type2(const QByteArray& data); // Service components
    bool processFig1Type1(const QByteArray& data); // Service labels

    // Data extraction helpers
    QString extractLabel(const QByteArray& data, int offset, int length) const;
    QString extractShortLabel(const QString& fullLabel) const;
    bool validateFigHeader(const QByteArray& data) const;
    quint8 calculateFigLength(const QByteArray& data) const;

    // Service management
    void addOrUpdateService(const ServiceInfo& service);
    void addOrUpdateComponent(const ComponentInfo& component);
    void updateEnsembleInfo(const EnsembleInfo& ensemble);

    // Error handling and statistics
    void updateErrorStatistics(bool hasError);
    void resetDecoder();

    // Member variables
    bool m_initialized;
    DecoderStatus m_status;
    quint32 m_ficCount;
    double m_errorRate;
    double m_errorThreshold;
    quint32 m_errorCount;
    int m_syncLevel;

    // Information storage
    QMap<quint32, ServiceInfo> m_services;
    QMap<quint32, QList<ComponentInfo>> m_serviceComponents;
    EnsembleInfo m_ensembleInfo;
    bool m_hasEnsembleInfo;
    
    // Test-compatible data structures
    QMap<quint32, Service> m_testServices;
    QMap<quint8, SubChannel> m_subChannels;
    EnsembleData m_ensembleData;

    // Statistics and monitoring
    QHash<FigType, quint32> m_figTypeStats;
    QDateTime m_lastProcessTime;

    // Processing state
    QByteArray m_ficBuffer;
    bool m_processingActive;

    // Constants
    static constexpr double DEFAULT_ERROR_THRESHOLD = 0.1; // 10%
    static constexpr quint32 MAX_FIC_BUFFER_SIZE = 1024;
    static constexpr quint8 FIG_HEADER_SIZE = 1;
    static constexpr quint8 MAX_SERVICES_PER_ENSEMBLE = 64;
};