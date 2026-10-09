// Placeholder test file to fix CMake build
#include <QtTest/QtTest>
#include <QObject>

class TestMainWorkflow : public QObject
{
    Q_OBJECT

private slots:
    void placeholderTest();
};

void TestMainWorkflow::placeholderTest()
{
    QVERIFY(true);
}

QTEST_MAIN(TestMainWorkflow)
#include "test_main_workflow.moc"