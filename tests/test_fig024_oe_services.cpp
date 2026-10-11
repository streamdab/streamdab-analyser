/**
 * @file test_fig024_oe_services.cpp
 * @brief Comprehensive Test Suite for FIG 0/24 Other Ensemble Services
 *
 * ETSI EN 300 401 Section 8.1.18 compliance testing
 * PDCA Week 7 - Final / Agent 33 implementation
 *
 * Test Coverage:
 * - Basic parsing (3 tests)
 * - CA flag detection (2 tests)
 * - Multiple ensembles (2 tests)
 * - Edge cases (3 tests)
 * - Performance (1 test)
 * - Signal emission (1 test)
 *
 * Total: 12+ tests
 *
 * @author Agent 33 - FIG 0/24 Specialist
 * @date October 28, 2025
 */

#include <QTest>
#include <QSignalSpy>
#include "../src/core/fig_parser.hpp"
#include "../src/utils/logger.h"
#include <chrono>

using namespace eti::fig;

/**
 * @brief FIG 0/24 Test Class
 */
class TestFig024OEServices : public QObject {
    Q_OBJECT

private:
    FigParser* parser;

    /**
     * @brief Helper: Create FIG 0/24 test data
     */
    std::vector<uint8_t> createFig024Data(
        uint16_t ensemble_id,
        uint32_t service_id,
        bool ca_flag) {

        std::vector<uint8_t> data;

        // Byte 0: Extension (0x18 = 24)
        data.push_back(0x18);

        // Bytes 1-2: Ensemble ID (16-bit big-endian)
        data.push_back((ensemble_id >> 8) & 0xFF);
        data.push_back(ensemble_id & 0xFF);

        // Bytes 3-6: Service ID (32-bit big-endian)
        data.push_back((service_id >> 24) & 0xFF);
        data.push_back((service_id >> 16) & 0xFF);
        data.push_back((service_id >> 8) & 0xFF);
        data.push_back(service_id & 0xFF);

        // Byte 7: [CA flag bit 7][RFA bits 6-0]
        uint8_t ca_byte = ca_flag ? 0x80 : 0x00;
        data.push_back(ca_byte);

        return data;
    }

private slots:
    void initTestCase() {
        // Initialize logger
        Logger::instance().setLogLevel(Logger::Debug);

        // Create parser
        parser = new FigParser();
        QVERIFY(parser != nullptr);

        // Initialize with Thai support enabled
        bool init_result = parser->initialize(true, false);
        QVERIFY(init_result);
    }

    void cleanupTestCase() {
        delete parser;
        parser = nullptr;
    }

    // ========================================================================
    // CATEGORY 1: Basic Parsing Tests (3 tests)
    // ========================================================================

    /**
     * @test FIG024_BasicParsing_ValidData
     * @brief Test basic parsing of valid FIG 0/24 data
     */
    void test_BasicParsing_ValidData() {
        // Create valid FIG 0/24 data
        auto data = createFig024Data(0x4001, 0xE1C00521, false);

        // Parse
        auto result = parser->parseFig024_OEServices(data);

        // Verify
        QVERIFY(result.is_valid);
        QCOMPARE(result.ensemble_id, static_cast<uint16_t>(0x4001));
        QCOMPARE(result.service_id, static_cast<uint32_t>(0xE1C00521));
        QVERIFY(!result.ca_flag);
    }

    /**
     * @test FIG024_BasicParsing_DifferentEnsemble
     * @brief Test parsing with different ensemble ID
     */
    void test_BasicParsing_DifferentEnsemble() {
        // Create FIG 0/24 with different ensemble ID
        auto data = createFig024Data(0xC1A5, 0xD3B20001, false);

        // Parse
        auto result = parser->parseFig024_OEServices(data);

        // Verify
        QVERIFY(result.is_valid);
        QCOMPARE(result.ensemble_id, static_cast<uint16_t>(0xC1A5));
        QCOMPARE(result.service_id, static_cast<uint32_t>(0xD3B20001));
        QVERIFY(!result.ca_flag);
    }

    /**
     * @test FIG024_BasicParsing_HelperMethods
     * @brief Test helper methods in OEServices struct
     */
    void test_BasicParsing_HelperMethods() {
        // Create FIG 0/24 data
        auto data = createFig024Data(0x4001, 0xE1C00521, true);

        // Parse
        auto result = parser->parseFig024_OEServices(data);

        // Verify helper methods
        QVERIFY(result.isProtected());
        QCOMPARE(result.getEnsembleId(), static_cast<uint16_t>(0x4001));
        QVERIFY(result.validateEnsembleId());
        QVERIFY(result.validateServiceId());
    }

    // ========================================================================
    // CATEGORY 2: CA Flag Detection Tests (2 tests)
    // ========================================================================

    /**
     * @test FIG024_CAFlag_Protected
     * @brief Test CA flag detection for protected service
     */
    void test_CAFlag_Protected() {
        // Create FIG 0/24 with CA flag set
        auto data = createFig024Data(0x4001, 0xE1C00521, true);

        // Parse
        auto result = parser->parseFig024_OEServices(data);

        // Verify CA flag
        QVERIFY(result.is_valid);
        QVERIFY(result.ca_flag);
        QVERIFY(result.isProtected());
    }

    /**
     * @test FIG024_CAFlag_NotProtected
     * @brief Test CA flag detection for non-protected service
     */
    void test_CAFlag_NotProtected() {
        // Create FIG 0/24 without CA flag
        auto data = createFig024Data(0x4001, 0xE1C00521, false);

        // Parse
        auto result = parser->parseFig024_OEServices(data);

        // Verify CA flag
        QVERIFY(result.is_valid);
        QVERIFY(!result.ca_flag);
        QVERIFY(!result.isProtected());
    }

    // ========================================================================
    // CATEGORY 3: Multiple Ensembles Tests (2 tests)
    // ========================================================================

    /**
     * @test FIG024_MultipleEnsembles_TwoEnsembles
     * @brief Test parsing services from two different ensembles
     */
    void test_MultipleEnsembles_TwoEnsembles() {
        // Create FIG 0/24 for ensemble 1
        auto data1 = createFig024Data(0x4001, 0xE1C00521, false);
        auto result1 = parser->parseFig024_OEServices(data1);

        // Create FIG 0/24 for ensemble 2
        auto data2 = createFig024Data(0x4002, 0xE1C00522, true);
        auto result2 = parser->parseFig024_OEServices(data2);

        // Verify both parsed successfully
        QVERIFY(result1.is_valid);
        QVERIFY(result2.is_valid);

        // Verify they're different
        QVERIFY(result1.ensemble_id != result2.ensemble_id);
        QVERIFY(result1.service_id != result2.service_id);
        QVERIFY(result1.ca_flag != result2.ca_flag);
    }

    /**
     * @test FIG024_MultipleEnsembles_SameServiceDifferentEnsemble
     * @brief Test same service ID in different ensembles
     */
    void test_MultipleEnsembles_SameServiceDifferentEnsemble() {
        // Same service ID but different ensemble IDs
        auto data1 = createFig024Data(0x4001, 0xE1C00521, false);
        auto result1 = parser->parseFig024_OEServices(data1);

        auto data2 = createFig024Data(0x4002, 0xE1C00521, false);
        auto result2 = parser->parseFig024_OEServices(data2);

        // Verify both valid
        QVERIFY(result1.is_valid);
        QVERIFY(result2.is_valid);

        // Same service, different ensemble
        QVERIFY(result1.ensemble_id != result2.ensemble_id);
        QCOMPARE(result1.service_id, result2.service_id);
    }

    // ========================================================================
    // CATEGORY 4: Edge Cases Tests (3 tests)
    // ========================================================================

    /**
     * @test FIG024_EdgeCases_InsufficientData
     * @brief Test handling of insufficient data
     */
    void test_EdgeCases_InsufficientData() {
        // Create data with only 7 bytes (need 8)
        std::vector<uint8_t> data = {0x18, 0x40, 0x01, 0xE1, 0xC0, 0x05, 0x21};

        // Parse
        auto result = parser->parseFig024_OEServices(data);

        // Verify invalid
        QVERIFY(!result.is_valid);
    }

    /**
     * @test FIG024_EdgeCases_ZeroEnsembleId
     * @brief Test rejection of zero ensemble ID
     */
    void test_EdgeCases_ZeroEnsembleId() {
        // Create FIG 0/24 with ensemble ID = 0 (invalid)
        auto data = createFig024Data(0x0000, 0xE1C00521, false);

        // Parse
        auto result = parser->parseFig024_OEServices(data);

        // Verify rejected
        QVERIFY(!result.is_valid);
    }

    /**
     * @test FIG024_EdgeCases_ZeroServiceId
     * @brief Test rejection of zero service ID
     */
    void test_EdgeCases_ZeroServiceId() {
        // Create FIG 0/24 with service ID = 0 (invalid)
        auto data = createFig024Data(0x4001, 0x00000000, false);

        // Parse
        auto result = parser->parseFig024_OEServices(data);

        // Verify rejected
        QVERIFY(!result.is_valid);
    }

    // ========================================================================
    // CATEGORY 5: Performance Test (1 test)
    // ========================================================================

    /**
     * @test FIG024_Performance_1000Iterations
     * @brief Test parsing performance (target: <15µs per parse)
     */
    void test_Performance_1000Iterations() {
        // The benchmark measures parsing, not per-call debug logging: keep the
        // logger quiet for the timed section (restored on scope exit).
        struct LogLevelGuard {
            Logger::LogLevel saved = Logger::instance().getLogLevel();
            LogLevelGuard() { Logger::instance().setLogLevel(Logger::LogLevel::Warning); }
            ~LogLevelGuard() { Logger::instance().setLogLevel(saved); }
        } logGuard;

        // Create test data
        auto data = createFig024Data(0x4001, 0xE1C00521, false);

        // Warm-up
        for (int i = 0; i < 100; ++i) {
            parser->parseFig024_OEServices(data);
        }

        // Benchmark
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < 1000; ++i) {
            auto result = parser->parseFig024_OEServices(data);
            QVERIFY(result.is_valid);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        double avg_time_us = duration.count() / 1000.0;

        // Log performance
        qDebug() << "FIG 0/24 parsing performance:" << avg_time_us << "µs/parse";

        // Target: <15µs per parse
        QVERIFY(avg_time_us < 15.0);
    }

    // ========================================================================
    // CATEGORY 6: Signal Emission Test (1 test)
    // ========================================================================

    /**
     * @test FIG024_SignalEmission_ValidData
     * @brief Test that oeServicesDiscovered signal is emitted
     */
    void test_SignalEmission_ValidData() {
        // Create signal spy
        QSignalSpy spy(parser, &FigParser::oeServicesDiscovered);

        // Create and parse valid data
        auto data = createFig024Data(0x4001, 0xE1C00521, true);
        auto result = parser->parseFig024_OEServices(data);

        // Verify signal emitted
        QVERIFY(result.is_valid);
        QCOMPARE(spy.count(), 1);

        // Verify signal parameters
        auto signal_args = spy.takeFirst();
        auto emitted_result = signal_args.at(0).value<OEServices>();

        QCOMPARE(emitted_result.ensemble_id, static_cast<uint16_t>(0x4001));
        QCOMPARE(emitted_result.service_id, static_cast<uint32_t>(0xE1C00521));
        QVERIFY(emitted_result.ca_flag);
    }
};

// Register metatype for signal testing
Q_DECLARE_METATYPE(eti::fig::OEServices)

// Qt Test main macro
QTEST_MAIN(TestFig024OEServices)
#include "test_fig024_oe_services.moc"
