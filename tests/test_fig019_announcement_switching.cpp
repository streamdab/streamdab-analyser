/**
 * @file test_fig019_announcement_switching.cpp
 * @brief Comprehensive Test Suite for FIG 0/19 Announcement Switching Parser
 *
 * Tests ETSI EN 300 401 Section 8.1.12 compliance for announcement switching.
 * Part of PDCA Week 7 - Batch 2 / Agent 22 implementation.
 *
 * @author Python Pro Agent
 * @date November 3, 2025
 */

#include <QTest>
#include <QSignalSpy>
#include "../src/core/fig_parser.hpp"
#include "../src/utils/logger.h"

using namespace eti::fig;

class TestFig019AnnouncementSwitching : public QObject {
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
     * @brief Test 1: Basic FIG 0/19 parsing
     * Validates cluster ID, ASw flags, and basic structure
     */
    void test01_BasicParsing() {
        // FIG 0/19: ClusterID=0x42, ASw=0x0001 (Alarm), New=false, Region=false
        std::vector<uint8_t> fig_data = {
            0x13,        // Extension 19
            0x42,        // Cluster ID = 66
            0x00, 0x01,  // ASw flags = 0x0001 (Alarm announcement)
            0x00         // New=0, Region=0, unused bits
        };

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.cluster_id, static_cast<uint8_t>(0x42));
        QCOMPARE(result.asw_flags, static_cast<uint16_t>(0x0001));
        QVERIFY(!result.new_flag);
        QVERIFY(!result.region_flag);
        QCOMPARE(result.subchannel_id, static_cast<uint8_t>(0));
        QVERIFY(result.region_ids.empty());
    }

    /**
     * @brief Test 2: New announcement scenario
     * Tests SubChId extraction when new_flag is set
     */
    void test02_NewAnnouncementWithSubChannel() {
        // FIG 0/19: New announcement with SubChId=0x15 (21)
        std::vector<uint8_t> fig_data = {
            0x13,        // Extension 19
            0x10,        // Cluster ID = 16
            0x00, 0x02,  // ASw flags = 0x0002 (Traffic announcement)
            0x95         // New=1, Region=0, SubChId=0x15 (10010101 = 0x80 | 0x15)
        };

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.cluster_id, static_cast<uint8_t>(0x10));
        QCOMPARE(result.asw_flags, static_cast<uint16_t>(0x0002));
        QVERIFY(result.new_flag);
        QVERIFY(!result.region_flag);
        QCOMPARE(result.subchannel_id, static_cast<uint8_t>(0x15)); // 21
        QVERIFY(result.region_ids.empty());
    }

    /**
     * @brief Test 3: Region scenario
     * Tests Region ID extraction when region_flag is set
     */
    void test03_RegionAnnouncement() {
        // FIG 0/19: Region announcement with Region ID=0x0A (10)
        std::vector<uint8_t> fig_data = {
            0x13,        // Extension 19
            0x05,        // Cluster ID = 5
            0x00, 0x04,  // ASw flags = 0x0004 (Transport Flash)
            0x4A         // New=0, Region=1, Region ID lower=0x0A (01001010 = 0x40 | 0x0A)
        };

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.cluster_id, static_cast<uint8_t>(0x05));
        QCOMPARE(result.asw_flags, static_cast<uint16_t>(0x0004));
        QVERIFY(!result.new_flag);
        QVERIFY(result.region_flag);
        QCOMPARE(result.subchannel_id, static_cast<uint8_t>(0));
        QCOMPARE(result.region_ids.size(), size_t(1));
        QCOMPARE(result.region_ids[0], static_cast<uint8_t>(0x0A));
    }

    /**
     * @brief Test 4: Emergency announcement detection (Alarm)
     * Tests emergency type 0 (Alarm)
     */
    void test04_EmergencyAlarm() {
        std::vector<uint8_t> fig_data = {
            0x13,        // Extension 19
            0x01,        // Cluster ID = 1
            0x00, 0x01,  // ASw flags = 0x0001 (Alarm - emergency)
            0x00
        };

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.asw_flags, static_cast<uint16_t>(0x0001));
        QVERIFY(result.isActive());

        // Check announcement types
        auto types = result.getActiveTypes();
        QCOMPARE(types.size(), size_t(1));
        QCOMPARE(types[0], static_cast<uint8_t>(0)); // Type 0 = Alarm
    }

    /**
     * @brief Test 5: Traffic announcement
     * Tests type 1 (Traffic)
     */
    void test05_TrafficAnnouncement() {
        std::vector<uint8_t> fig_data = {
            0x13,
            0x02,
            0x00, 0x02,  // ASw flags = 0x0002 (Traffic)
            0x00
        };

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.asw_flags, static_cast<uint16_t>(0x0002));

        auto types = result.getActiveTypes();
        QCOMPARE(types.size(), size_t(1));
        QCOMPARE(types[0], static_cast<uint8_t>(1)); // Type 1 = Traffic
    }

    /**
     * @brief Test 6: Transport Flash announcement
     * Tests type 2 (Transport Flash)
     */
    void test06_TransportFlash() {
        std::vector<uint8_t> fig_data = {
            0x13,
            0x03,
            0x00, 0x04,  // ASw flags = 0x0004 (Transport Flash)
            0x00
        };

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.asw_flags, static_cast<uint16_t>(0x0004));

        auto types = result.getActiveTypes();
        QCOMPARE(types.size(), size_t(1));
        QCOMPARE(types[0], static_cast<uint8_t>(2)); // Type 2 = Transport Flash
    }

    /**
     * @brief Test 7: Warning announcement
     * Tests type 3 (Warning)
     */
    void test07_WarningAnnouncement() {
        std::vector<uint8_t> fig_data = {
            0x13,
            0x04,
            0x00, 0x08,  // ASw flags = 0x0008 (Warning)
            0x00
        };

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.asw_flags, static_cast<uint16_t>(0x0008));

        auto types = result.getActiveTypes();
        QCOMPARE(types.size(), size_t(1));
        QCOMPARE(types[0], static_cast<uint8_t>(3)); // Type 3 = Warning
    }

    /**
     * @brief Test 8: News Flash announcement
     * Tests type 4 (News Flash)
     */
    void test08_NewsFlash() {
        std::vector<uint8_t> fig_data = {
            0x13,
            0x05,
            0x00, 0x10,  // ASw flags = 0x0010 (News Flash)
            0x00
        };

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.asw_flags, static_cast<uint16_t>(0x0010));

        auto types = result.getActiveTypes();
        QCOMPARE(types.size(), size_t(1));
        QCOMPARE(types[0], static_cast<uint8_t>(4)); // Type 4 = News Flash
    }

    /**
     * @brief Test 9: Multiple announcement types
     * Tests multiple simultaneous announcements
     */
    void test09_MultipleAnnouncementTypes() {
        std::vector<uint8_t> fig_data = {
            0x13,
            0x06,
            0x00, 0x1B,  // ASw flags = 0x001B (Alarm + Traffic + Warning + News)
            0x00         // = 0x0001 | 0x0002 | 0x0008 | 0x0010
        };

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.asw_flags, static_cast<uint16_t>(0x001B));
        QVERIFY(result.isActive());

        auto types = result.getActiveTypes();
        QCOMPARE(types.size(), size_t(4));
        QVERIFY(types[0] == 0); // Alarm
        QVERIFY(types[1] == 1); // Traffic
        QVERIFY(types[2] == 3); // Warning
        QVERIFY(types[3] == 4); // News Flash
    }

    /**
     * @brief Test 10: Area Weather announcement
     * Tests type 5 (Area Weather)
     */
    void test10_AreaWeather() {
        std::vector<uint8_t> fig_data = {
            0x13,
            0x07,
            0x00, 0x20,  // ASw flags = 0x0020 (Area Weather)
            0x00
        };

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.asw_flags, static_cast<uint16_t>(0x0020));

        auto types = result.getActiveTypes();
        QCOMPARE(types.size(), size_t(1));
        QCOMPARE(types[0], static_cast<uint8_t>(5)); // Type 5 = Area Weather
    }

    /**
     * @brief Test 11: Event announcement
     * Tests type 6 (Event)
     */
    void test11_EventAnnouncement() {
        std::vector<uint8_t> fig_data = {
            0x13,
            0x08,
            0x00, 0x40,  // ASw flags = 0x0040 (Event)
            0x00
        };

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.asw_flags, static_cast<uint16_t>(0x0040));

        auto types = result.getActiveTypes();
        QCOMPARE(types.size(), size_t(1));
        QCOMPARE(types[0], static_cast<uint8_t>(6)); // Type 6 = Event
    }

    /**
     * @brief Test 12: Special Event announcement
     * Tests type 7 (Special Event)
     */
    void test12_SpecialEvent() {
        std::vector<uint8_t> fig_data = {
            0x13,
            0x09,
            0x00, 0x80,  // ASw flags = 0x0080 (Special Event)
            0x00
        };

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.asw_flags, static_cast<uint16_t>(0x0080));

        auto types = result.getActiveTypes();
        QCOMPARE(types.size(), size_t(1));
        QCOMPARE(types[0], static_cast<uint8_t>(7)); // Type 7 = Special Event
    }

    /**
     * @brief Test 13: Programme Info announcement
     * Tests type 8 (Programme Info)
     */
    void test13_ProgrammeInfo() {
        std::vector<uint8_t> fig_data = {
            0x13,
            0x0A,
            0x01, 0x00,  // ASw flags = 0x0100 (Programme Info)
            0x00
        };

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.asw_flags, static_cast<uint16_t>(0x0100));

        auto types = result.getActiveTypes();
        QCOMPARE(types.size(), size_t(1));
        QCOMPARE(types[0], static_cast<uint8_t>(8)); // Type 8 = Programme Info
    }

    /**
     * @brief Test 14: Sport Report announcement
     * Tests type 9 (Sport Report)
     */
    void test14_SportReport() {
        std::vector<uint8_t> fig_data = {
            0x13,
            0x0B,
            0x02, 0x00,  // ASw flags = 0x0200 (Sport Report)
            0x00
        };

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.asw_flags, static_cast<uint16_t>(0x0200));

        auto types = result.getActiveTypes();
        QCOMPARE(types.size(), size_t(1));
        QCOMPARE(types[0], static_cast<uint8_t>(9)); // Type 9 = Sport Report
    }

    /**
     * @brief Test 15: Financial Report announcement
     * Tests type 10 (Financial Report)
     */
    void test15_FinancialReport() {
        std::vector<uint8_t> fig_data = {
            0x13,
            0x0C,
            0x04, 0x00,  // ASw flags = 0x0400 (Financial Report)
            0x00
        };

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.asw_flags, static_cast<uint16_t>(0x0400));

        auto types = result.getActiveTypes();
        QCOMPARE(types.size(), size_t(1));
        QCOMPARE(types[0], static_cast<uint8_t>(10)); // Type 10 = Financial Report
    }

    /**
     * @brief Test 16: Insufficient data - too short
     * Edge case: data too short for complete FIG 0/19
     */
    void test16_InsufficientDataTooShort() {
        std::vector<uint8_t> fig_data = {
            0x13,        // Extension only
            0x01,        // Cluster ID
            0x00         // Only 1 byte of ASw flags (need 2)
        };

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);
        QVERIFY(!result.is_valid);
    }

    /**
     * @brief Test 17: Empty data
     * Edge case: completely empty data
     */
    void test17_EmptyData() {
        std::vector<uint8_t> fig_data;

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);
        QVERIFY(!result.is_valid);
    }

    /**
     * @brief Test 18: Cluster matching
     * Tests the matchesCluster() helper function
     */
    void test18_ClusterMatching() {
        std::vector<uint8_t> fig_data = {
            0x13,
            0x2A,        // Cluster ID = 42
            0x00, 0x01,
            0x00
        };

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QVERIFY(result.matchesCluster(42));
        QVERIFY(!result.matchesCluster(10));
        QVERIFY(!result.matchesCluster(0));
    }

    /**
     * @brief Test 19: Signal emission
     * Tests that the signal is emitted correctly
     */
    void test19_SignalEmission() {
        QSignalSpy spy(parser, &FigParser::announcementSwitchingDetected);
        QVERIFY(spy.isValid());

        std::vector<uint8_t> fig_data = {
            0x13,
            0x15,
            0x00, 0x01,
            0x00
        };

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(spy.count(), 1);

        // Verify signal argument
        auto arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
    }

    /**
     * @brief Test 20: Maximum SubChId (63)
     * Tests maximum valid SubChId value in new announcement
     */
    void test20_MaxSubChannelId() {
        std::vector<uint8_t> fig_data = {
            0x13,
            0x20,
            0x00, 0x01,
            0xBF         // New=1, Region=0, SubChId=0x3F (63) = 10111111
        };

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QVERIFY(result.new_flag);
        QCOMPARE(result.subchannel_id, static_cast<uint8_t>(63));
    }

    /**
     * @brief Test 21: No announcement active (ASw=0x0000)
     * Tests when no announcements are active
     */
    void test21_NoAnnouncementActive() {
        std::vector<uint8_t> fig_data = {
            0x13,
            0x30,
            0x00, 0x00,  // ASw flags = 0x0000 (no announcements)
            0x00
        };

        auto result = parser->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.asw_flags, static_cast<uint16_t>(0x0000));
        QVERIFY(!result.isActive());

        auto types = result.getActiveTypes();
        QVERIFY(types.empty());
    }

    /**
     * @brief Test 22: Performance benchmark
     * Verify parsing performance meets <15µs target
     */
    void test22_PerformanceBenchmark() {
        std::vector<uint8_t> fig_data = {
            0x13,
            0x42,
            0x00, 0x1F,  // Multiple announcements
            0x95         // New announcement with SubChId
        };

        // Warm up
        for (int i = 0; i < 100; ++i) {
            parser->parseFig019_AnnouncementSwitching(fig_data);
        }

        // Benchmark
        const int iterations = 10000;
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            auto result = parser->parseFig019_AnnouncementSwitching(fig_data);
            Q_UNUSED(result);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double avg_us = static_cast<double>(duration.count()) / iterations;

        qDebug() << "FIG 0/19 parsing performance:" << avg_us << "µs per parse";
        QVERIFY2(avg_us < 15.0, QString("Performance target missed: %1 µs (target: <15 µs)")
            .arg(avg_us).toUtf8().constData());
    }
};

QTEST_MAIN(TestFig019AnnouncementSwitching)
#include "test_fig019_announcement_switching.moc"
