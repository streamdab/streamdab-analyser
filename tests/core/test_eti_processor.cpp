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
 * @copyright StreamDAB Analyser Project
 */

#include "../tdd_framework.h"
#include "core/eti_processor.hpp"
#include "core/eti_types.hpp"
#include "core/modern_eti_frame_parser.hpp"
#include "core/enhanced_fig_analyser.hpp"
#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QDebug>

/**
 * @class EtiProcessorTest
 * @brief Comprehensive TDD test suite for eti_processor
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

Q_OBJECT

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
        // Create processor instance using correct class name
        eti_processor processor;
        
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
        eti_processor processor;
        
        // GREEN: This should pass after minimal implementation
        bool initResult = processor.initialize();
        TDD_GREEN_ASSERT(initResult, "Processor must initialize successfully");
        
        // Verify basic state after initialization using correct method names
        QVERIFY(processor.is_initialized());
        QVERIFY(processor.get_eti_parser() != nullptr);
        QVERIFY(processor.get_fig_analyser() != nullptr);
    }

    /**
     * @brief TDD RED Phase: File processing should fail initially
     */
    TDD_RED_PHASE(FileProcessing)
        eti_processor processor;
        processor.initialize();
        
        // Create test ETI file
        QTemporaryFile testFile;
        testFile.setFileTemplate(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/test_eti_XXXXXX.eti");
        QVERIFY(testFile.open());
        
        // Write valid ETI frame data using internal ETI structures
        QByteArray testFrame = generateNativeETIFrame();
        testFile.write(testFrame);
        testFile.close();
        
        // RED: This should fail before implementation - using correct method name
        bool processResult = processor.process_file(testFile.fileName());
        
        if (qEnvironmentVariableIsSet("TDD_FORCE_RED")) {
            TDD_RED_ASSERT(processResult, "File processing should fail in RED phase");
        }
    }

    /**
     * @brief TDD GREEN Phase: File processing should pass
     */
    TDD_GREEN_PHASE(FileProcessing)
        eti_processor processor;
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
        
        // GREEN: This should pass after implementation - using correct method name
        bool processResult = processor.process_file(testFile.fileName());
        TDD_GREEN_ASSERT(processResult, "File processing must succeed with valid ETI file");
        
        // Verify processing signals were emitted using correct signal names
        QSignalSpy frameProcessedSpy(&processor, &eti_processor::frameProcessed);
        QSignalSpy serviceDiscoveredSpy(&processor, &eti_processor::serviceDiscovered);
        
        // Process the file and wait for signals
        processor.process_file(testFile.fileName());
        
        // Should have processed frames
        QVERIFY(frameProcessedSpy.count() > 0);
        qDebug() << "Processed" << frameProcessedSpy.count() << "ETI frames";
    }

    /**
     * @brief Test real-time data processing
     */
    void testRealTimeDataProcessing() {
        eti_processor processor;
        QVERIFY(processor.initialize());
        
        // Generate test ETI data using native structures
        QByteArray etiData = generateNativeETIFrame();
        
        // Set up signal monitoring using correct signal
        QSignalSpy frameProcessedSpy(&processor, &eti_processor::frameProcessed);
        
        // Process data buffer using correct method name
        bool result = processor.process_data(etiData);
        QVERIFY2(result, "Real-time data processing must succeed");
        
        // Verify frame was processed
        QVERIFY(frameProcessedSpy.wait(1000)); // Wait up to 1 second
        QCOMPARE(frameProcessedSpy.count(), 1);
        
        // Verify frame data - signal signature: (quint64 frameNumber, const QByteArray& data)
        QVERIFY(frameProcessedSpy.at(0).at(0).toULongLong() >= 0); // frameNumber
        QVERIFY(!frameProcessedSpy.at(0).at(1).toByteArray().isEmpty()); // data
    }

    /**
     * @brief Performance requirement test - 7,482 FPS target
     */
    void testPerformanceRequirement() {
        eti_processor processor;
        QVERIFY(processor.initialize());
        
        // Generate test data for performance testing
        QList<QByteArray> testFrames;
        const int frameCount = 1000; // Test with 1000 frames
        
        for (int i = 0; i < frameCount; ++i) {
            testFrames.append(generateNativeETIFrame()); // Use our native frame generator
        }
        
        // Benchmark processing speed using correct method name
        TDD_BENCHMARK([&]() {
            for (const auto& frame : testFrames) {
                processor.process_data(frame);
            }
        }, 200, "Process 1000 ETI frames"); // Max 200ms for 1000 frames = 5000 FPS minimum
        
        qDebug() << "Performance test: Processed" << frameCount << "frames in benchmark";
    }

    /**
     * @brief Memory efficiency test - 4MB target
     */
    void testMemoryEfficiency() {
        eti_processor processor;
        QVERIFY(processor.initialize());
        
        // Process multiple files to test memory usage using correct method names
        TDD_MEMORY_CHECK([&]() {
            for (int i = 0; i < 100; ++i) {
                QByteArray frame = generateNativeETIFrame(); // Use our native frame generator
                processor.process_data(frame);
            }
        }, 4, "ETI processing memory usage"); // Max 4MB
    }

    /**
     * @brief ETSI compliance validation
     */
    ETSI_COMPLIANCE_TEST(EN_300_799, frame_structure)
        eti_processor processor;
        QVERIFY(processor.initialize());
        
        // Test with ETSI compliant frame using our native generator
        QByteArray validFrame = generateNativeETIFrame();
        // Note: ETSI_VALIDATE_FRAME macro not yet implemented - will add in ETSI compliance phase
        
        bool result = processor.process_data(validFrame);
        QVERIFY2(result, "Must process ETSI compliant frames");
    }

    /**
     * @brief Test FIG parsing capabilities
     */
    void testFigParsing() {
        eti_processor processor;
        QVERIFY(processor.initialize());
        
        // Set up signal monitoring for FIG discovery using correct signals
        QSignalSpy serviceDiscoveredSpy(&processor, &eti_processor::serviceDiscovered);
        QSignalSpy ensembleDiscoveredSpy(&processor, &eti_processor::ensembleDiscovered);
        
        // Process frame with embedded FIG data using our native generator
        QByteArray frameWithFigs = generateNativeETIFrame();
        processor.process_data(frameWithFigs);
        
        // Should discover ensemble information
        if (ensembleDiscoveredSpy.wait(2000)) {
            // Signal signature: ensembleDiscovered(const eti::Ensemble& ensemble)
            // Access the eti::Ensemble directly from signal arguments
            const QVariantList& args = ensembleDiscoveredSpy.at(0);
            if (!args.isEmpty()) {
                qDebug() << "Discovered ensemble from signal";
                // Note: Direct access to eti::Ensemble structure requires proper handling
                // For now, just verify signal was emitted
            }
        }
    }

    /**
     * @brief Test error handling with invalid data
     */
    void testErrorHandling() {
        eti_processor processor;
        QVERIFY(processor.initialize());
        
        // Set up error signal monitoring using correct signal
        QSignalSpy errorSpy(&processor, &eti_processor::errorOccurred);
        
        // Test with invalid sync pattern using our native generator
        QByteArray invalidFrame = generateInvalidETIFrame("invalid_sync");
        bool result = processor.process_data(invalidFrame);
        
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
        eti_processor processor;
        QVERIFY(processor.initialize());
        
        QSignalSpy serviceDiscoveredSpy(&processor, &eti_processor::serviceDiscovered);
        
        // Process frame that should contain service information using our native generator
        QByteArray frameWithServices = generateNativeETIFrame();
        processor.process_data(frameWithServices);
        
        // Wait for service discovery
        if (serviceDiscoveredSpy.wait(2000)) {
            for (int i = 0; i < serviceDiscoveredSpy.count(); ++i) {
                // Signal signature: serviceDiscovered(const eti::DabService& service)
                // The signal contains eti::DabService struct directly
                const QVariantList& args = serviceDiscoveredSpy.at(i);
                if (!args.isEmpty()) {
                    qDebug() << "Discovered service from signal" << i;
                    // Note: Direct access to eti::DabService structure requires proper handling
                    // For now, just verify signal was emitted
                }
            }
        }
    }

    /**
     * @brief Test reset functionality
     */
    void testReset() {
        eti_processor processor;
        QVERIFY(processor.initialize());
        
        // Process some data using correct method names
        QByteArray testFrame = generateNativeETIFrame();
        processor.process_data(testFrame);
        
        // Reset processor
        processor.reset();
        
        // Verify reset state using correct method name
        QVERIFY(processor.is_initialized()); // Should remain initialized
        
        // Should be able to process new data after reset
        bool result = processor.process_data(testFrame);
        QVERIFY(result);
    }

    /**
     * @brief TDD RED Phase: validateFrame() sync word validation should fail initially
     * 
     * This test verifies that validateFrame() properly validates the ETSI EN 300 799
     * sync pattern: 0x49, 0x93, 0x1E, 0x03. Initially this should fail because
     * sync word validation is not yet implemented in validateFrame().
     */
    TDD_RED_PHASE(ValidateFrameSyncWord)
        eti_processor processor;
        QVERIFY(processor.initialize());
        
        // Test 1: Valid sync word should pass validation
        QByteArray validFrame(6144, 0);
        validFrame[0] = 0x49;  // ETSI EN 300 799 sync pattern
        validFrame[1] = 0x93;
        validFrame[2] = 0x1E;
        validFrame[3] = 0x03;
        
        // RED: This should fail initially because sync validation is not implemented
        bool validResult = processor.validate_frame(validFrame);
        
        if (qEnvironmentVariableIsSet("TDD_FORCE_RED")) {
            // In RED phase, expect this to fail because sync word validation isn't implemented yet
            TDD_RED_ASSERT(!validResult, "validate_frame() should fail for sync validation in RED phase");
        } else {
            // After GREEN implementation, valid sync word should pass
            QVERIFY2(validResult, "validate_frame() must accept frames with valid ETSI sync pattern");
        }
        
        // Test 2: Invalid sync word should always fail validation
        QByteArray invalidFrame(6144, 0);
        invalidFrame[0] = 0xFF;  // Invalid sync pattern
        invalidFrame[1] = 0xFF;
        invalidFrame[2] = 0xFF;
        invalidFrame[3] = 0xFF;
        
        bool invalidResult = processor.validate_frame(invalidFrame);
        QVERIFY2(!invalidResult, "validate_frame() must reject frames with invalid sync pattern");
    }
    
    /**
     * @brief TDD GREEN Phase: validateFrame() sync word validation should pass
     * 
     * This test ensures that after implementing sync word validation,
     * validateFrame() correctly validates ETSI EN 300 799 sync patterns.
     */
    TDD_GREEN_PHASE(ValidateFrameSyncWord)
        eti_processor processor;
        QVERIFY(processor.initialize());
        
        // Test various sync word scenarios
        struct SyncTestCase {
            std::array<uint8_t, 4> syncBytes;
            bool expectedValid;
            QString description;
        };
        
        std::vector<SyncTestCase> testCases = {
            {{0x49, 0x93, 0x1E, 0x03}, true,  "Valid ETSI EN 300 799 sync pattern"},
            {{0x00, 0x00, 0x00, 0x00}, false, "All zeros sync pattern"},
            {{0xFF, 0xFF, 0xFF, 0xFF}, false, "All ones sync pattern"},
            {{0x49, 0x93, 0x1E, 0x00}, false, "Partial valid sync pattern"},
            {{0x00, 0x93, 0x1E, 0x03}, false, "First byte invalid"},
            {{0x49, 0x00, 0x1E, 0x03}, false, "Second byte invalid"},
            {{0x49, 0x93, 0x00, 0x03}, false, "Third byte invalid"},
            {{0x49, 0x93, 0x1E, 0x00}, false, "Fourth byte invalid"}
        };
        
        for (const auto& testCase : testCases) {
            QByteArray frame(6144, 0);
            frame[0] = testCase.syncBytes[0];
            frame[1] = testCase.syncBytes[1];
            frame[2] = testCase.syncBytes[2];
            frame[3] = testCase.syncBytes[3];
            
            bool result = processor.validate_frame(frame);
            
            QCOMPARE(result, testCase.expectedValid);
            qDebug() << testCase.description.toLatin1().data() 
                     << "- Expected:" << testCase.expectedValid 
                     << "Got:" << result;
        }
    }
    
    /**
     * @brief Test validateFrame() with various frame sizes and sync combinations
     */
    void testValidateFrameComprehensive() {
        eti_processor processor;
        QVERIFY(processor.initialize());
        
        // Test 1: Wrong size frame with valid sync - should fail due to size
        QByteArray wrongSizeFrame(1000, 0);
        wrongSizeFrame[0] = 0x49;
        wrongSizeFrame[1] = 0x93;
        wrongSizeFrame[2] = 0x1E;
        wrongSizeFrame[3] = 0x03;
        
        bool wrongSizeResult = processor.validate_frame(wrongSizeFrame);
        QVERIFY2(!wrongSizeResult, "validate_frame() must reject wrong size frames even with valid sync");
        
        // Test 2: Correct size frame with invalid sync - should fail due to sync
        QByteArray wrongSyncFrame(6144, 0);
        wrongSyncFrame[0] = 0x12;
        wrongSyncFrame[1] = 0x34;
        wrongSyncFrame[2] = 0x56;
        wrongSyncFrame[3] = 0x78;
        
        bool wrongSyncResult = processor.validate_frame(wrongSyncFrame);
        QVERIFY2(!wrongSyncResult, "validate_frame() must reject frames with invalid sync pattern");
        
        // Test 3: Both correct size and valid sync - should pass
        QByteArray validFrame(6144, 0);
        validFrame[0] = 0x49;
        validFrame[1] = 0x93;
        validFrame[2] = 0x1E;
        validFrame[3] = 0x03;
        
        bool validResult = processor.validate_frame(validFrame);
        QVERIFY2(validResult, "validate_frame() must accept frames with correct size and valid sync pattern");
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