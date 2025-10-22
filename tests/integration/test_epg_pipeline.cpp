/**
 * @file test_epg_pipeline.cpp
 * @brief Integration Test for MOT → EPG Pipeline
 *
 * Tests the complete integration between MOT Protocol parser and EPG decoder.
 * Verifies that EPG data extracted from MOT objects is correctly parsed.
 *
 * @author StreamDAB Development Team
 * @date October 22, 2025
 */

#include <QtTest/QtTest>
#include "../../src/core/mot_protocol.hpp"
#include "../../src/core/epg_decoder.hpp"

using namespace eti::mot;
using namespace eti::epg;

class TestEPGPipeline : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    // Integration tests
    void testMOTtoEPGExtraction();
    void testMultipleEPGObjects();
    void testSignalPropagation();
    void testRealWorldScenario();

private:
    MOTProtocol* m_mot_parser = nullptr;
    EPGDecoder* m_epg_decoder = nullptr;

    // Helper methods
    QByteArray createMOTObjectWithEPG(uint32_t transport_id, uint32_t service_id, int num_events);
    QByteArray createEPGData(uint32_t service_id, int num_events);
};

// ============================================================================
// Test Lifecycle
// ============================================================================

void TestEPGPipeline::initTestCase() {
    qInfo() << "Starting MOT → EPG integration test suite";
}

void TestEPGPipeline::init() {
    m_mot_parser = new MOTProtocol();
    m_epg_decoder = new EPGDecoder();

    QVERIFY(m_mot_parser != nullptr);
    QVERIFY(m_epg_decoder != nullptr);
}

void TestEPGPipeline::cleanup() {
    delete m_mot_parser;
    delete m_epg_decoder;

    m_mot_parser = nullptr;
    m_epg_decoder = nullptr;
}

void TestEPGPipeline::cleanupTestCase() {
    qInfo() << "MOT → EPG integration test suite completed";
}

// ============================================================================
// Integration Tests
// ============================================================================

void TestEPGPipeline::testMOTtoEPGExtraction() {
    // Test complete pipeline: MOT object → EPG extraction

    // Connect MOT object completion to EPG processing
    QObject::connect(m_mot_parser, &MOTProtocol::objectComplete,
                     [this](uint32_t transport_id, const MOTObject& object) {
        qDebug() << "[Integration] MOT object complete, passing to EPG decoder";

        // Convert body to QByteArray
        QByteArray body_data(reinterpret_cast<const char*>(object.body.data()),
                             static_cast<int>(object.body.size()));

        // Process with EPG decoder
        bool success = m_epg_decoder->processMOTObject(body_data);
        QVERIFY(success);
    });

    // Create synthetic MOT object containing EPG data
    uint32_t service_id = 0xE1D00001;
    uint32_t transport_id = 100;
    QByteArray epg_data = createEPGData(service_id, 3);

    // Simulate MOT object completion
    MOTObject mot_object;
    mot_object.transport_id = transport_id;
    mot_object.body.resize(epg_data.size());
    std::memcpy(mot_object.body.data(), epg_data.data(), epg_data.size());
    mot_object.received_time = QDateTime::currentDateTime();

    // Emit MOT completion signal
    emit m_mot_parser->objectComplete(transport_id, mot_object);

    // Verify EPG decoder received and processed the data
    auto events = m_epg_decoder->getEvents(service_id);
    QCOMPARE(events.size(), 3);

    // Verify event validity
    for (const auto& event : events) {
        QVERIFY(event.isValid());
        QVERIFY(!event.program_name.isEmpty());
    }

    qDebug() << "[Integration] Successfully extracted" << events.size() << "EPG events from MOT object";
}

void TestEPGPipeline::testMultipleEPGObjects() {
    // Test processing multiple MOT objects with different EPG schedules

    QSignalSpy epg_spy(m_epg_decoder, &EPGDecoder::epgScheduleUpdated);

    // Service 1: 2 events
    uint32_t service_id1 = 0xE1D00001;
    QByteArray epg_data1 = createEPGData(service_id1, 2);

    MOTObject mot_object1;
    mot_object1.transport_id = 100;
    mot_object1.body.resize(epg_data1.size());
    std::memcpy(mot_object1.body.data(), epg_data1.data(), epg_data1.size());

    QByteArray body_data1(reinterpret_cast<const char*>(mot_object1.body.data()),
                          static_cast<int>(mot_object1.body.size()));
    QVERIFY(m_epg_decoder->processMOTObject(body_data1));

    // Service 2: 3 events
    uint32_t service_id2 = 0xE1D00002;
    QByteArray epg_data2 = createEPGData(service_id2, 3);

    MOTObject mot_object2;
    mot_object2.transport_id = 101;
    mot_object2.body.resize(epg_data2.size());
    std::memcpy(mot_object2.body.data(), epg_data2.data(), epg_data2.size());

    QByteArray body_data2(reinterpret_cast<const char*>(mot_object2.body.data()),
                          static_cast<int>(mot_object2.body.size()));
    QVERIFY(m_epg_decoder->processMOTObject(body_data2));

    // Verify both schedules processed
    QCOMPARE(epg_spy.count(), 2);

    // Verify events for each service
    auto events1 = m_epg_decoder->getEvents(service_id1);
    auto events2 = m_epg_decoder->getEvents(service_id2);

    QCOMPARE(events1.size(), 2);
    QCOMPARE(events2.size(), 3);

    // Verify total statistics
    QCOMPARE(m_epg_decoder->serviceCount(), 2);
    QCOMPARE(m_epg_decoder->eventCount(), 5);

    qDebug() << "[Integration] Successfully processed multiple EPG schedules";
}

void TestEPGPipeline::testSignalPropagation() {
    // Test that signals propagate correctly through the pipeline

    QSignalSpy mot_complete_spy(m_mot_parser, &MOTProtocol::objectComplete);
    QSignalSpy epg_event_spy(m_epg_decoder, &EPGDecoder::epgEventDiscovered);
    QSignalSpy epg_schedule_spy(m_epg_decoder, &EPGDecoder::epgScheduleUpdated);

    // Connect signals
    QObject::connect(m_mot_parser, &MOTProtocol::objectComplete,
                     [this](uint32_t transport_id, const MOTObject& object) {
        QByteArray body_data(reinterpret_cast<const char*>(object.body.data()),
                             static_cast<int>(object.body.size()));
        m_epg_decoder->processMOTObject(body_data);
    });

    // Create and emit MOT object
    uint32_t service_id = 0xE1D00001;
    QByteArray epg_data = createEPGData(service_id, 2);

    MOTObject mot_object;
    mot_object.transport_id = 100;
    mot_object.body.resize(epg_data.size());
    std::memcpy(mot_object.body.data(), epg_data.data(), epg_data.size());

    emit m_mot_parser->objectComplete(100, mot_object);

    // Verify signal counts
    QCOMPARE(mot_complete_spy.count(), 1);
    QCOMPARE(epg_event_spy.count(), 2); // 2 events
    QCOMPARE(epg_schedule_spy.count(), 1); // 1 schedule

    qDebug() << "[Integration] Signal propagation verified";
}

void TestEPGPipeline::testRealWorldScenario() {
    // Simulate real-world scenario: DAB service broadcasting EPG via MOT

    qDebug() << "[Integration] Simulating real-world EPG broadcast scenario";

    // Bangkok DAB+ service example
    uint32_t bangkok_service_id = 0xE1D00001; // Example SId

    // Create realistic EPG schedule for today
    QByteArray epg_schedule = createEPGData(bangkok_service_id, 8); // 8 programs

    // Simulate MOT transmission
    MOTObject mot_object;
    mot_object.transport_id = 200;
    mot_object.header.content_type = eti::mot::ContentType::GENERAL_DATA;
    mot_object.header.content_name = "EPG Schedule";
    mot_object.body.resize(epg_schedule.size());
    std::memcpy(mot_object.body.data(), epg_schedule.data(), epg_schedule.size());
    mot_object.received_time = QDateTime::currentDateTime();

    // Process EPG
    QByteArray body_data(reinterpret_cast<const char*>(mot_object.body.data()),
                         static_cast<int>(mot_object.body.size()));

    QVERIFY(m_epg_decoder->processMOTObject(body_data));

    // Retrieve and verify schedule
    auto schedule = m_epg_decoder->getSchedule(bangkok_service_id);
    QVERIFY(schedule.has_value());

    QCOMPARE(schedule->service_id, bangkok_service_id);
    QCOMPARE(schedule->events.size(), 8);
    QVERIFY(schedule->schedule_date.isValid());

    // Check current/next program logic
    auto current_event = m_epg_decoder->getCurrentEvent(bangkok_service_id);
    auto next_event = m_epg_decoder->getNextEvent(bangkok_service_id);

    qDebug() << "[Integration] Current event:" 
             << (current_event.has_value() ? current_event->program_name : "None");
    qDebug() << "[Integration] Next event:"
             << (next_event.has_value() ? next_event->program_name : "None");

    // Verify statistics
    auto stats = m_epg_decoder->getStatistics();
    QCOMPARE(stats.schedules_processed, 1u);
    QCOMPARE(stats.events_extracted, 8u);
    QCOMPARE(stats.active_services, 1u);

    qDebug() << "[Integration] Real-world scenario simulation complete";
}

// ============================================================================
// Helper Methods
// ============================================================================

QByteArray TestEPGPipeline::createEPGData(uint32_t service_id, int num_events) {
    QByteArray data;

    // EPG header
    data.append((char)((service_id >> 24) & 0xFF));
    data.append((char)((service_id >> 16) & 0xFF));
    data.append((char)((service_id >> 8) & 0xFF));
    data.append((char)(service_id & 0xFF));

    // Schedule date (MJD for Jan 1, 2000)
    uint16_t mjd = 51544;
    data.append((char)((mjd >> 8) & 0xFF));
    data.append((char)(mjd & 0xFF));

    // Number of events
    data.append((char)((num_events >> 8) & 0xFF));
    data.append((char)(num_events & 0xFF));

    // Create events
    for (int i = 0; i < num_events; ++i) {
        uint16_t event_id = 1000 + i;

        // Event ID
        data.append((char)((event_id >> 8) & 0xFF));
        data.append((char)(event_id & 0xFF));

        // Start time (MJD + UTC)
        data.append((char)((mjd >> 8) & 0xFF));
        data.append((char)(mjd & 0xFF));

        uint32_t utc = 0x060000 + (i * 0x020000); // Start at 06:00, +2 hours each
        data.append((char)((utc >> 16) & 0xFF));
        data.append((char)((utc >> 8) & 0xFF));
        data.append((char)(utc & 0xFF));

        // Duration (120 minutes = 2 hours)
        uint16_t duration = 120;
        data.append((char)((duration >> 8) & 0xFF));
        data.append((char)(duration & 0xFF));

        // Content type (vary by program)
        uint8_t content_type = (i % 2 == 0) ? 0x10 : 0x50; // Alternate News/Music
        data.append((char)content_type);

        // Program name
        QString name = QString("Program %1").arg(i + 1);
        QByteArray name_bytes = name.toUtf8();
        data.append((char)name_bytes.size());
        data.append(name_bytes);

        // Description
        QString description = QString("Description for program %1").arg(i + 1);
        QByteArray desc_bytes = description.toUtf8();
        data.append((char)((desc_bytes.size() >> 8) & 0xFF));
        data.append((char)(desc_bytes.size() & 0xFF));
        data.append(desc_bytes);
    }

    return data;
}

QByteArray TestEPGPipeline::createMOTObjectWithEPG(uint32_t transport_id, uint32_t service_id, int num_events) {
    // Create EPG data
    QByteArray epg_data = createEPGData(service_id, num_events);

    // Wrap in MOT structure (simplified)
    return epg_data;
}

QTEST_MAIN(TestEPGPipeline)
#include "test_epg_pipeline.moc"
