#pragma once

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QTimer>
#include <QMutex>
#include <QHash>
#include <QIcon>
#include <QDateTime>
#include <memory>

#include "../core/eti_types.hpp"

// Forward declarations for Modern ETI Core Engine
namespace eti {
namespace modern {
    class ModernETIFrameParser;
    class EnhancedFIGAnalyser;
    class ComprehensiveETSIValidator;
    struct EnhancedServiceInfo;
    struct EnhancedEnsembleInfo;
    struct FIGAnalysisResult;
    struct ETIParseResult;
}
}

/**
 * @class EtiServiceTreeModel
 * @brief Professional ETI Service Tree Model for DAB service hierarchy
 * 
 * This model provides a hierarchical representation of ETI streams with:
 * - Ensemble → Services → Subchannels organization
 * - Real-time updates from ETISnoop wrapper
 * - ETSI compliance status indicators
 * - Service quality metrics
 * - FIG information display
 * - Professional broadcasting industry standards
 */
class EtiServiceTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    /**
     * @brief Tree item types for proper identification
     */
    enum class ItemType {
        Root = 0,
        Ensemble = 1,
        Service = 2,
        Subchannel = 3,
        FigData = 4,
        Parameter = 5
    };

    /**
     * @brief Service status enumeration with ETSI compliance
     */
    enum class ServiceStatus {
        Unknown = 0,
        Active = 1,
        Inactive = 2,
        Error = 3,
        Warning = 4,
        Compliant = 5,
        NonCompliant = 6
    };

    /**
     * @brief Tree item data structure
     */
    struct TreeItem {
        ItemType type;
        QString name;
        QString value;
        QString description;
        ServiceStatus status;
        QVariantMap metadata;
        quint32 id;
        QDateTime lastUpdate;
        QList<std::shared_ptr<TreeItem>> children;
        std::weak_ptr<TreeItem> parent;
        
        TreeItem(ItemType t = ItemType::Root) 
            : type(t), status(ServiceStatus::Unknown), id(0) {}
    };

    /**
     * @brief Ensemble information from ETISnoop
     */
    struct EnsembleInfo {
        quint16 ensembleId;
        QString ensembleName;
        QString ensembleNameThai;
        QString country;
        QString description;
        ServiceStatus status;
        QDateTime lastUpdate;
        QVariantMap figData;
        QList<quint32> serviceIds;
        double qualityScore;
        int totalSubchannels;
        
        EnsembleInfo() : ensembleId(0), status(ServiceStatus::Unknown), 
                        qualityScore(0.0), totalSubchannels(0) {}
    };

    /**
     * @brief Service information with ETI-specific data
     */
    struct ServiceInfo {
        quint32 serviceId;
        quint16 subChannelId;
        QString serviceName;
        QString serviceNameThai;
        QString programType;
        QString language;
        QString audioFormat;
        ServiceStatus status;
        bool isActive;
        double signalQuality;
        quint32 bitRate;
        QString protectionLevel;
        QDateTime lastUpdate;
        QVariantMap figAnalysis;
        QVariantMap additionalData;
        
        ServiceInfo() : serviceId(0), subChannelId(0), status(ServiceStatus::Unknown),
                       isActive(false), signalQuality(0.0), bitRate(0) {}
    };

    /**
     * @brief Subchannel information
     */
    struct SubchannelInfo {
        quint8 subchannelId;
        QString name;
        quint16 startAddress;
        quint16 subchannelSize;
        QString protectionLevel;
        QString uepTableIndex;
        ServiceStatus status;
        QDateTime lastUpdate;
        
        SubchannelInfo() : subchannelId(0), startAddress(0), subchannelSize(0),
                          status(ServiceStatus::Unknown) {}
    };

    /**
     * @brief FIG analysis information
     */
    struct FigAnalysis {
        quint8 figType;
        QString figName;
        QString description;
        QVariantMap figData;
        ServiceStatus complianceStatus;
        QStringList issues;
        QDateTime lastUpdate;
        
        FigAnalysis() : figType(0), complianceStatus(ServiceStatus::Unknown) {}
    };

public:
    /**
     * @brief Constructor
     * @param parent Parent object
     */
    explicit EtiServiceTreeModel(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~EtiServiceTreeModel();

    /**
     * @brief Initialize model with ETISnoop wrapper
     * @param wrapper ETISnoop wrapper instance
     * @return true if successful
     */
    bool initializeWithModernEngine(eti::modern::ModernETIFrameParser *frameParser, 
                                   eti::modern::EnhancedFIGAnalyser *figAnalyser);

    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    /**
     * @brief Update model with ensemble information
     * @param ensemble Ensemble information
     */
    void updateFromEnsemble(const EnsembleInfo& ensemble);

    /**
     * @brief Add or update service in the model
     * @param service Service information
     */
    void addService(const ServiceInfo& service);

    /**
     * @brief Update FIG analysis data
     * @param figAnalysis FIG analysis results
     */
    void updateFIGData(const FigAnalysis& figAnalysis);

    /**
     * @brief Update service status
     * @param serviceId Service identifier
     * @param active Service active status
     */
    void updateServiceStatus(quint32 serviceId, bool active);

    /**
     * @brief Add or update subchannel information
     * @param subchannel Subchannel information
     */
    void addSubchannel(const SubchannelInfo& subchannel);

    /**
     * @brief Get ensemble index
     * @return Model index for ensemble
     */
    QModelIndex getEnsembleIndex() const;

    /**
     * @brief Get service index by ID
     * @param serviceId Service identifier
     * @return Model index for service
     */
    QModelIndex getServiceIndex(quint32 serviceId) const;

    /**
     * @brief Get subchannel index by ID
     * @param subchannelId Subchannel identifier
     * @return Model index for subchannel
     */
    QModelIndex getSubchannelIndex(quint8 subchannelId) const;

    /**
     * @brief Get item from index
     * @param index Model index
     * @return Tree item pointer
     */
    std::shared_ptr<TreeItem> getItem(const QModelIndex &index) const;

    /**
     * @brief Get service information by ID
     * @param serviceId Service identifier
     * @return Service information
     */
    ServiceInfo getServiceInfo(quint32 serviceId) const;

    /**
     * @brief Get all services
     * @return List of all service IDs
     */
    QList<quint32> getAllServices() const;

    /**
     * @brief Get active services
     * @return List of active service IDs
     */
    QList<quint32> getActiveServices() const;

    /**
     * @brief Clear all model data
     */
    void clearModel();

    /**
     * @brief Enable/disable real-time updates
     * @param enabled Update status
     */
    void setRealTimeEnabled(bool enabled);

    /**
     * @brief Check if real-time updates are enabled
     * @return true if enabled
     */
    bool isRealTimeEnabled() const { return m_realTimeEnabled; }

    /**
     * @brief Get service statistics
     * @return Statistics map (total, active, errors, etc.)
     */
    QVariantMap getServiceStatistics() const;

signals:
    /**
     * @brief Emitted when service selection should change
     * @param serviceId Service identifier
     */
    void serviceSelectionRequested(quint32 serviceId);

    /**
     * @brief Emitted when service status changes
     * @param serviceId Service identifier
     * @param status New status
     */
    void serviceStatusChanged(quint32 serviceId, ServiceStatus status);

    /**
     * @brief Emitted when ensemble information updates
     * @param ensembleId Ensemble identifier
     */
    void ensembleUpdated(quint16 ensembleId);

    /**
     * @brief Emitted when FIG data updates
     * @param figType FIG type
     */
    void figDataUpdated(quint8 figType);

    /**
     * @brief Emitted when model statistics change
     * @param stats Updated statistics
     */
    void statisticsChanged(const QVariantMap &stats);

private slots:
    /**
     * @brief Handle ETISnoop wrapper signals
     */
    // Modern ETI Core Engine slot methods
    void onModernServiceDiscovered(const eti::DabService& service);
    void onModernEnsembleUpdated(const eti::modern::EnhancedEnsembleInfo& ensemble);
    void onFIGAnalysisComplete(uint32_t frameNumber, const eti::modern::FIGAnalysisResult& analysis);
    void onFigAnalysisReady(const QVariantMap& figData);

    /**
     * @brief Handle real-time update timer
     */
    void updateModel();

private:
    /**
     * @brief Create tree structure
     */
    void createTreeStructure();

    /**
     * @brief Add ensemble item
     * @param ensemble Ensemble information
     * @return Tree item for ensemble
     */
    std::shared_ptr<TreeItem> addEnsembleItem(const EnsembleInfo& ensemble);

    /**
     * @brief Add service item
     * @param service Service information
     * @param ensembleItem Parent ensemble item
     * @return Tree item for service
     */
    std::shared_ptr<TreeItem> addServiceItem(const ServiceInfo& service, 
                                           std::shared_ptr<TreeItem> ensembleItem);

    /**
     * @brief Add subchannel item
     * @param subchannel Subchannel information
     * @param serviceItem Parent service item
     * @return Tree item for subchannel
     */
    std::shared_ptr<TreeItem> addSubchannelItem(const SubchannelInfo& subchannel,
                                              std::shared_ptr<TreeItem> serviceItem);

    /**
     * @brief Add FIG data item
     * @param figData FIG analysis
     * @param parentItem Parent item
     * @return Tree item for FIG data
     */
    std::shared_ptr<TreeItem> addFigDataItem(const FigAnalysis& figData,
                                           std::shared_ptr<TreeItem> parentItem);

    /**
     * @brief Find item by ID and type
     * @param id Item identifier
     * @param type Item type
     * @return Tree item or nullptr
     */
    std::shared_ptr<TreeItem> findItem(quint32 id, ItemType type) const;

    /**
     * @brief Get icon for item
     * @param item Tree item
     * @return Icon for display
     */
    QIcon getItemIcon(const TreeItem& item) const;

    /**
     * @brief Get status color
     * @param status Service status
     * @return Color for status display
     */
    QColor getStatusColor(ServiceStatus status) const;

    /**
     * @brief Format item display text
     * @param item Tree item
     * @param column Column index
     * @return Formatted display text
     */
    QString formatItemText(const TreeItem& item, int column) const;

    /**
     * @brief Update statistics
     */
    void updateStatistics();

    /**
     * @brief Convert DAB service to internal format
     * @param service DAB service info
     * @return Internal service info
     */
    ServiceInfo convertFromDabService(const eti::DabService& service) const;
    
    /**
     * @brief Convert ETISnoop service to internal format
     * @param etisnoopService ETISnoop service info
     * @return Internal service info
     */
    ServiceInfo convertFromModernService(const eti::modern::EnhancedServiceInfo& service) const;

    /**
     * @brief Convert ETISnoop ensemble to internal format
     * @param etisnoopEnsemble ETISnoop ensemble info
     * @return Internal ensemble info
     */
    EnsembleInfo convertFromModernEnsemble(const eti::modern::EnhancedEnsembleInfo& ensemble) const;

    /**
     * @brief Validate tree structure
     * @return true if structure is valid
     */
    bool validateTreeStructure() const;

    // Core components
    // Modern ETI Core Engine components
    eti::modern::ModernETIFrameParser *m_frameParser;
    eti::modern::EnhancedFIGAnalyser *m_figAnalyser;

    // Tree structure
    std::shared_ptr<TreeItem> m_rootItem;
    std::shared_ptr<TreeItem> m_ensembleItem;

    // Data storage
    QHash<quint32, ServiceInfo> m_services;
    QHash<quint8, SubchannelInfo> m_subchannels;
    QHash<quint8, FigAnalysis> m_figData;
    EnsembleInfo m_currentEnsemble;

    // Index mapping for performance
    QHash<quint32, std::shared_ptr<TreeItem>> m_serviceItems;
    QHash<quint8, std::shared_ptr<TreeItem>> m_subchannelItems;
    QHash<quint8, std::shared_ptr<TreeItem>> m_figItems;

    // Update system
    QTimer *m_updateTimer;
    bool m_realTimeEnabled;
    mutable QMutex m_dataMutex;

    // Statistics
    int m_totalServices;
    int m_activeServices;
    int m_errorServices;
    int m_totalSubchannels;

    // Configuration
    static constexpr int UPDATE_INTERVAL_MS = 1000;  // 1 second
    static constexpr int COLUMN_COUNT = 3;           // Name, Value, Status
};

Q_DECLARE_METATYPE(EtiServiceTreeModel::ItemType)
Q_DECLARE_METATYPE(EtiServiceTreeModel::ServiceStatus)
Q_DECLARE_METATYPE(EtiServiceTreeModel::ServiceInfo)
Q_DECLARE_METATYPE(EtiServiceTreeModel::EnsembleInfo)