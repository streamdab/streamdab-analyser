/**
 * @file test_logger_integration.cpp
 * @brief TDD Integration tests for unified Logger interface
 * 
 * RED PHASE: These tests will initially FAIL until we implement
 * the unified logger interface properly.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>

// Include the unified logger (will fail initially - RED phase)
#include "../../src/utils/logger.h"

/**
 * @class LoggerIntegrationTest
 * @brief Test fixture for Logger integration testing
 */
class LoggerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure we have a QCoreApplication for Qt functionality
        if (!QCoreApplication::instance()) {
            int argc = 1;
            char* argv[] = { const_cast<char*>("test"), nullptr };
            app = new QCoreApplication(argc, argv);
        }
    }

    void TearDown() override {
        // Clean up if we created the app
        if (app && app->parent() == nullptr) {
            delete app;
            app = nullptr;
        }
    }

    QCoreApplication* app = nullptr;
};

/**
 * RED PHASE TEST 1: Single Logger Interface
 * This test will FAIL initially because LoggerCompat alias conflicts with Logger class
 */
TEST_F(LoggerIntegrationTest, SingleLoggerInterface) {
    // Test that Logger class exists and is accessible
    Logger logger;
    EXPECT_NO_THROW(logger.initialize());
    
    // Test that we can log messages without issues
    EXPECT_NO_THROW(logger.logInfo("Test message", "TestCategory"));
    EXPECT_NO_THROW(logger.log(Logger::LogLevel::Info, "Test message", "TestCategory"));
    
    // Verify logger state
    EXPECT_TRUE(logger.supportsGuiIntegration());
    EXPECT_TRUE(logger.canConnectToMainWindow());
    EXPECT_TRUE(logger.hasRealtimeDisplayCapability());
}

/**
 * RED PHASE TEST 2: No LoggerCompat Class Conflicts
 * This test verifies that LoggerCompat doesn't interfere with Logger
 */
TEST_F(LoggerIntegrationTest, NoLoggerCompatConflicts) {
    // Create Logger instance - should work without conflicts
    Logger logger;
    EXPECT_TRUE(logger.initialize());
    
    // Test that Logger methods work as expected
    logger.setLogLevel(Logger::LogLevel::Info);
    EXPECT_EQ(logger.getLogLevel(), Logger::LogLevel::Info);
    
    // Test logging functionality
    QSignalSpy logSpy(&logger, &Logger::logMessageAdded);
    logger.logInfo("Integration test message", "IntegrationTest");
    
    // We should receive the signal
    EXPECT_EQ(logSpy.count(), 1);
    
    auto arguments = logSpy.takeFirst();
    EXPECT_EQ(arguments.at(0).value<Logger::LogLevel>(), Logger::LogLevel::Info);
    EXPECT_EQ(arguments.at(1).toString(), "Integration test message");
    EXPECT_EQ(arguments.at(2).toString(), "IntegrationTest");
}

/**
 * RED PHASE TEST 3: GUI Integration Compatibility
 * This test ensures Logger works with Qt GUI components
 */
TEST_F(LoggerIntegrationTest, QtGuiCompatibility) {
    Logger logger;
    EXPECT_TRUE(logger.initialize());
    
    // Test Qt-specific functionality
    EXPECT_NO_THROW(logger.enableLogging(true));
    EXPECT_TRUE(logger.isLoggingEnabled());
    
    // Test that we can connect signals (GUI integration requirement)
    QSignalSpy errorSpy(&logger, &Logger::errorMessageAdded);
    logger.logError("Test error", "ErrorTest");
    
    EXPECT_GE(errorSpy.count(), 1);
    
    // Test performance monitoring (required for GUI status display)
    auto stats = logger.getPerformanceStatistics();
    EXPECT_FALSE(stats.isEmpty());
    
    // Test recent messages retrieval (required for GUI log viewer)
    auto recentMessages = logger.getRecentMessages(10);
    EXPECT_GE(recentMessages.size(), 1);
}

/**
 * RED PHASE TEST 4: Thread Safety for Real-time Operation
 * This test verifies Logger works in multi-threaded environments
 */
TEST_F(LoggerIntegrationTest, ThreadSafeOperation) {
    Logger logger;
    EXPECT_TRUE(logger.initialize());
    
    // Enable async logging for performance
    logger.enableAsyncLogging(true);
    EXPECT_TRUE(logger.isAsyncLoggingEnabled());
    
    // Test that logging from multiple threads works
    std::atomic<int> messageCount{0};
    const int numThreads = 5;
    const int messagesPerThread = 100;
    
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&logger, &messageCount, messagesPerThread, i]() {
            for (int j = 0; j < messagesPerThread; ++j) {
                logger.logInfo(QString("Thread %1 Message %2").arg(i).arg(j), "ThreadTest");
                messageCount++;
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Wait for async operations to complete
    EXPECT_TRUE(logger.waitForAsyncCompletion(5000));
    
    // Verify all messages were processed
    EXPECT_EQ(messageCount.load(), numThreads * messagesPerThread);
}

/**
 * RED PHASE TEST 5: Memory Performance Requirements
 * This test ensures Logger meets memory usage requirements
 */
TEST_F(LoggerIntegrationTest, MemoryPerformanceRequirements) {
    Logger logger;
    EXPECT_TRUE(logger.initialize());
    
    // Set memory limits for testing
    const qint64 maxMemoryLimit = 10 * 1024 * 1024; // 10MB
    logger.setMaxMemoryUsage(maxMemoryLimit);
    EXPECT_EQ(logger.getMaxMemoryUsage(), maxMemoryLimit);
    
    // Log many messages and check memory usage
    for (int i = 0; i < 1000; ++i) {
        logger.logInfo(QString("Performance test message %1").arg(i), "PerformanceTest");
    }
    
    // Verify memory usage is within limits
    qint64 currentUsage = logger.getMemoryUsage();
    EXPECT_LT(currentUsage, maxMemoryLimit);
    EXPECT_GT(currentUsage, 0); // Should be using some memory
}

/**
 * RED PHASE TEST 6: File Operations Integration
 * This test verifies Logger file operations work correctly
 */
TEST_F(LoggerIntegrationTest, FileOperationsIntegration) {
    Logger logger;
    EXPECT_TRUE(logger.initialize());
    
    // Test that we can set log directory
    QString tempDir = QDir::tempPath() + "/eti_analyser_test_logs";
    EXPECT_TRUE(logger.setLogDirectory(tempDir));
    EXPECT_EQ(logger.getLogDirectory(), tempDir);
    
    // Test file creation and writing
    logger.logInfo("File test message", "FileTest");
    logger.flush();
    
    // Verify log file was created
    QString currentLogFile = logger.getCurrentLogFilePath();
    EXPECT_FALSE(currentLogFile.isEmpty());
    EXPECT_TRUE(QFile::exists(currentLogFile));
    
    // Test log rotation
    logger.setMaxLogFileSize(1024); // 1KB for testing
    
    // Write enough data to trigger rotation
    for (int i = 0; i < 100; ++i) {
        logger.logInfo(QString("Rotation test message %1 with extra content to fill up the log file").arg(i), "RotationTest");
    }
    
    logger.flush();
    
    // Should have multiple log files now
    QStringList logFiles = logger.getLogFileList();
    EXPECT_GT(logFiles.size(), 1);
    
    // Clean up test directory
    QDir(tempDir).removeRecursively();
}