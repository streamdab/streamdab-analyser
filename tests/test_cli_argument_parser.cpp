#include <QTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include "../src/cli/cli_argument_parser.hpp"

/**
 * @brief Test suite for CLIArgumentParser
 *
 * Comprehensive testing of command-line argument parsing with 15+ tests
 * covering basic parsing, validation, help/version, and edge cases.
 *
 * Target: 15/15 tests passing (100%)
 */
class TestCLIArgumentParser : public QObject {
    Q_OBJECT

private:
    QTemporaryFile* tempInputFile{nullptr};
    QTemporaryDir* tempDir{nullptr};

private slots:
    void initTestCase() {
        // Create temporary input file for testing
        tempDir = new QTemporaryDir();
        QVERIFY(tempDir->isValid());

        tempInputFile = new QTemporaryFile(tempDir->path() + "/test_XXXXXX.eti");
        QVERIFY(tempInputFile->open());
        tempInputFile->write("ETI test data");
        tempInputFile->flush();
        // Keep file open so it exists during tests
    }

    void cleanupTestCase() {
        delete tempInputFile;
        delete tempDir;
    }

    // ========================================
    // BASIC PARSING TESTS (5 tests minimum)
    // ========================================

    void test01_ParseInputFile() {
        QByteArray path = tempInputFile->fileName().toUtf8();
        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--input"),
            path.data()
        };

        try {
            auto options = cli::CLIArgumentParser::parse(3, argv);
            QVERIFY(options.cli_mode);
            QCOMPARE(options.input_file, QString::fromUtf8(path));
            QVERIFY(!options.show_help);
            QVERIFY(!options.show_version);
        } catch (const std::exception& e) {
            QFAIL(QString("Exception thrown: %1").arg(e.what()).toUtf8().constData());
        }
    }

    void test02_ParseOutputFile() {
        QByteArray inputPath = tempInputFile->fileName().toUtf8();
        QString outputPath = tempDir->path() + "/output.yaml";
        QByteArray outputBytes = outputPath.toUtf8();

        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--input"),
            inputPath.data(),
            const_cast<char*>("--output"),
            outputBytes.data()
        };

        try {
            auto options = cli::CLIArgumentParser::parse(5, argv);
            QCOMPARE(options.output_file, outputPath);
        } catch (const std::exception& e) {
            QFAIL(QString("Exception thrown: %1").arg(e.what()).toUtf8().constData());
        }
    }

    void test03_ParseQuietFlag() {
        QByteArray path = tempInputFile->fileName().toUtf8();
        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--input"),
            path.data(),
            const_cast<char*>("--quiet")
        };

        try {
            auto options = cli::CLIArgumentParser::parse(4, argv);
            QVERIFY(options.quiet);
            QVERIFY(!options.verbose);
        } catch (const std::exception& e) {
            QFAIL(QString("Exception thrown: %1").arg(e.what()).toUtf8().constData());
        }
    }

    void test04_ParseVerboseFlag() {
        QByteArray path = tempInputFile->fileName().toUtf8();
        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--input"),
            path.data(),
            const_cast<char*>("--verbose")
        };

        try {
            auto options = cli::CLIArgumentParser::parse(4, argv);
            QVERIFY(options.verbose);
            QVERIFY(!options.quiet);
        } catch (const std::exception& e) {
            QFAIL(QString("Exception thrown: %1").arg(e.what()).toUtf8().constData());
        }
    }

    void test05_ParseMaxFrames() {
        QByteArray path = tempInputFile->fileName().toUtf8();
        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--input"),
            path.data(),
            const_cast<char*>("--max-frames"),
            const_cast<char*>("1000")
        };

        try {
            auto options = cli::CLIArgumentParser::parse(5, argv);
            QCOMPARE(options.max_frames, 1000);
        } catch (const std::exception& e) {
            QFAIL(QString("Exception thrown: %1").arg(e.what()).toUtf8().constData());
        }
    }

    // ========================================
    // VALIDATION TESTS (5 tests minimum)
    // ========================================

    void test06_InputFileNotFound() {
        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--input"),
            const_cast<char*>("/nonexistent/file.eti")
        };

        bool exceptionThrown = false;
        try {
            cli::CLIArgumentParser::parse(3, argv);
        } catch (const std::runtime_error& e) {
            exceptionThrown = true;
            QString msg(e.what());
            QVERIFY(msg.contains("not found") || msg.contains("not readable"));
        } catch (const std::exception& e) {
            QFAIL(QString("Wrong exception type: %1").arg(e.what()).toUtf8().constData());
        }
        QVERIFY2(exceptionThrown, "Expected runtime_error for nonexistent file");
    }

    void test07_InvalidOutputPath() {
        QByteArray path = tempInputFile->fileName().toUtf8();
        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--input"),
            path.data(),
            const_cast<char*>("--output"),
            const_cast<char*>("/nonexistent/dir/output.yaml")
        };

        bool exceptionThrown = false;
        try {
            cli::CLIArgumentParser::parse(5, argv);
        } catch (const std::runtime_error& e) {
            exceptionThrown = true;
            QString msg(e.what());
            QVERIFY(msg.contains("not writable") || msg.contains("directory does not exist"));
        } catch (const std::exception& e) {
            QFAIL(QString("Wrong exception type: %1").arg(e.what()).toUtf8().constData());
        }
        QVERIFY2(exceptionThrown, "Expected runtime_error for invalid output path");
    }

    void test08_MissingRequiredInput() {
        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--cli")
        };

        bool exceptionThrown = false;
        try {
            cli::CLIArgumentParser::parse(2, argv);
        } catch (const std::invalid_argument& e) {
            exceptionThrown = true;
            QString msg(e.what());
            QVERIFY(msg.contains("requires") || msg.contains("input"));
        } catch (const std::exception& e) {
            QFAIL(QString("Wrong exception type: %1").arg(e.what()).toUtf8().constData());
        }
        QVERIFY2(exceptionThrown, "Expected invalid_argument for missing input");
    }

    void test09_ConflictingFlags() {
        QByteArray path = tempInputFile->fileName().toUtf8();
        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--input"),
            path.data(),
            const_cast<char*>("--quiet"),
            const_cast<char*>("--verbose")
        };

        bool exceptionThrown = false;
        try {
            cli::CLIArgumentParser::parse(5, argv);
        } catch (const std::invalid_argument& e) {
            exceptionThrown = true;
            QString msg(e.what());
            QVERIFY(msg.contains("quiet") && msg.contains("verbose"));
        } catch (const std::exception& e) {
            QFAIL(QString("Wrong exception type: %1").arg(e.what()).toUtf8().constData());
        }
        QVERIFY2(exceptionThrown, "Expected invalid_argument for conflicting flags");
    }

    void test10_InvalidMaxFrames() {
        QByteArray path = tempInputFile->fileName().toUtf8();
        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--input"),
            path.data(),
            const_cast<char*>("--max-frames"),
            const_cast<char*>("not_a_number")
        };

        bool exceptionThrown = false;
        try {
            cli::CLIArgumentParser::parse(5, argv);
        } catch (const std::invalid_argument& e) {
            exceptionThrown = true;
            QString msg(e.what());
            QVERIFY(msg.contains("integer") || msg.contains("max-frames"));
        } catch (const std::exception& e) {
            QFAIL(QString("Wrong exception type: %1").arg(e.what()).toUtf8().constData());
        }
        QVERIFY2(exceptionThrown, "Expected invalid_argument for invalid max-frames");
    }

    // ========================================
    // HELP AND VERSION TESTS (3 tests minimum)
    // ========================================

    void test11_ShowHelp() {
        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--help")
        };

        try {
            auto options = cli::CLIArgumentParser::parse(2, argv);
            QVERIFY(options.show_help);
            QVERIFY(!options.show_version);
            QVERIFY(!options.cli_mode);
        } catch (const std::exception& e) {
            QFAIL(QString("Exception thrown: %1").arg(e.what()).toUtf8().constData());
        }
    }

    void test12_ShowVersion() {
        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--version")
        };

        try {
            auto options = cli::CLIArgumentParser::parse(2, argv);
            QVERIFY(options.show_version);
            QVERIFY(!options.show_help);
            QVERIFY(!options.cli_mode);
        } catch (const std::exception& e) {
            QFAIL(QString("Exception thrown: %1").arg(e.what()).toUtf8().constData());
        }
    }

    void test13_HelpOverridesOtherArgs() {
        QByteArray path = tempInputFile->fileName().toUtf8();
        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--input"),
            path.data(),
            const_cast<char*>("--help"),
            const_cast<char*>("--verbose")
        };

        try {
            auto options = cli::CLIArgumentParser::parse(5, argv);
            QVERIFY(options.show_help);
            // Help should override everything else
            QVERIFY(options.input_file.isEmpty());
        } catch (const std::exception& e) {
            QFAIL(QString("Exception thrown: %1").arg(e.what()).toUtf8().constData());
        }
    }

    // ========================================
    // EDGE CASES (2 tests minimum)
    // ========================================

    void test14_LongFilePaths() {
        // Create file with long path
        QString longName = "very_long_filename_to_test_path_handling_" + QString("x").repeated(100) + ".eti";
        QTemporaryFile longFile(tempDir->path() + "/" + longName);
        QVERIFY(longFile.open());
        longFile.write("ETI data");
        longFile.flush();

        QByteArray path = longFile.fileName().toUtf8();
        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--input"),
            path.data()
        };

        try {
            auto options = cli::CLIArgumentParser::parse(3, argv);
            QVERIFY(options.cli_mode);
            QVERIFY(!options.input_file.isEmpty());
        } catch (const std::exception& e) {
            QFAIL(QString("Exception thrown: %1").arg(e.what()).toUtf8().constData());
        }
    }

    void test15_SpecialCharactersInPath() {
        // Create file with special characters (spaces, unicode)
        QString specialName = "test file with spaces and 中文.eti";
        QTemporaryFile specialFile(tempDir->path() + "/" + specialName);
        QVERIFY(specialFile.open());
        specialFile.write("ETI data");
        specialFile.flush();

        QByteArray path = specialFile.fileName().toUtf8();
        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--input"),
            path.data()
        };

        try {
            auto options = cli::CLIArgumentParser::parse(3, argv);
            QVERIFY(options.cli_mode);
            QVERIFY(!options.input_file.isEmpty());
        } catch (const std::exception& e) {
            QFAIL(QString("Exception thrown: %1").arg(e.what()).toUtf8().constData());
        }
    }

    // ========================================
    // ADDITIONAL COMPREHENSIVE TESTS
    // ========================================

    void test16_ShortHelpFlag() {
        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("-h")
        };

        try {
            auto options = cli::CLIArgumentParser::parse(2, argv);
            QVERIFY(options.show_help);
        } catch (const std::exception& e) {
            QFAIL(QString("Exception thrown: %1").arg(e.what()).toUtf8().constData());
        }
    }

    void test17_ShortVersionFlag() {
        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("-v")
        };

        try {
            auto options = cli::CLIArgumentParser::parse(2, argv);
            QVERIFY(options.show_version);
        } catch (const std::exception& e) {
            QFAIL(QString("Exception thrown: %1").arg(e.what()).toUtf8().constData());
        }
    }

    void test18_NoArgumentsReturnsDefault() {
        char* argv[] = {
            const_cast<char*>("program")
        };

        try {
            auto options = cli::CLIArgumentParser::parse(1, argv);
            QVERIFY(!options.cli_mode);
            QVERIFY(!options.show_help);
            QVERIFY(!options.show_version);
            QVERIFY(options.input_file.isEmpty());
        } catch (const std::exception& e) {
            QFAIL(QString("Exception thrown: %1").arg(e.what()).toUtf8().constData());
        }
    }

    void test19_MaxFramesBoundaryValidation() {
        QByteArray path = tempInputFile->fileName().toUtf8();

        // Test zero (invalid - minimum is 1)
        char* argv_zero[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--input"),
            path.data(),
            const_cast<char*>("--max-frames"),
            const_cast<char*>("0")
        };

        bool exceptionThrown = false;
        try {
            cli::CLIArgumentParser::parse(5, argv_zero);
        } catch (const std::invalid_argument&) {
            exceptionThrown = true;
        }
        QVERIFY2(exceptionThrown, "Expected exception for max-frames=0");

        // Test negative (invalid)
        char* argv_neg[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--input"),
            path.data(),
            const_cast<char*>("--max-frames"),
            const_cast<char*>("-100")
        };

        exceptionThrown = false;
        try {
            cli::CLIArgumentParser::parse(5, argv_neg);
        } catch (const std::invalid_argument&) {
            exceptionThrown = true;
        }
        QVERIFY2(exceptionThrown, "Expected exception for negative max-frames");

        // Test valid boundary (1)
        char* argv_valid[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--input"),
            path.data(),
            const_cast<char*>("--max-frames"),
            const_cast<char*>("1")
        };

        try {
            auto options = cli::CLIArgumentParser::parse(5, argv_valid);
            QCOMPARE(options.max_frames, 1);
        } catch (const std::exception& e) {
            QFAIL(QString("Exception thrown for valid value: %1").arg(e.what()).toUtf8().constData());
        }
    }

    void test20_UnknownArgumentDetection() {
        QByteArray path = tempInputFile->fileName().toUtf8();
        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--input"),
            path.data(),
            const_cast<char*>("--unknown-flag")
        };

        bool exceptionThrown = false;
        try {
            cli::CLIArgumentParser::parse(4, argv);
        } catch (const std::invalid_argument& e) {
            exceptionThrown = true;
            QString msg(e.what());
            QVERIFY(msg.contains("Unknown") || msg.contains("unknown-flag"));
        }
        QVERIFY2(exceptionThrown, "Expected exception for unknown argument");
    }

    void test21_EmptyFile() {
        // Create empty file
        QTemporaryFile emptyFile(tempDir->path() + "/empty_XXXXXX.eti");
        QVERIFY(emptyFile.open());
        // Don't write anything - leave empty
        emptyFile.flush();

        QByteArray path = emptyFile.fileName().toUtf8();
        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--input"),
            path.data()
        };

        bool exceptionThrown = false;
        try {
            cli::CLIArgumentParser::parse(3, argv);
        } catch (const std::runtime_error& e) {
            exceptionThrown = true;
            // Empty files should be rejected by validation
        }
        QVERIFY2(exceptionThrown, "Expected exception for empty input file");
    }

    void test22_MissingValueForArgument() {
        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--input")
            // Missing value!
        };

        bool exceptionThrown = false;
        try {
            cli::CLIArgumentParser::parse(2, argv);
        } catch (const std::invalid_argument& e) {
            exceptionThrown = true;
            QString msg(e.what());
            QVERIFY(msg.contains("requires"));
        }
        QVERIFY2(exceptionThrown, "Expected exception for missing argument value");
    }

    void test23_CLIModeAutoDetection() {
        QByteArray path = tempInputFile->fileName().toUtf8();
        char* argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--input"),
            path.data()
            // Note: no --cli flag
        };

        try {
            auto options = cli::CLIArgumentParser::parse(3, argv);
            // Should auto-detect CLI mode when --input is provided
            QVERIFY(options.cli_mode);
        } catch (const std::exception& e) {
            QFAIL(QString("Exception thrown: %1").arg(e.what()).toUtf8().constData());
        }
    }

    void test24_PrintUsageFunctionality() {
        // Test that printUsage doesn't crash
        // (Can't easily capture stdout in QTest, but we can verify it doesn't throw)
        try {
            cli::CLIArgumentParser::printUsage();
            QVERIFY(true); // If we get here, it didn't crash
        } catch (const std::exception& e) {
            QFAIL(QString("printUsage threw exception: %1").arg(e.what()).toUtf8().constData());
        }
    }

    void test25_PrintVersionFunctionality() {
        // Test that printVersion doesn't crash
        try {
            cli::CLIArgumentParser::printVersion();
            QVERIFY(true); // If we get here, it didn't crash
        } catch (const std::exception& e) {
            QFAIL(QString("printVersion threw exception: %1").arg(e.what()).toUtf8().constData());
        }
    }
};

QTEST_MAIN(TestCLIArgumentParser)
#include "test_cli_argument_parser.moc"
