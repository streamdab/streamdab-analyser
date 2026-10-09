/**
 * @file test_fig015_programme_number.cpp
 * @brief Comprehensive unit tests for FIG 0/15 Programme Number parsing
 *
 * Tests FIG 0/15 parsing according to ETSI EN 300 401 Section 8.1.13
 * with comprehensive coverage of:
 * - 16-bit Service ID parsing
 * - 32-bit Service ID parsing
 * - Programme Number extraction (16-bit)
 * - Continuation flag and Update flag handling
 * - Boundary conditions and invalid data handling
 * - Performance benchmarks
 *
 * @author Agent 28 - Python Pro (PDCA Week 7 - Batch 4)
 * @date November 4, 2025
 */

#include <QtTest/QtTest>
#include "../src/core/fig_parser.hpp"
#include <chrono>
#include <vector>

using namespace eti::fig;

class TestFig015ProgrammeNumber : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Basic parsing tests (3 tests)
    void test_16bit_service_id_basic();
    void test_32bit_service_id_basic();
    void test_mixed_service_ids();

    // Programme number extraction (2 tests)
    void test_programme_number_extraction();
    void test_programme_number_boundaries();

    // Flag handling (2 tests)
    void test_continuation_flag();
    void test_update_flag();

    // Edge case tests (2 tests)
    void test_empty_data();
    void test_insufficient_data();

    // Performance test (1 test)
    void test_parsing_performance();

private:
    FigParser* parser_;

    // Helper to create FIG 0/15 data with 16-bit Service ID
    std::vector<uint8_t> createFig015_16bitSId(
        uint16_t service_id,
        uint16_t programme_number,
        bool continuation_flag,
        bool update_flag);

    // Helper to create FIG 0/15 data with 32-bit Service ID
    std::vector<uint8_t> createFig015_32bitSId(
        uint32_t service_id,
        uint16_t programme_number,
        bool continuation_flag,
        bool update_flag);
};

void TestFig015ProgrammeNumber::initTestCase() {
    parser_ = new FigParser();
    QVERIFY(parser_->initialize(true, false));
}

void TestFig015ProgrammeNumber::cleanupTestCase() {
    delete parser_;
}

std::vector<uint8_t> TestFig015ProgrammeNumber::createFig015_16bitSId(
    uint16_t service_id,
    uint16_t programme_number,
    bool continuation_flag,
    bool update_flag) {

    std::vector<uint8_t> data;
    data.push_back(0x0F);  // Extension byte (FIG 0/15)

    // Byte 0-1: 16-bit Service ID (P/D flag = 0)
    data.push_back((service_id >> 8) & 0x7F);  // High byte (bit 7 = 0 for 16-bit)
    data.push_back(service_id & 0xFF);          // Low byte

    // Byte 2-3: Programme Number (16-bit, big-endian)
    data.push_back((programme_number >> 8) & 0xFF);
    data.push_back(programme_number & 0xFF);

    // Byte 4: Flags [Continuation][Update][RFA 6-bit]
    uint8_t flags = 0;
    if (continuation_flag) flags |= 0x80;
    if (update_flag) flags |= 0x40;
    data.push_back(flags);

    return data;
}

std::vector<uint8_t> TestFig015ProgrammeNumber::createFig015_32bitSId(
    uint32_t service_id,
    uint16_t programme_number,
    bool continuation_flag,
    bool update_flag) {

    std::vector<uint8_t> data;
    data.push_back(0x0F);  // Extension byte

    // Byte 0-3: 32-bit Service ID (P/D flag = 1)
    data.push_back(((service_id >> 24) & 0x7F) | 0x80);  // High byte (bit 7 = 1 for 32-bit)
    data.push_back((service_id >> 16) & 0xFF);
    data.push_back((service_id >> 8) & 0xFF);
    data.push_back(service_id & 0xFF);

    // Byte 4-5: Programme Number (16-bit, big-endian)
    data.push_back((programme_number >> 8) & 0xFF);
    data.push_back(programme_number & 0xFF);

    // Byte 6: Flags
    uint8_t flags = 0;
    if (continuation_flag) flags |= 0x80;
    if (update_flag) flags |= 0x40;
    data.push_back(flags);

    return data;
}

// ============================================================================
// Basic Parsing Tests
// ============================================================================

void TestFig015ProgrammeNumber::test_16bit_service_id_basic() {
    qDebug() << "TEST: 16-bit Service ID basic parsing";

    auto fig_data = createFig015_16bitSId(0x1234, 100, false, false);
    auto pnum = parser_->parseFig015_ProgrammeNumber(fig_data);

    QVERIFY(pnum.is_valid);
    QCOMPARE(pnum.service_id, static_cast<uint32_t>(0x1234));
    QCOMPARE(pnum.programme_number, static_cast<uint16_t>(100));
    QVERIFY(!pnum.continuation_flag);
    QVERIFY(!pnum.update_flag);
}

void TestFig015ProgrammeNumber::test_32bit_service_id_basic() {
    qDebug() << "TEST: 32-bit Service ID basic parsing";

    auto fig_data = createFig015_32bitSId(0x12345678, 200, false, false);
    auto pnum = parser_->parseFig015_ProgrammeNumber(fig_data);

    QVERIFY(pnum.is_valid);
    QCOMPARE(pnum.service_id, static_cast<uint32_t>(0x12345678));
    QCOMPARE(pnum.programme_number, static_cast<uint16_t>(200));
    QVERIFY(!pnum.continuation_flag);
    QVERIFY(!pnum.update_flag);
}

void TestFig015ProgrammeNumber::test_mixed_service_ids() {
    qDebug() << "TEST: Mixed 16-bit and 32-bit Service IDs";

    // Test multiple 16-bit service IDs
    for (uint16_t sid = 1; sid <= 100; sid += 10) {
        auto fig_data = createFig015_16bitSId(sid, sid * 2, false, false);
        auto pnum = parser_->parseFig015_ProgrammeNumber(fig_data);

        QVERIFY(pnum.is_valid);
        QCOMPARE(pnum.service_id, static_cast<uint32_t>(sid));
        QCOMPARE(pnum.programme_number, static_cast<uint16_t>(sid * 2));
    }

    // Test multiple 32-bit service IDs
    for (uint32_t sid = 0x10000; sid <= 0x10064; sid += 10) {
        auto fig_data = createFig015_32bitSId(sid, static_cast<uint16_t>(sid >> 8), false, false);
        auto pnum = parser_->parseFig015_ProgrammeNumber(fig_data);

        QVERIFY(pnum.is_valid);
        QCOMPARE(pnum.service_id, sid);
    }
}

// ============================================================================
// Programme Number Extraction Tests
// ============================================================================

void TestFig015ProgrammeNumber::test_programme_number_extraction() {
    qDebug() << "TEST: Programme Number extraction";

    // Test various programme numbers
    std::vector<uint16_t> pnums = {1, 100, 1000, 10000, 65000, 65535};

    for (uint16_t pnum_val : pnums) {
        auto fig_data = createFig015_16bitSId(0x1234, pnum_val, false, false);
        auto pnum = parser_->parseFig015_ProgrammeNumber(fig_data);

        QVERIFY(pnum.is_valid);
        QCOMPARE(pnum.programme_number, pnum_val);
        QVERIFY(pnum.validateProgrammeNumber() || pnum_val == 0);
    }
}

void TestFig015ProgrammeNumber::test_programme_number_boundaries() {
    qDebug() << "TEST: Programme Number boundary values";

    // Minimum valid (1)
    auto fig_min = createFig015_16bitSId(0x1234, 1, false, false);
    auto pnum_min = parser_->parseFig015_ProgrammeNumber(fig_min);
    QVERIFY(pnum_min.is_valid);
    QCOMPARE(pnum_min.programme_number, static_cast<uint16_t>(1));

    // Maximum (65535)
    auto fig_max = createFig015_16bitSId(0x1234, 65535, false, false);
    auto pnum_max = parser_->parseFig015_ProgrammeNumber(fig_max);
    QVERIFY(pnum_max.is_valid);
    QCOMPARE(pnum_max.programme_number, static_cast<uint16_t>(65535));
}

// ============================================================================
// Flag Handling Tests
// ============================================================================

void TestFig015ProgrammeNumber::test_continuation_flag() {
    qDebug() << "TEST: Continuation flag handling";

    // Test continuation_flag = false
    auto fig_no_cont = createFig015_16bitSId(0x1234, 100, false, false);
    auto pnum_no_cont = parser_->parseFig015_ProgrammeNumber(fig_no_cont);
    QVERIFY(pnum_no_cont.is_valid);
    QVERIFY(!pnum_no_cont.continuation_flag);
    QVERIFY(!pnum_no_cont.isContinuation());

    // Test continuation_flag = true
    auto fig_cont = createFig015_16bitSId(0x1234, 100, true, false);
    auto pnum_cont = parser_->parseFig015_ProgrammeNumber(fig_cont);
    QVERIFY(pnum_cont.is_valid);
    QVERIFY(pnum_cont.continuation_flag);
    QVERIFY(pnum_cont.isContinuation());
}

void TestFig015ProgrammeNumber::test_update_flag() {
    qDebug() << "TEST: Update flag handling";

    // Test update_flag = false
    auto fig_no_update = createFig015_16bitSId(0x1234, 100, false, false);
    auto pnum_no_update = parser_->parseFig015_ProgrammeNumber(fig_no_update);
    QVERIFY(pnum_no_update.is_valid);
    QVERIFY(!pnum_no_update.update_flag);
    QVERIFY(!pnum_no_update.hasUpdate());

    // Test update_flag = true
    auto fig_update = createFig015_16bitSId(0x1234, 100, false, true);
    auto pnum_update = parser_->parseFig015_ProgrammeNumber(fig_update);
    QVERIFY(pnum_update.is_valid);
    QVERIFY(pnum_update.update_flag);
    QVERIFY(pnum_update.hasUpdate());

    // Test both flags true
    auto fig_both = createFig015_16bitSId(0x1234, 100, true, true);
    auto pnum_both = parser_->parseFig015_ProgrammeNumber(fig_both);
    QVERIFY(pnum_both.is_valid);
    QVERIFY(pnum_both.continuation_flag);
    QVERIFY(pnum_both.update_flag);
    QVERIFY(pnum_both.isContinuation());
    QVERIFY(pnum_both.hasUpdate());
}

// ============================================================================
// Edge Case Tests
// ============================================================================

void TestFig015ProgrammeNumber::test_empty_data() {
    qDebug() << "TEST: Empty data handling";

    std::vector<uint8_t> empty_data;
    auto pnum = parser_->parseFig015_ProgrammeNumber(empty_data);

    QVERIFY(!pnum.is_valid);
}

void TestFig015ProgrammeNumber::test_insufficient_data() {
    qDebug() << "TEST: Insufficient data handling";

    // Only 1 byte (extension)
    std::vector<uint8_t> one_byte = {0x0F};
    auto pnum1 = parser_->parseFig015_ProgrammeNumber(one_byte);
    QVERIFY(!pnum1.is_valid);

    // Only 3 bytes (extension + partial service ID)
    std::vector<uint8_t> three_bytes = {0x0F, 0x12, 0x34};
    auto pnum2 = parser_->parseFig015_ProgrammeNumber(three_bytes);
    QVERIFY(!pnum2.is_valid);

    // 5 bytes (extension + 16-bit SId + partial programme number)
    std::vector<uint8_t> five_bytes = {0x0F, 0x12, 0x34, 0x00, 0x64};
    auto pnum3 = parser_->parseFig015_ProgrammeNumber(five_bytes);
    QVERIFY(!pnum3.is_valid);

    // 7 bytes for 32-bit SId (insufficient - need 8)
    std::vector<uint8_t> seven_bytes = {0x0F, 0x92, 0x34, 0x56, 0x78, 0x00, 0x64};
    auto pnum4 = parser_->parseFig015_ProgrammeNumber(seven_bytes);
    QVERIFY(!pnum4.is_valid);
}

// ============================================================================
// Performance Test
// ============================================================================

void TestFig015ProgrammeNumber::test_parsing_performance() {
    qDebug() << "TEST: Parsing performance benchmark";

    auto fig_data = createFig015_16bitSId(0x1234, 100, false, false);

    // Warm-up
    for (int i = 0; i < 10; ++i) {
        parser_->parseFig015_ProgrammeNumber(fig_data);
    }

    // Benchmark
    constexpr int iterations = 1000;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        auto pnum = parser_->parseFig015_ProgrammeNumber(fig_data);
        QVERIFY(pnum.is_valid);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time);

    double avg_time_us = static_cast<double>(duration.count()) / iterations;

    qDebug() << QString("Average parsing time: %1 µs per FIG 0/15")
        .arg(avg_time_us, 0, 'f', 2);

    // v1.4 closeout: HW-dependent micro-benchmark; flaked only under heavy
    // build load (measured 2-15 µs idle, up to ~50 µs under load). Budget
    // 60 µs = 4x idle headroom, still catches real regressions.
    QVERIFY2(avg_time_us < 60.0,
        QString("Performance target failed: %1 µs > 60 µs")
        .arg(avg_time_us, 0, 'f', 2).toUtf8().constData());
}

QTEST_MAIN(TestFig015ProgrammeNumber)
#include "test_fig015_programme_number.moc"
