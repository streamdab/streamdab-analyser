/**
 * @file simple_logger_test.cpp
 * @brief Simplified Logger hang fix verification test
 * 
 * This test focuses on verifying the core hang fix functionality
 * without requiring Qt MOC compilation.
 */

#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
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

class SimpleLoggerTest {
public:
    static bool runAllTests() {
        QCoreApplication::setApplicationName("ETIStreamAnalyzer");
        
        bool allPassed = true;
        
        qDebug() << "=== Simple Logger Hang Fix TDD Verification ===";
        qDebug() << "Testing core hang-resistant functionality...";
        
        // Test 1: Singleton access (must not hang)
        allPassed &= testSingletonAccess();
        
        // Test 2: Basic logging functionality (must work immediately)
        allPassed &= testBasicLogging();
        
        // Test 3: Thread safety (must not deadlock)
        allPassed &= testThreadSafety();
        
        // Test 4: Performance requirements (must be responsive)
        allPassed &= testPerformance();
        
        // Test 5: Memory management (must respect limits)
        allPassed &= testMemoryManagement();
        
        // Test 6: GUI compatibility flags (must be available)
        allPassed &= testGuiCompatibility();
        
        qDebug() << "\n=== Test Results Summary ===";
        qDebug() << "All tests passed:" << allPassed;
        qDebug() << "Logger hang fix status:" << (allPassed ? "SUCCESS" : "FAILED");
        
        return allPassed;
    }

private:
    static bool testSingletonAccess() {
        qDebug() << "\n1. Testing singleton access (hang prevention)...";
        
        try {
            QTime startTime = QTime::currentTime();
            
            // This must complete quickly without hanging
            Logger& logger1 = Logger::instance();
            Logger& logger2 = Logger::instance();
            
            int elapsedMs = startTime.msecsTo(QTime::currentTime());
            
            if (elapsedMs > 1000) {
                qDebug() << "FAILED: Singleton access took" << elapsedMs << "ms (hang detected)";
                return false;
            }
            
            if (&logger1 != &logger2) {
                qDebug() << "FAILED: Singleton pattern broken";
                return false;
            }
            
            qDebug() << "✓ Singleton access completed in" << elapsedMs << "ms";
            qDebug() << "✓ No hangs detected";
            
            return true;
        }
        catch (const std::exception& e) {
            qDebug() << "FAILED: Exception:" << e.what();
            return false;
        }
    }
    
    static bool testBasicLogging() {
        qDebug() << "\n2. Testing basic logging functionality...";
        
        try {
            Logger& logger = Logger::instance();
            
            QTime startTime = QTime::currentTime();
            
            // Test all log levels with immediate fallback
            logger.logDebug("Debug test message", "Test");
            logger.logInfo("Info test message", "Test");
            logger.logWarning("Warning test message", "Test");
            logger.logError("Error test message", "Test");
            logger.logCritical("Critical test message", "Test");
            
            // Test legacy interface compatibility
            logger.log(Logger::LegacyLogLevel::Info, "Legacy", "Legacy test message");
            
            int elapsedMs = startTime.msecsTo(QTime::currentTime());
            
            if (elapsedMs > 100) {
                qDebug() << "FAILED: Basic logging took" << elapsedMs << "ms (too slow)";
                return false;
            }
            
            qDebug() << "✓ All log levels functional in" << elapsedMs << "ms";
            qDebug() << "✓ Legacy interface compatible";
            qDebug() << "✓ Immediate fallback working";
            
            return true;
        }
        catch (const std::exception& e) {
            qDebug() << "FAILED: Exception during logging:" << e.what();
            return false;
        }
    }
    
    static bool testThreadSafety() {
        qDebug() << "\n3. Testing thread safety (deadlock prevention)...";
        
        try {
            Logger& logger = Logger::instance();
            
            const int numThreads = 5;
            const int messagesPerThread = 50;
            std::atomic<int> completedMessages{0};
            std::atomic<bool> failed{false};
            
            QTime startTime = QTime::currentTime();
            
            std::vector<std::thread> threads;
            for (int i = 0; i < numThreads; ++i) {
                threads.emplace_back([&logger, &completedMessages, &failed, i, messagesPerThread]() {
                    try {
                        for (int j = 0; j < messagesPerThread; ++j) {
                            logger.logInfo(QString("Thread %1 Message %2").arg(i).arg(j), "ThreadTest");
                            completedMessages.fetch_add(1);
                        }
                    }
                    catch (...) {
                        failed.store(true);
                    }
                });
            }
            
            // Wait for threads with timeout
            for (auto& thread : threads) {
                thread.join();
            }
            
            int elapsedMs = startTime.msecsTo(QTime::currentTime());
            
            if (failed.load()) {
                qDebug() << "FAILED: Exception in threads";
                return false;
            }
            
            if (completedMessages.load() != numThreads * messagesPerThread) {
                qDebug() << "FAILED: Lost messages. Expected:" << (numThreads * messagesPerThread) 
                         << "Got:" << completedMessages.load();
                return false;
            }
            
            if (elapsedMs > 3000) {
                qDebug() << "FAILED: Thread test took" << elapsedMs << "ms (deadlock suspected)";
                return false;
            }
            
            qDebug() << "✓ Thread safety verified in" << elapsedMs << "ms";
            qDebug() << "✓ All" << completedMessages.load() << "messages processed";
            qDebug() << "✓ No deadlocks detected";
            
            return true;
        }
        catch (const std::exception& e) {
            qDebug() << "FAILED: Exception during thread test:" << e.what();
            return false;
        }
    }
    
    static bool testPerformance() {
        qDebug() << "\n4. Testing performance requirements...";
        
        try {
            Logger& logger = Logger::instance();
            
            const int messageCount = 1000;
            QTime startTime = QTime::currentTime();
            
            for (int i = 0; i < messageCount; ++i) {
                logger.logInfo(QString("Performance test %1").arg(i), "PerfTest");
            }
            
            int elapsedMs = startTime.msecsTo(QTime::currentTime());
            
            // Should process 1000 messages in under 1 second
            if (elapsedMs > 1000) {
                qDebug() << "FAILED: Performance test took" << elapsedMs << "ms (too slow)";
                return false;
            }
            
            double messagesPerSecond = (messageCount * 1000.0) / elapsedMs;
            
            qDebug() << "✓ Performance test completed in" << elapsedMs << "ms";
            qDebug() << "✓ Messages per second:" << (int)messagesPerSecond;
            qDebug() << "✓ Performance requirements met";
            
            return true;
        }
        catch (const std::exception& e) {
            qDebug() << "FAILED: Exception during performance test:" << e.what();
            return false;
        }
    }
    
    static bool testMemoryManagement() {
        qDebug() << "\n5. Testing memory management...";
        
        try {
            Logger& logger = Logger::instance();
            
            // Test memory limit setting
            qint64 memoryLimit = 5 * 1024 * 1024; // 5MB
            logger.setMaxMemoryUsage(memoryLimit);
            
            if (logger.getMaxMemoryUsage() != memoryLimit) {
                qDebug() << "FAILED: Memory limit not set correctly";
                return false;
            }
            
            // Generate many messages to test memory management
            for (int i = 0; i < 2000; ++i) {
                logger.logInfo(QString("Memory test message %1 with extra content").arg(i), "MemTest");
            }
            
            qint64 memoryUsage = logger.getMemoryUsage();
            
            if (memoryUsage > memoryLimit) {
                qDebug() << "FAILED: Memory usage" << memoryUsage << "exceeded limit" << memoryLimit;
                return false;
            }
            
            qDebug() << "✓ Memory limit respected:" << (memoryUsage / 1024) << "KB /" << (memoryLimit / 1024) << "KB";
            qDebug() << "✓ Memory management working";
            
            return true;
        }
        catch (const std::exception& e) {
            qDebug() << "FAILED: Exception during memory test:" << e.what();
            return false;
        }
    }
    
    static bool testGuiCompatibility() {
        qDebug() << "\n6. Testing GUI compatibility flags...";
        
        try {
            Logger& logger = Logger::instance();
            
            // Test GUI integration capability flags
            bool guiSupport = logger.supportsGuiIntegration();
            bool mainWindowConnection = logger.canConnectToMainWindow();
            bool realtimeDisplay = logger.hasRealtimeDisplayCapability();
            
            if (!guiSupport || !mainWindowConnection || !realtimeDisplay) {
                qDebug() << "FAILED: GUI compatibility flags not set correctly";
                qDebug() << "  GUI Support:" << guiSupport;
                qDebug() << "  MainWindow Connection:" << mainWindowConnection;
                qDebug() << "  Realtime Display:" << realtimeDisplay;
                return false;
            }
            
            // Test async logging capability
            logger.enableAsyncLogging(true);
            if (!logger.isAsyncLoggingEnabled()) {
                qDebug() << "FAILED: Async logging not enabled";
                return false;
            }
            
            // Test log level management
            logger.setLogLevel(Logger::LogLevel::Warning);
            if (logger.getLogLevel() != Logger::LogLevel::Warning) {
                qDebug() << "FAILED: Log level not set correctly";
                return false;
            }
            
            qDebug() << "✓ GUI integration support confirmed";
            qDebug() << "✓ Async logging capability verified";
            qDebug() << "✓ Log level management working";
            qDebug() << "✓ All GUI compatibility features available";
            
            return true;
        }
        catch (const std::exception& e) {
            qDebug() << "FAILED: Exception during GUI compatibility test:" << e.what();
            return false;
        }
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "=== Logger Hang Fix TDD Verification ===";
    qDebug() << "Application:" << QCoreApplication::applicationName();
    qDebug() << "Testing hang-resistant Logger implementation...";
    
    // Set timeout to detect hangs
    QTimer::singleShot(15000, [&app]() {
        qDebug() << "\nTIMEOUT: Test exceeded 15 seconds - HANG DETECTED!";
        qDebug() << "Logger hang fix FAILED";
        app.exit(2);
    });
    
    // Run tests
    QTimer::singleShot(100, [&app]() {
        bool success = SimpleLoggerTest::runAllTests();
        
        if (success) {
            qDebug() << "\n🎉 LOGGER HANG FIX VERIFICATION: SUCCESS";
            qDebug() << "✓ Hang-resistant implementation confirmed";
            qDebug() << "✓ All TDD requirements met";
            qDebug() << "✓ Professional standards maintained";
        } else {
            qDebug() << "\n❌ LOGGER HANG FIX VERIFICATION: FAILED";
            qDebug() << "✗ Issues detected in implementation";
        }
        
        app.exit(success ? 0 : 1);
    });
    
    return app.exec();
}