#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTextEdit>
#include <QFrame>
#include <QPushButton>
#include <QComboBox>
#include <QMutex>
#include <QDateTime>
#include <QMap>
#include <QColor>
#include <QFont>

// Forward declarations
class EtiProcessor;  // Qt camelCase for GUI layer

/**
 * @struct ETSIViolation
 * @brief ETSI compliance violation structure
 */
struct ETSIViolation {
    enum Severity {
        Info = 0,
        Warning = 1,
        Error = 2,
        Critical = 3
    };
    
    enum Category {
        FIG_Structure = 0,
        Service_Organization = 1,
        Audio_Quality = 2,
        Data_Integrity = 3,
        Timing_Compliance = 4,
        Thai_Standards = 5
    };
    
    QString violationId;
    Severity severity;
    Category category;
    QString description;
    QString standardReference;  // e.g., "ETSI EN 300 401 Section 8.1.1"
    QDateTime timestamp;
    QString frameContext;       // Frame number or context
    bool isActive;
    
    ETSIViolation() : severity(Info), category(FIG_Structure), timestamp(QDateTime::currentDateTime()), isActive(true) {}
    
    QString getSeverityString() const {
        switch (severity) {
            case Info: return "INFO";
            case Warning: return "WARNING";
            case Error: return "ERROR";
            case Critical: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }
    
    QString getCategoryString() const {
        switch (category) {
            case FIG_Structure: return "FIG Structure";
            case Service_Organization: return "Service Organization";
            case Audio_Quality: return "Audio Quality";
            case Data_Integrity: return "Data Integrity";
            case Timing_Compliance: return "Timing Compliance";
            case Thai_Standards: return "Thai Standards";
            default: return "Unknown";
        }
    }
};

/**
 * @struct ComplianceStatistics
 * @brief ETSI compliance statistics structure
 */
struct ComplianceStatistics {
    int totalViolations;
    int criticalViolations;
    int errorViolations;
    int warningViolations;
    int infoViolations;
    double overallScore;        // 0-100%
    QDateTime lastUpdate;
    
    ComplianceStatistics() : totalViolations(0), criticalViolations(0), errorViolations(0),
                           warningViolations(0), infoViolations(0), overallScore(100.0),
                           lastUpdate(QDateTime::currentDateTime()) {}
};

/**
 * @class ETSIComplianceMonitor
 * @brief Professional ETSI compliance monitoring widget for broadcast industry
 * 
 * This widget provides comprehensive real-time ETSI compliance monitoring
 * following broadcast industry standards:
 * - Real-time violation detection and categorization
 * - ETSI EN 300 401/799 standard compliance validation
 * - Thai DAB standards compliance monitoring
 * - Professional broadcast industry alert management
 * - Compliance trend analysis and reporting
 * - Violation severity classification and handling
 */
class ETSIComplianceMonitor : public QWidget
{
    Q_OBJECT
    
public:
    explicit ETSIComplianceMonitor(QWidget *parent = nullptr);
    ~ETSIComplianceMonitor();
    
    /**
     * @brief Connect to ETI processor for compliance data
     * @param processor ETI processor instance
     */
    void connectEtiProcessor(EtiProcessor* processor);
    
    /**
     * @brief Start real-time compliance monitoring
     */
    void startMonitoring();
    
    /**
     * @brief Stop real-time compliance monitoring
     */
    void stopMonitoring();
    
    /**
     * @brief Check if monitoring is active
     * @return true if monitoring is running
     */
    bool isMonitoring() const { return m_monitoringActive; }
    
    /**
     * @brief Get current compliance statistics
     * @return Current compliance statistics
     */
    ComplianceStatistics getCurrentStatistics() const { return m_statistics; }
    
    /**
     * @brief Get overall compliance score
     * @return Compliance score (0-100%)
     */
    double getComplianceScore() const { return m_statistics.overallScore; }
    
    /**
     * @brief Apply professional broadcasting theme
     */
    void applyBroadcastingTheme();
    
public slots:
    /**
     * @brief Add new ETSI violation
     * @param violation Violation information
     */
    void addViolation(const ETSIViolation& violation);
    
    /**
     * @brief Update violation status
     * @param violationId Violation ID
     * @param isActive Activity status
     */
    void updateViolationStatus(const QString& violationId, bool isActive);
    
    /**
     * @brief Clear all violations
     */
    void clearViolations();
    
    /**
     * @brief Update compliance statistics
     */
    void updateStatistics();
    
    /**
     * @brief Filter violations by severity
     * @param severity Minimum severity level to show
     */
    void filterBySeverity(ETSIViolation::Severity severity);
    
    /**
     * @brief Filter violations by category
     * @param category Category to filter by
     */
    void filterByCategory(ETSIViolation::Category category);
    
    /**
     * @brief Export violations to file
     * @param filename Output filename
     * @return true if export successful
     */
    bool exportViolations(const QString& filename);
    
signals:
    /**
     * @brief Emitted when critical violation occurs
     * @param violation Violation information
     */
    void criticalViolationDetected(const ETSIViolation& violation);
    
    /**
     * @brief Emitted when compliance score changes significantly
     * @param newScore New compliance score
     * @param oldScore Previous compliance score
     */
    void complianceScoreChanged(double newScore, double oldScore);
    
    /**
     * @brief Emitted when violation is selected
     * @param violation Selected violation
     */
    void violationSelected(const ETSIViolation& violation);
    
private slots:
    /**
     * @brief Handle violation tree item selection
     * @param item Selected item
     * @param column Selected column
     */
    void onViolationSelected(QTreeWidgetItem* item, int column);
    
    /**
     * @brief Handle clear violations button
     */
    void onClearViolations();
    
    /**
     * @brief Handle export violations button
     */
    void onExportViolations();
    
    /**
     * @brief Handle severity filter change
     * @param severityIndex Severity filter index
     */
    void onSeverityFilterChanged(int severityIndex);
    
    /**
     * @brief Handle category filter change
     * @param categoryIndex Category filter index
     */
    void onCategoryFilterChanged(int categoryIndex);
    
    /**
     * @brief Handle ETI processor compliance update
     * @param frameNumber Frame number
     * @param violations List of violations in frame
     */
    void onEtiComplianceUpdate(quint64 frameNumber, const QList<ETSIViolation>& violations);
    
private:
    /**
     * @brief Setup compliance monitor UI
     */
    void setupUI();
    
    /**
     * @brief Setup compliance overview panel
     */
    void setupComplianceOverview();
    
    /**
     * @brief Setup violations tree
     */
    void setupViolationsTree();
    
    /**
     * @brief Setup control panel
     */
    void setupControlPanel();
    
    /**
     * @brief Setup violation details panel
     */
    void setupViolationDetails();
    
    /**
     * @brief Update compliance overview display
     */
    void updateComplianceOverview();
    
    /**
     * @brief Update violations tree display
     */
    void updateViolationsTree();
    
    /**
     * @brief Calculate compliance score based on violations
     * @return Compliance score (0-100%)
     */
    double calculateComplianceScore() const;
    
    /**
     * @brief Get severity color for UI display
     * @param severity Violation severity
     * @return Color for severity level
     */
    QColor getSeverityColor(ETSIViolation::Severity severity) const;
    
    /**
     * @brief Get category icon for UI display
     * @param category Violation category
     * @return Icon for category
     */
    QIcon getCategoryIcon(ETSIViolation::Category category) const;
    
    /**
     * @brief Create violation tree item
     * @param violation Violation information
     * @return Tree widget item
     */
    QTreeWidgetItem* createViolationItem(const ETSIViolation& violation);
    
    /**
     * @brief Apply filters to violations tree
     */
    void applyFilters();
    
    /**
     * @brief Validate violation before adding
     * @param violation Violation to validate
     * @return true if violation is valid
     */
    bool validateViolation(const ETSIViolation& violation) const;
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_overviewLayout;
    QHBoxLayout* m_controlLayout;
    
    // Compliance overview
    QFrame* m_overviewFrame;
    QLabel* m_scoreLabel;
    QLabel* m_scoreValueLabel;
    QProgressBar* m_scoreProgressBar;
    QLabel* m_statusLabel;
    
    // Statistics display
    QFrame* m_statisticsFrame;
    QLabel* m_totalViolationsLabel;
    QLabel* m_criticalCountLabel;
    QLabel* m_errorCountLabel;
    QLabel* m_warningCountLabel;
    QLabel* m_infoCountLabel;
    
    // Violations tree
    QTreeWidget* m_violationsTree;
    
    // Control panel
    QComboBox* m_severityFilter;
    QComboBox* m_categoryFilter;
    QPushButton* m_clearButton;
    QPushButton* m_exportButton;
    QPushButton* m_refreshButton;
    
    // Violation details
    QTextEdit* m_violationDetails;
    
    // Data and state
    EtiProcessor* m_etiProcessor;
    bool m_monitoringActive;
    
    QMap<QString, ETSIViolation> m_violations;
    QMap<QString, QTreeWidgetItem*> m_violationItems;
    ComplianceStatistics m_statistics;
    // GUI-thread only: every lock site (addViolation/updateViolationStatus/
    // clearViolations/updateStatistics/getCurrentStatistics) runs on the Qt
    // main thread; the mutex is retained as a plain guard, not for worker
    // threads, so it is deliberately non-mutable (no const method locks it).
    QMutex m_dataMutex;
    
    // Filter state
    ETSIViolation::Severity m_selectedSeverity;
    ETSIViolation::Category m_selectedCategory;
    bool m_showAllCategories;
    bool m_showAllSeverities;
    
    // Professional styling
    QFont m_headerFont;
    QFont m_valueFont;
    QFont m_detailFont;
    
    // Colors for broadcast industry theme
    static const QColor BACKGROUND_DARK;     // #2D2D30
    static const QColor BACKGROUND_MEDIUM;   // #3E3E42
    static const QColor ACCENT_BLUE;         // #0078D4
    static const QColor SEVERITY_INFO;       // #17A2B8 (Cyan)
    static const QColor SEVERITY_WARNING;    // #FFC107 (Amber)
    static const QColor SEVERITY_ERROR;      // #DC3545 (Red)
    static const QColor SEVERITY_CRITICAL;   // #6F42C1 (Purple)
    static const QColor STATUS_EXCELLENT;    // #28A745 (Green)
    static const QColor STATUS_GOOD;         // #20C997 (Teal)
    static const QColor STATUS_FAIR;         // #FD7E14 (Orange)
    static const QColor STATUS_POOR;         // #E83E8C (Pink)
    static const QColor TEXT_PRIMARY;        // #FFFFFF
    static const QColor TEXT_SECONDARY;      // #6C757D
    
    // Constants
    static constexpr int UPDATE_INTERVAL_MS = 2000;  // 2 second updates
    static constexpr int MAX_VIOLATIONS_DISPLAYED = 1000;
    static constexpr double CRITICAL_COMPLIANCE_THRESHOLD = 85.0;
    static constexpr double WARNING_COMPLIANCE_THRESHOLD = 95.0;
};