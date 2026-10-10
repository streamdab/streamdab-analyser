/**
 * @file test_cli_fig_extended.cpp
 * @brief Backlog P2/P3 CLI FIG-detail tests (docs/fixes/CLI_FIG_COVERAGE.md,
 *        items 6/7/8 of the close-out plan).
 *
 * Validates on synthetic ETI-NI streams (and the real Bangkok fixture where
 * noted):
 *   (a) protection_tables.hpp unit values (EEP-A 16/8/6/4, EEP-B 27/21/18/15,
 *       UEP table index sizes/levels);
 *   (b) FIG 1/2 + 1/3 labels with charset-6 (TIS-620) Thai text decode to
 *       readable UTF-8 in the --fig-trace lines;
 *   (c) FIG 0/21 frequency info decodes to "freq=227.360MHz";
 *   (d) FIG 0/22 coordinates decode to "lat=+13.7 lon=+100.5";
 *   (e) FIG type 2 (OTH) header + UTF-8 segment label decode readably and
 *       the `figs:` inventory counts it as 2/1;
 *   (f) FIG 1/1 with a 32-bit data-service SId (P/D from SId MSB, GUI/CLI
 *       alignment) binds the label and surfaces sid32 in the YAML;
 *   (g) UEP short-form FIG 0/1 entries report protection table + cu_size
 *       (protection=UEP-3 index=7 cu_size=35, protection_table "UEP-3");
 *   (h) --figs-by-type groups the 6 core fixture FIGs with frame numbers and
 *       aggregate readable values;
 *   (i) --fib-hex dumps each FIB with [a-b] FIG t/e markers and CRC status;
 *   (j) default fixture output gains only the additive protection_table field.
 *
 * All CLI runs go through the real streamdab-cli executable (QProcess), so
 * the parser -> headless aggregation -> YAML pipeline is exercised E2E.
 * Runtime budget: < 60 s (frame-limited via --max-frames).
 */

#include <QTest>
#include <QProcess>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>

#include <cstdint>
#include <cstring>
#include <vector>

// FIB CRC-16/CCITT-FALSE (complemented, big-endian) for synthetic frames.
#include "../src/core/crc16.hpp"
// EEP/UEP protection tables (backlog item 8).
#include "../src/core/protection_tables.hpp"

namespace {

constexpr int ETI_FRAME_SIZE = 6144;
constexpr int FIB_SIZE = 32;          // 30 bytes FIG area + 2 bytes CRC
constexpr int FIG_AREA_SIZE = 30;

/**
 * @brief Append one FIB: 30-byte FIG area + complemented CCITT-FALSE CRC.
 */
void appendFib(QByteArray& frame, int ficOffset, const QByteArray& figArea)
{
    Q_ASSERT(figArea.size() <= FIG_AREA_SIZE);
    QByteArray area = figArea;
    area.resize(FIG_AREA_SIZE, static_cast<char>(0xFF));  // padding end marker

    const uint16_t crc = static_cast<uint16_t>(
        ~eti::crc16ccitt_false(reinterpret_cast<const uint8_t*>(area.constData()),
                               FIG_AREA_SIZE));

    for (int i = 0; i < FIG_AREA_SIZE; ++i) {
        frame[ficOffset + i] = area[i];
    }
    frame[ficOffset + 30] = static_cast<char>((crc >> 8) & 0xFF);
    frame[ficOffset + 31] = static_cast<char>(crc & 0xFF);
}

/**
 * @brief One ETI-NI frame with a 96-byte FIC (3 FIBs) and NST=0.
 */
QByteArray buildNiFrame(int fct, const QList<QByteArray>& fibAreas)
{
    QByteArray frame(ETI_FRAME_SIZE, static_cast<char>(0x00));
    frame[0] = static_cast<char>(0xFF);
    frame[1] = static_cast<char>(0xF8);
    frame[4] = static_cast<char>(fct & 0xFF);
    frame[5] = static_cast<char>(0x80);  // FICF=1, NST=0
    frame[6] = static_cast<char>(0x08);  // FP=0, MID=1, FL-hi=0
    frame[7] = static_cast<char>(0x00);

    int ficOffset = 12;
    for (const QByteArray& area : fibAreas) {
        appendFib(frame, ficOffset, area);
        ficOffset += FIB_SIZE;
    }
    return frame;
}

/**
 * @brief One FIG block: [type(3)|len(5) 1B] + data field.
 */
QByteArray figBlock(uint8_t type, const QByteArray& data)
{
    Q_ASSERT(data.size() <= 29);
    QByteArray block;
    block.append(static_cast<char>((type << 5) | static_cast<int>(data.size())));
    block.append(data);
    return block;
}

/**
 * @brief FIG 0/0 Ensemble Information block: [ext] [EId 2B] [change] [cif].
 */
QByteArray fig00(uint16_t ensembleId)
{
    QByteArray data;
    data.append(static_cast<char>(0x00));                                     // extension
    data.append(static_cast<char>((ensembleId >> 8) & 0xFF));                 // EId hi
    data.append(static_cast<char>(ensembleId & 0xFF));                        // EId lo
    data.append(static_cast<char>(0x00));                                     // change/alarm/CIF-hi
    data.append(static_cast<char>(0x00));                                     // CIF low
    return figBlock(0, data);
}

/**
 * @brief 16-byte label field padded with spaces.
 */
QByteArray label16(const QByteArray& bytes)
{
    QByteArray label = bytes;
    label.resize(16, static_cast<char>(0x20));
    return label;
}

}  // namespace

class TestCLIFigExtended : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void protectionTables_unit();
    void synthetic_fig12fig13ThaiLabels();
    void synthetic_fig021_freq();
    void synthetic_fig022_coords();
    void synthetic_figType2();
    void synthetic_fig11_sid32();
    void synthetic_uepSubchannel();
    void fixture_figsByType();
    void fixture_fibHex();
    void fixture_defaultOutputProtectionTable();
    // T35 regression: full 32-bit data-service SID survives CLI discovery
    void fixture_dataServiceSid32();
    // T39: ETSI compliance verdict semantics (protocol checks, not transport
    // error frames) + an explainable NO failure case.
    void fixture_complianceVerdict();
    void synthetic_badFibComplianceNo();

private:
    QString m_cliPath;
    QString m_fixturePath;
    QTemporaryDir* m_tmpDir = nullptr;

    QString runCli(const QStringList& args, int* exitCode = nullptr,
                   QString* stderrOut = nullptr, int timeoutMs = 60000) const;
};

void TestCLIFigExtended::initTestCase()
{
    m_tmpDir = new QTemporaryDir();
    QVERIFY(m_tmpDir->isValid());

    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList cliCandidates = {
#ifdef STREAMDAB_CLI_PATH
        QStringLiteral(STREAMDAB_CLI_PATH),  // exact path from CMake (multi-config, .exe)
#endif
        appDir + "/../streamdab-cli",
        appDir + "/streamdab-cli",
    };
    for (const QString& candidate : cliCandidates) {
        if (QFile::exists(candidate)) {
            m_cliPath = candidate;
            break;
        }
    }
    if (m_cliPath.isEmpty()) {
        QSKIP("streamdab-cli not found next to the test binary — build the project first.");
    }

    const QStringList fixtureCandidates = {
        QStringLiteral(QT_TESTCASE_SOURCEDIR) + QStringLiteral("/../eti/bkk_20062022_141637.eti"),
        appDir + "/../eti/bkk_20062022_141637.eti",
        appDir + "/../../eti/bkk_20062022_141637.eti",
    };
    for (const QString& candidate : fixtureCandidates) {
        if (QFile::exists(candidate)) {
            m_fixturePath = candidate;
            break;
        }
    }
    if (m_fixturePath.isEmpty()) {
        QSKIP("Real ETI fixture eti/bkk_20062022_141637.eti not found.");
    }
    qDebug() << "CLI:" << m_cliPath << "fixture:" << m_fixturePath;
}

void TestCLIFigExtended::cleanupTestCase()
{
    delete m_tmpDir;
    m_tmpDir = nullptr;
}

QString TestCLIFigExtended::runCli(const QStringList& args, int* exitCode,
                                   QString* stderrOut, int timeoutMs) const
{
    QProcess process;
    process.start(m_cliPath, args);
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(5000);
        if (exitCode) {
            *exitCode = -1;
        }
        return QString();
    }
    if (exitCode) {
        *exitCode = process.exitCode();
    }
    if (stderrOut) {
        *stderrOut = QString::fromUtf8(process.readAllStandardError());
    }
    return QString::fromUtf8(process.readAllStandardOutput());
}

// ============================================================================
// (a) protection_tables.hpp unit values
// ============================================================================

void TestCLIFigExtended::protectionTables_unit()
{
    // EEP-A: 16/8/6/4 CU per level 1..4 (EN 300 401 Table 6.1).
    QCOMPARE(eti::eepSizeCu(0, 1), 16);
    QCOMPARE(eti::eepSizeCu(0, 2), 8);
    QCOMPARE(eti::eepSizeCu(0, 3), 6);
    QCOMPARE(eti::eepSizeCu(0, 4), 4);
    // EEP-B: 27/21/18/15 CU per level 1..4.
    QCOMPARE(eti::eepSizeCu(1, 1), 27);
    QCOMPARE(eti::eepSizeCu(1, 2), 21);
    QCOMPARE(eti::eepSizeCu(1, 3), 18);
    QCOMPARE(eti::eepSizeCu(1, 4), 15);
    // Unknown option/level -> 0.
    QCOMPARE(eti::eepSizeCu(2, 1), 0);
    QCOMPARE(eti::eepSizeCu(0, 5), 0);

    // UEP table (EN 300 401 Table 6.2): index 0 = 16 CU / level 5 (UEP-5),
    // index 7 = 35 CU / level 3 (UEP-3), index 32 = 104 CU.
    QCOMPARE(eti::uepSizeCu(0), 16);
    QCOMPARE(eti::uepLevel1Based(0), 5);
    QCOMPARE(eti::uepSizeCu(7), 35);
    QCOMPARE(eti::uepLevel1Based(7), 3);
    QCOMPARE(eti::uepSizeCu(32), 104);

    QCOMPARE(QString::fromStdString(eti::protectionTableName(0, false)), "EEP-A");
    QCOMPARE(QString::fromStdString(eti::protectionTableName(1, false)), "EEP-B");
    QCOMPARE(QString::fromStdString(eti::protectionTableName(0, true, 7)), "UEP-3");
    QCOMPARE(QString::fromStdString(eti::protectionTableName(0, true, 0)), "UEP-5");
}

// ============================================================================
// (b) FIG 1/2 + 1/3 Thai (TIS-620, charset 6) labels in the trace
// ============================================================================

void TestCLIFigExtended::synthetic_fig12fig13ThaiLabels()
{
    // The Thai string "ทดสอบ" in TIS-620 (charset 6).
    const QByteArray thai = QByteArray::fromHex("B7B4CACDBA");  // ท ด ส อ บ

    // FIG 1/2: [charset6|OE|ext2][SCIdS6=5|Rfu|PD=0][SId 0x1234][label][flag]
    QByteArray fig12data;
    fig12data.append(static_cast<char>(0x62));  // charset 6, ext 2
    fig12data.append(static_cast<char>(0x14));  // SCIdS=5 (b7-b2), PD=0
    fig12data.append(static_cast<char>(0x12));  // SId hi
    fig12data.append(static_cast<char>(0x34));  // SId lo
    fig12data.append(label16(thai));
    fig12data.append(static_cast<char>(0xFF));  // char flag hi
    fig12data.append(static_cast<char>(0xFF));  // char flag lo

    // FIG 1/3: [charset6|OE|ext3][SId 0xF32A0001][label][flag]
    QByteArray fig13data;
    fig13data.append(static_cast<char>(0x63));  // charset 6, ext 3
    fig13data.append(QByteArray::fromHex("F32A0001"));
    fig13data.append(label16(thai));
    fig13data.append(static_cast<char>(0xFF));
    fig13data.append(static_cast<char>(0xFF));

    const QString etiPath = m_tmpDir->path() + "/fig12_13.eti";
    {
        QFile file(etiPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(buildNiFrame(1, {figBlock(1, fig12data), figBlock(1, fig13data),
                                    QByteArray()}));
        file.close();
    }

    const QString outputPath = m_tmpDir->path() + "/fig12_13.yaml";
    int exitCode = -1;
    QString stderrText;
    runCli(QStringList()
               << "--input" << etiPath
               << "--output" << outputPath
               << "--fig-trace"
               << "--quiet",
           &exitCode, &stderrText);
    QCOMPARE(exitCode, 0);

    // FIG 1/2 readable: SCIdS + 16-bit SId + charset-decoded Thai label.
    QVERIFY2(stderrText.contains("type=1 ext=2"),
             "FIG 1/2 instance missing from trace");
    QVERIFY2(stderrText.contains(QString::fromUtf8("label=\"ทดสอบ\"")),
             "FIG 1/2 Thai label not decoded to UTF-8");
    QVERIFY2(stderrText.contains("scids=5 sid=0x00001234"),
             "FIG 1/2 SCIdS/SId readable missing");

    // FIG 1/3 readable: 32-bit SId + Thai label.
    QVERIFY2(stderrText.contains("type=1 ext=3"),
             "FIG 1/3 instance missing from trace");
    QVERIFY2(stderrText.contains("sid=0xf32a0001"),
             "FIG 1/3 SId readable missing");
    QCOMPARE(stderrText.count(QString::fromUtf8("label=\"ทดสอบ\"")), 2);

    // The `figs:` inventory counts both new FIGs.
    QFile file(outputPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString yaml = QString::fromUtf8(file.readAll());
    file.close();
    QVERIFY(yaml.contains("- fig: \"1/2\""));
    QVERIFY(yaml.contains("- fig: \"1/3\""));
}

// ============================================================================
// (c) FIG 0/21 frequency info -> freq=227.360MHz
// ============================================================================

void TestCLIFigExtended::synthetic_fig021_freq()
{
    // [ext 0x15][RegionId 0x200 | len_FI 6][FI: EId 0x2000, R&M 0, len 3]
    // [freq 3B: 0x3782 -> 227360 kHz = 227.360 MHz]
    const QByteArray fig21data = QByteArray::fromHex(
        "15 40 06 20 00 03 00 37 82");

    const QString etiPath = m_tmpDir->path() + "/fig021.eti";
    {
        QFile file(etiPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(buildNiFrame(1, {figBlock(0, fig21data), QByteArray(),
                                    QByteArray()}));
        file.close();
    }

    const QString outputPath = m_tmpDir->path() + "/fig021.yaml";
    int exitCode = -1;
    QString stderrText;
    runCli(QStringList()
               << "--input" << etiPath
               << "--output" << outputPath
               << "--fig-trace"
               << "--figs-by-type"
               << "--quiet",
           &exitCode, &stderrText);
    QCOMPARE(exitCode, 0);

    QVERIFY2(stderrText.contains("type=0 ext=21"),
             "FIG 0/21 instance missing from trace");
    QVERIFY2(stderrText.contains("freq=227.360MHz"),
             "FIG 0/21 frequency not decoded to 227.360MHz");

    // The grouped view aggregates the same readable value.
    QVERIFY2(stderrText.contains("figs-by-type: fig=0/21") &&
                 stderrText.contains("freq=227.360MHz"),
             "FIG 0/21 grouped view missing frequency value");

    QFile file(outputPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString yaml = QString::fromUtf8(file.readAll());
    file.close();
    QVERIFY(yaml.contains("- fig: \"0/21\""));
}

// ============================================================================
// (d) FIG 0/22 coordinates -> lat=+13.7 lon=+100.5
// ============================================================================

void TestCLIFigExtended::synthetic_fig022_coords()
{
    // [ext 0x16][M/S=0 MainId=0x41][lat coarse 0x1388][lon coarse 0x4774]
    // [lat_fine=0 | lon_fine=0xB] -> lat = 80000*90/524288 = +13.73,
    // lon = 292683*180/524288 = +100.50.
    const QByteArray fig22data = QByteArray::fromHex(
        "16 41 13 88 47 74 0B");

    const QString etiPath = m_tmpDir->path() + "/fig022.eti";
    {
        QFile file(etiPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(buildNiFrame(1, {figBlock(0, fig22data), QByteArray(),
                                    QByteArray()}));
        file.close();
    }

    const QString outputPath = m_tmpDir->path() + "/fig022.yaml";
    int exitCode = -1;
    QString stderrText;
    runCli(QStringList()
               << "--input" << etiPath
               << "--output" << outputPath
               << "--fig-trace"
               << "--quiet",
           &exitCode, &stderrText);
    QCOMPARE(exitCode, 0);

    QVERIFY2(stderrText.contains("type=0 ext=22"),
             "FIG 0/22 instance missing from trace");
    QVERIFY2(stderrText.contains("lat=+13.7 lon=+100.5"),
             "FIG 0/22 coordinates not decoded readably");

    QFile file(outputPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString yaml = QString::fromUtf8(file.readAll());
    file.close();
    QVERIFY(yaml.contains("- fig: \"0/22\""));
}

// ============================================================================
// (e) FIG type 2 (OTH) minimal decode + inventory count
// ============================================================================

void TestCLIFigExtended::synthetic_figType2()
{
    // [toggle0|seg0|rfu0|ext1][SId 0x1234][enc UTF-8][char flag 0xFFFF]
    // [label "TEST"]
    QByteArray fig2data;
    fig2data.append(static_cast<char>(0x01));  // ext 1 = programme label
    fig2data.append(static_cast<char>(0x12));
    fig2data.append(static_cast<char>(0x34));
    fig2data.append(static_cast<char>(0x00));  // encoding 0 (UTF-8)
    fig2data.append(static_cast<char>(0xFF));  // char flag
    fig2data.append(static_cast<char>(0xFF));
    fig2data.append("TEST");

    const QString etiPath = m_tmpDir->path() + "/fig2.eti";
    {
        QFile file(etiPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(buildNiFrame(1, {figBlock(2, fig2data), QByteArray(),
                                    QByteArray()}));
        file.close();
    }

    const QString outputPath = m_tmpDir->path() + "/fig2.yaml";
    int exitCode = -1;
    QString stderrText;
    runCli(QStringList()
               << "--input" << etiPath
               << "--output" << outputPath
               << "--fig-trace"
               << "--figs-by-type"
               << "--quiet",
           &exitCode, &stderrText);
    QCOMPARE(exitCode, 0);

    QVERIFY2(stderrText.contains("type=2 ext=1"),
             "FIG 2/1 instance missing from trace");
    QVERIFY2(stderrText.contains("fig2:ext=1 toggle=0 seg=0 id=0x00001234"),
             "FIG type 2 header decode missing");
    QVERIFY2(stderrText.contains("label=\"TEST\""),
             "FIG type 2 label segment missing");

    QFile file(outputPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString yaml = QString::fromUtf8(file.readAll());
    file.close();
    QVERIFY2(yaml.contains("- fig: \"2/1\""),
             qPrintable(QString("figs: inventory must count 2/1, have:\n%1")
                            .arg(yaml.section("figs:", 1, 1).section("\n\n", 0, 0))));
}

// ============================================================================
// (f) FIG 1/1 with a 32-bit data-service SId (P/D from SId MSB)
// ============================================================================

void TestCLIFigExtended::synthetic_fig11_sid32()
{
    // [charset0|OE0|ext1][SId 0xF32041B6 (P/D=1)][label "DAB DATA SRVC 1"]
    // [flag 0xFFFF]
    QByteArray fig11data;
    fig11data.append(static_cast<char>(0x01));
    fig11data.append(QByteArray::fromHex("F32041B6"));
    fig11data.append(label16(QByteArray("DAB DATA SRVC 1")));
    fig11data.append(static_cast<char>(0xFF));
    fig11data.append(static_cast<char>(0xFF));

    const QString etiPath = m_tmpDir->path() + "/fig11_32.eti";
    {
        QFile file(etiPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(buildNiFrame(1, {figBlock(1, fig11data), QByteArray(),
                                    QByteArray()}));
        file.close();
    }

    const QString outputPath = m_tmpDir->path() + "/fig11_32.yaml";
    int exitCode = -1;
    QString stderrText;
    runCli(QStringList()
               << "--input" << etiPath
               << "--output" << outputPath
               << "--fig-trace"
               << "--quiet",
           &exitCode, &stderrText);
    QCOMPARE(exitCode, 0);

    // The 32-bit SId label must decode (the old fixed-offset 16-bit read
    // would have consumed bytes 0xF3 0x20 as a programme SId and produced a
    // garbage label or no label at all).
    QVERIFY2(stderrText.contains("label=\"DAB DATA SRVC 1\""),
             "FIG 1/1 32-bit SId label missing from trace");

    QFile file(outputPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString yaml = QString::fromUtf8(file.readAll());
    file.close();
    QVERIFY2(yaml.contains("service_id: 0xF32041B6"),
             "32-bit data service SId must surface in the YAML services");
    QVERIFY(yaml.contains("service_label: \"DAB DATA SRVC 1\""));
}

// ============================================================================
// (g) UEP short-form sub-channel: protection table + cu_size
// ============================================================================

void TestCLIFigExtended::synthetic_uepSubchannel()
{
    // FIG 0/1 short form: [ext 0x01][SubCh1|SADr-hi][SADr lo 0]
    // [TableSwitch 0 | TableIndex 7]
    const QByteArray fig01data = QByteArray::fromHex("01 04 00 07");

    const QString etiPath = m_tmpDir->path() + "/uep.eti";
    {
        QFile file(etiPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(buildNiFrame(1, {figBlock(0, fig01data), fig00(0x2000),
                                    QByteArray()}));
        file.close();
    }

    const QString outputPath = m_tmpDir->path() + "/uep.yaml";
    int exitCode = -1;
    QString stderrText;
    runCli(QStringList()
               << "--input" << etiPath
               << "--output" << outputPath
               << "--fig-trace"
               << "--quiet",
           &exitCode, &stderrText);
    QCOMPARE(exitCode, 0);

    // Trace: UEP table name (level 3), wire index and table CU size.
    QVERIFY2(stderrText.contains("type=0 ext=1") &&
                 stderrText.contains("protection=UEP-3 index=7 cu_size=35"),
             "UEP protection table not reported on the 0/1 trace");

    QFile file(outputPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString yaml = QString::fromUtf8(file.readAll());
    file.close();
    QVERIFY2(yaml.contains("protection_table: \"UEP-3\""),
             "subchannels YAML must carry the UEP protection table name");
    QVERIFY2(yaml.contains("size: 35"),
             "subchannel size must derive from the UEP table (35 CU)");
}

// ============================================================================
// (h) --figs-by-type on the fixture: 6 core FIGs grouped
// ============================================================================

void TestCLIFigExtended::fixture_figsByType()
{
    const QString outputPath = m_tmpDir->path() + "/bytype_h.yaml";
    int exitCode = -1;
    QString stderrText;
    runCli(QStringList()
               << "--input" << m_fixturePath
               << "--output" << outputPath
               << "--figs-by-type"
               << "--max-frames" << "200"
               << "--quiet",
           &exitCode, &stderrText);
    QCOMPARE(exitCode, 0);

    const QStringList lines = stderrText.split('\n', Qt::SkipEmptyParts);
    QVERIFY2(lines.size() >= 6,
             qPrintable(QString("expected >=6 figs-by-type lines, got %1")
                            .arg(lines.size())));

    QMap<QString, QString> groups;
    for (const QString& line : lines) {
        QVERIFY2(line.startsWith("figs-by-type: fig="),
                 qPrintable(QString("unparseable grouped line: %1").arg(line)));
        const QString fig = line.section("fig=", 1, 1).section(' ', 0, 0);
        groups[fig] = line;
    }

    for (const QString& fig : {"0/0", "0/1", "0/2", "0/9", "1/0", "1/1"}) {
        QVERIFY2(groups.contains(fig),
                 qPrintable(QString("figs-by-type missing %1").arg(fig)));
        QVERIFY2(groups[fig].contains("count=") &&
                     groups[fig].contains("frames=\""),
                 qPrintable(QString("figs-by-type %1 incomplete: %2")
                                .arg(fig, groups[fig])));
    }

    // Aggregate readable values: ensemble id + label, subchannel protection.
    QVERIFY2(groups["0/0"].contains("ensemble=0x2000"),
             "grouped 0/0 must aggregate the ensemble id");
    QVERIFY2(groups["1/0"].contains("label=\"Bangkok DAB+\""),
             "grouped 1/0 must aggregate the ensemble label");
    QVERIFY2(groups["0/1"].contains("protection=EEP-3A cu_size=6"),
             "grouped 0/1 must aggregate the protection table value");
    QVERIFY2(groups["1/1"].contains("label=\"NBTCtest\""),
             "grouped 1/1 must aggregate service labels");
}

// ============================================================================
// (i) --fib-hex on the fixture: FIB dumps with FIG markers + CRC
// ============================================================================

void TestCLIFigExtended::fixture_fibHex()
{
    const QString outputPath = m_tmpDir->path() + "/fibhex_i.yaml";
    int exitCode = -1;
    QString stderrText;
    runCli(QStringList()
               << "--input" << m_fixturePath
               << "--output" << outputPath
               << "--fib-hex"
               << "--max-frames" << "5"
               << "--quiet",
           &exitCode, &stderrText);
    QCOMPARE(exitCode, 0);

    // Fixture: frame 2 FIB 1 carries FIG 0/0 at bytes 0-5 and FIG 1/1 at
    // bytes 6-27; all FIB CRCs are valid.
    QVERIFY2(stderrText.contains("fib-hex: frame=2 fib=1 crc=ok"),
             "FIB hex header line missing");
    QVERIFY2(stderrText.contains("bytes: 05 00 20 00 26 1c 35 01"),
             "FIB 1 hex bytes missing");
    QVERIFY2(stderrText.contains("[0-5] FIG 0/0"),
             "FIG byte-range marker [0-5] FIG 0/0 missing");
    QVERIFY2(stderrText.contains("[6-27] FIG 1/1"),
             "FIG byte-range marker [6-27] FIG 1/1 missing");

    // 5 frames: frame 1 of the fixture carries no structured FIC (the FIG
    // trace starts at frame 2), so 4 frames x 3 FIBs = 12 header lines, all
    // with valid CRCs.
    const int headerCount = stderrText.count("fib-hex: frame=");
    QVERIFY2(headerCount == 12,
             qPrintable(QString("expected 12 FIB headers (4x3, frame 1 has no "
                                "FIC), got %1").arg(headerCount)));
    QVERIFY(!stderrText.contains("crc=bad"));
}

// ============================================================================
// (j) default fixture output: only the additive protection_table field
// ============================================================================

void TestCLIFigExtended::fixture_defaultOutputProtectionTable()
{
    const QString outputPath = m_tmpDir->path() + "/default_j.yaml";
    int exitCode = -1;
    runCli(QStringList()
               << "--input" << m_fixturePath
               << "--output" << outputPath
               << "--max-frames" << "200"
               << "--quiet",
           &exitCode);
    QCOMPARE(exitCode, 0);

    QFile file(outputPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString yaml = QString::fromUtf8(file.readAll());
    file.close();

    // 16 sub-channels, all EEP-3A -> 16 additive protection_table lines and
    // the existing fields stay byte-identical in shape.
    QCOMPARE(yaml.count("protection_table: \"EEP-A\""), 16);
    QVERIFY(yaml.contains("size: 24"));
    QVERIFY(yaml.contains("protection_form: \"EEP-3A\""));
    QVERIFY(yaml.contains("protection_level: 3"));
}

// ============================================================================
// (k) T35 regression: the legitimate full 32-bit data service 0xF3200000
// ============================================================================

void TestCLIFigExtended::fixture_dataServiceSid32()
{
    const QString outputPath = m_tmpDir->path() + "/dataSids_k.yaml";
    int exitCode = -1;
    runCli(QStringList()
               << "--input" << m_fixturePath
               << "--output" << outputPath
               << "--max-frames" << "200"
               << "--quiet",
           &exitCode);
    QCOMPARE(exitCode, 0);

    QFile file(outputPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString yaml = QString::fromUtf8(file.readAll());
    file.close();

    // T35: CLI and GUI agree on the fixture's service count (16). Before the
    // fix the CLI reported 15 because 0xF3200000 (SRef == 0) was filtered.
    QCOMPARE(yaml.count("  - service_id:"), 16);

    // Both data services surface with their FULL 32-bit SIDs (not truncated to
    // 0x00000000 / 0x00000001).
    const int idx0 = yaml.indexOf(QStringLiteral("service_id: 0xF3200000"));
    const int idx1 = yaml.indexOf(QStringLiteral("service_id: 0xF3200001"));
    QVERIFY2(idx0 >= 0, "the legitimate data service 0xF3200000 must not be dropped");
    QVERIFY2(idx1 >= 0, "the data service 0xF3200001 must be present");
    QVERIFY2(idx0 < idx1, "0xF3200000 must precede 0xF3200001 in the services list");

    // Unlabelled data services label-fallback to the GUI's "SId 0x..." form.
    QVERIFY2(yaml.contains("service_label: \"SId 0xF3200000\""),
             "0xF3200000 must use the GUI's SId label fallback");
    QVERIFY2(yaml.contains("service_label: \"SId 0xF3200001\""),
             "0xF3200001 must use the GUI's SId label fallback");

    // 0xF3200000 keeps its component mapping (SubCh 0); the block runs until
    // the next service entry.
    const QString data0Block = yaml.mid(idx0, idx1 - idx0);
    QVERIFY2(data0Block.contains(QStringLiteral("components:")),
             "0xF3200000 must keep its component list");
    QVERIFY2(data0Block.contains(QStringLiteral("component_id: 0")),
             "0xF3200000 must keep its component mapping (SubCh 0)");

    // T35 review #9: the aggregate map is keyed on the full SID, so the two
    // 32-bit data services (0xF3...) sort AFTER all 16-bit programme SIDs
    // (order-only change vs the old head placement).
    const int lastProgramme = yaml.lastIndexOf(QStringLiteral("service_id: 0x00002CBD"));
    QVERIFY2(lastProgramme >= 0, "a late programme service (0x2CBD) must be present");
    QVERIFY2(lastProgramme < idx0,
             "the 32-bit data services must sort after the programme SIDs");

    // T35 review #5: subchannel used_by_services uses the same unified
    // "SId 0x..." form as the service_label fallback (space, not underscore).
    QVERIFY2(yaml.contains(QStringLiteral("      - \"SId 0xF3200000\"")),
             "used_by_services must use the unified 'SId 0x...' form");
    QVERIFY2(!yaml.contains(QStringLiteral("SId_0x")),
             "the legacy 'SId_0x...' underscore form must be gone");
}

// ============================================================================
// T39: ETSI compliance verdict = real protocol checks, not transport errors
// ============================================================================

void TestCLIFigExtended::fixture_complianceVerdict()
{
    // The Bangkok fixture carries 3 transport error frames but passes every
    // FIC/FIB/FIG-level protocol check (0 FIB CRC failures, 0 erroneous FIGs,
    // FIC decoded, 16 services / 1 ensemble). The verdict must therefore be YES
    // with an explainable reason; the 3 error frames stay reported separately.
    const QString outputPath = m_tmpDir->path() + "/compliance_fixture.yaml";
    int exitCode = -1;
    const QString stdoutText = runCli(QStringList()
                                          << "--input" << m_fixturePath
                                          << "--output" << outputPath,
                                      &exitCode);
    QCOMPARE(exitCode, 0);

    QVERIFY2(stdoutText.contains(QStringLiteral("ETSI Compliant:  YES")),
             qPrintable(QStringLiteral("expected YES verdict, got tail: %1")
                            .arg(stdoutText.right(400))));
    // Transport error frames are still reported separately (the fixture has 3).
    QVERIFY2(stdoutText.contains(QRegularExpression(QStringLiteral("Errors:\\s+3\\b"))),
             "transport error frames must remain reported separately");
    // The short reason is printed and carries the protocol counters.
    QVERIFY2(stdoutText.contains(QStringLiteral("ETSI Reason:")),
             "a short ETSI verdict reason must be printed");
    QVERIFY2(stdoutText.contains(QStringLiteral("FIB CRC failures=0")),
             "the reason must carry the FIB CRC failure count");
    QVERIFY2(stdoutText.contains(QStringLiteral("transport error frames=3")),
             "the reason must mention the separate transport error-frame count");

    // YAML verdict must agree with the console.
    QFile file(outputPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString yaml = QString::fromUtf8(file.readAll());
    file.close();
    QVERIFY2(yaml.contains(QStringLiteral("overall_compliance: true")),
             "YAML overall_compliance must be true");
    QVERIFY2(yaml.contains(QStringLiteral("reason:")),
             "YAML must carry the verdict reason");
    QVERIFY2(yaml.contains(QStringLiteral("FIB CRC failures=0")),
             "YAML reason must carry the FIB CRC failure count");
}

void TestCLIFigExtended::synthetic_badFibComplianceNo()
{
    // A synthetic ETI-NI stream whose only non-padding FIB has a corrupted
    // CRC. The strict FIB walk counts the failure and drops the FIG, so the
    // protocol verdict must be NO with a reason that names the FIB CRC failure.
    QByteArray frame = buildNiFrame(1, {fig00(0x2000), QByteArray(), QByteArray()});
    QCOMPARE(frame.size(), ETI_FRAME_SIZE);
    // Corrupt FIB 0's stored CRC (bytes 42..43 of the 96-byte FIC at offset 12)
    // so it can never match the computed value.
    frame[12 + 30] = static_cast<char>(static_cast<uint8_t>(frame[12 + 30]) ^ 0xAA);
    frame[12 + 31] = static_cast<char>(static_cast<uint8_t>(frame[12 + 31]) ^ 0x55);

    const QString etiPath = m_tmpDir->path() + "/bad_fib.eti";
    {
        QFile file(etiPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(frame);
        file.close();
    }

    const QString outputPath = m_tmpDir->path() + "/bad_fib.yaml";
    int exitCode = -1;
    const QString stdoutText = runCli(QStringList()
                                          << "--input" << etiPath
                                          << "--output" << outputPath,
                                      &exitCode);
    QCOMPARE(exitCode, 0);

    QVERIFY2(stdoutText.contains(QStringLiteral("ETSI Compliant:  NO")),
             qPrintable(QStringLiteral("expected NO verdict, got tail: %1")
                            .arg(stdoutText.right(400))));
    QVERIFY2(stdoutText.contains(QStringLiteral("FIB CRC failures=1")),
             "the NO reason must name the FIB CRC failure count");

    QFile file(outputPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString yaml = QString::fromUtf8(file.readAll());
    file.close();
    QVERIFY2(yaml.contains(QStringLiteral("overall_compliance: false")),
             "YAML overall_compliance must be false");
    QVERIFY2(yaml.contains(QStringLiteral("FIB CRC failures=1")),
             "YAML reason must name the FIB CRC failure count");
}

QTEST_MAIN(TestCLIFigExtended)
#include "test_cli_fig_extended.moc"