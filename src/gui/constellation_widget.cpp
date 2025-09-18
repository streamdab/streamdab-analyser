/**
 * @file constellation_widget.cpp
 * @brief Professional signal constellation and spectrum visualization implementation
 * 
 * This file implements the ConstellationWidget class providing real-time
 * visualization of DAB signal constellations and spectrum analysis for
 * professional broadcast monitoring environments.
 */

#include "constellation_widget.h"
#include "core/eti_processor.h"
#include "core/dab_decoder.h"
#include "utils/logger.h"

#include <QPainter>
#include <QPainterPath>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <QApplication>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QMutexLocker>
#include <QElapsedTimer>
#include <QDebug>
#include <cmath>
#include <random>

ConstellationWidget::ConstellationWidget(QWidget *parent)
    : QWidget(parent)
    , m_etiProcessor(nullptr)
    , m_dabDecoder(nullptr)
    , m_displayMode(DisplayMode::Constellation)
    , m_zoomMode(ZoomMode::None)
    , m_zoomLevel(1.0)
    , m_panOffset(0.0, 0.0)
    , m_realTimeEnabled(false)
    , m_isInitialized(false)
    , m_updateTimer(nullptr)
    , m_signalQuality(0.0)
    , m_errorVectorMagnitude(0.0)
    , m_signalToNoiseRatio(0.0)
    , m_bitErrorRate(0.0)
    , m_backgroundColor(QColor(20, 20, 30))      // Dark blue background
    , m_gridColor(QColor(60, 60, 80))            // Subtle grid
    , m_axisColor(QColor(120, 120, 140))         // Axis lines
    , m_signalColor(QColor(0, 255, 128))         // Bright green signals
    , m_textColor(QColor(200, 200, 220))         // Light text
    , m_mousePressed(false)
    , m_lastMousePos(0.0, 0.0)
    , m_mousePressPos(0.0, 0.0)
    , m_lastUpdateTime(0)
    , m_frameCount(0)
    , m_averageFrameTime(0.0)
{
    // Set widget properties
    setObjectName("ConstellationWidget");
    setMinimumSize(300, 300);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setMouseTracking(true);
    
    // Initialize brushes and pens
    m_signalBrush = QBrush(m_signalColor);
    m_gridPen = QPen(m_gridColor, 1, Qt::DotLine);
    m_axisPen = QPen(m_axisColor, 2, Qt::SolidLine);
    m_signalPen = QPen(m_signalColor, 1, Qt::SolidLine);
    
    // Initialize font
    m_labelFont = QFont("Arial", 8);
    m_labelFont.setStyleHint(QFont::SansSerif);
    
    // Initialize data containers
    m_constellationPoints.reserve(MAX_CONSTELLATION_POINTS);
    m_spectrumData.resize(SPECTRUM_BINS);
    m_waterfallData.resize(WATERFALL_HISTORY);
    for (auto& row : m_waterfallData) {
        row.resize(SPECTRUM_BINS);
    }
    
    Logger::instance().log(Logger::Info, "ConstellationWidget", "Constructor completed");
}

ConstellationWidget::~ConstellationWidget()
{
    Logger::instance().log(Logger::Info, "ConstellationWidget", "Destructor starting");
    
    // Stop updates
    setRealTimeEnabled(false);
    
    // Cleanup timer
    if (m_updateTimer) {
        m_updateTimer->stop();
        delete m_updateTimer;
        m_updateTimer = nullptr;
    }
    
    Logger::instance().log(Logger::Info, "ConstellationWidget", "Destructor completed");
}

bool ConstellationWidget::initialize()
{
    Logger::instance().log(Logger::Info, "ConstellationWidget", "Initialization starting");
    
    try {
        // Setup real-time timer
        setupRealTimeTimer();
        
        // Generate initial test data
        generateTestData();
        
        // Set initialized flag
        m_isInitialized = true;
        
        Logger::instance().log(Logger::Info, "ConstellationWidget", "Initialization completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "ConstellationWidget", 
                             QString("Initialization failed: %1").arg(e.what()));
        return false;
    }
}

void ConstellationWidget::setupRealTimeTimer()
{
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(UPDATE_INTERVAL_MS);  // ~60 FPS
    m_updateTimer->setSingleShot(false);
    
    connect(m_updateTimer, &QTimer::timeout, this, &ConstellationWidget::handleRealTimeUpdate);
}

void ConstellationWidget::setEtiProcessor(EtiProcessor *processor)
{
    m_etiProcessor = processor;
    
    if (m_etiProcessor) {
        Logger::instance().log(Logger::Info, "ConstellationWidget", "ETI processor connected");
        // Connect to processor signals if available
    } else {
        Logger::instance().log(Logger::Warning, "ConstellationWidget", "ETI processor disconnected");
    }
}

void ConstellationWidget::setDisplayMode(DisplayMode mode)
{
    if (m_displayMode != mode) {
        m_displayMode = mode;
        
        // Clear current display when switching modes
        clearDisplay();
        
        // Generate appropriate test data for new mode
        generateTestData();
        
        // Update display
        update();
        
        emit displayModeChanged(mode);
        
        Logger::instance().log(Logger::Info, "ConstellationWidget", 
                             QString("Display mode changed to: %1").arg(static_cast<int>(mode)));
    }
}

void ConstellationWidget::setZoomMode(ZoomMode mode)
{
    if (m_zoomMode != mode) {
        m_zoomMode = mode;
        
        // Update cursor based on zoom mode
        switch (mode) {
            case ZoomMode::None:
                setCursor(Qt::ArrowCursor);
                break;
            case ZoomMode::ZoomIn:
                setCursor(Qt::CrossCursor);
                break;
            case ZoomMode::ZoomOut:
                setCursor(Qt::CrossCursor);
                break;
            case ZoomMode::Pan:
                setCursor(Qt::OpenHandCursor);
                break;
        }
        
        Logger::instance().log(Logger::Debug, "ConstellationWidget", 
                             QString("Zoom mode changed to: %1").arg(static_cast<int>(mode)));
    }
}

void ConstellationWidget::setZoomLevel(double zoom)
{
    // Clamp zoom level to valid range
    zoom = qBound(MIN_ZOOM, zoom, MAX_ZOOM);
    
    if (qAbs(m_zoomLevel - zoom) > 0.01) {
        m_zoomLevel = zoom;
        
        // Update display
        update();
        
        emit zoomLevelChanged(zoom);
        
        Logger::instance().log(Logger::Debug, "ConstellationWidget", 
                             QString("Zoom level changed to: %1").arg(zoom));
    }
}

void ConstellationWidget::resetView()
{
    m_zoomLevel = 1.0;
    m_panOffset = QPointF(0.0, 0.0);
    
    update();
    
    emit zoomLevelChanged(m_zoomLevel);
    
    Logger::instance().log(Logger::Info, "ConstellationWidget", "View reset to default");
}

void ConstellationWidget::setRealTimeEnabled(bool enabled)
{
    if (m_realTimeEnabled != enabled) {
        m_realTimeEnabled = enabled;
        
        if (enabled && m_updateTimer) {
            m_updateTimer->start();
            Logger::instance().log(Logger::Info, "ConstellationWidget", "Real-time updates enabled");
        } else if (m_updateTimer) {
            m_updateTimer->stop();
            Logger::instance().log(Logger::Info, "ConstellationWidget", "Real-time updates disabled");
        }
    }
}

void ConstellationWidget::updateDisplay()
{
    if (!m_isInitialized) {
        return;
    }
    
    // Update signal statistics
    updateSignalStatistics();
    
    // Trigger repaint
    update();
}

void ConstellationWidget::clearDisplay()
{
    QMutexLocker locker(&m_dataMutex);
    
    // Clear all data
    m_constellationPoints.clear();
    m_spectrumData.fill(0.0);
    m_eyeDiagramData.clear();
    
    // Clear waterfall data
    for (auto& row : m_waterfallData) {
        row.fill(0.0);
    }
    
    // Reset statistics
    m_signalQuality = 0.0;
    m_errorVectorMagnitude = 0.0;
    m_signalToNoiseRatio = 0.0;
    m_bitErrorRate = 0.0;
    
    locker.unlock();
    
    // Update display
    update();
    
    Logger::instance().log(Logger::Info, "ConstellationWidget", "Display cleared");
}

void ConstellationWidget::takeScreenshot()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Save Constellation Screenshot"),
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/constellation_screenshot.png",
        tr("PNG Files (*.png);;All Files (*)")
    );
    
    if (!fileName.isEmpty()) {
        QPixmap screenshot = grab();
        if (screenshot.save(fileName)) {
            QMessageBox::information(this, tr("Screenshot"), 
                                    tr("Screenshot saved successfully:\n%1").arg(fileName));
            
            Logger::instance().log(Logger::Info, "ConstellationWidget", 
                                 QString("Screenshot saved: %1").arg(fileName));
        } else {
            QMessageBox::warning(this, tr("Screenshot"), 
                                tr("Failed to save screenshot."));
            
            Logger::instance().log(Logger::Error, "ConstellationWidget", 
                                 QString("Failed to save screenshot: %1").arg(fileName));
        }
    }
}

void ConstellationWidget::exportData()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Export Constellation Data"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/constellation_data.csv",
        tr("CSV Files (*.csv);;Text Files (*.txt);;All Files (*)")
    );
    
    if (!fileName.isEmpty()) {
        // TODO: Implement actual data export
        QMessageBox::information(this, tr("Export"), 
                                tr("Constellation data export functionality will be implemented.\n"
                                   "Target file: %1").arg(fileName));
        
        Logger::instance().log(Logger::Info, "ConstellationWidget", 
                             QString("Data export requested: %1").arg(fileName));
    }
}

// Event handlers

void ConstellationWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    if (!m_isInitialized) {
        return;
    }
    
    QElapsedTimer paintTimer;
    paintTimer.start();
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    
    // Clear background
    painter.fillRect(rect(), m_backgroundColor);
    
    // Paint based on current display mode
    switch (m_displayMode) {
        case DisplayMode::Constellation:
            paintConstellation(painter);
            break;
        case DisplayMode::Spectrum:
            paintSpectrum(painter);
            break;
        case DisplayMode::EyeDiagram:
            paintEyeDiagram(painter);
            break;
        case DisplayMode::Waterfall:
            paintWaterfall(painter);
            break;
    }
    
    // Paint common elements
    paintGrid(painter);
    paintScale(painter);
    paintQualityIndicators(painter);
    
    // Track rendering performance
    qint64 paintTime = paintTimer.elapsed();
    m_frameCount++;
    m_averageFrameTime = (m_averageFrameTime * 0.9) + (paintTime * 0.1);
}

void ConstellationWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // Update display after resize
    update();
    
    Logger::instance().log(Logger::Debug, "ConstellationWidget", 
                         QString("Widget resized to %1x%2")
                         .arg(event->size().width())
                         .arg(event->size().height()));
}

void ConstellationWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = true;
        m_mousePressPos = event->pos();
        m_lastMousePos = event->pos();
        
        // Handle zoom mode
        if (m_zoomMode == ZoomMode::Pan) {
            setCursor(Qt::ClosedHandCursor);
        }
        
        // Emit constellation point clicked signal
        QPointF signalPoint = transformFromWidget(event->pos());
        emit constellationPointClicked(signalPoint);
    }
    
    QWidget::mousePressEvent(event);
}

void ConstellationWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_mousePressed && m_zoomMode == ZoomMode::Pan) {
        // Calculate pan delta
        QPointF delta = event->pos() - m_lastMousePos;
        m_panOffset += delta / m_zoomLevel;
        
        // Update display
        update();
    }
    
    m_lastMousePos = event->pos();
    
    QWidget::mouseMoveEvent(event);
}

void ConstellationWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = false;
        
        // Handle zoom clicks
        if (m_zoomMode == ZoomMode::ZoomIn) {
            setZoomLevel(m_zoomLevel * ZOOM_FACTOR);
        } else if (m_zoomMode == ZoomMode::ZoomOut) {
            setZoomLevel(m_zoomLevel / ZOOM_FACTOR);
        } else if (m_zoomMode == ZoomMode::Pan) {
            setCursor(Qt::OpenHandCursor);
        }
    }
    
    QWidget::mouseReleaseEvent(event);
}

void ConstellationWidget::wheelEvent(QWheelEvent *event)
{
    // Zoom with mouse wheel
    double zoomFactor = (event->angleDelta().y() > 0) ? ZOOM_FACTOR : (1.0 / ZOOM_FACTOR);
    setZoomLevel(m_zoomLevel * zoomFactor);
    
    QWidget::wheelEvent(event);
}

// Private slot implementations

void ConstellationWidget::handleRealTimeUpdate()
{
    if (!m_realTimeEnabled || !m_isInitialized) {
        return;
    }
    
    // Generate new test data
    generateTestData();
    
    // Update signal statistics
    updateSignalStatistics();
    
    // Trigger repaint
    update();
}

void ConstellationWidget::handleSignalDataUpdate()
{
    // Handle signal data updates from ETI processor
    if (m_etiProcessor) {
        // TODO: Process real signal data from ETI processor
        updateDisplay();
    }
}

// Private helper methods

void ConstellationWidget::paintConstellation(QPainter& painter)
{
    if (m_constellationPoints.isEmpty()) {
        return;
    }
    
    QMutexLocker locker(&m_dataMutex);
    
    // Set constellation rendering properties
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_signalBrush);
    
    // Draw constellation points
    for (const QPointF& point : m_constellationPoints) {
        QPointF widgetPoint = transformToWidget(point);
        
        // Only draw points within widget bounds
        if (rect().contains(widgetPoint.toPoint())) {
            painter.drawEllipse(widgetPoint, 2.0, 2.0);
        }
    }
    
    // Draw ideal constellation positions (for reference)
    painter.setPen(QPen(m_gridColor, 1, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    
    // Draw QPSK ideal positions
    QVector<QPointF> idealPoints = {
        QPointF(-0.7, -0.7), QPointF(0.7, -0.7),
        QPointF(-0.7, 0.7), QPointF(0.7, 0.7)
    };
    
    for (const QPointF& ideal : idealPoints) {
        QPointF widgetPoint = transformToWidget(ideal);
        painter.drawEllipse(widgetPoint, 6.0, 6.0);
    }
}

void ConstellationWidget::paintSpectrum(QPainter& painter)
{
    if (m_spectrumData.isEmpty()) {
        return;
    }
    
    QMutexLocker locker(&m_dataMutex);
    
    // Calculate spectrum display area
    QRectF spectrumRect = rect().adjusted(40, 20, -20, -40);
    
    // Set spectrum rendering properties
    painter.setPen(m_signalPen);
    painter.setBrush(Qt::NoBrush);
    
    // Draw spectrum
    QPainterPath spectrumPath;
    bool firstPoint = true;
    
    for (int i = 0; i < m_spectrumData.size(); ++i) {
        double x = spectrumRect.left() + (i * spectrumRect.width() / (m_spectrumData.size() - 1));
        double y = spectrumRect.bottom() - (m_spectrumData[i] * spectrumRect.height());
        
        if (firstPoint) {
            spectrumPath.moveTo(x, y);
            firstPoint = false;
        } else {
            spectrumPath.lineTo(x, y);
        }
    }
    
    painter.drawPath(spectrumPath);
    
    // Fill spectrum area
    painter.setBrush(QBrush(m_signalColor, Qt::SolidPattern));
    painter.setOpacity(0.3);
    spectrumPath.lineTo(spectrumRect.bottomRight());
    spectrumPath.lineTo(spectrumRect.bottomLeft());
    spectrumPath.closeSubpath();
    painter.drawPath(spectrumPath);
    painter.setOpacity(1.0);
}

void ConstellationWidget::paintEyeDiagram(QPainter& painter)
{
    if (m_eyeDiagramData.isEmpty()) {
        return;
    }
    
    QMutexLocker locker(&m_dataMutex);
    
    // Set eye diagram rendering properties
    painter.setPen(QPen(m_signalColor, 1, Qt::SolidLine));
    painter.setBrush(Qt::NoBrush);
    
    // Draw eye traces
    for (int i = 1; i < m_eyeDiagramData.size(); i += 2) {
        QPointF startPoint = transformToWidget(m_eyeDiagramData[i-1]);
        QPointF endPoint = transformToWidget(m_eyeDiagramData[i]);
        painter.drawLine(startPoint, endPoint);
    }
}

void ConstellationWidget::paintWaterfall(QPainter& painter)
{
    if (m_waterfallData.isEmpty()) {
        return;
    }
    
    QMutexLocker locker(&m_dataMutex);
    
    // Calculate waterfall display area
    QRectF waterfallRect = rect().adjusted(40, 20, -20, -40);
    
    // Draw waterfall data
    double pixelWidth = waterfallRect.width() / SPECTRUM_BINS;
    double pixelHeight = waterfallRect.height() / WATERFALL_HISTORY;
    
    for (int y = 0; y < WATERFALL_HISTORY && y < m_waterfallData.size(); ++y) {
        for (int x = 0; x < SPECTRUM_BINS && x < m_waterfallData[y].size(); ++x) {
            double intensity = m_waterfallData[y][x];
            
            // Create color based on intensity
            QColor pixelColor;
            if (intensity < 0.5) {
                // Blue to green
                pixelColor = QColor(0, static_cast<int>(intensity * 510), static_cast<int>((0.5 - intensity) * 510));
            } else {
                // Green to red
                pixelColor = QColor(static_cast<int>((intensity - 0.5) * 510), 255, 0);
            }
            
            // Draw pixel
            QRectF pixelRect(
                waterfallRect.left() + x * pixelWidth,
                waterfallRect.top() + y * pixelHeight,
                pixelWidth,
                pixelHeight
            );
            
            painter.fillRect(pixelRect, pixelColor);
        }
    }
}

void ConstellationWidget::paintGrid(QPainter& painter)
{
    painter.setPen(m_gridPen);
    
    // Calculate grid spacing based on zoom level
    double gridSpacing = 50.0 / m_zoomLevel;
    if (gridSpacing < 10.0) gridSpacing = 10.0;
    if (gridSpacing > 100.0) gridSpacing = 100.0;
    
    // Draw vertical grid lines
    for (double x = 0; x < width(); x += gridSpacing) {
        painter.drawLine(static_cast<int>(x), 0, static_cast<int>(x), height());
    }
    
    // Draw horizontal grid lines
    for (double y = 0; y < height(); y += gridSpacing) {
        painter.drawLine(0, static_cast<int>(y), width(), static_cast<int>(y));
    }
    
    // Draw center axes
    painter.setPen(m_axisPen);
    painter.drawLine(width() / 2, 0, width() / 2, height());  // Vertical center
    painter.drawLine(0, height() / 2, width(), height() / 2); // Horizontal center
}

void ConstellationWidget::paintScale(QPainter& painter)
{
    painter.setPen(QPen(m_textColor));
    painter.setFont(m_labelFont);
    
    // Draw scale labels based on display mode
    switch (m_displayMode) {
        case DisplayMode::Constellation:
            painter.drawText(10, 20, tr("I/Q Constellation"));
            painter.drawText(10, height() - 10, tr("I"));
            painter.drawText(width() - 20, height() / 2, tr("Q"));
            break;
            
        case DisplayMode::Spectrum:
            painter.drawText(10, 20, tr("Spectrum Analysis"));
            painter.drawText(10, height() - 10, tr("Frequency"));
            break;
            
        case DisplayMode::EyeDiagram:
            painter.drawText(10, 20, tr("Eye Diagram"));
            painter.drawText(10, height() - 10, tr("Time"));
            break;
            
        case DisplayMode::Waterfall:
            painter.drawText(10, 20, tr("Waterfall Display"));
            painter.drawText(10, height() - 10, tr("Frequency"));
            break;
    }
    
    // Draw zoom level indicator
    painter.drawText(width() - 100, 20, tr("Zoom: %1x").arg(QString::number(m_zoomLevel, 'f', 1)));
}

void ConstellationWidget::paintQualityIndicators(QPainter& painter)
{
    painter.setPen(QPen(m_textColor));
    painter.setFont(m_labelFont);
    
    // Quality indicators area
    int indicatorY = 40;
    int lineHeight = 15;
    
    // Signal quality
    QColor qualityColor = (m_signalQuality > 80.0) ? Qt::green : 
                         (m_signalQuality > 60.0) ? Qt::yellow : Qt::red;
    painter.setPen(QPen(qualityColor));
    painter.drawText(10, indicatorY, tr("Quality: %1%").arg(QString::number(m_signalQuality, 'f', 1)));
    
    // EVM
    indicatorY += lineHeight;
    painter.setPen(QPen(m_textColor));
    painter.drawText(10, indicatorY, tr("EVM: %1 dB").arg(QString::number(m_errorVectorMagnitude, 'f', 2)));
    
    // SNR
    indicatorY += lineHeight;
    painter.drawText(10, indicatorY, tr("SNR: %1 dB").arg(QString::number(m_signalToNoiseRatio, 'f', 1)));
    
    // Frame rate
    indicatorY += lineHeight;
    double frameRate = (m_averageFrameTime > 0) ? (1000.0 / m_averageFrameTime) : 0.0;
    painter.drawText(10, indicatorY, tr("FPS: %1").arg(QString::number(frameRate, 'f', 1)));
}

QPointF ConstellationWidget::transformToWidget(const QPointF& point) const
{
    // Transform from signal space (-1 to 1) to widget coordinates
    double x = (point.x() * m_zoomLevel + 1.0) * width() / 2.0 + m_panOffset.x();
    double y = (-point.y() * m_zoomLevel + 1.0) * height() / 2.0 + m_panOffset.y();
    
    return QPointF(x, y);
}

QPointF ConstellationWidget::transformFromWidget(const QPointF& point) const
{
    // Transform from widget coordinates to signal space (-1 to 1)
    double x = (2.0 * (point.x() - m_panOffset.x()) / width() - 1.0) / m_zoomLevel;
    double y = -(2.0 * (point.y() - m_panOffset.y()) / height() - 1.0) / m_zoomLevel;
    
    return QPointF(x, y);
}

void ConstellationWidget::updateSignalStatistics()
{
    QMutexLocker locker(&m_dataMutex);
    
    // Calculate EVM and SNR
    m_errorVectorMagnitude = calculateEVM();
    m_signalToNoiseRatio = calculateSNR();
    
    // Calculate signal quality based on EVM and SNR
    double evmQuality = qMax(0.0, 100.0 - qAbs(m_errorVectorMagnitude));
    double snrQuality = qMin(100.0, m_signalToNoiseRatio * 5.0);
    m_signalQuality = (evmQuality + snrQuality) / 2.0;
    
    // Emit signal quality change
    static double lastEmittedQuality = -1.0;
    if (qAbs(m_signalQuality - lastEmittedQuality) > 1.0) {
        emit signalQualityChanged(m_signalQuality);
        lastEmittedQuality = m_signalQuality;
    }
}

void ConstellationWidget::generateTestData()
{
    QMutexLocker locker(&m_dataMutex);
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::normal_distribution<double> noise(0.0, 0.1);
    
    switch (m_displayMode) {
        case DisplayMode::Constellation: {
            // Generate QPSK constellation with noise
            m_constellationPoints.clear();
            
            QVector<QPointF> idealPoints = {
                QPointF(-0.7, -0.7), QPointF(0.7, -0.7),
                QPointF(-0.7, 0.7), QPointF(0.7, 0.7)
            };
            
            for (int i = 0; i < 200; ++i) {
                QPointF ideal = idealPoints[i % 4];
                QPointF noisy(ideal.x() + noise(gen), ideal.y() + noise(gen));
                m_constellationPoints.append(noisy);
            }
            break;
        }
        
        case DisplayMode::Spectrum: {
            // Generate spectrum data
            for (int i = 0; i < SPECTRUM_BINS; ++i) {
                double freq = static_cast<double>(i) / SPECTRUM_BINS;
                double amplitude = 0.5 + 0.3 * sin(freq * 10.0 * M_PI) + 0.1 * noise(gen);
                m_spectrumData[i] = qBound(0.0, amplitude, 1.0);
            }
            break;
        }
        
        case DisplayMode::EyeDiagram: {
            // Generate eye diagram data
            m_eyeDiagramData.clear();
            for (int i = 0; i < 100; ++i) {
                double t = static_cast<double>(i) / 50.0 - 1.0;
                double amplitude = sin(t * M_PI) + 0.1 * noise(gen);
                m_eyeDiagramData.append(QPointF(t, amplitude));
            }
            break;
        }
        
        case DisplayMode::Waterfall: {
            // Shift waterfall data and add new line
            for (int y = WATERFALL_HISTORY - 1; y > 0; --y) {
                m_waterfallData[y] = m_waterfallData[y - 1];
            }
            
            // Generate new top line
            for (int x = 0; x < SPECTRUM_BINS; ++x) {
                double freq = static_cast<double>(x) / SPECTRUM_BINS;
                double intensity = 0.5 + 0.3 * sin(freq * 8.0 * M_PI + QDateTime::currentMSecsSinceEpoch() * 0.01);
                m_waterfallData[0][x] = qBound(0.0, intensity, 1.0);
            }
            break;
        }
    }
}

double ConstellationWidget::calculateEVM() const
{
    if (m_constellationPoints.isEmpty()) {
        return 0.0;
    }
    
    double totalError = 0.0;
    QVector<QPointF> idealPoints = {
        QPointF(-0.7, -0.7), QPointF(0.7, -0.7),
        QPointF(-0.7, 0.7), QPointF(0.7, 0.7)
    };
    
    for (const QPointF& point : m_constellationPoints) {
        // Find nearest ideal point
        double minDistance = std::numeric_limits<double>::max();
        for (const QPointF& ideal : idealPoints) {
            double distance = qPow(point.x() - ideal.x(), 2) + qPow(point.y() - ideal.y(), 2);
            if (distance < minDistance) {
                minDistance = distance;
            }
        }
        totalError += minDistance;
    }
    
    double rmsError = sqrt(totalError / m_constellationPoints.size());
    return 20.0 * log10(rmsError);  // Convert to dB
}

double ConstellationWidget::calculateSNR() const
{
    // Simplified SNR calculation based on signal quality
    return 20.0 + (m_signalQuality / 100.0) * 30.0;  // Range: 20-50 dB
}

bool ConstellationWidget::validateState() const
{
    return (m_isInitialized &&
            m_updateTimer != nullptr &&
            !m_constellationPoints.isEmpty());
}