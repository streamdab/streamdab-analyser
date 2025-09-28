cmake_minimum_required(VERSION 3.20)
project(StreamDAB_Phase1_4 VERSION 1.4.0 LANGUAGES CXX)

# Phase 1.4: Advanced FIG Analysis and Real DAB Service Discovery
# Features: Advanced FIC processing, real service discovery, enhanced DAB-XPlorer integration

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 Configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

# Enhanced ETI Processor (from Phase 1.1/1.2/1.3)
set(ETI_PROCESSOR_SOURCES
    ../../src/core/enhanced_eti_processor_qt.cpp
    ../../src/core/enhanced_eti_processor_qt.h
    ../../src/core/eti_types.h
)

# Advanced FIG Analyzer (Phase 1.4)
set(FIG_ANALYZER_SOURCES
    ../../src/core/advanced_fig_analyzer.cpp
    ../../src/core/advanced_fig_analyzer.h
)

# DAB-XPlorer GUI Components (Phase 1.3/1.4 Enhanced)
set(DAB_XPLORER_SOURCES
    ../../src/gui/dab_xplorer_main_window.cpp
    ../../src/gui/dab_xplorer_main_window.h
)

# Main application
set(MAIN_SOURCES
    ../../src/dab_xplorer_main.cpp
)

# Create Phase 1.4 executable
add_executable(StreamDAB_Phase1_4
    ${ETI_PROCESSOR_SOURCES}
    ${FIG_ANALYZER_SOURCES}
    ${DAB_XPLORER_SOURCES}
    ${MAIN_SOURCES}
)

# Include directories
target_include_directories(StreamDAB_Phase1_4 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/gui
)

# Link Qt6 libraries
target_link_libraries(StreamDAB_Phase1_4
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

# Compiler-specific settings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(StreamDAB_Phase1_4 PRIVATE
        -Wall -Wextra -Wpedantic
        -O2
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(StreamDAB_Phase1_4 PRIVATE
        /W4 /O2
    )
endif()

# Phase 1.4 feature definitions
target_compile_definitions(StreamDAB_Phase1_4 PRIVATE
    PHASE_1_4_ADVANCED_FIG=1
    ENHANCED_ETI_PROCESSOR=1
    BANGKOK_ETI_SUPPORT=1
    REAL_SERVICE_DISCOVERY=1
    QT_DISABLE_DEPRECATED_BEFORE=0x060000
)

# Installation (optional)
install(TARGETS StreamDAB_Phase1_4
    RUNTIME DESTINATION bin
)

# Print build information
message(STATUS "StreamDAB Phase 1.4: Advanced FIG Analysis & Real Service Discovery")
message(STATUS "Qt6 Version: ${Qt6_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Features: Advanced FIG analysis, real service discovery, enhanced DAB-XPlorer")
message(STATUS "FIG Support: 0/0, 0/1, 0/2, 1/0, 1/1, 1/4 with real-time filtering")
message(STATUS "ETI Support: Standard ETI-NI + Bangkok ETI-LI formats + FIC extraction")
