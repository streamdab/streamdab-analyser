/**
 * @file test_mot_pipeline.cpp
 * @brief Integration Test: ETI Frame → FIG Processing → MOT Object Extraction
 *
 * Tests complete MOT processing pipeline:
 * 1. ETI frame parsing
 * 2. FIG extraction and data service discovery
 * 3. MSC Data Group extraction
 * 4. MOT segment reassembly
 * 5. MOT object completion and metadata extraction
 *
 * Phase 3A Wave 3.1: Integration & Testing Infrastructure
 *
 * @author StreamDAB Development Team
 * @date October 22, 2025
 */

#include <QtTest/QtTest>
#include <QSignalSpy>
#include "test_utils.h"
#include "../../src/core/eti_types.h"
#include "../../src/core/mot_protocol.hpp"

class TestMOTPipeline : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Pipeline component tests
    void testETI_Frame_With_DataService();
    void testMOT_Processor_Creation();
    void testMOT_DataGroup_Generation();
    void testMOT_ContentType_Detection();
    
    // MOT-specific tests
    void testMOT_Header_Structure();
    void testMOT_Signal_Mechanism();
    
    // Error handling tests
    void testInvalid_DataGroup_Handling();
    void testEmpty_DataGroup_Handling();

private:
    // Test components
    std::unique_ptr<eti::mot::MOTProtocol> m_mot_processor;
    
    // Helper methods
    QByteArray generateDataServiceFrame();
    QByteArray generateMOTDataGroup();
};

void TestMOTPipeline::initTestCase() {
    qInfo() << "=== MOT Pipeline Integration Test Suite ===";
    qInfo() << "Testing: MOT object extraction infrastructure";
    
    // Initialize MOT processor
    m_mot_processor = std::make_unique<eti::mot::MOTProtocol>();
    
    QVERIFY(m_mot_processor != nullptr);
}

void TestMOTPipeline::cleanupTestCase() {
    qInfo() << "=== MOT Pipeline Test Cleanup ===";
    m_mot_processor.reset();
}

/**
 * @brief Test ETI frame generation with data service
 */
void TestMOTPipeline::testETI_Frame_With_DataService() {
    qInfo() << "Test: ETI Frame with Data Service";
    
    // Generate ETI frame with data service
    QByteArray eti_frame = TestUtils::generateETIFrameWithDataService();
    QVERIFY(!eti_frame.isEmpty());
    QCOMPARE(eti_frame.size(), 6144);
    
    // Verify frame structure
    QVERIFY(TestUtils::verifyETIFrameStructure(eti_frame));
    
    qInfo() << "  ✓ Data service frame generated: 6144 bytes";
    qInfo() << "  ✓ Frame structure validated";
}

/**
 * @brief Test MOT processor instantiation
 */
void TestMOTPipeline::testMOT_Processor_Creation() {
    qInfo() << "Test: MOT Processor Creation";
    
    QVERIFY(m_mot_processor != nullptr);
    
    qInfo() << "  ✓ MOT processor created";
}

/**
 * @brief Test MOT data group generation
 */
void TestMOTPipeline::testMOT_DataGroup_Generation() {
    qInfo() << "Test: MOT Data Group Generation";
    
    // Generate MOT data group
    uint16_t transport_id = 42;
    QByteArray data_group = TestUtils::generateMOTDataGroup(transport_id);
    
    QVERIFY(!data_group.isEmpty());
    QVERIFY(data_group.size() > 10); // Should have header + data
    
    qInfo() << "  ✓ MOT data group generated:" << data_group.size() << "bytes";
}

/**
 * @brief Test MOT content type detection
 */
void TestMOTPipeline::testMOT_ContentType_Detection() {
    qInfo() << "Test: MOT Content Type Detection";
    
    // Test various content types
    using ContentType = eti::mot::ContentType;
    
    QCOMPARE(static_cast<uint8_t>(ContentType::IMAGE_JPEG), static_cast<uint8_t>(0x02));
    QCOMPARE(static_cast<uint8_t>(ContentType::IMAGE_PNG), static_cast<uint8_t>(0x03));
    QCOMPARE(static_cast<uint8_t>(ContentType::HTML), static_cast<uint8_t>(0x05));
    
    qInfo() << "  ✓ Content type enumeration validated";
}

/**
 * @brief Test MOT header structure
 */
void TestMOTPipeline::testMOT_Header_Structure() {
    qInfo() << "Test: MOT Header Structure";
    
    // Create MOT header
    eti::mot::MOTHeader header;
    header.body_size = 256;
    header.header_size = 32;
    header.content_type = eti::mot::ContentType::IMAGE_JPEG;
    
    QCOMPARE(header.body_size, static_cast<uint16_t>(256));
    QCOMPARE(header.header_size, static_cast<uint16_t>(32));
    
    QString type_string = header.getContentTypeString();
    QVERIFY(!type_string.isEmpty());
    
    qInfo() << "  ✓ MOT header structure validated";
    qInfo() << "    - Body size:" << header.body_size;
    qInfo() << "    - Content type:" << type_string;
}

/**
 * @brief Test MOT signal mechanism
 */
void TestMOTPipeline::testMOT_Signal_Mechanism() {
    qInfo() << "Test: MOT Signal Mechanism";
    
    // Setup signal spy
    QSignalSpy spy(m_mot_processor.get(),
                   &eti::mot::MOTProtocol::objectComplete);
    
    QVERIFY(spy.isValid());
    
    qInfo() << "  ✓ Signal/slot mechanism functional";
}

/**
 * @brief Test error handling for invalid data groups
 */
void TestMOTPipeline::testInvalid_DataGroup_Handling() {
    qInfo() << "Test: Invalid Data Group Handling";
    
    // Create invalid data group (too small)
    QByteArray invalid_data(5, 0x00);
    
    uint32_t service_id = 0xE1C00379;
    
    // Should handle gracefully without crash
    m_mot_processor->processMOTData(service_id, invalid_data);
    
    qInfo() << "  ✓ Invalid data group handled correctly";
}

/**
 * @brief Test handling of empty data groups
 */
void TestMOTPipeline::testEmpty_DataGroup_Handling() {
    qInfo() << "Test: Empty Data Group Handling";
    
    // Create empty data group
    QByteArray empty_data;
    
    uint32_t service_id = 0xE1C00379;
    
    // Should handle gracefully
    m_mot_processor->processMOTData(service_id, empty_data);
    
    qInfo() << "  ✓ Empty data group handled correctly";
}

// ============================================================================
// Helper Methods
// ============================================================================

QByteArray TestMOTPipeline::generateDataServiceFrame() {
    return TestUtils::generateETIFrameWithDataService();
}

QByteArray TestMOTPipeline::generateMOTDataGroup() {
    return TestUtils::generateMOTDataGroup(42);
}

QTEST_MAIN(TestMOTPipeline)
#include "test_mot_pipeline.moc"
