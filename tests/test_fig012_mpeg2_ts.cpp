/**
 * @file test_fig012_mpeg2_ts.cpp
 * @brief Comprehensive unit tests for FIG 0/12 MPEG-2 TS Streaming parsing
 *
 * Tests FIG 0/12 parsing according to ETSI EN 300 401 Section 8.1.10
 * with comprehensive coverage of:
 * - Service ID extraction (32-bit)
 * - MSC flag parsing
 * - Sub-channel ID extraction (6-bit)
 * - MPEG-2 TS frame size extraction (16-bit)
 * - Invalid data handling
 * - Performance benchmarks
 *
 * @author Agent 27 - Python Pro (PDCA Week 7 - Batch 4)
 * @date November 4, 2025
 */

#include <QtTest/QtTest>
#include "../src/core/fig_parser.hpp"
#include "../src/utils/logger.h"
#include <chrono>
#include <vector>

using namespace eti::fig;

class TestFig012Mpeg2Ts : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Basic parsing tests (3 tests)
    void test_basic_parsing();
    void test_msc_flag_set();
    void test_msc_flag_clear();

    // MSC flag handling tests (2 tests)
    void test_msc_transport();
    void test_non_msc_transport();

    // Frame size extraction tests (2 tests)
    void test_frame_size_188_bytes();
    void test_frame_size_large();

    // Edge case tests (2 tests)
    void test_insufficient_data();
    void test_service_id_zero();

    // Performance test (1 test)
    void test_parsing_performance();

private:
    FigParser* parser_;

    // Helper to create FIG 0/12 data
    std::vector<uint8_t> createFig012(
        uint32_t service_id,
        bool msc_flag,
        uint8_t sub_ch_id,
        uint16_t mpeg_frame_size);
};

void TestFig012Mpeg2Ts::initTestCase() {
    parser_ = new FigParser();
    QVERIFY(parser_->initialize(true, false));
}

void TestFig012Mpeg2Ts::cleanupTestCase() {
    delete parser_;
}

std::vector<uint8_t> TestFig012Mpeg2Ts::createFig012(
    uint32_t service_id,
    bool msc_flag,
    uint8_t sub_ch_id,
    uint16_t mpeg_frame_size) {
    
    std::vector<uint8_t> data;
    data.push_back(0x0C);  // Extension byte (FIG 0/12)

    // Bytes 1-4: Service ID (32-bit, big-endian)
    data.push_back((service_id >> 24) & 0xFF);
    data.push_back((service_id >> 16) & 0xFF);
    data.push_back((service_id >> 8) & 0xFF);
    data.push_back(service_id & 0xFF);

    // Byte 5: [MSC flag 1-bit][SubChId 6-bit][RFA 1-bit]
    uint8_t byte5 = 0;
    if (msc_flag) {
        byte5 |= 0x80;  // Set MSC flag (bit 7)
    }
    byte5 |= ((sub_ch_id & 0x3F) << 1);  // SubChId in bits 6-1
    data.push_back(byte5);

    // Bytes 6-7: MPEG-2 TS frame size (16-bit, big-endian)
    data.push_back((mpeg_frame_size >> 8) & 0xFF);
    data.push_back(mpeg_frame_size & 0xFF);

    return data;
}

void TestFig012Mpeg2Ts::test_basic_parsing() {
    // Standard MPEG-2 TS packet: 188 bytes
    auto data = createFig012(0x12345678, true, 10, 188);
    auto result = parser_->parseFig012_Mpeg2TsStreaming(data);

    QVERIFY(result.is_valid);
    QCOMPARE(result.service_id, static_cast<uint32_t>(0x12345678));
    QCOMPARE(result.msc_flag, true);
    QCOMPARE(result.sub_ch_id, static_cast<uint8_t>(10));
    QCOMPARE(result.mpeg_frame_size, static_cast<uint16_t>(188));
}

void TestFig012Mpeg2Ts::test_msc_flag_set() {
    auto data = createFig012(0xABCDEF01, true, 20, 376);
    auto result = parser_->parseFig012_Mpeg2TsStreaming(data);

    QVERIFY(result.is_valid);
    QVERIFY(result.isMscTransport());
    QCOMPARE(result.msc_flag, true);
}

void TestFig012Mpeg2Ts::test_msc_flag_clear() {
    auto data = createFig012(0x11223344, false, 5, 188);
    auto result = parser_->parseFig012_Mpeg2TsStreaming(data);

    QVERIFY(result.is_valid);
    QVERIFY(!result.isMscTransport());
    QCOMPARE(result.msc_flag, false);
}

void TestFig012Mpeg2Ts::test_msc_transport() {
    auto data = createFig012(0xDEADBEEF, true, 63, 512);  // Max SubChId
    auto result = parser_->parseFig012_Mpeg2TsStreaming(data);

    QVERIFY(result.is_valid);
    QCOMPARE(result.isMscTransport(), true);
    QCOMPARE(result.sub_ch_id, static_cast<uint8_t>(63));
}

void TestFig012Mpeg2Ts::test_non_msc_transport() {
    auto data = createFig012(0xCAFEBABE, false, 0, 188);
    auto result = parser_->parseFig012_Mpeg2TsStreaming(data);

    QVERIFY(result.is_valid);
    QCOMPARE(result.isMscTransport(), false);
    QCOMPARE(result.sub_ch_id, static_cast<uint8_t>(0));
}

void TestFig012Mpeg2Ts::test_frame_size_188_bytes() {
    // Standard MPEG-2 TS packet size
    auto data = createFig012(0x99887766, true, 15, 188);
    auto result = parser_->parseFig012_Mpeg2TsStreaming(data);

    QVERIFY(result.is_valid);
    QCOMPARE(result.getFrameSize(), static_cast<uint16_t>(188));
    QVERIFY(result.validateFrameSize());
}

void TestFig012Mpeg2Ts::test_frame_size_large() {
    // Large frame size (65535 bytes max for 16-bit)
    auto data = createFig012(0x55443322, true, 30, 65535);
    auto result = parser_->parseFig012_Mpeg2TsStreaming(data);

    QVERIFY(result.is_valid);
    QCOMPARE(result.getFrameSize(), static_cast<uint16_t>(65535));
    QVERIFY(result.validateFrameSize());
}

void TestFig012Mpeg2Ts::test_insufficient_data() {
    // Only 7 bytes (need 8)
    std::vector<uint8_t> data = {0x0C, 0x12, 0x34, 0x56, 0x78, 0x80, 0x00};
    auto result = parser_->parseFig012_Mpeg2TsStreaming(data);

    QVERIFY(!result.is_valid);
}

void TestFig012Mpeg2Ts::test_service_id_zero() {
    // Service ID 0 is invalid
    auto data = createFig012(0x00000000, true, 10, 188);
    auto result = parser_->parseFig012_Mpeg2TsStreaming(data);

    QVERIFY(!result.is_valid);
}

void TestFig012Mpeg2Ts::test_parsing_performance() {
    // The benchmark measures parsing, not per-call logging: keep the logger
    // quiet for the timed section (restored on scope exit).
    struct LogLevelGuard {
        Logger::LogLevel saved = Logger::instance().getLogLevel();
        LogLevelGuard() { Logger::instance().setLogLevel(Logger::LogLevel::Warning); }
        ~LogLevelGuard() { Logger::instance().setLogLevel(saved); }
    } logGuard;

    auto data = createFig012(0x12345678, true, 10, 188);
    
    const int iterations = 10000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        auto result = parser_->parseFig012_Mpeg2TsStreaming(data);
        Q_UNUSED(result);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double avg_time_us = static_cast<double>(duration.count()) / iterations;
    
    qDebug() << "Average parsing time:" << avg_time_us << "microseconds";
    QVERIFY2(avg_time_us < 15.0, "Parsing should take less than 15 microseconds");
}

QTEST_MAIN(TestFig012Mpeg2Ts)
#include "test_fig012_mpeg2_ts.moc"
