#include "cli_argument_parser.hpp"
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <iostream>
#include <stdexcept>

namespace cli {

CLIOptions CLIArgumentParser::parse(int argc, char** argv) {
    CLIOptions options;

    // If no arguments, return default (GUI mode)
    if (argc <= 1) {
        return options;
    }

    // First pass: Check for help/version flags which override everything
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromUtf8(argv[i]);
        if (arg == "--help" || arg == "-h") {
            options.show_help = true;
            return options;
        }
        if (arg == "--version" || arg == "-v") {
            options.show_version = true;
            return options;
        }
    }

    // Second pass: Parse all other arguments
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromUtf8(argv[i]);

        // CLI mode flag
        if (arg == "--cli") {
            options.cli_mode = true;
            continue;
        }

        // Quiet flag
        if (arg == "--quiet") {
            if (options.verbose) {
                throw std::invalid_argument("Cannot use both --quiet and --verbose");
            }
            options.quiet = true;
            continue;
        }

        // Verbose flag
        if (arg == "--verbose") {
            if (options.quiet) {
                throw std::invalid_argument("Cannot use both --quiet and --verbose");
            }
            options.verbose = true;
            continue;
        }

        // Input file (requires value)
        if (arg == "--input") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("--input requires a file path");
            }
            options.input_file = QString::fromUtf8(argv[++i]);

            if (!isValidETIFile(options.input_file)) {
                throw std::runtime_error(
                    QString("Input file not found or not readable: %1")
                        .arg(options.input_file)
                        .toStdString()
                );
            }
            continue;
        }

        // Output file (requires value)
        if (arg == "--output") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("--output requires a file path");
            }
            options.output_file = QString::fromUtf8(argv[++i]);

            if (!isValidOutputPath(options.output_file)) {
                throw std::runtime_error(
                    QString("Output path not writable: %1")
                        .arg(options.output_file)
                        .toStdString()
                );
            }
            continue;
        }

        // Max frames (requires value)
        if (arg == "--max-frames") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("--max-frames requires a positive integer");
            }
            QString value = QString::fromUtf8(argv[++i]);
            options.max_frames = parseIntArg("--max-frames", value, 1, 1000000);
            continue;
        }

        // Stream URI (future, placeholder)
        if (arg == "--stream") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("--stream requires a URI");
            }
            options.stream_uri = QString::fromUtf8(argv[++i]);
            // No validation yet - future feature
            continue;
        }

        // Unknown argument
        throw std::invalid_argument(
            QString("Unknown argument: %1").arg(arg).toStdString()
        );
    }

    // Auto-detect CLI mode if --input provided without explicit --cli
    if (!options.input_file.isEmpty() && !options.cli_mode) {
        options.cli_mode = true;
    }

    // Validation: CLI mode requires input file
    if (options.cli_mode && options.input_file.isEmpty() && !options.show_help && !options.show_version) {
        throw std::invalid_argument(
            "CLI mode requires --input <file> argument"
        );
    }

    return options;
}

void CLIArgumentParser::printUsage() {
    QTextStream out(stdout);
    out << "StreamDAB Analyzer v1.1 - CLI Mode\n";
    out << "ETSI EN 300 401 compliant DAB/DAB+ stream analyzer\n\n";

    out << "USAGE:\n";
    out << "  streamdab-analyzer --input <file> [OPTIONS]\n";
    out << "  streamdab-analyzer --help\n";
    out << "  streamdab-analyzer --version\n\n";

    out << "REQUIRED ARGUMENTS:\n";
    out << "  --input <file>          ETI file to analyze (.eti format)\n\n";

    out << "OPTIONAL ARGUMENTS:\n";
    out << "  --output <file>         YAML output file (default: stdout)\n";
    out << "  --quiet                 Minimal console output (errors only)\n";
    out << "  --verbose               Detailed progress logging\n";
    out << "  --max-frames <n>        Process first N frames only (default: all)\n";
    out << "  --cli                   Explicitly enable CLI mode (auto-detected)\n\n";

    out << "INFORMATION:\n";
    out << "  --help, -h              Show this help message and exit\n";
    out << "  --version, -v           Show version information and exit\n\n";

    out << "EXAMPLES:\n";
    out << "  # Analyze ETI file to stdout (YAML format)\n";
    out << "  streamdab-analyzer --input sample.eti\n\n";

    out << "  # Save analysis to file with quiet mode\n";
    out << "  streamdab-analyzer --input sample.eti --output report.yaml --quiet\n\n";

    out << "  # Analyze first 1000 frames with verbose logging\n";
    out << "  streamdab-analyzer --input stream.eti --max-frames 1000 --verbose\n\n";

    out << "EXIT CODES:\n";
    out << "  0   Success\n";
    out << "  1   Invalid arguments\n";
    out << "  2   File not found or not readable\n";
    out << "  3   Processing error\n\n";

    out << "SUPPORTED FEATURES:\n";
    out << "  - ETSI EN 300 401 compliant ETI frame parsing\n";
    out << "  - FIG Type 0/0 through 0/19 decoding (20 types)\n";
    out << "  - FIG Type 1/0, 1/1, 1/4, 1/5 (ensemble/service labels)\n";
    out << "  - Service discovery and metadata extraction\n";
    out << "  - DAB+ stream validation (ETSI TS 102 563)\n";
    out << "  - ETSI TS 101 756 registered tables (458 entries)\n";
    out << "  - Thai UTF-8 support throughout\n\n";

    out << "For more information, visit: https://github.com/seksan/streamdab-analyzer\n";
    out.flush();
}

void CLIArgumentParser::printVersion() {
    QTextStream out(stdout);
    out << "StreamDAB Analyzer v1.1 - CLI Mode\n";
    out << "Build date: " << __DATE__ << " " << __TIME__ << "\n";
    out << "Qt version: " << QT_VERSION_STR << "\n";
    out << "ETSI Standards:\n";
    out << "  - EN 300 401 (DAB System Specification)\n";
    out << "  - EN 300 799 (ETI Specification)\n";
    out << "  - TS 102 563 (DAB+ Audio Specification)\n";
    out << "  - TS 101 756 (Registered Tables)\n";
    out << "  - TS 102 371 (EPG Specification)\n";
    out << "  - TS 102 979 (Journaline Specification)\n\n";
    out << "Copyright (C) 2025 StreamDAB Project\n";
    out << "License: Open Source\n";
    out.flush();
}

bool CLIArgumentParser::isValidETIFile(const QString& path) {
    QFileInfo fileInfo(path);

    // Check file exists
    if (!fileInfo.exists()) {
        return false;
    }

    // Check is regular file (not directory)
    if (!fileInfo.isFile()) {
        return false;
    }

    // Check readable
    if (!fileInfo.isReadable()) {
        return false;
    }

    // Check non-empty
    if (fileInfo.size() == 0) {
        return false;
    }

    // Optional: check file extension (warn if not .eti but allow)
    // This allows processing files without .eti extension

    return true;
}

bool CLIArgumentParser::isValidOutputPath(const QString& path) {
    QFileInfo fileInfo(path);

    // Get parent directory
    QDir parentDir = fileInfo.absoluteDir();

    // Check parent directory exists
    if (!parentDir.exists()) {
        return false;
    }

    // Check parent directory is writable
    QFileInfo parentInfo(parentDir.absolutePath());
    if (!parentInfo.isWritable()) {
        return false;
    }

    // If file already exists, check it's writable
    if (fileInfo.exists()) {
        if (!fileInfo.isWritable()) {
            return false;
        }
        // Check it's a regular file, not directory
        if (!fileInfo.isFile()) {
            return false;
        }
    }

    return true;
}

int CLIArgumentParser::parseIntArg(const QString& arg, const QString& value, int min, int max) {
    bool ok = false;
    int result = value.toInt(&ok);

    if (!ok) {
        throw std::invalid_argument(
            QString("%1 requires a valid integer, got: %2")
                .arg(arg, value)
                .toStdString()
        );
    }

    if (result < min || result > max) {
        throw std::invalid_argument(
            QString("%1 must be between %2 and %3, got: %4")
                .arg(arg)
                .arg(min)
                .arg(max)
                .arg(result)
                .toStdString()
        );
    }

    return result;
}

} // namespace cli
