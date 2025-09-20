/**
 * @file eti_service_tree_model.cpp
 * @brief Professional ETI Service Tree Model implementation
 * 
 * This file implements the EtiServiceTreeModel class providing hierarchical
 * representation of ETI streams with real-time updates and ETSI compliance.
 */

#include "eti_service_tree_model.h"
#include "core/etisnoop_wrapper.h"
#include "utils/logger.h"

#include <QIcon>
#include <QColor>
#include <QBrush>
#include <QFont>
#include <QDateTime>
#include <QTimer>
#include <QMutexLocker>
#include <QDebug>

EtiServiceTreeModel::EtiServiceTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_etisnoopWrapper(nullptr)
    , m_rootItem(nullptr)
    , m_ensembleItem(nullptr)
    , m_updateTimer(nullptr)
    , m_realTimeEnabled(false)
    , m_totalServices(0)
    , m_activeServices(0)
    , m_errorServices(0)
    , m_totalSubchannels(0)
{
    // Create root item
    m_rootItem = std::make_shared<TreeItem>(ItemType::Root);
    m_rootItem->name = "ETI Stream";
    m_rootItem->description = "ETI Stream Analysis Root";
    
    Logger::instance().log(Logger::Info, "EtiServiceTreeModel", "Constructor completed");
}

EtiServiceTreeModel::~EtiServiceTreeModel()
{
    Logger::instance().log(Logger::Info, "EtiServiceTreeModel", "Destructor starting");
    
    // Stop real-time updates
    setRealTimeEnabled(false);
    
    // Cleanup timer
    if (m_updateTimer) {
        m_updateTimer->stop();
        delete m_updateTimer;
        m_updateTimer = nullptr;
    }
    
    Logger::instance().log(Logger::Info, "EtiServiceTreeModel", "Destructor completed");
}

bool EtiServiceTreeModel::initialize(EtisnoopWrapper *wrapper)
{
    if (!wrapper) {
        Logger::instance().log(Logger::Error, "EtiServiceTreeModel", "Invalid ETISnoop wrapper provided");
        return false;
    }
    
    Logger::instance().log(Logger::Info, "EtiServiceTreeModel", "Initializing with ETISnoop wrapper");
    
    m_etisnoopWrapper = wrapper;
    
    // Connect ETISnoop wrapper signals
    connect(m_etisnoopWrapper, &EtisnoopWrapper::serviceDiscovered,
            this, &EtiServiceTreeModel::onServiceDiscovered);
    connect(m_etisnoopWrapper, &EtisnoopWrapper::ensembleUpdated,
            this, &EtiServiceTreeModel::onEnsembleUpdated);
    connect(m_etisnoopWrapper, &EtisnoopWrapper::serviceStatusChanged,
            this, &EtiServiceTreeModel::onServiceStatusChanged);
    
    // Setup update timer
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &EtiServiceTreeModel::updateModel);
    
    // Create initial tree structure
    createTreeStructure();
    
    // Enable real-time updates by default
    setRealTimeEnabled(true);
    
    Logger::instance().log(Logger::Info, "EtiServiceTreeModel", "Initialization completed successfully");
    return true;
}

QModelIndex EtiServiceTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }
    
    std::shared_ptr<TreeItem> parentItem;
    if (!parent.isValid()) {
        parentItem = m_rootItem;
    } else {
        parentItem = getItem(parent);
    }
    
    if (!parentItem || row >= parentItem->children.size()) {
        return QModelIndex();
    }
    
    auto childItem = parentItem->children.at(row);
    if (childItem) {
        return createIndex(row, column, childItem.get());
    }
    
    return QModelIndex();
}

QModelIndex EtiServiceTreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) {
        return QModelIndex();
    }
    
    auto childItem = getItem(child);
    if (!childItem) {
        return QModelIndex();
    }
    
    auto parentItem = childItem->parent.lock();
    if (!parentItem || parentItem == m_rootItem) {
        return QModelIndex();
    }
    
    // Find parent's position in grandparent
    auto grandParent = parentItem->parent.lock();
    if (!grandParent) {
        return QModelIndex();
    }
    
    for (int i = 0; i < grandParent->children.size(); ++i) {
        if (grandParent->children[i] == parentItem) {
            return createIndex(i, 0, parentItem.get());
        }
    }
    
    return QModelIndex();
}

int EtiServiceTreeModel::rowCount(const QModelIndex &parent) const
{
    std::shared_ptr<TreeItem> parentItem;
    if (!parent.isValid()) {
        parentItem = m_rootItem;
    } else {
        parentItem = getItem(parent);
    }
    
    return parentItem ? parentItem->children.size() : 0;
}

int EtiServiceTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return COLUMN_COUNT;
}

QVariant EtiServiceTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    
    auto item = getItem(index);
    if (!item) {
        return QVariant();
    }
    
    switch (role) {
    case Qt::DisplayRole:
        return formatItemText(*item, index.column());
        
    case Qt::DecorationRole:
        if (index.column() == 0) {
            return getItemIcon(*item);
        }
        break;
        
    case Qt::BackgroundRole:
        if (item->status != ServiceStatus::Unknown) {
            QColor color = getStatusColor(item->status);
            color.setAlpha(50); // Semi-transparent background
            return QBrush(color);
        }
        break;
        
    case Qt::ForegroundRole:
        if (item->status == ServiceStatus::Error) {
            return QBrush(QColor(Qt::red));
        } else if (item->status == ServiceStatus::Warning) {
            return QBrush(QColor(255, 140, 0)); // Orange
        } else if (item->status == ServiceStatus::Active) {
            return QBrush(QColor(Qt::darkGreen));
        }
        break;
        
    case Qt::FontRole:
        if (item->type == ItemType::Ensemble || item->type == ItemType::Service) {
            QFont font;
            font.setBold(true);
            return font;
        }
        break;
        
    case Qt::ToolTipRole:
        return QString("%1\n%2\nLast Update: %3")
                .arg(item->name)
                .arg(item->description)
                .arg(item->lastUpdate.toString(Qt::ISODate));
        
    case Qt::UserRole:
        return static_cast<int>(item->type);
        
    case Qt::UserRole + 1:
        return item->id;
        
    case Qt::UserRole + 2:
        return static_cast<int>(item->status);
    }
    
    return QVariant();
}

QVariant EtiServiceTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }
    
    switch (section) {
    case 0:
        return tr("ETI Element");
    case 1:
        return tr("Value");
    case 2:
        return tr("Status");
    default:
        return QVariant();
    }
}

Qt::ItemFlags EtiServiceTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void EtiServiceTreeModel::updateFromEnsemble(const EnsembleInfo& ensemble)
{
    QMutexLocker locker(&m_dataMutex);
    
    Logger::instance().log(Logger::Debug, "EtiServiceTreeModel", 
                          QString("Updating ensemble: %1 (ID: 0x%2)")
                          .arg(ensemble.ensembleName)
                          .arg(ensemble.ensembleId, 4, 16, QChar('0')));
    
    m_currentEnsemble = ensemble;
    
    beginResetModel();
    
    // Update or create ensemble item
    if (!m_ensembleItem) {
        m_ensembleItem = addEnsembleItem(ensemble);
        m_rootItem->children.append(m_ensembleItem);
    } else {
        // Update existing ensemble
        m_ensembleItem->name = ensemble.ensembleName.isEmpty() ? 
                              QString("Ensemble 0x%1").arg(ensemble.ensembleId, 4, 16, QChar('0')) :
                              ensemble.ensembleName;
        m_ensembleItem->value = QString("ID: 0x%1").arg(ensemble.ensembleId, 4, 16, QChar('0'));
        m_ensembleItem->description = ensemble.description;
        m_ensembleItem->status = ensemble.status;
        m_ensembleItem->lastUpdate = QDateTime::currentDateTime();
        m_ensembleItem->metadata = ensemble.figData;
    }
    
    endResetModel();
    
    emit ensembleUpdated(ensemble.ensembleId);
    updateStatistics();
}

void EtiServiceTreeModel::addService(const ServiceInfo& service)
{
    QMutexLocker locker(&m_dataMutex);
    
    Logger::instance().log(Logger::Debug, "EtiServiceTreeModel", 
                          QString("Adding service: %1 (ID: 0x%2)")
                          .arg(service.serviceName)
                          .arg(service.serviceId, 8, 16, QChar('0')));
    
    // Store service information
    m_services[service.serviceId] = service;
    
    if (!m_ensembleItem) {
        // Create default ensemble if none exists
        EnsembleInfo defaultEnsemble;
        defaultEnsemble.ensembleId = 0x0000;
        defaultEnsemble.ensembleName = "Unknown Ensemble";
        defaultEnsemble.status = ServiceStatus::Unknown;
        updateFromEnsemble(defaultEnsemble);
    }
    
    // Check if service already exists
    auto existingItem = m_serviceItems.value(service.serviceId);
    if (existingItem) {
        // Update existing service
        beginResetModel();
        existingItem->name = service.serviceName.isEmpty() ? 
                            QString("Service 0x%1").arg(service.serviceId, 8, 16, QChar('0')) :
                            service.serviceName;
        existingItem->value = QString("Type: %1").arg(service.programType);
        existingItem->description = QString("Subchannel: %1, Bitrate: %2 kbps")
                                   .arg(service.subChannelId)
                                   .arg(service.bitRate);
        existingItem->status = service.status;
        existingItem->lastUpdate = QDateTime::currentDateTime();
        existingItem->metadata.clear();
        existingItem->metadata["serviceId"] = service.serviceId;
        existingItem->metadata["subChannelId"] = service.subChannelId;
        existingItem->metadata["bitRate"] = service.bitRate;
        existingItem->metadata["quality"] = service.signalQuality;
        existingItem->metadata["isActive"] = service.isActive;
        endResetModel();
    } else {
        // Add new service
        beginInsertRows(getEnsembleIndex(), m_ensembleItem->children.size(), 
                       m_ensembleItem->children.size());
        
        auto serviceItem = addServiceItem(service, m_ensembleItem);
        m_ensembleItem->children.append(serviceItem);
        m_serviceItems[service.serviceId] = serviceItem;
        
        endInsertRows();
    }
    
    emit serviceStatusChanged(service.serviceId, service.status);
    updateStatistics();
}

void EtiServiceTreeModel::updateServiceStatus(quint32 serviceId, bool active)
{
    QMutexLocker locker(&m_dataMutex);
    
    if (m_services.contains(serviceId)) {
        m_services[serviceId].isActive = active;
        m_services[serviceId].status = active ? ServiceStatus::Active : ServiceStatus::Inactive;
        m_services[serviceId].lastUpdate = QDateTime::currentDateTime();
        
        auto serviceItem = m_serviceItems.value(serviceId);
        if (serviceItem) {
            serviceItem->status = m_services[serviceId].status;
            serviceItem->lastUpdate = QDateTime::currentDateTime();
            serviceItem->metadata["isActive"] = active;
            
            // Update display
            QModelIndex index = getServiceIndex(serviceId);
            if (index.isValid()) {
                emit dataChanged(index, index.siblingAtColumn(columnCount() - 1));
            }
        }
        
        Logger::instance().log(Logger::Debug, "EtiServiceTreeModel", 
                              QString("Service 0x%1 status updated: %2")
                              .arg(serviceId, 8, 16, QChar('0'))
                              .arg(active ? "Active" : "Inactive"));
        
        emit serviceStatusChanged(serviceId, m_services[serviceId].status);
        updateStatistics();
    }
}

void EtiServiceTreeModel::addSubchannel(const SubchannelInfo& subchannel)
{
    QMutexLocker locker(&m_dataMutex);
    
    Logger::instance().log(Logger::Debug, "EtiServiceTreeModel", 
                          QString("Adding subchannel: %1 (ID: %2)")
                          .arg(subchannel.name)
                          .arg(subchannel.subchannelId));
    
    m_subchannels[subchannel.subchannelId] = subchannel;
    
    // Find parent service for this subchannel
    std::shared_ptr<TreeItem> parentService = nullptr;
    for (const auto& service : m_services) {
        if (service.subChannelId == subchannel.subchannelId) {
            parentService = m_serviceItems.value(service.serviceId);
            break;
        }
    }
    
    if (!parentService) {
        Logger::instance().log(Logger::Warning, "EtiServiceTreeModel", 
                              QString("No parent service found for subchannel %1")
                              .arg(subchannel.subchannelId));
        return;
    }
    
    // Check if subchannel already exists
    auto existingItem = m_subchannelItems.value(subchannel.subchannelId);
    if (existingItem) {
        // Update existing subchannel
        existingItem->name = subchannel.name.isEmpty() ? 
                            QString("Subchannel %1").arg(subchannel.subchannelId) :
                            subchannel.name;
        existingItem->value = QString("Size: %1 CU").arg(subchannel.subchannelSize);
        existingItem->description = QString("Address: %1, Protection: %2")
                                   .arg(subchannel.startAddress)
                                   .arg(subchannel.protectionLevel);
        existingItem->status = subchannel.status;
        existingItem->lastUpdate = QDateTime::currentDateTime();
        
        QModelIndex parentIndex = getServiceIndex(parentService->id);
        if (parentIndex.isValid()) {
            QModelIndex subchannelIndex = index(0, 0, parentIndex);
            if (subchannelIndex.isValid()) {
                emit dataChanged(subchannelIndex, subchannelIndex.siblingAtColumn(columnCount() - 1));
            }
        }
    } else {
        // Add new subchannel
        QModelIndex parentIndex = getServiceIndex(parentService->id);
        if (parentIndex.isValid()) {
            beginInsertRows(parentIndex, parentService->children.size(), 
                           parentService->children.size());
            
            auto subchannelItem = addSubchannelItem(subchannel, parentService);
            parentService->children.append(subchannelItem);
            m_subchannelItems[subchannel.subchannelId] = subchannelItem;
            
            endInsertRows();
        }
    }
    
    updateStatistics();
}

void EtiServiceTreeModel::updateFIGData(const FigAnalysis& figData)
{
    QMutexLocker locker(&m_dataMutex);
    
    Logger::instance().log(Logger::Debug, "EtiServiceTreeModel", 
                          QString("Updating FIG data: Type %1 - %2")
                          .arg(figData.figType)
                          .arg(figData.figName));
    
    m_figData[figData.figType] = figData;
    
    if (!m_ensembleItem) {
        return;
    }
    
    // Check if FIG item already exists
    auto existingItem = m_figItems.value(figData.figType);
    if (existingItem) {
        // Update existing FIG data
        existingItem->value = figData.description;
        existingItem->status = figData.complianceStatus;
        existingItem->lastUpdate = QDateTime::currentDateTime();
        existingItem->metadata = figData.figData;
        
        QModelIndex figIndex = index(0, 0, getEnsembleIndex()); // Simplified lookup
        if (figIndex.isValid()) {
            emit dataChanged(figIndex, figIndex.siblingAtColumn(columnCount() - 1));
        }
    } else {
        // Add new FIG data item
        QModelIndex ensembleIndex = getEnsembleIndex();
        if (ensembleIndex.isValid()) {
            beginInsertRows(ensembleIndex, m_ensembleItem->children.size(), 
                           m_ensembleItem->children.size());
            
            auto figItem = addFigDataItem(figData, m_ensembleItem);
            m_ensembleItem->children.append(figItem);
            m_figItems[figData.figType] = figItem;
            
            endInsertRows();
        }
    }
    
    emit figDataUpdated(figData.figType);
}

QModelIndex EtiServiceTreeModel::getEnsembleIndex() const
{
    if (!m_ensembleItem) {
        return QModelIndex();
    }
    
    // Ensemble is always the first child of root
    return index(0, 0, QModelIndex());
}

QModelIndex EtiServiceTreeModel::getServiceIndex(quint32 serviceId) const
{
    auto serviceItem = m_serviceItems.value(serviceId);
    if (!serviceItem || !m_ensembleItem) {
        return QModelIndex();
    }
    
    // Find service position in ensemble children
    for (int i = 0; i < m_ensembleItem->children.size(); ++i) {
        if (m_ensembleItem->children[i] == serviceItem) {
            return index(i, 0, getEnsembleIndex());
        }
    }
    
    return QModelIndex();
}

QModelIndex EtiServiceTreeModel::getSubchannelIndex(quint8 subchannelId) const
{
    auto subchannelItem = m_subchannelItems.value(subchannelId);
    if (!subchannelItem) {
        return QModelIndex();
    }
    
    auto parentService = subchannelItem->parent.lock();
    if (!parentService) {
        return QModelIndex();
    }
    
    // Find subchannel position in service children
    for (int i = 0; i < parentService->children.size(); ++i) {
        if (parentService->children[i] == subchannelItem) {
            QModelIndex serviceIndex = getServiceIndex(parentService->id);
            return index(i, 0, serviceIndex);
        }
    }
    
    return QModelIndex();
}

std::shared_ptr<EtiServiceTreeModel::TreeItem> EtiServiceTreeModel::getItem(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return m_rootItem;
    }
    
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    if (!item) {
        return m_rootItem;
    }
    
    // Find the shared_ptr for this raw pointer
    std::function<std::shared_ptr<TreeItem>(const std::shared_ptr<TreeItem>&, TreeItem*)> findSharedPtr;
    findSharedPtr = [&](const std::shared_ptr<TreeItem>& node, TreeItem* target) -> std::shared_ptr<TreeItem> {
        if (node.get() == target) {
            return node;
        }
        for (const auto& child : node->children) {
            auto result = findSharedPtr(child, target);
            if (result) {
                return result;
            }
        }
        return nullptr;
    };
    
    return findSharedPtr(m_rootItem, item);
}

EtiServiceTreeModel::ServiceInfo EtiServiceTreeModel::getServiceInfo(quint32 serviceId) const
{
    QMutexLocker locker(&m_dataMutex);
    return m_services.value(serviceId, ServiceInfo());
}

QList<quint32> EtiServiceTreeModel::getAllServices() const
{
    QMutexLocker locker(&m_dataMutex);
    return m_services.keys();
}

QList<quint32> EtiServiceTreeModel::getActiveServices() const
{
    QMutexLocker locker(&m_dataMutex);
    QList<quint32> activeServices;
    
    for (auto it = m_services.constBegin(); it != m_services.constEnd(); ++it) {
        if (it.value().isActive) {
            activeServices.append(it.key());
        }
    }
    
    return activeServices;
}

void EtiServiceTreeModel::clearModel()
{
    QMutexLocker locker(&m_dataMutex);
    
    Logger::instance().log(Logger::Info, "EtiServiceTreeModel", "Clearing model data");
    
    beginResetModel();
    
    // Clear all data
    m_services.clear();
    m_subchannels.clear();
    m_figData.clear();
    m_serviceItems.clear();
    m_subchannelItems.clear();
    m_figItems.clear();
    
    // Reset tree structure
    m_rootItem->children.clear();
    m_ensembleItem.reset();
    
    // Reset statistics
    m_totalServices = 0;
    m_activeServices = 0;
    m_errorServices = 0;
    m_totalSubchannels = 0;
    
    endResetModel();
    
    emit statisticsChanged(getServiceStatistics());
}

void EtiServiceTreeModel::setRealTimeEnabled(bool enabled)
{
    if (m_realTimeEnabled == enabled) {
        return;
    }
    
    m_realTimeEnabled = enabled;
    
    if (m_updateTimer) {
        if (enabled) {
            m_updateTimer->start(UPDATE_INTERVAL_MS);
            Logger::instance().log(Logger::Info, "EtiServiceTreeModel", "Real-time updates enabled");
        } else {
            m_updateTimer->stop();
            Logger::instance().log(Logger::Info, "EtiServiceTreeModel", "Real-time updates disabled");
        }
    }
}

QVariantMap EtiServiceTreeModel::getServiceStatistics() const
{
    QMutexLocker locker(&m_dataMutex);
    
    QVariantMap stats;
    stats["totalServices"] = m_totalServices;
    stats["activeServices"] = m_activeServices;
    stats["errorServices"] = m_errorServices;
    stats["totalSubchannels"] = m_totalSubchannels;
    stats["ensembleId"] = m_currentEnsemble.ensembleId;
    stats["ensembleName"] = m_currentEnsemble.ensembleName;
    
    return stats;
}

void EtiServiceTreeModel::onServiceDiscovered(const EtisnoopWrapper::ServiceInfo& service)
{
    ServiceInfo internalService = convertFromEtisnoopService(service);
    addService(internalService);
}

void EtiServiceTreeModel::onEnsembleUpdated(const EtisnoopWrapper::EnsembleInfo& ensemble)
{
    EnsembleInfo internalEnsemble = convertFromEtisnoopEnsemble(ensemble);
    updateFromEnsemble(internalEnsemble);
}

void EtiServiceTreeModel::onServiceStatusChanged(quint32 serviceId, bool isActive)
{
    updateServiceStatus(serviceId, isActive);
}

void EtiServiceTreeModel::onFigAnalysisReady(const QVariantMap& figData)
{
    // Convert QVariantMap to FigAnalysis structure
    FigAnalysis analysis;
    analysis.figType = figData.value("figType", 0).toUInt();
    analysis.figName = figData.value("figName", QString("FIG %1").arg(analysis.figType)).toString();
    analysis.description = figData.value("description", "").toString();
    analysis.figData = figData;
    analysis.complianceStatus = ServiceStatus::Unknown; // Default status
    analysis.lastUpdate = QDateTime::currentDateTime();
    
    updateFIGData(analysis);
}

void EtiServiceTreeModel::updateModel()
{
    // Periodic model updates
    updateStatistics();
    emit statisticsChanged(getServiceStatistics());
}

void EtiServiceTreeModel::createTreeStructure()
{
    // Root is already created in constructor
    Logger::instance().log(Logger::Debug, "EtiServiceTreeModel", "Creating initial tree structure");
}

std::shared_ptr<EtiServiceTreeModel::TreeItem> EtiServiceTreeModel::addEnsembleItem(const EnsembleInfo& ensemble)
{
    auto item = std::make_shared<TreeItem>(ItemType::Ensemble);
    item->id = ensemble.ensembleId;
    item->name = ensemble.ensembleName.isEmpty() ? 
                QString("Ensemble 0x%1").arg(ensemble.ensembleId, 4, 16, QChar('0')) :
                ensemble.ensembleName;
    item->value = QString("ID: 0x%1").arg(ensemble.ensembleId, 4, 16, QChar('0'));
    item->description = ensemble.description;
    item->status = ensemble.status;
    item->lastUpdate = QDateTime::currentDateTime();
    item->metadata = ensemble.figData;
    item->parent = m_rootItem;
    
    return item;
}

std::shared_ptr<EtiServiceTreeModel::TreeItem> EtiServiceTreeModel::addServiceItem(const ServiceInfo& service, 
                                                                                   std::shared_ptr<TreeItem> ensembleItem)
{
    auto item = std::make_shared<TreeItem>(ItemType::Service);
    item->id = service.serviceId;
    item->name = service.serviceName.isEmpty() ? 
                QString("Service 0x%1").arg(service.serviceId, 8, 16, QChar('0')) :
                service.serviceName;
    item->value = QString("Type: %1").arg(service.programType);
    item->description = QString("Subchannel: %1, Bitrate: %2 kbps")
                       .arg(service.subChannelId)
                       .arg(service.bitRate);
    item->status = service.status;
    item->lastUpdate = QDateTime::currentDateTime();
    item->metadata["serviceId"] = service.serviceId;
    item->metadata["subChannelId"] = service.subChannelId;
    item->metadata["bitRate"] = service.bitRate;
    item->metadata["quality"] = service.signalQuality;
    item->metadata["isActive"] = service.isActive;
    item->parent = ensembleItem;
    
    return item;
}

std::shared_ptr<EtiServiceTreeModel::TreeItem> EtiServiceTreeModel::addSubchannelItem(const SubchannelInfo& subchannel,
                                                                                      std::shared_ptr<TreeItem> serviceItem)
{
    auto item = std::make_shared<TreeItem>(ItemType::Subchannel);
    item->id = subchannel.subchannelId;
    item->name = subchannel.name.isEmpty() ? 
                QString("Subchannel %1").arg(subchannel.subchannelId) :
                subchannel.name;
    item->value = QString("Size: %1 CU").arg(subchannel.subchannelSize);
    item->description = QString("Address: %1, Protection: %2")
                       .arg(subchannel.startAddress)
                       .arg(subchannel.protectionLevel);
    item->status = subchannel.status;
    item->lastUpdate = QDateTime::currentDateTime();
    item->metadata["startAddress"] = subchannel.startAddress;
    item->metadata["size"] = subchannel.subchannelSize;
    item->metadata["protection"] = subchannel.protectionLevel;
    item->parent = serviceItem;
    
    return item;
}

std::shared_ptr<EtiServiceTreeModel::TreeItem> EtiServiceTreeModel::addFigDataItem(const FigAnalysis& figData,
                                                                                   std::shared_ptr<TreeItem> parentItem)
{
    auto item = std::make_shared<TreeItem>(ItemType::FigData);
    item->id = figData.figType;
    item->name = figData.figName.isEmpty() ? 
                QString("FIG %1").arg(figData.figType) :
                figData.figName;
    item->value = figData.description;
    item->description = QString("FIG Type %1 Analysis").arg(figData.figType);
    item->status = figData.complianceStatus;
    item->lastUpdate = QDateTime::currentDateTime();
    item->metadata = figData.figData;
    item->parent = parentItem;
    
    return item;
}

std::shared_ptr<EtiServiceTreeModel::TreeItem> EtiServiceTreeModel::findItem(quint32 id, ItemType type) const
{
    std::function<std::shared_ptr<TreeItem>(const std::shared_ptr<TreeItem>&)> search;
    search = [&](const std::shared_ptr<TreeItem>& node) -> std::shared_ptr<TreeItem> {
        if (node->type == type && node->id == id) {
            return node;
        }
        for (const auto& child : node->children) {
            auto result = search(child);
            if (result) {
                return result;
            }
        }
        return nullptr;
    };
    
    return search(m_rootItem);
}

QIcon EtiServiceTreeModel::getItemIcon(const TreeItem& item) const
{
    switch (item.type) {
    case ItemType::Root:
        return QIcon(":/icons/stream.png");
    case ItemType::Ensemble:
        return QIcon(":/icons/ensemble.png");
    case ItemType::Service:
        if (item.status == ServiceStatus::Active) {
            return QIcon(":/icons/service-active.png");
        } else if (item.status == ServiceStatus::Error) {
            return QIcon(":/icons/service-error.png");
        } else {
            return QIcon(":/icons/service.png");
        }
    case ItemType::Subchannel:
        return QIcon(":/icons/subchannel.png");
    case ItemType::FigData:
        return QIcon(":/icons/fig.png");
    case ItemType::Parameter:
        return QIcon(":/icons/parameter.png");
    default:
        return QIcon();
    }
}

QColor EtiServiceTreeModel::getStatusColor(ServiceStatus status) const
{
    switch (status) {
    case ServiceStatus::Active:
        return QColor(76, 175, 80);   // Green
    case ServiceStatus::Inactive:
        return QColor(158, 158, 158); // Gray
    case ServiceStatus::Error:
        return QColor(244, 67, 54);   // Red
    case ServiceStatus::Warning:
        return QColor(255, 152, 0);   // Orange
    case ServiceStatus::Compliant:
        return QColor(33, 150, 243);  // Blue
    case ServiceStatus::NonCompliant:
        return QColor(156, 39, 176);  // Purple
    default:
        return QColor(224, 224, 224); // Light gray
    }
}

QString EtiServiceTreeModel::formatItemText(const TreeItem& item, int column) const
{
    switch (column) {
    case 0:
        return item.name;
    case 1:
        return item.value;
    case 2:
        switch (item.status) {
        case ServiceStatus::Active:
            return tr("Active");
        case ServiceStatus::Inactive:
            return tr("Inactive");
        case ServiceStatus::Error:
            return tr("Error");
        case ServiceStatus::Warning:
            return tr("Warning");
        case ServiceStatus::Compliant:
            return tr("Compliant");
        case ServiceStatus::NonCompliant:
            return tr("Non-Compliant");
        default:
            return tr("Unknown");
        }
    default:
        return QString();
    }
}

void EtiServiceTreeModel::updateStatistics()
{
    int totalServices = 0;
    int activeServices = 0;
    int errorServices = 0;
    
    for (const auto& service : m_services) {
        totalServices++;
        if (service.isActive) {
            activeServices++;
        }
        if (service.status == ServiceStatus::Error) {
            errorServices++;
        }
    }
    
    m_totalServices = totalServices;
    m_activeServices = activeServices;
    m_errorServices = errorServices;
    m_totalSubchannels = m_subchannels.size();
}

EtiServiceTreeModel::ServiceInfo EtiServiceTreeModel::convertFromEtisnoopService(const EtisnoopWrapper::ServiceInfo& etisnoopService) const
{
    ServiceInfo service;
    service.serviceId = etisnoopService.serviceId;
    service.subChannelId = etisnoopService.subChannelId;
    service.serviceName = etisnoopService.serviceName;
    service.serviceNameThai = etisnoopService.serviceNameThai;
    service.programType = etisnoopService.programType;
    service.language = etisnoopService.language;
    service.isActive = etisnoopService.isActive;
    service.signalQuality = etisnoopService.signalQuality;
    service.status = etisnoopService.isActive ? ServiceStatus::Active : ServiceStatus::Inactive;
    service.lastUpdate = QDateTime::currentDateTime();
    service.figAnalysis = etisnoopService.additionalData;
    service.additionalData = etisnoopService.additionalData;
    
    return service;
}

EtiServiceTreeModel::EnsembleInfo EtiServiceTreeModel::convertFromEtisnoopEnsemble(const EtisnoopWrapper::EnsembleInfo& etisnoopEnsemble) const
{
    EnsembleInfo ensemble;
    ensemble.ensembleId = etisnoopEnsemble.ensembleId;
    ensemble.ensembleName = etisnoopEnsemble.ensembleName;
    ensemble.ensembleNameThai = etisnoopEnsemble.ensembleNameThai;
    ensemble.country = etisnoopEnsemble.country;
    ensemble.totalSubchannels = etisnoopEnsemble.totalSubchannels;
    ensemble.qualityScore = etisnoopEnsemble.overallQuality;
    ensemble.status = ServiceStatus::Active; // Default to active
    ensemble.lastUpdate = QDateTime::currentDateTime();
    ensemble.figData = etisnoopEnsemble.metadata;
    
    // Extract service IDs
    for (const auto& service : etisnoopEnsemble.services) {
        ensemble.serviceIds.append(service.serviceId);
    }
    
    return ensemble;
}

bool EtiServiceTreeModel::validateTreeStructure() const
{
    // Basic validation of tree structure
    if (!m_rootItem) {
        return false;
    }
    
    // Validate parent-child relationships
    std::function<bool(const std::shared_ptr<TreeItem>&)> validateNode;
    validateNode = [&](const std::shared_ptr<TreeItem>& node) -> bool {
        for (const auto& child : node->children) {
            if (child->parent.lock() != node) {
                return false;
            }
            if (!validateNode(child)) {
                return false;
            }
        }
        return true;
    };
    
    return validateNode(m_rootItem);
}