#pragma once

#include <QWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QMutex>
#include <QColor>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QVector>
#include <QPointF>
#include <QDateTime>
#include <QStringList>
#include <deque>
#include <memory>

/**
 * @struct ChartDataPoint
 * @brief Data point for real-time charting
 */
struct ChartDataPoint {
    QDateTime timestamp;
    double value;
    QString label;
    QColor color;
    
    ChartDataPoint(const QDateTime& ts = QDateTime::currentDateTime(), 
                   double val = 0.0, 
                   const QString& lbl = QString(),
                   const QColor& clr = Qt::white)
        : timestamp(ts), value(val), label(lbl), color(clr) {}
};

/**
 * @struct ChartSeries
 * @brief Data series for multi-line charts
 */
struct ChartSeries {
    QString name;
    QColor color;
    double minValue;
    double maxValue;
    double targetValue;
    QString unit;
    std::deque<ChartDataPoint> dataPoints;
    bool visible;
    
    ChartSeries(const QString& n = QString(), 
                const QColor& c = Qt::white,
                double target = 0.0,
                const QString& u = QString())
        : name(n), color(c), minValue(0.0), maxValue(100.0), 
          targetValue(target), unit(u), visible(true) {}
};

/**
 * @class RealTimeChartWidget
 * @brief Professional real-time chart widget for broadcast industry
 * 
 * This widget provides professional real-time charting capabilities
 * designed for broadcast monitoring environments:
 * - Multi-series line charts with real-time updates
 * - Professional broadcast industry color schemes
 * - Interactive zoom and pan capabilities
 * - Target thresholds with visual indicators
 * - Performance optimized for 60 FPS updates
 * - Memory-efficient circular buffer storage
 * - Grid overlay with professional styling
 * - Legend with series status indicators
 */
class RealTimeChartWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Chart type enumeration
     */
    enum class ChartType {
        Line,           ///< Line chart (default)
        Area,           ///< Area chart
        Bar,            ///< Bar chart
        Scatter,        ///< Scatter plot
        Histogram       ///< Histogram
    };

    /**
     * @brief Time range enumeration
     */
    enum class TimeRange {
        Last30Seconds,  ///< 30 seconds
        Last1Minute,    ///< 1 minute
        Last5Minutes,   ///< 5 minutes
        Last15Minutes,  ///< 15 minutes
        Last1Hour,      ///< 1 hour
        Custom          ///< Custom range
    };

    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit RealTimeChartWidget(QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~RealTimeChartWidget();

    /**
     * @brief Initialize the chart widget
     * @return true if successful
     */
    bool initialize();

    /**
     * @brief Add data series to chart
     * @param series Chart series to add
     * @return Series ID for future reference
     */
    int addSeries(const ChartSeries& series);

    /**
     * @brief Remove data series from chart
     * @param seriesId Series ID to remove
     */
    void removeSeries(int seriesId);

    /**
     * @brief Clear all data series
     */
    void clearAllSeries();

    /**
     * @brief Add data point to series
     * @param seriesId Series ID
     * @param dataPoint Data point to add
     */
    void addDataPoint(int seriesId, const ChartDataPoint& dataPoint);

    /**
     * @brief Add data point with timestamp
     * @param seriesId Series ID
     * @param value Data value
     * @param timestamp Optional timestamp (current time if not specified)
     */
    void addDataPoint(int seriesId, double value, const QDateTime& timestamp = QDateTime::currentDateTime());

    /**
     * @brief Set chart type
     * @param type Chart type to set
     */
    void setChartType(ChartType type);

    /**
     * @brief Get current chart type
     * @return Current chart type
     */
    ChartType getChartType() const { return m_chartType; }

    /**
     * @brief Set time range for display
     * @param range Time range to display
     */
    void setTimeRange(TimeRange range);

    /**
     * @brief Get current time range
     * @return Current time range
     */
    TimeRange getTimeRange() const { return m_timeRange; }

    /**
     * @brief Set custom time range in seconds
     * @param seconds Number of seconds to display
     */
    void setCustomTimeRange(int seconds);

    /**
     * @brief Enable/disable auto-scaling
     * @param enabled true to enable auto-scaling
     */
    void setAutoScale(bool enabled);

    /**
     * @brief Check if auto-scaling is enabled
     * @return true if auto-scaling enabled
     */
    bool isAutoScale() const { return m_autoScale; }

    /**
     * @brief Set Y-axis range manually
     * @param minValue Minimum Y value
     * @param maxValue Maximum Y value
     */
    void setYRange(double minValue, double maxValue);

    /**
     * @brief Get Y-axis minimum value
     * @return Minimum Y value
     */
    double getYMin() const { return m_yMin; }

    /**
     * @brief Get Y-axis maximum value
     * @return Maximum Y value
     */
    double getYMax() const { return m_yMax; }

    /**
     * @brief Enable/disable grid display
     * @param enabled true to show grid
     */
    void setGridEnabled(bool enabled);

    /**
     * @brief Check if grid is enabled
     * @return true if grid enabled
     */
    bool isGridEnabled() const { return m_gridEnabled; }

    /**
     * @brief Enable/disable legend display
     * @param enabled true to show legend
     */
    void setLegendEnabled(bool enabled);

    /**
     * @brief Check if legend is enabled
     * @return true if legend enabled
     */
    bool isLegendEnabled() const { return m_legendEnabled; }

    /**
     * @brief Set series visibility
     * @param seriesId Series ID
     * @param visible true to show series
     */
    void setSeriesVisible(int seriesId, bool visible);

    /**
     * @brief Check if series is visible
     * @param seriesId Series ID
     * @return true if series visible
     */
    bool isSeriesVisible(int seriesId) const;

    /**
     * @brief Set target threshold for series
     * @param seriesId Series ID
     * @param target Target value
     */
    void setSeriesTarget(int seriesId, double target);

    /**
     * @brief Get series target value
     * @param seriesId Series ID
     * @return Target value
     */
    double getSeriesTarget(int seriesId) const;

    /**
     * @brief Apply professional broadcast theme
     */
    void applyBroadcastTheme();

    /**
     * @brief Start real-time updates
     */
    void startRealTimeUpdates();

    /**
     * @brief Stop real-time updates
     */
    void stopRealTimeUpdates();

    /**
     * @brief Check if real-time updates are active
     * @return true if real-time updates enabled
     */
    bool isRealTimeActive() const { return m_realTimeActive; }

public slots:
    /**
     * @brief Update chart display
     */
    void updateDisplay();

    /**
     * @brief Clear all chart data
     */
    void clearData();

    /**
     * @brief Take screenshot of chart
     */
    void takeScreenshot();

    /**
     * @brief Export chart data to CSV
     */
    void exportDataToCSV();

signals:
    /**
     * @brief Emitted when chart is clicked
     * @param point Clicked point in chart coordinates
     */
    void chartClicked(const QPointF& point);

    /**
     * @brief Emitted when series is clicked
     * @param seriesId Clicked series ID
     * @param dataPoint Nearest data point
     */
    void seriesClicked(int seriesId, const ChartDataPoint& dataPoint);

    /**
     * @brief Emitted when threshold is exceeded
     * @param seriesId Series ID
     * @param value Current value
     * @param threshold Threshold value
     */
    void thresholdExceeded(int seriesId, double value, double threshold);

protected:
    /**
     * @brief Handle paint events for chart rendering
     * @param event Paint event
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief Handle resize events
     * @param event Resize event
     */
    void resizeEvent(QResizeEvent *event) override;

    /**
     * @brief Handle mouse press events
     * @param event Mouse press event
     */
    void mousePressEvent(QMouseEvent *event) override;

    /**
     * @brief Handle mouse move events
     * @param event Mouse move event
     */
    void mouseMoveEvent(QMouseEvent *event) override;

    /**
     * @brief Handle mouse release events
     * @param event Mouse release event
     */
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    /**
     * @brief Handle real-time update timer
     */
    void onRealTimeUpdate();

private:
    /**
     * @brief Setup chart layout and margins
     */
    void setupChartLayout();

    /**
     * @brief Paint chart background
     * @param painter QPainter instance
     */
    void paintBackground(QPainter& painter);

    /**
     * @brief Paint chart grid
     * @param painter QPainter instance
     */
    void paintGrid(QPainter& painter);

    /**
     * @brief Paint chart axes
     * @param painter QPainter instance
     */
    void paintAxes(QPainter& painter);

    /**
     * @brief Paint data series
     * @param painter QPainter instance
     */
    void paintSeries(QPainter& painter);

    /**
     * @brief Paint single line series
     * @param painter QPainter instance
     * @param series Chart series
     */
    void paintLineSeries(QPainter& painter, const ChartSeries& series);

    /**
     * @brief Paint single area series
     * @param painter QPainter instance
     * @param series Chart series
     */
    void paintAreaSeries(QPainter& painter, const ChartSeries& series);

    /**
     * @brief Paint target thresholds
     * @param painter QPainter instance
     */
    void paintTargets(QPainter& painter);

    /**
     * @brief Paint chart legend
     * @param painter QPainter instance
     */
    void paintLegend(QPainter& painter);

    /**
     * @brief Transform data point to widget coordinates
     * @param dataPoint Data point to transform
     * @return Widget coordinates
     */
    QPointF transformToWidget(const ChartDataPoint& dataPoint) const;

    /**
     * @brief Transform widget coordinates to data coordinates
     * @param widgetPoint Widget coordinates
     * @return Data coordinates
     */
    QPointF transformFromWidget(const QPointF& widgetPoint) const;

    /**
     * @brief Update auto-scale ranges
     */
    void updateAutoScale();

    /**
     * @brief Get time range in seconds
     * @return Time range in seconds
     */
    int getTimeRangeSeconds() const;

    /**
     * @brief Cleanup old data points
     */
    void cleanupOldData();

    /**
     * @brief Find nearest data point to widget coordinates
     * @param widgetPoint Widget coordinates
     * @return Pair of series ID and data point
     */
    std::pair<int, ChartDataPoint> findNearestDataPoint(const QPointF& widgetPoint) const;

    /**
     * @brief Check threshold violations
     */
    void checkThresholds();

    /**
     * @brief Calculate chart area rectangle
     * @return Chart area rectangle
     */
    QRectF getChartArea() const;

    /**
     * @brief Calculate legend area rectangle
     * @return Legend area rectangle
     */
    QRectF getLegendArea() const;

    // Chart settings
    ChartType m_chartType;
    TimeRange m_timeRange;
    int m_customTimeRangeSeconds;
    bool m_autoScale;
    double m_yMin;
    double m_yMax;
    bool m_gridEnabled;
    bool m_legendEnabled;
    bool m_realTimeActive;
    bool m_isInitialized;

    // Data storage
    QVector<ChartSeries> m_series;
    int m_nextSeriesId;
    mutable QMutex m_dataMutex;

    // Real-time update system
    QTimer *m_updateTimer;

    // Layout properties
    int m_marginLeft;
    int m_marginRight;
    int m_marginTop;
    int m_marginBottom;
    int m_legendWidth;
    int m_axisLabelSpacing;

    // Professional broadcast colors
    QColor m_backgroundColor;
    QColor m_gridColor;
    QColor m_axisColor;
    QColor m_textColor;
    QColor m_targetLineColor;

    // Rendering properties
    QBrush m_backgroundBrush;
    QPen m_gridPen;
    QPen m_axisPen;
    QPen m_targetPen;
    QFont m_labelFont;
    QFont m_titleFont;
    QFont m_legendFont;

    // Interaction state
    bool m_mousePressed;
    QPointF m_lastMousePos;

    // Performance tracking
    qint64 m_lastUpdateTime;
    int m_frameCount;

    // Constants
    static constexpr int UPDATE_INTERVAL_MS = 16;  // ~60 FPS
    static constexpr int MAX_DATA_POINTS_PER_SERIES = 3600;  // 1 hour at 1 second intervals
    static constexpr int LEGEND_ITEM_HEIGHT = 20;
    static constexpr int AXIS_TICK_LENGTH = 5;
    static constexpr int MIN_CHART_WIDTH = 200;
    static constexpr int MIN_CHART_HEIGHT = 100;
};