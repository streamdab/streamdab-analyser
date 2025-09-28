/**
 * @file service_browser.cpp
 * @brief Professional DAB/DAB+ service browser and manager implementation
 * 
 * This file implements the ServiceBrowser widget providing comprehensive
 * browsing and management of DAB services and ensembles for professional
 * broadcast monitoring environments.
 */

#include "service_browser.h"
#include "core/eti_processor.hpp"
#include "core/ensemble_manager.h"
#include "core/service_manager.hpp"
#include "utils/logger.h"

#include <QApplication>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QMutexLocker>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QProgressDialog>
#include <QClipboard>
#include <QMimeData>
#include <QDrag>
#include <QDebug>

ServiceBrowser::ServiceBrowser(QWidget *parent)
    : QWidget(parent)
    , m_etiProcessor(nullptr)
    , m_ensembleManager(nullptr)
    , m_serviceManager(nullptr)
    , m_mainLayout(nullptr)
    , m_controlLayout(nullptr)
    , m_controlGroup(nullptr)
    , m_viewModeCombo(nullptr)
    , m_filterEdit(nullptr)
    , m_refreshButton(nullptr)
    , m_expandAllButton(nullptr)
    , m_collapseAllButton(nullptr)
    , m_serviceCountLabel(nullptr)
    , m_viewSplitter(nullptr)
    , m_treeView(nullptr)
    , m_tableView(nullptr)
    , m_listView(nullptr)
    , m_contextMenu(nullptr)
    , m_selectServiceAction(nullptr)
    , m_servicePropertiesAction(nullptr)
    , m_exportServicesAction(nullptr)
    , m_selectedServiceId(0)
    , m_viewMode(ViewMode::Tree)
    , m_realTimeEnabled(false)
    , m_isInitialized(false)
    , m_updateTimer(nullptr)
    , m_totalServices(0)
    , m_activeServices(0)
    , m_filteredServices(0)
{
    // Set widget properties
    setObjectName("ServiceBrowser");
    setMinimumSize(250, 200);
    
    Logger::instance().log(Logger::Info, "ServiceBrowser", "Constructor completed");
}

ServiceBrowser::~ServiceBrowser()
{
    Logger::instance().log(Logger::Info, "ServiceBrowser", "Destructor starting");
    
    // Stop updates
    setRealTimeEnabled(false);
    
    // Cleanup timer
    if (m_updateTimer) {
        m_updateTimer->stop();
        delete m_updateTimer;
        m_updateTimer = nullptr;
    }
    
    Logger::instance().log(Logger::Info, "ServiceBrowser", "Destructor completed");
}

bool ServiceBrowser::initialize()
{
    Logger::instance().log(Logger::Info, "ServiceBrowser", "Initialization starting");
    
    try {
        // Create UI components
        createUI();
        
        // Setup real-time timer
        setupRealTimeTimer();
        
        // Generate test data
        generateTestServices();
        
        // Set initialized flag
        m_isInitialized = true;
        
        // Initial update
        updateServices();
        
        Logger::instance().log(Logger::Info, "ServiceBrowser", "Initialization completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "ServiceBrowser", 
                             QString("Initialization failed: %1").arg(e.what()));
        return false;
    }
}

void ServiceBrowser::createUI()
{
    // Main layout
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(6, 6, 6, 6);
    m_mainLayout->setSpacing(4);
    
    // Create control panel
    createControlPanel();
    
    // Create view splitter
    m_viewSplitter = new QSplitter(Qt::Vertical, this);
    
    // Create view widgets
    createTreeView();
    createTableView();
    createListView();
    
    // Add views to splitter
    m_viewSplitter->addWidget(m_treeView);
    m_viewSplitter->addWidget(m_tableView);
    m_viewSplitter->addWidget(m_listView);
    
    // Set initial view mode
    setViewMode(ViewMode::Tree);
    
    // Create context menu
    createContextMenu();
    
    // Add to main layout
    m_mainLayout->addWidget(m_controlGroup);
    m_mainLayout->addWidget(m_viewSplitter, 1);  // Expand view area
    
    Logger::instance().log(Logger::Debug, "ServiceBrowser", "UI creation completed");
}

void ServiceBrowser::createControlPanel()
{
    m_controlGroup = new QGroupBox(tr("Service Browser Control"), this);
    m_controlLayout = new QHBoxLayout(m_controlGroup);
    
    // View mode selection
    QLabel *viewLabel = new QLabel(tr("View:"), m_controlGroup);
    m_viewModeCombo = new QComboBox(m_controlGroup);
    m_viewModeCombo->addItem(tr("Tree"), static_cast<int>(ViewMode::Tree));
    m_viewModeCombo->addItem(tr("Table"), static_cast<int>(ViewMode::Table));
    m_viewModeCombo->addItem(tr("List"), static_cast<int>(ViewMode::List));
    m_viewModeCombo->addItem(tr("Grid"), static_cast<int>(ViewMode::Grid));
    
    // Filter edit
    QLabel *filterLabel = new QLabel(tr("Filter:"), m_controlGroup);
    m_filterEdit = new QLineEdit(m_controlGroup);
    m_filterEdit->setPlaceholderText(tr("Filter services..."));
    m_filterEdit->setClearButtonEnabled(true);
    
    // Control buttons
    m_refreshButton = new QPushButton(QIcon(":/icons/refresh.png"), tr("Refresh"), m_controlGroup);
    m_expandAllButton = new QPushButton(tr("Expand All"), m_controlGroup);
    m_collapseAllButton = new QPushButton(tr("Collapse All"), m_controlGroup);
    
    // Service count label
    m_serviceCountLabel = new QLabel(tr("Services: 0"), m_controlGroup);
    m_serviceCountLabel->setStyleSheet("font-weight: bold;");
    
    // Layout control panel
    m_controlLayout->addWidget(viewLabel);
    m_controlLayout->addWidget(m_viewModeCombo);
    m_controlLayout->addWidget(filterLabel);
    m_controlLayout->addWidget(m_filterEdit, 1);  // Expand filter edit
    m_controlLayout->addWidget(m_refreshButton);
    m_controlLayout->addWidget(m_expandAllButton);
    m_controlLayout->addWidget(m_collapseAllButton);
    m_controlLayout->addWidget(m_serviceCountLabel);
    
    // Connect signals
    connect(m_viewModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ServiceBrowser::handleViewModeChanged);
    connect(m_filterEdit, &QLineEdit::textChanged, this, &ServiceBrowser::handleFilterTextChanged);
    connect(m_refreshButton, &QPushButton::clicked, this, &ServiceBrowser::refreshServices);
    connect(m_expandAllButton, &QPushButton::clicked, this, &ServiceBrowser::expandAll);
    connect(m_collapseAllButton, &QPushButton::clicked, this, &ServiceBrowser::collapseAll);
}

void ServiceBrowser::createTreeView()
{
    m_treeView = new QTreeWidget(this);
    m_treeView->setObjectName("ServiceTreeView");
    m_treeView->setHeaderLabels({
        tr("Service"), tr("Type"), tr("Bit Rate"), tr("Quality"), tr("Status")
    });
    
    // Set tree properties
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setRootIsDecorated(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView->setContextMenuPolicy(Qt::DefaultContextMenu);
    
    // Configure header
    QHeaderView *header = m_treeView->header();
    header->setStretchLastSection(true);
    header->setDefaultSectionSize(100);
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);  // Service name
    
    // Connect signals
    connect(m_treeView, &QTreeWidget::itemSelectionChanged,
            this, &ServiceBrowser::handleTreeSelectionChanged);
    connect(m_treeView, &QTreeWidget::itemActivated,
            this, &ServiceBrowser::handleTreeItemActivated);
}

void ServiceBrowser::createTableView()
{
    m_tableView = new QTableWidget(0, TABLE_COLUMNS, this);
    m_tableView->setObjectName("ServiceTableView");
    m_tableView->setHorizontalHeaderLabels({
        tr("Service ID"), tr("Name"), tr("Ensemble"), tr("Type"), 
        tr("Bit Rate"), tr("Quality"), tr("Status")
    });
    
    // Set table properties
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setSortingEnabled(true);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setContextMenuPolicy(Qt::DefaultContextMenu);
    
    // Configure header
    QHeaderView *header = m_tableView->horizontalHeader();
    header->setStretchLastSection(true);
    header->setDefaultSectionSize(80);
    header->setSectionResizeMode(1, QHeaderView::Stretch);  // Service name
    
    // Hide by default
    m_tableView->setVisible(false);
    
    // Connect signals
    connect(m_tableView, &QTableWidget::itemSelectionChanged,
            this, &ServiceBrowser::handleTableSelectionChanged);
    connect(m_tableView, &QTableWidget::itemActivated,
            this, &ServiceBrowser::handleTableItemActivated);
}

void ServiceBrowser::createListView()
{
    m_listView = new QListWidget(this);
    m_listView->setObjectName("ServiceListView");
    
    // Set list properties
    m_listView->setAlternatingRowColors(true);
    m_listView->setSortingEnabled(true);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setContextMenuPolicy(Qt::DefaultContextMenu);
    
    // Hide by default
    m_listView->setVisible(false);
    
    // Connect signals
    connect(m_listView, &QListWidget::itemSelectionChanged,
            this, &ServiceBrowser::handleListSelectionChanged);
    connect(m_listView, &QListWidget::itemActivated,
            this, &ServiceBrowser::handleListItemActivated);
}

void ServiceBrowser::createContextMenu()
{
    m_contextMenu = new QMenu(this);
    
    // Create actions
    m_selectServiceAction = new QAction(QIcon(":/icons/select.png"), tr("Select Service"), this);
    m_servicePropertiesAction = new QAction(QIcon(":/icons/properties.png"), tr("Properties..."), this);
    m_exportServicesAction = new QAction(QIcon(":/icons/export.png"), tr("Export Services..."), this);
    
    // Add actions to menu
    m_contextMenu->addAction(m_selectServiceAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_servicePropertiesAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(m_exportServicesAction);
    
    // Connect actions
    connect(m_selectServiceAction, &QAction::triggered, this, &ServiceBrowser::handleSelectService);
    connect(m_servicePropertiesAction, &QAction::triggered, this, &ServiceBrowser::handleServiceProperties);
    connect(m_exportServicesAction, &QAction::triggered, this, &ServiceBrowser::handleExportServices);
}

void ServiceBrowser::setupRealTimeTimer()
{
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(UPDATE_INTERVAL_MS);  // 1 second
    m_updateTimer->setSingleShot(false);
    
    connect(m_updateTimer, &QTimer::timeout, this, &ServiceBrowser::handleRealTimeUpdate);
}

void ServiceBrowser::setEtiProcessor(EtiProcessor *processor)
{
    m_etiProcessor = processor;
    
    if (m_etiProcessor) {
        Logger::instance().log(Logger::Info, "ServiceBrowser", "ETI processor connected");
    } else {
        Logger::instance().log(Logger::Warning, "ServiceBrowser", "ETI processor disconnected");
    }
}

void ServiceBrowser::setEnsembleManager(EnsembleManager *manager)
{
    m_ensembleManager = manager;
    
    if (m_ensembleManager) {
        Logger::instance().log(Logger::Info, "ServiceBrowser", "Ensemble manager connected");
    } else {
        Logger::instance().log(Logger::Warning, "ServiceBrowser", "Ensemble manager disconnected");
    }
}

void ServiceBrowser::setServiceManager(ServiceManager *manager)
{
    m_serviceManager = manager;
    
    if (m_serviceManager) {
        Logger::instance().log(Logger::Info, "ServiceBrowser", "Service manager connected");
    } else {
        Logger::instance().log(Logger::Warning, "ServiceBrowser", "Service manager disconnected");
    }
}

void ServiceBrowser::setViewMode(ViewMode mode)
{
    if (m_viewMode != mode) {
        m_viewMode = mode;
        
        // Update combo box
        if (m_viewModeCombo) {
            m_viewModeCombo->setCurrentIndex(static_cast<int>(mode));
        }
        
        // Show/hide appropriate view widgets
        m_treeView->setVisible(mode == ViewMode::Tree);
        m_tableView->setVisible(mode == ViewMode::Table);
        m_listView->setVisible(mode == ViewMode::List || mode == ViewMode::Grid);
        
        // Update expand/collapse button state
        if (m_expandAllButton && m_collapseAllButton) {
            bool treeMode = (mode == ViewMode::Tree);
            m_expandAllButton->setEnabled(treeMode);
            m_collapseAllButton->setEnabled(treeMode);
        }
        
        // Update services for new view
        updateServices();
        
        Logger::instance().log(Logger::Info, "ServiceBrowser", 
                             QString("View mode changed to: %1").arg(static_cast<int>(mode)));
    }
}

void ServiceBrowser::setRealTimeEnabled(bool enabled)
{
    if (m_realTimeEnabled != enabled) {
        m_realTimeEnabled = enabled;
        
        if (enabled && m_updateTimer) {
            m_updateTimer->start();
            Logger::instance().log(Logger::Info, "ServiceBrowser", "Real-time updates enabled");
        } else if (m_updateTimer) {
            m_updateTimer->stop();
            Logger::instance().log(Logger::Info, "ServiceBrowser", "Real-time updates disabled");
        }
    }
}

ServiceBrowser::ServiceInfo ServiceBrowser::getServiceInfo(quint32 serviceId) const
{
    QMutexLocker locker(&const_cast<ServiceBrowser*>(this)->m_dataMutex);
    return m_services.value(serviceId, ServiceInfo());
}

QList<quint32> ServiceBrowser::getAllServices() const
{
    QMutexLocker locker(&const_cast<ServiceBrowser*>(this)->m_dataMutex);
    return m_services.keys();
}

QList<quint32> ServiceBrowser::getServicesInEnsemble(quint32 ensembleId) const
{
    QMutexLocker locker(&const_cast<ServiceBrowser*>(this)->m_dataMutex);
    
    QList<quint32> services;
    for (auto it = m_services.begin(); it != m_services.end(); ++it) {
        // Extract ensemble ID from service ID (simplified)
        quint32 serviceEnsembleId = (it->serviceId >> 16) & 0xFFFF;
        if (serviceEnsembleId == ensembleId) {
            services.append(it->serviceId);
        }
    }
    return services;
}

bool ServiceBrowser::isServiceActive(quint32 serviceId) const
{
    ServiceInfo info = getServiceInfo(serviceId);
    return info.isActive;
}

// Public slots

void ServiceBrowser::updateServices()
{
    if (!m_isInitialized) {
        return;
    }
    
    // If ETI processor is connected and we have no services, generate test data
    if (m_etiProcessor && m_services.isEmpty()) {
        Logger::instance().log(Logger::Info, "ServiceBrowser", "Generating test services for demo");
        generateTestServices();
    }
    
    // Update based on current view mode
    switch (m_viewMode) {
        case ViewMode::Tree:
            updateTreeView();
            break;
        case ViewMode::Table:
            updateTableView();
            break;
        case ViewMode::List:
        case ViewMode::Grid:
            updateListView();
            break;
    }
    
    // Update statistics
    updateServiceStatistics();
}

void ServiceBrowser::clearServices()
{
    QMutexLocker locker(&m_dataMutex);
    
    // Clear data
    m_services.clear();
    m_ensembles.clear();
    m_selectedServiceId = 0;
    
    locker.unlock();
    
    // Clear views
    if (m_treeView) {
        m_treeView->clear();
    }
    
    if (m_tableView) {
        m_tableView->setRowCount(0);
    }
    
    if (m_listView) {
        m_listView->clear();
    }
    
    // Update statistics
    updateServiceStatistics();
    
    Logger::instance().log(Logger::Info, "ServiceBrowser", "Services cleared");
}

void ServiceBrowser::refreshServices()
{
    Logger::instance().log(Logger::Info, "ServiceBrowser", "Refreshing services");
    
    // Generate new test data
    generateTestServices();
    
    // Update display
    updateServices();
    
    Logger::instance().log(Logger::Info, "ServiceBrowser", "Services refreshed");
}

void ServiceBrowser::selectService(quint32 serviceId)
{
    if (m_selectedServiceId != serviceId) {
        m_selectedServiceId = serviceId;
        
        // Update selection in current view across all view modes
        switch (m_viewMode) {
        case ViewMode::Tree:
            if (m_treeView) {
                // Find and select the service in tree view
                for (int i = 0; i < m_treeView->topLevelItemCount(); ++i) {
                    QTreeWidgetItem* ensembleItem = m_treeView->topLevelItem(i);
                    for (int j = 0; j < ensembleItem->childCount(); ++j) {
                        QTreeWidgetItem* serviceItem = ensembleItem->child(j);
                        if (serviceItem->data(0, Qt::UserRole).toUInt() == serviceId) {
                            m_treeView->setCurrentItem(serviceItem);
                            m_treeView->scrollToItem(serviceItem);
                            break;
                        }
                    }
                }
            }
            break;
            
        case ViewMode::Table:
            if (m_tableView) {
                // Find and select the service in table view
                for (int row = 0; row < m_tableView->rowCount(); ++row) {
                    QTableWidgetItem* item = m_tableView->item(row, 0);
                    if (item && item->data(Qt::UserRole).toUInt() == serviceId) {
                        m_tableView->selectRow(row);
                        m_tableView->scrollToItem(item);
                        break;
                    }
                }
            }
            break;
            
        case ViewMode::List:
            if (m_listView) {
                // Find and select the service in list view
                for (int row = 0; row < m_listView->count(); ++row) {
                    QListWidgetItem* item = m_listView->item(row);
                    if (item && item->data(Qt::UserRole).toUInt() == serviceId) {
                        m_listView->setCurrentItem(item);
                        m_listView->scrollToItem(item);
                        break;
                    }
                }
            }
            break;
        }
        
        emit serviceSelectionChanged(serviceId);
        
        Logger::instance().log(Logger::Debug, "ServiceBrowser", 
                             QString("Service selected: %1").arg(serviceId));
    }
}

void ServiceBrowser::expandAll()
{
    if (m_treeView && m_viewMode == ViewMode::Tree) {
        m_treeView->expandAll();
        Logger::instance().log(Logger::Debug, "ServiceBrowser", "Tree expanded");
    }
}

void ServiceBrowser::collapseAll()
{
    if (m_treeView && m_viewMode == ViewMode::Tree) {
        m_treeView->collapseAll();
        Logger::instance().log(Logger::Debug, "ServiceBrowser", "Tree collapsed");
    }
}

void ServiceBrowser::filterServices(const QString& filterText)
{
    m_filterText = filterText;
    updateServices();
    
    Logger::instance().log(Logger::Debug, "ServiceBrowser", 
                         QString("Filter applied: %1").arg(filterText));
}

// Protected methods

void ServiceBrowser::contextMenuEvent(QContextMenuEvent *event)
{
    if (m_contextMenu) {
        // Update context menu actions based on selection
        bool hasSelection = (m_selectedServiceId != 0);
        m_selectServiceAction->setEnabled(hasSelection);
        m_servicePropertiesAction->setEnabled(hasSelection);
        
        // Show context menu
        m_contextMenu->exec(event->globalPos());
    }
    
    QWidget::contextMenuEvent(event);
}

// Private slots

void ServiceBrowser::handleTreeSelectionChanged()
{
    QList<QTreeWidgetItem*> selectedItems = m_treeView->selectedItems();
    if (!selectedItems.isEmpty()) {
        QTreeWidgetItem *item = selectedItems.first();
        
        // Check if this is a service item (has parent)
        if (item->parent()) {
            bool ok;
            quint32 serviceId = item->data(0, Qt::UserRole).toUInt(&ok);
            if (ok) {
                selectService(serviceId);
            }
        }
    }
}

void ServiceBrowser::handleTreeItemActivated(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)
    
    if (item && item->parent()) {
        bool ok;
        quint32 serviceId = item->data(0, Qt::UserRole).toUInt(&ok);
        if (ok) {
            emit serviceActivated(serviceId);
            Logger::instance().log(Logger::Debug, "ServiceBrowser", 
                                 QString("Service activated: %1").arg(serviceId));
        }
    }
}

void ServiceBrowser::handleTableSelectionChanged()
{
    int row = m_tableView->currentRow();
    if (row >= 0) {
        QTableWidgetItem *item = m_tableView->item(row, 0);  // Service ID column
        if (item) {
            bool ok;
            quint32 serviceId = item->data(Qt::UserRole).toUInt(&ok);
            if (ok) {
                selectService(serviceId);
            }
        }
    }
}

void ServiceBrowser::handleTableItemActivated(QTableWidgetItem *item)
{
    if (item) {
        int row = item->row();
        QTableWidgetItem *idItem = m_tableView->item(row, 0);
        if (idItem) {
            bool ok;
            quint32 serviceId = idItem->data(Qt::UserRole).toUInt(&ok);
            if (ok) {
                emit serviceActivated(serviceId);
                Logger::instance().log(Logger::Debug, "ServiceBrowser", 
                                     QString("Service activated: %1").arg(serviceId));
            }
        }
    }
}

void ServiceBrowser::handleListSelectionChanged()
{
    QListWidgetItem *item = m_listView->currentItem();
    if (item) {
        bool ok;
        quint32 serviceId = item->data(Qt::UserRole).toUInt(&ok);
        if (ok) {
            selectService(serviceId);
        }
    }
}

void ServiceBrowser::handleListItemActivated(QListWidgetItem *item)
{
    if (item) {
        bool ok;
        quint32 serviceId = item->data(Qt::UserRole).toUInt(&ok);
        if (ok) {
            emit serviceActivated(serviceId);
            Logger::instance().log(Logger::Debug, "ServiceBrowser", 
                                 QString("Service activated: %1").arg(serviceId));
        }
    }
}

void ServiceBrowser::handleViewModeChanged()
{
    if (m_viewModeCombo) {
        int modeIndex = m_viewModeCombo->currentIndex();
        setViewMode(static_cast<ViewMode>(modeIndex));
    }
}

void ServiceBrowser::handleFilterTextChanged()
{
    if (m_filterEdit) {
        filterServices(m_filterEdit->text());
    }
}

void ServiceBrowser::handleRealTimeUpdate()
{
    if (!m_realTimeEnabled || !m_isInitialized) {
        return;
    }
    
    // Update service information
    // In a real implementation, this would query the ETI processor
    // For now, we'll just update timestamps and quality values
    
    QMutexLocker locker(&m_dataMutex);
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    for (auto& service : m_services) {
        service.lastUpdate = currentTime;
        
        // Simulate quality changes
        if (service.isActive) {
            // Small random quality variation
            int qualityChange = (rand() % 21) - 10;  // -10 to +10
            int newQuality = static_cast<int>(service.quality) + qualityChange;
            newQuality = qBound(0, newQuality, 4);
            service.quality = static_cast<ServiceQuality>(newQuality);
        }
    }
    
    locker.unlock();
    
    // Update display
    updateServices();
}

void ServiceBrowser::handleSelectService()
{
    if (m_selectedServiceId != 0) {
        emit serviceActivated(m_selectedServiceId);
    }
}

void ServiceBrowser::handleServiceProperties()
{
    if (m_selectedServiceId != 0) {
        ServiceInfo info = getServiceInfo(m_selectedServiceId);
        
        QString properties = tr("Service Properties\n\n");
        properties += tr("Service ID: %1\n").arg(QString::number(info.serviceId, 16).toUpper());
        properties += tr("Name: %1\n").arg(info.serviceName);
        properties += tr("Ensemble: %1\n").arg(info.ensembleName);
        properties += tr("Type: %1\n").arg(getServiceTypeString(info.serviceType));
        properties += tr("Bit Rate: %1 kbps\n").arg(info.bitRate);
        properties += tr("Quality: %1\n").arg(getServiceQualityString(info.quality));
        properties += tr("Audio Format: %1\n").arg(info.audioFormat);
        properties += tr("Language: %1\n").arg(info.language);
        properties += tr("Description: %1\n").arg(info.description);
        properties += tr("Active: %1\n").arg(info.isActive ? tr("Yes") : tr("No"));
        
        QMessageBox::information(this, tr("Service Properties"), properties);
        
        Logger::instance().log(Logger::Info, "ServiceBrowser", 
                             QString("Properties shown for service: %1").arg(m_selectedServiceId));
    }
}

void ServiceBrowser::handleExportServices()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Export Service List"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/services.csv",
        tr("CSV Files (*.csv);;Text Files (*.txt);;All Files (*)")
    );
    
    if (!fileName.isEmpty()) {
        // Implement comprehensive service export functionality
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            
            // Determine export format based on file extension
            bool isCsvFormat = fileName.endsWith(".csv", Qt::CaseInsensitive);
            QString separator = isCsvFormat ? "," : "\t";
            
            // Write header
            if (isCsvFormat) {
                out << "Service ID,Service Name,Ensemble,Type,Bit Rate (kbps),Quality,Audio Format,Language,Active,Description\n";
            } else {
                out << "DAB/DAB+ Service Export - " << QDateTime::currentDateTime().toString() << "\n";
                out << "==========================================\n\n";
            }
            
            // Export services from current data
            QList<quint32> serviceIds = getAllServices();
            QList<ServiceInfo> services;
            for (quint32 serviceId : serviceIds) {
                services.append(getServiceInfo(serviceId));
            }
            for (const ServiceInfo& service : services) {
                if (isCsvFormat) {
                    // CSV format with proper escaping
                    out << QString("0x%1").arg(service.serviceId, 4, 16, QChar('0')).toUpper() << separator;
                    QString escapedServiceName = service.serviceName;
                    QString escapedEnsembleName = service.ensembleName;
                    out << "\"" << escapedServiceName.replace("\"", "\"\"") << "\"" << separator;
                    out << "\"" << escapedEnsembleName.replace("\"", "\"\"") << "\"" << separator;
                    out << getServiceTypeString(service.serviceType) << separator;
                    out << service.bitRate << separator;
                    out << getServiceQualityString(service.quality) << separator;
                    out << service.audioFormat << separator;
                    out << service.language << separator;
                    out << (service.isActive ? "Yes" : "No") << separator;
                    QString escapedDescription = service.description;
                    out << "\"" << escapedDescription.replace("\"", "\"\"") << "\"\n";
                } else {
                    // Human-readable text format
                    out << "Service: " << service.serviceName << "\n";
                    out << "  ID: 0x" << QString::number(service.serviceId, 16).toUpper() << "\n";
                    out << "  Ensemble: " << service.ensembleName << "\n";
                    out << "  Type: " << getServiceTypeString(service.serviceType) << "\n";
                    out << "  Bit Rate: " << service.bitRate << " kbps\n";
                    out << "  Quality: " << getServiceQualityString(service.quality) << "\n";
                    out << "  Audio Format: " << service.audioFormat << "\n";
                    out << "  Language: " << service.language << "\n";
                    out << "  Active: " << (service.isActive ? "Yes" : "No") << "\n";
                    if (!service.description.isEmpty()) {
                        out << "  Description: " << service.description << "\n";
                    }
                    out << "\n";
                }
            }
            
            // Add summary for text format
            if (!isCsvFormat) {
                out << "Export Summary:\n";
                out << "Total Services: " << services.size() << "\n";
                out << "Active Services: " << std::count_if(services.begin(), services.end(), 
                                                           [](const ServiceInfo& s) { return s.isActive; }) << "\n";
                out << "Export Date: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
            }
            
            file.close();
            QMessageBox::information(this, tr("Export Successful"), 
                                   tr("Service list exported successfully to:\n%1\n\n"
                                      "Exported %2 services").arg(fileName).arg(services.size()));
        } else {
            QMessageBox::warning(this, tr("Export Failed"), 
                               tr("Could not write to file:\n%1").arg(fileName));
        }
        
        Logger::instance().log(Logger::Info, "ServiceBrowser", 
                             QString("Service export requested: %1").arg(fileName));
    }
}

// Private helper methods

void ServiceBrowser::updateTreeView()
{
    if (!m_treeView) {
        return;
    }
    
    QMutexLocker locker(&m_dataMutex);
    
    // Clear existing tree
    m_treeView->clear();
    
    // Group services by ensemble
    QHash<quint32, QList<ServiceInfo>> ensembleServices;
    m_filteredServices = 0;
    
    for (const ServiceInfo& service : m_services) {
        if (filterServiceItem(service, m_filterText)) {
            quint32 ensembleId = (service.serviceId >> 16) & 0xFFFF;
            ensembleServices[ensembleId].append(service);
            m_filteredServices++;
        }
    }
    
    locker.unlock();
    
    // Add ensemble and service items
    for (auto it = ensembleServices.begin(); it != ensembleServices.end(); ++it) {
        quint32 ensembleId = it.key();
        QString ensembleName = m_ensembles.value(ensembleId, tr("Unknown Ensemble"));
        
        QTreeWidgetItem *ensembleItem = addEnsembleToTree(ensembleId, ensembleName);
        
        for (const ServiceInfo& service : it.value()) {
            addServiceToTree(ensembleItem, service);
        }
        
        // Expand ensemble by default
        ensembleItem->setExpanded(true);
    }
    
    // Sort tree
    m_treeView->sortByColumn(0, Qt::AscendingOrder);
}

void ServiceBrowser::updateTableView()
{
    if (!m_tableView) {
        return;
    }
    
    QMutexLocker locker(&m_dataMutex);
    
    // Clear existing table
    m_tableView->setRowCount(0);
    m_filteredServices = 0;
    
    // Add services to table
    for (const ServiceInfo& service : m_services) {
        if (filterServiceItem(service, m_filterText)) {
            addServiceToTable(service);
            m_filteredServices++;
        }
    }
    
    locker.unlock();
    
    // Sort table
    m_tableView->sortByColumn(1, Qt::AscendingOrder);  // Sort by name
}

void ServiceBrowser::updateListView()
{
    if (!m_listView) {
        return;
    }
    
    QMutexLocker locker(&m_dataMutex);
    
    // Clear existing list
    m_listView->clear();
    m_filteredServices = 0;
    
    // Add services to list
    for (const ServiceInfo& service : m_services) {
        if (filterServiceItem(service, m_filterText)) {
            addServiceToList(service);
            m_filteredServices++;
        }
    }
    
    locker.unlock();
    
    // Sort list
    m_listView->sortItems(Qt::AscendingOrder);
}

QTreeWidgetItem* ServiceBrowser::addEnsembleToTree(quint32 ensembleId, const QString& ensembleName)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(m_treeView);
    item->setText(0, ensembleName);
    item->setText(1, tr("Ensemble"));
    item->setText(2, tr("-"));
    item->setText(3, tr("-"));
    item->setText(4, tr("Active"));
    
    item->setData(0, Qt::UserRole, ensembleId);
    item->setIcon(0, QIcon(":/icons/ensemble.png"));
    item->setFont(0, QFont("Arial", 9, QFont::Bold));
    
    return item;
}

QTreeWidgetItem* ServiceBrowser::addServiceToTree(QTreeWidgetItem* ensembleItem, const ServiceInfo& serviceInfo)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(ensembleItem);
    item->setText(0, serviceInfo.serviceName);
    item->setText(1, getServiceTypeString(serviceInfo.serviceType));
    item->setText(2, tr("%1 kbps").arg(serviceInfo.bitRate));
    item->setText(3, getServiceQualityString(serviceInfo.quality));
    item->setText(4, serviceInfo.isActive ? tr("Active") : tr("Inactive"));
    
    item->setData(0, Qt::UserRole, serviceInfo.serviceId);
    item->setIcon(0, QIcon(":/icons/service.png"));
    
    // Set quality-based coloring
    QColor qualityColor = getQualityColor(serviceInfo.quality);
    item->setForeground(3, QBrush(qualityColor));
    
    // Set inactive service styling
    if (!serviceInfo.isActive) {
        item->setForeground(0, QBrush(Qt::gray));
    }
    
    return item;
}

int ServiceBrowser::addServiceToTable(const ServiceInfo& serviceInfo)
{
    int row = m_tableView->rowCount();
    m_tableView->insertRow(row);
    
    // Service ID
    QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(serviceInfo.serviceId, 16).toUpper());
    idItem->setData(Qt::UserRole, serviceInfo.serviceId);
    m_tableView->setItem(row, 0, idItem);
    
    // Service Name
    QTableWidgetItem *nameItem = new QTableWidgetItem(serviceInfo.serviceName);
    nameItem->setIcon(QIcon(":/icons/service.png"));
    if (!serviceInfo.isActive) {
        nameItem->setForeground(QBrush(Qt::gray));
    }
    m_tableView->setItem(row, 1, nameItem);
    
    // Ensemble
    m_tableView->setItem(row, 2, new QTableWidgetItem(serviceInfo.ensembleName));
    
    // Type
    m_tableView->setItem(row, 3, new QTableWidgetItem(getServiceTypeString(serviceInfo.serviceType)));
    
    // Bit Rate
    m_tableView->setItem(row, 4, new QTableWidgetItem(tr("%1 kbps").arg(serviceInfo.bitRate)));
    
    // Quality
    QTableWidgetItem *qualityItem = new QTableWidgetItem(getServiceQualityString(serviceInfo.quality));
    qualityItem->setForeground(QBrush(getQualityColor(serviceInfo.quality)));
    m_tableView->setItem(row, 5, qualityItem);
    
    // Status
    m_tableView->setItem(row, 6, new QTableWidgetItem(serviceInfo.isActive ? tr("Active") : tr("Inactive")));
    
    return row;
}

QListWidgetItem* ServiceBrowser::addServiceToList(const ServiceInfo& serviceInfo)
{
    QString itemText = QString("%1 (%2)")
                      .arg(serviceInfo.serviceName)
                      .arg(getServiceTypeString(serviceInfo.serviceType));
    
    QListWidgetItem *item = new QListWidgetItem(itemText, m_listView);
    item->setData(Qt::UserRole, serviceInfo.serviceId);
    item->setIcon(QIcon(":/icons/service.png"));
    
    // Set inactive service styling
    if (!serviceInfo.isActive) {
        item->setForeground(QBrush(Qt::gray));
    }
    
    // Set tooltip with detailed information
    QString tooltip = tr("Service: %1\nEnsemble: %2\nType: %3\nBit Rate: %4 kbps\nQuality: %5")
                     .arg(serviceInfo.serviceName)
                     .arg(serviceInfo.ensembleName)
                     .arg(getServiceTypeString(serviceInfo.serviceType))
                     .arg(serviceInfo.bitRate)
                     .arg(getServiceQualityString(serviceInfo.quality));
    item->setToolTip(tooltip);
    
    return item;
}

QString ServiceBrowser::getServiceTypeString(ServiceType type) const
{
    switch (type) {
        case ServiceType::Audio: return tr("Audio");
        case ServiceType::Data: return tr("Data");
        case ServiceType::FIDC: return tr("FIDC");
        case ServiceType::MSC: return tr("MSC");
        case ServiceType::DAB: return tr("DAB");
        case ServiceType::DABPlus: return tr("DAB+");
        case ServiceType::DMB: return tr("DMB");
        default: return tr("Unknown");
    }
}

QString ServiceBrowser::getServiceQualityString(ServiceQuality quality) const
{
    switch (quality) {
        case ServiceQuality::Poor: return tr("Poor");
        case ServiceQuality::Fair: return tr("Fair");
        case ServiceQuality::Good: return tr("Good");
        case ServiceQuality::Excellent: return tr("Excellent");
        default: return tr("Unknown");
    }
}

QColor ServiceBrowser::getQualityColor(ServiceQuality quality) const
{
    switch (quality) {
        case ServiceQuality::Poor: return Qt::red;
        case ServiceQuality::Fair: return QColor(255, 165, 0);  // Orange
        case ServiceQuality::Good: return QColor(255, 215, 0);   // Gold
        case ServiceQuality::Excellent: return Qt::green;
        default: return Qt::gray;
    }
}

bool ServiceBrowser::filterServiceItem(const ServiceInfo& serviceInfo, const QString& filterText) const
{
    if (filterText.isEmpty()) {
        return true;
    }
    
    QString lowerFilter = filterText.toLower();
    
    return serviceInfo.serviceName.toLower().contains(lowerFilter) ||
           serviceInfo.ensembleName.toLower().contains(lowerFilter) ||
           getServiceTypeString(serviceInfo.serviceType).toLower().contains(lowerFilter) ||
           serviceInfo.description.toLower().contains(lowerFilter);
}

void ServiceBrowser::generateTestServices()
{
    QMutexLocker locker(&m_dataMutex);
    
    // Clear existing data
    m_services.clear();
    m_ensembles.clear();
    
    // Add test ensembles
    m_ensembles[0x1234] = "BBC National DAB";
    m_ensembles[0x5678] = "Commercial Radio";
    m_ensembles[0x9ABC] = "Local DAB Network";
    
    // Add test services using individual ServiceInfo objects
    QList<ServiceInfo> testServices;
    
    // BBC National DAB
    ServiceInfo bbc1; bbc1.serviceId = 0x12340001; bbc1.serviceName = "BBC Radio 1"; bbc1.ensembleName = "BBC National DAB"; bbc1.serviceType = ServiceType::DABPlus; bbc1.bitRate = 128; bbc1.quality = ServiceQuality::Excellent; bbc1.audioFormat = "AAC"; bbc1.language = "English"; bbc1.description = "Popular music and entertainment"; bbc1.isActive = true; bbc1.lastUpdate = 0;
    testServices.append(bbc1);
    
    ServiceInfo bbc2; bbc2.serviceId = 0x12340002; bbc2.serviceName = "BBC Radio 2"; bbc2.ensembleName = "BBC National DAB"; bbc2.serviceType = ServiceType::DABPlus; bbc2.bitRate = 128; bbc2.quality = ServiceQuality::Excellent; bbc2.audioFormat = "AAC"; bbc2.language = "English"; bbc2.description = "Popular music and culture"; bbc2.isActive = true; bbc2.lastUpdate = 0;
    testServices.append(bbc2);
    
    ServiceInfo bbc3; bbc3.serviceId = 0x12340003; bbc3.serviceName = "BBC Radio 3"; bbc3.ensembleName = "BBC National DAB"; bbc3.serviceType = ServiceType::DAB; bbc3.bitRate = 192; bbc3.quality = ServiceQuality::Good; bbc3.audioFormat = "MP2"; bbc3.language = "English"; bbc3.description = "Classical music and arts"; bbc3.isActive = true; bbc3.lastUpdate = 0;
    testServices.append(bbc3);
    
    ServiceInfo bbc4; bbc4.serviceId = 0x12340004; bbc4.serviceName = "BBC Radio 4"; bbc4.ensembleName = "BBC National DAB"; bbc4.serviceType = ServiceType::DABPlus; bbc4.bitRate = 96; bbc4.quality = ServiceQuality::Excellent; bbc4.audioFormat = "AAC"; bbc4.language = "English"; bbc4.description = "News, drama and documentaries"; bbc4.isActive = true; bbc4.lastUpdate = 0;
    testServices.append(bbc4);
    
    ServiceInfo bbc6; bbc6.serviceId = 0x12340005; bbc6.serviceName = "BBC 6 Music"; bbc6.ensembleName = "BBC National DAB"; bbc6.serviceType = ServiceType::DABPlus; bbc6.bitRate = 128; bbc6.quality = ServiceQuality::Good; bbc6.audioFormat = "AAC"; bbc6.language = "English"; bbc6.description = "Alternative and indie music"; bbc6.isActive = true; bbc6.lastUpdate = 0;
    testServices.append(bbc6);
    
    // Commercial Radio
    ServiceInfo heart; heart.serviceId = 0x56780001; heart.serviceName = "Heart FM"; heart.ensembleName = "Commercial Radio"; heart.serviceType = ServiceType::DABPlus; heart.bitRate = 128; heart.quality = ServiceQuality::Good; heart.audioFormat = "AAC"; heart.language = "English"; heart.description = "Hit music station"; heart.isActive = true; heart.lastUpdate = 0;
    testServices.append(heart);
    
    // Add services to hash
    for (const ServiceInfo& service : testServices) {
        m_services[service.serviceId] = service;
    }
    
    // Update statistics
    m_totalServices = m_services.size();
    m_activeServices = 0;
    for (const ServiceInfo& service : m_services) {
        if (service.isActive) {
            m_activeServices++;
        }
    }
    
    locker.unlock();
    
    Logger::instance().log(Logger::Info, "ServiceBrowser", 
                         QString("Generated %1 test services").arg(m_totalServices));
}

void ServiceBrowser::updateServiceStatistics()
{
    if (m_serviceCountLabel) {
        QString countText = tr("Services: %1/%2").arg(m_filteredServices).arg(m_totalServices);
        if (m_activeServices != m_totalServices) {
            countText += tr(" (Active: %1)").arg(m_activeServices);
        }
        m_serviceCountLabel->setText(countText);
    }
}

bool ServiceBrowser::validateState() const
{
    return (m_isInitialized &&
            m_mainLayout != nullptr &&
            m_treeView != nullptr &&
            m_tableView != nullptr &&
            m_listView != nullptr &&
            m_updateTimer != nullptr);
}

// Enhanced DAB+ file handling methods
void ServiceBrowser::setupDabPlusFilters()
{
    Logger::instance().log(Logger::Debug, "ServiceBrowser", "Setting up DAB+ file filters");
    
    // Initialize DAB+ file extensions
    m_dabPlusExtensions << "*.eti" << "*.etini" << "*.etili" << "*.dab" << "*.dabplus";
    
    // Add file type recognition
    addFileTypeRecognition();
    
    // Create metadata preview panel
    createMetadataPreview();
    
    // Enable drag and drop for file handling
    setAcceptDrops(true);
    
    Logger::instance().log(Logger::Info, "ServiceBrowser", 
                          QString("DAB+ file filters configured: %1").arg(m_dabPlusExtensions.join(", ")));
}

bool ServiceBrowser::validateDabPlusFile(const QString& filename)
{
    Logger::instance().log(Logger::Debug, "ServiceBrowser", 
                          QString("Validating DAB+ file: %1").arg(filename));
    
    if (filename.isEmpty()) {
        Logger::instance().log(Logger::Warning, "ServiceBrowser", "Empty filename provided for validation");
        return false;
    }
    
    QFileInfo fileInfo(filename);
    
    // Check if file exists
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        Logger::instance().log(Logger::Warning, "ServiceBrowser", 
                              QString("File does not exist or is not a file: %1").arg(filename));
        return false;
    }
    
    // Check file extension
    QString extension = "*." + fileInfo.suffix().toLower();
    if (!m_dabPlusExtensions.contains(extension, Qt::CaseInsensitive)) {
        Logger::instance().log(Logger::Warning, "ServiceBrowser", 
                              QString("File extension not recognized as DAB+: %1").arg(extension));
        return false;
    }
    
    // Check file size (reasonable limits for ETI files)
    qint64 fileSize = fileInfo.size();
    if (fileSize < 1024) { // Less than 1KB
        Logger::instance().log(Logger::Warning, "ServiceBrowser", 
                              QString("File too small to be valid ETI: %1 bytes").arg(fileSize));
        return false;
    }
    
    if (fileSize > 1024 * 1024 * 1024) { // Greater than 1GB
        Logger::instance().log(Logger::Warning, "ServiceBrowser", 
                              QString("File suspiciously large for ETI: %1 bytes").arg(fileSize));
        return false;
    }
    
    // Perform basic ETI header validation
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        Logger::instance().log(Logger::Warning, "ServiceBrowser", 
                              QString("Cannot open file for validation: %1").arg(filename));
        return false;
    }
    
    // Read first few bytes to check for ETI frame sync
    QByteArray header = file.read(32);
    file.close();
    
    if (header.size() < 4) {
        Logger::instance().log(Logger::Warning, "ServiceBrowser", "File too small for ETI header validation");
        return false;
    }
    
    // Check for ETI frame sync pattern (0xFF pattern or specific ETI markers)
    bool hasEtiMarkers = false;
    
    // Look for common ETI patterns
    if (header.contains("ETI") || header.contains("DAB") || 
        header.at(0) == static_cast<char>(0xFF)) {
        hasEtiMarkers = true;
    }
    
    // Check for consistent frame structure indicators
    for (int i = 0; i < header.size() - 4; i += 4) {
        if (static_cast<unsigned char>(header.at(i)) == 0xFF) {
            hasEtiMarkers = true;
            break;
        }
    }
    
    if (!hasEtiMarkers) {
        Logger::instance().log(Logger::Warning, "ServiceBrowser", 
                              "File does not contain recognizable ETI frame markers");
        return false;
    }
    
    Logger::instance().log(Logger::Info, "ServiceBrowser", 
                          QString("DAB+ file validation successful: %1").arg(filename));
    return true;
}

void ServiceBrowser::showFileMetadataPreview(const QString& filename)
{
    Logger::instance().log(Logger::Debug, "ServiceBrowser", 
                          QString("Showing file metadata preview: %1").arg(filename));
    
    if (!m_metadataPreview) {
        Logger::instance().log(Logger::Warning, "ServiceBrowser", "Metadata preview widget not initialized");
        return;
    }
    
    QString metadataInfo = extractDabPlusInfo(filename);
    m_metadataPreview->setText(metadataInfo);
    m_metadataPreview->setVisible(true);
    
    // Update UI to show metadata panel
    if (m_metadataPreview->parent()) {
        QWidget* parentWidget = qobject_cast<QWidget*>(m_metadataPreview->parent());
        if (parentWidget) {
            parentWidget->setVisible(true);
        }
    }
}

void ServiceBrowser::onDabPlusFileSelected(const QString& filename)
{
    Logger::instance().log(Logger::Info, "ServiceBrowser", 
                          QString("DAB+ file selected: %1").arg(filename));
    
    // Validate the selected file
    if (!validateDabPlusFile(filename)) {
        QMessageBox::warning(this, tr("Invalid DAB+ File"), 
                           tr("The selected file does not appear to be a valid DAB+ ETI file:\n%1")
                           .arg(filename));
        return;
    }
    
    // Show metadata preview
    showFileMetadataPreview(filename);
    
    // Emit signal for main window to handle file loading
    emit serviceSelectionChanged(0); // Special service ID for file selection
    
    // Update service browser with file information
    if (m_etiProcessor) {
        // Process the file through ETI processor
        // This would typically trigger service discovery and populate the browser
        Logger::instance().log(Logger::Info, "ServiceBrowser", 
                              "Requesting ETI processor to load file for service analysis");
    }
}

void ServiceBrowser::onFileDropped(const QStringList& filenames)
{
    Logger::instance().log(Logger::Info, "ServiceBrowser", 
                          QString("Files dropped: %1").arg(filenames.join(", ")));
    
    if (filenames.isEmpty()) {
        return;
    }
    
    // Process each dropped file
    QStringList validFiles;
    QStringList invalidFiles;
    
    for (const QString& filename : filenames) {
        if (validateDabPlusFile(filename)) {
            validFiles.append(filename);
        } else {
            invalidFiles.append(filename);
        }
    }
    
    // Show summary of dropped files
    if (!invalidFiles.isEmpty()) {
        QMessageBox::information(this, tr("File Drop Results"),
                               tr("Valid DAB+ files: %1\nInvalid files (ignored): %2")
                               .arg(validFiles.size())
                               .arg(invalidFiles.size()));
    }
    
    // Process the first valid file
    if (!validFiles.isEmpty()) {
        onDabPlusFileSelected(validFiles.first());
        
        // If multiple valid files, offer to process them in sequence
        if (validFiles.size() > 1) {
            int ret = QMessageBox::question(this, tr("Multiple Files"),
                                          tr("Process all %1 valid files in sequence?")
                                          .arg(validFiles.size()),
                                          QMessageBox::Yes | QMessageBox::No);
            
            if (ret == QMessageBox::Yes) {
                // Queue additional files for processing
                for (int i = 1; i < validFiles.size(); ++i) {
                    // This would typically queue files for sequential processing
                    Logger::instance().log(Logger::Info, "ServiceBrowser", 
                                          QString("Queued for processing: %1").arg(validFiles.at(i)));
                }
            }
        }
    }
}

void ServiceBrowser::addFileTypeRecognition()
{
    Logger::instance().log(Logger::Debug, "ServiceBrowser", "Adding file type recognition capabilities");
    
    // Add tooltips and visual indicators for different file types
    if (m_treeView) {
        m_treeView->setToolTip(tr("Drag and drop DAB+ ETI files here for analysis\n"
                                "Supported formats: .eti, .etini, .etili, .dab, .dabplus"));
    }
    
    if (m_tableView) {
        m_tableView->setToolTip(tr("Services and ensembles from loaded ETI files\n"
                                 "Drop DAB+ files to analyze their content"));
    }
    
    if (m_listView) {
        m_listView->setToolTip(tr("Service list from DAB+ ensemble analysis\n"
                               "Supports drag and drop of ETI files"));
    }
}

void ServiceBrowser::createMetadataPreview()
{
    Logger::instance().log(Logger::Debug, "ServiceBrowser", "Creating metadata preview panel");
    
    // Create metadata preview label if not already created
    if (!m_metadataPreview) {
        m_metadataPreview = new QLabel(this);
        m_metadataPreview->setObjectName("metadataPreview");
        m_metadataPreview->setWordWrap(true);
        m_metadataPreview->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        m_metadataPreview->setStyleSheet(
            "QLabel#metadataPreview {"
            "    background-color: #f0f0f0;"
            "    border: 1px solid #cccccc;"
            "    border-radius: 4px;"
            "    padding: 8px;"
            "    margin: 4px;"
            "    font-family: 'Courier New', monospace;"
            "    font-size: 9pt;"
            "}"
        );
        m_metadataPreview->setMinimumHeight(100);
        m_metadataPreview->setMaximumHeight(200);
        m_metadataPreview->hide(); // Initially hidden
        
        // Add to layout if main layout exists
        if (m_mainLayout) {
            m_mainLayout->addWidget(m_metadataPreview);
        }
    }
}

QString ServiceBrowser::extractDabPlusInfo(const QString& filename)
{
    Logger::instance().log(Logger::Debug, "ServiceBrowser", 
                          QString("Extracting DAB+ info from: %1").arg(filename));
    
    QFileInfo fileInfo(filename);
    QString info;
    
    // Basic file information
    info += tr("DAB+ File Information\n");
    info += tr("========================\n\n");
    info += tr("Filename: %1\n").arg(fileInfo.fileName());
    info += tr("Path: %1\n").arg(fileInfo.absolutePath());
    info += tr("Size: %1 bytes (%2)\n")
            .arg(fileInfo.size())
            .arg(formatFileSize(fileInfo.size()));
    info += tr("Modified: %1\n").arg(fileInfo.lastModified().toString(Qt::ISODate));
    info += tr("Type: %1\n\n").arg(fileInfo.suffix().toUpper());
    
    // ETI-specific information (basic analysis)
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray header = file.read(1024); // Read first 1KB for analysis
        file.close();
        
        info += tr("ETI Stream Analysis\n");
        info += tr("==================\n\n");
        
        // Analyze frame structure
        int syncPatterns = 0;
        
        for (int i = 0; i < header.size() - 4; i++) {
            if (static_cast<unsigned char>(header.at(i)) == 0xFF) {
                syncPatterns++;
            }
        }
        
        info += tr("Sync patterns detected: %1\n").arg(syncPatterns);
        info += tr("Estimated frame rate: %1 Hz\n").arg(syncPatterns > 0 ? "~24" : "Unknown");
        
        // Check for DAB+ indicators
        bool hasDabPlus = header.contains("AAC") || header.contains("HE-");
        info += tr("DAB+ indicators: %1\n").arg(hasDabPlus ? "Present" : "Not detected");
        
        // Estimate duration based on file size
        qint64 estimatedFrames = fileInfo.size() / 6144; // Typical ETI frame size
        double estimatedDuration = estimatedFrames / 24.0; // 24 fps typical
        info += tr("Estimated duration: %1 seconds\n").arg(estimatedDuration, 0, 'f', 1);
        
        info += tr("\nNote: This is a preliminary analysis.\n");
        info += tr("Load the file for complete service discovery.");
    } else {
        info += tr("Error: Cannot read file for detailed analysis.\n");
    }
    
    return info;
}

QString ServiceBrowser::formatFileSize(qint64 bytes)
{
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;
    
    if (bytes >= GB) {
        return tr("%1 GB").arg(static_cast<double>(bytes) / GB, 0, 'f', 2);
    } else if (bytes >= MB) {
        return tr("%1 MB").arg(static_cast<double>(bytes) / MB, 0, 'f', 1);
    } else if (bytes >= KB) {
        return tr("%1 KB").arg(static_cast<double>(bytes) / KB, 0, 'f', 1);
    } else {
        return tr("%1 bytes").arg(bytes);
    }
}

// ========== UI INTEGRATION SPECIALIST: SERVICE DISCOVERY HANDLERS ==========

void ServiceBrowser::addDiscoveredService(const eti::DabService& service)
{
    QMutexLocker locker(&m_dataMutex);
    
    // Convert DAB service to ServiceBrowser format
    ServiceInfo serviceInfo;
    serviceInfo.serviceId = service.serviceId;
    serviceInfo.serviceName = QString::fromStdString(service.label);
    serviceInfo.ensembleName = QString::fromStdString(service.ensembleLabel);
    
    // Determine service type based on DAB service type
    switch (service.serviceType) {
        case 0: // Audio service
            serviceInfo.serviceType = ServiceType::DAB;
            if (service.subChannelId != 0) {
                serviceInfo.serviceType = ServiceType::DABPlus; // Assume DAB+ for subchannel services
            }
            break;
        case 1: // Data service
            serviceInfo.serviceType = ServiceType::Data;
            break;
        default:
            serviceInfo.serviceType = ServiceType::Unknown;
            break;
    }
    
    serviceInfo.bitRate = service.bitRate;
    serviceInfo.quality = ServiceQuality::Good; // Default quality
    serviceInfo.audioFormat = QString::fromStdString(service.audioFormat);
    serviceInfo.language = QString::fromStdString(service.language);
    
    // Add to internal service list (avoid duplicates)
    bool serviceExists = false;
    for (const auto& existing : m_discoveredServices) {
        if (existing.serviceId == serviceInfo.serviceId) {
            serviceExists = true;
            break;
        }
    }
    
    if (!serviceExists) {
        m_discoveredServices.append(serviceInfo);
        
        // Update UI elements
        switch (m_viewMode) {
            case ViewMode::Tree:
                if (m_treeView) {
                    // Find or create ensemble item
                    QTreeWidgetItem* ensembleItem = nullptr;
                    for (int i = 0; i < m_treeView->topLevelItemCount(); ++i) {
                        QTreeWidgetItem* item = m_treeView->topLevelItem(i);
                        if (item && item->text(0) == serviceInfo.ensembleName) {
                            ensembleItem = item;
                            break;
                        }
                    }
                    
                    if (!ensembleItem) {
                        ensembleItem = new QTreeWidgetItem(m_treeView);
                        ensembleItem->setText(0, serviceInfo.ensembleName);
                        ensembleItem->setText(1, "Ensemble");
                        ensembleItem->setIcon(0, QIcon(":/icons/ensemble.png"));
                    }
                    
                    // Add service to ensemble
                    addServiceToTree(ensembleItem, serviceInfo);
                    ensembleItem->setExpanded(true);
                }
                break;
                
            case ViewMode::Table:
                addServiceToTable(serviceInfo);
                break;
                
            case ViewMode::List:
                addServiceToList(serviceInfo);
                break;
                
            case ViewMode::Grid:
                // Grid view implementation would go here
                break;
        }
        
        // Update service count
        updateServiceCount();
        
        Logger::instance().log(Logger::Info, "ServiceBrowser", 
                              QString("Added discovered service: %1 (ID: %2)")
                              .arg(serviceInfo.serviceName).arg(serviceInfo.serviceId));
    }
}

void ServiceBrowser::updateEnsemble(const eti::Ensemble& ensemble)
{
    QMutexLocker locker(&m_dataMutex);
    
    // Store current ensemble information
    m_currentEnsemble = QString::fromStdString(ensemble.label);
    
    // Update ensemble information display
    if (m_serviceCountLabel) {
        m_serviceCountLabel->setText(tr("Ensemble: %1 (%2 services)")
                                   .arg(m_currentEnsemble)
                                   .arg(ensemble.services.size()));
    }
    
    // Clear existing services for fresh ensemble data
    clearServices();
    
    // Add all services from the ensemble
    for (const auto& service : ensemble.services) {
        addDiscoveredService(service);
    }
    
    Logger::instance().log(Logger::Info, "ServiceBrowser", 
                          QString("Updated ensemble: %1 with %2 services")
                          .arg(m_currentEnsemble).arg(ensemble.services.size()));
}

void ServiceBrowser::updateServiceCount()
{
    m_totalServices = m_discoveredServices.size();
    m_activeServices = 0;
    m_filteredServices = 0;
    
    // Count active and filtered services
    for (const auto& service : m_discoveredServices) {
        if (service.quality != ServiceQuality::Unknown) {
            m_activeServices++;
        }
    }
    
    // Update display
    if (m_serviceCountLabel) {
        if (m_currentEnsemble.isEmpty()) {
            m_serviceCountLabel->setText(tr("Services: %1 total, %2 active")
                                       .arg(m_totalServices).arg(m_activeServices));
        } else {
            m_serviceCountLabel->setText(tr("%1: %2 services")
                                       .arg(m_currentEnsemble).arg(m_totalServices));
        }
    }
}