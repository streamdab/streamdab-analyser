/**
 * @file test_fig006_service_linking.cpp
 * @brief Unit tests for FIG 0/6 Service Linking parser
 *
 * Reference: ETSI EN 300 401 Section 8.1.15
 * PDCA Week 4 - Agent 12 Test Suite
 */

#include <QtTest/QtTest>
#include "../src/core/fig_parser.hpp"
#include "../src/utils/logger.h"

using namespace eti::fig;

class TestFig006ServiceLinking : public QObject {
    Q_OBJECT

private:
    FigParser* parser;

private slots:
    void initTestCase() {
        parser = new FigParser();
    }

    void cleanupTestCase() {
        delete parser;
    }

    // Test 1: Valid national soft link, 16-bit SId, single linked service
    void test_national_soft_link_16bit() {
        qDebug() << "TEST: National soft link with 16-bit SId";

        std::vector<uint8_t> fig_data = {
            0x06,        // FIG type 0/6
            0x80,        // IdLQ=1 (single), P/D=0 (16-bit)
            0x12, 0x34,  // SId = 0x1234
            0x01, 0x23,  // LA=0, SH=0, IIS=0, LSN=0x123
            0x56, 0x78   // Linked SId = 0x5678
        };

        auto result = parser->parseFig006_ServiceLinking(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.service_id, 0x1234u);
        QCOMPARE(result.id_list_flag, true);
        QCOMPARE(result.la_flag, false);
        QCOMPARE(result.sh_flag, false);  // Soft link
        QCOMPARE(result.iis_flag, false); // National
        QCOMPARE(result.lsn, 0x123u);
        QCOMPARE(result.linked_service_ids.size(), 1);
        QCOMPARE(result.linked_service_ids[0], 0x5678u);
        QVERIFY(result.getLinkageType().find("Soft") != std::string::npos);
        QVERIFY(result.getLinkageType().find("National") != std::string::npos);
    }

    // Test 2: Hard link
    void test_hard_link() {
        qDebug() << "TEST: Hard link (required alternative)";

        std::vector<uint8_t> fig_data = {
            0x06, 0x80, 0xAB, 0xCD,
            0x41, 0x00,  // SH=1 (hard link), LSN=0x100
            0x11, 0x22
        };

        auto result = parser->parseFig006_ServiceLinking(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.sh_flag, true);  // Hard link
        QVERIFY(result.getLinkageType().find("Hard") != std::string::npos);
    }

    // Test 3: International linking with ECC
    void test_international_with_ecc() {
        qDebug() << "TEST: International linking with ECC";

        std::vector<uint8_t> fig_data = {
            0x06, 0x80, 0x22, 0x33,
            0x21, 0x50,  // IIS=1 (international), LSN=0x150
            0x44, 0x55,  // Linked SId
            0xE0         // ECC (Extended Country Code)
        };

        auto result = parser->parseFig006_ServiceLinking(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.iis_flag, true);  // International
        QCOMPARE(result.linked_service_ids.size(), 1);
        QCOMPARE(result.ecc_ids.size(), 1);
        QCOMPARE(result.ecc_ids[0], 0xE0u);
        QVERIFY(result.getLinkageType().find("International") != std::string::npos);
    }

    // Test 4: LA flag (Linkage Actuator) set
    void test_linkage_actuator() {
        qDebug() << "TEST: Linkage Actuator flag set";

        std::vector<uint8_t> fig_data = {
            0x06, 0x80, 0x33, 0x44,
            0x82, 0x00,  // LA=1, SH=0, IIS=0, LSN=0x200
            0x55, 0x66
        };

        auto result = parser->parseFig006_ServiceLinking(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.la_flag, true);
        QVERIFY(result.getLinkageType().find("Actuator Active") != std::string::npos);
    }

    // Test 5: 32-bit SId
    void test_32bit_sid() {
        qDebug() << "TEST: 32-bit Service ID";

        std::vector<uint8_t> fig_data = {
            0x06,
            0xC0,              // IdLQ=1, P/D=1 (32-bit)
            0x12, 0x34, 0x56, 0x78,  // SId = 0x12345678
            0x03, 0xFF,        // LSN=0x3FF
            0xAB, 0xCD, 0xEF, 0x01   // Linked SId (32-bit)
        };

        auto result = parser->parseFig006_ServiceLinking(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.service_id, 0x12345678u);
        QCOMPARE(result.linked_service_ids[0], 0xABCDEF01u);
    }

    // Test 6: Multiple linked services (list mode)
    void test_multiple_linked_services() {
        qDebug() << "TEST: Multiple linked services";

        std::vector<uint8_t> fig_data = {
            0x06, 0x00,  // IdLQ=0 (list), P/D=0
            0x11, 0x11,  // SId
            0x01, 0x00,  // LSN=0x100
            0x22, 0x22,  // Linked SId 1
            0x33, 0x33,  // Linked SId 2
            0x44, 0x44   // Linked SId 3
        };

        auto result = parser->parseFig006_ServiceLinking(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.id_list_flag, false);  // List mode
        QCOMPARE(result.linked_service_ids.size(), 3);
        QCOMPARE(result.linked_service_ids[0], 0x2222u);
        QCOMPARE(result.linked_service_ids[1], 0x3333u);
        QCOMPARE(result.linked_service_ids[2], 0x4444u);
    }

    // Test 7: LSN maximum value (12-bit: 0-4095)
    void test_lsn_maximum() {
        qDebug() << "TEST: LSN at maximum (4095)";

        std::vector<uint8_t> fig_data = {
            0x06, 0x80, 0x55, 0x55,
            0x0F, 0xFF,  // LSN=0xFFF (4095, maximum)
            0x66, 0x66
        };

        auto result = parser->parseFig006_ServiceLinking(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.lsn, 0xFFFu);  // 4095
    }

    // Test 8: All flags set (LA + SH + IIS)
    void test_all_flags_set() {
        qDebug() << "TEST: All flags set (LA + SH + IIS)";

        std::vector<uint8_t> fig_data = {
            0x06, 0x80, 0x77, 0x77,
            0xE1, 0x23,  // LA=1, SH=1, IIS=1, LSN=0x123
            0x88, 0x88,  // Linked SId
            0xE1         // ECC
        };

        auto result = parser->parseFig006_ServiceLinking(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.la_flag, true);
        QCOMPARE(result.sh_flag, true);
        QCOMPARE(result.iis_flag, true);
        std::string linkage_type = result.getLinkageType();
        QVERIFY(linkage_type.find("Hard") != std::string::npos);
        QVERIFY(linkage_type.find("International") != std::string::npos);
        QVERIFY(linkage_type.find("Actuator Active") != std::string::npos);
    }

    // Test 9: International with multiple linked services and ECCs
    void test_international_multiple_ecc() {
        qDebug() << "TEST: International with multiple services and ECCs";

        std::vector<uint8_t> fig_data = {
            0x06, 0x00,  // List mode, 16-bit
            0x99, 0x99,  // SId
            0x22, 0x34,  // IIS=1, LSN=0x234
            0xAA, 0xAA,  // Linked SId 1
            0xE1,        // ECC 1
            0xBB, 0xBB,  // Linked SId 2
            0xE2         // ECC 2
        };

        auto result = parser->parseFig006_ServiceLinking(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.iis_flag, true);
        QCOMPARE(result.linked_service_ids.size(), 2);
        QCOMPARE(result.ecc_ids.size(), 2);
        QCOMPARE(result.ecc_ids[0], 0xE1u);
        QCOMPARE(result.ecc_ids[1], 0xE2u);
    }

    // Test 10: Invalid data size (too small)
    void test_invalid_data_size() {
        qDebug() << "TEST: Invalid data size (too small)";

        std::vector<uint8_t> fig_data = {
            0x06, 0x80, 0x11  // Only 3 bytes, need at least 6
        };

        auto result = parser->parseFig006_ServiceLinking(fig_data);

        QVERIFY(!result.is_valid);
    }

    // Test 11: Empty buffer
    void test_empty_buffer() {
        qDebug() << "TEST: Empty buffer handling";

        std::vector<uint8_t> fig_data = {};

        auto result = parser->parseFig006_ServiceLinking(fig_data);

        QVERIFY(!result.is_valid);
    }

    // Test 12: No linked services (invalid)
    void test_no_linked_services() {
        qDebug() << "TEST: No linked services (should be invalid)";

        std::vector<uint8_t> fig_data = {
            0x06, 0x80, 0xCC, 0xCC,
            0x01, 0x00  // LSN but no linked SIds following
        };

        auto result = parser->parseFig006_ServiceLinking(fig_data);

        QVERIFY(!result.is_valid);  // No linked services = invalid
    }

    // Test 13: LSN=0 (valid edge case)
    void test_lsn_zero() {
        qDebug() << "TEST: LSN=0 (valid edge case)";

        std::vector<uint8_t> fig_data = {
            0x06, 0x80, 0xDD, 0xDD,
            0x00, 0x00,  // LSN=0
            0xEE, 0xEE
        };

        auto result = parser->parseFig006_ServiceLinking(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.lsn, 0u);
    }

    // Test 14: Signal emission verification
    void test_signal_emission() {
        qDebug() << "TEST: Signal emission verification";

        QSignalSpy spy(parser, &FigParser::serviceLinkingDiscovered);

        std::vector<uint8_t> fig_data = {
            0x06, 0x80, 0x11, 0x22, 0x03, 0x45, 0x33, 0x44
        };

        parser->parseFig006_ServiceLinking(fig_data);

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        ServiceLinking emitted = qvariant_cast<ServiceLinking>(arguments.at(0));
        QVERIFY(emitted.is_valid);
    }

    // Test 15: Soft national link (most common scenario)
    void test_common_scenario_soft_national() {
        qDebug() << "TEST: Common scenario - Soft national link";

        std::vector<uint8_t> fig_data = {
            0x06, 0x80,
            0xC0, 0x01,  // SId (DAB service)
            0x00, 0x10,  // Soft, National, LSN=16
            0xC0, 0x02   // Linked to another DAB service
        };

        auto result = parser->parseFig006_ServiceLinking(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.sh_flag, false);  // Soft
        QCOMPARE(result.iis_flag, false); // National
        QCOMPARE(result.la_flag, false);
    }

    // Test 16: Hard national link (regional variant)
    void test_hard_national_regional() {
        qDebug() << "TEST: Hard national link (regional variant)";

        std::vector<uint8_t> fig_data = {
            0x06, 0x80,
            0xD1, 0x00,  // Regional service
            0x42, 0x50,  // Hard, National, LSN=0x250
            0xD1, 0x01   // Regional variant
        };

        auto result = parser->parseFig006_ServiceLinking(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.sh_flag, true);   // Hard (required)
        QCOMPARE(result.iis_flag, false); // National
        QVERIFY(result.getLinkageType().find("Hard") != std::string::npos);
    }

    // Test 17: Minimum valid data size
    void test_minimum_valid_size() {
        qDebug() << "TEST: Minimum valid data size";

        std::vector<uint8_t> fig_data = {
            0x06,        // 1: FIG type
            0x80,        // 2: Flags
            0x00, 0x01,  // 3-4: SId
            0x00, 0x00,  // 5-6: LSN
            0x00, 0x02   // 7-8: Linked SId
        };

        auto result = parser->parseFig006_ServiceLinking(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.linked_service_ids.size(), 1);
    }

    // Test 18: 32-bit international with ECC
    void test_32bit_international_ecc() {
        qDebug() << "TEST: 32-bit SId international with ECC";

        std::vector<uint8_t> fig_data = {
            0x06,
            0xC0,  // IdLQ=1, P/D=1 (32-bit)
            0xE1, 0x00, 0x01, 0x23,  // SId (32-bit)
            0x23, 0x00,  // IIS=1, LSN=0x300
            0xE2, 0x00, 0x01, 0x24,  // Linked SId (32-bit)
            0xE3     // ECC
        };

        auto result = parser->parseFig006_ServiceLinking(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.iis_flag, true);
        QCOMPARE(result.service_id, 0xE1000123u);
        QCOMPARE(result.linked_service_ids[0], 0xE2000124u);
        QCOMPARE(result.ecc_ids[0], 0xE3u);
    }

    // Test 19: Performance benchmark
    void test_parsing_performance() {
        // The benchmark measures parsing, not per-call logging: keep the
        // logger quiet for the timed section (restored on scope exit).
        struct LogLevelGuard {
            Logger::LogLevel saved = Logger::instance().getLogLevel();
            LogLevelGuard() { Logger::instance().setLogLevel(Logger::LogLevel::Warning); }
            ~LogLevelGuard() { Logger::instance().setLogLevel(saved); }
        } logGuard;

        qDebug() << "TEST: Parsing performance benchmark";

        std::vector<uint8_t> fig_data = {
            0x06, 0x80, 0x12, 0x34, 0x01, 0x23, 0x56, 0x78
        };

        const int iterations = 10000;
        QElapsedTimer timer;
        timer.start();

        for (int i = 0; i < iterations; ++i) {
            parser->parseFig006_ServiceLinking(fig_data);
        }

        qint64 elapsed = timer.nsecsElapsed();
        double avg_us = (elapsed / 1000.0) / iterations;

        qDebug() << QString("Average parsing time: %1 µs per FIG 0/6").arg(avg_us, 0, 'f', 2);

        // Performance target: < 15µs (3x tolerance)
        QVERIFY2(avg_us < 15.0,
            QString("Performance: %1 µs exceeds 15µs tolerance").arg(avg_us).toUtf8());
    }

    // Test 20: Complex scenario - Multiple international links with ECCs
    void test_complex_international_scenario() {
        qDebug() << "TEST: Complex international scenario";

        std::vector<uint8_t> fig_data = {
            0x06, 0x00,  // List mode, 16-bit
            0xE1, 0x23,  // SId
            0xE4, 0x56,  // LA=1, SH=1, IIS=1, LSN=0x456
            0xE2, 0x00,  // Linked SId 1
            0xE0,        // ECC 1 (Germany)
            0xE3, 0x00,  // Linked SId 2
            0xF0,        // ECC 2 (France)
            0xE4, 0x00,  // Linked SId 3
            0xF1         // ECC 3 (France variant)
        };

        auto result = parser->parseFig006_ServiceLinking(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.la_flag, true);
        QCOMPARE(result.sh_flag, true);
        QCOMPARE(result.iis_flag, true);
        QCOMPARE(result.linked_service_ids.size(), 3);
        QCOMPARE(result.ecc_ids.size(), 3);
    }
};

// Qt Test main
QTEST_MAIN(TestFig006ServiceLinking)
#include "test_fig006_service_linking.moc"
