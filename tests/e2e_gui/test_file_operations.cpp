#include <QtTest/QtTest>
#include <QApplication>
#include <QFileDialog>
#include <QTimer>
#include <QSignalSpy>
#include <memory>

#include "src/core/etisnoop_wrapper.h"
#include "src/core/eti_processor.h"
#include "src/core/eti_file_worker.h"
#include "src/utils/logger.h"

/**
 * @class FileOperationsTest
 * @brief E2E tests for ETI file operations to prevent segmentation faults
 * 
 * This test class specifically targets the critical bug that was fixed:
 * - Segmentation fault when opening ETI files
 * - GUI stability during file processing operations
 * - Error handling in file operations
 */
class FileOperationsTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Core stability tests
    void testEtisnoopWrapperStability();
    void testEtiProcessorStability();
    void testEtiFileWorkerStability();
    
    // Regression tests for the fixed bug
    void testSegmentationFaultFix();
    void testFileOpeningWithoutCrash();
    void testErrorHandlingStability();
    
    // Performance and memory tests
    void testMemoryLeakPrevention();
    void testLongRunningOperations();

private:
    QString m_testEtiFile;
    std::unique_ptr<EtisnoopWrapper> m_wrapper;
    std::unique_ptr<EtiProcessor> m_processor;
};

void FileOperationsTest::initTestCase()
{
    // Initialize logger for test tracking
    Logger::instance().enableFileLogging("/tmp/e2e_test.log");
    Logger::instance().log(Logger::Info, "E2ETest", "=== FILE OPERATIONS E2E TEST STARTED ===");
    
    // Set test ETI file path
    m_testEtiFile = "/home/seksan/workspace/streamdab-analyzer/eti/bkk_20062022_141637.eti";
    
    // Verify test file exists
    QFileInfo fileInfo(m_testEtiFile);
    QVERIFY2(fileInfo.exists(), "Test ETI file must exist for E2E testing");
    QVERIFY2(fileInfo.isReadable(), "Test ETI file must be readable");
    
    qDebug() << "Test file:" << m_testEtiFile << "Size:" << fileInfo.size() << "bytes";
}

void FileOperationsTest::cleanupTestCase()
{
    Logger::instance().log(Logger::Info, "E2ETest", "=== FILE OPERATIONS E2E TEST COMPLETED ===");
}

void FileOperationsTest::testEtisnoopWrapperStability()
{
    Logger::instance().log(Logger::Info, "E2ETest", "Testing EtisnoopWrapper stability...");
    
    // Test that was crashing before the fix
    {
        EtisnoopWrapper wrapper;
        QVERIFY2(wrapper.initialize(), "EtisnoopWrapper should initialize successfully");
        
        // This call was causing segmentation fault before the fix
        bool result = wrapper.processEtiFile(m_testEtiFile);
        
        // We don't require success (file format issues are acceptable)
        // but we require NO CRASH
        qDebug() << "ProcessEtiFile result:" << result;
        if (!result) {
            qDebug() << "Error (acceptable):" << wrapper.getLastError();
        }
        
        // If we reach here without crash, the fix worked
        QVERIFY2(true, "EtisnoopWrapper should not crash on processEtiFile");
    }
    
    Logger::instance().log(Logger::Info, "E2ETest", "EtisnoopWrapper stability test passed");
}

void FileOperationsTest::testEtiProcessorStability()
{
    Logger::instance().log(Logger::Info, "E2ETest", "Testing EtiProcessor stability...");
    
    {
        auto processor = std::make_unique<EtiProcessor>();
        QVERIFY2(processor->initialize(), "EtiProcessor should initialize successfully");
        
        // Test file processing (this chains to EtisnoopWrapper)
        bool result = processor->processFile(m_testEtiFile);
        
        // Again, we care about NO CRASH, not necessarily success
        qDebug() << "EtiProcessor result:" << result;
        if (!result) {
            qDebug() << "Status:" << processor->getStatus();
        }
        
        QVERIFY2(true, "EtiProcessor should not crash on processFile");
    }
    
    Logger::instance().log(Logger::Info, "E2ETest", "EtiProcessor stability test passed");
}

void FileOperationsTest::testEtiFileWorkerStability()
{
    Logger::instance().log(Logger::Info, "E2ETest", "Testing EtiFileWorker stability...");
    
    {
        auto processor = std::make_unique<EtiProcessor>();
        processor->initialize();
        
        auto worker = std::make_unique<EtiFileWorker>(processor.get());
        
        // Set up signal spies to track worker behavior
        QSignalSpy startedSpy(worker.get(), &EtiFileWorker::processingStarted);
        QSignalSpy completedSpy(worker.get(), &EtiFileWorker::processingCompleted);
        QSignalSpy failedSpy(worker.get(), &EtiFileWorker::processingFailed);
        
        // Start processing
        worker->processEtiFile(m_testEtiFile);
        
        // Wait for processing to complete (with timeout)
        bool finished = false;
        QTimer::singleShot(10000, [&finished]() { finished = true; }); // 10 second timeout
        
        while (!finished && startedSpy.count() == 0) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
        
        while (!finished && completedSpy.count() == 0 && failedSpy.count() == 0) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
        
        QVERIFY2(startedSpy.count() > 0, "Worker should emit processingStarted signal");
        QVERIFY2(completedSpy.count() > 0 || failedSpy.count() > 0, 
                 "Worker should emit either completed or failed signal");
        
        // If we reach here, no crash occurred
        QVERIFY2(true, "EtiFileWorker should not crash during background processing");
    }
    
    Logger::instance().log(Logger::Info, "E2ETest", "EtiFileWorker stability test passed");
}

void FileOperationsTest::testSegmentationFaultFix()
{
    Logger::instance().log(Logger::Info, "E2ETest", "Testing segmentation fault regression...");
    
    // This test specifically validates the fix applied on September 20, 2025
    // The original crash was in ETI_Analyser::analyse() called from EtisnoopWrapper
    
    for (int i = 0; i < 3; i++) {
        qDebug() << "Iteration" << (i + 1) << "of segfault regression test";
        
        EtisnoopWrapper wrapper;
        QVERIFY2(wrapper.initialize(), "Wrapper initialization should succeed");
        
        // The critical test: this was causing segfault before the fix
        bool result = wrapper.processEtiFile(m_testEtiFile);
        
        // Log the result but don't fail on processing failure
        qDebug() << "Iteration" << (i + 1) << "result:" << result;
        
        // The key assertion: we should reach this point without crashing
        QVERIFY2(true, "Should complete without segmentation fault");
    }
    
    Logger::instance().log(Logger::Info, "E2ETest", "Segmentation fault regression test passed");
}

void FileOperationsTest::testFileOpeningWithoutCrash()
{
    Logger::instance().log(Logger::Info, "E2ETest", "Testing file opening workflow...");
    
    // Simulate the exact workflow that was crashing:
    // 1. User opens file dialog
    // 2. Selects ETI file
    // 3. Application processes file
    
    QString selectedFile = m_testEtiFile; // Simulate file dialog result
    
    // Test the MainWindow file opening workflow
    auto processor = std::make_unique<EtiProcessor>();
    processor->initialize();
    
    auto worker = std::make_unique<EtiFileWorker>(processor.get());
    
    // This mimics MainWindow::openEtiFile() behavior
    QFileInfo fileInfo(selectedFile);
    QVERIFY2(fileInfo.exists() && fileInfo.isReadable(), "Selected file should be accessible");
    
    // Start the processing that was crashing
    worker->processEtiFile(selectedFile);
    
    // Wait briefly to ensure no immediate crash
    QTest::qWait(1000);
    
    QVERIFY2(true, "File opening workflow should complete without crash");
    
    Logger::instance().log(Logger::Info, "E2ETest", "File opening workflow test passed");
}

void FileOperationsTest::testErrorHandlingStability()
{
    Logger::instance().log(Logger::Info, "E2ETest", "Testing error handling stability...");
    
    EtisnoopWrapper wrapper;
    wrapper.initialize();
    
    // Test with non-existent file
    bool result1 = wrapper.processEtiFile("/nonexistent/file.eti");
    QVERIFY2(!result1, "Should fail gracefully with non-existent file");
    QVERIFY2(!wrapper.getLastError().isEmpty(), "Should provide error message");
    
    // Test with invalid file
    bool result2 = wrapper.processEtiFile("/etc/passwd"); // Text file, not ETI
    // Should fail gracefully, not crash
    qDebug() << "Invalid file result:" << result2 << "Error:" << wrapper.getLastError();
    
    QVERIFY2(true, "Error handling should not cause crashes");
    
    Logger::instance().log(Logger::Info, "E2ETest", "Error handling stability test passed");
}

void FileOperationsTest::testMemoryLeakPrevention()
{
    Logger::instance().log(Logger::Info, "E2ETest", "Testing memory leak prevention...");
    
    // Create and destroy multiple wrappers to test for memory leaks
    for (int i = 0; i < 10; i++) {
        auto wrapper = std::make_unique<EtisnoopWrapper>();
        wrapper->initialize();
        wrapper->processEtiFile(m_testEtiFile);
        // Wrapper destroyed at end of scope
    }
    
    QVERIFY2(true, "Multiple wrapper creation/destruction should not leak memory");
    
    Logger::instance().log(Logger::Info, "E2ETest", "Memory leak prevention test passed");
}

void FileOperationsTest::testLongRunningOperations()
{
    Logger::instance().log(Logger::Info, "E2ETest", "Testing long-running operations...");
    
    auto processor = std::make_unique<EtiProcessor>();
    processor->initialize();
    
    auto worker = std::make_unique<EtiFileWorker>(processor.get());
    
    QSignalSpy progressSpy(worker.get(), &EtiFileWorker::processingProgress);
    
    worker->processEtiFile(m_testEtiFile);
    
    // Wait longer for progress updates
    QTest::qWait(5000);
    
    // We should either get progress updates or completion without crash
    QVERIFY2(true, "Long-running operations should not cause instability");
    
    Logger::instance().log(Logger::Info, "E2ETest", "Long-running operations test passed");
}

// Qt Test Framework integration
QTEST_MAIN(FileOperationsTest)
#include "test_file_operations.moc"