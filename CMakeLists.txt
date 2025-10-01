cmake_minimum_required(VERSION 3.16)
project(DABAnalyzer_Enhanced)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 setup with automatic MOC
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Test Concurrent)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Enhanced build flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -flto -ffast-math -funroll-loops -finline-functions -DRELEASE_BUILD")
set(CMAKE_BUILD_TYPE Release)

# Source files
set(SOURCES
    src/main.cpp
    src/gui/dab_analyzer_main_window.cpp
    src/core/enhanced_eti_processor_qt.cpp
    src/core/advanced_fig_analyzer.cpp
    src/core/etsi_compliance_engine.cpp
    src/core/multi_stream_processor.cpp
    src/core/advanced_error_detection.cpp
    src/core/professional_logging_system.cpp
    src/utils/logger.cpp
)

# Include directories
include_directories(src)

# Create executable
add_executable(DABAnalyzer_Enhanced_FilteringBuild ${SOURCES})

# Link Qt6 libraries
target_link_libraries(DABAnalyzer_Enhanced_FilteringBuild
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Test
    Qt6::Concurrent
    pthread
)
