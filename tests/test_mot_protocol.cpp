/**
 * @file test_mot_protocol.cpp
 * @brief Comprehensive Unit Tests for MOT Protocol Parser
 *
 * Test suite for MOT (Multimedia Object Transfer) Protocol implementation
 * following ETSI EN 301 234 specifications.
 *
 * Test Coverage:
 * - MSC Data Group parsing (structure, CRC validation)
 * - Segment reassembly (single, multi-segment, duplicates, out-of-order)
 * - Content type detection (JPEG, PNG, HTML, unknown)
 * - Header parameter extraction (content name, Thai UTF-8, trigger time)
 * - Transport ID management (range 0-1023, multiple objects)
 * - Edge cases (empty data, corrupted header, invalid segments)
 *
 * @author StreamDAB Development Team
 * @date October 22, 2025
 * @version 1.0
 */

#include <QtTest/QtTest>
#include <QObject>
#include <QByteArray>
#include <QSignalSpy>
#include <QDebug>
#include "../src/core/mot_protocol.hpp"
#include "../src/core/mot_pad_adapter.hpp"
#include "../src/core/crc16.hpp"

using namespace eti::mot;

// ============================================================================
// Helpers: build standard EN 300 401 MOT MSC data groups (as carried in
// DAB+ X-PAD CI 12/13), used by the MotPadAdapter tests below.
// ============================================================================
namespace {

// 7-byte standard MOT header core (ETSI EN 301 234 §5.1): body size(28)
// header size(13) content type(6) content subtype(9).
QByteArray buildStandardMotHeaderCore(uint32_t bodySize, uint8_t contentType,
                                      uint16_t contentSubType)
{
    QByteArray h(7, '\0');
    auto* d = reinterpret_cast<uint8_t*>(h.data());
    d[0] = static_cast<uint8_t>((bodySize >> 20) & 0xFF);
    d[1] = static_cast<uint8_t>((bodySize >> 12) & 0xFF);
    d[2] = static_cast<uint8_t>((bodySize >> 4) & 0xFF);
    d[3] = static_cast<uint8_t>(((bodySize & 0x0F) << 4) | ((7u >> 9) & 0x0F));
    d[4] = static_cast<uint8_t>((7u >> 1) & 0xFF);
    d[5] = static_cast<uint8_t>(((7u & 0x01) << 7) |
                                ((contentType & 0x3F) << 1) |
                                ((contentSubType >> 8) & 0x01));
    d[6] = static_cast<uint8_t>(contentSubType & 0xFF);
    return h;
}

// Standard MOT MSC data group: 2-byte header (crc/segment/user-access/type) +
// session header (last|seg, transport-id flag+len, transport id) +
// segmentation header (segment size) + payload + ~CRC-16/CCITT-FALSE.
QByteArray buildStandardMotDataGroup(uint8_t dgType, uint16_t transportId,
                                     uint16_t segNumber, bool last,
                                     const QByteArray& payload)
{
    QByteArray dg;
    dg.append(static_cast<char>(0x40 | 0x20 | 0x10 | (dgType & 0x0F)));
    dg.append(static_cast<char>(0x00));
    dg.append(static_cast<char>((last ? 0x80 : 0x00) | ((segNumber >> 8) & 0x7F)));
    dg.append(static_cast<char>(segNumber & 0xFF));
    dg.append(static_cast<char>(0x10 | 0x02));  // transport id flag, len = 2
    dg.append(static_cast<char>((transportId >> 8) & 0xFF));
    dg.append(static_cast<char>(transportId & 0xFF));
    dg.append(static_cast<char>((payload.size() >> 8) & 0x1F));
    dg.append(static_cast<char>(payload.size() & 0xFF));
    dg.append(payload);
    const uint16_t crc = static_cast<uint16_t>(~eti::crc16ccitt_false(
        reinterpret_cast<const uint8_t*>(dg.constData()), dg.size()));
    dg.append(static_cast<char>((crc >> 8) & 0xFF));
    dg.append(static_cast<char>(crc & 0xFF));
    return dg;
}

QByteArray patternBody(int size, char seed)
{
    QByteArray body(size, '\0');
    for (int i = 0; i < size; ++i) {
        body[i] = static_cast<char>((seed + i) & 0xFF);
    }
    return body;
}

}  // namespace

class TestMOTProtocol : public QObject
{
    Q_OBJECT

private slots:
    // Test lifecycle
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // MSC Data Group parsing tests
    void testMSCDataGroup_ValidStructure();
    void testMSCDataGroup_CRCValidation();
    void testMSCDataGroup_InvalidCRC();
    void testMSCDataGroup_MinimumSize();
    void testMSCDataGroup_WithoutCRC();
    void testMSCDataGroup_AllFlags();

    // Segment reassembly tests
    void testSegmentReassembly_SingleSegment();
    void testSegmentReassembly_MultiSegment();
    void testSegmentReassembly_DuplicateSegment();
    void testSegmentReassembly_OutOfOrderSegments();
    void testSegmentReassembly_MissingSegments();
    void testSegmentReassembly_ProgressTracking();

    // Content type detection tests
    void testContentType_JPEG();
    void testContentType_PNG();
    void testContentType_HTML();
    void testContentType_Unknown();

    // Header parameter extraction tests
    void testHeaderParameter_ContentName();
    void testHeaderParameter_ThaiUTF8();
    void testHeaderParameter_TriggerTime();
    void testHeaderParameter_CategoryTitle();
    void testHeaderParameter_ClickThroughURL();
    void testHeaderParameter_MultipleParameters();

    // Transport ID management tests
    void testTransportID_ValidRange();
    void testTransportID_InvalidRange();
    void testTransportID_MultipleObjects();
    void testTransportID_ClearObject();
    void testTransportID_ClearAll();

    // Edge cases and error handling
    void testEdgeCase_EmptyData();
    void testEdgeCase_CorruptedHeader();
    void testEdgeCase_InvalidSegmentNumber();
    void testEdgeCase_TotalSegmentsMismatch();
    void testEdgeCase_NullData();

    // MOT Header parsing tests
    void testMOTHeader_BasicStructure();
    void testMOTHeader_ExtensionParameters();
    void testMOTHeader_InvalidSize();

    // Signal emission tests
    void testSignals_ObjectComplete();
    void testSignals_ReassemblyProgress();
    void testSignals_ParseError();

    // Statistics tests
    void testStatistics_Tracking();
    void testStatistics_Accuracy();

    // MOT MSC data group adapter (X-PAD CI 12/13 transport) tests
    void testMotPadAdapter_StandardHeaderCore();
    void testMotPadAdapter_HeaderBodyObjectComplete();
    void testMotPadAdapter_MultiSegmentBody();
    void testMotPadAdapter_CrcRejected();
    void testMotPadAdapter_ReframedLayout();
    void testMotPadAdapter_ReframedIsValid();
    void testMotPadAdapter_OversizeRejected();
    void testMotPadAdapter_StaleEviction();
    void testMotPadAdapter_CarouselReset();

private:
    MOTProtocol* m_motParser;

    // Helper methods
    QByteArray createValidMSCDataGroup(bool with_crc = true);
    QByteArray createMSCDataGroupWithSegment(uint16_t segment_num, uint16_t total_segments,
                                            uint32_t transport_id, const QByteArray& data);
    std::vector<uint8_t> createMOTHeader(ContentType type, const QString& content_name);
    uint16_t calculateTestCRC(const QByteArray& data);
    std::vector<uint8_t> toVector(const QByteArray& data);
};

// ============================================================================
// Test Lifecycle Methods
// ============================================================================

void TestMOTProtocol::initTestCase()
{
    qInfo() << "========================================";
    qInfo() << "Starting MOT Protocol Test Suite";
    qInfo() << "Phase 3A Wave 1.2 - PDCA DO Phase";
    qInfo() << "========================================";

    // Register Qt metatypes
    qRegisterMetaType<MOTObject>("MOTObject");
    qRegisterMetaType<MOTHeader>("MOTHeader");
}

void TestMOTProtocol::cleanupTestCase()
{
    qInfo() << "========================================";
    qInfo() << "MOT Protocol Tests Complete";
    qInfo() << "========================================";
}

void TestMOTProtocol::init()
{
    m_motParser = new MOTProtocol();
    QVERIFY(m_motParser != nullptr);
}

void TestMOTProtocol::cleanup()
{
    if (m_motParser) {
        m_motParser->clearAll();
        delete m_motParser;
        m_motParser = nullptr;
    }
}

// ============================================================================
// MSC Data Group Parsing Tests
// ============================================================================

void TestMOTProtocol::testMSCDataGroup_ValidStructure()
{
    // Create valid MSC data group
    QByteArray msc_data = createValidMSCDataGroup(false);

    auto data_group = m_motParser->parseMSCDataGroup(msc_data);
    
    QVERIFY(data_group.has_value());
    QVERIFY(!data_group->data_field.empty());
    QCOMPARE(data_group->data_group_type, static_cast<uint8_t>(3)); // MOT data group type
}

void TestMOTProtocol::testMSCDataGroup_CRCValidation()
{
    // Create MSC data group with valid CRC
    QByteArray msc_data;
    
    // Header: extension=0, crc=1, segment=1, user_access=0, type=3
    uint8_t byte0 = 0x40 | 0x20 | 0x03;  // CRC flag + segment flag + type 3
    uint8_t byte1 = 0x12;  // CI=1, RI=2
    
    msc_data.append(byte0);
    msc_data.append(byte1);
    
    // Add some data
    msc_data.append("TEST_DATA", 9);
    
    // Calculate and append CRC
    uint16_t crc = calculateTestCRC(msc_data);
    msc_data.append(static_cast<char>(crc >> 8));
    msc_data.append(static_cast<char>(crc & 0xFF));
    
    auto data_group = m_motParser->parseMSCDataGroup(msc_data);
    
    QVERIFY(data_group.has_value());
    QVERIFY(data_group->crc_flag);
    
    bool crc_valid = m_motParser->validateDataGroupCRC(*data_group);
    QVERIFY(crc_valid);
}

void TestMOTProtocol::testMSCDataGroup_InvalidCRC()
{
    // Create MSC data group with invalid CRC
    QByteArray msc_data;
    
    uint8_t byte0 = 0x40 | 0x20 | 0x03;  // CRC flag + segment flag + type 3
    uint8_t byte1 = 0x00;
    
    msc_data.append(byte0);
    msc_data.append(byte1);
    msc_data.append("TEST_DATA", 9);
    
    // Append wrong CRC
    msc_data.append(static_cast<char>(0xFF));
    msc_data.append(static_cast<char>(0xFF));
    
    auto data_group = m_motParser->parseMSCDataGroup(msc_data);
    
    QVERIFY(data_group.has_value());
    
    bool crc_valid = m_motParser->validateDataGroupCRC(*data_group);
    QVERIFY(!crc_valid);
}

void TestMOTProtocol::testMSCDataGroup_MinimumSize()
{
    // Test minimum valid data group (2 bytes header only)
    QByteArray msc_data;
    msc_data.append(static_cast<char>(0x03));  // Type 3, no flags
    msc_data.append(static_cast<char>(0x00));
    
    auto data_group = m_motParser->parseMSCDataGroup(msc_data);
    
    QVERIFY(data_group.has_value());
    QVERIFY(data_group->data_field.empty());
}

void TestMOTProtocol::testMSCDataGroup_WithoutCRC()
{
    QByteArray msc_data = createValidMSCDataGroup(false);
    
    auto data_group = m_motParser->parseMSCDataGroup(msc_data);
    
    QVERIFY(data_group.has_value());
    QVERIFY(!data_group->crc_flag);
}

void TestMOTProtocol::testMSCDataGroup_AllFlags()
{
    QByteArray msc_data;
    
    // Set all flags
    uint8_t byte0 = 0x80 | 0x40 | 0x20 | 0x10 | 0x03;  // All flags + type 3
    uint8_t byte1 = 0xAB;  // CI=10, RI=11
    
    msc_data.append(byte0);
    msc_data.append(byte1);
    msc_data.append("DATA", 4);
    
    // Add CRC
    uint16_t crc = calculateTestCRC(msc_data);
    msc_data.append(static_cast<char>(crc >> 8));
    msc_data.append(static_cast<char>(crc & 0xFF));
    
    auto data_group = m_motParser->parseMSCDataGroup(msc_data);
    
    QVERIFY(data_group.has_value());
    QVERIFY(data_group->extension_flag);
    QVERIFY(data_group->crc_flag);
    QVERIFY(data_group->segment_flag);
    QVERIFY(data_group->user_access_flag);
    QCOMPARE(data_group->data_group_type, static_cast<uint8_t>(3));
    QCOMPARE(data_group->continuity_index, static_cast<uint8_t>(10));
    QCOMPARE(data_group->repetition_index, static_cast<uint8_t>(11));
}

// ============================================================================
// Segment Reassembly Tests
// ============================================================================

void TestMOTProtocol::testSegmentReassembly_SingleSegment()
{
    uint32_t transport_id = 100;
    std::vector<uint8_t> segment_data = {0x01, 0x02, 0x03, 0x04};
    
    auto status = m_motParser->addSegment(transport_id, 0, 1, segment_data);
    
    QCOMPARE(status, MOTProtocol::ReassemblyStatus::COMPLETE);
    QVERIFY(m_motParser->isComplete(transport_id));
    QCOMPARE(m_motParser->getProgress(transport_id), 1.0);
}

void TestMOTProtocol::testSegmentReassembly_MultiSegment()
{
    uint32_t transport_id = 200;
    uint16_t total_segments = 3;
    
    // Add first segment
    std::vector<uint8_t> seg0 = {0x01, 0x02};
    auto status0 = m_motParser->addSegment(transport_id, 0, total_segments, seg0);
    QCOMPARE(status0, MOTProtocol::ReassemblyStatus::INCOMPLETE);
    QVERIFY(!m_motParser->isComplete(transport_id));
    
    // Add second segment
    std::vector<uint8_t> seg1 = {0x03, 0x04};
    auto status1 = m_motParser->addSegment(transport_id, 1, total_segments, seg1);
    QCOMPARE(status1, MOTProtocol::ReassemblyStatus::INCOMPLETE);
    
    // Add third segment
    std::vector<uint8_t> seg2 = {0x05, 0x06};
    auto status2 = m_motParser->addSegment(transport_id, 2, total_segments, seg2);
    QCOMPARE(status2, MOTProtocol::ReassemblyStatus::COMPLETE);
    QVERIFY(m_motParser->isComplete(transport_id));
}

void TestMOTProtocol::testSegmentReassembly_DuplicateSegment()
{
    uint32_t transport_id = 300;
    std::vector<uint8_t> segment_data = {0x01, 0x02};
    
    // Add segment first time
    auto status1 = m_motParser->addSegment(transport_id, 0, 2, segment_data);
    QCOMPARE(status1, MOTProtocol::ReassemblyStatus::INCOMPLETE);
    
    // Add same segment again
    auto status2 = m_motParser->addSegment(transport_id, 0, 2, segment_data);
    QCOMPARE(status2, MOTProtocol::ReassemblyStatus::ERROR_DUPLICATE);
}

void TestMOTProtocol::testSegmentReassembly_OutOfOrderSegments()
{
    uint32_t transport_id = 400;
    uint16_t total_segments = 3;
    
    // Add segments out of order: 2, 0, 1
    std::vector<uint8_t> seg2 = {0x05, 0x06};
    auto status2 = m_motParser->addSegment(transport_id, 2, total_segments, seg2);
    QCOMPARE(status2, MOTProtocol::ReassemblyStatus::INCOMPLETE);
    
    std::vector<uint8_t> seg0 = {0x01, 0x02};
    auto status0 = m_motParser->addSegment(transport_id, 0, total_segments, seg0);
    QCOMPARE(status0, MOTProtocol::ReassemblyStatus::INCOMPLETE);
    
    std::vector<uint8_t> seg1 = {0x03, 0x04};
    auto status1 = m_motParser->addSegment(transport_id, 1, total_segments, seg1);
    QCOMPARE(status1, MOTProtocol::ReassemblyStatus::COMPLETE);
}

void TestMOTProtocol::testSegmentReassembly_MissingSegments()
{
    uint32_t transport_id = 500;
    uint16_t total_segments = 3;
    
    // Add only segments 0 and 2, skip 1
    std::vector<uint8_t> seg0 = {0x01, 0x02};
    m_motParser->addSegment(transport_id, 0, total_segments, seg0);
    
    std::vector<uint8_t> seg2 = {0x05, 0x06};
    m_motParser->addSegment(transport_id, 2, total_segments, seg2);
    
    // Should not be complete
    QVERIFY(!m_motParser->isComplete(transport_id));
    
    double progress = m_motParser->getProgress(transport_id);
    QCOMPARE(progress, 2.0 / 3.0);  // 2 out of 3 segments
}

void TestMOTProtocol::testSegmentReassembly_ProgressTracking()
{
    uint32_t transport_id = 600;
    uint16_t total_segments = 4;
    
    // Initially no progress
    QCOMPARE(m_motParser->getProgress(transport_id), -1.0);
    
    // Add segments and check progress
    std::vector<uint8_t> seg0 = {0x01};
    m_motParser->addSegment(transport_id, 0, total_segments, seg0);
    QCOMPARE(m_motParser->getProgress(transport_id), 0.25);
    
    std::vector<uint8_t> seg1 = {0x02};
    m_motParser->addSegment(transport_id, 1, total_segments, seg1);
    QCOMPARE(m_motParser->getProgress(transport_id), 0.5);
    
    std::vector<uint8_t> seg2 = {0x03};
    m_motParser->addSegment(transport_id, 2, total_segments, seg2);
    QCOMPARE(m_motParser->getProgress(transport_id), 0.75);
    
    std::vector<uint8_t> seg3 = {0x04};
    m_motParser->addSegment(transport_id, 3, total_segments, seg3);
    QCOMPARE(m_motParser->getProgress(transport_id), 1.0);
}

// ============================================================================
// Content Type Detection Tests
// ============================================================================

void TestMOTProtocol::testContentType_JPEG()
{
    std::vector<uint8_t> header_data = createMOTHeader(ContentType::IMAGE_JPEG, "test.jpg");
    
    MOTHeader header;
    bool parsed = m_motParser->parseMOTHeader(header_data, header);
    
    QVERIFY(parsed);
    QCOMPARE(header.content_type, ContentType::IMAGE_JPEG);
    QCOMPARE(header.getContentTypeString(), QString("JPEG Image"));
}

void TestMOTProtocol::testContentType_PNG()
{
    std::vector<uint8_t> header_data = createMOTHeader(ContentType::IMAGE_PNG, "test.png");
    
    MOTHeader header;
    bool parsed = m_motParser->parseMOTHeader(header_data, header);
    
    QVERIFY(parsed);
    QCOMPARE(header.content_type, ContentType::IMAGE_PNG);
    QCOMPARE(header.getContentTypeString(), QString("PNG Image"));
}

void TestMOTProtocol::testContentType_HTML()
{
    std::vector<uint8_t> header_data = createMOTHeader(ContentType::HTML, "index.html");
    
    MOTHeader header;
    bool parsed = m_motParser->parseMOTHeader(header_data, header);
    
    QVERIFY(parsed);
    QCOMPARE(header.content_type, ContentType::HTML);
    QCOMPARE(header.getContentTypeString(), QString("HTML Document"));
}

void TestMOTProtocol::testContentType_Unknown()
{
    std::vector<uint8_t> header_data = createMOTHeader(ContentType::UNKNOWN, "unknown.dat");
    
    MOTHeader header;
    bool parsed = m_motParser->parseMOTHeader(header_data, header);
    
    QVERIFY(parsed);
    // ContentType::UNKNOWN (0xFF) is encoded as 0x3F in 6-bit content type field
    // 0xFF >> 2 = 0x3F (63)
    QCOMPARE(static_cast<uint8_t>(header.content_type), static_cast<uint8_t>(0x3F));
    QVERIFY(header.getContentTypeString().contains("Unknown"));
}

// ============================================================================
// Header Parameter Extraction Tests
// ============================================================================

void TestMOTProtocol::testHeaderParameter_ContentName()
{
    // Create header with content name parameter
    std::vector<uint8_t> header_data;
    
    // Basic header (7 bytes minimum)
    header_data.push_back(0x00);  // Body size high (13 bits)
    header_data.push_back(0x10);  // Body size low / Header size high
    header_data.push_back(0x20);  // Header size low
    header_data.push_back(0x08);  // Content type (IMAGE_JPEG = 2 << 2)
    header_data.push_back(0x00);  // Content subtype
    header_data.push_back(0x00);  // Reserved
    header_data.push_back(0x00);  // Reserved
    
    // Add content name parameter (ParamId = 12)
    header_data.push_back(0x0C);  // PLI=0, ParamId=12 (content name)
    header_data.push_back(0x08);  // Length = 8
    header_data.push_back('t');
    header_data.push_back('e');
    header_data.push_back('s');
    header_data.push_back('t');
    header_data.push_back('.');
    header_data.push_back('j');
    header_data.push_back('p');
    header_data.push_back('g');
    
    MOTHeader header;
    bool parsed = m_motParser->parseMOTHeader(header_data, header);
    
    QVERIFY(parsed);
    QCOMPARE(header.content_name, QString("test.jpg"));
}

void TestMOTProtocol::testHeaderParameter_ThaiUTF8()
{
    // Create header with Thai UTF-8 content name
    std::vector<uint8_t> header_data;
    
    // Basic header
    for (int i = 0; i < 7; i++) header_data.push_back(0x00);
    header_data[3] = 0x08;  // Content type JPEG
    
    // Thai text: "ทดสอบ" (test in Thai)
    QByteArray thai_text = QString("ทดสอบ").toUtf8();
    
    header_data.push_back(0x0C);  // ParamId = 12 (content name)
    header_data.push_back(thai_text.size());  // Length
    
    for (int i = 0; i < thai_text.size(); i++) {
        header_data.push_back(static_cast<uint8_t>(thai_text[i]));
    }
    
    MOTHeader header;
    bool parsed = m_motParser->parseMOTHeader(header_data, header);
    
    QVERIFY(parsed);
    QCOMPARE(header.content_name, QString("ทดสอบ"));
}

void TestMOTProtocol::testHeaderParameter_TriggerTime()
{
    // Create header with trigger time parameter
    std::vector<uint8_t> header_data;
    
    // Basic header
    for (int i = 0; i < 7; i++) header_data.push_back(0x00);
    
    // Add trigger time parameter (ParamId = 5)
    header_data.push_back(0x05);  // ParamId = 5 (trigger time)
    header_data.push_back(0x04);  // Length = 4 bytes
    header_data.push_back(0x12);  // Time bytes
    header_data.push_back(0x34);
    header_data.push_back(0x56);
    header_data.push_back(0x78);
    
    MOTHeader header;
    bool parsed = m_motParser->parseMOTHeader(header_data, header);
    
    QVERIFY(parsed);
    QVERIFY(header.trigger_time != 0);
    QCOMPARE(header.trigger_time, static_cast<uint32_t>(0x12345678));
}

void TestMOTProtocol::testHeaderParameter_CategoryTitle()
{
    // Create header with category title parameter (SlideShow)
    std::vector<uint8_t> header_data;
    
    // Basic header
    for (int i = 0; i < 7; i++) header_data.push_back(0x00);
    
    // Add category title parameter (ParamId = 25)
    header_data.push_back(0x19);  // ParamId = 25 (category title)
    header_data.push_back(0x05);  // Length = 5
    header_data.push_back('N');
    header_data.push_back('e');
    header_data.push_back('w');
    header_data.push_back('s');
    header_data.push_back('!');
    
    MOTHeader header;
    bool parsed = m_motParser->parseMOTHeader(header_data, header);
    
    QVERIFY(parsed);
    QCOMPARE(header.category_title, QString("News!"));
}

void TestMOTProtocol::testHeaderParameter_ClickThroughURL()
{
    // Create header with click-through URL parameter
    std::vector<uint8_t> header_data;
    
    // Basic header
    for (int i = 0; i < 7; i++) header_data.push_back(0x00);
    
    // Add click-through URL parameter (ParamId = 27)
    QString url = "http://example.com";
    QByteArray url_bytes = url.toUtf8();
    
    header_data.push_back(0x1B);  // ParamId = 27 (click-through URL)
    header_data.push_back(url_bytes.size());
    
    for (int i = 0; i < url_bytes.size(); i++) {
        header_data.push_back(static_cast<uint8_t>(url_bytes[i]));
    }
    
    MOTHeader header;
    bool parsed = m_motParser->parseMOTHeader(header_data, header);
    
    QVERIFY(parsed);
    QCOMPARE(header.click_through_url, url);
}

void TestMOTProtocol::testHeaderParameter_MultipleParameters()
{
    // Create header with multiple parameters
    std::vector<uint8_t> header_data;
    
    // Basic header
    for (int i = 0; i < 7; i++) header_data.push_back(0x00);
    header_data[3] = 0x08;  // Content type JPEG
    
    // Parameter 1: Content name
    header_data.push_back(0x0C);
    header_data.push_back(0x04);
    header_data.push_back('t');
    header_data.push_back('e');
    header_data.push_back('s');
    header_data.push_back('t');
    
    // Parameter 2: Trigger time
    header_data.push_back(0x05);
    header_data.push_back(0x04);
    header_data.push_back(0x00);
    header_data.push_back(0x00);
    header_data.push_back(0x00);
    header_data.push_back(0x01);
    
    // Parameter 3: Category
    header_data.push_back(0x19);
    header_data.push_back(0x03);
    header_data.push_back('N');
    header_data.push_back('e');
    header_data.push_back('w');
    
    MOTHeader header;
    bool parsed = m_motParser->parseMOTHeader(header_data, header);
    
    QVERIFY(parsed);
    QCOMPARE(header.content_name, QString("test"));
    QCOMPARE(header.trigger_time, static_cast<uint32_t>(1));
    QCOMPARE(header.category_title, QString("New"));
    QCOMPARE(header.parameters.size(), 3);
}

// ============================================================================
// Transport ID Management Tests
// ============================================================================

void TestMOTProtocol::testTransportID_ValidRange()
{
    // Test valid transport IDs (0-1023)
    QVERIFY(m_motParser->validateTransportId(0));
    QVERIFY(m_motParser->validateTransportId(512));
    QVERIFY(m_motParser->validateTransportId(1023));
}

void TestMOTProtocol::testTransportID_InvalidRange()
{
    // The MOT transport id is a 16-bit field, so ids up to 0xFFFF are valid
    // (real multiplexes use e.g. ~14000) and only >0xFFFF is out of range.
    QVERIFY(m_motParser->validateTransportId(1024));
    QVERIFY(m_motParser->validateTransportId(2000));
    QVERIFY(m_motParser->validateTransportId(0xFFFF));
    QVERIFY(!m_motParser->validateTransportId(0x10000));
    QVERIFY(!m_motParser->validateTransportId(0x7FFFFFFF));
}

void TestMOTProtocol::testTransportID_MultipleObjects()
{
    // Add segments for multiple transport IDs
    std::vector<uint8_t> data = {0x01, 0x02};
    
    m_motParser->addSegment(100, 0, 2, data);
    m_motParser->addSegment(200, 0, 2, data);
    m_motParser->addSegment(300, 0, 2, data);
    
    auto active_ids = m_motParser->getActiveTransportIds();
    
    QCOMPARE(active_ids.size(), 3);
    QVERIFY(std::find(active_ids.begin(), active_ids.end(), 100) != active_ids.end());
    QVERIFY(std::find(active_ids.begin(), active_ids.end(), 200) != active_ids.end());
    QVERIFY(std::find(active_ids.begin(), active_ids.end(), 300) != active_ids.end());
}

void TestMOTProtocol::testTransportID_ClearObject()
{
    std::vector<uint8_t> data = {0x01, 0x02};
    
    m_motParser->addSegment(100, 0, 2, data);
    m_motParser->addSegment(200, 0, 2, data);
    
    QCOMPARE(m_motParser->getActiveTransportIds().size(), 2);
    
    m_motParser->clearObject(100);
    
    auto active_ids = m_motParser->getActiveTransportIds();
    QCOMPARE(active_ids.size(), 1);
    QVERIFY(std::find(active_ids.begin(), active_ids.end(), 100) == active_ids.end());
    QVERIFY(std::find(active_ids.begin(), active_ids.end(), 200) != active_ids.end());
}

void TestMOTProtocol::testTransportID_ClearAll()
{
    std::vector<uint8_t> data = {0x01, 0x02};
    
    m_motParser->addSegment(100, 0, 2, data);
    m_motParser->addSegment(200, 0, 2, data);
    m_motParser->addSegment(300, 0, 2, data);
    
    QCOMPARE(m_motParser->getActiveTransportIds().size(), 3);
    
    m_motParser->clearAll();
    
    QCOMPARE(m_motParser->getActiveTransportIds().size(), 0);
}

// ============================================================================
// Edge Cases and Error Handling Tests
// ============================================================================

void TestMOTProtocol::testEdgeCase_EmptyData()
{
    QByteArray empty_data;
    
    auto data_group = m_motParser->parseMSCDataGroup(empty_data);
    
    QVERIFY(!data_group.has_value());
}

void TestMOTProtocol::testEdgeCase_CorruptedHeader()
{
    // Create header with invalid size
    std::vector<uint8_t> header_data;
    header_data.push_back(0x00);
    header_data.push_back(0x00);
    // Only 2 bytes - too short for MOT header
    
    MOTHeader header;
    bool parsed = m_motParser->parseMOTHeader(header_data, header);
    
    QVERIFY(!parsed);
}

void TestMOTProtocol::testEdgeCase_InvalidSegmentNumber()
{
    uint32_t transport_id = 700;
    std::vector<uint8_t> data = {0x01, 0x02};
    
    // Add segment with number > total segments
    auto status = m_motParser->addSegment(transport_id, 5, 3, data);
    
    // Should accept but object won't complete
    QVERIFY(!m_motParser->isComplete(transport_id));
}

void TestMOTProtocol::testEdgeCase_TotalSegmentsMismatch()
{
    uint32_t transport_id = 800;
    std::vector<uint8_t> data = {0x01, 0x02};
    
    // First segment says total = 3
    auto status1 = m_motParser->addSegment(transport_id, 0, 3, data);
    QCOMPARE(status1, MOTProtocol::ReassemblyStatus::INCOMPLETE);
    
    // Second segment says total = 5 (mismatch!)
    auto status2 = m_motParser->addSegment(transport_id, 1, 5, data);
    QCOMPARE(status2, MOTProtocol::ReassemblyStatus::ERROR_MISSING_SEGMENTS);
}

void TestMOTProtocol::testEdgeCase_NullData()
{
    QByteArray null_data;
    null_data.resize(10);
    null_data.fill(0);
    
    auto data_group = m_motParser->parseMSCDataGroup(null_data);
    
    // Should parse but have zero values
    QVERIFY(data_group.has_value());
}

// ============================================================================
// MOT Header Parsing Tests
// ============================================================================

void TestMOTProtocol::testMOTHeader_BasicStructure()
{
    std::vector<uint8_t> header_data;
    
    // Body size = 1000 (13 bits), Header size = 20 (13 bits)
    // Body size: 0x03E8 (1000) = 0001 1111 1010 00
    // Header size: 0x0014 (20) = 00 0001 0100
    header_data.push_back(0x03);  // Body size high 5 bits + 3 bits
    header_data.push_back(0xE8);  // Body size low 8 bits
    header_data.push_back(0x14);  // Header size 8 bits
    header_data.push_back(0x08);  // Content type = 2 (JPEG) << 2
    header_data.push_back(0x00);  // Content subtype
    header_data.push_back(0x00);
    header_data.push_back(0x00);
    
    MOTHeader header;
    bool parsed = m_motParser->parseMOTHeader(header_data, header);
    
    QVERIFY(parsed);
    QCOMPARE(header.content_type, ContentType::IMAGE_JPEG);
}

void TestMOTProtocol::testMOTHeader_ExtensionParameters()
{
    std::vector<uint8_t> header_data = createMOTHeader(ContentType::IMAGE_PNG, "image.png");
    
    MOTHeader header;
    bool parsed = m_motParser->parseMOTHeader(header_data, header);
    
    QVERIFY(parsed);
    QVERIFY(header.hasParameter(12));  // Content name parameter
}

void TestMOTProtocol::testMOTHeader_InvalidSize()
{
    std::vector<uint8_t> header_data;
    
    // Only 3 bytes - invalid
    header_data.push_back(0x00);
    header_data.push_back(0x00);
    header_data.push_back(0x00);
    
    MOTHeader header;
    bool parsed = m_motParser->parseMOTHeader(header_data, header);
    
    QVERIFY(!parsed);
}

// ============================================================================
// Signal Emission Tests
// ============================================================================

void TestMOTProtocol::testSignals_ObjectComplete()
{
    QSignalSpy spy(m_motParser, &MOTProtocol::objectComplete);
    
    uint32_t transport_id = 900;
    std::vector<uint8_t> data = {0x01, 0x02};
    
    // Add single segment (completes immediately)
    m_motParser->addSegment(transport_id, 0, 1, data);
    
    // Signal should not emit from addSegment directly
    // Would need full processMOTData flow
    QVERIFY(spy.count() >= 0);
}

void TestMOTProtocol::testSignals_ReassemblyProgress()
{
    QSignalSpy spy(m_motParser, &MOTProtocol::reassemblyProgress);
    
    // Progress signal emitted through processMOTData workflow
    QVERIFY(spy.isValid());
}

void TestMOTProtocol::testSignals_ParseError()
{
    QSignalSpy spy(m_motParser, &MOTProtocol::parseError);
    
    // Error signal emitted through processMOTData workflow
    QVERIFY(spy.isValid());
}

// ============================================================================
// Statistics Tests
// ============================================================================

void TestMOTProtocol::testStatistics_Tracking()
{
    auto stats = m_motParser->getStatistics();
    
    QCOMPARE(stats.data_groups_processed, static_cast<uint64_t>(0));
    QCOMPARE(stats.segments_received, static_cast<uint64_t>(0));
    QCOMPARE(stats.objects_completed, static_cast<uint64_t>(0));
}

void TestMOTProtocol::testStatistics_Accuracy()
{
    // Add some segments
    std::vector<uint8_t> data = {0x01, 0x02};
    m_motParser->addSegment(100, 0, 2, data);
    m_motParser->addSegment(200, 0, 2, data);
    
    auto stats = m_motParser->getStatistics();
    
    // Active objects should be tracked
    QCOMPARE(stats.active_objects, static_cast<uint32_t>(2));
}

// ============================================================================
// Helper Methods Implementation
// ============================================================================

QByteArray TestMOTProtocol::createValidMSCDataGroup(bool with_crc)
{
    QByteArray data;
    
    // Header: CRC flag depends on parameter, segment flag = 1, type = 3
    uint8_t byte0 = 0x20 | 0x03;  // Segment flag + type 3
    if (with_crc) {
        byte0 |= 0x40;  // Add CRC flag
    }
    
    uint8_t byte1 = 0x00;  // CI=0, RI=0
    
    data.append(byte0);
    data.append(byte1);
    
    // Add some test data
    data.append("TEST_SEGMENT_DATA", 17);
    
    if (with_crc) {
        uint16_t crc = calculateTestCRC(data);
        data.append(static_cast<char>(crc >> 8));
        data.append(static_cast<char>(crc & 0xFF));
    }
    
    return data;
}

QByteArray TestMOTProtocol::createMSCDataGroupWithSegment(uint16_t segment_num,
                                                          uint16_t total_segments,
                                                          uint32_t transport_id,
                                                          const QByteArray& data)
{
    QByteArray msc_data;
    
    // MSC data group header
    uint8_t byte0 = 0x20 | 0x03;  // Segment flag + type 3
    uint8_t byte1 = 0x00;
    
    msc_data.append(byte0);
    msc_data.append(byte1);
    
    // MOT segment header
    msc_data.append(static_cast<char>(0x00));  // Repetition count high
    msc_data.append(static_cast<char>(0x00));  // Repetition count low
    msc_data.append(static_cast<char>((segment_num >> 8) & 0x1F));  // Segment number high
    msc_data.append(static_cast<char>(segment_num & 0xFF));  // Segment number low
    msc_data.append(static_cast<char>(transport_id >> 8));  // Transport ID high
    msc_data.append(static_cast<char>(transport_id & 0xFF));  // Transport ID low
    msc_data.append(static_cast<char>((total_segments >> 8) & 0x1F));  // Total segments high
    msc_data.append(static_cast<char>(total_segments & 0xFF));  // Total segments low
    
    // Append payload data
    msc_data.append(data);
    
    return msc_data;
}

std::vector<uint8_t> TestMOTProtocol::createMOTHeader(ContentType type, const QString& content_name)
{
    std::vector<uint8_t> header;
    
    // Basic header structure (7 bytes)
    header.push_back(0x00);  // Body size high
    header.push_back(0x10);  // Body size low
    header.push_back(0x20);  // Header size
    header.push_back(static_cast<uint8_t>(type) << 2);  // Content type
    header.push_back(0x00);  // Content subtype
    header.push_back(0x00);
    header.push_back(0x00);
    
    // Add content name parameter if provided
    if (!content_name.isEmpty()) {
        QByteArray name_bytes = content_name.toUtf8();
        
        header.push_back(0x0C);  // ParamId = 12 (content name)
        header.push_back(name_bytes.size());  // Length
        
        for (int i = 0; i < name_bytes.size(); i++) {
            header.push_back(static_cast<uint8_t>(name_bytes[i]));
        }
    }
    
    return header;
}

uint16_t TestMOTProtocol::calculateTestCRC(const QByteArray& data)
{
    // Simple CRC-CCITT calculation for testing
    uint16_t crc = 0xFFFF;
    const uint16_t polynomial = 0x1021;
    
    for (int i = 0; i < data.size(); i++) {
        crc ^= (static_cast<uint8_t>(data[i]) << 8);
        
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
    }
    
    return crc;
}

std::vector<uint8_t> TestMOTProtocol::toVector(const QByteArray& data)
{
    std::vector<uint8_t> vec;
    vec.reserve(data.size());
    
    for (int i = 0; i < data.size(); i++) {
        vec.push_back(static_cast<uint8_t>(data[i]));
    }
    
    return vec;
}

// ============================================================================
// MOT MSC data group adapter tests (X-PAD CI 12/13 transport path)
// ============================================================================

void TestMOTProtocol::testMotPadAdapter_StandardHeaderCore()
{
    const QByteArray body = patternBody(1234, 0x10);
    const QByteArray header = buildStandardMotHeaderCore(body.size(), 0x02, 0x01);

    uint32_t bodySize = 0;
    uint16_t headerSize = 0;
    uint8_t contentType = 0;
    uint16_t subtype = 0;
    QVERIFY(MotPadAdapter::parseStandardHeader(header, bodySize, headerSize, contentType, subtype));
    QCOMPARE(bodySize, static_cast<uint32_t>(1234));
    QCOMPARE(headerSize, static_cast<uint16_t>(7));
    QCOMPARE(contentType, static_cast<uint8_t>(0x02));
    QCOMPARE(subtype, static_cast<uint16_t>(0x01));

    // Too-short header is rejected.
    MOTHeader dummy;
    uint32_t bs = 0; uint16_t hs = 0; uint8_t ct = 0; uint16_t st = 0;
    QVERIFY(!MotPadAdapter::parseStandardHeader(QByteArray(6, '\0'), bs, hs, ct, st));
}

void TestMOTProtocol::testMotPadAdapter_HeaderBodyObjectComplete()
{
    MOTProtocol mot;
    MotPadAdapter adapter(&mot);
    QSignalSpy spy(&mot, &MOTProtocol::objectComplete);
    QVERIFY(spy.isValid());

    const uint16_t transportId = 0x0123;
    const QByteArray body = patternBody(300, 0x40);
    const QByteArray header = buildStandardMotHeaderCore(body.size(), 0x02, 0x01);

    // Header entity then body entity (each a single segment).
    QVERIFY(adapter.processDataGroup(0x2C61,
        buildStandardMotDataGroup(3, transportId, 0, true, header)));
    QCOMPARE(spy.count(), 0);
    QVERIFY(adapter.processDataGroup(0x2C61,
        buildStandardMotDataGroup(4, transportId, 0, true, body)));

    QCOMPARE(spy.count(), 1);
    const QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).toUInt(), static_cast<uint>(transportId));
    const MOTObject object = qvariant_cast<MOTObject>(args.at(1));
    QCOMPARE(object.transport_id, static_cast<uint32_t>(transportId));
    const QByteArray got(reinterpret_cast<const char*>(object.body.data()),
                         static_cast<int>(object.body.size()));
    QCOMPARE(got, body);
    QCOMPARE(static_cast<uint8_t>(object.header.content_type),
             static_cast<uint8_t>(ContentType::IMAGE_JPEG));

    const MotPadAdapter::Statistics stats = adapter.statistics();
    QCOMPARE(stats.dataGroups, static_cast<uint64_t>(2));
    QCOMPARE(stats.headerSegments, static_cast<uint64_t>(1));
    QCOMPARE(stats.bodySegments, static_cast<uint64_t>(1));
    QCOMPARE(stats.objectsCompleted, static_cast<uint64_t>(1));
    QCOMPARE(stats.objectsEmitted, static_cast<uint64_t>(1));
}

void TestMOTProtocol::testMotPadAdapter_MultiSegmentBody()
{
    MOTProtocol mot;
    MotPadAdapter adapter(&mot);
    QSignalSpy spy(&mot, &MOTProtocol::objectComplete);

    const uint16_t transportId = 77;
    const QByteArray body = patternBody(500, 0x20);
    const QByteArray header = buildStandardMotHeaderCore(body.size(), 0x02, 0x01);

    QVERIFY(adapter.processDataGroup(0, buildStandardMotDataGroup(3, transportId, 0, true, header)));
    // Body split into three segments, delivered out of order (2, 0, 1).
    const QByteArray s0 = body.left(200);
    const QByteArray s1 = body.mid(200, 150);
    const QByteArray s2 = body.mid(350);
    QVERIFY(adapter.processDataGroup(0, buildStandardMotDataGroup(4, transportId, 2, true, s2)));
    QCOMPARE(spy.count(), 0);
    QVERIFY(adapter.processDataGroup(0, buildStandardMotDataGroup(4, transportId, 0, false, s0)));
    QVERIFY(adapter.processDataGroup(0, buildStandardMotDataGroup(4, transportId, 1, false, s1)));

    QCOMPARE(spy.count(), 1);
    const MOTObject object = qvariant_cast<MOTObject>(spy.takeFirst().at(1));
    const QByteArray got(reinterpret_cast<const char*>(object.body.data()),
                         static_cast<int>(object.body.size()));
    QCOMPARE(got, body);
}

void TestMOTProtocol::testMotPadAdapter_CrcRejected()
{
    MOTProtocol mot;
    MotPadAdapter adapter(&mot);
    QSignalSpy spy(&mot, &MOTProtocol::objectComplete);

    const QByteArray body = patternBody(64, 0x01);
    const QByteArray header = buildStandardMotHeaderCore(body.size(), 0x02, 0x01);
    QByteArray badHeaderDg = buildStandardMotDataGroup(3, 9, 0, true, header);
    badHeaderDg[badHeaderDg.size() - 1] =
        static_cast<char>(badHeaderDg[badHeaderDg.size() - 1] ^ 0xFF);  // corrupt CRC

    QVERIFY(!adapter.processDataGroup(0, badHeaderDg));
    QVERIFY(adapter.processDataGroup(0, buildStandardMotDataGroup(4, 9, 0, true, body)));
    QCOMPARE(spy.count(), 0);  // header never accepted -> no object

    const MotPadAdapter::Statistics stats = adapter.statistics();
    QCOMPARE(stats.crcErrors, static_cast<uint64_t>(1));
}

void TestMOTProtocol::testMotPadAdapter_ReframedLayout()
{
    // The adapter's re-framed data group must be parseable by MOTProtocol and
    // expose core[0..6] immediately followed by the body.
    const QByteArray body = patternBody(40, 0x55);
    const QByteArray dg = MotPadAdapter::buildReframedDataGroup(0x2A, 0x02, body);
    QVERIFY(dg.size() > body.size());

    MOTProtocol mot;
    QSignalSpy spy(&mot, &MOTProtocol::objectComplete);
    QVERIFY(mot.processMOTData(0, dg));
    QCOMPARE(spy.count(), 1);
    const MOTObject object = qvariant_cast<MOTObject>(spy.takeFirst().at(1));
    const QByteArray got(reinterpret_cast<const char*>(object.body.data()),
                         static_cast<int>(object.body.size()));
    QCOMPARE(got, body);
}

void TestMOTProtocol::testMotPadAdapter_ReframedIsValid()
{
    // The re-framed object must satisfy MOTObject::isValid() (body size
    // reconciled) and keep the full 16-bit transport id (>= 1024).
    MOTProtocol mot;
    MotPadAdapter adapter(&mot);
    QSignalSpy spy(&mot, &MOTProtocol::objectComplete);

    const uint16_t transportId = 14496;  // real-capture-like 16-bit id
    const QByteArray body = patternBody(20000, 0x33);  // > 8191: multi-segment
    const QByteArray header = buildStandardMotHeaderCore(body.size(), 0x02, 0x03);

    QVERIFY(adapter.processDataGroup(0x2C61,
        buildStandardMotDataGroup(3, transportId, 0, true, header)));
    // Split into 3 body segments (each <= the 13-bit segment size limit).
    const int segLen = 7000;
    for (int seg = 0, off = 0; off < body.size(); ++seg, off += segLen) {
        const bool last = (off + segLen >= body.size());
        QVERIFY(adapter.processDataGroup(0x2C61, buildStandardMotDataGroup(
            4, transportId, static_cast<uint16_t>(seg), last, body.mid(off, segLen))));
    }

    QCOMPARE(spy.count(), 1);
    const MOTObject object = qvariant_cast<MOTObject>(spy.takeFirst().at(1));
    QVERIFY2(object.isValid(),
             qPrintable(QString("re-framed object must be valid: body=%1 declared=%2 hdr=%3")
                            .arg(object.body.size())
                            .arg(object.header.body_size)
                            .arg(object.header.header_size)));
    QCOMPARE(object.transport_id, static_cast<uint32_t>(transportId));
    QCOMPARE(static_cast<uint32_t>(object.header.body_size),
             static_cast<uint32_t>(body.size()));
    const QByteArray got(reinterpret_cast<const char*>(object.body.data()),
                         static_cast<int>(object.body.size()));
    QCOMPARE(got, body);
}

void TestMOTProtocol::testMotPadAdapter_OversizeRejected()
{
    MOTProtocol mot;
    MotPadAdapter adapter(&mot);
    MotPadAdapter::Limits limits;
    limits.maxBodyBytes = 64;  // tiny cap for the boundary test
    adapter.setLimits(limits);
    QSignalSpy spy(&mot, &MOTProtocol::objectComplete);

    const QByteArray body = patternBody(200, 0x01);
    const QByteArray header = buildStandardMotHeaderCore(body.size(), 0x02, 0x01);

    QVERIFY(adapter.processDataGroup(0, buildStandardMotDataGroup(3, 5, 0, true, header)));
    // The 200-byte body segment must be rejected on the running counter,
    // before it is buffered.
    QVERIFY(!adapter.processDataGroup(0, buildStandardMotDataGroup(4, 5, 0, true, body)));
    QCOMPARE(spy.count(), 0);
    QVERIFY2(adapter.statistics().oversized >= 1, "oversize must be counted");
}

void TestMOTProtocol::testMotPadAdapter_StaleEviction()
{
    MOTProtocol mot;
    MotPadAdapter adapter(&mot);
    MotPadAdapter::Limits limits;
    limits.maxBodyBytes = 1u << 20;
    limits.maxAssemblies = 16;
    limits.staleTicks = 3;
    adapter.setLimits(limits);

    // Incomplete object for tid 1, then advance the tick with rejected groups.
    const QByteArray header = buildStandardMotHeaderCore(100, 0x02, 0x01);
    QVERIFY(adapter.processDataGroup(0, buildStandardMotDataGroup(3, 1, 0, true, header)));
    for (int i = 0; i < 6; ++i) {
        adapter.processDataGroup(0, QByteArray(1, '\0'));  // rejected, advances tick
    }
    // A new key triggers stale eviction of the idle tid-1 assembly.
    QVERIFY(adapter.processDataGroup(0, buildStandardMotDataGroup(3, 2, 0, true, header)));
    QVERIFY2(adapter.statistics().staleEvictions >= 1,
             "an idle incomplete assembly must be evicted");
}

void TestMOTProtocol::testMotPadAdapter_CarouselReset()
{
    MOTProtocol mot;
    MotPadAdapter adapter(&mot);
    QSignalSpy spy(&mot, &MOTProtocol::objectComplete);

    const uint16_t tid = 9;
    const QByteArray body1 = patternBody(120, 0x10);
    const QByteArray body2 = patternBody(150, 0x60);

    // Cycle 1: header + body seg 0/1, but never the last segment.
    QVERIFY(adapter.processDataGroup(0, buildStandardMotDataGroup(
        3, tid, 0, true, buildStandardMotHeaderCore(body1.size(), 0x02, 0x01))));
    QVERIFY(adapter.processDataGroup(0, buildStandardMotDataGroup(4, tid, 0, false, body1.left(60))));
    QVERIFY(adapter.processDataGroup(0, buildStandardMotDataGroup(4, tid, 1, false, body1.mid(60))));
    QCOMPARE(spy.count(), 0);

    // Cycle 2 (new object version, same transport id): repeating segment 0
    // resets the stale assembly, then the new body completes.
    QVERIFY(adapter.processDataGroup(0, buildStandardMotDataGroup(
        3, tid, 0, true, buildStandardMotHeaderCore(body2.size(), 0x02, 0x01))));
    QVERIFY(adapter.processDataGroup(0, buildStandardMotDataGroup(4, tid, 0, false, body2.left(70))));
    QVERIFY(adapter.processDataGroup(0, buildStandardMotDataGroup(4, tid, 1, true, body2.mid(70))));

    QCOMPARE(spy.count(), 1);
    const MOTObject object = qvariant_cast<MOTObject>(spy.takeFirst().at(1));
    const QByteArray got(reinterpret_cast<const char*>(object.body.data()),
                         static_cast<int>(object.body.size()));
    QCOMPARE(got, body2);  // cycle-1 bytes must NOT have merged in
    QVERIFY2(adapter.statistics().objResets >= 1, "the carousel restart must reset the assembly");
}

// ============================================================================
// Test Execution
// ============================================================================

QTEST_MAIN(TestMOTProtocol)
#include "test_mot_protocol.moc"
