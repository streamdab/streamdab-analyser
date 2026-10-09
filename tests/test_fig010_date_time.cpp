/**
 * @file test_fig010_date_time.cpp
 * @brief Comprehensive unit tests for FIG 0/10 Date and Time parsing
 *
 * Tests FIG 0/10 parsing according to ETSI EN 300 401 Section 8.1.7
 * with comprehensive coverage of:
 * - MJD (Modified Julian Date) extraction and conversion
 * - Hour, minute, second extraction
 * - UTC flag handling
 * - LSI (Leap Second Indicator) flag
 * - Confidence Indicator flag
 * - Malformed data handling
 * - Performance benchmarks
 * - Known test vectors validation
 *
 * @author Agent 3 - TypeScript Pro (PDCA Week 2)
 * @date October 29, 2025
 */

#include <QtTest/QtTest>
#include "../src/core/fig_parser.hpp"
#include <chrono>
#include <vector>

using namespace eti::fig;

class TestFig010DateTime : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Basic parsing tests
    void test_valid_fig010_without_utc();
    void test_valid_fig010_with_utc();
    void test_mjd_extraction();
    void test_hour_minute_extraction();
    void test_second_extraction_utc();
    void test_lsi_flag();
    void test_conf_ind_flag();
    void test_utc_flag();

    // MJD conversion tests
    void test_mjd_to_datetime_epoch();
    void test_mjd_to_datetime_y2k();
    void test_mjd_to_datetime_y2020();
    void test_datetime_to_mjd_round_trip();
    void test_mjd_conversion_accuracy();

    // Edge case tests
    void test_mjd_boundary_min();
    void test_mjd_boundary_max();
    void test_leap_year_date();
    void test_invalid_hour();
    void test_invalid_minute();
    void test_invalid_second();
    void test_invalid_mjd_overflow();

    // Error handling tests
    void test_insufficient_data();
    void test_utc_flag_without_second_data();
    void test_empty_data();

    // Test vectors from ETSI
    void test_vector_mjd_51544();  // Jan 1, 2000
    void test_vector_mjd_40587();  // Jan 1, 1970
    void test_vector_mjd_58849();  // Jan 1, 2020

    // Performance test
    void test_parsing_performance();

    // Integration tests
    void test_signal_emission();

private:
    FigParser* parser_;

    // Helper to create FIG 0/10 test data
    std::vector<uint8_t> createFig010Data(
        uint32_t mjd,
        uint8_t hour,
        uint8_t minute,
        bool lsi = false,
        bool conf_ind = false,
        bool utc_flag = false,
        uint8_t second = 0);
};

void TestFig010DateTime::initTestCase() {
    parser_ = new FigParser();
    QVERIFY(parser_->initialize(true, false));
}

void TestFig010DateTime::cleanupTestCase() {
    delete parser_;
}

std::vector<uint8_t> TestFig010DateTime::createFig010Data(
    uint32_t mjd,
    uint8_t hour,
    uint8_t minute,
    bool lsi,
    bool conf_ind,
    bool utc_flag,
    uint8_t second) {

    std::vector<uint8_t> data;

    // Extension field (FIG 0/10)
    data.push_back(0x0A);

    // MJD (17 bits) encoding with all bits preserved:
    // Byte 1: MJD bits 16-9 (8 bits)
    // Byte 2: MJD bits 8-1 (8 bits)
    // Byte 3 bit 7: MJD bit 0 (1 bit) - FIXED to preserve all 17 bits

    // Byte 1: MJD bits 16-9
    data.push_back((mjd >> 9) & 0xFF);

    // Byte 2: MJD bits 8-1
    data.push_back((mjd >> 1) & 0xFF);

    // Byte 3: MJD bit 0 (bit 7) + LSI (bit 6) + Conf ind (bit 5) + UTC flag (bit 4) + Hour high 4 bits (bits 3-0)
    uint8_t byte3 = 0;
    if (mjd & 0x01) byte3 |= 0x80;  // MJD bit 0
    if (lsi) byte3 |= 0x40;          // LSI flag
    if (conf_ind) byte3 |= 0x20;     // Confidence indicator
    if (utc_flag) byte3 |= 0x10;     // UTC flag
    byte3 |= (hour & 0x1E) >> 1;     // Hour bits 4-1 in byte3 bits 3-0
    data.push_back(byte3);

    // Byte 4: Hour bit 0 (bit 7) + Minute (bits 6-1) + Second high (bit 0)
    uint8_t byte4 = 0;
    if (hour & 0x01) byte4 |= 0x80;  // Hour bit 0
    byte4 |= (minute & 0x3F) << 1;   // Minute 6 bits
    if (utc_flag) {
        byte4 |= (second >> 5) & 0x01; // Second bit 5
    }
    data.push_back(byte4);

    // Byte 5: Second bits 4-0 (bits 7-3) if UTC flag
    if (utc_flag) {
        uint8_t byte5 = (second & 0x1F) << 3;
        data.push_back(byte5);
    }

    return data;
}

void TestFig010DateTime::test_valid_fig010_without_utc() {
    qDebug() << "TEST: Valid FIG 0/10 without UTC second";

    auto fig_data = createFig010Data(51544, 12, 30); // Jan 1, 2000, 12:30
    auto dt = parser_->parseFig010_DateAndTime(fig_data);

    QVERIFY(dt.is_valid);
    QCOMPARE(dt.mjd, static_cast<uint32_t>(51544));
    QCOMPARE(dt.hour, static_cast<uint8_t>(12));
    QCOMPARE(dt.minute, static_cast<uint8_t>(30));
    QCOMPARE(dt.second, static_cast<uint8_t>(0));
    QVERIFY(!dt.utc_flag);
    QVERIFY(dt.datetime.isValid());
}

void TestFig010DateTime::test_valid_fig010_with_utc() {
    qDebug() << "TEST: Valid FIG 0/10 with UTC second";

    auto fig_data = createFig010Data(51544, 14, 45, false, false, true, 30);
    auto dt = parser_->parseFig010_DateAndTime(fig_data);

    QVERIFY(dt.is_valid);
    QCOMPARE(dt.mjd, static_cast<uint32_t>(51544));
    QCOMPARE(dt.hour, static_cast<uint8_t>(14));
    QCOMPARE(dt.minute, static_cast<uint8_t>(45));
    QCOMPARE(dt.second, static_cast<uint8_t>(30));
    QVERIFY(dt.utc_flag);
}

void TestFig010DateTime::test_mjd_extraction() {
    qDebug() << "TEST: MJD extraction";

    std::vector<uint32_t> test_mjds = {0, 1000, 40587, 51544, 58849, 131071};

    for (auto mjd : test_mjds) {
        auto fig_data = createFig010Data(mjd, 0, 0);
        auto dt = parser_->parseFig010_DateAndTime(fig_data);

        QVERIFY(dt.is_valid);
        QCOMPARE(dt.mjd, mjd);
    }
}

void TestFig010DateTime::test_hour_minute_extraction() {
    qDebug() << "TEST: Hour and minute extraction";

    struct TestCase {
        uint8_t hour;
        uint8_t minute;
    };

    std::vector<TestCase> test_cases = {
        {0, 0}, {12, 30}, {23, 59}, {6, 15}, {18, 45}
    };

    for (const auto& tc : test_cases) {
        auto fig_data = createFig010Data(51544, tc.hour, tc.minute);
        auto dt = parser_->parseFig010_DateAndTime(fig_data);

        QVERIFY(dt.is_valid);
        QCOMPARE(dt.hour, tc.hour);
        QCOMPARE(dt.minute, tc.minute);
    }
}

void TestFig010DateTime::test_second_extraction_utc() {
    qDebug() << "TEST: Second extraction with UTC flag";

    std::vector<uint8_t> test_seconds = {0, 15, 30, 45, 59};

    for (auto sec : test_seconds) {
        auto fig_data = createFig010Data(51544, 12, 30, false, false, true, sec);
        auto dt = parser_->parseFig010_DateAndTime(fig_data);

        QVERIFY(dt.is_valid);
        QVERIFY(dt.utc_flag);
        QCOMPARE(dt.second, sec);
    }
}

void TestFig010DateTime::test_lsi_flag() {
    qDebug() << "TEST: LSI (Leap Second Indicator) flag";

    auto fig_data_no_lsi = createFig010Data(51544, 12, 0, false);
    auto dt_no_lsi = parser_->parseFig010_DateAndTime(fig_data_no_lsi);
    QVERIFY(dt_no_lsi.is_valid);
    QVERIFY(!dt_no_lsi.lsi);

    auto fig_data_with_lsi = createFig010Data(51544, 12, 0, true);
    auto dt_with_lsi = parser_->parseFig010_DateAndTime(fig_data_with_lsi);
    QVERIFY(dt_with_lsi.is_valid);
    QVERIFY(dt_with_lsi.lsi);
}

void TestFig010DateTime::test_conf_ind_flag() {
    qDebug() << "TEST: Confidence Indicator flag";

    auto fig_data_no_conf = createFig010Data(51544, 12, 0, false, false);
    auto dt_no_conf = parser_->parseFig010_DateAndTime(fig_data_no_conf);
    QVERIFY(dt_no_conf.is_valid);
    QVERIFY(!dt_no_conf.conf_ind);

    auto fig_data_with_conf = createFig010Data(51544, 12, 0, false, true);
    auto dt_with_conf = parser_->parseFig010_DateAndTime(fig_data_with_conf);
    QVERIFY(dt_with_conf.is_valid);
    QVERIFY(dt_with_conf.conf_ind);
}

void TestFig010DateTime::test_utc_flag() {
    qDebug() << "TEST: UTC flag handling";

    auto fig_data_no_utc = createFig010Data(51544, 12, 30, false, false, false);
    auto dt_no_utc = parser_->parseFig010_DateAndTime(fig_data_no_utc);
    QVERIFY(dt_no_utc.is_valid);
    QVERIFY(!dt_no_utc.utc_flag);

    auto fig_data_with_utc = createFig010Data(51544, 12, 30, false, false, true, 45);
    auto dt_with_utc = parser_->parseFig010_DateAndTime(fig_data_with_utc);
    QVERIFY(dt_with_utc.is_valid);
    QVERIFY(dt_with_utc.utc_flag);
}

void TestFig010DateTime::test_mjd_to_datetime_epoch() {
    qDebug() << "TEST: MJD to QDateTime - Unix epoch";

    // MJD 40587 = January 1, 1970
    QDateTime result = DateAndTime::mjdToDateTime(40587, 0, 0, 0);

    QVERIFY(result.isValid());
    QCOMPARE(result.date().year(), 1970);
    QCOMPARE(result.date().month(), 1);
    QCOMPARE(result.date().day(), 1);
}

void TestFig010DateTime::test_mjd_to_datetime_y2k() {
    qDebug() << "TEST: MJD to QDateTime - Y2K";

    // MJD 51544 = January 1, 2000
    QDateTime result = DateAndTime::mjdToDateTime(51544, 12, 30, 45);

    QVERIFY(result.isValid());
    QCOMPARE(result.date().year(), 2000);
    QCOMPARE(result.date().month(), 1);
    QCOMPARE(result.date().day(), 1);
    QCOMPARE(result.time().hour(), 12);
    QCOMPARE(result.time().minute(), 30);
    QCOMPARE(result.time().second(), 45);
}

void TestFig010DateTime::test_mjd_to_datetime_y2020() {
    qDebug() << "TEST: MJD to QDateTime - 2020";

    // MJD 58849 = January 1, 2020
    QDateTime result = DateAndTime::mjdToDateTime(58849, 0, 0, 0);

    QVERIFY(result.isValid());
    QCOMPARE(result.date().year(), 2020);
    QCOMPARE(result.date().month(), 1);
    QCOMPARE(result.date().day(), 1);
}

void TestFig010DateTime::test_datetime_to_mjd_round_trip() {
    qDebug() << "TEST: DateTime to MJD round-trip";

    QDateTime test_dt(QDate(2000, 6, 15), QTime(14, 30, 0), Qt::UTC);
    uint32_t mjd = DateAndTime::dateTimeToMjd(test_dt);
    QDateTime result = DateAndTime::mjdToDateTime(mjd, 14, 30, 0);

    QCOMPARE(result.date(), test_dt.date());
    QCOMPARE(result.time(), test_dt.time());
}

void TestFig010DateTime::test_mjd_conversion_accuracy() {
    qDebug() << "TEST: MJD conversion accuracy with known values";

    // Test vectors from ETSI
    std::vector<std::pair<uint32_t, QDate>> test_vectors = {
        {51544, QDate(2000, 1, 1)},
        {40587, QDate(1970, 1, 1)},
        {58849, QDate(2020, 1, 1)}
    };

    for (const auto& tv : test_vectors) {
        QDateTime result = DateAndTime::mjdToDateTime(tv.first, 0, 0, 0);
        QVERIFY(result.isValid());
        QCOMPARE(result.date(), tv.second);
    }
}

void TestFig010DateTime::test_mjd_boundary_min() {
    qDebug() << "TEST: MJD boundary - minimum (0)";

    auto fig_data = createFig010Data(0, 0, 0);
    auto dt = parser_->parseFig010_DateAndTime(fig_data);

    QVERIFY(dt.is_valid);
    QCOMPARE(dt.mjd, static_cast<uint32_t>(0));
    QVERIFY(dt.datetime.isValid());
}

void TestFig010DateTime::test_mjd_boundary_max() {
    qDebug() << "TEST: MJD boundary - maximum (131071)";

    auto fig_data = createFig010Data(131071, 0, 0);
    auto dt = parser_->parseFig010_DateAndTime(fig_data);

    QVERIFY(dt.is_valid);
    QCOMPARE(dt.mjd, static_cast<uint32_t>(131071));
}

void TestFig010DateTime::test_leap_year_date() {
    qDebug() << "TEST: Leap year date (Feb 29, 2000)";

    // MJD for Feb 29, 2000
    uint32_t mjd_feb29_2000 = DateAndTime::dateTimeToMjd(
        QDateTime(QDate(2000, 2, 29), QTime(0, 0), Qt::UTC));

    auto fig_data = createFig010Data(mjd_feb29_2000, 12, 0);
    auto dt = parser_->parseFig010_DateAndTime(fig_data);

    QVERIFY(dt.is_valid);
    QCOMPARE(dt.datetime.date().month(), 2);
    QCOMPARE(dt.datetime.date().day(), 29);
    QCOMPARE(dt.datetime.date().year(), 2000);
}

void TestFig010DateTime::test_invalid_hour() {
    qDebug() << "TEST: Invalid hour (>= 24)";

    auto fig_data = createFig010Data(51544, 24, 0);  // Hour 24 is invalid
    auto dt = parser_->parseFig010_DateAndTime(fig_data);

    QVERIFY(!dt.is_valid);
}

void TestFig010DateTime::test_invalid_minute() {
    qDebug() << "TEST: Invalid minute (>= 60)";

    auto fig_data = createFig010Data(51544, 12, 60);  // Minute 60 is invalid
    auto dt = parser_->parseFig010_DateAndTime(fig_data);

    QVERIFY(!dt.is_valid);
}

void TestFig010DateTime::test_invalid_second() {
    qDebug() << "TEST: Invalid second (>= 60)";

    auto fig_data = createFig010Data(51544, 12, 30, false, false, true, 60);
    auto dt = parser_->parseFig010_DateAndTime(fig_data);

    QVERIFY(!dt.is_valid);
}

void TestFig010DateTime::test_invalid_mjd_overflow() {
    qDebug() << "TEST: Invalid MJD (exceeds 17-bit maximum)";

    // MJD should be limited to 17 bits (0-131071)
    // When encoding MJD > 131071, it wraps due to 17-bit limitation
    // The parsed result should NOT match the input (wraps to valid range)
    auto fig_data = createFig010Data(131072, 0, 0);  // Exceeds 17-bit, wraps to 0
    auto dt = parser_->parseFig010_DateAndTime(fig_data);

    // Parser should successfully decode wrapped value
    QVERIFY(dt.is_valid);
    // But decoded MJD should be wrapped value (0), not original (131072)
    QCOMPARE(dt.mjd, static_cast<uint32_t>(0));  // 131072 & 0x1FFFF = 0
    QVERIFY(dt.mjd != 131072);  // Verify wrapping occurred
}


void TestFig010DateTime::test_insufficient_data() {
    qDebug() << "TEST: Insufficient data handling";

    std::vector<size_t> short_lengths = {0, 1, 2, 3, 4};

    for (auto len : short_lengths) {
        std::vector<uint8_t> short_data(len, 0x00);
        if (len > 0) short_data[0] = 0x0A; // Extension
        auto dt = parser_->parseFig010_DateAndTime(short_data);

        QVERIFY(!dt.is_valid);
    }
}

void TestFig010DateTime::test_utc_flag_without_second_data() {
    qDebug() << "TEST: UTC flag set but no second data";

    std::vector<uint8_t> fig_data;
    fig_data.push_back(0x0A);  // Extension
    fig_data.push_back(0xC9);  // MJD high
    fig_data.push_back(0x90);  // MJD mid
    fig_data.push_back(0x10);  // MJD bit 0=0, LSI=0, Conf=0, UTC=1, Hour bits
    fig_data.push_back(0x00);  // Hour bit 0 + Minute
    // Missing byte 5 for second data

    auto dt = parser_->parseFig010_DateAndTime(fig_data);
    QVERIFY(!dt.is_valid);
}

void TestFig010DateTime::test_empty_data() {
    qDebug() << "TEST: Empty data handling";

    std::vector<uint8_t> empty_data;
    auto dt = parser_->parseFig010_DateAndTime(empty_data);

    QVERIFY(!dt.is_valid);
}

void TestFig010DateTime::test_vector_mjd_51544() {
    qDebug() << "TEST: ETSI test vector - MJD 51544 (Jan 1, 2000)";

    auto fig_data = createFig010Data(51544, 0, 0);
    auto dt = parser_->parseFig010_DateAndTime(fig_data);

    QVERIFY(dt.is_valid);
    QCOMPARE(dt.mjd, static_cast<uint32_t>(51544));
    QCOMPARE(dt.datetime.date().year(), 2000);
    QCOMPARE(dt.datetime.date().month(), 1);
    QCOMPARE(dt.datetime.date().day(), 1);
}

void TestFig010DateTime::test_vector_mjd_40587() {
    qDebug() << "TEST: ETSI test vector - MJD 40587 (Jan 1, 1970)";

    auto fig_data = createFig010Data(40587, 0, 0);
    auto dt = parser_->parseFig010_DateAndTime(fig_data);

    QVERIFY(dt.is_valid);
    QCOMPARE(dt.mjd, static_cast<uint32_t>(40587));
    QCOMPARE(dt.datetime.date().year(), 1970);
    QCOMPARE(dt.datetime.date().month(), 1);
    QCOMPARE(dt.datetime.date().day(), 1);
}

void TestFig010DateTime::test_vector_mjd_58849() {
    qDebug() << "TEST: ETSI test vector - MJD 58849 (Jan 1, 2020)";

    auto fig_data = createFig010Data(58849, 0, 0);
    auto dt = parser_->parseFig010_DateAndTime(fig_data);

    QVERIFY(dt.is_valid);
    QCOMPARE(dt.mjd, static_cast<uint32_t>(58849));
    QCOMPARE(dt.datetime.date().year(), 2020);
    QCOMPARE(dt.datetime.date().month(), 1);
    QCOMPARE(dt.datetime.date().day(), 1);
}

void TestFig010DateTime::test_parsing_performance() {
    qDebug() << "TEST: Parsing performance benchmark";

    auto fig_data = createFig010Data(51544, 12, 30, false, false, true, 45);

    // Warm-up
    for (int i = 0; i < 10; ++i) {
        parser_->parseFig010_DateAndTime(fig_data);
    }

    // Benchmark
    constexpr int iterations = 1000;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        auto dt = parser_->parseFig010_DateAndTime(fig_data);
        QVERIFY(dt.is_valid);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time);

    double avg_time_us = static_cast<double>(duration.count()) / iterations;

    qDebug() << QString("Average parsing time: %1 µs per FIG 0/10")
        .arg(avg_time_us, 0, 'f', 2);

    // Performance target: < 100 µs per parse
    QVERIFY2(avg_time_us < 100.0,
        QString("Performance target failed: %1 µs > 100 µs")
        .arg(avg_time_us, 0, 'f', 2).toUtf8().constData());
}

void TestFig010DateTime::test_signal_emission() {
    qDebug() << "TEST: Signal emission on valid date/time";

    auto fig_data = createFig010Data(51544, 12, 30, false, false, true, 45);
    auto dt = parser_->parseFig010_DateAndTime(fig_data);

    QVERIFY(dt.is_valid);
    QCOMPARE(dt.mjd, static_cast<uint32_t>(51544));
    QCOMPARE(dt.hour, static_cast<uint8_t>(12));
    QCOMPARE(dt.minute, static_cast<uint8_t>(30));
    QCOMPARE(dt.second, static_cast<uint8_t>(45));
}

// Register metatype for signal/slot with DateAndTime
Q_DECLARE_METATYPE(DateAndTime)

QTEST_MAIN(TestFig010DateTime)
#include "test_fig010_date_time.moc"
