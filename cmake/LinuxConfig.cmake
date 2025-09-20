# Linux-specific build configuration
# For ETI Stream Analyzer on Linux platforms

message(STATUS "Configuring for Linux platform")

# Linux-specific dependencies
find_package(Threads REQUIRED)
pkg_check_modules(ALSA alsa)

# Optional ALSA support
if(ALSA_FOUND)
    add_compile_definitions(HAVE_ALSA)
    message(STATUS "ALSA support enabled")
else()
    message(STATUS "ALSA not found - audio output will use default Qt audio")
endif()

# Linux-specific compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# Linux-specific libraries to link
function(add_linux_libraries target)
    target_link_libraries(${target} Threads::Threads)

    if(ALSA_FOUND)
        target_link_libraries(${target} ${ALSA_LIBRARIES})
        target_include_directories(${target} PRIVATE ${ALSA_INCLUDE_DIRS})
    endif()
endfunction()

# Linux packaging configuration
set(CPACK_GENERATOR "DEB;RPM;TGZ")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "ETI Stream Analyzer Team")
set(CPACK_DEBIAN_PACKAGE_SECTION "electronics")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6core6, libqt6widgets6, libqt6gui6, libfdk-aac2 (>= 2.0.0) | libfaad2 (>= 2.9.2), libfftw3-3")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Engineering")
set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase, (fdk-aac >= 2.0.0 or faad2-libs >= 2.9.2), fftw")
