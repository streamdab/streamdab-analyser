/**
 * @file test_e2e_mot_slideshow.cpp
 * @brief End-to-end MOT SlideShow pipeline test.
 *
 * Exercises the real decode path: a standard EN 300 401 MSC data group set
 * (MOT header + body entities, as carried in DAB+ X-PAD CI 12/13) is fed
 * through MotPadAdapter -> MOTProtocol; the completed object's body must
 * decode to a non-null QImage.
 *
 * The adapter-level X-PAD framing (CI 1 DGLI + CI 12/13) is covered by
 * tests/gui/test_gui_dock_layout.cpp (testMotPadAdapterXpadPipeline,
 * testMotXpadCiFlagZeroContinuation), which compiles the real PAD parser, and
 * the real Bangkok capture is covered by testMotSlideshowRealFixture. This E2E
 * test uses a small synthetic PNG on the decoder side and asserts an image
 * actually decodes.
 *
 * @date October 2026
 */

#include <QtTest/QtTest>
#include <QImage>
#include <QBuffer>
#include <QByteArray>
#include <QSignalSpy>
#include <cstdint>

#include "core/mot_protocol.hpp"
#include "core/mot_pad_adapter.hpp"
#include "core/crc16.hpp"

namespace {

// 7-byte standard MOT header core: body size(28) header size(13) type(6) sub(9).
QByteArray headerCore(uint32_t bodySize, uint8_t contentType, uint16_t subType)
{
    QByteArray h(7, '\0');
    auto* d = reinterpret_cast<uint8_t*>(h.data());
    d[0] = static_cast<uint8_t>((bodySize >> 20) & 0xFF);
    d[1] = static_cast<uint8_t>((bodySize >> 12) & 0xFF);
    d[2] = static_cast<uint8_t>((bodySize >> 4) & 0xFF);
    d[3] = static_cast<uint8_t>(((bodySize & 0x0F) << 4) | ((7u >> 9) & 0x0F));
    d[4] = static_cast<uint8_t>((7u >> 1) & 0xFF);
    d[5] = static_cast<uint8_t>(((7u & 0x01) << 7) |
                                ((contentType & 0x3F) << 1) |
                                ((subType >> 8) & 0x01));
    d[6] = static_cast<uint8_t>(subType & 0xFF);
    return h;
}

QByteArray standardDataGroup(uint8_t dgType, uint16_t tid, uint16_t seg, bool last,
                             const QByteArray& payload)
{
    QByteArray dg;
    dg.append(static_cast<char>(0x40 | 0x20 | 0x10 | (dgType & 0x0F)));
    dg.append(static_cast<char>(0x00));
    dg.append(static_cast<char>((last ? 0x80 : 0x00) | ((seg >> 8) & 0x7F)));
    dg.append(static_cast<char>(seg & 0xFF));
    dg.append(static_cast<char>(0x10 | 0x02));
    dg.append(static_cast<char>((tid >> 8) & 0xFF));
    dg.append(static_cast<char>(tid & 0xFF));
    dg.append(static_cast<char>((payload.size() >> 8) & 0x1F));
    dg.append(static_cast<char>(payload.size() & 0xFF));
    dg.append(payload);
    const uint16_t crc = static_cast<uint16_t>(~eti::crc16ccitt_false(
        reinterpret_cast<const uint8_t*>(dg.constData()), dg.size()));
    dg.append(static_cast<char>((crc >> 8) & 0xFF));
    dg.append(static_cast<char>(crc & 0xFF));
    return dg;
}

QByteArray makePng(int w, int h)
{
    QImage img(w, h, QImage::Format_RGB32);
    img.fill(Qt::blue);
    QByteArray ba;
    QBuffer buf(&ba);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");
    return ba;
}

}  // namespace

class TestE2EMotSlideshow : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void testAdapterDecodesImage();
    void testMultiSegmentBodyDecodesImage();
    void testParseErrorDoesNotComplete();
};

void TestE2EMotSlideshow::initTestCase()
{
    qRegisterMetaType<eti::mot::MOTObject>("eti::mot::MOTObject");
}

void TestE2EMotSlideshow::testAdapterDecodesImage()
{
    const QByteArray png = makePng(32, 24);
    QVERIFY2(!png.isEmpty(), "test PNG must encode");

    eti::mot::MOTProtocol mot;
    eti::mot::MotPadAdapter adapter(&mot);
    QSignalSpy spy(&mot, &eti::mot::MOTProtocol::objectComplete);
    QVERIFY(spy.isValid());

    const uint16_t tid = 0x0123;
    QVERIFY(adapter.processDataGroup(0x2C61,
        standardDataGroup(3, tid, 0, true, headerCore(png.size(), 0x02, 0x03))));
    QVERIFY(adapter.processDataGroup(0x2C61,
        standardDataGroup(4, tid, 0, true, png)));

    QCOMPARE(spy.count(), 1);
    const eti::mot::MOTObject object =
        qvariant_cast<eti::mot::MOTObject>(spy.takeFirst().at(1));
    QVERIFY(!object.body.empty());

    QImage image;
    QVERIFY2(image.loadFromData(reinterpret_cast<const uchar*>(object.body.data()),
                                static_cast<int>(object.body.size()), "PNG"),
             "the MOT object body must decode to a QImage");
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), 32);
    QCOMPARE(image.height(), 24);

    const eti::mot::MotPadAdapter::Statistics st = adapter.statistics();
    QCOMPARE(st.objectsCompleted, static_cast<uint64_t>(1));
    QCOMPARE(st.objectsEmitted, static_cast<uint64_t>(1));
    QCOMPARE(st.crcErrors, static_cast<uint64_t>(0));
}

void TestE2EMotSlideshow::testMultiSegmentBodyDecodesImage()
{
    const QByteArray png = makePng(40, 30);
    QVERIFY(!png.isEmpty());

    eti::mot::MOTProtocol mot;
    eti::mot::MotPadAdapter adapter(&mot);
    QSignalSpy spy(&mot, &eti::mot::MOTProtocol::objectComplete);

    const uint16_t tid = 0x0055;
    QVERIFY(adapter.processDataGroup(0,
        standardDataGroup(3, tid, 0, true, headerCore(png.size(), 0x02, 0x03))));

    // Split the body into three standard segments; last flag on segment 2.
    const QByteArray s0 = png.left(png.size() / 3);
    const QByteArray s1 = png.mid(s0.size(), png.size() / 3);
    const QByteArray s2 = png.mid(s0.size() + s1.size());
    QVERIFY(adapter.processDataGroup(0, standardDataGroup(4, tid, 0, false, s0)));
    QVERIFY(adapter.processDataGroup(0, standardDataGroup(4, tid, 1, false, s1)));
    QVERIFY(adapter.processDataGroup(0, standardDataGroup(4, tid, 2, true, s2)));

    QCOMPARE(spy.count(), 1);
    const eti::mot::MOTObject object =
        qvariant_cast<eti::mot::MOTObject>(spy.takeFirst().at(1));
    QImage image;
    QVERIFY(image.loadFromData(reinterpret_cast<const uchar*>(object.body.data()),
                               static_cast<int>(object.body.size()), "PNG"));
    QCOMPARE(image.width(), 40);
    QCOMPARE(image.height(), 30);
}

void TestE2EMotSlideshow::testParseErrorDoesNotComplete()
{
    // A body data group with a corrupted CRC must be rejected by the adapter
    // and must never reach objectComplete.
    eti::mot::MOTProtocol mot;
    eti::mot::MotPadAdapter adapter(&mot);
    QSignalSpy spy(&mot, &eti::mot::MOTProtocol::objectComplete);

    const QByteArray png = makePng(16, 16);
    QByteArray bad = standardDataGroup(4, 7, 0, true, png);
    bad[bad.size() - 1] = static_cast<char>(bad[bad.size() - 1] ^ 0xFF);
    QVERIFY(!adapter.processDataGroup(0, bad));
    QCOMPARE(spy.count(), 0);
    QCOMPARE(adapter.statistics().crcErrors, static_cast<uint64_t>(1));
}

QTEST_GUILESS_MAIN(TestE2EMotSlideshow)
#include "test_e2e_mot_slideshow.moc"
