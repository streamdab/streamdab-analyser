# Windows-specific build configuration
# For ETI Stream Analyzer on Windows platforms

message(STATUS "Configuring for Windows platform")

# Windows-specific dependencies
find_library(WINMM_LIBRARY winmm)
find_library(WSOCK32_LIBRARY wsock32)
find_library(WS2_32_LIBRARY ws2_32)

# Verify Windows libraries are found
if(NOT WINMM_LIBRARY)
    message(FATAL_ERROR "Windows Multimedia API (winmm.lib) not found")
endif()

if(NOT WSOCK32_LIBRARY)
    message(FATAL_ERROR "Windows Socket API (wsock32.lib) not found")
endif()

if(NOT WS2_32_LIBRARY)
    message(FATAL_ERROR "Windows Socket 2 API (ws2_32.lib) not found")
endif()

# Windows-specific compiler flags
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")

    # Disable specific MSVC warnings
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(_WINSOCK_DEPRECATED_NO_WARNINGS)
elseif(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

# Windows-specific libraries to link
function(add_windows_libraries target)
    target_link_libraries(${target}
        ${WINMM_LIBRARY}
        ${WSOCK32_LIBRARY}
        ${WS2_32_LIBRARY}
    )
endfunction()

# Windows packaging configuration
set(CPACK_GENERATOR "NSIS;ZIP")
set(CPACK_NSIS_DISPLAY_NAME "ETI Stream Analyzer")
set(CPACK_NSIS_PACKAGE_NAME "ETI Stream Analyzer")
set(CPACK_NSIS_CONTACT "support@eti-analyzer.com")
set(CPACK_NSIS_HELP_LINK "https://github.com/eti-analyzer/desktop")
set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/eti-analyzer/desktop")
set(CPACK_NSIS_MODIFY_PATH ON)

# Windows-specific install configuration
set(CMAKE_INSTALL_BINDIR bin)
set(CMAKE_INSTALL_LIBDIR lib)
set(CMAKE_INSTALL_INCLUDEDIR include)
