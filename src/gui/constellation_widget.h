#pragma once

#include <QWidget>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QMutex>
#include <QPointF>
#include <QColor>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QVector>
#include <memory>

// Forward declarations
class EtiProcessor;
class DabDecoder;

/**
 * @class ConstellationWidget
 * @brief Professional signal constellation and spectrum visualization widget
 * 
 * This widget provides real-time visualization of DAB signal constellations,
 * spectrum analysis, and signal quality metrics for professional broadcast
 * monitoring. Features include:
 * - Real-time constellation diagram display
 * - Signal spectrum visualization
 * - Signal quality and error vector magnitude (EVM) analysis
 * - Interactive zoom and pan capabilities
 * - Professional color schemes for broadcast environments
 * - 60 FPS smooth updates for real-time monitoring
 * - Memory-efficient rendering for continuous operation
 */
class ConstellationWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Display mode enumeration
     */
    enum class DisplayMode {
        Constellation,  ///< Constellation diagram
        Spectrum,      ///< Spectrum analysis
        EyeDiagram,    ///< Eye diagram
        Waterfall,     ///< Waterfall display
        Scatter,       ///< Scatter plot mode
        Histogram      ///< Histogram display
    };

    /**
     * @brief Zoom mode enumeration
     */
    enum class ZoomMode {
        None,          ///< No zoom
        ZoomIn,        ///< Zoom in mode
        ZoomOut,       ///< Zoom out mode
        Pan            ///< Pan mode
    };

    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit ConstellationWidget(QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~ConstellationWidget();

    /**
     * @brief Initialize the constellation widget
     * @return true if successful
     */
    bool initialize();

    /**
     * @brief Set ETI processor for signal data
     * @param processor ETI processor instance
     */
    void setEtiProcessor(EtiProcessor *processor);

    /**
     * @brief Set display mode
     * @param mode Display mode to set
     */
    void setDisplayMode(DisplayMode mode);

    /**
     * @brief Get current display mode
     * @return Current display mode
     */
    DisplayMode getDisplayMode() const { return m_displayMode; }

    /**
     * @brief Set zoom mode
     * @param mode Zoom mode to set
     */
    void setZoomMode(ZoomMode mode);

    /**
     * @brief Get current zoom mode
     * @return Current zoom mode
     */
    ZoomMode getZoomMode() const { return m_zoomMode; }

    /**
     * @brief Set zoom level
     * @param zoom Zoom factor (1.0 = 100%)
     */
    void setZoomLevel(double zoom);

    /**
     * @brief Get current zoom level
     * @return Current zoom factor
     */
    double getZoomLevel() const { return m_zoomLevel; }

    /**
     * @brief Reset view to default
     */
    void resetView();

    /**
     * @brief Enable/disable real-time updates
     * @param enabled true to enable real-time updates
     */
    void setRealTimeEnabled(bool enabled);

    /**
     * @brief Check if real-time updates are enabled
     * @return true if real-time updates enabled
     */
    bool isRealTimeEnabled() const { return m_realTimeEnabled; }

    /**
     * @brief Start real-time updates
     */
    void startRealTimeUpdates();

    /**
     * @brief Stop real-time updates
     */
    void stopRealTimeUpdates();

    /**
     * @brief Check if the widget completed initialize()
     * @return true once the update timer + initial data are set up
     */
    bool isInitialized() const { return m_isInitialized; }

    /**
     * @brief Number of constellation points currently held
     * @return Point count (>0 once synthetic data has been generated)
     */
    int constellationPointCount() const;

    /**
     * @brief Get current signal quality
     * @return Signal quality percentage (0-100)
     */
    double getSignalQuality() const { return m_signalQuality; }

    /**
     * @brief Get current EVM (Error Vector Magnitude)
     * @return EVM in dB
     */
    double getErrorVectorMagnitude() const { return m_errorVectorMagnitude; }

    /**
     * @brief Get signal-to-noise ratio
     * @return SNR in dB
     */
    double getSignalToNoiseRatio() const { return m_signalToNoiseRatio; }

public slots:
    /**
     * @brief Update display with new signal data
     */
    void updateDisplay();

    /**
     * @brief Clear display data
     */
    void clearDisplay();

    /**
     * @brief Take screenshot of constellation
     */
    void takeScreenshot();

    /**
     * @brief Export constellation data
     */
    void exportData();

signals:
    /**
     * @brief Emitted when display mode changes
     * @param mode New display mode
     */
    void displayModeChanged(DisplayMode mode);

    /**
     * @brief Emitted when zoom level changes
     * @param zoom New zoom level
     */
    void zoomLevelChanged(double zoom);

    /**
     * @brief Emitted when signal quality changes
     * @param quality Signal quality (0-100%)
     */
    void signalQualityChanged(double quality);

    /**
     * @brief Emitted when constellation point is clicked
     * @param point Clicked point coordinates
     */
    void constellationPointClicked(const QPointF& point);

protected:
    /**
     * @brief Handle paint events for constellation rendering
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

    /**
     * @brief Handle wheel events for zooming
     * @param event Wheel event
     */
    void wheelEvent(QWheelEvent *event) override;

private slots:
    /**
     * @brief Handle real-time update timer
     */
    void handleRealTimeUpdate();

    /**
     * @brief Handle signal data update
     */
    void handleSignalDataUpdate();

private:
    /**
     * @brief Setup real-time timer
     */
    void setupRealTimeTimer();

    /**
     * @brief Paint constellation diagram
     * @param painter QPainter instance
     */
    void paintConstellation(QPainter& painter);

    /**
     * @brief Paint spectrum display
     * @param painter QPainter instance
     */
    void paintSpectrum(QPainter& painter);

    /**
     * @brief Paint eye diagram
     * @param painter QPainter instance
     */
    void paintEyeDiagram(QPainter& painter);

    /**
     * @brief Paint waterfall display
     * @param painter QPainter instance
     */
    void paintWaterfall(QPainter& painter);

    /**
     * @brief Paint grid and axes
     * @param painter QPainter instance
     */
    void paintGrid(QPainter& painter);

    /**
     * @brief Paint scale and labels
     * @param painter QPainter instance
     */
    void paintScale(QPainter& painter);

    /**
     * @brief Paint signal quality indicators
     * @param painter QPainter instance
     */
    void paintQualityIndicators(QPainter& painter);

    /**
     * @brief Transform point from signal space to widget coordinates
     * @param point Signal space point
     * @return Widget coordinates
     */
    QPointF transformToWidget(const QPointF& point) const;

    /**
     * @brief Transform point from widget coordinates to signal space
     * @param point Widget coordinates
     * @return Signal space point
     */
    QPointF transformFromWidget(const QPointF& point) const;

    /**
     * @brief Update signal statistics
     */
    void updateSignalStatistics();

    /**
     * @brief Generate test constellation data
     */
    void generateTestData();
    
    /**
     * @brief Generate constellation data based on ETI processor performance
     */
    void generatePerformanceBasedConstellation();

    /**
     * @brief Calculate EVM from constellation points
     * @return EVM in dB
     */
    double calculateEVM() const;

    /**
     * @brief Calculate SNR from signal data
     * @return SNR in dB
     */
    double calculateSNR() const;

    /**
     * @brief Validate widget state
     * @return true if state is valid
     */
    bool validateState() const;

    // Core components
    EtiProcessor *m_etiProcessor;
    DabDecoder *m_dabDecoder;

    // Display settings
    DisplayMode m_displayMode;
    ZoomMode m_zoomMode;
    double m_zoomLevel;
    QPointF m_panOffset;
    bool m_realTimeEnabled;
    bool m_isInitialized;

    // Real-time update system
    QTimer *m_updateTimer;
    // GUI-thread only: the render/update timer, paint and data mutation all
    // run on the Qt main thread; the guard is non-mutable (no const method
    // locks it).
    QMutex m_dataMutex;

    // Signal data
    QVector<QPointF> m_constellationPoints;
    QVector<double> m_spectrumData;
    QVector<QPointF> m_eyeDiagramData;
    QVector<QVector<double>> m_waterfallData;

    // Signal quality metrics
    double m_signalQuality;
    double m_errorVectorMagnitude;
    double m_signalToNoiseRatio;
    double m_bitErrorRate;

    // Rendering properties
    QColor m_backgroundColor;
    QColor m_gridColor;
    QColor m_axisColor;
    QColor m_signalColor;
    QColor m_textColor;
    QBrush m_signalBrush;
    QPen m_gridPen;
    QPen m_axisPen;
    QPen m_signalPen;
    QFont m_labelFont;

    // Interaction state
    bool m_mousePressed;
    QPointF m_lastMousePos;
    QPointF m_mousePressPos;

    // Performance tracking
    qint64 m_lastUpdateTime;
    int m_frameCount;
    double m_averageFrameTime;

    // Constants
    static constexpr int UPDATE_INTERVAL_MS = 16;  // ~60 FPS
    static constexpr int MAX_CONSTELLATION_POINTS = 1000;
    static constexpr int SPECTRUM_BINS = 512;
    static constexpr int WATERFALL_HISTORY = 256;
    static constexpr double MIN_ZOOM = 0.1;
    static constexpr double MAX_ZOOM = 10.0;
    static constexpr double ZOOM_FACTOR = 1.2;
};