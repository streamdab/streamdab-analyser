/**
 * @file test_fig019_announcement_switch.cpp
 * @brief Comprehensive unit tests for FIG 0/19 Announcement Switching parser
 *
 * Tests FIG 0/19 parsing according to ETSI EN 300 401 Section 8.1.12
 * with comprehensive coverage of:
 * - Cluster ID extraction
 * - ASw flags parsing
 * - New flag and Region flag detection
 * - Sub-channel ID extraction
 * - Region IDs parsing (0, 1, multiple)
 * - Active announcement detection
 * - Cluster matching
 * - Invalid data handling
 * - Performance benchmarks
 *
 * @author TypeScript Pro Agent (Agent 2 - PDCA Week 2)
 * @date October 29, 2025
 */

#include <QtTest/QtTest>
#include "../src/core/fig_parser.hpp"
#include <chrono>
#include <vector>

using namespace eti::fig;

class TestFig019AnnouncementSwitch : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Basic parsing tests
    void test_valid_basic_parsing();
    void test_cluster_id_extraction();
    void test_asw_flags_parsing();
    void test_new_flag_detection();
    void test_region_flag_handling();
    void test_subchannel_id_extraction();

    // Region IDs tests
    void test_no_regions();
    void test_single_region();
    void test_multiple_regions();

    // Helper method tests
    void test_isActive();
    void test_getActiveTypes();
    void test_matchesCluster();

    // Validation tests
    void test_too_short_data();
    void test_minimum_size_data();
    void test_edge_cases();

    // Performance test
    void test_parsing_performance();

private:
    FigParser* parser_;

    // Helper method to create FIG 0/19 test data
    std::vector<uint8_t> createFig019Data(
        uint8_t cluster_id,
        uint16_t asw_flags,
        bool new_flag,
        bool region_flag,
        uint8_t subchannel_id,
        const std::vector<uint8_t>& region_ids = {});
};

void TestFig019AnnouncementSwitch::initTestCase() {
    parser_ = new FigParser();
    QVERIFY(parser_->initialize(false, false)); // No Thai support needed
}

void TestFig019AnnouncementSwitch::cleanupTestCase() {
    delete parser_;
}

std::vector<uint8_t> TestFig019AnnouncementSwitch::createFig019Data(
    uint8_t cluster_id,
    uint16_t asw_flags,
    bool new_flag,
    bool region_flag,
    uint8_t subchannel_id,
    const std::vector<uint8_t>& region_ids) {

    std::vector<uint8_t> data;

    // Extension field (FIG 0/19)
    data.push_back(0x13); // Extension 19

    // Cluster ID (8 bits)
    data.push_back(cluster_id);

    // ASw flags (16 bits, big-endian)
    data.push_back((asw_flags >> 8) & 0xFF);
    data.push_back(asw_flags & 0xFF);

    // New flag, Region flag, and SubChId
    uint8_t flags_subch = (subchannel_id & 0x3F);
    if (new_flag) flags_subch |= 0x80;
    if (region_flag) flags_subch |= 0x40;
    data.push_back(flags_subch);

    // Region IDs (if region_flag is set)
    for (uint8_t region_id : region_ids) {
        data.push_back(region_id);
    }

    return data;
}

void TestFig019AnnouncementSwitch::test_valid_basic_parsing() {
    qDebug() << "TEST: Valid basic FIG 0/19 parsing";

    auto fig_data = createFig019Data(0x01, 0x00FF, false, false, 0x05);
    auto result = parser_->parseFig019_AnnouncementSwitching(fig_data);

    QVERIFY(result.is_valid);
    QCOMPARE(result.cluster_id, static_cast<uint8_t>(0x01));
    QCOMPARE(result.asw_flags, static_cast<uint16_t>(0x00FF));
    QCOMPARE(result.new_flag, false);
    QCOMPARE(result.region_flag, false);
    QCOMPARE(result.subchannel_id, static_cast<uint8_t>(0x05));
    QVERIFY(result.region_ids.empty());
}

void TestFig019AnnouncementSwitch::test_cluster_id_extraction() {
    qDebug() << "TEST: Cluster ID extraction";

    std::vector<uint8_t> test_clusters = {0x00, 0x01, 0x0F, 0x7F, 0xFF};

    for (uint8_t cluster : test_clusters) {
        auto fig_data = createFig019Data(cluster, 0x0001, false, false, 0x00);
        auto result = parser_->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.cluster_id, cluster);
    }
}

void TestFig019AnnouncementSwitch::test_asw_flags_parsing() {
    qDebug() << "TEST: ASw flags parsing";

    struct TestCase {
        uint16_t asw_flags;
        bool should_be_active;
    };

    std::vector<TestCase> test_cases = {
        {0x0000, false},  // No flags set
        {0x0001, true},   // Type 0
        {0x0002, true},   // Type 1
        {0x00FF, true},   // Types 0-7
        {0xFFFF, true},   // All types
        {0x8000, true},   // Type 15
    };

    for (const auto& test : test_cases) {
        auto fig_data = createFig019Data(0x01, test.asw_flags, false, false, 0x00);
        auto result = parser_->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.asw_flags, test.asw_flags);
        QCOMPARE(result.isActive(), test.should_be_active);
    }
}

void TestFig019AnnouncementSwitch::test_new_flag_detection() {
    qDebug() << "TEST: New flag detection";

    // Test with new_flag = false
    auto fig_data1 = createFig019Data(0x01, 0x0001, false, false, 0x00);
    auto result1 = parser_->parseFig019_AnnouncementSwitching(fig_data1);

    QVERIFY(result1.is_valid);
    QCOMPARE(result1.new_flag, false);

    // Test with new_flag = true
    auto fig_data2 = createFig019Data(0x01, 0x0001, true, false, 0x00);
    auto result2 = parser_->parseFig019_AnnouncementSwitching(fig_data2);

    QVERIFY(result2.is_valid);
    QCOMPARE(result2.new_flag, true);
}

void TestFig019AnnouncementSwitch::test_region_flag_handling() {
    qDebug() << "TEST: Region flag handling";

    // Test with region_flag = false
    auto fig_data1 = createFig019Data(0x01, 0x0001, false, false, 0x00);
    auto result1 = parser_->parseFig019_AnnouncementSwitching(fig_data1);

    QVERIFY(result1.is_valid);
    QCOMPARE(result1.region_flag, false);

    // Test with region_flag = true
    auto fig_data2 = createFig019Data(0x01, 0x0001, false, true, 0x00, {0x10});
    auto result2 = parser_->parseFig019_AnnouncementSwitching(fig_data2);

    QVERIFY(result2.is_valid);
    QCOMPARE(result2.region_flag, true);
}

void TestFig019AnnouncementSwitch::test_subchannel_id_extraction() {
    qDebug() << "TEST: Sub-channel ID extraction";

    std::vector<uint8_t> test_subch_ids = {0x00, 0x01, 0x0F, 0x1F, 0x3F};

    for (uint8_t subch_id : test_subch_ids) {
        auto fig_data = createFig019Data(0x01, 0x0001, false, false, subch_id);
        auto result = parser_->parseFig019_AnnouncementSwitching(fig_data);

        QVERIFY(result.is_valid);
        QCOMPARE(result.subchannel_id, subch_id);
    }
}

void TestFig019AnnouncementSwitch::test_no_regions() {
    qDebug() << "TEST: No region IDs (region_flag = false)";

    auto fig_data = createFig019Data(0x01, 0x0001, false, false, 0x00);
    auto result = parser_->parseFig019_AnnouncementSwitching(fig_data);

    QVERIFY(result.is_valid);
    QVERIFY(result.region_ids.empty());
}

void TestFig019AnnouncementSwitch::test_single_region() {
    qDebug() << "TEST: Single region ID";

    auto fig_data = createFig019Data(0x01, 0x0001, false, true, 0x00, {0x42});
    auto result = parser_->parseFig019_AnnouncementSwitching(fig_data);

    QVERIFY(result.is_valid);
    QCOMPARE(result.region_ids.size(), static_cast<size_t>(1));
    QCOMPARE(result.region_ids[0], static_cast<uint8_t>(0x42));
}

void TestFig019AnnouncementSwitch::test_multiple_regions() {
    qDebug() << "TEST: Multiple region IDs";

    std::vector<uint8_t> regions = {0x10, 0x20, 0x30, 0x40, 0x50};
    auto fig_data = createFig019Data(0x01, 0x0001, false, true, 0x00, regions);
    auto result = parser_->parseFig019_AnnouncementSwitching(fig_data);

    QVERIFY(result.is_valid);
    QCOMPARE(result.region_ids.size(), regions.size());
    for (size_t i = 0; i < regions.size(); ++i) {
        QCOMPARE(result.region_ids[i], regions[i]);
    }
}

void TestFig019AnnouncementSwitch::test_isActive() {
    qDebug() << "TEST: isActive() method";

    // Test inactive (no ASw flags)
    auto fig_data1 = createFig019Data(0x01, 0x0000, false, false, 0x00);
    auto result1 = parser_->parseFig019_AnnouncementSwitching(fig_data1);
    QVERIFY(!result1.isActive());

    // Test active (with ASw flags)
    auto fig_data2 = createFig019Data(0x01, 0x0001, false, false, 0x00);
    auto result2 = parser_->parseFig019_AnnouncementSwitching(fig_data2);
    QVERIFY(result2.isActive());
}

void TestFig019AnnouncementSwitch::test_getActiveTypes() {
    qDebug() << "TEST: getActiveTypes() method";

    // Test with specific announcement types
    uint16_t asw_flags = 0x0005; // Types 0 and 2
    auto fig_data = createFig019Data(0x01, asw_flags, false, false, 0x00);
    auto result = parser_->parseFig019_AnnouncementSwitching(fig_data);

    auto active_types = result.getActiveTypes();
    QCOMPARE(active_types.size(), static_cast<size_t>(2));
    QVERIFY(std::find(active_types.begin(), active_types.end(), 0) != active_types.end());
    QVERIFY(std::find(active_types.begin(), active_types.end(), 2) != active_types.end());
}

void TestFig019AnnouncementSwitch::test_matchesCluster() {
    qDebug() << "TEST: matchesCluster() method";

    auto fig_data = createFig019Data(0x42, 0x0001, false, false, 0x00);
    auto result = parser_->parseFig019_AnnouncementSwitching(fig_data);

    QVERIFY(result.matchesCluster(0x42));
    QVERIFY(!result.matchesCluster(0x43));
}

void TestFig019AnnouncementSwitch::test_too_short_data() {
    qDebug() << "TEST: Too short data handling";

    std::vector<size_t> short_lengths = {0, 1, 2, 3, 4};

    for (size_t len : short_lengths) {
        std::vector<uint8_t> short_data(len, 0x00);
        auto result = parser_->parseFig019_AnnouncementSwitching(short_data);

        QVERIFY(!result.is_valid);
    }
}

void TestFig019AnnouncementSwitch::test_minimum_size_data() {
    qDebug() << "TEST: Minimum size data (5 bytes)";

    // Minimum: Extension + Cluster ID + ASw flags (2) + flags/SubChId = 5 bytes
    std::vector<uint8_t> min_data = {0x13, 0x01, 0x00, 0x01, 0x05};
    auto result = parser_->parseFig019_AnnouncementSwitching(min_data);

    QVERIFY(result.is_valid);
    QCOMPARE(result.cluster_id, static_cast<uint8_t>(0x01));
    QCOMPARE(result.asw_flags, static_cast<uint16_t>(0x0001));
    QCOMPARE(result.subchannel_id, static_cast<uint8_t>(0x05));
}

void TestFig019AnnouncementSwitch::test_edge_cases() {
    qDebug() << "TEST: Edge cases";

    // Test with all flags set and maximum values
    auto fig_data = createFig019Data(0xFF, 0xFFFF, true, true, 0x3F, {0xFF, 0xFE, 0xFD});
    auto result = parser_->parseFig019_AnnouncementSwitching(fig_data);

    QVERIFY(result.is_valid);
    QCOMPARE(result.cluster_id, static_cast<uint8_t>(0xFF));
    QCOMPARE(result.asw_flags, static_cast<uint16_t>(0xFFFF));
    QVERIFY(result.new_flag);
    QVERIFY(result.region_flag);
    QCOMPARE(result.subchannel_id, static_cast<uint8_t>(0x3F));
    QCOMPARE(result.region_ids.size(), static_cast<size_t>(3));
}

void TestFig019AnnouncementSwitch::test_parsing_performance() {
    qDebug() << "TEST: Parsing performance benchmark";

    auto fig_data = createFig019Data(0x01, 0x00FF, false, true, 0x05, {0x10, 0x20});

    // Warm-up
    for (int i = 0; i < 10; ++i) {
        parser_->parseFig019_AnnouncementSwitching(fig_data);
    }

    // Benchmark
    constexpr int iterations = 1000;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        auto result = parser_->parseFig019_AnnouncementSwitching(fig_data);
        QVERIFY(result.is_valid);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time);

    double avg_time_us = static_cast<double>(duration.count()) / iterations;

    qDebug() << QString("Average parsing time: %1 µs per FIG 0/19")
        .arg(avg_time_us, 0, 'f', 2);

    // Performance target: < 100 µs per parse
    QVERIFY2(avg_time_us < 100.0,
        QString("Performance target failed: %1 µs > 100 µs")
        .arg(avg_time_us, 0, 'f', 2).toUtf8().constData());
}

// Register metatype for signal/slot with AnnouncementSwitching
Q_DECLARE_METATYPE(AnnouncementSwitching)

QTEST_MAIN(TestFig019AnnouncementSwitch)
#include "test_fig019_announcement_switch.moc"
