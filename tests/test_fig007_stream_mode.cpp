/**
 * @file test_fig007_stream_mode.cpp
 * @brief Comprehensive Test Suite for FIG 0/7 Service Component Stream Mode Parser
 *
 * Tests ETSI EN 300 401 Section 8.1.4 compliance for service component stream mode.
 * Part of PDCA Week 7 - Batch 3 / Agent 23 implementation.
 *
 * @author Python Pro Agent
 * @date November 3, 2025
 */

#include <QTest>
#include <QSignalSpy>
#include "../src/core/fig_parser.hpp"
#include "../src/utils/logger.h"

using namespace eti::fig;

class TestFig007ServiceComponentStreamMode : public QObject {
    Q_OBJECT

private:
    FigParser* parser;

private slots:
    void initTestCase() {
        // Initialize logger
        Logger::instance().setLogLevel(Logger::Debug);

        // Create parser
        parser = new FigParser();
        QVERIFY(parser != nullptr);

        // Initialize parser
        QVERIFY(parser->initialize(true, false));
        QVERIFY(parser->isReady());
    }

    void cleanupTestCase() {
        delete parser;
    }

    /**
     * @brief Test 1: Basic FIG 0/7 parsing without CA or DG flags
     * Validates basic SCId and SubChId extraction
     */
    void test01_BasicParsing() {
        // FIG 0/7: SCId=0x123 (291), SubChId=0x0A (10), CA=0, DG=0
        std::vector<uint8_t> fig_data = {
            0x07,        // Extension 7
            0x12, 0x30,  // SCId = 0x123 (12-bit big-endian in first 12 bits)
                         // Byte 1: 0x12 = 0001 0010
                         // Byte 2: 0x30 = 0011 0000
                         // SCId = 0001 0010 0011 = 0x123 (291 decimal)
                         // CA=0 (bit 2 of byte 2), DG=0 (bit 3 of byte 2)
            0x0A         // SubChId = 0x0A (10) in lower 6 bits
        };

        auto result = parser->parseFig007_ServiceComponentStreamMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.service_component_id, static_cast<uint16_t>(0x123));
        QCOMPARE(result.sub_ch_id, static_cast<uint8_t>(0x0A));
        QVERIFY(!result.ca_flag);
        QVERIFY(!result.dg_flag);
        QVERIFY(!result.isCaProtected());
        QVERIFY(!result.hasDataGroups());
    }

    /**
     * @brief Test 2: CA flag set
     * Tests conditional access flag extraction
     */
    void test02_CAFlagSet() {
        // FIG 0/7: SCId=0x456 (1110), SubChId=0x15 (21), CA=1, DG=0
        std::vector<uint8_t> fig_data = {
            0x07,        // Extension 7
            0x45, 0x64,  // SCId = 0x456, CA=1 (bit 2 of byte 2 = 1)
                         // Byte 1: 0x45 = 0100 0101
                         // Byte 2: 0x64 = 0110 0100
                         // SCId = 0100 0101 0110 = 0x456 (1110 decimal)
                         // CA=1 (0x64 has bit 2 set)
            0x15         // SubChId = 0x15 (21)
        };

        auto result = parser->parseFig007_ServiceComponentStreamMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.service_component_id, static_cast<uint16_t>(0x456));
        QCOMPARE(result.sub_ch_id, static_cast<uint8_t>(0x15));
        QVERIFY(result.ca_flag);
        QVERIFY(!result.dg_flag);
        QVERIFY(result.isCaProtected());
        QVERIFY(!result.hasDataGroups());
    }

    /**
     * @brief Test 3: DG flag set
     * Tests data group flag extraction
     */
    void test03_DGFlagSet() {
        // FIG 0/7: SCId=0x789 (1929), SubChId=0x20 (32), CA=0, DG=1
        std::vector<uint8_t> fig_data = {
            0x07,        // Extension 7
            0x78, 0x98,  // SCId = 0x789, DG=1 (bit 3 of byte 2 = 1)
                         // Byte 1: 0x78 = 0111 1000
                         // Byte 2: 0x98 = 1001 1000
                         // SCId = 0111 1000 1001 = 0x789 (1929 decimal)
                         // DG=1 (0x98 has bit 3 set)
            0x20         // SubChId = 0x20 (32)
        };

        auto result = parser->parseFig007_ServiceComponentStreamMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.service_component_id, static_cast<uint16_t>(0x789));
        QCOMPARE(result.sub_ch_id, static_cast<uint8_t>(0x20));
        QVERIFY(!result.ca_flag);
        QVERIFY(result.dg_flag);
        QVERIFY(!result.isCaProtected());
        QVERIFY(result.hasDataGroups());
    }

    /**
     * @brief Test 4: Both CA and DG flags set
     * Tests combined flag extraction
     */
    void test04_BothCAandDGFlagsSet() {
        // FIG 0/7: SCId=0xABC (2748), SubChId=0x2F (47), CA=1, DG=1
        std::vector<uint8_t> fig_data = {
            0x07,        // Extension 7
            0xAB, 0xCC,  // SCId = 0xABC, CA=1, DG=1
                         // Byte 1: 0xAB = 1010 1011
                         // Byte 2: 0xCC = 1100 1100
                         // SCId = 1010 1011 1100 = 0xABC (2748 decimal)
                         // CA=1, DG=1 (0xCC has both bits 2 and 3 set)
            0x2F         // SubChId = 0x2F (47)
        };

        auto result = parser->parseFig007_ServiceComponentStreamMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.service_component_id, static_cast<uint16_t>(0xABC));
        QCOMPARE(result.sub_ch_id, static_cast<uint8_t>(0x2F));
        QVERIFY(result.ca_flag);
        QVERIFY(result.dg_flag);
        QVERIFY(result.isCaProtected());
        QVERIFY(result.hasDataGroups());
    }

    /**
     * @brief Test 5: Minimum SCId value (0)
     * Tests minimum valid Service Component ID
     */
    void test05_MinimumSCId() {
        // FIG 0/7: SCId=0x000 (0), SubChId=0x00 (0)
        std::vector<uint8_t> fig_data = {
            0x07,        // Extension 7
            0x00, 0x00,  // SCId = 0x000 (0)
            0x00         // SubChId = 0x00 (0)
        };

        auto result = parser->parseFig007_ServiceComponentStreamMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.service_component_id, static_cast<uint16_t>(0x000));
        QCOMPARE(result.sub_ch_id, static_cast<uint8_t>(0x00));
        QVERIFY(result.validateServiceComponentId());
        QVERIFY(result.validateSubChannelId());
    }

    /**
     * @brief Test 6: Maximum SCId value (4095)
     * Tests maximum valid Service Component ID (12-bit)
     */
    void test06_MaximumSCId() {
        // FIG 0/7: SCId=0xFFF (4095), SubChId=0x3F (63)
        std::vector<uint8_t> fig_data = {
            0x07,        // Extension 7
            0xFF, 0xF0,  // SCId = 0xFFF (4095 decimal, maximum 12-bit value)
                         // Byte 1: 0xFF = 1111 1111
                         // Byte 2: 0xF0 = 1111 0000
                         // SCId = 1111 1111 1111 = 0xFFF (4095 decimal)
            0x3F         // SubChId = 0x3F (63, maximum 6-bit value)
        };

        auto result = parser->parseFig007_ServiceComponentStreamMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.service_component_id, static_cast<uint16_t>(0xFFF));
        QCOMPARE(result.sub_ch_id, static_cast<uint8_t>(0x3F));
        QVERIFY(result.validateServiceComponentId());
        QVERIFY(result.validateSubChannelId());
    }

    /**
     * @brief Test 7: Multiple service components (first component)
     * Tests first of multiple consecutive FIG 0/7 entries
     */
    void test07_MultipleComponents_First() {
        // FIG 0/7: SCId=0x111, SubChId=0x05
        std::vector<uint8_t> fig_data = {
            0x07,        // Extension 7
            0x11, 0x10,  // SCId = 0x111
            0x05         // SubChId = 0x05
        };

        auto result = parser->parseFig007_ServiceComponentStreamMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.service_component_id, static_cast<uint16_t>(0x111));
        QCOMPARE(result.sub_ch_id, static_cast<uint8_t>(0x05));
    }

    /**
     * @brief Test 8: Multiple service components (second component)
     * Tests second of multiple consecutive FIG 0/7 entries
     */
    void test08_MultipleComponents_Second() {
        // FIG 0/7: SCId=0x222, SubChId=0x0B
        std::vector<uint8_t> fig_data = {
            0x07,        // Extension 7
            0x22, 0x20,  // SCId = 0x222
            0x0B         // SubChId = 0x0B
        };

        auto result = parser->parseFig007_ServiceComponentStreamMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.service_component_id, static_cast<uint16_t>(0x222));
        QCOMPARE(result.sub_ch_id, static_cast<uint8_t>(0x0B));
    }

    /**
     * @brief Test 9: Insufficient data - too short
     * Edge case: data too short for complete FIG 0/7
     */
    void test09_InsufficientDataTooShort() {
        std::vector<uint8_t> fig_data = {
            0x07,        // Extension only
            0x12         // Only 1 byte of SCId (need 2 bytes + 1 byte SubChId)
        };

        auto result = parser->parseFig007_ServiceComponentStreamMode(fig_data);
        QVERIFY(!result.is_valid);
    }

    /**
     * @brief Test 10: Empty data
     * Edge case: completely empty data
     */
    void test10_EmptyData() {
        std::vector<uint8_t> fig_data;

        auto result = parser->parseFig007_ServiceComponentStreamMode(fig_data);
        QVERIFY(!result.is_valid);
    }

    /**
     * @brief Test 11: SubChId validation
     * Tests validateSubChannelId() helper function
     */
    void test11_SubChIdValidation() {
        // Valid SubChId (0-63)
        std::vector<uint8_t> fig_data = {
            0x07,
            0x12, 0x30,
            0x3F         // SubChId = 63 (maximum valid)
        };

        auto result = parser->parseFig007_ServiceComponentStreamMode(fig_data);

        QVERIFY(result.is_valid);
        QVERIFY(result.validateSubChannelId());
        QCOMPARE(result.sub_ch_id, static_cast<uint8_t>(63));
    }

    /**
     * @brief Test 12: SCId validation
     * Tests validateServiceComponentId() helper function
     */
    void test12_SCIdValidation() {
        // Valid SCId (0-4095)
        std::vector<uint8_t> fig_data = {
            0x07,
            0xFE, 0xD0,  // SCId = 0xFED (4077)
            0x1A
        };

        auto result = parser->parseFig007_ServiceComponentStreamMode(fig_data);

        QVERIFY(result.is_valid);
        QVERIFY(result.validateServiceComponentId());
        QCOMPARE(result.service_component_id, static_cast<uint16_t>(0xFED));
    }

    /**
     * @brief Test 13: Performance benchmark
     * Verify parsing performance meets <15µs target
     */
    void test13_PerformanceBenchmark() {
        // The benchmark measures parsing, not per-call logging: keep the
        // logger quiet for the timed section (restored on scope exit).
        struct LogLevelGuard {
            Logger::LogLevel saved = Logger::instance().getLogLevel();
            LogLevelGuard() { Logger::instance().setLogLevel(Logger::LogLevel::Warning); }
            ~LogLevelGuard() { Logger::instance().setLogLevel(saved); }
        } logGuard;

        std::vector<uint8_t> fig_data = {
            0x07,
            0x12, 0x34,  // SCId with CA and DG flags
            0x15
        };

        // Warm up
        for (int i = 0; i < 100; ++i) {
            parser->parseFig007_ServiceComponentStreamMode(fig_data);
        }

        // Benchmark
        const int iterations = 10000;
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            auto result = parser->parseFig007_ServiceComponentStreamMode(fig_data);
            Q_UNUSED(result);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double avg_us = static_cast<double>(duration.count()) / iterations;

        qDebug() << "FIG 0/7 parsing performance:" << avg_us << "µs per parse";
        QVERIFY2(avg_us < 15.0, QString("Performance target missed: %1 µs (target: <15 µs)")
            .arg(avg_us).toUtf8().constData());
    }
};

QTEST_MAIN(TestFig007ServiceComponentStreamMode)
#include "test_fig007_stream_mode.moc"
