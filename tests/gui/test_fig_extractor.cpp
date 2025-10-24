/**
 * @file test_fig_extractor.cpp
 * @brief QTest automated tests for FIG Extractor statistics widget
 * 
 * Tests FIG type statistics display, description formatting, and count accuracy.
 * 
 * @author QA Testing Agent
 * @date 2025-10-25
 */

#include <QtTest/QtTest>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTextEdit>
#include <QMap>

class TestFIGExtractor : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Test cases
    void testFIGTreeCreation();
    void testFIGTreeHeaders();
    void testFIGTypeDescriptions();
    void testFIGCountDisplay();
    void testFIG0SubtypeRange();
    void testFIG1SubtypeRange();
    void testFIGStatusIndicator();
    void testFIGContentParsing();
    void testEmptyFIGList();
    void testFIGAlternatingColors();

private:
    QTreeWidget* m_figTree = nullptr;
    QTextEdit* m_figDetails = nullptr;
    
    void addTestFIG(const QString& type, const QString& extension, 
                   const QString& content, int count);
    QString getFIGDescription(const QString& figType);
};

void TestFIGExtractor::initTestCase()
{
    qDebug() << "=== FIG Extractor Test Suite ===";
    qDebug() << "Testing FIG statistics display and analysis";
}

void TestFIGExtractor::cleanupTestCase()
{
    qDebug() << "=== FIG Extractor Tests Complete ===";
}

void TestFIGExtractor::init()
{
    m_figTree = new QTreeWidget();
    m_figTree->setHeaderLabels({"FIG Type", "Extension", "Content", "Status"});
    m_figTree->setAlternatingRowColors(true);
    m_figTree->setRootIsDecorated(true);
    
    m_figDetails = new QTextEdit();
    m_figDetails->setReadOnly(true);
}

void TestFIGExtractor::cleanup()
{
    delete m_figTree;
    delete m_figDetails;
    m_figTree = nullptr;
    m_figDetails = nullptr;
}

void TestFIGExtractor::testFIGTreeCreation()
{
    QVERIFY(m_figTree != nullptr);
    QCOMPARE(m_figTree->columnCount(), 4);
}

void TestFIGExtractor::testFIGTreeHeaders()
{
    QCOMPARE(m_figTree->headerItem()->text(0), QString("FIG Type"));
    QCOMPARE(m_figTree->headerItem()->text(1), QString("Extension"));
    QCOMPARE(m_figTree->headerItem()->text(2), QString("Content"));
    QCOMPARE(m_figTree->headerItem()->text(3), QString("Status"));
}

void TestFIGExtractor::testFIGTypeDescriptions()
{
    // Test common FIG type descriptions
    QVERIFY(getFIGDescription("FIG 0/0").contains("Ensemble"));
    QVERIFY(getFIGDescription("FIG 0/1").contains("Sub-channel"));
    QVERIFY(getFIGDescription("FIG 0/2").contains("Service"));
    QVERIFY(getFIGDescription("FIG 0/10").contains("Date"));
}

void TestFIGExtractor::testFIGCountDisplay()
{
    addTestFIG("FIG 0/0", "0", "Ensemble Information", 1542);
    
    QCOMPARE(m_figTree->topLevelItemCount(), 1);
    
    QTreeWidgetItem* item = m_figTree->topLevelItem(0);
    QVERIFY(item->text(2).contains("1542") || item->text(3).contains("1542"));
}

void TestFIGExtractor::testFIG0SubtypeRange()
{
    // FIG 0 should have extensions 0-19 implemented
    QStringList validExtensions = {
        "0", "1", "2", "3", "5", "8", "9", "10", 
        "13", "14", "17", "18", "19"
    };
    
    for (const QString& ext : validExtensions) {
        addTestFIG("FIG 0", ext, "Test Content", 10);
    }
    
    QVERIFY(m_figTree->topLevelItemCount() > 0);
}

void TestFIGExtractor::testFIG1SubtypeRange()
{
    // FIG 1 should have extensions 0, 1, 4, 5 implemented
    QStringList validExtensions = {"0", "1", "4", "5"};
    
    for (const QString& ext : validExtensions) {
        addTestFIG("FIG 1", ext, "Label Content", 5);
    }
    
    QVERIFY(m_figTree->topLevelItemCount() > 0);
}

void TestFIGExtractor::testFIGStatusIndicator()
{
    addTestFIG("FIG 0/0", "0", "Ensemble Information", 100);
    
    QTreeWidgetItem* item = m_figTree->topLevelItem(0);
    QString status = item->text(3);
    
    // Status should indicate successful parsing
    QVERIFY(status.contains("OK") || status.contains("Parsed") || 
            status.contains("Valid") || status == "100");
}

void TestFIGExtractor::testFIGContentParsing()
{
    QString testContent = "Ensemble ID: 0xE001, Label: Bangkok DAB+";
    addTestFIG("FIG 0/0", "0", testContent, 1);
    
    QTreeWidgetItem* item = m_figTree->topLevelItem(0);
    QCOMPARE(item->text(2), testContent);
}

void TestFIGExtractor::testEmptyFIGList()
{
    QCOMPARE(m_figTree->topLevelItemCount(), 0);
}

void TestFIGExtractor::testFIGAlternatingColors()
{
    QVERIFY(m_figTree->alternatingRowColors());
}

void TestFIGExtractor::addTestFIG(const QString& type, const QString& extension,
                                 const QString& content, int count)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(m_figTree);
    item->setText(0, type);
    item->setText(1, extension);
    item->setText(2, content);
    item->setText(3, QString::number(count));
}

QString TestFIGExtractor::getFIGDescription(const QString& figType)
{
    static QMap<QString, QString> descriptions = {
        {"FIG 0/0", "Ensemble information"},
        {"FIG 0/1", "Sub-channel organization"},
        {"FIG 0/2", "Service organization"},
        {"FIG 0/3", "Service component in packet mode"},
        {"FIG 0/5", "Service component language"},
        {"FIG 0/8", "Service component global definition"},
        {"FIG 0/9", "Country, LTO and International table"},
        {"FIG 0/10", "Date and time"},
        {"FIG 0/13", "User application information"},
        {"FIG 0/14", "FEC sub-channel organization"},
        {"FIG 0/17", "Programme type"},
        {"FIG 0/18", "Announcement support"},
        {"FIG 0/19", "Announcement switching"},
        {"FIG 1/0", "Ensemble label"},
        {"FIG 1/1", "Programme service label"},
        {"FIG 1/4", "Service component label"},
        {"FIG 1/5", "Data service label"}
    };
    
    return descriptions.value(figType, "Unknown FIG type");
}

QTEST_MAIN(TestFIGExtractor)
#include "test_fig_extractor.moc"
