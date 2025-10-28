/**
 * @file test_fig018_announcement.cpp
 * @brief FIG 0/18 (Announcement Support) test suite
 * @author StreamDAB Team
 * @date 2025-10-28
 *
 * Test Coverage:
 * - Valid FIG 0/18 parsing (16-bit and 24-bit Service IDs)
 * - ASu flags extraction and interpretation
 * - Cluster ID list parsing
 * - Emergency announcement detection
 * - All announcement types (0-15)
 * - Edge cases (zero clusters, max clusters)
 * - Invalid data handling
 * - Performance benchmarking
 *
 * Standards: ETSI EN 300 401 V2.1.1 Section 8.1.6.2
 */

#include <QtTest/QtTest>
#include "../src/core/fig_parser.hpp"
#include <vector>


using namespace eti::fig;
class TestFig018Announcement : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Valid parsing tests
    void test_valid_16bit_sid();
    void test_valid_24bit_sid();

    // ASu flags tests
    void test_asu_flags_extraction();

    // Cluster ID tests
    void test_cluster_ids_parsing();
    void test_zero_clusters();
    void test_single_cluster();
    void test_multiple_clusters();

    // Announcement type tests
    void test_emergency_alarm_type();
    void test_traffic_flash_type();
    void test_news_flash_type();
    void test_multiple_announcement_types();
    void test_all_announcement_types();

    // Utility method tests
    void test_supportsAnnouncementType();
    void test_getSupportedTypes();
    void test_isEmergencyCapable();

    // Edge cases and invalid data
    void test_invalid_empty_data();
    void test_invalid_insufficient_data();
    void test_invalid_incomplete_cluster_ids();
    void test_max_clusters();

    // Performance test
    void test_parsing_performance();

private:
    FigParser* parser_;

    // Helper method to create FIG 0/18 test data
    std::vector<uint8_t> createFig018Data(
        uint32_t service_id,
        bool use_24bit_sid,
        uint16_t asu_flags,
        const std::vector<uint8_t>& cluster_ids);
};

void TestFig018Announcement::initTestCase() {
    parser_ = new FigParser();
    QVERIFY(parser_->initialize(true, false)); // Enable Thai support, no strict compliance
}

void TestFig018Announcement::cleanupTestCase() {
    delete parser_;
}

std::vector<uint8_t> TestFig018Announcement::createFig018Data(
    uint32_t service_id,
    bool use_24bit_sid,
    uint16_t asu_flags,
    const std::vector<uint8_t>& cluster_ids) {

    std::vector<uint8_t> data;

    // Extension field (FIG 0/18)
    data.push_back(18);

    // Service ID with PD flag
    if (use_24bit_sid) {
        // PD flag = 1 (bit 7), followed by 23 bits of SId
        data.push_back(0x80 | ((service_id >> 16) & 0x7F));
        data.push_back((service_id >> 8) & 0xFF);
        data.push_back(service_id & 0xFF);
    } else {
        // PD flag = 0, followed by 15 bits of SId (16-bit SId)
        // CRITICAL FIX: Mask bit 7 to ensure PD=0 for 16-bit Service IDs
        data.push_back((service_id >> 8) & 0x7F);
        data.push_back(service_id & 0xFF);
    }

    // ASu flags (16 bits, big-endian)
    data.push_back((asu_flags >> 8) & 0xFF);
    data.push_back(asu_flags & 0xFF);

    // Cluster count
    data.push_back(static_cast<uint8_t>(cluster_ids.size()));

    // Cluster IDs
    for (uint8_t cluster_id : cluster_ids) {
        data.push_back(cluster_id);
    }

    return data;
}

void TestFig018Announcement::test_valid_16bit_sid() {
    qDebug() << "TEST: Valid FIG 0/18 with 16-bit Service ID";

    auto fig_data = createFig018Data(0x1234, false, 0x0001, {0x10});
    auto announcement = parser_->parseFig018_AnnouncementSupport(fig_data);

    QVERIFY(announcement.is_valid);
    QCOMPARE(announcement.service_id, static_cast<uint32_t>(0x1234));
    QCOMPARE(announcement.asu_flags, static_cast<uint16_t>(0x0001));
    QCOMPARE(announcement.cluster_count, static_cast<uint8_t>(1));
    QCOMPARE(announcement.cluster_ids.size(), static_cast<size_t>(1));
    QCOMPARE(announcement.cluster_ids[0], static_cast<uint8_t>(0x10));
    QVERIFY(announcement.isEmergencyCapable());
}

void TestFig018Announcement::test_valid_24bit_sid() {
    qDebug() << "TEST: Valid FIG 0/18 with 24-bit Service ID";

    auto fig_data = createFig018Data(0x123456, true, 0x0002, {0x20});
    auto announcement = parser_->parseFig018_AnnouncementSupport(fig_data);

    QVERIFY(announcement.is_valid);
    QCOMPARE(announcement.service_id, static_cast<uint32_t>(0x123456));
    QCOMPARE(announcement.asu_flags, static_cast<uint16_t>(0x0002));
    QCOMPARE(announcement.cluster_count, static_cast<uint8_t>(1));
    QCOMPARE(announcement.cluster_ids.size(), static_cast<size_t>(1));
    QCOMPARE(announcement.cluster_ids[0], static_cast<uint8_t>(0x20));
    QVERIFY(announcement.isEmergencyCapable());
}

void TestFig018Announcement::test_asu_flags_extraction() {
    qDebug() << "TEST: ASu flags extraction";

    // Test different ASu flag values
    auto ann1 = parser_->parseFig018_AnnouncementSupport(
        createFig018Data(0x5000, false, 0x0001, {0x30}));
    QVERIFY(ann1.is_valid);
    QCOMPARE(ann1.asu_flags, static_cast<uint16_t>(0x0001));
    QVERIFY(ann1.isEmergencyCapable());

    auto ann2 = parser_->parseFig018_AnnouncementSupport(
        createFig018Data(0x5000, false, 0x0002, {0x30}));
    QVERIFY(ann2.is_valid);
    QCOMPARE(ann2.asu_flags, static_cast<uint16_t>(0x0002));
    QVERIFY(ann2.isEmergencyCapable());

    auto ann3 = parser_->parseFig018_AnnouncementSupport(
        createFig018Data(0x5000, false, 0x0010, {0x30}));
    QVERIFY(ann3.is_valid);
    QCOMPARE(ann3.asu_flags, static_cast<uint16_t>(0x0010));
    QVERIFY(ann3.isEmergencyCapable());

    auto ann4 = parser_->parseFig018_AnnouncementSupport(
        createFig018Data(0x5000, false, 0x00FF, {0x30}));
    QVERIFY(ann4.is_valid);
    QCOMPARE(ann4.asu_flags, static_cast<uint16_t>(0x00FF));
    QVERIFY(ann4.isEmergencyCapable());

    auto ann5 = parser_->parseFig018_AnnouncementSupport(
        createFig018Data(0x5000, false, 0xFF00, {0x30}));
    QVERIFY(ann5.is_valid);
    QCOMPARE(ann5.asu_flags, static_cast<uint16_t>(0xFF00));
    QVERIFY(!ann5.isEmergencyCapable()); // No emergency bits set

    auto ann6 = parser_->parseFig018_AnnouncementSupport(
        createFig018Data(0x5000, false, 0xFFFF, {0x30}));
    QVERIFY(ann6.is_valid);
    QCOMPARE(ann6.asu_flags, static_cast<uint16_t>(0xFFFF));
    QVERIFY(ann6.isEmergencyCapable());
}

void TestFig018Announcement::test_cluster_ids_parsing() {
    qDebug() << "TEST: Cluster IDs parsing";

    std::vector<uint8_t> clusters = {0x01, 0x02, 0x03, 0x04};
    auto fig_data = createFig018Data(0x6000, false, 0x0001, clusters);
    auto announcement = parser_->parseFig018_AnnouncementSupport(fig_data);

    QVERIFY(announcement.is_valid);
    QCOMPARE(announcement.cluster_count, static_cast<uint8_t>(4));
    QCOMPARE(announcement.cluster_ids.size(), static_cast<size_t>(4));
    QCOMPARE(announcement.cluster_ids[0], static_cast<uint8_t>(0x01));
    QCOMPARE(announcement.cluster_ids[1], static_cast<uint8_t>(0x02));
    QCOMPARE(announcement.cluster_ids[2], static_cast<uint8_t>(0x03));
    QCOMPARE(announcement.cluster_ids[3], static_cast<uint8_t>(0x04));
}

void TestFig018Announcement::test_zero_clusters() {
    qDebug() << "TEST: Zero clusters";

    auto fig_data = createFig018Data(0x7000, false, 0x0001, {});
    auto announcement = parser_->parseFig018_AnnouncementSupport(fig_data);

    QVERIFY(announcement.is_valid);
    QCOMPARE(announcement.cluster_count, static_cast<uint8_t>(0));
    QVERIFY(announcement.cluster_ids.empty());
}

void TestFig018Announcement::test_single_cluster() {
    qDebug() << "TEST: Single cluster";

    auto fig_data = createFig018Data(0x1000, false, 0x0004, {0x50});
    auto announcement = parser_->parseFig018_AnnouncementSupport(fig_data);

    QVERIFY(announcement.is_valid);
    QCOMPARE(announcement.cluster_count, static_cast<uint8_t>(1));
    QCOMPARE(announcement.cluster_ids.size(), static_cast<size_t>(1));
}

void TestFig018Announcement::test_multiple_clusters() {
    qDebug() << "TEST: Multiple clusters";

    std::vector<uint8_t> clusters = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    auto fig_data = createFig018Data(0x2000, false, 0x00FF, clusters);
    auto announcement = parser_->parseFig018_AnnouncementSupport(fig_data);

    QVERIFY(announcement.is_valid);
    QCOMPARE(announcement.cluster_count, static_cast<uint8_t>(8));
    QCOMPARE(announcement.cluster_ids.size(), static_cast<size_t>(8));
}

void TestFig018Announcement::test_emergency_alarm_type() {
    qDebug() << "TEST: Emergency alarm announcement (Type 0)";

    auto fig_data = createFig018Data(0x3000, false, 0x0001, {0x60}); // Type 0 = Alarm
    auto announcement = parser_->parseFig018_AnnouncementSupport(fig_data);

    QVERIFY(announcement.is_valid);
    QVERIFY(announcement.supportsAnnouncementType(0));
    QVERIFY(announcement.isEmergencyCapable());
}

void TestFig018Announcement::test_traffic_flash_type() {
    qDebug() << "TEST: Traffic flash announcement (Type 1)";

    auto fig_data = createFig018Data(0x4000, false, 0x0002, {0x70}); // Type 1 = Traffic flash
    auto announcement = parser_->parseFig018_AnnouncementSupport(fig_data);

    QVERIFY(announcement.is_valid);
    QVERIFY(announcement.supportsAnnouncementType(1));
    QVERIFY(announcement.isEmergencyCapable());
}

void TestFig018Announcement::test_news_flash_type() {
    qDebug() << "TEST: News flash announcement (Type 3)";

    auto fig_data = createFig018Data(0x5000, false, 0x0008, {0x80}); // Type 3 = News flash
    auto announcement = parser_->parseFig018_AnnouncementSupport(fig_data);

    QVERIFY(announcement.is_valid);
    QVERIFY(announcement.supportsAnnouncementType(3));
    QVERIFY(announcement.isEmergencyCapable());
}

void TestFig018Announcement::test_multiple_announcement_types() {
    qDebug() << "TEST: Multiple announcement types";

    // Support types 0, 1, and 2 (bits 0, 1, 2)
    auto fig_data = createFig018Data(0x6000, false, 0x0007, {0x90});
    auto announcement = parser_->parseFig018_AnnouncementSupport(fig_data);

    QVERIFY(announcement.is_valid);
    QVERIFY(announcement.supportsAnnouncementType(0));
    QVERIFY(announcement.supportsAnnouncementType(1));
    QVERIFY(announcement.supportsAnnouncementType(2));
    QVERIFY(!announcement.supportsAnnouncementType(3));
}

void TestFig018Announcement::test_all_announcement_types() {
    qDebug() << "TEST: All announcement types";

    // Support all 16 types (bits 0-15)
    auto fig_data = createFig018Data(0x7000, false, 0xFFFF, {0xA0});
    auto announcement = parser_->parseFig018_AnnouncementSupport(fig_data);

    QVERIFY(announcement.is_valid);
    for (uint8_t type = 0; type < 16; ++type) {
        QVERIFY2(announcement.supportsAnnouncementType(type),
                 QString("Type %1 should be supported").arg(type).toUtf8());
    }
}

void TestFig018Announcement::test_supportsAnnouncementType() {
    qDebug() << "TEST: supportsAnnouncementType() method";

    auto fig_data = createFig018Data(0x1000, false, 0x0005, {0xB0}); // Types 0 and 2
    auto announcement = parser_->parseFig018_AnnouncementSupport(fig_data);

    QVERIFY(announcement.is_valid);
    QVERIFY(announcement.supportsAnnouncementType(0));
    QVERIFY(!announcement.supportsAnnouncementType(1));
    QVERIFY(announcement.supportsAnnouncementType(2));
    QVERIFY(!announcement.supportsAnnouncementType(3));
}

void TestFig018Announcement::test_getSupportedTypes() {
    qDebug() << "TEST: getSupportedTypes() method";

    auto fig_data = createFig018Data(0x2000, false, 0x000F, {0xC0}); // Types 0-3
    auto announcement = parser_->parseFig018_AnnouncementSupport(fig_data);

    QVERIFY(announcement.is_valid);
    auto types = announcement.getSupportedTypes();
    QCOMPARE(types.size(), static_cast<size_t>(4));
    QVERIFY(std::find(types.begin(), types.end(), 0) != types.end());
    QVERIFY(std::find(types.begin(), types.end(), 1) != types.end());
    QVERIFY(std::find(types.begin(), types.end(), 2) != types.end());
    QVERIFY(std::find(types.begin(), types.end(), 3) != types.end());
}

void TestFig018Announcement::test_isEmergencyCapable() {
    qDebug() << "TEST: isEmergencyCapable() method";

    // Type 0 (Alarm) - emergency
    auto ann1 = parser_->parseFig018_AnnouncementSupport(
        createFig018Data(0x1000, false, 0x0001, {0xD0}));
    QVERIFY(ann1.is_valid);
    QVERIFY(ann1.isEmergencyCapable());

    // Type 5 (Sport report) - NOT emergency
    auto ann2 = parser_->parseFig018_AnnouncementSupport(
        createFig018Data(0x2000, false, 0x0020, {0xE0}));
    QVERIFY(ann2.is_valid);
    QVERIFY(!ann2.isEmergencyCapable());

    // Types 0 and 5 - emergency (Type 0 is emergency)
    auto ann3 = parser_->parseFig018_AnnouncementSupport(
        createFig018Data(0x3000, false, 0x0021, {0xF0}));
    QVERIFY(ann3.is_valid);
    QVERIFY(ann3.isEmergencyCapable());
}

void TestFig018Announcement::test_invalid_empty_data() {
    qDebug() << "TEST: Invalid empty data";

    std::vector<uint8_t> empty_data;
    auto announcement = parser_->parseFig018_AnnouncementSupport(empty_data);

    QVERIFY(!announcement.is_valid);
}

void TestFig018Announcement::test_invalid_insufficient_data() {
    qDebug() << "TEST: Invalid insufficient data";

    // Only 5 bytes (need at least 6)
    std::vector<uint8_t> short_data = {18, 0x12, 0x34, 0x00, 0x01};
    auto announcement = parser_->parseFig018_AnnouncementSupport(short_data);

    QVERIFY(!announcement.is_valid);
}

void TestFig018Announcement::test_invalid_incomplete_cluster_ids() {
    qDebug() << "TEST: Invalid incomplete cluster IDs";

    // Claim 3 clusters but only provide 2
    std::vector<uint8_t> data = {18, 0x12, 0x34, 0x00, 0x01, 3, 0x10, 0x20};
    auto announcement = parser_->parseFig018_AnnouncementSupport(data);

    QVERIFY(!announcement.is_valid);
}

void TestFig018Announcement::test_max_clusters() {
    qDebug() << "TEST: Maximum clusters (255)";

    std::vector<uint8_t> max_clusters(255);
    for (int i = 0; i < 255; ++i) {
        max_clusters[i] = static_cast<uint8_t>(i);
    }

    auto fig_data = createFig018Data(0x1000, false, 0x0001, max_clusters);
    auto announcement = parser_->parseFig018_AnnouncementSupport(fig_data);

    QVERIFY(announcement.is_valid);
    QCOMPARE(announcement.cluster_count, static_cast<uint8_t>(255));
    QCOMPARE(announcement.cluster_ids.size(), static_cast<size_t>(255));
}

void TestFig018Announcement::test_parsing_performance() {
    qDebug() << "TEST: Parsing performance";

    auto fig_data = createFig018Data(0x5555, false, 0x00FF, {0x01, 0x02, 0x03});

    // Warm-up
    for (int i = 0; i < 100; ++i) {
        parser_->parseFig018_AnnouncementSupport(fig_data);
    }

    // Benchmark
    const int iterations = 10000;
    QElapsedTimer timer;
    timer.start();

    for (int i = 0; i < iterations; ++i) {
        auto result = parser_->parseFig018_AnnouncementSupport(fig_data);
        Q_UNUSED(result);
    }

    qint64 elapsed_ns = timer.nsecsElapsed();
    double avg_time_us = static_cast<double>(elapsed_ns) / iterations / 1000.0;

    qDebug() << QString("Average parsing time: %1 µs per FIG 0/18")
                .arg(avg_time_us, 0, 'f', 2);

    // Should be faster than 100 µs per parse
    QVERIFY2(avg_time_us < 100.0,
             QString("Parsing too slow: %1 µs").arg(avg_time_us).toUtf8());
}

QTEST_MAIN(TestFig018Announcement)
#include "test_fig018_announcement.moc"
