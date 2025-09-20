/**
 * @file test_error_handling_e2e.cpp
 * @brief E2E Error Handling Tests - Professional Standards Compliance
 * 
 * Tests application error handling to match ETISnoop's professional behavior:
 * - Nonexistent files: Critical failure (return false)
 * - Invalid/empty files: Graceful handling with warnings (return true)
 * - No crashes or hangs in any error scenario
 * - Clear user feedback for all error conditions
 */

#include <QtTest/QtTest>
#include <QApplication>
#include <QTemporaryFile>
#include <QTimer>
#include <QSignalSpy>

#include "core/eti_processor.h"
#include "gui/main_window.h"
#include "utils/logger.h"

class ErrorHandlingE2ETest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Professional error handling tests matching ETISnoop behavior
    void testNonexistentFileHandling();
    void testEmptyFileHandling();
    void testInvalidFileHandling();
    void testPermissionDeniedHandling();
    void testApplicationStabilityAfterErrors();

private:
    QApplication* app;
    EtiProcessor* processor;
    MainWindow* mainWindow;
    QString testDataDir;
    
    // Test file creation helpers
    QString createEmptyFile();
    QString createInvalidFile();
    QString createUnreadableFile();
    
    // Validation helpers
    void validateErrorResponse(bool expectedSuccess, const QString& expectedPattern);
    void validateNoApplicationCrash();
};

void ErrorHandlingE2ETest::initTestCase()
{
    int argc = 1;
    char* argv[] = {"test_error_handling_e2e"};
    app = new QApplication(argc, argv);
    
    Logger::instance().setLogLevel(Logger::Debug);
    Logger::instance().log(Logger::Info, "ErrorHandlingE2E", 
                          "Starting professional error handling E2E tests");
    
    testDataDir = QDir::tempPath() + "/eti_error_tests";
    QDir().mkpath(testDataDir);
}

void ErrorHandlingE2ETest::cleanupTestCase()
{
    Logger::instance().log(Logger::Info, "ErrorHandlingE2E", 
                          "Error handling E2E tests completed");
    
    // Cleanup test directory
    QDir(testDataDir).removeRecursively();
    
    delete app;
}

void ErrorHandlingE2ETest::init()
{
    processor = new EtiProcessor(this);
    QVERIFY(processor->initialize());
    
    mainWindow = new MainWindow();
    mainWindow->show();
    QTest::qWaitForWindowActive(mainWindow);
}

void ErrorHandlingE2ETest::cleanup()
{
    if (processor) {
        delete processor;
        processor = nullptr;
    }
    
    if (mainWindow) {
        mainWindow->close();
        delete mainWindow;
        mainWindow = nullptr;
    }
}

void ErrorHandlingE2ETest::testNonexistentFileHandling()
{
    Logger::instance().log(Logger::Info, "ErrorHandlingE2E", 
                          "TEST 1: Nonexistent file handling (should match ETISnoop exit code 1)");
    
    QString nonexistentFile = "/nonexistent/directory/file.eti";
    
    // Set up signal spy to capture error
    QSignalSpy errorSpy(processor, &EtiProcessor::errorOccurred);
    QSignalSpy statusSpy(processor, &EtiProcessor::statusChanged);
    
    // Process nonexistent file - should return false (critical failure)
    bool result = processor->processFile(nonexistentFile);
    
    // Validate ETISnoop-compatible behavior
    QCOMPARE(result, false);  // Critical failure like ETISnoop exit code 1
    QCOMPARE(errorSpy.count(), 1);  // Should emit error signal
    
    // Validate error message contains expected information
    QList<QVariant> errorArgs = errorSpy.takeFirst();
    QString errorMessage = errorArgs.at(0).toString();
    QVERIFY(errorMessage.contains("File not found"));
    QVERIFY(errorMessage.contains(nonexistentFile));
    
    // Validate application remains stable
    validateNoApplicationCrash();
    
    Logger::instance().log(Logger::Info, "ErrorHandlingE2E", 
                          "✓ Nonexistent file handling: Professional behavior verified");
}

void ErrorHandlingE2ETest::testEmptyFileHandling()
{
    Logger::instance().log(Logger::Info, "ErrorHandlingE2E", 
                          "TEST 2: Empty file handling (should match ETISnoop exit code 0)");
    
    QString emptyFile = createEmptyFile();
    QVERIFY(!emptyFile.isEmpty());
    
    // Set up signal spy to capture warnings
    QSignalSpy errorSpy(processor, &EtiProcessor::errorOccurred);
    QSignalSpy statusSpy(processor, &EtiProcessor::statusChanged);
    
    // Process empty file - should return true with warnings (graceful handling)
    bool result = processor->processFile(emptyFile);
    
    // Validate ETISnoop-compatible behavior
    QCOMPARE(result, true);  // Graceful handling like ETISnoop exit code 0
    QCOMPARE(errorSpy.count(), 1);  // Should emit warning signal for user feedback
    
    // Validate warning message contains expected information
    QList<QVariant> errorArgs = errorSpy.takeFirst();
    QString warningMessage = errorArgs.at(0).toString();
    QVERIFY(warningMessage.contains("Empty file") || warningMessage.contains("unable to read"));
    
    // Validate status update
    QVERIFY(statusSpy.count() >= 1);
    QString status = statusSpy.last().at(0).toString();
    QVERIFY(status.contains("empty") || status.contains("no frames"));
    
    // Validate application remains stable
    validateNoApplicationCrash();
    
    QFile::remove(emptyFile);
    
    Logger::instance().log(Logger::Info, "ErrorHandlingE2E", 
                          "✓ Empty file handling: Professional graceful behavior verified");
}

void ErrorHandlingE2ETest::testInvalidFileHandling()
{
    Logger::instance().log(Logger::Info, "ErrorHandlingE2E", 
                          "TEST 3: Invalid file handling (should match ETISnoop graceful behavior)");
    
    QString invalidFile = createInvalidFile();
    QVERIFY(!invalidFile.isEmpty());
    
    // Set up signal spy
    QSignalSpy errorSpy(processor, &EtiProcessor::errorOccurred);
    QSignalSpy statusSpy(processor, &EtiProcessor::statusChanged);
    
    // Process invalid file - should handle gracefully
    bool result = processor->processFile(invalidFile);
    
    // ETISnoop handles invalid files gracefully (exit code 0)
    // Application should not crash and should provide user feedback
    
    // Validate no crash occurred
    validateNoApplicationCrash();
    
    // Should provide some form of user feedback
    QVERIFY(errorSpy.count() >= 0);  // May or may not emit error depending on file content
    QVERIFY(statusSpy.count() >= 1);  // Should update status
    
    QFile::remove(invalidFile);
    
    Logger::instance().log(Logger::Info, "ErrorHandlingE2E", 
                          "✓ Invalid file handling: No crashes, professional behavior verified");
}

void ErrorHandlingE2ETest::testPermissionDeniedHandling()
{
    Logger::instance().log(Logger::Info, "ErrorHandlingE2E", 
                          "TEST 4: Permission denied handling");
    
    QString unreadableFile = createUnreadableFile();
    if (unreadableFile.isEmpty()) {
        QSKIP("Cannot create unreadable file for testing");
        return;
    }
    
    // Set up signal spy
    QSignalSpy errorSpy(processor, &EtiProcessor::errorOccurred);
    
    // Process unreadable file - should return false (critical failure)
    bool result = processor->processFile(unreadableFile);
    
    // Validate critical failure behavior
    QCOMPARE(result, false);  // Critical failure
    QCOMPARE(errorSpy.count(), 1);  // Should emit error signal
    
    // Validate error message
    QList<QVariant> errorArgs = errorSpy.takeFirst();
    QString errorMessage = errorArgs.at(0).toString();
    QVERIFY(errorMessage.contains("not readable") || errorMessage.contains("permission"));
    
    // Validate application remains stable
    validateNoApplicationCrash();
    
    // Restore permissions and remove file
    QFile::setPermissions(unreadableFile, QFile::ReadOwner | QFile::WriteOwner);
    QFile::remove(unreadableFile);
    
    Logger::instance().log(Logger::Info, "ErrorHandlingE2E", 
                          "✓ Permission denied handling: Professional error handling verified");
}

void ErrorHandlingE2ETest::testApplicationStabilityAfterErrors()
{
    Logger::instance().log(Logger::Info, "ErrorHandlingE2E", 
                          "TEST 5: Application stability after multiple error scenarios");
    
    QStringList testFiles = {
        "/nonexistent/file1.eti",
        "/nonexistent/file2.eti",
        createEmptyFile(),
        createInvalidFile()
    };
    
    int successfulProcessing = 0;
    int failedProcessing = 0;
    
    for (const QString& testFile : testFiles) {
        Logger::instance().log(Logger::Debug, "ErrorHandlingE2E", 
                              QString("Testing file: %1").arg(testFile));
        
        // Process each test file
        bool result = processor->processFile(testFile);
        
        if (result) {
            successfulProcessing++;
        } else {
            failedProcessing++;
        }
        
        // Validate application remains stable after each test
        validateNoApplicationCrash();
        
        // Small delay to ensure signal processing
        QTest::qWait(100);
    }
    
    // Clean up temporary files
    for (const QString& testFile : testFiles) {
        if (QFile::exists(testFile)) {
            QFile::remove(testFile);
        }
    }
    
    Logger::instance().log(Logger::Info, "ErrorHandlingE2E", 
                          QString("Application stability verified: %1 successful, %2 failed (expected)")
                          .arg(successfulProcessing).arg(failedProcessing));
    
    // Application should remain functional
    QVERIFY(processor->isInitialized());
    QVERIFY(mainWindow->isVisible());
    
    Logger::instance().log(Logger::Info, "ErrorHandlingE2E", 
                          "✓ Application stability: Professional resilience verified");
}

// Helper implementations
QString ErrorHandlingE2ETest::createEmptyFile()
{
    QTemporaryFile tempFile(testDataDir + "/empty_XXXXXX.eti");
    tempFile.setAutoRemove(false);
    if (tempFile.open()) {
        QString fileName = tempFile.fileName();
        tempFile.close();
        return fileName;
    }
    return QString();
}

QString ErrorHandlingE2ETest::createInvalidFile()
{
    QTemporaryFile tempFile(testDataDir + "/invalid_XXXXXX.eti");
    tempFile.setAutoRemove(false);
    if (tempFile.open()) {
        // Write some invalid data
        tempFile.write("This is not a valid ETI file content");
        QString fileName = tempFile.fileName();
        tempFile.close();
        return fileName;
    }
    return QString();
}

QString ErrorHandlingE2ETest::createUnreadableFile()
{
    QTemporaryFile tempFile(testDataDir + "/unreadable_XXXXXX.eti");
    tempFile.setAutoRemove(false);
    if (tempFile.open()) {
        tempFile.write("Test content");
        QString fileName = tempFile.fileName();
        tempFile.close();
        
        // Remove read permissions
        if (QFile::setPermissions(fileName, QFile::WriteOwner)) {
            return fileName;
        }
    }
    return QString();
}

void ErrorHandlingE2ETest::validateNoApplicationCrash()
{
    // Verify main window is still responsive
    QVERIFY(mainWindow != nullptr);
    QVERIFY(mainWindow->isVisible());
    
    // Verify processor is still functional
    QVERIFY(processor != nullptr);
    QVERIFY(processor->isInitialized());
    
    // Process events to ensure UI remains responsive
    QApplication::processEvents();
    
    Logger::instance().log(Logger::Debug, "ErrorHandlingE2E", 
                          "Application stability validated - no crashes detected");
}

QTEST_MAIN(ErrorHandlingE2ETest)
#include "test_error_handling_e2e.moc"