#pragma once

#include <QObject>
#include <QDialog>
#include <QProgressDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QFileDialog>
#include <QProgressBar>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <memory>

// Forward declarations
class EtiProcessor;
class ServiceBrowser;
class PerformanceDashboard;

/**
 * @enum ExportFormat
 * @brief Supported export formats for professional reporting
 */
enum class ExportFormat {
    CSV,                    ///< Comma-separated values
    XML,                    ///< XML with ETI schema
    JSON,                   ///< JSON for API integration
    HTML,                   ///< HTML report with styling
    PDF,                    ///< Professional PDF report
    Excel,                  ///< Microsoft Excel format
    YAML,                   ///< YAML configuration format
    Binary,                 ///< Binary ETI data
    Text                    ///< Plain text summary
};

/**
 * @enum ExportScope
 * @brief Scope of data to export
 */
enum class ExportScope {
    CurrentView,            ///< Currently visible data
    SelectedItems,          ///< Selected items only
    AllData,                ///< Complete dataset
    TimeRange,              ///< Specific time range
    FilteredData,           ///< Filtered/searched data
    ComplianceResults,      ///< ETSI compliance results only
    PerformanceMetrics,     ///< Performance data only
    ServiceDetails          ///< Service information only
};

/**
 * @enum ExportTemplate
 * @brief Professional report templates
 */
enum class ExportTemplate {
    BroadcastAnalysis,      ///< Complete broadcast analysis report
    ComplianceReport,       ///< ETSI compliance report
    PerformanceReport,      ///< Performance monitoring report
    ServiceInventory,       ///< DAB service inventory
    ErrorSummary,           ///< Error analysis summary
    TechnicalSpecification, ///< Technical specification document
    CustomFormat            ///< User-defined custom format
};

/**
 * @struct ExportConfiguration
 * @brief Configuration for export operations
 */
struct ExportConfiguration {
    ExportFormat format;
    ExportScope scope;
    ExportTemplate template_;
    QString fileName;
    QString outputDirectory;
    QDateTime startTime;
    QDateTime endTime;
    QStringList includedFields;
    QStringList excludedFields;
    QVariantMap customSettings;
    bool includeMetadata;
    bool includeCharts;
    bool includeRawData;
    bool compressOutput;
    QString compressionLevel;
    
    ExportConfiguration() 
        : format(ExportFormat::CSV)
        , scope(ExportScope::CurrentView)
        , template_(ExportTemplate::BroadcastAnalysis)
        , includeMetadata(true)
        , includeCharts(false)
        , includeRawData(false)
        , compressOutput(false)
        , compressionLevel("normal")
    {
        startTime = QDateTime::currentDateTime().addSecs(-3600); // Last hour
        endTime = QDateTime::currentDateTime();
    }
};

/**
 * @class ExportWorker
 * @brief Background worker for data export operations
 */
class ExportWorker : public QObject
{
    Q_OBJECT

public:
    explicit ExportWorker(const ExportConfiguration& config, QObject* parent = nullptr);
    
public slots:
    void startExport();
    void cancelExport();
    
signals:
    void progressUpdated(int percentage, const QString& status);
    void exportCompleted(bool success, const QString& fileName);
    void exportError(const QString& error);
    
private:
    void exportToCSV();
    void exportToXML();
    void exportToJSON();
    void exportToHTML();
    void exportToPDF();
    void exportToExcel();
    void exportToYAML();
    void exportToBinary();
    void exportToText();
    
    ExportConfiguration m_config;
    bool m_cancelled;
    QMutex m_mutex;
};

/**
 * @class ExportConfigurationDialog
 * @brief Professional export configuration dialog
 */
class ExportConfigurationDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ExportConfigurationDialog(QWidget* parent = nullptr);
    
    ExportConfiguration getConfiguration() const;
    void setConfiguration(const ExportConfiguration& config);
    
private slots:
    void onFormatChanged();
    void onScopeChanged();
    void onTemplateChanged();
    void onBrowseOutputDirectory();
    void onPreviewExport();
    void onResetToDefaults();
    void onLoadPreset();
    void onSavePreset();
    
private:
    void setupUI();
    void setupFormatOptions();
    void setupScopeOptions();
    void setupTemplateOptions();
    void setupAdvancedOptions();
    void updateUIForFormat();
    void updateUIForScope();
    void validateConfiguration();
    
    // UI components
    QVBoxLayout* m_mainLayout;
    QGridLayout* m_configLayout;
    
    QGroupBox* m_formatGroup;
    QComboBox* m_formatCombo;
    QLabel* m_formatDescription;
    
    QGroupBox* m_scopeGroup;
    QComboBox* m_scopeCombo;
    QLabel* m_scopeDescription;
    
    QGroupBox* m_templateGroup;
    QComboBox* m_templateCombo;
    QLabel* m_templateDescription;
    
    QGroupBox* m_outputGroup;
    QLineEdit* m_fileNameEdit;
    QLineEdit* m_outputDirEdit;
    QPushButton* m_browseButton;
    
    QGroupBox* m_timeRangeGroup;
    QDateTime m_startTime;
    QDateTime m_endTime;
    
    QGroupBox* m_optionsGroup;
    QCheckBox* m_includeMetadataCheck;
    QCheckBox* m_includeChartsCheck;
    QCheckBox* m_includeRawDataCheck;
    QCheckBox* m_compressOutputCheck;
    QComboBox* m_compressionCombo;
    
    QGroupBox* m_fieldsGroup;
    QTextEdit* m_includedFieldsEdit;
    QTextEdit* m_excludedFieldsEdit;
    
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_previewButton;
    QPushButton* m_resetButton;
    QPushButton* m_loadPresetButton;
    QPushButton* m_savePresetButton;
    QPushButton* m_exportButton;
    QPushButton* m_cancelButton;
    
    ExportConfiguration m_config;
};

/**
 * @class ProfessionalExportManager
 * @brief Comprehensive data export manager for broadcast applications
 * 
 * This manager provides professional-grade data export capabilities
 * designed for broadcast monitoring environments with support for
 * multiple formats, templates, and advanced configuration options.
 */
class ProfessionalExportManager : public QObject
{
    Q_OBJECT

public:
    explicit ProfessionalExportManager(QObject* parent = nullptr);
    ~ProfessionalExportManager();

    /**
     * @brief Initialize export manager with application components
     * @param etiProcessor ETI processor instance
     * @param serviceBrowser Service browser instance
     * @param dashboard Performance dashboard instance
     */
    void initialize(EtiProcessor* etiProcessor, 
                   ServiceBrowser* serviceBrowser,
                   PerformanceDashboard* dashboard);

    /**
     * @brief Show export configuration dialog
     * @param config Initial configuration
     * @return true if export was initiated
     */
    bool showExportDialog(const ExportConfiguration& config = ExportConfiguration());

    /**
     * @brief Start export with configuration
     * @param config Export configuration
     * @return true if export started successfully
     */
    bool startExport(const ExportConfiguration& config);

    /**
     * @brief Quick export with default settings
     * @param format Export format
     * @param fileName Output file name
     * @return true if export started successfully
     */
    bool quickExport(ExportFormat format, const QString& fileName = QString());

    /**
     * @brief Export current view data
     * @param format Export format
     * @return true if export started successfully
     */
    bool exportCurrentView(ExportFormat format = ExportFormat::CSV);

    /**
     * @brief Export selected items
     * @param format Export format
     * @return true if export started successfully
     */
    bool exportSelectedItems(ExportFormat format = ExportFormat::CSV);

    /**
     * @brief Generate compliance report
     * @param template_ Report template
     * @param fileName Output file name
     * @return true if export started successfully
     */
    bool generateComplianceReport(ExportTemplate template_ = ExportTemplate::ComplianceReport,
                                 const QString& fileName = QString());

    /**
     * @brief Generate performance report
     * @param template_ Report template
     * @param fileName Output file name
     * @return true if export started successfully
     */
    bool generatePerformanceReport(ExportTemplate template_ = ExportTemplate::PerformanceReport,
                                  const QString& fileName = QString());

    /**
     * @brief Cancel current export operation
     */
    void cancelExport();

    /**
     * @brief Check if export is currently running
     * @return true if export is active
     */
    bool isExportRunning() const { return m_exportRunning; }

    /**
     * @brief Get supported export formats
     * @return List of supported formats
     */
    static QStringList getSupportedFormats();

    /**
     * @brief Get available export templates
     * @return List of available templates
     */
    static QStringList getAvailableTemplates();

    /**
     * @brief Get default output directory
     * @return Default output directory path
     */
    QString getDefaultOutputDirectory() const;

    /**
     * @brief Set default output directory
     * @param directory Directory path
     */
    void setDefaultOutputDirectory(const QString& directory);

    /**
     * @brief Load export configuration from file
     * @param fileName Configuration file name
     * @return Loaded configuration
     */
    ExportConfiguration loadConfiguration(const QString& fileName) const;

    /**
     * @brief Save export configuration to file
     * @param config Configuration to save
     * @param fileName Configuration file name
     * @return true if saved successfully
     */
    bool saveConfiguration(const ExportConfiguration& config, const QString& fileName) const;

public slots:
    /**
     * @brief Show quick export menu
     */
    void showQuickExportMenu();

    /**
     * @brief Export to clipboard
     * @param format Export format
     */
    void exportToClipboard(ExportFormat format = ExportFormat::CSV);

signals:
    /**
     * @brief Emitted when export starts
     * @param fileName Output file name
     */
    void exportStarted(const QString& fileName);

    /**
     * @brief Emitted when export progress updates
     * @param percentage Progress percentage (0-100)
     * @param status Current status message
     */
    void exportProgress(int percentage, const QString& status);

    /**
     * @brief Emitted when export completes
     * @param success true if successful
     * @param fileName Output file name
     */
    void exportCompleted(bool success, const QString& fileName);

    /**
     * @brief Emitted when export error occurs
     * @param error Error message
     */
    void exportError(const QString& error);

private slots:
    void onWorkerProgress(int percentage, const QString& status);
    void onWorkerCompleted(bool success, const QString& fileName);
    void onWorkerError(const QString& error);

private:
    /**
     * @brief Create default configuration for format
     * @param format Export format
     * @return Default configuration
     */
    ExportConfiguration createDefaultConfiguration(ExportFormat format) const;

    /**
     * @brief Generate unique file name
     * @param format Export format
     * @param template_ Export template
     * @return Unique file name
     */
    QString generateFileName(ExportFormat format, ExportTemplate template_) const;

    /**
     * @brief Get file extension for format
     * @param format Export format
     * @return File extension (with dot)
     */
    QString getFileExtension(ExportFormat format) const;

    /**
     * @brief Validate export configuration
     * @param config Configuration to validate
     * @return true if valid
     */
    bool validateConfiguration(const ExportConfiguration& config) const;

    /**
     * @brief Setup export worker thread
     * @param config Export configuration
     */
    void setupExportWorker(const ExportConfiguration& config);

    /**
     * @brief Cleanup export worker
     */
    void cleanupExportWorker();

    // Application components
    EtiProcessor* m_etiProcessor;
    ServiceBrowser* m_serviceBrowser;
    PerformanceDashboard* m_dashboard;

    // Export worker and thread
    ExportWorker* m_exportWorker;
    QThread* m_exportThread;
    bool m_exportRunning;

    // Progress tracking
    QProgressDialog* m_progressDialog;

    // Configuration
    QString m_defaultOutputDirectory;
    ExportConfiguration m_lastConfiguration;

    // Settings
    bool m_initialized;
};