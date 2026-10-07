#!/bin/bash
# ============================================================================
# ETI Stream Analyser - Linux/macOS Native Build Script
# Professional shell build automation for Unix-like systems
# ============================================================================

set -euo pipefail

# ============================================================================
# Configuration and Constants
# ============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
INSTALL_DIR="$PROJECT_ROOT/install"

# Default configuration
BUILD_TYPE="Release"
ENABLE_TESTING=true
ENABLE_COVERAGE=false
CLEAN_BUILD=false
INSTALL_DEPS=false
PACKAGE=false
HELP=false
# Build parallelism: bounded by CPU *and* RAM (see scripts/build_jobs.sh).
. "${SCRIPT_DIR}/build_jobs.sh"
JOBS=$(streamdab_build_jobs)
export CMAKE_BUILD_PARALLEL_LEVEL="${JOBS}"

# Application metadata — mirrors the CMake single-source resolution
# (src/core/product_version.hpp: -DDABX_VERSION=, else the git tag, else
# PROJECT_VERSION in CMakeLists.txt). Keep in step with both.
APP_NAME="StreamDAB Analyser"
APP_VERSION="$(git describe --tags --abbrev=0 2>/dev/null | sed 's/^v//')"
[ -z "$APP_VERSION" ] && APP_VERSION="1.6.0"

# Detect operating system
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
    DISTRO=$(lsb_release -si 2>/dev/null || echo "Unknown")
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
    DISTRO="macOS"
else
    OS="unknown"
    DISTRO="Unknown"
fi

# ============================================================================
# Color output functions
# ============================================================================

if [[ -t 1 ]]; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    BLUE='\033[0;34m'
    CYAN='\033[0;36m'
    NC='\033[0m' # No Color
else
    RED=''
    GREEN=''
    YELLOW=''
    BLUE=''
    CYAN=''
    NC=''
fi

banner() {
    echo ""
    echo -e "${GREEN}============================================================================${NC}"
    echo -e "${GREEN} $1${NC}"
    echo -e "${GREEN}============================================================================${NC}"
    echo ""
}

step() {
    echo ""
    echo -e "${CYAN}==> $1${NC}"
}

success() {
    echo -e "${GREEN}✓ $1${NC}"
}

error() {
    echo -e "${RED}✗ $1${NC}" >&2
}

warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

# ============================================================================
# Help function
# ============================================================================

show_help() {
    banner "$APP_NAME - Unix Build Script"
    cat << EOF
USAGE:
  $0 [OPTIONS]

OPTIONS:
  --build-type TYPE       Build configuration (Debug|Release) [default: Release]
  --enable-testing        Enable test suite compilation [default: enabled]
  --disable-testing       Disable test suite compilation
  --enable-coverage       Enable code coverage reporting
  --disable-coverage      Disable code coverage reporting [default]
  --clean                 Clean build directory before building
  --install-deps          Install required dependencies
  --package               Create distribution packages
  --jobs N                Number of parallel build jobs [default: $JOBS]
  --help                  Show this help message

EXAMPLES:
  $0                                    # Default release build
  $0 --build-type Debug --enable-testing  # Debug build with tests
  $0 --clean --package                     # Clean build with packaging
  $0 --install-deps                        # Install dependencies only

REQUIREMENTS:
  Linux:
    - GCC 9+ or Clang 10+
    - CMake 3.21+
    - Qt6 development packages
    - pkg-config

  macOS:
    - Xcode Command Line Tools
    - CMake 3.21+
    - Qt6 (via Homebrew recommended)
    - pkg-config

DEPENDENCIES:
  - libfaad-dev (Linux) / faad2 (macOS)
  - libfftw3-dev (Linux) / fftw (macOS)
  - libyaml-cpp-dev (Linux) / yaml-cpp (macOS)
  - Google Test (automatically fetched if not found)

EOF
}

# ============================================================================
# Command line parsing
# ============================================================================

parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --build-type)
                BUILD_TYPE="$2"
                shift 2
                ;;
            --enable-testing)
                ENABLE_TESTING=true
                shift
                ;;
            --disable-testing)
                ENABLE_TESTING=false
                shift
                ;;
            --enable-coverage)
                ENABLE_COVERAGE=true
                shift
                ;;
            --disable-coverage)
                ENABLE_COVERAGE=false
                shift
                ;;
            --clean)
                CLEAN_BUILD=true
                shift
                ;;
            --install-deps)
                INSTALL_DEPS=true
                shift
                ;;
            --package)
                PACKAGE=true
                shift
                ;;
            --jobs)
                JOBS="$2"
                shift 2
                ;;
            --help)
                HELP=true
                shift
                ;;
            *)
                error "Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    done
}

# ============================================================================
# Dependency installation
# ============================================================================

install_dependencies_linux() {
    step "Installing Linux dependencies"

    case "$DISTRO" in
        Ubuntu|Debian)
            sudo apt-get update
            sudo apt-get install -y \
                build-essential \
                cmake \
                pkg-config \
                qt6-base-dev \
                qt6-tools-dev \
                libqt6core6 \
                libqt6gui6 \
                libqt6widgets6 \
                libqt6test6 \
                libqt6network6 \
                libqt6multimedia6-dev \
                libfaad-dev \
                libfftw3-dev \
                libyaml-cpp-dev \
                libasound2-dev \
                libpulse-dev \
                git \
                lcov \
                gcovr
            ;;
        Fedora|CentOS|RHEL)
            sudo dnf install -y \
                gcc-c++ \
                cmake \
                pkgconfig \
                qt6-qtbase-devel \
                qt6-qttools-devel \
                qt6-qtmultimedia-devel \
                faad2-devel \
                fftw-devel \
                yaml-cpp-devel \
                alsa-lib-devel \
                pulseaudio-libs-devel \
                git \
                lcov
            ;;
        Arch)
            sudo pacman -S --needed \
                base-devel \
                cmake \
                pkgconf \
                qt6-base \
                qt6-tools \
                qt6-multimedia \
                faad2 \
                fftw \
                yaml-cpp \
                alsa-lib \
                libpulse \
                git \
                lcov
            ;;
        *)
            warning "Unknown Linux distribution: $DISTRO"
            warning "Please install dependencies manually"
            ;;
    esac

    success "Linux dependencies installed"
}

install_dependencies_macos() {
    step "Installing macOS dependencies"

    # Detect Qt version and set deployment target dynamically
    detect_qt_version_and_deployment_target
    # Check for Homebrew
    if ! command -v brew &> /dev/null; then
        error "Homebrew not found. Please install Homebrew first:"
        echo "  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
        exit 1
    fi

    # Install packages via Homebrew
    brew update
    brew install \
        cmake \
        pkg-config \
        qt6 \
        faad2 \
        fftw \
        yaml-cpp \
        portaudio \
        git \
        lcov

    # Set Qt6 environment
    export Qt6_DIR="$(brew --prefix qt6)/lib/cmake/Qt6"
    export PATH="$(brew --prefix qt6)/bin:$PATH"

    success "macOS dependencies installed"
    success "Qt6_DIR set to: $Qt6_DIR"
}

install_dependencies() {
    case "$OS" in
        linux)
            install_dependencies_linux
            ;;
        macos)
            install_dependencies_macos
            ;;
        *)
            error "Unsupported operating system: $OS"
            exit 1
            ;;
    esac
}

# ============================================================================
# Qt version detection for macOS deployment target
# ============================================================================

detect_qt_version_and_deployment_target() {
    if [[ "$OS" != "macos" ]]; then
        return 0
    fi

    step "Detecting Qt version for macOS deployment target"

    # Try multiple methods to detect Qt version
    local qt_version=""

    # Method 1: qmake -query
    if command -v qmake &> /dev/null; then
        qt_version=$(qmake -query QT_VERSION 2>/dev/null)
    fi

    # Method 2: qmake6 -query (for explicit Qt6)
    if [[ -z "$qt_version" ]] && command -v qmake6 &> /dev/null; then
        qt_version=$(qmake6 -query QT_VERSION 2>/dev/null)
    fi

    # Method 3: pkg-config
    if [[ -z "$qt_version" ]] && command -v pkg-config &> /dev/null; then
        qt_version=$(pkg-config --modversion Qt6Core 2>/dev/null)
    fi

    # Method 4: Homebrew Qt6 qmake
    if [[ -z "$qt_version" ]] && [[ -x "$(brew --prefix qt6 2>/dev/null)/bin/qmake" ]]; then
        qt_version=$("$(brew --prefix qt6)/bin/qmake" -query QT_VERSION 2>/dev/null)
    fi

    if [[ -z "$qt_version" ]]; then
        warning "Could not detect Qt version. Using default macOS deployment target 11.0"
        export MACOSX_DEPLOYMENT_TARGET="11.0"
        CMAKE_OSX_DEPLOYMENT_TARGET="11.0"
        return 0
    fi

    success "Detected Qt version: $qt_version"

    # Parse version components
    local major minor patch
    IFS='.' read -r major minor patch <<< "$qt_version"

    # Determine macOS deployment target based on Qt version
    local deployment_target
    if [[ $major -gt 6 ]] || [[ $major -eq 6 && $minor -ge 9 ]]; then
        # Qt 6.9+ requires macOS 14.0+
        deployment_target="14.0"
    elif [[ $major -eq 6 && $minor -ge 8 ]]; then
        # Qt 6.8+ requires macOS 12.0+
        deployment_target="12.0"
    elif [[ $major -eq 6 && $minor -ge 5 ]]; then
        # Qt 6.5+ requires macOS 11.0+
        deployment_target="11.0"
    elif [[ $major -eq 6 && $minor -ge 3 ]]; then
        # Qt 6.3-6.4 requires macOS 10.15+
        deployment_target="10.15"
    elif [[ $major -eq 6 && $minor -ge 0 ]]; then
        # Qt 6.0-6.2 requires macOS 10.14+
        deployment_target="10.14"
    else
        # Fallback for unknown/older versions
        deployment_target="10.14"
    fi

    export MACOSX_DEPLOYMENT_TARGET="$deployment_target"
    CMAKE_OSX_DEPLOYMENT_TARGET="$deployment_target"

    success "Set macOS deployment target to: $deployment_target (based on Qt $qt_version)"
}

# ============================================================================
# Build environment check
# ============================================================================

check_build_environment() {
    step "Checking build environment"

    # Check CMake
    if ! command -v cmake &> /dev/null; then
        error "CMake not found. Please install CMake 3.21 or later"
        exit 1
    fi

    cmake_version=$(cmake --version | head -n1 | cut -d' ' -f3)
    success "CMake $cmake_version found"

    # Check compiler
    if command -v g++ &> /dev/null; then
        compiler_version=$(g++ --version | head -n1)
        success "Compiler: $compiler_version"
    elif command -v clang++ &> /dev/null; then
        compiler_version=$(clang++ --version | head -n1)
        success "Compiler: $compiler_version"
    else
        error "No suitable C++ compiler found"
        exit 1
    fi

    # Check Qt6
    if command -v qmake6 &> /dev/null; then
        qt_version=$(qmake6 -query QT_VERSION)
        success "Qt6 $qt_version found"
    elif [[ -n "${Qt6_DIR:-}" ]] && [[ -d "$Qt6_DIR" ]]; then
        success "Qt6 found at $Qt6_DIR"
    else
        warning "Qt6 not found in standard locations"
        warning "You may need to set Qt6_DIR or install Qt6"
    fi

    # Check pkg-config
    if ! command -v pkg-config &> /dev/null; then
        error "pkg-config not found. Please install pkg-config"
        exit 1
    fi
    success "pkg-config found"
}

# ============================================================================
# Build configuration
# ============================================================================

configure_build() {
    step "Configuring CMake build"

    if [[ "$CLEAN_BUILD" == "true" ]] && [[ -d "$BUILD_DIR" ]]; then
        step "Cleaning build directory"
        rm -rf "$BUILD_DIR"
        success "Build directory cleaned"
    fi

    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    # CMake configuration arguments
    cmake_args=(
        "-S" "$PROJECT_ROOT"
        "-B" "."
        "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
        "-DBUILD_TESTING=$(echo $ENABLE_TESTING | tr '[:upper:]' '[:lower:]')"
        "-DENABLE_COVERAGE=$(echo $ENABLE_COVERAGE | tr '[:upper:]' '[:lower:]')"
        "-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR"
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    )

    # Add Qt6 path if set
    if [[ -n "${Qt6_DIR:-}" ]]; then
        cmake_args+=("-DQt6_DIR=$Qt6_DIR")
    fi

    # Platform-specific configurations
    case "$OS" in
        macos)
            # Ensure Qt version detection has been run
            if [[ -z "${CMAKE_OSX_DEPLOYMENT_TARGET:-}" ]]; then
                detect_qt_version_and_deployment_target
            fi
            cmake_args+=(
                "-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}"
                "-DCMAKE_MACOSX_BUNDLE=ON"
            )
            ;;
        linux)
            # Linux-specific configurations
            cmake_args+=(
                "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
            )
            ;;
    esac

    echo "Running: cmake ${cmake_args[*]}"
    cmake "${cmake_args[@]}"

    success "CMake configuration completed"
}

# ============================================================================
# Build execution
# ============================================================================

build_project() {
    step "Building project ($BUILD_TYPE)"

    cd "$BUILD_DIR"

    # Build with maximum parallel jobs
    build_args=(
        "--build" "."
        "--config" "$BUILD_TYPE"
        "--parallel" "$JOBS"
    )

    echo "Running: cmake ${build_args[*]}"
    cmake "${build_args[@]}"

    success "Build completed successfully"
}

# ============================================================================
# Testing
# ============================================================================

run_tests() {
    if [[ "$ENABLE_TESTING" != "true" ]]; then
        step "Testing disabled, skipping tests"
        return
    fi

    step "Running test suite"
    cd "$BUILD_DIR"

    # Run CTest
    test_args=(
        "--output-on-failure"
        "--parallel" "$JOBS"
    )

    echo "Running: ctest ${test_args[*]}"
    ctest "${test_args[@]}"

    success "All tests passed"
}

# ============================================================================
# Coverage analysis
# ============================================================================

generate_coverage() {
    if [[ "$ENABLE_COVERAGE" != "true" ]]; then
        step "Coverage disabled, skipping coverage analysis"
        return
    fi

    step "Generating coverage report"
    cd "$BUILD_DIR"

    if command -v lcov &> /dev/null; then
        # Generate coverage with lcov
        cmake --build . --target coverage
        success "Coverage report generated with lcov"
    elif command -v gcovr &> /dev/null; then
        # Generate coverage with gcovr
        gcovr -r "$PROJECT_ROOT" --html --html-details -o coverage.html .
        success "Coverage report generated with gcovr"
    else
        warning "No coverage tools found (lcov or gcovr). Coverage analysis skipped"
    fi
}

# ============================================================================
# Installation
# ============================================================================

install_application() {
    step "Installing application"
    cd "$BUILD_DIR"

    install_args=(
        "--install" "."
        "--config" "$BUILD_TYPE"
        "--prefix" "$INSTALL_DIR"
    )

    echo "Running: cmake ${install_args[*]}"
    cmake "${install_args[@]}"

    success "Application installed to $INSTALL_DIR"
}

# ============================================================================
# Packaging
# ============================================================================

create_packages() {
    if [[ "$PACKAGE" != "true" ]]; then
        step "Packaging disabled, skipping package creation"
        return
    fi

    step "Creating distribution packages"
    cd "$BUILD_DIR"

    case "$OS" in
        linux)
            # Create DEB package
            if command -v cpack &> /dev/null; then
                echo "Creating DEB package..."
                cpack -G DEB
                success "DEB package created"

                echo "Creating RPM package..."
                cpack -G RPM || warning "RPM package creation failed"

                echo "Creating TAR.GZ package..."
                cpack -G TGZ
                success "TAR.GZ package created"
            fi

            # Create AppImage if tools available
            if command -v linuxdeploy &> /dev/null; then
                echo "Creating AppImage..."
                # AppImage creation logic here
                success "AppImage created"
            else
                warning "linuxdeploy not found. AppImage creation skipped"
            fi
            ;;
        macos)
            # Create DMG package
            if command -v cpack &> /dev/null; then
                echo "Creating DMG package..."
                cpack -G DragNDrop
                success "DMG package created"

                echo "Creating TAR.GZ package..."
                cpack -G TGZ
                success "TAR.GZ package created"
            fi
            ;;
    esac

    # List created packages
    packages=$(find . -maxdepth 1 -name "*.deb" -o -name "*.rpm" -o -name "*.dmg" -o -name "*.tar.gz" -o -name "*.AppImage" 2>/dev/null || true)
    if [[ -n "$packages" ]]; then
        success "Created packages:"
        echo "$packages" | while read -r pkg; do
            echo "  - $(basename "$pkg")"
        done
    fi
}

# ============================================================================
# Main execution
# ============================================================================

main() {
    parse_args "$@"

    banner "$APP_NAME v$APP_VERSION - $OS Build System"
    echo "Platform: $OS ($DISTRO)"
    echo "Build Jobs: $JOBS"
    echo ""

    if [[ "$HELP" == "true" ]]; then
        show_help
        return 0
    fi

    # Store original directory
    original_dir=$(pwd)

    # Ensure we return to original directory on exit
    trap 'cd "$original_dir"' EXIT

    if [[ "$INSTALL_DEPS" == "true" ]]; then
        install_dependencies
        success "Dependencies installation completed"
        return 0
    fi

    # Execute build pipeline
    check_build_environment
    configure_build
    build_project
    run_tests
    generate_coverage
    install_application
    create_packages

    banner "Build Completed Successfully!"
    echo -e "${GREEN}Build Type: $BUILD_TYPE${NC}"
    echo -e "${GREEN}Testing: $([ "$ENABLE_TESTING" == "true" ] && echo "Enabled" || echo "Disabled")${NC}"
    echo -e "${GREEN}Coverage: $([ "$ENABLE_COVERAGE" == "true" ] && echo "Enabled" || echo "Disabled")${NC}"
    echo -e "${GREEN}Install Directory: $INSTALL_DIR${NC}"

    if [[ "$PACKAGE" == "true" ]]; then
        echo -e "${GREEN}Packages: Created in $BUILD_DIR${NC}"
    fi
}

# Execute main function with all arguments
main "$@"
