#include "etsi_compliance_monitor.h"
#include "core/eti_processor.hpp"
#include "utils/logger.h"
#include <QApplication>
#include <QStyle>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTextStream>
#include <QMutexLocker>
#include <QSplitter>

// Professional broadcast industry color definitions
const QColor ETSIComplianceMonitor::BACKGROUND_DARK = QColor(45, 45, 48);     // #2D2D30
const QColor ETSIComplianceMonitor::BACKGROUND_MEDIUM = QColor(62, 62, 66);   // #3E3E42
const QColor ETSIComplianceMonitor::ACCENT_BLUE = QColor(0, 120, 212);        // #0078D4
const QColor ETSIComplianceMonitor::SEVERITY_INFO = QColor(23, 162, 184);     // #17A2B8
const QColor ETSIComplianceMonitor::SEVERITY_WARNING = QColor(255, 193, 7);   // #FFC107
const QColor ETSIComplianceMonitor::SEVERITY_ERROR = QColor(220, 53, 69);     // #DC3545
const QColor ETSIComplianceMonitor::SEVERITY_CRITICAL = QColor(111, 66, 193); // #6F42C1
const QColor ETSIComplianceMonitor::STATUS_EXCELLENT = QColor(40, 167, 69);   // #28A745
const QColor ETSIComplianceMonitor::STATUS_GOOD = QColor(32, 201, 151);       // #20C997
const QColor ETSIComplianceMonitor::STATUS_FAIR = QColor(253, 126, 20);       // #FD7E14
const QColor ETSIComplianceMonitor::STATUS_POOR = QColor(232, 62, 140);       // #E83E8C
const QColor ETSIComplianceMonitor::TEXT_PRIMARY = QColor(255, 255, 255);     // #FFFFFF
const QColor ETSIComplianceMonitor::TEXT_SECONDARY = QColor(108, 117, 125);   // #6C757D

ETSIComplianceMonitor::ETSIComplianceMonitor(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_overviewLayout(nullptr)
    , m_controlLayout(nullptr)
    , m_overviewFrame(nullptr)
    , m_scoreLabel(nullptr)
    , m_scoreValueLabel(nullptr)
    , m_scoreProgressBar(nullptr)
    , m_statusLabel(nullptr)
    , m_statisticsFrame(nullptr)
    , m_totalViolationsLabel(nullptr)
    , m_criticalCountLabel(nullptr)
    , m_errorCountLabel(nullptr)
    , m_warningCountLabel(nullptr)
    , m_infoCountLabel(nullptr)
    , m_violationsTree(nullptr)
    , m_severityFilter(nullptr)
    , m_categoryFilter(nullptr)
    , m_clearButton(nullptr)
    , m_exportButton(nullptr)
    , m_refreshButton(nullptr)
    , m_violationDetails(nullptr)
    , m_etiProcessor(nullptr)
    , m_monitoringActive(false)
    , m_selectedSeverity(ETSIViolation::Info)
    , m_selectedCategory(ETSIViolation::FIG_Structure)
    , m_showAllCategories(true)
    , m_showAllSeverities(true)
{
    setupUI();
    setupComplianceOverview();
    setupViolationsTree();
    setupControlPanel();
    setupViolationDetails();
    applyBroadcastingTheme();
    
    // Professional fonts
    m_headerFont = QFont("Segoe UI", 9, QFont::Bold);
    m_valueFont = QFont("Consolas", 11, QFont::Bold);
    m_detailFont = QFont("Consolas", 9, QFont::Normal);
    
    Logger::instance().log(Logger::Info, "ETSIComplianceMonitor", 
                          "Professional ETSI Compliance Monitor initialized");
}

ETSIComplianceMonitor::~ETSIComplianceMonitor()
{
    stopMonitoring();
}

void ETSIComplianceMonitor::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(8);
    
    // Create splitter for overview and details
    auto* splitter = new QSplitter(Qt::Vertical, this);
    
    // Top section: Overview and statistics
    auto* topWidget = new QWidget();
    auto* topLayout = new QVBoxLayout(topWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(6);
    
    // Overview layout
    m_overviewLayout = new QHBoxLayout();
    m_overviewLayout->setSpacing(8);
    topLayout->addLayout(m_overviewLayout);
    
    // Control layout
    m_controlLayout = new QHBoxLayout();
    m_controlLayout->setSpacing(6);
    topLayout->addLayout(m_controlLayout);
    
    splitter->addWidget(topWidget);
    
    // Bottom section: Violations tree and details
    auto* bottomWidget = new QWidget();
    auto* bottomLayout = new QHBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(8);
    
    // Violations tree (70% width)
    m_violationsTree = new QTreeWidget();
    bottomLayout->addWidget(m_violationsTree, 7);
    
    // Violation details (30% width)
    m_violationDetails = new QTextEdit();
    m_violationDetails->setReadOnly(true);
    m_violationDetails->setMaximumWidth(300);
    bottomLayout->addWidget(m_violationDetails, 3);
    
    splitter->addWidget(bottomWidget);
    
    // Set splitter proportions
    splitter->setStretchFactor(0, 1);  // Top section: 30%
    splitter->setStretchFactor(1, 2);  // Bottom section: 70%
    
    m_mainLayout->addWidget(splitter);
}

void ETSIComplianceMonitor::setupComplianceOverview()
{
    // Compliance score frame
    m_overviewFrame = new QFrame();
    m_overviewFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_overviewFrame->setObjectName("ComplianceOverviewFrame");
    
    auto* overviewLayout = new QVBoxLayout(m_overviewFrame);
    overviewLayout->setContentsMargins(8, 6, 8, 6);
    overviewLayout->setSpacing(4);
    
    // Score header
    auto* scoreHeaderLayout = new QHBoxLayout();
    
    m_scoreLabel = new QLabel(tr("ETSI Compliance Score"));
    m_scoreLabel->setFont(m_headerFont);
    scoreHeaderLayout->addWidget(m_scoreLabel);
    
    scoreHeaderLayout->addStretch();
    
    m_scoreValueLabel = new QLabel("100%");
    m_scoreValueLabel->setFont(m_valueFont);
    scoreHeaderLayout->addWidget(m_scoreValueLabel);
    
    overviewLayout->addLayout(scoreHeaderLayout);
    
    // Score progress bar
    m_scoreProgressBar = new QProgressBar();
    m_scoreProgressBar->setRange(0, 100);
    m_scoreProgressBar->setValue(100);
    m_scoreProgressBar->setTextVisible(false);
    overviewLayout->addWidget(m_scoreProgressBar);
    
    // Status label
    m_statusLabel = new QLabel(tr("EXCELLENT"));
    m_statusLabel->setFont(QFont("Segoe UI", 8, QFont::Bold));
    m_statusLabel->setAlignment(Qt::AlignCenter);
    overviewLayout->addWidget(m_statusLabel);
    
    m_overviewLayout->addWidget(m_overviewFrame, 1);
    
    // Statistics frame
    m_statisticsFrame = new QFrame();
    m_statisticsFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_statisticsFrame->setObjectName("StatisticsFrame");
    
    auto* statsLayout = new QGridLayout(m_statisticsFrame);
    statsLayout->setContentsMargins(8, 6, 8, 6);
    statsLayout->setSpacing(4);
    
    // Statistics labels
    auto* statsHeaderLabel = new QLabel(tr("Violation Statistics"));
    statsHeaderLabel->setFont(m_headerFont);
    statsHeaderLabel->setAlignment(Qt::AlignCenter);
    statsLayout->addWidget(statsHeaderLabel, 0, 0, 1, 2);
    
    // Total violations
    auto* totalLabel = new QLabel(tr("Total:"));
    totalLabel->setFont(QFont("Segoe UI", 8));
    statsLayout->addWidget(totalLabel, 1, 0);
    
    m_totalViolationsLabel = new QLabel("0");
    m_totalViolationsLabel->setFont(QFont("Consolas", 8, QFont::Bold));
    m_totalViolationsLabel->setAlignment(Qt::AlignRight);
    statsLayout->addWidget(m_totalViolationsLabel, 1, 1);
    
    // Critical violations
    auto* criticalLabel = new QLabel(tr("Critical:"));
    criticalLabel->setFont(QFont("Segoe UI", 8));
    criticalLabel->setStyleSheet(QString("color: %1;").arg(SEVERITY_CRITICAL.name()));
    statsLayout->addWidget(criticalLabel, 2, 0);
    
    m_criticalCountLabel = new QLabel("0");
    m_criticalCountLabel->setFont(QFont("Consolas", 8, QFont::Bold));
    m_criticalCountLabel->setAlignment(Qt::AlignRight);
    m_criticalCountLabel->setStyleSheet(QString("color: %1;").arg(SEVERITY_CRITICAL.name()));
    statsLayout->addWidget(m_criticalCountLabel, 2, 1);
    
    // Error violations
    auto* errorLabel = new QLabel(tr("Errors:"));
    errorLabel->setFont(QFont("Segoe UI", 8));
    errorLabel->setStyleSheet(QString("color: %1;").arg(SEVERITY_ERROR.name()));
    statsLayout->addWidget(errorLabel, 3, 0);
    
    m_errorCountLabel = new QLabel("0");
    m_errorCountLabel->setFont(QFont("Consolas", 8, QFont::Bold));
    m_errorCountLabel->setAlignment(Qt::AlignRight);
    m_errorCountLabel->setStyleSheet(QString("color: %1;").arg(SEVERITY_ERROR.name()));
    statsLayout->addWidget(m_errorCountLabel, 3, 1);
    
    // Warning violations
    auto* warningLabel = new QLabel(tr("Warnings:"));
    warningLabel->setFont(QFont("Segoe UI", 8));
    warningLabel->setStyleSheet(QString("color: %1;").arg(SEVERITY_WARNING.name()));
    statsLayout->addWidget(warningLabel, 4, 0);
    
    m_warningCountLabel = new QLabel("0");
    m_warningCountLabel->setFont(QFont("Consolas", 8, QFont::Bold));
    m_warningCountLabel->setAlignment(Qt::AlignRight);
    m_warningCountLabel->setStyleSheet(QString("color: %1;").arg(SEVERITY_WARNING.name()));
    statsLayout->addWidget(m_warningCountLabel, 4, 1);
    
    // Info violations
    auto* infoLabel = new QLabel(tr("Info:"));
    infoLabel->setFont(QFont("Segoe UI", 8));
    infoLabel->setStyleSheet(QString("color: %1;").arg(SEVERITY_INFO.name()));
    statsLayout->addWidget(infoLabel, 5, 0);
    
    m_infoCountLabel = new QLabel("0");
    m_infoCountLabel->setFont(QFont("Consolas", 8, QFont::Bold));
    m_infoCountLabel->setAlignment(Qt::AlignRight);
    m_infoCountLabel->setStyleSheet(QString("color: %1;").arg(SEVERITY_INFO.name()));
    statsLayout->addWidget(m_infoCountLabel, 5, 1);
    
    m_overviewLayout->addWidget(m_statisticsFrame, 1);
}

void ETSIComplianceMonitor::setupViolationsTree()
{
    m_violationsTree->setObjectName("ViolationsTree");
    m_violationsTree->setHeaderLabels({tr("Severity"), tr("Category"), tr("Description"), tr("Standard"), tr("Time"), tr("Frame")});
    m_violationsTree->setAlternatingRowColors(true);
    m_violationsTree->setRootIsDecorated(false);
    m_violationsTree->setSortingEnabled(true);
    m_violationsTree->setSelectionMode(QAbstractItemView::SingleSelection);
    
    // Professional column sizing
    m_violationsTree->setColumnWidth(0, 80);   // Severity
    m_violationsTree->setColumnWidth(1, 120);  // Category
    m_violationsTree->setColumnWidth(2, 300);  // Description
    m_violationsTree->setColumnWidth(3, 120);  // Standard
    m_violationsTree->setColumnWidth(4, 80);   // Time
    m_violationsTree->setColumnWidth(5, 60);   // Frame
    
    // Header styling
    QHeaderView* header = m_violationsTree->header();
    header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    header->setStretchLastSection(false);
    header->setSectionResizeMode(2, QHeaderView::Stretch); // Description stretches
    
    // Connect selection signal
    connect(m_violationsTree, &QTreeWidget::itemClicked,
            this, &ETSIComplianceMonitor::onViolationSelected);
}

void ETSIComplianceMonitor::setupControlPanel()
{
    // Filter controls
    auto* filtersLabel = new QLabel(tr("Filters:"));
    filtersLabel->setFont(QFont("Segoe UI", 8, QFont::Bold));
    m_controlLayout->addWidget(filtersLabel);
    
    // Severity filter
    auto* severityLabel = new QLabel(tr("Severity:"));
    severityLabel->setFont(QFont("Segoe UI", 8));
    m_controlLayout->addWidget(severityLabel);
    
    m_severityFilter = new QComboBox();
    m_severityFilter->addItems({tr("All"), tr("Info+"), tr("Warning+"), tr("Error+"), tr("Critical")});
    m_severityFilter->setCurrentIndex(0); // All
    connect(m_severityFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ETSIComplianceMonitor::onSeverityFilterChanged);
    m_controlLayout->addWidget(m_severityFilter);
    
    // Category filter
    auto* categoryLabel = new QLabel(tr("Category:"));
    categoryLabel->setFont(QFont("Segoe UI", 8));
    m_controlLayout->addWidget(categoryLabel);
    
    m_categoryFilter = new QComboBox();
    m_categoryFilter->addItems({tr("All"), tr("FIG Structure"), tr("Service Organization"), 
                               tr("Audio Quality"), tr("Data Integrity"), tr("Timing"), tr("Thai Standards")});
    m_categoryFilter->setCurrentIndex(0); // All
    connect(m_categoryFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ETSIComplianceMonitor::onCategoryFilterChanged);
    m_controlLayout->addWidget(m_categoryFilter);
    
    m_controlLayout->addStretch();
    
    // Action buttons
    m_refreshButton = new QPushButton(tr("Refresh"));
    m_refreshButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_BrowserReload));
    connect(m_refreshButton, &QPushButton::clicked, this, &ETSIComplianceMonitor::updateStatistics);
    m_controlLayout->addWidget(m_refreshButton);
    
    m_clearButton = new QPushButton(tr("Clear"));
    m_clearButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_TrashIcon));
    connect(m_clearButton, &QPushButton::clicked, this, &ETSIComplianceMonitor::onClearViolations);
    m_controlLayout->addWidget(m_clearButton);
    
    m_exportButton = new QPushButton(tr("Export"));
    m_exportButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton));
    connect(m_exportButton, &QPushButton::clicked, this, &ETSIComplianceMonitor::onExportViolations);
    m_controlLayout->addWidget(m_exportButton);
}

void ETSIComplianceMonitor::setupViolationDetails()
{
    m_violationDetails->setObjectName("ViolationDetails");
    m_violationDetails->setFont(m_detailFont);
    m_violationDetails->setPlainText(tr("Select a violation to view details..."));
}

void ETSIComplianceMonitor::connectEtiProcessor(EtiProcessor* processor)
{
    if (m_etiProcessor) {
        // Disconnect previous processor
        disconnect(m_etiProcessor, nullptr, this, nullptr);
    }
    
    m_etiProcessor = processor;
    
    if (m_etiProcessor) {
        // Connect to ETI processor signals for real-time compliance data
        // Note: These signals would need to be added to EtiProcessor
        Logger::instance().log(Logger::Info, "ETSIComplianceMonitor",
                              "Connected to ETI processor for real-time compliance monitoring");
    } else {
        Logger::instance().log(Logger::Warning, "ETSIComplianceMonitor",
                              "ETI processor disconnected from compliance monitor");
    }
}

void ETSIComplianceMonitor::startMonitoring()
{
    // Real violations are delivered asynchronously via
    // addViolation()/updateViolationStatus(); there is no polling timer and
    // no synthetic generation. This flag only reflects the monitoring state.
    if (!m_monitoringActive) {
        m_monitoringActive = true;
        Logger::instance().log(Logger::Info, "ETSIComplianceMonitor",
                              "Real-time compliance monitoring started");
    }
}

void ETSIComplianceMonitor::stopMonitoring()
{
    if (m_monitoringActive) {
        m_monitoringActive = false;
        Logger::instance().log(Logger::Info, "ETSIComplianceMonitor",
                              "Real-time compliance monitoring stopped");
    }
}

void ETSIComplianceMonitor::addViolation(const ETSIViolation& violation)
{
    if (!validateViolation(violation)) {
        Logger::instance().log(Logger::Warning, "ETSIComplianceMonitor",
                              "Invalid violation rejected");
        return;
    }
    
    QMutexLocker locker(&m_dataMutex);
    
    // Add or update violation
    m_violations[violation.violationId] = violation;
    
    // Create or update tree item
    QTreeWidgetItem* item = m_violationItems.value(violation.violationId, nullptr);
    if (!item) {
        item = createViolationItem(violation);
        m_violationItems[violation.violationId] = item;
        m_violationsTree->addTopLevelItem(item);
    } else {
        // Update existing item
        item->setText(0, violation.getSeverityString());
        item->setText(1, violation.getCategoryString());
        item->setText(2, violation.description);
        item->setText(3, violation.standardReference);
        item->setText(4, violation.timestamp.toString("hh:mm:ss"));
        item->setText(5, violation.frameContext);
        
        // Update colors
        QColor severityColor = getSeverityColor(violation.severity);
        item->setForeground(0, QBrush(severityColor));
        item->setIcon(0, getCategoryIcon(violation.category));
    }
    
    // Apply filters (tree-only; safe while the data lock is held).
    applyFilters();

    // Release the lock before updateStatistics(): it acquires the
    // non-recursive m_dataMutex itself, so calling it here would deadlock.
    locker.unlock();

    // Update statistics
    updateStatistics();

    // Emit signal for critical violations
    if (violation.severity == ETSIViolation::Critical) {
        emit criticalViolationDetected(violation);
    }
    
    Logger::instance().log(Logger::Debug, "ETSIComplianceMonitor",
                          QString("Added %1 violation: %2")
                          .arg(violation.getSeverityString())
                          .arg(violation.description));
}

void ETSIComplianceMonitor::updateViolationStatus(const QString& violationId, bool isActive)
{
    bool existed = false;
    {
        QMutexLocker locker(&m_dataMutex);

        if (m_violations.contains(violationId)) {
            existed = true;
            m_violations[violationId].isActive = isActive;

            // Update tree item appearance
            if (m_violationItems.contains(violationId)) {
                QTreeWidgetItem* item = m_violationItems[violationId];
                if (isActive) {
                    item->setFont(0, QFont("Segoe UI", 8, QFont::Bold));
                } else {
                    item->setFont(0, QFont("Segoe UI", 8, QFont::Normal));
                    // Dim inactive violations
                    for (int col = 0; col < item->columnCount(); ++col) {
                        QColor dimmedColor = item->foreground(col).color();
                        dimmedColor.setAlpha(128);
                        item->setForeground(col, QBrush(dimmedColor));
                    }
                }
            }
        }
    }

    // updateStatistics() takes m_dataMutex itself (see note in addViolation).
    if (existed) {
        updateStatistics();
    }
}

void ETSIComplianceMonitor::clearViolations()
{
    {
        QMutexLocker locker(&m_dataMutex);

        m_violations.clear();
        m_violationItems.clear();
        m_violationsTree->clear();
        m_violationDetails->clear();
    }

    // updateStatistics() takes m_dataMutex itself (see note in addViolation).
    updateStatistics();
    
    Logger::instance().log(Logger::Info, "ETSIComplianceMonitor", "All violations cleared");
}

void ETSIComplianceMonitor::updateStatistics()
{
    QMutexLocker locker(&m_dataMutex);
    
    // Reset statistics
    m_statistics = ComplianceStatistics();
    
    // Count violations by severity
    for (const auto& violation : m_violations) {
        if (!violation.isActive) continue;
        
        m_statistics.totalViolations++;
        
        switch (violation.severity) {
            case ETSIViolation::Critical:
                m_statistics.criticalViolations++;
                break;
            case ETSIViolation::Error:
                m_statistics.errorViolations++;
                break;
            case ETSIViolation::Warning:
                m_statistics.warningViolations++;
                break;
            case ETSIViolation::Info:
                m_statistics.infoViolations++;
                break;
        }
    }
    
    // Calculate compliance score
    double oldScore = m_statistics.overallScore;
    m_statistics.overallScore = calculateComplianceScore();
    m_statistics.lastUpdate = QDateTime::currentDateTime();
    
    locker.unlock();
    
    // Update UI
    updateComplianceOverview();
    
    // Emit score change signal if significant
    if (qAbs(m_statistics.overallScore - oldScore) > 1.0) {
        emit complianceScoreChanged(m_statistics.overallScore, oldScore);
    }
}

double ETSIComplianceMonitor::calculateComplianceScore() const
{
    if (m_statistics.totalViolations == 0) {
        return 100.0;
    }
    
    // Weight violations by severity for professional broadcast standards
    double penaltyPoints = 0.0;
    penaltyPoints += m_statistics.criticalViolations * 20.0;  // Critical: -20 points each
    penaltyPoints += m_statistics.errorViolations * 10.0;     // Error: -10 points each
    penaltyPoints += m_statistics.warningViolations * 3.0;    // Warning: -3 points each
    penaltyPoints += m_statistics.infoViolations * 0.5;       // Info: -0.5 points each
    
    double score = 100.0 - penaltyPoints;
    return qMax(0.0, qMin(100.0, score));
}

void ETSIComplianceMonitor::updateComplianceOverview()
{
    // Update score display
    m_scoreValueLabel->setText(QString("%1%").arg(m_statistics.overallScore, 0, 'f', 1));
    m_scoreProgressBar->setValue(static_cast<int>(m_statistics.overallScore));
    
    // Update score color and status
    QColor scoreColor;
    QString statusText;
    
    if (m_statistics.overallScore >= WARNING_COMPLIANCE_THRESHOLD) {
        scoreColor = STATUS_EXCELLENT;
        statusText = tr("EXCELLENT");
    } else if (m_statistics.overallScore >= CRITICAL_COMPLIANCE_THRESHOLD) {
        scoreColor = STATUS_GOOD;
        statusText = tr("GOOD");
    } else if (m_statistics.overallScore >= 70.0) {
        scoreColor = STATUS_FAIR;
        statusText = tr("FAIR");
    } else if (m_statistics.overallScore >= 50.0) {
        scoreColor = STATUS_POOR;
        statusText = tr("POOR");
    } else {
        scoreColor = SEVERITY_CRITICAL;
        statusText = tr("CRITICAL");
    }
    
    m_scoreProgressBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }").arg(scoreColor.name()));
    m_scoreValueLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(scoreColor.name()));
    m_statusLabel->setText(statusText);
    m_statusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(scoreColor.name()));
    
    // Update statistics counts
    m_totalViolationsLabel->setText(QString::number(m_statistics.totalViolations));
    m_criticalCountLabel->setText(QString::number(m_statistics.criticalViolations));
    m_errorCountLabel->setText(QString::number(m_statistics.errorViolations));
    m_warningCountLabel->setText(QString::number(m_statistics.warningViolations));
    m_infoCountLabel->setText(QString::number(m_statistics.infoViolations));
}

QTreeWidgetItem* ETSIComplianceMonitor::createViolationItem(const ETSIViolation& violation)
{
    auto* item = new QTreeWidgetItem();
    
    item->setText(0, violation.getSeverityString());
    item->setText(1, violation.getCategoryString());
    item->setText(2, violation.description);
    item->setText(3, violation.standardReference);
    item->setText(4, violation.timestamp.toString("hh:mm:ss"));
    item->setText(5, violation.frameContext);
    
    // Set severity color
    QColor severityColor = getSeverityColor(violation.severity);
    item->setForeground(0, QBrush(severityColor));
    
    // Set category icon
    item->setIcon(1, getCategoryIcon(violation.category));
    
    // Store violation data
    item->setData(0, Qt::UserRole, violation.violationId);
    
    return item;
}

QColor ETSIComplianceMonitor::getSeverityColor(ETSIViolation::Severity severity) const
{
    switch (severity) {
        case ETSIViolation::Critical: return SEVERITY_CRITICAL;
        case ETSIViolation::Error: return SEVERITY_ERROR;
        case ETSIViolation::Warning: return SEVERITY_WARNING;
        case ETSIViolation::Info: return SEVERITY_INFO;
        default: return TEXT_SECONDARY;
    }
}

QIcon ETSIComplianceMonitor::getCategoryIcon(ETSIViolation::Category category) const
{
    switch (category) {
        case ETSIViolation::FIG_Structure:
            return QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView);
        case ETSIViolation::Service_Organization:
            return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
        case ETSIViolation::Audio_Quality:
            return QApplication::style()->standardIcon(QStyle::SP_MediaVolume);
        case ETSIViolation::Data_Integrity:
            return QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
        case ETSIViolation::Timing_Compliance:
            return QApplication::style()->standardIcon(QStyle::SP_BrowserReload);
        case ETSIViolation::Thai_Standards:
            return QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation);
        default:
            return QApplication::style()->standardIcon(QStyle::SP_MessageBoxQuestion);
    }
}

bool ETSIComplianceMonitor::validateViolation(const ETSIViolation& violation) const
{
    return !violation.violationId.isEmpty() && 
           !violation.description.isEmpty() && 
           violation.timestamp.isValid();
}

void ETSIComplianceMonitor::applyFilters()
{
    for (int i = 0; i < m_violationsTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_violationsTree->topLevelItem(i);
        QString violationId = item->data(0, Qt::UserRole).toString();
        
        if (m_violations.contains(violationId)) {
            const ETSIViolation& violation = m_violations[violationId];
            
            bool showItem = true;
            
            // Apply severity filter
            if (!m_showAllSeverities) {
                showItem &= (violation.severity >= m_selectedSeverity);
            }
            
            // Apply category filter
            if (!m_showAllCategories) {
                showItem &= (violation.category == m_selectedCategory);
            }
            
            item->setHidden(!showItem);
        }
    }
}

void ETSIComplianceMonitor::onViolationSelected(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column)
    
    if (!item) return;
    
    QString violationId = item->data(0, Qt::UserRole).toString();
    
    if (m_violations.contains(violationId)) {
        const ETSIViolation& violation = m_violations[violationId];
        
        // Update details display
        QString details = QString(
            "Violation ID: %1\n"
            "Severity: %2\n"
            "Category: %3\n"
            "Standard Reference: %4\n"
            "Timestamp: %5\n"
            "Frame Context: %6\n"
            "Active: %7\n\n"
            "Description:\n%8"
        ).arg(violation.violationId)
         .arg(violation.getSeverityString())
         .arg(violation.getCategoryString())
         .arg(violation.standardReference)
         .arg(violation.timestamp.toString("yyyy-MM-dd hh:mm:ss"))
         .arg(violation.frameContext)
         .arg(violation.isActive ? "Yes" : "No")
         .arg(violation.description);
        
        m_violationDetails->setPlainText(details);
        
        emit violationSelected(violation);
    }
}

void ETSIComplianceMonitor::onClearViolations()
{
    int result = QMessageBox::question(this, tr("Clear Violations"),
                                      tr("Are you sure you want to clear all violations?"),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No);
    
    if (result == QMessageBox::Yes) {
        clearViolations();
    }
}

void ETSIComplianceMonitor::onExportViolations()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Export Violations"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/etsi_violations.csv",
        tr("CSV Files (*.csv);;Text Files (*.txt);;All Files (*)")
    );
    
    if (!fileName.isEmpty()) {
        exportViolations(fileName);
    }
}

bool ETSIComplianceMonitor::exportViolations(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export Failed"),
                            tr("Could not open file for writing: %1").arg(filename));
        return false;
    }
    
    QTextStream stream(&file);
    
    // Write CSV header
    stream << "Violation ID,Severity,Category,Description,Standard Reference,Timestamp,Frame Context,Active\n";
    
    // Write violations
    QMutexLocker locker(&m_dataMutex);
    for (const auto& violation : m_violations) {
        stream << QString("\"%1\",\"%2\",\"%3\",\"%4\",\"%5\",\"%6\",\"%7\",\"%8\"\n")
                  .arg(violation.violationId)
                  .arg(violation.getSeverityString())
                  .arg(violation.getCategoryString())
                  .arg(violation.description)
                  .arg(violation.standardReference)
                  .arg(violation.timestamp.toString("yyyy-MM-dd hh:mm:ss"))
                  .arg(violation.frameContext)
                  .arg(violation.isActive ? "Yes" : "No");
    }
    
    file.close();
    
    QMessageBox::information(this, tr("Export Successful"),
                            tr("Violations exported to: %1").arg(filename));
    
    Logger::instance().log(Logger::Info, "ETSIComplianceMonitor",
                          QString("Violations exported to: %1").arg(filename));
    
    return true;
}

void ETSIComplianceMonitor::onSeverityFilterChanged(int severityIndex)
{
    m_showAllSeverities = (severityIndex == 0);
    if (!m_showAllSeverities) {
        m_selectedSeverity = static_cast<ETSIViolation::Severity>(severityIndex - 1);
    }
    applyFilters();
}

void ETSIComplianceMonitor::onCategoryFilterChanged(int categoryIndex)
{
    m_showAllCategories = (categoryIndex == 0);
    if (!m_showAllCategories) {
        m_selectedCategory = static_cast<ETSIViolation::Category>(categoryIndex - 1);
    }
    applyFilters();
}

void ETSIComplianceMonitor::applyBroadcastingTheme()
{
    setStyleSheet(QString(
        "ETSIComplianceMonitor { background-color: %1; }"
        "QFrame#ComplianceOverviewFrame, QFrame#StatisticsFrame { "
        "    background-color: %2; "
        "    border: 1px solid %3; "
        "    border-radius: 4px; "
        "    margin: 2px; "
        "}"
        "QLabel { color: %4; }"
        "QTreeWidget#ViolationsTree { "
        "    background-color: %2; "
        "    border: 1px solid %3; "
        "    color: %4; "
        "    selection-background-color: %3; "
        "}"
        "QTextEdit#ViolationDetails { "
        "    background-color: %2; "
        "    border: 1px solid %3; "
        "    color: %4; "
        "}"
        "QProgressBar { "
        "    border: 1px solid %3; "
        "    border-radius: 3px; "
        "    background-color: %1; "
        "    text-align: center; "
        "}"
        "QProgressBar::chunk { "
        "    background-color: %5; "
        "    border-radius: 2px; "
        "}"
        "QPushButton { "
        "    background-color: %2; "
        "    border: 1px solid %3; "
        "    border-radius: 3px; "
        "    padding: 4px 8px; "
        "    color: %4; "
        "}"
        "QPushButton:hover { "
        "    background-color: %3; "
        "}"
        "QComboBox { "
        "    background-color: %2; "
        "    border: 1px solid %3; "
        "    border-radius: 3px; "
        "    padding: 2px 4px; "
        "    color: %4; "
        "}"
    ).arg(BACKGROUND_DARK.name())
     .arg(BACKGROUND_MEDIUM.name())
     .arg(ACCENT_BLUE.name())
     .arg(TEXT_PRIMARY.name())
     .arg(STATUS_GOOD.name()));
}

// Missing method implementations

void ETSIComplianceMonitor::onEtiComplianceUpdate(quint64 frameNumber, const QList<ETSIViolation>& violations) {
    Logger::instance().log(Logger::Debug, "ETSIComplianceMonitor", 
        QString("onEtiComplianceUpdate: frame %1, violations: %2").arg(frameNumber).arg(violations.size()));
    
    // Process each violation from the ETI frame analysis
    for (const auto& violation : violations) {
        // Create frame-specific violation with context
        ETSIViolation frameViolation = violation;
        frameViolation.frameContext = QString("Frame %1").arg(frameNumber);
        frameViolation.timestamp = QDateTime::currentDateTime();
        
        // Generate unique violation ID including frame number
        frameViolation.violationId = QString("ETI_%1_%2_%3")
            .arg(frameNumber)
            .arg(static_cast<int>(violation.category))
            .arg(static_cast<int>(violation.severity));
        
        // Add violation to monitoring system
        addViolation(frameViolation);
        
        Logger::instance().log(Logger::Debug, "ETSIComplianceMonitor",
            QString("Added ETI frame %1 violation: %2 (%3)")
            .arg(frameNumber)
            .arg(violation.description)
            .arg(violation.getSeverityString()));
    }
    
    // Update compliance score based on violations
    double oldScore = m_statistics.overallScore;
    m_statistics.overallScore = calculateComplianceScore();
    
    // Emit signal for UI updates if critical violations detected
    bool hasCritical = std::any_of(violations.begin(), violations.end(),
        [](const ETSIViolation& v) { return v.severity == ETSIViolation::Critical; });
    
    if (hasCritical) {
        emit complianceScoreChanged(m_statistics.overallScore, oldScore);
    }
}

void ETSIComplianceMonitor::filterByCategory(ETSIViolation::Category category) {
    Logger::instance().log(Logger::Debug, "ETSIComplianceMonitor", 
        QString("filterByCategory: %1").arg(static_cast<int>(category)));
    
    // Update category filter state
    m_selectedCategory = category;
    m_showAllCategories = false;
    
    // Update category filter combo box to reflect selection
    if (m_categoryFilter) {
        m_categoryFilter->setCurrentIndex(static_cast<int>(category) + 1); // +1 for "All Categories" option
    }
    
    // Apply filters to violation tree
    applyFilters();
    
    Logger::instance().log(Logger::Info, "ETSIComplianceMonitor",
        QString("Filtered violations by category: %1").arg(static_cast<int>(category)));
}

void ETSIComplianceMonitor::filterBySeverity(ETSIViolation::Severity severity) {
    Logger::instance().log(Logger::Debug, "ETSIComplianceMonitor", 
        QString("filterBySeverity: %1").arg(static_cast<int>(severity)));
    
    // Update severity filter state
    m_selectedSeverity = severity;
    m_showAllSeverities = false;
    
    // Update severity filter combo box to reflect selection
    if (m_severityFilter) {
        m_severityFilter->setCurrentIndex(static_cast<int>(severity) + 1); // +1 for "All Severities" option
    }
    
    // Apply filters to violation tree
    applyFilters();
    
    Logger::instance().log(Logger::Info, "ETSIComplianceMonitor",
        QString("Filtered violations by severity: %1").arg(static_cast<int>(severity)));
}