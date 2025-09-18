# TDD Framework Configuration for ETI Stream Analyzer
cmake_minimum_required(VERSION 3.21)
# TDD MANDATE: All testing frameworks must be operational
message(STATUS "=== TDD Framework Setup - Phase 1 Priority ===")
# TDD FRAMEWORK: Use targets from parent CMakeLists.txt
# Google Test and Mock should be available from parent cmake configuration
# Find Qt6 Test module for GUI testing
find_package(Qt6 REQUIRED COMPONENTS Test Core Widgets Gui)
# Enable Qt test framework
set(CMAKE_AUTOMOC ON)
# TDD Configuration
set(CMAKE_CXX_STANDARD 17)
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
# TDD MANDATE: 80% minimum coverage enforcement
set(COVERAGE_MINIMUM_PERCENTAGE 80)
message(STATUS "TDD: Minimum coverage requirement: ${COVERAGE_MINIMUM_PERCENTAGE}%")
# TDD FRAMEWORK: Organize tests by type and priority
set(GTEST_UNIT_SOURCES
    test_main.cpp
    core/test_eti_processor.cpp
    core/test_dab_decoder.cpp
    core/test_fic_decoder.cpp
    utils/test_config_manager.cpp
    utils/test_file_utils.cpp
    utils/test_signal_processing.cpp
)
# Qt GUI Test Framework - Professional broadcast testing
set(QT_GUI_TEST_SOURCES
    gui/test_main_window.cpp
    gui/test_analyzer_widget.cpp
    gui/test_constellation_widget.cpp
    gui/test_service_browser.cpp
    gui/test_settings_dialog.cpp
    gui/test_about_dialog.cpp
)
# GUI Test Framework Sources
set(GUI_TEST_FRAMEWORK_SOURCES
    gui/gui_test_framework.h
    gui/gui_test_framework.cpp
)
# GUI Mock Components
set(GUI_MOCK_SOURCES
    mocks/mock_gui_components.h
    mocks/mock_gui_components.cpp
)
# TDD REQUIREMENT: Create separate test executables for different frameworks
add_executable(eti_analyzer_unit_tests
    ${GTEST_UNIT_SOURCES}
)
# TDD GUI TESTING: Professional Qt Test Framework Integration
add_executable(eti_analyzer_gui_tests
    ${QT_GUI_TEST_SOURCES}
    ${GUI_TEST_FRAMEWORK_SOURCES}
    ${GUI_MOCK_SOURCES}
)
# TDD FRAMEWORK: Link Google Test for unit tests
target_link_libraries(eti_analyzer_unit_tests
    GTest::GTest
    GTest::Main
    Qt6::Core
    Qt6::Test
)
# TDD REQUIREMENT: Link to main project components for testing
target_include_directories(eti_analyzer_unit_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
# TDD FRAMEWORK: Link Qt Test for GUI tests with comprehensive framework
target_link_libraries(eti_analyzer_gui_tests
    Qt6::Core
    Qt6::Test
    Qt6::Widgets
    Qt6::Gui
    Qt6::Network
    GTest::GTest
)
# TDD REQUIREMENT: Link to main project components for GUI testing
target_include_directories(eti_analyzer_gui_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
# TDD FRAMEWORK: Compile main project sources with test executables
# This creates a more integrated testing approach
set(MAIN_PROJECT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/eti_processor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/dab_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fic_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/ensemble_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/service_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/audio_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/config_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/file_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/signal_processing.cpp
)
set(MAIN_PROJECT_GUI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui/main_window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui/analyzer_widget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui/constellation_widget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui/service_browser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui/settings_dialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui/about_dialog.cpp
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
find_package(yaml-cpp REQUIRED)
# Link dependencies to unit tests
target_link_libraries(eti_analyzer_unit_tests
    ${pkgcfg_lib_FAAD2_faad}
    ${pkgcfg_lib_FFTW3_fftw3}
    yaml-cpp::yaml-cpp
)
# Link dependencies to GUI tests
target_link_libraries(eti_analyzer_gui_tests
    ${pkgcfg_lib_FAAD2_faad}
    ${pkgcfg_lib_FFTW3_fftw3}
    yaml-cpp::yaml-cpp
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
# TDD MANDATE: Register both test suites with CTest
add_test(NAME unit_tests
         COMMAND eti_analyzer_unit_tests)
add_test(NAME gui_tests
         COMMAND eti_analyzer_gui_tests)
# TDD FRAMEWORK: Google Test discovery integration
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.10)
    include(GoogleTest)
    gtest_discover_tests(eti_analyzer_unit_tests)
    message(STATUS "TDD: Google Test discovery enabled")
endif()
# TDD MANDATE: Custom test targets for Red-Green-Refactor workflow
add_custom_target(tdd_red_phase
    COMMAND echo "🔴 TDD RED Phase: Running failing tests to verify test correctness"
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose || true
    DEPENDS eti_analyzer_unit_tests eti_analyzer_gui_tests
    COMMENT "TDD RED: Expecting test failures (this is correct!)"
)
add_custom_target(tdd_green_phase
    COMMAND echo "🟢 TDD GREEN Phase: Running tests expecting all to pass"
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS eti_analyzer_unit_tests eti_analyzer_gui_tests
    COMMENT "TDD GREEN: All tests must pass"
)
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS eti_analyzer_unit_tests eti_analyzer_gui_tests
    COMMENT "Running all TDD test suites"
)
add_custom_target(run_tests_quiet
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS eti_analyzer_unit_tests eti_analyzer_gui_tests
    COMMENT "Running all tests (quiet mode)"
)
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
# TDD FRAMEWORK: Performance benchmarking (optional enhancement)
option(ENABLE_TDD_BENCHMARKS "Enable TDD performance benchmark tests" OFF)
if(ENABLE_TDD_BENCHMARKS)
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
            COMMENT "TDD: Running performance benchmarks"
        )
        message(STATUS "TDD: Performance benchmarking enabled")
    else()
        message(STATUS "TDD: Google Benchmark not found, benchmarking disabled")
    endif()
endif()
# TDD FRAMEWORK STATUS SUMMARY
message(STATUS "")
message(STATUS "=== TDD Framework Configuration Complete ===")
message(STATUS "Google Test Framework: ENABLED")
message(STATUS "Qt Test Framework: ENABLED")
message(STATUS "Google Mock Framework: ENABLED")
message(STATUS "Code Coverage: ${ENABLE_COVERAGE}")
message(STATUS "Minimum Coverage: ${COVERAGE_MINIMUM_PERCENTAGE}%")
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
message(STATUS "  - run_tests_quiet: Minimal output testing")
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
