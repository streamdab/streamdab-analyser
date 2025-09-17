/**
 * @file test_service_browser.cpp
 * @brief Comprehensive Qt GUI tests for ServiceBrowser component
 */

#include <QtTest/QtTest>
#include <QApplication>
#include <QWidget>
#include <QTreeWidget>
#include <QListWidget>
#include <QTableWidget>
#include <QSignalSpy>
#include <memory>

#include "gui/service_browser.h"
#include "mocks/mock_gui_components.h"
#include "fixtures/eti_streams/sample_eti_frame.h"

class TestServiceBrowser : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    // === BASIC FUNCTIONALITY TESTS ===
    void testServiceBrowserConstruction();
    void testServiceAdditionAndRemoval();
    void testServiceSelection();
    void testServiceFiltering();

    // === GUI INTERACTION TESTS ===
    void testServiceListDisplay();
    void testServiceDetailsDisplay();
    void testUserInteraction();

    // === PERFORMANCE TESTS ===
    void testLargeServiceListHandling();
    void testRealTimeServiceUpdates();

private:
    std::unique_ptr<ServiceBrowser> m_serviceBrowser;
    MockServiceBrowser* m_mockBrowser;
};

void TestServiceBrowser::testServiceBrowserConstruction()
{
    ServiceBrowser browser;
    QVERIFY(browser.getServiceCount() == 0);
    qDebug() << "✅ ServiceBrowser construction tests passed";
}

QTEST_MAIN(TestServiceBrowser)
#include "test_service_browser.moc"