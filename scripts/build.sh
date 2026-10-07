#!/bin/bash
# Cross-platform build script for ETI Stream Analyser
# Supports Linux, macOS, and Windows (via Git Bash/MSYS2)

set -e

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "${SCRIPT_DIR}")"
BUILD_DIR="${PROJECT_ROOT}/build"

# Default values
BUILD_TYPE="Release"
CLEAN_BUILD=false
INSTALL=false
PACKAGE=false
# Build parallelism: bounded by CPU *and* RAM (see scripts/build_jobs.sh).
# Override with JOBS=<n> or CMAKE_BUILD_PARALLEL_LEVEL=<n>.
. "${SCRIPT_DIR}/build_jobs.sh"
JOBS=$(streamdab_build_jobs)
export CMAKE_BUILD_PARALLEL_LEVEL="${JOBS}"
RUN_TESTS=false
TDD_MODE=false
COVERAGE=false

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
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Help function
show_help() {
    cat << EOF
ETI Stream Analyser TDD-Integrated Build Script

Usage: $0 [OPTIONS]

OPTIONS:
    -h, --help          Show this help message
    -d, --debug         Build in Debug mode (default: Release)
    -c, --clean         Clean build directory before building
    -i, --install       Install after building
    -p, --package       Create package after building
    -j, --jobs N        Number of parallel jobs (default: auto-detect)
    -t, --test          Run tests after building
    --tdd               Enable TDD mode (Debug + Tests + Coverage)
    --coverage          Enable code coverage reporting
    --qt-dir PATH       Qt installation directory

TDD WORKFLOW:
    $0 --tdd            # Complete TDD workflow (Debug + Tests + Coverage)
    $0 --debug --test   # Debug build with tests
    $0 --coverage       # Build with coverage reporting

EXAMPLES:
    $0                  # Basic release build
    $0 --debug          # Debug build
    $0 --tdd            # Full TDD development build
    $0 --clean --install # Clean build and install
    $0 --package        # Build and create package

EOF
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -i|--install)
            INSTALL=true
            shift
            ;;
        -p|--package)
            PACKAGE=true
            shift
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        -t|--test)
            RUN_TESTS=true
            shift
            ;;
        --tdd)
            TDD_MODE=true
            BUILD_TYPE="Debug"
            RUN_TESTS=true
            COVERAGE=true
            shift
            ;;
        --coverage)
            COVERAGE=true
            BUILD_TYPE="Debug"  # Coverage requires Debug build
            shift
            ;;
        --qt-dir)
            QT_DIR="$2"
            shift 2
            ;;
        *)
            print_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Detect platform
detect_platform() {
    case "$(uname -s)" in
        Linux*)
            PLATFORM="Linux"
            ;;
        Darwin*)
            PLATFORM="macOS"
            ;;
        CYGWIN*|MINGW*|MSYS*)
            PLATFORM="Windows"
            ;;
        *)
            print_error "Unsupported platform: $(uname -s)"
            exit 1
            ;;
    esac
    print_info "Detected platform: ${PLATFORM}"
}

# Check dependencies
check_dependencies() {
    print_info "Checking build dependencies..."

    # Check CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake is not installed or not in PATH"
        exit 1
    fi
    CMAKE_VERSION=$(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
    print_info "CMake version: ${CMAKE_VERSION}"

    # Check compiler
    case "${PLATFORM}" in
        Linux|macOS)
            if command -v g++ &> /dev/null; then
                COMPILER="g++"
                COMPILER_VERSION=$(g++ --version | head -n1)
            elif command -v clang++ &> /dev/null; then
                COMPILER="clang++"
                COMPILER_VERSION=$(clang++ --version | head -n1)
            else
                print_error "No suitable C++ compiler found (g++ or clang++)"
                exit 1
            fi
            ;;
        Windows)
            # For Windows, we'll rely on CMake to find the compiler
            print_info "Using CMake to detect Windows compiler"
            ;;
    esac

    if [[ -n "${COMPILER:-}" ]]; then
        print_info "Compiler: ${COMPILER_VERSION}"
    fi

    # Check Qt6
    if [[ -n "${QT_DIR:-}" ]]; then
        export Qt6_DIR="${QT_DIR}"
        export CMAKE_PREFIX_PATH="${QT_DIR}:${CMAKE_PREFIX_PATH:-}"
        print_info "Using Qt6 from: ${QT_DIR}"
    fi

    # TDD-specific dependency checks
    if [[ "${RUN_TESTS}" == true ]] || [[ "${TDD_MODE}" == true ]]; then
        print_info "Checking TDD framework dependencies..."

        # Check for Google Test (will be fetched by CMake if not found)
        if pkg-config --exists gtest 2>/dev/null; then
            print_info "Google Test found via pkg-config"
        else
            print_info "Google Test will be fetched by CMake"
        fi

        # Check for coverage tools
        if [[ "${COVERAGE}" == true ]]; then
            if ! command -v lcov &> /dev/null; then
                print_warning "lcov not found - coverage reporting may not work"
                case "${PLATFORM}" in
                    Linux)
                        print_info "Install with: sudo apt-get install lcov"
                        ;;
                    macOS)
                        print_info "Install with: brew install lcov"
                        ;;
                esac
            else
                print_info "Coverage tools available: lcov"
            fi
        fi
    fi
}

# Check system dependencies
check_system_dependencies() {
    print_info "Checking system dependencies..."

    case "${PLATFORM}" in
        Linux)
            # Check for required development packages
            MISSING_PACKAGES=""

            if ! pkg-config --exists faad2; then
                MISSING_PACKAGES="${MISSING_PACKAGES} libfaad-dev"
            fi

            if ! pkg-config --exists fftw3; then
                MISSING_PACKAGES="${MISSING_PACKAGES} libfftw3-dev"
            fi

            if ! pkg-config --exists yaml-cpp; then
                MISSING_PACKAGES="${MISSING_PACKAGES} libyaml-cpp-dev"
            fi

            if [[ -n "${MISSING_PACKAGES}" ]]; then
                print_warning "Missing packages detected:${MISSING_PACKAGES}"
                print_info "Install with: sudo apt-get install${MISSING_PACKAGES}"
                read -p "Continue anyway? (y/N): " -n 1 -r
                echo
                if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                    exit 1
                fi
            fi
            ;;
        macOS)
            # Check for Homebrew packages
            if command -v brew &> /dev/null; then
                print_info "Checking Homebrew packages..."
                brew list faad2 &> /dev/null || print_warning "faad2 not found (brew install faad2)"
                brew list fftw &> /dev/null || print_warning "fftw not found (brew install fftw)"
                brew list yaml-cpp &> /dev/null || print_warning "yaml-cpp not found (brew install yaml-cpp)"
            else
                print_warning "Homebrew not found. Dependencies must be installed manually."
            fi
            ;;
        Windows)
            print_info "Windows dependency checking not implemented yet"
            print_warning "Ensure vcpkg or manual installation of dependencies"
            ;;
    esac
}

# Configure build
configure_build() {
    print_info "Configuring build (${BUILD_TYPE})..."

    # Clean if requested
    if [[ "${CLEAN_BUILD}" == true ]]; then
        print_info "Cleaning build directory..."
        rm -rf "${BUILD_DIR}"
    fi

    # Create build directory
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"

    # CMake arguments
    CMAKE_ARGS=(
        "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    )

    # TDD-specific CMake arguments
    if [[ "${RUN_TESTS}" == true ]] || [[ "${TDD_MODE}" == true ]]; then
        CMAKE_ARGS+=("-DBUILD_TESTING=ON")
        print_info "TDD: Testing enabled"
    else
        CMAKE_ARGS+=("-DBUILD_TESTING=OFF")
        print_info "TDD: Testing disabled"
    fi

    if [[ "${COVERAGE}" == true ]]; then
        CMAKE_ARGS+=("-DENABLE_COVERAGE=ON")
        print_info "TDD: Coverage reporting enabled"
    else
        CMAKE_ARGS+=("-DENABLE_COVERAGE=OFF")
    fi

    # Platform-specific CMake arguments
    case "${PLATFORM}" in
        Windows)
            CMAKE_ARGS+=("-A" "x64")
            ;;
        macOS)
            CMAKE_ARGS+=("-DCMAKE_OSX_DEPLOYMENT_TARGET=14.0")
            ;;
    esac

    # Run CMake configure
    print_info "Running CMake configure..."
    cmake "${CMAKE_ARGS[@]}" "${PROJECT_ROOT}"

    print_success "Configuration complete"
}

# Build project
build_project() {
    print_info "Building project with ${JOBS} parallel jobs..."

    cd "${BUILD_DIR}"
    cmake --build . --config "${BUILD_TYPE}" --parallel "${JOBS}"

    print_success "Build complete"
}

# Run TDD tests
run_tests() {
    if [[ "${RUN_TESTS}" == true ]] || [[ "${TDD_MODE}" == true ]]; then
        print_info "Running TDD test suite..."
        cd "${BUILD_DIR}"

        if [[ "${TDD_MODE}" == true ]]; then
            print_info "🔴🟢🔵 TDD Mode: Running complete Red-Green-Refactor workflow"

            # Run TDD Green phase (all tests should pass)
            print_info "🟢 TDD GREEN: Running all tests expecting success..."
            if cmake --build . --target tdd-green; then
                print_success "🟢 TDD GREEN: All tests passed!"
            else
                print_error "🔴 TDD GREEN: Tests failed!"
                return 1
            fi
        else
            # Standard test run
            print_info "Running standard test suite..."
            if ctest --output-on-failure --verbose; then
                print_success "All tests passed!"
            else
                print_error "Some tests failed!"
                return 1
            fi
        fi

        print_success "Test execution complete"
    fi
}

# Generate coverage report
generate_coverage() {
    if [[ "${COVERAGE}" == true ]]; then
        print_info "Generating code coverage report..."
        cd "${BUILD_DIR}"

        if command -v lcov &> /dev/null; then
            if cmake --build . --target coverage; then
                print_success "Coverage report generated"
                print_info "Coverage report available at: ${BUILD_DIR}/coverage_html/index.html"

                # Run coverage quality gate
                if cmake --build . --target coverage-check; then
                    print_success "🟢 TDD Quality Gate: Coverage requirement met!"
                else
                    print_error "🔴 TDD Quality Gate: Coverage requirement not met!"
                    return 1
                fi
            else
                print_error "Failed to generate coverage report"
                return 1
            fi
        else
            print_warning "lcov not available, skipping coverage report"
        fi
    fi
}

# Install project
install_project() {
    if [[ "${INSTALL}" == true ]]; then
        print_info "Installing project..."
        cd "${BUILD_DIR}"
        cmake --install . --config "${BUILD_TYPE}"
        print_success "Installation complete"
    fi
}

# Create package
create_package() {
    if [[ "${PACKAGE}" == true ]]; then
        print_info "Creating package..."
        cd "${BUILD_DIR}"
        cpack -C "${BUILD_TYPE}"
        print_success "Package creation complete"
    fi
}

# Main execution
main() {
    if [[ "${TDD_MODE}" == true ]]; then
        print_info "ETI Stream Analyser TDD Build Workflow"
        print_info "======================================"
        print_info "🔴🟢🔵 TDD Mode: Debug + Tests + Coverage"
    else
        print_info "ETI Stream Analyser Build Script"
        print_info "==============================="
    fi

    detect_platform
    check_dependencies
    check_system_dependencies
    configure_build
    build_project

    # TDD workflow: Run tests and coverage after build
    run_tests
    generate_coverage

    install_project
    create_package

    print_success "Build process completed successfully!"
    print_info "Build artifacts are in: ${BUILD_DIR}"

    # TDD-specific output
    if [[ "${TDD_MODE}" == true ]]; then
        print_success "🔴🟢🔵 TDD Workflow Complete!"
        print_info "✅ Build: ${BUILD_TYPE}"
        print_info "✅ Tests: Executed"
        if [[ "${COVERAGE}" == true ]]; then
            print_info "✅ Coverage: Generated (${BUILD_DIR}/coverage_html/index.html)"
        fi
        print_info ""
        print_info "TDD Development Commands:"
        print_info "  cd ${BUILD_DIR}"
        print_info "  make tdd-red      # 🔴 Run failing tests"
        print_info "  make tdd-green    # 🟢 Run passing tests"
        print_info "  make tdd-refactor # 🔵 Optimize code"
        print_info "  make coverage     # Generate coverage"
        print_info "  make quality-gate # Full TDD validation"
    fi

    if [[ "${BUILD_TYPE}" == "Debug" ]]; then
        print_info ""
        print_info "To run the application:"
        case "${PLATFORM}" in
            Linux|macOS)
                print_info "  ${BUILD_DIR}/ETIStreamAnalyser"
                ;;
            Windows)
                print_info "  ${BUILD_DIR}\\${BUILD_TYPE}\\ETIStreamAnalyser.exe"
                ;;
        esac
    fi
}

# Run main function
main "$@"
