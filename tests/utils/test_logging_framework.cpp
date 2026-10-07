#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QTest>
#include <QSignalSpy>
#include <QApplication>
#include <QTimer>
#include <QTime>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <memory>
#include "../../src/utils/logger.h"

using namespace testing;

/**
 * @brief Test Logging Framework TDD Implementation
 * 
 * Professional Logging Framework for ETI Stream Analyser following TDD methodology.
 * Tests written BEFORE implementation (RED phase).
 * 
 * Features to test:
 * - Multi-level logging (Info/Warning/Error/Critical)
 * - Thread-safe logging for real-time operations
 * - Structured logging output (JSON, XML, Plain text)
 * - Professional log export capabilities
 * - Performance requirements for real-time operation
 * - Integration with Qt GUI error display
 * - Log rotation and archival
 * - Memory-efficient buffering
 */
class LoggingFrameworkTest : public ::testing::Test 
{
protected:
    void SetUp() override 
    {
        // Create QApplication if needed for Qt tests
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            app = std::make_unique<QApplication>(argc, argv);
        }
        
        // Create temporary test directory
        testDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/eti_analyser_test_logs";
        QDir().mkpath(testDir);
        
        // Initialize logger
        logger = std::make_unique<Logger>();
        logger->setLogDirectory(testDir);
    }

    void TearDown() override 
    {
        logger.reset();
        
        // Clean up test files
        QDir dir(testDir);
        dir.removeRecursively();
    }

    std::unique_ptr<QApplication> app;
    std::unique_ptr<Logger> logger;
    QString testDir;
};

// ============================================================================
// BASIC LOGGING FRAMEWORK FUNCTIONALITY TESTS
// ============================================================================

TEST_F(LoggingFrameworkTest, ConstructorInitializesCorrectly)
{
    // ARRANGE & ACT - Constructor called in SetUp
    
    // ASSERT
    EXPECT_TRUE(logger != nullptr);
    EXPECT_TRUE(logger->isInitialized());
    EXPECT_FALSE(logger->isLoggingEnabled());
    EXPECT_EQ(logger->getLogLevel(), Logger::LogLevel::Info);
}

TEST_F(LoggingFrameworkTest, EnableLoggingStartsLoggingSystem)
{
    // ARRANGE
    QSignalSpy loggingStartedSpy(logger.get(), &Logger::loggingStarted);
    
    // ACT
    bool result = logger->enableLogging(true);
    
    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_TRUE(logger->isLoggingEnabled());
    EXPECT_EQ(loggingStartedSpy.count(), 1);
}

TEST_F(LoggingFrameworkTest, DisableLoggingStopsLoggingSystem)
{
    // ARRANGE
    logger->enableLogging(true);
    QSignalSpy loggingStopped(logger.get(), &Logger::loggingStopped);
    
    // ACT
    bool result = logger->enableLogging(false);
    
    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_FALSE(logger->isLoggingEnabled());
    EXPECT_EQ(loggingStopped.count(), 1);
}

// ============================================================================
// MULTI-LEVEL LOGGING TESTS
// ============================================================================

TEST_F(LoggingFrameworkTest, LogInfoMessageCreatesInfoEntry)
{
    // ARRANGE
    logger->enableLogging(true);
    QSignalSpy logMessageSpy(logger.get(), &Logger::logMessageAdded);
    const QString testMessage = "Test info message";
    
    // ACT
    logger->logInfo(testMessage);
    
    // ASSERT
    EXPECT_EQ(logMessageSpy.count(), 1);
    
    auto logData = logMessageSpy.at(0);
    EXPECT_EQ(logData.at(0).value<Logger::LogLevel>(), Logger::LogLevel::Info);
    EXPECT_EQ(logData.at(1).toString(), testMessage);
}

TEST_F(LoggingFrameworkTest, LogWarningMessageCreatesWarningEntry)
{
    // ARRANGE
    logger->enableLogging(true);
    QSignalSpy logMessageSpy(logger.get(), &Logger::logMessageAdded);
    const QString testMessage = "Test warning message";
    
    // ACT
    logger->logWarning(testMessage);
    
    // ASSERT
    EXPECT_EQ(logMessageSpy.count(), 1);
    
    auto logData = logMessageSpy.at(0);
    EXPECT_EQ(logData.at(0).value<Logger::LogLevel>(), Logger::LogLevel::Warning);
    EXPECT_EQ(logData.at(1).toString(), testMessage);
}

TEST_F(LoggingFrameworkTest, LogErrorMessageCreatesErrorEntry)
{
    // ARRANGE
    logger->enableLogging(true);
    QSignalSpy logMessageSpy(logger.get(), &Logger::logMessageAdded);
    const QString testMessage = "Test error message";
    
    // ACT
    logger->logError(testMessage);
    
    // ASSERT
    EXPECT_EQ(logMessageSpy.count(), 1);
    
    auto logData = logMessageSpy.at(0);
    EXPECT_EQ(logData.at(0).value<Logger::LogLevel>(), Logger::LogLevel::Error);
    EXPECT_EQ(logData.at(1).toString(), testMessage);
}

TEST_F(LoggingFrameworkTest, LogCriticalMessageCreatesCriticalEntry)
{
    // ARRANGE
    logger->enableLogging(true);
    QSignalSpy logMessageSpy(logger.get(), &Logger::logMessageAdded);
    const QString testMessage = "Test critical message";
    
    // ACT
    logger->logCritical(testMessage);
    
    // ASSERT
    EXPECT_EQ(logMessageSpy.count(), 1);
    
    auto logData = logMessageSpy.at(0);
    EXPECT_EQ(logData.at(0).value<Logger::LogLevel>(), Logger::LogLevel::Critical);
    EXPECT_EQ(logData.at(1).toString(), testMessage);
}

TEST_F(LoggingFrameworkTest, LogLevelFilteringWorksCorrectly)
{
    // ARRANGE
    logger->setLogLevel(Logger::LogLevel::Warning);
    logger->enableLogging(true);
    QSignalSpy logMessageSpy(logger.get(), &Logger::logMessageAdded);
    
    // ACT
    logger->logInfo("This should be filtered");      // Below Warning level
    logger->logWarning("This should pass");          // At Warning level
    logger->logError("This should pass");            // Above Warning level
    
    // ASSERT
    EXPECT_EQ(logMessageSpy.count(), 2);  // Only Warning and Error should pass
}

// ============================================================================
// THREAD-SAFE LOGGING TESTS
// ============================================================================

class LoggingThread : public QThread
{
public:
    LoggingThread(Logger* logger, const QString& prefix, int messageCount)
        : m_logger(logger), m_prefix(prefix), m_messageCount(messageCount) {}
    
    void run() override {
        for (int i = 0; i < m_messageCount; ++i) {
            m_logger->logInfo(QString("%1 message %2").arg(m_prefix).arg(i));
            msleep(1); // Small delay to create interleaving
        }
    }
    
private:
    Logger* m_logger;
    QString m_prefix;
    int m_messageCount;
};

TEST_F(LoggingFrameworkTest, ConcurrentLoggingIsThreadSafe)
{
    // ARRANGE
    logger->enableLogging(true);
    QSignalSpy logMessageSpy(logger.get(), &Logger::logMessageAdded);
    
    const int threadCount = 5;
    const int messagesPerThread = 20;
    QList<LoggingThread*> threads;
    
    // ACT
    for (int i = 0; i < threadCount; ++i) {
        LoggingThread* thread = new LoggingThread(logger.get(), 
                                                 QString("Thread%1").arg(i), 
                                                 messagesPerThread);
        threads.append(thread);
        thread->start();
    }
    
    // Wait for all threads to complete
    for (auto* thread : threads) {
        thread->wait();
        delete thread;
    }
    
    // ASSERT
    EXPECT_EQ(logMessageSpy.count(), threadCount * messagesPerThread);
    
    // Verify no message corruption by checking all messages contain "Thread" and "message"
    for (int i = 0; i < logMessageSpy.count(); ++i) {
        QString message = logMessageSpy.at(i).at(1).toString();
        EXPECT_TRUE(message.contains("Thread"));
        EXPECT_TRUE(message.contains("message"));
    }
}

// ============================================================================
// STRUCTURED LOGGING OUTPUT TESTS
// ============================================================================

TEST_F(LoggingFrameworkTest, StructuredLoggingSupportsJsonFormat)
{
    // ARRANGE
    logger->setOutputFormat(Logger::OutputFormat::Json);
    logger->enableLogging(true);
    
    // ACT
    logger->logInfo("Test JSON message", {{"key1", "value1"}, {"key2", 42}});
    logger->flush(); // Ensure immediate write
    
    // ASSERT
    QFile logFile(logger->getCurrentLogFilePath());
    EXPECT_TRUE(logFile.open(QIODevice::ReadOnly));
    
    QJsonDocument doc = QJsonDocument::fromJson(logFile.readAll());
    EXPECT_FALSE(doc.isNull());
    
    QJsonObject logEntry = doc.object();
    EXPECT_TRUE(logEntry.contains("timestamp"));
    EXPECT_TRUE(logEntry.contains("level"));
    EXPECT_TRUE(logEntry.contains("message"));
    EXPECT_EQ(logEntry["level"].toString(), "Info");
    EXPECT_EQ(logEntry["message"].toString(), "Test JSON message");
}

TEST_F(LoggingFrameworkTest, StructuredLoggingSupportsPlainTextFormat)
{
    // ARRANGE
    logger->setOutputFormat(Logger::OutputFormat::PlainText);
    logger->enableLogging(true);
    
    // ACT
    logger->logWarning("Test plain text message");
    logger->flush();
    
    // ASSERT
    QFile logFile(logger->getCurrentLogFilePath());
    EXPECT_TRUE(logFile.open(QIODevice::ReadOnly));
    
    QString content = QString::fromUtf8(logFile.readAll());
    EXPECT_TRUE(content.contains("WARNING"));
    EXPECT_TRUE(content.contains("Test plain text message"));
    EXPECT_TRUE(content.contains(QDateTime::currentDateTime().toString("yyyy-MM-dd")));
}

TEST_F(LoggingFrameworkTest, StructuredLoggingSupportsXmlFormat)
{
    // ARRANGE
    logger->setOutputFormat(Logger::OutputFormat::Xml);
    logger->enableLogging(true);
    
    // ACT
    logger->logError("Test XML message");
    logger->flush();
    
    // ASSERT
    QFile logFile(logger->getCurrentLogFilePath());
    EXPECT_TRUE(logFile.open(QIODevice::ReadOnly));
    
    QString content = QString::fromUtf8(logFile.readAll());
    EXPECT_TRUE(content.contains("<log>"));
    EXPECT_TRUE(content.contains("<level>Error</level>"));
    EXPECT_TRUE(content.contains("<message>Test XML message</message>"));
    EXPECT_TRUE(content.contains("</log>"));
}

// ============================================================================
// PROFESSIONAL LOG EXPORT TESTS
// ============================================================================

TEST_F(LoggingFrameworkTest, ExportLogsToPdfCreatesValidFile)
{
    // ARRANGE
    logger->enableLogging(true);
    logger->logInfo("Test message 1");
    logger->logWarning("Test message 2");
    logger->logError("Test message 3");
    
    QString exportPath = testDir + "/exported_logs.pdf";
    
    // ACT
    bool result = logger->exportToPdf(exportPath);
    
    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_TRUE(QFile::exists(exportPath));
    
    // Verify file is not empty
    QFile pdfFile(exportPath);
    EXPECT_TRUE(pdfFile.open(QIODevice::ReadOnly));
    EXPECT_GT(pdfFile.size(), 0);
}

TEST_F(LoggingFrameworkTest, ExportLogsToHtmlCreatesValidFile)
{
    // ARRANGE
    logger->enableLogging(true);
    logger->logInfo("Test HTML message 1");
    logger->logWarning("Test HTML message 2");
    
    QString exportPath = testDir + "/exported_logs.html";
    
    // ACT
    bool result = logger->exportToHtml(exportPath);
    
    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_TRUE(QFile::exists(exportPath));
    
    // Verify HTML structure
    QFile htmlFile(exportPath);
    EXPECT_TRUE(htmlFile.open(QIODevice::ReadOnly));
    QString content = QString::fromUtf8(htmlFile.readAll());
    EXPECT_TRUE(content.contains("<html>"));
    EXPECT_TRUE(content.contains("Test HTML message 1"));
    EXPECT_TRUE(content.contains("Test HTML message 2"));
}

TEST_F(LoggingFrameworkTest, ExportLogsToCsvCreatesValidFile)
{
    // ARRANGE
    logger->enableLogging(true);
    logger->logInfo("CSV message 1");
    logger->logError("CSV message 2");
    
    QString exportPath = testDir + "/exported_logs.csv";
    
    // ACT
    bool result = logger->exportToCsv(exportPath);
    
    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_TRUE(QFile::exists(exportPath));
    
    // Verify CSV structure
    QFile csvFile(exportPath);
    EXPECT_TRUE(csvFile.open(QIODevice::ReadOnly));
    QString content = QString::fromUtf8(csvFile.readAll());
    QStringList lines = content.split('\n');
    EXPECT_GT(lines.size(), 2);  // Header + at least 2 log entries
    EXPECT_TRUE(lines[0].contains("Timestamp"));
    EXPECT_TRUE(lines[0].contains("Level"));
    EXPECT_TRUE(lines[0].contains("Message"));
}

// ============================================================================
// LOG ROTATION AND ARCHIVAL TESTS
// ============================================================================

TEST_F(LoggingFrameworkTest, LogRotationOccursAtConfiguredSize)
{
    // ARRANGE
    logger->setMaxLogFileSize(1024); // 1KB limit
    logger->enableLogging(true);
    
    QString initialLogFile = logger->getCurrentLogFilePath();
    
    // ACT
    // Generate enough log data to trigger rotation
    for (int i = 0; i < 100; ++i) {
        logger->logInfo(QString("Large message with lots of text to fill up the log file quickly - iteration %1").arg(i));
    }
    logger->flush();
    
    // ASSERT
    QString currentLogFile = logger->getCurrentLogFilePath();
    EXPECT_NE(initialLogFile, currentLogFile); // File should have rotated
    
    // Original file should still exist (archived)
    EXPECT_TRUE(QFile::exists(initialLogFile));
}

TEST_F(LoggingFrameworkTest, LogRotationMaintainsConfiguredFileCount)
{
    // ARRANGE
    logger->setMaxLogFileSize(500);  // Very small limit
    logger->setMaxLogFileCount(3);   // Keep only 3 files
    logger->enableLogging(true);
    
    // ACT
    // Generate enough data for multiple rotations
    for (int i = 0; i < 200; ++i) {
        logger->logInfo(QString("Rotation test message %1 with extra content").arg(i));
        if (i % 20 == 0) {
            logger->flush(); // Periodic flush to trigger rotation checks
        }
    }
    logger->flush();
    
    // ASSERT
    QDir logDir(testDir);
    QStringList logFiles = logDir.entryList(QStringList() << "*.log", QDir::Files);
    EXPECT_LE(logFiles.size(), 3); // Should not exceed max file count
}

// ============================================================================
// PERFORMANCE TESTS
// ============================================================================

TEST_F(LoggingFrameworkTest, HighFrequencyLoggingMaintainsPerformance)
{
    // ARRANGE
    logger->enableLogging(true);
    
    const int messageCount = 10000;
    
    // ACT
    QTime startTime = QTime::currentTime();
    for (int i = 0; i < messageCount; ++i) {
        logger->logInfo(QString("Performance test message %1").arg(i));
    }
    logger->flush();
    int elapsedMs = startTime.msecsTo(QTime::currentTime());
    
    // ASSERT
    // Should log 10,000 messages in less than 1 second
    EXPECT_LT(elapsedMs, 1000);
    
    // Memory usage should remain reasonable
    EXPECT_LT(logger->getMemoryUsage(), 50 * 1024 * 1024); // <50MB
}

TEST_F(LoggingFrameworkTest, AsyncLoggingDoesNotBlockMainThread)
{
    // ARRANGE
    logger->enableAsyncLogging(true);
    logger->enableLogging(true);
    
    // ACT
    QTime startTime = QTime::currentTime();
    for (int i = 0; i < 1000; ++i) {
        logger->logInfo(QString("Async test message %1").arg(i));
    }
    int elapsedMs = startTime.msecsTo(QTime::currentTime());
    
    // ASSERT
    // Async logging should return almost immediately
    EXPECT_LT(elapsedMs, 100); // Should complete in <100ms
    
    // Wait for async processing to complete
    logger->waitForAsyncCompletion(5000);
    
    // Verify all messages were logged
    QFile logFile(logger->getCurrentLogFilePath());
    EXPECT_TRUE(logFile.open(QIODevice::ReadOnly));
    QString content = QString::fromUtf8(logFile.readAll());
    EXPECT_TRUE(content.contains("Async test message 999")); // Last message should be present
}

// ============================================================================
// MEMORY MANAGEMENT TESTS
// ============================================================================

TEST_F(LoggingFrameworkTest, MemoryUsageStaysWithinLimits)
{
    // ARRANGE
    logger->setMaxMemoryUsage(10 * 1024 * 1024); // 10MB limit
    logger->enableLogging(true);
    
    // ACT
    // Generate large amount of log data
    for (int i = 0; i < 50000; ++i) {
        logger->logInfo(QString("Memory test message %1 with additional content to increase memory usage").arg(i));
    }
    
    // ASSERT
    qint64 memoryUsage = logger->getMemoryUsage();
    EXPECT_LT(memoryUsage, 10 * 1024 * 1024); // Should stay within limit
}

TEST_F(LoggingFrameworkTest, BufferFlushingWorksCorrectly)
{
    // ARRANGE
    logger->setBufferSize(1024); // 1KB buffer
    logger->enableLogging(true);
    
    // ACT
    logger->logInfo("Test message before flush");
    
    // Verify buffer is not immediately flushed
    QFile logFile(logger->getCurrentLogFilePath());
    logFile.open(QIODevice::ReadOnly);
    EXPECT_EQ(logFile.size(), 0); // File should be empty before flush
    logFile.close();
    
    // Flush manually
    logger->flush();
    
    // ASSERT
    logFile.open(QIODevice::ReadOnly);
    EXPECT_GT(logFile.size(), 0); // File should contain data after flush
    QString content = QString::fromUtf8(logFile.readAll());
    EXPECT_TRUE(content.contains("Test message before flush"));
}

// ============================================================================
// CONFIGURATION TESTS
// ============================================================================

TEST_F(LoggingFrameworkTest, ConfigurationCanBeSavedAndLoaded)
{
    // ARRANGE
    logger->setLogLevel(Logger::LogLevel::Warning);
    logger->setOutputFormat(Logger::OutputFormat::Json);
    logger->setMaxLogFileSize(2048);
    logger->setMaxLogFileCount(5);
    
    QString configPath = testDir + "/logger_config.json";
    
    // ACT
    bool saveResult = logger->saveConfiguration(configPath);
    
    auto newLogger = std::make_unique<Logger>();
    bool loadResult = newLogger->loadConfiguration(configPath);
    
    // ASSERT
    EXPECT_TRUE(saveResult);
    EXPECT_TRUE(loadResult);
    EXPECT_EQ(newLogger->getLogLevel(), Logger::LogLevel::Warning);
    EXPECT_EQ(newLogger->getOutputFormat(), Logger::OutputFormat::Json);
    EXPECT_EQ(newLogger->getMaxLogFileSize(), 2048);
    EXPECT_EQ(newLogger->getMaxLogFileCount(), 5);
}

// ============================================================================
// GUI INTEGRATION TESTS
// ============================================================================

TEST_F(LoggingFrameworkTest, LoggerIntegratesWithGuiErrorDisplay)
{
    // ARRANGE & ACT & ASSERT
    EXPECT_TRUE(logger->supportsGuiIntegration());
    EXPECT_TRUE(logger->canConnectToMainWindow());
    EXPECT_TRUE(logger->hasRealtimeDisplayCapability());
}

TEST_F(LoggingFrameworkTest, LogMessagesEmitSignalsForGuiUpdates)
{
    // ARRANGE
    logger->enableLogging(true);
    QSignalSpy logMessageSpy(logger.get(), &Logger::logMessageAdded);
    QSignalSpy errorMessageSpy(logger.get(), &Logger::errorMessageAdded);
    
    // ACT
    logger->logInfo("Info message");
    logger->logError("Error message");
    
    // ASSERT
    EXPECT_EQ(logMessageSpy.count(), 2);    // Both messages should emit general signal
    EXPECT_EQ(errorMessageSpy.count(), 1);  // Only error should emit error signal
}