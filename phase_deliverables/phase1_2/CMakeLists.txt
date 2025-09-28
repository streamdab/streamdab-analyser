cmake_minimum_required(VERSION 3.20)

project(StreamDAB_Phase1_2 VERSION 1.2.0)

# C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 Configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Qt6 version check
if(Qt6_VERSION VERSION_LESS "6.2.0")
    message(FATAL_ERROR "Qt6 version 6.2.0 or higher required")
endif()

# Phase 1.2 Source Files
set(PHASE1_2_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    enhanced_eti_processor_qt.cpp
    enhanced_eti_processor_qt.h
)

# Phase 1.2 Executable
add_executable(StreamDAB_Phase1_2 ${PHASE1_2_SOURCES})

# Qt6 Linking
target_link_libraries(StreamDAB_Phase1_2
    Qt6::Core
    Qt6::Widgets
)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(StreamDAB_Phase1_2 PRIVATE
        -Wall -Wextra -O2
    )
endif()

# Platform-specific configuration
if(WIN32)
    set_target_properties(StreamDAB_Phase1_2 PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Install configuration
install(TARGETS StreamDAB_Phase1_2
    RUNTIME DESTINATION bin
)

# Phase 1.2 Information
message(STATUS "========================================")
message(STATUS "StreamDAB Analyzer - Phase 1.2 Qt GUI Integration")
message(STATUS "========================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Qt6 Version: ${Qt6_VERSION}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "========================================")
message(STATUS "Phase 1.2 Features:")
message(STATUS "✅ Qt6 GUI Framework Integration")
message(STATUS "✅ ETI-LI Bangkok File Support")
message(STATUS "✅ Real-time Processing Display")
message(STATUS "✅ Professional Broadcast UI")
message(STATUS "========================================")
