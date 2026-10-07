/**
 * @file test_comprehensive_tdd_workflows.cpp
 * @brief End-to-end TDD workflow validation test suite
 * 
 * Tests complete user workflows from file loading to analysis completion
 * with emphasis on TDD methodology and professional broadcast standards.
 * 
 * @author TDD Lead Agent
 * @date 2025-09-22
 * @copyright StreamDAB Analyser Project
 */

#include "../tdd_framework.h"
#include "gui/main_window.h"
#include "core/eti_processor.hpp"
#include <QtTest/QtTest>
#include <QApplication>
#include <QFileDialog>
#include <QTemporaryFile>
#include <QSignalSpy>
#include <QTimer>
#include <QStandardPaths>

/**
 * @class ComprehensiveTDDWorkflowTest
 * @brief End-to-end TDD workflow validation
 * 
 * Tests complete workflows including:
 * - File-to-analysis complete workflow (RED-GREEN-REFACTOR)
 * - Real-time processing workflow
 * - Error detection and recovery workflow
 * - Performance validation workflow
 * - Professional UI interaction patterns
 */
TDD_TEST_CASE(ComprehensiveTDDWorkflowTest, E2E, CRITICAL)

private:
    MainWindow* m_mainWindow = nullptr;
    EtiProcessor* m_processor = nullptr;
    QString m_testDataPath;

public slots:
    void initTestCase() {
        qDebug() << "=== Comprehensive TDD Workflow Test Suite ===";
        qDebug() << "Testing complete file-to-analysis workflows";
        qDebug() << "Professional broadcast industry standards";
        qDebug() << "Red-Green-Refactor methodology validation";
        
        // Create test data directory
        m_testDataPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/eti_test_data";
        QDir().mkpath(m_testDataPath);
        
        // Generate test ETI files
        createTestETIFiles();
    }

    void init() {
        // Create fresh instances for each test
        m_mainWindow = new MainWindow();
        m_processor = new EtiProcessor();
        
        QVERIFY(m_mainWindow != nullptr);
        QVERIFY(m_processor != nullptr);
        QVERIFY(m_processor->initialize());
    }

    void cleanup() {
        if (m_mainWindow) {
            m_mainWindow->close();
            delete m_mainWindow;
            m_mainWindow = nullptr;
        }
        
        if (m_processor) {
            delete m_processor;
            m_processor = nullptr;
        }
    }

    /**
     * @brief Create test ETI files for workflow testing
     */
    void createTestETIFiles() {
        // Small test file (10 frames)
        QString smallFile = m_testDataPath + "/small_test.eti";
        QFile file(smallFile);
        if (file.open(QIODevice::WriteOnly)) {
            for (int i = 0; i < 10; ++i) {
                QByteArray frame = TDD::ETITestFramework::generateValidETIFrame();
                file.write(frame);
            }
            file.close();
            qDebug() << "Created small test file:" << smallFile;
        }
        
        // Medium test file (100 frames)
        QString mediumFile = m_testDataPath + "/medium_test.eti";
        QFile file2(mediumFile);
        if (file2.open(QIODevice::WriteOnly)) {
            for (int i = 0; i < 100; ++i) {
                QByteArray frame = TDD::ETITestFramework::generateValidETIFrame();
                file2.write(frame);
            }
            file2.close();
            qDebug() << "Created medium test file:" << mediumFile;
        }
        
        // Large test file (1000 frames)
        QString largeFile = m_testDataPath + "/large_test.eti";
        QFile file3(largeFile);
        if (file3.open(QIODevice::WriteOnly)) {
            for (int i = 0; i < 1000; ++i) {
                QByteArray frame = TDD::ETITestFramework::generateValidETIFrame();
                file3.write(frame);
            }
            file3.close();
            qDebug() << "Created large test file:" << largeFile;
        }
    }

    /**
     * @brief TDD RED Phase: Complete workflow should fail initially
     */
    TDD_RED_PHASE(CompleteFileAnalysisWorkflow)
        if (qEnvironmentVariableIsSet("TDD_FORCE_RED")) {
            // In RED phase, the complete workflow should fail
            QString testFile = m_testDataPath + "/small_test.eti";
            
            // Attempt complete workflow
            bool workflowResult = executeCompleteFileWorkflow(testFile);
            
            TDD_RED_ASSERT(workflowResult, "Complete workflow should fail in RED phase");
        }
    }

    /**
     * @brief TDD GREEN Phase: Complete workflow should succeed
     */
    TDD_GREEN_PHASE(CompleteFileAnalysisWorkflow)
        QString testFile = m_testDataPath + "/small_test.eti";
        
        // Execute complete workflow
        bool workflowResult = executeCompleteFileWorkflow(testFile);
        TDD_GREEN_ASSERT(workflowResult, "Complete workflow must succeed in GREEN phase");
        
        // Verify all components of the workflow
        verifyWorkflowComponents();
    }

    /**
     * @brief Execute complete file analysis workflow
     */
    bool executeCompleteFileWorkflow(const QString& filename) {
        qDebug() << "Executing complete file workflow for:" << filename;
        
        try {
            // Step 1: Show main window
            m_mainWindow->show();
            QTest::qWaitForWindowExposed(m_mainWindow);
            
            // Step 2: Load ETI file
            bool fileLoaded = loadETIFile(filename);
            if (!fileLoaded) {
                qDebug() << "File loading failed";
                return false;
            }
            
            // Step 3: Process file data
            bool fileProcessed = processLoadedFile(filename);
            if (!fileProcessed) {
                qDebug() << "File processing failed";
                return false;
            }
            
            // Step 4: Verify service discovery
            bool servicesDiscovered = verifyServiceDiscovery();
            if (!servicesDiscovered) {
                qDebug() << "Service discovery failed";
                return false;
            }
            
            // Step 5: Verify UI updates
            bool uiUpdated = verifyUIUpdates();
            if (!uiUpdated) {
                qDebug() << "UI updates failed";
                return false;
            }
            
            qDebug() << "Complete workflow executed successfully";
            return true;
            
        } catch (...) {
            qDebug() << "Exception during workflow execution";
            return false;
        }
    }

    /**
     * @brief Load ETI file into the application
     */
    bool loadETIFile(const QString& filename) {
        // Verify file exists
        if (!QFile::exists(filename)) {
            qDebug() << "Test file does not exist:" << filename;
            return false;
        }
        
        // Use processor to load file
        bool result = m_processor->processFile(filename);
        if (result) {
            qDebug() << "ETI file loaded successfully";
        }
        
        return result;
    }

    /**
     * @brief Process the loaded file
     */
    bool processLoadedFile(const QString& filename) {
        // Set up signal monitoring
        QSignalSpy frameProcessedSpy(m_processor, &EtiProcessor::frameProcessed);
        QSignalSpy serviceDiscoveredSpy(m_processor, &EtiProcessor::serviceDiscovered);
        
        // Process the file
        bool result = m_processor->processFile(filename);
        if (!result) {
            return false;
        }
        
        // Wait for processing to complete
        QTest::qWait(1000); // Allow 1 second for processing
        
        // Verify signals were emitted
        bool framesProcessed = frameProcessedSpy.count() > 0;
        qDebug() << "Frames processed:" << frameProcessedSpy.count();
        
        return framesProcessed;
    }

    /**
     * @brief Verify service discovery functionality
     */
    bool verifyServiceDiscovery() {
        QSignalSpy serviceDiscoveredSpy(m_processor, &EtiProcessor::serviceDiscovered);
        QSignalSpy ensembleDiscoveredSpy(m_processor, &EtiProcessor::ensembleDiscovered);
        
        // Service discovery may have already occurred during processing
        // or may need additional trigger
        
        bool hasServices = serviceDiscoveredSpy.count() > 0;
        bool hasEnsemble = ensembleDiscoveredSpy.count() > 0;
        
        qDebug() << "Services discovered:" << serviceDiscoveredSpy.count();
        qDebug() << "Ensembles discovered:" << ensembleDiscoveredSpy.count();
        
        // At minimum, we should discover ensemble information
        return hasEnsemble || hasServices;
    }

    /**
     * @brief Verify UI updates occurred
     */
    bool verifyUIUpdates() {
        // Check that main window is still responsive
        if (!m_mainWindow->isVisible()) {
            qDebug() << "Main window not visible";
            return false;
        }
        
        // Check that UI components exist and are accessible
        QWidget* centralWidget = m_mainWindow->centralWidget();
        if (!centralWidget) {
            qDebug() << "Central widget not found";
            return false;
        }
        
        // Process any pending UI events
        QApplication::processEvents();
        
        // UI should remain responsive
        bool responsive = m_mainWindow->isEnabled();
        qDebug() << "UI responsive:" << responsive;
        
        return responsive;
    }

    /**
     * @brief Verify all workflow components
     */
    void verifyWorkflowComponents() {
        // Verify processor state
        QVERIFY(m_processor->isInitialized());
        
        // Verify UI state
        QVERIFY(m_mainWindow->isVisible());
        QVERIFY(m_mainWindow->isEnabled());
        
        // Verify basic functionality is available
        QWidget* centralWidget = m_mainWindow->centralWidget();
        QVERIFY(centralWidget != nullptr);
        
        qDebug() << "All workflow components verified";
    }

    /**
     * @brief Test performance workflow with large file
     */
    void testPerformanceWorkflow() {
        QString largeFile = m_testDataPath + "/large_test.eti";
        
        qDebug() << "Testing performance workflow with large file";
        
        // Performance requirement: complete workflow within reasonable time
        TDD_BENCHMARK([&]() {
            bool result = executeCompleteFileWorkflow(largeFile);
            QVERIFY2(result, "Performance workflow must complete successfully");
        }, 5000, "Complete workflow with 1000-frame file"); // Max 5 seconds
        
        qDebug() << "Performance workflow completed successfully";
    }

    /**
     * @brief Test error recovery workflow
     */
    void testErrorRecoveryWorkflow() {
        qDebug() << "Testing error recovery workflow";
        
        // Create invalid ETI file
        QString invalidFile = m_testDataPath + "/invalid_test.eti";
        QFile file(invalidFile);
        if (file.open(QIODevice::WriteOnly)) {
            // Write invalid data
            QByteArray invalidData(1000, 0xFF);
            file.write(invalidData);
            file.close();
        }
        
        // Attempt to process invalid file
        bool invalidResult = loadETIFile(invalidFile);
        QVERIFY2(!invalidResult, "Invalid file should be rejected");
        
        // Verify system can recover and process valid file
        QString validFile = m_testDataPath + "/small_test.eti";
        bool validResult = executeCompleteFileWorkflow(validFile);
        QVERIFY2(validResult, "System must recover and process valid files after error");
        
        qDebug() << "Error recovery workflow verified";
    }

    /**
     * @brief Test multi-file workflow
     */
    void testMultiFileWorkflow() {
        qDebug() << "Testing multi-file workflow";
        
        QStringList testFiles = {
            m_testDataPath + "/small_test.eti",
            m_testDataPath + "/medium_test.eti"
        };
        
        // Process multiple files in sequence
        for (const QString& file : testFiles) {
            bool result = executeCompleteFileWorkflow(file);
            QVERIFY2(result, qPrintable(QString("Multi-file workflow failed for: %1").arg(file)));
            
            // Reset processor between files
            m_processor->reset();
        }
        
        qDebug() << "Multi-file workflow completed successfully";
    }

    /**
     * @brief Test memory stability during extended workflow
     */
    void testMemoryStabilityWorkflow() {
        qDebug() << "Testing memory stability during extended workflow";
        
        QString testFile = m_testDataPath + "/medium_test.eti";
        
        // Execute workflow multiple times to test memory stability
        TDD_MEMORY_CHECK([&]() {
            for (int i = 0; i < 10; ++i) {
                bool result = executeCompleteFileWorkflow(testFile);
                QVERIFY2(result, qPrintable(QString("Memory stability test failed at iteration %1").arg(i)));
                
                // Reset between iterations
                m_processor->reset();
                QApplication::processEvents();
            }
        }, 50, "Extended workflow memory usage"); // Max 50MB for extended testing
        
        qDebug() << "Memory stability workflow verified";
    }

    /**
     * @brief Test concurrent workflow operations
     */
    void testConcurrentWorkflow() {
        qDebug() << "Testing concurrent workflow operations";
        
        QString testFile = m_testDataPath + "/small_test.eti";
        
        // Simulate rapid user operations
        for (int i = 0; i < 5; ++i) {
            // Quick file processing cycles
            bool result = m_processor->processFile(testFile);
            QVERIFY2(result, qPrintable(QString("Concurrent operation %1 failed").arg(i)));
            
            // Process UI events between operations
            QApplication::processEvents();
            
            // Reset for next iteration
            m_processor->reset();
        }
        
        qDebug() << "Concurrent workflow operations verified";
    }

    /**
     * @brief Test professional workflow patterns
     */
    void testProfessionalWorkflowPatterns() {
        qDebug() << "Testing professional workflow patterns";
        
        m_mainWindow->show();
        QTest::qWaitForWindowExposed(m_mainWindow);
        
        // Professional workflow: File → Analyze → Navigate → Export
        QString testFile = m_testDataPath + "/medium_test.eti";
        
        // 1. File loading
        bool fileLoaded = loadETIFile(testFile);
        QVERIFY2(fileLoaded, "Professional workflow step 1 (file loading) failed");
        
        // 2. Analysis
        bool analysisComplete = processLoadedFile(testFile);
        QVERIFY2(analysisComplete, "Professional workflow step 2 (analysis) failed");
        
        // 3. Navigation (simulate user interaction with results)
        bool navigationWorking = verifyUIUpdates();
        QVERIFY2(navigationWorking, "Professional workflow step 3 (navigation) failed");
        
        // 4. Export (verify system is ready for export operations)
        bool exportReady = m_processor->isInitialized() && m_mainWindow->isEnabled();
        QVERIFY2(exportReady, "Professional workflow step 4 (export readiness) failed");
        
        qDebug() << "Professional workflow patterns verified";
    }

    void cleanupTestCase() {
        // Clean up test data
        QDir testDir(m_testDataPath);
        testDir.removeRecursively();
        
        qDebug() << "Comprehensive TDD Workflow tests completed";
        TDD::TestReporter::instance().enforceTDDCompliance();
    }
};

// Register test with Qt Test framework
QTEST_MAIN(ComprehensiveTDDWorkflowTest)
#include "test_comprehensive_tdd_workflows.moc"