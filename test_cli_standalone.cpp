// Standalone CLI Test Program
// Tests CLI argument parser without needing full build

#include "src/cli/cli_argument_parser.hpp"
#include <QCoreApplication>
#include <QDebug>
#include <QTextStream>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    QTextStream out(stdout);
    out << "=== StreamDAB Analyzer CLI Test ===" << Qt::endl;
    out << Qt::endl;

    // Test 1: Show help
    if (argc == 1) {
        out << "No arguments provided. Showing help:" << Qt::endl;
        out << Qt::endl;
        cli::CLIArgumentParser::printUsage();
        return 0;
    }

    // Test 2: Parse arguments
    try {
        auto options = cli::CLIArgumentParser::parse(argc, argv);

        out << "Parsed options:" << Qt::endl;
        out << "  CLI mode: " << (options.cli_mode ? "YES" : "NO") << Qt::endl;
        out << "  Input file: " << (options.input_file.isEmpty() ? "(none)" : options.input_file) << Qt::endl;
        out << "  Output file: " << (options.output_file.isEmpty() ? "(stdout)" : options.output_file) << Qt::endl;
        out << "  Quiet: " << (options.quiet ? "YES" : "NO") << Qt::endl;
        out << "  Verbose: " << (options.verbose ? "YES" : "NO") << Qt::endl;
        out << "  Max frames: " << (options.max_frames == 0 ? "ALL" : QString::number(options.max_frames)) << Qt::endl;
        out << "  Show help: " << (options.show_help ? "YES" : "NO") << Qt::endl;
        out << "  Show version: " << (options.show_version ? "YES" : "NO") << Qt::endl;

        if (options.show_help) {
            out << Qt::endl;
            cli::CLIArgumentParser::printUsage();
        }

        if (options.show_version) {
            out << Qt::endl;
            cli::CLIArgumentParser::printVersion();
        }

        out << Qt::endl << "✅ Argument parsing successful!" << Qt::endl;
        return 0;

    } catch (const std::invalid_argument& e) {
        out << "❌ Invalid argument: " << e.what() << Qt::endl;
        return 1;
    } catch (const std::runtime_error& e) {
        out << "❌ Runtime error: " << e.what() << Qt::endl;
        return 2;
    } catch (const std::exception& e) {
        out << "❌ Exception: " << e.what() << Qt::endl;
        return 3;
    }
}
