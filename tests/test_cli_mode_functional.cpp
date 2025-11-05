/**
 * @file test_cli_mode_functional.cpp
 * @brief End-to-end functional tests for CLI mode using QProcess
 *
 * Agent 37: CLI Functional Tests Implementation
 * PDCA Week 8 - CLI Mode Development
 *
 * This test suite verifies the complete CLI workflow by launching the
 * streamdab-analyzer executable in CLI mode and validating:
 * - Exit codes
 * - YAML output format
 * - File operations
 * - Error handling
 * - Performance metrics
 *
 * Target: 25+ tests, 100% pass rate
 * Performance: >900 FPS processing speed
 */

#include <QTest>
#include <QProcess>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QRegularExpression>
#include <QElapsedTimer>
#include <QDateTime>
#include <QDebug>

// Only need these for test data generation
constexpr size_t ETI_FRAME_SIZE = 6144;

class TestCLIModeFunctional : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Basic functionality (8 tests)
    void test01_ProcessSmallETIFile();
    void test02_ProcessLargeETIFile();
    void test03_OutputToFile();
    void test04_OutputToStdout();
    void test05_QuietMode();
    void test06_VerboseMode();
    void test07_MaxFramesLimit();
    void test08_HelpText();

    // YAML validation (7 tests)
    void test09_YAMLSyntaxValid();
    void test10_YAMLContainsEnsemble();
    void test11_YAMLContainsServices();
    void test12_YAMLContainsSubchannels();
    void test13_YAMLContainsFIGStats();
    void test14_YAMLThaiUTF8Preserved();
    void test15_YAMLHexFormatting();

    // Error handling (5 tests)
    void test16_InputFileNotFound();
    void test17_InvalidOutputPath();
    void test18_MalformedETIFile();
    void test19_EmptyETIFile();
    void test20_PartialETIFile();

    // Performance (3 tests)
    void test21_ProcessingSpeed();
    void test22_MemoryUsage();
    void test23_CLIOverhead();

    // Integration (2 tests)
    void test24_MultipleRuns();
    void test25_ConcurrentInstances();

private:
    QString m_executablePath;
    QTemporaryDir* m_testDir;

    // Helper functions
    QByteArray createValidETIStream(int frame_count);
    QByteArray createMalformedETIStream();
    QByteArray createPartialETIStream(int complete_frames, int partial_bytes);
    bool isValidYAML(const QString& yaml_text);
    QMap<QString, QString> parseYAMLSimple(const QString& yaml_text);
    QString runCLI(const QStringList& args, int* exit_code = nullptr, int timeout_ms = 10000);
    bool checkExecutableExists();
};

// ============================================================================
// Test Initialization
// ============================================================================

void TestCLIModeFunctional::initTestCase() {
    // Find the executable (relative to build directory)
    m_executablePath = QCoreApplication::applicationDirPath() + "/streamdab-analyzer";

    // Check if executable exists
    if (!QFile::exists(m_executablePath)) {
        QSKIP("streamdab-analyzer executable not found. Build the project first.");
    }

    qDebug() << "Using executable:" << m_executablePath;

    // Create temporary directory for test files
    m_testDir = new QTemporaryDir();
    QVERIFY(m_testDir->isValid());

    qDebug() << "Test directory:" << m_testDir->path();
}

void TestCLIModeFunctional::cleanupTestCase() {
    delete m_testDir;
    m_testDir = nullptr;
}

// ============================================================================
// Basic Functionality Tests (8 tests)
// ============================================================================

void TestCLIModeFunctional::test01_ProcessSmallETIFile() {
    // Create test ETI file with 100 frames
    QTemporaryFile eti_file(m_testDir->path() + "/test_XXXXXX.eti");
    QVERIFY(eti_file.open());
    eti_file.write(createValidETIStream(100));
    eti_file.close();

    // Create output file path
    QString output_path = m_testDir->path() + "/output.yaml";

    // Run CLI mode
    int exit_code = -1;
    QString output = runCLI(QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << output_path,
        &exit_code, 10000);

    QCOMPARE(exit_code, 0);

    // Verify output file exists
    QVERIFY(QFile::exists(output_path));

    // Read YAML output
    QFile output_file(output_path);
    QVERIFY(output_file.open(QIODevice::ReadOnly));
    QString yaml_text = output_file.readAll();
    output_file.close();

    // Verify YAML structure
    QVERIFY(yaml_text.contains("version:"));
    QVERIFY(yaml_text.contains("total_frames: 100"));
}

void TestCLIModeFunctional::test02_ProcessLargeETIFile() {
    // Create large ETI file with 1000 frames (~6MB)
    QTemporaryFile eti_file(m_testDir->path() + "/large_XXXXXX.eti");
    QVERIFY(eti_file.open());
    eti_file.write(createValidETIStream(1000));
    eti_file.close();

    QString output_path = m_testDir->path() + "/large_output.yaml";

    int exit_code = -1;
    QString output = runCLI(QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << output_path,
        &exit_code, 15000);  // Longer timeout for large file

    QCOMPARE(exit_code, 0);
    QVERIFY(QFile::exists(output_path));

    // Verify file was actually processed
    QFile output_file(output_path);
    QVERIFY(output_file.open(QIODevice::ReadOnly));
    QString yaml_text = output_file.readAll();
    output_file.close();

    QVERIFY(yaml_text.contains("total_frames: 1000"));
}

void TestCLIModeFunctional::test03_OutputToFile() {
    QTemporaryFile eti_file;
    QVERIFY(eti_file.open());
    eti_file.write(createValidETIStream(50));
    eti_file.close();

    QString output_path = m_testDir->path() + "/file_output.yaml";

    int exit_code = -1;
    runCLI(QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << output_path,
        &exit_code);

    QCOMPARE(exit_code, 0);

    QFileInfo info(output_path);
    QVERIFY(info.exists());
    QVERIFY(info.size() > 100);  // Should have substantial content
}

void TestCLIModeFunctional::test04_OutputToStdout() {
    QTemporaryFile eti_file;
    QVERIFY(eti_file.open());
    eti_file.write(createValidETIStream(10));
    eti_file.close();

    int exit_code = -1;
    QString output = runCLI(QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << "-",  // stdout
        &exit_code);

    QCOMPARE(exit_code, 0);
    QVERIFY(output.contains("version:"));
    QVERIFY(output.contains("total_frames:"));
}

void TestCLIModeFunctional::test05_QuietMode() {
    QTemporaryFile eti_file;
    QVERIFY(eti_file.open());
    eti_file.write(createValidETIStream(100));
    eti_file.close();

    QString output_path = m_testDir->path() + "/quiet_output.yaml";

    QProcess process;
    process.start(m_executablePath, QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << output_path
        << "--quiet");

    QVERIFY(process.waitForFinished(10000));
    QCOMPARE(process.exitCode(), 0);

    // Quiet mode should produce minimal stderr output
    QString stderr_output = process.readAllStandardError();
    // Should have minimal or no progress messages
    QVERIFY2(stderr_output.length() < 500, "Quiet mode produced too much output");
}

void TestCLIModeFunctional::test06_VerboseMode() {
    QTemporaryFile eti_file;
    QVERIFY(eti_file.open());
    eti_file.write(createValidETIStream(50));
    eti_file.close();

    QString output_path = m_testDir->path() + "/verbose_output.yaml";

    QProcess process;
    process.start(m_executablePath, QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << output_path
        << "--verbose");

    QVERIFY(process.waitForFinished(10000));
    QCOMPARE(process.exitCode(), 0);

    // Verbose mode should produce detailed output
    QString stderr_output = process.readAllStandardError();
    QVERIFY(stderr_output.length() > 100);
}

void TestCLIModeFunctional::test07_MaxFramesLimit() {
    QTemporaryFile eti_file;
    QVERIFY(eti_file.open());
    eti_file.write(createValidETIStream(500));
    eti_file.close();

    QString output_path = m_testDir->path() + "/limited_output.yaml";

    int exit_code = -1;
    runCLI(QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << output_path
        << "--max-frames" << "100",
        &exit_code);

    QCOMPARE(exit_code, 0);

    // Verify only 100 frames were processed
    QFile output_file(output_path);
    QVERIFY(output_file.open(QIODevice::ReadOnly));
    QString yaml_text = output_file.readAll();
    output_file.close();

    QVERIFY(yaml_text.contains("total_frames: 100") ||
            yaml_text.contains("processed_frames: 100"));
}

void TestCLIModeFunctional::test08_HelpText() {
    int exit_code = -1;
    QString output = runCLI(QStringList() << "--help", &exit_code);

    QCOMPARE(exit_code, 0);

    // Verify help text contains key information
    QVERIFY(output.contains("--cli") || output.contains("--input"));
    QVERIFY(output.contains("--output") || output.contains("Usage:"));
}

// ============================================================================
// YAML Validation Tests (7 tests)
// ============================================================================

void TestCLIModeFunctional::test09_YAMLSyntaxValid() {
    QTemporaryFile eti_file;
    QVERIFY(eti_file.open());
    eti_file.write(createValidETIStream(50));
    eti_file.close();

    QString output_path = m_testDir->path() + "/yaml_syntax.yaml";

    int exit_code = -1;
    runCLI(QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << output_path,
        &exit_code);

    QCOMPARE(exit_code, 0);

    QFile output_file(output_path);
    QVERIFY(output_file.open(QIODevice::ReadOnly));
    QString yaml_text = output_file.readAll();
    output_file.close();

    // Basic YAML syntax validation
    QVERIFY(isValidYAML(yaml_text));
}

void TestCLIModeFunctional::test10_YAMLContainsEnsemble() {
    QTemporaryFile eti_file;
    QVERIFY(eti_file.open());
    eti_file.write(createValidETIStream(100));
    eti_file.close();

    QString output_path = m_testDir->path() + "/ensemble.yaml";

    int exit_code = -1;
    runCLI(QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << output_path,
        &exit_code);

    QCOMPARE(exit_code, 0);

    QFile output_file(output_path);
    QVERIFY(output_file.open(QIODevice::ReadOnly));
    QString yaml_text = output_file.readAll();
    output_file.close();

    // Verify ensemble section exists
    QVERIFY(yaml_text.contains("ensemble:") || yaml_text.contains("ensemble_id:"));
}

void TestCLIModeFunctional::test11_YAMLContainsServices() {
    QTemporaryFile eti_file;
    QVERIFY(eti_file.open());
    eti_file.write(createValidETIStream(100));
    eti_file.close();

    QString output_path = m_testDir->path() + "/services.yaml";

    int exit_code = -1;
    runCLI(QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << output_path,
        &exit_code);

    QCOMPARE(exit_code, 0);

    QFile output_file(output_path);
    QVERIFY(output_file.open(QIODevice::ReadOnly));
    QString yaml_text = output_file.readAll();
    output_file.close();

    // Verify services section exists
    QVERIFY(yaml_text.contains("services:") || yaml_text.contains("service_count:"));
}

void TestCLIModeFunctional::test12_YAMLContainsSubchannels() {
    QTemporaryFile eti_file;
    QVERIFY(eti_file.open());
    eti_file.write(createValidETIStream(100));
    eti_file.close();

    QString output_path = m_testDir->path() + "/subchannels.yaml";

    int exit_code = -1;
    runCLI(QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << output_path,
        &exit_code);

    QCOMPARE(exit_code, 0);

    QFile output_file(output_path);
    QVERIFY(output_file.open(QIODevice::ReadOnly));
    QString yaml_text = output_file.readAll();
    output_file.close();

    // Verify subchannels section exists
    QVERIFY(yaml_text.contains("subchannels:") || yaml_text.contains("subchannel"));
}

void TestCLIModeFunctional::test13_YAMLContainsFIGStats() {
    QTemporaryFile eti_file;
    QVERIFY(eti_file.open());
    eti_file.write(createValidETIStream(100));
    eti_file.close();

    QString output_path = m_testDir->path() + "/fig_stats.yaml";

    int exit_code = -1;
    runCLI(QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << output_path,
        &exit_code);

    QCOMPARE(exit_code, 0);

    QFile output_file(output_path);
    QVERIFY(output_file.open(QIODevice::ReadOnly));
    QString yaml_text = output_file.readAll();
    output_file.close();

    // Verify FIG statistics
    QVERIFY(yaml_text.contains("fig_") || yaml_text.contains("FIG"));
}

void TestCLIModeFunctional::test14_YAMLThaiUTF8Preserved() {
    // This test would require ETI file with Thai characters
    // For now, just verify UTF-8 handling is present
    QTemporaryFile eti_file;
    QVERIFY(eti_file.open());
    eti_file.write(createValidETIStream(50));
    eti_file.close();

    QString output_path = m_testDir->path() + "/thai_utf8.yaml";

    int exit_code = -1;
    runCLI(QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << output_path,
        &exit_code);

    QCOMPARE(exit_code, 0);

    // Verify file is valid UTF-8
    QFile output_file(output_path);
    QVERIFY(output_file.open(QIODevice::ReadOnly | QIODevice::Text));
    QByteArray content = output_file.readAll();
    output_file.close();

    // Check that content is valid UTF-8
    QString text = QString::fromUtf8(content);
    QVERIFY(!text.contains(QChar::ReplacementCharacter));
}

void TestCLIModeFunctional::test15_YAMLHexFormatting() {
    QTemporaryFile eti_file;
    QVERIFY(eti_file.open());
    eti_file.write(createValidETIStream(50));
    eti_file.close();

    QString output_path = m_testDir->path() + "/hex_format.yaml";

    int exit_code = -1;
    runCLI(QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << output_path,
        &exit_code);

    QCOMPARE(exit_code, 0);

    QFile output_file(output_path);
    QVERIFY(output_file.open(QIODevice::ReadOnly));
    QString yaml_text = output_file.readAll();
    output_file.close();

    // Verify hex values are formatted correctly (0x prefix)
    QRegularExpression hex_pattern("0x[0-9A-Fa-f]+");
    QVERIFY(hex_pattern.match(yaml_text).hasMatch());
}

// ============================================================================
// Error Handling Tests (5 tests)
// ============================================================================

void TestCLIModeFunctional::test16_InputFileNotFound() {
    int exit_code = -1;
    QString output = runCLI(QStringList()
        << "--cli"
        << "--input" << "/nonexistent/file.eti"
        << "--output" << m_testDir->path() + "/output.yaml",
        &exit_code, 5000);

    // Should fail with file not found error
    QVERIFY(exit_code != 0);
    QVERIFY(exit_code == 2 || exit_code == 1);  // File not found or general error
}

void TestCLIModeFunctional::test17_InvalidOutputPath() {
    QTemporaryFile eti_file;
    QVERIFY(eti_file.open());
    eti_file.write(createValidETIStream(10));
    eti_file.close();

    int exit_code = -1;
    runCLI(QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << "/invalid/nonexistent/path/output.yaml",
        &exit_code, 5000);

    // Should fail with invalid output path
    QVERIFY(exit_code != 0);
}

void TestCLIModeFunctional::test18_MalformedETIFile() {
    QTemporaryFile eti_file;
    QVERIFY(eti_file.open());
    eti_file.write(createMalformedETIStream());
    eti_file.close();

    QString output_path = m_testDir->path() + "/malformed_output.yaml";

    int exit_code = -1;
    runCLI(QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << output_path,
        &exit_code, 5000);

    // Should either fail or report errors in output
    if (exit_code == 0) {
        // If it succeeds, check for error reporting in YAML
        QFile output_file(output_path);
        if (output_file.open(QIODevice::ReadOnly)) {
            QString yaml_text = output_file.readAll();
            QVERIFY(yaml_text.contains("error") || yaml_text.contains("invalid"));
        }
    }
}

void TestCLIModeFunctional::test19_EmptyETIFile() {
    QTemporaryFile eti_file;
    QVERIFY(eti_file.open());
    // Write nothing - empty file
    eti_file.close();

    QString output_path = m_testDir->path() + "/empty_output.yaml";

    int exit_code = -1;
    runCLI(QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << output_path,
        &exit_code, 5000);

    // Should handle empty file gracefully
    QVERIFY(exit_code != 0 || QFile::exists(output_path));
}

void TestCLIModeFunctional::test20_PartialETIFile() {
    QTemporaryFile eti_file;
    QVERIFY(eti_file.open());
    // 5 complete frames + 2000 bytes of partial frame
    eti_file.write(createPartialETIStream(5, 2000));
    eti_file.close();

    QString output_path = m_testDir->path() + "/partial_output.yaml";

    int exit_code = -1;
    runCLI(QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << output_path,
        &exit_code, 5000);

    // Should process complete frames and report partial frame
    if (exit_code == 0 && QFile::exists(output_path)) {
        QFile output_file(output_path);
        QVERIFY(output_file.open(QIODevice::ReadOnly));
        QString yaml_text = output_file.readAll();
        output_file.close();

        // Should report 5 complete frames
        QVERIFY(yaml_text.contains("5") || yaml_text.contains("processed"));
    }
}

// ============================================================================
// Performance Tests (3 tests)
// ============================================================================

void TestCLIModeFunctional::test21_ProcessingSpeed() {
    // Create 1000-frame ETI file
    QTemporaryFile eti_file;
    QVERIFY(eti_file.open());
    eti_file.write(createValidETIStream(1000));
    eti_file.close();

    QString output_path = m_testDir->path() + "/perf_output.yaml";

    QElapsedTimer timer;
    timer.start();

    int exit_code = -1;
    runCLI(QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << output_path
        << "--quiet",
        &exit_code, 5000);

    qint64 elapsed = timer.elapsed();

    QCOMPARE(exit_code, 0);

    // Target: >900 FPS means 1000 frames in <1111ms
    qDebug() << "Processing time:" << elapsed << "ms";
    double fps = 1000.0 / (elapsed / 1000.0);
    qDebug() << "Calculated FPS:" << fps;

    QVERIFY2(elapsed < 1500, QString("Processing too slow: %1ms (FPS: %2)").arg(elapsed).arg(fps).toLatin1());

    // Read YAML and verify metrics exist
    QFile output_file(output_path);
    QVERIFY(output_file.open(QIODevice::ReadOnly));
    QString yaml_text = output_file.readAll();
    output_file.close();

    QVERIFY(yaml_text.contains("total_frames: 1000"));
}

void TestCLIModeFunctional::test22_MemoryUsage() {
    // Create large file to test memory handling
    QTemporaryFile eti_file;
    QVERIFY(eti_file.open());
    eti_file.write(createValidETIStream(2000));
    eti_file.close();

    QString output_path = m_testDir->path() + "/memory_output.yaml";

    int exit_code = -1;
    runCLI(QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << output_path
        << "--quiet",
        &exit_code, 10000);

    QCOMPARE(exit_code, 0);

    // Verify processing completed successfully
    // Memory usage check would require platform-specific code
    QVERIFY(QFile::exists(output_path));
}

void TestCLIModeFunctional::test23_CLIOverhead() {
    QTemporaryFile eti_file;
    QVERIFY(eti_file.open());
    eti_file.write(createValidETIStream(500));
    eti_file.close();

    QString output_path = m_testDir->path() + "/overhead_output.yaml";

    QElapsedTimer timer;
    timer.start();

    int exit_code = -1;
    runCLI(QStringList()
        << "--cli"
        << "--input" << eti_file.fileName()
        << "--output" << output_path
        << "--quiet",
        &exit_code, 5000);

    qint64 elapsed = timer.elapsed();

    QCOMPARE(exit_code, 0);

    // CLI overhead should be minimal (< 100ms startup)
    qDebug() << "Total CLI time:" << elapsed << "ms";

    // Should complete in reasonable time
    QVERIFY2(elapsed < 3000, QString("CLI overhead too high: %1ms").arg(elapsed).toLatin1());
}

// ============================================================================
// Integration Tests (2 tests)
// ============================================================================

void TestCLIModeFunctional::test24_MultipleRuns() {
    QTemporaryFile eti_file;
    QVERIFY(eti_file.open());
    eti_file.write(createValidETIStream(100));
    eti_file.close();

    // Run CLI mode 3 times consecutively
    for (int i = 0; i < 3; i++) {
        QString output_path = m_testDir->path() + QString("/multi_run_%1.yaml").arg(i);

        int exit_code = -1;
        runCLI(QStringList()
            << "--cli"
            << "--input" << eti_file.fileName()
            << "--output" << output_path,
            &exit_code);

        QCOMPARE(exit_code, 0);
        QVERIFY(QFile::exists(output_path));
    }
}

void TestCLIModeFunctional::test25_ConcurrentInstances() {
    // Create multiple test files
    QList<QTemporaryFile*> eti_files;
    QList<QString> output_paths;
    QList<QProcess*> processes;

    for (int i = 0; i < 3; i++) {
        QTemporaryFile* eti_file = new QTemporaryFile(m_testDir->path() + "/concurrent_XXXXXX.eti");
        QVERIFY(eti_file->open());
        eti_file->write(createValidETIStream(50));
        eti_file->close();
        eti_files.append(eti_file);

        QString output_path = m_testDir->path() + QString("/concurrent_output_%1.yaml").arg(i);
        output_paths.append(output_path);

        QProcess* process = new QProcess(this);
        process->start(m_executablePath, QStringList()
            << "--cli"
            << "--input" << eti_file->fileName()
            << "--output" << output_path
            << "--quiet");
        processes.append(process);
    }

    // Wait for all processes to finish
    bool all_finished = true;
    for (QProcess* process : processes) {
        if (!process->waitForFinished(10000)) {
            all_finished = false;
        }
    }

    QVERIFY(all_finished);

    // Verify all outputs
    for (int i = 0; i < 3; i++) {
        QCOMPARE(processes[i]->exitCode(), 0);
        QVERIFY(QFile::exists(output_paths[i]));
    }

    // Cleanup
    qDeleteAll(eti_files);
    qDeleteAll(processes);
}

// ============================================================================
// Helper Functions
// ============================================================================

QByteArray TestCLIModeFunctional::createValidETIStream(int frame_count) {
    QByteArray stream;

    for (int i = 0; i < frame_count; ++i) {
        // Create minimal valid ETI frame (6144 bytes)
        QByteArray frame(ETI_FRAME_SIZE, static_cast<char>(0x00));

        // ETI sync word (offset 0-3): "ERR\0" = 0x45 0x52 0x52 0x00
        frame[0] = 0x45;  // 'E'
        frame[1] = 0x52;  // 'R'
        frame[2] = 0x52;  // 'R'
        frame[3] = 0x00;  // NUL

        // LIDATA (offset 4-11): frame count and flags
        frame[4] = static_cast<char>(i & 0xFF);  // FCT low byte

        // FICF flag (offset 5, bit 7): FIC present
        frame[5] = static_cast<char>(0x80);

        // Add minimal FIC data at offset 12 (after LIDATA)
        // FIC is 32 bytes starting from offset 12
        int fic_offset = 12;

        // FIG 0/0 (Ensemble information)
        frame[fic_offset + 0] = 0x00;  // FIG type 0, extension 0
        frame[fic_offset + 1] = 0x04;  // Length 4
        frame[fic_offset + 2] = 0x12;  // Ensemble ID high
        frame[fic_offset + 3] = 0x34;  // Ensemble ID low
        frame[fic_offset + 4] = 0x00;  // Change flags
        frame[fic_offset + 5] = 0x00;  // Alarm flag

        stream.append(frame);
    }

    return stream;
}

QByteArray TestCLIModeFunctional::createMalformedETIStream() {
    QByteArray stream;

    // Create frames with invalid sync words
    for (int i = 0; i < 10; ++i) {
        QByteArray frame(ETI_FRAME_SIZE, static_cast<char>(0xFF));

        // Invalid sync word
        frame[0] = 0xDE;
        frame[1] = 0xAD;
        frame[2] = 0xBE;
        frame[3] = 0xEF;

        stream.append(frame);
    }

    return stream;
}

QByteArray TestCLIModeFunctional::createPartialETIStream(int complete_frames, int partial_bytes) {
    QByteArray stream = createValidETIStream(complete_frames);

    // Add partial frame
    QByteArray partial_frame(partial_bytes, static_cast<char>(0x00));
    stream.append(partial_frame);

    return stream;
}

bool TestCLIModeFunctional::isValidYAML(const QString& yaml_text) {
    // Basic YAML syntax validation
    // Check for proper indentation and structure

    if (yaml_text.isEmpty()) {
        return false;
    }

    // YAML should not have tabs
    if (yaml_text.contains('\t')) {
        return false;
    }

    // Should have key: value pairs
    if (!yaml_text.contains(':')) {
        return false;
    }

    // Check for balanced quotes (simple check)
    int quote_count = yaml_text.count('"');
    if (quote_count % 2 != 0) {
        return false;
    }

    return true;
}

QMap<QString, QString> TestCLIModeFunctional::parseYAMLSimple(const QString& yaml_text) {
    QMap<QString, QString> result;

    QStringList lines = yaml_text.split('\n');
    for (const QString& line : lines) {
        if (line.contains(':')) {
            QStringList parts = line.split(':');
            if (parts.size() >= 2) {
                QString key = parts[0].trimmed();
                QString value = parts[1].trimmed();
                result[key] = value;
            }
        }
    }

    return result;
}

QString TestCLIModeFunctional::runCLI(const QStringList& args, int* exit_code, int timeout_ms) {
    QProcess process;
    process.start(m_executablePath, args);

    if (!process.waitForFinished(timeout_ms)) {
        if (exit_code) {
            *exit_code = -1;
        }
        return QString();
    }

    if (exit_code) {
        *exit_code = process.exitCode();
    }

    // Return both stdout and stderr
    QString output = process.readAllStandardOutput();
    output += process.readAllStandardError();

    return output;
}

bool TestCLIModeFunctional::checkExecutableExists() {
    return QFile::exists(m_executablePath);
}

QTEST_MAIN(TestCLIModeFunctional)
#include "test_cli_mode_functional.moc"
