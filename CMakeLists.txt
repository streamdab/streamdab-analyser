cmake_minimum_required(VERSION 3.21)

# Project definition
project(ETIStreamAnalyzer
    VERSION 1.0.0
    DESCRIPTION "ETI Stream Analyzer - Professional DAB/DAB+ Analysis Tool"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Add cmake modules path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# Platform-specific settings
if(WIN32)
    set(CMAKE_WIN32_EXECUTABLE ON)
    # Enable static linking for Windows distribution
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "macOS deployment target")
    set(CMAKE_MACOSX_BUNDLE ON)
endif()

# Compiler-specific settings
if(MSVC)
    add_compile_options(/W4 /permissive-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Debug/Release specific settings
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG_BUILD)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-g -O0)
    endif()
else()
    add_compile_definitions(NDEBUG)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-O3)
    endif()
endif()

# Find Qt6 - Core modules required
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    Network
)

# Qt6 automatic tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find required dependencies
find_package(PkgConfig REQUIRED)

# libfaad2 for AAC decoding
pkg_check_modules(FAAD2 REQUIRED faad2)

# libfftw3 for FFT operations
pkg_check_modules(FFTW3 REQUIRED fftw3)

# yaml-cpp for configuration management
find_package(yaml-cpp REQUIRED)

# Platform-specific dependency handling
if(UNIX AND NOT APPLE)
    # Linux specific dependencies
    find_package(Threads REQUIRED)
    pkg_check_modules(ALSA alsa)
    if(ALSA_FOUND)
        add_compile_definitions(HAVE_ALSA)
    endif()
elseif(WIN32)
    # Windows specific dependencies
    find_library(WINMM_LIBRARY winmm)
    find_library(WSOCK32_LIBRARY wsock32)
    find_library(WS2_32_LIBRARY ws2_32)
elseif(APPLE)
    # macOS specific dependencies
    find_library(COREAUDIO_FRAMEWORK CoreAudio)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
    find_library(AUDIOUNIT_FRAMEWORK AudioUnit)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# Source files organization
set(CORE_SOURCES
    src/core/eti_processor.cpp
    src/core/dab_decoder.cpp
    src/core/fic_decoder.cpp
    src/core/ensemble_manager.cpp
    src/core/service_manager.cpp
    src/core/audio_decoder.cpp
)

set(CORE_HEADERS
    src/core/eti_processor.h
    src/core/dab_decoder.h
    src/core/fic_decoder.h
    src/core/ensemble_manager.h
    src/core/service_manager.h
    src/core/audio_decoder.h
    src/core/eti_types.h
)

set(GUI_SOURCES
    src/gui/main_window.cpp
    src/gui/analyzer_widget.cpp
    src/gui/constellation_widget.cpp
    src/gui/service_browser.cpp
    src/gui/settings_dialog.cpp
    src/gui/about_dialog.cpp
)

set(GUI_HEADERS
    src/gui/main_window.h
    src/gui/analyzer_widget.h
    src/gui/constellation_widget.h
    src/gui/service_browser.h
    src/gui/settings_dialog.h
    src/gui/about_dialog.h
)

set(UTILS_SOURCES
    src/utils/config_manager.cpp
    src/utils/file_utils.cpp
    src/utils/logger.cpp
    src/utils/signal_processing.cpp
)

set(UTILS_HEADERS
    src/utils/config_manager.h
    src/utils/file_utils.h
    src/utils/logger.h
    src/utils/signal_processing.h
)

# Qt UI files
set(UI_FILES
    src/gui/main_window.ui
    src/gui/settings_dialog.ui
    src/gui/about_dialog.ui
)

# Qt Resource files
set(RESOURCE_FILES
    resources/application.qrc
)

# Create the main executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    ${CORE_SOURCES}
    ${CORE_HEADERS}
    ${GUI_SOURCES}
    ${GUI_HEADERS}
    ${UTILS_SOURCES}
    ${UTILS_HEADERS}
    ${UI_FILES}
    ${RESOURCE_FILES}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Network
    ${FAAD2_LIBRARIES}
    ${FFTW3_LIBRARIES}
    yaml-cpp
)

# Platform-specific linking
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} Threads::Threads)
    if(ALSA_FOUND)
        target_link_libraries(${PROJECT_NAME} ${ALSA_LIBRARIES})
    endif()
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME}
        ${WINMM_LIBRARY}
        ${WSOCK32_LIBRARY}
        ${WS2_32_LIBRARY}
    )
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME}
        ${COREAUDIO_FRAMEWORK}
        ${AUDIOTOOLBOX_FRAMEWORK}
        ${AUDIOUNIT_FRAMEWORK}
    )
endif()

# Include directories for dependencies
target_include_directories(${PROJECT_NAME} PRIVATE
    ${FAAD2_INCLUDE_DIRS}
    ${FFTW3_INCLUDE_DIRS}
)

# Compile definitions for dependencies
target_compile_definitions(${PROJECT_NAME} PRIVATE
    ${FAAD2_CFLAGS_OTHER}
    ${FFTW3_CFLAGS_OTHER}
)

# TDD FRAMEWORK INTEGRATION - MANDATORY FOR BUILD SYSTEM
message(STATUS "=== TDD-Integrated Build System ===")
enable_testing()

# TDD REQUIREMENT: Google Test framework integration
find_package(GTest QUIET)
find_package(GMock QUIET)

# TDD OPTION: Allow disabling tests for release builds only
option(BUILD_TESTING "Build TDD test suite" ON)
option(ENABLE_COVERAGE "Enable TDD code coverage reporting" ON)

if(BUILD_TESTING)
    message(STATUS "TDD: Test framework integration enabled")

    # TDD MANDATE: If GTest not found, fetch it
    if(NOT GTest_FOUND)
        message(STATUS "TDD: Google Test not found, fetching...")
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG release-1.12.1
        )
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)

        # Create aliases for consistent naming
        if(NOT TARGET GTest::GTest)
            add_library(GTest::GTest ALIAS gtest)
            add_library(GTest::Main ALIAS gtest_main)
        endif()
        if(NOT TARGET GMock::GMock)
            add_library(GMock::GMock ALIAS gmock)
            add_library(GMock::Main ALIAS gmock_main)
        endif()

        message(STATUS "TDD: Google Test fetched and configured")
    endif()

    # TDD COVERAGE: Enable coverage for Debug builds
    if(ENABLE_COVERAGE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
            add_link_options(--coverage)
            message(STATUS "TDD: Code coverage enabled for Debug build")
        endif()
    endif()

    add_subdirectory(tests)
else()
    message(STATUS "TDD: Testing disabled (BUILD_TESTING=OFF)")
endif()

# TDD BUILD TARGETS: Red-Green-Refactor workflow integration
if(BUILD_TESTING)
    # TDD Cycle Targets
    add_custom_target(tdd-red
        COMMAND echo "🔴 TDD RED Phase: Running tests (expecting failures)"
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose || true
        COMMENT "TDD RED: Run failing tests to verify test correctness"
    )

    add_custom_target(tdd-green
        COMMAND echo "🟢 TDD GREEN Phase: Running tests (expecting success)"
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
        COMMENT "TDD GREEN: All tests must pass"
    )

    add_custom_target(tdd-refactor
        COMMAND echo "🔵 TDD REFACTOR Phase: Code optimization while maintaining tests"
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        COMMENT "TDD REFACTOR: Optimize code while keeping tests green"
    )

    add_custom_target(tdd-cycle
        DEPENDS tdd-red tdd-green tdd-refactor
        COMMENT "Complete TDD Red-Green-Refactor cycle"
    )

    # Standard test targets
    add_custom_target(run_all_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
        COMMENT "Run all tests"
    )

    # Coverage targets (when enabled)
    if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        find_program(LCOV_EXECUTABLE lcov)
        find_program(GENHTML_EXECUTABLE genhtml)

        if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
            add_custom_target(coverage
                COMMAND ${LCOV_EXECUTABLE} --directory . --capture --output-file coverage.info
                COMMAND ${LCOV_EXECUTABLE} --remove coverage.info '/usr/*' '*/tests/*' '*/third_party/*' --output-file coverage_clean.info
                COMMAND ${GENHTML_EXECUTABLE} coverage_clean.info --output-directory coverage_html
                DEPENDS test
                COMMENT "Generate code coverage report"
            )

            add_custom_target(coverage-check
                COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_coverage.sh
                DEPENDS coverage
                COMMENT "Enforce 80% coverage requirement"
            )

            add_custom_target(quality-gate
                DEPENDS tdd-green coverage-check
                COMMENT "TDD quality gate: tests pass + coverage check"
            )

            message(STATUS "TDD: Coverage targets available (coverage, coverage-check, quality-gate)")
        endif()
    endif()

    message(STATUS "TDD: Build targets available (tdd-red, tdd-green, tdd-refactor, tdd-cycle)")
endif()

# Installation configuration
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

# Platform-specific installation
if(WIN32)
    # Install Qt libraries and dependencies
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_WINDEPLOYQT_EXECUTABLE ${QT_QMAKE_EXECUTABLE} PATH)
    set(QT_WINDEPLOYQT_EXECUTABLE "${QT_WINDEPLOYQT_EXECUTABLE}/windeployqt.exe")

    install(CODE "
        execute_process(COMMAND ${QT_WINDEPLOYQT_EXECUTABLE} \${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME}.exe)
    ")
elseif(APPLE)
    # macOS bundle configuration
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "ETI Stream Analyzer"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_COPYRIGHT "Copyright © 2024 ETI Stream Analyzer Team"
        MACOSX_BUNDLE_IDENTIFIER "com.etianalyzer.desktop"
    )

    # Install Qt libraries
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_MACDEPLOYQT_EXECUTABLE ${QT_QMAKE_EXECUTABLE} PATH)
    set(QT_MACDEPLOYQT_EXECUTABLE "${QT_MACDEPLOYQT_EXECUTABLE}/macdeployqt")

    install(CODE "
        execute_process(COMMAND ${QT_MACDEPLOYQT_EXECUTABLE} \${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app)
    ")
endif()

# CPack configuration for packaging
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "ETI Stream Analyzer")
set(CPACK_PACKAGE_VENDOR "ETI Stream Analyzer Team")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_PACKAGE_NAME "ETI Stream Analyzer")
    set(CPACK_NSIS_DISPLAY_NAME "ETI Stream Analyzer")
    set(CPACK_NSIS_HELP_LINK "https://github.com/your-org/eti-stream-analyzer")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/your-org/eti-stream-analyzer")
    set(CPACK_NSIS_CONTACT "support@etianalyzer.com")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_FORMAT "UDZO")
    set(CPACK_DMG_VOLUME_NAME "ETI Stream Analyzer")
elseif(UNIX)
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "ETI Stream Analyzer Team")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-base-dev, libfaad-dev, libfftw3-dev, libyaml-cpp-dev")
endif()

include(CPack)

# Print configuration summary
message(STATUS "=== ETI Stream Analyzer Build Configuration ===")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Qt Version: ${Qt6_VERSION}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Architecture: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "===============================================")

# Include professional cross-platform packaging configuration
include(cmake/packaging.cmake)
