/**
 * @file test_cli_fig_trace.cpp
 * @brief P1 CLI FIG-detail tests: per-frame FIG instance trace + `figs:` /
 *        `fic_health:` YAML sections (docs/fixes/CLI_FIG_COVERAGE.md).
 *
 * Validates on the REAL Bangkok fixture (eti/bkk_20062022_141637.eti,
 * ensemble 0x2000 "Bangkok DAB+", 16 services, 16 subchannels EEP-3A):
 *   (a) --fig-trace emits a parseable per-frame FIG instance trace with
 *       readable values (FIG 0/0 ensemble id, FIG 1/0 + 1/1 labels);
 *   (b) the `figs:` inventory lists the 6 core FIGs with crc_failures 0;
 *   (c) the `fic_health:` block carries FIB/FIG error + reconfiguration
 *       counters (total_fibs 600 for 200 frames x 3 FIBs);
 *   (d) default output keeps the committed baseline fields (ensemble /
 *       services / subchannels) and adds the two new sections only;
 *   (e) a synthetic reconfiguration (sub-channel added mid-stream) is
 *       detected (reconfiguration_count) and flagged on the trace;
 *   (f) FIC-less streams yield an empty `figs:` list.
 *
 * All CLI runs go through the real streamdab-cli executable (QProcess),
 * so the parser -> headless aggregation -> YAML pipeline is exercised E2E.
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

// FIB CRC-16/CCITT-FALSE (complemented, big-endian) for synthetic frames —
// header-only implementation shared with the core (crc16.hpp).
#include "../src/core/crc16.hpp"

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
 *
 * Layout mirrors modern_eti_frame_parser.cpp parseETI_NI_Frame: sync
 * 0xFF 0xF8, byte4 FCT, byte5 FICF|NST, byte6 FP|MID|FL-hi, FIC at
 * offset 12 + 4*NST (=12), 3 FIBs of 32 bytes each.
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
    QByteArray block;
    block.append(static_cast<char>(0x05));  // type 0, length 5
    block.append(data);
    return block;
}

/**
 * @brief FIG 0/1 Sub-channel Organization block (short/UEP entries).
 */
QByteArray fig01(const QList<int>& subchannelIds)
{
    QByteArray data;
    data.append(static_cast<char>(0x01));  // extension
    for (int id : subchannelIds) {
        data.append(static_cast<char>((id << 2) & 0xFC));  // SubChId(6)|SADr-hi
        data.append(static_cast<char>(0x00));              // SADr low
        data.append(static_cast<char>(0x00));              // TableSwitch|TableIndex
    }
    QByteArray block;
    block.append(static_cast<char>(0x00 | static_cast<int>(data.size())));  // type 0, length
    block.append(data);
    return block;
}

}  // namespace

class TestCLIFigTrace : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void figTrace_linesPerFrameWithReadables();
    void figInventory_sixCoreFigs();
    void ficHealth_fields();
    void defaultOutput_keepsBaselineFields();
    void reconfig_syntheticDetection();
    void noFic_yieldsEmptyInventory();

private:
    QString m_cliPath;
    QString m_fixturePath;
    QTemporaryDir* m_tmpDir = nullptr;

    QString runCli(const QStringList& args, int* exitCode = nullptr,
                   QString* stderrOut = nullptr, int timeoutMs = 30000) const;
};

void TestCLIFigTrace::initTestCase()
{
    m_tmpDir = new QTemporaryDir();
    QVERIFY(m_tmpDir->isValid());

    // streamdab-cli sits one level above the test binary (build/tests).
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

void TestCLIFigTrace::cleanupTestCase()
{
    delete m_tmpDir;
    m_tmpDir = nullptr;
}

QString TestCLIFigTrace::runCli(const QStringList& args, int* exitCode,
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
// (a) --fig-trace: per-frame FIG instance lines with readable values
// ============================================================================

void TestCLIFigTrace::figTrace_linesPerFrameWithReadables()
{
    const QString outputPath = m_tmpDir->path() + "/trace_a.yaml";
    int exitCode = -1;
    QString stderrText;
    runCli(QStringList()
               << "--input" << m_fixturePath
               << "--output" << outputPath
               << "--fig-trace"
               << "--max-frames" << "200"
               << "--quiet",
           &exitCode, &stderrText);

    QCOMPARE(exitCode, 0);

    // Every trace line is parseable key=value output.
    const QRegularExpression lineRe(
        "^fig-trace: frame=\\d+ type=\\d+ ext=\\d+ len=\\d+ fib=\\d+ "
        "crc=(ok|bad) reconfig=(true|false)( .*)?$");
    const QStringList lines = stderrText.split('\n', Qt::SkipEmptyParts);
    QVERIFY2(lines.size() >= 200,
             qPrintable(QString("expected >=200 trace lines, got %1").arg(lines.size())));
    for (const QString& line : lines) {
        QVERIFY2(lineRe.match(line).hasMatch(),
                 qPrintable(QString("unparseable trace line: %1").arg(line)));
    }

    // FIG 0/0 readable: ensemble id.
    QVERIFY2(stderrText.contains("type=0 ext=0"),
             "FIG 0/0 instances missing from trace");
    QVERIFY2(stderrText.contains("ensemble=0x2000"),
             "FIG 0/0 readable ensemble id missing");

    // FIG 1/0 readable: charset-decoded ensemble label.
    QVERIFY2(stderrText.contains("type=1 ext=0"),
             "FIG 1/0 instances missing from trace");
    QVERIFY2(stderrText.contains("label=\"Bangkok DAB+\""),
             "FIG 1/0 readable ensemble label missing");

    // FIG 1/1 readable: service labels (at least a handful of the 15).
    const QRegularExpression labelRe("type=1 ext=1 .*label=\"([^\"]+)\"");
    QSet<QString> labels;
    for (const QString& line : lines) {
        const QRegularExpressionMatch match = labelRe.match(line);
        if (match.hasMatch()) {
            labels.insert(match.captured(1));
        }
    }
    QVERIFY2(labels.size() >= 5,
             qPrintable(QString("expected >=5 distinct FIG 1/1 labels, got %1")
                            .arg(labels.size())));

    // FIB indices are 1..3 on the structured FIC (CRC 100% on fixture).
    QVERIFY2(stderrText.contains("fib=1 crc=ok") &&
                 stderrText.contains("fib=3 crc=ok"),
             "FIB provenance (fib=1..3, crc=ok) not present");
}

// ============================================================================
// (b) --fig-trace: `figs:` inventory contains the 6 core FIGs, crc_failures 0
// ============================================================================

void TestCLIFigTrace::figInventory_sixCoreFigs()
{
    const QString outputPath = m_tmpDir->path() + "/inventory_b.yaml";
    int exitCode = -1;
    runCli(QStringList()
               << "--input" << m_fixturePath
               << "--output" << outputPath
               << "--fig-trace"
               << "--max-frames" << "200"
               << "--quiet",
           &exitCode);
    QCOMPARE(exitCode, 0);

    QFile file(outputPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString yaml = QString::fromUtf8(file.readAll());
    file.close();

    // Collect fig -> (count, crc_failures) from the `figs:` section.
    QMap<QString, QPair<qint64, qint64>> inventory;
    bool inFigs = false;
    QString currentFig;
    for (const QString& line : yaml.split('\n')) {
        if (line.startsWith("figs:")) {
            inFigs = true;
            continue;
        }
        if (inFigs && line.startsWith("  - fig:")) {
            currentFig = line.section('"', 1, 1);
            inventory[currentFig] = qMakePair(0, 0);
            continue;
        }
        if (inFigs && line.startsWith("    count:")) {
            inventory[currentFig].first = line.section(':', 1).trimmed().toLongLong();
            continue;
        }
        if (inFigs && line.startsWith("    crc_failures:")) {
            inventory[currentFig].second = line.section(':', 1).trimmed().toLongLong();
            continue;
        }
        if (inFigs && !line.startsWith(' ') && !line.isEmpty()) {
            inFigs = false;  // next top-level section
        }
    }

    const QStringList required = {"0/0", "0/1", "0/2", "0/9", "1/0", "1/1"};
    for (const QString& fig : required) {
        QVERIFY2(inventory.contains(fig),
                 qPrintable(QString("figs: inventory missing %1 (have: %2)")
                                .arg(fig, inventory.keys().join(","))));
        QVERIFY2(inventory[fig].first > 0,
                 qPrintable(QString("figs: %1 count must be > 0").arg(fig)));
        QCOMPARE(inventory[fig].second, 0);  // crc_failures
    }
}

// ============================================================================
// (c) `fic_health:` block: FIB/FIG error + reconfiguration fields
// ============================================================================

void TestCLIFigTrace::ficHealth_fields()
{
    const QString outputPath = m_tmpDir->path() + "/health_c.yaml";
    int exitCode = -1;
    runCli(QStringList()
               << "--input" << m_fixturePath
               << "--output" << outputPath
               << "--fig-trace"
               << "--max-frames" << "200"
               << "--quiet",
           &exitCode);
    QCOMPARE(exitCode, 0);

    QFile file(outputPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString yaml = QString::fromUtf8(file.readAll());
    file.close();

    const QString health = yaml.section("fic_health:", 1, 1).section("\n\n", 0, 0);
    QVERIFY2(!health.isEmpty(), "fic_health: section missing");
    for (const QString& key : {"fib_crc_failures", "total_fibs",
                               "erroneous_figs", "reconfiguration_count"}) {
        QVERIFY2(health.contains(key),
                 qPrintable(QString("fic_health missing key %1").arg(key)));
    }

    // 200 frames x 3 FIBs, all CRCs valid on the fixture.
    QVERIFY(health.contains("fib_crc_failures: 0"));
    QVERIFY(health.contains("total_fibs: 600"));
    QVERIFY(health.contains("erroneous_figs: 0"));

    // Lock-in frames (initial config growth) trigger reconfigurations; a
    // static mux must stay well below the per-rotation noise (~4455).
    const QRegularExpression reconfigRe("reconfiguration_count: (\\d+)");
    const QRegularExpressionMatch match = reconfigRe.match(health);
    QVERIFY(match.hasMatch());
    const int reconfigs = match.captured(1).toInt();
    QVERIFY2(reconfigs >= 1 && reconfigs <= 10,
             qPrintable(QString("reconfiguration_count %1 out of sane range [1,10]")
                            .arg(reconfigs)));
}

// ============================================================================
// (d) default output keeps the committed baseline fields
// ============================================================================

void TestCLIFigTrace::defaultOutput_keepsBaselineFields()
{
    const QString outputPath = m_tmpDir->path() + "/default_d.yaml";
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

    // Baseline fields: ensemble 0x2000 "Bangkok DAB+", 16 services (14 DAB+
    // audio + 2 data services incl. the full 32-bit SID 0xF3200000),
    // 16 subchannels EEP-3A, FIG statistics section.
    QVERIFY(yaml.contains("ensemble_id: 0x2000"));
    QVERIFY(yaml.contains("ensemble_label: \"Bangkok DAB+\""));
    QCOMPARE(yaml.count("  - service_id:"), 16);
    QCOMPARE(yaml.count("  - subchannel_id:"), 16);
    QVERIFY(yaml.contains("protection_form: \"EEP-3A\""));
    QVERIFY(yaml.contains("fig_statistics:"));
    QVERIFY(yaml.contains("etsi_compliance:"));

    // Additive P1 sections present in default runs too.
    QVERIFY(yaml.contains("figs:"));
    QVERIFY(yaml.contains("fic_health:"));

    // No trace lines leak into stdout in default mode.
    QVERIFY(!yaml.contains("fig-trace:"));
}

// ============================================================================
// (e) synthetic reconfiguration: sub-channel added mid-stream
// ============================================================================

void TestCLIFigTrace::reconfig_syntheticDetection()
{
    // Frames: 1+2 identical config (subch 1), frame 3 adds subch 2.
    const QString etiPath = m_tmpDir->path() + "/reconfig.eti";
    {
        QFile file(etiPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        const QByteArray frame1 = buildNiFrame(1, {fig00(0x2000), fig01({1}), QByteArray()});
        const QByteArray frame2 = buildNiFrame(2, {fig00(0x2000), fig01({1}), QByteArray()});
        const QByteArray frame3 = buildNiFrame(3, {fig00(0x2000), fig01({1, 2}), QByteArray()});
        file.write(frame1);
        file.write(frame2);
        file.write(frame3);
        file.close();
    }

    const QString outputPath = m_tmpDir->path() + "/reconfig_e.yaml";
    int exitCode = -1;
    QString stderrText;
    runCli(QStringList()
               << "--input" << etiPath
               << "--output" << outputPath
               << "--fig-trace"
               << "--max-frames" << "3"
               << "--quiet",
           &exitCode, &stderrText);
    QCOMPARE(exitCode, 0);

    QFile file(outputPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString yaml = QString::fromUtf8(file.readAll());
    file.close();

    const QString health = yaml.section("fic_health:", 1, 1).section("\n\n", 0, 0);
    QVERIFY(health.contains("total_fibs: 9"));           // 3 frames x 3 FIBs
    QVERIFY(health.contains("fib_crc_failures: 0"));     // valid synthetic CRCs
    QVERIFY2(health.contains("reconfiguration_count: 2"),
             qPrintable(QString("expected 2 reconfigs (frame 1 lock-in + frame 3 "
                                "add), got: %1").arg(health.trimmed())));

    // Frame 2 (unchanged config) must NOT be flagged; frame 3 must be.
    {
        bool sawFrame2 = false;
        bool sawFrame3 = false;
        for (const QString& line : stderrText.split('\n')) {
            if (line.startsWith("fig-trace: frame=2 ") && line.contains("ext=1")) {
                sawFrame2 = true;
                QVERIFY2(line.contains("reconfig=false"),
                         qPrintable(QString("frame 2 must not reconfigure: %1").arg(line)));
            }
            if (line.startsWith("fig-trace: frame=3 ") && line.contains("ext=1")) {
                sawFrame3 = true;
                QVERIFY2(line.contains("reconfig=true"),
                         qPrintable(QString("frame 3 must reconfigure: %1").arg(line)));
            }
        }
        QVERIFY2(sawFrame2, "missing FIG 0/1 trace line for frame 2");
        QVERIFY2(sawFrame3, "missing FIG 0/1 trace line for frame 3");
    }
}

// ============================================================================
// (f) FIC-less stream: empty `figs:` list, zero FIB health
// ============================================================================

void TestCLIFigTrace::noFic_yieldsEmptyInventory()
{
    const QString etiPath = m_tmpDir->path() + "/no_fic.eti";
    {
        QFile file(etiPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QByteArray noFicFrame(ETI_FRAME_SIZE, static_cast<char>(0x00));
        noFicFrame[0] = static_cast<char>(0xFF);
        noFicFrame[1] = static_cast<char>(0xF8);
        noFicFrame[4] = static_cast<char>(0x01);
        noFicFrame[5] = static_cast<char>(0x00);  // FICF=0, NST=0
        noFicFrame[6] = static_cast<char>(0x08);
        file.write(noFicFrame);
        file.write(noFicFrame);
        file.close();
    }

    const QString outputPath = m_tmpDir->path() + "/no_fic_f.yaml";
    int exitCode = -1;
    QString stderrText;
    runCli(QStringList()
               << "--input" << etiPath
               << "--output" << outputPath
               << "--fig-trace"
               << "--quiet",
           &exitCode, &stderrText);
    QCOMPARE(exitCode, 0);

    QFile file(outputPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString yaml = QString::fromUtf8(file.readAll());
    file.close();

    QVERIFY(yaml.contains("figs:\n  []"));
    const QString health = yaml.section("fic_health:", 1, 1).section("\n\n", 0, 0);
    QVERIFY(health.contains("total_fibs: 0"));
    QVERIFY(health.contains("fib_crc_failures: 0"));
    QVERIFY(health.contains("erroneous_figs: 0"));
    QVERIFY(!stderrText.contains("fig-trace:"));  // no FIC, no trace lines
}

QTEST_MAIN(TestCLIFigTrace)
#include "test_cli_fig_trace.moc"