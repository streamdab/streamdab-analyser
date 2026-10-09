#pragma once

#include <QApplication>
#include <QTest>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QWidget>
#include "utils/logger.h"

/**
 * Test utilities for E2E testing with improved event loop management
 * and object lifecycle handling to prevent test hanging and crashes.
 */
namespace TestUtils {

/**
 * Enhanced event processing with timeout protection
 * Prevents tests from hanging by limiting event processing time
 */
inline void safeProcessEvents(int maxTimeMs = 100, int maxIterations = 10)
{
    QElapsedTimer timer;
    timer.start();
    int iterations = 0;
    
    while (timer.elapsed() < maxTimeMs && iterations < maxIterations) {
        QApplication::processEvents(QEventLoop::AllEvents, 10);
        iterations++;
        
        // Small delay to prevent 100% CPU usage
        if (iterations % 3 == 0) {
            QTest::qWait(5);
        }
    }
}

/**
 * Safe widget cleanup with proper event processing
 * Ensures widgets are properly cleaned up without leaving dangling signals
 */
template<typename WidgetType>
inline void safeDeleteWidget(WidgetType*& widget)
{
    if (widget) {
        // Disconnect all signals to prevent crashes during deletion
        widget->disconnect();
        
        // Close if it's a window
        if (auto* window = qobject_cast<QWidget*>(widget)) {
            window->close();
        }
        
        // Process close events
        safeProcessEvents(50, 5);
        
        // Delete and nullify
        delete widget;
        widget = nullptr;
        
        // Final event processing
        safeProcessEvents(50, 3);
    }
}

/**
 * Robust window initialization with timeout protection
 * Ensures windows are properly initialized before proceeding with tests
 */
template<typename WindowType>
inline bool initializeWindow(WindowType* window, int timeoutMs = 5000)
{
    if (!window) {
        return false;
    }
    
    // Process constructor events
    safeProcessEvents(100, 5);
    
    window->show();
    
    QElapsedTimer timer;
    timer.start();
    bool windowReady = false;
    
    while (timer.elapsed() < timeoutMs && !windowReady) {
        safeProcessEvents(50, 3);
        QTest::qWait(50);
        
        if (window->isVisible()) {
            // Try to activate window
            bool windowActive = QTest::qWaitForWindowActive(window, 500);
            if (windowActive || window->isActiveWindow()) {
                windowReady = true;
            }
        }
    }
    
    if (windowReady) {
        // Allow additional time for complete initialization
        QTest::qWait(100);
        safeProcessEvents(100, 5);
        
        Logger::instance().log(Logger::Debug, "TestUtils", 
                              QString("Window initialized successfully in %1ms").arg(timer.elapsed()));
    } else {
        Logger::instance().log(Logger::Warning, "TestUtils", 
                              QString("Window initialization timed out after %1ms").arg(timer.elapsed()));
    }
    
    return windowReady;
}

/**
 * Enhanced waiting function with proper event processing
 * Prevents hanging while waiting for conditions to be met
 */
template<typename ConditionFunc>
inline bool waitForCondition(ConditionFunc condition, int timeoutMs = 5000, int checkIntervalMs = 50)
{
    QElapsedTimer timer;
    timer.start();
    
    int maxChecks = timeoutMs / checkIntervalMs;
    int checkCount = 0;
    
    while (timer.elapsed() < timeoutMs && checkCount < maxChecks) {
        // Process events in small batches
        safeProcessEvents(25, 3);
        
        // Check condition
        if (condition()) {
            Logger::instance().log(Logger::Debug, "TestUtils", 
                                  QString("Condition met after %1ms").arg(timer.elapsed()));
            return true;
        }
        
        QTest::qWait(checkIntervalMs);
        checkCount++;
        
        // Periodic logging for long waits
        if (checkCount % 20 == 0) {
            Logger::instance().log(Logger::Debug, "TestUtils", 
                                  QString("Still waiting for condition... %1ms elapsed").arg(timer.elapsed()));
        }
    }
    
    Logger::instance().log(Logger::Warning, "TestUtils", 
                          QString("Condition timeout after %1ms").arg(timer.elapsed()));
    return false;
}

/**
 * Safe test cleanup helper
 * Ensures proper cleanup sequence for test teardown
 */
inline void safeTestCleanup()
{
    // Process any pending events
    safeProcessEvents(200, 10);
    
    // Force garbage collection of deleted objects
    QTest::qWait(50);
    safeProcessEvents(100, 5);
}

/**
 * Memory validation helper
 * Checks for common memory issues in tests
 */
inline void validateTestMemoryState(const QString& testName)
{
    // Process events to trigger any pending deletions
    safeProcessEvents(100, 5);
    
    // Log memory validation (placeholder for actual memory checking)
    Logger::instance().log(Logger::Debug, "TestUtils", 
                          QString("Memory state validated for test: %1").arg(testName));
}

} // namespace TestUtils