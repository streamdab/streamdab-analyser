#pragma once

#include <QObject>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QKeySequence>
#include <QVariant>
#include <QPixmap>
#include <QIcon>
#include <QWidget>
#include <memory>

// Forward declarations
class EtiProcessor;
class ServiceBrowser;

/**
 * @struct WorkflowContext
 * @brief Context information for workflow-specific menus
 */
struct WorkflowContext {
    QString itemType;           ///< Type of item (frame, service, error, etc.)
    QString itemId;             ///< Unique identifier for the item
    QVariantMap properties;     ///< Additional properties
    QWidget* sourceWidget;      ///< Widget that triggered the menu
    
    WorkflowContext() : sourceWidget(nullptr) {}
    WorkflowContext(const QString& type, const QString& id, QWidget* widget = nullptr)
        : itemType(type), itemId(id), sourceWidget(widget) {}
};

Q_DECLARE_METATYPE(WorkflowContext)

/**
 * @class ContextMenuFactory
 * @brief Professional context menu generation for broadcast industry workflows
 * 
 * This enhanced context menu factory provides comprehensive workflow support
 * for professional broadcast monitoring applications with industry-standard
 * operations and keyboard shortcuts.
 */
class ContextMenuFactory : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Context menu types for different UI contexts
     */
    enum class MenuType {
        ETIFrame,           ///< ETI frame analysis operations
        ServiceTree,        ///< DAB service tree operations
        ServiceDetails,     ///< Service detail view operations
        ErrorMessage,       ///< Error handling and navigation
        AudioStream,        ///< Audio monitoring and analysis
        ComplianceReport,   ///< ETSI compliance operations
        NetworkStream,      ///< Network streaming operations
        ChartVisualization, ///< Chart and visualization operations
        FileOperations,     ///< File and data operations
        General             ///< General purpose operations
    };

    /**
     * @brief Constructor
     * @param parent Parent object
     */
    explicit ContextMenuFactory(QObject* parent = nullptr);

    /**
     * @brief Initialize factory with application components
     * @param etiProcessor ETI processor instance
     * @param serviceBrowser Service browser instance
     */
    void initialize(EtiProcessor* etiProcessor, ServiceBrowser* serviceBrowser);

    /**
     * @brief Create professional context menu
     * @param type Menu type to create
     * @param context Workflow context information
     * @return Configured context menu (caller takes ownership)
     */
    std::unique_ptr<QMenu> createContextMenu(MenuType type, const WorkflowContext& context = WorkflowContext());

    /**
     * @brief Create ETI frame analysis context menu
     * @param frameNumber Frame number for context
     * @param frameData Frame data properties
     * @return ETI frame-specific context menu
     */
    std::unique_ptr<QMenu> createETIFrameMenu(quint64 frameNumber, const QVariantMap& frameData = QVariantMap());

    /**
     * @brief Create DAB service tree context menu
     * @param serviceId Service ID for context
     * @param serviceType Service type (audio/data/packet)
     * @return Service tree-specific context menu
     */
    std::unique_ptr<QMenu> createServiceTreeMenu(const QString& serviceId, const QString& serviceType);

    /**
     * @brief Create audio stream analysis context menu
     * @param streamId Audio stream ID
     * @param codecInfo Codec information
     * @return Audio stream-specific context menu
     */
    std::unique_ptr<QMenu> createAudioStreamMenu(const QString& streamId, const QString& codecInfo);

    /**
     * @brief Create ETSI compliance context menu
     * @param violationType Type of compliance violation
     * @param severity Violation severity
     * @return Compliance-specific context menu
     */
    std::unique_ptr<QMenu> createComplianceMenu(const QString& violationType, const QString& severity);

    /**
     * @brief Create network streaming context menu
     * @param streamUrl Stream URL or address
     * @param connectionStatus Connection status
     * @return Network streaming-specific context menu
     */
    std::unique_ptr<QMenu> createNetworkStreamMenu(const QString& streamUrl, const QString& connectionStatus);

    /**
     * @brief Create chart visualization context menu
     * @param chartType Type of chart
     * @param dataSeriesId Data series identifier
     * @return Chart-specific context menu
     */
    std::unique_ptr<QMenu> createChartMenu(const QString& chartType, const QString& dataSeriesId);

    /**
     * @brief Create file operations context menu
     * @param fileName File name
     * @param fileType File type
     * @return File operations-specific context menu
     */
    std::unique_ptr<QMenu> createFileOperationsMenu(const QString& fileName, const QString& fileType);

    /**
     * @brief Set enabled/disabled state for actions
     * @param actionName Action name
     * @param enabled true to enable action
     */
    void setActionEnabled(const QString& actionName, bool enabled);

    /**
     * @brief Get action by name
     * @param actionName Action name
     * @return Action pointer or nullptr if not found
     */
    QAction* getAction(const QString& actionName) const;

    /**
     * @brief Enable professional keyboard shortcuts
     * @param enabled true to enable shortcuts
     */
    void setKeyboardShortcutsEnabled(bool enabled);

signals:
    /**
     * @brief Signal emitted when context action is triggered
     * @param actionType Type of action triggered
     * @param context Workflow context information
     */
    void contextActionTriggered(const QString& actionType, const WorkflowContext& context);

    /**
     * @brief Signal emitted for ETI frame operations
     * @param operation Operation type (analyze, extract, export)
     * @param frameNumber Frame number
     * @param properties Additional properties
     */
    void etiFrameOperation(const QString& operation, quint64 frameNumber, const QVariantMap& properties);

    /**
     * @brief Signal emitted for service operations
     * @param operation Operation type (select, analyze, monitor)
     * @param serviceId Service ID
     * @param serviceType Service type
     */
    void serviceOperation(const QString& operation, const QString& serviceId, const QString& serviceType);

    /**
     * @brief Signal emitted for audio operations
     * @param operation Operation type (play, record, analyze)
     * @param streamId Stream ID
     * @param codecInfo Codec information
     */
    void audioOperation(const QString& operation, const QString& streamId, const QString& codecInfo);

    /**
     * @brief Signal emitted for compliance operations
     * @param operation Operation type (validate, report, resolve)
     * @param violationType Violation type
     * @param severity Severity level
     */
    void complianceOperation(const QString& operation, const QString& violationType, const QString& severity);

    /**
     * @brief Signal emitted for network operations
     * @param operation Operation type (connect, disconnect, configure)
     * @param streamUrl Stream URL
     * @param parameters Operation parameters
     */
    void networkOperation(const QString& operation, const QString& streamUrl, const QVariantMap& parameters);

    /**
     * @brief Signal emitted for file operations
     * @param operation Operation type (open, save, export)
     * @param fileName File name
     * @param parameters Operation parameters
     */
    void fileOperation(const QString& operation, const QString& fileName, const QVariantMap& parameters);

private slots:
    // ETI Frame operations
    void onAnalyzeETIFrame();
    void onExtractETIFrame();
    void onExportETIFrame();
    void onJumpToFrame();
    void onViewFrameDetails();
    void onCompareFrames();

    // Service operations
    void onSelectService();
    void onMonitorService();
    void onAnalyzeService();
    void onExportServiceData();
    void onShowServiceProperties();
    void onFilterByService();

    // Audio operations
    void onPlayAudioStream();
    void onRecordAudioStream();
    void onAnalyzeAudioQuality();
    void onExportAudioData();
    void onShowAudioProperties();
    void onMonitorAudioLevels();

    // Compliance operations
    void onValidateCompliance();
    void onGenerateComplianceReport();
    void onResolveViolation();
    void onShowComplianceDetails();
    void onExportComplianceData();

    // Network operations
    void onConnectToStream();
    void onDisconnectFromStream();
    void onConfigureNetworkSettings();
    void onTestNetworkConnection();
    void onShowNetworkStatistics();
    void onRecordNetworkStream();

    // Chart operations
    void onZoomChart();
    void onResetChartView();
    void onExportChartImage();
    void onExportChartData();
    void onConfigureChart();
    void onToggleChartSeries();

    // File operations
    void onOpenFile();
    void onSaveFile();
    void onSaveAsFile();
    void onExportToCSV();
    void onExportToXML();
    void onShowFileProperties();
    void onRecentFiles();

    // General operations
    void onCopy();
    void onCopyAll();
    void onSelectAll();
    void onRefresh();
    void onProperties();
    void onHelp();

private:
    /**
     * @brief Create action with professional styling
     * @param text Action text
     * @param iconName Icon name or path
     * @param shortcut Keyboard shortcut
     * @param toolTip Tool tip text
     * @return Configured action
     */
    QAction* createAction(const QString& text, 
                         const QString& iconName = QString(), 
                         const QKeySequence& shortcut = QKeySequence(),
                         const QString& toolTip = QString());

    /**
     * @brief Create separator action
     * @return Separator action
     */
    QAction* createSeparator();

    /**
     * @brief Create submenu with actions
     * @param title Submenu title
     * @param actions List of actions to add
     * @return Configured submenu
     */
    QMenu* createSubMenu(const QString& title, const QList<QAction*>& actions);

    /**
     * @brief Create professional icon for action
     * @param iconName Icon name
     * @return Icon pixmap
     */
    QIcon createActionIcon(const QString& iconName) const;

    /**
     * @brief Get current workflow context
     * @return Current context
     */
    WorkflowContext getCurrentContext() const;

    /**
     * @brief Initialize standard actions
     */
    void initializeStandardActions();

    /**
     * @brief Connect action signals to slots
     */
    void connectActionSignals();

    // Application components
    EtiProcessor* m_etiProcessor;
    ServiceBrowser* m_serviceBrowser;

    // Standard actions
    QHash<QString, QAction*> m_actions;
    QActionGroup* m_serviceGroup;
    QActionGroup* m_audioGroup;

    // Current context
    WorkflowContext m_currentContext;

    // Settings
    bool m_keyboardShortcutsEnabled;
    bool m_initialized;
};