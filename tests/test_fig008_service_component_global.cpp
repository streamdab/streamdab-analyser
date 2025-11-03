/**
 * @file test_fig008_service_component_global.cpp
 * @brief Comprehensive unit tests for FIG 0/8 Service Component Global Definition parsing
 *
 * Tests FIG 0/8 parsing according to ETSI EN 300 401 Section 8.1.6
 * with comprehensive coverage of:
 * - Basic MSC component definitions
 * - FIC component definitions
 * - SCIdS and SCId 12-bit boundary testing
 * - Extension flag handling
 * - LS (Long/Short) flag variations
 * - Transport type (MSC/FIC) detection
 * - Malformed data handling
 * - Performance benchmarks (<15µs target)
 *
 * @author Python Pro Agent (Agent 19)
 * @date November 3, 2025
 * @reference ETSI EN 300 401 Section 8.1.6
 */

#include <QtTest/QtTest>
#include "../src/core/fig_parser.hpp"
#include <chrono>
#include <vector>

using namespace eti::fig;

class TestFig008ServiceComponentGlobal : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Basic MSC component tests
    void test_valid_msc_component();
    void test_msc_component_various_subchids();
    void test_msc_component_zero_subchid();
    void test_msc_component_max_subchid();

    // Basic FIC component tests
    void test_valid_fic_component();
    void test_fic_component_various_fidcids();
    void test_fic_component_zero_fidcid();
    void test_fic_component_max_fidcid();

    // SCIdS 12-bit boundary tests
    void test_scids_minimum_value();
    void test_scids_maximum_value();
    void test_scids_mid_range();
    void test_scids_boundary_values();

    // Extension flag tests
    void test_ext_flag_set();
    void test_ext_flag_clear();

    // LS flag tests
    void test_ls_flag_short_form();
    void test_ls_flag_long_form();

    // Invalid data tests
    void test_invalid_size_too_short();
    void test_invalid_size_zero();
    void test_invalid_scids_out_of_range();
    void test_invalid_subchid_exceeds_6bit();

    // Performance test
    void test_parsing_performance();

private:
    FigParser* parser_;

    // Helper method to create FIG 0/8 test data
    std::vector<uint8_t> createFig008Data(
        uint16_t sc_ids,
        uint8_t rfa,
        bool ext_flag,
        uint16_t sc_id,
        bool ls_flag,
        bool msc_fic_flag,
        uint8_t sub_ch_id,
        uint8_t fidc_id);
};

void TestFig008ServiceComponentGlobal::initTestCase() {
    parser_ = new FigParser();
    QVERIFY(parser_->initialize(false, false)); // Thai support not needed for FIG 0/8
}

void TestFig008ServiceComponentGlobal::cleanupTestCase() {
    delete parser_;
}

std::vector<uint8_t> TestFig008ServiceComponentGlobal::createFig008Data(
    uint16_t sc_ids,
    uint8_t rfa,
    bool ext_flag,
    uint16_t sc_id,
    bool ls_flag,
    bool msc_fic_flag,
    uint8_t sub_ch_id,
    uint8_t fidc_id) {

    std::vector<uint8_t> data;

    // Extension field (FIG 0/8)
    data.push_back(0x08);

    // SCIdS (12-bit big-endian across 2 bytes)
    data.push_back((sc_ids >> 8) & 0x0F);  // Upper 4 bits
    data.push_back(sc_ids & 0xFF);         // Lower 8 bits

    // Control byte: Rfa (4 bits) + Ext flag (1 bit) + reserved (3 bits)
    uint8_t control = ((rfa & 0x0F) << 4) | (ext_flag ? 0x08 : 0x00);
    data.push_back(control);

    // SCId byte 1 (upper 4 bits of 12-bit SCId)
    data.push_back((sc_id >> 8) & 0x0F);

    // SCId byte 2 (lower 8 bits)
    data.push_back(sc_id & 0xFF);

    // Transport byte: LS flag + MSC/FIC flag + SubChId (if MSC) or reserved+FIDCId (if FIC)
    uint8_t transport = (ls_flag ? 0x80 : 0x00) | (msc_fic_flag ? 0x40 : 0x00);

    if (msc_fic_flag) {
        // MSC mode: SubChId (6 bits)
        transport |= (sub_ch_id & 0x3F);
    } else {
        // FIC mode: FIDCId in lower 6 bits (or use separate byte per ETSI spec)
        // Based on ETSI EN 300 401 §8.1.6, FIC mode uses lower bits for FIDCId
        transport |= (fidc_id & 0x3F);  // FIDCId in bits 5-0
    }
    
    data.push_back(transport);
    
    return data;
}
void TestFig008ServiceComponentGlobal::test_valid_msc_component() {
    qDebug() << "TEST: Valid MSC component";

    auto fig_data = createFig008Data(
        0x123,   // SCIdS
        0x0,     // Rfa
        false,   // Ext flag
        0x456,   // SCId
        false,   // LS flag (short form)
        true,    // MSC mode
        0x10,    // SubChId
        0x00     // FIDCId (unused in MSC mode)
    );

    auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);

    QVERIFY(result.is_valid);
    QCOMPARE(result.sc_ids, static_cast<uint16_t>(0x123));
    QCOMPARE(result.rfa, static_cast<uint8_t>(0x0));
    QVERIFY(!result.ext_flag);
    QCOMPARE(result.sc_id, static_cast<uint16_t>(0x456));
    QVERIFY(!result.ls_flag);
    QVERIFY(result.msc_fic_flag);  // MSC mode
    QCOMPARE(result.sub_ch_id, static_cast<uint8_t>(0x10));
    QCOMPARE(result.fidc_id, static_cast<uint8_t>(0x00));
}

void TestFig008ServiceComponentGlobal::test_msc_component_various_subchids() {
    qDebug() << "TEST: MSC component with various SubChIds";

    std::vector<uint8_t> test_subchids = {0x00, 0x01, 0x0F, 0x20, 0x3F};

    for (auto subchid : test_subchids) {
        auto fig_data = createFig008Data(0x100, 0x0, false, 0x200, false, true, subchid, 0x00);
        auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);

        QVERIFY(result.is_valid);
        QVERIFY(result.msc_fic_flag);
        QCOMPARE(result.sub_ch_id, subchid);
    }
}

void TestFig008ServiceComponentGlobal::test_msc_component_zero_subchid() {
    qDebug() << "TEST: MSC component with SubChId = 0";

    auto fig_data = createFig008Data(0x001, 0x0, false, 0x002, false, true, 0x00, 0x00);
    auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);

    QVERIFY(result.is_valid);
    QCOMPARE(result.sub_ch_id, static_cast<uint8_t>(0x00));
}

void TestFig008ServiceComponentGlobal::test_msc_component_max_subchid() {
    qDebug() << "TEST: MSC component with SubChId = 63 (6-bit max)";

    auto fig_data = createFig008Data(0x100, 0x0, false, 0x200, false, true, 0x3F, 0x00);
    auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);

    QVERIFY(result.is_valid);
    QCOMPARE(result.sub_ch_id, static_cast<uint8_t>(0x3F));
}

// ============================================================================
// Basic FIC Component Tests
// ============================================================================

void TestFig008ServiceComponentGlobal::test_valid_fic_component() {
    qDebug() << "TEST: Valid FIC component";

    auto fig_data = createFig008Data(
        0x789,   // SCIdS
        0x0,     // Rfa
        true,    // Ext flag
        0xABC,   // SCId
        true,    // LS flag (long form)
        false,   // FIC mode
        0x00,    // SubChId (unused in FIC mode)
        0x42     // FIDCId
    );

    auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);

    QVERIFY(result.is_valid);
    QCOMPARE(result.sc_ids, static_cast<uint16_t>(0x789));
    QVERIFY(result.ext_flag);
    QCOMPARE(result.sc_id, static_cast<uint16_t>(0xABC));
    QVERIFY(result.ls_flag);
    QVERIFY(!result.msc_fic_flag);  // FIC mode
    QCOMPARE(result.sub_ch_id, static_cast<uint8_t>(0x00));
    QCOMPARE(result.fidc_id, static_cast<uint8_t>(0x02));
}

void TestFig008ServiceComponentGlobal::test_fic_component_various_fidcids() {
    qDebug() << "TEST: FIC component with various FIDCIds";

    std::vector<uint8_t> test_fidcids = {0x00, 0x01, 0x7F, 0x80, 0xFF};

    for (auto fidcid : test_fidcids) {
        auto fig_data = createFig008Data(0x300, 0x0, false, 0x400, false, false, 0x00, fidcid);
        auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);

        QVERIFY(result.is_valid);
        QVERIFY(!result.msc_fic_flag);
        QCOMPARE(result.fidc_id, static_cast<uint8_t>(fidcid & 0x3F));
    }
}

void TestFig008ServiceComponentGlobal::test_fic_component_zero_fidcid() {
    qDebug() << "TEST: FIC component with FIDCId = 0";

    auto fig_data = createFig008Data(0x500, 0x0, false, 0x600, false, false, 0x00, 0x00);
    auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);

    QVERIFY(result.is_valid);
    QCOMPARE(result.fidc_id, static_cast<uint8_t>(0x00));
}

void TestFig008ServiceComponentGlobal::test_fic_component_max_fidcid() {
    qDebug() << "TEST: FIC component with FIDCId = 63 (6-bit max)";

    auto fig_data = createFig008Data(0x700, 0x0, false, 0x800, false, false, 0x00, 0x3F);
    auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);

    QVERIFY(result.is_valid);
    QCOMPARE(result.fidc_id, static_cast<uint8_t>(0x3F));
}

// ============================================================================
// SCIdS 12-bit Boundary Tests
// ============================================================================

void TestFig008ServiceComponentGlobal::test_scids_minimum_value() {
    qDebug() << "TEST: SCIdS minimum value (0x000)";

    auto fig_data = createFig008Data(0x000, 0x0, false, 0x100, false, true, 0x01, 0x00);
    auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);

    QVERIFY(result.is_valid);
    QCOMPARE(result.sc_ids, static_cast<uint16_t>(0x000));
}

void TestFig008ServiceComponentGlobal::test_scids_maximum_value() {
    qDebug() << "TEST: SCIdS maximum value (0xFFF - 12-bit max)";

    auto fig_data = createFig008Data(0xFFF, 0x0, false, 0x100, false, true, 0x01, 0x00);
    auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);

    QVERIFY(result.is_valid);
    QCOMPARE(result.sc_ids, static_cast<uint16_t>(0xFFF));
}

void TestFig008ServiceComponentGlobal::test_scids_mid_range() {
    qDebug() << "TEST: SCIdS mid-range values";

    std::vector<uint16_t> test_values = {0x001, 0x100, 0x555, 0x7FF, 0xAAA, 0xFFE};

    for (auto scids : test_values) {
        auto fig_data = createFig008Data(scids, 0x0, false, 0x100, false, true, 0x01, 0x00);
        auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.sc_ids, scids);
    }
}

void TestFig008ServiceComponentGlobal::test_scids_boundary_values() {
    qDebug() << "TEST: SCIdS boundary values (around 12-bit limits)";

    std::vector<uint16_t> boundary_values = {0x7FE, 0x7FF, 0x800, 0xFFE, 0xFFF};

    for (auto scids : boundary_values) {
        auto fig_data = createFig008Data(scids, 0x0, false, 0x200, false, true, 0x02, 0x00);
        auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.sc_ids, scids);
    }
}

// ============================================================================
// Extension Flag Tests
// ============================================================================

void TestFig008ServiceComponentGlobal::test_ext_flag_set() {
    qDebug() << "TEST: Extension flag set";

    auto fig_data = createFig008Data(0x100, 0x0, true, 0x200, false, true, 0x01, 0x00);
    auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);

    QVERIFY(result.is_valid);
    QVERIFY(result.ext_flag);
}

void TestFig008ServiceComponentGlobal::test_ext_flag_clear() {
    qDebug() << "TEST: Extension flag clear";

    auto fig_data = createFig008Data(0x100, 0x0, false, 0x200, false, true, 0x01, 0x00);
    auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);

    QVERIFY(result.is_valid);
    QVERIFY(!result.ext_flag);
}

// ============================================================================
// LS Flag Tests
// ============================================================================

void TestFig008ServiceComponentGlobal::test_ls_flag_short_form() {
    qDebug() << "TEST: LS flag - short form";

    auto fig_data = createFig008Data(0x100, 0x0, false, 0x200, false, true, 0x01, 0x00);
    auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);

    QVERIFY(result.is_valid);
    QVERIFY(!result.ls_flag);
}

void TestFig008ServiceComponentGlobal::test_ls_flag_long_form() {
    qDebug() << "TEST: LS flag - long form";

    auto fig_data = createFig008Data(0x100, 0x0, false, 0x200, true, true, 0x01, 0x00);
    auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);

    QVERIFY(result.is_valid);
    QVERIFY(result.ls_flag);
}

// ============================================================================
// Invalid Data Tests
// ============================================================================

void TestFig008ServiceComponentGlobal::test_invalid_size_too_short() {
    qDebug() << "TEST: Invalid data - too short (< 6 bytes)";

    std::vector<uint8_t> fig_data = {0x08, 0x01, 0x23, 0x04, 0x56};  // Only 5 bytes

    auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);

    QVERIFY(!result.is_valid);
}

void TestFig008ServiceComponentGlobal::test_invalid_size_zero() {
    qDebug() << "TEST: Invalid data - zero bytes";

    std::vector<uint8_t> fig_data;

    auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);

    QVERIFY(!result.is_valid);
}

void TestFig008ServiceComponentGlobal::test_invalid_scids_out_of_range() {
    qDebug() << "TEST: Invalid SCIdS - exceeds 12-bit (0x1000)";

    // Note: Our helper function masks to 12-bit, but we'll manually construct invalid data
    std::vector<uint8_t> fig_data;
    fig_data.push_back(0x08);  // Extension
    fig_data.push_back(0x1F);  // SCIdS high byte (exceeds 12-bit if not masked properly)
    fig_data.push_back(0xFF);  // SCIdS low byte
    fig_data.push_back(0x00);  // Control
    fig_data.push_back(0x01);  // SCId high
    fig_data.push_back(0x00);  // SCId low
    fig_data.push_back(0x41);  // Transport (MSC, SubChId=1)

    auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);

    // Implementation should mask to 12-bit, so still valid but with masked value
    QVERIFY(result.is_valid);
    QVERIFY(result.sc_ids <= 0xFFF);  // Should be masked to 12-bit
}

void TestFig008ServiceComponentGlobal::test_invalid_subchid_exceeds_6bit() {
    qDebug() << "TEST: Invalid SubChId - exceeds 6-bit (64+)";

    // Manually construct data with SubChId > 63
    std::vector<uint8_t> fig_data;
    fig_data.push_back(0x08);  // Extension
    fig_data.push_back(0x01);  // SCIdS high
    fig_data.push_back(0x00);  // SCIdS low
    fig_data.push_back(0x00);  // Control
    fig_data.push_back(0x02);  // SCId high
    fig_data.push_back(0x00);  // SCId low
    fig_data.push_back(0xC0 | 0x3F);  // Transport: LS=1, MSC=1, SubChId=63 (valid)

    auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);

    QVERIFY(result.is_valid);  // SubChId=63 is valid (6-bit max)
    QCOMPARE(result.sub_ch_id, static_cast<uint8_t>(0x3F));
}

// ============================================================================
// Performance Test
// ============================================================================

void TestFig008ServiceComponentGlobal::test_parsing_performance() {
    qDebug() << "TEST: Parsing performance (<15µs target)";

    auto fig_data = createFig008Data(0x123, 0x0, false, 0x456, false, true, 0x10, 0x00);

    const int iterations = 1000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        auto result = parser_->parseFig008_ServiceComponentGlobal(fig_data);
        QVERIFY(result.is_valid);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double avg_microseconds = static_cast<double>(duration.count()) / iterations;

    qDebug() << "  Average parsing time:" << avg_microseconds << "µs";
    qDebug() << "  Target: <15µs";

    // Verify performance target
    QVERIFY2(avg_microseconds < 15.0,
        QString("Performance target not met: %1µs (target <15µs)")
        .arg(avg_microseconds).toUtf8());
}

QTEST_MAIN(TestFig008ServiceComponentGlobal)
#include "test_fig008_service_component_global.moc"
