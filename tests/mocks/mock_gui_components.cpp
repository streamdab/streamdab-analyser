/**
 * @file mock_gui_components.cpp
 * @brief Implementation of mock GUI components for TDD testing
 */

#include "mock_gui_components.h"
#include <QApplication>
#include <QTimer>
#include <QTest>
#include <QElapsedTimer>
#include <QDir>
#include <QStandardPaths>
#include <random>

// MockMainWindow Implementation

MockMainWindow::MockMainWindow(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
    , m_realTimeMode(false)
    , m_updateRate(60)
{
    // Create mock child widgets
    m_mockAnalyzerWidget = std::make_unique<QWidget>();
    m_mockConstellationWidget = std::make_unique<QWidget>();
    m_mockServiceBrowser = std::make_unique<QWidget>();
}

MockMainWindow::~MockMainWindow() = default;

void MockMainWindow::setRealTimeMode(bool enabled)
{
    if (m_realTimeMode != enabled) {
        m_realTimeMode = enabled;
        emit realTimeModeChanged(enabled);
    }
}

void MockMainWindow::setUpdateRate(int rate)
{
    m_updateRate = qBound(1, rate, 120);  // Limit to reasonable range
}

void MockMainWindow::simulateEtiFileOpen(const QString& filename)
{
    m_currentFile = filename;
    emit etiFileOpened(filename);
}

void MockMainWindow::simulateMenuAction(const QString& actionName)
{
    if (actionName == "openFile") {
        simulateEtiFileOpen("/test/sample.eti");
    } else if (actionName == "closeFile") {
        m_currentFile.clear();
        emit etiFileClosed();
    } else if (actionName == "settings") {
        emit settingsChanged();
    }
}

// MockAnalyzerWidget Implementation

MockAnalyzerWidget::MockAnalyzerWidget(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
    , m_analysisRunning(false)
    , m_analysisMode(AnalysisMode::File)
    , m_displayMode(DisplayMode::Overview)
    , m_currentFrameRate(0.0)
    , m_processedFrames(0)
    , m_errorCount(0)
    , m_etiProcessor(nullptr)
{
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(16);  // ~60 FPS
    connect(m_updateTimer, &QTimer::timeout, this, &MockAnalyzerWidget::handleUpdateTimer);
}

bool MockAnalyzerWidget::startAnalysis()
{
    if (!m_analysisRunning) {
        m_analysisRunning = true;
        m_updateTimer->start();
        emit analysisStarted();
        return true;
    }
    return false;
}

void MockAnalyzerWidget::stopAnalysis()
{
    if (m_analysisRunning) {
        m_analysisRunning = false;
        m_updateTimer->stop();
        emit analysisStopped();
    }
}

void MockAnalyzerWidget::setAnalysisMode(AnalysisMode mode)
{
    m_analysisMode = mode;
}

void MockAnalyzerWidget::setDisplayMode(DisplayMode mode)
{
    m_displayMode = mode;
}

void MockAnalyzerWidget::setEtiProcessor(QObject* processor)
{
    m_etiProcessor = processor;
}

void MockAnalyzerWidget::updateDisplay()
{
    // Simulate frame processing
    m_processedFrames++;
    
    // Simulate occasional errors (1% error rate)
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.0, 1.0);
    
    if (dis(gen) < 0.01) {  // 1% error rate
        m_errorCount++;
        emit errorDetected("Simulated frame error");
    }
    
    // Calculate frame rate
    static QElapsedTimer frameTimer;
    static int frameCount = 0;
    
    if (!frameTimer.isValid()) {
        frameTimer.start();
    }
    
    frameCount++;
    if (frameTimer.elapsed() >= 1000) {  // Update every second
        m_currentFrameRate = (frameCount * 1000.0) / frameTimer.elapsed();
        emit frameRateChanged(m_currentFrameRate);
        emit statisticsUpdated(m_processedFrames, m_errorCount);
        frameTimer.restart();
        frameCount = 0;
    }
}

void MockAnalyzerWidget::clearDisplay()
{
    m_processedFrames = 0;
    m_errorCount = 0;
    m_currentFrameRate = 0.0;
}

void MockAnalyzerWidget::resetStatistics()
{
    clearDisplay();
}

void MockAnalyzerWidget::handleUpdateTimer()
{
    if (m_analysisRunning) {
        updateDisplay();
    }
}

// MockConstellationWidget Implementation

MockConstellationWidget::MockConstellationWidget(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
    , m_realTimeUpdating(false)
    , m_zoomLevel(1.0)
    , m_panOffset(0.0, 0.0)
    , m_pointCount(0)
{
    m_renderTimer = new QTimer(this);
    m_renderTimer->setInterval(16);  // ~60 FPS
}

void MockConstellationWidget::startRealTimeUpdates()
{
    if (!m_realTimeUpdating) {
        m_realTimeUpdating = true;
        m_renderTimer->start();
        emit realTimeUpdatesStarted();
    }
}

void MockConstellationWidget::stopRealTimeUpdates()
{
    if (m_realTimeUpdating) {
        m_realTimeUpdating = false;
        m_renderTimer->stop();
        emit realTimeUpdatesStopped();
    }
}

void MockConstellationWidget::updateConstellationData(const QVector<QPointF>& points)
{
    m_pointCount = points.size();
}

void MockConstellationWidget::setZoomLevel(double zoom)
{
    if (zoom != m_zoomLevel) {
        m_zoomLevel = qBound(0.1, zoom, 10.0);  // Reasonable zoom range
        emit zoomChanged(m_zoomLevel);
    }
}

void MockConstellationWidget::setPanOffset(const QPointF& offset)
{
    if (offset != m_panOffset) {
        m_panOffset = offset;
        emit panChanged(m_panOffset);
    }
}

// MockServiceBrowser Implementation

MockServiceBrowser::MockServiceBrowser(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
{
    m_selectedService.serviceId = -1;  // No selection
}

void MockServiceBrowser::addService(const ServiceInfo& service)
{
    m_services.append(service);
    emit serviceAdded(service.serviceId);
}

void MockServiceBrowser::removeService(int serviceId)
{
    for (int i = 0; i < m_services.size(); ++i) {
        if (m_services[i].serviceId == serviceId) {
            m_services.removeAt(i);
            emit serviceRemoved(serviceId);
            break;
        }
    }
}

void MockServiceBrowser::clearServices()
{
    m_services.clear();
    m_selectedService.serviceId = -1;
    emit servicesCleared();
}

void MockServiceBrowser::selectService(int serviceId)
{
    for (const auto& service : m_services) {
        if (service.serviceId == serviceId) {
            m_selectedService = service;
            emit serviceSelected(serviceId);
            break;
        }
    }
}

void MockServiceBrowser::refreshServices()
{
    // Simulate service refresh - could add/remove services
}

// GuiTestUtilities Implementation

QElapsedTimer GuiTestUtilities::s_performanceTimer;
qint64 GuiTestUtilities::s_baselineMemory = 0;

double GuiTestUtilities::measureFrameRate(std::function<void()> updateFunction, int durationMs)
{
    s_performanceTimer.start();
    int frameCount = 0;
    
    while (s_performanceTimer.elapsed() < durationMs) {
        updateFunction();
        frameCount++;
        waitForGuiEvents(1);  // Minimal wait to allow processing
    }
    
    qint64 elapsed = s_performanceTimer.elapsed();
    return (frameCount * 1000.0) / elapsed;
}

qint64 GuiTestUtilities::measureMemoryUsage(std::function<void()> testFunction)
{
    qint64 initialMemory = getCurrentMemoryUsage();
    testFunction();
    qint64 finalMemory = getCurrentMemoryUsage();
    return finalMemory - initialMemory;
}

void GuiTestUtilities::waitForGuiEvents(int timeoutMs)
{
    QTest::qWait(timeoutMs);
}

void GuiTestUtilities::simulateMouseClick(QWidget* widget, const QPoint& pos, Qt::MouseButton button)
{
    if (widget) {
        QTest::mouseClick(widget, button, Qt::NoModifier, pos);
    }
}

void GuiTestUtilities::simulateKeyPress(QWidget* widget, Qt::Key key, Qt::KeyboardModifiers modifier)
{
    if (widget) {
        QTest::keyPress(widget, key, modifier);
    }
}

bool GuiTestUtilities::validateWidgetLayout(QWidget* widget)
{
    if (!widget) return false;
    
    // Basic layout validation
    return widget->isVisible() && 
           widget->size().isValid() &&
           widget->size().width() > 0 &&
           widget->size().height() > 0;
}

QString GuiTestUtilities::createTestDataDirectory()
{
    QString testDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/eti_analyzer_tests";
    QDir dir;
    dir.mkpath(testDir);
    return testDir;
}

void GuiTestUtilities::cleanupTestDataDirectory(const QString& path)
{
    QDir dir(path);
    if (dir.exists()) {
        dir.removeRecursively();
    }
}

qint64 GuiTestUtilities::getCurrentMemoryUsage()
{
    // Simplified memory usage measurement
    // In a real implementation, this would use platform-specific APIs
    return QCoreApplication::applicationPid() * 1024;  // Placeholder
}

void GuiTestUtilities::setupTestEnvironment()
{
    s_baselineMemory = getCurrentMemoryUsage();
}

void GuiTestUtilities::cleanupTestEnvironment()
{
    // Cleanup any test resources
}

// GuiTestDataGenerator Implementation

QVector<QPointF> GuiTestDataGenerator::generateConstellationPoints(int count, double noise)
{
    QVector<QPointF> points;
    points.reserve(count);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-1.0, 1.0);
    std::uniform_real_distribution<> noiseDis(-noise, noise);
    
    // Generate QPSK-like constellation with noise
    for (int i = 0; i < count; ++i) {
        double baseI = (i % 2 == 0) ? -0.7 : 0.7;
        double baseQ = ((i / 2) % 2 == 0) ? -0.7 : 0.7;
        
        double noiseI = noiseDis(gen);
        double noiseQ = noiseDis(gen);
        
        points.append(QPointF(baseI + noiseI, baseQ + noiseQ));
    }
    
    return points;
}

QList<MockServiceBrowser::ServiceInfo> GuiTestDataGenerator::generateTestServices(int count)
{
    QList<MockServiceBrowser::ServiceInfo> services;
    
    QStringList serviceNames = {
        "BBC Radio 1", "BBC Radio 2", "BBC Radio 4", "Classic FM",
        "Heart FM", "Capital FM", "Smooth Radio", "Jazz FM",
        "Talk Sport", "Virgin Radio", "Absolute Radio", "Kiss FM"
    };
    
    QStringList serviceTypes = {"DAB", "DAB+", "DMB"};
    
    for (int i = 0; i < count && i < serviceNames.size(); ++i) {
        MockServiceBrowser::ServiceInfo service;
        service.serviceId = i + 1;
        service.serviceName = serviceNames[i];
        service.serviceType = serviceTypes[i % serviceTypes.size()];
        service.isActive = (i % 3 != 0);  // Most services active
        services.append(service);
    }
    
    return services;
}

GuiTestDataGenerator::AnalysisStats GuiTestDataGenerator::generateAnalysisStats(quint64 frameCount, double errorRate)
{
    AnalysisStats stats;
    stats.processedFrames = frameCount;
    stats.errorCount = static_cast<quint32>(frameCount * errorRate);
    stats.frameRate = 59.8 + (static_cast<double>(rand()) / RAND_MAX) * 0.4;  // 59.8-60.2 FPS
    stats.bitRate = 1152.0 + (static_cast<double>(rand()) / RAND_MAX) * 10.0;  // Typical DAB bit rate
    stats.signalQuality = 0.85 + (static_cast<double>(rand()) / RAND_MAX) * 0.1;  // 85-95% quality
    
    return stats;
}

QList<qint64> GuiTestDataGenerator::generateFrameTimings(int frameCount, double targetFps)
{
    QList<qint64> timings;
    timings.reserve(frameCount);
    
    qint64 targetInterval = static_cast<qint64>(1000.0 / targetFps);
    qint64 currentTime = 0;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> jitter(-2.0, 2.0);  // ±2ms jitter
    
    for (int i = 0; i < frameCount; ++i) {
        qint64 interval = targetInterval + static_cast<qint64>(jitter(gen));
        currentTime += interval;
        timings.append(currentTime);
    }
    
    return timings;
}