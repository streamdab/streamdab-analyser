#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QTimer>
#include <QHash>
#include <QMap>
#include <QColor>
#include <QFont>
#include <QBrush>
#include <QIcon>
#include "core/eti_types.hpp"

// Forward declarations
class EtiProcessor;

/**
 * @struct ServiceInfo
 * @brief Professional DAB service information structure
 */
struct ServiceInfo {
    uint32_t serviceId;
    uint16_t ensembleId;
    QString label;
    QString ensembleLabel;
    QString typeString;
    int quality;           // 0-100%
    int bitrate;          // kbps
    bool isActive;
    bool isDabPlus;
    
    enum ServiceType {
        Unknown = 0,
        Audio = 1,
        Data = 2,
        DABPlus = 3,
        Packet = 4
    } type;
    
    ServiceInfo() : serviceId(0), ensembleId(0), quality(0), bitrate(0), 
                   isActive(false), isDabPlus(false), type(Unknown) {}
    
    QString getTypeString() const {
        switch (type) {
            case Audio: return "Audio";
            case Data: return "Data";
            case DABPlus: return "DAB+";
            case Packet: return "Packet";
            default: return "Unknown";
        }
    }
};

/**
 * @struct EnsembleInfo
 * @brief Professional DAB ensemble information structure
 */
struct EnsembleInfo {
    uint16_t ensembleId;
    QString label;
    QString country;
    int serviceCount;
    int dabPlusCount;
    double totalBitrate;  // Mbps
    bool isActive;
    
    EnsembleInfo() : ensembleId(0), serviceCount(0), dabPlusCount(0), 
                    totalBitrate(0.0), isActive(false) {}
};

/**
 * @class ServiceExplorerPanel
 * @brief Professional DAB Service Explorer Panel for Broadcasting Industry
 * 
 * This panel provides comprehensive DAB service tree visualization with:
 * - Hierarchical ensemble/service structure
 * - Professional broadcasting industry styling
 * - Real-time service status updates
 * - Quality indicators and bitrate information
 * - Service type color coding
 * - Interactive service selection
 * - Filtering and search capabilities
 */
class ServiceExplorerPanel : public QWidget
{
    Q_OBJECT
    
public:
    explicit ServiceExplorerPanel(QWidget* parent = nullptr);
    ~ServiceExplorerPanel();
    
    /**
     * @brief Connect to ETI processor for service data
     * @param processor ETI processor instance
     */
    void connectEtiProcessor(EtiProcessor* processor);
    
    /**
     * @brief Get currently selected service ID
     * @return Selected service ID (0 if none)
     */
    uint32_t getSelectedServiceId() const { return m_selectedServiceId; }
    
    /**
     * @brief Get list of visible service IDs
     * @return List of visible service IDs
     */
    QList<uint32_t> getVisibleServices() const;
    
    /**
     * @brief Apply professional broadcasting theme
     */
    void applyBroadcastingTheme();
    
public slots:
    /**
     * @brief Add DAB service to the tree
     * @param service Service information
     */
    void addDABService(const ServiceInfo& service);
    
    /**
     * @brief Update service status
     * @param serviceId Service ID
     * @param active Activity status
     */
    void updateServiceStatus(uint32_t serviceId, bool active);
    
    /**
     * @brief Update ensemble information
     * @param ensemble Ensemble information
     */
    void onEnsembleUpdated(const EnsembleInfo& ensemble);

    /**
     * @brief Handle service discovery from HeadlessETIProcessor
     * @param serviceId Service ID
     * @param serviceName Service name/label
     * @param serviceType Service type string
     */
    void onServiceDiscoveredFromProcessor(uint32_t serviceId, const QString& serviceName, const QString& serviceType);

    /**
     * @brief Handle ensemble discovery from HeadlessETIProcessor
     * @param ensembleId Ensemble ID
     * @param ensembleName Ensemble name/label
     */
    void onEnsembleDiscoveredFromProcessor(uint16_t ensembleId, const QString& ensembleName);
    
    /**
     * @brief Clear all services from tree
     */
    void clearServices();
    
    /**
     * @brief Refresh service tree display
     */
    void refreshServiceTree();
    
    /**
     * @brief Filter services by type
     * @param typeFilter Service type filter
     */
    void filterByServiceType(ServiceInfo::ServiceType typeFilter);
    
    /**
     * @brief Expand all ensemble nodes
     */
    void expandAll();
    
    /**
     * @brief Collapse all ensemble nodes  
     */
    void collapseAll();
    
signals:
    /**
     * @brief Emitted when service is selected
     * @param serviceId Selected service ID
     */
    void serviceSelected(uint32_t serviceId);
    
    /**
     * @brief Emitted when service filter changes
     * @param visibleServices List of visible service IDs
     */
    void serviceFilterChanged(const QList<uint32_t>& visibleServices);
    
    /**
     * @brief Emitted when ensemble is selected
     * @param ensembleId Selected ensemble ID
     */
    void ensembleSelected(uint16_t ensembleId);
    
private slots:
    /**
     * @brief Handle service tree item click
     * @param item Clicked item
     * @param column Clicked column
     */
    void onServiceTreeItemClicked(QTreeWidgetItem* item, int column);
    
    /**
     * @brief Handle service tree item change
     * @param item Changed item
     * @param column Changed column
     */
    void onServiceTreeItemChanged(QTreeWidgetItem* item, int column);
    
    /**
     * @brief Handle search text change
     * @param searchText Search text
     */
    void onSearchTextChanged(const QString& searchText);
    
    /**
     * @brief Handle filter combo box change
     * @param filterType Filter type index
     */
    void onFilterTypeChanged(int filterType);
    
    /**
     * @brief Handle refresh button click
     */
    void onRefreshClicked();
    
    /**
     * @brief Handle ensemble discovery from ETI processor
     * @param ensemble Discovered ensemble information
     */
    void onEnsembleDiscovered(const eti::Ensemble& ensemble);
    
    /**
     * @brief Handle service discovery from ETI processor
     * @param service Discovered service information
     */
    void onServiceDiscovered(const eti::DabService& service);
    
    /**
     * @brief Handle service quality change from ETI processor
     * @param serviceId Service ID
     * @param signalStrength Signal strength (0.0-1.0)
     * @param errorRate Error rate (0.0-1.0)
     */
    void onServiceQualityChanged(uint16_t serviceId, double signalStrength, double errorRate);
    
private:
    /**
     * @brief Setup service tree widget
     */
    void setupServiceTree();
    
    /**
     * @brief Setup control panel
     */
    void setupControlPanel();
    
    /**
     * @brief Apply service type colors
     */
    void applyServiceTypeColors();
    
    /**
     * @brief Find or create ensemble item
     * @param ensembleId Ensemble ID
     * @return Ensemble tree item
     */
    QTreeWidgetItem* findOrCreateEnsembleItem(uint16_t ensembleId);
    
    /**
     * @brief Find service item by ID
     * @param serviceId Service ID
     * @return Service tree item or nullptr
     */
    QTreeWidgetItem* findServiceItem(uint32_t serviceId);
    
    /**
     * @brief Get service type color
     * @param type Service type
     * @return Color for service type
     */
    QColor getServiceTypeColor(ServiceInfo::ServiceType type) const;
    
    /**
     * @brief Update statistics display
     */
    void updateStatistics();
    
    /**
     * @brief Apply search filter
     */
    void applySearchFilter();
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_controlLayout;
    QTreeWidget* m_serviceTree;
    QLabel* m_headerLabel;
    QLabel* m_statisticsLabel;
    QLineEdit* m_searchEdit;
    QComboBox* m_filterCombo;
    QPushButton* m_refreshButton;
    QPushButton* m_expandAllButton;
    QPushButton* m_collapseAllButton;
    
    // Data structures
    QHash<uint16_t, QTreeWidgetItem*> m_ensembleItems;
    QHash<uint32_t, QTreeWidgetItem*> m_serviceItems;
    QHash<uint32_t, ServiceInfo> m_services;
    QHash<uint16_t, EnsembleInfo> m_ensembles;
    
    // State
    EtiProcessor* m_etiProcessor;
    uint32_t m_selectedServiceId;
    ServiceInfo::ServiceType m_currentFilter;
    QString m_currentSearchText;
    
    // Statistics
    int m_totalServices;
    int m_activeServices;
    int m_dabPlusServices;
    int m_totalEnsembles;
    
    // Professional styling
    QFont m_headerFont;
    QFont m_serviceFont;
    QFont m_statisticsFont;
    
    // Colors for broadcast industry theme
    static const QColor BACKGROUND_DARK;     // #2D2D30
    static const QColor BACKGROUND_MEDIUM;   // #3E3E42
    static const QColor ACCENT_BLUE;         // #0078D4
    static const QColor SERVICE_AUDIO;       // #00AA00 (Green)
    static const QColor SERVICE_DATA;        // #FF8C00 (Orange)
    static const QColor SERVICE_DABPLUS;     // #0078D4 (Blue)
    static const QColor SERVICE_ENSEMBLE;    // #8A2BE2 (Purple)
    static const QColor ERROR_CRITICAL;      // #DC3545 (Red)
    static const QColor WARNING_MEDIUM;      // #FFC107 (Yellow)
    static const QColor TEXT_PRIMARY;        // #FFFFFF
    static const QColor TEXT_SECONDARY;      // #CCCCCC
};