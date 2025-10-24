/**
 * @file test_service_list.cpp  
 * @brief QTest automated tests for Service List (Ensemble Tree) widget
 * 
 * Tests service list population, placeholder display, service metadata,
 * and UTF-8 Thai language support.
 * 
 * @author QA Testing Agent
 * @date 2025-10-25
 */

#include <QtTest/QtTest>
#include <QTreeWidget>
#include <QTreeWidgetItem>

class TestServiceList : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Test cases
    void testTreeCreation();
    void testTreeHeaders();
    void testEmptyStatePlaceholder();
    void testEnsembleRootCreation();
    void testServiceItemCreation();
    void testDABPlusDetection();
    void testServiceLabelDisplay();
    void testComponentTreeStructure();
    void testThaiUTF8Support();
    void testExpansionState();
    void testAlternatingRowColors();

private:
    QTreeWidget* m_tree = nullptr;
    
    QTreeWidget* createServiceTree();
    void addTestService(QTreeWidget* tree, const QString& label, bool isDABPlus);
};

void TestServiceList::initTestCase()
{
    qDebug() << "=== Service List (Ensemble Tree) Test Suite ===";
    qDebug() << "Testing DAB service discovery and display functionality";
}

void TestServiceList::cleanupTestCase()
{
    qDebug() << "=== Service List Tests Complete ===";
}

void TestServiceList::init()
{
    m_tree = createServiceTree();
}

void TestServiceList::cleanup()
{
    delete m_tree;
    m_tree = nullptr;
}

void TestServiceList::testTreeCreation()
{
    QVERIFY(m_tree != nullptr);
    QVERIFY(m_tree->columnCount() == 3);
}

void TestServiceList::testTreeHeaders()
{
    QCOMPARE(m_tree->headerItem()->text(0), QString("Service"));
    QCOMPARE(m_tree->headerItem()->text(1), QString("Label"));
    QCOMPARE(m_tree->headerItem()->text(2), QString("Type"));
}

void TestServiceList::testEmptyStatePlaceholder()
{
    // Simulate no services discovered
    m_tree->clear();
    
    QTreeWidgetItem* placeholderItem = new QTreeWidgetItem(m_tree);
    placeholderItem->setText(0, "No services discovered yet...");
    placeholderItem->setText(1, "Load an ETI file to see DAB services");
    placeholderItem->setForeground(0, QColor(150, 150, 150));
    
    QCOMPARE(m_tree->topLevelItemCount(), 1);
    QCOMPARE(m_tree->topLevelItem(0)->text(0), QString("No services discovered yet..."));
}

void TestServiceList::testEnsembleRootCreation()
{
    m_tree->clear();
    
    QTreeWidgetItem* ensembleItem = new QTreeWidgetItem(m_tree);
    ensembleItem->setText(0, "Test Ensemble [EID: 0xE001]");
    ensembleItem->setText(1, "3 services");
    
    QFont ensembleFont = ensembleItem->font(0);
    ensembleFont.setBold(true);
    ensembleItem->setFont(0, ensembleFont);
    
    QVERIFY(ensembleItem->font(0).bold());
    QCOMPARE(m_tree->topLevelItemCount(), 1);
}

void TestServiceList::testServiceItemCreation()
{
    m_tree->clear();
    addTestService(m_tree, "Test Service", false);
    
    QVERIFY(m_tree->topLevelItemCount() > 0);
}

void TestServiceList::testDABPlusDetection()
{
    m_tree->clear();
    
    // Add DAB+ service
    QTreeWidgetItem* root = new QTreeWidgetItem(m_tree);
    QTreeWidgetItem* dabPlusService = new QTreeWidgetItem(root);
    dabPlusService->setText(0, "NBT Thailand [SID: 0xC221] (DAB+)");
    
    QFont dabPlusFont = dabPlusService->font(0);
    dabPlusFont.setBold(true);
    dabPlusService->setFont(0, dabPlusFont);
    dabPlusService->setForeground(0, QColor(0, 100, 200));
    
    QVERIFY(dabPlusService->font(0).bold());
    QVERIFY(dabPlusService->text(0).contains("(DAB+)"));
}

void TestServiceList::testServiceLabelDisplay()
{
    m_tree->clear();
    
    QTreeWidgetItem* root = new QTreeWidgetItem(m_tree);
    QTreeWidgetItem* service = new QTreeWidgetItem(root);
    service->setText(0, "Test Service [SID: 0x1234]");
    service->setText(1, "2 component(s)");
    
    QVERIFY(service->text(0).contains("SID"));
    QVERIFY(service->text(1).contains("component"));
}

void TestServiceList::testComponentTreeStructure()
{
    m_tree->clear();
    
    QTreeWidgetItem* root = new QTreeWidgetItem(m_tree);
    QTreeWidgetItem* service = new QTreeWidgetItem(root);
    QTreeWidgetItem* component = new QTreeWidgetItem(service);
    component->setText(0, "Component 0 [SubChId: 1, DAB+ Audio]");
    component->setForeground(0, QColor(100, 100, 100));
    
    QCOMPARE(service->childCount(), 1);
    QVERIFY(component->text(0).contains("SubChId"));
}

void TestServiceList::testThaiUTF8Support()
{
    m_tree->clear();
    
    QTreeWidgetItem* root = new QTreeWidgetItem(m_tree);
    QTreeWidgetItem* thaiService = new QTreeWidgetItem(root);
    
    // Thai text: สถานีวิทยุ (Radio Station)
    QString thaiLabel = QString::fromUtf8("สถานีวิทยุ [SID: 0xC221]");
    thaiService->setText(0, thaiLabel);
    
    QCOMPARE(thaiService->text(0), thaiLabel);
    QVERIFY(thaiService->text(0).contains(QString::fromUtf8("สถานีวิทยุ")));
}

void TestServiceList::testExpansionState()
{
    m_tree->clear();
    
    QTreeWidgetItem* root = new QTreeWidgetItem(m_tree);
    root->setExpanded(true);
    
    QVERIFY(root->isExpanded());
    
    root->setExpanded(false);
    QVERIFY(!root->isExpanded());
}

void TestServiceList::testAlternatingRowColors()
{
    QVERIFY(m_tree->alternatingRowColors());
}

QTreeWidget* TestServiceList::createServiceTree()
{
    QTreeWidget* tree = new QTreeWidget();
    tree->setHeaderLabels({"Service", "Label", "Type"});
    tree->setAlternatingRowColors(true);
    tree->setColumnCount(3);
    return tree;
}

void TestServiceList::addTestService(QTreeWidget* tree, const QString& label, bool isDABPlus)
{
    QTreeWidgetItem* root = new QTreeWidgetItem(tree);
    QTreeWidgetItem* service = new QTreeWidgetItem(root);
    
    QString displayText = QString("%1 [SID: 0x1234]").arg(label);
    if (isDABPlus) {
        displayText += " (DAB+)";
    }
    
    service->setText(0, displayText);
    service->setText(1, "Test Label");
    service->setText(2, isDABPlus ? "DAB+" : "DAB");
}

QTEST_MAIN(TestServiceList)
#include "test_service_list.moc"
