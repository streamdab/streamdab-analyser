#!/bin/bash
# Build verification script for ETI Stream Analyzer
# Checks project structure, dependencies, and configuration without compiling

set -e

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "${SCRIPT_DIR}")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print functions
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[⚠]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

print_section() {
    echo
    echo -e "${BLUE}=== $1 ===${NC}"
}

# Verification counters
CHECKS_PASSED=0
CHECKS_FAILED=0
CHECKS_WARNING=0

check_result() {
    if [ $1 -eq 0 ]; then
        print_success "$2"
        ((CHECKS_PASSED++))
    else
        if [ "$3" = "warning" ]; then
            print_warning "$2"
            ((CHECKS_WARNING++))
        else
            print_error "$2"
            ((CHECKS_FAILED++))
        fi
    fi
}

# Check project structure
check_project_structure() {
    print_section "Project Structure Verification"

    # Check required directories
    local required_dirs=(
        "src"
        "src/core"
        "src/gui"
        "src/utils"
        "resources"
        "third_party"
        "tests"
        "tests/core"
        "tests/gui"
        "tests/utils"
        "cmake"
        "cmake/modules"
        "scripts"
        "build"
    )

    for dir in "${required_dirs[@]}"; do
        if [ -d "${PROJECT_ROOT}/${dir}" ]; then
            check_result 0 "Directory exists: ${dir}"
        else
            check_result 1 "Missing directory: ${dir}"
        fi
    done

    # Check required files
    local required_files=(
        "CMakeLists.txt"
        "src/main.cpp"
        "src/core/eti_types.h"
        "cmake/modules/FindFAAD2.cmake"
        "cmake/modules/FindFFTW3.cmake"
        "resources/application.qrc"
        "tests/CMakeLists.txt"
        "tests/test_main.cpp"
        "scripts/build.sh"
        "scripts/build.bat"
    )

    for file in "${required_files[@]}"; do
        if [ -f "${PROJECT_ROOT}/${file}" ]; then
            check_result 0 "File exists: ${file}"
        else
            check_result 1 "Missing file: ${file}"
        fi
    done
}

# Check CMake configuration
check_cmake_config() {
    print_section "CMake Configuration Verification"

    # Check CMake availability
    if command -v cmake &> /dev/null; then
        CMAKE_VERSION=$(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
        check_result 0 "CMake available: ${CMAKE_VERSION}"

        # Check minimum version (3.21 required)
        if [ "$(printf '%s\n' "3.21" "${CMAKE_VERSION}" | sort -V | head -n1)" = "3.21" ]; then
            check_result 0 "CMake version meets minimum requirement (3.21+)"
        else
            check_result 1 "CMake version too old (need 3.21+, have ${CMAKE_VERSION})"
        fi
    else
        check_result 1 "CMake not found in PATH"
    fi

    # Check CMakeLists.txt syntax
    if [ -f "${PROJECT_ROOT}/CMakeLists.txt" ]; then
        if grep -q "cmake_minimum_required.*3\.21" "${PROJECT_ROOT}/CMakeLists.txt"; then
            check_result 0 "CMake minimum version specified correctly"
        else
            check_result 1 "CMake minimum version not specified or incorrect"
        fi

        if grep -q "find_package.*Qt6.*REQUIRED" "${PROJECT_ROOT}/CMakeLists.txt"; then
            check_result 0 "Qt6 dependency specified"
        else
            check_result 1 "Qt6 dependency not found in CMakeLists.txt"
        fi

        if grep -q "CMAKE_CXX_STANDARD.*17" "${PROJECT_ROOT}/CMakeLists.txt"; then
            check_result 0 "C++17 standard specified"
        else
            check_result 1 "C++17 standard not specified"
        fi
    fi
}

# Check Qt6 availability
check_qt_availability() {
    print_section "Qt6 Dependency Verification"

    # Check for qmake
    if command -v qmake6 &> /dev/null; then
        QT_VERSION=$(qmake6 -query QT_VERSION)
        check_result 0 "Qt6 qmake found: ${QT_VERSION}"
    elif command -v qmake &> /dev/null; then
        QT_VERSION=$(qmake -query QT_VERSION)
        if [[ "${QT_VERSION}" == 6.* ]]; then
            check_result 0 "Qt6 qmake found: ${QT_VERSION}"
        else
            check_result 1 "qmake found but wrong version: ${QT_VERSION} (need Qt6)" "warning"
        fi
    else
        check_result 1 "Qt6 qmake not found" "warning"
        print_info "Install Qt6 with: brew install qt@6 (macOS) or apt-get install qt6-base-dev (Ubuntu)"
    fi

    # Check for Qt6 CMake files
    local qt_cmake_paths=(
        "/usr/local/lib/cmake/Qt6"
        "/opt/homebrew/lib/cmake/Qt6"
        "/usr/lib/x86_64-linux-gnu/cmake/Qt6"
        "/usr/lib/cmake/Qt6"
    )

    local qt_found=false
    for path in "${qt_cmake_paths[@]}"; do
        if [ -d "${path}" ]; then
            check_result 0 "Qt6 CMake files found: ${path}"
            qt_found=true
            break
        fi
    done

    if [ "${qt_found}" = false ]; then
        check_result 1 "Qt6 CMake files not found in standard locations" "warning"
    fi
}

# Check system dependencies
check_system_dependencies() {
    print_section "System Dependencies Verification"

    # Detect platform
    case "$(uname -s)" in
        Linux*)
            PLATFORM="Linux"
            check_result 0 "Platform detected: Linux"

            # Check pkg-config
            if command -v pkg-config &> /dev/null; then
                check_result 0 "pkg-config available"

                # Check for required libraries
                if pkg-config --exists faad2; then
                    FAAD_VERSION=$(pkg-config --modversion faad2)
                    check_result 0 "libfaad2 found: ${FAAD_VERSION}"
                else
                    check_result 1 "libfaad2 not found (install libfaad-dev)" "warning"
                fi

                if pkg-config --exists fftw3; then
                    FFTW_VERSION=$(pkg-config --modversion fftw3)
                    check_result 0 "libfftw3 found: ${FFTW_VERSION}"
                else
                    check_result 1 "libfftw3 not found (install libfftw3-dev)" "warning"
                fi

                if pkg-config --exists yaml-cpp; then
                    YAML_VERSION=$(pkg-config --modversion yaml-cpp)
                    check_result 0 "yaml-cpp found: ${YAML_VERSION}"
                else
                    check_result 1 "yaml-cpp not found (install libyaml-cpp-dev)" "warning"
                fi
            else
                check_result 1 "pkg-config not available"
            fi
            ;;
        Darwin*)
            PLATFORM="macOS"
            check_result 0 "Platform detected: macOS"

            # Check Homebrew
            if command -v brew &> /dev/null; then
                check_result 0 "Homebrew available"

                # Check for required libraries
                if brew list faad2 &> /dev/null; then
                    check_result 0 "faad2 installed via Homebrew"
                else
                    check_result 1 "faad2 not installed (brew install faad2)" "warning"
                fi

                if brew list fftw &> /dev/null; then
                    check_result 0 "fftw installed via Homebrew"
                else
                    check_result 1 "fftw not installed (brew install fftw)" "warning"
                fi

                if brew list yaml-cpp &> /dev/null; then
                    check_result 0 "yaml-cpp installed via Homebrew"
                else
                    check_result 1 "yaml-cpp not installed (brew install yaml-cpp)" "warning"
                fi
            else
                check_result 1 "Homebrew not available (recommended for dependency management)" "warning"
            fi
            ;;
        CYGWIN*|MINGW*|MSYS*)
            PLATFORM="Windows"
            check_result 0 "Platform detected: Windows"

            # Check for Visual Studio
            if command -v cl &> /dev/null; then
                check_result 0 "Visual Studio compiler available"
            else
                check_result 1 "Visual Studio compiler not found" "warning"
            fi
            ;;
        *)
            check_result 1 "Unknown platform: $(uname -s)"
            ;;
    esac
}

# Check compiler availability
check_compiler() {
    print_section "Compiler Verification"

    case "${PLATFORM}" in
        Linux|macOS)
            if command -v g++ &> /dev/null; then
                GCC_VERSION=$(g++ --version | head -n1)
                check_result 0 "GCC available: ${GCC_VERSION}"
            else
                check_result 1 "GCC not available" "warning"
            fi

            if command -v clang++ &> /dev/null; then
                CLANG_VERSION=$(clang++ --version | head -n1)
                check_result 0 "Clang available: ${CLANG_VERSION}"
            else
                check_result 1 "Clang not available" "warning"
            fi
            ;;
        Windows)
            if command -v cl &> /dev/null; then
                check_result 0 "MSVC compiler available"
            else
                check_result 1 "MSVC compiler not available" "warning"
            fi
            ;;
    esac
}

# Check source code integrity
check_source_integrity() {
    print_section "Source Code Integrity"

    # Count source files
    CPP_FILES=$(find "${PROJECT_ROOT}/src" -name "*.cpp" 2>/dev/null | wc -l)
    H_FILES=$(find "${PROJECT_ROOT}/src" -name "*.h" 2>/dev/null | wc -l)

    check_result 0 "Source files found: ${CPP_FILES} .cpp, ${H_FILES} .h"

    # Check main.cpp content
    if [ -f "${PROJECT_ROOT}/src/main.cpp" ]; then
        if grep -q "QApplication" "${PROJECT_ROOT}/src/main.cpp"; then
            check_result 0 "main.cpp contains Qt application initialization"
        else
            check_result 1 "main.cpp missing Qt application initialization"
        fi

        if grep -q "#include.*QApplication" "${PROJECT_ROOT}/src/main.cpp"; then
            check_result 0 "main.cpp includes required Qt headers"
        else
            check_result 1 "main.cpp missing Qt headers"
        fi
    fi

    # Check for placeholder implementations
    PLACEHOLDER_COUNT=$(grep -r "Placeholder implementation" "${PROJECT_ROOT}/src" 2>/dev/null | wc -l)
    if [ "${PLACEHOLDER_COUNT}" -gt 0 ]; then
        check_result 0 "Placeholder implementations detected: ${PLACEHOLDER_COUNT} (expected for Phase 1)" "warning"
    fi
}

# Check build scripts
check_build_scripts() {
    print_section "Build Scripts Verification"

    # Check Unix build script
    if [ -f "${PROJECT_ROOT}/scripts/build.sh" ]; then
        if [ -x "${PROJECT_ROOT}/scripts/build.sh" ]; then
            check_result 0 "Unix build script is executable"
        else
            check_result 1 "Unix build script not executable (chmod +x needed)"
        fi

        if grep -q "cmake.*CMAKE_BUILD_TYPE" "${PROJECT_ROOT}/scripts/build.sh"; then
            check_result 0 "Build script configures CMake correctly"
        else
            check_result 1 "Build script missing CMake configuration"
        fi
    fi

    # Check Windows build script
    if [ -f "${PROJECT_ROOT}/scripts/build.bat" ]; then
        check_result 0 "Windows build script exists"

        if grep -q "cmake.*CMAKE_BUILD_TYPE" "${PROJECT_ROOT}/scripts/build.bat"; then
            check_result 0 "Windows build script configures CMake correctly"
        else
            check_result 1 "Windows build script missing CMake configuration"
        fi
    fi
}

# Generate summary report
generate_summary() {
    print_section "Verification Summary"

    local total_checks=$((CHECKS_PASSED + CHECKS_FAILED + CHECKS_WARNING))

    echo "Total checks performed: ${total_checks}"
    print_success "Passed: ${CHECKS_PASSED}"
    if [ "${CHECKS_WARNING}" -gt 0 ]; then
        print_warning "Warnings: ${CHECKS_WARNING}"
    fi
    if [ "${CHECKS_FAILED}" -gt 0 ]; then
        print_error "Failed: ${CHECKS_FAILED}"
    fi

    local success_rate=$(( (CHECKS_PASSED * 100) / total_checks ))
    echo "Success rate: ${success_rate}%"

    echo
    if [ "${CHECKS_FAILED}" -eq 0 ]; then
        print_success "Build configuration verification PASSED"
        print_info "Project is ready for compilation once dependencies are installed"
        echo
        print_info "Next steps:"
        print_info "1. Install Qt6: brew install qt@6 (macOS) or apt-get install qt6-base-dev (Ubuntu)"
        print_info "2. Install system dependencies (faad2, fftw3, yaml-cpp)"
        print_info "3. Run: ./scripts/build.sh"
        return 0
    else
        print_error "Build configuration verification FAILED"
        print_info "Fix the failed checks above before proceeding"
        return 1
    fi
}

# Main execution
main() {
    print_info "ETI Stream Analyzer - Build Configuration Verification"
    print_info "======================================================"

    cd "${PROJECT_ROOT}"

    check_project_structure
    check_cmake_config
    check_qt_availability
    check_system_dependencies
    check_compiler
    check_source_integrity
    check_build_scripts

    generate_summary
}

# Run main function
main "$@"
