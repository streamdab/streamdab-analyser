cmake_minimum_required(VERSION 3.20)
project(StreamDAB_Phase1_3 VERSION 1.3.0 LANGUAGES CXX)

# Phase 1.3: DAB  Analyzer Professional Interface Implementation
# Features: 4-panel layout, frame navigation, service selection, FIC analysis

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 Configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

# Enhanced ETI Processor (from Phase 1.1/1.2)
set(ETI_PROCESSOR_SOURCES
    ../../src/core/enhanced_eti_processor_qt.cpp
    ../../src/core/enhanced_eti_processor_qt.h
)

# DAB  Analyzer GUI Components (Phase 1.3)
set(DAB_XPLORER_SOURCES
    ../../src/gui/dab_xplorer_main_window.cpp
    ../../src/gui/dab_xplorer_main_window.h
)

# Main application
set(MAIN_SOURCES
    ../../src/dab_xplorer_main.cpp
)

# Create Phase 1.3 executable
add_executable(StreamDAB_Phase1_3
    ${ETI_PROCESSOR_SOURCES}
    ${DAB_XPLORER_SOURCES}
    ${MAIN_SOURCES}
)

# Include directories
target_include_directories(StreamDAB_Phase1_3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/gui
)

# Link Qt6 libraries
target_link_libraries(StreamDAB_Phase1_3
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

# Compiler-specific settings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(StreamDAB_Phase1_3 PRIVATE
        -Wall -Wextra -Wpedantic
        -O2
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(StreamDAB_Phase1_3 PRIVATE
        /W4 /O2
    )
endif()

# Phase 1.3 feature definitions
target_compile_definitions(StreamDAB_Phase1_3 PRIVATE
    PHASE_1_3_DAB_XPLORER=1
    ENHANCED_ETI_PROCESSOR=1
    BANGKOK_ETI_SUPPORT=1
    QT_DISABLE_DEPRECATED_BEFORE=0x060000
)

# Installation (optional)
install(TARGETS StreamDAB_Phase1_3
    RUNTIME DESTINATION bin
)

# Print build information
message(STATUS "StreamDAB Phase 1.3: DAB Analyzer Professional Interface")
message(STATUS "Qt6 Version: ${Qt6_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Features: 4-panel layout, frame navigation, service selection, FIC analysis")
message(STATUS "ETI Support: Standard ETI-NI + Bangkok ETI-LI formats")
