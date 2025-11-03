#include <QtTest/QtTest>
class TestE2EMotSlideshow : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { QSKIP("E2E test stub"); }
};
QTEST_MAIN(TestE2EMotSlideshow)
#include "test_e2e_mot_slideshow.moc"
