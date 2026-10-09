/**
 * @file test_journaline_pipeline.cpp
 * @brief Integration Tests for Journaline Pipeline
 *
 * Tests complete MOT → Journaline processing pipeline with real-world scenarios.
 *
 * @author StreamDAB Development Team
 * @date October 22, 2025
 */

#include <QtTest/QtTest>
#include "../../src/core/journaline_decoder.hpp"
#include "../../src/core/mot_protocol.hpp"

using namespace eti::journaline;
using namespace eti::mot;

class TestJournalinePipeline : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    // Integration tests
    void testPipeline_MOTToJournaline();
    void testPipeline_MultipleObjects();
    void testPipeline_MenuNavigation();
    void testPipeline_LinkResolution();
    void testPipeline_RealWorldScenario();

private:
    JournalineDecoder* m_decoder = nullptr;
    MOTProtocol* m_mot_parser = nullptr;

    // Helper methods
    QByteArray createMOTObjectWithJournaline(const QByteArray& journaline_data);
    QByteArray createJournalineNewsArticle(uint16_t id, const QString& headline, const QString& body);
    QByteArray createJournalineSportsUpdate(uint16_t id);
    QByteArray createJournalineWeatherForecast(uint16_t id);
};

// ============================================================================
// Test Lifecycle
// ============================================================================

void TestJournalinePipeline::initTestCase() {
    qInfo() << "Starting Journaline pipeline integration tests";
}

void TestJournalinePipeline::init() {
    m_decoder = new JournalineDecoder();
    m_mot_parser = new MOTProtocol();

    QVERIFY(m_decoder != nullptr);
    QVERIFY(m_mot_parser != nullptr);
}

void TestJournalinePipeline::cleanup() {
    delete m_decoder;
    delete m_mot_parser;

    m_decoder = nullptr;
    m_mot_parser = nullptr;
}

void TestJournalinePipeline::cleanupTestCase() {
    qInfo() << "Journaline pipeline integration tests completed";
}

// ============================================================================
// Integration Tests
// ============================================================================

void TestJournalinePipeline::testPipeline_MOTToJournaline() {
    // Test complete MOT → Journaline pipeline
    
    // Create Journaline object data
    QByteArray journaline_data;
    
    // Object ID
    journaline_data.append(static_cast<char>(0x00));
    journaline_data.append(static_cast<char>(0x01));
    
    // Category (NEWS_GENERAL)
    journaline_data.append(static_cast<char>(0x01));
    
    // Timestamp (MJD + UTC)
    journaline_data.append(static_cast<char>(0xC3)); // MJD high
    journaline_data.append(static_cast<char>(0x50)); // MJD low (50000)
    journaline_data.append(static_cast<char>(0x12)); // UTC hour (BCD)
    journaline_data.append(static_cast<char>(0x00)); // UTC min
    journaline_data.append(static_cast<char>(0x00)); // UTC sec
    
    // Title
    QString title = "Breaking News";
    QByteArray title_bytes = title.toUtf8();
    journaline_data.append(static_cast<char>(title_bytes.length()));
    journaline_data.append(title_bytes);
    
    // Content
    QString content = "This is a test news article from the integration pipeline.";
    QByteArray content_bytes = content.toUtf8();
    uint16_t content_len = content_bytes.length();
    journaline_data.append(static_cast<char>((content_len >> 8) & 0xFF));
    journaline_data.append(static_cast<char>(content_len & 0xFF));
    journaline_data.append(content_bytes);
    
    // No link
    journaline_data.append(static_cast<char>(0x00));
    
    // Process through Journaline decoder
    bool result = m_decoder->processMOTObject(journaline_data);
    QVERIFY(result);
    
    // Verify object was decoded
    auto obj = m_decoder->getObject(1);
    QVERIFY(obj.has_value());
    QCOMPARE(obj->title, title);
    QCOMPARE(obj->text_content, content);
    QCOMPARE(obj->category, static_cast<uint8_t>(0x01));
    
    qInfo() << "Pipeline test successful: MOT → Journaline";
}

void TestJournalinePipeline::testPipeline_MultipleObjects() {
    // Test processing multiple Journaline objects through pipeline
    
    QVector<QPair<uint16_t, QString>> test_objects = {
        {1, "First News Article"},
        {2, "Second News Article"},
        {3, "Third News Article"},
        {4, "Fourth News Article"},
        {5, "Fifth News Article"}
    };
    
    for (const auto& pair : test_objects) {
        QByteArray journaline_data = createJournalineNewsArticle(
            pair.first,
            pair.second,
            QString("Content for article %1").arg(pair.first)
        );
        
        bool result = m_decoder->processMOTObject(journaline_data);
        QVERIFY(result);
    }
    
    // Verify all objects were decoded
    QCOMPARE(m_decoder->objectCount(), 5);
    
    auto objects = m_decoder->getObjects();
    QCOMPARE(objects.size(), 5);
    
    // Verify menu was constructed
    QVERIFY(m_decoder->menuCount() > 0);
    
    qInfo() << "Multiple objects processed:" << m_decoder->objectCount();
    qInfo() << "Menus created:" << m_decoder->menuCount();
}

void TestJournalinePipeline::testPipeline_MenuNavigation() {
    // Test complete menu navigation workflow
    
    // Create objects in different categories
    QByteArray news1 = createJournalineNewsArticle(1, "News 1", "Content 1");
    QByteArray news2 = createJournalineNewsArticle(2, "News 2", "Content 2");
    QByteArray sport1 = createJournalineSportsUpdate(10);
    QByteArray weather1 = createJournalineWeatherForecast(20);
    
    QVERIFY(m_decoder->processMOTObject(news1));
    QVERIFY(m_decoder->processMOTObject(news2));
    QVERIFY(m_decoder->processMOTObject(sport1));
    QVERIFY(m_decoder->processMOTObject(weather1));
    
    // Get root menu
    auto root_menus = m_decoder->getRootMenus();
    QVERIFY(!root_menus.isEmpty());
    
    JournalineMenu root = root_menus.first();
    QVERIFY(root.isRootMenu());
    qInfo() << "Root menu:" << root.toString();
    
    // Navigate to submenus
    auto submenus = m_decoder->getSubMenus(root.menu_id);
    QVERIFY(!submenus.isEmpty());
    
    qInfo() << "Found" << submenus.size() << "category submenus";
    
    // Verify menu structure
    for (const auto& menu : submenus) {
        QVERIFY(menu.isValid());
        QCOMPARE(menu.parent_menu_id, root.menu_id);
        QVERIFY(!menu.items.isEmpty());
        
        qInfo() << "  Category menu:" << menu.menu_title
                << "Items:" << menu.items.size();
    }
}

void TestJournalinePipeline::testPipeline_LinkResolution() {
    // Test complete link resolution workflow
    
    // Create summary article with link
    QByteArray summary_data;
    summary_data.append(static_cast<char>(0x00));
    summary_data.append(static_cast<char>(0x0A)); // ID = 10
    summary_data.append(static_cast<char>(0x01)); // Category
    
    // Timestamp
    for (int i = 0; i < 5; ++i) {
        summary_data.append(static_cast<char>(0x00));
    }
    
    QString summary_title = "Breaking: Major Event";
    QByteArray title_bytes = summary_title.toUtf8();
    summary_data.append(static_cast<char>(title_bytes.length()));
    summary_data.append(title_bytes);
    
    QString summary_content = "Click for full story...";
    QByteArray content_bytes = summary_content.toUtf8();
    uint16_t len = content_bytes.length();
    summary_data.append(static_cast<char>((len >> 8) & 0xFF));
    summary_data.append(static_cast<char>(len & 0xFF));
    summary_data.append(content_bytes);
    
    // Add link to object 20
    summary_data.append(static_cast<char>(0x80)); // Link flag
    summary_data.append(static_cast<char>(0x00));
    summary_data.append(static_cast<char>(0x14)); // Link target = 20
    
    // Create full article (target)
    QByteArray full_article = createJournalineNewsArticle(
        20,
        "Full Story: Major Event Details",
        "This is the complete article with all details..."
    );
    
    // Process both objects
    QVERIFY(m_decoder->processMOTObject(full_article));
    QVERIFY(m_decoder->processMOTObject(summary_data));
    
    // Get summary object
    auto summary_obj = m_decoder->getObject(10);
    QVERIFY(summary_obj.has_value());
    QVERIFY(summary_obj->is_link);
    QCOMPARE(summary_obj->link_target, static_cast<uint16_t>(20));
    
    // Resolve link
    auto full_obj = m_decoder->resolveLinkTarget(*summary_obj);
    QVERIFY(full_obj.has_value());
    QCOMPARE(full_obj->object_id, static_cast<uint16_t>(20));
    QVERIFY(full_obj->text_content.contains("complete article"));
    
    qInfo() << "Link resolution successful:";
    qInfo() << "  Summary:" << summary_obj->title;
    qInfo() << "  Full article:" << full_obj->title;
}

void TestJournalinePipeline::testPipeline_RealWorldScenario() {
    // Simulate real-world Journaline service with mixed content
    
    qInfo() << "=== Real-World Scenario Test ===";
    
    // News section (3 articles)
    QByteArray news1 = createJournalineNewsArticle(
        1, "Political Update", "Government announces new policy..."
    );
    QByteArray news2 = createJournalineNewsArticle(
        2, "Economic News", "Markets reach record high..."
    );
    QByteArray news3 = createJournalineNewsArticle(
        3, "International", "Global summit concludes..."
    );
    
    // Sports section (2 updates)
    QByteArray sport1 = createJournalineSportsUpdate(10);
    QByteArray sport2 = createJournalineSportsUpdate(11);
    
    // Weather section (1 forecast)
    QByteArray weather1 = createJournalineWeatherForecast(20);
    
    // Process all objects
    QVector<QByteArray> all_objects = {
        news1, news2, news3, sport1, sport2, weather1
    };
    
    for (const auto& obj_data : all_objects) {
        bool result = m_decoder->processMOTObject(obj_data);
        QVERIFY(result);
    }
    
    // Verify complete service
    QCOMPARE(m_decoder->objectCount(), 6);
    qInfo() << "Total objects processed:" << m_decoder->objectCount();
    
    // Verify categories
    auto news_objects = m_decoder->getObjectsByCategory(0x01);
    auto sport_objects = m_decoder->getObjectsByCategory(0x10);
    auto weather_objects = m_decoder->getObjectsByCategory(0x21); // WEATHER_FORECAST
    
    QCOMPARE(news_objects.size(), 3);
    QCOMPARE(sport_objects.size(), 2);
    QCOMPARE(weather_objects.size(), 1);
    
    qInfo() << "News articles:" << news_objects.size();
    qInfo() << "Sports updates:" << sport_objects.size();
    qInfo() << "Weather forecasts:" << weather_objects.size();
    
    // Verify menu structure
    auto root_menus = m_decoder->getRootMenus();
    QVERIFY(!root_menus.isEmpty());
    
    auto category_menus = m_decoder->getSubMenus(root_menus.first().menu_id);
    QVERIFY(category_menus.size() >= 3); // News, Sport, Weather
    
    qInfo() << "Menu structure created with" << category_menus.size() << "categories";
    
    // Verify statistics
    auto stats = m_decoder->getStatistics();
    QCOMPARE(stats.objects_processed, static_cast<uint32_t>(6));
    QVERIFY(stats.menus_processed > 0);
    QCOMPARE(stats.parsing_errors, static_cast<uint32_t>(0));
    
    qInfo() << "Statistics:";
    qInfo() << "  Objects:" << stats.objects_processed;
    qInfo() << "  Menus:" << stats.menus_processed;
    qInfo() << "  Errors:" << stats.parsing_errors;
    
    qInfo() << "=== Real-World Scenario PASSED ===";
}

// ============================================================================
// Helper Methods
// ============================================================================

QByteArray TestJournalinePipeline::createJournalineNewsArticle(
    uint16_t id, const QString& headline, const QString& body)
{
    QByteArray data;
    
    // Object ID
    data.append(static_cast<char>((id >> 8) & 0xFF));
    data.append(static_cast<char>(id & 0xFF));
    
    // Category (NEWS_GENERAL)
    data.append(static_cast<char>(0x01));
    
    // Timestamp (default)
    uint16_t mjd = 50000;
    data.append(static_cast<char>((mjd >> 8) & 0xFF));
    data.append(static_cast<char>(mjd & 0xFF));
    
    uint32_t utc = 0x120000;
    data.append(static_cast<char>((utc >> 16) & 0xFF));
    data.append(static_cast<char>((utc >> 8) & 0xFF));
    data.append(static_cast<char>(utc & 0xFF));
    
    // Title
    QByteArray title_bytes = headline.toUtf8();
    data.append(static_cast<char>(title_bytes.length()));
    data.append(title_bytes);
    
    // Content
    QByteArray content_bytes = body.toUtf8();
    uint16_t len = content_bytes.length();
    data.append(static_cast<char>((len >> 8) & 0xFF));
    data.append(static_cast<char>(len & 0xFF));
    data.append(content_bytes);
    
    // No link
    data.append(static_cast<char>(0x00));
    
    return data;
}

QByteArray TestJournalinePipeline::createJournalineSportsUpdate(uint16_t id) {
    QByteArray data;
    
    // Object ID
    data.append(static_cast<char>((id >> 8) & 0xFF));
    data.append(static_cast<char>(id & 0xFF));
    
    // Category (SPORT_GENERAL)
    data.append(static_cast<char>(0x10));
    
    // Timestamp
    uint16_t mjd = 50000;
    data.append(static_cast<char>((mjd >> 8) & 0xFF));
    data.append(static_cast<char>(mjd & 0xFF));
    
    uint32_t utc = 0x150000;
    data.append(static_cast<char>((utc >> 16) & 0xFF));
    data.append(static_cast<char>((utc >> 8) & 0xFF));
    data.append(static_cast<char>(utc & 0xFF));
    
    // Title
    QString title = QString("Sports Update %1").arg(id);
    QByteArray title_bytes = title.toUtf8();
    data.append(static_cast<char>(title_bytes.length()));
    data.append(title_bytes);
    
    // Content
    QString content = "Local team wins championship match...";
    QByteArray content_bytes = content.toUtf8();
    uint16_t len = content_bytes.length();
    data.append(static_cast<char>((len >> 8) & 0xFF));
    data.append(static_cast<char>(len & 0xFF));
    data.append(content_bytes);
    
    // No link
    data.append(static_cast<char>(0x00));
    
    return data;
}

QByteArray TestJournalinePipeline::createJournalineWeatherForecast(uint16_t id) {
    QByteArray data;
    
    // Object ID
    data.append(static_cast<char>((id >> 8) & 0xFF));
    data.append(static_cast<char>(id & 0xFF));
    
    // Category (WEATHER_FORECAST)
    data.append(static_cast<char>(0x21));
    
    // Timestamp
    uint16_t mjd = 50000;
    data.append(static_cast<char>((mjd >> 8) & 0xFF));
    data.append(static_cast<char>(mjd & 0xFF));
    
    uint32_t utc = 0x180000;
    data.append(static_cast<char>((utc >> 16) & 0xFF));
    data.append(static_cast<char>((utc >> 8) & 0xFF));
    data.append(static_cast<char>(utc & 0xFF));
    
    // Title
    QString title = "Weather Forecast";
    QByteArray title_bytes = title.toUtf8();
    data.append(static_cast<char>(title_bytes.length()));
    data.append(title_bytes);
    
    // Content
    QString content = "Sunny with temperatures up to 28°C. Light winds from the east.";
    QByteArray content_bytes = content.toUtf8();
    uint16_t len = content_bytes.length();
    data.append(static_cast<char>((len >> 8) & 0xFF));
    data.append(static_cast<char>(len & 0xFF));
    data.append(content_bytes);
    
    // No link
    data.append(static_cast<char>(0x00));
    
    return data;
}

QTEST_MAIN(TestJournalinePipeline)
#include "test_journaline_pipeline.moc"
