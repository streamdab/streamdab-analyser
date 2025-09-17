#pragma once

/**
 * @file gui_test_framework.h
 * @brief Comprehensive Qt GUI testing framework for professional applications
 * 
 * This framework provides advanced GUI testing capabilities specifically designed
 * for professional broadcast applications requiring real-time performance,
 * cross-platform consistency, and memory leak detection.
 */

#include <QtTest/QtTest>
#include <QApplication>
#include <QWidget>
#include <QMainWindow>
#include <QTimer>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <QTestEventLoop>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QScreen>
#include <QStyleFactory>
#include <memory>
#include <functional>

/**
 * @class GuiTestFramework
 * @brief Advanced GUI testing framework for ETI Stream Analyzer
 * 
 * Provides comprehensive testing infrastructure for GUI components with:
 * - Real-time performance validation (60 FPS target)
 * - Memory leak detection and monitoring
 * - Cross-platform consistency testing
 * - User interaction simulation
 * - Visual regression testing
 * - Professional broadcast interface validation
 */
class GuiTestFramework : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Performance requirements for GUI testing
     */
    struct PerformanceRequirements {
        double minFrameRate = 55.0;          ///< Minimum acceptable frame rate (FPS)
        double maxFrameRate = 65.0;          ///< Maximum acceptable frame rate (FPS)
        qint64 maxMemoryGrowth = 50 * 1024 * 1024;  ///< Max memory growth (50MB)
        qint64 maxRenderTime = 20;           ///< Max single frame render time (ms)
        double maxCpuUsage = 0.8;            ///< Max CPU usage (80%)
        int maxUIResponseTime = 100;         ///< Max UI response time (ms)
    };

    /**
     * @brief Visual consistency requirements
     */
    struct VisualRequirements {
        QSize minWindowSize = QSize(800, 600);     ///< Minimum window size
        QSize maxWindowSize = QSize(3840, 2160);   ///< Maximum window size (4K)
        double minDpiScaling = 1.0;                ///< Minimum DPI scaling
        double maxDpiScaling = 3.0;                ///< Maximum DPI scaling
        QStringList requiredStyles = {"Fusion", "Windows", "macOS"};  ///< Required styles
    };

    /**
     * @brief Memory validation requirements
     */
    struct MemoryRequirements {
        qint64 maxStartupMemory = 100 * 1024 * 1024;  ///< Max startup memory (100MB)
        qint64 maxRuntimeMemory = 500 * 1024 * 1024;  ///< Max runtime memory (500MB)
        double maxMemoryFragmentation = 0.2;          ///< Max fragmentation (20%)
        int memoryLeakThreshold = 1024 * 1024;        ///< Leak threshold (1MB)
    };

    explicit GuiTestFramework(QObject* parent = nullptr);
    ~GuiTestFramework();

    /**
     * @brief Initialize the GUI testing framework
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * @brief Set performance requirements for testing
     * @param requirements Performance requirements
     */
    void setPerformanceRequirements(const PerformanceRequirements& requirements);

    /**
     * @brief Set visual requirements for testing
     * @param requirements Visual requirements
     */
    void setVisualRequirements(const VisualRequirements& requirements);

    /**
     * @brief Set memory requirements for testing
     * @param requirements Memory requirements
     */
    void setMemoryRequirements(const MemoryRequirements& requirements);

    // === PERFORMANCE TESTING METHODS ===

    /**
     * @brief Test widget rendering performance
     * @param widget Widget to test
     * @param testDuration Test duration in milliseconds
     * @return Performance test results
     */
    struct PerformanceResults {
        double averageFrameRate;
        double minFrameRate;
        double maxFrameRate;
        qint64 averageRenderTime;
        qint64 maxRenderTime;
        qint64 memoryUsage;
        double cpuUsage;
        bool passedRequirements;
    };
    PerformanceResults testRenderingPerformance(QWidget* widget, int testDuration = 5000);

    /**
     * @brief Test real-time update performance
     * @param updateFunction Function to call for updates
     * @param testDuration Test duration in milliseconds
     * @return Performance results
     */
    PerformanceResults testRealTimeUpdates(std::function<void()> updateFunction, int testDuration = 5000);

    /**
     * @brief Test memory usage under sustained load
     * @param loadFunction Function to generate load
     * @param testDuration Test duration in milliseconds
     * @return Memory usage results
     */
    struct MemoryResults {
        qint64 initialMemory;
        qint64 peakMemory;
        qint64 finalMemory;
        qint64 memoryGrowth;
        double averageMemory;
        bool memoryLeakDetected;
        bool passedRequirements;
    };
    MemoryResults testMemoryUsage(std::function<void()> loadFunction, int testDuration = 10000);

    // === USER INTERACTION TESTING ===

    /**
     * @brief Test mouse interaction handling
     * @param widget Target widget
     * @param interactions List of mouse interactions to simulate
     * @return true if all interactions handled correctly
     */
    struct MouseInteraction {
        QPoint position;
        Qt::MouseButton button;
        Qt::KeyboardModifiers modifiers;
        int delayMs;
    };
    bool testMouseInteractions(QWidget* widget, const QList<MouseInteraction>& interactions);

    /**
     * @brief Test keyboard interaction handling
     * @param widget Target widget
     * @param keySequences List of key sequences to simulate
     * @return true if all key sequences handled correctly
     */
    struct KeySequence {
        Qt::Key key;
        Qt::KeyboardModifiers modifiers;
        int delayMs;
    };
    bool testKeyboardInteractions(QWidget* widget, const QList<KeySequence>& keySequences);

    /**
     * @brief Test drag and drop operations
     * @param sourceWidget Source widget
     * @param targetWidget Target widget
     * @param mimeData Data to drag
     * @return true if drag and drop successful
     */
    bool testDragAndDrop(QWidget* sourceWidget, QWidget* targetWidget, QMimeData* mimeData);

    // === LAYOUT AND VISUAL TESTING ===

    /**
     * @brief Test widget layout consistency across different sizes
     * @param widget Widget to test
     * @param testSizes List of sizes to test
     * @return Layout test results
     */
    struct LayoutResults {
        bool layoutValid;
        bool minSizeRespected;
        bool maxSizeRespected;
        bool proportionsCorrect;
        QStringList errors;
    };
    LayoutResults testLayoutConsistency(QWidget* widget, const QList<QSize>& testSizes);

    /**
     * @brief Test cross-platform visual consistency
     * @param widget Widget to test
     * @param styles List of styles to test
     * @return Visual consistency results
     */
    struct VisualResults {
        bool consistentAcrossStyles;
        bool fontsConsistent;
        bool colorsConsistent;
        bool iconsConsistent;
        QStringList inconsistencies;
    };
    VisualResults testVisualConsistency(QWidget* widget, const QStringList& styles);

    /**
     * @brief Test high DPI scaling support
     * @param widget Widget to test
     * @param scalingFactors List of scaling factors to test
     * @return High DPI test results
     */
    struct DpiResults {
        bool scalingSupported;
        bool textReadable;
        bool iconsSharp;
        bool layoutPreserved;
        QList<double> failedScalings;
    };
    DpiResults testHighDpiSupport(QWidget* widget, const QList<double>& scalingFactors);

    // === MEMORY LEAK DETECTION ===

    /**
     * @brief Start memory leak monitoring
     * @param widget Widget to monitor
     */
    void startMemoryMonitoring(QWidget* widget);

    /**
     * @brief Stop memory leak monitoring and get results
     * @return Memory leak analysis results
     */
    struct MemoryLeakResults {
        bool leaksDetected;
        qint64 totalLeakage;
        QStringList leakSources;
        qint64 peakMemoryUsage;
    };
    MemoryLeakResults stopMemoryMonitoring();

    // === STRESS TESTING ===

    /**
     * @brief Perform stress test on widget
     * @param widget Widget to stress test
     * @param stressFunction Function to generate stress
     * @param iterations Number of stress iterations
     * @return Stress test results
     */
    struct StressResults {
        bool widgetStable;
        int successfulIterations;
        int failedIterations;
        QStringList errors;
        PerformanceResults performance;
        MemoryResults memory;
    };
    StressResults performStressTest(QWidget* widget, std::function<void()> stressFunction, int iterations = 1000);

    // === PROFESSIONAL BROADCAST VALIDATION ===

    /**
     * @brief Validate professional broadcast interface requirements
     * @param widget Widget to validate
     * @return Broadcast compliance results
     */
    struct BroadcastComplianceResults {
        bool colorSchemeCompliant;
        bool terminologyCorrect;
        bool layoutProfessional;
        bool responsivenessSufficient;
        bool accessibilitySupported;
        QStringList complianceIssues;
    };
    BroadcastComplianceResults validateBroadcastCompliance(QWidget* widget);

    // === UTILITY METHODS ===

    /**
     * @brief Wait for GUI events with timeout
     * @param timeoutMs Maximum wait time
     */
    static void waitForEvents(int timeoutMs = 100);

    /**
     * @brief Get current system performance metrics
     * @return System performance data
     */
    struct SystemMetrics {
        double cpuUsage;
        qint64 memoryUsage;
        qint64 availableMemory;
        double diskUsage;
    };
    static SystemMetrics getSystemMetrics();

    /**
     * @brief Take screenshot of widget
     * @param widget Widget to capture
     * @param filename Output filename
     * @return true if screenshot saved successfully
     */
    static bool takeScreenshot(QWidget* widget, const QString& filename);

    /**
     * @brief Compare widget screenshots for visual regression
     * @param expected Expected screenshot path
     * @param actual Actual screenshot path
     * @param tolerance Comparison tolerance (0.0-1.0)
     * @return true if screenshots match within tolerance
     */
    static bool compareScreenshots(const QString& expected, const QString& actual, double tolerance = 0.05);

signals:
    /**
     * @brief Emitted when performance test completes
     * @param results Performance test results
     */
    void performanceTestCompleted(const PerformanceResults& results);

    /**
     * @brief Emitted when memory test completes
     * @param results Memory test results
     */
    void memoryTestCompleted(const MemoryResults& results);

    /**
     * @brief Emitted when visual test completes
     * @param results Visual test results
     */
    void visualTestCompleted(const VisualResults& results);

    /**
     * @brief Emitted when memory leak detected
     * @param widget Widget with leak
     * @param leakSize Size of leak in bytes
     */
    void memoryLeakDetected(QWidget* widget, qint64 leakSize);

private:
    /**
     * @brief Monitor widget performance in background thread
     */
    class PerformanceMonitor : public QThread
    {
        Q_OBJECT
    public:
        explicit PerformanceMonitor(QWidget* widget, QObject* parent = nullptr);
        void setTestDuration(int durationMs);
        void run() override;
        
        PerformanceResults getResults() const { return m_results; }
        
    signals:
        void performanceUpdate(double currentFps, qint64 currentMemory);
        
    private:
        QWidget* m_widget;
        int m_testDuration;
        PerformanceResults m_results;
        QMutex m_mutex;
        bool m_running;
    };

    /**
     * @brief Monitor memory usage in background thread
     */
    class MemoryMonitor : public QThread
    {
        Q_OBJECT
    public:
        explicit MemoryMonitor(QWidget* widget, QObject* parent = nullptr);
        void run() override;
        void stopMonitoring();
        
        MemoryResults getResults() const { return m_results; }
        
    signals:
        void memoryUpdate(qint64 currentMemory);
        void leakDetected(qint64 leakSize);
        
    private:
        QWidget* m_widget;
        MemoryResults m_results;
        QMutex m_mutex;
        QWaitCondition m_condition;
        bool m_monitoring;
        qint64 m_baselineMemory;
    };

    // Framework state
    bool m_initialized;
    PerformanceRequirements m_performanceReqs;
    VisualRequirements m_visualReqs;
    MemoryRequirements m_memoryReqs;

    // Monitoring components
    std::unique_ptr<PerformanceMonitor> m_performanceMonitor;
    std::unique_ptr<MemoryMonitor> m_memoryMonitor;

    // Utility methods
    qint64 getCurrentMemoryUsage() const;
    double getCurrentCpuUsage() const;
    bool validateWidgetHierarchy(QWidget* widget) const;
    QStringList analyzeMemoryUsage(QWidget* widget) const;
    bool checkAccessibilitySupport(QWidget* widget) const;

    // Constants
    static constexpr int DEFAULT_TEST_DURATION = 5000;
    static constexpr int MEMORY_SAMPLE_INTERVAL = 100;
    static constexpr int PERFORMANCE_SAMPLE_INTERVAL = 16;  // ~60 FPS
    static constexpr double CPU_USAGE_THRESHOLD = 0.8;     // 80%
    static constexpr int UI_RESPONSE_TIMEOUT = 1000;       // 1 second
};

/**
 * @class GuiTestRunner
 * @brief Test runner for automated GUI testing
 * 
 * Provides automated test execution with reporting and integration
 * with continuous integration systems.
 */
class GuiTestRunner : public QObject
{
    Q_OBJECT

public:
    explicit GuiTestRunner(QObject* parent = nullptr);

    /**
     * @brief Add test suite to runner
     * @param testSuite Test suite to add
     */
    void addTestSuite(QObject* testSuite);

    /**
     * @brief Run all test suites
     * @param outputPath Path for test results
     * @return true if all tests passed
     */
    bool runAllTests(const QString& outputPath = QString());

    /**
     * @brief Generate test report
     * @param format Report format ("html", "xml", "json")
     * @param outputPath Output file path
     * @return true if report generated successfully
     */
    bool generateReport(const QString& format, const QString& outputPath);

private:
    QList<QObject*> m_testSuites;
    QStringList m_testResults;
    int m_totalTests;
    int m_passedTests;
    int m_failedTests;
};

// Include MOC
#include "gui_test_framework.moc"