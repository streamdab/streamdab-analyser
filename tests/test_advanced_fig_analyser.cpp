/**
 * @file test_advanced_fig_analyser.cpp
 * @brief Unit tests for the GUI-path FIC/FIB decode fix in
 *        AdvancedFIGAnalyser (DAB Ensemble Explorer labels).
 *
 * Covers:
 *  (a) REAL FIB bytes from eti/bkk_20062022_141637.eti frame 5 (all three
 *      FIBs): ensemble 0x2000 "Bangkok DAB+", service 0x2C61 label
 *      "RROne FM 101    " (mask 0xF870), zero FIB CRC failures.
 *  (b) FIB CRC handling: strict skip on bad CRC, fib_ignore_crc recovery,
 *      raw-mode recovery, and auto-fallback when every FIB fails.
 *  (c) Charset-6 (TIS-620) service label -> "กขฃ" (Thai UTF-8).
 *  (d) Charset-0 EBU-Latin special bytes 0x80/0xE1 -> "áÅ".
 *  (e) Input guards: empty FIC, oversized FIC (> 768 B), force_charset.
 *
 * Reference: ETSI EN 300 799 clause 5.2 / EN 300 401 clause 5.2 (FIB CRC:
 * CRC-16/CCITT-FALSE over the 30-byte FIG area, one's complement, BE).
 *
 * @author C++ Qt Developer Agent (GUI decode path fix)
 */

#include <QtTest/QtTest>
#include <cstring>

#include "core/advanced_fig_analyser.h"
#include "core/analyser_settings.hpp"
#include "core/crc16.hpp"

namespace {

// ---------------------------------------------------------------------------
// Real frame-5 FIC of eti/bkk_20062022_141637.eti (ETI-NI, NST=16, MID=1 ->
// 96-byte FIC at frame offset 76). All three FIBs verified against the file:
//   FIB1: FIG 0/0 (EId 0x2000), FIG 0/18, ... CRC 0xFAD1
//   FIB2: FIG 0/5, FIG 1/1 (SId 0x2C61 "RROne FM 101    " mask 0xF870),
//         CRC 0x89F4
//   FIB3: FIG 1/0 (EId 0x2000 "Bangkok DAB+    " mask 0x1000), FIG 0/17,
//         CRC 0x7DB9
// ---------------------------------------------------------------------------

const uint8_t kFIB1_FIG_AREA[30] = {
    0x05, 0x00, 0x20, 0x00, 0x26, 0x20, 0x16, 0x12, 0x25, 0x0B,
    0x07, 0xFF, 0x02, 0x01, 0x04, 0x25, 0x0A, 0x07, 0xFF, 0x02,
    0x01, 0x04, 0x25, 0x00, 0x07, 0xFF, 0x02, 0x01, 0x04, 0xFF
};
const uint16_t kFIB1_CRC = 0xFAD1;

const uint8_t kFIB2_FIG_AREA[30] = {
    0x05, 0x05, 0x0A, 0x09, 0x00, 0x09, 0x35, 0x01, 0x2C, 0x61,
    0x52, 0x52, 0x4F, 0x6E, 0x65, 0x20, 0x46, 0x4D, 0x20, 0x31,
    0x30, 0x31, 0x20, 0x20, 0x20, 0x20, 0xF8, 0x70, 0xFF, 0x00
};
const uint16_t kFIB2_CRC = 0x89F4;

const uint8_t kFIB3_FIG_AREA[30] = {
    0x35, 0x00, 0x20, 0x00, 0x42, 0x61, 0x6E, 0x67, 0x6B, 0x6F,
    0x6B, 0x20, 0x44, 0x41, 0x42, 0x2B, 0x20, 0x20, 0x20, 0x20,
    0x10, 0x00, 0x06, 0x11, 0x25, 0x0D, 0x20, 0x09, 0x09, 0xFF
};
const uint16_t kFIB3_CRC = 0x7DB9;

/** @brief Compose one 32-byte FIB (30-byte FIG area + complemented CRC BE). */
QByteArray makeFib(const uint8_t* fig_area, uint16_t crc)
{
    QByteArray fib(reinterpret_cast<const char*>(fig_area), 30);
    fib.append(static_cast<char>((crc >> 8) & 0xFF));
    fib.append(static_cast<char>(crc & 0xFF));
    return fib;
}

/** @brief Compute the expected (complemented) FIB CRC for a FIG area. */
uint16_t expectedFibCrc(const uint8_t* fig_area)
{
    return static_cast<uint16_t>(~eti::crc16ccitt_false(fig_area, 30));
}

/** @brief All-padding FIB (FIG type 7 stops any walk immediately). */
QByteArray makePaddingFib()
{
    uint8_t area[30];
    std::memset(area, 0xFF, sizeof(area));
    return makeFib(area, expectedFibCrc(area));
}

} // namespace

class TestAdvancedFIGAnalyser : public QObject
{
    Q_OBJECT

private slots:
    // (a) Real fixture FIBs
    void testRealFrame5FIBs();
    // (1) 19-byte FIG 1/0 without the optional character flag (no OOB)
    void testFig10ShortNoMask();
    // (b) FIB CRC handling
    void testFIBBadCRCStrict();
    void testFIBBadCRCIgnore();
    void testFIBRawMode();
    void testFIBAllBadAutoFallback();
    // (c) charset-6 TIS-620 Thai label
    void testCharset6ThaiLabel();
    // (d) charset-0 EBU-Latin specials
    void testCharset0EBULatinSpecials();
    // (e) input guards + settings overrides
    void testInputGuards();
    void testForceCharsetOverride();

private:
    static QByteArray serviceLabelFig(uint8_t charset, uint32_t sid16,
                                      const QByteArray& label16,
                                      uint16_t mask);
    static void expectService(const AdvancedFIGAnalyser& ana, uint32_t sid,
                              const QString& label, const QString& shortLabel,
                              uint16_t mask);
};

// (a) ========================================================================

void TestAdvancedFIGAnalyser::testRealFrame5FIBs()
{
    AdvancedFIGAnalyser ana;

    QByteArray fic;
    fic += makeFib(kFIB1_FIG_AREA, kFIB1_CRC);
    fic += makeFib(kFIB2_FIG_AREA, kFIB2_CRC);
    fic += makeFib(kFIB3_FIG_AREA, kFIB3_CRC);
    QCOMPARE(fic.size(), 96);

    ana.analyzeFICData(fic);

    // All three FIB CRCs are valid
    QCOMPARE(ana.getFibCrcFailureCount(), 0);
    QVERIFY(!ana.rawFicFallbackUsed());

    // Ensemble from FIG 0/0 (FIB1) + FIG 1/0 label (FIB3)
    EnsembleInfo ensemble = ana.getCurrentEnsemble();
    QCOMPARE(ensemble.ensembleId, static_cast<uint16_t>(0x2000));
    QCOMPARE(ensemble.ensembleLabel.trimmed(), QStringLiteral("Bangkok DAB+"));
    // Short label matches the CLI oracle (--show-short-labels: "g"): mask
    // 0x1000 = bit 12 -> character (15-12)=3 of "Bangkok DAB+    " = 'g'
    // (spec/etisnoop order: bit 15 = char 0).
    QCOMPARE(ensemble.shortLabel, QStringLiteral("g"));

    // Service 0x2C61 "RROne FM 101    " with mask 0xF870 (FIG 1/1 in FIB2)
    expectService(ana, 0x2C61, QStringLiteral("RROne FM 101"),
                  QStringLiteral("RROne101"), 0xF870);

    // FIG 0/0 + FIG 1/0 + FIG 1/1 (+ FIG 0/5 / 0/17, disabled by default)
    QVERIFY(ana.getTotalFIGsProcessed() >= 3);
}

// (1) ========================================================================

void TestAdvancedFIGAnalyser::testFig10ShortNoMask()
{
    AdvancedFIGAnalyser ana;

    // FIG 1/0 with the LEGAL 19-byte payload (ext + EId + 16-byte label, NO
    // optional 2-byte character flag — EN 300 401 8.1.13). The old
    // parseLabel read bytes 3..20, i.e. two bytes past the buffer; under
    // ASan this vector trips the OOB read. With the padded-field fix the
    // label is decoded and the absent mask leaves the flag at 0.
    QByteArray fig;
    fig.append(static_cast<char>((1 << 5) | 19));  // FIG type 1, length 19
    fig.append(static_cast<char>(0x00));           // charset 0 (EBU Latin), ext 0
    fig.append(static_cast<char>(0x20));           // EId MSB: 0x2000
    fig.append(static_cast<char>(0x00));           // EId LSB
    fig.append("Bangkok DAB+    ", 16);            // 16-byte label (no mask)
    QCOMPARE(fig.size(), 20);

    ana.analyzeFICData(fig);

    QCOMPARE(ana.getFibCrcFailureCount(), 0);
    QCOMPARE(ana.getCurrentEnsemble().ensembleId, static_cast<uint16_t>(0x2000));
    QCOMPARE(ana.getCurrentEnsemble().ensembleLabel.trimmed(),
             QStringLiteral("Bangkok DAB+"));
    QCOMPARE(ana.getCurrentEnsemble().shortLabel, QString());  // no mask bytes
    QCOMPARE(ana.getTotalFIGsProcessed(), 1);                  // FIG 1/0 decoded once
}

// (b) ========================================================================

void TestAdvancedFIGAnalyser::testFIBBadCRCStrict()
{
    AdvancedFIGAnalyser ana;

    // FIB1 carries the service label but its stored CRC is corrupted
    QByteArray badArea(reinterpret_cast<const char*>(kFIB2_FIG_AREA), 30);
    badArea[28] = 0x00;  // flip the padding terminator: CRC no longer matches, FIGs intact
    QByteArray fic;
    fic += makeFib(reinterpret_cast<const uint8_t*>(badArea.constData()), kFIB2_CRC);
    fic += makePaddingFib();
    fic += makePaddingFib();

    ana.analyzeFICData(fic);

    // Strict default: the corrupted FIB is skipped, the padding FIBs carry
    // no FIGs -> nothing decoded, one CRC failure counted.
    QCOMPARE(ana.getFibCrcFailureCount(), 1);
    QCOMPARE(ana.getTotalFIGsProcessed(), 0);
    QVERIFY(ana.getDABServices().empty());
}

void TestAdvancedFIGAnalyser::testFIBBadCRCIgnore()
{
    AdvancedFIGAnalyser ana;
    eti::AnalyserSettings settings = eti::AnalyserSettings::defaults();
    settings.fib_ignore_crc = true;
    ana.setAnalyserSettings(settings);

    QByteArray badArea(reinterpret_cast<const char*>(kFIB2_FIG_AREA), 30);
    badArea[28] = 0x00;  // corrupt one byte -> CRC mismatch
    QByteArray fic;
    fic += makeFib(reinterpret_cast<const uint8_t*>(badArea.constData()), kFIB2_CRC);
    fic += makePaddingFib();
    fic += makePaddingFib();

    ana.analyzeFICData(fic);

    // fib_ignore_crc: the bad FIB is still decoded, failures are counted
    QCOMPARE(ana.getFibCrcFailureCount(), 1);
    expectService(ana, 0x2C61, QStringLiteral("RROne FM 101"),
                  QStringLiteral("RROne101"), 0xF870);
}

void TestAdvancedFIGAnalyser::testFIBRawMode()
{
    AdvancedFIGAnalyser ana;
    eti::AnalyserSettings settings = eti::AnalyserSettings::defaults();
    settings.fic_mode = eti::FicModeSetting::Raw;
    ana.setAnalyserSettings(settings);

    // Same corrupted FIB bytes, but raw mode walks the whole FIC without
    // any per-FIB CRC gate.
    QByteArray badArea(reinterpret_cast<const char*>(kFIB2_FIG_AREA), 30);
    badArea[28] = 0x00;
    QByteArray fic;
    fic += makeFib(reinterpret_cast<const uint8_t*>(badArea.constData()), kFIB2_CRC);
    fic += makePaddingFib();
    fic += makePaddingFib();

    ana.analyzeFICData(fic);

    QCOMPARE(ana.getFibCrcFailureCount(), 0);
    expectService(ana, 0x2C61, QStringLiteral("RROne FM 101"),
                  QStringLiteral("RROne101"), 0xF870);
}

void TestAdvancedFIGAnalyser::testFIBAllBadAutoFallback()
{
    AdvancedFIGAnalyser ana;
    eti::AnalyserSettings settings = eti::AnalyserSettings::defaults();
    settings.fic_mode = eti::FicModeSetting::Auto;  // default
    ana.setAnalyserSettings(settings);

    // Corrupt every FIB's CRC (stored CRC = 0x0000)
    QByteArray fic;
    fic += makeFib(kFIB1_FIG_AREA, 0x0000);
    fic += makeFib(kFIB2_FIG_AREA, 0x0000);
    fic += makeFib(kFIB3_FIG_AREA, 0x0000);

    ana.analyzeFICData(fic);

    // Auto fallback: all 3 FIB CRCs fail -> the FIC is re-walked as a raw
    // FIG stream. Per-FIB padding (0xFF) terminates the raw stream walk at
    // the end of the first FIB (CLI parity: EtiFicField::decodeFigBlocks'
    // parse_fig_area stops on the type-7 padding marker), so FIB1's
    // FIG 0/0 (ensemble EId) is recovered while FIB2/FIB3 stay undecoded.
    QCOMPARE(ana.getFibCrcFailureCount(), 3);
    QVERIFY(ana.rawFicFallbackUsed());
    QCOMPARE(ana.getCurrentEnsemble().ensembleId, static_cast<uint16_t>(0x2000));
}

// (c) ========================================================================

QByteArray TestAdvancedFIGAnalyser::serviceLabelFig(uint8_t charset,
                                                    uint32_t sid16,
                                                    const QByteArray& label16,
                                                    uint16_t mask)
{
    QByteArray fig;
    // FIG 1/1 (EN 300 401 8.1.14.1): ext(1) + SId(2) + label(16) + mask(2) =
    // 21 bytes. The P/D flag is the MSB of the SId (clear for programme).
    fig.append(static_cast<char>((1 << 5) | 21));
    fig.append(static_cast<char>((charset << 4) | 0x01));  // charset + ext 1
    fig.append(static_cast<char>((sid16 >> 8) & 0x7F));    // SId MSB, P/D=0
    fig.append(static_cast<char>(sid16 & 0xFF));
    fig.append(label16);
    fig.append(static_cast<char>((mask >> 8) & 0xFF));
    fig.append(static_cast<char>(mask & 0xFF));
    return fig;
}

void TestAdvancedFIGAnalyser::testCharset6ThaiLabel()
{
    AdvancedFIGAnalyser ana;

    // 16-character label field with TIS-620 bytes: 0xA1 0xA2 0xA3 = ก ข ฃ
    QByteArray label16(16, ' ');
    label16[0] = static_cast<char>(0xA1);
    label16[1] = static_cast<char>(0xA2);
    label16[2] = static_cast<char>(0xA3);

    QByteArray fig = serviceLabelFig(6, 0x2C61, label16, 0xFFFF);
    ana.analyzeFICData(fig);

    const QString expected = QStringLiteral("\u0E01\u0E02\u0E03");  // กขฃ
    expectService(ana, 0x2C61, expected, expected, 0xFFFF);
}

// (d) ========================================================================

void TestAdvancedFIGAnalyser::testCharset0EBULatinSpecials()
{
    AdvancedFIGAnalyser ana;

    // EBU-Latin special bytes per the TS 101 756 table used by
    // charset_converter.cpp: 0x80 -> 'á' (U+00E1), 0xE1 -> 'Å' (U+00C5)
    QByteArray label16(16, ' ');
    label16[0] = static_cast<char>(0x80);
    label16[1] = static_cast<char>(0xE1);

    QByteArray fig = serviceLabelFig(0, 0x2C61, label16, 0xFFFF);
    ana.analyzeFICData(fig);

    const QString expected = QStringLiteral("\u00E1\u00C5");  // áÅ
    expectService(ana, 0x2C61, expected, expected, 0xFFFF);
}

// (e) ========================================================================

void TestAdvancedFIGAnalyser::testInputGuards()
{
    AdvancedFIGAnalyser ana;

    // Empty FIC
    ana.analyzeFICData(QByteArray());
    QVERIFY(!ana.getLastError().isEmpty());

    // FIC larger than 768 bytes (max full on-air FIC)
    QByteArray huge(769, '\0');
    huge[0] = static_cast<char>(0x35);  // would be a valid FIG 1/0 header
    ana.analyzeFICData(huge);
    QVERIFY(ana.getLastError().contains("too large"));

    // 768-byte FIC is accepted (24 FIBs of padding)
    AdvancedFIGAnalyser ana768;
    QByteArray fic768;
    while (fic768.size() < 768) {
        fic768 += makePaddingFib();
    }
    ana768.analyzeFICData(fic768);
    QVERIFY(!ana768.getLastError().contains("too large"));
    QCOMPARE(ana768.getFibCrcFailureCount(), 0);  // padding CRCs valid
}

void TestAdvancedFIGAnalyser::testForceCharsetOverride()
{
    AdvancedFIGAnalyser ana;
    eti::AnalyserSettings settings = eti::AnalyserSettings::defaults();
    settings.force_charset = 6;  // override: treat every label as TIS-620
    ana.setAnalyserSettings(settings);

    QByteArray label16(16, ' ');
    label16[0] = static_cast<char>(0xA1);
    label16[1] = static_cast<char>(0xA2);
    label16[2] = static_cast<char>(0xA3);

    // Stream charset says 0 (EBU Latin), force_charset=6 must win
    QByteArray fig = serviceLabelFig(0, 0x2C61, label16, 0xFFFF);
    ana.analyzeFICData(fig);

    const QString expected = QStringLiteral("\u0E01\u0E02\u0E03");  // กขฃ
    expectService(ana, 0x2C61, expected, expected, 0xFFFF);
}

// ============================================================================
// Helpers
// ============================================================================

void TestAdvancedFIGAnalyser::expectService(const AdvancedFIGAnalyser& ana,
                                            uint32_t sid,
                                            const QString& label,
                                            const QString& shortLabel,
                                            uint16_t mask)
{
    const auto services = ana.getDABServices();
    for (const auto& svc : services) {
        if (svc.service_id == sid) {
            QCOMPARE(svc.service_label.trimmed(), label);
            QCOMPARE(svc.short_label, shortLabel);
            QCOMPARE(svc.character_flag, mask);
            return;
        }
    }
    QFAIL(QString("Service 0x%1 not found").arg(sid, 8, 16, QChar('0')).toUtf8());
}

QTEST_MAIN(TestAdvancedFIGAnalyser)
#include "test_advanced_fig_analyser.moc"