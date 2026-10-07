#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QGroupBox>
#include <QProgressBar>
#include <QTimer>
#include <QMutex>
#include <QHash>
#include <QVariantMap>
#include <QDateTime>
#include <memory>

// Forward declarations
// Forward declarations for Modern ETI Core Engine
namespace eti::modern {
    class EnhancedFIGAnalyser;
}

/**
 * @class FigAnalysisWidget
 * @brief Professional FIG (Fast Information Group) Analysis Widget
 * 
 * This widget provides comprehensive FIG analysis capabilities with:
 * - Real-time FIG decoding and display
 * - ETSI compliance validation per FIG type
 * - Professional parameter presentation
 * - Color-coded compliance status
 * - Detailed FIG structure breakdown
 * - Broadcasting industry standards compliance
 */
class FigAnalysisWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief FIG compliance status enumeration
     */
    enum class ComplianceStatus {
        Unknown = 0,
        Compliant = 1,
        Warning = 2,
        NonCompliant = 3,
        Error = 4
    };

    /**
     * @brief FIG analysis result structure
     */
    struct FigAnalysis {
        quint8 figType;
        quint8 figExtension;
        QString figName;
        QString description;
        QVariantMap figData;
        ComplianceStatus complianceStatus;
        QStringList complianceIssues;
        QStringList recommendations;
        QDateTime lastUpdate;
        double completeness; // 0.0-100.0%
        
        FigAnalysis() : figType(0), figExtension(0), 
                       complianceStatus(ComplianceStatus::Unknown), 
                       completeness(0.0) {}
    };

    /**
     * @brief FIG parameter information
     */
    struct FigParameter {
        QString name;
        QString value;
        QString description;
        QString unit;
        ComplianceStatus status;
        QString expectedValue;
        QString actualValue;
        
        FigParameter() : status(ComplianceStatus::Unknown) {}
    };

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit FigAnalysisWidget(QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~FigAnalysisWidget();

    /**
     * @brief Initialize widget with ETISnoop wrapper
     * @param wrapper ETISnoop wrapper instance
     * @return true if successful
     */
    bool initialize(eti::modern::EnhancedFIGAnalyser *analyser);

    /**
     * @brief Update FIG analysis data
     * @param figData FIG analysis information
     */
    void updateFigData(const FigAnalysis& figData);

    /**
     * @brief Display FIG types present in stream
     * @param figTypes List of detected FIG types
     */
    void displayFigTypes(const QList<quint8>& figTypes);

    /**
     * @brief Show detailed FIG information
     * @param figType FIG type to display
     * @param figData FIG data content
     */
    void showFigDetails(quint8 figType, const QVariantMap& figData);

    /**
     * @brief Validate ETSI compliance for FIG data
     * @param figData FIG analysis information
     */
    void validateETSICompliance(const FigAnalysis& figData);

    /**
     * @brief Enhanced methods for ETSI 300 799 compliance
     */
    void validateEtsi300799Compliance();
    void highlightDabPlusServices();
    void displayAudioCodecInformation();
    void setupComplianceValidation();

    /**
     * @brief Get current FIG analysis for type
     * @param figType FIG type
     * @return FIG analysis data
     */
    FigAnalysis getFigAnalysis(quint8 figType) const;

    /**
     * @brief Get all detected FIG types
     * @return List of FIG types
     */
    QList<quint8> getDetectedFigTypes() const;

    /**
     * @brief Clear all FIG analysis data
     */
    void clearAnalysis();

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
     * @brief Get overall FIG compliance score
     * @return Compliance score (0.0-100.0)
     */
    double getOverallComplianceScore() const;

    /**
     * @brief Get FIG analysis statistics
     * @return Statistics map
     */
    QVariantMap getFigStatistics() const;

signals:
    /**
     * @brief Emitted when FIG selection changes
     * @param figType Selected FIG type
     */
    void figSelectionChanged(quint8 figType);

    /**
     * @brief Emitted when compliance status changes
     * @param figType FIG type
     * @param status New compliance status
     */
    void complianceStatusChanged(quint8 figType, ComplianceStatus status);

    /**
     * @brief Emitted when FIG analysis updates
     * @param figType FIG type
     * @param analysis Updated analysis
     */
    void figAnalysisUpdated(quint8 figType, const FigAnalysis& analysis);

    /**
     * @brief Emitted when ETSI compliance issue detected
     * @param figType FIG type
     * @param issue Issue description
     * @param severity Issue severity
     */
    void etsiComplianceIssue(quint8 figType, const QString& issue, const QString& severity);

    /**
     * @brief Enhanced signals for ETSI 300 799 compliance
     */
    void complianceUpdated(int level);
    void dabPlusServiceDetected(const QString& serviceId);

private slots:
    /**
     * @brief Handle ETISnoop wrapper FIG signals
     */
    void onFigDataReceived(const QVariantMap& figData);
    void onEtsiComplianceIssue(const QString& standard, const QString& issue, const QString& severity);

    /**
     * @brief Handle FIG selection in tree
     */
    void handleFigSelectionChanged();

    /**
     * @brief Handle FIG type filter change
     */
    void handleFigTypeFilterChanged();

    /**
     * @brief Update FIG display
     */
    void updateFigDisplay();

    /**
     * @brief Refresh compliance analysis
     */
    void refreshComplianceAnalysis();

private:
    /**
     * @brief Create user interface
     */
    void createUI();

    /**
     * @brief Create FIG overview panel
     */
    void createFigOverviewPanel();

    /**
     * @brief Create FIG details panel
     */
    void createFigDetailsPanel();

    /**
     * @brief Create compliance panel
     */
    void createCompliancePanel();

    /**
     * @brief Create FIG tree structure
     */
    void createFigTree();

    /**
     * @brief Add FIG to tree
     * @param figAnalysis FIG analysis data
     * @return Tree widget item
     */
    QTreeWidgetItem* addFigToTree(const FigAnalysis& figAnalysis);

    /**
     * @brief Update FIG tree item
     * @param item Tree widget item
     * @param figAnalysis FIG analysis data
     */
    void updateFigTreeItem(QTreeWidgetItem* item, const FigAnalysis& figAnalysis);

    /**
     * @brief Add FIG parameters to table
     * @param figType FIG type
     * @param parameters FIG parameters
     */
    void addFigParametersToTable(quint8 figType, const QList<FigParameter>& parameters);

    /**
     * @brief Parse FIG data to parameters
     * @param figType FIG type
     * @param figData Raw FIG data
     * @return List of parsed parameters
     */
    QList<FigParameter> parseFigData(quint8 figType, const QVariantMap& figData) const;

    /**
     * @brief Validate FIG compliance
     * @param figType FIG type
     * @param parameters FIG parameters
     * @return Compliance status
     */
    ComplianceStatus validateFigCompliance(quint8 figType, const QList<FigParameter>& parameters) const;

    /**
     * @brief Get FIG type name
     * @param figType FIG type
     * @return Human-readable FIG name
     */
    QString getFigTypeName(quint8 figType) const;

    /**
     * @brief Get FIG type description
     * @param figType FIG type
     * @return FIG description
     */
    QString getFigTypeDescription(quint8 figType) const;

    /**
     * @brief Get compliance color
     * @param status Compliance status
     * @return Color for display
     */
    QColor getComplianceColor(ComplianceStatus status) const;

    /**
     * @brief Get compliance icon
     * @param status Compliance status
     * @return Icon for display
     */
    QIcon getComplianceIcon(ComplianceStatus status) const;

    /**
     * @brief Format compliance percentage
     * @param score Compliance score (0.0-100.0)
     * @return Formatted percentage string
     */
    QString formatComplianceScore(double score) const;

    /**
     * @brief Update compliance summary
     */
    void updateComplianceSummary();

    /**
     * @brief Calculate overall compliance
     * @return Overall compliance score
     */
    double calculateOverallCompliance() const;

    /**
     * @brief Validate widget state
     * @return true if state is valid
     */
    bool validateState() const;

    /**
     * @brief ETSI 300 799 specific validation methods
     */
    void addColorCodedIndicators();
    QColor getComplianceColor(int level);
    
    // ETSI 300 799 specific validation
    bool validateFig00Services();
    bool validateFig01Subchannels();
    bool validateFig02ServiceOrg();

    // Core components
    eti::modern::EnhancedFIGAnalyser *m_figAnalyser;

    // Main layout
    QVBoxLayout *m_mainLayout;
    QTabWidget *m_mainTabs;

    // FIG Overview tab
    QWidget *m_overviewTab;
    QVBoxLayout *m_overviewLayout;
    QTreeWidget *m_figTree;
    QLabel *m_figCountLabel;
    QLabel *m_complianceScoreLabel;
    QProgressBar *m_complianceProgress;

    // FIG Details tab
    QWidget *m_detailsTab;
    QVBoxLayout *m_detailsLayout;
    QComboBox *m_figTypeCombo;
    QTableWidget *m_parametersTable;
    QTextEdit *m_figDescriptionText;

    // Compliance tab
    QWidget *m_complianceTab;
    QVBoxLayout *m_complianceLayout;
    QTreeWidget *m_complianceTree;
    QTextEdit *m_complianceIssuesText;
    QTextEdit *m_recommendationsText;

    // Control panel
    QGroupBox *m_controlGroup;
    QHBoxLayout *m_controlLayout;
    QPushButton *m_refreshButton;
    QPushButton *m_clearButton;
    QPushButton *m_exportButton;
    QComboBox *m_filterCombo;

    // Data storage
    QHash<quint8, FigAnalysis> m_figAnalyses;
    QList<quint8> m_detectedFigTypes;
    quint8 m_selectedFigType;

    // Update system
    QTimer *m_updateTimer;
    bool m_realTimeEnabled;
    mutable QMutex m_dataMutex;

    // Statistics
    int m_totalFigTypes;
    int m_compliantFigTypes;
    double m_overallComplianceScore;

    // Constants
    static constexpr int UPDATE_INTERVAL_MS = 2000;  // 2 second updates
    static constexpr int MAX_FIG_HISTORY = 1000;     // Keep last 1000 FIG updates
};

Q_DECLARE_METATYPE(FigAnalysisWidget::ComplianceStatus)
Q_DECLARE_METATYPE(FigAnalysisWidget::FigAnalysis)
Q_DECLARE_METATYPE(FigAnalysisWidget::FigParameter)