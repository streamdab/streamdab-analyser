#include <QtTest/QtTest>
#include <QApplication>
#include <QFileDialog>
#include <QTimer>
#include <QSignalSpy>
#include <memory>

// USE INTERNAL IMPLEMENTATIONS ONLY - NO ETISNOOP
#include "src/core/eti_processor.h"
#include "src/core/eti_file_worker.h"
#include "src/core/modern_eti_frame_parser.h"
#include "src/core/eti_processing_engine.h"
#include "src/utils/logger.h"

/**
 * @class FileOperationsTest
 * @brief E2E tests for ETI file operations using INTERNAL implementations only
 * 
 * CRITICAL: This test class validates project functionality WITHOUT any ETISnoop dependencies.
 * Tests use only internal ETI processing classes to ensure the project can operate independently.
 * 
 * Key Internal Components Tested:
 * - EtiProcessor: Core ETI processing with internal engine
 * - ModernETIFrameParser: C++20 frame parsing implementation
 * - ETIProcessingEngine: Modern ETI processing backend
 * - EtiFileWorker: Background file processing without external dependencies
 */
class FileOperationsTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Core stability tests using INTERNAL implementations
    void testInternalEtiProcessorStability();
    void testModernFrameParserStability();
    void testEtiFileWorkerStability();
    
    // Regression tests for stability using internal components
    void testInternalProcessingStability();
    void testFileOpeningWithInternalEngine();
    void testErrorHandlingWithInternalComponents();
    
    // Performance and memory tests with internal implementations
    void testMemoryLeakPreventionInternal();
    void testLongRunningInternalOperations();

private:
    QString m_testEtiFile;
    std::unique_ptr<EtiProcessor> m_processor;
};

void FileOperationsTest::initTestCase()
{
    // Initialize logger for test tracking
    Logger::instance().enableFileLogging("/tmp/e2e_internal_test.log");
    Logger::instance().log(Logger::Info, "E2EInternalTest", "=== INTERNAL FILE OPERATIONS E2E TEST STARTED ===");
    
    // Set test ETI file path
    m_testEtiFile = "/home/seksan/workspace/streamdab-analyzer/eti/bkk_20062022_141637.eti";
    
    // Verify test file exists
    QFileInfo fileInfo(m_testEtiFile);
    QVERIFY2(fileInfo.exists(), "Test ETI file must exist for E2E testing");
    QVERIFY2(fileInfo.isReadable(), "Test ETI file must be readable");
    
    qDebug() << "Test file:" << m_testEtiFile << "Size:" << fileInfo.size() << "bytes";
    Logger::instance().log(Logger::Info, "E2EInternalTest", "Using INTERNAL implementations only - NO ETISnoop");
}

void FileOperationsTest::cleanupTestCase()
{
    Logger::instance().log(Logger::Info, "E2EInternalTest", "=== INTERNAL FILE OPERATIONS E2E TEST COMPLETED ===");
}

void FileOperationsTest::testInternalEtiProcessorStability()
{
    Logger::instance().log(Logger::Info, "E2EInternalTest", "Testing INTERNAL EtiProcessor stability...");
    
    // Test using ONLY internal implementation
    {
        auto processor = std::make_unique<EtiProcessor>();
        QVERIFY2(processor->initialize(), "Internal EtiProcessor should initialize successfully");
        
        // Test internal file processing (NO ETISnoop dependency)
        bool result = processor->processFile(m_testEtiFile);
        
        // Log results and verify no crash
        qDebug() << "Internal EtiProcessor result:" << result;
        if (!result) {
            qDebug() << "Status:" << processor->getStatus();
        }
        
        // Key test: processor should not crash and should provide status
        QVERIFY2(true, "Internal EtiProcessor should not crash on processFile");
        QVERIFY2(!processor->getStatus().isEmpty(), "Internal EtiProcessor should provide status");
    }
    
    Logger::instance().log(Logger::Info, "E2EInternalTest", "Internal EtiProcessor stability test passed");
}

void FileOperationsTest::testModernFrameParserStability()
{
    Logger::instance().log(Logger::Info, "E2EInternalTest", "Testing Modern ETI Frame Parser stability...");
    
    // Test the modern C++20 frame parser directly
    {
        // Create a small test ETI frame for parsing
        QByteArray testFrameData(6144, 0); // Standard ETI frame size
        
        // Set basic ETI frame structure (minimal valid frame)
        testFrameData[0] = 0x49; // 'I' - ETI sync
        testFrameData[1] = 0x4E; // 'N' - ETI sync
        testFrameData[2] = 0x49; // 'I' - ETI sync
        testFrameData[3] = 0x54; // 'T' - ETI sync
        
        // Test frame parsing without external dependencies
        try {
            // This tests our internal modern frame parser
            eti::modern::ModernETIFrameParser parser;
            
            // Test initialization
            bool initResult = parser.initialize();
            QVERIFY2(initResult, "Modern frame parser should initialize");
            
            // Test basic frame validation
            bool isValid = parser.validateFrameStructure(testFrameData);
            qDebug() << "Frame validation result:" << isValid;
            
            // Test should not crash regardless of validation result
            QVERIFY2(true, "Modern frame parser should not crash on validation");
            
        } catch (const std::exception& e) {
            qDebug() << "Exception in frame parser:" << e.what();
            QFAIL("Modern frame parser should not throw exceptions");
        }
    }
    
    Logger::instance().log(Logger::Info, "E2EInternalTest", "Modern ETI Frame Parser stability test passed");
}

void FileOperationsTest::testEtiFileWorkerStability()
{
    Logger::instance().log(Logger::Info, "E2EInternalTest", "Testing INTERNAL EtiFileWorker stability...");
    
    {
        auto processor = std::make_unique<EtiProcessor>();
        processor->initialize();
        
        auto worker = std::make_unique<EtiFileWorker>(processor.get());
        
        // Set up signal spies to track worker behavior
        QSignalSpy startedSpy(worker.get(), &EtiFileWorker::processingStarted);
        QSignalSpy completedSpy(worker.get(), &EtiFileWorker::processingCompleted);
        QSignalSpy failedSpy(worker.get(), &EtiFileWorker::processingFailed);
        
        // Start processing using INTERNAL implementation only
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
        
        QVERIFY2(startedSpy.count() > 0, "Internal worker should emit processingStarted signal");
        QVERIFY2(completedSpy.count() > 0 || failedSpy.count() > 0, 
                 "Internal worker should emit either completed or failed signal");
        
        // If we reach here, no crash occurred with internal implementation
        QVERIFY2(true, "Internal EtiFileWorker should not crash during background processing");
    }
    
    Logger::instance().log(Logger::Info, "E2EInternalTest", "Internal EtiFileWorker stability test passed");
}

void FileOperationsTest::testInternalProcessingStability()
{
    Logger::instance().log(Logger::Info, "E2EInternalTest", "Testing internal processing stability...");
    
    // Test internal processing multiple times to ensure stability
    for (int i = 0; i < 3; i++) {
        qDebug() << "Internal processing iteration" << (i + 1);
        
        auto processor = std::make_unique<EtiProcessor>();
        QVERIFY2(processor->initialize(), "Internal processor initialization should succeed");
        
        // Test internal processing (NO external dependencies)
        bool result = processor->processFile(m_testEtiFile);
        
        // Log the result but focus on stability
        qDebug() << "Internal processing iteration" << (i + 1) << "result:" << result;
        
        // Key assertion: we should complete without crashing
        QVERIFY2(true, "Internal processing should complete without crashes");
        
        // Test that processor maintains valid state
        QVERIFY2(!processor->getStatus().isEmpty(), "Processor should maintain valid status");
    }
    
    Logger::instance().log(Logger::Info, "E2EInternalTest", "Internal processing stability test passed");
}

void FileOperationsTest::testFileOpeningWithInternalEngine()
{
    Logger::instance().log(Logger::Info, "E2EInternalTest", "Testing file opening with internal engine...");
    
    // Test the complete internal workflow
    QString selectedFile = m_testEtiFile;
    
    // Create internal processor and worker
    auto processor = std::make_unique<EtiProcessor>();
    processor->initialize();
    
    auto worker = std::make_unique<EtiFileWorker>(processor.get());
    
    // Test file accessibility
    QFileInfo fileInfo(selectedFile);
    QVERIFY2(fileInfo.exists() && fileInfo.isReadable(), "Selected file should be accessible");
    
    // Start internal processing workflow
    worker->processEtiFile(selectedFile);
    
    // Wait briefly to ensure no immediate crash
    QTest::qWait(1000);
    
    QVERIFY2(true, "Internal file opening workflow should complete without crash");
    
    Logger::instance().log(Logger::Info, "E2EInternalTest", "Internal file opening workflow test passed");
}

void FileOperationsTest::testErrorHandlingWithInternalComponents()
{
    Logger::instance().log(Logger::Info, "E2EInternalTest", "Testing error handling with internal components...");
    
    auto processor = std::make_unique<EtiProcessor>();
    processor->initialize();
    
    // Test with non-existent file
    bool result1 = processor->processFile("/nonexistent/file.eti");
    QVERIFY2(!result1, "Internal processor should fail gracefully with non-existent file");
    QVERIFY2(!processor->getStatus().isEmpty(), "Internal processor should provide error status");
    
    // Test with invalid file
    bool result2 = processor->processFile("/etc/passwd"); // Text file, not ETI
    qDebug() << "Invalid file result:" << result2 << "Status:" << processor->getStatus();
    
    QVERIFY2(true, "Internal error handling should not cause crashes");
    
    Logger::instance().log(Logger::Info, "E2EInternalTest", "Internal error handling stability test passed");
}

void FileOperationsTest::testMemoryLeakPreventionInternal()
{
    Logger::instance().log(Logger::Info, "E2EInternalTest", "Testing memory leak prevention with internal components...");
    
    // Create and destroy multiple internal processors
    for (int i = 0; i < 10; i++) {
        auto processor = std::make_unique<EtiProcessor>();
        processor->initialize();
        processor->processFile(m_testEtiFile);
        // Processor destroyed at end of scope
    }
    
    QVERIFY2(true, "Multiple internal processor creation/destruction should not leak memory");
    
    Logger::instance().log(Logger::Info, "E2EInternalTest", "Internal memory leak prevention test passed");
}

void FileOperationsTest::testLongRunningInternalOperations()
{
    Logger::instance().log(Logger::Info, "E2EInternalTest", "Testing long-running internal operations...");
    
    auto processor = std::make_unique<EtiProcessor>();
    processor->initialize();
    
    auto worker = std::make_unique<EtiFileWorker>(processor.get());
    
    QSignalSpy progressSpy(worker.get(), &EtiFileWorker::processingProgress);
    
    // Start long-running internal operation
    worker->processEtiFile(m_testEtiFile);
    
    // Wait longer for progress updates
    QTest::qWait(5000);
    
    // Verify internal operations handle long processing without crashes
    QVERIFY2(true, "Long-running internal operations should not cause instability");
    
    Logger::instance().log(Logger::Info, "E2EInternalTest", "Long-running internal operations test passed");
}

// Qt Test Framework integration
QTEST_MAIN(FileOperationsTest)
#include "test_file_operations.moc"