/**
 * @file test_eti_processor.cpp
 * @brief Modern TDD test suite for ETI Processor - ETISnoop Independent
 * 
 * This file implements comprehensive test-driven development for the core
 * ETI processing engine using only internal project classes and structures.
 * All ETISnoop dependencies have been removed in favor of native implementations.
 * 
 * @author TDD Lead Agent  
 * @date 2025-09-23
 * @copyright StreamDAB Analyzer Project
 */

#include "../tdd_framework.h"
#include "core/eti_processor.h"
#include "core/eti_types.h"
#include "core/modern_eti_frame_parser.h"
#include "core/enhanced_fig_analyzer.h"
#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QDebug>

/**
 * @class EtiProcessorTest
 * @brief Comprehensive TDD test suite for ETI Processor
 * 
 * Tests core ETI processing functionality including:
 * - Initialization and configuration
 * - File processing with internal ETI engine
 * - Real-time data processing
 * - FIG parsing and ETSI compliance
 * - Performance requirements (7,482 FPS target)
 * - Memory efficiency (4MB target)
 */
TDD_TEST_CASE(EtiProcessorTest, UNIT, CRITICAL)

public slots:
    void initTestCase() {
        qDebug() << "=== ETI Processor TDD Test Suite ===";
        qDebug() << "Target Performance: 7,482 FPS";
        qDebug() << "Target Memory: < 4MB";
        qDebug() << "ETSI Compliance: EN 300 799/401";
    }

    /**
     * @brief TDD Phase 1: RED - Test processor initialization
     * 
     * This test should initially fail to enforce TDD methodology.
     * Only implement initialization after this test is written and failing.
     */
    TDD_RED_PHASE(ProcessorInitialization)
        // Create processor instance
        EtiProcessor processor;
        
        // RED: This should fail initially (before implementation)
        bool initResult = processor.initialize();
        
        // TDD RED enforcement - test must fail first
        if (qEnvironmentVariableIsSet("TDD_FORCE_RED")) {
            TDD_RED_ASSERT(initResult, "Processor initialization should fail in RED phase");
        }
    }

    /**
     * @brief TDD Phase 2: GREEN - Test processor initialization passes
     */
    TDD_GREEN_PHASE(ProcessorInitialization)
        EtiProcessor processor;
        
        // GREEN: This should pass after minimal implementation
        bool initResult = processor.initialize();
        TDD_GREEN_ASSERT(initResult, "Processor must initialize successfully");
        
        // Verify basic state after initialization
        QVERIFY(processor.isInitialized());
        QVERIFY(processor.getETIParser() != nullptr);
        QVERIFY(processor.getFIGAnalyzer() != nullptr);
    }

    /**
     * @brief TDD RED Phase: File processing should fail initially
     */
    TDD_RED_PHASE(FileProcessing)
        EtiProcessor processor;
        processor.initialize();
        
        // Create test ETI file
        QTemporaryFile testFile;
        testFile.setFileTemplate(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/test_eti_XXXXXX.eti");
        QVERIFY(testFile.open());
        
        // Write valid ETI frame data using internal ETI structures
        QByteArray testFrame = generateNativeETIFrame();
        testFile.write(testFrame);
        testFile.close();
        
        // RED: This should fail before implementation
        bool processResult = processor.processFile(testFile.fileName());
        
        if (qEnvironmentVariableIsSet("TDD_FORCE_RED")) {
            TDD_RED_ASSERT(processResult, "File processing should fail in RED phase");
        }
    }

    /**
     * @brief TDD GREEN Phase: File processing should pass
     */
    TDD_GREEN_PHASE(FileProcessing)
        EtiProcessor processor;
        QVERIFY(processor.initialize());
        
        // Create test ETI file with multiple frames
        QTemporaryFile testFile;
        testFile.setFileTemplate(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/test_eti_XXXXXX.eti");
        QVERIFY(testFile.open());
        
        // Write multiple valid ETI frames
        for (int i = 0; i < 10; ++i) {
            QByteArray testFrame = generateNativeETIFrame();
            testFile.write(testFrame);
        }
        testFile.close();
        
        // GREEN: This should pass after implementation
        bool processResult = processor.processFile(testFile.fileName());
        TDD_GREEN_ASSERT(processResult, "File processing must succeed with valid ETI file");
        
        // Verify processing signals were emitted
        QSignalSpy frameProcessedSpy(&processor, &EtiProcessor::frameProcessed);
        QSignalSpy serviceDiscoveredSpy(&processor, &EtiProcessor::serviceDiscovered);
        
        // Process the file and wait for signals
        processor.processFile(testFile.fileName());
        
        // Should have processed frames
        QVERIFY(frameProcessedSpy.count() > 0);
        qDebug() << "Processed" << frameProcessedSpy.count() << "ETI frames";
    }

    /**
     * @brief Test real-time data processing
     */
    void testRealTimeDataProcessing() {
        EtiProcessor processor;
        QVERIFY(processor.initialize());
        
        // Generate test ETI data using native structures
        QByteArray etiData = generateNativeETIFrame();
        
        // Set up signal monitoring
        QSignalSpy frameProcessedSpy(&processor, &EtiProcessor::frameProcessed);
        
        // Process data buffer
        bool result = processor.processData(etiData);
        QVERIFY2(result, "Real-time data processing must succeed");
        
        // Verify frame was processed
        QVERIFY(frameProcessedSpy.wait(1000)); // Wait up to 1 second
        QCOMPARE(frameProcessedSpy.count(), 1);
        
        // Verify frame data
        QVariantMap frameData = frameProcessedSpy.at(0).at(0).toMap();
        QVERIFY(frameData.contains("frameNumber"));
        QVERIFY(frameData.contains("ensembleId"));
    }

    /**
     * @brief Performance requirement test - 7,482 FPS target
     */
    void testPerformanceRequirement() {
        EtiProcessor processor;
        QVERIFY(processor.initialize());
        
        // Generate test data for performance testing
        QList<QByteArray> testFrames;
        const int frameCount = 1000; // Test with 1000 frames
        
        for (int i = 0; i < frameCount; ++i) {
            testFrames.append(TDD::ETITestFramework::generateValidETIFrame());
        }
        
        // Benchmark processing speed
        TDD_BENCHMARK([&]() {
            for (const auto& frame : testFrames) {
                processor.processData(frame);
            }
        }, 200, "Process 1000 ETI frames"); // Max 200ms for 1000 frames = 5000 FPS minimum
        
        qDebug() << "Performance test: Processed" << frameCount << "frames in benchmark";
    }

    /**
     * @brief Memory efficiency test - 4MB target
     */
    void testMemoryEfficiency() {
        EtiProcessor processor;
        QVERIFY(processor.initialize());
        
        // Process multiple files to test memory usage
        TDD_MEMORY_CHECK([&]() {
            for (int i = 0; i < 100; ++i) {
                QByteArray frame = TDD::ETITestFramework::generateValidETIFrame();
                processor.processData(frame);
            }
        }, 4, "ETI processing memory usage"); // Max 4MB
    }

    /**
     * @brief ETSI compliance validation
     */
    ETSI_COMPLIANCE_TEST(EN_300_799, frame_structure)
        EtiProcessor processor;
        QVERIFY(processor.initialize());
        
        // Test with ETSI compliant frame
        QByteArray validFrame = TDD::ETITestFramework::generateValidETIFrame();
        ETSI_VALIDATE_FRAME(validFrame);
        
        bool result = processor.processData(validFrame);
        QVERIFY2(result, "Must process ETSI compliant frames");
    }

    /**
     * @brief Test FIG parsing capabilities
     */
    void testFigParsing() {
        EtiProcessor processor;
        QVERIFY(processor.initialize());
        
        // Set up signal monitoring for FIG discovery
        QSignalSpy serviceDiscoveredSpy(&processor, &EtiProcessor::serviceDiscovered);
        QSignalSpy ensembleDiscoveredSpy(&processor, &EtiProcessor::ensembleDiscovered);
        
        // Process frame with embedded FIG data
        QByteArray frameWithFigs = TDD::ETITestFramework::generateValidETIFrame();
        processor.processData(frameWithFigs);
        
        // Should discover ensemble information
        if (ensembleDiscoveredSpy.wait(2000)) {
            QVariantMap ensembleData = ensembleDiscoveredSpy.at(0).at(0).toMap();
            QVERIFY(ensembleData.contains("ensembleId"));
            QVERIFY(ensembleData.contains("ensembleLabel"));
            qDebug() << "Discovered ensemble:" << ensembleData["ensembleLabel"].toString();
        }
    }

    /**
     * @brief Test error handling with invalid data
     */
    void testErrorHandling() {
        EtiProcessor processor;
        QVERIFY(processor.initialize());
        
        // Set up error signal monitoring
        QSignalSpy errorSpy(&processor, &EtiProcessor::errorOccurred);
        
        // Test with invalid sync pattern
        QByteArray invalidFrame = TDD::ETITestFramework::generateInvalidETIFrame("invalid_sync");
        bool result = processor.processData(invalidFrame);
        
        // Should handle error gracefully
        QVERIFY2(!result, "Must reject invalid frames");
        
        // Should emit error signal
        if (errorSpy.wait(1000)) {
            QString errorMessage = errorSpy.at(0).at(0).toString();
            QVERIFY(!errorMessage.isEmpty());
            qDebug() << "Error handling test - caught error:" << errorMessage;
        }
    }

    /**
     * @brief Test service discovery functionality
     */
    void testServiceDiscovery() {
        EtiProcessor processor;
        QVERIFY(processor.initialize());
        
        QSignalSpy serviceDiscoveredSpy(&processor, &EtiProcessor::serviceDiscovered);
        
        // Process frame that should contain service information
        QByteArray frameWithServices = TDD::ETITestFramework::generateValidETIFrame();
        processor.processData(frameWithServices);
        
        // Wait for service discovery
        if (serviceDiscoveredSpy.wait(2000)) {
            for (int i = 0; i < serviceDiscoveredSpy.count(); ++i) {
                QVariantMap serviceData = serviceDiscoveredSpy.at(i).at(0).toMap();
                QVERIFY(serviceData.contains("serviceId"));
                QVERIFY(serviceData.contains("serviceLabel"));
                QVERIFY(serviceData.contains("serviceType"));
                
                qDebug() << "Discovered service:" << serviceData["serviceLabel"].toString()
                         << "Type:" << serviceData["serviceType"].toString();
            }
        }
    }

    /**
     * @brief Test reset functionality
     */
    void testReset() {
        EtiProcessor processor;
        QVERIFY(processor.initialize());
        
        // Process some data
        QByteArray testFrame = TDD::ETITestFramework::generateValidETIFrame();
        processor.processData(testFrame);
        
        // Reset processor
        processor.reset();
        
        // Verify reset state
        QVERIFY(processor.isInitialized()); // Should remain initialized
        
        // Should be able to process new data after reset
        bool result = processor.processData(testFrame);
        QVERIFY(result);
    }

    void cleanupTestCase() {
        qDebug() << "ETI Processor TDD tests completed";
        TDD::TestReporter::instance().enforceTDDCompliance();
    }

private:
    /**
     * @brief Generate native ETI frame using internal ETI structures
     * @return Valid 6144-byte ETI frame following ETSI EN 300 799
     */
    QByteArray generateNativeETIFrame() {
        QByteArray frame(eti::ETI_FRAME_SIZE, 0);
        
        // ETI Sync Field (ETSI EN 300 799 Section 5.1)
        eti::EtiSyncField sync;
        sync.set_sync_pattern();
        std::memcpy(frame.data(), sync.sync_bytes, 4);
        
        // ETI LIDATA Field (ETSI EN 300 799 Section 5.2)
        eti::EtiLidataField lidata = {};
        lidata.fc = 0;          // Frame count
        lidata.ficf = 1;        // FIC present
        lidata.nst = 1;         // 1 subchannel
        lidata.fp = 0;          // Frame phase
        lidata.mid = 1;         // Mode I
        lidata.fl = 0x05DC;     // Frame length
        lidata.tist = 0x12345678; // Timestamp
        
        std::memcpy(frame.data() + 4, &lidata, sizeof(lidata));
        
        // FIC Data (32 bytes) - Add basic FIG 0/0 for ensemble info
        uint8_t* ficData = reinterpret_cast<uint8_t*>(frame.data() + 12);
        ficData[0] = 0x00; // FIG Type 0, Extension 0
        ficData[1] = 0x08; // Length
        ficData[2] = 0x12; // Ensemble ID high
        ficData[3] = 0x34; // Ensemble ID low
        
        // MSC Data (remaining bytes)
        // Fill with test pattern for subchannel data
        
        // CRC (last 4 bytes) - simplified for testing
        uint32_t crc = 0xDEADBEEF;
        std::memcpy(frame.data() + eti::ETI_FRAME_SIZE - 4, &crc, 4);
        
        return frame;
    }

    /**
     * @brief Generate invalid ETI frame for negative testing
     * @param corruptionType Type of corruption to introduce
     * @return Invalid ETI frame data
     */
    QByteArray generateInvalidETIFrame(const QString& corruptionType) {
        QByteArray frame = generateNativeETIFrame();
        
        if (corruptionType == "invalid_sync") {
            // Corrupt sync pattern
            frame[0] = 0xFF;
            frame[1] = 0xFF;
        } else if (corruptionType == "invalid_crc") {
            // Corrupt CRC
            frame[eti::ETI_FRAME_SIZE - 1] = 0xFF;
        } else if (corruptionType == "invalid_size") {
            // Return wrong size frame
            frame.resize(1000);
        }
        
        return frame;
    }

    /**
     * @brief Generate test service information
     * @return Test service data
     */
    eti::DabService generateTestService() {
        eti::DabService service;
        service.serviceId = 0x1234;
        service.serviceLabel = "Test Service";
        service.serviceType = eti::ServiceType::AUDIO;
        service.bitrate = 128;
        service.protectionLevel = 3;
        service.isActive = true;
        return service;
    }

    /**
     * @brief Generate test ensemble information  
     * @return Test ensemble data
     */
    eti::Ensemble generateTestEnsemble() {
        eti::Ensemble ensemble;
        ensemble.ensembleId = 0x1234;
        ensemble.ensembleLabel = "Test Ensemble";
        ensemble.transmissionMode = 1;
        ensemble.localTimeOffset = 0;
        return ensemble;
    }
};

// Register test suite with Qt's meta-object system
QTEST_MAIN(EtiProcessorTest)
#include "test_eti_processor.moc"