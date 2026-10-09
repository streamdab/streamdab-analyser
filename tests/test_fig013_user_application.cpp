/**
 * @file test_fig013_user_application.cpp
 * @brief Unit tests for FIG 0/13 User Application Information parser
 *
 * Reference: ETSI EN 300 401 Section 8.1.14
 * PDCA Week 4 - Agent 13 Test Suite
 */

#include <QtTest/QtTest>
#include "../src/core/fig_parser.hpp"

using namespace eti::fig;

class TestFig013UserApplication : public QObject {
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

    // Test 1: Valid 16-bit SId with MOT SlideShow (UAType=0x002)
    void test_valid_16bit_sid_mot() {
        qDebug() << "TEST: Valid 16-bit SId with MOT SlideShow (UAType=0x002)";

        std::vector<uint8_t> fig_data = {
            0x0D,        // FIG type 0/13
            0x0F, 0xFF,  // SCIdS = 0xFFF (12-bit)
            0x12, 0x34,  // SId = 0x1234 (16-bit, P/D=0)
            0x00, 0x42,  // UAType = 0x002 (MOT SlideShow), CA=0
            0x03,        // UA data length = 3 bytes
            0x01, 0x02, 0x03  // UA data
        };

        auto result = parser->parseFig013_UserApplicationInfo(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.service_id, 0x1234u);
        QCOMPARE(result.sc_ids, 0xFFFu);
        QCOMPARE(result.ua_type, 0x002u);
        QCOMPARE(result.ca_flag, 0u);
        QCOMPARE(result.ua_data_length, 3u);
        QCOMPARE(result.ua_data.size(), static_cast<size_t>(3));
        QVERIFY(result.isMOT());
        QVERIFY(result.getUATypeName().find("MOT") != std::string::npos);
    }

    // Test 2: Valid 32-bit SId with TPEG (UAType=0x004)
    void test_valid_32bit_sid_tpeg() {
        qDebug() << "TEST: Valid 32-bit SId with TPEG (UAType=0x004)";

        std::vector<uint8_t> fig_data = {
            0x0D,              // FIG type 0/13
            0x8A, 0xBC,        // P/D=1 (32-bit), SCIdS high/low
            0x12, 0x34, 0x56, 0x78,  // SId = 0x12345678 (32-bit)
            0x00, 0x84,        // UAType = 0x004 (TPEG), CA=0
            0x05,              // UA data length = 5 bytes
            0x11, 0x22, 0x33, 0x44, 0x55  // UA data
        };

        auto result = parser->parseFig013_UserApplicationInfo(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.service_id, 0x12345678u);
        QCOMPARE(result.ua_type, 0x004u);
        QCOMPARE(result.ua_data_length, 5u);
        QVERIFY(result.isTPEG());
        QVERIFY(result.getUATypeName().find("TPEG") != std::string::npos);
    }

    // Test 3: Journaline (UAType=0x441)
    void test_uatype_journaline() {
        qDebug() << "TEST: UAType=0x441 (Journaline)";

        std::vector<uint8_t> fig_data = {
            0x0D, 0x01, 0x23, 0xAB, 0xCD,
            0x88, 0x21,  // UAType = 0x441 (Journaline)
            0x02,        // UA data length = 2 bytes
            0xAA, 0xBB
        };

        auto result = parser->parseFig013_UserApplicationInfo(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.ua_type, 0x441u);
        QVERIFY(result.isJournaline());
        QVERIFY(result.getUATypeName().find("Journaline") != std::string::npos);
    }

    // Test 4: EPG (UAType=0x123)
    void test_uatype_epg() {
        qDebug() << "TEST: UAType=0x123 (EPG - Electronic Programme Guide)";

        std::vector<uint8_t> fig_data = {
            0x0D, 0x02, 0x34, 0x11, 0x22,
            0x24, 0x60,  // UAType = 0x123 (EPG) - 0x123 << 5 = 0x2460
            0x04,        // UA data length = 4 bytes
            0x01, 0x02, 0x03, 0x04
        };

        auto result = parser->parseFig013_UserApplicationInfo(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.ua_type, 0x123u);
        QVERIFY(result.isEPG());
        QVERIFY(result.getUATypeName().find("EPG") != std::string::npos);
    }

    // Test 5: CA flag set (conditional access)
    void test_ca_flag_set() {
        qDebug() << "TEST: CA flag set (conditional access)";

        std::vector<uint8_t> fig_data = {
            0x0D, 0x03, 0x45, 0x33, 0x44,
            0x80, 0x40,  // UAType = 0x002, CA=1 - (0x8000 | (0x002 << 5)) = 0x8040
            0x01,        // UA data length = 1 byte
            0xFF
        };

        auto result = parser->parseFig013_UserApplicationInfo(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.ca_flag, 1u);
    }

    // Test 6: Zero-length UA data
    void test_zero_length_ua_data() {
        qDebug() << "TEST: Zero-length UA data";

        std::vector<uint8_t> fig_data = {
            0x0D, 0x04, 0x56, 0x55, 0x66,
            0x00, 0x42,  // UAType = 0x002 (MOT)
            0x00         // UA data length = 0 bytes
        };

        auto result = parser->parseFig013_UserApplicationInfo(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.ua_data_length, 0u);
        QCOMPARE(result.ua_data.size(), static_cast<size_t>(0));
    }

    // Test 7: Maximum UA data length (31 bytes)
    void test_max_ua_data_length() {
        qDebug() << "TEST: Maximum UA data length (31 bytes)";

        std::vector<uint8_t> fig_data = {
            0x0D, 0x05, 0x67, 0x77, 0x88,
            0x00, 0x84,  // UAType = 0x004 (TPEG)
            0x1F         // UA data length = 31 bytes (max)
        };

        // Add 31 bytes of UA data
        for (int i = 0; i < 31; ++i) {
            fig_data.push_back(static_cast<uint8_t>(i));
        }

        auto result = parser->parseFig013_UserApplicationInfo(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.ua_data_length, 31u);
        QCOMPARE(result.ua_data.size(), static_cast<size_t>(31));
    }

    // Test 8: SCIdS 12-bit range
    void test_scids_12bit() {
        qDebug() << "TEST: SCIdS 12-bit maximum (0xFFF)";

        std::vector<uint8_t> fig_data = {
            0x0D,
            0x0F, 0xFF,  // SCIdS = 0xFFF (12-bit max)
            0xAA, 0xBB, 0x00, 0x42, 0x00
        };

        auto result = parser->parseFig013_UserApplicationInfo(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.sc_ids, 0xFFFu);
    }

    // Test 9: UAType 11-bit range (0-2047)
    void test_uatype_11bit_max() {
        qDebug() << "TEST: UAType 11-bit maximum (0x7FF = 2047)";

        std::vector<uint8_t> fig_data = {
            0x0D, 0x06, 0x78, 0xCC, 0xDD,
            0xFF, 0xE0,  // UAType = 0x7FF (2047, max 11-bit) - 0x7FF << 5 = 0xFFE0
            0x00
        };

        auto result = parser->parseFig013_UserApplicationInfo(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.ua_type, 0x7FFu);
    }

    // Test 10: Invalid data size (too small for 16-bit)
    void test_invalid_size_16bit() {
        qDebug() << "TEST: Invalid data size for 16-bit SId";

        std::vector<uint8_t> fig_data = {
            0x0D, 0x01, 0x23, 0xAB  // Only 4 bytes, need at least 8
        };

        auto result = parser->parseFig013_UserApplicationInfo(fig_data);

        QVERIFY(!result.is_valid);
    }

    // Test 11: Invalid data size (too small for 32-bit)
    void test_invalid_size_32bit() {
        qDebug() << "TEST: Invalid data size for 32-bit SId";

        std::vector<uint8_t> fig_data = {
            0x0D,
            0x81, 0x23,  // P/D=1 indicates 32-bit
            0x12, 0x34   // Only 2 bytes of SId, need 4
        };

        auto result = parser->parseFig013_UserApplicationInfo(fig_data);

        QVERIFY(!result.is_valid);
    }

    // Test 12: Empty buffer
    void test_empty_buffer() {
        qDebug() << "TEST: Empty buffer handling";

        std::vector<uint8_t> fig_data = {};

        auto result = parser->parseFig013_UserApplicationInfo(fig_data);

        QVERIFY(!result.is_valid);
    }

    // Test 13: UAType=0x001 (Not used)
    void test_uatype_not_used() {
        qDebug() << "TEST: UAType=0x001 (Not used)";

        std::vector<uint8_t> fig_data = {
            0x0D, 0x07, 0x89, 0xEE, 0xFF,
            0x00, 0x22,  // UAType = 0x001 (Not used)
            0x00
        };

        auto result = parser->parseFig013_UserApplicationInfo(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.ua_type, 0x001u);
        QVERIFY(result.getUATypeName().find("Not used") != std::string::npos);
    }

    // Test 14: Unknown UAType
    void test_uatype_unknown() {
        qDebug() << "TEST: Unknown UAType value";

        std::vector<uint8_t> fig_data = {
            0x0D, 0x08, 0x9A, 0x11, 0x22,
            0x3F, 0xE0,  // UAType = 0x1FF (unknown) - 0x1FF << 5 = 0x3FE0
            0x00
        };

        auto result = parser->parseFig013_UserApplicationInfo(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.ua_type, 0x1FFu);
        QVERIFY(result.getUATypeName().find("Unknown") != std::string::npos);
    }

    // Test 15: Multiple UA data validation
    void test_ua_data_content() {
        qDebug() << "TEST: UA data content validation";

        std::vector<uint8_t> fig_data = {
            0x0D, 0x09, 0xAB, 0x33, 0x44,
            0x00, 0x42,  // UAType = 0x002 (MOT)
            0x05,        // UA data length = 5 bytes
            0x11, 0x22, 0x33, 0x44, 0x55
        };

        auto result = parser->parseFig013_UserApplicationInfo(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.ua_data.size(), static_cast<size_t>(5));
        QCOMPARE(result.ua_data[0], 0x11u);
        QCOMPARE(result.ua_data[1], 0x22u);
        QCOMPARE(result.ua_data[2], 0x33u);
        QCOMPARE(result.ua_data[3], 0x44u);
        QCOMPARE(result.ua_data[4], 0x55u);
    }

    // Test 16: Signal emission verification
    void test_signal_emission() {
        qDebug() << "TEST: Signal emission verification";

        QSignalSpy spy(parser, &FigParser::userApplicationInfoDiscovered);

        std::vector<uint8_t> fig_data = {
            0x0D, 0x0A, 0xBC, 0x55, 0x66, 0x00, 0x42, 0x00
        };

        parser->parseFig013_UserApplicationInfo(fig_data);

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        UserApplicationInfo emitted = qvariant_cast<UserApplicationInfo>(arguments.at(0));
        QVERIFY(emitted.is_valid);
    }

    // Test 17: Minimum valid data size (8 bytes for 16-bit)
    void test_minimum_valid_size() {
        qDebug() << "TEST: Minimum valid data size (8 bytes for 16-bit)";

        std::vector<uint8_t> fig_data = {
            0x0D,        // 1: FIG type
            0x00, 0x01,  // 2-3: SCIdS
            0x00, 0x02,  // 4-5: SId (16-bit)
            0x00, 0x42,  // 6-7: UAType
            0x00         // 8: UA data length = 0
        };

        auto result = parser->parseFig013_UserApplicationInfo(fig_data);

        QVERIFY(result.is_valid);
    }

    // Test 18: All helper methods
    void test_helper_methods() {
        qDebug() << "TEST: Helper methods (isMOT, isTPEG, isJournaline, isEPG)";

        // MOT
        std::vector<uint8_t> mot_data = {
            0x0D, 0x0B, 0xCD, 0x77, 0x88, 0x00, 0x42, 0x00
        };
        auto mot_result = parser->parseFig013_UserApplicationInfo(mot_data);
        QVERIFY(mot_result.isMOT());
        QVERIFY(!mot_result.isTPEG());
        QVERIFY(!mot_result.isJournaline());
        QVERIFY(!mot_result.isEPG());

        // TPEG
        std::vector<uint8_t> tpeg_data = {
            0x0D, 0x0C, 0xDE, 0x99, 0xAA, 0x00, 0x84, 0x00
        };
        auto tpeg_result = parser->parseFig013_UserApplicationInfo(tpeg_data);
        QVERIFY(!tpeg_result.isMOT());
        QVERIFY(tpeg_result.isTPEG());
        QVERIFY(!tpeg_result.isJournaline());
        QVERIFY(!tpeg_result.isEPG());

        // Journaline
        std::vector<uint8_t> jl_data = {
            0x0D, 0x0D, 0xEF, 0xBB, 0xCC, 0x88, 0x21, 0x00
        };
        auto jl_result = parser->parseFig013_UserApplicationInfo(jl_data);
        QVERIFY(!jl_result.isMOT());
        QVERIFY(!jl_result.isTPEG());
        QVERIFY(jl_result.isJournaline());
        QVERIFY(!jl_result.isEPG());

        // EPG
        std::vector<uint8_t> epg_data = {
            0x0D, 0x0E, 0xF0, 0xDD, 0xEE, 0x24, 0x60, 0x00  // UAType = 0x123 (EPG)
        };
        auto epg_result = parser->parseFig013_UserApplicationInfo(epg_data);
        QVERIFY(!epg_result.isMOT());
        QVERIFY(!epg_result.isTPEG());
        QVERIFY(!epg_result.isJournaline());
        QVERIFY(epg_result.isEPG());
    }

    // Test 19: Performance benchmark
    void test_parsing_performance() {
        qDebug() << "TEST: Parsing performance benchmark";

        std::vector<uint8_t> fig_data = {
            0x0D, 0x0F, 0xFF, 0xFF, 0xFF, 0x00, 0x42, 0x03, 0x01, 0x02, 0x03
        };

        const int iterations = 10000;
        QElapsedTimer timer;
        timer.start();

        for (int i = 0; i < iterations; ++i) {
            parser->parseFig013_UserApplicationInfo(fig_data);
        }

        qint64 elapsed = timer.nsecsElapsed();
        double avg_us = (elapsed / 1000.0) / iterations;

        qDebug() << QString("Average parsing time: %1 µs per FIG 0/13").arg(avg_us, 0, 'f', 2);

        // Performance target: < 15µs (3x tolerance of 5µs target)
        QVERIFY2(avg_us < 15.0,
            QString("Performance: %1 µs exceeds 15µs tolerance").arg(avg_us).toUtf8());
    }

    // Test 20: All UAType coverage
    void test_all_uatype_coverage() {
        qDebug() << "TEST: Coverage of all common UAType values";

        QMap<uint16_t, QString> uatype_map = {
            {0x001, "Not used"},
            {0x002, "MOT"},
            {0x004, "TPEG"},
            {0x123, "EPG"},
            {0x441, "Journaline"}
        };

        for (auto it = uatype_map.begin(); it != uatype_map.end(); ++it) {
            std::vector<uint8_t> fig_data = {
                0x0D, 0x00, 0x10, 0xAA, 0xBB,
                static_cast<uint8_t>((it.key() >> 3) & 0xFF),
                static_cast<uint8_t>((it.key() << 5) & 0xE0),
                0x00
            };

            auto result = parser->parseFig013_UserApplicationInfo(fig_data);
            QVERIFY(result.is_valid);
            QCOMPARE(result.ua_type, it.key());
        }
    }
};

// Qt Test main
QTEST_MAIN(TestFig013UserApplication)
#include "test_fig013_user_application.moc"
