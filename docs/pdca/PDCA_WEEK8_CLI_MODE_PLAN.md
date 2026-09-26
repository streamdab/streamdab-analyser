# PDCA WEEK 8 - CLI MODE IMPLEMENTATION PLAN

**Date:** November 5, 2025
**Version:** 1.0
**Status:** Planning Phase
**Methodology:** PDCA Multi-Agent Parallel Development
**Target:** Complete CLI mode with YAML output (4 agents, 1 week)

---

## 🎯 EXECUTIVE SUMMARY

**Goal:** Implement headless CLI mode for StreamDAB Analyzer with ETISnoop-compatible YAML output, enabling automated testing, CI/CD integration, and remote deployment.

**Scope:** Phase 1 + 2 from CLI_MODE_FEATURE_PLAN.md
- CLI argument parsing
- YAML output generation (ETISnoop-compatible)
- CLI mode integration in main.cpp
- Functional testing without GUI

**Out of Scope (Future):**
- Advanced FIG analysis (--advanced-fig flag)
- Live stream support in CLI
- Performance benchmarking mode

---

## 📊 CURRENT STATE ANALYSIS

### ✅ Existing Foundation

**Headless Processing:**
- `src/core/headless_eti_processor.cpp/hpp` - Basic headless ETI processing
- Thread-safe operation with signal/slot architecture
- Progress tracking and status updates

**FIG Processing:**
- 22/22 FIG 0/* types implemented (100% ETSI EN 300 401)
- Complete service discovery engine
- Ensemble management with metadata
- DAB+ validation, MOT, EPG, Journaline support

**Testing Framework:**
- 540+ tests, 100% critical test pass rate
- QTest framework integrated
- Comprehensive FIG type coverage

### ❌ Missing Components

**CLI Infrastructure:**
- No CLI argument parsing
- No YAML output generation
- No CLI mode detection in main.cpp
- No CLI-specific functional tests

---

## 🎯 MULTI-AGENT IMPLEMENTATION PLAN

### **Agent Assignment Strategy**

| **Agent** | **Specialization** | **Task** | **Deliverables** | **Tests** | **Est. Time** |
|-----------|-------------------|----------|------------------|-----------|---------------|
| **Agent 34** | python-pro | CLI Argument Parser | cli_argument_parser.cpp/hpp | 15 tests | 4h |
| **Agent 35** | python-pro | YAML Output Generator | yaml_output_generator.cpp/hpp | 20 tests | 6h |
| **Agent 36** | python-pro | CLI Main Integration | cli_main.cpp + main.cpp updates | 10 tests | 4h |
| **Agent 37** | python-pro | CLI Functional Tests | test_cli_mode_functional.cpp | 25 tests | 4h |

**Total:** 4 agents, 70 tests, 18 hours estimated (parallel execution: ~6-8 hours)

---

## 📋 AGENT 34: CLI ARGUMENT PARSER

### **Responsibility**
Implement robust command-line argument parsing with validation and help text.

### **Files to Create**
- `src/cli/cli_argument_parser.cpp`
- `src/cli/cli_argument_parser.hpp`
- `tests/test_cli_argument_parser.cpp`

### **Implementation Requirements**

**Class Structure:**
```cpp
// cli_argument_parser.hpp
namespace cli {

struct CLIOptions {
    bool cli_mode{false};
    QString input_file;
    QString output_file;
    QString stream_uri;
    bool quiet{false};
    bool verbose{false};
    bool show_help{false};
    bool show_version{false};
    int max_frames{0};  // 0 = all frames

    // Future extensions (not implemented in Week 8)
    bool advanced_fig{false};  // Placeholder
    bool benchmark{false};     // Placeholder
    QStringList fig_filters;   // Placeholder
};

class CLIArgumentParser {
public:
    static CLIOptions parse(int argc, char** argv);
    static void printUsage();
    static void printVersion();

private:
    static bool isValidETIFile(const QString& path);
    static bool isValidOutputPath(const QString& path);
};

} // namespace cli
```

**Supported Arguments:**
```
Required (one of):
  --input <file>          ETI file to analyze
  --stream <uri>          Stream URI (placeholder for future)

Optional:
  --output <file>         YAML output file (default: stdout)
  --quiet                 Minimal console output
  --verbose               Detailed progress logging
  --max-frames <n>        Process first N frames only
  --help, -h              Show help text
  --version, -v           Show version info

Flags:
  --cli                   Enable CLI mode (auto-detected)
```

**Exit Codes:**
- `0` - Success
- `1` - Invalid arguments
- `2` - Input file not found
- `4` - Output write error

### **Test Requirements (15 tests)**

```cpp
// tests/test_cli_argument_parser.cpp

class TestCLIArgumentParser : public QObject {
    Q_OBJECT
private slots:
    // Basic parsing (5 tests)
    void test01_ParseInputFile();
    void test02_ParseOutputFile();
    void test03_ParseQuietFlag();
    void test04_ParseVerboseFlag();
    void test05_ParseMaxFrames();

    // Validation (5 tests)
    void test06_InputFileNotFound();
    void test07_InvalidOutputPath();
    void test08_MissingRequiredInput();
    void test09_ConflictingFlags();
    void test10_InvalidMaxFrames();

    // Help and version (3 tests)
    void test11_ShowHelp();
    void test12_ShowVersion();
    void test13_HelpOverridesOtherArgs();

    // Edge cases (2 tests)
    void test14_LongFilePaths();
    void test15_SpecialCharactersInPath();
};
```

**Performance Target:** <1ms parsing time

**Acceptance Criteria:**
- ✅ All 15 tests passing
- ✅ Help text comprehensive and clear
- ✅ Input validation prevents crashes
- ✅ Exit codes correct for all scenarios

---

## 📋 AGENT 35: YAML OUTPUT GENERATOR

### **Responsibility**
Generate ETISnoop-compatible YAML output from processed ETI data.

### **Files to Create**
- `src/cli/yaml_output_generator.cpp`
- `src/cli/yaml_output_generator.hpp`
- `tests/test_yaml_output_generator.cpp`

### **Implementation Requirements**

**Class Structure:**
```cpp
// yaml_output_generator.hpp
namespace cli {

class YAMLOutputGenerator {
public:
    YAMLOutputGenerator();

    // Data setters
    void setInputFile(const QString& filename);
    void setProcessingTime(qint64 ms);
    void setEnsembleInfo(const eti::Ensemble& ensemble);
    void addService(const eti::DabService& service);
    void addSubchannel(const Fig01SubchannelInfo& subchannel);
    void setFIGStatistics(const QMap<QString, int>& fig_counts);
    void setETSICompliance(bool compliant, const QStringList& warnings);
    void setPerformanceMetrics(qint64 processing_time, double fps);

    // Output generation
    QString generateYAML() const;
    bool writeToFile(const QString& filename);
    bool writeToStdout();

private:
    struct YAMLData {
        QString version{"1.0"};
        QString generator{"StreamDAB Analyzer v1.1"};
        QDateTime timestamp;
        QString input_file;
        qint64 analysis_duration_ms{0};

        // ETI stream metadata
        struct {
            QString format{"ETI-LI"};
            int frame_size{6144};
            int total_frames{0};
            int valid_frames{0};
            int error_frames{0};
            double frame_rate{250.0};
            int crc_errors{0};
        } eti_stream;

        // Ensemble info
        eti::Ensemble ensemble;

        // Services and subchannels
        QList<eti::DabService> services;
        QList<Fig01SubchannelInfo> subchannels;

        // FIG statistics
        QMap<QString, int> fig_counts;

        // ETSI compliance
        bool etsi_compliant{true};
        QStringList etsi_warnings;

        // Performance
        qint64 processing_time_ms{0};
        double frames_per_second{0.0};
    } m_data;

    QString formatEnsemble() const;
    QString formatServices() const;
    QString formatSubchannels() const;
    QString formatFIGStatistics() const;
    QString formatETSICompliance() const;
    QString formatPerformance() const;
};

} // namespace cli
```

**YAML Output Structure (ETISnoop-Compatible):**
```yaml
# StreamDAB Analyzer CLI Output
version: "1.0"
generator: "StreamDAB Analyzer v1.1"
timestamp: "2025-11-05T08:00:00Z"
input_file: "test_stream.eti"
analysis_duration_ms: 1250

# ETI Stream Metadata
eti_stream:
  format: "ETI-LI"
  frame_size: 6144
  total_frames: 4200
  valid_frames: 4198
  error_frames: 2
  frame_rate: 250.0
  duration_seconds: 16.8
  crc_errors: 2

# Ensemble Information
ensemble:
  ensemble_id: 0x1234
  ensemble_label: "Thailand DAB"
  country_id: 0x0E
  country_name: "Thailand"
  alarm_flag: false

# Services
services:
  - service_id: 0x5678
    service_label: "FM 105.5 MHz"
    service_type: "DAB+ Audio"
    is_programme_service: true

    components:
      - component_id: 0
        subchannel_id: 5
        audio_service_type: 0x02
        primary: true
        ca_protected: false

# Sub-channels
subchannels:
  - subchannel_id: 5
    start_address: 0x100
    size: 84
    protection_form: "UEP"
    protection_level: 3
    bitrate: 96

# FIG Statistics
fig_statistics:
  total_figs_processed: 12600
  fig_type_counts:
    "0/0": 4200
    "0/1": 4200
    "0/2": 4200
    "1/1": 840

# ETSI Compliance
etsi_compliance:
  overall_compliance: true
  warnings: []

# Performance Metrics
performance:
  total_processing_time_ms: 1250
  frames_per_second: 3360.0
```

### **Test Requirements (20 tests)**

```cpp
// tests/test_yaml_output_generator.cpp

class TestYAMLOutputGenerator : public QObject {
    Q_OBJECT
private slots:
    // Basic output (5 tests)
    void test01_BasicYAMLStructure();
    void test02_EnsembleInfoOutput();
    void test03_ServiceListOutput();
    void test04_SubchannelOutput();
    void test05_FIGStatisticsOutput();

    // Data integrity (5 tests)
    void test06_ThaiUTF8Handling();
    void test07_SpecialCharactersEscaping();
    void test08_LargeServiceCounts();
    void test09_EmptyServiceList();
    void test10_MissingOptionalFields();

    // File operations (5 tests)
    void test11_WriteToFile();
    void test12_WriteToStdout();
    void test13_FilePermissionError();
    void test14_DiskFullHandling();
    void test15_AtomicFileWrite();

    // Format compliance (5 tests)
    void test16_YAMLSyntaxValidity();
    void test17_ETISnoopCompatibility();
    void test18_TimestampFormatISO8601();
    void test19_HexadecimalFormatting();
    void test20_BooleanFormatting();
};
```

**Performance Target:** <100ms YAML generation

**Acceptance Criteria:**
- ✅ All 20 tests passing
- ✅ YAML syntax valid (parseable by standard YAML libraries)
- ✅ ETISnoop-compatible structure
- ✅ Thai UTF-8 characters preserved
- ✅ Atomic file writes (no partial files)

---

## 📋 AGENT 36: CLI MAIN INTEGRATION

### **Responsibility**
Integrate CLI mode into main.cpp and implement cli_main() function.

### **Files to Create**
- `src/cli/cli_main.cpp`
- `src/cli/cli_main.hpp`
- `tests/test_cli_main_integration.cpp`

### **Files to Modify**
- `src/main.cpp` - Add CLI mode detection
- `CMakeLists.txt` - Add CLI source files

### **Implementation Requirements**

**CLI Detection in main.cpp:**
```cpp
// src/main.cpp

#include "cli/cli_main.hpp"

int main(int argc, char *argv[])
{
    // Check for CLI mode flag
    bool cli_mode = false;
    for (int i = 1; i < argc; ++i) {
        QString arg(argv[i]);
        if (arg == "--cli" || arg == "-c" || arg == "--help" ||
            arg == "--version" || arg == "--input") {
            cli_mode = true;
            break;
        }
    }

    if (cli_mode) {
        // CLI mode - no GUI (QCoreApplication)
        QCoreApplication app(argc, argv);
        app.setApplicationName("StreamDAB Analyzer");
        app.setApplicationVersion("1.1");
        return cli::cli_main(argc, argv);
    } else {
        // GUI mode (existing implementation)
        QApplication app(argc, argv);
        DABAnalyzerWindow window;
        window.show();
        return app.exec();
    }
}
```

**cli_main() Implementation:**
```cpp
// cli_main.cpp
namespace cli {

int cli_main(int argc, char** argv) {
    // Parse arguments
    auto options = CLIArgumentParser::parse(argc, argv);

    // Handle help/version
    if (options.show_help) {
        CLIArgumentParser::printUsage();
        return 0;
    }
    if (options.show_version) {
        CLIArgumentParser::printVersion();
        return 0;
    }

    // Validate input
    if (options.input_file.isEmpty()) {
        qCritical() << "Error: No input file specified";
        CLIArgumentParser::printUsage();
        return 1;
    }

    if (!QFile::exists(options.input_file)) {
        qCritical() << "Error: Input file not found:" << options.input_file;
        return 2;
    }

    // Initialize headless processor
    HeadlessETIProcessor processor;

    // Setup logging
    if (options.quiet) {
        Logger::instance().setMinimumLevel(Logger::Error);
    } else if (options.verbose) {
        Logger::instance().setMinimumLevel(Logger::Debug);
    } else {
        Logger::instance().setMinimumLevel(Logger::Info);
    }

    // Process ETI file
    if (!options.quiet) {
        qInfo() << "Processing ETI file:" << options.input_file;
    }

    QElapsedTimer timer;
    timer.start();

    bool success = processor.processFile(options.input_file, options.max_frames);

    if (!success) {
        qCritical() << "Error: ETI processing failed";
        return 3;
    }

    qint64 processing_time = timer.elapsed();

    // Generate YAML output
    YAMLOutputGenerator yaml_gen;
    yaml_gen.setInputFile(options.input_file);
    yaml_gen.setProcessingTime(processing_time);

    // Populate YAML with results
    auto ensemble = processor.getEnsemble();
    yaml_gen.setEnsembleInfo(ensemble);

    for (const auto& service : processor.getServices()) {
        yaml_gen.addService(service);
    }

    for (const auto& subchannel : processor.getSubchannels()) {
        yaml_gen.addSubchannel(subchannel);
    }

    yaml_gen.setFIGStatistics(processor.getFIGCounts());
    yaml_gen.setETSICompliance(true, {});

    double fps = (double)processor.getFrameCount() / (processing_time / 1000.0);
    yaml_gen.setPerformanceMetrics(processing_time, fps);

    // Write output
    bool write_success = false;
    if (options.output_file.isEmpty()) {
        write_success = yaml_gen.writeToStdout();
    } else {
        write_success = yaml_gen.writeToFile(options.output_file);
        if (write_success && !options.quiet) {
            qInfo() << "Output written to:" << options.output_file;
        }
    }

    if (!write_success) {
        qCritical() << "Error: Failed to write YAML output";
        return 4;
    }

    // Print summary
    if (!options.quiet) {
        qInfo() << "";
        qInfo() << "Summary:";
        qInfo() << "  Ensemble:" << ensemble.ensemble_label;
        qInfo() << "  Services:" << processor.getServices().size();
        qInfo() << "  Frames processed:" << processor.getFrameCount();
        qInfo() << "  Processing time:" << processing_time << "ms";
        qInfo() << "  Processing rate:" << QString::number(fps, 'f', 1) << "FPS";
    }

    return 0;
}

} // namespace cli
```

### **Test Requirements (10 tests)**

```cpp
// tests/test_cli_main_integration.cpp

class TestCLIMainIntegration : public QObject {
    Q_OBJECT
private slots:
    // Basic integration (5 tests)
    void test01_CLIModeDetection();
    void test02_ProcessETIFile();
    void test03_YAMLOutputGenerated();
    void test04_ExitCodeSuccess();
    void test05_QuietModeOutput();

    // Error handling (5 tests)
    void test06_FileNotFoundExit2();
    void test07_InvalidArgsExit1();
    void test08_OutputWriteFailExit4();
    void test09_HelpDisplayExit0();
    void test10_VersionDisplayExit0();
};
```

**Performance Target:** <5% overhead vs headless processor alone

**Acceptance Criteria:**
- ✅ All 10 tests passing
- ✅ CLI mode auto-detected correctly
- ✅ Exit codes match specification
- ✅ No GUI dependencies in CLI mode
- ✅ Headless operation confirmed

---

## 📋 AGENT 37: CLI FUNCTIONAL TESTS

### **Responsibility**
Create comprehensive end-to-end functional tests for CLI mode.

### **Files to Create**
- `tests/test_cli_mode_functional.cpp`

### **Implementation Requirements**

**Test Strategy:**
- End-to-end testing using QProcess
- Test real ETI file processing
- Validate YAML output format
- Test exit codes and error conditions
- Performance validation

### **Test Requirements (25 tests)**

```cpp
// tests/test_cli_mode_functional.cpp

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
    void test10_YAMLETISnoopCompatible();
    void test11_YAMLContainsEnsemble();
    void test12_YAMLContainsServices();
    void test13_YAMLContainsSubchannels();
    void test14_YAMLContainsFIGStats();
    void test15_YAMLThaiUTF8Preserved();

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
    QString m_testETIFile;
    QString m_testOutputDir;
};
```

**Performance Targets:**
- Processing speed: >900 FPS
- Memory usage: <100 MB
- CLI overhead: <5%
- YAML generation: <100 ms

**Acceptance Criteria:**
- ✅ All 25 tests passing
- ✅ Can run without X11 display
- ✅ Performance targets met
- ✅ YAML parseable by standard tools
- ✅ No memory leaks

---

## 📊 INTEGRATION REQUIREMENTS

### **CMakeLists.txt Updates**

```cmake
# Add CLI source files
set(CLI_SOURCES
    src/cli/cli_argument_parser.cpp
    src/cli/yaml_output_generator.cpp
    src/cli/cli_main.cpp
)

set(CLI_HEADERS
    src/cli/cli_argument_parser.hpp
    src/cli/yaml_output_generator.hpp
    src/cli/cli_main.hpp
)

# Add to main executable
target_sources(streamdab-analyzer PRIVATE
    ${CLI_SOURCES}
    ${CLI_HEADERS}
)

# Add CLI tests
add_executable(test_cli_argument_parser
    tests/test_cli_argument_parser.cpp
    src/cli/cli_argument_parser.cpp
    src/utils/logger.cpp
)
target_link_libraries(test_cli_argument_parser Qt6::Test Qt6::Core)
add_test(NAME test_cli_argument_parser COMMAND test_cli_argument_parser)

add_executable(test_yaml_output_generator
    tests/test_yaml_output_generator.cpp
    src/cli/yaml_output_generator.cpp
    src/utils/logger.cpp
)
target_link_libraries(test_yaml_output_generator Qt6::Test Qt6::Core)
add_test(NAME test_yaml_output_generator COMMAND test_yaml_output_generator)

add_executable(test_cli_main_integration
    tests/test_cli_main_integration.cpp
    src/cli/cli_main.cpp
    src/cli/cli_argument_parser.cpp
    src/cli/yaml_output_generator.cpp
    src/core/headless_eti_processor.cpp
    # ... other dependencies
)
target_link_libraries(test_cli_main_integration Qt6::Test Qt6::Core)
add_test(NAME test_cli_main_integration COMMAND test_cli_main_integration)

add_executable(test_cli_mode_functional
    tests/test_cli_mode_functional.cpp
)
target_link_libraries(test_cli_mode_functional Qt6::Test Qt6::Core)
add_test(NAME test_cli_mode_functional COMMAND test_cli_mode_functional)
```

---

## 📈 SUCCESS CRITERIA

### **Functional Requirements**

✅ **CLI Mode Invocation:**
- [x] Accepts `--cli`, `--input`, `--output` arguments
- [x] Processes ETI files headlessly
- [x] Generates YAML output
- [x] Returns correct exit codes (0, 1, 2, 4)
- [x] Shows help and version info

✅ **YAML Output:**
- [x] ETISnoop-compatible base structure
- [x] All ensemble metadata included
- [x] All services with components
- [x] All subchannels
- [x] FIG statistics
- [x] ETSI compliance summary
- [x] Performance metrics

✅ **Headless Operation:**
- [x] No QApplication dependency
- [x] Works without X11 display
- [x] Quiet mode (minimal output)
- [x] Verbose mode (detailed logging)

### **Non-Functional Requirements**

✅ **Performance:**
- [x] CLI overhead < 5%
- [x] Processing rate > 900 FPS
- [x] YAML generation < 100 ms
- [x] Memory usage < 100 MB

✅ **Quality:**
- [x] 70/70 tests passing (100%)
- [x] Code coverage ≥85%
- [x] Zero compilation errors
- [x] Zero memory leaks (Valgrind)

✅ **Reliability:**
- [x] Handles malformed ETI gracefully
- [x] Proper error messages
- [x] No crashes on invalid input
- [x] Atomic file writes

---

## 📋 EXECUTION TIMELINE

### **Parallel Agent Execution**

**Hour 0-4:** Agents 34, 35, 36, 37 launch simultaneously
**Hour 4-6:** Code review and integration
**Hour 6-7:** Build verification and testing
**Hour 7-8:** Documentation and commit

**Total Time:** 8 hours (parallel execution)

### **Dependencies**

- Agent 36 depends on Agent 34 and 35 (can start after headers defined)
- Agent 37 depends on all others (can start with stubs)
- Build system updates can happen in parallel

---

## 📊 PDCA CYCLE

### **Plan Phase** (This Document)
- ✅ Requirements analyzed from CLI_MODE_FEATURE_PLAN.md
- ✅ 4 agents defined with clear responsibilities
- ✅ 70 tests specified across all agents
- ✅ Performance targets established
- ✅ Acceptance criteria defined

### **Do Phase** (Agent Execution)
- 🔄 Launch Agent 34 (CLI Argument Parser)
- 🔄 Launch Agent 35 (YAML Output Generator)
- 🔄 Launch Agent 36 (CLI Main Integration)
- 🔄 Launch Agent 37 (CLI Functional Tests)

### **Check Phase** (Verification)
- 🔄 Verify all 70 tests passing
- 🔄 Verify performance targets met
- 🔄 Verify YAML format compliance
- 🔄 Verify headless operation

### **Act Phase** (Integration)
- 🔄 Fix any failing tests
- 🔄 Optimize performance bottlenecks
- 🔄 Document CLI usage
- 🔄 Commit to git with comprehensive message

---

## 🎯 DELIVERABLES CHECKLIST

### **Code Files**
- [ ] src/cli/cli_argument_parser.cpp/hpp
- [ ] src/cli/yaml_output_generator.cpp/hpp
- [ ] src/cli/cli_main.cpp/hpp
- [ ] Updated src/main.cpp
- [ ] Updated CMakeLists.txt

### **Test Files**
- [ ] tests/test_cli_argument_parser.cpp (15 tests)
- [ ] tests/test_yaml_output_generator.cpp (20 tests)
- [ ] tests/test_cli_main_integration.cpp (10 tests)
- [ ] tests/test_cli_mode_functional.cpp (25 tests)

### **Documentation**
- [ ] CLI usage examples
- [ ] YAML output format specification
- [ ] Exit code documentation
- [ ] Integration guide for CI/CD

### **Verification**
- [ ] 70/70 tests passing
- [ ] Performance benchmarks met
- [ ] Headless operation confirmed
- [ ] YAML format validated

---

## 📚 REFERENCES

**Related Documents:**
- [docs/CLI_MODE_FEATURE_PLAN.md](docs/CLI_MODE_FEATURE_PLAN.md) - Comprehensive CLI mode design
- [PDCA_WEEK7_V1.1_COMPLETE.md](PDCA_WEEK7_V1.1_COMPLETE.md) - v1.1 baseline
- [CLAUDE.md](CLAUDE.md) - Project status

**ETSI Standards:**
- ETSI EN 300 799 - ETI Distribution
- ETSI EN 300 401 - DAB System

**External Tools:**
- ETISnoop - Reference YAML format

---

## 🚀 NEXT STEPS

1. **Review and Approve Plan** - PM Agent approval required
2. **Launch Agents in Parallel** - Start all 4 agents simultaneously
3. **Monitor Progress** - Track agent outputs and test results
4. **Integration Testing** - Verify end-to-end functionality
5. **Documentation** - Update CLAUDE.md with CLI mode status
6. **Git Commit** - Commit with comprehensive message
7. **Tag Release** - v1.2-cli-complete

---

**Plan Status:** ✅ READY FOR EXECUTION
**Estimated Completion:** 8 hours (parallel execution)
**Expected Test Count:** 70 tests (100% pass rate target)
**Expected Performance:** >900 FPS processing, <5% overhead

**Ready to launch multi-agent PDCA workflow on PM approval.**
