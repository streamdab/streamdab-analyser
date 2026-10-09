#include "performance_dashboard.h"
#include "core/eti_processor.hpp"
#include "utils/logger.h"
#include <QApplication>
#include <QStyle>
#include <QStyleOption>
#include <QDebug>
#include <QMutexLocker>
#include <QProcess>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <algorithm>

// Forward declaration for internal chart widget
class PerformanceChartWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit PerformanceChartWidget(PerformanceDashboard* dashboard, QWidget* parent = nullptr)
        : QWidget(parent), m_dashboard(dashboard)
    {
        setMinimumSize(200, 100);
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        setMouseTracking(true);
    }
    
    void setPerformanceData(const std::deque<PerformanceStats>& data) {
        m_performanceData = data;
        update();
    }
    
protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)
        
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        
        // Professional background
        painter.fillRect(rect(), PerformanceDashboard::BACKGROUND_DARK);
        
        if (m_performanceData.empty() || !m_dashboard) {
            // Draw "No Data" message
            painter.setPen(PerformanceDashboard::TEXT_SECONDARY);
            painter.setFont(QFont("Segoe UI", 10, QFont::Bold));
            painter.drawText(rect(), Qt::AlignCenter, tr("No Performance Data\nMonitoring Not Active"));
            return;
        }
        
        // Get chart area with margins for professional appearance
        QRect chartRect = rect().adjusted(40, 20, -20, -30);
        if (chartRect.width() < 100 || chartRect.height() < 50) return;
        
        // Draw professional grid
        drawGrid(painter, chartRect);
        
        // Draw performance metrics charts
        drawFPSChart(painter, chartRect);
        drawMemoryChart(painter, chartRect);
        drawLatencyChart(painter, chartRect);
        
        // Draw legend
        drawLegend(painter);
        
        // Draw professional axes labels
        drawAxesLabels(painter, chartRect);
    }
    
    void mouseMoveEvent(QMouseEvent* event) override
    {
        // Professional tooltip on hover (future enhancement)
        Q_UNUSED(event)
        QWidget::mouseMoveEvent(event);
    }
    
private:
    void drawGrid(QPainter& painter, const QRect& chartRect)
    {
        painter.setPen(QPen(PerformanceDashboard::BACKGROUND_MEDIUM, 1, Qt::DotLine));
        
        // Vertical grid lines (time intervals)
        int numVerticalLines = 10;
        for (int i = 0; i <= numVerticalLines; ++i) {
            int x = chartRect.left() + (chartRect.width() * i / numVerticalLines);
            painter.drawLine(x, chartRect.top(), x, chartRect.bottom());
        }
        
        // Horizontal grid lines (performance levels)
        int numHorizontalLines = 5;
        for (int i = 0; i <= numHorizontalLines; ++i) {
            int y = chartRect.top() + (chartRect.height() * i / numHorizontalLines);
            painter.drawLine(chartRect.left(), y, chartRect.right(), y);
        }
    }
    
    void drawFPSChart(QPainter& painter, const QRect& chartRect)
    {
        if (m_performanceData.size() < 2) return;
        
        painter.setPen(QPen(PerformanceDashboard::STATUS_EXCELLENT, 2));
        painter.setBrush(Qt::NoBrush);
        
        QPainterPath fpsPath;
        bool firstPoint = true;
        
        int dataSize = static_cast<int>(m_performanceData.size());
        double maxFPS = m_dashboard->getTargetFPS();
        
        for (int i = 0; i < dataSize; ++i) {
            double x = chartRect.left() + (chartRect.width() * i / (dataSize - 1));
            double normalizedFPS = qBound(0.0, m_performanceData[i].frameRate / maxFPS, 1.0);
            double y = chartRect.bottom() - (normalizedFPS * chartRect.height());
            
            if (firstPoint) {
                fpsPath.moveTo(x, y);
                firstPoint = false;
            } else {
                fpsPath.lineTo(x, y);
            }
        }
        
        painter.drawPath(fpsPath);
    }
    
    void drawMemoryChart(QPainter& painter, const QRect& chartRect)
    {
        if (m_performanceData.size() < 2) return;
        
        painter.setPen(QPen(PerformanceDashboard::STATUS_FAIR, 2));
        painter.setBrush(Qt::NoBrush);
        
        QPainterPath memoryPath;
        bool firstPoint = true;
        
        int dataSize = static_cast<int>(m_performanceData.size());
        double maxMemory = m_dashboard->getTargetMemoryMB();
        
        for (int i = 0; i < dataSize; ++i) {
            double x = chartRect.left() + (chartRect.width() * i / (dataSize - 1));
            double normalizedMemory = qBound(0.0, m_performanceData[i].memoryUsage / maxMemory, 1.0);
            double y = chartRect.bottom() - (normalizedMemory * chartRect.height());
            
            if (firstPoint) {
                memoryPath.moveTo(x, y);
                firstPoint = false;
            } else {
                memoryPath.lineTo(x, y);
            }
        }
        
        painter.drawPath(memoryPath);
    }
    
    void drawLatencyChart(QPainter& painter, const QRect& chartRect)
    {
        if (m_performanceData.size() < 2) return;
        
        painter.setPen(QPen(PerformanceDashboard::ACCENT_BLUE, 2));
        painter.setBrush(Qt::NoBrush);
        
        QPainterPath latencyPath;
        bool firstPoint = true;
        
        int dataSize = static_cast<int>(m_performanceData.size());
        double maxLatency = m_dashboard->getTargetLatencyMS();
        
        for (int i = 0; i < dataSize; ++i) {
            double x = chartRect.left() + (chartRect.width() * i / (dataSize - 1));
            double normalizedLatency = qBound(0.0, m_performanceData[i].averageLatency / maxLatency, 1.0);
            double y = chartRect.bottom() - (normalizedLatency * chartRect.height());
            
            if (firstPoint) {
                latencyPath.moveTo(x, y);
                firstPoint = false;
            } else {
                latencyPath.lineTo(x, y);
            }
        }
        
        painter.drawPath(latencyPath);
    }
    
    void drawLegend(QPainter& painter)
    {
        // Professional legend in top-right corner
        QRect legendRect(width() - 120, 10, 110, 60);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(QColor(45, 45, 48, 200)));
        painter.drawRoundedRect(legendRect, 3, 3);
        
        painter.setPen(PerformanceDashboard::TEXT_PRIMARY);
        painter.setFont(QFont("Segoe UI", 8));
        
        // Legend items
        int y = legendRect.top() + 12;
        painter.setPen(PerformanceDashboard::STATUS_EXCELLENT);
        painter.drawText(legendRect.left() + 20, y, tr("FPS"));
        painter.drawLine(legendRect.left() + 5, y - 3, legendRect.left() + 15, y - 3);
        
        y += 15;
        painter.setPen(PerformanceDashboard::STATUS_FAIR);
        painter.drawText(legendRect.left() + 20, y, tr("Memory"));
        painter.drawLine(legendRect.left() + 5, y - 3, legendRect.left() + 15, y - 3);
        
        y += 15;
        painter.setPen(PerformanceDashboard::ACCENT_BLUE);
        painter.drawText(legendRect.left() + 20, y, tr("Latency"));
        painter.drawLine(legendRect.left() + 5, y - 3, legendRect.left() + 15, y - 3);
    }
    
    void drawAxesLabels(QPainter& painter, const QRect& chartRect)
    {
        painter.setPen(PerformanceDashboard::TEXT_SECONDARY);
        painter.setFont(QFont("Segoe UI", 8));
        
        // Y-axis label (Performance %)
        painter.save();
        painter.translate(15, chartRect.center().y());
        painter.rotate(-90);
        painter.drawText(-30, 0, tr("Performance %"));
        painter.restore();
        
        // X-axis label (Time)
        painter.drawText(chartRect.center().x() - 15, height() - 5, tr("Time"));
        
        // Y-axis scale
        painter.drawText(5, chartRect.top() + 5, tr("100%"));
        painter.drawText(5, chartRect.bottom() + 5, tr("0%"));
    }
    
private:
    PerformanceDashboard* m_dashboard;
    std::deque<PerformanceStats> m_performanceData;
};

// Professional broadcast industry color definitions
const QColor PerformanceDashboard::BACKGROUND_DARK = QColor(45, 45, 48);     // #2D2D30
const QColor PerformanceDashboard::BACKGROUND_MEDIUM = QColor(62, 62, 66);   // #3E3E42
const QColor PerformanceDashboard::ACCENT_BLUE = QColor(0, 120, 212);        // #0078D4
const QColor PerformanceDashboard::STATUS_EXCELLENT = QColor(0, 170, 0);     // #00AA00
const QColor PerformanceDashboard::STATUS_GOOD = QColor(76, 175, 80);        // #4CAF50
const QColor PerformanceDashboard::STATUS_FAIR = QColor(255, 193, 7);        // #FFC107
const QColor PerformanceDashboard::STATUS_POOR = QColor(255, 152, 0);        // #FF9800
const QColor PerformanceDashboard::STATUS_CRITICAL = QColor(220, 53, 69);    // #DC3545
const QColor PerformanceDashboard::TEXT_PRIMARY = QColor(255, 255, 255);     // #FFFFFF
const QColor PerformanceDashboard::TEXT_SECONDARY = QColor(204, 204, 204);   // #CCCCCC

PerformanceDashboard::PerformanceDashboard(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_metricsLayout(nullptr)
    , m_complianceLayout(nullptr)
    , m_fpsFrame(nullptr)
    , m_fpsLabel(nullptr)
    , m_fpsValueLabel(nullptr)
    , m_fpsProgressBar(nullptr)
    , m_fpsStatusLabel(nullptr)
    , m_memoryFrame(nullptr)
    , m_memoryLabel(nullptr)
    , m_memoryValueLabel(nullptr)
    , m_memoryProgressBar(nullptr)
    , m_memoryStatusLabel(nullptr)
    , m_cpuFrame(nullptr)
    , m_cpuLabel(nullptr)
    , m_cpuValueLabel(nullptr)
    , m_cpuProgressBar(nullptr)
    , m_cpuStatusLabel(nullptr)
    , m_latencyFrame(nullptr)
    , m_latencyLabel(nullptr)
    , m_latencyValueLabel(nullptr)
    , m_latencyProgressBar(nullptr)
    , m_latencyStatusLabel(nullptr)
    , m_complianceFrame(nullptr)
    , m_complianceLabel(nullptr)
    , m_complianceScoreLabel(nullptr)
    , m_complianceProgressBar(nullptr)
    , m_violationsDisplay(nullptr)
    , m_etiProcessor(nullptr)
    , m_updateTimer(nullptr)
    , m_monitoringActive(false)
    , m_targetFPS(DEFAULT_TARGET_FPS)
    , m_targetMemoryMB(DEFAULT_TARGET_MEMORY_MB)
    , m_targetCPU(DEFAULT_TARGET_CPU)
    , m_targetLatencyMS(DEFAULT_TARGET_LATENCY_MS)
{
    setupUI();
    setupPerformanceIndicators();
    setupComplianceMonitor();
    setupTrendCharts();
    applyBroadcastingTheme();
    
    // Initialize timer
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(UPDATE_INTERVAL_MS);
    connect(m_updateTimer, &QTimer::timeout, this, &PerformanceDashboard::onUpdateTimer);
    
    Logger::instance().log(Logger::Info, "PerformanceDashboard", 
                          "Professional Performance Dashboard initialized");
}

PerformanceDashboard::~PerformanceDashboard()
{
    stopMonitoring();
}

void PerformanceDashboard::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(8);
    
    // Create main metrics grid
    m_metricsLayout = new QGridLayout();
    m_metricsLayout->setSpacing(8);
    
    // Create compliance layout
    m_complianceLayout = new QHBoxLayout();
    m_complianceLayout->setSpacing(8);
    
    // Professional fonts
    m_headerFont = QFont("Segoe UI", 9, QFont::Bold);
    m_valueFont = QFont("Consolas", 11, QFont::Bold);
    m_statusFont = QFont("Segoe UI", 8, QFont::Normal);
}

void PerformanceDashboard::setupPerformanceIndicators()
{
    // FPS Performance Indicator
    m_fpsFrame = new QFrame();
    m_fpsFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_fpsFrame->setObjectName("PerformanceMetricFrame");
    
    auto* fpsLayout = new QVBoxLayout(m_fpsFrame);
    fpsLayout->setContentsMargins(8, 6, 8, 6);
    fpsLayout->setSpacing(4);
    
    m_fpsLabel = new QLabel(tr("Processing Rate"));
    m_fpsLabel->setFont(m_headerFont);
    m_fpsLabel->setAlignment(Qt::AlignCenter);
    fpsLayout->addWidget(m_fpsLabel);
    
    m_fpsValueLabel = new QLabel("0 FPS");
    m_fpsValueLabel->setFont(m_valueFont);
    m_fpsValueLabel->setAlignment(Qt::AlignCenter);
    fpsLayout->addWidget(m_fpsValueLabel);
    
    m_fpsProgressBar = new QProgressBar();
    m_fpsProgressBar->setRange(0, static_cast<int>(m_targetFPS));
    m_fpsProgressBar->setValue(0);
    m_fpsProgressBar->setTextVisible(false);
    fpsLayout->addWidget(m_fpsProgressBar);
    
    m_fpsStatusLabel = new QLabel(tr("WAITING"));
    m_fpsStatusLabel->setFont(m_statusFont);
    m_fpsStatusLabel->setAlignment(Qt::AlignCenter);
    fpsLayout->addWidget(m_fpsStatusLabel);
    
    m_metricsLayout->addWidget(m_fpsFrame, 0, 0);
    
    // Memory Usage Indicator
    m_memoryFrame = new QFrame();
    m_memoryFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_memoryFrame->setObjectName("PerformanceMetricFrame");
    
    auto* memoryLayout = new QVBoxLayout(m_memoryFrame);
    memoryLayout->setContentsMargins(8, 6, 8, 6);
    memoryLayout->setSpacing(4);
    
    m_memoryLabel = new QLabel(tr("Memory Usage"));
    m_memoryLabel->setFont(m_headerFont);
    m_memoryLabel->setAlignment(Qt::AlignCenter);
    memoryLayout->addWidget(m_memoryLabel);
    
    m_memoryValueLabel = new QLabel("0 MB");
    m_memoryValueLabel->setFont(m_valueFont);
    m_memoryValueLabel->setAlignment(Qt::AlignCenter);
    memoryLayout->addWidget(m_memoryValueLabel);
    
    m_memoryProgressBar = new QProgressBar();
    m_memoryProgressBar->setRange(0, static_cast<int>(m_targetMemoryMB));
    m_memoryProgressBar->setValue(0);
    m_memoryProgressBar->setTextVisible(false);
    memoryLayout->addWidget(m_memoryProgressBar);
    
    m_memoryStatusLabel = new QLabel(tr("OPTIMAL"));
    m_memoryStatusLabel->setFont(m_statusFont);
    m_memoryStatusLabel->setAlignment(Qt::AlignCenter);
    memoryLayout->addWidget(m_memoryStatusLabel);
    
    m_metricsLayout->addWidget(m_memoryFrame, 0, 1);
    
    // CPU Usage Indicator
    m_cpuFrame = new QFrame();
    m_cpuFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_cpuFrame->setObjectName("PerformanceMetricFrame");
    
    auto* cpuLayout = new QVBoxLayout(m_cpuFrame);
    cpuLayout->setContentsMargins(8, 6, 8, 6);
    cpuLayout->setSpacing(4);
    
    m_cpuLabel = new QLabel(tr("CPU Load"));
    m_cpuLabel->setFont(m_headerFont);
    m_cpuLabel->setAlignment(Qt::AlignCenter);
    cpuLayout->addWidget(m_cpuLabel);
    
    m_cpuValueLabel = new QLabel("0%");
    m_cpuValueLabel->setFont(m_valueFont);
    m_cpuValueLabel->setAlignment(Qt::AlignCenter);
    cpuLayout->addWidget(m_cpuValueLabel);
    
    m_cpuProgressBar = new QProgressBar();
    m_cpuProgressBar->setRange(0, 100);
    m_cpuProgressBar->setValue(0);
    m_cpuProgressBar->setTextVisible(false);
    cpuLayout->addWidget(m_cpuProgressBar);
    
    m_cpuStatusLabel = new QLabel(tr("NORMAL"));
    m_cpuStatusLabel->setFont(m_statusFont);
    m_cpuStatusLabel->setAlignment(Qt::AlignCenter);
    cpuLayout->addWidget(m_cpuStatusLabel);
    
    m_metricsLayout->addWidget(m_cpuFrame, 0, 2);
    
    // Latency Indicator
    m_latencyFrame = new QFrame();
    m_latencyFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_latencyFrame->setObjectName("PerformanceMetricFrame");
    
    auto* latencyLayout = new QVBoxLayout(m_latencyFrame);
    latencyLayout->setContentsMargins(8, 6, 8, 6);
    latencyLayout->setSpacing(4);
    
    m_latencyLabel = new QLabel(tr("Latency"));
    m_latencyLabel->setFont(m_headerFont);
    m_latencyLabel->setAlignment(Qt::AlignCenter);
    latencyLayout->addWidget(m_latencyLabel);
    
    m_latencyValueLabel = new QLabel("0 ms");
    m_latencyValueLabel->setFont(m_valueFont);
    m_latencyValueLabel->setAlignment(Qt::AlignCenter);
    latencyLayout->addWidget(m_latencyValueLabel);
    
    m_latencyProgressBar = new QProgressBar();
    m_latencyProgressBar->setRange(0, static_cast<int>(m_targetLatencyMS));
    m_latencyProgressBar->setValue(0);
    m_latencyProgressBar->setTextVisible(false);
    latencyLayout->addWidget(m_latencyProgressBar);
    
    m_latencyStatusLabel = new QLabel(tr("EXCELLENT"));
    m_latencyStatusLabel->setFont(m_statusFont);
    m_latencyStatusLabel->setAlignment(Qt::AlignCenter);
    latencyLayout->addWidget(m_latencyStatusLabel);
    
    m_metricsLayout->addWidget(m_latencyFrame, 0, 3);
    
    m_mainLayout->addLayout(m_metricsLayout);
}

void PerformanceDashboard::setupComplianceMonitor()
{
    // ETSI Compliance Monitor Frame
    m_complianceFrame = new QFrame();
    m_complianceFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_complianceFrame->setObjectName("ComplianceFrame");
    
    auto* complianceLayout = new QVBoxLayout(m_complianceFrame);
    complianceLayout->setContentsMargins(8, 6, 8, 6);
    complianceLayout->setSpacing(4);
    
    // Compliance header
    auto* complianceHeaderLayout = new QHBoxLayout();
    
    m_complianceLabel = new QLabel(tr("ETSI Compliance"));
    m_complianceLabel->setFont(m_headerFont);
    complianceHeaderLayout->addWidget(m_complianceLabel);
    
    complianceHeaderLayout->addStretch();
    
    m_complianceScoreLabel = new QLabel("100%");
    m_complianceScoreLabel->setFont(m_valueFont);
    complianceHeaderLayout->addWidget(m_complianceScoreLabel);
    
    complianceLayout->addLayout(complianceHeaderLayout);
    
    // Compliance progress bar
    m_complianceProgressBar = new QProgressBar();
    m_complianceProgressBar->setRange(0, 100);
    m_complianceProgressBar->setValue(100);
    m_complianceProgressBar->setTextVisible(false);
    complianceLayout->addWidget(m_complianceProgressBar);
    
    // Violations display
    m_violationsDisplay = new QTextEdit();
    m_violationsDisplay->setMaximumHeight(80);
    m_violationsDisplay->setReadOnly(true);
    m_violationsDisplay->setPlainText(tr("No compliance violations detected"));
    m_violationsDisplay->setFont(QFont("Consolas", 8));
    complianceLayout->addWidget(m_violationsDisplay);
    
    m_mainLayout->addWidget(m_complianceFrame);
}

void PerformanceDashboard::setupTrendCharts()
{
    // Professional real-time trend charts for broadcast monitoring
    
    // Create trend chart container
    m_trendChartsFrame = new QFrame();
    m_trendChartsFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_trendChartsFrame->setObjectName("TrendChartsFrame");
    m_trendChartsFrame->setMinimumHeight(180);
    
    auto* trendLayout = new QVBoxLayout(m_trendChartsFrame);
    trendLayout->setContentsMargins(8, 6, 8, 6);
    trendLayout->setSpacing(4);
    
    // Trend charts header
    auto* trendHeaderLayout = new QHBoxLayout();
    
    auto* trendLabel = new QLabel(tr("Performance Trends"));
    trendLabel->setFont(m_headerFont);
    trendHeaderLayout->addWidget(trendLabel);
    
    trendHeaderLayout->addStretch();
    
    // Time range selector for professional analysis
    m_timeRangeCombo = new QComboBox();
    m_timeRangeCombo->addItem(tr("Last 1 Min"), 60);
    m_timeRangeCombo->addItem(tr("Last 5 Min"), 300);
    m_timeRangeCombo->addItem(tr("Last 15 Min"), 900);
    m_timeRangeCombo->addItem(tr("Last 30 Min"), 1800);
    m_timeRangeCombo->setCurrentIndex(1); // Default to 5 minutes
    trendHeaderLayout->addWidget(m_timeRangeCombo);
    
    trendLayout->addLayout(trendHeaderLayout);
    
    // Chart display area with custom rendering
    m_chartDisplayArea = new PerformanceChartWidget(this);
    m_chartDisplayArea->setMinimumHeight(140);
    m_chartDisplayArea->setStyleSheet(QString(
        "background-color: %1; "
        "border: 1px solid %2; "
        "border-radius: 3px;"
    ).arg(BACKGROUND_DARK.name()).arg(ACCENT_BLUE.name()));
    
    trendLayout->addWidget(m_chartDisplayArea);
    
    // Add to main layout
    m_mainLayout->addWidget(m_trendChartsFrame);
    
    // Connect time range selector
    connect(m_timeRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PerformanceDashboard::onTimeRangeChanged);
}

void PerformanceDashboard::connectEtiProcessor(EtiProcessor* processor)
{
    if (m_etiProcessor) {
        // Disconnect previous processor
        disconnect(m_etiProcessor, nullptr, this, nullptr);
    }
    
    m_etiProcessor = processor;
    
    if (m_etiProcessor) {
        // NOTE: the GUI runs on EnhancedETIProcessorQt, not this legacy
        // EtiProcessor, and the old frameProcessed(quint64,QByteArray) signal
        // does not exist. The dashboard is fed real FPS / memory / CPU /
        // latency samples from DABAnalyserWindow via updatePerformanceMetrics();
        // this hook only records the association (kept for API stability).
        Logger::instance().log(Logger::Info, "PerformanceDashboard",
                              "Connected to ETI processor for real-time performance monitoring");
    } else {
        Logger::instance().log(Logger::Warning, "PerformanceDashboard",
                              "ETI processor disconnected from performance dashboard");
    }
}

void PerformanceDashboard::startMonitoring()
{
    if (!m_monitoringActive) {
        m_monitoringActive = true;
        m_updateTimer->start();
        
        Logger::instance().log(Logger::Info, "PerformanceDashboard",
                              "Real-time performance monitoring started");
    }
}

void PerformanceDashboard::stopMonitoring()
{
    if (m_monitoringActive) {
        m_monitoringActive = false;
        m_updateTimer->stop();
        
        Logger::instance().log(Logger::Info, "PerformanceDashboard",
                              "Real-time performance monitoring stopped");
    }
}

void PerformanceDashboard::updatePerformanceMetrics(const PerformanceStats& stats)
{
    QMutexLocker locker(&m_dataMutex);
    
    m_currentStats = stats;
    
    // Update UI components
    updateFPSDisplay(stats.frameRate);
    updateMemoryDisplay(stats.memoryUsage);
    
    // Update CPU usage
    m_cpuValueLabel->setText(formatPerformanceValue(stats.cpuUsage, "%", 1));
    m_cpuProgressBar->setValue(static_cast<int>(stats.cpuUsage));
    
    QColor cpuColor = getPerformanceStatusColor(stats.cpuUsage, m_targetCPU, true);
    m_cpuProgressBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }").arg(cpuColor.name()));
    
    if (stats.cpuUsage > m_targetCPU * 0.9) {
        m_cpuStatusLabel->setText(tr("HIGH"));
        m_cpuStatusLabel->setStyleSheet(QString("color: %1;").arg(STATUS_POOR.name()));
    } else if (stats.cpuUsage > m_targetCPU * 0.7) {
        m_cpuStatusLabel->setText(tr("MODERATE"));
        m_cpuStatusLabel->setStyleSheet(QString("color: %1;").arg(STATUS_FAIR.name()));
    } else {
        m_cpuStatusLabel->setText(tr("NORMAL"));
        m_cpuStatusLabel->setStyleSheet(QString("color: %1;").arg(STATUS_EXCELLENT.name()));
    }
    
    // Update latency
    m_latencyValueLabel->setText(formatPerformanceValue(stats.averageLatency, " ms", 1));
    m_latencyProgressBar->setValue(static_cast<int>(stats.averageLatency));
    
    QColor latencyColor = getPerformanceStatusColor(stats.averageLatency, m_targetLatencyMS, true);
    m_latencyProgressBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }").arg(latencyColor.name()));
    
    if (stats.averageLatency > m_targetLatencyMS) {
        m_latencyStatusLabel->setText(tr("HIGH"));
        m_latencyStatusLabel->setStyleSheet(QString("color: %1;").arg(STATUS_POOR.name()));
    } else if (stats.averageLatency > m_targetLatencyMS * 0.7) {
        m_latencyStatusLabel->setText(tr("GOOD"));
        m_latencyStatusLabel->setStyleSheet(QString("color: %1;").arg(STATUS_GOOD.name()));
    } else {
        m_latencyStatusLabel->setText(tr("EXCELLENT"));
        m_latencyStatusLabel->setStyleSheet(QString("color: %1;").arg(STATUS_EXCELLENT.name()));
    }
    
    // Release the lock before the helpers below: updateHistoricalData()
    // acquires the non-recursive m_dataMutex itself, so calling it while
    // holding the lock here would deadlock.
    locker.unlock();

    // Update historical data
    updateHistoricalData();
    
    // Check thresholds
    checkPerformanceThresholds();
}

void PerformanceDashboard::updateComplianceStatus(const ComplianceResult& compliance)
{
    QMutexLocker locker(&m_dataMutex);
    
    m_currentCompliance = compliance;
    updateComplianceDisplay(compliance.compliancePercentage);
    
    // Update violations display
    if (compliance.activeViolations.isEmpty()) {
        m_violationsDisplay->setPlainText(tr("No compliance violations detected"));
        m_violationsDisplay->setStyleSheet("background-color: #1E2E1E; color: #00AA00;"); // Dark green background
    } else {
        QString violationsText = tr("Active Violations:\n");
        for (const QString& violation : compliance.activeViolations) {
            violationsText += "• " + violation + "\n";
        }
        m_violationsDisplay->setPlainText(violationsText);
        m_violationsDisplay->setStyleSheet("background-color: #2E1E1E; color: #FF9800;"); // Dark orange background
    }
}

void PerformanceDashboard::updateFPSDisplay(double fps)
{
    m_fpsValueLabel->setText(formatPerformanceValue(fps, " FPS", 1));
    
    int progressValue = static_cast<int>(qMin(fps, m_targetFPS));
    m_fpsProgressBar->setValue(progressValue);
    
    // Professional color coding based on broadcast industry standards
    QColor fpsColor = getPerformanceStatusColor(fps, m_targetFPS, false);
    m_fpsProgressBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }").arg(fpsColor.name()));
    
    // Status text with professional broadcast terminology
    if (fps >= m_targetFPS) {
        m_fpsStatusLabel->setText(tr("EXCELLENT"));
        m_fpsStatusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(STATUS_EXCELLENT.name()));
    } else if (fps >= m_targetFPS * 0.8) {
        m_fpsStatusLabel->setText(tr("GOOD"));
        m_fpsStatusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(STATUS_GOOD.name()));
    } else if (fps >= m_targetFPS * 0.5) {
        m_fpsStatusLabel->setText(tr("FAIR"));
        m_fpsStatusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(STATUS_FAIR.name()));
    } else if (fps >= m_targetFPS * 0.25) {
        m_fpsStatusLabel->setText(tr("POOR"));
        m_fpsStatusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(STATUS_POOR.name()));
    } else {
        m_fpsStatusLabel->setText(tr("CRITICAL"));
        m_fpsStatusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(STATUS_CRITICAL.name()));
    }
}

void PerformanceDashboard::updateMemoryDisplay(double memoryMB)
{
    m_memoryValueLabel->setText(formatPerformanceValue(memoryMB, " MB", 1));
    
    int progressValue = static_cast<int>(qMin(memoryMB, m_targetMemoryMB));
    m_memoryProgressBar->setValue(progressValue);
    
    // Memory is inverted - lower values are better
    QColor memoryColor = getPerformanceStatusColor(memoryMB, m_targetMemoryMB, true);
    m_memoryProgressBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }").arg(memoryColor.name()));
    
    // Status text with memory-specific terminology
    if (memoryMB <= m_targetMemoryMB * 0.5) {
        m_memoryStatusLabel->setText(tr("OPTIMAL"));
        m_memoryStatusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(STATUS_EXCELLENT.name()));
    } else if (memoryMB <= m_targetMemoryMB * 0.7) {
        m_memoryStatusLabel->setText(tr("GOOD"));
        m_memoryStatusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(STATUS_GOOD.name()));
    } else if (memoryMB <= m_targetMemoryMB * 0.9) {
        m_memoryStatusLabel->setText(tr("MODERATE"));
        m_memoryStatusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(STATUS_FAIR.name()));
    } else if (memoryMB <= m_targetMemoryMB) {
        m_memoryStatusLabel->setText(tr("HIGH"));
        m_memoryStatusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(STATUS_POOR.name()));
    } else {
        m_memoryStatusLabel->setText(tr("CRITICAL"));
        m_memoryStatusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(STATUS_CRITICAL.name()));
    }
}

void PerformanceDashboard::updateComplianceDisplay(double compliancePercentage)
{
    m_complianceScoreLabel->setText(QString("%1%").arg(compliancePercentage, 0, 'f', 1));
    m_complianceProgressBar->setValue(static_cast<int>(compliancePercentage));
    
    // Color coding for compliance
    QColor complianceColor;
    if (compliancePercentage >= 95.0) {
        complianceColor = STATUS_EXCELLENT;
    } else if (compliancePercentage >= 85.0) {
        complianceColor = STATUS_GOOD;
    } else if (compliancePercentage >= 70.0) {
        complianceColor = STATUS_FAIR;
    } else if (compliancePercentage >= 50.0) {
        complianceColor = STATUS_POOR;
    } else {
        complianceColor = STATUS_CRITICAL;
    }
    
    m_complianceProgressBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }").arg(complianceColor.name()));
    m_complianceScoreLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(complianceColor.name()));
}

QColor PerformanceDashboard::getPerformanceStatusColor(double value, double threshold, bool invert) const
{
    double ratio = value / threshold;
    
    if (invert) {
        // For metrics where lower is better (memory, latency, CPU)
        if (ratio <= 0.5) return STATUS_EXCELLENT;
        else if (ratio <= 0.7) return STATUS_GOOD;
        else if (ratio <= 0.9) return STATUS_FAIR;
        else if (ratio <= 1.0) return STATUS_POOR;
        else return STATUS_CRITICAL;
    } else {
        // For metrics where higher is better (FPS)
        if (ratio >= 1.0) return STATUS_EXCELLENT;
        else if (ratio >= 0.8) return STATUS_GOOD;
        else if (ratio >= 0.5) return STATUS_FAIR;
        else if (ratio >= 0.25) return STATUS_POOR;
        else return STATUS_CRITICAL;
    }
}

QString PerformanceDashboard::formatPerformanceValue(double value, const QString& unit, int precision) const
{
    return QString("%1%2").arg(QString::number(value, 'f', precision)).arg(unit);
}

void PerformanceDashboard::checkPerformanceThresholds()
{
    // Check FPS threshold
    if (m_currentStats.frameRate < m_targetFPS * 0.8) {
        emit performanceThresholdExceeded("FPS", m_currentStats.frameRate, m_targetFPS * 0.8);
    }
    
    // Check memory threshold
    if (m_currentStats.memoryUsage > m_targetMemoryMB * 0.9) {
        emit performanceThresholdExceeded("Memory", m_currentStats.memoryUsage, m_targetMemoryMB * 0.9);
    }
    
    // Check CPU threshold
    if (m_currentStats.cpuUsage > m_targetCPU * 0.9) {
        emit performanceThresholdExceeded("CPU", m_currentStats.cpuUsage, m_targetCPU * 0.9);
    }
    
    // Check latency threshold
    if (m_currentStats.averageLatency > m_targetLatencyMS) {
        emit performanceThresholdExceeded("Latency", m_currentStats.averageLatency, m_targetLatencyMS);
    }
}

void PerformanceDashboard::updateHistoricalData()
{
    QMutexLocker locker(&m_dataMutex);
    
    m_historicalStats.push_back(m_currentStats);
    
    // Keep only recent data
    while (m_historicalStats.size() > MAX_HISTORICAL_POINTS) {
        m_historicalStats.pop_front();
    }
    
    // Update chart widget with new data
    if (m_chartDisplayArea) {
        auto* chartWidget = qobject_cast<PerformanceChartWidget*>(m_chartDisplayArea);
        if (chartWidget) {
            chartWidget->setPerformanceData(m_historicalStats);
        }
    }
}

void PerformanceDashboard::onUpdateTimer()
{
    if (!m_monitoringActive) {
        return;
    }
    
    // Generate current performance stats if no external source
    if (!m_etiProcessor) {
        PerformanceStats stats;
        stats.frameRate = 0.0;
        stats.memoryUsage = 0.0;
        stats.frameCount = 0;
        stats.cpuUsage = 0.0;
        stats.averageLatency = 0.0;
        stats.timestamp = QDateTime::currentDateTime();
        
        updatePerformanceMetrics(stats);
    }
}

void PerformanceDashboard::onEtiPerformanceUpdate(double frameRate, double memoryMB)
{
    PerformanceStats stats;
    stats.frameRate = frameRate;
    stats.memoryUsage = memoryMB;
    stats.frameCount = m_currentStats.frameCount + 1;
    stats.cpuUsage = m_currentStats.cpuUsage; // Retain previous CPU usage
    stats.averageLatency = m_currentStats.averageLatency; // Retain previous latency
    stats.timestamp = QDateTime::currentDateTime();
    
    updatePerformanceMetrics(stats);
}

void PerformanceDashboard::resetStatistics()
{
    QMutexLocker locker(&m_dataMutex);
    
    m_currentStats = PerformanceStats();
    m_currentCompliance = ComplianceResult();
    m_historicalStats.clear();
    
    // Reset UI displays
    updateFPSDisplay(0.0);
    updateMemoryDisplay(0.0);
    updateComplianceDisplay(100.0);
    
    Logger::instance().log(Logger::Info, "PerformanceDashboard", "Performance statistics reset");
}

void PerformanceDashboard::onTimeRangeChanged(int index)
{
    Q_UNUSED(index)
    
    // Get selected time range in seconds
    int timeRangeSeconds = m_timeRangeCombo->currentData().toInt();
    
    // Update chart display based on new time range
    update();
    
    Logger::instance().log(Logger::Debug, "PerformanceDashboard",
                          QString("Time range changed to %1 seconds").arg(timeRangeSeconds));
}

void PerformanceDashboard::updateTargets(double targetFPS, double targetMemoryMB)
{
    m_targetFPS = targetFPS;
    m_targetMemoryMB = targetMemoryMB;
    
    // Update progress bar ranges
    m_fpsProgressBar->setRange(0, static_cast<int>(targetFPS));
    m_memoryProgressBar->setRange(0, static_cast<int>(targetMemoryMB));
    
    Logger::instance().log(Logger::Info, "PerformanceDashboard",
                          QString("Performance targets updated: FPS=%1, Memory=%2MB")
                          .arg(targetFPS).arg(targetMemoryMB));
}

void PerformanceDashboard::applyBroadcastingTheme()
{
    // Main widget background
    setStyleSheet(QString(
        "PerformanceDashboard { background-color: %1; }"
        "QFrame#PerformanceMetricFrame { "
        "    background-color: %2; "
        "    border: 1px solid %3; "
        "    border-radius: 4px; "
        "    margin: 2px; "
        "}"
        "QFrame#ComplianceFrame { "
        "    background-color: %2; "
        "    border: 1px solid %3; "
        "    border-radius: 4px; "
        "    margin: 2px; "
        "}"
        "QLabel { color: %4; }"
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
        "QTextEdit { "
        "    background-color: %1; "
        "    border: 1px solid %3; "
        "    border-radius: 3px; "
        "    color: %4; "
        "}"
    ).arg(BACKGROUND_DARK.name())
     .arg(BACKGROUND_MEDIUM.name())
     .arg(ACCENT_BLUE.name())
     .arg(TEXT_PRIMARY.name())
     .arg(STATUS_GOOD.name()));
}

// Include MOC file for embedded Q_OBJECT class
#include "performance_dashboard.moc"