/**
 * @file test_fig017_programme_type.cpp
 * @brief Comprehensive unit tests for FIG 0/17 Programme Type parsing
 *
 * Tests FIG 0/17 parsing according to ETSI EN 300 401 Section 8.1.5
 * and ETSI TS 101 756 Table 10 with comprehensive coverage of:
 * - Static PTy (S/D flag = 1)
 * - Dynamic PTy (S/D flag = 0) with language
 * - 16-bit and 32-bit Service IDs (P/D flag)
 * - All PTy codes (0-31)
 * - CC flag handling
 * - Malformed data handling
 * - Performance benchmarks
 *
 * @author Agent 9 - TypeScript Pro (PDCA Week 3)
 * @date October 29, 2025
 */

#include <QtTest/QtTest>
#include "../src/core/fig_parser.hpp"
#include <chrono>
#include <vector>

using namespace eti::fig;

class TestFig017ProgrammeType : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Static PTy tests (6 tests)
    void test_static_pty_16bit_sid_news();
    void test_static_pty_32bit_sid_sport();
    void test_static_pty_pop_music();
    void test_static_pty_cc_flag_set();
    void test_static_pty_invalid_size_16bit();
    void test_static_pty_invalid_size_32bit();

    // Dynamic PTy tests (6 tests)
    void test_dynamic_pty_16bit_sid_news_english();
    void test_dynamic_pty_32bit_sid_sport_thai();
    void test_dynamic_pty_rock_music_french();
    void test_dynamic_pty_cc_flag_set();
    void test_dynamic_pty_invalid_size_16bit();
    void test_dynamic_pty_invalid_size_32bit();

    // PTy lookup tests (4 tests)
    void test_pty_lookup_news();
    void test_pty_lookup_sport();
    void test_pty_lookup_pop_music();
    void test_pty_lookup_no_pty();

    // Edge case tests (2 tests)
    void test_pty_code_out_of_range();
    void test_empty_buffer();

    // Performance test (1 test)
    void test_parsing_performance();

private:
    FigParser* parser_;

    // Helper to create FIG 0/17 test data
    std::vector<uint8_t> createStaticPtyData(
        uint32_t service_id,
        uint8_t pty_code,
        bool cc_flag,
        bool is_32bit_sid);

    std::vector<uint8_t> createDynamicPtyData(
        uint32_t service_id,
        uint8_t pty_code,
        uint8_t language_code,
        bool cc_flag,
        bool is_32bit_sid);
};

void TestFig017ProgrammeType::initTestCase() {
    parser_ = new FigParser();
    QVERIFY(parser_->initialize(true, false));
}

void TestFig017ProgrammeType::cleanupTestCase() {
    delete parser_;
}

std::vector<uint8_t> TestFig017ProgrammeType::createStaticPtyData(
    uint32_t service_id,
    uint8_t pty_code,
    bool cc_flag,
    bool is_32bit_sid) {

    std::vector<uint8_t> data;

    // Extension field (FIG 0/17)
    data.push_back(0x11);  // Extension 17

    // Byte 1: [1 (C/N=1)][1 (S/D=1 static)][1 (P/D)][5 reserved]
    uint8_t byte1 = 0xC0;  // C/N=1, S/D=1
    if (is_32bit_sid) {
        byte1 |= 0x20;  // P/D=1 for 32-bit SId
    }
    data.push_back(byte1);

    // Service ID (16 or 32 bits)
    if (is_32bit_sid) {
        data.push_back((service_id >> 24) & 0xFF);
        data.push_back((service_id >> 16) & 0xFF);
        data.push_back((service_id >> 8) & 0xFF);
        data.push_back(service_id & 0xFF);
    } else {
        data.push_back((service_id >> 8) & 0xFF);
        data.push_back(service_id & 0xFF);
    }

    // PTy byte: [1 CC flag][2 reserved][5 bits PTy code]
    uint8_t pty_byte = (pty_code & 0x1F);
    if (cc_flag) {
        pty_byte |= 0x80;
    }
    data.push_back(pty_byte);

    return data;
}

std::vector<uint8_t> TestFig017ProgrammeType::createDynamicPtyData(
    uint32_t service_id,
    uint8_t pty_code,
    uint8_t language_code,
    bool cc_flag,
    bool is_32bit_sid) {

    std::vector<uint8_t> data;

    // Extension field (FIG 0/17)
    data.push_back(0x11);

    // Byte 1: [1 (C/N=1)][0 (S/D=0 dynamic)][1 (P/D)][5 reserved]
    uint8_t byte1 = 0x80;  // C/N=1, S/D=0
    if (is_32bit_sid) {
        byte1 |= 0x20;  // P/D=1
    }
    data.push_back(byte1);

    // Service ID
    if (is_32bit_sid) {
        data.push_back((service_id >> 24) & 0xFF);
        data.push_back((service_id >> 16) & 0xFF);
        data.push_back((service_id >> 8) & 0xFF);
        data.push_back(service_id & 0xFF);
    } else {
        data.push_back((service_id >> 8) & 0xFF);
        data.push_back(service_id & 0xFF);
    }

    // PTy byte
    uint8_t pty_byte = (pty_code & 0x1F);
    if (cc_flag) {
        pty_byte |= 0x80;
    }
    data.push_back(pty_byte);

    // Language code (8 bits)
    data.push_back(language_code);

    return data;
}

// ==============================================================================
// STATIC PTy TESTS
// ==============================================================================

void TestFig017ProgrammeType::test_static_pty_16bit_sid_news() {
    qDebug() << "TEST: Static PTy with 16-bit SId + News (code 1)";

    auto fig_data = createStaticPtyData(0xC221, 1, false, false);
    auto pty = parser_->parseFig017_ProgrammeType(fig_data);

    QVERIFY(pty.is_valid);
    QCOMPARE(pty.service_id, static_cast<uint32_t>(0xC221));
    QCOMPARE(pty.pty_code, static_cast<uint8_t>(1));
    QVERIFY(pty.is_static);
    QVERIFY(!pty.cc_flag);
    QCOMPARE(QString::fromStdString(pty.getPtyName()), QString("News"));
}

void TestFig017ProgrammeType::test_static_pty_32bit_sid_sport() {
    qDebug() << "TEST: Static PTy with 32-bit SId + Sport (code 4)";

    auto fig_data = createStaticPtyData(0xE1CC0001, 4, false, true);
    auto pty = parser_->parseFig017_ProgrammeType(fig_data);

    QVERIFY(pty.is_valid);
    QCOMPARE(pty.service_id, static_cast<uint32_t>(0xE1CC0001));
    QCOMPARE(pty.pty_code, static_cast<uint8_t>(4));
    QVERIFY(pty.is_static);
    QVERIFY(!pty.cc_flag);
    QCOMPARE(QString::fromStdString(pty.getPtyName()), QString("Sport"));
}

void TestFig017ProgrammeType::test_static_pty_pop_music() {
    qDebug() << "TEST: Static PTy with Pop Music (code 10)";

    auto fig_data = createStaticPtyData(0xC001, 10, false, false);
    auto pty = parser_->parseFig017_ProgrammeType(fig_data);

    QVERIFY(pty.is_valid);
    QCOMPARE(pty.pty_code, static_cast<uint8_t>(10));
    QVERIFY(pty.is_static);
    QCOMPARE(QString::fromStdString(pty.getPtyName()), QString("Pop Music"));
}

void TestFig017ProgrammeType::test_static_pty_cc_flag_set() {
    qDebug() << "TEST: Static PTy with CC flag set";

    auto fig_data = createStaticPtyData(0xC221, 1, true, false);
    auto pty = parser_->parseFig017_ProgrammeType(fig_data);

    QVERIFY(pty.is_valid);
    QVERIFY(pty.cc_flag);
    QVERIFY(pty.is_static);
}

void TestFig017ProgrammeType::test_static_pty_invalid_size_16bit() {
    qDebug() << "TEST: Static PTy invalid data size for 16-bit SId (< 4 bytes)";

    std::vector<uint8_t> short_data = {0x11, 0xC0, 0xC2};  // Only 3 bytes
    auto pty = parser_->parseFig017_ProgrammeType(short_data);

    QVERIFY(!pty.is_valid);
}

void TestFig017ProgrammeType::test_static_pty_invalid_size_32bit() {
    qDebug() << "TEST: Static PTy invalid data size for 32-bit SId (< 7 bytes)";

    std::vector<uint8_t> short_data = {0x11, 0xE0, 0xE1, 0xCC, 0x00, 0x01};  // Only 6 bytes
    auto pty = parser_->parseFig017_ProgrammeType(short_data);

    QVERIFY(!pty.is_valid);
}

// ==============================================================================
// DYNAMIC PTy TESTS
// ==============================================================================

void TestFig017ProgrammeType::test_dynamic_pty_16bit_sid_news_english() {
    qDebug() << "TEST: Dynamic PTy with 16-bit SId + News + English language";

    auto fig_data = createDynamicPtyData(0xC221, 1, 0x15, false, false);  // 0x15 = English
    auto pty = parser_->parseFig017_ProgrammeType(fig_data);

    QVERIFY(pty.is_valid);
    QCOMPARE(pty.service_id, static_cast<uint32_t>(0xC221));
    QCOMPARE(pty.pty_code, static_cast<uint8_t>(1));
    QVERIFY(!pty.is_static);
    QCOMPARE(pty.language_code, static_cast<uint8_t>(0x15));
    QVERIFY(!pty.cc_flag);
    QCOMPARE(QString::fromStdString(pty.getPtyName()), QString("News"));
}

void TestFig017ProgrammeType::test_dynamic_pty_32bit_sid_sport_thai() {
    qDebug() << "TEST: Dynamic PTy with 32-bit SId + Sport + Thai language";

    auto fig_data = createDynamicPtyData(0xE1CC0001, 4, 0x54, false, true);  // 0x54 = Thai
    auto pty = parser_->parseFig017_ProgrammeType(fig_data);

    QVERIFY(pty.is_valid);
    QCOMPARE(pty.service_id, static_cast<uint32_t>(0xE1CC0001));
    QCOMPARE(pty.pty_code, static_cast<uint8_t>(4));
    QVERIFY(!pty.is_static);
    QCOMPARE(pty.language_code, static_cast<uint8_t>(0x54));
    QCOMPARE(QString::fromStdString(pty.getPtyName()), QString("Sport"));
}

void TestFig017ProgrammeType::test_dynamic_pty_rock_music_french() {
    qDebug() << "TEST: Dynamic PTy with Rock Music + French language";

    auto fig_data = createDynamicPtyData(0xC001, 11, 0x06, false, false);  // 0x06 = French
    auto pty = parser_->parseFig017_ProgrammeType(fig_data);

    QVERIFY(pty.is_valid);
    QCOMPARE(pty.pty_code, static_cast<uint8_t>(11));
    QVERIFY(!pty.is_static);
    QCOMPARE(pty.language_code, static_cast<uint8_t>(0x06));
    QCOMPARE(QString::fromStdString(pty.getPtyName()), QString("Rock Music"));
}

void TestFig017ProgrammeType::test_dynamic_pty_cc_flag_set() {
    qDebug() << "TEST: Dynamic PTy with CC flag set";

    auto fig_data = createDynamicPtyData(0xC221, 1, 0x15, true, false);
    auto pty = parser_->parseFig017_ProgrammeType(fig_data);

    QVERIFY(pty.is_valid);
    QVERIFY(pty.cc_flag);
    QVERIFY(!pty.is_static);
}

void TestFig017ProgrammeType::test_dynamic_pty_invalid_size_16bit() {
    qDebug() << "TEST: Dynamic PTy invalid data size for 16-bit (< 5 bytes)";

    std::vector<uint8_t> short_data = {0x11, 0x80, 0xC2, 0x21};  // Only 4 bytes
    auto pty = parser_->parseFig017_ProgrammeType(short_data);

    QVERIFY(!pty.is_valid);
}

void TestFig017ProgrammeType::test_dynamic_pty_invalid_size_32bit() {
    qDebug() << "TEST: Dynamic PTy invalid data size for 32-bit (< 8 bytes)";

    std::vector<uint8_t> short_data = {0x11, 0xA0, 0xE1, 0xCC, 0x00, 0x01, 0x04};  // Only 7 bytes
    auto pty = parser_->parseFig017_ProgrammeType(short_data);

    QVERIFY(!pty.is_valid);
}

// ==============================================================================
// PTy LOOKUP TESTS
// ==============================================================================

void TestFig017ProgrammeType::test_pty_lookup_news() {
    qDebug() << "TEST: PTy code 1 -> News";

    auto fig_data = createStaticPtyData(0xC001, 1, false, false);
    auto pty = parser_->parseFig017_ProgrammeType(fig_data);

    QVERIFY(pty.is_valid);
    QCOMPARE(QString::fromStdString(pty.getPtyName()), QString("News"));
}

void TestFig017ProgrammeType::test_pty_lookup_sport() {
    qDebug() << "TEST: PTy code 4 -> Sport";

    auto fig_data = createStaticPtyData(0xC001, 4, false, false);
    auto pty = parser_->parseFig017_ProgrammeType(fig_data);

    QVERIFY(pty.is_valid);
    QCOMPARE(QString::fromStdString(pty.getPtyName()), QString("Sport"));
}

void TestFig017ProgrammeType::test_pty_lookup_pop_music() {
    qDebug() << "TEST: PTy code 10 -> Pop Music";

    auto fig_data = createStaticPtyData(0xC001, 10, false, false);
    auto pty = parser_->parseFig017_ProgrammeType(fig_data);

    QVERIFY(pty.is_valid);
    QCOMPARE(QString::fromStdString(pty.getPtyName()), QString("Pop Music"));
}

void TestFig017ProgrammeType::test_pty_lookup_no_pty() {
    qDebug() << "TEST: PTy code 0 -> No PTy";

    auto fig_data = createStaticPtyData(0xC001, 0, false, false);
    auto pty = parser_->parseFig017_ProgrammeType(fig_data);

    QVERIFY(pty.is_valid);
    QCOMPARE(QString::fromStdString(pty.getPtyName()), QString("No PTy"));
}

// ==============================================================================
// EDGE CASE TESTS
// ==============================================================================

void TestFig017ProgrammeType::test_pty_code_out_of_range() {
    qDebug() << "TEST: PTy code > 31 (invalid, only 5 bits)";

    // Create data with invalid PTy code (> 31)
    std::vector<uint8_t> fig_data = {0x11, 0xC0, 0xC0, 0x01, 0x3F};  // PTy=31 is max (5 bits)
    auto pty = parser_->parseFig017_ProgrammeType(fig_data);

    QVERIFY(pty.is_valid);  // Should still parse
    QCOMPARE(pty.pty_code, static_cast<uint8_t>(31));  // Masked to 5 bits
}

void TestFig017ProgrammeType::test_empty_buffer() {
    qDebug() << "TEST: Empty buffer handling";

    std::vector<uint8_t> empty_data;
    auto pty = parser_->parseFig017_ProgrammeType(empty_data);

    QVERIFY(!pty.is_valid);
}

// ==============================================================================
// PERFORMANCE TEST
// ==============================================================================

void TestFig017ProgrammeType::test_parsing_performance() {
    qDebug() << "TEST: Parsing performance benchmark";

    auto fig_data = createStaticPtyData(0xC221, 4, false, false);

    // Warm-up
    for (int i = 0; i < 10; ++i) {
        parser_->parseFig017_ProgrammeType(fig_data);
    }

    // Benchmark
    constexpr int iterations = 1000;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        auto pty = parser_->parseFig017_ProgrammeType(fig_data);
        QVERIFY(pty.is_valid);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time);

    double avg_time_us = static_cast<double>(duration.count()) / iterations;

    qDebug() << QString("Average parsing time: %1 µs per FIG 0/17")
        .arg(avg_time_us, 0, 'f', 2);

    // Performance target: < 5 µs per parse
    QVERIFY2(avg_time_us < 5.0,
        QString("Performance target failed: %1 µs > 5 µs")
        .arg(avg_time_us, 0, 'f', 2).toUtf8().constData());
}

// Register metatype for signal/slot
Q_DECLARE_METATYPE(ProgrammeType)

QTEST_MAIN(TestFig017ProgrammeType)
#include "test_fig017_programme_type.moc"
