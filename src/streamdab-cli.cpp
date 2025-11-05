/**
 * @file streamdab-cli.cpp
 * @brief Standalone CLI-only executable for StreamDAB Analyzer
 *
 * This is a minimal CLI-only build that avoids Qt MOC issues with the GUI components.
 * Perfect for headless server deployment and automated processing.
 *
 * Usage:
 *   ./streamdab-cli --help
 *   ./streamdab-cli --input sample.eti --output report.yaml
 */

#include <QCoreApplication>
#include "cli/cli_argument_parser.hpp"
#include "cli/cli_main.hpp"

int main(int argc, char *argv[]) {
    // CLI mode only - no GUI
    QCoreApplication app(argc, argv);
    app.setApplicationName("StreamDAB Analyzer CLI");
    app.setApplicationVersion("1.2.0");

    // Run CLI mode
    return cli::cli_main(argc, argv);
}
