cmake_minimum_required(VERSION 3.21)
project(Phase1_4_QtMOC_Test VERSION 1.4.0 LANGUAGES CXX)

# ========================================
# Phase 1.4 Qt MOC Test - Minimal Build
# ========================================
message(STATUS "========================================")
message(STATUS "Phase 1.4 Qt MOC Test - Bangkok ETI Processing")
message(STATUS "========================================")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt6 components for professional application
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network Test)
qt6_standard_project_setup()

message(STATUS "Qt6 Version: ${Qt6_VERSION}")

# ========================================
# PHASE 1.4 EXECUTABLE - STANDALONE TEST
# ========================================

add_executable(Phase1_4_Test
    phase1_4_test_executable.cpp
    src/core/enhanced_eti_processor_qt.cpp
    src/core/advanced_fig_analyzer.cpp
)

# ========================================
# PHASE 1.4 BANGKOK ETI VALIDATION TEST
# ========================================

add_executable(Bangkok_ETI_Test
    test_phase1_4_bangkok.cpp
    src/core/enhanced_eti_processor_qt.cpp
    src/core/advanced_fig_analyzer.cpp
)

# Include directories
target_include_directories(Phase1_4_Test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

target_include_directories(Bangkok_ETI_Test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

# Link Qt6 libraries
target_link_libraries(Phase1_4_Test
    Qt6::Core
    Qt6::Widgets
)

target_link_libraries(Bangkok_ETI_Test
    Qt6::Core
)

# Build settings
target_compile_definitions(Phase1_4_Test PRIVATE
    QT_DEPRECATED_WARNINGS
    QT_DISABLE_DEPRECATED_BEFORE=0x060400
    PHASE_1_4_TEST=1
)

# Compiler settings
if(MSVC)
    target_compile_options(Phase1_4_Test PRIVATE /W3)
else()
    target_compile_options(Phase1_4_Test PRIVATE -Wall)
endif()

message(STATUS "Phase 1.4 Qt MOC Test Configuration Complete")
message(STATUS "Target: Phase1_4_Test")
message(STATUS "Testing: Enhanced ETI Processor Qt + Advanced FIG Analyzer")
message(STATUS "=========================================")

# =========================================
# PHASE 1.4 COMPLETE DAB-ANALYZER APPLICATION
# =========================================
add_executable(Phase14_DAB_Analyzer_Complete
    phase1_4_dab_analyzer_complete.cpp
    src/core/enhanced_eti_processor_qt.cpp
    src/core/advanced_fig_analyzer.cpp
    src/core/eti_types.cpp
)

# Qt6 libraries
target_link_libraries(Phase14_DAB_Analyzer_Complete
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
)

# Include directories
target_include_directories(Phase14_DAB_Analyzer_Complete PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

# Build settings
set_target_properties(Phase14_DAB_Analyzer_Complete PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    AUTOMOC ON
)

# Compiler definitions
target_compile_definitions(Phase14_DAB_Analyzer_Complete PRIVATE
    QT_DEPRECATED_WARNINGS
    QT_DISABLE_DEPRECATED_BEFORE=0x060400
    PHASE_1_4_COMPLETE=1
)

message(STATUS "Phase 1.4 Complete DAB-Analyzer Configuration")
message(STATUS "Target: Phase14_DAB_Analyzer_Complete")
message(STATUS "Features: 4-Panel GUI + Bangkok ETI Processing")
message(STATUS "=========================================")

add_executable(eti_validation_test
    eti_test_validation.cpp
    src/core/enhanced_eti_processor_qt.cpp
)

target_include_directories(eti_validation_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

target_link_libraries(eti_validation_test
    Qt6::Core
)

add_executable(test_phase14_automation
    test_phase14_automation.cpp
    src/core/enhanced_eti_processor_qt.cpp
    src/core/advanced_fig_analyzer.cpp
)

target_include_directories(test_phase14_automation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

target_link_libraries(test_phase14_automation
    Qt6::Core
    Qt6::Widgets
)

add_executable(test_accurate_performance
    test_accurate_performance.cpp
    src/core/enhanced_eti_processor_qt.cpp
)

target_include_directories(test_accurate_performance PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

target_link_libraries(test_accurate_performance
    Qt6::Core
    Qt6::Widgets
)

# =========================================
# MAIN PROFESSIONAL STREAMDAB APPLICATION
# =========================================
add_executable(StreamDABAnalyzer
    src/main.cpp
    src/gui/main_window.cpp
    src/core/headless_eti_processor.cpp
    src/utils/config_manager.cpp
    src/utils/logger.cpp
    src/core/enhanced_eti_processor_qt.cpp
    src/core/advanced_fig_analyzer.cpp
    src/core/eti_types.cpp
)

# Qt6 libraries with UI support
target_link_libraries(StreamDABAnalyzer
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Network
)

# Include directories
target_include_directories(StreamDABAnalyzer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
)

# Build settings for professional application
set_target_properties(StreamDABAnalyzer PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# Set UIC search path for main_window.ui
set_property(TARGET StreamDABAnalyzer PROPERTY AUTOUIC_SEARCH_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/ui
)

# Compiler definitions
target_compile_definitions(StreamDABAnalyzer PRIVATE
    QT_DEPRECATED_WARNINGS
    QT_DISABLE_DEPRECATED_BEFORE=0x060400
    STREAMDAB_PROFESSIONAL=1
)

message(STATUS "Professional StreamDAB Analyzer Configuration")
message(STATUS "Target: StreamDABAnalyzer")
message(STATUS "Features: Three-Panel Professional Layout + Full ETI Processing")
message(STATUS "===========================================")

# =========================================
# ENHANCED FIC ANALYSIS HUB TEST
# =========================================
message(STATUS "Enhanced FIC Analysis Hub Test Configuration")
message(STATUS "Testing: Option C Hybrid Implementation")
message(STATUS "===========================================")

add_executable(Test_Enhanced_FIC_Analysis_Hub
    test_enhanced_fic_analysis_hub.cpp
)

target_link_libraries(Test_Enhanced_FIC_Analysis_Hub
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Test
    Threads::Threads
)

target_include_directories(Test_Enhanced_FIC_Analysis_Hub PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(Test_Enhanced_FIC_Analysis_Hub PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    AUTOMOC ON
)

message(STATUS "Target: Test_Enhanced_FIC_Analysis_Hub")
message(STATUS "Testing: Professional 5-Tab Interface + Bangkok ETI Processing")
message(STATUS "==========================================")
