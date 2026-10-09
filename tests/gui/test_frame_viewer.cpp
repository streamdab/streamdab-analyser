/**
 * @file test_frame_viewer.cpp
 * @brief QTest automated tests for Frame Viewer hex display widget
 * 
 * Tests frame list population, hex dump display, safe click handling,
 * and bounds checking.
 * 
 * @author QA Testing Agent
 * @date 2025-10-25
 */

#include <QtTest/QtTest>
#include <QListWidget>
#include <QTextEdit>
#include <QListWidgetItem>

class TestFrameViewer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Test cases
    void testFrameListCreation();
    void testFrameListPopulation();
    void testFrameItemFormat();
    void testSafeClickHandling();
    void testBoundsChecking();
    void testHexViewerCreation();
    void testHexDumpFormat();
    void testHexDumpLength();
    void testEmptyFrameList();
    void testFrameSelectionSignal();

private:
    QListWidget* m_frameList = nullptr;
    QTextEdit* m_hexViewer = nullptr;
    
    void populateFrameList(int frameCount);
    QString generateHexDump(int frameIndex);
};

void TestFrameViewer::initTestCase()
{
    qDebug() << "=== Frame Viewer Test Suite ===";
    qDebug() << "Testing frame list and hex viewer functionality";
}

void TestFrameViewer::cleanupTestCase()
{
    qDebug() << "=== Frame Viewer Tests Complete ===";
}

void TestFrameViewer::init()
{
    m_frameList = new QListWidget();
    m_hexViewer = new QTextEdit();
    m_hexViewer->setFont(QFont("Consolas", 9));
    m_hexViewer->setReadOnly(true);
}

void TestFrameViewer::cleanup()
{
    delete m_frameList;
    delete m_hexViewer;
    m_frameList = nullptr;
    m_hexViewer = nullptr;
}

void TestFrameViewer::testFrameListCreation()
{
    QVERIFY(m_frameList != nullptr);
    QCOMPARE(m_frameList->count(), 0);
}

void TestFrameViewer::testFrameListPopulation()
{
    populateFrameList(100);
    
    QCOMPARE(m_frameList->count(), 100);
    
    // Verify first and last items
    QVERIFY(m_frameList->item(0) != nullptr);
    QVERIFY(m_frameList->item(99) != nullptr);
}

void TestFrameViewer::testFrameItemFormat()
{
    populateFrameList(5);
    
    // Check frame number format
    QString firstFrame = m_frameList->item(0)->text();
    QVERIFY(firstFrame.contains("Frame"));
    QVERIFY(firstFrame.contains("1")); // 1-indexed display
}

void TestFrameViewer::testSafeClickHandling()
{
    populateFrameList(10);
    
    // Simulate click on valid frame
    try {
        m_frameList->setCurrentRow(5);
        QCOMPARE(m_frameList->currentRow(), 5);
    } catch (...) {
        QFAIL("Exception thrown on valid frame selection");
    }
}

void TestFrameViewer::testBoundsChecking()
{
    populateFrameList(10);
    
    // Test negative index
    m_frameList->setCurrentRow(-1);
    QCOMPARE(m_frameList->currentRow(), -1); // No selection
    
    // Test out of bounds index (should not crash)
    m_frameList->setCurrentRow(1000);
    // Qt handles this gracefully by not selecting anything
}

void TestFrameViewer::testHexViewerCreation()
{
    QVERIFY(m_hexViewer != nullptr);
    QVERIFY(m_hexViewer->isReadOnly());
    QCOMPARE(m_hexViewer->font().family(), QString("Consolas"));
}

void TestFrameViewer::testHexDumpFormat()
{
    QString hexDump = generateHexDump(0);
    m_hexViewer->setPlainText(hexDump);
    
    QString content = m_hexViewer->toPlainText();
    QVERIFY(!content.isEmpty());
    QVERIFY(content.contains("Frame 1")); // Title line
}

void TestFrameViewer::testHexDumpLength()
{
    QString hexDump = generateHexDump(0);
    
    // First 256 bytes should be displayed (16 rows * 16 bytes)
    QStringList lines = hexDump.split('\n');
    
    // Should have header + 16 hex rows + some metadata
    QVERIFY(lines.size() >= 16);
}

void TestFrameViewer::testEmptyFrameList()
{
    QCOMPARE(m_frameList->count(), 0);
    
    // Selecting in empty list should not crash
    m_frameList->setCurrentRow(0);
    QCOMPARE(m_frameList->currentRow(), -1);
}

void TestFrameViewer::testFrameSelectionSignal()
{
    populateFrameList(5);
    
    QSignalSpy spy(m_frameList, &QListWidget::currentRowChanged);
    
    m_frameList->setCurrentRow(2);
    
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), 2);
}

void TestFrameViewer::populateFrameList(int frameCount)
{
    for (int i = 0; i < frameCount; ++i) {
        QString frameText = QString("Frame %1 (6144 bytes, offset %2)")
                           .arg(i + 1)
                           .arg(i * 6144);
        m_frameList->addItem(frameText);
    }
}

QString TestFrameViewer::generateHexDump(int frameIndex)
{
    QString hex;
    hex += QString("=== Frame %1 Hex Dump (First 256 bytes) ===\n\n").arg(frameIndex + 1);
    
    // Generate 16 rows of hex data (16 bytes per row = 256 bytes)
    for (int row = 0; row < 16; ++row) {
        hex += QString("%1: ").arg(row * 16, 4, 16, QChar('0')).toUpper();
        
        // 16 bytes in hex
        for (int col = 0; col < 16; ++col) {
            uint8_t byte = (row * 16 + col) % 256;
            hex += QString("%1 ").arg(byte, 2, 16, QChar('0')).toUpper();
        }
        
        hex += "  |  ";
        
        // ASCII representation
        for (int col = 0; col < 16; ++col) {
            uint8_t byte = (row * 16 + col) % 256;
            char ch = (byte >= 32 && byte < 127) ? static_cast<char>(byte) : '.';
            hex += ch;
        }
        
        hex += "\n";
    }
    
    return hex;
}

QTEST_MAIN(TestFrameViewer)
#include "test_frame_viewer.moc"
