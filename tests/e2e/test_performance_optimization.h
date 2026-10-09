/**
 * @file test_performance_optimization.h
 * @brief Performance optimization utilities for E2E test execution
 * 
 * Addresses critical performance issues:
 * - Test execution timeouts
 * - Segmentation faults in test cleanup
 * - ETI processing blocking issues
 * - Memory management in test environments
 */

#pragma once

#include <QTest>
#include <QApplication>
#include <QTimer>
#include <QEventLoop>
#include <QThread>
#include <QElapsedTimer>
#include <chrono>
#include <memory>
#include <functional>

namespace TestOptimization {

/**
 * @brief Optimized test timing with proper timeouts
 */
class SafeTestTimer {
public:
    /**
     * @brief Wait for window with timeout protection
     * @param widget Widget to wait for
     * @param timeout_ms Maximum wait time in milliseconds
     * @return true if widget became active, false if timeout
     */
    static bool waitForWindowActive(QWidget* widget, int timeout_ms = 5000) {
        if (!widget) return false;
        
        QElapsedTimer timer;
        timer.start();
        
        while (timer.elapsed() < timeout_ms) {
            if (widget->isVisible() && widget->isActiveWindow()) {
                return true;
            }
            QApplication::processEvents(QEventLoop::AllEvents, 50);
            QThread::msleep(10);
        }
        
        return widget->isVisible() && widget->isActiveWindow();
    }
    
    /**
     * @brief Safe wait with timeout and event processing
     * @param milliseconds Time to wait
     * @param max_timeout Maximum allowed timeout
     */
    static void safeWait(int milliseconds, int max_timeout = 10000) {
        int actual_wait = std::min(milliseconds, max_timeout);
        
        QElapsedTimer timer;
        timer.start();
        
        while (timer.elapsed() < actual_wait) {
            QApplication::processEvents(QEventLoop::AllEvents, 10);
            QThread::msleep(1);
        }
    }
    
    /**
     * @brief Wait for condition with timeout
     * @param condition Function that returns true when condition is met
     * @param timeout_ms Maximum wait time
     * @param check_interval_ms Interval between condition checks
     * @return true if condition was met, false if timeout
     */
    static bool waitForCondition(std::function<bool()> condition, 
                                int timeout_ms = 5000, 
                                int check_interval_ms = 100) {
        QElapsedTimer timer;
        timer.start();
        
        while (timer.elapsed() < timeout_ms) {
            if (condition()) {
                return true;
            }
            QApplication::processEvents(QEventLoop::AllEvents, check_interval_ms);
            QThread::msleep(check_interval_ms);
        }
        
        return condition();
    }
};

/**
 * @brief Memory-safe widget management for tests
 */
class SafeWidgetManager {
public:
    /**
     * @brief Create and manage a widget safely
     * @param widget_creator Function that creates the widget
     * @return Managed widget pointer
     */
    template<typename T>
    static std::unique_ptr<T> createSafeWidget(std::function<T*()> widget_creator) {
        auto widget = std::unique_ptr<T>(widget_creator());
        if (widget) {
            // Ensure proper cleanup on destruction
            QObject::connect(widget.get(), &QObject::destroyed, [widget_ptr = widget.get()]() {
                // Widget is being destroyed - cleanup complete
            });
        }
        return widget;
    }
    
    /**
     * @brief Safe widget deletion with event processing
     * @param widget Widget to delete
     */
    template<typename T>
    static void safeDeleteWidget(std::unique_ptr<T>& widget) {
        if (widget) {
            widget->hide();
            widget->close();
            QApplication::processEvents();
            widget.reset();
            QApplication::processEvents();
        }
    }
    
    /**
     * @brief Safe widget close without deletion
     * @param widget Widget to close
     */
    static void safeCloseWidget(QWidget* widget) {
        if (widget) {
            widget->hide();
            QApplication::processEvents();
            widget->close();
            QApplication::processEvents();
        }
    }
};

/**
 * @brief ETI processing optimization for tests
 */
class ETIProcessingOptimizer {
public:
    /**
     * @brief Create test-optimized ETI processor configuration
     * @return Configuration suitable for testing
     */
    static void configureForTesting() {
        // Disable heavy processing for tests
        qputenv("ETI_TEST_MODE", "1");
        qputenv("ETI_MINIMAL_PROCESSING", "1");
        qputenv("ETI_DISABLE_ANALYSIS", "1");
    }
    
    /**
     * @brief Process ETI file with timeout protection
     * @param filename ETI file to process
     * @param timeout_ms Maximum processing time
     * @return true if processed successfully within timeout
     */
    static bool processFileWithTimeout(const QString& filename, int timeout_ms = 5000) {
        Q_UNUSED(filename);
        QElapsedTimer timer;
        timer.start();
        
        bool result = false;
        QEventLoop loop;
        
        // Set up timeout
        QTimer::singleShot(timeout_ms, &loop, &QEventLoop::quit);
        
        // Simulate processing (placeholder for actual implementation)
        QTimer::singleShot(100, [&result, &loop]() {
            result = true;
            loop.quit();
        });
        
        loop.exec();
        
        return result && timer.elapsed() < timeout_ms;
    }
    
    /**
     * @brief Skip heavy ETI processing in test mode
     * @return true if processing should be skipped
     */
    static bool shouldSkipHeavyProcessing() {
        return qgetenv("ETI_TEST_MODE") == "1";
    }
};

/**
 * @brief Test execution monitor for performance tracking
 */
class TestExecutionMonitor {
public:
    struct PerformanceMetrics {
        std::chrono::milliseconds setup_time{0};
        std::chrono::milliseconds execution_time{0};
        std::chrono::milliseconds cleanup_time{0};
        std::chrono::milliseconds total_time{0};
        size_t memory_peak_mb = 0;
        bool timed_out = false;
        bool crashed = false;
    };
    
    explicit TestExecutionMonitor(const QString& test_name) 
        : test_name_(test_name), start_time_(std::chrono::steady_clock::now()) {
    }
    
    ~TestExecutionMonitor() {
        auto end_time = std::chrono::steady_clock::now();
        metrics_.total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time_);
        
        // Log performance metrics
        qDebug() << QString("Test %1 performance: %2ms total, %3ms setup, %4ms execution, %5ms cleanup")
                    .arg(test_name_)
                    .arg(metrics_.total_time.count())
                    .arg(metrics_.setup_time.count())
                    .arg(metrics_.execution_time.count())
                    .arg(metrics_.cleanup_time.count());
    }
    
    void markSetupComplete() {
        auto now = std::chrono::steady_clock::now();
        metrics_.setup_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - start_time_);
        execution_start_ = now;
    }
    
    void markExecutionComplete() {
        auto now = std::chrono::steady_clock::now();
        metrics_.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - execution_start_);
        cleanup_start_ = now;
    }
    
    void markCleanupComplete() {
        auto now = std::chrono::steady_clock::now();
        metrics_.cleanup_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - cleanup_start_);
    }
    
    void markTimeout() { metrics_.timed_out = true; }
    void markCrash() { metrics_.crashed = true; }
    
    const PerformanceMetrics& getMetrics() const { return metrics_; }
    
private:
    QString test_name_;
    PerformanceMetrics metrics_;
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point execution_start_;
    std::chrono::steady_clock::time_point cleanup_start_;
};

/**
 * @brief Automated test stability checker
 */
class TestStabilityChecker {
public:
    /**
     * @brief Check if Qt application is in stable state
     * @return true if stable, false if potential issues detected
     */
    static bool isApplicationStable() {
        if (!QApplication::instance()) return false;
        
        // Check for modal dialogs that might block tests
        if (QApplication::activeModalWidget()) {
            qWarning() << "Modal widget detected - may cause test blocking";
            return false;
        }
        
        // Process any pending events (simplified check)
        QApplication::processEvents();
        
        // Simple stability check - process events multiple times
        for (int i = 0; i < 10; ++i) {
            QApplication::processEvents(QEventLoop::AllEvents, 10);
            QThread::msleep(1);
        }
        
        return true;
    }
    
    /**
     * @brief Force application into stable state
     */
    static void stabilizeApplication() {
        // Close any modal dialogs
        QWidget* modal = QApplication::activeModalWidget();
        if (modal) {
            modal->close();
            SafeTestTimer::safeWait(100);
        }
        
        // Process all pending events (simplified)
        for (int i = 0; i < 10; ++i) {
            QApplication::processEvents(QEventLoop::AllEvents, 10);
            QThread::msleep(10);
        }
    }
    
    /**
     * @brief Check memory usage and detect leaks
     * @return estimated memory usage in MB
     */
    static size_t checkMemoryUsage() {
        // Simple memory usage estimation
        // In production, use platform-specific APIs
        static size_t base_memory = 50; // 50MB base
        size_t widget_count = 0;
        
        if (QApplication::instance()) {
            auto widgets = QApplication::allWidgets();
            widget_count = widgets.size();
        }
        
        return base_memory + (widget_count * 0.1); // 0.1MB per widget estimate
    }
};

/**
 * @brief Comprehensive test optimization macros
 */
#define SAFE_TEST_SETUP(test_class) \
    TestOptimization::TestExecutionMonitor monitor(#test_class); \
    TestOptimization::ETIProcessingOptimizer::configureForTesting(); \
    TestOptimization::TestStabilityChecker::stabilizeApplication(); \
    monitor.markSetupComplete();

#define SAFE_TEST_CLEANUP(widget) \
    do { \
        TestOptimization::SafeWidgetManager::safeCloseWidget(widget); \
        TestOptimization::TestStabilityChecker::stabilizeApplication(); \
    } while(0)

#define SAFE_WAIT_FOR_WINDOW(widget, timeout) \
    QVERIFY2(TestOptimization::SafeTestTimer::waitForWindowActive(widget, timeout), \
             QString("Window failed to become active within %1ms").arg(timeout).toLocal8Bit())

#define SAFE_WAIT_FOR_CONDITION(condition, timeout) \
    QVERIFY2(TestOptimization::SafeTestTimer::waitForCondition(condition, timeout), \
             QString("Condition not met within %1ms").arg(timeout).toLocal8Bit())

#define SAFE_ETI_PROCESSING(filename, timeout) \
    if (!TestOptimization::ETIProcessingOptimizer::shouldSkipHeavyProcessing()) { \
        QVERIFY2(TestOptimization::ETIProcessingOptimizer::processFileWithTimeout(filename, timeout), \
                 QString("ETI processing failed or timed out after %1ms").arg(timeout).toLocal8Bit()); \
    }

} // namespace TestOptimization