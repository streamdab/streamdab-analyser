/**
 * @file test_fig016_programme_type_intl.cpp
 * @brief Comprehensive unit tests for FIG 0/16 Programme Type International parsing
 *
 * Tests FIG 0/16 parsing according to ETSI EN 300 401 Section 8.1.14
 * with comprehensive coverage of:
 * - International code extraction (8-bit, 0-255)
 * - Programme type extraction (5-bit, 0-31)
 * - Language code extraction (8-bit)
 * - Service ID extraction (16-bit)
 * - Malformed data handling
 * - Performance benchmarks
 *
 * @author Agent 29 - Python Pro (PDCA Week 7 - Batch 4)
 * @date November 4, 2025
 */

#include <QtTest/QtTest>
#include "../src/core/fig_parser.hpp"
#include "../src/utils/logger.h"
#include <chrono>
#include <vector>

using namespace eti::fig;

class TestFig016ProgrammeTypeIntl : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Basic parsing tests (3 tests)
    void test_basic_parsing_rds_compatible();
    void test_basic_parsing_rbds();
    void test_basic_parsing_proprietary();

    // International code extraction (2 tests)
    void test_intl_code_rds();
    void test_intl_code_reserved();

    // Programme type extraction (2 tests)
    void test_pty_news();
    void test_pty_sport();

    // Edge case tests (2 tests)
    void test_insufficient_data();
    void test_invalid_service_id_zero();

    // Performance test (1 test)
    void test_parsing_performance();

private:
    FigParser* parser_;

    // Helper to create FIG 0/16 test data
    std::vector<uint8_t> createFig016Data(
        uint16_t service_id,
        uint8_t international_code,
        uint8_t programme_type,
        uint8_t language_code);
};

void TestFig016ProgrammeTypeIntl::initTestCase() {
    parser_ = new FigParser();
    QVERIFY(parser_->initialize(true, false));
}

void TestFig016ProgrammeTypeIntl::cleanupTestCase() {
    delete parser_;
}

std::vector<uint8_t> TestFig016ProgrammeTypeIntl::createFig016Data(
    uint16_t service_id,
    uint8_t international_code,
    uint8_t programme_type,
    uint8_t language_code) {

    std::vector<uint8_t> data;

    // Extension field (FIG 0/16)
    data.push_back(0x10);  // Extension 16

    // Service ID (16-bit, big-endian) - bytes 1-2
    data.push_back((service_id >> 8) & 0xFF);
    data.push_back(service_id & 0xFF);

    // International code (8-bit) - byte 3
    data.push_back(international_code);

    // Programme type (5-bit) + Language code (8-bit)
    // Byte 4: [5 bits PTy (7-3)][3 bits Lang upper (2-0)]
    // Byte 5: [5 bits Lang lower (7-3)][3 bits reserved (2-0)]
    uint8_t pty_masked = (programme_type & 0x1F) << 3;  // PTy in bits 7-3
    uint8_t lang_upper = (language_code >> 5) & 0x07;   // Upper 3 bits of Lang
    uint8_t byte4 = pty_masked | lang_upper;
    data.push_back(byte4);

    uint8_t lang_lower = (language_code & 0x1F) << 3;   // Lower 5 bits of Lang
    uint8_t byte5 = lang_lower;
    data.push_back(byte5);

    return data;
}

// ==============================================================================
// BASIC PARSING TESTS
// ==============================================================================

void TestFig016ProgrammeTypeIntl::test_basic_parsing_rds_compatible() {
    qDebug() << "TEST: Basic parsing - RDS PTy Compatible (0x00)";

    auto fig_data = createFig016Data(0xC221, 0x00, 1, 0x15);  // News, English
    auto pty_intl = parser_->parseFig016_ProgrammeTypeInternational(fig_data);

    QVERIFY(pty_intl.is_valid);
    QCOMPARE(pty_intl.service_id, static_cast<uint32_t>(0xC221));
    QCOMPARE(pty_intl.international_code, static_cast<uint8_t>(0x00));
    QCOMPARE(pty_intl.programme_type, static_cast<uint8_t>(1));
    QCOMPARE(pty_intl.language_code, static_cast<uint8_t>(0x15));
    QCOMPARE(QString::fromStdString(pty_intl.getInternationalTypeName()),
             QString("RDS PTy Compatible"));
    QCOMPARE(QString::fromStdString(pty_intl.getProgrammeTypeName()),
             QString("News"));
}

void TestFig016ProgrammeTypeIntl::test_basic_parsing_rbds() {
    qDebug() << "TEST: Basic parsing - RBDS North America (0x01)";

    auto fig_data = createFig016Data(0xC001, 0x01, 4, 0x15);  // Sport, English
    auto pty_intl = parser_->parseFig016_ProgrammeTypeInternational(fig_data);

    QVERIFY(pty_intl.is_valid);
    QCOMPARE(pty_intl.service_id, static_cast<uint32_t>(0xC001));
    QCOMPARE(pty_intl.international_code, static_cast<uint8_t>(0x01));
    QCOMPARE(pty_intl.programme_type, static_cast<uint8_t>(4));
    QCOMPARE(pty_intl.language_code, static_cast<uint8_t>(0x15));
    QCOMPARE(QString::fromStdString(pty_intl.getInternationalTypeName()),
             QString("RBDS (North America)"));
}

void TestFig016ProgrammeTypeIntl::test_basic_parsing_proprietary() {
    qDebug() << "TEST: Basic parsing - Proprietary/Regional (0x80)";

    auto fig_data = createFig016Data(0xC002, 0x80, 10, 0x54);  // Pop Music, Thai
    auto pty_intl = parser_->parseFig016_ProgrammeTypeInternational(fig_data);

    QVERIFY(pty_intl.is_valid);
    QCOMPARE(pty_intl.international_code, static_cast<uint8_t>(0x80));
    QCOMPARE(QString::fromStdString(pty_intl.getInternationalTypeName()),
             QString("Proprietary/Regional"));
}

// ==============================================================================
// INTERNATIONAL CODE EXTRACTION TESTS
// ==============================================================================

void TestFig016ProgrammeTypeIntl::test_intl_code_rds() {
    qDebug() << "TEST: International code extraction - RDS (0x00)";

    auto fig_data = createFig016Data(0xC221, 0x00, 1, 0x15);
    auto pty_intl = parser_->parseFig016_ProgrammeTypeInternational(fig_data);

    QVERIFY(pty_intl.is_valid);
    QCOMPARE(pty_intl.international_code, static_cast<uint8_t>(0x00));
    QVERIFY(pty_intl.isValidCode());
}

void TestFig016ProgrammeTypeIntl::test_intl_code_reserved() {
    qDebug() << "TEST: International code extraction - Reserved (0x50)";

    auto fig_data = createFig016Data(0xC221, 0x50, 1, 0x15);
    auto pty_intl = parser_->parseFig016_ProgrammeTypeInternational(fig_data);

    QVERIFY(pty_intl.is_valid);
    QCOMPARE(pty_intl.international_code, static_cast<uint8_t>(0x50));
    QCOMPARE(QString::fromStdString(pty_intl.getInternationalTypeName()),
             QString("Reserved"));
}

// ==============================================================================
// PROGRAMME TYPE EXTRACTION TESTS
// ==============================================================================

void TestFig016ProgrammeTypeIntl::test_pty_news() {
    qDebug() << "TEST: Programme type extraction - News (1)";

    auto fig_data = createFig016Data(0xC221, 0x00, 1, 0x15);
    auto pty_intl = parser_->parseFig016_ProgrammeTypeInternational(fig_data);

    QVERIFY(pty_intl.is_valid);
    QCOMPARE(pty_intl.programme_type, static_cast<uint8_t>(1));
    QCOMPARE(QString::fromStdString(pty_intl.getProgrammeTypeName()),
             QString("News"));
}

void TestFig016ProgrammeTypeIntl::test_pty_sport() {
    qDebug() << "TEST: Programme type extraction - Sport (4)";

    auto fig_data = createFig016Data(0xC221, 0x01, 4, 0x15);
    auto pty_intl = parser_->parseFig016_ProgrammeTypeInternational(fig_data);

    QVERIFY(pty_intl.is_valid);
    QCOMPARE(pty_intl.programme_type, static_cast<uint8_t>(4));
    QCOMPARE(QString::fromStdString(pty_intl.getProgrammeTypeName()),
             QString("Sport"));
}

// ==============================================================================
// EDGE CASE TESTS
// ==============================================================================

void TestFig016ProgrammeTypeIntl::test_insufficient_data() {
    qDebug() << "TEST: Insufficient data (< 6 bytes)";

    std::vector<uint8_t> short_data = {0x10, 0xC2, 0x21, 0x00, 0x08};  // Only 5 bytes
    auto pty_intl = parser_->parseFig016_ProgrammeTypeInternational(short_data);

    QVERIFY(!pty_intl.is_valid);
}

void TestFig016ProgrammeTypeIntl::test_invalid_service_id_zero() {
    qDebug() << "TEST: Invalid Service ID (0x0000)";

    auto fig_data = createFig016Data(0x0000, 0x00, 1, 0x15);
    auto pty_intl = parser_->parseFig016_ProgrammeTypeInternational(fig_data);

    QVERIFY(!pty_intl.is_valid);
}

// ==============================================================================
// PERFORMANCE TEST
// ==============================================================================

void TestFig016ProgrammeTypeIntl::test_parsing_performance() {
    // The benchmark measures parsing, not per-call logging: keep the logger
    // quiet for the timed section (restored on scope exit).
    struct LogLevelGuard {
        Logger::LogLevel saved = Logger::instance().getLogLevel();
        LogLevelGuard() { Logger::instance().setLogLevel(Logger::LogLevel::Warning); }
        ~LogLevelGuard() { Logger::instance().setLogLevel(saved); }
    } logGuard;

    qDebug() << "TEST: Parsing performance benchmark";

    auto fig_data = createFig016Data(0xC221, 0x00, 4, 0x15);

    // Warm-up
    for (int i = 0; i < 10; ++i) {
        parser_->parseFig016_ProgrammeTypeInternational(fig_data);
    }

    // Benchmark
    constexpr int iterations = 1000;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        auto pty_intl = parser_->parseFig016_ProgrammeTypeInternational(fig_data);
        QVERIFY(pty_intl.is_valid);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time);

    double avg_time_us = static_cast<double>(duration.count()) / iterations;

    qDebug() << QString("Average parsing time: %1 µs per FIG 0/16")
        .arg(avg_time_us, 0, 'f', 2);

    // Performance target: < 15 µs per parse
    QVERIFY2(avg_time_us < 15.0,
        QString("Performance target failed: %1 µs > 15 µs")
        .arg(avg_time_us, 0, 'f', 2).toUtf8().constData());
}

// Register metatype for signal/slot
Q_DECLARE_METATYPE(ProgrammeTypeInternational)

QTEST_MAIN(TestFig016ProgrammeTypeIntl)
#include "test_fig016_programme_type_intl.moc"
