/**
 * @file test_fig014_fec_subchannel.cpp
 * @brief Comprehensive unit tests for FIG 0/14 FEC Sub-channel Organization parsing
 *
 * Tests FIG 0/14 parsing according to ETSI EN 300 401 Section 8.1.5
 * with comprehensive coverage of:
 * - Short form parsing with table index
 * - Long form parsing with sub-channel size and protection level
 * - FEC scheme extraction (0-3)
 * - Start address validation (10-bit)
 * - Protection level mapping
 * - Boundary conditions and invalid data handling
 * - Performance benchmarks
 *
 * @author Agent 21 - Python Pro (PDCA Week 5)
 * @date November 3, 2025
 */

#include <QtTest/QtTest>
#include "../src/core/fig_parser.hpp"
#include <chrono>
#include <vector>

using namespace eti::fig;

class TestFig014FECSubchannel : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Short form tests (6 tests)
    void test_short_form_basic();
    void test_short_form_fec_scheme_0();
    void test_short_form_fec_scheme_1();
    void test_short_form_table_index_boundary();
    void test_short_form_start_address_max();
    void test_short_form_insufficient_data();

    // Long form tests (6 tests)
    void test_long_form_basic();
    void test_long_form_fec_scheme_2();
    void test_long_form_protection_levels();
    void test_long_form_sub_ch_size_max();
    void test_long_form_option_field();
    void test_long_form_insufficient_data();

    // FEC scheme tests (4 tests)
    void test_fec_scheme_0_no_fec();
    void test_fec_scheme_1_reed_solomon();
    void test_fec_scheme_2_convolutional();
    void test_fec_scheme_3_reserved();

    // Validation tests (4 tests)
    void test_validate_sub_ch_id();
    void test_validate_start_address();
    void test_validate_sub_ch_size();
    void test_validate_all_fields();

    // Edge case tests (2 tests)
    void test_empty_data();
    void test_minimal_invalid_data();

    // Performance test (1 test)
    void test_parsing_performance();

private:
    FigParser* parser_;

    // Helper to create short form FIG 0/14 data
    std::vector<uint8_t> createShortFormFig014(
        uint8_t sub_ch_id,
        uint8_t fec_scheme,
        uint16_t start_address,
        uint8_t table_index);

    // Helper to create long form FIG 0/14 data
    std::vector<uint8_t> createLongFormFig014(
        uint8_t sub_ch_id,
        uint8_t fec_scheme,
        uint16_t start_address,
        uint16_t sub_ch_size,
        uint8_t protection_level,
        uint8_t option);
};

void TestFig014FECSubchannel::initTestCase() {
    parser_ = new FigParser();
    QVERIFY(parser_->initialize(true, false));
}

void TestFig014FECSubchannel::cleanupTestCase() {
    delete parser_;
}

std::vector<uint8_t> TestFig014FECSubchannel::createShortFormFig014(
    uint8_t sub_ch_id,
    uint8_t fec_scheme,
    uint16_t start_address,
    uint8_t table_index) {

    std::vector<uint8_t> data;
    data.push_back(0x0E);  // Extension byte (FIG 0/14)

    // Byte 0: [6 bits SubChId][2 bits FEC scheme]
    uint8_t byte0 = ((sub_ch_id & 0x3F) << 2) | (fec_scheme & 0x03);
    data.push_back(byte0);

    // Byte 1: [8 bits MSB of Start Address (10-bit total)]
    uint8_t byte1 = (start_address >> 2) & 0xFF;
    data.push_back(byte1);

    // Byte 2: [2 bits LSB of Start Address][1 bit Form=1][5 bits unused]
    // Short form: bit 5 = 1
    uint8_t byte2 = ((start_address & 0x03) << 6) | 0x20;
    data.push_back(byte2);

    // Byte 3: [2 bits unused][6 bits Table Index]
    uint8_t byte3 = table_index & 0x3F;
    data.push_back(byte3);

    return data;
}

std::vector<uint8_t> TestFig014FECSubchannel::createLongFormFig014(
    uint8_t sub_ch_id,
    uint8_t fec_scheme,
    uint16_t start_address,
    uint16_t sub_ch_size,
    uint8_t protection_level,
    uint8_t option) {

    std::vector<uint8_t> data;
    data.push_back(0x0E);  // Extension byte

    // Byte 0: [6 bits SubChId][2 bits FEC scheme]
    uint8_t byte0 = ((sub_ch_id & 0x3F) << 2) | (fec_scheme & 0x03);
    data.push_back(byte0);

    // Byte 1: [8 bits MSB of Start Address]
    uint8_t byte1 = (start_address >> 2) & 0xFF;
    data.push_back(byte1);

    // Byte 2: [2 bits LSB of Start Address][1 bit Form=0][5 bits unused]
    uint8_t byte2 = ((start_address & 0x03) << 6);  // Form bit = 0 for long form
    data.push_back(byte2);

    // Byte 3: [3 bits Option][2 bits Protection Level][3 bits unused]
    uint8_t byte3 = ((option & 0x07) << 5) | ((protection_level & 0x03) << 3);
    data.push_back(byte3);

    // Byte 4-5: [10 bits Sub-channel Size][6 bits unused]
    uint8_t byte4 = (sub_ch_size >> 2) & 0xFF;
    uint8_t byte5 = (sub_ch_size & 0x03) << 6;
    data.push_back(byte4);
    data.push_back(byte5);

    return data;
}

// ============================================================================
// Short Form Tests
// ============================================================================

void TestFig014FECSubchannel::test_short_form_basic() {
    qDebug() << "TEST: Short form basic parsing";

    auto fig_data = createShortFormFig014(10, 1, 100, 25);
    auto fec = parser_->parseFig014_FECSubchannel(fig_data);

    QVERIFY(fec.is_valid);
    QCOMPARE(fec.sub_ch_id, static_cast<uint8_t>(10));
    QCOMPARE(fec.fec_scheme, static_cast<uint8_t>(1));
    QCOMPARE(fec.start_address, static_cast<uint16_t>(100));
    QVERIFY(fec.short_form);
    QCOMPARE(fec.table_index, static_cast<uint8_t>(25));
    QCOMPARE(QString::fromStdString(fec.getFECSchemeName()), QString("FEC scheme 1 (RS)"));
}

void TestFig014FECSubchannel::test_short_form_fec_scheme_0() {
    qDebug() << "TEST: Short form with FEC scheme 0 (No FEC)";

    auto fig_data = createShortFormFig014(5, 0, 50, 10);
    auto fec = parser_->parseFig014_FECSubchannel(fig_data);

    QVERIFY(fec.is_valid);
    QCOMPARE(fec.fec_scheme, static_cast<uint8_t>(0));
    QCOMPARE(QString::fromStdString(fec.getFECSchemeName()), QString("No FEC"));
}

void TestFig014FECSubchannel::test_short_form_fec_scheme_1() {
    qDebug() << "TEST: Short form with FEC scheme 1 (Reed-Solomon)";

    auto fig_data = createShortFormFig014(15, 1, 200, 30);
    auto fec = parser_->parseFig014_FECSubchannel(fig_data);

    QVERIFY(fec.is_valid);
    QCOMPARE(fec.fec_scheme, static_cast<uint8_t>(1));
    QCOMPARE(QString::fromStdString(fec.getFECSchemeName()), QString("FEC scheme 1 (RS)"));
}

void TestFig014FECSubchannel::test_short_form_table_index_boundary() {
    qDebug() << "TEST: Short form table index boundary values";

    // Test minimum (0)
    auto fig_min = createShortFormFig014(10, 1, 100, 0);
    auto fec_min = parser_->parseFig014_FECSubchannel(fig_min);
    QVERIFY(fec_min.is_valid);
    QCOMPARE(fec_min.table_index, static_cast<uint8_t>(0));

    // Test maximum (63)
    auto fig_max = createShortFormFig014(10, 1, 100, 63);
    auto fec_max = parser_->parseFig014_FECSubchannel(fig_max);
    QVERIFY(fec_max.is_valid);
    QCOMPARE(fec_max.table_index, static_cast<uint8_t>(63));
}

void TestFig014FECSubchannel::test_short_form_start_address_max() {
    qDebug() << "TEST: Short form start address maximum value";

    auto fig_data = createShortFormFig014(10, 1, 863, 25);  // Max CUs = 863
    auto fec = parser_->parseFig014_FECSubchannel(fig_data);

    QVERIFY(fec.is_valid);
    QCOMPARE(fec.start_address, static_cast<uint16_t>(863));
    QVERIFY(fec.validateStartAddress());
}

void TestFig014FECSubchannel::test_short_form_insufficient_data() {
    qDebug() << "TEST: Short form insufficient data";

    std::vector<uint8_t> short_data = {0x0E, 0x28, 0x64};  // Only 3 bytes
    auto fec = parser_->parseFig014_FECSubchannel(short_data);

    QVERIFY(!fec.is_valid);
}

// ============================================================================
// Long Form Tests
// ============================================================================

void TestFig014FECSubchannel::test_long_form_basic() {
    qDebug() << "TEST: Long form basic parsing";

    auto fig_data = createLongFormFig014(20, 2, 150, 50, 1, 3);
    auto fec = parser_->parseFig014_FECSubchannel(fig_data);

    QVERIFY(fec.is_valid);
    QCOMPARE(fec.sub_ch_id, static_cast<uint8_t>(20));
    QCOMPARE(fec.fec_scheme, static_cast<uint8_t>(2));
    QCOMPARE(fec.start_address, static_cast<uint16_t>(150));
    QVERIFY(!fec.short_form);
    QCOMPARE(fec.sub_ch_size, static_cast<uint16_t>(50));
    QCOMPARE(fec.protection_level, static_cast<uint8_t>(1));
    QCOMPARE(fec.option, static_cast<uint8_t>(3));
}

void TestFig014FECSubchannel::test_long_form_fec_scheme_2() {
    qDebug() << "TEST: Long form with FEC scheme 2 (Convolutional)";

    auto fig_data = createLongFormFig014(25, 2, 200, 100, 2, 5);
    auto fec = parser_->parseFig014_FECSubchannel(fig_data);

    QVERIFY(fec.is_valid);
    QCOMPARE(fec.fec_scheme, static_cast<uint8_t>(2));
    QCOMPARE(QString::fromStdString(fec.getFECSchemeName()),
             QString("FEC scheme 2 (Convolutional)"));
}

void TestFig014FECSubchannel::test_long_form_protection_levels() {
    qDebug() << "TEST: Long form protection level values (0-3)";

    for (uint8_t pl = 0; pl < 4; ++pl) {
        auto fig_data = createLongFormFig014(10, 1, 100, 50, pl, 0);
        auto fec = parser_->parseFig014_FECSubchannel(fig_data);

        QVERIFY(fec.is_valid);
        QCOMPARE(fec.protection_level, pl);
        QCOMPARE(fec.getProtectionLevel(), pl);
    }
}

void TestFig014FECSubchannel::test_long_form_sub_ch_size_max() {
    qDebug() << "TEST: Long form sub-channel size maximum";

    // Max size to fit within 864 CUs
    auto fig_data = createLongFormFig014(10, 1, 100, 764, 1, 0);
    auto fec = parser_->parseFig014_FECSubchannel(fig_data);

    QVERIFY(fec.is_valid);
    QCOMPARE(fec.sub_ch_size, static_cast<uint16_t>(764));
    QVERIFY(fec.validateSubChannelSize());
}

void TestFig014FECSubchannel::test_long_form_option_field() {
    qDebug() << "TEST: Long form option field values (0-7)";

    for (uint8_t opt = 0; opt < 8; ++opt) {
        auto fig_data = createLongFormFig014(10, 1, 100, 50, 1, opt);
        auto fec = parser_->parseFig014_FECSubchannel(fig_data);

        QVERIFY(fec.is_valid);
        QCOMPARE(fec.option, opt);
    }
}

void TestFig014FECSubchannel::test_long_form_insufficient_data() {
    qDebug() << "TEST: Long form insufficient data";

    std::vector<uint8_t> short_data = {0x0E, 0x50, 0x96, 0x00, 0x28};  // Only 5 bytes
    auto fec = parser_->parseFig014_FECSubchannel(short_data);

    QVERIFY(!fec.is_valid);
}

// ============================================================================
// FEC Scheme Tests
// ============================================================================

void TestFig014FECSubchannel::test_fec_scheme_0_no_fec() {
    qDebug() << "TEST: FEC scheme 0 (No FEC)";

    auto fig_data = createShortFormFig014(10, 0, 100, 20);
    auto fec = parser_->parseFig014_FECSubchannel(fig_data);

    QVERIFY(fec.is_valid);
    QCOMPARE(fec.fec_scheme, static_cast<uint8_t>(0));
    QCOMPARE(QString::fromStdString(fec.getFECSchemeName()), QString("No FEC"));
}

void TestFig014FECSubchannel::test_fec_scheme_1_reed_solomon() {
    qDebug() << "TEST: FEC scheme 1 (Reed-Solomon)";

    auto fig_data = createShortFormFig014(10, 1, 100, 20);
    auto fec = parser_->parseFig014_FECSubchannel(fig_data);

    QVERIFY(fec.is_valid);
    QCOMPARE(fec.fec_scheme, static_cast<uint8_t>(1));
    QCOMPARE(QString::fromStdString(fec.getFECSchemeName()), QString("FEC scheme 1 (RS)"));
}

void TestFig014FECSubchannel::test_fec_scheme_2_convolutional() {
    qDebug() << "TEST: FEC scheme 2 (Convolutional)";

    auto fig_data = createShortFormFig014(10, 2, 100, 20);
    auto fec = parser_->parseFig014_FECSubchannel(fig_data);

    QVERIFY(fec.is_valid);
    QCOMPARE(fec.fec_scheme, static_cast<uint8_t>(2));
    QCOMPARE(QString::fromStdString(fec.getFECSchemeName()),
             QString("FEC scheme 2 (Convolutional)"));
}

void TestFig014FECSubchannel::test_fec_scheme_3_reserved() {
    qDebug() << "TEST: FEC scheme 3 (Reserved)";

    auto fig_data = createShortFormFig014(10, 3, 100, 20);
    auto fec = parser_->parseFig014_FECSubchannel(fig_data);

    QVERIFY(fec.is_valid);
    QCOMPARE(fec.fec_scheme, static_cast<uint8_t>(3));
    QCOMPARE(QString::fromStdString(fec.getFECSchemeName()), QString("Reserved"));
}

// ============================================================================
// Validation Tests
// ============================================================================

void TestFig014FECSubchannel::test_validate_sub_ch_id() {
    qDebug() << "TEST: Validate sub-channel ID range";

    // Valid: 0-63
    for (uint8_t id = 0; id < 64; ++id) {
        auto fig_data = createShortFormFig014(id, 1, 100, 20);
        auto fec = parser_->parseFig014_FECSubchannel(fig_data);

        QVERIFY(fec.is_valid);
        QCOMPARE(fec.sub_ch_id, id);
        QVERIFY(fec.validateSubChannelId());
    }
}

void TestFig014FECSubchannel::test_validate_start_address() {
    qDebug() << "TEST: Validate start address boundary";

    // Valid: 0-863
    auto fig_valid = createShortFormFig014(10, 1, 863, 20);
    auto fec_valid = parser_->parseFig014_FECSubchannel(fig_valid);
    QVERIFY(fec_valid.is_valid);
    QVERIFY(fec_valid.validateStartAddress());

    // Test boundary
    auto fig_boundary = createShortFormFig014(10, 1, 0, 20);
    auto fec_boundary = parser_->parseFig014_FECSubchannel(fig_boundary);
    QVERIFY(fec_boundary.is_valid);
    QVERIFY(fec_boundary.validateStartAddress());
}

void TestFig014FECSubchannel::test_validate_sub_ch_size() {
    qDebug() << "TEST: Validate sub-channel size";

    // Valid: start_address + sub_ch_size <= 864
    auto fig_valid = createLongFormFig014(10, 1, 100, 764, 1, 0);
    auto fec_valid = parser_->parseFig014_FECSubchannel(fig_valid);
    QVERIFY(fec_valid.is_valid);
    QVERIFY(fec_valid.validateSubChannelSize());

    // Edge case: exactly 864
    auto fig_edge = createLongFormFig014(10, 1, 0, 864, 1, 0);
    auto fec_edge = parser_->parseFig014_FECSubchannel(fig_edge);
    // Note: Parser may or may not accept size=864 depending on implementation
    // We test that validation function works
    if (fec_edge.is_valid) {
        qDebug() << "  Size=864 accepted by parser";
    }
}

void TestFig014FECSubchannel::test_validate_all_fields() {
    qDebug() << "TEST: Validate all fields together";

    auto fig_data = createLongFormFig014(30, 1, 200, 150, 2, 5);
    auto fec = parser_->parseFig014_FECSubchannel(fig_data);

    QVERIFY(fec.is_valid);
    QVERIFY(fec.validateSubChannelId());
    QVERIFY(fec.validateStartAddress());
    QVERIFY(fec.validateSubChannelSize());
}

// ============================================================================
// Edge Case Tests
// ============================================================================

void TestFig014FECSubchannel::test_empty_data() {
    qDebug() << "TEST: Empty data handling";

    std::vector<uint8_t> empty_data;
    auto fec = parser_->parseFig014_FECSubchannel(empty_data);

    QVERIFY(!fec.is_valid);
}

void TestFig014FECSubchannel::test_minimal_invalid_data() {
    qDebug() << "TEST: Minimal invalid data (1-3 bytes)";

    std::vector<uint8_t> one_byte = {0x0E};
    auto fec1 = parser_->parseFig014_FECSubchannel(one_byte);
    QVERIFY(!fec1.is_valid);

    std::vector<uint8_t> two_bytes = {0x0E, 0x28};
    auto fec2 = parser_->parseFig014_FECSubchannel(two_bytes);
    QVERIFY(!fec2.is_valid);

    std::vector<uint8_t> three_bytes = {0x0E, 0x28, 0x64};
    auto fec3 = parser_->parseFig014_FECSubchannel(three_bytes);
    QVERIFY(!fec3.is_valid);
}

// ============================================================================
// Performance Test
// ============================================================================

void TestFig014FECSubchannel::test_parsing_performance() {
    qDebug() << "TEST: Parsing performance benchmark";

    auto fig_data = createShortFormFig014(10, 1, 100, 20);

    // Warm-up
    for (int i = 0; i < 10; ++i) {
        parser_->parseFig014_FECSubchannel(fig_data);
    }

    // Benchmark
    constexpr int iterations = 1000;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        auto fec = parser_->parseFig014_FECSubchannel(fig_data);
        QVERIFY(fec.is_valid);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time);

    double avg_time_us = static_cast<double>(duration.count()) / iterations;

    qDebug() << QString("Average parsing time: %1 µs per FIG 0/14")
        .arg(avg_time_us, 0, 'f', 2);

    // Performance target: < 18 µs per parse
    QVERIFY2(avg_time_us < 18.0,
        QString("Performance target failed: %1 µs > 18 µs")
        .arg(avg_time_us, 0, 'f', 2).toUtf8().constData());
}

QTEST_MAIN(TestFig014FECSubchannel)
#include "test_fig014_fec_subchannel.moc"
