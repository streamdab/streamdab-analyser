#include <QtTest/QtTest>
class TestE2EEtiToAudio : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { QSKIP("E2E test stub"); }
};
QTEST_MAIN(TestE2EEtiToAudio)
#include "test_e2e_eti_to_audio.moc"
