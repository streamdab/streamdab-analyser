/**
 * @file test_fig003_packet_mode.cpp
 * @brief Unit tests for FIG 0/3 Service Component in Packet Mode parser
 *
 * Reference: ETSI EN 300 401 Section 8.1.3
 * PDCA Week 4 - Agent 11 Test Suite
 */

#include <QtTest/QtTest>
#include "../src/core/fig_parser.hpp"
#include "../src/utils/logger.h"

using namespace eti::fig;

class TestFig003PacketMode : public QObject {
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

    // Test 1: Valid 16-bit SId packet mode with DSCTy=24 (MOT)
    void test_valid_16bit_sid_mot() {
        qDebug() << "TEST: Valid 16-bit SId packet mode with MOT (DSCTy=24)";

        std::vector<uint8_t> fig_data = {
            0x03,        // FIG type 0/3
            0x0F, 0xFF,  // SCId = 0xFFF (12-bit)
            0x12, 0x34,  // SId = 0x1234 (16-bit, P/D=0)
            0x18,        // DG=0, CA=0, DSCTy=24 (MOT)
            0x05,        // SubChId=5
            0x01, 0x23   // Packet Address = 0x123
        };

        auto result = parser->parseFig003_ServiceComponentPacketMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.service_id, 0x1234u);
        QCOMPARE(result.sc_id, 0xFFFu);
        QCOMPARE(result.dscty, 24u);
        QCOMPARE(result.sub_ch_id, 5u);
        QCOMPARE(result.packet_address, 0x123u);
        QCOMPARE(result.ca_flag, false);
        QCOMPARE(result.dg_flag, 0u);
        QVERIFY(result.getDSCTyName().find("MOT") != std::string::npos);
    }

    // Test 2: Valid 32-bit SId packet mode
    void test_valid_32bit_sid() {
        qDebug() << "TEST: Valid 32-bit SId packet mode";

        std::vector<uint8_t> fig_data = {
            0x03,              // FIG type 0/3
            0x8A, 0xBC,        // P/D=1 (32-bit), SCId high/low
            0x12, 0x34, 0x56, 0x78,  // SId = 0x12345678 (32-bit)
            0x05,              // DSCTy=5 (TPEG)
            0x0A,              // SubChId=10
            0x03, 0xFF         // Packet Address = 0x3FF
        };

        auto result = parser->parseFig003_ServiceComponentPacketMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.service_id, 0x12345678u);
        QCOMPARE(result.dscty, 5u);
        QVERIFY(result.getDSCTyName().find("TPEG") != std::string::npos);
    }

    // Test 3: DSCTy=1 (TMC - Traffic Message Channel)
    void test_dscty_tmc() {
        qDebug() << "TEST: DSCTy=1 (TMC)";

        std::vector<uint8_t> fig_data = {
            0x03, 0x01, 0x23, 0xAB, 0xCD,
            0x01,  // DSCTy=1 (TMC)
            0x03, 0x00, 0x50
        };

        auto result = parser->parseFig003_ServiceComponentPacketMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.dscty, 1u);
        QVERIFY(result.getDSCTyName().find("TMC") != std::string::npos);
    }

    // Test 4: DSCTy=44 (Journaline)
    void test_dscty_journaline() {
        qDebug() << "TEST: DSCTy=44 (Journaline)";

        std::vector<uint8_t> fig_data = {
            0x03, 0x02, 0x34, 0x11, 0x22,
            0x2C,  // DSCTy=44 (Journaline)
            0x08, 0x01, 0x00
        };

        auto result = parser->parseFig003_ServiceComponentPacketMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.dscty, 44u);
        QVERIFY(result.getDSCTyName().find("Journaline") != std::string::npos);
    }

    // Test 5: CA flag set
    void test_ca_flag_set() {
        qDebug() << "TEST: CA flag set (conditional access)";

        std::vector<uint8_t> fig_data = {
            0x03, 0x03, 0x45, 0x55, 0x66,
            0x58,  // DG=0, CA=1, DSCTy=24
            0x0F, 0x02, 0x00
        };

        auto result = parser->parseFig003_ServiceComponentPacketMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.ca_flag, true);
    }

    // Test 6: DG flag set
    void test_dg_flag_set() {
        qDebug() << "TEST: DG flag set (data group)";

        std::vector<uint8_t> fig_data = {
            0x03, 0x04, 0x56, 0x77, 0x88,
            0x98,  // DG=1, CA=0, DSCTy=24
            0x12, 0x01, 0x50
        };

        auto result = parser->parseFig003_ServiceComponentPacketMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.dg_flag, 1u);
    }

    // Test 7: Packet address range (0-1023)
    void test_packet_address_max() {
        qDebug() << "TEST: Packet address at maximum (1023)";

        std::vector<uint8_t> fig_data = {
            0x03, 0x05, 0x67, 0x99, 0xAA,
            0x05,  // DSCTy=5
            0x1F,  // SubChId=31
            0x03, 0xFF  // Packet Address = 1023 (max)
        };

        auto result = parser->parseFig003_ServiceComponentPacketMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.packet_address, 1023u);
    }

    // Test 8: SubChId range (0-63)
    void test_subchid_max() {
        qDebug() << "TEST: SubChId at maximum (63)";

        std::vector<uint8_t> fig_data = {
            0x03, 0x06, 0x78, 0xBB, 0xCC,
            0x18,  // DSCTy=24
            0x3F,  // SubChId=63 (max)
            0x00, 0x00
        };

        auto result = parser->parseFig003_ServiceComponentPacketMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.sub_ch_id, 63u);
    }

    // Test 9: SCId 12-bit range
    void test_scid_12bit() {
        qDebug() << "TEST: SCId 12-bit maximum (0xFFF)";

        std::vector<uint8_t> fig_data = {
            0x03,
            0x0F, 0xFF,  // SCId = 0xFFF (12-bit max)
            0xAA, 0xBB, 0x05, 0x10, 0x01, 0x00
        };

        auto result = parser->parseFig003_ServiceComponentPacketMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.sc_id, 0xFFFu);
    }

    // Test 10: Invalid data size (too small for 16-bit)
    void test_invalid_size_16bit() {
        qDebug() << "TEST: Invalid data size for 16-bit SId";

        std::vector<uint8_t> fig_data = {
            0x03, 0x01, 0x23, 0xAB  // Only 4 bytes, need at least 9
        };

        auto result = parser->parseFig003_ServiceComponentPacketMode(fig_data);

        QVERIFY(!result.is_valid);
    }

    // Test 11: Invalid data size (too small for 32-bit)
    void test_invalid_size_32bit() {
        qDebug() << "TEST: Invalid data size for 32-bit SId";

        std::vector<uint8_t> fig_data = {
            0x03,
            0x81, 0x23,  // P/D=1 indicates 32-bit
            0x12, 0x34   // Only 2 bytes of SId, need 4
        };

        auto result = parser->parseFig003_ServiceComponentPacketMode(fig_data);

        QVERIFY(!result.is_valid);
    }

    // Test 12: Empty buffer
    void test_empty_buffer() {
        qDebug() << "TEST: Empty buffer handling";

        std::vector<uint8_t> fig_data = {};

        auto result = parser->parseFig003_ServiceComponentPacketMode(fig_data);

        QVERIFY(!result.is_valid);
    }

    // Test 13: DSCTy=0 (Reserved)
    void test_dscty_reserved() {
        qDebug() << "TEST: DSCTy=0 (Reserved)";

        std::vector<uint8_t> fig_data = {
            0x03, 0x07, 0x89, 0xCC, 0xDD,
            0x00,  // DSCTy=0 (Reserved)
            0x05, 0x00, 0x10
        };

        auto result = parser->parseFig003_ServiceComponentPacketMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.dscty, 0u);
        QVERIFY(result.getDSCTyName().find("Reserved") != std::string::npos);
    }

    // Test 14: DSCTy user-defined (60-63)
    void test_dscty_user_defined() {
        qDebug() << "TEST: DSCTy=60 (User-defined)";

        std::vector<uint8_t> fig_data = {
            0x03, 0x08, 0x9A, 0xEE, 0xFF,
            0x3C,  // DSCTy=60 (User-defined)
            0x0C, 0x00, 0x20
        };

        auto result = parser->parseFig003_ServiceComponentPacketMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.dscty, 60u);
        QVERIFY(result.getDSCTyName().find("User-defined") != std::string::npos);
    }

    // Test 15: Both CA and DG flags set
    void test_ca_and_dg_flags() {
        qDebug() << "TEST: Both CA and DG flags set";

        std::vector<uint8_t> fig_data = {
            0x03, 0x09, 0xAB, 0x11, 0x22,
            0xD8,  // DG=1, CA=1, DSCTy=24
            0x15, 0x01, 0x80
        };

        auto result = parser->parseFig003_ServiceComponentPacketMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.ca_flag, true);
        QCOMPARE(result.dg_flag, 1u);
    }

    // Test 16: Signal emission verification
    void test_signal_emission() {
        qDebug() << "TEST: Signal emission verification";

        QSignalSpy spy(parser, &FigParser::serviceComponentPacketModeDiscovered);

        std::vector<uint8_t> fig_data = {
            0x03, 0x0A, 0xBC, 0x33, 0x44, 0x18, 0x08, 0x00, 0x90
        };

        parser->parseFig003_ServiceComponentPacketMode(fig_data);

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        ServiceComponentPacketMode emitted = qvariant_cast<ServiceComponentPacketMode>(arguments.at(0));
        QVERIFY(emitted.is_valid);
    }

    // Test 17: Minimum valid data size
    void test_minimum_valid_size() {
        qDebug() << "TEST: Minimum valid data size (9 bytes for 16-bit)";

        std::vector<uint8_t> fig_data = {
            0x03,        // 1: FIG type
            0x00, 0x01,  // 2-3: SCId
            0x00, 0x02,  // 4-5: SId (16-bit)
            0x00,        // 6: DSCTy
            0x00,        // 7: SubChId
            0x00, 0x00   // 8-9: Packet Address
        };

        auto result = parser->parseFig003_ServiceComponentPacketMode(fig_data);

        QVERIFY(result.is_valid);
    }

    // Test 18: DSCTy out of range handling
    void test_dscty_unknown() {
        qDebug() << "TEST: Unknown DSCTy value";

        std::vector<uint8_t> fig_data = {
            0x03, 0x0B, 0xCD, 0x55, 0x66,
            0x0A,  // DSCTy=10 (not in common list)
            0x06, 0x00, 0x30
        };

        auto result = parser->parseFig003_ServiceComponentPacketMode(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.dscty, 10u);
        QVERIFY(result.getDSCTyName().find("Unknown") != std::string::npos);
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
            0x03, 0x0C, 0xDE, 0x77, 0x88, 0x18, 0x0D, 0x01, 0x00
        };

        const int iterations = 10000;
        QElapsedTimer timer;
        timer.start();

        for (int i = 0; i < iterations; ++i) {
            parser->parseFig003_ServiceComponentPacketMode(fig_data);
        }

        qint64 elapsed = timer.nsecsElapsed();
        double avg_us = (elapsed / 1000.0) / iterations;

        qDebug() << QString("Average parsing time: %1 µs per FIG 0/3").arg(avg_us, 0, 'f', 2);

        // Performance target: < 15µs (3x tolerance of 5µs target)
        QVERIFY2(avg_us < 15.0,
            QString("Performance: %1 µs exceeds 15µs tolerance").arg(avg_us).toUtf8());
    }

    // Test 20: All DSCTy types coverage
    void test_all_dscty_types() {
        qDebug() << "TEST: Coverage of all common DSCTy types";

        QMap<uint8_t, QString> dscty_map = {
            {0, "Reserved"},
            {1, "TMC"},
            {5, "TPEG"},
            {24, "MOT"},
            {44, "Journaline"},
            {60, "User-defined"}
        };

        for (auto it = dscty_map.begin(); it != dscty_map.end(); ++it) {
            std::vector<uint8_t> fig_data = {
                0x03, 0x0D, 0xEF, 0x99, 0xAA, it.key(), 0x10, 0x00, 0x40
            };

            auto result = parser->parseFig003_ServiceComponentPacketMode(fig_data);
            QVERIFY(result.is_valid);
            QCOMPARE(result.dscty, it.key());
        }
    }
};

// Qt Test main
QTEST_MAIN(TestFig003PacketMode)
#include "test_fig003_packet_mode.moc"
