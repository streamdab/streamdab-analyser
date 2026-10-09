/**
 * @file test_mot_protocol.cpp
 * @brief Unit tests for the standard-layout MOT Protocol parser (T26).
 *
 * The parser consumes standard ETSI EN 300 401 §5.3.3 MSC data groups:
 * data-group header (+ optional extension) · MOT session header · segmentation
 * header · segment data · ~CRC-16/CCITT-FALSE. Data-group type 3 carries a MOT
 * header entity, type 4 a MOT body entity. Tests build real standard data
 * groups (no legacy "simplified layout") and drive MOTProtocol::processMOTData.
 *
 * Coverage:
 * - Standard MSC data group parsing + CRC-16/CCITT-FALSE (one's complement)
 * - Standard 7-byte MOT header core (28-bit body size, 13-bit header size)
 * - Header extension parameters (content name, URL, category, trigger time, Thai)
 * - Header+body entity reassembly (single, multi-segment, out-of-order, gap)
 * - CRC reject, transport-id validation, carousel reset, oversize, stale eviction
 * - Progress reporting, statistics, clear/reset
 * - MotPadAdapter thin-shim façade (forwarding + statistics mapping)
 *
 * @date October 2026
 */

#include <QtTest/QtTest>
#include <QObject>
#include <QByteArray>
#include <QSignalSpy>
#include <QDebug>
#include <algorithm>
#include <vector>

#include "../src/core/mot_protocol.hpp"
#include "../src/core/mot_pad_adapter.hpp"
#include "../src/core/crc16.hpp"

using namespace eti::mot;

// ============================================================================
// Helpers: build standard EN 300 401 / EN 301 234 structures.
// ============================================================================
namespace {

// 7-byte standard MOT header core (ETSI EN 301 234 §5.1): body size(28)
// header size(13) content type(6) content subtype(9).
QByteArray buildHeaderCore(uint32_t bodySize, uint8_t contentType, uint16_t subType,
                           uint16_t headerSize = 7)
{
    QByteArray h(7, '\0');
    auto* d = reinterpret_cast<uint8_t*>(h.data());
    d[0] = static_cast<uint8_t>((bodySize >> 20) & 0xFF);
    d[1] = static_cast<uint8_t>((bodySize >> 12) & 0xFF);
    d[2] = static_cast<uint8_t>((bodySize >> 4) & 0xFF);
    d[3] = static_cast<uint8_t>(((bodySize & 0x0F) << 4) | ((headerSize >> 9) & 0x0F));
    d[4] = static_cast<uint8_t>((headerSize >> 1) & 0xFF);
    d[5] = static_cast<uint8_t>(((headerSize & 0x01) << 7) |
                                ((contentType & 0x3F) << 1) |
                                ((subType >> 8) & 0x01));
    d[6] = static_cast<uint8_t>(subType & 0xFF);
    return h;
}

// One MOT header extension parameter (EN 301 234 §6.2 Fig.22). `data` is the
// DataField (for text parameters it starts with the charset/Rfa byte).
struct ParamSpec {
    uint8_t id;
    uint8_t pli;       // 0..3 (Parameter Length Indicator)
    QByteArray data;   // DataField
};

// Encode a parameter with an explicit PLI, matching the spec semantics:
//   0 -> length 0; 1 -> 8-bit length; 2 -> 32-bit length; 3 -> 7/15-bit.
QByteArray encodeParam(const ParamSpec& p)
{
    QByteArray out;
    out.append(static_cast<char>(((p.pli & 0x03) << 6) | (p.id & 0x3F)));
    const int n = p.data.size();
    switch (p.pli & 0x03) {
        case 0x0:
            break;
        case 0x1:
            out.append(static_cast<char>(n & 0xFF));
            break;
        case 0x2:
            out.append(static_cast<char>((n >> 24) & 0xFF));
            out.append(static_cast<char>((n >> 16) & 0xFF));
            out.append(static_cast<char>((n >> 8) & 0xFF));
            out.append(static_cast<char>(n & 0xFF));
            break;
        case 0x3:
            if (n < 0x80) {
                out.append(static_cast<char>(n & 0x7F));
            } else {
                out.append(static_cast<char>(0x80 | ((n >> 8) & 0x7F)));
                out.append(static_cast<char>(n & 0xFF));
            }
            break;
    }
    out.append(p.data);
    return out;
}

// Text parameter DataField: charset indicator (high nibble) + Rfa (low nibble)
// then the character field (EN 301 234 §6.2.2.1.1 Fig.24).
QByteArray textParam(uint8_t charsetNibble, const QByteArray& text)
{
    QByteArray d;
    d.append(static_cast<char>((charsetNibble & 0x0F) << 4));
    d.append(text);
    return d;
}

// Standard header entity: core + extension parameters.
QByteArray buildHeaderEntity(uint32_t bodySize, uint8_t contentType, uint16_t subType,
                             const QList<ParamSpec>& params = {})
{
    QByteArray ext;
    for (const ParamSpec& p : params) {
        ext.append(encodeParam(p));
    }
    const uint16_t headerSize = static_cast<uint16_t>(7 + ext.size());
    QByteArray h = buildHeaderCore(bodySize, contentType, subType, headerSize);
    h.append(ext);
    return h;
}

// Standard MOT MSC data group: 2-byte header (crc/segment/user-access/type) +
// session header (last|seg, transport-id flag+len, transport id) +
// segmentation header (segment size) + payload + ~CRC-16/CCITT-FALSE.
QByteArray buildDataGroup(uint8_t dgType, uint32_t transportId,
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

std::vector<uint8_t> toVector(const QByteArray& data)
{
    return std::vector<uint8_t>(data.begin(), data.end());
}

QByteArray objectBody(const MOTObject& object)
{
    return QByteArray(reinterpret_cast<const char*>(object.body.data()),
                      static_cast<int>(object.body.size()));
}

}  // namespace

class TestMOTProtocol : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Standard MSC data group parsing / CRC
    void testMSCDataGroup_Parse();
    void testMSCDataGroup_CRCValid();
    void testMSCDataGroup_CRCInvalid();
    void testMSCDataGroup_MinimumSize();
    void testMSCDataGroup_Flags();

    // Standard MOT header core / extension parameters
    void testHeaderCore_StandardLayout();
    void testHeaderCore_28BitBodySize();
    void testParseMOTHeader_Core();
    void testHeaderParameter_ContentName();
    void testHeaderParameter_ClickThroughURL();
    void testHeaderParameter_CategoryAndTrigger();
    void testHeaderParameter_ThaiUTF8();
    void testHeaderParameter_MultipleParameters();
    void testHeaderParameter_PliBoundsAndZeroLength();

    // Content type
    void testContentType_JPEG();
    void testContentType_PNG();
    void testContentType_HTML();

    // Transport ID management
    void testTransportID_ValidRange();
    void testTransportID_InvalidRange();

    // Standard end-to-end reassembly through processMOTData
    void testProcessMOTData_SingleObject();
    void testProcessMOTData_Full16BitTransportId();
    void testProcessMOTData_HeaderParamsRestored();
    void testProcessMOTData_MultiSegmentOutOfOrder();
    void testProcessMOTData_DuplicateSegment();
    void testProcessMOTData_DuplicateHeaderRepeatNoReset();
    void testProcessMOTData_GapTolerance();
    void testProcessMOTData_CrcRejected();
    void testProcessMOTData_CarouselReset();
    void testProcessMOTData_OversizeRejected();
    void testProcessMOTData_StaleEviction();
    void testProcessMOTData_MultipleObjects();
    void testProcessMOTData_Progress();

    // State management / statistics
    void testState_ProgressAndComplete();
    void testState_ClearAll();
    void testState_ResetStatistics();

    // MotPadAdapter thin shim
    void testAdapter_ParseStandardHeader();
    void testAdapter_ShimCompletesObject();
    void testAdapter_StatisticsMapping();

private:
    MOTProtocol* m_motParser = nullptr;
};

// ============================================================================
// Lifecycle
// ============================================================================

void TestMOTProtocol::initTestCase()
{
    qInfo() << "========================================";
    qInfo() << "Starting MOT Protocol Test Suite (standard layout, T26)";
    qInfo() << "========================================";
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
// Standard MSC data group parsing / CRC
// ============================================================================

void TestMOTProtocol::testMSCDataGroup_Parse()
{
    const QByteArray dg = buildDataGroup(3, 0x0123, 0, true, buildHeaderCore(100, 0x02, 0x01));
    auto parsed = m_motParser->parseMSCDataGroup(dg);
    QVERIFY(parsed.has_value());
    QCOMPARE(parsed->data_group_type, static_cast<uint8_t>(3));
    QVERIFY(parsed->crc_flag);
    QVERIFY(parsed->segment_flag);
    QVERIFY(parsed->user_access_flag);
    QVERIFY(!parsed->data_field.empty());
    QVERIFY(parsed->crc_valid);
}

void TestMOTProtocol::testMSCDataGroup_CRCValid()
{
    const QByteArray dg = buildDataGroup(4, 0x0042, 0, true, patternBody(64, 0x10));
    auto parsed = m_motParser->parseMSCDataGroup(dg);
    QVERIFY(parsed.has_value());
    QVERIFY(parsed->crc_flag);
    QVERIFY(m_motParser->validateDataGroupCRC(*parsed));
}

void TestMOTProtocol::testMSCDataGroup_CRCInvalid()
{
    QByteArray dg = buildDataGroup(4, 0x0042, 0, true, patternBody(64, 0x10));
    dg[dg.size() - 1] = static_cast<char>(dg[dg.size() - 1] ^ 0xFF);
    auto parsed = m_motParser->parseMSCDataGroup(dg);
    QVERIFY(parsed.has_value());
    QVERIFY(!m_motParser->validateDataGroupCRC(*parsed));

    // processMOTData must reject it and count a CRC error.
    QVERIFY(!m_motParser->processMOTData(0, dg));
    QCOMPARE(m_motParser->getStatistics().crc_errors, static_cast<uint64_t>(1));
}

void TestMOTProtocol::testMSCDataGroup_MinimumSize()
{
    QByteArray dg;
    dg.append(static_cast<char>(0x03));  // type 3, no flags
    dg.append(static_cast<char>(0x00));
    auto parsed = m_motParser->parseMSCDataGroup(dg);
    QVERIFY(parsed.has_value());
    QVERIFY(parsed->data_field.empty());
}

void TestMOTProtocol::testMSCDataGroup_Flags()
{
    QByteArray dg;
    dg.append(static_cast<char>(0x80 | 0x40 | 0x20 | 0x10 | 0x03));  // all flags + type 3
    dg.append(static_cast<char>(0xAB));  // CI=10, RI=11
    dg.append(static_cast<char>(0xDE));  // extension byte 0
    dg.append(static_cast<char>(0xAD));  // extension byte 1
    dg.append("DATA", 4);
    const uint16_t crc = static_cast<uint16_t>(~eti::crc16ccitt_false(
        reinterpret_cast<const uint8_t*>(dg.constData()), dg.size()));
    dg.append(static_cast<char>(crc >> 8));
    dg.append(static_cast<char>(crc & 0xFF));

    auto parsed = m_motParser->parseMSCDataGroup(dg);
    QVERIFY(parsed.has_value());
    QVERIFY(parsed->extension_flag);
    QVERIFY(parsed->crc_flag);
    QVERIFY(parsed->segment_flag);
    QVERIFY(parsed->user_access_flag);
    QCOMPARE(parsed->data_group_type, static_cast<uint8_t>(3));
    QCOMPARE(parsed->continuity_index, static_cast<uint8_t>(10));
    QCOMPARE(parsed->repetition_index, static_cast<uint8_t>(11));
    QCOMPARE(parsed->extension_data.size(), static_cast<size_t>(2));
    QVERIFY(m_motParser->validateDataGroupCRC(*parsed));
}

// ============================================================================
// Standard MOT header core / extension parameters
// ============================================================================

void TestMOTProtocol::testHeaderCore_StandardLayout()
{
    const QByteArray core = buildHeaderCore(1234, 0x02, 0x01, 7);
    uint32_t bodySize = 0;
    uint16_t headerSize = 0;
    uint8_t contentType = 0;
    uint16_t subType = 0;
    QVERIFY(MOTProtocol::parseHeaderCore(
        reinterpret_cast<const uint8_t*>(core.constData()),
        static_cast<size_t>(core.size()), bodySize, headerSize, contentType, subType));
    QCOMPARE(bodySize, static_cast<uint32_t>(1234));
    QCOMPARE(headerSize, static_cast<uint16_t>(7));
    QCOMPARE(contentType, static_cast<uint8_t>(0x02));
    QCOMPARE(subType, static_cast<uint16_t>(0x01));

    // Too short.
    QVERIFY(!MOTProtocol::parseHeaderCore(nullptr, 0, bodySize, headerSize, contentType, subType));
    const QByteArray six(6, '\0');
    QVERIFY(!MOTProtocol::parseHeaderCore(
        reinterpret_cast<const uint8_t*>(six.constData()), 6,
        bodySize, headerSize, contentType, subType));
}

void TestMOTProtocol::testHeaderCore_28BitBodySize()
{
    // A body size that does not fit in the legacy 13-bit field, exercising the
    // standard 28-bit field (the fixture's largest body is ~55 kB).
    const uint32_t bodySize = 0x0ABCDE;  // 703710
    const QByteArray core = buildHeaderCore(bodySize, 0x02, 0x0003);
    MOTHeader header;
    QVERIFY(m_motParser->parseMOTHeader(toVector(core), header));
    QCOMPARE(header.body_size, bodySize);
    QCOMPARE(static_cast<uint8_t>(header.content_type), static_cast<uint8_t>(0x02));
    QCOMPARE(header.content_subtype, static_cast<uint16_t>(0x0003));
}

void TestMOTProtocol::testParseMOTHeader_Core()
{
    MOTHeader header;
    QVERIFY(m_motParser->parseMOTHeader(toVector(buildHeaderCore(1000, 0x03, 0x0012)), header));
    QCOMPARE(header.body_size, static_cast<uint32_t>(1000));
    QCOMPARE(header.header_size, static_cast<uint16_t>(7));
    QCOMPARE(static_cast<uint8_t>(header.content_type), static_cast<uint8_t>(ContentType::IMAGE_PNG));
    QCOMPARE(header.content_subtype, static_cast<uint16_t>(0x0012));
}

void TestMOTProtocol::testHeaderParameter_ContentName()
{
    const QByteArray header = buildHeaderEntity(
        500, 0x02, 0x03,
        {{0x0C, 0x3, textParam(0x0, QByteArray("test.jpg"))}});
    MOTHeader parsed;
    QVERIFY(m_motParser->parseMOTHeader(toVector(header), parsed));
    QCOMPARE(parsed.content_name, QString("test.jpg"));
    QVERIFY(parsed.hasParameter(0x0C));
}

void TestMOTProtocol::testHeaderParameter_ClickThroughURL()
{
    const QByteArray header = buildHeaderEntity(
        500, 0x02, 0x03,
        {{0x27, 0x3, textParam(0x0, QByteArray("http://example.com/slide"))}});
    MOTHeader parsed;
    QVERIFY(m_motParser->parseMOTHeader(toVector(header), parsed));
    QCOMPARE(parsed.click_through_url, QString("http://example.com/slide"));
}

void TestMOTProtocol::testHeaderParameter_CategoryAndTrigger()
{
    // Category title is a text parameter (PLI=3, 7/15-bit); TriggerTime uses
    // the 32-bit-length form (PLI=2) per the fixture/EN 301 234.
    const QByteArray header = buildHeaderEntity(
        500, 0x02, 0x03,
        {{0x26, 0x3, textParam(0x0, QByteArray("News!"))},
         {0x05, 0x2, QByteArray("\x12\x34\x56\x78", 4)}});
    MOTHeader parsed;
    QVERIFY(m_motParser->parseMOTHeader(toVector(header), parsed));
    QCOMPARE(parsed.category_title, QString("News!"));
    QCOMPARE(parsed.trigger_time, static_cast<uint32_t>(0x12345678));
}

void TestMOTProtocol::testHeaderParameter_ThaiUTF8()
{
    const QByteArray thai = QString("ทดสอบ").toUtf8();
    // charset nibble 0xF = UTF-8 (EN 301 234 Table 3).
    const QByteArray header = buildHeaderEntity(
        500, 0x02, 0x03,
        {{0x0C, 0x3, textParam(0xF, thai)}});
    MOTHeader parsed;
    QVERIFY(m_motParser->parseMOTHeader(toVector(header), parsed));
    QCOMPARE(parsed.content_name, QString("ทดสอบ"));
}

void TestMOTProtocol::testHeaderParameter_MultipleParameters()
{
    const QByteArray header = buildHeaderEntity(
        500, 0x02, 0x03,
        {{0x0C, 0x3, textParam(0x0, QByteArray("test"))},
         {0x26, 0x3, textParam(0x0, QByteArray("New"))},
         {0x27, 0x3, textParam(0x0, QByteArray("http://x"))}});
    MOTHeader parsed;
    QVERIFY(m_motParser->parseMOTHeader(toVector(header), parsed));
    QCOMPARE(parsed.content_name, QString("test"));
    QCOMPARE(parsed.category_title, QString("New"));
    QCOMPARE(parsed.click_through_url, QString("http://x"));
    QCOMPARE(parsed.parameters.size(), static_cast<size_t>(3));
}

void TestMOTProtocol::testHeaderParameter_PliBoundsAndZeroLength()
{
    // PLI=00 is a valid 1-byte parameter with an empty DataField.
    {
        const std::vector<uint8_t> data = {static_cast<uint8_t>(0x00 | 0x0C)};
        MOTHeader h;
        QVERIFY(m_motParser->parseHeaderParameters(data, 0, data.size(), h));
        QVERIFY(h.hasParameter(0x0C));
    }
    // PLI=01 with a length that overruns the buffer -> rejected.
    {
        const std::vector<uint8_t> data = {
            static_cast<uint8_t>(0x40 | 0x0C), 0x20, 'A'};
        MOTHeader h;
        QVERIFY(!m_motParser->parseHeaderParameters(data, 0, data.size(), h));
    }
    // PLI=11 with a 15-bit length that overruns the buffer -> rejected.
    {
        const std::vector<uint8_t> data = {
            static_cast<uint8_t>(0xC0 | 0x0C), 0x80, 0x7F, 'A'};
        MOTHeader h;
        QVERIFY(!m_motParser->parseHeaderParameters(data, 0, data.size(), h));
    }
    // PLI=10 with a length field that itself is truncated -> rejected.
    {
        const std::vector<uint8_t> data = {
            static_cast<uint8_t>(0x80 | 0x0C), 0x00, 0x00};
        MOTHeader h;
        QVERIFY(!m_motParser->parseHeaderParameters(data, 0, data.size(), h));
    }
}

// ============================================================================
// Content type
// ============================================================================

void TestMOTProtocol::testContentType_JPEG()
{
    MOTHeader header;
    QVERIFY(m_motParser->parseMOTHeader(
        toVector(buildHeaderCore(100, static_cast<uint8_t>(ContentType::IMAGE_JPEG), 0)), header));
    QCOMPARE(header.content_type, ContentType::IMAGE_JPEG);
    QCOMPARE(header.getContentTypeString(), QString("JPEG Image"));
}

void TestMOTProtocol::testContentType_PNG()
{
    MOTHeader header;
    QVERIFY(m_motParser->parseMOTHeader(
        toVector(buildHeaderCore(100, static_cast<uint8_t>(ContentType::IMAGE_PNG), 0)), header));
    QCOMPARE(header.content_type, ContentType::IMAGE_PNG);
    QCOMPARE(header.getContentTypeString(), QString("PNG Image"));
}

void TestMOTProtocol::testContentType_HTML()
{
    MOTHeader header;
    QVERIFY(m_motParser->parseMOTHeader(
        toVector(buildHeaderCore(100, static_cast<uint8_t>(ContentType::HTML), 0)), header));
    QCOMPARE(header.content_type, ContentType::HTML);
    QCOMPARE(header.getContentTypeString(), QString("HTML Document"));
}

// ============================================================================
// Transport ID management
// ============================================================================

void TestMOTProtocol::testTransportID_ValidRange()
{
    QVERIFY(m_motParser->validateTransportId(0));
    QVERIFY(m_motParser->validateTransportId(512));
    QVERIFY(m_motParser->validateTransportId(0x3FFF));
    QVERIFY(m_motParser->validateTransportId(0xFFFF));
}

void TestMOTProtocol::testTransportID_InvalidRange()
{
    QVERIFY(!m_motParser->validateTransportId(0x10000));
    QVERIFY(!m_motParser->validateTransportId(0x7FFFFFFF));
}

// ============================================================================
// Standard end-to-end reassembly through processMOTData
// ============================================================================

void TestMOTProtocol::testProcessMOTData_SingleObject()
{
    QSignalSpy spy(m_motParser, &MOTProtocol::objectComplete);
    QVERIFY(spy.isValid());

    const uint16_t tid = 0x0123;
    const QByteArray body = patternBody(300, 0x40);
    const QByteArray header = buildHeaderEntity(body.size(), 0x02, 0x01);

    QVERIFY(m_motParser->processMOTData(0x2C61, buildDataGroup(3, tid, 0, true, header)));
    QCOMPARE(spy.count(), 0);  // header alone must not complete an object
    QVERIFY(m_motParser->processMOTData(0x2C61, buildDataGroup(4, tid, 0, true, body)));

    QCOMPARE(spy.count(), 1);
    const QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).toUInt(), static_cast<uint>(tid));
    const MOTObject object = qvariant_cast<MOTObject>(args.at(1));
    QCOMPARE(object.transport_id, static_cast<uint32_t>(tid));
    QCOMPARE(objectBody(object), body);
    QCOMPARE(object.body.size(), static_cast<size_t>(body.size()));
    QCOMPARE(object.header.body_size, static_cast<uint32_t>(body.size()));
    QCOMPARE(static_cast<uint8_t>(object.header.content_type),
             static_cast<uint8_t>(ContentType::IMAGE_JPEG));
    QVERIFY(object.isValid());

    const auto stats = m_motParser->getStatistics();
    QCOMPARE(stats.data_groups_valid, static_cast<uint64_t>(2));
    QCOMPARE(stats.header_segments, static_cast<uint64_t>(1));
    QCOMPARE(stats.body_segments, static_cast<uint64_t>(1));
    QCOMPARE(stats.objects_completed, static_cast<uint64_t>(1));
    QCOMPARE(stats.objects_emitted, static_cast<uint64_t>(1));
    QCOMPARE(stats.crc_errors, static_cast<uint64_t>(0));
}

void TestMOTProtocol::testProcessMOTData_Full16BitTransportId()
{
    QSignalSpy spy(m_motParser, &MOTProtocol::objectComplete);

    const uint32_t tid = 14496;  // real-capture-like 16-bit id
    const QByteArray body = patternBody(20000, 0x33);  // multi-segment
    const QByteArray header = buildHeaderEntity(body.size(), 0x02, 0x03);

    QVERIFY(m_motParser->processMOTData(0x2C61, buildDataGroup(3, tid, 0, true, header)));
    const int segLen = 7000;
    for (int seg = 0, off = 0; off < body.size(); ++seg, off += segLen) {
        const bool last = (off + segLen >= body.size());
        QVERIFY(m_motParser->processMOTData(0x2C61, buildDataGroup(
            4, tid, static_cast<uint16_t>(seg), last, body.mid(off, segLen))));
    }

    QCOMPARE(spy.count(), 1);
    const MOTObject object = qvariant_cast<MOTObject>(spy.takeFirst().at(1));
    QCOMPARE(object.transport_id, tid);
    QCOMPARE(objectBody(object), body);
    QVERIFY(object.isValid());
}

void TestMOTProtocol::testProcessMOTData_HeaderParamsRestored()
{
    QSignalSpy spy(m_motParser, &MOTProtocol::objectComplete);

    const uint16_t tid = 0x0201;
    const QByteArray body = patternBody(128, 0x11);
    const QByteArray header = buildHeaderEntity(
        body.size(), 0x02, 0x03,
        {{0x0C, 0x3, textParam(0x0, QByteArray("slide.jpg"))},
         {0x26, 0x3, textParam(0x0, QByteArray("Weather"))},
         {0x27, 0x3, textParam(0x0, QByteArray("https://dab.example"))}});

    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(3, tid, 0, true, header)));
    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(4, tid, 0, true, body)));

    QCOMPARE(spy.count(), 1);
    const MOTObject object = qvariant_cast<MOTObject>(spy.takeFirst().at(1));
    // The standard header extension parameters survive end to end (no reframing).
    QCOMPARE(object.header.content_name, QString("slide.jpg"));
    QCOMPARE(object.header.category_title, QString("Weather"));
    QCOMPARE(object.header.click_through_url, QString("https://dab.example"));
    QCOMPARE(object.header.content_subtype, static_cast<uint16_t>(0x03));
    QCOMPARE(objectBody(object), body);
}

void TestMOTProtocol::testProcessMOTData_MultiSegmentOutOfOrder()
{
    QSignalSpy spy(m_motParser, &MOTProtocol::objectComplete);

    const uint16_t tid = 77;
    const QByteArray body = patternBody(500, 0x20);
    const QByteArray header = buildHeaderEntity(body.size(), 0x02, 0x01);

    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(3, tid, 0, true, header)));
    const QByteArray s0 = body.left(200);
    const QByteArray s1 = body.mid(200, 150);
    const QByteArray s2 = body.mid(350);
    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(4, tid, 2, true, s2)));
    QCOMPARE(spy.count(), 0);
    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(4, tid, 0, false, s0)));
    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(4, tid, 1, false, s1)));

    QCOMPARE(spy.count(), 1);
    const MOTObject object = qvariant_cast<MOTObject>(spy.takeFirst().at(1));
    QCOMPARE(objectBody(object), body);
}

void TestMOTProtocol::testProcessMOTData_DuplicateSegment()
{
    // F11: a duplicate body segment must be tolerated (ignored) and must not
    // corrupt the assembly or prevent completion.
    QSignalSpy spy(m_motParser, &MOTProtocol::objectComplete);

    const uint16_t tid = 0x0D01;
    const QByteArray body = patternBody(300, 0x3C);
    const QByteArray header = buildHeaderEntity(body.size(), 0x02, 0x01);

    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(3, tid, 0, true, header)));

    const QByteArray s0 = body.left(100);
    const QByteArray s1 = body.mid(100, 100);
    const QByteArray s2 = body.mid(200);

    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(4, tid, 0, false, s0)));
    // Exact duplicate of segment 0: accepted but ignored (no state change).
    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(4, tid, 0, false, s0)));
    QCOMPARE(spy.count(), 0);
    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(4, tid, 1, false, s1)));
    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(4, tid, 2, true, s2)));

    QCOMPARE(spy.count(), 1);
    const MOTObject object = qvariant_cast<MOTObject>(spy.takeFirst().at(1));
    QCOMPARE(objectBody(object), body);  // duplicate must not duplicate bytes
}

void TestMOTProtocol::testProcessMOTData_DuplicateHeaderRepeatNoReset()
{
    // F11: a byte-identical header retransmission is just a repeat and must NOT
    // count as a carousel reset / discard the in-flight body.
    const uint16_t tid = 0x0D02;
    const QByteArray body = patternBody(240, 0x2A);
    const QByteArray header = buildHeaderEntity(body.size(), 0x02, 0x01);

    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(3, tid, 0, true, header)));
    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(4, tid, 0, false, body.left(120))));

    // Identical header repeat mid-object.
    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(3, tid, 0, true, header)));
    QCOMPARE(m_motParser->getStatistics().object_resets, static_cast<uint64_t>(0));

    QSignalSpy spy(m_motParser, &MOTProtocol::objectComplete);
    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(4, tid, 1, true, body.mid(120))));
    QCOMPARE(spy.count(), 1);
    const MOTObject object = qvariant_cast<MOTObject>(spy.takeFirst().at(1));
    QCOMPARE(objectBody(object), body);
}

void TestMOTProtocol::testProcessMOTData_GapTolerance()
{
    QSignalSpy spy(m_motParser, &MOTProtocol::objectComplete);

    const uint16_t tid = 88;
    const QByteArray body = patternBody(300, 0x55);
    const QByteArray header = buildHeaderEntity(body.size(), 0x02, 0x01);

    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(3, tid, 0, true, header)));
    // Body segments 0 and 2 with segment 1 missing: must never complete.
    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(4, tid, 0, false, body.left(100))));
    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(4, tid, 2, true, body.mid(200))));
    QCOMPARE(spy.count(), 0);
    QVERIFY(!m_motParser->isComplete(tid));
}

void TestMOTProtocol::testProcessMOTData_CrcRejected()
{
    MOTProtocol mot;
    QSignalSpy spy(&mot, &MOTProtocol::objectComplete);

    const uint16_t tid = 9;
    const QByteArray body = patternBody(64, 0x01);
    const QByteArray header = buildHeaderEntity(body.size(), 0x02, 0x01);
    QByteArray badHeader = buildDataGroup(3, tid, 0, true, header);
    badHeader[badHeader.size() - 1] =
        static_cast<char>(badHeader[badHeader.size() - 1] ^ 0xFF);

    QVERIFY(!mot.processMOTData(0, badHeader));
    QVERIFY(mot.processMOTData(0, buildDataGroup(4, tid, 0, true, body)));
    QCOMPARE(spy.count(), 0);  // header never accepted -> no object
    QCOMPARE(mot.getStatistics().crc_errors, static_cast<uint64_t>(1));
}

void TestMOTProtocol::testProcessMOTData_CarouselReset()
{
    QSignalSpy spy(m_motParser, &MOTProtocol::objectComplete);

    const uint16_t tid = 9;
    const QByteArray body1 = patternBody(120, 0x10);
    const QByteArray body2 = patternBody(150, 0x60);

    // Cycle 1: header + body seg 0/1, but never the last segment.
    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(
        3, tid, 0, true, buildHeaderEntity(body1.size(), 0x02, 0x01))));
    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(4, tid, 0, false, body1.left(60))));
    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(4, tid, 1, false, body1.mid(60))));
    QCOMPARE(spy.count(), 0);

    // Cycle 2 (new object version, same transport id): a changed header segment 0
    // resets the stale assembly, then the new body completes.
    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(
        3, tid, 0, true, buildHeaderEntity(body2.size(), 0x02, 0x01))));
    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(4, tid, 0, false, body2.left(70))));
    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(4, tid, 1, true, body2.mid(70))));

    QCOMPARE(spy.count(), 1);
    const MOTObject object = qvariant_cast<MOTObject>(spy.takeFirst().at(1));
    QCOMPARE(objectBody(object), body2);  // cycle-1 bytes must NOT have merged in
    QVERIFY2(m_motParser->getStatistics().object_resets >= 1,
             "the carousel restart must reset the assembly");
}

void TestMOTProtocol::testProcessMOTData_OversizeRejected()
{
    MOTProtocol mot;
    MOTProtocol::Limits limits;
    limits.maxBodyBytes = 64;  // tiny cap for the boundary test
    mot.setLimits(limits);
    QSignalSpy spy(&mot, &MOTProtocol::objectComplete);

    const QByteArray body = patternBody(200, 0x01);
    const QByteArray header = buildHeaderEntity(body.size(), 0x02, 0x01);

    QVERIFY(mot.processMOTData(0, buildDataGroup(3, 5, 0, true, header)));
    // The 200-byte body segment must be rejected on the running counter.
    QVERIFY(!mot.processMOTData(0, buildDataGroup(4, 5, 0, true, body)));
    QCOMPARE(spy.count(), 0);
    QVERIFY2(mot.getStatistics().oversized >= 1, "oversize must be counted");
}

void TestMOTProtocol::testProcessMOTData_StaleEviction()
{
    MOTProtocol mot;
    MOTProtocol::Limits limits;
    limits.maxBodyBytes = 1u << 20;
    limits.maxAssemblies = 16;
    limits.staleTicks = 3;
    mot.setLimits(limits);

    const QByteArray header = buildHeaderEntity(100, 0x02, 0x01);
    QVERIFY(mot.processMOTData(0, buildDataGroup(3, 1, 0, true, header)));
    // Advance the tick with rejected (too-short) data groups.
    for (int i = 0; i < 6; ++i) {
        mot.processMOTData(0, QByteArray(1, '\0'));
    }
    // A new key triggers stale eviction of the idle tid-1 assembly.
    QVERIFY(mot.processMOTData(0, buildDataGroup(3, 2, 0, true, header)));
    QVERIFY2(mot.getStatistics().stale_evictions >= 1,
             "an idle incomplete assembly must be evicted");
}

void TestMOTProtocol::testProcessMOTData_MultipleObjects()
{
    QSignalSpy spy(m_motParser, &MOTProtocol::objectComplete);

    for (uint16_t tid = 1; tid <= 3; ++tid) {
        const QByteArray body = patternBody(50 + tid, static_cast<char>(tid));
        QVERIFY(m_motParser->processMOTData(0, buildDataGroup(
            3, tid, 0, true, buildHeaderEntity(body.size(), 0x02, 0x01))));
        QVERIFY(m_motParser->processMOTData(0, buildDataGroup(4, tid, 0, true, body)));
    }
    QCOMPARE(spy.count(), 3);
    QCOMPARE(m_motParser->getStatistics().objects_completed, static_cast<uint64_t>(3));
}

void TestMOTProtocol::testProcessMOTData_Progress()
{
    QSignalSpy progressSpy(m_motParser, &MOTProtocol::reassemblyProgress);
    QSignalSpy completeSpy(m_motParser, &MOTProtocol::objectComplete);

    const uint16_t tid = 0x0500;
    const QByteArray body = patternBody(400, 0x22);
    const QByteArray header = buildHeaderEntity(body.size(), 0x02, 0x01);

    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(3, tid, 0, true, header)));
    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(4, tid, 0, false, body.left(200))));

    QVERIFY(progressSpy.count() >= 1);
    const double fraction = progressSpy.last().at(1).toDouble();
    QVERIFY2(fraction > 0.0 && fraction < 1.0,
             qPrintable(QString("expected partial progress, got %1").arg(fraction)));
    QCOMPARE(completeSpy.count(), 0);

    QVERIFY(m_motParser->processMOTData(0, buildDataGroup(4, tid, 1, true, body.mid(200))));
    QCOMPARE(completeSpy.count(), 1);
}

// ============================================================================
// State management / statistics
// ============================================================================

void TestMOTProtocol::testState_ProgressAndComplete()
{
    const uint16_t tid = 0x0300;
    const QByteArray body = patternBody(200, 0x77);
    const QByteArray header = buildHeaderEntity(body.size(), 0x02, 0x01);

    QCOMPARE(m_motParser->getProgress(tid), -1.0);
    m_motParser->processMOTData(0, buildDataGroup(3, tid, 0, true, header));
    QCOMPARE(m_motParser->getProgress(tid), 0.0);
    m_motParser->processMOTData(0, buildDataGroup(4, tid, 0, false, body.left(100)));
    QCOMPARE(m_motParser->getProgress(tid), 0.5);
    QVERIFY(!m_motParser->isComplete(tid));

    auto active = m_motParser->getActiveTransportIds();
    QVERIFY(std::find(active.begin(), active.end(), tid) != active.end());

    m_motParser->processMOTData(0, buildDataGroup(4, tid, 1, true, body.mid(100)));
    // Completed objects are removed from the active set.
    active = m_motParser->getActiveTransportIds();
    QVERIFY(std::find(active.begin(), active.end(), tid) == active.end());
}

void TestMOTProtocol::testState_ClearAll()
{
    std::vector<uint32_t> ids = {0x100, 0x200, 0x300};
    for (uint32_t tid : ids) {
        m_motParser->processMOTData(0, buildDataGroup(
            3, tid, 0, true, buildHeaderEntity(100, 0x02, 0x01)));
    }
    QCOMPARE(m_motParser->getActiveTransportIds().size(), static_cast<size_t>(3));

    QSignalSpy clearedSpy(m_motParser, &MOTProtocol::objectCleared);
    m_motParser->clearObject(0x100);
    QCOMPARE(m_motParser->getActiveTransportIds().size(), static_cast<size_t>(2));
    QCOMPARE(clearedSpy.count(), 1);

    m_motParser->clearAll();
    QCOMPARE(m_motParser->getActiveTransportIds().size(), static_cast<size_t>(0));
}

void TestMOTProtocol::testState_ResetStatistics()
{
    const QByteArray body = patternBody(64, 0x01);
    m_motParser->processMOTData(0, buildDataGroup(
        3, 1, 0, true, buildHeaderEntity(body.size(), 0x02, 0x01)));
    m_motParser->processMOTData(0, buildDataGroup(4, 1, 0, true, body));
    QVERIFY(m_motParser->getStatistics().objects_completed >= 1);

    m_motParser->resetStatistics();
    QCOMPARE(m_motParser->getStatistics().objects_completed, static_cast<uint64_t>(0));
    QCOMPARE(m_motParser->getStatistics().data_groups_processed, static_cast<uint64_t>(0));
}

// ============================================================================
// MotPadAdapter thin shim
// ============================================================================

void TestMOTProtocol::testAdapter_ParseStandardHeader()
{
    const QByteArray header = buildHeaderCore(1234, 0x02, 0x01);
    uint32_t bodySize = 0;
    uint16_t headerSize = 0;
    uint8_t contentType = 0;
    uint16_t subType = 0;
    QVERIFY(MotPadAdapter::parseStandardHeader(header, bodySize, headerSize, contentType, subType));
    QCOMPARE(bodySize, static_cast<uint32_t>(1234));
    QCOMPARE(headerSize, static_cast<uint16_t>(7));
    QCOMPARE(contentType, static_cast<uint8_t>(0x02));
    QCOMPARE(subType, static_cast<uint16_t>(0x01));

    uint32_t bs = 0; uint16_t hs = 0; uint8_t ct = 0; uint16_t st = 0;
    QVERIFY(!MotPadAdapter::parseStandardHeader(QByteArray(6, '\0'), bs, hs, ct, st));
}

void TestMOTProtocol::testAdapter_ShimCompletesObject()
{
    MOTProtocol mot;
    MotPadAdapter adapter(&mot);
    QSignalSpy spy(&mot, &MOTProtocol::objectComplete);

    const uint16_t tid = 0x0123;
    const QByteArray body = patternBody(300, 0x40);
    const QByteArray header = buildHeaderEntity(body.size(), 0x02, 0x01);

    QVERIFY(adapter.processDataGroup(0x2C61, buildDataGroup(3, tid, 0, true, header)));
    QCOMPARE(spy.count(), 0);
    QVERIFY(adapter.processDataGroup(0x2C61, buildDataGroup(4, tid, 0, true, body)));

    QCOMPARE(spy.count(), 1);
    const MOTObject object = qvariant_cast<MOTObject>(spy.takeFirst().at(1));
    QCOMPARE(adapter.statistics().dataGroups, static_cast<uint64_t>(2));
    QCOMPARE(objectBody(object), body);
}

void TestMOTProtocol::testAdapter_StatisticsMapping()
{
    MOTProtocol mot;
    MotPadAdapter adapter(&mot);
    QSignalSpy spy(&mot, &MOTProtocol::objectComplete);

    const QByteArray body = patternBody(120, 0x05);
    QVERIFY(adapter.processDataGroup(0, buildDataGroup(
        3, 3, 0, true, buildHeaderEntity(body.size(), 0x02, 0x01))));
    QVERIFY(adapter.processDataGroup(0, buildDataGroup(4, 3, 0, true, body)));

    const MotPadAdapter::Statistics st = adapter.statistics();
    QCOMPARE(st.dataGroups, static_cast<uint64_t>(2));
    QCOMPARE(st.headerSegments, static_cast<uint64_t>(1));
    QCOMPARE(st.bodySegments, static_cast<uint64_t>(1));
    QCOMPARE(st.objectsCompleted, static_cast<uint64_t>(1));
    QCOMPARE(st.objectsEmitted, static_cast<uint64_t>(1));
    QCOMPARE(st.crcErrors, static_cast<uint64_t>(0));
    QCOMPARE(st.maxObjectBytes, static_cast<uint64_t>(body.size()));
}

// ============================================================================
// Test Execution
// ============================================================================

QTEST_MAIN(TestMOTProtocol)
#include "test_mot_protocol.moc"
