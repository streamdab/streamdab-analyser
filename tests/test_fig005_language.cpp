/**
 * @file test_fig005_language.cpp
 * @brief Comprehensive unit tests for FIG 0/5 Service Component Language parsing
 *
 * Tests FIG 0/5 parsing according to ETSI EN 300 401 Section 8.1.2
 * with comprehensive coverage of:
 * - Short form (MSC stream audio) parsing
 * - Long form (packet mode) parsing with 16-bit and 32-bit SId
 * - Language code extraction (ISO 639-2 / ETSI TS 101 756 Table 9)
 * - Thai language support (code 0x45)
 * - Invalid data handling
 * - Performance benchmarks
 *
 * @author Agent 8 - TypeScript Pro (PDCA Week 3)
 * @date October 29, 2025
 */

#include <QtTest/QtTest>
#include "../src/core/fig_parser.hpp"
#include <chrono>
#include <vector>

using namespace eti::fig;

class TestFig005Language : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Short form tests (6 tests)
    void test_short_form_english();
    void test_short_form_thai();
    void test_short_form_unknown();
    void test_short_form_insufficient_data();
    void test_short_form_sc_ids_boundary();
    void test_short_form_language_boundary();

    // Long form 16-bit SId tests (4 tests)
    void test_long_form_16bit_english();
    void test_long_form_16bit_thai();
    void test_long_form_16bit_french();
    void test_long_form_16bit_insufficient_data();

    // Long form 32-bit SId tests (4 tests)
    void test_long_form_32bit_german();
    void test_long_form_32bit_thai();
    void test_long_form_32bit_sid_zero();
    void test_long_form_32bit_insufficient_data();

    // Language lookup tests (4 tests)
    void test_language_lookup_english();
    void test_language_lookup_thai();
    void test_language_lookup_french();
    void test_language_lookup_unknown();

    // Edge case tests (2 tests)
    void test_empty_data();
    void test_minimal_invalid_data();

    // Performance test (1 test)
    void test_parsing_performance();

private:
    FigParser* parser_;

    // Helper to create short form FIG 0/5 data
    std::vector<uint8_t> createShortFormFig005(
        uint16_t sc_ids,
        uint8_t language_code);

    // Helper to create long form FIG 0/5 data (16-bit SId)
    std::vector<uint8_t> createLongForm16BitFig005(
        uint16_t service_id,
        uint16_t sc_ids,
        uint8_t language_code);

    // Helper to create long form FIG 0/5 data (32-bit SId)
    std::vector<uint8_t> createLongForm32BitFig005(
        uint32_t service_id,
        uint16_t sc_ids,
        uint8_t language_code);
};

void TestFig005Language::initTestCase() {
    parser_ = new FigParser();
    QVERIFY(parser_->initialize(true, false));  // Enable Thai support
}

void TestFig005Language::cleanupTestCase() {
    delete parser_;
}

std::vector<uint8_t> TestFig005Language::createShortFormFig005(
    uint16_t sc_ids,
    uint8_t language_code) {
    
    std::vector<uint8_t> data;
    data.push_back(0x05);  // Extension byte (FIG 0/5)

    // Short form: L/S = 1
    // Byte 0: [1 L/S=1][7 bits MSB of SCIdS]
    uint8_t byte0 = 0x80 | ((sc_ids >> 5) & 0x7F);
    data.push_back(byte0);

    // Byte 1: [5 bits LSB of SCIdS][3 bits MSB of Language]
    uint8_t byte1 = ((sc_ids & 0x1F) << 3) | ((language_code >> 5) & 0x07);
    data.push_back(byte1);

    // Byte 2: [5 bits LSB of Language][3 bits reserved]
    uint8_t byte2 = (language_code & 0x1F) << 3;
    data.push_back(byte2);

    return data;
}

std::vector<uint8_t> TestFig005Language::createLongForm16BitFig005(
    uint16_t service_id,
    uint16_t sc_ids,
    uint8_t language_code) {
    
    std::vector<uint8_t> data;
    data.push_back(0x05);  // Extension byte

    // Long form: L/S = 0, P/D = 0 (16-bit SId)
    data.push_back(0x00);  // Flags byte

    // 16-bit SId
    data.push_back((service_id >> 8) & 0xFF);
    data.push_back(service_id & 0xFF);

    // 12-bit SCIdS + 4-bit reserved
    data.push_back((sc_ids >> 4) & 0xFF);
    data.push_back((sc_ids << 4) & 0xF0);

    // 8-bit Language
    data.push_back(language_code);

    return data;
}

std::vector<uint8_t> TestFig005Language::createLongForm32BitFig005(
    uint32_t service_id,
    uint16_t sc_ids,
    uint8_t language_code) {
    
    std::vector<uint8_t> data;
    data.push_back(0x05);  // Extension byte

    // Long form: L/S = 0, P/D = 1 (32-bit SId)
    data.push_back(0x40);  // Flags byte with P/D=1

    // 32-bit SId
    data.push_back((service_id >> 24) & 0xFF);
    data.push_back((service_id >> 16) & 0xFF);
    data.push_back((service_id >> 8) & 0xFF);
    data.push_back(service_id & 0xFF);

    // 12-bit SCIdS + 4-bit reserved
    data.push_back((sc_ids >> 4) & 0xFF);
    data.push_back((sc_ids << 4) & 0xF0);

    // 8-bit Language
    data.push_back(language_code);

    return data;
}

// ============================================================================
// Short Form Tests
// ============================================================================

void TestFig005Language::test_short_form_english() {
    qDebug() << "TEST: Short form with English language";

    auto fig_data = createShortFormFig005(100, 0x09);  // SCIdS=100, English
    auto lang = parser_->parseFig005_ServiceComponentLanguage(fig_data);

    QVERIFY(lang.is_valid);
    QCOMPARE(lang.sc_ids, static_cast<uint16_t>(100));
    QCOMPARE(lang.language_code, static_cast<uint8_t>(0x09));
    QCOMPARE(lang.service_id, static_cast<uint32_t>(0));  // Short form
    QVERIFY(lang.is_short_form);
    QCOMPARE(QString::fromStdString(lang.getLanguageName()), QString("English"));
}

void TestFig005Language::test_short_form_thai() {
    qDebug() << "TEST: Short form with Thai language (CRITICAL)";

    auto fig_data = createShortFormFig005(200, 0x45);  // SCIdS=200, Thai
    auto lang = parser_->parseFig005_ServiceComponentLanguage(fig_data);

    QVERIFY(lang.is_valid);
    QCOMPARE(lang.sc_ids, static_cast<uint16_t>(200));
    QCOMPARE(lang.language_code, static_cast<uint8_t>(0x45));
    QVERIFY(lang.is_short_form);
    QCOMPARE(QString::fromStdString(lang.getLanguageName()), QString("Thai"));
}

void TestFig005Language::test_short_form_unknown() {
    qDebug() << "TEST: Short form with unknown language";

    auto fig_data = createShortFormFig005(50, 0x00);
    auto lang = parser_->parseFig005_ServiceComponentLanguage(fig_data);

    QVERIFY(lang.is_valid);
    QCOMPARE(lang.language_code, static_cast<uint8_t>(0x00));
    QCOMPARE(QString::fromStdString(lang.getLanguageName()), QString("Unknown"));
}

void TestFig005Language::test_short_form_insufficient_data() {
    qDebug() << "TEST: Short form insufficient data";

    std::vector<uint8_t> short_data = {0x05, 0x80};  // Only 2 bytes
    auto lang = parser_->parseFig005_ServiceComponentLanguage(short_data);

    QVERIFY(!lang.is_valid);
}

void TestFig005Language::test_short_form_sc_ids_boundary() {
    qDebug() << "TEST: Short form SCIdS boundary values";

    // Test minimum (0)
    auto fig_min = createShortFormFig005(0, 0x09);
    auto lang_min = parser_->parseFig005_ServiceComponentLanguage(fig_min);
    QVERIFY(lang_min.is_valid);
    QCOMPARE(lang_min.sc_ids, static_cast<uint16_t>(0));

    // Test maximum (4095 = 0xFFF)
    auto fig_max = createShortFormFig005(4095, 0x09);
    auto lang_max = parser_->parseFig005_ServiceComponentLanguage(fig_max);
    QVERIFY(lang_max.is_valid);
    QCOMPARE(lang_max.sc_ids, static_cast<uint16_t>(4095));
}

void TestFig005Language::test_short_form_language_boundary() {
    qDebug() << "TEST: Short form language code boundary values";

    // Test minimum (0)
    auto fig_min = createShortFormFig005(100, 0);
    auto lang_min = parser_->parseFig005_ServiceComponentLanguage(fig_min);
    QVERIFY(lang_min.is_valid);
    QCOMPARE(lang_min.language_code, static_cast<uint8_t>(0));

    // Test maximum (255)
    auto fig_max = createShortFormFig005(100, 255);
    auto lang_max = parser_->parseFig005_ServiceComponentLanguage(fig_max);
    QVERIFY(lang_max.is_valid);
    QCOMPARE(lang_max.language_code, static_cast<uint8_t>(255));
}

// ============================================================================
// Long Form 16-bit SId Tests
// ============================================================================

void TestFig005Language::test_long_form_16bit_english() {
    qDebug() << "TEST: Long form 16-bit SId with English";

    auto fig_data = createLongForm16BitFig005(0x1234, 50, 0x09);
    auto lang = parser_->parseFig005_ServiceComponentLanguage(fig_data);

    QVERIFY(lang.is_valid);
    QCOMPARE(lang.service_id, static_cast<uint32_t>(0x1234));
    QCOMPARE(lang.sc_ids, static_cast<uint16_t>(50));
    QCOMPARE(lang.language_code, static_cast<uint8_t>(0x09));
    QVERIFY(!lang.is_short_form);
    QCOMPARE(QString::fromStdString(lang.getLanguageName()), QString("English"));
}

void TestFig005Language::test_long_form_16bit_thai() {
    qDebug() << "TEST: Long form 16-bit SId with Thai";

    auto fig_data = createLongForm16BitFig005(0xABCD, 100, 0x45);
    auto lang = parser_->parseFig005_ServiceComponentLanguage(fig_data);

    QVERIFY(lang.is_valid);
    QCOMPARE(lang.service_id, static_cast<uint32_t>(0xABCD));
    QCOMPARE(lang.language_code, static_cast<uint8_t>(0x45));
    QCOMPARE(QString::fromStdString(lang.getLanguageName()), QString("Thai"));
}

void TestFig005Language::test_long_form_16bit_french() {
    qDebug() << "TEST: Long form 16-bit SId with French";

    auto fig_data = createLongForm16BitFig005(0x5678, 75, 0x0F);
    auto lang = parser_->parseFig005_ServiceComponentLanguage(fig_data);

    QVERIFY(lang.is_valid);
    QCOMPARE(lang.language_code, static_cast<uint8_t>(0x0F));
    QCOMPARE(QString::fromStdString(lang.getLanguageName()), QString("French"));
}

void TestFig005Language::test_long_form_16bit_insufficient_data() {
    qDebug() << "TEST: Long form 16-bit insufficient data";

    std::vector<uint8_t> short_data = {0x05, 0x00, 0x12, 0x34, 0x50};  // Missing last 2 bytes
    auto lang = parser_->parseFig005_ServiceComponentLanguage(short_data);

    QVERIFY(!lang.is_valid);
}

// ============================================================================
// Long Form 32-bit SId Tests
// ============================================================================

void TestFig005Language::test_long_form_32bit_german() {
    qDebug() << "TEST: Long form 32-bit SId with German";

    auto fig_data = createLongForm32BitFig005(0x12345678, 150, 0x08);
    auto lang = parser_->parseFig005_ServiceComponentLanguage(fig_data);

    QVERIFY(lang.is_valid);
    QCOMPARE(lang.service_id, static_cast<uint32_t>(0x12345678));
    QCOMPARE(lang.sc_ids, static_cast<uint16_t>(150));
    QCOMPARE(lang.language_code, static_cast<uint8_t>(0x08));
    QVERIFY(!lang.is_short_form);
    QCOMPARE(QString::fromStdString(lang.getLanguageName()), QString("German"));
}

void TestFig005Language::test_long_form_32bit_thai() {
    qDebug() << "TEST: Long form 32-bit SId with Thai (CRITICAL)";

    auto fig_data = createLongForm32BitFig005(0xABCDEF01, 200, 0x45);
    auto lang = parser_->parseFig005_ServiceComponentLanguage(fig_data);

    QVERIFY(lang.is_valid);
    QCOMPARE(lang.service_id, static_cast<uint32_t>(0xABCDEF01));
    QCOMPARE(lang.language_code, static_cast<uint8_t>(0x45));
    QCOMPARE(QString::fromStdString(lang.getLanguageName()), QString("Thai"));
}

void TestFig005Language::test_long_form_32bit_sid_zero() {
    qDebug() << "TEST: Long form 32-bit with SId=0 (should still parse)";

    auto fig_data = createLongForm32BitFig005(0, 100, 0x09);
    auto lang = parser_->parseFig005_ServiceComponentLanguage(fig_data);

    // Parser should accept SId=0 for long form (validation happens elsewhere)
    QVERIFY(lang.is_valid);
    QCOMPARE(lang.service_id, static_cast<uint32_t>(0));
}

void TestFig005Language::test_long_form_32bit_insufficient_data() {
    qDebug() << "TEST: Long form 32-bit insufficient data";

    std::vector<uint8_t> short_data = {0x05, 0x40, 0x12, 0x34, 0x56};  // Missing bytes
    auto lang = parser_->parseFig005_ServiceComponentLanguage(short_data);

    QVERIFY(!lang.is_valid);
}

// ============================================================================
// Language Lookup Tests
// ============================================================================

void TestFig005Language::test_language_lookup_english() {
    qDebug() << "TEST: Language lookup - English (0x09)";

    ServiceComponentLanguage lang;
    lang.language_code = 0x09;
    QCOMPARE(QString::fromStdString(lang.getLanguageName()), QString("English"));
}

void TestFig005Language::test_language_lookup_thai() {
    qDebug() << "TEST: Language lookup - Thai (0x45) - CRITICAL";

    ServiceComponentLanguage lang;
    lang.language_code = 0x45;
    QCOMPARE(QString::fromStdString(lang.getLanguageName()), QString("Thai"));
}

void TestFig005Language::test_language_lookup_french() {
    qDebug() << "TEST: Language lookup - French (0x0F)";

    ServiceComponentLanguage lang;
    lang.language_code = 0x0F;
    QCOMPARE(QString::fromStdString(lang.getLanguageName()), QString("French"));
}

void TestFig005Language::test_language_lookup_unknown() {
    qDebug() << "TEST: Language lookup - Unknown (0x00)";

    ServiceComponentLanguage lang;
    lang.language_code = 0x00;
    QCOMPARE(QString::fromStdString(lang.getLanguageName()), QString("Unknown"));
}

// ============================================================================
// Edge Case Tests
// ============================================================================

void TestFig005Language::test_empty_data() {
    qDebug() << "TEST: Empty data handling";

    std::vector<uint8_t> empty_data;
    auto lang = parser_->parseFig005_ServiceComponentLanguage(empty_data);

    QVERIFY(!lang.is_valid);
}

void TestFig005Language::test_minimal_invalid_data() {
    qDebug() << "TEST: Minimal invalid data (1-2 bytes)";

    std::vector<uint8_t> one_byte = {0x05};
    auto lang1 = parser_->parseFig005_ServiceComponentLanguage(one_byte);
    QVERIFY(!lang1.is_valid);

    std::vector<uint8_t> two_bytes = {0x05, 0x80};
    auto lang2 = parser_->parseFig005_ServiceComponentLanguage(two_bytes);
    QVERIFY(!lang2.is_valid);
}

// ============================================================================
// Performance Test
// ============================================================================

void TestFig005Language::test_parsing_performance() {
    qDebug() << "TEST: Parsing performance benchmark";

    auto fig_data = createShortFormFig005(100, 0x09);

    // Warm-up
    for (int i = 0; i < 10; ++i) {
        parser_->parseFig005_ServiceComponentLanguage(fig_data);
    }

    // Benchmark
    constexpr int iterations = 1000;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        auto lang = parser_->parseFig005_ServiceComponentLanguage(fig_data);
        QVERIFY(lang.is_valid);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time);

    double avg_time_us = static_cast<double>(duration.count()) / iterations;

    qDebug() << QString("Average parsing time: %1 µs per FIG 0/5")
        .arg(avg_time_us, 0, 'f', 2);

    // Performance target: < 5 µs per parse
    QVERIFY2(avg_time_us < 5.0,
        QString("Performance target failed: %1 µs > 5 µs")
        .arg(avg_time_us, 0, 'f', 2).toUtf8().constData());
}

QTEST_MAIN(TestFig005Language)
#include "test_fig005_language.moc"
