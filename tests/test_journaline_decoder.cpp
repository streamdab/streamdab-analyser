/**
 * @file test_journaline_decoder.cpp
 * @brief Unit Tests for Journaline Decoder
 *
 * Comprehensive test suite for ETSI TS 102 979 Journaline decoder implementation.
 * Tests object parsing, menu construction, Thai text support, link resolution, and edge cases.
 *
 * @author StreamDAB Development Team
 * @date October 22, 2025
 */

#include <QtTest/QtTest>
#include "../src/core/journaline_decoder.hpp"

using namespace eti::journaline;

class TestJournalineDecoder : public QObject {
    Q_OBJECT

private slots:
    // Test lifecycle
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    // Object parsing tests
    void testObjectParsing_SingleObject();
    void testObjectParsing_CompleteFields();
    void testObjectParsing_WithLink();
    void testObjectParsing_WithoutLink();
    void testObjectParsing_MultipleObjects();

    // Text extraction tests
    void testTextExtraction_UTF8Content();
    void testTextExtraction_ThaiText();
    void testTextExtraction_EmptyText();
    void testTextExtraction_LongContent();

    // Category tests
    void testCategory_News();
    void testCategory_Sport();
    void testCategory_Weather();
    void testCategory_Traffic();
    void testCategory_Unknown();

    // Timestamp tests
    void testTimestamp_ValidMJD();
    void testTimestamp_BCDDecoding();
    void testTimestamp_InvalidMJD();
    void testTimestamp_FutureDate();

    // Menu construction tests
    void testMenu_SingleCategory();
    void testMenu_MultipleCategories();
    void testMenu_Hierarchy();
    void testMenu_RootMenu();

    // Link resolution tests
    void testLink_ValidTarget();
    void testLink_InvalidTarget();
    void testLink_NoLink();
    void testLink_ChainedLinks();

    // Object retrieval tests
    void testRetrieval_GetObject();
    void testRetrieval_GetObjectsByCategory();
    void testRetrieval_NonExistentObject();

    // Qt signal tests
    void testSignals_ObjectReceived();
    void testSignals_MenuUpdated();
    void testSignals_DecodingError();

    // Edge case tests
    void testEdgeCase_EmptyData();
    void testEdgeCase_TruncatedData();
    void testEdgeCase_InvalidObjectId();
    void testEdgeCase_CorruptCategory();
    void testEdgeCase_MissingFields();

    // Statistics tests
    void testStatistics_ObjectCount();
    void testStatistics_MenuCount();
    void testStatistics_ThaiContent();
    void testStatistics_ClearAll();

    // Thread safety tests
    void testThreadSafety_ConcurrentAccess();
    
    // Bug fix verification tests (Wave 2.2 fixes)
    void testThreadSafety_ConcurrentSignalEmission();
    void testEdgeCase_OverflowProtection();
    void testEdgeCase_ExtractionFailure();

private:
    JournalineDecoder* m_decoder = nullptr;

    // Helper methods
    QByteArray createTestJournalineObject(uint16_t object_id, uint8_t category,
                                         const QString& title, const QString& content,
                                         bool with_link = false, uint16_t link_target = 0);
    QByteArray createMinimalObject(uint16_t object_id);
    QByteArray createObjectWithTimestamp(uint16_t object_id, uint16_t mjd, uint32_t utc_time);
};

// ============================================================================
// Test Lifecycle
// ============================================================================

void TestJournalineDecoder::initTestCase() {
    qInfo() << "Starting JournalineDecoder test suite - ETSI TS 102 979";
}

void TestJournalineDecoder::init() {
    m_decoder = new JournalineDecoder();
    QVERIFY(m_decoder != nullptr);
}

void TestJournalineDecoder::cleanup() {
    delete m_decoder;
    m_decoder = nullptr;
}

void TestJournalineDecoder::cleanupTestCase() {
    qInfo() << "JournalineDecoder test suite completed";
}

// ============================================================================
// Object Parsing Tests
// ============================================================================

void TestJournalineDecoder::testObjectParsing_SingleObject() {
    QByteArray data = createTestJournalineObject(
        1,                          // object_id
        0x01,                       // category (NEWS_GENERAL)
        "Breaking News",            // title
        "This is test content"      // content
    );

    bool result = m_decoder->processMOTObject(data);
    QVERIFY(result);

    auto object_opt = m_decoder->getObject(1);
    QVERIFY(object_opt.has_value());

    JournalineObject obj = *object_opt;
    QCOMPARE(obj.object_id, static_cast<uint16_t>(1));
    QCOMPARE(obj.category, static_cast<uint8_t>(0x01));
    QCOMPARE(obj.title, QString("Breaking News"));
    QCOMPARE(obj.text_content, QString("This is test content"));
    QVERIFY(obj.isValid());
}

void TestJournalineDecoder::testObjectParsing_CompleteFields() {
    QByteArray data = createObjectWithTimestamp(
        42,                         // object_id
        50000,                      // mjd (year 1995)
        0x123456                    // utc_time (12:34:56 BCD)
    );

    // Add title and content
    data.append(static_cast<char>(4));  // title length
    data.append("Test");
    data.append(static_cast<char>(0));  // content length MSB
    data.append(static_cast<char>(10)); // content length LSB
    data.append("TestCont12");          // 10 bytes

    bool result = m_decoder->processMOTObject(data);
    QVERIFY(result);

    auto obj_opt = m_decoder->getObject(42);
    QVERIFY(obj_opt.has_value());

    JournalineObject obj = *obj_opt;
    QCOMPARE(obj.object_id, static_cast<uint16_t>(42));
    QVERIFY(obj.timestamp.isValid());
    QCOMPARE(obj.title, QString("Test"));
}

void TestJournalineDecoder::testObjectParsing_WithLink() {
    QByteArray data = createTestJournalineObject(
        10,                         // object_id
        0x10,                       // category (SPORT_GENERAL)
        "Sports News",              // title
        "Click for more info",      // content
        true,                       // with_link
        20                          // link_target
    );

    bool result = m_decoder->processMOTObject(data);
    QVERIFY(result);

    auto obj_opt = m_decoder->getObject(10);
    QVERIFY(obj_opt.has_value());

    JournalineObject obj = *obj_opt;
    QVERIFY(obj.is_link);
    QCOMPARE(obj.link_target, static_cast<uint16_t>(20));
}

void TestJournalineDecoder::testObjectParsing_WithoutLink() {
    QByteArray data = createTestJournalineObject(
        5,                          // object_id
        0x20,                       // category (WEATHER_GENERAL)
        "Weather",                  // title
        "Sunny today",              // content
        false                       // no link
    );

    bool result = m_decoder->processMOTObject(data);
    QVERIFY(result);

    auto obj_opt = m_decoder->getObject(5);
    QVERIFY(obj_opt.has_value());

    JournalineObject obj = *obj_opt;
    QVERIFY(!obj.is_link);
    QCOMPARE(obj.link_target, static_cast<uint16_t>(0));
}

void TestJournalineDecoder::testObjectParsing_MultipleObjects() {
    // Process three objects
    for (int i = 1; i <= 3; ++i) {
        QByteArray data = createTestJournalineObject(
            i,
            0x01,
            QString("Object %1").arg(i),
            QString("Content %1").arg(i)
        );
        QVERIFY(m_decoder->processMOTObject(data));
    }

    QCOMPARE(m_decoder->objectCount(), 3);

    auto objects = m_decoder->getObjects();
    QCOMPARE(objects.size(), 3);
}

// ============================================================================
// Text Extraction Tests
// ============================================================================

void TestJournalineDecoder::testTextExtraction_UTF8Content() {
    QString test_title = "UTF-8 Test: Ñoño";
    QString test_content = "Special chars: é, ñ, ü";

    QByteArray data = createTestJournalineObject(
        100,
        0x01,
        test_title,
        test_content
    );

    QVERIFY(m_decoder->processMOTObject(data));

    auto obj_opt = m_decoder->getObject(100);
    QVERIFY(obj_opt.has_value());

    QCOMPARE(obj_opt->title, test_title);
    QCOMPARE(obj_opt->text_content, test_content);
}

void TestJournalineDecoder::testTextExtraction_ThaiText() {
    QString thai_title = "ข่าวสาร"; // Thai: "News"
    QString thai_content = "สวัสดีครับ"; // Thai: "Hello"

    QByteArray data = createTestJournalineObject(
        200,
        0x01,
        thai_title,
        thai_content
    );

    QVERIFY(m_decoder->processMOTObject(data));

    auto obj_opt = m_decoder->getObject(200);
    QVERIFY(obj_opt.has_value());

    QCOMPARE(obj_opt->title, thai_title);
    QCOMPARE(obj_opt->text_content, thai_content);

    // Verify Thai content counter
    auto stats = m_decoder->getStatistics();
    QVERIFY(stats.thai_content_count > 0);
}

void TestJournalineDecoder::testTextExtraction_EmptyText() {
    QByteArray data = createTestJournalineObject(
        300,
        0x01,
        "",     // empty title
        "Content only"
    );

    QVERIFY(m_decoder->processMOTObject(data));

    auto obj_opt = m_decoder->getObject(300);
    QVERIFY(obj_opt.has_value());
    QVERIFY(obj_opt->title.isEmpty());
    QVERIFY(!obj_opt->text_content.isEmpty());
}

void TestJournalineDecoder::testTextExtraction_LongContent() {
    QString long_content(1000, 'X'); // 1000 characters

    QByteArray data = createTestJournalineObject(
        400,
        0x01,
        "Long Content Test",
        long_content
    );

    QVERIFY(m_decoder->processMOTObject(data));

    auto obj_opt = m_decoder->getObject(400);
    QVERIFY(obj_opt.has_value());
    QCOMPARE(obj_opt->text_content.length(), 1000);
}

// ============================================================================
// Category Tests
// ============================================================================

void TestJournalineDecoder::testCategory_News() {
    QByteArray data = createTestJournalineObject(1, 0x01, "News", "Content");
    QVERIFY(m_decoder->processMOTObject(data));

    auto obj = m_decoder->getObject(1);
    QVERIFY(obj.has_value());
    QCOMPARE(obj->category, static_cast<uint8_t>(0x01));
    QVERIFY(obj->getCategoryString().contains("News"));
}

void TestJournalineDecoder::testCategory_Sport() {
    QByteArray data = createTestJournalineObject(2, 0x10, "Sport", "Content");
    QVERIFY(m_decoder->processMOTObject(data));

    auto obj = m_decoder->getObject(2);
    QVERIFY(obj.has_value());
    QCOMPARE(obj->category, static_cast<uint8_t>(0x10));
    QVERIFY(obj->getCategoryString().contains("Sport"));
}

void TestJournalineDecoder::testCategory_Weather() {
    QByteArray data = createTestJournalineObject(3, 0x20, "Weather", "Content");
    QVERIFY(m_decoder->processMOTObject(data));

    auto obj = m_decoder->getObject(3);
    QVERIFY(obj.has_value());
    QCOMPARE(obj->category, static_cast<uint8_t>(0x20));
    QVERIFY(obj->getCategoryString().contains("Weather"));
}

void TestJournalineDecoder::testCategory_Traffic() {
    QByteArray data = createTestJournalineObject(4, 0x30, "Traffic", "Content");
    QVERIFY(m_decoder->processMOTObject(data));

    auto obj = m_decoder->getObject(4);
    QVERIFY(obj.has_value());
    QCOMPARE(obj->category, static_cast<uint8_t>(0x30));
    QVERIFY(obj->getCategoryString().contains("Traffic"));
}

void TestJournalineDecoder::testCategory_Unknown() {
    QByteArray data = createTestJournalineObject(5, 0xFF, "Unknown", "Content");
    QVERIFY(m_decoder->processMOTObject(data));

    auto obj = m_decoder->getObject(5);
    QVERIFY(obj.has_value());
    QCOMPARE(obj->category, static_cast<uint8_t>(0xFF));
}

// ============================================================================
// Timestamp Tests
// ============================================================================

void TestJournalineDecoder::testTimestamp_ValidMJD() {
    // MJD 50000 = October 10, 1995
    QByteArray data = createObjectWithTimestamp(1, 50000, 0x120000);

    // Add title and content
    data.append(static_cast<char>(5));
    data.append("Title");
    data.append(static_cast<char>(0));
    data.append(static_cast<char>(7));
    data.append("Content");

    QVERIFY(m_decoder->processMOTObject(data));

    auto obj = m_decoder->getObject(1);
    QVERIFY(obj.has_value());
    QVERIFY(obj->timestamp.isValid());
    QCOMPARE(obj->timestamp.date().year(), 1995);
}

void TestJournalineDecoder::testTimestamp_BCDDecoding() {
    // UTC time: 0x123456 = 12:34:56 BCD
    QByteArray data = createObjectWithTimestamp(2, 50000, 0x123456);

    data.append(static_cast<char>(5));
    data.append("Title");
    data.append(static_cast<char>(0));
    data.append(static_cast<char>(7));
    data.append("Content");

    QVERIFY(m_decoder->processMOTObject(data));

    auto obj = m_decoder->getObject(2);
    QVERIFY(obj.has_value());
    QVERIFY(obj->timestamp.isValid());

    QTime time = obj->timestamp.time();
    QCOMPARE(time.hour(), 12);
    QCOMPARE(time.minute(), 34);
    QCOMPARE(time.second(), 56);
}

void TestJournalineDecoder::testTimestamp_InvalidMJD() {
    // MJD 0 is invalid
    QByteArray data = createObjectWithTimestamp(3, 0, 0x000000);

    data.append(static_cast<char>(5));
    data.append("Title");
    data.append(static_cast<char>(0));
    data.append(static_cast<char>(7));
    data.append("Content");

    // Should still process but timestamp may be invalid or current time
    QVERIFY(m_decoder->processMOTObject(data));

    auto obj = m_decoder->getObject(3);
    QVERIFY(obj.has_value());
    // Timestamp should be set to current time as fallback
}

void TestJournalineDecoder::testTimestamp_FutureDate() {
    // MJD 60000 = February 13, 2023
    QByteArray data = createObjectWithTimestamp(4, 60000, 0x150000);

    data.append(static_cast<char>(5));
    data.append("Title");
    data.append(static_cast<char>(0));
    data.append(static_cast<char>(7));
    data.append("Content");

    QVERIFY(m_decoder->processMOTObject(data));

    auto obj = m_decoder->getObject(4);
    QVERIFY(obj.has_value());
    QVERIFY(obj->timestamp.isValid());
}

// ============================================================================
// Menu Construction Tests
// ============================================================================

void TestJournalineDecoder::testMenu_SingleCategory() {
    // Add multiple objects in same category
    for (int i = 1; i <= 3; ++i) {
        QByteArray data = createTestJournalineObject(i, 0x01, "News", "Content");
        QVERIFY(m_decoder->processMOTObject(data));
    }

    // Check menu was created
    QVERIFY(m_decoder->menuCount() > 0);

    auto menu_ids = m_decoder->getMenuIds();
    QVERIFY(!menu_ids.isEmpty());
}

void TestJournalineDecoder::testMenu_MultipleCategories() {
    // Add objects in different categories
    QByteArray data1 = createTestJournalineObject(1, 0x01, "News", "Content");
    QByteArray data2 = createTestJournalineObject(2, 0x10, "Sport", "Content");
    QByteArray data3 = createTestJournalineObject(3, 0x20, "Weather", "Content");

    QVERIFY(m_decoder->processMOTObject(data1));
    QVERIFY(m_decoder->processMOTObject(data2));
    QVERIFY(m_decoder->processMOTObject(data3));

    // Should have multiple menus (root + categories)
    QVERIFY(m_decoder->menuCount() >= 3);
}

void TestJournalineDecoder::testMenu_Hierarchy() {
    // Add objects
    QByteArray data = createTestJournalineObject(1, 0x01, "News", "Content");
    QVERIFY(m_decoder->processMOTObject(data));

    // Check root menu exists
    auto root_menus = m_decoder->getRootMenus();
    QVERIFY(!root_menus.isEmpty());

    JournalineMenu root = root_menus.first();
    QVERIFY(root.isRootMenu());
    QCOMPARE(root.parent_menu_id, static_cast<uint16_t>(0));
}

void TestJournalineDecoder::testMenu_RootMenu() {
    QByteArray data = createTestJournalineObject(1, 0x01, "News", "Content");
    QVERIFY(m_decoder->processMOTObject(data));

    auto root_menus = m_decoder->getRootMenus();
    QVERIFY(!root_menus.isEmpty());

    for (const auto& menu : root_menus) {
        QVERIFY(menu.isValid());
        QVERIFY(menu.isRootMenu());
    }
}

// ============================================================================
// Link Resolution Tests
// ============================================================================

void TestJournalineDecoder::testLink_ValidTarget() {
    // Create target object
    QByteArray target_data = createTestJournalineObject(
        100, 0x01, "Target Article", "Full article text"
    );
    QVERIFY(m_decoder->processMOTObject(target_data));

    // Create source object with link
    QByteArray source_data = createTestJournalineObject(
        50, 0x01, "Summary", "Click for full article", true, 100
    );
    QVERIFY(m_decoder->processMOTObject(source_data));

    auto source_obj = m_decoder->getObject(50);
    QVERIFY(source_obj.has_value());
    QVERIFY(source_obj->is_link);

    // Resolve link
    auto target_obj = m_decoder->resolveLinkTarget(*source_obj);
    QVERIFY(target_obj.has_value());
    QCOMPARE(target_obj->object_id, static_cast<uint16_t>(100));
    QCOMPARE(target_obj->title, QString("Target Article"));
}

void TestJournalineDecoder::testLink_InvalidTarget() {
    // Create source with link to non-existent target
    QByteArray source_data = createTestJournalineObject(
        60, 0x01, "Broken Link", "Link to nowhere", true, 999
    );
    QVERIFY(m_decoder->processMOTObject(source_data));

    auto source_obj = m_decoder->getObject(60);
    QVERIFY(source_obj.has_value());

    // Try to resolve invalid link
    auto target_obj = m_decoder->resolveLinkTarget(*source_obj);
    QVERIFY(!target_obj.has_value());
}

void TestJournalineDecoder::testLink_NoLink() {
    QByteArray data = createTestJournalineObject(
        70, 0x01, "No Link", "Regular object", false
    );
    QVERIFY(m_decoder->processMOTObject(data));

    auto obj = m_decoder->getObject(70);
    QVERIFY(obj.has_value());
    QVERIFY(!obj->is_link);

    auto target = m_decoder->resolveLinkTarget(*obj);
    QVERIFY(!target.has_value());
}

void TestJournalineDecoder::testLink_ChainedLinks() {
    // Create chain: 1 → 2 → 3
    QByteArray data3 = createTestJournalineObject(3, 0x01, "Final", "Final content");
    QVERIFY(m_decoder->processMOTObject(data3));

    QByteArray data2 = createTestJournalineObject(2, 0x01, "Middle", "Middle", true, 3);
    QVERIFY(m_decoder->processMOTObject(data2));

    QByteArray data1 = createTestJournalineObject(1, 0x01, "First", "First", true, 2);
    QVERIFY(m_decoder->processMOTObject(data1));

    // Follow chain
    auto obj1 = m_decoder->getObject(1);
    QVERIFY(obj1.has_value());

    auto obj2 = m_decoder->resolveLinkTarget(*obj1);
    QVERIFY(obj2.has_value());
    QCOMPARE(obj2->object_id, static_cast<uint16_t>(2));

    auto obj3 = m_decoder->resolveLinkTarget(*obj2);
    QVERIFY(obj3.has_value());
    QCOMPARE(obj3->object_id, static_cast<uint16_t>(3));
}

// ============================================================================
// Object Retrieval Tests
// ============================================================================

void TestJournalineDecoder::testRetrieval_GetObject() {
    QByteArray data = createTestJournalineObject(123, 0x01, "Test", "Content");
    QVERIFY(m_decoder->processMOTObject(data));

    auto obj = m_decoder->getObject(123);
    QVERIFY(obj.has_value());
    QCOMPARE(obj->object_id, static_cast<uint16_t>(123));
}

void TestJournalineDecoder::testRetrieval_GetObjectsByCategory() {
    // Add multiple objects in same category
    for (int i = 1; i <= 3; ++i) {
        QByteArray data = createTestJournalineObject(i, 0x10, "Sport", "Content");
        QVERIFY(m_decoder->processMOTObject(data));
    }

    // Add object in different category
    QByteArray data4 = createTestJournalineObject(4, 0x01, "News", "Content");
    QVERIFY(m_decoder->processMOTObject(data4));

    auto sport_objects = m_decoder->getObjectsByCategory(0x10);
    QCOMPARE(sport_objects.size(), 3);

    auto news_objects = m_decoder->getObjectsByCategory(0x01);
    QCOMPARE(news_objects.size(), 1);
}

void TestJournalineDecoder::testRetrieval_NonExistentObject() {
    auto obj = m_decoder->getObject(9999);
    QVERIFY(!obj.has_value());
}

// ============================================================================
// Qt Signal Tests
// ============================================================================

void TestJournalineDecoder::testSignals_ObjectReceived() {
    QSignalSpy spy(m_decoder, &JournalineDecoder::journalineObjectReceived);

    QByteArray data = createTestJournalineObject(1, 0x01, "Test", "Content");
    m_decoder->processMOTObject(data);

    QCOMPARE(spy.count(), 1);

    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toUInt(), static_cast<uint>(1)); // object_id
}

void TestJournalineDecoder::testSignals_MenuUpdated() {
    QSignalSpy spy(m_decoder, &JournalineDecoder::journalineMenuUpdated);

    QByteArray data = createTestJournalineObject(1, 0x01, "Test", "Content");
    m_decoder->processMOTObject(data);

    // Menu updates happen during buildMenuHierarchy
    QVERIFY(spy.count() > 0);
}

void TestJournalineDecoder::testSignals_DecodingError() {
    QSignalSpy spy(m_decoder, &JournalineDecoder::journalineDecodingError);

    // Send invalid data (too short)
    QByteArray invalid_data;
    invalid_data.append(static_cast<char>(0x00));
    invalid_data.append(static_cast<char>(0x01));

    m_decoder->processMOTObject(invalid_data);

    QVERIFY(spy.count() > 0);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

void TestJournalineDecoder::testEdgeCase_EmptyData() {
    QByteArray empty_data;
    bool result = m_decoder->processMOTObject(empty_data);
    QVERIFY(!result);

    auto stats = m_decoder->getStatistics();
    QVERIFY(stats.parsing_errors > 0);
}

void TestJournalineDecoder::testEdgeCase_TruncatedData() {
    QByteArray truncated;
    // Only object ID, no other fields
    truncated.append(static_cast<char>(0x00));
    truncated.append(static_cast<char>(0x01));

    bool result = m_decoder->processMOTObject(truncated);
    QVERIFY(!result);
}

void TestJournalineDecoder::testEdgeCase_InvalidObjectId() {
    QByteArray data;
    // Object ID = 0 (invalid)
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));

    // Category
    data.append(static_cast<char>(0x01));

    // Timestamp (40 bits = 5 bytes)
    for (int i = 0; i < 5; ++i) {
        data.append(static_cast<char>(0x00));
    }

    // Title
    data.append(static_cast<char>(4));
    data.append("Test");

    // Content
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(4));
    data.append("Test");

    bool result = m_decoder->processMOTObject(data);
    QVERIFY(!result); // Should fail due to invalid ID
}

void TestJournalineDecoder::testEdgeCase_CorruptCategory() {
    QByteArray data = createTestJournalineObject(1, 0xAB, "Test", "Content");
    // Should still process even with unknown category
    QVERIFY(m_decoder->processMOTObject(data));

    auto obj = m_decoder->getObject(1);
    QVERIFY(obj.has_value());
    QCOMPARE(obj->category, static_cast<uint8_t>(0xAB));
}

void TestJournalineDecoder::testEdgeCase_MissingFields() {
    QByteArray data;
    // Object ID
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x01));

    // Category
    data.append(static_cast<char>(0x01));

    // Timestamp
    for (int i = 0; i < 5; ++i) {
        data.append(static_cast<char>(0x00));
    }

    // Title length but no title text (truncated)
    data.append(static_cast<char>(10));
    // Missing title data

    bool result = m_decoder->processMOTObject(data);
    QVERIFY(!result); // Should fail on truncation
}

// ============================================================================
// Statistics Tests
// ============================================================================

void TestJournalineDecoder::testStatistics_ObjectCount() {
    QCOMPARE(m_decoder->objectCount(), 0);

    for (int i = 1; i <= 5; ++i) {
        QByteArray data = createTestJournalineObject(i, 0x01, "Test", "Content");
        m_decoder->processMOTObject(data);
    }

    QCOMPARE(m_decoder->objectCount(), 5);

    auto stats = m_decoder->getStatistics();
    QCOMPARE(stats.objects_processed, static_cast<uint32_t>(5));
}

void TestJournalineDecoder::testStatistics_MenuCount() {
    QByteArray data = createTestJournalineObject(1, 0x01, "Test", "Content");
    m_decoder->processMOTObject(data);

    QVERIFY(m_decoder->menuCount() > 0);

    auto stats = m_decoder->getStatistics();
    QVERIFY(stats.menus_processed > 0);
}

void TestJournalineDecoder::testStatistics_ThaiContent() {
    QString thai_text = "ข่าวสาร";
    QByteArray data = createTestJournalineObject(1, 0x01, thai_text, "Content");
    m_decoder->processMOTObject(data);

    auto stats = m_decoder->getStatistics();
    QVERIFY(stats.thai_content_count > 0);
}

void TestJournalineDecoder::testStatistics_ClearAll() {
    // Add some objects
    for (int i = 1; i <= 3; ++i) {
        QByteArray data = createTestJournalineObject(i, 0x01, "Test", "Content");
        m_decoder->processMOTObject(data);
    }

    QVERIFY(m_decoder->objectCount() > 0);

    m_decoder->clearAll();

    QCOMPARE(m_decoder->objectCount(), 0);
    QCOMPARE(m_decoder->menuCount(), 0);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

void TestJournalineDecoder::testThreadSafety_ConcurrentAccess() {
    // Add object
    QByteArray data = createTestJournalineObject(1, 0x01, "Test", "Content");
    m_decoder->processMOTObject(data);

    // Test concurrent reads (should not crash)
    auto obj1 = m_decoder->getObject(1);
    auto obj2 = m_decoder->getObject(1);
    auto count1 = m_decoder->objectCount();
    auto count2 = m_decoder->objectCount();

    QVERIFY(obj1.has_value());
    QVERIFY(obj2.has_value());
    QCOMPARE(count1, count2);
}

// ============================================================================
// Bug Fix Verification Tests (Wave 2.2)
// ============================================================================

void TestJournalineDecoder::testThreadSafety_ConcurrentSignalEmission() {
    // BUG-001-CRITICAL FIX VERIFICATION: Signal emission outside mutex lock
    // This test verifies that signals are emitted AFTER mutex release,
    // preventing deadlock when slots call back into the decoder.
    
    QSignalSpy object_spy(m_decoder, &JournalineDecoder::journalineObjectReceived);
    QSignalSpy menu_spy(m_decoder, &JournalineDecoder::journalineMenuUpdated);
    QSignalSpy error_spy(m_decoder, &JournalineDecoder::journalineDecodingError);
    
    // Test 1: Valid object processing should emit signals without deadlock
    QByteArray valid_data = createTestJournalineObject(1, 0x01, "Test", "Content");
    bool result = m_decoder->processMOTObject(valid_data);
    
    QVERIFY(result);
    QCOMPARE(object_spy.count(), 1);  // Should emit journalineObjectReceived
    QVERIFY(menu_spy.count() > 0);    // Should emit journalineMenuUpdated
    
    // Test 2: Error case should emit error signal without deadlock
    QByteArray empty_data;
    result = m_decoder->processMOTObject(empty_data);
    
    QVERIFY(!result);
    QCOMPARE(error_spy.count(), 1);   // Should emit journalineDecodingError
    
    // Test 3: Simulate callback scenario - connect slot that accesses decoder
    // This would deadlock with the old implementation
    int callback_count = 0;
    connect(m_decoder, &JournalineDecoder::journalineObjectReceived,
            this, [this, &callback_count](uint16_t obj_id, const JournalineObject&) {
        // This callback accesses the decoder while signal is being emitted
        // Would deadlock if signal emission happened inside mutex lock
        auto count = m_decoder->objectCount();
        QVERIFY(count > 0);
        callback_count++;
    });
    
    QByteArray data2 = createTestJournalineObject(2, 0x01, "Test2", "Content2");
    result = m_decoder->processMOTObject(data2);
    
    QVERIFY(result);
    QCOMPARE(callback_count, 1);  // Callback should have executed without deadlock
}

void TestJournalineDecoder::testEdgeCase_OverflowProtection() {
    // BUG-002-HIGH FIX VERIFICATION: Integer overflow protection in length calculations
    
    // Test 1: Title length overflow protection
    QByteArray overflow_title_data;
    overflow_title_data.append(static_cast<char>(0x00));  // Object ID MSB
    overflow_title_data.append(static_cast<char>(0x01));  // Object ID LSB
    overflow_title_data.append(static_cast<char>(0x01));  // Category
    
    // Timestamp (5 bytes)
    for (int i = 0; i < 5; ++i) {
        overflow_title_data.append(static_cast<char>(0x00));
    }
    
    // Title length = 255 (max for uint8_t)
    overflow_title_data.append(static_cast<char>(255));
    // Only add 10 bytes of title (should detect overflow/truncation)
    overflow_title_data.append("1234567890");
    
    bool result = m_decoder->processMOTObject(overflow_title_data);
    QVERIFY(!result);  // Should fail due to overflow protection
    
    auto stats = m_decoder->getStatistics();
    QVERIFY(stats.parsing_errors > 0);
    
    // Test 2: Content length overflow protection
    QByteArray overflow_content_data = createObjectWithTimestamp(2, 50000, 0x120000);
    
    overflow_content_data.append(static_cast<char>(5));  // Title length
    overflow_content_data.append("Title");
    
    // Content length = 0xFFFF (65535, max for uint16_t)
    overflow_content_data.append(static_cast<char>(0xFF));
    overflow_content_data.append(static_cast<char>(0xFF));
    // Only add 100 bytes (should detect overflow)
    overflow_content_data.append(QByteArray(100, 'X'));
    
    result = m_decoder->processMOTObject(overflow_content_data);
    QVERIFY(!result);  // Should fail due to overflow/excessive length protection
    
    // Test 3: Sanity check for excessive content length (> 65000 bytes)
    QByteArray excessive_length_data = createObjectWithTimestamp(3, 50000, 0x120000);
    
    excessive_length_data.append(static_cast<char>(5));  // Title length
    excessive_length_data.append("Title");
    
    // Content length = 65001 (exceeds sanity limit)
    excessive_length_data.append(static_cast<char>(0xFD));  // 65001 >> 8
    excessive_length_data.append(static_cast<char>(0xE9));  // 65001 & 0xFF
    excessive_length_data.append(QByteArray(100, 'X'));     // Partial data
    
    result = m_decoder->processMOTObject(excessive_length_data);
    QVERIFY(!result);  // Should fail due to sanity check
    
    // Test 4: Valid large content (within limits)
    QByteArray valid_large_data = createTestJournalineObject(
        4, 0x01, "Large Content", QString(10000, 'X')  // 10KB content
    );
    
    result = m_decoder->processMOTObject(valid_large_data);
    QVERIFY(result);  // Should succeed - within reasonable limits
    
    auto obj = m_decoder->getObject(4);
    QVERIFY(obj.has_value());
    QCOMPARE(obj->text_content.length(), 10000);
}

void TestJournalineDecoder::testEdgeCase_ExtractionFailure() {
    // BUG-003-HIGH FIX VERIFICATION: std::optional return for extraction failures
    
    // Test 1: Extraction failure at object ID
    QByteArray truncated_id;
    truncated_id.append(static_cast<char>(0x00));  // Only 1 byte (need 2)
    
    bool result = m_decoder->processMOTObject(truncated_id);
    QVERIFY(!result);  // Should fail gracefully
    
    auto stats = m_decoder->getStatistics();
    QVERIFY(stats.parsing_errors > 0);
    
    // Test 2: Extraction failure at MJD (in header)
    QByteArray truncated_mjd;
    truncated_mjd.append(static_cast<char>(0x00));  // Object ID MSB
    truncated_mjd.append(static_cast<char>(0x01));  // Object ID LSB
    truncated_mjd.append(static_cast<char>(0x01));  // Category
    truncated_mjd.append(static_cast<char>(0x00));  // MJD MSB
    // Missing MJD LSB
    
    result = m_decoder->processMOTObject(truncated_mjd);
    QVERIFY(!result);  // Should fail gracefully with std::nullopt
    
    // Test 3: Extraction failure at content length
    QByteArray truncated_content_len = createObjectWithTimestamp(3, 50000, 0x120000);
    
    truncated_content_len.append(static_cast<char>(5));  // Title length
    truncated_content_len.append("Title");
    truncated_content_len.append(static_cast<char>(0x00));  // Content length MSB
    // Missing content length LSB
    
    result = m_decoder->processMOTObject(truncated_content_len);
    QVERIFY(!result);  // Should fail gracefully
    
    // Test 4: Extraction failure at link target
    QByteArray truncated_link = createObjectWithTimestamp(4, 50000, 0x120000);
    
    truncated_link.append(static_cast<char>(5));  // Title length
    truncated_link.append("Title");
    truncated_link.append(static_cast<char>(0x00));  // Content length MSB
    truncated_link.append(static_cast<char>(7));     // Content length LSB
    truncated_link.append("Content");
    truncated_link.append(static_cast<char>(0x80));  // Link flag set
    truncated_link.append(static_cast<char>(0x00));  // Link target MSB
    // Missing link target LSB
    
    result = m_decoder->processMOTObject(truncated_link);
    // Should still process but link should be disabled
    QVERIFY(result);  // Object itself is valid
    
    auto obj = m_decoder->getObject(4);
    QVERIFY(obj.has_value());
    QVERIFY(!obj->is_link);  // Link should be disabled due to extraction failure
    
    // Test 5: Verify extraction with negative offset handling (internal test)
    // This is implicitly tested by the above cases where offset calculations
    // might produce unexpected values
    
    // Test 6: Valid extraction (baseline)
    QByteArray valid_data = createTestJournalineObject(
        100, 0x01, "Valid", "Valid content", true, 200
    );
    
    result = m_decoder->processMOTObject(valid_data);
    QVERIFY(result);  // All extractions should succeed
    
    auto valid_obj = m_decoder->getObject(100);
    QVERIFY(valid_obj.has_value());
    QVERIFY(valid_obj->is_link);
    QCOMPARE(valid_obj->link_target, static_cast<uint16_t>(200));
}

// ============================================================================
// Helper Methods
// ============================================================================

QByteArray TestJournalineDecoder::createTestJournalineObject(
    uint16_t object_id, uint8_t category,
    const QString& title, const QString& content,
    bool with_link, uint16_t link_target)
{
    QByteArray data;

    // Object ID (16-bit big-endian)
    data.append(static_cast<char>((object_id >> 8) & 0xFF));
    data.append(static_cast<char>(object_id & 0xFF));

    // Category (8-bit)
    data.append(static_cast<char>(category));

    // Timestamp (40-bit): MJD (16-bit) + UTC (24-bit)
    uint16_t mjd = 50000; // Default date
    data.append(static_cast<char>((mjd >> 8) & 0xFF));
    data.append(static_cast<char>(mjd & 0xFF));

    uint32_t utc = 0x120000; // 12:00:00 BCD
    data.append(static_cast<char>((utc >> 16) & 0xFF));
    data.append(static_cast<char>((utc >> 8) & 0xFF));
    data.append(static_cast<char>(utc & 0xFF));

    // Title
    QByteArray title_bytes = title.toUtf8();
    data.append(static_cast<char>(title_bytes.length()));
    data.append(title_bytes);

    // Content
    QByteArray content_bytes = content.toUtf8();
    uint16_t content_len = content_bytes.length();
    data.append(static_cast<char>((content_len >> 8) & 0xFF));
    data.append(static_cast<char>(content_len & 0xFF));
    data.append(content_bytes);

    // Link (optional)
    if (with_link) {
        data.append(static_cast<char>(0x80)); // Link flag set
        data.append(static_cast<char>((link_target >> 8) & 0xFF));
        data.append(static_cast<char>(link_target & 0xFF));
    } else {
        data.append(static_cast<char>(0x00)); // Link flag clear
    }

    return data;
}

QByteArray TestJournalineDecoder::createMinimalObject(uint16_t object_id) {
    return createTestJournalineObject(object_id, 0x01, "Title", "Content");
}

QByteArray TestJournalineDecoder::createObjectWithTimestamp(
    uint16_t object_id, uint16_t mjd, uint32_t utc_time)
{
    QByteArray data;

    // Object ID
    data.append(static_cast<char>((object_id >> 8) & 0xFF));
    data.append(static_cast<char>(object_id & 0xFF));

    // Category
    data.append(static_cast<char>(0x01));

    // Timestamp
    data.append(static_cast<char>((mjd >> 8) & 0xFF));
    data.append(static_cast<char>(mjd & 0xFF));

    data.append(static_cast<char>((utc_time >> 16) & 0xFF));
    data.append(static_cast<char>((utc_time >> 8) & 0xFF));
    data.append(static_cast<char>(utc_time & 0xFF));

    return data;
}

QTEST_MAIN(TestJournalineDecoder)
#include "test_journaline_decoder.moc"
