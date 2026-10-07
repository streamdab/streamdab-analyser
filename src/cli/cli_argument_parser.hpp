#ifndef CLI_ARGUMENT_PARSER_HPP
#define CLI_ARGUMENT_PARSER_HPP

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariant>
#include <stdexcept>

namespace cli {

/**
 * @brief CLI options parsed from command-line arguments
 *
 * Stores all command-line configuration options for StreamDAB Analyser
 * CLI mode operation.
 */
struct CLIOptions {
    bool cli_mode{false};           // Enable CLI mode (auto-detected)
    QString input_file;             // ETI file to analyze
    QString output_file;            // YAML output file (default: stdout)
    QString stream_uri;             // Network stream URI (future)
    bool quiet{false};              // Minimal console output
    bool verbose{false};            // Detailed progress logging
    bool verbose_frames{false};     // NEW: Frame-by-frame detailed output
    bool fig_trace{false};          // P1: per-frame FIG instance trace output
    bool figs_by_type{false};       // P2/P3: grouped-by-FIG view
    bool fib_hex{false};            // P2/P3: per-FIB hex dumps with FIG markers
    bool show_help{false};          // Show help text and exit
    bool show_version{false};       // Show version info and exit
    int max_frames{0};              // Process first N frames (0 = all)

    // Future placeholders (not implemented in v1.1)
    bool advanced_fig{false};       // Advanced FIG analysis
    bool benchmark{false};          // Performance benchmarking
    QStringList fig_filters;        // FIG type filters

    // ========================================================================
    // Analyser settings flags (option-variant matrix rows 1-14; defaults
    // mirror the etisnoop-parity behavior — see AnalyserSettings::defaults()).
    // Empty strings / -1 / false mean "not specified, keep the default".
    // ========================================================================
    int force_charset{-1};                 // Row 1: -1 follow stream, 0/3/6 override
    QString charset_fallback_policy;       // Row 1: "raw"|"utf8"|"skip"
    QString eti_mode;                      // Row 2: "auto"|"li"|"ni"
    bool nst_offset_1{false};              // Row 3: legacy NST count-1 correction
    QString fic_mode;                      // Row 4: "strict"|"raw"|"auto"
    bool fib_ignore_crc{false};            // Row 5: decode FIBs despite CRC failures
    bool strict_frame_crc{false};          // Row 11: validate + count EOH gap CRC
    int ecc_override{-1};                  // Row 6: manual ECC for country lookup
    bool service_ecc_deep_parse{false};    // Row 14: FIG 0/9 per-service ECC
    QString label_source_priority;         // Row 8: "fig016"|"fig11"
    bool show_short_labels{false};         // Row 8: emit service/ensemble_short_label
    QString timestamp_source;              // Row 10: "tist"|"mtime"
    QString sid_display_mode;              // Row 12: "hex32"|"ecc-sid"|"hex16"

    /**
     * @brief Build the AnalyserSettings override map from the flags that were
     *        explicitly provided. Keys match AnalyserSettings::applyCliOverrides().
     * @return Sparse map of setting-name -> value (absent = keep default)
     */
    QMap<QString, QVariant> analyserOverrides() const;
};

/**
 * @brief Command-line argument parser for StreamDAB Analyser
 *
 * Parses and validates CLI arguments with comprehensive error handling.
 * Supports both short and long argument forms, with clear error messages.
 *
 * Performance: <1ms parsing time (target)
 * Thread-safety: Static methods, no shared state
 *
 * Example usage:
 * @code
 * try {
 *     auto options = CLIArgumentParser::parse(argc, argv);
 *     if (options.show_help) {
 *         CLIArgumentParser::printUsage();
 *         return 0;
 *     }
 *     // Process options...
 * } catch (const std::invalid_argument& e) {
 *     qCritical() << "Invalid arguments:" << e.what();
 *     return 1;
 * }
 * @endcode
 */
class CLIArgumentParser {
public:
    /**
     * @brief Parse command-line arguments into CLIOptions structure
     *
     * @param argc Argument count
     * @param argv Argument values
     * @return CLIOptions Parsed options structure
     * @throws std::invalid_argument If arguments are invalid
     * @throws std::runtime_error If file validation fails
     */
    static CLIOptions parse(int argc, char** argv);

    /**
     * @brief Print comprehensive usage information to stdout
     *
     * Displays all supported arguments, examples, and exit codes.
     */
    static void printUsage();

    /**
     * @brief Print version information to stdout
     *
     * Shows StreamDAB Analyser version and build information.
     */
    static void printVersion();

private:
    /**
     * @brief Validate that file exists and is readable
     *
     * @param path File path to validate
     * @return true if file exists and is readable
     */
    static bool isValidETIFile(const QString& path);

    /**
     * @brief Validate that output path is writable
     *
     * Checks parent directory exists and is writable.
     *
     * @param path Output file path to validate
     * @return true if path is writable
     */
    static bool isValidOutputPath(const QString& path);

    /**
     * @brief Parse integer argument value with validation
     *
     * @param arg Argument name (for error messages)
     * @param value String value to parse
     * @param min Minimum valid value
     * @param max Maximum valid value
     * @return int Parsed integer value
     * @throws std::invalid_argument If value is invalid or out of range
     */
    static int parseIntArg(const QString& arg, const QString& value, int min, int max);
};

} // namespace cli

#endif // CLI_ARGUMENT_PARSER_HPP
