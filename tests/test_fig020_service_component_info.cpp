/**
 * @file test_fig020_service_component_info.cpp
 * @brief Comprehensive Test Suite for FIG 0/20 Service Component Information Parser
 *
 * Tests ETSI EN 300 401 Section 8.1.15 compliance for service component information.
 * Part of PDCA Week 7 - Batch 4 / Agent 30 implementation.
 *
 * @author Python Pro Agent
 * @date November 4, 2025
 */

#include <QTest>
#include <QSignalSpy>
#include "../src/core/fig_parser.hpp"
#include "../src/utils/logger.h"
#include <chrono>

using namespace eti::fig;

class TestFig020ServiceComponentInfo : public QObject {
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
     * @brief Test 1: Basic FIG 0/20 parsing
     * Validates service ID, SC ID, CA flag extraction
     */
    void test01_BasicParsing() {
        // FIG 0/20: SId=0x1234, SCId=0x123 (291), CA=false, NumComp=0
        std::vector<uint8_t> fig_data = {
            0x14,        // Extension 20
            0x12, 0x34,  // Service ID = 0x1234 (4660)
            0x12, 0x30,  // SCId=0x123 (bits 15-4), CA=0 (bit 3), RFA=0
            0x00         // Number of components = 0
        };

        auto result = parser->parseFig020_ServiceComponentInfo(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.service_id, static_cast<uint32_t>(0x1234));
        QCOMPARE(result.sc_id, static_cast<uint16_t>(0x123));
        QVERIFY(!result.ca_flag);
        QCOMPARE(result.num_components, static_cast<uint8_t>(0));
        QCOMPARE(result.component_list.size(), static_cast<size_t>(0));
    }

    /**
     * @brief Test 2: CA flag set
     * Tests conditional access flag extraction
     */
    void test02_CAFlagSet() {
        // FIG 0/20: SId=0xABCD, SCId=0xFFF (4095 max), CA=true, NumComp=0
        std::vector<uint8_t> fig_data = {
            0x14,        // Extension 20
            0xAB, 0xCD,  // Service ID = 0xABCD (43981)
            0xFF, 0xF8,  // SCId=0xFFF (max 12-bit), CA=1 (bit 3 set)
            0x00         // Number of components = 0
        };

        auto result = parser->parseFig020_ServiceComponentInfo(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.service_id, static_cast<uint32_t>(0xABCD));
        QCOMPARE(result.sc_id, static_cast<uint16_t>(0xFFF));  // Max 12-bit value
        QVERIFY(result.ca_flag);  // CA flag should be set
        QCOMPARE(result.num_components, static_cast<uint8_t>(0));
    }

    /**
     * @brief Test 3: Component list extraction (3 components)
     * Tests variable-length component list parsing
     */
    void test03_ComponentListExtraction() {
        // FIG 0/20: SId=0x5678, SCId=0x456, CA=false, NumComp=3, CompList=[0x01,0x02,0x03]
        std::vector<uint8_t> fig_data = {
            0x14,        // Extension 20
            0x56, 0x78,  // Service ID = 0x5678
            0x45, 0x60,  // SCId=0x456, CA=0
            0x03,        // Number of components = 3
            0x01, 0x02, 0x03  // Component list
        };

        auto result = parser->parseFig020_ServiceComponentInfo(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.service_id, static_cast<uint32_t>(0x5678));
        QCOMPARE(result.sc_id, static_cast<uint16_t>(0x456));
        QVERIFY(!result.ca_flag);
        QCOMPARE(result.num_components, static_cast<uint8_t>(3));
        QCOMPARE(result.component_list.size(), static_cast<size_t>(3));
        QCOMPARE(result.component_list[0], static_cast<uint8_t>(0x01));
        QCOMPARE(result.component_list[1], static_cast<uint8_t>(0x02));
        QCOMPARE(result.component_list[2], static_cast<uint8_t>(0x03));
    }

    /**
     * @brief Test 4: Maximum component list
     * Tests handling of larger component lists
     */
    void test04_MaximumComponentList() {
        // FIG 0/20: SId=0x9999, SCId=0x789, CA=true, NumComp=5
        std::vector<uint8_t> fig_data = {
            0x14,        // Extension 20
            0x99, 0x99,  // Service ID = 0x9999
            0x78, 0x98,  // SCId=0x789, CA=1
            0x05,        // Number of components = 5
            0xAA, 0xBB, 0xCC, 0xDD, 0xEE  // Component list (5 components)
        };

        auto result = parser->parseFig020_ServiceComponentInfo(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.service_id, static_cast<uint32_t>(0x9999));
        QCOMPARE(result.sc_id, static_cast<uint16_t>(0x789));
        QVERIFY(result.ca_flag);
        QCOMPARE(result.num_components, static_cast<uint8_t>(5));
        QCOMPARE(result.component_list.size(), static_cast<size_t>(5));
        QCOMPARE(result.component_list[0], static_cast<uint8_t>(0xAA));
        QCOMPARE(result.component_list[4], static_cast<uint8_t>(0xEE));
    }

    /**
     * @brief Test 5: Helper method - getComponentCount()
     * Tests component count helper
     */
    void test05_GetComponentCount() {
        std::vector<uint8_t> fig_data = {
            0x14,
            0x11, 0x11,
            0x11, 0x10,
            0x02,
            0xAA, 0xBB
        };

        auto result = parser->parseFig020_ServiceComponentInfo(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.getComponentCount(), static_cast<size_t>(2));
        QCOMPARE(result.getComponentCount(), result.component_list.size());
    }

    /**
     * @brief Test 6: Helper method - isProtected()
     * Tests CA protection helper
     */
    void test06_IsProtected() {
        // Test CA=false
        std::vector<uint8_t> fig_data_no_ca = {
            0x14, 0x22, 0x22, 0x22, 0x20, 0x00
        };
        auto result1 = parser->parseFig020_ServiceComponentInfo(fig_data_no_ca);
        QVERIFY(result1.is_valid);
        QVERIFY(!result1.isProtected());

        // Test CA=true
        std::vector<uint8_t> fig_data_with_ca = {
            0x14, 0x33, 0x33, 0x33, 0x38, 0x00  // CA flag bit 3 set
        };
        auto result2 = parser->parseFig020_ServiceComponentInfo(fig_data_with_ca);
        QVERIFY(result2.is_valid);
        QVERIFY(result2.isProtected());
    }

    /**
     * @brief Test 7: Validation - validateSCId()
     * Tests SC ID validation (must be < 4096)
     */
    void test07_ValidateSCId() {
        // Valid SC ID (4095, max 12-bit)
        std::vector<uint8_t> fig_data = {
            0x14, 0x44, 0x44, 0xFF, 0xF0, 0x00
        };
        auto result = parser->parseFig020_ServiceComponentInfo(fig_data);
        QVERIFY(result.is_valid);
        QCOMPARE(result.sc_id, static_cast<uint16_t>(0xFFF));
        QVERIFY(result.validateSCId());
    }

    /**
     * @brief Test 8: Validation - validateServiceId()
     * Tests service ID validation (must be non-zero)
     */
    void test08_ValidateServiceId() {
        // Valid service ID
        std::vector<uint8_t> fig_data = {
            0x14, 0x00, 0x01, 0x00, 0x10, 0x00
        };
        auto result = parser->parseFig020_ServiceComponentInfo(fig_data);
        QVERIFY(result.is_valid);
        QCOMPARE(result.service_id, static_cast<uint32_t>(0x0001));
        QVERIFY(result.validateServiceId());
    }

    /**
     * @brief Test 9: Edge case - insufficient data
     * Tests handling of truncated FIG data
     */
    void test09_InsufficientData() {
        // Only 5 bytes (need 6 minimum)
        std::vector<uint8_t> fig_data = {
            0x14, 0x55, 0x55, 0x55, 0x50
        };

        auto result = parser->parseFig020_ServiceComponentInfo(fig_data);
        QVERIFY(!result.is_valid);  // Should fail validation
    }

    /**
     * @brief Test 10: Edge case - num_components exceeds data
     * Tests handling of invalid num_components field
     */
    void test10_ExcessiveComponentCount() {
        // NumComp=10 but only 2 bytes of component data
        std::vector<uint8_t> fig_data = {
            0x14, 0x66, 0x66, 0x66, 0x60,
            0x0A,  // num_components = 10
            0xAA, 0xBB  // Only 2 components provided
        };

        auto result = parser->parseFig020_ServiceComponentInfo(fig_data);
        QVERIFY(!result.is_valid);  // Should fail due to insufficient data
    }

    /**
     * @brief Test 11: Edge case - empty component list
     * Tests handling of zero components (valid case)
     */
    void test11_EmptyComponentList() {
        std::vector<uint8_t> fig_data = {
            0x14, 0x77, 0x77, 0x77, 0x70, 0x00
        };

        auto result = parser->parseFig020_ServiceComponentInfo(fig_data);
        QVERIFY(result.is_valid);
        QCOMPARE(result.num_components, static_cast<uint8_t>(0));
        QCOMPARE(result.component_list.size(), static_cast<size_t>(0));
        QCOMPARE(result.getComponentCount(), static_cast<size_t>(0));
    }

    /**
     * @brief Test 12: Performance benchmark
     * Validates parsing performance <15µs target
     */
    void test12_PerformanceBenchmark() {
        std::vector<uint8_t> fig_data = {
            0x14, 0x88, 0x88, 0x88, 0x80,
            0x04,
            0x11, 0x22, 0x33, 0x44
        };

        constexpr int iterations = 1000;
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            auto result = parser->parseFig020_ServiceComponentInfo(fig_data);
            Q_UNUSED(result);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        double avg_time_us = static_cast<double>(duration.count()) / iterations / 1000.0;

        qDebug() << "FIG 0/20 average parsing time:" << avg_time_us << "µs per parse";

        // Verify performance target <15µs
        QVERIFY2(avg_time_us < 15.0,
                 QString("Performance target missed: %1µs > 15µs").arg(avg_time_us).toUtf8());
    }
};

QTEST_GUILESS_MAIN(TestFig020ServiceComponentInfo)
#include "test_fig020_service_component_info.moc"
