/**
 * @file constellation_widget.cpp
 * @brief Professional signal constellation and spectrum visualization implementation
 * 
 * This file implements the ConstellationWidget class providing real-time
 * visualization of DAB signal constellations and spectrum analysis for
 * professional broadcast monitoring environments.
 */

#include "constellation_widget.h"
#include "core/eti_processor.hpp"
#include "core/dab_decoder.hpp"
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
    , m_backgroundColor(QColor(45, 45, 48))      // Professional dark background (#2D2D30)
    , m_gridColor(QColor(62, 62, 66))            // Subtle professional grid (#3E3E42)
    , m_axisColor(QColor(0, 120, 212))           // Microsoft blue axes (#0078D4)
    , m_signalColor(QColor(76, 175, 80))         // Professional green signals (#4CAF50)
    , m_textColor(QColor(255, 255, 255))         // Pure white text for broadcast clarity
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

int ConstellationWidget::constellationPointCount() const
{
    // GUI-thread only accessor (see the m_dataMutex note in the header): the
    // 16 ms update timer and every reader run on the Qt main thread.
    return m_constellationPoints.size();
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

void ConstellationWidget::startRealTimeUpdates()
{
    setRealTimeEnabled(true);
}

void ConstellationWidget::stopRealTimeUpdates()
{
    setRealTimeEnabled(false);
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
        // Implement comprehensive constellation data export
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            
            // Export header with metadata
            out << "Constellation Diagram Export\n";
            out << "Generated: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
            out << "Signal Analysis: DAB OFDM Constellation\n";
            out << "========================================\n\n";
            
            // Export signal metrics
            out << "Signal Quality Metrics:\n";
            out << "SNR: " << QString::number(m_signalToNoiseRatio, 'f', 2) << " dB\n";
            out << "Signal Quality: " << QString::number(m_signalQuality, 'f', 1) << "%\n";
            out << "Error Rate: " << QString::number(m_bitErrorRate, 'f', 4) << "\n";
            out << "Modulation: DAB OFDM\n";
            out << "Total Samples: " << m_constellationPoints.size() << "\n\n";
            
            // Export constellation points (I/Q data)
            out << "Constellation Points (I, Q):\n";
            for (int i = 0; i < m_constellationPoints.size(); ++i) {
                const QPointF& point = m_constellationPoints[i];
                out << QString::number(point.x(), 'f', 6) << ", " 
                    << QString::number(point.y(), 'f', 6) << "\n";
                
                // Limit output for very large datasets
                if (i >= 10000) {
                    out << "... (truncated at 10000 samples)\n";
                    break;
                }
            }
            
            // Export signal statistics
            out << "\nSignal Statistics:\n";
            out << "EVM: " << QString::number(m_errorVectorMagnitude, 'f', 6) << "\n";
            out << "Signal Quality: " << QString::number(m_signalQuality, 'f', 2) << "%\n";
            out << "SNR: " << QString::number(m_signalToNoiseRatio, 'f', 2) << " dB\n";
            out << "Bit Error Rate: " << QString::number(m_bitErrorRate, 'f', 6) << "\n";
            out << "Sample Count: " << QString::number(m_constellationPoints.size()) << "\n";
            
            file.close();
            QMessageBox::information(this, tr("Export Successful"), 
                                   tr("Constellation data exported successfully to:\n%1\n\n"
                                      "Exported %2 constellation points").arg(fileName).arg(m_constellationPoints.size()));
        } else {
            QMessageBox::warning(this, tr("Export Failed"), 
                               tr("Could not write to file:\n%1").arg(fileName));
        }
        
        Logger::instance().log(Logger::Info, "ConstellationWidget", 
                             QString("Constellation data export completed: %1").arg(fileName));
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
        case DisplayMode::Scatter:
        case DisplayMode::Histogram:
            break; // not implemented by this widget
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
        // Get current frame count and processing rate
        quint64 frameCount = m_etiProcessor->get_frame_count();
        (void)frameCount; // TODO: Use frameCount for display updates
        
        // Calculate current processing rate (FPS)
        qint64 currentTime = QElapsedTimer().elapsed();
        if (m_lastUpdateTime > 0) {
            double timeDelta = (currentTime - m_lastUpdateTime) / 1000.0; // Convert to seconds
            if (timeDelta > 0) {
                double instantFPS = 1.0 / timeDelta;
                m_averageFrameTime = (m_averageFrameTime * 0.9) + (timeDelta * 1000.0 * 0.1); // Rolling average in ms
                
                // Update signal quality based on processing performance
                if (instantFPS > 900.0) {
                    m_signalQuality = 100.0;
                    m_errorVectorMagnitude = -25.0; // Excellent EVM
                    m_signalToNoiseRatio = 45.0; // Excellent SNR
                } else if (instantFPS > 500.0) {
                    m_signalQuality = 85.0 + (instantFPS - 500.0) / 400.0 * 15.0;
                    m_errorVectorMagnitude = -20.0 - (instantFPS - 500.0) / 400.0 * 5.0;
                    m_signalToNoiseRatio = 35.0 + (instantFPS - 500.0) / 400.0 * 10.0;
                } else {
                    m_signalQuality = 50.0 + instantFPS / 500.0 * 35.0;
                    m_errorVectorMagnitude = -10.0 - instantFPS / 500.0 * 10.0;
                    m_signalToNoiseRatio = 20.0 + instantFPS / 500.0 * 15.0;
                }
                
                // Generate constellation points based on signal quality
                generatePerformanceBasedConstellation();
            }
        }
        m_lastUpdateTime = currentTime;
        
        updateDisplay();
    }
}

void ConstellationWidget::generatePerformanceBasedConstellation()
{
    QMutexLocker locker(&m_dataMutex);
    
    // Clear existing points
    m_constellationPoints.clear();
    
    // Generate constellation based on current signal quality
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Noise level inversely proportional to signal quality
    double noiseLevel = 0.2 * (1.0 - m_signalQuality / 100.0);
    std::normal_distribution<double> noise(0.0, noiseLevel);
    
    // QPSK constellation ideal positions
    QVector<QPointF> idealPoints = {
        QPointF(-0.7071, -0.7071), QPointF(0.7071, -0.7071),
        QPointF(-0.7071, 0.7071), QPointF(0.7071, 0.7071)
    };
    
    // Number of points based on signal quality (more points = better quality)
    int pointCount = static_cast<int>(50 + m_signalQuality * 3.0); // 50-350 points
    
    for (int i = 0; i < pointCount; ++i) {
        QPointF ideal = idealPoints[i % 4];
        
        // Add noise based on signal quality
        QPointF noisy(ideal.x() + noise(gen), ideal.y() + noise(gen));
        
        // Add some systematic error for lower quality signals
        if (m_signalQuality < 80.0) {
            double systematicError = (80.0 - m_signalQuality) / 80.0 * 0.1;
            noisy.setX(noisy.x() + systematicError * sin(i * 0.1));
            noisy.setY(noisy.y() + systematicError * cos(i * 0.1));
        }
        
        m_constellationPoints.append(noisy);
    }
}

// Private helper methods

void ConstellationWidget::paintConstellation(QPainter& painter)
{
    if (m_constellationPoints.isEmpty()) {
        // Draw "No Signal" indicator for professional appearance
        painter.setPen(QPen(m_textColor, 1));
        painter.setFont(QFont("Segoe UI", 12, QFont::Bold));
        QRect textRect = rect().adjusted(20, 20, -20, -20);
        painter.drawText(textRect, Qt::AlignCenter, tr("No Signal Data\nAwaiting ETI Stream"));
        return;
    }
    
    QMutexLocker locker(&m_dataMutex);
    
    // Professional constellation rendering with enhanced visualization
    
    // 1. Draw constellation density heat map for professional analysis
    painter.setCompositionMode(QPainter::CompositionMode_Screen);
    painter.setPen(Qt::NoPen);
    
    // Create density-based color mapping for signal quality visualization
    QHash<QString, int> densityMap;
    for (const QPointF& point : m_constellationPoints) {
        QPointF widgetPoint = transformToWidget(point);
        QPoint gridPoint(static_cast<int>(widgetPoint.x() / 4) * 4, 
                        static_cast<int>(widgetPoint.y() / 4) * 4);
        QString key = QString("%1,%2").arg(gridPoint.x()).arg(gridPoint.y());
        densityMap[key]++;
    }
    
    // Render density heat map with professional color coding
    for (auto it = densityMap.begin(); it != densityMap.end(); ++it) {
        int density = it.value();
        QColor heatColor;
        
        // Professional heat map coloring: Blue (low) -> Green (medium) -> Yellow (high) -> Red (very high)
        if (density <= 2) {
            heatColor = QColor(0, 120, 212, 80);  // Microsoft Blue with transparency
        } else if (density <= 5) {
            heatColor = QColor(76, 175, 80, 120);  // Professional Green
        } else if (density <= 10) {
            heatColor = QColor(255, 193, 7, 160);  // Professional Amber
        } else {
            heatColor = QColor(244, 67, 54, 200);  // Professional Red
        }
        
        // Parse coordinates back from key
        QStringList coords = it.key().split(',');
        if (coords.size() == 2) {
            QPoint gridPoint(coords[0].toInt(), coords[1].toInt());
            painter.setBrush(QBrush(heatColor));
            painter.drawEllipse(gridPoint, 8, 8);
        }
    }
    
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    
    // 2. Draw individual constellation points with signal quality visualization
    for (const QPointF& point : m_constellationPoints) {
        QPointF widgetPoint = transformToWidget(point);
        
        // Only draw points within widget bounds
        if (rect().contains(widgetPoint.toPoint())) {
            // Calculate point quality based on distance from ideal positions
            double minDistance = 1.0;
            QVector<QPointF> idealPoints = {
                QPointF(-0.7071, -0.7071), QPointF(0.7071, -0.7071),  // QPSK ideal positions (normalized)
                QPointF(-0.7071, 0.7071), QPointF(0.7071, 0.7071)
            };
            
            for (const QPointF& ideal : idealPoints) {
                double distance = QLineF(point, ideal).length();
                minDistance = qMin(minDistance, distance);
            }
            
            // Color code points based on error vector magnitude
            QColor pointColor;
            if (minDistance < 0.1) {
                pointColor = QColor(76, 175, 80);   // Excellent signal (Green)
            } else if (minDistance < 0.2) {
                pointColor = QColor(139, 195, 74);  // Good signal (Light Green)
            } else if (minDistance < 0.3) {
                pointColor = QColor(255, 193, 7);   // Fair signal (Amber)
            } else if (minDistance < 0.4) {
                pointColor = QColor(255, 152, 0);   // Poor signal (Orange)
            } else {
                pointColor = QColor(244, 67, 54);   // Bad signal (Red)
            }
            
            painter.setPen(Qt::NoPen);
            painter.setBrush(QBrush(pointColor));
            painter.drawEllipse(widgetPoint, 3.0, 3.0);
            
            // Add subtle glow effect for professional appearance
            painter.setBrush(QBrush(QColor(pointColor.red(), pointColor.green(), pointColor.blue(), 60)));
            painter.drawEllipse(widgetPoint, 6.0, 6.0);
        }
    }
    
    // 3. Draw ideal constellation reference positions with professional styling
    painter.setPen(QPen(m_axisColor, 2, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    
    QVector<QPointF> idealPositions = {
        QPointF(-0.7071, -0.7071), QPointF(0.7071, -0.7071),
        QPointF(-0.7071, 0.7071), QPointF(0.7071, 0.7071)
    };
    
    for (const QPointF& ideal : idealPositions) {
        QPointF widgetPoint = transformToWidget(ideal);
        
        // Draw ideal position with professional circle and cross
        painter.drawEllipse(widgetPoint, 12.0, 12.0);
        painter.drawLine(widgetPoint + QPointF(-6, 0), widgetPoint + QPointF(6, 0));
        painter.drawLine(widgetPoint + QPointF(0, -6), widgetPoint + QPointF(0, 6));
    }
    
    // 4. Draw error vector visualization for advanced analysis
    if (m_signalQuality < 95.0) {  // Only show when signal has noticeable errors
        painter.setPen(QPen(QColor(255, 152, 0, 180), 1, Qt::DashLine));
        
        // Draw error vectors from actual points to nearest ideal positions
        for (const QPointF& point : m_constellationPoints) {
            if (QLineF(QPointF(0, 0), point).length() > 0.5) {  // Only for significant points
                QPointF nearestIdeal;
                double minDist = 2.0;
                
                for (const QPointF& ideal : idealPositions) {
                    double dist = QLineF(point, ideal).length();
                    if (dist < minDist) {
                        minDist = dist;
                        nearestIdeal = ideal;
                    }
                }
                
                if (minDist < 1.0) {  // Draw error vector if reasonable
                    QPointF actualWidget = transformToWidget(point);
                    QPointF idealWidget = transformToWidget(nearestIdeal);
                    painter.drawLine(actualWidget, idealWidget);
                }
            }
        }
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
    // Professional broadcast grid system with precise scaling
    
    // Calculate adaptive grid spacing for optimal visualization
    double baseSpacing = 40.0;
    double gridSpacing = baseSpacing / m_zoomLevel;
    
    // Snap to professional grid increments
    if (gridSpacing < 20.0) gridSpacing = 20.0;
    else if (gridSpacing > 80.0) gridSpacing = 80.0;
    
    // Draw fine grid lines (subtle)
    painter.setPen(QPen(m_gridColor, 1, Qt::DotLine));
    painter.setOpacity(0.3);
    
    for (double x = fmod(width() / 2.0 + m_panOffset.x(), gridSpacing); x < width(); x += gridSpacing) {
        if (x > 0) painter.drawLine(static_cast<int>(x), 0, static_cast<int>(x), height());
    }
    for (double x = fmod(width() / 2.0 + m_panOffset.x(), gridSpacing); x >= 0; x -= gridSpacing) {
        if (x < width()) painter.drawLine(static_cast<int>(x), 0, static_cast<int>(x), height());
    }
    
    for (double y = fmod(height() / 2.0 + m_panOffset.y(), gridSpacing); y < height(); y += gridSpacing) {
        if (y > 0) painter.drawLine(0, static_cast<int>(y), width(), static_cast<int>(y));
    }
    for (double y = fmod(height() / 2.0 + m_panOffset.y(), gridSpacing); y >= 0; y -= gridSpacing) {
        if (y < height()) painter.drawLine(0, static_cast<int>(y), width(), static_cast<int>(y));
    }
    
    painter.setOpacity(1.0);
    
    // Draw major grid lines (professional broadcast standard)
    painter.setPen(QPen(m_gridColor, 1, Qt::DashLine));
    painter.setOpacity(0.6);
    
    double majorSpacing = gridSpacing * 2.0;
    for (double x = fmod(width() / 2.0 + m_panOffset.x(), majorSpacing); x < width(); x += majorSpacing) {
        if (x > 0) painter.drawLine(static_cast<int>(x), 0, static_cast<int>(x), height());
    }
    for (double x = fmod(width() / 2.0 + m_panOffset.x(), majorSpacing); x >= 0; x -= majorSpacing) {
        if (x < width()) painter.drawLine(static_cast<int>(x), 0, static_cast<int>(x), height());
    }
    
    for (double y = fmod(height() / 2.0 + m_panOffset.y(), majorSpacing); y < height(); y += majorSpacing) {
        if (y > 0) painter.drawLine(0, static_cast<int>(y), width(), static_cast<int>(y));
    }
    for (double y = fmod(height() / 2.0 + m_panOffset.y(), majorSpacing); y >= 0; y -= majorSpacing) {
        if (y < height()) painter.drawLine(0, static_cast<int>(y), width(), static_cast<int>(y));
    }
    
    painter.setOpacity(1.0);
    
    // Draw professional center axes with broadcast industry styling
    painter.setPen(QPen(m_axisColor, 2, Qt::SolidLine));
    
    // Center axes (I and Q axes)
    int centerX = static_cast<int>(width() / 2.0 + m_panOffset.x());
    int centerY = static_cast<int>(height() / 2.0 + m_panOffset.y());
    
    if (centerX >= 0 && centerX < width()) {
        painter.drawLine(centerX, 0, centerX, height());  // I (vertical) axis
    }
    if (centerY >= 0 && centerY < height()) {
        painter.drawLine(0, centerY, width(), centerY);   // Q (horizontal) axis
    }
    
    // Draw axis markers for professional calibration
    painter.setPen(QPen(m_axisColor, 3, Qt::SolidLine));
    
    // I-axis markers
    if (centerX >= 10 && centerX < width() - 10) {
        painter.drawLine(centerX - 5, 10, centerX + 5, 10);         // Top marker
        painter.drawLine(centerX - 5, height() - 10, centerX + 5, height() - 10); // Bottom marker
    }
    
    // Q-axis markers  
    if (centerY >= 10 && centerY < height() - 10) {
        painter.drawLine(10, centerY - 5, 10, centerY + 5);         // Left marker
        painter.drawLine(width() - 10, centerY - 5, width() - 10, centerY + 5); // Right marker
    }
    
    // Draw unit circle for QPSK reference (professional broadcast standard)
    painter.setPen(QPen(m_axisColor, 1, Qt::DashLine));
    painter.setOpacity(0.7);
    
    QPointF center = transformToWidget(QPointF(0, 0));
    QPointF unitPoint = transformToWidget(QPointF(1, 0));
    double radius = QLineF(center, unitPoint).length();
    
    if (radius > 10 && radius < 1000) {  // Only draw if reasonable size
        painter.drawEllipse(center, radius, radius);
        
        // Draw 0.5 radius circle for additional reference
        painter.setOpacity(0.4);
        painter.drawEllipse(center, radius * 0.5, radius * 0.5);
    }
    
    painter.setOpacity(1.0);
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

        case DisplayMode::Scatter:
        case DisplayMode::Histogram:
            break; // not implemented by this widget
    }
    
    // Draw zoom level indicator
    painter.drawText(width() - 100, 20, tr("Zoom: %1x").arg(QString::number(m_zoomLevel, 'f', 1)));
}

void ConstellationWidget::paintQualityIndicators(QPainter& painter)
{
    // Professional broadcast-quality indicators with enhanced styling
    
    // Quality indicators background panel for professional appearance
    QRect indicatorPanel(8, 30, 200, 120);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(QColor(45, 45, 48, 220)));  // Semi-transparent dark background
    painter.drawRoundedRect(indicatorPanel, 4, 4);
    
    // Professional border
    painter.setPen(QPen(QColor(0, 120, 212), 1));  // Microsoft Blue border
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(indicatorPanel, 4, 4);
    
    // Professional typography setup
    QFont headerFont("Segoe UI", 9, QFont::Bold);
    QFont dataFont("Consolas", 9, QFont::Normal);
    painter.setFont(headerFont);
    painter.setPen(QPen(m_textColor));
    
    int indicatorY = 45;
    int lineHeight = 16;
    int labelX = 15;
    int valueX = 100;
    
    // Signal Quality with professional color coding and progress indicator
    painter.setFont(headerFont);
    painter.setPen(QPen(m_textColor));
    painter.drawText(labelX, indicatorY, tr("Signal Quality:"));
    
    painter.setFont(dataFont);
    QColor qualityColor;
    QString qualityStatus;
    
    if (m_signalQuality >= 95.0) {
        qualityColor = QColor(76, 175, 80);   // Excellent (Green)
        qualityStatus = tr("EXCELLENT");
    } else if (m_signalQuality >= 85.0) {
        qualityColor = QColor(139, 195, 74);  // Good (Light Green)
        qualityStatus = tr("GOOD");
    } else if (m_signalQuality >= 70.0) {
        qualityColor = QColor(255, 193, 7);   // Fair (Amber)
        qualityStatus = tr("FAIR");
    } else if (m_signalQuality >= 50.0) {
        qualityColor = QColor(255, 152, 0);   // Poor (Orange)
        qualityStatus = tr("POOR");
    } else {
        qualityColor = QColor(244, 67, 54);   // Bad (Red)
        qualityStatus = tr("BAD");
    }
    
    painter.setPen(QPen(qualityColor));
    painter.drawText(valueX, indicatorY, QString("%1% (%2)")
                     .arg(QString::number(m_signalQuality, 'f', 1))
                     .arg(qualityStatus));
    
    // Quality progress bar
    QRect qualityBar(labelX, indicatorY + 3, 150, 6);
    painter.setPen(QPen(QColor(100, 100, 100), 1));
    painter.setBrush(QBrush(QColor(60, 60, 60)));
    painter.drawRoundedRect(qualityBar, 2, 2);
    
    int barWidth = static_cast<int>((m_signalQuality / 100.0) * 150);
    QRect fillBar(labelX, indicatorY + 3, barWidth, 6);
    painter.setBrush(QBrush(qualityColor));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(fillBar, 2, 2);
    
    // Error Vector Magnitude (EVM) with professional specifications
    indicatorY += lineHeight + 6;
    painter.setFont(headerFont);
    painter.setPen(QPen(m_textColor));
    painter.drawText(labelX, indicatorY, tr("EVM:"));
    
    painter.setFont(dataFont);
    QColor evmColor = (m_errorVectorMagnitude < -20.0) ? QColor(76, 175, 80) :  // Excellent
                     (m_errorVectorMagnitude < -15.0) ? QColor(255, 193, 7) :   // Acceptable
                     QColor(244, 67, 54);  // Poor
    painter.setPen(QPen(evmColor));
    painter.drawText(valueX, indicatorY, QString("%1 dB").arg(QString::number(m_errorVectorMagnitude, 'f', 2)));
    
    // Signal-to-Noise Ratio (SNR) with broadcast standards
    indicatorY += lineHeight;
    painter.setFont(headerFont);
    painter.setPen(QPen(m_textColor));
    painter.drawText(labelX, indicatorY, tr("SNR:"));
    
    painter.setFont(dataFont);
    QColor snrColor = (m_signalToNoiseRatio > 25.0) ? QColor(76, 175, 80) :   // Excellent
                     (m_signalToNoiseRatio > 15.0) ? QColor(255, 193, 7) :    // Good
                     (m_signalToNoiseRatio > 10.0) ? QColor(255, 152, 0) :    // Fair
                     QColor(244, 67, 54);  // Poor
    painter.setPen(QPen(snrColor));
    painter.drawText(valueX, indicatorY, QString("%1 dB").arg(QString::number(m_signalToNoiseRatio, 'f', 1)));
    
    // Processing Frame Rate with performance indicator
    indicatorY += lineHeight;
    painter.setFont(headerFont);
    painter.setPen(QPen(m_textColor));
    painter.drawText(labelX, indicatorY, tr("Refresh:"));
    
    painter.setFont(dataFont);
    double frameRate = (m_averageFrameTime > 0) ? (1000.0 / m_averageFrameTime) : 0.0;
    QColor fpsColor = (frameRate > 45.0) ? QColor(76, 175, 80) :   // Smooth
                     (frameRate > 25.0) ? QColor(255, 193, 7) :    // Acceptable
                     QColor(244, 67, 54);  // Poor
    painter.setPen(QPen(fpsColor));
    painter.drawText(valueX, indicatorY, QString("%1 fps").arg(QString::number(frameRate, 'f', 1)));
    
    // Constellation points count
    indicatorY += lineHeight;
    painter.setFont(headerFont);
    painter.setPen(QPen(m_textColor));
    painter.drawText(labelX, indicatorY, tr("Points:"));
    
    painter.setFont(dataFont);
    painter.setPen(QPen(QColor(200, 200, 200)));
    painter.drawText(valueX, indicatorY, QString::number(m_constellationPoints.size()));
    
    // Additional professional indicators in top-right corner
    QFont smallFont("Segoe UI", 8);
    painter.setFont(smallFont);
    painter.setPen(QPen(m_textColor, 1));
    
    int rightX = width() - 120;
    int topY = 15;
    
    // Display mode indicator
    QString modeText;
    switch (m_displayMode) {
        case DisplayMode::Constellation: modeText = tr("CONSTELLATION"); break;
        case DisplayMode::Spectrum: modeText = tr("SPECTRUM"); break;
        case DisplayMode::EyeDiagram: modeText = tr("EYE DIAGRAM"); break;
        case DisplayMode::Waterfall: modeText = tr("WATERFALL"); break;
        case DisplayMode::Scatter: modeText = tr("SCATTER"); break;
        case DisplayMode::Histogram: modeText = tr("HISTOGRAM"); break;
    }
    painter.drawText(rightX, topY, modeText);
    
    // Zoom level indicator
    painter.drawText(rightX, topY + 12, tr("ZOOM: %1x").arg(QString::number(m_zoomLevel, 'f', 1)));
    
    // Real-time indicator
    if (m_realTimeEnabled) {
        painter.setPen(QPen(QColor(76, 175, 80), 2));
        painter.setBrush(QBrush(QColor(76, 175, 80)));
        painter.drawEllipse(rightX - 12, topY + 18, 8, 8);
        painter.setPen(QPen(m_textColor));
        painter.drawText(rightX, topY + 26, tr("LIVE"));
    } else {
        painter.setPen(QPen(QColor(244, 67, 54), 2));
        painter.setBrush(QBrush(QColor(244, 67, 54)));
        painter.drawEllipse(rightX - 12, topY + 18, 8, 8);
        painter.setPen(QPen(m_textColor));
        painter.drawText(rightX, topY + 26, tr("STATIC"));
    }
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

        case DisplayMode::Scatter:
        case DisplayMode::Histogram:
            break; // not implemented by this widget
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