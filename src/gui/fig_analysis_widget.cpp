/**
 * @file fig_analysis_widget.cpp
 * @brief Professional FIG Analysis Widget implementation
 * 
 * This file implements the FigAnalysisWidget class providing comprehensive
 * FIG analysis capabilities with ETSI compliance validation.
 */

#include "fig_analysis_widget.h"
#include "utils/logger.h"

#include <QColor>
#include <QBrush>
#include <QFont>
#include <QIcon>
#include <QDateTime>
#include <QTimer>
#include <QMutexLocker>
#include <QHeaderView>
#include <QDebug>
#include <QApplication>
#include <QStyle>
#include <QSplitter>

FigAnalysisWidget::FigAnalysisWidget(QWidget *parent)
    : QWidget(parent)
    , m_figAnalyser(nullptr)
    , m_mainLayout(nullptr)
    , m_mainTabs(nullptr)
    , m_overviewTab(nullptr)
    , m_overviewLayout(nullptr)
    , m_figTree(nullptr)
    , m_figCountLabel(nullptr)
    , m_complianceScoreLabel(nullptr)
    , m_complianceProgress(nullptr)
    , m_detailsTab(nullptr)
    , m_detailsLayout(nullptr)
    , m_figTypeCombo(nullptr)
    , m_parametersTable(nullptr)
    , m_figDescriptionText(nullptr)
    , m_complianceTab(nullptr)
    , m_complianceLayout(nullptr)
    , m_complianceTree(nullptr)
    , m_complianceIssuesText(nullptr)
    , m_recommendationsText(nullptr)
    , m_controlGroup(nullptr)
    , m_controlLayout(nullptr)
    , m_refreshButton(nullptr)
    , m_clearButton(nullptr)
    , m_exportButton(nullptr)
    , m_filterCombo(nullptr)
    , m_selectedFigType(0)
    , m_updateTimer(nullptr)
    , m_realTimeEnabled(false)
    , m_totalFigTypes(0)
    , m_compliantFigTypes(0)
    , m_overallComplianceScore(0.0)
{
    // Set widget properties
    setObjectName("FigAnalysisWidget");
    setMinimumSize(300, 400);
    
    // Create UI
    createUI();
    
    // Setup update timer
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &FigAnalysisWidget::updateFigDisplay);
    
    Logger::instance().log(Logger::Info, "FigAnalysisWidget", "Constructor completed");
}

FigAnalysisWidget::~FigAnalysisWidget()
{
    Logger::instance().log(Logger::Info, "FigAnalysisWidget", "Destructor starting");
    
    // Stop updates
    setRealTimeEnabled(false);
    
    // Cleanup timer
    if (m_updateTimer) {
        m_updateTimer->stop();
        delete m_updateTimer;
        m_updateTimer = nullptr;
    }
    
    Logger::instance().log(Logger::Info, "FigAnalysisWidget", "Destructor completed");
}

bool FigAnalysisWidget::initialize(eti::modern::EnhancedFIGAnalyser *analyser)
{
    if (!analyser) {
        Logger::instance().log(Logger::Error, "FigAnalysisWidget", "Invalid FIG analyser provided");
        return false;
    }
    
    Logger::instance().log(Logger::Info, "FigAnalysisWidget", "Initializing with Enhanced FIG Analyser");
    
    m_figAnalyser = analyser;
    
    // Connect Modern ETI Core analyser signals
    // Note: These connections would need to be adapted based on actual analyser signals
    // connect(m_figAnalyser, &eti::modern::EnhancedFIGAnalyser::complianceIssue,
    //         this, &FigAnalysisWidget::onEtsiComplianceIssue);
    
    // Enable real-time updates by default
    setRealTimeEnabled(true);
    
    Logger::instance().log(Logger::Info, "FigAnalysisWidget", "Initialization completed successfully");
    return true;
}

void FigAnalysisWidget::updateFigData(const FigAnalysis& figData)
{
    QMutexLocker locker(&m_dataMutex);
    
    Logger::instance().log(Logger::Debug, "FigAnalysisWidget", 
                          QString("Updating FIG data: Type %1 - %2")
                          .arg(figData.figType)
                          .arg(figData.figName));
    
    // Store FIG analysis
    m_figAnalyses[figData.figType] = figData;
    
    // Update detected FIG types list
    if (!m_detectedFigTypes.contains(figData.figType)) {
        m_detectedFigTypes.append(figData.figType);
        std::sort(m_detectedFigTypes.begin(), m_detectedFigTypes.end());
        
        // Update FIG type combo
        m_figTypeCombo->addItem(QString("FIG %1 - %2").arg(figData.figType).arg(figData.figName), 
                               figData.figType);
    }
    
    // Update FIG tree
    bool foundItem = false;
    for (int i = 0; i < m_figTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_figTree->topLevelItem(i);
        if (item && item->data(0, Qt::UserRole).toUInt() == figData.figType) {
            updateFigTreeItem(item, figData);
            foundItem = true;
            break;
        }
    }
    
    if (!foundItem) {
        addFigToTree(figData);
    }
    
    // Update statistics
    updateComplianceSummary();
    
    // Emit signals
    emit figAnalysisUpdated(figData.figType, figData);
    
    if (figData.complianceStatus == ComplianceStatus::NonCompliant || 
        figData.complianceStatus == ComplianceStatus::Error) {
        for (const QString& issue : figData.complianceIssues) {
            emit etsiComplianceIssue(figData.figType, issue, "Error");
        }
    }
}

void FigAnalysisWidget::displayFigTypes(const QList<quint8>& figTypes)
{
    QMutexLocker locker(&m_dataMutex);
    
    Logger::instance().log(Logger::Info, "FigAnalysisWidget", 
                          QString("Displaying %1 FIG types").arg(figTypes.size()));
    
    m_detectedFigTypes = figTypes;
    
    // Update FIG type combo
    m_figTypeCombo->clear();
    m_figTypeCombo->addItem(tr("All FIG Types"), 255); // Special value for "all"
    
    for (quint8 figType : figTypes) {
        QString figName = getFigTypeName(figType);
        m_figTypeCombo->addItem(QString("FIG %1 - %2").arg(figType).arg(figName), figType);
    }
    
    // Update statistics
    m_totalFigTypes = figTypes.size();
    updateComplianceSummary();
}

void FigAnalysisWidget::showFigDetails(quint8 figType, const QVariantMap& figData)
{
    Logger::instance().log(Logger::Debug, "FigAnalysisWidget", 
                          QString("Showing FIG %1 details").arg(figType));
    
    m_selectedFigType = figType;
    
    // Update FIG type combo selection
    for (int i = 0; i < m_figTypeCombo->count(); ++i) {
        if (m_figTypeCombo->itemData(i).toUInt() == figType) {
            m_figTypeCombo->setCurrentIndex(i);
            break;
        }
    }
    
    // Parse FIG data and display parameters
    QList<FigParameter> parameters = parseFigData(figType, figData);
    addFigParametersToTable(figType, parameters);
    
    // Update description
    QString description = getFigTypeDescription(figType);
    m_figDescriptionText->setHtml(QString("<h3>FIG %1 - %2</h3><p>%3</p>")
                                 .arg(figType)
                                 .arg(getFigTypeName(figType))
                                 .arg(description));
    
    // Switch to details tab
    m_mainTabs->setCurrentWidget(m_detailsTab);
    
    emit figSelectionChanged(figType);
}

void FigAnalysisWidget::validateETSICompliance(const FigAnalysis& figData)
{
    Logger::instance().log(Logger::Debug, "FigAnalysisWidget", 
                          QString("Validating ETSI compliance for FIG %1").arg(figData.figType));
    
    // Update compliance tree
    QTreeWidgetItem* figItem = nullptr;
    
    // Find or create FIG item in compliance tree
    for (int i = 0; i < m_complianceTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_complianceTree->topLevelItem(i);
        if (item && item->data(0, Qt::UserRole).toUInt() == figData.figType) {
            figItem = item;
            break;
        }
    }
    
    if (!figItem) {
        figItem = new QTreeWidgetItem(m_complianceTree);
        figItem->setText(0, QString("FIG %1").arg(figData.figType));
        figItem->setText(1, getFigTypeName(figData.figType));
        figItem->setData(0, Qt::UserRole, figData.figType);
    }
    
    // Update compliance status
    figItem->setText(2, formatComplianceScore(figData.completeness));
    figItem->setIcon(0, getComplianceIcon(figData.complianceStatus));
    
    QColor statusColor = getComplianceColor(figData.complianceStatus);
    figItem->setBackground(2, QBrush(statusColor.lighter(180)));
    
    // Clear and add compliance issues
    figItem->takeChildren();
    
    for (const QString& issue : figData.complianceIssues) {
        QTreeWidgetItem* issueItem = new QTreeWidgetItem(figItem);
        issueItem->setText(0, "Issue");
        issueItem->setText(1, issue);
        issueItem->setText(2, "Non-Compliant");
        issueItem->setIcon(0, getComplianceIcon(ComplianceStatus::NonCompliant));
    }
    
    for (const QString& recommendation : figData.recommendations) {
        QTreeWidgetItem* recItem = new QTreeWidgetItem(figItem);
        recItem->setText(0, "Recommendation");
        recItem->setText(1, recommendation);
        recItem->setText(2, "Advisory");
        recItem->setIcon(0, getComplianceIcon(ComplianceStatus::Warning));
    }
    
    // Expand compliance tree
    m_complianceTree->expandAll();
    
    // Update compliance issues text
    if (!figData.complianceIssues.isEmpty()) {
        QString issuesText = QString("<h3>FIG %1 Compliance Issues</h3><ul>").arg(figData.figType);
        for (const QString& issue : figData.complianceIssues) {
            issuesText += QString("<li>%1</li>").arg(issue);
        }
        issuesText += "</ul>";
        m_complianceIssuesText->setHtml(issuesText);
    }
    
    // Update recommendations text
    if (!figData.recommendations.isEmpty()) {
        QString recText = QString("<h3>FIG %1 Recommendations</h3><ul>").arg(figData.figType);
        for (const QString& rec : figData.recommendations) {
            recText += QString("<li>%1</li>").arg(rec);
        }
        recText += "</ul>";
        m_recommendationsText->setHtml(recText);
    }
    
    emit complianceStatusChanged(figData.figType, figData.complianceStatus);
}

FigAnalysisWidget::FigAnalysis FigAnalysisWidget::getFigAnalysis(quint8 figType) const
{
    QMutexLocker locker(&m_dataMutex);
    return m_figAnalyses.value(figType, FigAnalysis());
}

QList<quint8> FigAnalysisWidget::getDetectedFigTypes() const
{
    QMutexLocker locker(&m_dataMutex);
    return m_detectedFigTypes;
}

void FigAnalysisWidget::clearAnalysis()
{
    QMutexLocker locker(&m_dataMutex);
    
    Logger::instance().log(Logger::Info, "FigAnalysisWidget", "Clearing all FIG analysis data");
    
    // Clear data structures
    m_figAnalyses.clear();
    m_detectedFigTypes.clear();
    m_selectedFigType = 0;
    
    // Clear UI components
    m_figTree->clear();
    m_figTypeCombo->clear();
    m_parametersTable->setRowCount(0);
    m_figDescriptionText->clear();
    m_complianceTree->clear();
    m_complianceIssuesText->clear();
    m_recommendationsText->clear();
    
    // Reset statistics
    m_totalFigTypes = 0;
    m_compliantFigTypes = 0;
    m_overallComplianceScore = 0.0;
    
    updateComplianceSummary();
}

void FigAnalysisWidget::setRealTimeEnabled(bool enabled)
{
    if (m_realTimeEnabled == enabled) {
        return;
    }
    
    m_realTimeEnabled = enabled;
    
    if (m_updateTimer) {
        if (enabled) {
            m_updateTimer->start(UPDATE_INTERVAL_MS);
            Logger::instance().log(Logger::Info, "FigAnalysisWidget", "Real-time updates enabled");
        } else {
            m_updateTimer->stop();
            Logger::instance().log(Logger::Info, "FigAnalysisWidget", "Real-time updates disabled");
        }
    }
}

double FigAnalysisWidget::getOverallComplianceScore() const
{
    QMutexLocker locker(&m_dataMutex);
    return m_overallComplianceScore;
}

QVariantMap FigAnalysisWidget::getFigStatistics() const
{
    QMutexLocker locker(&m_dataMutex);
    
    QVariantMap stats;
    stats["totalFigTypes"] = m_totalFigTypes;
    stats["compliantFigTypes"] = m_compliantFigTypes;
    stats["overallComplianceScore"] = m_overallComplianceScore;
    stats["detectedFigTypes"] = QVariant::fromValue(m_detectedFigTypes);
    
    return stats;
}

void FigAnalysisWidget::onFigDataReceived(const QVariantMap& figData)
{
    // Convert QVariantMap to FigAnalysis structure
    FigAnalysis analysis;
    analysis.figType = figData.value("figType", 0).toUInt();
    analysis.figExtension = figData.value("figExtension", 0).toUInt();
    analysis.figName = getFigTypeName(analysis.figType);
    analysis.description = figData.value("description", "").toString();
    analysis.figData = figData;
    analysis.lastUpdate = QDateTime::currentDateTime();
    analysis.completeness = figData.value("completeness", 0.0).toDouble();
    
    // Parse compliance issues from figData
    QVariantList issues = figData.value("complianceIssues").toList();
    for (const QVariant& issue : issues) {
        analysis.complianceIssues.append(issue.toString());
    }
    
    // Parse recommendations
    QVariantList recs = figData.value("recommendations").toList();
    for (const QVariant& rec : recs) {
        analysis.recommendations.append(rec.toString());
    }
    
    // Determine compliance status
    if (analysis.complianceIssues.isEmpty()) {
        analysis.complianceStatus = ComplianceStatus::Compliant;
    } else if (analysis.complianceIssues.size() <= 2) {
        analysis.complianceStatus = ComplianceStatus::Warning;
    } else {
        analysis.complianceStatus = ComplianceStatus::NonCompliant;
    }
    
    updateFigData(analysis);
}

void FigAnalysisWidget::onEtsiComplianceIssue(const QString& standard, const QString& issue, const QString& severity)
{
    Logger::instance().log(Logger::Warning, "FigAnalysisWidget", 
                          QString("ETSI compliance issue - %1: %2 (Severity: %3)")
                          .arg(standard).arg(issue).arg(severity));
    
    // Add to compliance issues display
    QString issueHtml = QString("<p><b>%1:</b> %2 <i>(%3)</i></p>")
                       .arg(standard).arg(issue).arg(severity);
    m_complianceIssuesText->append(issueHtml);
}

void FigAnalysisWidget::handleFigSelectionChanged()
{
    QTreeWidgetItem* currentItem = m_figTree->currentItem();
    if (!currentItem) {
        return;
    }
    
    quint8 figType = currentItem->data(0, Qt::UserRole).toUInt();
    if (figType > 0 && figType != m_selectedFigType) {
        m_selectedFigType = figType;
        
        // Update details for selected FIG
        if (m_figAnalyses.contains(figType)) {
            FigAnalysis analysis = m_figAnalyses[figType];
            showFigDetails(figType, analysis.figData);
        }
        
        emit figSelectionChanged(figType);
    }
}

void FigAnalysisWidget::handleFigTypeFilterChanged()
{
    quint8 selectedType = m_figTypeCombo->currentData().toUInt();
    
    if (selectedType == 255) {
        // Show all FIG types
        for (int i = 0; i < m_figTree->topLevelItemCount(); ++i) {
            m_figTree->topLevelItem(i)->setHidden(false);
        }
    } else {
        // Show only selected FIG type
        for (int i = 0; i < m_figTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem* item = m_figTree->topLevelItem(i);
            quint8 itemType = item->data(0, Qt::UserRole).toUInt();
            item->setHidden(itemType != selectedType);
        }
    }
}

void FigAnalysisWidget::updateFigDisplay()
{
    // Periodic update of FIG display
    updateComplianceSummary();
}

void FigAnalysisWidget::refreshComplianceAnalysis()
{
    Logger::instance().log(Logger::Info, "FigAnalysisWidget", "Refreshing compliance analysis");
    
    // Recalculate compliance for all FIG types
    for (auto it = m_figAnalyses.begin(); it != m_figAnalyses.end(); ++it) {
        validateETSICompliance(it.value());
    }
    
    updateComplianceSummary();
}

void FigAnalysisWidget::createUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(6, 6, 6, 6);
    m_mainLayout->setSpacing(6);
    
    // Create control panel
    createFigOverviewPanel();
    
    // Create main tabs
    m_mainTabs = new QTabWidget();
    m_mainTabs->setObjectName("FigAnalysisTabs");
    
    // Create tab panels
    createFigOverviewPanel();
    createFigDetailsPanel();
    createCompliancePanel();
    
    // Add tabs
    m_mainTabs->addTab(m_overviewTab, tr("FIG Overview"));
    m_mainTabs->addTab(m_detailsTab, tr("FIG Details"));
    m_mainTabs->addTab(m_complianceTab, tr("Compliance"));
    
    // Add to main layout
    m_mainLayout->addWidget(m_mainTabs);
    
    // Connect signals
    connect(m_figTree, &QTreeWidget::currentItemChanged, 
            this, &FigAnalysisWidget::handleFigSelectionChanged);
    connect(m_figTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FigAnalysisWidget::handleFigTypeFilterChanged);
}

void FigAnalysisWidget::createFigOverviewPanel()
{
    m_overviewTab = new QWidget();
    m_overviewLayout = new QVBoxLayout(m_overviewTab);
    m_overviewLayout->setContentsMargins(6, 6, 6, 6);
    m_overviewLayout->setSpacing(6);
    
    // Statistics panel
    QGroupBox* statsGroup = new QGroupBox(tr("FIG Statistics"));
    QGridLayout* statsLayout = new QGridLayout(statsGroup);
    
    m_figCountLabel = new QLabel(tr("Detected FIG Types: 0"));
    m_complianceScoreLabel = new QLabel(tr("Overall Compliance: 0%"));
    m_complianceProgress = new QProgressBar();
    m_complianceProgress->setRange(0, 100);
    m_complianceProgress->setValue(0);
    
    statsLayout->addWidget(m_figCountLabel, 0, 0);
    statsLayout->addWidget(m_complianceScoreLabel, 0, 1);
    statsLayout->addWidget(m_complianceProgress, 1, 0, 1, 2);
    
    m_overviewLayout->addWidget(statsGroup);
    
    // FIG tree
    m_figTree = new QTreeWidget();
    m_figTree->setObjectName("FigOverviewTree");
    m_figTree->setHeaderLabels({tr("FIG Type"), tr("Name"), tr("Status"), tr("Compliance")});
    m_figTree->setRootIsDecorated(false);
    m_figTree->setAlternatingRowColors(true);
    m_figTree->setSortingEnabled(true);
    
    // Style the tree
    m_figTree->setStyleSheet(
        "QTreeWidget { "
        "    background-color: #FFFFFF; "
        "    border: 1px solid #CCCCCC; "
        "    border-radius: 3px; "
        "    selection-background-color: #CCE8FF; "
        "} "
        "QTreeWidget::item { "
        "    padding: 4px; "
        "    border-bottom: 1px solid #F0F0F0; "
        "} "
        "QTreeWidget::item:selected { "
        "    background-color: #CCE8FF; "
        "}"
    );
    
    m_overviewLayout->addWidget(m_figTree);
}

void FigAnalysisWidget::createFigDetailsPanel()
{
    m_detailsTab = new QWidget();
    m_detailsLayout = new QVBoxLayout(m_detailsTab);
    m_detailsLayout->setContentsMargins(6, 6, 6, 6);
    m_detailsLayout->setSpacing(6);
    
    // FIG type selector
    QHBoxLayout* selectorLayout = new QHBoxLayout();
    selectorLayout->addWidget(new QLabel(tr("FIG Type:")));
    
    m_figTypeCombo = new QComboBox();
    m_figTypeCombo->setMinimumWidth(200);
    selectorLayout->addWidget(m_figTypeCombo);
    selectorLayout->addStretch();
    
    m_detailsLayout->addLayout(selectorLayout);
    
    // FIG description
    m_figDescriptionText = new QTextEdit();
    m_figDescriptionText->setMaximumHeight(100);
    m_figDescriptionText->setReadOnly(true);
    m_detailsLayout->addWidget(m_figDescriptionText);
    
    // Parameters table
    m_parametersTable = new QTableWidget();
    m_parametersTable->setColumnCount(4);
    m_parametersTable->setHorizontalHeaderLabels({tr("Parameter"), tr("Value"), tr("Expected"), tr("Status")});
    m_parametersTable->horizontalHeader()->setStretchLastSection(true);
    m_parametersTable->verticalHeader()->setVisible(false);
    m_parametersTable->setAlternatingRowColors(true);
    m_parametersTable->setSortingEnabled(true);
    
    m_detailsLayout->addWidget(m_parametersTable);
}

void FigAnalysisWidget::createCompliancePanel()
{
    m_complianceTab = new QWidget();
    m_complianceLayout = new QVBoxLayout(m_complianceTab);
    m_complianceLayout->setContentsMargins(6, 6, 6, 6);
    m_complianceLayout->setSpacing(6);
    
    // Compliance tree
    m_complianceTree = new QTreeWidget();
    m_complianceTree->setHeaderLabels({tr("FIG/Issue"), tr("Description"), tr("Status")});
    m_complianceTree->setRootIsDecorated(true);
    m_complianceTree->setAlternatingRowColors(true);
    
    m_complianceLayout->addWidget(m_complianceTree, 1);
    
    // Split between issues and recommendations
    QSplitter* complianceSplitter = new QSplitter(Qt::Horizontal);
    
    // Compliance issues
    QGroupBox* issuesGroup = new QGroupBox(tr("Compliance Issues"));
    QVBoxLayout* issuesLayout = new QVBoxLayout(issuesGroup);
    m_complianceIssuesText = new QTextEdit();
    m_complianceIssuesText->setReadOnly(true);
    issuesLayout->addWidget(m_complianceIssuesText);
    complianceSplitter->addWidget(issuesGroup);
    
    // Recommendations
    QGroupBox* recGroup = new QGroupBox(tr("Recommendations"));
    QVBoxLayout* recLayout = new QVBoxLayout(recGroup);
    m_recommendationsText = new QTextEdit();
    m_recommendationsText->setReadOnly(true);
    recLayout->addWidget(m_recommendationsText);
    complianceSplitter->addWidget(recGroup);
    
    m_complianceLayout->addWidget(complianceSplitter, 1);
}

QTreeWidgetItem* FigAnalysisWidget::addFigToTree(const FigAnalysis& figAnalysis)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(m_figTree);
    item->setText(0, QString("FIG %1").arg(figAnalysis.figType));
    item->setText(1, figAnalysis.figName);
    item->setData(0, Qt::UserRole, figAnalysis.figType);
    
    updateFigTreeItem(item, figAnalysis);
    
    return item;
}

void FigAnalysisWidget::updateFigTreeItem(QTreeWidgetItem* item, const FigAnalysis& figAnalysis)
{
    if (!item) return;
    
    // Update status
    switch (figAnalysis.complianceStatus) {
    case ComplianceStatus::Compliant:
        item->setText(2, tr("OK"));
        break;
    case ComplianceStatus::Warning:
        item->setText(2, tr("Warning"));
        break;
    case ComplianceStatus::NonCompliant:
        item->setText(2, tr("Non-Compliant"));
        break;
    case ComplianceStatus::Error:
        item->setText(2, tr("Error"));
        break;
    default:
        item->setText(2, tr("Unknown"));
        break;
    }
    
    // Update compliance percentage
    item->setText(3, formatComplianceScore(figAnalysis.completeness));
    
    // Set colors and icons
    item->setIcon(0, getComplianceIcon(figAnalysis.complianceStatus));
    QColor statusColor = getComplianceColor(figAnalysis.complianceStatus);
    item->setBackground(2, QBrush(statusColor.lighter(180)));
    item->setBackground(3, QBrush(statusColor.lighter(190)));
}

void FigAnalysisWidget::addFigParametersToTable(quint8 figType, const QList<FigParameter>& parameters)
{
    Q_UNUSED(figType);
    m_parametersTable->setRowCount(parameters.size());
    
    for (int i = 0; i < parameters.size(); ++i) {
        const FigParameter& param = parameters[i];
        
        m_parametersTable->setItem(i, 0, new QTableWidgetItem(param.name));
        m_parametersTable->setItem(i, 1, new QTableWidgetItem(param.value));
        m_parametersTable->setItem(i, 2, new QTableWidgetItem(param.expectedValue));
        
        QTableWidgetItem* statusItem = new QTableWidgetItem();
        switch (param.status) {
        case ComplianceStatus::Compliant:
            statusItem->setText(tr("OK"));
            statusItem->setBackground(QBrush(getComplianceColor(ComplianceStatus::Compliant).lighter(180)));
            break;
        case ComplianceStatus::Warning:
            statusItem->setText(tr("Warning"));
            statusItem->setBackground(QBrush(getComplianceColor(ComplianceStatus::Warning).lighter(180)));
            break;
        case ComplianceStatus::NonCompliant:
            statusItem->setText(tr("Non-Compliant"));
            statusItem->setBackground(QBrush(getComplianceColor(ComplianceStatus::NonCompliant).lighter(180)));
            break;
        default:
            statusItem->setText(tr("Unknown"));
            break;
        }
        
        m_parametersTable->setItem(i, 3, statusItem);
    }
    
    m_parametersTable->resizeColumnsToContents();
}

QList<FigAnalysisWidget::FigParameter> FigAnalysisWidget::parseFigData(quint8 figType, const QVariantMap& figData) const
{
    QList<FigParameter> parameters;
    
    // Enhanced FIG 2 (Dynamic Label Segment) parsing
    if (figType == 2) {
        // Parse FIG 2 specific fields with ETSI compliance validation
        for (auto it = figData.constBegin(); it != figData.constEnd(); ++it) {
            FigParameter param;
            param.name = it.key();
            param.value = it.value().toString();
            param.status = ComplianceStatus::Compliant; // Default status
            
            // Enhanced validation for FIG 2 parameters
            if (param.name.contains("segment", Qt::CaseInsensitive)) {
                bool ok;
                int segmentNum = param.value.toInt(&ok);
                if (!ok || segmentNum < 0 || segmentNum > 7) {
                    param.status = ComplianceStatus::Warning;
                    param.expectedValue = "0-7";
                } else {
                    param.expectedValue = "0-7 (valid)";
                }
            } else if (param.name.contains("charset", Qt::CaseInsensitive)) {
                bool ok;
                int charset = param.value.toInt(&ok);
                if (!ok || charset > 15) {
                    param.status = ComplianceStatus::Warning;
                    param.expectedValue = "0-15";
                } else {
                    param.expectedValue = "0-15 (valid)";
                }
            } else if (param.name.contains("label", Qt::CaseInsensitive) || 
                       param.name.contains("text", Qt::CaseInsensitive)) {
                if (param.value.length() > 128) {
                    param.status = ComplianceStatus::Warning;
                    param.expectedValue = "≤128 chars";
                } else {
                    param.expectedValue = QString("≤128 chars (%1)").arg(param.value.length());
                }
            }
            
            parameters.append(param);
        }
        
        // Add missing mandatory fields with warnings
        bool hasSegment = false, hasCharset = false, hasLabel = false;
        for (const auto& p : parameters) {
            if (p.name.contains("segment", Qt::CaseInsensitive)) hasSegment = true;
            if (p.name.contains("charset", Qt::CaseInsensitive)) hasCharset = true;
            if (p.name.contains("label", Qt::CaseInsensitive)) hasLabel = true;
        }
        
        if (!hasSegment) {
            FigParameter param;
            param.name = "Segment Number";
            param.value = "Missing";
            param.expectedValue = "0-7";
            param.status = ComplianceStatus::Warning;
            parameters.append(param);
        }
        
        if (!hasCharset) {
            FigParameter param;
            param.name = "Character Set";
            param.value = "Missing";
            param.expectedValue = "0-15";
            param.status = ComplianceStatus::Warning;
            parameters.append(param);
        }
        
        if (!hasLabel) {
            FigParameter param;
            param.name = "Label Text";
            param.value = "Missing";
            param.expectedValue = "≤128 chars";
            param.status = ComplianceStatus::Warning;
            parameters.append(param);
        }
    } else {
        // Standard parsing for other FIG types
        for (auto it = figData.constBegin(); it != figData.constEnd(); ++it) {
            FigParameter param;
            param.name = it.key();
            param.value = it.value().toString();
            param.status = ComplianceStatus::Compliant; // Default status
            
            parameters.append(param);
        }
    }
    
    return parameters;
}

FigAnalysisWidget::ComplianceStatus FigAnalysisWidget::validateFigCompliance(quint8 figType, const QList<FigParameter>& parameters) const
{
    // Enhanced ETSI compliance validation - addressing Phase 5.3 warnings
    int nonCompliantCount = 0;
    int warningCount = 0;
    
    // FIG 2 specific validation (Dynamic Label Segment)
    if (figType == 2) {
        bool hasSegmentField = false;
        bool hasValidCharacterSet = false;
        bool hasProperLength = false;
        
        for (const FigParameter& param : parameters) {
            // Check for required segment field
            if (param.name.contains("segment", Qt::CaseInsensitive)) {
                hasSegmentField = true;
                // Validate segment number range (0-7 as per ETSI EN 300 401)
                bool ok;
                int segmentNum = param.value.toInt(&ok);
                if (!ok || segmentNum < 0 || segmentNum > 7) {
                    warningCount++;
                }
            }
            
            // Check for character set compliance
            if (param.name.contains("charset", Qt::CaseInsensitive) || 
                param.name.contains("character", Qt::CaseInsensitive)) {
                hasValidCharacterSet = true;
                // Validate character set (should be 0 for basic Latin per ETSI)
                bool ok;
                int charset = param.value.toInt(&ok);
                if (!ok || charset > 15) {  // Maximum 4-bit charset value
                    warningCount++;
                }
            }
            
            // Check label length compliance
            if (param.name.contains("label", Qt::CaseInsensitive) || 
                param.name.contains("text", Qt::CaseInsensitive)) {
                hasProperLength = true;
                // ETSI EN 300 401: Dynamic label segments should not exceed 128 characters
                if (param.value.length() > 128) {
                    warningCount++;
                }
            }
            
            // Check individual parameter status
            if (param.status == ComplianceStatus::NonCompliant) {
                nonCompliantCount++;
            } else if (param.status == ComplianceStatus::Warning) {
                warningCount++;
            }
        }
        
        // Validate mandatory fields for FIG 2 compliance
        if (!hasSegmentField) {
            // This addresses Warning 1: Missing segment field validation
            warningCount++;
        }
        if (!hasValidCharacterSet) {
            // This addresses Warning 2: Character set validation
            warningCount++;
        }
        if (!hasProperLength) {
            // This addresses Warning 3: Label length validation
            warningCount++;
        }
    } else {
        // Standard validation for other FIG types
        for (const FigParameter& param : parameters) {
            if (param.status == ComplianceStatus::NonCompliant) {
                nonCompliantCount++;
            } else if (param.status == ComplianceStatus::Warning) {
                warningCount++;
            }
        }
    }
    
    if (nonCompliantCount > 0) {
        return ComplianceStatus::NonCompliant;
    } else if (warningCount > 0) {
        return ComplianceStatus::Warning;
    } else {
        return ComplianceStatus::Compliant;
    }
}

QString FigAnalysisWidget::getFigTypeName(quint8 figType) const
{
    // ETSI EN 300 401 FIG type names
    static const QHash<quint8, QString> figNames = {
        {0, "MCI and part of FIC"},
        {1, "FIC data channel"},
        {2, "Dynamic label segment"},
        {3, "Reserved for future definition"},
        {4, "Reserved for future definition"},
        {5, "FIDC"},
        {6, "Conditional Access"},
        {7, "In House information"}
    };
    
    return figNames.value(figType, QString("Unknown FIG %1").arg(figType));
}

QString FigAnalysisWidget::getFigTypeDescription(quint8 figType) const
{
    // ETSI EN 300 401 FIG type descriptions
    static const QHash<quint8, QString> figDescriptions = {
        {0, "Multiplex Configuration Information (MCI) and part of Fast Information Channel (FIC)"},
        {1, "FIC data channel for Programme Associated Data"},
        {2, "Dynamic label segment for service and ensemble labels with segmentation support"},
        {3, "Reserved for future definition by ETSI"},
        {4, "Reserved for future definition by ETSI"},
        {5, "Fast Information Data Channel (FIDC)"},
        {6, "Conditional Access information"},
        {7, "In House information for broadcaster use"}
    };
    
    return figDescriptions.value(figType, "Unknown FIG type - refer to ETSI EN 300 401");
}

QColor FigAnalysisWidget::getComplianceColor(ComplianceStatus status) const
{
    switch (status) {
    case ComplianceStatus::Compliant:
        return QColor(76, 175, 80);   // Green
    case ComplianceStatus::Warning:
        return QColor(255, 152, 0);   // Orange
    case ComplianceStatus::NonCompliant:
        return QColor(244, 67, 54);   // Red
    case ComplianceStatus::Error:
        return QColor(156, 39, 176);  // Purple
    default:
        return QColor(158, 158, 158); // Gray
    }
}

QIcon FigAnalysisWidget::getComplianceIcon(ComplianceStatus status) const
{
    switch (status) {
    case ComplianceStatus::Compliant:
        return QApplication::style()->standardIcon(QStyle::SP_DialogApplyButton);
    case ComplianceStatus::Warning:
        return QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning);
    case ComplianceStatus::NonCompliant:
    case ComplianceStatus::Error:
        return QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical);
    default:
        return QApplication::style()->standardIcon(QStyle::SP_MessageBoxQuestion);
    }
}

QString FigAnalysisWidget::formatComplianceScore(double score) const
{
    return QString("%1%").arg(score, 0, 'f', 1);
}

void FigAnalysisWidget::updateComplianceSummary()
{
    m_totalFigTypes = m_detectedFigTypes.size();
    m_compliantFigTypes = 0;
    double totalCompliance = 0.0;
    
    for (const auto& analysis : m_figAnalyses) {
        if (analysis.complianceStatus == ComplianceStatus::Compliant) {
            m_compliantFigTypes++;
        }
        totalCompliance += analysis.completeness;
    }
    
    m_overallComplianceScore = (m_totalFigTypes > 0) ? (totalCompliance / m_totalFigTypes) : 0.0;
    
    // Update UI
    m_figCountLabel->setText(tr("Detected FIG Types: %1").arg(m_totalFigTypes));
    m_complianceScoreLabel->setText(tr("Overall Compliance: %1%").arg(m_overallComplianceScore, 0, 'f', 1));
    m_complianceProgress->setValue(static_cast<int>(m_overallComplianceScore));
    
    // Update progress bar color based on compliance
    QString progressStyle;
    if (m_overallComplianceScore >= 90.0) {
        progressStyle = "QProgressBar::chunk { background-color: #4CAF50; }"; // Green
    } else if (m_overallComplianceScore >= 70.0) {
        progressStyle = "QProgressBar::chunk { background-color: #FF9800; }"; // Orange
    } else {
        progressStyle = "QProgressBar::chunk { background-color: #F44336; }"; // Red
    }
    m_complianceProgress->setStyleSheet(progressStyle);
}

double FigAnalysisWidget::calculateOverallCompliance() const
{
    return m_overallComplianceScore;
}

bool FigAnalysisWidget::validateState() const
{
    // Basic state validation
    return m_mainTabs != nullptr && m_figTree != nullptr;
}

// Enhanced ETSI 300 799 compliance validation methods
void FigAnalysisWidget::validateEtsi300799Compliance()
{
    Logger::instance().log(Logger::Info, "FigAnalysisWidget", "Performing enhanced ETSI 300 799 compliance validation");
    
    int complianceLevel = 0;
    int totalChecks = 0;
    QStringList complianceIssues;
    QStringList passedChecks;
    
    // Core FIG validation (40% of total score)
    
    // Validate FIG 0/0 - Ensemble Information (10%)
    if (validateFig00Services()) {
        complianceLevel += 10;
        passedChecks << "FIG 0/0: Ensemble Information - PASSED";
    } else {
        complianceIssues << "FIG 0/0: Missing or invalid ensemble information";
    }
    totalChecks += 10;
    
    // Validate FIG 0/1 - Sub-channel Organization (10%)
    if (validateFig01Subchannels()) {
        complianceLevel += 10;
        passedChecks << "FIG 0/1: Sub-channel Organization - PASSED";
    } else {
        complianceIssues << "FIG 0/1: Missing or invalid sub-channel organization";
    }
    totalChecks += 10;
    
    // Validate FIG 0/2 - Service Organization (20%)
    if (validateFig02ServiceOrg()) {
        complianceLevel += 20;
        passedChecks << "FIG 0/2: Service Organization - PASSED";
    } else {
        complianceIssues << "FIG 0/2: Missing or invalid service organization";
    }
    totalChecks += 20;
    
    // DAB+ specific validation (30% of total score)
    
    // Check for DAB+ services presence (15%)
    int dabPlusServices = 0;
    int totalServices = 0;
    
    for (auto it = m_figAnalyses.begin(); it != m_figAnalyses.end(); ++it) {
        const FigAnalysis& analysis = it.value();
        if (analysis.figType == 0 && analysis.figExtension == 2) {
            QVariantMap figData = analysis.figData;
            totalServices++;
            
            // Check for DAB+ indicators (TMId = 3)
            QVariant tmId = figData.value("TMId", figData.value("transport_mechanism", 0));
            if (tmId.toInt() == 3) {
                dabPlusServices++;
            }
        }
    }
    
    if (dabPlusServices > 0) {
        complianceLevel += 15;
        passedChecks << QString("DAB+ Services: %1/%2 services detected - PASSED").arg(dabPlusServices).arg(totalServices);
        emit dabPlusServiceDetected(QString("Multiple services detected: %1").arg(dabPlusServices));
    } else if (totalServices > 0) {
        complianceLevel += 5; // Partial credit for legacy DAB
        complianceIssues << "DAB+ Services: No DAB+ services detected (legacy DAB only)";
    } else {
        complianceIssues << "DAB+ Services: No services detected";
    }
    totalChecks += 15;
    
    // Audio codec compliance (15%)
    bool hasValidAudioCodec = false;
    for (auto it = m_figAnalyses.begin(); it != m_figAnalyses.end(); ++it) {
        const FigAnalysis& analysis = it.value();
        if (analysis.figData.contains("audio_codec") || analysis.figData.contains("AudioServiceComponentType")) {
            QString codecType = analysis.figData.value("audio_codec", 
                                    analysis.figData.value("AudioServiceComponentType", "")).toString();
            
            if (codecType.contains("HE-AAC", Qt::CaseInsensitive) || 
                codecType.contains("AAC", Qt::CaseInsensitive) ||
                analysis.figData.value("ASCTy", 0).toInt() == 63) { // DAB+ Audio Service Component Type
                hasValidAudioCodec = true;
                break;
            }
        }
    }
    
    if (hasValidAudioCodec) {
        complianceLevel += 15;
        passedChecks << "Audio Codec: Valid DAB+ HE-AAC codec detected - PASSED";
    } else {
        complianceLevel += 3; // Minimal credit for any audio
        complianceIssues << "Audio Codec: DAB+ HE-AAC codec not detected";
    }
    totalChecks += 15;
    
    // Enhanced technical compliance (30% of total score)
    
    // FIG completeness check (10%)
    QList<quint8> requiredFigs = {0, 1}; // Core required FIGs
    QList<quint8> optionalFigs = {2, 5, 6}; // Beneficial FIGs
    
    int requiredPresent = 0;
    int optionalPresent = 0;
    
    for (quint8 fig : requiredFigs) {
        if (m_figAnalyses.contains(fig)) {
            requiredPresent++;
        }
    }
    
    for (quint8 fig : optionalFigs) {
        if (m_figAnalyses.contains(fig)) {
            optionalPresent++;
        }
    }
    
    int figComplianceScore = (requiredPresent * 7) + (optionalPresent * 1); // Max 10 points
    complianceLevel += figComplianceScore;
    
    if (requiredPresent == requiredFigs.size()) {
        passedChecks << QString("FIG Completeness: %1/%2 required FIGs present - PASSED").arg(requiredPresent).arg(requiredFigs.size());
    } else {
        complianceIssues << QString("FIG Completeness: Only %1/%2 required FIGs present").arg(requiredPresent).arg(requiredFigs.size());
    }
    totalChecks += 10;
    
    // Data integrity check (10%)
    bool hasDataIntegrityIssues = false;
    for (auto it = m_figAnalyses.begin(); it != m_figAnalyses.end(); ++it) {
        const FigAnalysis& analysis = it.value();
        if (analysis.complianceStatus == ComplianceStatus::Error || 
            analysis.complianceStatus == ComplianceStatus::NonCompliant) {
            hasDataIntegrityIssues = true;
            complianceIssues << QString("FIG %1: Data integrity issues detected").arg(analysis.figType);
        }
    }
    
    if (!hasDataIntegrityIssues) {
        complianceLevel += 10;
        passedChecks << "Data Integrity: All FIGs pass integrity checks - PASSED";
    } else {
        complianceLevel += 3; // Partial credit
        complianceIssues << "Data Integrity: Some FIGs have integrity issues";
    }
    totalChecks += 10;
    
    // Service labeling compliance (10%)
    bool hasServiceLabels = false;
    for (auto it = m_figAnalyses.begin(); it != m_figAnalyses.end(); ++it) {
        const FigAnalysis& analysis = it.value();
        if (analysis.figData.contains("service_label") || 
            analysis.figData.contains("ServiceLabel") ||
            (analysis.figType == 1 && analysis.figExtension == 0)) { // FIG 1/0 contains service labels
            hasServiceLabels = true;
            break;
        }
    }
    
    if (hasServiceLabels) {
        complianceLevel += 10;
        passedChecks << "Service Labeling: Service labels detected - PASSED";
    } else {
        complianceIssues << "Service Labeling: No service labels detected";
    }
    totalChecks += 10;
    
    // Calculate final compliance percentage
    int finalComplianceLevel = totalChecks > 0 ? (complianceLevel * 100) / totalChecks : 0;
    
    // Update compliance indicators with enhanced information
    addColorCodedIndicators();
    
    // Update compliance progress bar with enhanced styling
    if (m_complianceProgress) {
        m_complianceProgress->setValue(finalComplianceLevel);
        
        QColor complianceColor = getComplianceColor(finalComplianceLevel);
        QString progressStyle = QString(
            "QProgressBar {"
            "    border: 2px solid #555555;"
            "    border-radius: 5px;"
            "    background-color: #2D2D30;"
            "    text-align: center;"
            "    color: white;"
            "    font-weight: bold;"
            "    font-size: 12px;"
            "}"
            "QProgressBar::chunk {"
            "    background-color: %1;"
            "    border-radius: 3px;"
            "    margin: 1px;"
            "}"
        ).arg(complianceColor.name());
        
        m_complianceProgress->setStyleSheet(progressStyle);
        m_complianceProgress->setFormat(QString("ETSI 300 799: %p% (%1/%2 checks passed)")
                                       .arg(complianceLevel).arg(totalChecks));
    }
    
    // Update compliance label with detailed information
    if (m_complianceScoreLabel) {
        QString statusText;
        QColor textColor = getComplianceColor(finalComplianceLevel);
        
        if (finalComplianceLevel >= 95) {
            statusText = "✅ EXCELLENT COMPLIANCE";
        } else if (finalComplianceLevel >= 85) {
            statusText = "✅ GOOD COMPLIANCE";
        } else if (finalComplianceLevel >= 70) {
            statusText = "⚠️ ACCEPTABLE COMPLIANCE";
        } else if (finalComplianceLevel >= 50) {
            statusText = "⚠️ PARTIAL COMPLIANCE";
        } else {
            statusText = "❌ POOR COMPLIANCE";
        }
        
        m_complianceScoreLabel->setText(QString("%1 (%2%)").arg(statusText).arg(finalComplianceLevel));
        m_complianceScoreLabel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 12px;")
                                             .arg(textColor.name()));
    }
    
    // Update compliance issues text (if widget exists)
    if (m_complianceIssuesText) {
        QString issuesText = "Compliance Issues:\n";
        for (const QString& issue : complianceIssues) {
            issuesText += "• " + issue + "\n";
        }
        
        if (complianceIssues.isEmpty()) {
            issuesText += "No compliance issues detected! ✅\n";
        }
        
        issuesText += "\nPassed Checks:\n";
        for (const QString& check : passedChecks) {
            issuesText += "• " + check + "\n";
        }
        
        m_complianceIssuesText->setPlainText(issuesText);
    }
    
    // Emit enhanced compliance update signal
    emit complianceUpdated(finalComplianceLevel);
    
    Logger::instance().log(Logger::Info, "FigAnalysisWidget", 
                          QString("Enhanced ETSI 300 799 compliance: %1% (%2/%3 points, %4 issues)")
                          .arg(finalComplianceLevel).arg(complianceLevel).arg(totalChecks).arg(complianceIssues.size()));
}

void FigAnalysisWidget::highlightDabPlusServices()
{
    Logger::instance().log(Logger::Debug, "FigAnalysisWidget", "Highlighting DAB+ services");
    
    QMutexLocker locker(&m_dataMutex);
    
    // Iterate through FIG analyses to find DAB+ services
    for (auto it = m_figAnalyses.begin(); it != m_figAnalyses.end(); ++it) {
        const FigAnalysis& analysis = it.value();
        
        // Check for DAB+ indicators in FIG 0/2 (Service Organization)
        if (analysis.figType == 0 && analysis.figExtension == 2) {
            QVariantMap figData = analysis.figData;
            
            // Look for TMId (Transport Mechanism Identifier)
            if (figData.contains("TMId") || figData.contains("transport_mechanism")) {
                QVariant tmId = figData.value("TMId", figData.value("transport_mechanism"));
                
                // TMId = 3 indicates DAB+ (HE-AAC v2)
                if (tmId.toInt() == 3) {
                    QString serviceId = figData.value("serviceId", 
                                                    figData.value("service_id", "unknown")).toString();
                    
                    // Highlight the service in the tree
                    for (int i = 0; i < m_figTree->topLevelItemCount(); ++i) {
                        QTreeWidgetItem* item = m_figTree->topLevelItem(i);
                        if (item && item->data(0, Qt::UserRole).toString() == serviceId) {
                            // Set DAB+ indicator styling
                            QFont font = item->font(0);
                            font.setBold(true);
                            item->setFont(0, font);
                            item->setFont(1, font);
                            
                            // Set DAB+ color (green for DAB+)
                            item->setForeground(0, QBrush(QColor(0, 150, 0)));
                            item->setForeground(1, QBrush(QColor(0, 150, 0)));
                            
                            // Add DAB+ icon/indicator
                            item->setIcon(0, QIcon(":/icons/dabplus.png")); // Would need to add this icon
                            
                            // Set tooltip
                            item->setToolTip(0, tr("DAB+ Service (HE-AAC v2)"));
                            item->setToolTip(1, tr("Enhanced audio codec with improved efficiency"));
                        }
                    }
                    
                    emit dabPlusServiceDetected(serviceId);
                    
                    Logger::instance().log(Logger::Info, "FigAnalysisWidget", 
                                          QString("DAB+ service detected: %1").arg(serviceId));
                }
            }
        }
        
        // Check for additional DAB+ indicators in FIG 0/17 (Programme Type)
        if (analysis.figType == 0 && analysis.figExtension == 17) {
            QVariantMap figData = analysis.figData;
            
            // Look for audio service component types
            if (figData.contains("ASCTy") || figData.contains("audio_service_component_type")) {
                QVariant ascTy = figData.value("ASCTy", figData.value("audio_service_component_type"));
                
                // ASCTy = 63 (0x3F) indicates DAB+ audio
                if (ascTy.toInt() == 63) {
                    QString serviceId = figData.value("serviceId", 
                                                    figData.value("service_id", "unknown")).toString();
                    emit dabPlusServiceDetected(serviceId);
                }
            }
        }
    }
}

void FigAnalysisWidget::displayAudioCodecInformation()
{
    Logger::instance().log(Logger::Debug, "FigAnalysisWidget", "Displaying audio codec information");
    
    // Update the parameters table with codec information
    if (!m_parametersTable) return;
    
    // Add codec information section
    int currentRow = m_parametersTable->rowCount();
    m_parametersTable->insertRow(currentRow);
    
    // Add codec header
    QTableWidgetItem* codecHeaderItem = new QTableWidgetItem(tr("Audio Codec Information"));
    codecHeaderItem->setFont(QFont("Arial", 10, QFont::Bold));
    codecHeaderItem->setBackground(QBrush(QColor(230, 230, 230)));
    m_parametersTable->setItem(currentRow, 0, codecHeaderItem);
    m_parametersTable->setItem(currentRow, 1, new QTableWidgetItem(""));
    m_parametersTable->setItem(currentRow, 2, new QTableWidgetItem(""));
    
    // Analyze services for codec information
    QStringList dabServices;
    QStringList dabPlusServices;
    
    for (auto it = m_figAnalyses.begin(); it != m_figAnalyses.end(); ++it) {
        const FigAnalysis& analysis = it.value();
        
        if (analysis.figType == 0 && analysis.figExtension == 2) {
            QVariantMap figData = analysis.figData;
            QString serviceId = figData.value("serviceId", "unknown").toString();
            int tmId = figData.value("TMId", 0).toInt();
            
            if (tmId == 0) {
                dabServices.append(serviceId);
            } else if (tmId == 3) {
                dabPlusServices.append(serviceId);
            }
        }
    }
    
    // Add DAB service count
    currentRow = m_parametersTable->rowCount();
    m_parametersTable->insertRow(currentRow);
    m_parametersTable->setItem(currentRow, 0, new QTableWidgetItem(tr("DAB Services (MPEG-1 Layer II)")));
    m_parametersTable->setItem(currentRow, 1, new QTableWidgetItem(QString::number(dabServices.size())));
    m_parametersTable->setItem(currentRow, 2, new QTableWidgetItem(dabServices.join(", ")));
    
    // Add DAB+ service count
    currentRow = m_parametersTable->rowCount();
    m_parametersTable->insertRow(currentRow);
    QTableWidgetItem* dabPlusItem = new QTableWidgetItem(tr("DAB+ Services (HE-AAC v2)"));
    dabPlusItem->setForeground(QBrush(QColor(0, 150, 0))); // Green for DAB+
    dabPlusItem->setFont(QFont("Arial", 9, QFont::Bold));
    m_parametersTable->setItem(currentRow, 0, dabPlusItem);
    
    QTableWidgetItem* dabPlusCountItem = new QTableWidgetItem(QString::number(dabPlusServices.size()));
    dabPlusCountItem->setForeground(QBrush(QColor(0, 150, 0)));
    dabPlusCountItem->setFont(QFont("Arial", 9, QFont::Bold));
    m_parametersTable->setItem(currentRow, 1, dabPlusCountItem);
    
    m_parametersTable->setItem(currentRow, 2, new QTableWidgetItem(dabPlusServices.join(", ")));
    
    // Add codec efficiency information
    currentRow = m_parametersTable->rowCount();
    m_parametersTable->insertRow(currentRow);
    m_parametersTable->setItem(currentRow, 0, new QTableWidgetItem(tr("Codec Efficiency Ratio")));
    
    double efficiencyRatio = dabServices.isEmpty() ? 0.0 : 
                           static_cast<double>(dabPlusServices.size()) / (dabServices.size() + dabPlusServices.size()) * 100.0;
    m_parametersTable->setItem(currentRow, 1, new QTableWidgetItem(QString("%1%").arg(efficiencyRatio, 0, 'f', 1)));
    m_parametersTable->setItem(currentRow, 2, new QTableWidgetItem(tr("Percentage of bandwidth-efficient services")));
}

void FigAnalysisWidget::setupComplianceValidation()
{
    Logger::instance().log(Logger::Debug, "FigAnalysisWidget", "Setting up compliance validation framework");
    
    // Initialize compliance validation framework
    if (m_complianceProgress) {
        m_complianceProgress->setRange(0, 100);
        m_complianceProgress->setValue(0);
        m_complianceProgress->setTextVisible(true);
        m_complianceProgress->setFormat("%p%");
    }
    
    // Set up compliance tree structure
    if (m_complianceTree) {
        m_complianceTree->clear();
        
        // Add ETSI standard categories
        QTreeWidgetItem* etsi300799Item = new QTreeWidgetItem(m_complianceTree);
        etsi300799Item->setText(0, tr("ETSI EN 300 799"));
        etsi300799Item->setText(1, tr("Digital Audio Broadcasting (DAB) System"));
        etsi300799Item->setText(2, tr("Pending"));
        
        // Add sub-categories
        QTreeWidgetItem* figStructureItem = new QTreeWidgetItem(etsi300799Item);
        figStructureItem->setText(0, tr("FIG Structure"));
        figStructureItem->setText(1, tr("Fast Information Group compliance"));
        figStructureItem->setText(2, tr("Checking..."));
        
        QTreeWidgetItem* serviceOrgItem = new QTreeWidgetItem(etsi300799Item);
        serviceOrgItem->setText(0, tr("Service Organization"));
        serviceOrgItem->setText(1, tr("Service and component organization"));
        serviceOrgItem->setText(2, tr("Checking..."));
        
        QTreeWidgetItem* audioCodecItem = new QTreeWidgetItem(etsi300799Item);
        audioCodecItem->setText(0, tr("Audio Codecs"));
        audioCodecItem->setText(1, tr("DAB and DAB+ codec compliance"));
        audioCodecItem->setText(2, tr("Checking..."));
        
        m_complianceTree->expandAll();
    }
}

void FigAnalysisWidget::addColorCodedIndicators()
{
    // Update tree items with color-coded compliance indicators
    for (int i = 0; i < m_figTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_figTree->topLevelItem(i);
        if (!item) continue;
        
        quint8 figType = item->data(0, Qt::UserRole).toUInt();
        if (m_figAnalyses.contains(figType)) {
            const FigAnalysis& analysis = m_figAnalyses[figType];
            QColor statusColor = getComplianceColor(analysis.complianceStatus);
            
            // Update item appearance based on compliance
            item->setForeground(2, QBrush(statusColor));
            item->setText(2, formatComplianceScore(analysis.completeness));
            
            // Add compliance icon
            item->setIcon(0, getComplianceIcon(analysis.complianceStatus));
        }
    }
}

QColor FigAnalysisWidget::getComplianceColor(int level)
{
    if (level >= 90) return QColor(0, 150, 0);      // Dark green - Excellent
    if (level >= 75) return QColor(100, 200, 0);    // Light green - Good  
    if (level >= 50) return QColor(255, 165, 0);    // Orange - Fair
    if (level >= 25) return QColor(255, 100, 0);    // Red-orange - Poor
    return QColor(200, 0, 0);                       // Dark red - Critical
}

bool FigAnalysisWidget::validateFig00Services()
{
    // Validate FIG 0/0 (Ensemble Information) compliance
    if (!m_figAnalyses.contains(0)) {
        Logger::instance().log(Logger::Warning, "FigAnalysisWidget", "FIG 0/0 not found - ensemble information missing");
        return false;
    }
    
    const FigAnalysis& fig00 = m_figAnalyses[0];
    const QVariantMap& figData = fig00.figData;
    
    // Check mandatory fields per ETSI EN 300 799
    bool hasEnsembleId = figData.contains("ensembleId") || figData.contains("ensemble_id");
    bool hasEnsembleLabel = figData.contains("ensembleLabel") || figData.contains("ensemble_label");
    bool hasCountryId = figData.contains("countryId") || figData.contains("country_id");
    
    if (!hasEnsembleId || !hasEnsembleLabel || !hasCountryId) {
        Logger::instance().log(Logger::Warning, "FigAnalysisWidget", 
                              "FIG 0/0 missing mandatory fields for ETSI compliance");
        return false;
    }
    
    Logger::instance().log(Logger::Info, "FigAnalysisWidget", "FIG 0/0 validation passed");
    return true;
}

bool FigAnalysisWidget::validateFig01Subchannels()
{
    // Validate FIG 0/1 (Sub-channel Organization) compliance
    if (!m_figAnalyses.contains(1)) {
        Logger::instance().log(Logger::Warning, "FigAnalysisWidget", "FIG 0/1 not found - subchannel organization missing");
        return false;
    }
    
    const FigAnalysis& fig01 = m_figAnalyses[1];
    const QVariantMap& figData = fig01.figData;
    
    // Check for valid subchannel configuration
    bool hasSubchannelId = figData.contains("subchannelId") || figData.contains("subchannel_id");
    bool hasStartAddress = figData.contains("startAddress") || figData.contains("start_address");
    bool hasSubchannelSize = figData.contains("subchannelSize") || figData.contains("subchannel_size");
    
    if (!hasSubchannelId || !hasStartAddress || !hasSubchannelSize) {
        Logger::instance().log(Logger::Warning, "FigAnalysisWidget", 
                              "FIG 0/1 missing mandatory subchannel fields");
        return false;
    }
    
    Logger::instance().log(Logger::Info, "FigAnalysisWidget", "FIG 0/1 validation passed");
    return true;
}

bool FigAnalysisWidget::validateFig02ServiceOrg()
{
    // Validate FIG 0/2 (Service Organization) compliance
    if (!m_figAnalyses.contains(2)) {
        Logger::instance().log(Logger::Warning, "FigAnalysisWidget", "FIG 0/2 not found - service organization missing");
        return false;
    }
    
    const FigAnalysis& fig02 = m_figAnalyses[2];
    const QVariantMap& figData = fig02.figData;
    
    // Check for valid service organization
    bool hasServiceId = figData.contains("serviceId") || figData.contains("service_id");
    bool hasComponentCount = figData.contains("componentCount") || figData.contains("component_count");
    bool hasTransportMechanism = figData.contains("TMId") || figData.contains("transport_mechanism");
    
    if (!hasServiceId || !hasComponentCount || !hasTransportMechanism) {
        Logger::instance().log(Logger::Warning, "FigAnalysisWidget", 
                              "FIG 0/2 missing mandatory service organization fields");
        return false;
    }
    
    Logger::instance().log(Logger::Info, "FigAnalysisWidget", "FIG 0/2 validation passed");
    return true;
}