/**
 * @file cli_main.cpp
 * @brief CLI Mode Entry Point and Orchestration for StreamDAB Analyser
 *
 * Implements the complete CLI workflow:
 * 1. Parse command-line arguments (CLIArgumentParser)
 * 2. Process ETI file (HeadlessETIProcessor)
 * 3. Generate YAML output (YAMLOutputGenerator)
 * 4. Return appropriate exit code
 *
 * @author Agent 36 - CLI Main Integration
 * @date 2025-11-05
 *
 * AGENT 45 FIX (2025-11-08): Pass aggregated data to YAMLOutputGenerator
 * Previously: Only basic statistics sent to YAML generator
 * Now: Ensemble, services, subchannels, and FIG counts properly forwarded
 *
 * VERBOSE FRAMES COMPLETE (2025-11-08): Added frame-by-frame data collection
 * HeadlessETIProcessor collects: ETI header, STC fields, FIC data, FIG blocks
 * YAMLOutputGenerator formats all frame data to YAML
 *
 * @copyright StreamDAB Analyser Project - PDCA Week 8
 */

#include "cli_main.hpp"
#include "cli_argument_parser.hpp"
#include "yaml_output_generator.hpp"
#include "../core/headless_eti_processor.hpp"
#include "../core/analyser_settings.hpp"
#include "../utils/logger.h"
#include <QFile>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QTextStream>
#include <iostream>
#include <stdexcept>

namespace cli {

namespace {
    // Exit codes as per PDCA Week 8 specification
    constexpr int CLI_EXIT_SUCCESS = 0;
    constexpr int CLI_EXIT_INVALID_ARGS = 1;
    constexpr int CLI_EXIT_FILE_NOT_FOUND = 2;
    constexpr int CLI_EXIT_PROCESSING_ERROR = 3;
    constexpr int CLI_EXIT_WRITE_ERROR = 4;

    /**
     * @brief Configure logging based on verbosity flags
     * @param quiet Quiet mode (errors only)
     * @param verbose Verbose mode (debug output)
     */
    void configureLogging(bool quiet, bool verbose) {
        Logger::LogLevel logLevel;
        if (quiet) {
            logLevel = Logger::LogLevel::Error;
            // Qt logging categories (e.g. streamdab.eti progress/summary lines
            // from the headless processor) bypass Logger; keep errors only.
            QLoggingCategory::setFilterRules(QStringLiteral(
                "streamdab.*.debug=false\n"
                "streamdab.*.info=false\n"
                "streamdab.*.warning=false\n"));
        } else if (verbose) {
            logLevel = Logger::LogLevel::Debug;
        } else {
            logLevel = Logger::LogLevel::Info;
        }
        Logger::instance().setLogLevel(logLevel);
    }

    /**
     * @brief Print progress message to stdout (respects quiet mode)
     * @param message Progress message
     * @param quiet Quiet mode flag
     */
    void printProgress(const QString& message, bool quiet) {
        if (!quiet) {
            std::cout << message.toStdString() << std::endl;
        }
    }

    /**
     * @brief Print processing summary (respects quiet mode)
     * @param result Processing result from HeadlessETIProcessor
     * @param totalTimeMs Total processing time in milliseconds
     * @param quiet Quiet mode flag
     */
    void printSummary(const HeadlessETIProcessor::ProcessingResult& result,
                      qint64 totalTimeMs, bool quiet) {
        if (quiet) return;

        std::cout << "\n========================================\n";
        std::cout << "Processing Summary\n";
        std::cout << "========================================\n";
        std::cout << "Status:          " << (result.success ? "SUCCESS" : "FAILED") << "\n";
        std::cout << "Frames:          " << result.processedFrames << " / " << result.totalFrames << "\n";
        std::cout << "Errors:          " << result.errorFrames << "\n";
        std::cout << "Processing Time: " << QString::number(totalTimeMs / 1000.0, 'f', 2).toStdString() << " seconds\n";
        std::cout << "Average FPS:     " << QString::number(result.averageFPS, 'f', 2).toStdString() << "\n";
        std::cout << "Ensembles:       " << result.ensemblesFound;
        if (!result.ensembleName.isEmpty()) {
            std::cout << " (" << result.ensembleName.toStdString() << ")";
        }
        std::cout << "\n";
        std::cout << "Services:        " << result.servicesFound << "\n";
        std::cout << "Subchannels:     " << result.subchannels.size() << "\n";  // AGENT 45 FIX
        if (result.verbose_mode) {
            std::cout << "Verbose Frames:  " << result.frame_analyses.size() << "\n";
        }
        std::cout << "ETSI Compliant:  " << (result.etsiCompliant ? "YES" : "NO") << "\n";
        // T39: always print the short reason (the protocol counters that drove
        // the verdict) so a NO is explainable. Transport error frames are
        // reported separately in the "Errors:" line above and here.
        if (!result.complianceReason.isEmpty()) {
            std::cout << "ETSI Reason:     " << result.complianceReason.toStdString() << "\n";
        }
        std::cout << "========================================\n\n";
    }
}

int cli_main(int argc, char** argv) {
    QElapsedTimer totalTimer;
    totalTimer.start();

    try {
        // Parse command-line arguments
        CLIOptions options = CLIArgumentParser::parse(argc, argv);

        // Handle help
        if (options.show_help) {
            CLIArgumentParser::printUsage();
            return CLI_EXIT_SUCCESS;
        }

        // Handle version
        if (options.show_version) {
            CLIArgumentParser::printVersion();
            return CLI_EXIT_SUCCESS;
        }

        // Configure logging based on verbosity
        configureLogging(options.quiet, options.verbose);

        // Verify input file exists (should be validated by parser, but double-check)
        QFile inputFile(options.input_file);
        if (!inputFile.exists()) {
            std::cerr << "ERROR: Input file not found: " << options.input_file.toStdString() << "\n";
            return CLI_EXIT_FILE_NOT_FOUND;
        }

        // Print initial status
        printProgress(QString("Processing ETI file: %1").arg(options.input_file), options.quiet);
        if (!options.output_file.isEmpty()) {
            printProgress(QString("Output YAML file: %1").arg(options.output_file), options.quiet);
        } else {
            printProgress("Output: stdout", options.quiet);
        }

        // Warn about verbose frames producing large output
        if (options.verbose_frames) {
            printProgress("\n*** WARNING: --verbose-frames produces VERY LARGE output files (50-100MB+) ***", false);
            printProgress("*** Consider using --max-frames to limit output size ***\n", false);
        }

        // Create headless ETI processor
        HeadlessETIProcessor processor;

        // Build the AnalyserSettings from defaults + the provided CLI flags.
        // NOTE: the CLI deliberately does NOT read the GUI QSettings so that a
        // default-config run is byte-identical to the baseline (gate 2).
        eti::AnalyserSettings analyserSettings = eti::AnalyserSettings::defaults();
        analyserSettings.applyCliOverrides(options.analyserOverrides());
        QString settingsError;
        if (!analyserSettings.validate(&settingsError)) {
            std::cerr << "WARNING: invalid analyser setting: "
                      << settingsError.toStdString() << " (using sane defaults)\n";
        }
        processor.setAnalyserSettings(analyserSettings);

        // *** ENABLE VERBOSE MODE IF REQUESTED ***
        if (options.verbose_frames) {
            processor.setVerboseMode(true);
            printProgress("Verbose frame-by-frame analysis: ENABLED", options.quiet);
        }

        // P1: per-frame FIG instance trace (--fig-trace; --verbose-frames
        // includes the same lines by definition).
        if (options.fig_trace || options.verbose_frames) {
            processor.setFigTraceMode(true);
            if (options.fig_trace && !options.quiet) {
                printProgress("FIG instance trace: ENABLED (lines on stderr)", false);
            }
        }

        // Backlog P2/P3: grouped-by-FIG view + per-FIB hex dumps (stderr).
        if (options.figs_by_type) {
            processor.setFigsByTypeMode(true);
            if (!options.quiet) {
                printProgress("FIG grouped view (--figs-by-type): ENABLED (lines on stderr)", false);
            }
        }
        if (options.fib_hex) {
            processor.setFibHexMode(true);
            if (!options.quiet) {
                printProgress("FIB hex dumps (--fib-hex): ENABLED (lines on stderr)", false);
            }
        }

        // P1: limit the processed frame prefix (--max-frames).
        if (options.max_frames > 0) {
            processor.setMaxFrames(options.max_frames);
        }

        // Connect progress signals for non-quiet mode
        if (!options.quiet) {
            QObject::connect(&processor, &HeadlessETIProcessor::status_changed,
                             [](const QString& status) {
                                 std::cout << "\r" << status.toStdString() << std::flush;
                             });
        }

        // Start ETI processing
        printProgress("\nStarting ETI processing...", options.quiet);

        QElapsedTimer processingTimer;
        processingTimer.start();

        // Note: HeadlessETIProcessor::process_file requires output file parameter
        // We'll use a temporary output if only stdout is desired
        QString tempOutputFile = options.output_file;
        if (tempOutputFile.isEmpty()) {
            tempOutputFile = "/tmp/streamdab_temp_output.yaml";
        }

        HeadlessETIProcessor::ProcessingResult result =
            processor.process_file(options.input_file, tempOutputFile);

        qint64 processingTimeMs = processingTimer.elapsed();

        // Check processing result
        if (!result.success) {
            std::cerr << "\nERROR: ETI processing failed: "
                      << result.errorMessage.toStdString() << "\n";
            return CLI_EXIT_PROCESSING_ERROR;
        }

        printProgress("\nETI processing completed successfully.", options.quiet);

        // ============================================================================
        // AGENT 45 FIX + VERBOSE FRAMES: Generate YAML output with aggregated data
        // ============================================================================
        printProgress("Generating YAML output...", options.quiet);

        YAMLOutputGenerator yamlGenerator;

        // Thread the analyser settings into the output layer (rows 6/8/12).
        yamlGenerator.setAnalyserSettings(analyserSettings);

        // Row 11 diagnostics: emitted only when strict_frame_crc is enabled.
        yamlGenerator.setFrameCrcStats(analyserSettings.strict_frame_crc,
                                       result.frame_crc_failures);

        // Enable verbose mode if requested
        if (options.verbose_frames) {
            yamlGenerator.setVerboseMode(true);

            // *** ADD ALL FRAME ANALYSES TO YAML GENERATOR ***
            for (const auto& frame : result.frame_analyses) {
                yamlGenerator.addFrameAnalysis(frame);
            }

            printProgress(QString("Added %1 frames to verbose output").arg(result.frame_analyses.size()), options.quiet);
        }

        // Set input file and processing time
        yamlGenerator.setInputFile(options.input_file);
        yamlGenerator.setProcessingTime(processingTimeMs);

        // Set ETI stream info
        yamlGenerator.setETIStreamInfo(
            result.totalFrames,
            result.processedFrames,
            result.errorFrames,
            0  // CRC errors - HeadlessETIProcessor doesn't provide this yet
        );

        // AGENT 45 FIX: Set ensemble info from aggregated data
        if (result.ensemblesFound > 0) {
            yamlGenerator.setEnsembleInfo(result.ensemble);
        }

        // AGENT 45 FIX: Add all discovered services
        for (const auto& service : result.services) {
            yamlGenerator.addService(service);
        }

        // AGENT 45 FIX: Add all discovered subchannels
        for (const auto& subchannel : result.subchannels) {
            yamlGenerator.addSubchannel(subchannel);
        }

        // AGENT 45 FIX: Set FIG type statistics
        yamlGenerator.setFIGStatistics(result.fig_type_counts);

        // P1: FIG inventory + FIC/FIB health (additive `figs:`/`fic_health:`).
        {
            QList<cli::FIGInventoryEntry> inventory;
            inventory.reserve(static_cast<int>(result.fig_inventory.size()));
            for (const auto& entry : result.fig_inventory) {
                inventory.append(entry);
            }
            yamlGenerator.setFigInventory(inventory);
        }
        yamlGenerator.setFicHealth(result.fic_health);

        // Set ETSI compliance
        yamlGenerator.setETSICompliance(
            result.etsiCompliant,
            result.complianceWarnings
        );
        yamlGenerator.setETSIComplianceReason(result.complianceReason);

        // Set performance metrics
        yamlGenerator.setPerformanceMetrics(processingTimeMs, result.averageFPS);

        // Generate YAML
        QString yamlContent = yamlGenerator.generateYAML();

        // Write output (file or stdout)
        bool writeSuccess = false;
        if (options.output_file.isEmpty()) {
            // Write to stdout
            writeSuccess = yamlGenerator.writeToStdout();
        } else {
            // Write to file
            writeSuccess = yamlGenerator.writeToFile(options.output_file);
        }

        if (!writeSuccess) {
            std::cerr << "ERROR: Failed to write YAML output\n";
            return CLI_EXIT_WRITE_ERROR;
        }

        if (!options.output_file.isEmpty()) {
            printProgress(QString("YAML output written to: %1").arg(options.output_file), options.quiet);
        }

        // Print summary
        qint64 totalTimeMs = totalTimer.elapsed();
        printSummary(result, totalTimeMs, options.quiet);

        // Log final status
        Logger::instance().logInfo(
            QString("CLI processing completed successfully in %1 ms").arg(totalTimeMs),
            "CLIMain"
        );

        return CLI_EXIT_SUCCESS;

    } catch (const std::invalid_argument& e) {
        // Argument parsing error
        std::cerr << "ERROR: " << e.what() << "\n";
        std::cerr << "Use --help for usage information.\n";
        return CLI_EXIT_INVALID_ARGS;

    } catch (const std::runtime_error& e) {
        // File validation or runtime error
        std::cerr << "ERROR: " << e.what() << "\n";
        return CLI_EXIT_FILE_NOT_FOUND;

    } catch (const std::exception& e) {
        // Other exceptions
        std::cerr << "FATAL ERROR: " << e.what() << "\n";
        return CLI_EXIT_PROCESSING_ERROR;
    }
}

} // namespace cli
