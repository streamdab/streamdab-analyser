/**
 * @file test_dabplus_audio_decoder.cpp
 * @brief T34: headless DAB+ audio decode tests (no sound device required).
 *
 * Drives the real Bangkok fixture through the production ETI frame parser and
 * per-sub-channel MSC slice extraction, assembles DAB+ superframes, extracts
 * the Access Units (ETSI TS 102 563) and decodes them with libfaad2. Asserts:
 *   - the decoder produces non-silent PCM for a known fixture sub-channel,
 *   - the output format (sample rate / channels / frame alignment) is plausible,
 *   - two different DAB+ sub-channels decode to DIFFERENT PCM (switching the
 *     selected service changes the decoded stream identity),
 *   - garbage MSC is rejected by the FireCode/AU-CRC extraction.
 *
 * The fixture-independent extraction invariant is covered here; the widget-level
 * wiring (Audio tab, enabled state) lives in test_gui_dock_layout.
 *
 * @date October 2026
 */

#include <QtTest/QtTest>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <vector>

#include "core/dabplus_audio_decoder.hpp"
#include "core/dabplus_superframe.hpp"
#include "core/crc16.hpp"
#include "core/enhanced_eti_processor_qt.h"  // ETIFrameParser

using streamdab::audio::DabPlusAudioDecoder;
using streamdab::audio::DabPlusFormat;
// T38: build the crafted superframes with the SAME FireCode implementation the
// decoder uses (the shared transport module), so this test exercises the
// shared code path rather than a private copy.
using streamdab::dabplus::firecodeCrc;

namespace {

// One raw DAB+ Access Unit (payload + its 2-byte AU CRC).
QByteArray makeAccessUnit(int payloadLen, char fill)
{
    QByteArray au(payloadLen + 2, fill);
    const uint16_t crc = static_cast<uint16_t>(~eti::crc16ccitt_false(
        reinterpret_cast<const uint8_t*>(au.constData()),
        static_cast<size_t>(payloadLen)));
    au[payloadLen] = static_cast<char>((crc >> 8) & 0xFF);
    au[payloadLen + 1] = static_cast<char>(crc & 0xFF);
    return au;
}

// 120-byte DAB+ superframe with sbr=1/dacRate=0 (numAus=2, firstAuStart=5).
// AU0 = [5, auStart1), AU1 = [auStart1, 110). If `corruptAu1Payload` the AU1
// payload is flipped AFTER its CRC was computed, so only AU0 is CRC-valid.
QByteArray buildSuperframe(int auStart1, const QByteArray& au0,
                           const QByteArray& au1, bool corruptAu1Payload)
{
    QByteArray sf(120, '\0');
    uint8_t* b = reinterpret_cast<uint8_t*>(sf.data());
    b[2] = 0x30;  // sbr (0x20) + stereo core (0x10); dacRate=0 -> numAus=2
    const int n0 = (auStart1 >> 8) & 0x0F;
    const int n1 = (auStart1 >> 4) & 0x0F;
    const int n2 = auStart1 & 0x0F;
    b[3] = static_cast<uint8_t>((n0 << 4) | n1);
    b[4] = static_cast<uint8_t>(n2 << 4);
    if (au0.size() != auStart1 - 5 || au1.size() != 110 - auStart1) {
        return {};
    }
    std::memcpy(b + 5, au0.constData(), static_cast<size_t>(au0.size()));
    std::memcpy(b + auStart1, au1.constData(), static_cast<size_t>(au1.size()));
    if (corruptAu1Payload) {
        b[auStart1] = static_cast<uint8_t>(b[auStart1] ^ 0xFF);
    }
    const uint16_t fc = firecodeCrc(b + 2, 9);
    b[0] = static_cast<uint8_t>((fc >> 8) & 0xFF);
    b[1] = static_cast<uint8_t>(fc & 0xFF);
    return sf;
}

} // namespace

class TestDabPlusAudioDecoder : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testExtractionRejectsGarbage();
    void testAuCrcRejectsCorruptedAccessUnit();
    void testDecodeProducesNonSilentPcm();
    void testServiceSwitchChangesDecodedStream();

private:
    struct DecodedStream {
        QByteArray pcm;
        int sampleRate = 0;
        int channels = 0;
        int accessUnits = 0;
        int nonZeroSamples = 0;
        DabPlusFormat format;
    };

    static int countNonZero(const QByteArray& pcm)
    {
        const int16_t* samples = reinterpret_cast<const int16_t*>(pcm.constData());
        const int count = pcm.size() / 2;
        int nonZero = 0;
        for (int i = 0; i < count; ++i) {
            if (samples[i] != 0) {
                ++nonZero;
            }
        }
        return nonZero;
    }

    // Captured once (heavy fixture pass) and reused by the assertions.
    std::map<int, DecodedStream> m_decoded;
};

void TestDabPlusAudioDecoder::initTestCase()
{
    if (!DabPlusAudioDecoder::faadAvailable()) {
        QSKIP("libfaad2 not available in this build");
    }

    const QString fixture = QStringLiteral("../../eti/bkk_20062022_141637.eti");
    QVERIFY2(QFileInfo::exists(fixture),
             qPrintable(QString("fixture missing: %1 (cwd=%2)")
                            .arg(fixture, QDir::currentPath())));

    QFile file(fixture);
    QVERIFY2(file.open(QIODevice::ReadOnly), "cannot open fixture");
    const QByteArray raw = file.readAll();
    file.close();
    QVERIFY2(raw.size() >= 6144, "fixture too small");

    constexpr int kFrameSize = 6144;
    constexpr int kMaxFrames = 800;  // ~19 s of audio, far beyond one superframe
    const int frames = qMin<int>(raw.size() / kFrameSize, kMaxFrames);

    ETIFrameParser parser;
    std::map<int, std::unique_ptr<DabPlusAudioDecoder>> extractors;
    std::map<int, std::vector<QByteArray>> accessUnits;

    for (int i = 0; i < frames; ++i) {
        const QByteArray frame = raw.mid(i * kFrameSize, kFrameSize);
        ETIFrameData parsed;
        if (!parser.parseFrame(frame, parsed)) {
            continue;
        }
        for (const SubChannelData& sc : parsed.sub_channels) {
            if (sc.data.isEmpty()) {
                continue;
            }
            auto it = extractors.find(sc.sub_channel_id);
            if (it == extractors.end()) {
                it = extractors.emplace(sc.sub_channel_id,
                                        std::make_unique<DabPlusAudioDecoder>()).first;
            }
            std::vector<QByteArray>& aus = accessUnits[sc.sub_channel_id];
            it->second->feedChunk(sc.data,
                                  [&aus](const QByteArray& au) { aus.push_back(au); });
        }
    }

    int decodedStreams = 0;
    for (const auto& entry : accessUnits) {
        const int subChannelId = entry.first;
        const std::vector<QByteArray>& aus = entry.second;
        if (aus.size() < 20) {
            continue;  // not enough for a meaningful stream
        }
        DabPlusAudioDecoder decoder;
        const DabPlusFormat fmt = extractors[subChannelId]->format();
        if (!decoder.beginDecode(fmt)) {
            continue;
        }
        QByteArray pcm;
        for (const QByteArray& au : aus) {
            decoder.decodeAu(au, pcm);
        }
        // Capture the real output format BEFORE endDecode() releases the handle
        // (endDecode resets the reported rate/channels to 0).
        const int outSampleRate = decoder.outputSampleRate();
        const int outChannels = decoder.outputChannels();
        decoder.endDecode();
        if (pcm.isEmpty()) {
            continue;
        }
        DecodedStream stream;
        stream.pcm = pcm;
        stream.sampleRate = outSampleRate;
        stream.channels = outChannels;
        stream.accessUnits = static_cast<int>(aus.size());
        stream.nonZeroSamples = countNonZero(pcm);
        stream.format = fmt;
        m_decoded.emplace(subChannelId, stream);
        ++decodedStreams;
        qInfo() << "[T34] subchannel" << subChannelId << fmt.profile()
                << fmt.bitrateKbps << "kbps" << "AUs:" << aus.size()
                << "sr:" << stream.sampleRate << "ch:" << stream.channels
                << "nonZero:" << stream.nonZeroSamples;
    }
    qInfo() << "[T34] decoded DAB+ subchannels:" << decodedStreams;
}

void TestDabPlusAudioDecoder::testExtractionRejectsGarbage()
{
    DabPlusAudioDecoder decoder;
    QByteArray zeroChunk(96, '\0');
    QByteArray garbage;
    for (int i = 0; i < 96; ++i) {
        garbage.append(static_cast<char>((i * 37 + 11) & 0xFF));
    }
    int aus = 0;
    for (int i = 0; i < 10; ++i) {
        decoder.feedChunk(zeroChunk, [&aus](const QByteArray&) { ++aus; });
        decoder.feedChunk(garbage, [&aus](const QByteArray&) { ++aus; });
    }
    QCOMPARE(aus, 0);
    QCOMPARE(decoder.superframesAccepted(), 0u);
    QCOMPARE(decoder.accessUnitsAccepted(), 0u);
}

// F10: a crafted superframe with a VALID FireCode but a corrupted AU CRC must
// accept only the intact Access Unit (the AU-CRC branch must be exercised).
void TestDabPlusAudioDecoder::testAuCrcRejectsCorruptedAccessUnit()
{
    const int auStart1 = 60;
    const QByteArray au0 = makeAccessUnit(auStart1 - 5 - 2, static_cast<char>(0x11));
    const QByteArray au1 = makeAccessUnit(110 - auStart1 - 2, static_cast<char>(0x22));

    // Sanity: with both CRCs correct, both AUs are accepted.
    {
        const QByteArray sf = buildSuperframe(auStart1, au0, au1, false);
        QVERIFY(!sf.isEmpty());
        DabPlusAudioDecoder decoder;
        int emitted = 0;
        for (int i = 0; i < 5; ++i) {
            decoder.feedChunk(sf.mid(i * 24, 24),
                              [&emitted](const QByteArray&) { ++emitted; });
        }
        QCOMPARE(decoder.superframesAccepted(), 1u);
        QCOMPARE(decoder.accessUnitsAccepted(), 2u);
        QCOMPARE(emitted, 2);
    }

    // Corrupt AU1's payload after its CRC was computed: only AU0 is accepted.
    {
        const QByteArray sf = buildSuperframe(auStart1, au0, au1, true);
        QVERIFY(!sf.isEmpty());
        DabPlusAudioDecoder decoder;
        int emitted = 0;
        QByteArray lastAu;
        for (int i = 0; i < 5; ++i) {
            decoder.feedChunk(sf.mid(i * 24, 24),
                              [&emitted, &lastAu](const QByteArray& au) {
                                  ++emitted;
                                  lastAu = au;
                              });
        }
        QCOMPARE(decoder.superframesAccepted(), 1u);
        QCOMPARE(decoder.accessUnitsAccepted(), 1u);
        QCOMPARE(emitted, 1);
        // The callback receives the AU payload WITHOUT the trailing CRC bytes.
        QCOMPARE(lastAu, au0.left(au0.size() - 2));
    }
}

void TestDabPlusAudioDecoder::testDecodeProducesNonSilentPcm()
{
    QVERIFY2(!m_decoded.empty(),
             "no DAB+ sub-channel decoded to PCM from the fixture");

    const DecodedStream* best = nullptr;
    for (const auto& entry : m_decoded) {
        if (!best || entry.second.nonZeroSamples > best->nonZeroSamples) {
            best = &entry.second;
        }
    }
    QVERIFY(best);
    QVERIFY2(best->nonZeroSamples > 0, "decoded PCM must contain non-zero samples");
    QVERIFY2(best->accessUnits >= 20, "expected several Access Units");
    QVERIFY2(best->sampleRate == 24000 || best->sampleRate == 32000
                 || best->sampleRate == 48000,
             qPrintable(QString("implausible output sample rate %1")
                            .arg(best->sampleRate)));
    QVERIFY2(best->channels == 1 || best->channels == 2,
             qPrintable(QString("implausible channel count %1").arg(best->channels)));
    QVERIFY2(best->channels > 0
                 && best->pcm.size() % (best->channels * 2) == 0,
             "PCM length must be a whole number of interleaved frames");
}

void TestDabPlusAudioDecoder::testServiceSwitchChangesDecodedStream()
{
    std::vector<const DecodedStream*> nonSilent;
    for (const auto& entry : m_decoded) {
        if (entry.second.nonZeroSamples > 0 && entry.second.pcm.size() > 4096) {
            nonSilent.push_back(&entry.second);
        }
    }
    QVERIFY2(nonSilent.size() >= 2,
             qPrintable(QString("need >=2 non-silent DAB+ subchannels, got %1")
                            .arg(nonSilent.size())));

    const DecodedStream& a = *nonSilent[0];
    const DecodedStream& b = *nonSilent[1];
    QVERIFY2(a.pcm != b.pcm,
             "two different DAB+ services must decode to different PCM");
    // Sanity: both streams are independently non-silent.
    QVERIFY(a.nonZeroSamples > 0);
    QVERIFY(b.nonZeroSamples > 0);
}

QTEST_MAIN(TestDabPlusAudioDecoder)
#include "test_dabplus_audio_decoder.moc"
