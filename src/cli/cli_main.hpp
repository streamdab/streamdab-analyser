#ifndef CLI_MAIN_HPP
#define CLI_MAIN_HPP

namespace cli {

/**
 * @brief CLI mode entry point for StreamDAB Analyser
 *
 * Orchestrates the complete CLI workflow:
 * 1. Parse command-line arguments
 * 2. Process ETI file with HeadlessETIProcessor
 * 3. Generate YAML output
 * 4. Return appropriate exit code
 *
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code (0=success, 1=invalid args, 2=file not found, 3=processing error, 4=write error)
 */
int cli_main(int argc, char** argv);

} // namespace cli

#endif // CLI_MAIN_HPP
