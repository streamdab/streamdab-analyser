# TDD Framework Configuration for ETI Stream Analyzer
cmake_minimum_required(VERSION 3.21)
# TDD MANDATE: All testing frameworks must be operational
message(STATUS "=== TDD Framework Setup - Phase 1 Priority ===")

# Find Google Test framework with GMock
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    message(STATUS "GTest not found, downloading and building from source")
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(gmock_build_tests OFF CACHE BOOL "" FORCE)
    set(gtest_build_tests OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    # Make gmock and gtest available
    set(GTEST_LIBS gtest gtest_main gmock gmock_main)
else()
    set(GTEST_LIBS GTest::GTest GTest::Main)
    # Try to find GMock separately if GTest was found but GMock wasn't included
    find_package(GMock QUIET)
    if(GMock_FOUND)
        list(APPEND GTEST_LIBS GMock::GMock GMock::Main)
    else()
        # Add gmock manually if available
        find_library(GMOCK_LIB gmock)
        find_library(GMOCK_MAIN_LIB gmock_main)
        if(GMOCK_LIB AND GMOCK_MAIN_LIB)
            list(APPEND GTEST_LIBS ${GMOCK_LIB} ${GMOCK_MAIN_LIB})
        endif()
    endif()
endif()

# Find Qt6 Test module for GUI testing
find_package(Qt6 REQUIRED COMPONENTS Test Core Widgets Gui)
# Enable Qt test framework
set(CMAKE_AUTOMOC ON)
# TDD Configuration - Use C++20 to match main project
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# TDD REQUIREMENT: Enable code coverage by default
option(ENABLE_COVERAGE "Enable TDD code coverage reporting" ON)
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
        add_link_options(--coverage)
        message(STATUS "TDD: Code coverage enabled")
    endif()
endif()
# Include project source directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)

# Include TDD framework
include(tdd_workflow.cmake)
# TDD MANDATE: 80% minimum coverage enforcement
set(COVERAGE_MINIMUM_PERCENTAGE 80)
message(STATUS "TDD: Minimum coverage requirement: ${COVERAGE_MINIMUM_PERCENTAGE}%")

# ============================================================================
# PHASE 1: ETI FRAME PARSING AND FIG PROCESSING TESTS (NEW)
# ============================================================================
message(STATUS "")
message(STATUS "=== Phase 1: ETI Processing Unit Tests ===")

# Source files from main project for Phase 1 tests
set(PHASE1_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/enhanced_eti_processor_qt.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/advanced_fig_analyzer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_compliance_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)

# Phase 1 test executables (Qt Test framework)
set(PHASE1_TESTS
    test_eti_frame_parser
    test_fig0_processor
    test_fig1_processor
    test_eti_li_crc_validation
)

# Create Phase 1 test executables
foreach(test_name ${PHASE1_TESTS})
    add_executable(${test_name}
        ${test_name}.cpp
        ${PHASE1_CORE_SOURCES}
    )

    # Set C++ standard
    set_property(TARGET ${test_name} PROPERTY CXX_STANDARD 20)
    set_property(TARGET ${test_name} PROPERTY CXX_STANDARD_REQUIRED ON)

    # Enable automatic MOC for Qt
    set_property(TARGET ${test_name} PROPERTY AUTOMOC ON)

    # Link libraries
    target_link_libraries(${test_name}
        PRIVATE
        Qt6::Core
        Qt6::Test
        pthread
    )

    # Include directories
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils
    )

    # Add test to CTest
    add_test(NAME ${test_name} COMMAND ${test_name})

    # Set test properties
    set_tests_properties(${test_name} PROPERTIES
        TIMEOUT 300
        LABELS "Phase1;Unit;ETI"
    )

    # Coverage flags
    if(ENABLE_COVERAGE)
        target_compile_options(${test_name} PRIVATE --coverage)
        target_link_options(${test_name} PRIVATE --coverage)
    endif()

    message(STATUS "Configured Phase 1 test: ${test_name}")
endforeach()

# Phase 1 test targets
add_custom_target(run_phase1_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -L "Phase1" --output-on-failure --verbose
    DEPENDS ${PHASE1_TESTS}
    COMMENT "Running Phase 1: ETI Processing Unit Tests"
)

add_custom_target(run_phase1_tests_quick
    COMMAND ${CMAKE_CTEST_COMMAND} -L "Phase1" --output-on-failure
    DEPENDS ${PHASE1_TESTS}
    COMMENT "Running Phase 1 tests (quick mode)"
)

message(STATUS "Phase 1 test targets created:")
message(STATUS "  - run_phase1_tests: Run all Phase 1 ETI tests (verbose)")
message(STATUS "  - run_phase1_tests_quick: Run Phase 1 tests (quiet mode)")
message(STATUS "==========================================")
message(STATUS "")

# ============================================================================
# EXISTING TDD FRAMEWORK TESTS
# ============================================================================

# TDD FRAMEWORK: Organize tests by type and priority
set(GTEST_UNIT_SOURCES
    test_main.cpp
    core/test_eti_processor.cpp
    core/test_dab_decoder.cpp
    core/test_fic_decoder.cpp
    core/test_headless_eti_processor.cpp  # NEW: TDD test for headless processing
    core/test_phase2_streaming_components.cpp  # Phase 2: ETSI Compliance & Streaming Engine
    utils/test_config_manager.cpp
    # utils/test_file_utils.cpp  # Temporarily disabled - missing header
    utils/test_signal_processing.cpp

    # TDD FRAMEWORK IMPLEMENTATION
    tdd_framework.cpp

    # NEW COMPREHENSIVE TESTS FOR REAL IMPLEMENTATIONS
    unit/test_etsi_compliance_validation.cpp
    unit/test_real_service_discovery.cpp
    unit/test_cross_platform_performance.cpp
    unit/test_modern_frame_parser.cpp

    # 100% ETSI COMPLIANCE VALIDATION TEST SUITE
    unit/test_100_percent_compliance.cpp
    unit/test_queue_management_optimization.cpp
    unit/test_audio_quality_optimization.cpp
    unit/test_crc_error_recovery.cpp
    unit/test_realworld_eti_compatibility.cpp
    unit/test_performance_impact_validation.cpp

    # STYLE COMPLIANCE TEST SUITE - TDD IMPLEMENTATION
    style_compliance_test.cpp
)

# Test fixtures and test data - only working files
set(TEST_FIXTURE_SOURCES
    fixtures/eti_test_data.cpp
    fixtures/eti_streams/optimized_compliance_frames.cpp
    fixtures/test_data_generators.cpp
)
# Qt GUI Test Framework - Professional broadcast testing
set(QT_GUI_TEST_SOURCES
    gui/test_main_window.cpp
    gui/test_analyzer_widget.cpp
    gui/test_constellation_widget.cpp
    gui/test_service_browser.cpp
    gui/test_settings_dialog.cpp
    gui/test_about_dialog.cpp
    gui/test_real_eti_file_loading.cpp  # NEW: Bangkok ETI file validation with AAA pattern
    # NEW PROFESSIONAL GUI TDD FRAMEWORK TESTS
    gui/test_three_panel_professional_layout.cpp
    gui/test_qt_designer_integration.cpp
    gui/test_professional_user_workflows.cpp
    gui/test_gui_performance_responsiveness.cpp
)

# E2E Test Framework - Complete workflow testing
set(E2E_TEST_SOURCES
    e2e/test_comprehensive_tdd_workflows.cpp
    e2e/test_eti_file_processing.cpp
    e2e/test_modern_eti_core_engine_integration.cpp
)
# GUI Test Framework Sources
set(GUI_TEST_FRAMEWORK_SOURCES
    gui/gui_test_framework.h
    gui/gui_test_framework.cpp
    # PROFESSIONAL GUI TDD FRAMEWORK
    gui/professional_gui_tdd_framework.h
    gui/professional_gui_tdd_framework.cpp
)
# GUI Mock Components
set(GUI_MOCK_SOURCES
    mocks/mock_gui_components.h
    mocks/mock_gui_components.cpp
)
# TDD REQUIREMENT: Create separate test executables for different frameworks
add_executable(eti_analyzer_unit_tests
    ${GTEST_UNIT_SOURCES}
    ${TEST_FIXTURE_SOURCES}
)

# E2E Test Executable - Complete workflow testing
add_executable(eti_analyzer_e2e_tests
    ${E2E_TEST_SOURCES}
    ${TEST_FIXTURE_SOURCES}
    tdd_framework.cpp
)
# TDD GUI TESTING: Professional Qt Test Framework Integration
add_executable(eti_analyzer_gui_tests
    ${QT_GUI_TEST_SOURCES}
    ${GUI_TEST_FRAMEWORK_SOURCES}
    ${GUI_MOCK_SOURCES}
    ${TEST_FIXTURE_SOURCES}
    # Add main project GUI sources that tests depend on
    # ../src/gui/main_window.cpp  # Embedded in main.cpp
    ../src/gui/analyzer_widget.cpp
    ../src/gui/service_browser.cpp
    ../src/gui/constellation_widget.cpp
    ../src/gui/settings_dialog.cpp
    ../src/gui/about_dialog.cpp
    ../src/gui/eti_service_tree_model.cpp
    ../src/gui/eti_frame_list_model.cpp
    ../src/gui/fig_analysis_widget.cpp
    ../src/gui/audio_monitoring_widget.cpp
    ../src/gui/service_explorer_panel.cpp
    ../src/gui/eti_analysis_widget.cpp
    # Add core sources that GUI depends on
    ../src/core/eti_processor.cpp

    ../src/core/eti_file_worker.cpp
    ../src/core/eti_types.cpp
    ../src/core/dab_decoder.cpp
    ../src/core/fic_decoder.cpp
    ../src/core/ensemble_manager.cpp
    ../src/core/service_manager.cpp
    ../src/core/dab_audio_decoder.cpp
    # Add utils sources
    ../src/utils/logger.cpp
    ../src/utils/config_manager.cpp
)
# TDD FRAMEWORK: Link Google Test for unit tests (GTEST_LIBS already set above)
target_link_libraries(eti_analyzer_unit_tests
    ${GTEST_LIBS}
    Qt6::Core
    Qt6::Test
)

# Link E2E test executable
target_link_libraries(eti_analyzer_e2e_tests
    ${GTEST_LIBS}
    Qt6::Core
    Qt6::Test
    Qt6::Widgets
    Qt6::Gui
)
# TDD REQUIREMENT: Link to main project components for testing
target_include_directories(eti_analyzer_unit_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

# Include directories for E2E tests
target_include_directories(eti_analyzer_e2e_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)
# TDD FRAMEWORK: Link Qt Test for GUI tests with comprehensive framework
target_link_libraries(eti_analyzer_gui_tests
    Qt6::Core
    Qt6::Test
    Qt6::Widgets
    Qt6::Gui
    Qt6::Network
    ${GTEST_LIBS}
)

# Conditionally link Qt6::UiTools if available
if(HAVE_QT6_UITOOLS)
    target_link_libraries(eti_analyzer_gui_tests Qt6::UiTools)
endif()
# TDD REQUIREMENT: Link to main project components for GUI testing
target_include_directories(eti_analyzer_gui_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)
# TDD FRAMEWORK: Compile main project sources with test executables
# This creates a more integrated testing approach
set(MAIN_PROJECT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/eti_processor.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/eti_file_worker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/eti_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/dab_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fic_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/ensemble_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/service_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/dab_audio_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/config_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/file_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/signal_processing.cpp
)
set(MAIN_PROJECT_GUI_SOURCES
#     ${CMAKE_CURRENT_SOURCE_DIR}/# ../src/gui/main_window.cpp  # Embedded in main.cpp  # Embedded in main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui/analyzer_widget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui/constellation_widget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui/service_browser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui/settings_dialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui/about_dialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui/audio_monitoring_widget.cpp
    ../src/gui/service_explorer_panel.cpp
    ../src/gui/eti_analysis_widget.cpp
)
# Link main project sources to unit tests
target_sources(eti_analyzer_unit_tests PRIVATE ${MAIN_PROJECT_SOURCES})
# Link main project sources to GUI tests
target_sources(eti_analyzer_gui_tests PRIVATE
    ${MAIN_PROJECT_SOURCES}
    ${MAIN_PROJECT_GUI_SOURCES}
)
# TDD FRAMEWORK: Platform-specific linking for unit tests
if(UNIX AND NOT APPLE)
    target_link_libraries(eti_analyzer_unit_tests Threads::Threads)
    target_link_libraries(eti_analyzer_gui_tests Threads::Threads)
elseif(WIN32)
    target_link_libraries(eti_analyzer_unit_tests
        ${WINMM_LIBRARY}
        ${WSOCK32_LIBRARY}
        ${WS2_32_LIBRARY}
    )
    target_link_libraries(eti_analyzer_gui_tests
        ${WINMM_LIBRARY}
        ${WSOCK32_LIBRARY}
        ${WS2_32_LIBRARY}
    )
elseif(APPLE)
    target_link_libraries(eti_analyzer_unit_tests
        ${COREAUDIO_FRAMEWORK}
        ${AUDIOTOOLBOX_FRAMEWORK}
        ${AUDIOUNIT_FRAMEWORK}
    )
    target_link_libraries(eti_analyzer_gui_tests
        ${COREAUDIO_FRAMEWORK}
        ${AUDIOTOOLBOX_FRAMEWORK}
        ${AUDIOUNIT_FRAMEWORK}
    )
endif()
# TDD FRAMEWORK: Add project dependencies (required for testing main project code)
# These dependencies are required since we're linking main project sources
# Link external dependencies using PkgConfig (same as main project)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FAAD2 REQUIRED faad2)
pkg_check_modules(FFTW3 REQUIRED fftw3)
find_package(yaml-cpp QUIET)
# Link dependencies to unit tests
set(TEST_LIBS
    ${pkgcfg_lib_FAAD2_faad}
    ${pkgcfg_lib_FFTW3_fftw3}
)
if(yaml-cpp_FOUND)
    list(APPEND TEST_LIBS yaml-cpp::yaml-cpp)
endif()
target_link_libraries(eti_analyzer_unit_tests ${TEST_LIBS}
)
# Link dependencies to GUI tests
target_link_libraries(eti_analyzer_gui_tests ${TEST_LIBS}
)
# Include directories for dependencies
target_include_directories(eti_analyzer_unit_tests PRIVATE
    ${FAAD2_INCLUDE_DIRS}
    ${FFTW3_INCLUDE_DIRS}
)
target_include_directories(eti_analyzer_gui_tests PRIVATE
    ${FAAD2_INCLUDE_DIRS}
    ${FFTW3_INCLUDE_DIRS}
)
# Compile definitions for dependencies
target_compile_definitions(eti_analyzer_unit_tests PRIVATE
    ${FAAD2_CFLAGS_OTHER}
    ${FFTW3_CFLAGS_OTHER}
)
target_compile_definitions(eti_analyzer_gui_tests PRIVATE
    ${FAAD2_CFLAGS_OTHER}
    ${FFTW3_CFLAGS_OTHER}
)
# TDD MANDATE: Register all test suites with CTest
add_test(NAME unit_tests
         COMMAND eti_analyzer_unit_tests)
add_test(NAME gui_tests
         COMMAND eti_analyzer_gui_tests)
add_test(NAME e2e_tests
         COMMAND eti_analyzer_e2e_tests)

# Register individual test categories using TDD framework
tdd_register_test(eti_analyzer_unit_tests "UNIT" "CRITICAL")
tdd_register_test(eti_analyzer_gui_tests "GUI" "HIGH")
tdd_register_test(eti_analyzer_e2e_tests "E2E" "CRITICAL")
# TDD FRAMEWORK: Test discovery integration
# Note: eti_analyzer_unit_tests uses Qt Test framework, not Google Test
# Google Test discovery is not applicable for Qt Test executables
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.10)
    include(GoogleTest)
    # gtest_discover_tests() removed - eti_analyzer_unit_tests uses Qt Test framework
    message(STATUS "TDD: Test discovery configured for Qt Test framework")
endif()
# TDD MANDATE: Custom test targets for Red-Green-Refactor workflow
add_custom_target(tdd_red_phase
    COMMAND echo "🔴 TDD RED Phase: Running failing tests to verify test correctness"
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose || true
    DEPENDS eti_analyzer_unit_tests eti_analyzer_gui_tests eti_analyzer_e2e_tests ${PHASE1_TESTS}
    COMMENT "TDD RED: Expecting test failures (this is correct!)"
)
add_custom_target(tdd_green_phase
    COMMAND echo "🟢 TDD GREEN Phase: Running tests expecting all to pass"
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS eti_analyzer_unit_tests eti_analyzer_gui_tests eti_analyzer_e2e_tests ${PHASE1_TESTS}
    COMMENT "TDD GREEN: All tests must pass"
)
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS eti_analyzer_unit_tests eti_analyzer_gui_tests eti_analyzer_e2e_tests ${PHASE1_TESTS}
    COMMENT "Running all TDD test suites (including Phase 1)"
)
add_custom_target(run_tests_quiet
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS eti_analyzer_unit_tests eti_analyzer_gui_tests eti_analyzer_e2e_tests ${PHASE1_TESTS}
    COMMENT "Running all tests (quiet mode)"
)

# TDD Workflow enforcement targets
tdd_create_quality_gate(tdd_core_gate eti_analyzer_unit_tests)
tdd_create_quality_gate(tdd_integration_gate eti_analyzer_e2e_tests)
tdd_create_quality_gate(tdd_full_gate eti_analyzer_unit_tests eti_analyzer_gui_tests eti_analyzer_e2e_tests)

# TDD Compliance check
tdd_create_compliance_check()
add_custom_target(run_unit_tests_only
    COMMAND eti_analyzer_unit_tests --gtest_output=xml:unit_test_results.xml
    DEPENDS eti_analyzer_unit_tests
    COMMENT "Running unit tests only with XML output"
)
add_custom_target(run_gui_tests_only
    COMMAND eti_analyzer_gui_tests
    DEPENDS eti_analyzer_gui_tests
    COMMENT "Running GUI tests only"
)

add_custom_target(run_e2e_tests_only
    COMMAND eti_analyzer_e2e_tests
    DEPENDS eti_analyzer_e2e_tests
    COMMENT "Running E2E tests only"
)

# TDD FRAMEWORK: 100% ETSI Compliance Validation Target
add_custom_target(run_100_percent_compliance_tests
    COMMAND echo "🧪 100% ETSI COMPLIANCE VALIDATION TESTS"
    COMMAND echo "========================================="
    COMMAND eti_analyzer_unit_tests --gtest_filter="*100PercentCompliance*:*QueueManagementOptimization*:*AudioQualityOptimization*:*CRCErrorRecovery*:*RealWorldETICompatibility*:*PerformanceImpactValidation*"
    DEPENDS eti_analyzer_unit_tests
    COMMENT "Running comprehensive 100% ETSI compliance validation test suite"
)

add_custom_target(run_compliance_validation_suite
    COMMAND echo "🎯 COMPREHENSIVE COMPLIANCE VALIDATION SUITE"
    COMMAND echo "============================================="
    COMMAND echo "Testing 100% ETSI compliance achievement..."
    COMMAND eti_analyzer_unit_tests --gtest_filter="*100PercentCompliance*" --gtest_output=xml:100_percent_compliance_results.xml
    COMMAND echo "Testing queue management optimization..."
    COMMAND eti_analyzer_unit_tests --gtest_filter="*QueueManagementOptimization*"
    COMMAND echo "Testing audio quality optimization..."
    COMMAND eti_analyzer_unit_tests --gtest_filter="*AudioQualityOptimization*"
    COMMAND echo "Testing CRC error recovery..."
    COMMAND eti_analyzer_unit_tests --gtest_filter="*CRCErrorRecovery*"
    COMMAND echo "Testing real-world ETI compatibility..."
    COMMAND eti_analyzer_unit_tests --gtest_filter="*RealWorldETICompatibility*"
    COMMAND echo "Testing performance impact validation..."
    COMMAND eti_analyzer_unit_tests --gtest_filter="*PerformanceImpactValidation*"
    COMMAND echo "✅ 100% ETSI COMPLIANCE VALIDATION COMPLETE"
    DEPENDS eti_analyzer_unit_tests
    COMMENT "Comprehensive 100% ETSI compliance validation with detailed reporting"
)

# TDD STYLE COMPLIANCE TARGET - Achieving 10.0/10.0 Perfect Score
add_custom_target(run_style_compliance_tests
    COMMAND echo "🎨 STYLE COMPLIANCE TEST SUITE - TDD METHODOLOGY"
    COMMAND echo "=================================================="
    COMMAND echo "Testing layer-based naming conventions..."
    COMMAND eti_analyzer_unit_tests --gtest_filter="*StyleCompliance*" --gtest_output=xml:style_compliance_results.xml
    COMMAND echo "✅ STYLE COMPLIANCE VALIDATION COMPLETE"
    DEPENDS eti_analyzer_unit_tests
    COMMENT "TDD Style Compliance Testing - Achieving 10.0/10.0 Perfect Score"
)
# TDD FRAMEWORK: Memory leak testing (Linux/macOS with Valgrind)
if(UNIX AND NOT APPLE)
    find_program(VALGRIND_EXECUTABLE valgrind)
    if(VALGRIND_EXECUTABLE)
        add_custom_target(tdd_memcheck_unit
            COMMAND ${VALGRIND_EXECUTABLE}
                --tool=memcheck
                --leak-check=full
                --show-reachable=yes
                --num-callers=20
                --track-fds=yes
                $<TARGET_FILE:eti_analyzer_unit_tests>
            DEPENDS eti_analyzer_unit_tests
            COMMENT "TDD: Memory checking unit tests with Valgrind"
        )
        add_custom_target(tdd_memcheck_gui
            COMMAND ${VALGRIND_EXECUTABLE}
                --tool=memcheck
                --leak-check=full
                --show-reachable=yes
                --num-callers=20
                --track-fds=yes
                $<TARGET_FILE:eti_analyzer_gui_tests>
            DEPENDS eti_analyzer_gui_tests
            COMMENT "TDD: Memory checking GUI tests with Valgrind"
        )
        add_custom_target(tdd_memcheck_all
            DEPENDS tdd_memcheck_unit tdd_memcheck_gui
            COMMENT "TDD: Memory checking all test suites"
        )
        message(STATUS "TDD: Valgrind memory checking available")
    endif()
endif()
# TDD MANDATE: Coverage testing with quality gates
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(ENABLE_COVERAGE)
        target_compile_options(eti_analyzer_unit_tests PRIVATE --coverage)
        target_link_options(eti_analyzer_unit_tests PRIVATE --coverage)
        target_compile_options(eti_analyzer_gui_tests PRIVATE --coverage)
        target_link_options(eti_analyzer_gui_tests PRIVATE --coverage)
        target_compile_options(eti_analyzer_e2e_tests PRIVATE --coverage)
        target_link_options(eti_analyzer_e2e_tests PRIVATE --coverage)
        find_program(GCOV_EXECUTABLE gcov)
        find_program(LCOV_EXECUTABLE lcov)
        find_program(GENHTML_EXECUTABLE genhtml)
        if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
            add_custom_target(tdd_coverage_generate
                COMMAND ${LCOV_EXECUTABLE} --directory . --capture --output-file coverage.info
                COMMAND ${LCOV_EXECUTABLE} --remove coverage.info '/usr/*' '*/tests/*' '*/third_party/*' --output-file coverage_clean.info
                COMMAND ${GENHTML_EXECUTABLE} coverage_clean.info --output-directory coverage_html
                DEPENDS run_tests
                COMMENT "TDD: Generating code coverage report"
            )
            add_custom_target(tdd_coverage_check
                COMMAND ${CMAKE_COMMAND} -E env
                    COVERAGE_FILE=coverage_clean.info
                    MINIMUM_COVERAGE=${COVERAGE_MINIMUM_PERCENTAGE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/check_coverage.sh
                DEPENDS tdd_coverage_generate
                COMMENT "TDD: Enforcing ${COVERAGE_MINIMUM_PERCENTAGE}% coverage requirement"
            )
            add_custom_target(tdd_quality_gate
                DEPENDS tdd_green_phase tdd_coverage_check
                COMMENT "TDD: Full quality gate validation"
            )
            message(STATUS "TDD: Coverage reporting with ${COVERAGE_MINIMUM_PERCENTAGE}% enforcement enabled")
        endif()
    endif()
endif()
# Benchmark tests (optional)
option(ENABLE_BENCHMARKS "Enable benchmark tests" OFF)
if(ENABLE_BENCHMARKS)
    # Add benchmark test executable here
    # Requires Google Benchmark or similar framework
endif()
# TDD FRAMEWORK: Performance benchmarking with real implementation tests
option(ENABLE_TDD_BENCHMARKS "Enable TDD performance benchmark tests" ON)
if(ENABLE_TDD_BENCHMARKS)
    # Performance regression tests for real implementation
    add_executable(eti_analyzer_performance_tests
        performance/test_performance_regression.cpp
        ${MAIN_PROJECT_SOURCES}
    )

    target_link_libraries(eti_analyzer_performance_tests
        ${GTEST_LIBS}
        Qt6::Core
        Qt6::Test
        ${TEST_LIBS}
    )

    target_include_directories(eti_analyzer_performance_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/fixtures
        ${FAAD2_INCLUDE_DIRS}
        ${FFTW3_INCLUDE_DIRS}

    )

    target_compile_definitions(eti_analyzer_performance_tests PRIVATE
        ${FAAD2_CFLAGS_OTHER}
        ${FFTW3_CFLAGS_OTHER}
    )

    add_test(NAME performance_tests
             COMMAND eti_analyzer_performance_tests)

    add_custom_target(run_performance_tests
        COMMAND eti_analyzer_performance_tests
        DEPENDS eti_analyzer_performance_tests
        COMMENT "TDD: Running comprehensive performance regression tests"
    )

    # Optional Google Benchmark integration
    find_package(benchmark QUIET)
    if(benchmark_FOUND)
        add_executable(eti_analyzer_benchmarks
            benchmarks/benchmark_eti_processing.cpp
            benchmarks/benchmark_fic_decoding.cpp
        )
        target_link_libraries(eti_analyzer_benchmarks
            benchmark::benchmark
            GTest::GTest
            Qt6::Core
        )
        add_custom_target(run_benchmarks
            COMMAND eti_analyzer_benchmarks
            DEPENDS eti_analyzer_benchmarks
            COMMENT "TDD: Running optional Google Benchmark tests"
        )
        message(STATUS "TDD: Google Benchmark integration enabled")
    else()
        message(STATUS "TDD: Google Benchmark not found, using Qt-based performance tests only")
    endif()

    message(STATUS "TDD: Comprehensive performance testing enabled")
else()
    message(STATUS "TDD: Performance testing disabled")
endif()
# Add GUI TDD Integration Tests subdirectory
add_subdirectory(gui)

# Add other specialized test directories (e2e is handled by main CMakeLists.txt)
add_subdirectory(e2e_gui)
add_subdirectory(etsi_standards)
add_subdirectory(network)
add_subdirectory(network_streaming)

# TDD FRAMEWORK STATUS SUMMARY
message(STATUS "")
message(STATUS "=== TDD Framework Configuration Complete ===")
message(STATUS "Google Test Framework: ENABLED")
message(STATUS "Qt Test Framework: ENABLED")
message(STATUS "Google Mock Framework: ENABLED")
message(STATUS "Code Coverage: ${ENABLE_COVERAGE}")
message(STATUS "Minimum Coverage: ${COVERAGE_MINIMUM_PERCENTAGE}%")
message(STATUS "")
message(STATUS "PHASE 1 ETI PROCESSING TESTS:")
message(STATUS "  - test_eti_frame_parser: ETI frame parsing")
message(STATUS "  - test_fig0_processor: FIG Type 0 decoding")
message(STATUS "  - test_fig1_processor: FIG Type 1 labels")
message(STATUS "  - test_eti_li_crc_validation: ETI-LI CRC validation (CRITICAL-003 fix)")
message(STATUS "  - run_phase1_tests: Run all Phase 1 tests")
message(STATUS "")
message(STATUS "TDD WORKFLOW TARGETS:")
message(STATUS "  🔴 tdd_red_phase: Verify tests fail correctly")
message(STATUS "  🟢 tdd_green_phase: Ensure all tests pass")
message(STATUS "  🔵 tdd_quality_gate: Full TDD compliance check")
message(STATUS "")
message(STATUS "AVAILABLE TEST TARGETS:")
message(STATUS "  - run_tests: Run all TDD test suites")
message(STATUS "  - run_unit_tests_only: Unit tests with Google Test")
message(STATUS "  - run_gui_tests_only: GUI tests with Qt Test")
message(STATUS "  - run_e2e_tests_only: End-to-end workflow tests")
message(STATUS "  - run_performance_tests: Comprehensive performance validation")
message(STATUS "  - run_tests_quiet: Minimal output testing")
message(STATUS "  - test_eti_ui_integration: TDD ETI UI Integration tests with Bangkok data")
message(STATUS "")
message(STATUS "100% ETSI COMPLIANCE VALIDATION TARGETS:")
message(STATUS "  - run_100_percent_compliance_tests: Comprehensive compliance validation")
message(STATUS "  - run_compliance_validation_suite: Detailed compliance test suite")
message(STATUS "    * 100% compliance score validation")
message(STATUS "    * Queue management optimization validation")
message(STATUS "    * Audio quality threshold optimization")
message(STATUS "    * CRC error recovery validation")
message(STATUS "    * Real-world ETI stream compatibility")
message(STATUS "    * Performance impact validation")
if(ENABLE_COVERAGE)
    message(STATUS "  - tdd_coverage_generate: Generate coverage reports")
    message(STATUS "  - tdd_coverage_check: Enforce coverage requirements")
endif()
if(UNIX AND NOT APPLE AND VALGRIND_EXECUTABLE)
    message(STATUS "  - tdd_memcheck_all: Memory leak detection")
endif()
message(STATUS "")
message(STATUS "TDD QUALITY GATES: ACTIVE")
message(STATUS "Red-Green-Refactor Workflow: ENFORCED")
message(STATUS "===========================================")
message(STATUS "")
