#include "real_time_chart_widget.h"
#include <QApplication>
#include <QFileDialog>
#include <QDateTime>
#include <QTextStream>
#include <QPainterPath>
#include <QFontMetrics>
#include <QDebug>
#include <algorithm>
#include <cmath>

// Professional broadcast industry colors
static const QColor BACKGROUND_DARK = QColor(0x2D, 0x2D, 0x30);      // #2D2D30
static const QColor BACKGROUND_MEDIUM = QColor(0x3E, 0x3E, 0x42);    // #3E3E42
static const QColor ACCENT_BLUE = QColor(0x00, 0x78, 0xD4);          // #0078D4
static const QColor GRID_COLOR = QColor(0x5A, 0x5A, 0x5A);           // #5A5A5A
static const QColor AXIS_COLOR = QColor(0x80, 0x80, 0x80);           // #808080
static const QColor TEXT_COLOR = QColor(0xFF, 0xFF, 0xFF);           // #FFFFFF
static const QColor TARGET_LINE_COLOR = QColor(0xFF, 0xC1, 0x07);    // #FFC107 (Amber)

RealTimeChartWidget::RealTimeChartWidget(QWidget *parent)
    : QWidget(parent)
    , m_chartType(ChartType::Line)
    , m_timeRange(TimeRange::Last1Minute)
    , m_customTimeRangeSeconds(60)
    , m_autoScale(true)
    , m_yMin(0.0)
    , m_yMax(100.0)
    , m_gridEnabled(true)
    , m_legendEnabled(true)
    , m_realTimeActive(false)
    , m_isInitialized(false)
    , m_nextSeriesId(1)
    , m_updateTimer(new QTimer(this))
    , m_marginLeft(60)
    , m_marginRight(20)
    , m_marginTop(20)
    , m_marginBottom(40)
    , m_legendWidth(150)
    , m_axisLabelSpacing(10)
    , m_backgroundColor(BACKGROUND_DARK)
    , m_gridColor(GRID_COLOR)
    , m_axisColor(AXIS_COLOR)
    , m_textColor(TEXT_COLOR)
    , m_targetLineColor(TARGET_LINE_COLOR)
    , m_mousePressed(false)
    , m_lastUpdateTime(0)
    , m_frameCount(0)
{
    setupChartLayout();
    
    // Setup real-time update timer
    m_updateTimer->setInterval(UPDATE_INTERVAL_MS);
    connect(m_updateTimer, &QTimer::timeout, this, &RealTimeChartWidget::onRealTimeUpdate);
    
    // Enable mouse tracking for interactions
    setMouseTracking(true);
    
    // Set minimum size
    setMinimumSize(MIN_CHART_WIDTH + m_marginLeft + m_marginRight + m_legendWidth,
                   MIN_CHART_HEIGHT + m_marginTop + m_marginBottom);
}

RealTimeChartWidget::~RealTimeChartWidget()
{
    stopRealTimeUpdates();
}

bool RealTimeChartWidget::initialize()
{
    if (m_isInitialized) {
        return true;
    }
    
    try {
        applyBroadcastTheme();
        m_isInitialized = true;
        return true;
    } catch (const std::exception& e) {
        qWarning() << "Failed to initialize RealTimeChartWidget:" << e.what();
        return false;
    }
}

int RealTimeChartWidget::addSeries(const ChartSeries& series)
{
    QMutexLocker locker(&m_dataMutex);
    
    ChartSeries newSeries = series;
    newSeries.dataPoints.clear();  // Start with empty data
    
    m_series.append(newSeries);
    int seriesId = m_nextSeriesId++;
    
    update();  // Trigger repaint
    return seriesId - 1;  // Return 0-based index
}

void RealTimeChartWidget::removeSeries(int seriesId)
{
    QMutexLocker locker(&m_dataMutex);
    
    if (seriesId >= 0 && seriesId < m_series.size()) {
        m_series.removeAt(seriesId);
        update();
    }
}

void RealTimeChartWidget::clearAllSeries()
{
    QMutexLocker locker(&m_dataMutex);
    
    m_series.clear();
    m_nextSeriesId = 1;
    update();
}

void RealTimeChartWidget::addDataPoint(int seriesId, const ChartDataPoint& dataPoint)
{
    QMutexLocker locker(&m_dataMutex);
    
    if (seriesId >= 0 && seriesId < m_series.size()) {
        ChartSeries& series = m_series[seriesId];
        series.dataPoints.push_back(dataPoint);
        
        // Update min/max values for auto-scaling
        if (m_autoScale) {
            series.minValue = std::min(series.minValue, dataPoint.value);
            series.maxValue = std::max(series.maxValue, dataPoint.value);
        }
        
        // Limit data points to prevent memory issues
        if (series.dataPoints.size() > MAX_DATA_POINTS_PER_SERIES) {
            series.dataPoints.pop_front();
        }
        
        // Check thresholds
        if (series.targetValue > 0.0 && std::abs(dataPoint.value - series.targetValue) > series.targetValue * 0.1) {
            emit thresholdExceeded(seriesId, dataPoint.value, series.targetValue);
        }
    }
}

void RealTimeChartWidget::addDataPoint(int seriesId, double value, const QDateTime& timestamp)
{
    ChartDataPoint dataPoint(timestamp, value);
    addDataPoint(seriesId, dataPoint);
}

void RealTimeChartWidget::setChartType(ChartType type)
{
    if (m_chartType != type) {
        m_chartType = type;
        update();
    }
}

void RealTimeChartWidget::setTimeRange(TimeRange range)
{
    if (m_timeRange != range) {
        m_timeRange = range;
        cleanupOldData();
        update();
    }
}

void RealTimeChartWidget::setCustomTimeRange(int seconds)
{
    m_customTimeRangeSeconds = std::max(1, seconds);
    if (m_timeRange == TimeRange::Custom) {
        cleanupOldData();
        update();
    }
}

void RealTimeChartWidget::setAutoScale(bool enabled)
{
    if (m_autoScale != enabled) {
        m_autoScale = enabled;
        if (enabled) {
            updateAutoScale();
        }
        update();
    }
}

void RealTimeChartWidget::setYRange(double minValue, double maxValue)
{
    if (minValue < maxValue) {
        m_yMin = minValue;
        m_yMax = maxValue;
        m_autoScale = false;
        update();
    }
}

void RealTimeChartWidget::setGridEnabled(bool enabled)
{
    if (m_gridEnabled != enabled) {
        m_gridEnabled = enabled;
        update();
    }
}

void RealTimeChartWidget::setLegendEnabled(bool enabled)
{
    if (m_legendEnabled != enabled) {
        m_legendEnabled = enabled;
        update();
    }
}

void RealTimeChartWidget::setSeriesVisible(int seriesId, bool visible)
{
    QMutexLocker locker(&m_dataMutex);
    
    if (seriesId >= 0 && seriesId < m_series.size()) {
        m_series[seriesId].visible = visible;
        update();
    }
}

bool RealTimeChartWidget::isSeriesVisible(int seriesId) const
{
    QMutexLocker locker(&m_dataMutex);
    
    if (seriesId >= 0 && seriesId < m_series.size()) {
        return m_series[seriesId].visible;
    }
    return false;
}

void RealTimeChartWidget::setSeriesTarget(int seriesId, double target)
{
    QMutexLocker locker(&m_dataMutex);
    
    if (seriesId >= 0 && seriesId < m_series.size()) {
        m_series[seriesId].targetValue = target;
        update();
    }
}

double RealTimeChartWidget::getSeriesTarget(int seriesId) const
{
    QMutexLocker locker(&m_dataMutex);
    
    if (seriesId >= 0 && seriesId < m_series.size()) {
        return m_series[seriesId].targetValue;
    }
    return 0.0;
}

void RealTimeChartWidget::applyBroadcastTheme()
{
    // Set professional broadcast colors
    m_backgroundColor = BACKGROUND_DARK;
    m_gridColor = GRID_COLOR;
    m_axisColor = AXIS_COLOR;
    m_textColor = TEXT_COLOR;
    m_targetLineColor = TARGET_LINE_COLOR;
    
    // Setup brushes and pens
    m_backgroundBrush = QBrush(m_backgroundColor);
    m_gridPen = QPen(m_gridColor, 1, Qt::DotLine);
    m_axisPen = QPen(m_axisColor, 1, Qt::SolidLine);
    m_targetPen = QPen(m_targetLineColor, 2, Qt::DashLine);
    
    // Setup fonts
    m_labelFont = QFont("Segoe UI", 9);
    m_titleFont = QFont("Segoe UI", 10, QFont::Bold);
    m_legendFont = QFont("Segoe UI", 8);
    
    // Set widget background
    setStyleSheet(QString("background-color: %1; color: %2;")
                  .arg(m_backgroundColor.name())
                  .arg(m_textColor.name()));
    
    update();
}

void RealTimeChartWidget::startRealTimeUpdates()
{
    if (!m_realTimeActive) {
        m_realTimeActive = true;
        m_updateTimer->start();
    }
}

void RealTimeChartWidget::stopRealTimeUpdates()
{
    if (m_realTimeActive) {
        m_realTimeActive = false;
        m_updateTimer->stop();
    }
}

void RealTimeChartWidget::updateDisplay()
{
    cleanupOldData();
    
    if (m_autoScale) {
        updateAutoScale();
    }
    
    update();  // Trigger repaint
}

void RealTimeChartWidget::clearData()
{
    QMutexLocker locker(&m_dataMutex);
    
    for (ChartSeries& series : m_series) {
        series.dataPoints.clear();
    }
    
    update();
}

void RealTimeChartWidget::takeScreenshot()
{
    QString fileName = QFileDialog::getSaveFileName(this, 
        tr("Save Chart Screenshot"), 
        QString("chart_screenshot_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        tr("PNG Files (*.png)"));
    
    if (!fileName.isEmpty()) {
        QPixmap pixmap = grab();
        pixmap.save(fileName);
    }
}

void RealTimeChartWidget::exportDataToCSV()
{
    QString fileName = QFileDialog::getSaveFileName(this, 
        tr("Export Chart Data"), 
        QString("chart_data_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        tr("CSV Files (*.csv)"));
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            
            // Write header
            stream << "Timestamp";
            for (const ChartSeries& series : m_series) {
                stream << "," << series.name;
            }
            stream << "\n";
            
            // Find common time range
            QDateTime startTime = QDateTime::currentDateTime().addSecs(-getTimeRangeSeconds());
            QDateTime endTime = QDateTime::currentDateTime();
            
            // Write data (simplified - in production would need proper time alignment)
            QMutexLocker locker(&m_dataMutex);
            for (const ChartSeries& series : m_series) {
                for (const ChartDataPoint& point : series.dataPoints) {
                    if (point.timestamp >= startTime && point.timestamp <= endTime) {
                        stream << point.timestamp.toString(Qt::ISODate) << "," << point.value << "\n";
                    }
                }
            }
        }
    }
}

void RealTimeChartWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    // Paint background
    paintBackground(painter);
    
    // Paint grid if enabled
    if (m_gridEnabled) {
        paintGrid(painter);
    }
    
    // Paint axes
    paintAxes(painter);
    
    // Paint data series
    paintSeries(painter);
    
    // Paint target thresholds
    paintTargets(painter);
    
    // Paint legend if enabled
    if (m_legendEnabled) {
        paintLegend(painter);
    }
    
    // Update performance tracking
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (m_lastUpdateTime > 0) {
        // Track frame rate for performance monitoring
        m_frameCount++;
    }
    m_lastUpdateTime = currentTime;
}

void RealTimeChartWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    setupChartLayout();
    update();
}

void RealTimeChartWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = true;
        m_lastMousePos = event->pos();
        
        // Find nearest data point and emit signal
        std::pair<int, ChartDataPoint> nearest = findNearestDataPoint(event->pos());
        if (nearest.first >= 0) {
            emit seriesClicked(nearest.first, nearest.second);
        }
        
        emit chartClicked(transformFromWidget(event->pos()));
    }
    
    QWidget::mousePressEvent(event);
}

void RealTimeChartWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_mousePressed) {
        // Handle pan operation if needed
        m_lastMousePos = event->pos();
    }
    
    QWidget::mouseMoveEvent(event);
}

void RealTimeChartWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = false;
    }
    
    QWidget::mouseReleaseEvent(event);
}

void RealTimeChartWidget::onRealTimeUpdate()
{
    if (m_realTimeActive) {
        cleanupOldData();
        
        if (m_autoScale) {
            updateAutoScale();
        }
        
        update();
    }
}

void RealTimeChartWidget::setupChartLayout()
{
    // Adjust legend width based on widget size
    if (m_legendEnabled) {
        m_legendWidth = std::min(150, width() / 4);
    } else {
        m_legendWidth = 0;
    }
    
    // Ensure minimum chart area
    int availableWidth = width() - m_marginLeft - m_marginRight - m_legendWidth;
    int availableHeight = height() - m_marginTop - m_marginBottom;
    
    if (availableWidth < MIN_CHART_WIDTH) {
        m_marginRight = std::max(10, width() - m_marginLeft - m_legendWidth - MIN_CHART_WIDTH);
    }
    
    if (availableHeight < MIN_CHART_HEIGHT) {
        m_marginBottom = std::max(20, height() - m_marginTop - MIN_CHART_HEIGHT);
    }
}

void RealTimeChartWidget::paintBackground(QPainter& painter)
{
    painter.fillRect(rect(), m_backgroundBrush);
}

void RealTimeChartWidget::paintGrid(QPainter& painter)
{
    painter.setPen(m_gridPen);
    
    QRectF chartArea = getChartArea();
    
    // Vertical grid lines (time axis)
    int verticalLines = 10;
    for (int i = 0; i <= verticalLines; ++i) {
        double x = chartArea.left() + (chartArea.width() * i) / verticalLines;
        painter.drawLine(QPointF(x, chartArea.top()), QPointF(x, chartArea.bottom()));
    }
    
    // Horizontal grid lines (value axis)
    int horizontalLines = 8;
    for (int i = 0; i <= horizontalLines; ++i) {
        double y = chartArea.top() + (chartArea.height() * i) / horizontalLines;
        painter.drawLine(QPointF(chartArea.left(), y), QPointF(chartArea.right(), y));
    }
}

void RealTimeChartWidget::paintAxes(QPainter& painter)
{
    painter.setPen(m_axisPen);
    painter.setFont(m_labelFont);
    
    QRectF chartArea = getChartArea();
    
    // Draw X axis (bottom)
    painter.drawLine(chartArea.bottomLeft(), chartArea.bottomRight());
    
    // Draw Y axis (left)
    painter.drawLine(chartArea.bottomLeft(), chartArea.topLeft());
    
    // Draw axis labels
    painter.setPen(QPen(m_textColor));
    
    // Y-axis labels
    int horizontalLines = 8;
    for (int i = 0; i <= horizontalLines; ++i) {
        double value = m_yMin + (m_yMax - m_yMin) * (horizontalLines - i) / horizontalLines;
        double y = chartArea.top() + (chartArea.height() * i) / horizontalLines;
        
        QString label = QString::number(value, 'f', 1);
        QRectF labelRect(0, y - 10, m_marginLeft - 5, 20);
        painter.drawText(labelRect, Qt::AlignRight | Qt::AlignVCenter, label);
    }
    
    // X-axis labels (time)
    int timeRangeSeconds = getTimeRangeSeconds();
    QDateTime endTime = QDateTime::currentDateTime();
    QDateTime startTime = endTime.addSecs(-timeRangeSeconds);
    
    int timeLabels = 5;
    for (int i = 0; i <= timeLabels; ++i) {
        QDateTime time = startTime.addSecs((timeRangeSeconds * i) / timeLabels);
        double x = chartArea.left() + (chartArea.width() * i) / timeLabels;
        
        QString label = time.toString("hh:mm:ss");
        QRectF labelRect(x - 30, chartArea.bottom() + 5, 60, 20);
        painter.drawText(labelRect, Qt::AlignCenter, label);
    }
}

void RealTimeChartWidget::paintSeries(QPainter& painter)
{
    QMutexLocker locker(&m_dataMutex);
    
    for (const ChartSeries& series : m_series) {
        if (!series.visible || series.dataPoints.empty()) {
            continue;
        }
        
        switch (m_chartType) {
            case ChartType::Line:
                paintLineSeries(painter, series);
                break;
            case ChartType::Area:
                paintAreaSeries(painter, series);
                break;
            default:
                paintLineSeries(painter, series);  // Default to line chart
                break;
        }
    }
}

void RealTimeChartWidget::paintLineSeries(QPainter& painter, const ChartSeries& series)
{
    if (series.dataPoints.size() < 2) {
        return;
    }
    
    QPen seriesPen(series.color, 2, Qt::SolidLine);
    painter.setPen(seriesPen);
    
    QRectF chartArea = getChartArea();
    int timeRangeSeconds = getTimeRangeSeconds();
    QDateTime endTime = QDateTime::currentDateTime();
    QDateTime startTime = endTime.addSecs(-timeRangeSeconds);
    
    QPainterPath path;
    bool firstPoint = true;
    
    for (const ChartDataPoint& point : series.dataPoints) {
        if (point.timestamp < startTime || point.timestamp > endTime) {
            continue;
        }
        
        // Calculate X position based on time
        qint64 timeOffset = startTime.msecsTo(point.timestamp);
        double xRatio = static_cast<double>(timeOffset) / (timeRangeSeconds * 1000);
        double x = chartArea.left() + chartArea.width() * xRatio;
        
        // Calculate Y position based on value
        double yRatio = (point.value - m_yMin) / (m_yMax - m_yMin);
        double y = chartArea.bottom() - chartArea.height() * yRatio;
        
        QPointF widgetPoint(x, y);
        
        if (firstPoint) {
            path.moveTo(widgetPoint);
            firstPoint = false;
        } else {
            path.lineTo(widgetPoint);
        }
    }
    
    painter.drawPath(path);
}

void RealTimeChartWidget::paintAreaSeries(QPainter& painter, const ChartSeries& series)
{
    if (series.dataPoints.size() < 2) {
        return;
    }
    
    QColor fillColor = series.color;
    fillColor.setAlpha(100);  // Semi-transparent fill
    QBrush areaBrush(fillColor);
    
    QPen seriesPen(series.color, 2, Qt::SolidLine);
    painter.setPen(seriesPen);
    painter.setBrush(areaBrush);
    
    QRectF chartArea = getChartArea();
    int timeRangeSeconds = getTimeRangeSeconds();
    QDateTime endTime = QDateTime::currentDateTime();
    QDateTime startTime = endTime.addSecs(-timeRangeSeconds);
    
    QPainterPath path;
    bool firstPoint = true;
    QPointF firstWidgetPoint;
    QPointF lastWidgetPoint;
    
    for (const ChartDataPoint& point : series.dataPoints) {
        if (point.timestamp < startTime || point.timestamp > endTime) {
            continue;
        }
        
        // Calculate positions (same as line series)
        qint64 timeOffset = startTime.msecsTo(point.timestamp);
        double xRatio = static_cast<double>(timeOffset) / (timeRangeSeconds * 1000);
        double x = chartArea.left() + chartArea.width() * xRatio;
        
        double yRatio = (point.value - m_yMin) / (m_yMax - m_yMin);
        double y = chartArea.bottom() - chartArea.height() * yRatio;
        
        QPointF widgetPoint(x, y);
        
        if (firstPoint) {
            firstWidgetPoint = widgetPoint;
            path.moveTo(QPointF(x, chartArea.bottom()));  // Start from baseline
            path.lineTo(widgetPoint);
            firstPoint = false;
        } else {
            path.lineTo(widgetPoint);
        }
        lastWidgetPoint = widgetPoint;
    }
    
    // Close the area to baseline
    if (!firstPoint) {
        path.lineTo(QPointF(lastWidgetPoint.x(), chartArea.bottom()));
        path.lineTo(QPointF(firstWidgetPoint.x(), chartArea.bottom()));
        path.closeSubpath();
    }
    
    painter.drawPath(path);
}

void RealTimeChartWidget::paintTargets(QPainter& painter)
{
    QMutexLocker locker(&m_dataMutex);
    
    painter.setPen(m_targetPen);
    QRectF chartArea = getChartArea();
    
    for (const ChartSeries& series : m_series) {
        if (!series.visible || series.targetValue <= 0.0) {
            continue;
        }
        
        // Calculate Y position for target value
        double yRatio = (series.targetValue - m_yMin) / (m_yMax - m_yMin);
        double y = chartArea.bottom() - chartArea.height() * yRatio;
        
        // Draw target line
        painter.drawLine(QPointF(chartArea.left(), y), QPointF(chartArea.right(), y));
        
        // Draw target label
        painter.setPen(QPen(m_textColor));
        QString targetLabel = QString("%1: %2 %3")
                              .arg(series.name)
                              .arg(series.targetValue, 0, 'f', 1)
                              .arg(series.unit);
        painter.drawText(QPointF(chartArea.right() + 5, y + 5), targetLabel);
        painter.setPen(m_targetPen);
    }
}

void RealTimeChartWidget::paintLegend(QPainter& painter)
{
    if (!m_legendEnabled) {
        return;
    }
    
    QMutexLocker locker(&m_dataMutex);
    
    QRectF legendArea = getLegendArea();
    painter.setPen(QPen(m_axisColor));
    painter.setBrush(QBrush(m_backgroundColor.lighter(110)));
    painter.drawRect(legendArea);
    
    painter.setFont(m_legendFont);
    painter.setPen(QPen(m_textColor));
    
    int itemY = static_cast<int>(legendArea.top()) + 10;
    for (int i = 0; i < m_series.size(); ++i) {
        const ChartSeries& series = m_series[i];
        
        // Draw series color indicator
        QRectF colorRect(legendArea.left() + 10, itemY, 12, 12);
        painter.fillRect(colorRect, series.color);
        
        // Draw series name
        QString seriesText = QString("%1 (%2)").arg(series.name).arg(series.unit);
        QRectF textRect(legendArea.left() + 30, itemY - 2, legendArea.width() - 40, 16);
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, seriesText);
        
        itemY += LEGEND_ITEM_HEIGHT;
        
        if (itemY > legendArea.bottom() - 10) {
            break;  // Don't overflow legend area
        }
    }
}

QPointF RealTimeChartWidget::transformToWidget(const ChartDataPoint& dataPoint) const
{
    QRectF chartArea = getChartArea();
    int timeRangeSeconds = getTimeRangeSeconds();
    QDateTime endTime = QDateTime::currentDateTime();
    QDateTime startTime = endTime.addSecs(-timeRangeSeconds);
    
    // Calculate X position based on time
    qint64 timeOffset = startTime.msecsTo(dataPoint.timestamp);
    double xRatio = static_cast<double>(timeOffset) / (timeRangeSeconds * 1000);
    double x = chartArea.left() + chartArea.width() * xRatio;
    
    // Calculate Y position based on value
    double yRatio = (dataPoint.value - m_yMin) / (m_yMax - m_yMin);
    double y = chartArea.bottom() - chartArea.height() * yRatio;
    
    return QPointF(x, y);
}

QPointF RealTimeChartWidget::transformFromWidget(const QPointF& widgetPoint) const
{
    QRectF chartArea = getChartArea();
    int timeRangeSeconds = getTimeRangeSeconds();
    
    // Calculate time from X position
    double xRatio = (widgetPoint.x() - chartArea.left()) / chartArea.width();
    qint64 timeOffset = static_cast<qint64>(xRatio * timeRangeSeconds * 1000);
    QDateTime endTime = QDateTime::currentDateTime();
    QDateTime startTime = endTime.addSecs(-timeRangeSeconds);
    QDateTime timestamp = startTime.addMSecs(timeOffset);
    
    // Calculate value from Y position
    double yRatio = (chartArea.bottom() - widgetPoint.y()) / chartArea.height();
    double value = m_yMin + yRatio * (m_yMax - m_yMin);
    
    return QPointF(timestamp.toMSecsSinceEpoch(), value);
}

void RealTimeChartWidget::updateAutoScale()
{
    if (!m_autoScale) {
        return;
    }
    
    QMutexLocker locker(&m_dataMutex);
    
    double globalMin = std::numeric_limits<double>::max();
    double globalMax = std::numeric_limits<double>::lowest();
    bool hasData = false;
    
    int timeRangeSeconds = getTimeRangeSeconds();
    QDateTime endTime = QDateTime::currentDateTime();
    QDateTime startTime = endTime.addSecs(-timeRangeSeconds);
    
    for (const ChartSeries& series : m_series) {
        if (!series.visible) {
            continue;
        }
        
        for (const ChartDataPoint& point : series.dataPoints) {
            if (point.timestamp >= startTime && point.timestamp <= endTime) {
                globalMin = std::min(globalMin, point.value);
                globalMax = std::max(globalMax, point.value);
                hasData = true;
            }
        }
    }
    
    if (hasData) {
        // Add 10% padding to the range
        double range = globalMax - globalMin;
        double padding = range * 0.1;
        
        m_yMin = globalMin - padding;
        m_yMax = globalMax + padding;
        
        // Ensure minimum range
        if (m_yMax - m_yMin < 1.0) {
            double center = (m_yMin + m_yMax) / 2.0;
            m_yMin = center - 0.5;
            m_yMax = center + 0.5;
        }
    }
}

int RealTimeChartWidget::getTimeRangeSeconds() const
{
    switch (m_timeRange) {
        case TimeRange::Last30Seconds: return 30;
        case TimeRange::Last1Minute: return 60;
        case TimeRange::Last5Minutes: return 300;
        case TimeRange::Last15Minutes: return 900;
        case TimeRange::Last1Hour: return 3600;
        case TimeRange::Custom: return m_customTimeRangeSeconds;
        default: return 60;
    }
}

void RealTimeChartWidget::cleanupOldData()
{
    QMutexLocker locker(&m_dataMutex);
    
    int timeRangeSeconds = getTimeRangeSeconds();
    QDateTime cutoffTime = QDateTime::currentDateTime().addSecs(-timeRangeSeconds - 60); // Keep 1 minute extra
    
    for (ChartSeries& series : m_series) {
        auto it = series.dataPoints.begin();
        while (it != series.dataPoints.end()) {
            if (it->timestamp < cutoffTime) {
                it = series.dataPoints.erase(it);
            } else {
                ++it;
            }
        }
    }
}

std::pair<int, ChartDataPoint> RealTimeChartWidget::findNearestDataPoint(const QPointF& widgetPoint) const
{
    QMutexLocker locker(&m_dataMutex);
    
    double minDistance = std::numeric_limits<double>::max();
    int nearestSeriesId = -1;
    ChartDataPoint nearestPoint;
    
    for (int i = 0; i < m_series.size(); ++i) {
        const ChartSeries& series = m_series[i];
        if (!series.visible) {
            continue;
        }
        
        for (const ChartDataPoint& point : series.dataPoints) {
            QPointF pointWidget = transformToWidget(point);
            double distance = std::sqrt(std::pow(widgetPoint.x() - pointWidget.x(), 2) + 
                                       std::pow(widgetPoint.y() - pointWidget.y(), 2));
            
            if (distance < minDistance) {
                minDistance = distance;
                nearestSeriesId = i;
                nearestPoint = point;
            }
        }
    }
    
    return std::make_pair(nearestSeriesId, nearestPoint);
}

void RealTimeChartWidget::checkThresholds()
{
    // Thresholds are checked in addDataPoint method
    // This method can be used for periodic threshold checks if needed
}

QRectF RealTimeChartWidget::getChartArea() const
{
    int chartWidth = width() - m_marginLeft - m_marginRight - m_legendWidth;
    int chartHeight = height() - m_marginTop - m_marginBottom;
    
    return QRectF(m_marginLeft, m_marginTop, chartWidth, chartHeight);
}

QRectF RealTimeChartWidget::getLegendArea() const
{
    if (!m_legendEnabled) {
        return QRectF();
    }
    
    int legendX = width() - m_legendWidth - 10;
    return QRectF(legendX, m_marginTop, m_legendWidth, height() - m_marginTop - m_marginBottom);
}