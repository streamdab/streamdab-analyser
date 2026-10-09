#include "cli_argument_parser.hpp"
#include "core/product_version.hpp"
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <iostream>
#include <stdexcept>

namespace cli {

namespace {
/**
 * @brief Version string reported by --help/--version
 * @return QCoreApplication::applicationVersion(), or the build's DABX_VERSION
 *         (see src/core/product_version.hpp) when no application instance has
 *         set one yet
 */
QString applicationBannerVersion()
{
    const QString version = QCoreApplication::applicationVersion();
    return version.isEmpty() ? QStringLiteral(DABX_VERSION) : version;
}
} // namespace

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

        // Verbose frames flag (frame-by-frame output)
        if (arg == "--verbose-frames") {
            options.verbose_frames = true;
            continue;
        }

        // FIG trace flag (per-frame FIG instance trace, P1)
        if (arg == "--fig-trace") {
            options.fig_trace = true;
            continue;
        }

        // Grouped-by-FIG view flag (backlog P2/P3)
        if (arg == "--figs-by-type") {
            options.figs_by_type = true;
            continue;
        }

        // Per-FIB hex dump flag (backlog P2/P3)
        if (arg == "--fib-hex") {
            options.fib_hex = true;
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
        if (arg == "--output" || arg == "-o") {
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

        // === Analyser settings flags (option-variant matrix rows 1-14) ===
        // Row 1: Force label charset (0 = EBU Latin, 3 = UTF-8, 6 = TIS-620)
        if (arg == "--force-charset") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("--force-charset requires a charset value");
            }
            QString value = QString::fromUtf8(argv[++i]);
            const int charset = parseIntArg("--force-charset", value, 0, 6);
            // Accept only the decodable charsets: 0, 3, 6.
            if (charset != 0 && charset != 3 && charset != 6) {
                throw std::invalid_argument(
                    "--force-charset must be 0 (EBU Latin), 3 (UTF-8) or 6 (TIS-620)");
            }
            options.force_charset = charset;
            continue;
        }

        // Row 1: Charset fallback policy for unknown charset flags
        if (arg == "--charset-fallback") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("--charset-fallback requires raw|utf8|skip");
            }
            QString value = QString::fromUtf8(argv[++i]).toLower();
            if (value != "raw" && value != "utf8" && value != "skip") {
                throw std::invalid_argument("--charset-fallback must be raw, utf8 or skip");
            }
            options.charset_fallback_policy = value;
            continue;
        }

        // Row 2: Forced ETI transport mode
        if (arg == "--eti-mode") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("--eti-mode requires auto|li|ni");
            }
            QString value = QString::fromUtf8(argv[++i]).toLower();
            if (value != "auto" && value != "li" && value != "ni") {
                throw std::invalid_argument("--eti-mode must be auto, li or ni");
            }
            options.eti_mode = value;
            continue;
        }

        // Row 3: Legacy NST count-1 correction
        if (arg == "--nst-offset-1") {
            options.nst_offset_1 = true;
            continue;
        }

        // Row 4: FIC decode mode
        if (arg == "--fic-mode") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("--fic-mode requires strict|raw|auto");
            }
            QString value = QString::fromUtf8(argv[++i]).toLower();
            if (value != "strict" && value != "raw" && value != "auto") {
                throw std::invalid_argument("--fic-mode must be strict, raw or auto");
            }
            options.fic_mode = value;
            continue;
        }

        // Row 5: Ignore FIB CRC failures (decode bad FIBs anyway)
        if (arg == "--fib-ignore-crc") {
            options.fib_ignore_crc = true;
            continue;
        }

        // Row 11: Validate + count the EOH gap frame CRC
        if (arg == "--strict-frame-crc") {
            options.strict_frame_crc = true;
            continue;
        }

        // Row 6: Manual ECC for country resolution (0x00..0xFF)
        if (arg == "--ecc-override") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("--ecc-override requires a hex byte (0xNN)");
            }
            QString value = QString::fromUtf8(argv[++i]);
            bool ok = false;
            int ecc = value.toInt(&ok, 16);
            if (!ok || ecc < 0 || ecc > 255) {
                throw std::invalid_argument(
                    QString("--ecc-override requires a hex byte 0x00-0xFF, got: %1").arg(value).toStdString());
            }
            options.ecc_override = ecc;
            continue;
        }

        // Row 14: Parse the FIG 0/9 extended per-service ECC subfield
        if (arg == "--service-ecc-deep-parse") {
            options.service_ecc_deep_parse = true;
            continue;
        }

        // Row 8: Label source priority
        if (arg == "--label-source") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("--label-source requires fig016|fig11");
            }
            QString value = QString::fromUtf8(argv[++i]).toLower();
            if (value != "fig016" && value != "fig11" && value != "0/16" && value != "1/1") {
                throw std::invalid_argument("--label-source must be fig016 or fig11");
            }
            options.label_source_priority = (value == "fig11" || value == "1/1") ? "fig11" : "fig016";
            continue;
        }

        // Row 8: Emit short labels derived from the 16-bit mask
        if (arg == "--show-short-labels") {
            options.show_short_labels = true;
            continue;
        }

        // Row 10: Headless per-frame timestamp source
        if (arg == "--timestamp-source") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("--timestamp-source requires tist|mtime");
            }
            QString value = QString::fromUtf8(argv[++i]).toLower();
            if (value != "tist" && value != "mtime") {
                throw std::invalid_argument("--timestamp-source must be tist or mtime");
            }
            options.timestamp_source = value;
            continue;
        }

        // Row 12: SId display mode
        if (arg == "--sid-display") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("--sid-display requires hex32|ecc-sid|hex16");
            }
            QString value = QString::fromUtf8(argv[++i]).toLower();
            if (value != "hex32" && value != "ecc-sid" && value != "hex16") {
                throw std::invalid_argument("--sid-display must be hex32, ecc-sid or hex16");
            }
            options.sid_display_mode = value;
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

QMap<QString, QVariant> CLIOptions::analyserOverrides() const
{
    QMap<QString, QVariant> m;
    if (force_charset >= 0) {
        m.insert(QStringLiteral("force_charset"), force_charset);
    }
    if (!charset_fallback_policy.isEmpty()) {
        m.insert(QStringLiteral("charset_fallback_policy"), charset_fallback_policy);
    }
    if (!eti_mode.isEmpty()) {
        m.insert(QStringLiteral("eti_mode"), eti_mode);
    }
    if (nst_offset_1) {
        m.insert(QStringLiteral("nst_offset_1"), true);
    }
    if (!fic_mode.isEmpty()) {
        m.insert(QStringLiteral("fic_mode"), fic_mode);
    }
    if (fib_ignore_crc) {
        m.insert(QStringLiteral("fib_ignore_crc"), true);
    }
    if (strict_frame_crc) {
        m.insert(QStringLiteral("strict_frame_crc"), true);
    }
    if (ecc_override >= 0) {
        m.insert(QStringLiteral("ecc_override"), ecc_override);
    }
    if (service_ecc_deep_parse) {
        m.insert(QStringLiteral("service_ecc_deep_parse"), true);
    }
    if (!label_source_priority.isEmpty()) {
        m.insert(QStringLiteral("label_source_priority"), label_source_priority);
    }
    if (show_short_labels) {
        m.insert(QStringLiteral("show_short_labels"), true);
    }
    if (!timestamp_source.isEmpty()) {
        m.insert(QStringLiteral("timestamp_source"), timestamp_source);
    }
    if (!sid_display_mode.isEmpty()) {
        m.insert(QStringLiteral("sid_display_mode"), sid_display_mode);
    }
    return m;
}

void CLIArgumentParser::printUsage() {
    QTextStream out(stdout);
    out << "StreamDAB Analyser v" << applicationBannerVersion() << " - CLI Mode\n";
    out << "ETSI EN 300 401 compliant DAB/DAB+ stream analyser\n\n";

    out << "USAGE:\n";
    out << "  streamdab-analyser --input <file> [OPTIONS]\n";
    out << "  streamdab-analyser --help\n";
    out << "  streamdab-analyser --version\n\n";

    out << "REQUIRED ARGUMENTS:\n";
    out << "  --input <file>          ETI file to analyze (.eti format)\n\n";

    out << "OPTIONAL ARGUMENTS:\n";
    out << "  -o, --output <file>     YAML output file (default: stdout)\n";
    out << "  --quiet                 Minimal console output (errors only)\n";
    out << "  --verbose               Detailed progress logging\n";
    out << "  --verbose-frames        Frame-by-frame detailed output (LARGE files)\n";
    out << "  --fig-trace             Per-frame FIG instance trace to stderr\n";
    out << "                          (fig-trace: frame=N type=T ext=E len=L fib=F\n";
    out << "                          crc=ok|bad reconfig=bool, with readable values\n";
    out << "                          for decoded FIGs; also enabled by --verbose-frames)\n";
    out << "  --figs-by-type          Grouped view: one section per FIG type/ext\n";
    out << "                          (frames + aggregate readable values: labels,\n";
    out << "                          frequencies, coordinates; FIC-XTractor style)\n";
    out << "  --fib-hex               Per-FIB hex dump with analyzed FIG byte ranges\n";
    out << "                          (fib-hex: frame=N fib=F crc=ok|bad; 30-byte hex\n";
    out << "                          line + [a-b] FIG t/e markers; FIB-Hex panel style)\n";
    out << "  --max-frames <n>        Process first N frames only (default: all)\n";
    out << "  --cli                   Explicitly enable CLI mode (auto-detected)\n\n";

    out << "ANALYSER OPTIONS (analyser decode decision points):\n";
    out << "  --force-charset N       Force label charset: 0 (EBU Latin), 3 (UTF-8),\n";
    out << "                          6 (TIS-620); default: follow the stream flag\n";
    out << "  --charset-fallback M    Unknown-charset policy: raw (default), utf8, skip\n";
    out << "  --eti-mode M            ETI transport: auto (default), li, ni (forced)\n";
    out << "  --nst-offset-1          Legacy NST correction: NST = (p[5]&0x7F) + 1\n";
    out << "  --fic-mode M            FIC decode: strict, raw, auto (default; all-FIB-CRC\n";
    out << "                          fail -> raw fallback; DABX_FIC_MODE env still honored)\n";
    out << "  --fib-ignore-crc        Decode FIBs whose CRC fails (default: dropped)\n";
    out << "  --strict-frame-crc      Validate + count the EOH gap frame CRC (CCITT-FALSE)\n";
    out << "  --ecc-override 0xNN     Manual ECC for country resolution (default: stream)\n";
    out << "  --service-ecc-deep-parse  Parse FIG 0/9 extended per-service ECC subfield\n";
    out << "  --label-source S        Label priority: fig016 (default), fig11\n";
    out << "  --show-short-labels     Emit service_short_label / ensemble_short_label\n";
    out << "  --timestamp-source S    Verbose timestamp: tist (default), mtime\n";
    out << "  --sid-display M         SId rendering: hex32 (default), ecc-sid, hex16\n\n";

    out << "INFORMATION:\n";
    out << "  --help, -h              Show this help message and exit\n";
    out << "  --version, -v           Show version information and exit\n\n";

    out << "EXAMPLES:\n";
    out << "  # Analyze ETI file to stdout (YAML format)\n";
    out << "  streamdab-analyser --input sample.eti\n\n";

    out << "  # Save analysis to file with quiet mode\n";
    out << "  streamdab-analyser --input sample.eti -o report.yaml --quiet\n\n";

    out << "  # Analyze first 1000 frames with verbose logging\n";
    out << "  streamdab-analyser --input stream.eti --max-frames 1000 --verbose\n\n";

    out << "  # Frame-by-frame detailed output (WARNING: produces LARGE files)\n";
    out << "  streamdab-analyser --input sample.eti --verbose-frames -o detailed.yaml\n\n";

    out << "EXIT CODES:\n";
    out << "  0   Success\n";
    out << "  1   Invalid arguments\n";
    out << "  2   File not found or not readable\n";
    out << "  3   Processing error\n";
    out << "  4   Write error\n\n";

    out << "SUPPORTED FEATURES:\n";
    out << "  - ETSI EN 300 401 compliant ETI frame parsing\n";
    out << "  - FIG Type 0/0 through 0/24 decoding (22 types)\n";
    out << "  - FIG Type 1/0, 1/1, 1/4, 1/5 (ensemble/service labels)\n";
    out << "  - Service discovery and metadata extraction\n";
    out << "  - DAB+ stream validation (ETSI TS 102 563)\n";
    out << "  - ETSI TS 101 756 registered tables (458 entries)\n";
    out << "  - Thai UTF-8 support throughout\n";
    out << "  - Frame-by-frame FIG decoding (--verbose-frames)\n\n";

    out << "For more information, visit: https://github.com/streamdab/streamdab-analyser\n";
    out.flush();
}

void CLIArgumentParser::printVersion() {
    QTextStream out(stdout);
    out << "StreamDAB Analyser v" << applicationBannerVersion() << " - CLI Mode\n";
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
