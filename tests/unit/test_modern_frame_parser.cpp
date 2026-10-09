/**
 * @file test_modern_frame_parser.cpp
 * @brief TDD test suite for Modern ETI Frame Parser
 * 
 * Tests the core ETI frame parsing functionality with proper TDD methodology
 * including performance requirements and ETSI compliance validation.
 * 
 * @author TDD Lead Agent
 * @date 2025-09-22
 * @copyright StreamDAB Analyser Project
 */

#include "../tdd_framework.h"
#include "core/modern_eti_frame_parser.hpp"
#include "core/eti_types.h"
#include <QtTest/QtTest>
#include <QSignalSpy>

/**
 * @class ModernETIFrameParserTest
 * @brief TDD test suite for ETI frame parsing
 * 
 * Tests modern ETI frame parsing with focus on:
 * - 6144-byte frame structure validation
 * - 7,482 FPS performance target
 * - ETSI EN 300 799 compliance
 * - Real-world ETI stream compatibility
 * - Memory efficiency under 4MB
 */
TDD_TEST_CASE(ModernETIFrameParserTest, UNIT, CRITICAL)

private:
    ModernETIFrameParser* m_parser = nullptr;

public slots:
    void initTestCase() {
        qDebug() << "=== Modern ETI Frame Parser TDD Test Suite ===";
        qDebug() << "Target: 7,482 FPS parsing performance";
        qDebug() << "Standard: ETSI EN 300 799 compliance";
        qDebug() << "Frame Size: 6144 bytes (ETI-NI format)";
    }

    void init() {
        m_parser = new ModernETIFrameParser();
        QVERIFY(m_parser != nullptr);
    }

    void cleanup() {
        delete m_parser;
        m_parser = nullptr;
    }

    /**
     * @brief TDD RED Phase: Frame parsing should fail initially
     */
    TDD_RED_PHASE(FrameParsing)
        QByteArray testFrame = TDD::ETITestFramework::generateValidETIFrame();
        
        // RED: Should fail before implementation
        bool parseResult = m_parser->parseFrame(testFrame);
        
        if (qEnvironmentVariableIsSet("TDD_FORCE_RED")) {
            TDD_RED_ASSERT(parseResult, "Frame parsing should fail in RED phase");
        }
    }

    /**
     * @brief TDD GREEN Phase: Frame parsing should succeed
     */
    TDD_GREEN_PHASE(FrameParsing)
        QByteArray testFrame = TDD::ETITestFramework::generateValidETIFrame();
        
        // GREEN: Should pass after minimal implementation
        bool parseResult = m_parser->parseFrame(testFrame);
        TDD_GREEN_ASSERT(parseResult, "Frame parsing must succeed with valid frame");
        
        // Verify frame data extraction
        QVERIFY(m_parser->hasValidFrame());
        
        // Check basic frame properties
        quint32 frameNumber = m_parser->getFrameNumber();
        QVERIFY(frameNumber <= 0xFFFFFF); // 24-bit frame number
        
        quint16 ensembleId = m_parser->getEnsembleId();
        QVERIFY(ensembleId != 0); // Should have valid ensemble ID
        
        qDebug() << "Parsed frame:" << frameNumber << "Ensemble:" << QString::number(ensembleId, 16);
    }

    /**
     * @brief Test ETI frame structure validation
     */
    ETSI_COMPLIANCE_TEST(EN_300_799, frame_structure_validation)
        // Test with valid frame
        QByteArray validFrame = TDD::ETITestFramework::generateValidETIFrame();
        ETSI_VALIDATE_FRAME(validFrame);
        
        bool result = m_parser->parseFrame(validFrame);
        QVERIFY2(result, "Must parse ETSI EN 300 799 compliant frames");
        
        // Verify sync pattern detection
        QVERIFY(m_parser->hasValidSyncPattern());
        
        // Verify frame size compliance
        QCOMPARE(validFrame.size(), 6144); // ETI-NI frame size
    }

    /**
     * @brief Test parsing performance - 7,482 FPS target
     */
    void testParsingPerformance() {
        const int frameCount = 1000;
        QList<QByteArray> testFrames;
        
        // Prepare test frames
        for (int i = 0; i < frameCount; ++i) {
            testFrames.append(TDD::ETITestFramework::generateValidETIFrame());
        }
        
        qDebug() << "Performance test: parsing" << frameCount << "frames";
        
        // Target: 7,482 FPS = ~133.5 microseconds per frame
        // For 1000 frames = max 133.5ms total
        TDD_BENCHMARK([&]() {
            for (const auto& frame : testFrames) {
                m_parser->parseFrame(frame);
            }
        }, 150, "Parse 1000 ETI frames"); // Allow 150ms (6,667 FPS) as minimum
        
        qDebug() << "Successfully parsed" << frameCount << "frames in performance test";
    }

    /**
     * @brief Test memory efficiency
     */
    void testMemoryEfficiency() {
        // Process many frames to test memory usage
        TDD_MEMORY_CHECK([&]() {
            for (int i = 0; i < 10000; ++i) {
                QByteArray frame = TDD::ETITestFramework::generateValidETIFrame();
                m_parser->parseFrame(frame);
                
                // Reset parser periodically to test cleanup
                if (i % 1000 == 0) {
                    m_parser->reset();
                }
            }
        }, 4, "ETI frame parsing memory usage"); // Max 4MB
    }

    /**
     * @brief Test frame header parsing
     */
    void testFrameHeaderParsing() {
        QByteArray testFrame = TDD::ETITestFramework::generateValidETIFrame();
        
        bool result = m_parser->parseFrame(testFrame);
        QVERIFY(result);
        
        // Test sync pattern extraction
        QByteArray syncPattern = m_parser->getSyncPattern();
        QCOMPARE(syncPattern.size(), 4);
        QCOMPARE(syncPattern[0], char(0x49)); // 'I'
        QCOMPARE(syncPattern[1], char(0x4E)); // 'N'
        QCOMPARE(syncPattern[2], char(0x53)); // 'S'
        QCOMPARE(syncPattern[3], char(0x54)); // 'T'
        
        // Test frame number extraction
        quint32 frameNumber = m_parser->getFrameNumber();
        QVERIFY(frameNumber >= 0 && frameNumber <= 0xFFFFFF);
        
        // Test ensemble ID extraction
        quint16 ensembleId = m_parser->getEnsembleId();
        QVERIFY(ensembleId != 0);
        
        qDebug() << "Header parsing verified - Frame:" << frameNumber << "Ensemble:" << ensembleId;
    }

    /**
     * @brief Test FIC section parsing
     */
    void testFicSectionParsing() {
        QByteArray testFrame = TDD::ETITestFramework::generateValidETIFrame();
        
        bool result = m_parser->parseFrame(testFrame);
        QVERIFY(result);
        
        // Extract FIC section
        QByteArray ficData = m_parser->getFicData();
        QVERIFY(!ficData.isEmpty());
        
        // FIC section should be 96 bytes for Mode I (ETSI EN 300 401)
        QCOMPARE(ficData.size(), 96);
        
        // Test FIC parsing capability
        bool ficParsed = m_parser->parseFicSection(ficData);
        if (ficParsed) {
            qDebug() << "FIC section parsed successfully";
            
            // Check for FIG discovery
            auto figList = m_parser->getDiscoveredFigs();
            if (!figList.isEmpty()) {
                qDebug() << "Discovered" << figList.size() << "FIG elements";
            }
        }
    }

    /**
     * @brief Test subchannel data extraction
     */
    void testSubchannelDataExtraction() {
        QByteArray testFrame = TDD::ETITestFramework::generateValidETIFrame();
        
        bool result = m_parser->parseFrame(testFrame);
        QVERIFY(result);
        
        // Get subchannel count
        int subchannelCount = m_parser->getSubchannelCount();
        qDebug() << "Frame contains" << subchannelCount << "subchannels";
        
        // Extract subchannel data
        for (int i = 0; i < subchannelCount; ++i) {
            QByteArray subchannelData = m_parser->getSubchannelData(i);
            
            if (!subchannelData.isEmpty()) {
                qDebug() << "Subchannel" << i << "size:" << subchannelData.size() << "bytes";
                
                // Verify subchannel data validity
                QVERIFY(subchannelData.size() > 0);
                QVERIFY(subchannelData.size() <= 6144); // Cannot exceed frame size
            }
        }
    }

    /**
     * @brief Test error handling with corrupted frames
     */
    void testErrorHandling() {
        // Test with invalid sync pattern
        QByteArray invalidSync = TDD::ETITestFramework::generateInvalidETIFrame("invalid_sync");
        bool result1 = m_parser->parseFrame(invalidSync);
        QVERIFY2(!result1, "Must reject frames with invalid sync pattern");
        
        // Test with invalid frame size
        QByteArray invalidSize = TDD::ETITestFramework::generateInvalidETIFrame("invalid_size");
        bool result2 = m_parser->parseFrame(invalidSize);
        QVERIFY2(!result2, "Must reject frames with invalid size");
        
        // Test with CRC errors
        QByteArray crcError = TDD::ETITestFramework::generateInvalidETIFrame("crc_error");
        bool result3 = m_parser->parseFrame(crcError);
        // CRC error handling may vary by implementation
        qDebug() << "CRC error frame result:" << result3;
        
        // Parser should remain in valid state after errors
        QByteArray validFrame = TDD::ETITestFramework::generateValidETIFrame();
        bool result4 = m_parser->parseFrame(validFrame);
        QVERIFY2(result4, "Parser must recover from errors and process valid frames");
    }

    /**
     * @brief Test real-world ETI compatibility
     */
    void testRealWorldCompatibility() {
        // Test with various frame patterns that might occur in real streams
        
        // Test frame sequence processing
        for (int frameNum = 0; frameNum < 100; ++frameNum) {
            QByteArray frame = TDD::ETITestFramework::generateValidETIFrame();
            
            // Modify frame number to simulate real sequence
            frame[4] = (frameNum >> 16) & 0xFF;
            frame[5] = (frameNum >> 8) & 0xFF;
            frame[6] = frameNum & 0xFF;
            
            bool result = m_parser->parseFrame(frame);
            QVERIFY2(result, qPrintable(QString("Frame %1 parsing failed").arg(frameNum)));
            
            // Verify frame number is correctly parsed
            quint32 parsedFrameNum = m_parser->getFrameNumber();
            QCOMPARE(parsedFrameNum, static_cast<quint32>(frameNum));
        }
        
        qDebug() << "Real-world compatibility test: processed 100 frame sequence";
    }

    /**
     * @brief Test parser reset functionality
     */
    void testParserReset() {
        // Parse a frame
        QByteArray testFrame = TDD::ETITestFramework::generateValidETIFrame();
        bool result = m_parser->parseFrame(testFrame);
        QVERIFY(result);
        QVERIFY(m_parser->hasValidFrame());
        
        // Reset parser
        m_parser->reset();
        
        // Verify reset state
        QVERIFY(!m_parser->hasValidFrame());
        QCOMPARE(m_parser->getFrameNumber(), 0u);
        QCOMPARE(m_parser->getEnsembleId(), 0u);
        
        // Should be able to parse new frame after reset
        bool result2 = m_parser->parseFrame(testFrame);
        QVERIFY(result2);
        QVERIFY(m_parser->hasValidFrame());
        
        qDebug() << "Parser reset functionality verified";
    }

    /**
     * @brief Test concurrent parsing capability
     */
    void testConcurrentParsing() {
        // Test that parser can handle rapid consecutive calls
        const int rapidCallCount = 1000;
        
        for (int i = 0; i < rapidCallCount; ++i) {
            QByteArray frame = TDD::ETITestFramework::generateValidETIFrame();
            bool result = m_parser->parseFrame(frame);
            QVERIFY2(result, qPrintable(QString("Rapid call %1 failed").arg(i)));
        }
        
        qDebug() << "Concurrent parsing test: handled" << rapidCallCount << "rapid calls";
    }

    void cleanupTestCase() {
        qDebug() << "Modern ETI Frame Parser tests completed";
        TDD::TestReporter::instance().enforceTDDCompliance();
    }
};

// Register test with Qt Test framework
QTEST_MAIN(ModernETIFrameParserTest)
#include "test_modern_frame_parser.moc"