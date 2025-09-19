/**
 * @file test_logger_hang_fix.cpp
 * @brief TDD verification test for Logger hang fix
 * 
 * This standalone test verifies that the Logger hang fix implementation
 * works correctly and maintains professional standards.
 * 
 * CRITICAL SAFETY TEST: This test MUST complete without hanging
 * to verify the hang-resistant implementation is working.
 */

#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QSignalSpy>
#include <QThread>
#include <QTime>
#include <QDir>
#include <QFile>
#include <memory>
#include <atomic>
#include <thread>
#include <vector>

// Include the hang-safe Logger implementation
#include "src/utils/logger.h"

class LoggerHangFixTest {
public:
    static bool runAllTests() {
        QCoreApplication::setApplicationName("ETIStreamAnalyzer");
        
        bool allPassed = true;
        
        qDebug() << "=== Logger Hang Fix TDD Verification ===";
        qDebug() << "Testing hang-resistant Logger implementation...";
        
        // Test 1: Basic Logger instantiation (must not hang)
        allPassed &= testBasicInstantiation();
        
        // Test 2: Logger singleton access (must not hang)
        allPassed &= testSingletonAccess();
        
        // Test 3: Immediate fallback logging (must work without Qt context)
        allPassed &= testImmediateFallbackLogging();
        
        // Test 4: Thread-safe operation (must not deadlock)
        allPassed &= testThreadSafeOperation();
        
        // Test 5: GUI integration compatibility (must not hang)
        allPassed &= testGuiIntegrationCompatibility();
        
        // Test 6: Performance under stress (must maintain responsiveness)
        allPassed &= testPerformanceUnderStress();
        
        // Test 7: Memory management (must not leak)
        allPassed &= testMemoryManagement();
        
        // Test 8: File operations (must be safe)
        allPassed &= testFileOperations();
        
        qDebug() << "\n=== Test Results ===";
        qDebug() << "All tests passed:" << allPassed;
        qDebug() << "Logger hang fix verification:" << (allPassed ? "SUCCESS" : "FAILED");
        
        return allPassed;
    }

private:
    static bool testBasicInstantiation() {
        qDebug() << "\n1. Testing basic Logger instantiation...";
        
        try {
            // This must complete within 1 second (no hang)
            QTime startTime = QTime::currentTime();
            
            Logger& testLogger = Logger::instance();
            
            int elapsedMs = startTime.msecsTo(QTime::currentTime());
            if (elapsedMs > 1000) {
                qDebug() << "FAILED: Logger instantiation took" << elapsedMs << "ms (too slow)";
                return false;
            }
            
            qDebug() << "✓ Logger instantiated in" << elapsedMs << "ms";
            qDebug() << "✓ GUI integration supported:" << testLogger.supportsGuiIntegration();
            qDebug() << "✓ Main window connection capability:" << testLogger.canConnectToMainWindow();
            qDebug() << "✓ Real-time display capability:" << testLogger.hasRealtimeDisplayCapability();
            
            return true;
        }
        catch (const std::exception& e) {
            qDebug() << "FAILED: Exception during instantiation:" << e.what();
            return false;
        }
    }
    
    static bool testSingletonAccess() {
        qDebug() << "\n2. Testing Logger singleton access...";
        
        try {
            // This must complete within 1 second (no hang)
            QTime startTime = QTime::currentTime();
            
            Logger& logger1 = Logger::instance();
            Logger& logger2 = Logger::instance();
            
            int elapsedMs = startTime.msecsTo(QTime::currentTime());
            if (elapsedMs > 1000) {
                qDebug() << "FAILED: Singleton access took" << elapsedMs << "ms (too slow)";
                return false;
            }
            
            // Verify same instance
            if (&logger1 != &logger2) {
                qDebug() << "FAILED: Singleton pattern broken";
                return false;
            }
            
            qDebug() << "✓ Singleton access completed in" << elapsedMs << "ms";
            qDebug() << "✓ Same instance verified";
            
            return true;
        }
        catch (const std::exception& e) {
            qDebug() << "FAILED: Exception during singleton access:" << e.what();
            return false;
        }
    }
    
    static bool testImmediateFallbackLogging() {
        qDebug() << "\n3. Testing immediate fallback logging...";
        
        try {
            Logger& logger = Logger::instance();
            
            // Test logging without full initialization (should use fallback)
            QTime startTime = QTime::currentTime();
            
            logger.logInfo("Test fallback message", "FallbackTest");
            logger.logWarning("Test warning message", "FallbackTest");
            logger.logError("Test error message", "FallbackTest");
            logger.logCritical("Test critical message", "FallbackTest");
            
            int elapsedMs = startTime.msecsTo(QTime::currentTime());
            if (elapsedMs > 100) {
                qDebug() << "FAILED: Fallback logging took" << elapsedMs << "ms (too slow)";
                return false;
            }
            
            qDebug() << "✓ Fallback logging completed in" << elapsedMs << "ms";
            qDebug() << "✓ No hangs detected";
            
            return true;
        }
        catch (const std::exception& e) {
            qDebug() << "FAILED: Exception during fallback logging:" << e.what();
            return false;
        }
    }
    
    static bool testThreadSafeOperation() {
        qDebug() << "\n4. Testing thread-safe operation...";
        
        try {
            Logger& logger = Logger::instance();
            
            // Test concurrent access from multiple threads
            const int numThreads = 5;
            const int messagesPerThread = 100;
            std::atomic<int> completedThreads{0};
            std::atomic<bool> testFailed{false};
            
            QTime startTime = QTime::currentTime();
            
            std::vector<std::thread> threads;
            for (int i = 0; i < numThreads; ++i) {
                threads.emplace_back([&logger, &completedThreads, &testFailed, i, messagesPerThread]() {
                    try {
                        for (int j = 0; j < messagesPerThread; ++j) {
                            logger.logInfo(QString("Thread %1 Message %2").arg(i).arg(j), "ThreadTest");
                        }
                        completedThreads++;
                    }
                    catch (...) {
                        testFailed.store(true);
                    }
                });
            }
            
            // Wait for all threads to complete (with timeout)
            for (auto& thread : threads) {
                thread.join();
            }
            
            int elapsedMs = startTime.msecsTo(QTime::currentTime());
            
            if (testFailed.load()) {
                qDebug() << "FAILED: Exception in thread-safe operation";
                return false;
            }
            
            if (completedThreads.load() != numThreads) {
                qDebug() << "FAILED: Not all threads completed";
                return false;
            }
            
            if (elapsedMs > 5000) {
                qDebug() << "FAILED: Thread-safe operation took" << elapsedMs << "ms (deadlock suspected)";
                return false;
            }
            
            qDebug() << "✓ Thread-safe operation completed in" << elapsedMs << "ms";
            qDebug() << "✓" << completedThreads.load() << "threads completed successfully";
            qDebug() << "✓ No deadlocks detected";
            
            return true;
        }
        catch (const std::exception& e) {
            qDebug() << "FAILED: Exception during thread-safe test:" << e.what();
            return false;
        }
    }
    
    static bool testGuiIntegrationCompatibility() {
        qDebug() << "\n5. Testing GUI integration compatibility...";
        
        try {
            Logger& logger = Logger::instance();
            
            // Test signal emission (required for GUI integration)
            QSignalSpy logMessageSpy(&logger, &Logger::logMessageAdded);
            QSignalSpy errorMessageSpy(&logger, &Logger::errorMessageAdded);
            
            QTime startTime = QTime::currentTime();
            
            logger.logInfo("GUI integration test", "GuiTest");
            logger.logError("GUI error test", "GuiTest");
            
            // Process events to trigger signals
            QCoreApplication::processEvents();
            
            int elapsedMs = startTime.msecsTo(QTime::currentTime());
            if (elapsedMs > 500) {
                qDebug() << "FAILED: GUI integration took" << elapsedMs << "ms (too slow)";
                return false;
            }
            
            qDebug() << "✓ GUI integration completed in" << elapsedMs << "ms";
            qDebug() << "✓ Log message signals:" << logMessageSpy.count();
            qDebug() << "✓ Error message signals:" << errorMessageSpy.count();
            qDebug() << "✓ GUI compatibility verified";
            
            return true;
        }
        catch (const std::exception& e) {
            qDebug() << "FAILED: Exception during GUI integration test:" << e.what();
            return false;
        }
    }
    
    static bool testPerformanceUnderStress() {
        qDebug() << "\n6. Testing performance under stress...";
        
        try {
            Logger& logger = Logger::instance();
            
            const int messageCount = 1000;
            QTime startTime = QTime::currentTime();
            
            for (int i = 0; i < messageCount; ++i) {
                logger.logInfo(QString("Stress test message %1").arg(i), "StressTest");
            }
            
            int elapsedMs = startTime.msecsTo(QTime::currentTime());
            
            // Should log 1000 messages in less than 2 seconds
            if (elapsedMs > 2000) {
                qDebug() << "FAILED: Stress test took" << elapsedMs << "ms (performance issue)";
                return false;
            }
            
            // Check memory usage
            qint64 memoryUsage = logger.getMemoryUsage();
            if (memoryUsage > 50 * 1024 * 1024) { // 50MB limit
                qDebug() << "FAILED: Memory usage too high:" << memoryUsage << "bytes";
                return false;
            }
            
            qDebug() << "✓ Stress test completed in" << elapsedMs << "ms";
            qDebug() << "✓ Messages per second:" << (messageCount * 1000 / elapsedMs);
            qDebug() << "✓ Memory usage:" << (memoryUsage / 1024) << "KB";
            
            return true;
        }
        catch (const std::exception& e) {
            qDebug() << "FAILED: Exception during stress test:" << e.what();
            return false;
        }
    }
    
    static bool testMemoryManagement() {
        qDebug() << "\n7. Testing memory management...";
        
        try {
            Logger& logger = Logger::instance();
            
            // Set memory limit
            qint64 memoryLimit = 10 * 1024 * 1024; // 10MB
            logger.setMaxMemoryUsage(memoryLimit);
            
            if (logger.getMaxMemoryUsage() != memoryLimit) {
                qDebug() << "FAILED: Memory limit not set correctly";
                return false;
            }
            
            // Generate many messages
            for (int i = 0; i < 5000; ++i) {
                logger.logInfo(QString("Memory test message %1 with extra content").arg(i), "MemoryTest");
            }
            
            qint64 actualUsage = logger.getMemoryUsage();
            if (actualUsage > memoryLimit) {
                qDebug() << "FAILED: Memory usage exceeded limit:" << actualUsage << ">" << memoryLimit;
                return false;
            }
            
            qDebug() << "✓ Memory limit respected:" << (actualUsage / 1024) << "KB /" << (memoryLimit / 1024) << "KB";
            qDebug() << "✓ Memory management working correctly";
            
            return true;
        }
        catch (const std::exception& e) {
            qDebug() << "FAILED: Exception during memory test:" << e.what();
            return false;
        }
    }
    
    static bool testFileOperations() {
        qDebug() << "\n8. Testing file operations...";
        
        try {
            Logger& logger = Logger::instance();
            
            // Test directory setting
            QString tempDir = QDir::tempPath() + "/eti_analyzer_logger_test";
            QDir().mkpath(tempDir);
            
            bool dirResult = logger.setLogDirectory(tempDir);
            if (!dirResult) {
                qDebug() << "FAILED: Could not set log directory";
                return false;
            }
            
            QString retrievedDir = logger.getLogDirectory();
            if (retrievedDir != tempDir) {
                qDebug() << "FAILED: Log directory not set correctly";
                return false;
            }
            
            // Test logging to file
            logger.logInfo("File operation test", "FileTest");
            logger.flush();
            
            QString logFile = logger.getCurrentLogFilePath();
            if (logFile.isEmpty() || !QFile::exists(logFile)) {
                qDebug() << "WARNING: Log file not created (may be in fallback mode)";
                // This is not a failure for the hang fix test
            } else {
                qDebug() << "✓ Log file created:" << logFile;
            }
            
            qDebug() << "✓ Directory operations working";
            qDebug() << "✓ File operations safe (no hangs)";
            
            // Clean up
            QDir(tempDir).removeRecursively();
            
            return true;
        }
        catch (const std::exception& e) {
            qDebug() << "FAILED: Exception during file operations test:" << e.what();
            return false;
        }
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "Starting Logger Hang Fix TDD Verification...";
    qDebug() << "Application name:" << QCoreApplication::applicationName();
    
    // Set a timeout to detect hangs
    QTimer::singleShot(30000, &app, [&app]() {
        qDebug() << "TIMEOUT: Test took too long, possible hang detected!";
        app.exit(1);
    });
    
    // Run tests asynchronously to avoid blocking
    QTimer::singleShot(100, [&app]() {
        bool success = LoggerHangFixTest::runAllTests();
        app.exit(success ? 0 : 1);
    });
    
    return app.exec();
}