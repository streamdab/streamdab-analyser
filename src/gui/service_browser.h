#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QGroupBox>
#include <QSplitter>
#include <QHeaderView>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QMutex>
#include <QHash>
#include <QString>
#include <QStringList>
#include <memory>

// Forward declarations
class EtiProcessor;  // Qt camelCase for GUI layer
class EnsembleManager;
class ServiceManager;

/**
 * @class ServiceBrowser
 * @brief Professional DAB/DAB+ service browser and manager widget
 * 
 * This widget provides comprehensive browsing and management of DAB services
 * and ensembles with professional broadcast industry features including:
 * - Hierarchical ensemble and service tree view
 * - Real-time service information updates
 * - Service quality monitoring
 * - Audio format and metadata display
 * - Service selection and control
 * - EPG (Electronic Program Guide) integration
 * - Professional broadcast workflow support
 * - Memory-efficient data management
 */
class ServiceBrowser : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief View mode enumeration
     */
    enum class ViewMode {
        Tree,          ///< Hierarchical tree view
        Table,         ///< Table view with details
        List,          ///< Simple list view
        Grid          ///< Grid view with thumbnails
    };

    /**
     * @brief Service type enumeration
     */
    enum class ServiceType {
        Unknown = 0,
        Audio = 1,
        Data = 2,
        FIDC = 3,
        MSC = 4,
        DAB = 5,
        DABPlus = 6,
        DMB = 7
    };

    /**
     * @brief Service quality enumeration
     */
    enum class ServiceQuality {
        Unknown,
        Poor,
        Fair,
        Good,
        Excellent
    };

    /**
     * @brief Service information structure
     */
    struct ServiceInfo {
        quint32 serviceId;
        QString serviceName;
        QString ensembleName;
        ServiceType serviceType;
        quint32 bitRate;
        ServiceQuality quality;
        QString audioFormat;
        QString language;
        QString description;
        bool isActive;
        qint64 lastUpdate;
        
        ServiceInfo() : serviceId(0), serviceType(ServiceType::Unknown), 
                       bitRate(0), quality(ServiceQuality::Unknown), 
                       isActive(false), lastUpdate(0) {}
    };

    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit ServiceBrowser(QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~ServiceBrowser();

    /**
     * @brief Initialize the service browser
     * @return true if successful
     */
    bool initialize();

    /**
     * @brief Set ETI processor for service data
     * @param processor ETI processor instance
     */
    void setEtiProcessor(EtiProcessor *processor);

    /**
     * @brief Set ensemble manager
     * @param manager Ensemble manager instance
     */
    void setEnsembleManager(EnsembleManager *manager);

    /**
     * @brief Set service manager
     * @param manager Service manager instance
     */
    void setServiceManager(ServiceManager *manager);

    /**
     * @brief Set view mode
     * @param mode View mode to set
     */
    void setViewMode(ViewMode mode);

    /**
     * @brief Get current view mode
     * @return Current view mode
     */
    ViewMode getViewMode() const { return m_viewMode; }

    /**
     * @brief Get selected service ID
     * @return Selected service ID (0 if none)
     */
    quint32 getSelectedServiceId() const { return m_selectedServiceId; }

    /**
     * @brief Get service information
     * @param serviceId Service ID
     * @return Service information structure
     */
    ServiceInfo getServiceInfo(quint32 serviceId) const;

    /**
     * @brief Get all services
     * @return List of all service IDs
     */
    QList<quint32> getAllServices() const;

    /**
     * @brief Get services in ensemble
     * @param ensembleId Ensemble ID
     * @return List of service IDs in ensemble
     */
    QList<quint32> getServicesInEnsemble(quint32 ensembleId) const;

    /**
     * @brief Check if service is active
     * @param serviceId Service ID
     * @return true if service is active
     */
    bool isServiceActive(quint32 serviceId) const;

    /**
     * @brief Enable/disable real-time updates
     * @param enabled true to enable real-time updates
     */
    void setRealTimeEnabled(bool enabled);

    /**
     * @brief Check if real-time updates are enabled
     * @return true if real-time updates enabled
     */
    bool isRealTimeEnabled() const { return m_realTimeEnabled; }

    /**
     * @brief Get total number of services
     * @return Total service count
     */
    int getServiceCount() const { return m_totalServices; }

    /**
     * @brief Enhanced DAB+ file handling methods
     */
    void setupDabPlusFilters();
    bool validateDabPlusFile(const QString& filename);
    void showFileMetadataPreview(const QString& filename);

public slots:
    /**
     * @brief Update service browser with new data
     */
    void updateServices();

    /**
     * @brief Clear all service data
     */
    void clearServices();

    /**
     * @brief Add a newly discovered DAB service
     * @param service Discovered service information
     */
    void addDiscoveredService(const eti::DabService& service);

    /**
     * @brief Update ensemble information
     * @param ensemble Discovered ensemble information
     */
    void updateEnsemble(const eti::Ensemble& ensemble);

    /**
     * @brief Refresh service information
     */
    void refreshServices();

    /**
     * @brief Select service by ID
     * @param serviceId Service ID to select
     */
    void selectService(quint32 serviceId);

    /**
     * @brief Expand all ensemble nodes
     */
    void expandAll();

    /**
     * @brief Collapse all ensemble nodes
     */
    void collapseAll();

    /**
     * @brief Filter services by text
     * @param filterText Filter text
     */
    void filterServices(const QString& filterText);

    /**
     * @brief Enhanced DAB+ file handling slots
     */
    void onDabPlusFileSelected(const QString& filename);
    void onFileDropped(const QStringList& filenames);

signals:
    /**
     * @brief Emitted when service selection changes
     * @param serviceId Selected service ID
     */
    void serviceSelectionChanged(quint32 serviceId);

    /**
     * @brief Emitted when service is activated (double-clicked)
     * @param serviceId Activated service ID
     */
    void serviceActivated(quint32 serviceId);

    /**
     * @brief Emitted when service information updates
     * @param serviceId Updated service ID
     */
    void serviceUpdated(quint32 serviceId);

    /**
     * @brief Emitted when new service is discovered
     * @param serviceId New service ID
     */
    void serviceDiscovered(quint32 serviceId);

    /**
     * @brief Emitted when service is lost
     * @param serviceId Lost service ID
     */
    void serviceLost(quint32 serviceId);

protected:
    /**
     * @brief Handle context menu events
     * @param event Context menu event
     */
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    /**
     * @brief Handle tree item selection change
     */
    void handleTreeSelectionChanged();

    /**
     * @brief Handle tree item activation (double-click)
     * @param item Activated item
     * @param column Column index
     */
    void handleTreeItemActivated(QTreeWidgetItem *item, int column);

    /**
     * @brief Handle table item selection change
     */
    void handleTableSelectionChanged();

    /**
     * @brief Handle table item activation (double-click)
     * @param item Activated item
     */
    void handleTableItemActivated(QTableWidgetItem *item);

    /**
     * @brief Handle list item selection change
     */
    void handleListSelectionChanged();

    /**
     * @brief Handle list item activation (double-click)
     * @param item Activated item
     */
    void handleListItemActivated(QListWidgetItem *item);

    /**
     * @brief Handle view mode change
     */
    void handleViewModeChanged();

    /**
     * @brief Handle filter text change
     */
    void handleFilterTextChanged();

    /**
     * @brief Handle real-time update timer
     */
    void handleRealTimeUpdate();

    /**
     * @brief Handle context menu actions
     */
    void handleSelectService();
    void handleServiceProperties();
    void handleExportServices();

private:
    /**
     * @brief Create the UI layout
     */
    void createUI();

    /**
     * @brief Create control panel
     */
    void createControlPanel();

    /**
     * @brief Create tree view
     */
    void createTreeView();

    /**
     * @brief Create table view
     */
    void createTableView();

    /**
     * @brief Create list view
     */
    void createListView();

    /**
     * @brief Create context menu
     */
    void createContextMenu();

    /**
     * @brief Setup real-time timer
     */
    void setupRealTimeTimer();

    /**
     * @brief Update tree view
     */
    void updateTreeView();

    /**
     * @brief Update table view
     */
    void updateTableView();

    /**
     * @brief Update list view
     */
    void updateListView();

    /**
     * @brief Add ensemble to tree
     * @param ensembleId Ensemble ID
     * @param ensembleName Ensemble name
     * @return Tree widget item for ensemble
     */
    QTreeWidgetItem* addEnsembleToTree(quint32 ensembleId, const QString& ensembleName);

    /**
     * @brief Add service to tree
     * @param ensembleItem Parent ensemble item
     * @param serviceInfo Service information
     * @return Tree widget item for service
     */
    QTreeWidgetItem* addServiceToTree(QTreeWidgetItem* ensembleItem, const ServiceInfo& serviceInfo);

    /**
     * @brief Add service to table
     * @param serviceInfo Service information
     * @return Table row index
     */
    int addServiceToTable(const ServiceInfo& serviceInfo);

    /**
     * @brief Add service to list
     * @param serviceInfo Service information
     * @return List widget item
     */
    QListWidgetItem* addServiceToList(const ServiceInfo& serviceInfo);

    /**
     * @brief Get service type string
     * @param type Service type
     * @return Human-readable service type
     */
    QString getServiceTypeString(ServiceType type) const;

    /**
     * @brief Get service quality string
     * @param quality Service quality
     * @return Human-readable quality description
     */
    QString getServiceQualityString(ServiceQuality quality) const;

    /**
     * @brief Get quality color
     * @param quality Service quality
     * @return Color for quality indication
     */
    QColor getQualityColor(ServiceQuality quality) const;

    /**
     * @brief Filter service item
     * @param serviceInfo Service information
     * @param filterText Filter text
     * @return true if service matches filter
     */
    bool filterServiceItem(const ServiceInfo& serviceInfo, const QString& filterText) const;

    /**
     * @brief Generate test services
     */
    void generateTestServices();

    /**
     * @brief Update service statistics
     */
    void updateServiceStatistics();

    /**
     * @brief Validate widget state
     * @return true if state is valid
     */
    bool validateState() const;

    /**
     * @brief DAB+ file handling private methods
     */
    void addFileTypeRecognition();
    void createMetadataPreview();
    QString extractDabPlusInfo(const QString& filename);
    QString formatFileSize(qint64 bytes);

    // Core components
    EtiProcessor *m_etiProcessor;
    EnsembleManager *m_ensembleManager;
    ServiceManager *m_serviceManager;

    // Main layout
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_controlLayout;

    // Control panel
    QGroupBox *m_controlGroup;
    QComboBox *m_viewModeCombo;
    QLineEdit *m_filterEdit;
    QPushButton *m_refreshButton;
    QPushButton *m_expandAllButton;
    QPushButton *m_collapseAllButton;
    QLabel *m_serviceCountLabel;

    // View widgets
    QSplitter *m_viewSplitter;
    QTreeWidget *m_treeView;
    QTableWidget *m_tableView;
    QListWidget *m_listView;

    // Context menu
    QMenu *m_contextMenu;
    QAction *m_selectServiceAction;
    QAction *m_servicePropertiesAction;
    QAction *m_exportServicesAction;

    // Service data
    QHash<quint32, ServiceInfo> m_services;
    QHash<quint32, QString> m_ensembles;
    quint32 m_selectedServiceId;
    QString m_filterText;

    // View state
    ViewMode m_viewMode;
    bool m_realTimeEnabled;
    bool m_isInitialized;

    // Real-time update system
    QTimer *m_updateTimer;
    QMutex m_dataMutex;

    // Statistics
    int m_totalServices;
    int m_activeServices;
    int m_filteredServices;

    // DAB+ file handling
    QStringList m_dabPlusExtensions;
    QLabel *m_metadataPreview;

    // UI Integration Specialist: Service discovery integration
    QList<ServiceInfo> m_discoveredServices;  // List of discovered services
    QString m_currentEnsemble;                // Current ensemble name

    // Constants
    static constexpr int UPDATE_INTERVAL_MS = 1000;  // 1 second updates
    static constexpr int TABLE_COLUMNS = 7;  // Table column count
};