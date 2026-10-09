/**
 * @file test_e2e_epg_journaline.cpp
 * @brief End-to-end EPG / Journaline / TPEG data-path validation on REAL
 *        captures (PRIVATE).
 *
 * What the captures really contain (empirically established in
 * docs/EPG_JOURNALINE_TPEG_VALIDATION.md):
 *
 *  - eti/bkk_20062022_141637.eti
 *      * SCId 14 "EPG" 32 kb/s: a reserved-but-idle packet-mode data
 *        subchannel. ~88 % of its capacity-unit frames are a fixed padding
 *        pattern (`CC 00 … 00 43 10`); the residual traffic does not
 *        reconstruct to valid TS 102 371 EPG schedules. -> EPGDecoder is
 *        fed the real bytes and (honestly) yields 0 schedules.
 *      * SCId 22 "TEPG" 8 kb/s: a TPEG2-style display carousel (24-byte
 *        frames, 4-phase markers `10/20/30/00 01 13`). It carries the
 *        application banner "TPEG TEC+TFP+PKI+E", a version marker and
 *        rolling parking/traffic texts. -> TpegDecoder decodes them.
 *
 *  - eti/HessischerRundfunk-Warntag2024-2024-09-12_journalline/
 *    hr_20240912T105801_EWS_Start.eti
 *      * SCId 8 (32 kb/s): stream-mode Journaline display carousel with
 *        real Hessischen Rundfunk texts ("hr4 - Britta am Vormittag",
 *        "Die ARD-Hitnacht ist eine Produktion des MDR …"). ->
 *        JournalineDecoder::processStreamData() extracts them.
 *
 * This test REQUIRES the local captures and is labelled PRIVATE.
 *
 * @date October 2026
 */

#include <QtTest/QtTest>
#include <QFile>
#include <QDir>

#include "core/epg_decoder.hpp"
#include "core/journaline_decoder.hpp"
#include "core/tpeg_decoder.hpp"
#include "core/data_service_store.hpp"
#include "core/eti_file_scanner.hpp"

using namespace eti;

namespace {

QString findFixture(const QString& relative)
{
    const QStringList candidates = {
        QStringLiteral(QT_TESTCASE_SOURCEDIR) + QStringLiteral("/../eti/") + relative,
        QStringLiteral("../../eti/") + relative,
        QStringLiteral("../eti/") + relative,
        QStringLiteral("eti/") + relative,
    };
    for (const QString& candidate : candidates) {
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return QString();
}

} // namespace

class TestE2EEpgJournaline : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    // bkk
    void testBkkEpg_ReservedSubchannelYieldsNoSchedule();
    void testBkkTepg_FramingAndInventory();

    // HR
    void testHr_JournalineStreamText();

    // GUI-agnostic data model
    void testDataServiceStore_AggregatesDecodedContent();

private:
    QString m_bkk;
    QString m_hr;
    QByteArray m_bkkEpgRaw;   // SCId 14 (32 kb/s)
    QByteArray m_bkkTepgRaw;  // SCId 22 (8 kb/s)
    QByteArray m_hrJrnlRaw;   // HR SCId 8 (32 kb/s)
};

void TestE2EEpgJournaline::initTestCase()
{
    m_bkk = findFixture(QStringLiteral("bkk_20062022_141637.eti"));
    m_hr = findFixture(QStringLiteral(
        "HessischerRundfunk-Warntag2024-2024-09-12_journalline/"
        "hr_20240912T105801_EWS_Start.eti"));
    if (m_bkk.isEmpty() || m_hr.isEmpty()) {
        QSKIP("Real ETI fixtures for EPG/Journaline/TPEG not found locally");
    }

    EtiScanResult bkk;
    QVERIFY2(scanEtiFile(m_bkk, bkk), "bkk capture must scan");
    const ScannedSubchannel* epg = bkk.find(14);
    const ScannedSubchannel* tepg = bkk.find(22);
    QVERIFY2(epg && tepg, "bkk must have EPG SCId 14 and TEPG SCId 22");
    m_bkkEpgRaw = epg->merged_bytes;
    m_bkkTepgRaw = tepg->merged_bytes;

    // HR: 10-minute file; 4000 frames is ample to capture the carousel while
    // keeping the scan fast.
    EtiScanResult hr;
    QVERIFY2(scanEtiFile(m_hr, hr, 4000), "HR capture must scan");
    const ScannedSubchannel* jrnl = hr.find(8);
    QVERIFY2(jrnl != nullptr, "HR SCId 8 (data subchannel) must be present");
    m_hrJrnlRaw = jrnl->merged_bytes;
    QVERIFY2(m_hrJrnlRaw.size() > 100000,
             "HR data subchannel must carry many frames");
}

void TestE2EEpgJournaline::testBkkEpg_ReservedSubchannelYieldsNoSchedule()
{
    // Honest empirical outcome: bkk declares an EPG data service (SCId 14,
    // 32 kb/s) but the captured window carries no decodable TS 102 371 EPG.
    // The raw stream must be REJECTED by the EPG decoder (no fabricated
    // events), and the fixed idle-padding pattern must be observable.
    epg::EPGDecoder decoder;
    QSignalSpy errorSpy(&decoder, &epg::EPGDecoder::epgDecodingError);
    const bool accepted = decoder.processMOTObject(m_bkkEpgRaw);
    QVERIFY2(!accepted,
             "the bkk SCId 14 idle stream must not parse as an EPG schedule");
    QVERIFY2(errorSpy.count() > 0,
             "the decoder must report the rejection honestly");
    QCOMPARE(decoder.eventCount(), 0);
    QCOMPARE(decoder.getServiceIds().size(), 0);

    // Empirically verify the idle CIF signature: the large majority of
    // 96-byte frames begin with the padding byte 0xCC (idle capacity).
    int ccFrames = 0;
    const int total = static_cast<int>(m_bkkEpgRaw.size() / 96);
    for (int i = 0; i < total; ++i) {
        const int off = i * 96;
        if (static_cast<quint8>(m_bkkEpgRaw.at(off)) == 0xCC) {
            ++ccFrames;
        }
    }
    QVERIFY2(ccFrames >= total / 2,
             qPrintable(QString("bkk EPG idle 0xCC frames: %1/%2")
                            .arg(ccFrames).arg(total)));
}

void TestE2EEpgJournaline::testBkkTepg_FramingAndInventory()
{
    tpeg::TpegDecoder dec;
    dec.processStreamData(m_bkkTepgRaw);
    dec.finish();

    // Framing: no 0x0A 0x46 sync; the observed carousel markers are detected.
    QVERIFY2(!tpeg::TpegDecoder::containsSyncPair(m_bkkTepgRaw),
             "no TS 102 894-1 sync in the bkk TEPG stream (empirical)");
    QVERIFY2(tpeg::TpegDecoder::detectsCarouselFraming(m_bkkTepgRaw),
             "the observed TPEG2 carousel framing must be detected");
    QVERIFY(dec.framingDescription().contains("carousel"));

    // Inventory: the real application set + real messages.
    QVERIFY2(dec.messageCount() >= 20,
             qPrintable(QString("TEPG unique messages: %1")
                            .arg(dec.messageCount())));
    bool banner = false;
    bool version = false;
    bool parking = false;
    for (const tpeg::TpegMessage& m : dec.messages()) {
        if (m.text.contains("TEC") && m.text.contains("TFP") &&
            m.text.contains("PKI")) {
            banner = true;
        }
        if (m.text.contains("R-3.1")) {
            version = true;
        }
        if (m.text.contains("Parking", Qt::CaseInsensitive)) {
            parking = true;
        }
    }
    QVERIFY(banner);
    QVERIFY(version);
    QVERIFY(parking);
}

void TestE2EEpgJournaline::testHr_JournalineStreamText()
{
    journaline::JournalineDecoder dec;
    const bool produced = dec.processStreamData(m_hrJrnlRaw);
    QVERIFY2(produced || dec.streamItemCount() > 0,
             "the HR Journaline stream-mode carousel must produce text items");

    const int hrItemCount = dec.streamItemCount();
    const int hrCandidateCount = dec.streamCandidateCount();
    qInfo().noquote() << QString("HR Journaline stream items: %1 (candidates %2, pass rate %3)")
                             .arg(hrItemCount)
                             .arg(hrCandidateCount)
                             .arg(hrCandidateCount > 0
                                      ? QString::number(
                                            double(hrItemCount) / hrCandidateCount, 'f', 3)
                                      : QStringLiteral("n/a"));
    QVERIFY2(hrItemCount >= 20,
             qPrintable(QString("HR Journaline stream items: %1")
                            .arg(hrItemCount)));

    // A-H1: assert the plausibility gate really filters, not just that text
    // survives. The raw >=6-byte scan accepts binary runs as well (measured on
    // this capture before the gate: ~11.4k candidates, ~71 % of them non-text),
    // so (a) the gate must reject a strict superset of what it keeps, (b) it
    // must never invent items, and (c) the clean set must stay a meaningful
    // share of the candidates (floor 5 %; the measured rate is logged above
    // and documented in docs/EPG_JOURNALINE_TPEG_VALIDATION.md §1.2).
    QVERIFY2(hrCandidateCount > hrItemCount,
             qPrintable(QString("the gate must reject binary runs: candidates %1 vs clean %2")
                            .arg(hrCandidateCount).arg(hrItemCount)));
    QVERIFY2(hrItemCount * 20 >= hrCandidateCount,
             qPrintable(QString("clean pass rate below 5 %%: %1 of %2")
                            .arg(hrItemCount).arg(hrCandidateCount)));

    bool sawHr4 = false;
    bool sawArticle = false;
    for (const journaline::JournalineObject& obj : dec.getStreamItems()) {
        if (obj.title.contains("hr4") || obj.text_content.contains("hr4")) {
            sawHr4 = true;
        }
        if (obj.text_content.contains("ARD-Hitnacht") ||
            obj.text_content.contains("ARD") ||
            obj.title.contains("ARD-Hitnacht")) {
            sawArticle = true;
        }
    }
    QVERIFY2(sawHr4, "the real HR Journaline carousel mentions hr4");
    QVERIFY2(sawArticle,
             "the real HR Journaline carousel carries an ARD article text");
}

void TestE2EEpgJournaline::testDataServiceStore_AggregatesDecodedContent()
{
    // Exercise the GUI-agnostic per-service data model with real decoded data.
    data::DataServiceStore store;

    tpeg::TpegDecoder tpeg;
    tpeg.processStreamData(m_bkkTepgRaw);
    tpeg.finish();

    journaline::JournalineDecoder jrnl;
    jrnl.processStreamData(m_hrJrnlRaw);

    data::ServiceDataSnapshot tepgSnap;
    tepgSnap.sub_channel_id = 22;
    tepgSnap.service_id = 0xf320;   // bkk ensemble SId for the data services
    tepgSnap.service_label = QStringLiteral("TEPG");
    tepgSnap.stream_bytes = static_cast<quint64>(m_bkkTepgRaw.size());
    tepgSnap.stream_frames = tpeg.frameCount();
    tepgSnap.tpeg_messages = tpeg.messageCount();
    tepgSnap.tpeg_framing = tpeg.framingDescription();
    tepgSnap.tpeg_sync_0a46 = tpeg.sawSyncBytes();
    store.updateService(tepgSnap);

    data::ServiceDataSnapshot jrnlSnap;
    jrnlSnap.sub_channel_id = 8;
    jrnlSnap.service_label = QStringLiteral("Journaline (hr)");
    jrnlSnap.stream_bytes = static_cast<quint64>(m_hrJrnlRaw.size());
    jrnlSnap.journaline_stream_items = jrnl.streamItemCount();
    store.updateService(jrnlSnap);

    QCOMPARE(store.serviceCount(), 2);

    data::ServiceDataSnapshot out;
    QVERIFY(store.getService(22, out));
    QCOMPARE(out.tpeg_framing, tepgSnap.tpeg_framing);
    QVERIFY(out.tpeg_messages >= 20);
    QVERIFY(!out.tpeg_sync_0a46);
    QVERIFY(out.hasContent());

    QVERIFY(store.getService(8, out));
    QVERIFY(out.journaline_stream_items >= 20);
    QVERIFY(out.hasContent());

    const QVector<data::ServiceDataSnapshot> services = store.getServices();
    QCOMPARE(services.size(), 2);

    store.clear();
    QCOMPARE(store.serviceCount(), 0);
}

QTEST_GUILESS_MAIN(TestE2EEpgJournaline)
#include "test_e2e_epg_journaline.moc"