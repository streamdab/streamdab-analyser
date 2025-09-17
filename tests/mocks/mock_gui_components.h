#pragma once

#include <QWidget>
#include <QMainWindow>
#include <QTimer>
#include <QSignalSpy>
#include <QTestEventLoop>
#include <memory>

// Include GUI components to mock
#include "gui/main_window.h"
#include "gui/analyzer_widget.h"
#include "gui/constellation_widget.h"
#include "gui/service_browser.h"

/**
 * @file mock_gui_components.h
 * @brief Mock GUI components for isolated TDD testing
 * 
 * This file provides mock implementations of GUI components that allow
 * for isolated testing of individual GUI elements without requiring
 * full GUI initialization or real hardware dependencies.
 */

/**
 * @class MockMainWindow
 * @brief Mock implementation of MainWindow for testing
 * 
 * Provides a lightweight mock of MainWindow that simulates core
 * functionality without requiring full Qt application context.
 */
class MockMainWindow : public QObject
{
    Q_OBJECT

public:
    explicit MockMainWindow(QObject* parent = nullptr);
    virtual ~MockMainWindow();

    // Mock interface methods
    bool initialize() { m_initialized = true; return true; }
    bool isInitialized() const { return m_initialized; }
    bool isRealTimeMode() const { return m_realTimeMode; }
    int getUpdateRate() const { return m_updateRate; }
    
    // Mock component getters
    QWidget* getAnalyzerWidget() const { return m_mockAnalyzerWidget.get(); }
    QWidget* getConstellationWidget() const { return m_mockConstellationWidget.get(); }
    QWidget* getServiceBrowser() const { return m_mockServiceBrowser.get(); }

    // Mock control methods
    void setRealTimeMode(bool enabled);
    void setUpdateRate(int rate);
    void simulateEtiFileOpen(const QString& filename);
    void simulateMenuAction(const QString& actionName);

signals:
    void etiFileOpened(const QString& filename);
    void etiFileClosed();
    void realTimeModeChanged(bool enabled);
    void settingsChanged();

private:
    bool m_initialized;
    bool m_realTimeMode;
    int m_updateRate;
    QString m_currentFile;
    
    std::unique_ptr<QWidget> m_mockAnalyzerWidget;
    std::unique_ptr<QWidget> m_mockConstellationWidget;
    std::unique_ptr<QWidget> m_mockServiceBrowser;
};

/**
 * @class MockAnalyzerWidget
 * @brief Mock implementation of AnalyzerWidget for testing
 */
class MockAnalyzerWidget : public QObject
{
    Q_OBJECT

public:
    enum class AnalysisMode { File, RealTime, Network, Playback };
    enum class DisplayMode { Overview, Detailed, Monitoring, Debug };

    explicit MockAnalyzerWidget(QObject* parent = nullptr);

    // Mock interface
    bool initialize() { m_initialized = true; return true; }
    bool startAnalysis();
    void stopAnalysis();
    bool isAnalysisRunning() const { return m_analysisRunning; }
    
    // Mock getters
    AnalysisMode getAnalysisMode() const { return m_analysisMode; }
    DisplayMode getDisplayMode() const { return m_displayMode; }
    double getCurrentFrameRate() const { return m_currentFrameRate; }
    quint64 getProcessedFrames() const { return m_processedFrames; }
    quint32 getErrorCount() const { return m_errorCount; }
    
    // Mock setters
    void setAnalysisMode(AnalysisMode mode);
    void setDisplayMode(DisplayMode mode);
    void setEtiProcessor(QObject* processor);

public slots:
    void updateDisplay();
    void clearDisplay();
    void resetStatistics();

signals:
    void analysisStarted();
    void analysisStopped();
    void errorDetected(const QString& error);
    void frameRateChanged(double frameRate);
    void statisticsUpdated(quint64 frames, quint32 errors);

private slots:
    void handleUpdateTimer();

private:
    bool m_initialized;
    bool m_analysisRunning;
    AnalysisMode m_analysisMode;
    DisplayMode m_displayMode;
    double m_currentFrameRate;
    quint64 m_processedFrames;
    quint32 m_errorCount;
    QTimer* m_updateTimer;
    QObject* m_etiProcessor;
};

/**
 * @class MockConstellationWidget
 * @brief Mock implementation of ConstellationWidget for testing
 */
class MockConstellationWidget : public QObject
{
    Q_OBJECT

public:
    explicit MockConstellationWidget(QObject* parent = nullptr);

    bool initialize() { m_initialized = true; return true; }
    void startRealTimeUpdates();
    void stopRealTimeUpdates();
    bool isRealTimeUpdating() const { return m_realTimeUpdating; }
    
    // Mock visualization methods
    void updateConstellationData(const QVector<QPointF>& points);
    void setZoomLevel(double zoom);
    void setPanOffset(const QPointF& offset);
    
    double getZoomLevel() const { return m_zoomLevel; }
    QPointF getPanOffset() const { return m_panOffset; }
    int getPointCount() const { return m_pointCount; }

signals:
    void realTimeUpdatesStarted();
    void realTimeUpdatesStopped();
    void zoomChanged(double zoom);
    void panChanged(const QPointF& offset);

private:
    bool m_initialized;
    bool m_realTimeUpdating;
    double m_zoomLevel;
    QPointF m_panOffset;
    int m_pointCount;
    QTimer* m_renderTimer;
};

/**
 * @class MockServiceBrowser
 * @brief Mock implementation of ServiceBrowser for testing
 */
class MockServiceBrowser : public QObject
{
    Q_OBJECT

public:
    struct ServiceInfo {
        int serviceId;
        QString serviceName;
        QString serviceType;
        bool isActive;
    };

    explicit MockServiceBrowser(QObject* parent = nullptr);

    bool initialize() { m_initialized = true; return true; }
    void addService(const ServiceInfo& service);
    void removeService(int serviceId);
    void clearServices();
    
    QList<ServiceInfo> getServices() const { return m_services; }
    ServiceInfo getSelectedService() const { return m_selectedService; }
    int getServiceCount() const { return m_services.size(); }

public slots:
    void selectService(int serviceId);
    void refreshServices();

signals:
    void serviceSelected(int serviceId);
    void serviceAdded(int serviceId);
    void serviceRemoved(int serviceId);
    void servicesCleared();

private:
    bool m_initialized;
    QList<ServiceInfo> m_services;
    ServiceInfo m_selectedService;
};

/**
 * @class GuiTestUtilities
 * @brief Utility functions for GUI testing
 * 
 * Provides common functionality needed across multiple GUI test suites
 * including performance measurement, memory tracking, and event simulation.
 */
class GuiTestUtilities
{
public:
    /**
     * @brief Measure frame rate of a widget update cycle
     * @param updateFunction Function to call for each update
     * @param durationMs Duration to measure in milliseconds
     * @return Achieved frame rate in FPS
     */
    static double measureFrameRate(std::function<void()> updateFunction, int durationMs = 1000);

    /**
     * @brief Measure memory usage of a test operation
     * @param testFunction Function to execute and measure
     * @return Memory usage delta in bytes
     */
    static qint64 measureMemoryUsage(std::function<void()> testFunction);

    /**
     * @brief Wait for GUI events with timeout
     * @param timeoutMs Maximum wait time in milliseconds
     */
    static void waitForGuiEvents(int timeoutMs = 100);

    /**
     * @brief Simulate mouse click on widget
     * @param widget Target widget
     * @param pos Position to click (relative to widget)
     * @param button Mouse button to simulate
     */
    static void simulateMouseClick(QWidget* widget, const QPoint& pos, Qt::MouseButton button = Qt::LeftButton);

    /**
     * @brief Simulate key press on widget
     * @param widget Target widget
     * @param key Key to simulate
     * @param modifier Modifier keys
     */
    static void simulateKeyPress(QWidget* widget, Qt::Key key, Qt::KeyboardModifiers modifier = Qt::NoModifier);

    /**
     * @brief Validate widget layout consistency
     * @param widget Widget to validate
     * @return true if layout is consistent
     */
    static bool validateWidgetLayout(QWidget* widget);

    /**
     * @brief Create test data directory
     * @return Path to test data directory
     */
    static QString createTestDataDirectory();

    /**
     * @brief Cleanup test data directory
     * @param path Path to cleanup
     */
    static void cleanupTestDataDirectory(const QString& path);

    /**
     * @brief Get current memory usage (platform-specific)
     * @return Current memory usage in bytes
     */
    static qint64 getCurrentMemoryUsage();

    /**
     * @brief Setup test environment for GUI testing
     */
    static void setupTestEnvironment();

    /**
     * @brief Cleanup test environment
     */
    static void cleanupTestEnvironment();

private:
    static QElapsedTimer s_performanceTimer;
    static qint64 s_baselineMemory;
};

/**
 * @class GuiTestDataGenerator
 * @brief Generate test data for GUI components
 * 
 * Provides standardized test data generation for consistent GUI testing
 * across different components and test scenarios.
 */
class GuiTestDataGenerator
{
public:
    /**
     * @brief Generate constellation points for testing
     * @param count Number of points to generate
     * @param noise Noise level (0.0 to 1.0)
     * @return Vector of constellation points
     */
    static QVector<QPointF> generateConstellationPoints(int count, double noise = 0.1);

    /**
     * @brief Generate service list for testing
     * @param count Number of services to generate
     * @return List of test services
     */
    static QList<MockServiceBrowser::ServiceInfo> generateTestServices(int count);

    /**
     * @brief Generate analysis statistics for testing
     * @param frameCount Number of frames processed
     * @param errorRate Error rate (0.0 to 1.0)
     * @return Generated statistics
     */
    struct AnalysisStats {
        quint64 processedFrames;
        quint32 errorCount;
        double frameRate;
        double bitRate;
        double signalQuality;
    };
    static AnalysisStats generateAnalysisStats(quint64 frameCount, double errorRate = 0.01);

    /**
     * @brief Generate ETI frame timing data
     * @param frameCount Number of frames
     * @param targetFps Target frame rate
     * @return List of frame timestamps
     */
    static QList<qint64> generateFrameTimings(int frameCount, double targetFps = 60.0);
};

// Include Qt MOC support
#include "mock_gui_components.moc"