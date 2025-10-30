#include <QtTest/QtTest>

class TestGuiMainWindow : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QSKIP("GUI test stub - not yet implemented");
    }
};

QTEST_MAIN(TestGuiMainWindow)
#include "test_gui_main_window.moc"
