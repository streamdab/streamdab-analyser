/**
 * @file test_fig009_country_lto_table.cpp
 * @brief Comprehensive unit tests for FIG 0/9 Country/LTO/International Table parsing
 *
 * Tests FIG 0/9 parsing according to ETSI EN 300 401 Section 8.1.8
 * with comprehensive coverage of:
 * - Extended Country Code (ECC) extraction
 * - Local Time Offset (LTO) extraction with sign handling
 * - International Table ID parsing
 * - Extension flag handling
 * - Half-hour resolution timezone conversion
 * - Thai ensemble detection (ECC 0xE1, LTO +7.0)
 * - Edge cases and error handling
 * - Performance benchmarks
 *
 * @author Agent 20 - Python Pro (PDCA Week 5)
 * @date November 3, 2025
 */

#include <QtTest/QtTest>
#include "../src/core/fig_parser.hpp"
#include "../src/utils/logger.h"
#include <chrono>
#include <vector>

using namespace eti::fig;

class TestFig009CountryLTO : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Basic parsing tests (6 tests)
    void test_thailand_lto_positive();
    void test_india_lto_half_hour();
    void test_uk_lto_zero();
    void test_usa_lto_negative();
    void test_extension_flag_set();
    void test_extension_flag_clear();

    // ECC validation tests (4 tests)
    void test_ecc_thailand();
    void test_ecc_germany();
    void test_ecc_france();
    void test_ecc_boundary_values();

    // LTO conversion tests (4 tests)
    void test_lto_to_hours_positive();
    void test_lto_to_hours_negative();
    void test_lto_to_minutes_positive();
    void test_lto_to_minutes_negative();

    // International table ID tests (2 tests)
    void test_international_table_latin();
    void test_international_table_unicode();

    // Edge case tests (6 tests)
    void test_lto_minimum_value();
    void test_lto_maximum_value();
    void test_lto_out_of_range_negative();
    void test_lto_out_of_range_positive();
    void test_insufficient_data();
    void test_empty_data();

    // Integration tests (2 tests)
    void test_thai_ensemble_complete();
    void test_signal_emission();

    // Performance test (1 test)
    void test_parsing_performance();

private:
    FigParser* parser_;

    // Helper to create FIG 0/9 test data
    std::vector<uint8_t> createFig009Data(
        bool ext_flag,
        int8_t lto,
        uint8_t ecc,
        uint8_t international_table_id);
};

void TestFig009CountryLTO::initTestCase() {
    parser_ = new FigParser();
    QVERIFY(parser_->initialize(true, false));
}

void TestFig009CountryLTO::cleanupTestCase() {
    delete parser_;
}

std::vector<uint8_t> TestFig009CountryLTO::createFig009Data(
    bool ext_flag,
    int8_t lto,
    uint8_t ecc,
    uint8_t international_table_id) {

    std::vector<uint8_t> data;
    data.push_back(0x09);  // Extension byte (FIG 0/9)

    // ETSI EN 300 401 Section 8.1.8 format:
    // Byte 1: [Ext 1-bit][LTO 5/6 bits depending on Ext]
    // If Ext=0: LTO is 5 bits (signed, range -16 to +15)
    // If Ext=1: LTO is 6 bits (signed, range -32 to +31)

    uint8_t byte1;
    if (ext_flag) {
        // 6-bit LTO: [1][6-bit LTO]
        byte1 = 0x80 | (static_cast<uint8_t>(lto) & 0x3F);
    } else {
        // 5-bit LTO: [0][5-bit LTO][2 reserved bits]
        byte1 = ((static_cast<uint8_t>(lto) & 0x1F) << 2);
    }
    data.push_back(byte1);

    // Byte 2: ECC (8 bits)
    data.push_back(ecc);

    // Byte 3: International table ID (8 bits)
    data.push_back(international_table_id);

    return data;
}

// ============================================================================
// Basic Parsing Tests
// ============================================================================

void TestFig009CountryLTO::test_thailand_lto_positive() {
    qDebug() << "TEST: Thailand ensemble (ECC=0xE1, LTO=+7.0)";

    // Thailand: ECC=0xE1, LTO=+14 half-hours = +7.0 hours
    auto fig_data = createFig009Data(false, 14, 0xE1, 0x00);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);
    QCOMPARE(country_lto.ext_flag, false);
    QCOMPARE(country_lto.lto, static_cast<int8_t>(14));
    QCOMPARE(country_lto.ecc, static_cast<uint8_t>(0xE1));
    QCOMPARE(country_lto.international_table_id, static_cast<uint8_t>(0x00));
    QCOMPARE(country_lto.getLTOHours(), 7.0f);
    QCOMPARE(country_lto.getLTOMinutes(), static_cast<int16_t>(420));
    QVERIFY(country_lto.isThaiEnsemble());
}

void TestFig009CountryLTO::test_india_lto_half_hour() {
    qDebug() << "TEST: India ensemble (LTO=+5.5 hours)";

    // India: LTO=+11 half-hours = +5.5 hours
    auto fig_data = createFig009Data(false, 11, 0xE2, 0x00);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);
    QCOMPARE(country_lto.lto, static_cast<int8_t>(11));
    QCOMPARE(country_lto.getLTOHours(), 5.5f);
    QCOMPARE(country_lto.getLTOMinutes(), static_cast<int16_t>(330));
}

void TestFig009CountryLTO::test_uk_lto_zero() {
    qDebug() << "TEST: UK ensemble (LTO=0, UTC)";

    // UK: LTO=0 (UTC), ECC=0xE0
    auto fig_data = createFig009Data(false, 0, 0xE0, 0x00);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);
    QCOMPARE(country_lto.lto, static_cast<int8_t>(0));
    QCOMPARE(country_lto.getLTOHours(), 0.0f);
    QCOMPARE(country_lto.getLTOMinutes(), static_cast<int16_t>(0));
}

void TestFig009CountryLTO::test_usa_lto_negative() {
    qDebug() << "TEST: USA East Coast (LTO=-5.0 hours)";

    // USA East: LTO=-10 half-hours = -5.0 hours
    int8_t lto_value = -10;
    auto fig_data = createFig009Data(false, lto_value, 0xA1, 0x00);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);
    QCOMPARE(country_lto.lto, static_cast<int8_t>(-10));
    QCOMPARE(country_lto.getLTOHours(), -5.0f);
    QCOMPARE(country_lto.getLTOMinutes(), static_cast<int16_t>(-300));
}

void TestFig009CountryLTO::test_extension_flag_set() {
    qDebug() << "TEST: Extension flag set (6-bit LTO)";

    // Extended LTO range: +20 half-hours = +10.0 hours
    auto fig_data = createFig009Data(true, 20, 0xE3, 0x01);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);
    QVERIFY(country_lto.ext_flag);
    QCOMPARE(country_lto.lto, static_cast<int8_t>(20));
    QCOMPARE(country_lto.getLTOHours(), 10.0f);
}

void TestFig009CountryLTO::test_extension_flag_clear() {
    qDebug() << "TEST: Extension flag clear (5-bit LTO)";

    auto fig_data = createFig009Data(false, 8, 0xE4, 0x00);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);
    QVERIFY(!country_lto.ext_flag);
    QCOMPARE(country_lto.getLTOHours(), 4.0f);
}

// ============================================================================
// ECC Validation Tests
// ============================================================================

void TestFig009CountryLTO::test_ecc_thailand() {
    qDebug() << "TEST: ECC Thailand (0xE1)";

    auto fig_data = createFig009Data(false, 14, 0xE1, 0x00);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);
    QCOMPARE(country_lto.ecc, static_cast<uint8_t>(0xE1));
    QVERIFY(country_lto.isThaiEnsemble());
}

void TestFig009CountryLTO::test_ecc_germany() {
    qDebug() << "TEST: ECC Germany (0xD0)";

    auto fig_data = createFig009Data(false, 2, 0xD0, 0x00);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);
    QCOMPARE(country_lto.ecc, static_cast<uint8_t>(0xD0));
}

void TestFig009CountryLTO::test_ecc_france() {
    qDebug() << "TEST: ECC France (0xF0)";

    auto fig_data = createFig009Data(false, 2, 0xF0, 0x00);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);
    QCOMPARE(country_lto.ecc, static_cast<uint8_t>(0xF0));
}

void TestFig009CountryLTO::test_ecc_boundary_values() {
    qDebug() << "TEST: ECC boundary values (0x00 and 0xFF)";

    // Minimum ECC
    auto fig_min = createFig009Data(false, 0, 0x00, 0x00);
    auto lto_min = parser_->parseFig009_CountryLTO(fig_min);
    QVERIFY(lto_min.is_valid);
    QCOMPARE(lto_min.ecc, static_cast<uint8_t>(0x00));

    // Maximum ECC
    auto fig_max = createFig009Data(false, 0, 0xFF, 0x00);
    auto lto_max = parser_->parseFig009_CountryLTO(fig_max);
    QVERIFY(lto_max.is_valid);
    QCOMPARE(lto_max.ecc, static_cast<uint8_t>(0xFF));
}

// ============================================================================
// LTO Conversion Tests
// ============================================================================

void TestFig009CountryLTO::test_lto_to_hours_positive() {
    qDebug() << "TEST: LTO to hours conversion (positive)";

    // +14 half-hours = +7.0 hours
    auto fig_data = createFig009Data(false, 14, 0xE1, 0x00);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);
    QCOMPARE(country_lto.getLTOHours(), 7.0f);
}

void TestFig009CountryLTO::test_lto_to_hours_negative() {
    qDebug() << "TEST: LTO to hours conversion (negative)";

    // -10 half-hours = -5.0 hours
    auto fig_data = createFig009Data(false, -10, 0xA1, 0x00);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);
    QCOMPARE(country_lto.getLTOHours(), -5.0f);
}

void TestFig009CountryLTO::test_lto_to_minutes_positive() {
    qDebug() << "TEST: LTO to minutes conversion (positive)";

    // +14 half-hours = +420 minutes
    auto fig_data = createFig009Data(false, 14, 0xE1, 0x00);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);
    QCOMPARE(country_lto.getLTOMinutes(), static_cast<int16_t>(420));
}

void TestFig009CountryLTO::test_lto_to_minutes_negative() {
    qDebug() << "TEST: LTO to minutes conversion (negative)";

    // -10 half-hours = -300 minutes
    auto fig_data = createFig009Data(false, -10, 0xA1, 0x00);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);
    QCOMPARE(country_lto.getLTOMinutes(), static_cast<int16_t>(-300));
}

// ============================================================================
// International Table ID Tests
// ============================================================================

void TestFig009CountryLTO::test_international_table_latin() {
    qDebug() << "TEST: International table ID - Complete EBU Latin (0x00)";

    auto fig_data = createFig009Data(false, 14, 0xE1, 0x00);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);
    QCOMPARE(country_lto.international_table_id, static_cast<uint8_t>(0x00));
}

void TestFig009CountryLTO::test_international_table_unicode() {
    qDebug() << "TEST: International table ID - UTF-8 (0x0F)";

    auto fig_data = createFig009Data(false, 14, 0xE1, 0x0F);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);
    QCOMPARE(country_lto.international_table_id, static_cast<uint8_t>(0x0F));
}

// ============================================================================
// Edge Case Tests
// ============================================================================

void TestFig009CountryLTO::test_lto_minimum_value() {
    qDebug() << "TEST: LTO minimum value (-12.0 hours)";

    // -24 half-hours = -12.0 hours
    auto fig_data = createFig009Data(true, -24, 0xA0, 0x00);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);
    QCOMPARE(country_lto.lto, static_cast<int8_t>(-24));
    QCOMPARE(country_lto.getLTOHours(), -12.0f);
    QVERIFY(country_lto.validateLTO());
}

void TestFig009CountryLTO::test_lto_maximum_value() {
    qDebug() << "TEST: LTO maximum value (+14.0 hours)";

    // +28 half-hours = +14.0 hours
    auto fig_data = createFig009Data(true, 28, 0xF0, 0x00);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);
    QCOMPARE(country_lto.lto, static_cast<int8_t>(28));
    QCOMPARE(country_lto.getLTOHours(), 14.0f);
    QVERIFY(country_lto.validateLTO());
}

void TestFig009CountryLTO::test_lto_out_of_range_negative() {
    qDebug() << "TEST: LTO out of range (negative)";

    // -30 half-hours = -15.0 hours (out of valid range)
    auto fig_data = createFig009Data(true, -30, 0xA0, 0x00);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);  // Parser should still parse
    QCOMPARE(country_lto.lto, static_cast<int8_t>(-30));
    QVERIFY(!country_lto.validateLTO());  // But validation should fail
}

void TestFig009CountryLTO::test_lto_out_of_range_positive() {
    qDebug() << "TEST: LTO out of range (positive)";

    // +30 half-hours = +15.0 hours (out of valid range)
    auto fig_data = createFig009Data(true, 30, 0xF0, 0x00);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);  // Parser should still parse
    QCOMPARE(country_lto.lto, static_cast<int8_t>(30));
    QVERIFY(!country_lto.validateLTO());  // But validation should fail
}

void TestFig009CountryLTO::test_insufficient_data() {
    qDebug() << "TEST: Insufficient data (< 4 bytes)";

    std::vector<uint8_t> short_data = {0x09, 0x1C, 0xE1};  // Missing table ID
    auto country_lto = parser_->parseFig009_CountryLTO(short_data);

    QVERIFY(!country_lto.is_valid);
}

void TestFig009CountryLTO::test_empty_data() {
    qDebug() << "TEST: Empty data handling";

    std::vector<uint8_t> empty_data;
    auto country_lto = parser_->parseFig009_CountryLTO(empty_data);

    QVERIFY(!country_lto.is_valid);
}

// ============================================================================
// Integration Tests
// ============================================================================

void TestFig009CountryLTO::test_thai_ensemble_complete() {
    qDebug() << "TEST: Complete Thai ensemble validation";

    // Thailand: ECC=0xE1, LTO=+7.0, Table ID=0x00 (EBU Latin)
    auto fig_data = createFig009Data(false, 14, 0xE1, 0x00);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);
    QVERIFY(country_lto.isThaiEnsemble());
    QCOMPARE(country_lto.ecc, static_cast<uint8_t>(0xE1));
    QCOMPARE(country_lto.lto, static_cast<int8_t>(14));
    QCOMPARE(country_lto.getLTOHours(), 7.0f);
    QCOMPARE(country_lto.getLTOMinutes(), static_cast<int16_t>(420));
    QVERIFY(country_lto.validateLTO());
    QCOMPARE(country_lto.international_table_id, static_cast<uint8_t>(0x00));
}

void TestFig009CountryLTO::test_signal_emission() {
    qDebug() << "TEST: Signal emission on successful parse";

    QSignalSpy spy(parser_, &FigParser::countryLTODiscovered);

    auto fig_data = createFig009Data(false, 14, 0xE1, 0x00);
    auto country_lto = parser_->parseFig009_CountryLTO(fig_data);

    QVERIFY(country_lto.is_valid);
    QCOMPARE(spy.count(), 1);

    // Verify signal payload
    auto args = spy.takeFirst();
    auto emitted_lto = args.at(0).value<CountryLTOInfo>();
    QCOMPARE(emitted_lto.ecc, static_cast<uint8_t>(0xE1));
    QCOMPARE(emitted_lto.lto, static_cast<int8_t>(14));
}

// ============================================================================
// Performance Test
// ============================================================================

void TestFig009CountryLTO::test_parsing_performance() {
    // The benchmark measures parsing, not per-call logging: keep the logger
    // quiet for the timed section (restored on scope exit).
    struct LogLevelGuard {
        Logger::LogLevel saved = Logger::instance().getLogLevel();
        LogLevelGuard() { Logger::instance().setLogLevel(Logger::LogLevel::Warning); }
        ~LogLevelGuard() { Logger::instance().setLogLevel(saved); }
    } logGuard;

    qDebug() << "TEST: Parsing performance benchmark";

    auto fig_data = createFig009Data(false, 14, 0xE1, 0x00);

    // Warm-up
    for (int i = 0; i < 10; ++i) {
        parser_->parseFig009_CountryLTO(fig_data);
    }

    // Benchmark
    constexpr int iterations = 1000;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        auto country_lto = parser_->parseFig009_CountryLTO(fig_data);
        QVERIFY(country_lto.is_valid);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time);

    double avg_time_us = static_cast<double>(duration.count()) / iterations;

    qDebug() << QString("Average parsing time: %1 µs per FIG 0/9")
        .arg(avg_time_us, 0, 'f', 2);

    // Performance target: < 10 µs per parse
    QVERIFY2(avg_time_us < 10.0,
        QString("Performance target failed: %1 µs > 10 µs")
        .arg(avg_time_us, 0, 'f', 2).toUtf8().constData());
}

QTEST_MAIN(TestFig009CountryLTO)
#include "test_fig009_country_lto_table.moc"
