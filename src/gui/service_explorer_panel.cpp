#include "service_explorer_panel.h"
#include "core/eti_processor.h"
#include "utils/logger.h"
#include <QApplication>
#include <QHeaderView>
#include <QBrush>
#include <QPalette>
#include <QStyle>
#include <QDebug>

// Professional broadcast industry color definitions
const QColor ServiceExplorerPanel::BACKGROUND_DARK = QColor(45, 45, 48);     // #2D2D30
const QColor ServiceExplorerPanel::BACKGROUND_MEDIUM = QColor(62, 62, 66);   // #3E3E42
const QColor ServiceExplorerPanel::ACCENT_BLUE = QColor(0, 120, 212);        // #0078D4
const QColor ServiceExplorerPanel::SERVICE_AUDIO = QColor(0, 170, 0);        // #00AA00
const QColor ServiceExplorerPanel::SERVICE_DATA = QColor(255, 140, 0);       // #FF8C00
const QColor ServiceExplorerPanel::SERVICE_DABPLUS = QColor(0, 120, 212);    // #0078D4
const QColor ServiceExplorerPanel::SERVICE_ENSEMBLE = QColor(138, 43, 226);  // #8A2BE2
const QColor ServiceExplorerPanel::ERROR_CRITICAL = QColor(220, 53, 69);     // #DC3545
const QColor ServiceExplorerPanel::WARNING_MEDIUM = QColor(255, 193, 7);     // #FFC107
const QColor ServiceExplorerPanel::TEXT_PRIMARY = QColor(255, 255, 255);     // #FFFFFF
const QColor ServiceExplorerPanel::TEXT_SECONDARY = QColor(204, 204, 204);   // #CCCCCC

ServiceExplorerPanel::ServiceExplorerPanel(QWidget* parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_controlLayout(nullptr)
    , m_serviceTree(nullptr)
    , m_headerLabel(nullptr)
    , m_statisticsLabel(nullptr)
    , m_searchEdit(nullptr)
    , m_filterCombo(nullptr)
    , m_refreshButton(nullptr)
    , m_expandAllButton(nullptr)
    , m_collapseAllButton(nullptr)
    , m_etiProcessor(nullptr)
    , m_selectedServiceId(0)
    , m_currentFilter(ServiceInfo::Unknown)
    , m_totalServices(0)
    , m_activeServices(0)
    , m_dabPlusServices(0)
    , m_totalEnsembles(0)
{
    setupServiceTree();
    setupControlPanel();
    applyServiceTypeColors();
    applyBroadcastingTheme();
    
    Logger::instance().log(Logger::Info, "ServiceExplorerPanel", 
                          "Professional DAB Service Explorer Panel initialized");
}

ServiceExplorerPanel::~ServiceExplorerPanel()
{
    // Cleanup handled by Qt parent-child relationship
}

void ServiceExplorerPanel::setupServiceTree()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(6, 6, 6, 6);
    m_mainLayout->setSpacing(6);
    
    // Professional panel header
    m_headerLabel = new QLabel(tr("ETI Service Explorer"));
    m_headerLabel->setObjectName("ServiceExplorerHeader");
    m_mainLayout->addWidget(m_headerLabel);
    
    // Statistics label
    m_statisticsLabel = new QLabel(tr("Services: 0 | Active: 0 | Ensembles: 0"));
    m_statisticsLabel->setObjectName("ServiceExplorerStats");
    m_mainLayout->addWidget(m_statisticsLabel);
    
    // Professional service tree with DAB hierarchy
    m_serviceTree = new QTreeWidget(this);
    m_serviceTree->setObjectName("ServiceExplorerTree");
    m_serviceTree->setHeaderLabels({"Service", "Type", "Quality", "Bitrate"});
    m_serviceTree->setAlternatingRowColors(true);
    m_serviceTree->setRootIsDecorated(true);
    m_serviceTree->setItemsExpandable(true);
    m_serviceTree->setSortingEnabled(true);
    m_serviceTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_serviceTree->setIndentation(20);
    
    // Professional column sizing for broadcast industry standards
    m_serviceTree->setColumnWidth(0, 200);  // Service name (primary)
    m_serviceTree->setColumnWidth(1, 80);   // Type (compact)
    m_serviceTree->setColumnWidth(2, 80);   // Quality (percentage)
    m_serviceTree->setColumnWidth(3, 80);   // Bitrate (kbps)
    
    // Header styling
    QHeaderView* header = m_serviceTree->header();
    header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Stretch);  // Service name stretches
    header->setSectionResizeMode(1, QHeaderView::Fixed);    // Type fixed
    header->setSectionResizeMode(2, QHeaderView::Fixed);    // Quality fixed
    header->setSectionResizeMode(3, QHeaderView::Fixed);    // Bitrate fixed
    
    m_mainLayout->addWidget(m_serviceTree);
    
    // Connect signals for professional interaction
    connect(m_serviceTree, &QTreeWidget::itemClicked,
            this, &ServiceExplorerPanel::onServiceTreeItemClicked);
    connect(m_serviceTree, &QTreeWidget::itemChanged,
            this, &ServiceExplorerPanel::onServiceTreeItemChanged);
}

void ServiceExplorerPanel::setupControlPanel()
{
    m_controlLayout = new QHBoxLayout();
    m_controlLayout->setSpacing(4);
    
    // Search functionality
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(tr("Search services..."));
    m_searchEdit->setObjectName("ServiceExplorerSearch");
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &ServiceExplorerPanel::onSearchTextChanged);
    
    // Filter by service type
    m_filterCombo = new QComboBox();
    m_filterCombo->setObjectName("ServiceExplorerFilter");
    m_filterCombo->addItem(tr("All Services"), static_cast<int>(ServiceInfo::Unknown));
    m_filterCombo->addItem(tr("Audio"), static_cast<int>(ServiceInfo::Audio));
    m_filterCombo->addItem(tr("DAB+"), static_cast<int>(ServiceInfo::DABPlus));
    m_filterCombo->addItem(tr("Data"), static_cast<int>(ServiceInfo::Data));
    m_filterCombo->addItem(tr("Packet"), static_cast<int>(ServiceInfo::Packet));
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ServiceExplorerPanel::onFilterTypeChanged);
    
    // Control buttons
    m_refreshButton = new QPushButton(tr("Refresh"));
    m_refreshButton->setObjectName("ServiceExplorerRefresh");
    connect(m_refreshButton, &QPushButton::clicked,
            this, &ServiceExplorerPanel::onRefreshClicked);
    
    m_expandAllButton = new QPushButton(tr("Expand"));
    m_expandAllButton->setObjectName("ServiceExplorerExpand");
    connect(m_expandAllButton, &QPushButton::clicked,
            this, &ServiceExplorerPanel::expandAll);
    
    m_collapseAllButton = new QPushButton(tr("Collapse"));
    m_collapseAllButton->setObjectName("ServiceExplorerCollapse");
    connect(m_collapseAllButton, &QPushButton::clicked,
            this, &ServiceExplorerPanel::collapseAll);
    
    // Add to layout
    m_controlLayout->addWidget(m_searchEdit);
    m_controlLayout->addWidget(m_filterCombo);
    m_controlLayout->addWidget(m_refreshButton);
    m_controlLayout->addWidget(m_expandAllButton);
    m_controlLayout->addWidget(m_collapseAllButton);
    
    m_mainLayout->addLayout(m_controlLayout);
}

void ServiceExplorerPanel::addDABService(const ServiceInfo& service)
{
    // Find or create ensemble item
    auto* ensembleItem = findOrCreateEnsembleItem(service.ensembleId);
    
    // Check if service already exists
    if (m_serviceItems.contains(service.serviceId)) {
        // Update existing service
        auto* serviceItem = m_serviceItems[service.serviceId];
        serviceItem->setText(1, service.getTypeString());
        serviceItem->setText(2, QString("%1%").arg(service.quality));
        serviceItem->setText(3, QString("%1 kbps").arg(service.bitrate));
        
        // Update service data
        m_services[service.serviceId] = service;
        
        // Update colors
        QColor serviceColor = getServiceTypeColor(service.type);
        serviceItem->setForeground(0, QBrush(serviceColor));
        
        Logger::instance().log(Logger::Debug, "ServiceExplorerPanel",
                              QString("Updated service: %1 (SID: 0x%2)")
                              .arg(service.label)
                              .arg(service.serviceId, 4, 16, QChar('0')));
        return;
    }
    
    // Create new service item with professional formatting
    auto* serviceItem = new QTreeWidgetItem(ensembleItem);
    serviceItem->setText(0, QString("%1 (SID: 0x%2)")
                        .arg(service.label)
                        .arg(service.serviceId, 4, 16, QChar('0')));
    serviceItem->setText(1, service.getTypeString());
    serviceItem->setText(2, QString("%1%").arg(service.quality));
    serviceItem->setText(3, QString("%1 kbps").arg(service.bitrate));
    serviceItem->setData(0, Qt::UserRole, service.serviceId);
    serviceItem->setCheckState(0, Qt::Checked);  // Visible by default
    
    // Professional color coding by service type
    QColor serviceColor = getServiceTypeColor(service.type);
    serviceItem->setForeground(0, QBrush(serviceColor));
    
    // Set icons for service types
    if (service.isDabPlus) {
        serviceItem->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
    } else if (service.type == ServiceInfo::Data) {
        serviceItem->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_FileIcon));
    }
    
    // Store service data
    m_serviceItems[service.serviceId] = serviceItem;
    m_services[service.serviceId] = service;
    
    // Update statistics
    m_totalServices++;
    if (service.isActive) m_activeServices++;
    if (service.isDabPlus) m_dabPlusServices++;
    
    // Expand ensemble automatically for professional visibility
    ensembleItem->setExpanded(true);
    
    // Update statistics display
    updateStatistics();
    
    Logger::instance().log(Logger::Info, "ServiceExplorerPanel",
                          QString("Added DAB service: %1 (SID: 0x%2, Type: %3)")
                          .arg(service.label)
                          .arg(service.serviceId, 4, 16, QChar('0'))
                          .arg(service.getTypeString()));
}

QTreeWidgetItem* ServiceExplorerPanel::findOrCreateEnsembleItem(uint16_t ensembleId)
{
    if (m_ensembleItems.contains(ensembleId)) {
        return m_ensembleItems[ensembleId];
    }
    
    // Create new ensemble item
    auto* ensembleItem = new QTreeWidgetItem(m_serviceTree);
    
    // Use ensemble label if available, otherwise use ID
    QString ensembleLabel = QString("Ensemble 0x%1").arg(ensembleId, 4, 16, QChar('0'));
    if (m_ensembles.contains(ensembleId)) {
        ensembleLabel = m_ensembles[ensembleId].label;
    }
    
    ensembleItem->setText(0, ensembleLabel);
    ensembleItem->setText(1, tr("Ensemble"));
    ensembleItem->setText(2, tr("--"));
    ensembleItem->setText(3, tr("--"));
    ensembleItem->setData(0, Qt::UserRole, ensembleId);
    ensembleItem->setExpanded(true);  // Professional default: expanded
    
    // Professional ensemble styling
    QFont boldFont = m_serviceTree->font();
    boldFont.setBold(true);
    ensembleItem->setFont(0, boldFont);
    ensembleItem->setForeground(0, QBrush(SERVICE_ENSEMBLE));
    ensembleItem->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DirIcon));
    
    m_ensembleItems[ensembleId] = ensembleItem;
    m_totalEnsembles++;
    
    Logger::instance().log(Logger::Debug, "ServiceExplorerPanel",
                          QString("Created ensemble item: %1 (EID: 0x%2)")
                          .arg(ensembleLabel)
                          .arg(ensembleId, 4, 16, QChar('0')));
    
    return ensembleItem;
}

QTreeWidgetItem* ServiceExplorerPanel::findServiceItem(uint32_t serviceId)
{
    return m_serviceItems.value(serviceId, nullptr);
}

QColor ServiceExplorerPanel::getServiceTypeColor(ServiceInfo::ServiceType type) const
{
    switch (type) {
        case ServiceInfo::Audio:   return SERVICE_AUDIO;
        case ServiceInfo::Data:    return SERVICE_DATA;
        case ServiceInfo::DABPlus: return SERVICE_DABPLUS;
        case ServiceInfo::Packet:  return WARNING_MEDIUM;
        default:                   return TEXT_SECONDARY;
    }
}

void ServiceExplorerPanel::updateServiceStatus(uint32_t serviceId, bool active)
{
    if (!m_serviceItems.contains(serviceId)) {
        return;
    }
    
    auto* serviceItem = m_serviceItems[serviceId];
    
    // Update service data
    if (m_services.contains(serviceId)) {
        bool wasActive = m_services[serviceId].isActive;
        m_services[serviceId].isActive = active;
        
        // Update statistics
        if (wasActive != active) {
            if (active) {
                m_activeServices++;
            } else {
                m_activeServices--;
            }
            updateStatistics();
        }
    }
    
    // Visual indication of activity status
    if (active) {
        serviceItem->setBackground(0, QBrush(BACKGROUND_MEDIUM));
    } else {
        serviceItem->setBackground(0, QBrush(BACKGROUND_DARK));
    }
    
    Logger::instance().log(Logger::Debug, "ServiceExplorerPanel",
                          QString("Service 0x%1 status: %2")
                          .arg(serviceId, 4, 16, QChar('0'))
                          .arg(active ? "Active" : "Inactive"));
}

void ServiceExplorerPanel::onEnsembleUpdated(const EnsembleInfo& ensemble)
{
    m_ensembles[ensemble.ensembleId] = ensemble;
    
    // Update ensemble item if it exists
    if (m_ensembleItems.contains(ensemble.ensembleId)) {
        auto* ensembleItem = m_ensembleItems[ensemble.ensembleId];
        ensembleItem->setText(0, ensemble.label);
        ensembleItem->setText(2, QString("%1%").arg(
            ensemble.isActive ? 100 : 0));
        ensembleItem->setText(3, QString("%1 Mbps").arg(
            ensemble.totalBitrate, 0, 'f', 2));
    }
    
    updateStatistics();
    
    Logger::instance().log(Logger::Info, "ServiceExplorerPanel",
                          QString("Updated ensemble: %1 (EID: 0x%2, Services: %3)")
                          .arg(ensemble.label)
                          .arg(ensemble.ensembleId, 4, 16, QChar('0'))
                          .arg(ensemble.serviceCount));
}

void ServiceExplorerPanel::onServiceTreeItemClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    
    if (!item) return;
    
    uint32_t serviceId = item->data(0, Qt::UserRole).toUInt();
    
    // Check if it's a service item (not ensemble)
    if (m_serviceItems.contains(serviceId)) {
        m_selectedServiceId = serviceId;
        emit serviceSelected(serviceId);
        
        Logger::instance().log(Logger::Debug, "ServiceExplorerPanel",
                              QString("Service selected: SID 0x%1")
                              .arg(serviceId, 4, 16, QChar('0')));
    } else {
        // Handle ensemble selection
        uint16_t ensembleId = static_cast<uint16_t>(serviceId);
        emit ensembleSelected(ensembleId);
        
        Logger::instance().log(Logger::Debug, "ServiceExplorerPanel",
                              QString("Ensemble selected: EID 0x%1")
                              .arg(ensembleId, 4, 16, QChar('0')));
    }
}

void ServiceExplorerPanel::onServiceTreeItemChanged(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    
    if (!item) return;
    
    // Handle visibility changes for service filtering
    QList<uint32_t> visibleServices = getVisibleServices();
    emit serviceFilterChanged(visibleServices);
}

void ServiceExplorerPanel::updateStatistics()
{
    m_statisticsLabel->setText(
        tr("Services: %1 | Active: %2 | DAB+: %3 | Ensembles: %4")
        .arg(m_totalServices)
        .arg(m_activeServices)
        .arg(m_dabPlusServices)
        .arg(m_totalEnsembles)
    );
}

QList<uint32_t> ServiceExplorerPanel::getVisibleServices() const
{
    QList<uint32_t> visibleServices;
    
    for (auto it = m_serviceItems.constBegin(); it != m_serviceItems.constEnd(); ++it) {
        QTreeWidgetItem* item = it.value();
        if (item && item->checkState(0) == Qt::Checked && !item->isHidden()) {
            visibleServices.append(it.key());
        }
    }
    
    return visibleServices;
}

void ServiceExplorerPanel::applyServiceTypeColors()
{
    // Applied during service creation in addDABService()
}

void ServiceExplorerPanel::applyBroadcastingTheme()
{
    // Professional broadcast industry styling
    m_headerFont = QFont("Segoe UI", 10, QFont::Bold);
    m_serviceFont = QFont("Segoe UI", 9);
    m_statisticsFont = QFont("Segoe UI", 8);
    
    // Apply fonts
    m_headerLabel->setFont(m_headerFont);
    m_statisticsLabel->setFont(m_statisticsFont);
    m_serviceTree->setFont(m_serviceFont);
    
    // Professional styling with broadcasting industry colors
    setStyleSheet(
        "QWidget#ServiceExplorerPanel { "
        "    background-color: #2D2D30; "
        "    color: #FFFFFF; "
        "}"
        "QLabel#ServiceExplorerHeader { "
        "    font-family: 'Segoe UI'; "
        "    font-weight: bold; "
        "    font-size: 12px; "
        "    color: #FFFFFF; "
        "    padding: 8px 12px; "
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "        stop:0 #0078D4, stop:1 #005A9E); "
        "    border: 1px solid #004578; "
        "    border-radius: 4px; "
        "    margin-bottom: 2px; "
        "}"
        "QLabel#ServiceExplorerStats { "
        "    color: #CCCCCC; "
        "    font-size: 10px; "
        "    padding: 4px; "
        "    background-color: #3E3E42; "
        "    border: 1px solid #555555; "
        "    border-radius: 2px; "
        "}"
        "QTreeWidget#ServiceExplorerTree { "
        "    background-color: #2D2D30; "
        "    alternate-background-color: #3E3E42; "
        "    color: #FFFFFF; "
        "    border: 1px solid #555555; "
        "    selection-background-color: #0078D4; "
        "    outline: none; "
        "}"
        "QTreeWidget#ServiceExplorerTree::item { "
        "    padding: 4px; "
        "    border-bottom: 1px solid #404040; "
        "}"
        "QTreeWidget#ServiceExplorerTree::item:selected { "
        "    background-color: #0078D4; "
        "    color: #FFFFFF; "
        "}"
        "QLineEdit#ServiceExplorerSearch { "
        "    background-color: #3E3E42; "
        "    border: 1px solid #555555; "
        "    color: #FFFFFF; "
        "    padding: 4px; "
        "    border-radius: 3px; "
        "}"
        "QComboBox#ServiceExplorerFilter { "
        "    background-color: #3E3E42; "
        "    border: 1px solid #555555; "
        "    color: #FFFFFF; "
        "    padding: 4px; "
        "    border-radius: 3px; "
        "}"
        "QPushButton { "
        "    background-color: #0078D4; "
        "    border: 1px solid #005A9E; "
        "    color: #FFFFFF; "
        "    padding: 6px 12px; "
        "    border-radius: 3px; "
        "    font-weight: bold; "
        "}"
        "QPushButton:hover { "
        "    background-color: #106EBE; "
        "}"
        "QPushButton:pressed { "
        "    background-color: #005A9E; "
        "}"
    );
}

void ServiceExplorerPanel::connectEtiProcessor(EtiProcessor* processor)
{
    if (m_etiProcessor) {
        // Disconnect previous processor
        disconnect(m_etiProcessor, nullptr, this, nullptr);
    }
    
    m_etiProcessor = processor;
    
    if (m_etiProcessor) {
        // Connect signals for real-time service updates
        // Note: These signals would need to be implemented in EtiProcessor
        Logger::instance().log(Logger::Info, "ServiceExplorerPanel",
                              "Connected to ETI processor for service data");
    }
}

void ServiceExplorerPanel::clearServices()
{
    m_serviceTree->clear();
    m_serviceItems.clear();
    m_ensembleItems.clear();
    m_services.clear();
    m_ensembles.clear();
    
    m_totalServices = 0;
    m_activeServices = 0;
    m_dabPlusServices = 0;
    m_totalEnsembles = 0;
    m_selectedServiceId = 0;
    
    updateStatistics();
    
    Logger::instance().log(Logger::Info, "ServiceExplorerPanel", "Cleared all services");
}

void ServiceExplorerPanel::refreshServiceTree()
{
    if (m_etiProcessor) {
        // Request refresh from ETI processor
        Logger::instance().log(Logger::Debug, "ServiceExplorerPanel", "Refreshing service tree");
    }
}

void ServiceExplorerPanel::expandAll()
{
    m_serviceTree->expandAll();
}

void ServiceExplorerPanel::collapseAll()
{
    m_serviceTree->collapseAll();
}

void ServiceExplorerPanel::onSearchTextChanged(const QString& searchText)
{
    m_currentSearchText = searchText.toLower();
    applySearchFilter();
}

void ServiceExplorerPanel::onFilterTypeChanged(int filterType)
{
    m_currentFilter = static_cast<ServiceInfo::ServiceType>(
        m_filterCombo->itemData(filterType).toInt());
    applySearchFilter();
}

void ServiceExplorerPanel::onRefreshClicked()
{
    refreshServiceTree();
}

void ServiceExplorerPanel::applySearchFilter()
{
    for (auto it = m_serviceItems.constBegin(); it != m_serviceItems.constEnd(); ++it) {
        QTreeWidgetItem* item = it.value();
        const ServiceInfo& service = m_services[it.key()];
        
        bool visible = true;
        
        // Apply search text filter
        if (!m_currentSearchText.isEmpty()) {
            visible = service.label.toLower().contains(m_currentSearchText) ||
                     service.ensembleLabel.toLower().contains(m_currentSearchText);
        }
        
        // Apply service type filter
        if (visible && m_currentFilter != ServiceInfo::Unknown) {
            visible = (service.type == m_currentFilter);
        }
        
        item->setHidden(!visible);
    }
    
    // Emit filter change signal
    QList<uint32_t> visibleServices = getVisibleServices();
    emit serviceFilterChanged(visibleServices);
}

void ServiceExplorerPanel::filterByServiceType(ServiceInfo::ServiceType typeFilter)
{
    m_currentFilter = typeFilter;
    
    // Update combo box to match
    for (int i = 0; i < m_filterCombo->count(); ++i) {
        if (m_filterCombo->itemData(i).toInt() == static_cast<int>(typeFilter)) {
            m_filterCombo->setCurrentIndex(i);
            break;
        }
    }
    
    applySearchFilter();
}