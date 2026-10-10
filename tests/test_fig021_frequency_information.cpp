/**
 * @file test_fig021_frequency_information.cpp
 * @brief Comprehensive Test Suite for FIG 0/21 Frequency Information Parser
 *
 * Tests ETSI EN 300 401 Section 8.1.16 compliance for frequency information.
 * Part of PDCA Week 7 - Batch 3 / Agent 25 implementation.
 *
 * @author Python Pro Agent
 * @date November 3, 2025
 */

#include <QTest>
#include <QSignalSpy>
#include "../src/core/fig_parser.hpp"
#include "../src/utils/logger.h"

using namespace eti::fig;

class TestFig021FrequencyInformation : public QObject {
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
     * @brief Test 1: Basic FIG 0/21 parsing - single frequency
     * Validates basic structure with one frequency
     */
    void test01_BasicParsingSingleFrequency() {
        // FIG 0/21: LI=1, Frequency code=0x0C80 (3200 * 16kHz + 174 MHz = 225.2 MHz)
        std::vector<uint8_t> fig_data = {
            0x15,        // Extension 21
            0x01,        // Length Indicator = 1 frequency
            0x0C, 0x80   // Frequency code = 0x0C80 (3200)
        };

        auto result = parser->parseFig021_FrequencyInformation(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.length_indicator, static_cast<uint8_t>(1));
        QCOMPARE(result.getFrequencyCount(), size_t(1));
        // Expected: 174000000 + (3200 * 16000) = 225200000 Hz = 225.2 MHz
        QCOMPARE(result.getFrequencyAt(0), uint32_t(225200000));
        QCOMPARE(result.getFrequencyMHz(0), 225.2);
    }

    /**
     * @brief Test 2: Multiple frequencies
     * Tests parsing of multiple frequency entries
     */
    void test02_MultipleFrequencies() {
        // FIG 0/21: LI=3, Three frequencies
        std::vector<uint8_t> fig_data = {
            0x15,        // Extension 21
            0x03,        // Length Indicator = 3 frequencies
            0x05, 0xDC,  // Freq 1: 0x05DC (1500 * 16kHz + 174 MHz = 198.0 MHz)
            0x0C, 0x80,  // Freq 2: 0x0C80 (3200 * 16kHz + 174 MHz = 225.2 MHz)
            0x10, 0x68   // Freq 3: 0x1068 (4200 * 16kHz + 174 MHz = 241.2 MHz - out of range!)
        };

        auto result = parser->parseFig021_FrequencyInformation(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.length_indicator, static_cast<uint8_t>(3));
        // Note: Freq 3 may be filtered out if out of Band III range (174-240 MHz)
        // So we expect 2 valid frequencies
        QCOMPARE(result.getFrequencyCount(), size_t(2));
        QCOMPARE(result.getFrequencyAt(0), uint32_t(198000000)); // 198.0 MHz
        QCOMPARE(result.getFrequencyAt(1), uint32_t(225200000)); // 225.2 MHz
    }

    /**
     * @brief Test 3: Minimum frequency (174 MHz)
     * Tests lower boundary of Band III
     */
    void test03_MinimumFrequency() {
        // FIG 0/21: Frequency code 0x0000 = 174 MHz
        std::vector<uint8_t> fig_data = {
            0x15,        // Extension 21
            0x01,        // LI = 1
            0x00, 0x00   // Frequency code = 0 (174 MHz)
        };

        auto result = parser->parseFig021_FrequencyInformation(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.getFrequencyAt(0), uint32_t(174000000)); // 174.0 MHz
        QVERIFY(FrequencyInformation::isValidBandIIIFrequency(174000000));
    }

    /**
     * @brief Test 4: Maximum frequency (near 240 MHz)
     * Tests upper boundary of Band III
     */
    void test04_MaximumFrequency() {
        // FIG 0/21: Frequency code for 239.2 MHz
        // (239.2 - 174) / 0.016 = 4075 = 0x0FEB
        std::vector<uint8_t> fig_data = {
            0x15,        // Extension 21
            0x01,        // LI = 1
            0x0F, 0xEB   // Frequency code = 0x0FEB (4075 * 16kHz + 174 MHz = 239.2 MHz)
        };

        auto result = parser->parseFig021_FrequencyInformation(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.getFrequencyAt(0), uint32_t(239200000)); // 239.2 MHz
        QVERIFY(FrequencyInformation::isValidBandIIIFrequency(239200000));
    }

    /**
     * @brief Test 5: Typical DAB frequencies
     * Tests common DAB channel frequencies (11B, 11D, 12A, 12C)
     */
    void test05_TypicalDABFrequencies() {
        // Common DAB channels:
        // 11B: 218.640 MHz = (218.640 - 174) / 0.016 = 2790 = 0x0AE6
        // 12A: 223.936 MHz = (223.936 - 174) / 0.016 = 3121 = 0x0C31
        std::vector<uint8_t> fig_data = {
            0x15,        // Extension 21
            0x02,        // LI = 2
            0x0A, 0xE6,  // 11B: 218.640 MHz
            0x0C, 0x31   // 12A: 223.936 MHz
        };

        auto result = parser->parseFig021_FrequencyInformation(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.getFrequencyCount(), size_t(2));
        QCOMPARE(result.getFrequencyAt(0), uint32_t(218640000)); // 218.64 MHz
        QCOMPARE(result.getFrequencyAt(1), uint32_t(223936000)); // 223.936 MHz
    }

    /**
     * @brief Test 6: Frequency conversion to MHz
     * Tests getFrequencyMHz() helper method
     */
    void test06_FrequencyConversionToMHz() {
        std::vector<uint8_t> fig_data = {
            0x15,
            0x01,
            0x0C, 0x80   // 225.2 MHz
        };

        auto result = parser->parseFig021_FrequencyInformation(fig_data);

        QVERIFY(result.is_valid);
        double freq_mhz = result.getFrequencyMHz(0);
        QCOMPARE(freq_mhz, 225.2);
    }

    /**
     * @brief Test 7: Frequency validation - Band III range
     * Tests isValidBandIIIFrequency() static method
     */
    void test07_FrequencyValidation() {
        QVERIFY(FrequencyInformation::isValidBandIIIFrequency(174000000));  // 174 MHz (min)
        QVERIFY(FrequencyInformation::isValidBandIIIFrequency(225200000));  // 225.2 MHz (mid)
        QVERIFY(FrequencyInformation::isValidBandIIIFrequency(240000000));  // 240 MHz (max)

        // Out of range
        QVERIFY(!FrequencyInformation::isValidBandIIIFrequency(173000000)); // Below 174 MHz
        QVERIFY(!FrequencyInformation::isValidBandIIIFrequency(241000000)); // Above 240 MHz
    }

    /**
     * @brief Test 8: Helper method - getFrequencyCount()
     * Tests frequency count retrieval
     */
    void test08_GetFrequencyCount() {
        std::vector<uint8_t> fig_data = {
            0x15,
            0x03,
            0x05, 0xDC,
            0x0C, 0x80,
            0x0F, 0xEB
        };

        auto result = parser->parseFig021_FrequencyInformation(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.getFrequencyCount(), size_t(3));
    }

    /**
     * @brief Test 9: Helper method - getFrequencyAt() out of range
     * Tests boundary checking in getFrequencyAt()
     */
    void test09_GetFrequencyAtOutOfRange() {
        std::vector<uint8_t> fig_data = {
            0x15,
            0x01,
            0x0C, 0x80
        };

        auto result = parser->parseFig021_FrequencyInformation(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.getFrequencyAt(0), uint32_t(225200000)); // Valid
        QCOMPARE(result.getFrequencyAt(1), uint32_t(0));         // Out of range -> 0
        QCOMPARE(result.getFrequencyAt(100), uint32_t(0));       // Out of range -> 0
    }

    /**
     * @brief Test 10: Insufficient data - too short
     * Edge case: data too short for complete FIG 0/21
     */
    void test10_InsufficientDataTooShort() {
        std::vector<uint8_t> fig_data = {
            0x15,        // Extension only
            0x01         // LI but no frequency data
        };

        auto result = parser->parseFig021_FrequencyInformation(fig_data);
        QVERIFY(!result.is_valid);
    }

    /**
     * @brief Test 11: Empty data
     * Edge case: completely empty data
     */
    void test11_EmptyData() {
        std::vector<uint8_t> fig_data;

        auto result = parser->parseFig021_FrequencyInformation(fig_data);
        QVERIFY(!result.is_valid);
    }

    /**
     * @brief Test 12: Zero length indicator
     * Edge case: LI = 0 (no frequencies)
     */
    void test12_ZeroLengthIndicator() {
        std::vector<uint8_t> fig_data = {
            0x15,        // Extension 21
            0x00         // LI = 0 (no frequencies)
        };

        auto result = parser->parseFig021_FrequencyInformation(fig_data);
        QVERIFY(!result.is_valid); // Should be invalid as no frequencies
    }

    /**
     * @brief Test 13: Signal emission
     * Tests that the signal is emitted correctly
     */
    void test13_SignalEmission() {
        QSignalSpy spy(parser, &FigParser::frequencyInformationDiscovered);
        QVERIFY(spy.isValid());

        std::vector<uint8_t> fig_data = {
            0x15,
            0x01,
            0x0C, 0x80
        };

        auto result = parser->parseFig021_FrequencyInformation(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(spy.count(), 1);

        // Verify signal argument
        auto arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
    }

    /**
     * @brief Test 14: Performance benchmark
     * Verify parsing performance meets <15µs target
     */
    void test14_PerformanceBenchmark() {
        // The benchmark measures parsing, not per-call logging: keep the
        // logger quiet for the timed section (restored on scope exit).
        struct LogLevelGuard {
            Logger::LogLevel saved = Logger::instance().getLogLevel();
            LogLevelGuard() { Logger::instance().setLogLevel(Logger::LogLevel::Warning); }
            ~LogLevelGuard() { Logger::instance().setLogLevel(saved); }
        } logGuard;

        std::vector<uint8_t> fig_data = {
            0x15,
            0x03,
            0x05, 0xDC,
            0x0C, 0x80,
            0x0F, 0xEB
        };

        // Warm up
        for (int i = 0; i < 100; ++i) {
            parser->parseFig021_FrequencyInformation(fig_data);
        }

        // Benchmark
        const int iterations = 10000;
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            auto result = parser->parseFig021_FrequencyInformation(fig_data);
            Q_UNUSED(result);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double avg_us = static_cast<double>(duration.count()) / iterations;

        qDebug() << "FIG 0/21 parsing performance:" << avg_us << "µs per parse";
        QVERIFY2(avg_us < 15.0, QString("Performance target missed: %1 µs (target: <15 µs)")
            .arg(avg_us).toUtf8().constData());
    }
};

QTEST_MAIN(TestFig021FrequencyInformation)
#include "test_fig021_frequency_information.moc"
