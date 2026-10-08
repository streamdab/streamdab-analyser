/**
 * @file test_tpeg_framing.cpp
 * @brief PUBLIC, capture-free unit tests for the TPEG/TEPG framing helpers.
 *
 * Covers the self-contained detection helpers of TpegDecoder:
 *   - carousel marker-phase detection (the 24-byte frame markers observed in
 *     the Bangkok TEPG capture are synthesised here, exactly as observed:
 *     `10 01 13`, `20 01 13`, `30 01 13`, `00 01 13`);
 *   - TS 102 894-1 sync-pair (0x0A 0x46) scanning;
 *   - a synthetic stream whose text inventory must be deduplicated;
 *   - N2: multi-feed lifecycle — several feeds into ONE shared decoder with a
 *     single finish() at the end (both feeds' messages must survive), and the
 *     refreshInventory() mid-stream view used for per-subchannel deltas.
 *
 * No capture is required; these tests run in PUBLIC CI.
 *
 * @date October 2026
 */

#include <QtTest/QtTest>
#include "../src/core/tpeg_decoder.hpp"

using namespace eti::tpeg;

class TestTpegFraming : public QObject {
    Q_OBJECT

private slots:
    void testDetectsCarouselFraming_ObservedMarkers();
    void testDetectsCarouselFraming_TooShort();
    void testDetectsCarouselFraming_WrongMarkers();
    void testSyncPair_Present();
    void testSyncPair_Absent();
    void testTextInventory_Deduplication();
    void testStats_EmptyDecoder();
    // N2: multi-feed lifecycle on ONE shared decoder.
    void testMultiFeed_FinishOnce_BothFeedsPresent();
    void testRefreshInventory_PerFeedDelta();
};

// Build the exact 4-phase marker page observed in the Bangkok TEPG capture.
static QByteArray observedPage()
{
    QByteArray page;
    const QByteArray m10 = QByteArray::fromHex("100113");
    const QByteArray m20 = QByteArray::fromHex("200113");
    const QByteArray m30 = QByteArray::fromHex("300113");
    const QByteArray m00 = QByteArray::fromHex("000113");
    // Each marker consumes 3 bytes when appearing at a 24-byte offset the
    // heuristic matches on the first 3 bytes of a 24-byte block.
    auto frame24 = [](const QByteArray& head3) {
        QByteArray f(24, char(0));
        f.replace(0, 3, head3);
        return f;
    };
    page.append(frame24(m10));
    page.append(frame24(m20));
    page.append(frame24(m30));
    page.append(frame24(m00));
    return page;
}

void TestTpegFraming::testDetectsCarouselFraming_ObservedMarkers()
{
    const QByteArray page = observedPage();
    const QByteArray twoPages = page + page;
    QVERIFY(TpegDecoder::detectsCarouselFraming(twoPages));

    // A single page (4 frames) is also enough (>= 2 marker phases).
    QVERIFY(TpegDecoder::detectsCarouselFraming(page));
}

void TestTpegFraming::testDetectsCarouselFraming_TooShort()
{
    QVERIFY(!TpegDecoder::detectsCarouselFraming(QByteArray(32, '\0')));
    QVERIFY(!TpegDecoder::detectsCarouselFraming(QByteArray()));
}

void TestTpegFraming::testDetectsCarouselFraming_WrongMarkers()
{
    // All-zero 24-byte frames: no phase markers.
    QByteArray zeros;
    for (int i = 0; i < 8; ++i) {
        zeros.append(QByteArray(24, '\0'));
    }
    QVERIFY(!TpegDecoder::detectsCarouselFraming(zeros));

    // A valid marker prefix but with a wrong low nibble is still accepted
    // (group match), so craft one that fails on the trailing "0113" marker.
    QByteArray wrong;
    for (int i = 0; i < 8; ++i) {
        QByteArray f(24, '\0');
        f[0] = char(0x4A);  // not in {0x00,0x10,0x20,0x30} groups
        f[1] = char(0x01);
        f[2] = char(0x13);
        wrong.append(f);
    }
    QVERIFY(!TpegDecoder::detectsCarouselFraming(wrong));
}

void TestTpegFraming::testSyncPair_Present()
{
    QVERIFY(TpegDecoder::containsSyncPair(QByteArray::fromHex("0a46")));
    QByteArray withSync;
    withSync.append(QByteArray(40, '\0'));
    withSync.append(QByteArray::fromHex("0a46"));
    QVERIFY(TpegDecoder::containsSyncPair(withSync));
}

void TestTpegFraming::testSyncPair_Absent()
{
    QVERIFY(!TpegDecoder::containsSyncPair(QByteArray()));
    QVERIFY(!TpegDecoder::containsSyncPair(QByteArray(64, '\x41')));
}

void TestTpegFraming::testTextInventory_Deduplication()
{
    // A tiny stream that repeats one visible message several times must yield
    // exactly one inventory entry with a repeat count > 1.
    QByteArray stream;
    const QByteArray text = "ACME 12 Traffic Update";
    for (int i = 0; i < 3; ++i) {
        stream.append(QByteArray(24, '\0'));
        stream.append(text);
    }
    TpegDecoder dec;
    dec.processStreamData(stream);
    dec.finish();
    QCOMPARE(dec.messageCount(), 1);
    QCOMPARE(dec.messages().size(), 1);
    QVERIFY2(dec.messages().at(0).repeat_count >= 3,
             "the repeated text must be counted 3x");
    QVERIFY(dec.messages().at(0).text.contains("Traffic Update"));
}

void TestTpegFraming::testStats_EmptyDecoder()
{
    TpegDecoder dec;
    TpegDecoder::Statistics st = dec.getStatistics();
    QCOMPARE(st.frames, static_cast<quint64>(0));
    QCOMPARE(st.unique_messages, 0);
    QVERIFY(!st.sync_0a46);
    QCOMPARE(dec.frameCount(), static_cast<quint64>(0));
    QCOMPARE(dec.messageCount(), 0);
}

// One bounded feed carrying a single visible text: the shape of one
// TPEG-routed subchannel's bytes (bounded chunks per feed in main.cpp).
static QByteArray feedWithText(const QByteArray& text)
{
    QByteArray feed;
    feed.append(QByteArray(24, '\0'));
    feed.append(text);
    feed.append(QByteArray(24, '\0'));
    return feed;
}

void TestTpegFraming::testMultiFeed_FinishOnce_BothFeedsPresent()
{
    // N2 regression: two feeds into ONE shared decoder, finish() called ONCE
    // after the last feed — exactly the shape of the per-subchannel loop in
    // MainWindow::feedDataServiceSubchannels(). Calling finish() after the
    // FIRST feed (the old behaviour) put the decoder in m_finished state, so
    // the 2nd feed's bytes were dropped with the one-shot post-finish warning
    // and its message never reached the inventory.
    const QByteArray text1 = "Bangkok Traffic Hotspot Alpha";
    const QByteArray text2 = "Khwaeng Samsen Parking Closed";
    const QByteArray feed1 = feedWithText(text1);
    const QByteArray feed2 = feedWithText(text2);

    TpegDecoder dec;
    dec.processStreamData(feed1);
    dec.processStreamData(feed2);
    dec.finish();   // once, at the very end

    const QVector<TpegMessage> messages = dec.messages();
    QCOMPARE(messages.size(), 2);
    QStringList texts;
    for (const TpegMessage& m : messages) {
        texts.append(m.text);
    }
    QVERIFY2(texts.contains(QString::fromUtf8(text1)),
             "the FIRST feed's message must be in the inventory");
    QVERIFY2(texts.contains(QString::fromUtf8(text2)),
             "the SECOND feed's message must be in the inventory "
             "(an early finish() dropped its bytes)");

    // The two-feed inventory must equal a single-shot feed of the same bytes.
    TpegDecoder single;
    single.processStreamData(feed1 + feed2);
    single.finish();
    QCOMPARE(single.messageCount(), dec.messageCount());
    QStringList singleTexts;
    for (const TpegMessage& m : single.messages()) {
        singleTexts.append(m.text);
    }
    QCOMPARE(texts, singleTexts);
}

void TestTpegFraming::testRefreshInventory_PerFeedDelta()
{
    // N2 attribution helper: refreshInventory() gives a consistent mid-stream
    // view of the shared inventory WITHOUT closing the decoder, which is what
    // lets a caller compute "what did THIS subchannel add?" (delta =
    // countAfter - countBefore). Without it, messageCount() after a feed
    // still reports the previous finish()'s (here: empty) inventory.
    const QByteArray text1 = "Bangkok Traffic Hotspot Alpha";
    const QByteArray text2 = "Khwaeng Samsen Parking Closed";

    TpegDecoder dec;
    dec.processStreamData(feedWithText(text1));
    QCOMPARE(dec.messageCount(), 0);   // materialised only by refresh/finish
    const int afterFeed1 = dec.refreshInventory();
    QCOMPARE(afterFeed1, 1);

    // refreshInventory() must NOT close the decoder: the next feed still counts.
    dec.processStreamData(feedWithText(text2));
    const int afterFeed2 = dec.refreshInventory();
    QCOMPARE(afterFeed2, 2);
    QCOMPARE(afterFeed2 - afterFeed1, 1);   // the delta the 2nd feed produced

    // finish() remains valid afterwards and yields the same inventory.
    dec.finish();
    QCOMPARE(dec.messageCount(), 2);
}

QTEST_GUILESS_MAIN(TestTpegFraming)
#include "test_tpeg_framing.moc"