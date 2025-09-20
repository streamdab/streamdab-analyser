/**
 * @file fig_analysis_widget.cpp
 * @brief Professional FIG Analysis Widget implementation
 * 
 * This file implements the FigAnalysisWidget class providing comprehensive
 * FIG analysis capabilities with ETSI compliance validation.
 */

#include "fig_analysis_widget.h"
#include "core/etisnoop_wrapper.h"
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
    , m_etisnoopWrapper(nullptr)
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

bool FigAnalysisWidget::initialize(EtisnoopWrapper *wrapper)
{
    if (!wrapper) {
        Logger::instance().log(Logger::Error, "FigAnalysisWidget", "Invalid ETISnoop wrapper provided");
        return false;
    }
    
    Logger::instance().log(Logger::Info, "FigAnalysisWidget", "Initializing with ETISnoop wrapper");
    
    m_etisnoopWrapper = wrapper;
    
    // Connect ETISnoop wrapper signals (using generic signal names that should exist)
    // Note: These connections would need to be adapted based on actual ETISnoop wrapper signals
    connect(m_etisnoopWrapper, &EtisnoopWrapper::etsiComplianceIssue,
            this, &FigAnalysisWidget::onEtsiComplianceIssue);
    
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