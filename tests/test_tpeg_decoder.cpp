/**
 * @file test_tpeg_decoder.cpp
 * @brief PRIVATE, capture-backed TPEG/TEPG tests (real bytes).
 *
 * Feeds the ACTUAL TEPG subchannel bytes of the Bangkok capture
 * (eti/bkk_20062022_141637.eti, SCId 22, 8 kb/s) into TpegDecoder and asserts
 * what the stream really contains:
 *   - no TS 102 894-1 sync pair (0x0A 0x46) anywhere in the stream;
 *   - the observed 4-phase marker framing is detected;
 *   - the text inventory contains the real TPEG2-style messages
 *     ("TPEG TEC+TFP+PKI+E", version "R-3.1-wip…", parking texts).
 *
 * This test REQUIRES the local capture and is labelled PRIVATE — it never
 * runs on PUBLIC CI.
 *
 * @date October 2026
 */

#include <QtTest/QtTest>
#include <QFile>
#include <QDir>

#include "../src/core/tpeg_decoder.hpp"
#include "../src/core/eti_file_scanner.hpp"

using namespace eti::tpeg;

namespace {
// Fixture discovery: rely on the build-tree source dir first, then fall back
// to common relative locations (mirrors test_cli_fig_trace / test_eti_ni_parser).
QString findBkkFixture()
{
    const QStringList candidates = {
        QStringLiteral(QT_TESTCASE_SOURCEDIR) +
            QStringLiteral("/../eti/bkk_20062022_141637.eti"),
        QStringLiteral("../../eti/bkk_20062022_141637.eti"),
        QStringLiteral("../eti/bkk_20062022_141637.eti"),
        QStringLiteral("eti/bkk_20062022_141637.eti"),
    };
    for (const QString& candidate : candidates) {
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return QString();
}
} // namespace

class TestTpegDecoder : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testRealTepg_NoSyncPair();
    void testRealTepg_FramingDetected();
    void testRealTepg_MessageInventory();
    void testRealTepg_ApplicationFamilies();
    void testRealTepg_DataServiceSnapshot();

private:
    QString m_fixture;
    QByteArray m_tepgStream;
};

void TestTpegDecoder::initTestCase()
{
    m_fixture = findBkkFixture();
    if (m_fixture.isEmpty()) {
        QSKIP("Real ETI fixture eti/bkk_20062022_141637.eti not found");
    }

    eti::EtiScanResult scan;
    QVERIFY2(eti::scanEtiFile(m_fixture, scan),
             "the Bangkok ETI capture must scan");
    const eti::ScannedSubchannel* tepg = scan.find(22);
    QVERIFY2(tepg != nullptr, "bkk SCId 22 (TEPG 8 kb/s) must be present");
    m_tepgStream = tepg->merged_bytes;
    QVERIFY2(m_tepgStream.size() > 48000,
             "TEPG subchannel must carry many frames of bytes");
}

void TestTpegDecoder::testRealTepg_NoSyncPair()
{
    // Empirical finding: the capture does NOT contain the TS 102 894-1 sync
    // pair 0x0A 0x46 anywhere in the TEPG subchannel (24 B/frame x ~5000
    // frames). The stream uses the observed vendor carousel framing instead.
    QVERIFY2(!TpegDecoder::containsSyncPair(m_tepgStream),
             "no TS 102 894-1 0x0A 0x46 sync in the bkk TEPG stream");
}

void TestTpegDecoder::testRealTepg_FramingDetected()
{
    TpegDecoder dec;
    dec.processStreamData(m_tepgStream);
    dec.finish();

    // The 4-phase marker framing must be detected on the real bytes.
    QVERIFY2(dec.frameCount() >= 1000,
             qPrintable(QString("TEPG frames decoded: %1")
                            .arg(dec.frameCount())));
    const QString framing = dec.framingDescription();
    QVERIFY2(framing.contains("carousel") || framing.contains("0x0A"),
             qPrintable(QString("unexpected framing report: %1").arg(framing)));
    // Sanity: the observed phase markers dominate the first-byte histogram.
    const QMap<quint8, quint32> markers = dec.markerDistribution();
    QVERIFY2(!markers.isEmpty(), "marker distribution must not be empty");
    quint32 top = 0;
    for (auto it = markers.cbegin(); it != markers.cend(); ++it) {
        top = qMax(top, it.value());
    }
    QVERIFY2(top > 10, "the dominant marker phase must repeat many times");
}

void TestTpegDecoder::testRealTepg_MessageInventory()
{
    TpegDecoder dec;
    dec.processStreamData(m_tepgStream);
    dec.finish();

    QVERIFY2(dec.messageCount() >= 20,
             qPrintable(QString("unique TEPG text inventory: %1")
                            .arg(dec.messageCount())));

    // The stream really contains the TPEG2 application banner.
    bool sawBanner = false;
    bool sawVersion = false;
    for (const TpegMessage& m : dec.messages()) {
        if (m.text.contains("TEC") && m.text.contains("TFP") &&
            m.text.contains("PKI")) {
            sawBanner = true;
        }
        if (m.text.contains("R-3.1")) {
            sawVersion = true;
        }
    }
    QVERIFY2(sawBanner,
             "the live TPEG2 application banner 'TPEG TEC+TFP+PKI+E' must be decoded");
    QVERIFY2(sawVersion,
             "the live version marker 'R-3.1-wip|…' must be decoded");
}

void TestTpegDecoder::testRealTepg_ApplicationFamilies()
{
    TpegDecoder dec;
    dec.processStreamData(m_tepgStream);
    dec.finish();

    bool sawParking = false;
    bool sawTraffic = false;
    for (const TpegMessage& m : dec.messages()) {
        if (m.category == "PKI" ||
            m.text.contains("Parking", Qt::CaseInsensitive) ||
            m.text.contains("charge", Qt::CaseInsensitive)) {
            sawParking = true;
        }
        if (m.category == "TEC" ||
            m.text.contains("Bangkok", Qt::CaseInsensitive) ||
            m.text.contains("Khwaeng", Qt::CaseInsensitive)) {
            sawTraffic = true;
        }
    }
    QVERIFY2(sawParking,
             "the real capture carries parking info messages (PKI family)");
    QVERIFY2(sawTraffic,
             "the real capture carries traffic/address messages (TEC family)");
}

void TestTpegDecoder::testRealTepg_DataServiceSnapshot()
{
    TpegDecoder dec;
    dec.processStreamData(m_tepgStream);
    dec.finish();

    const TpegDecoder::Statistics st = dec.getStatistics();
    QVERIFY(st.bytes_appended == static_cast<quint64>(m_tepgStream.size()));
    QVERIFY(st.unique_messages >= 20);
    QVERIFY(st.repeat_count >= st.unique_messages);
    QVERIFY(!st.sync_0a46);
    QVERIFY(st.frames > 1000);
}

QTEST_GUILESS_MAIN(TestTpegDecoder)
#include "test_tpeg_decoder.moc"