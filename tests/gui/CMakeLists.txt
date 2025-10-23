# GUI Tests CMakeLists.txt
# Automated GUI testing for DABAnalyzerWindow

message(STATUS "Configuring GUI tests...")

# Qt6 requirements for GUI testing
find_package(Qt6 REQUIRED COMPONENTS Test Widgets Core Gui Network)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/utils
    ${CMAKE_SOURCE_DIR}/src/network
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/..  # For tdd_framework.h
)

# Core source files needed by GUI tests
set(GUI_TEST_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/core/enhanced_eti_processor_qt.cpp
    ${CMAKE_SOURCE_DIR}/src/core/advanced_fig_analyzer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/etsi_compliance_engine.cpp
    ${CMAKE_SOURCE_DIR}/src/core/multi_stream_processor.cpp
    ${CMAKE_SOURCE_DIR}/src/core/advanced_error_detection.cpp
    ${CMAKE_SOURCE_DIR}/src/core/professional_logging_system.cpp
    ${CMAKE_SOURCE_DIR}/src/core/hex_viewer_formatter.cpp
    ${CMAKE_SOURCE_DIR}/src/core/dabplus_stream_validator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/mot_protocol.cpp
    ${CMAKE_SOURCE_DIR}/src/core/epg_decoder.cpp
    ${CMAKE_SOURCE_DIR}/src/core/journaline_decoder.cpp
    ${CMAKE_SOURCE_DIR}/src/core/dls_plus_decoder.cpp
    ${CMAKE_SOURCE_DIR}/src/core/phase3a_integration.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/logger.cpp
    ${CMAKE_SOURCE_DIR}/src/network/edi_packet_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/network/pft_reassembler.cpp
)

# Common link libraries for all GUI tests
set(GUI_TEST_LIBRARIES
    Qt6::Test
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    pthread
)

# ============================================================================
# GUI TEST 1: Framework Basic Tests
# ============================================================================
# This test validates Qt GUI infrastructure without requiring full app compilation
add_executable(test_gui_framework_basic
    test_gui_framework_basic.cpp
)
target_link_libraries(test_gui_framework_basic ${GUI_TEST_LIBRARIES})
set_target_properties(test_gui_framework_basic PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/gui"
)
add_test(
    NAME GUI_Framework_Basic
    COMMAND test_gui_framework_basic
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(GUI_Framework_Basic PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 30
    LABELS "GUI;Basic;P0"
)

message(STATUS "  ✓ test_gui_framework_basic configured (Qt infrastructure validation)")

# ============================================================================
# Additional GUI tests can be added here following the same pattern
# ============================================================================

# Note: Starting with basic framework test to validate Qt GUI infrastructure
# Full DABAnalyzerWindow testing requires integration test approach (see documentation)

message(STATUS "GUI tests configured: 1 test enabled (test_gui_framework_basic)")
message(STATUS "Run with: cd build && ctest -R GUI_Framework_Basic -V")
