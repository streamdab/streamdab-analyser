#include <QtTest/QtTest>
class TestE2EEpgJournaline : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { QSKIP("E2E test stub"); }
};
QTEST_MAIN(TestE2EEpgJournaline)
#include "test_e2e_epg_journaline.moc"
