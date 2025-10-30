#include <QtTest/QtTest>

class TestGuiServiceDiscoveryPanel : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QSKIP("GUI test stub - not yet implemented");
    }
};

QTEST_MAIN(TestGuiServiceDiscoveryPanel)
#include "test_gui_service_discovery_panel.moc"
