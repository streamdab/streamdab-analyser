#!/bin/bash
# StreamDAB Analyser - Cross-Platform Package Builder
# Automated build script for all distribution packages (Linux/Windows/macOS)

set -e

# Build configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
BUILD_TYPE="${BUILD_TYPE:-Release}"
# Bounded parallelism (repo rule: never a bare -j/unbounded). Uses the same
# helper as the CI/build scripts — min(nproc, RAM_GB/2, 8).
PARALLEL_JOBS="${PARALLEL_JOBS:-$(bash "${PROJECT_ROOT}/scripts/build_jobs.sh" 2>/dev/null || echo 8)}"

# Professional logging
BUILD_LOG="${BUILD_DIR}/professional_build.log"

log_message() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "${BUILD_LOG}"
}

log_section() {
    echo "" | tee -a "${BUILD_LOG}"
    echo "========================================" | tee -a "${BUILD_LOG}"
    echo "$1" | tee -a "${BUILD_LOG}"
    echo "========================================" | tee -a "${BUILD_LOG}"
}

# Professional platform detection
detect_platform() {
    case "$(uname -s)" in
        Linux*)     echo "linux" ;;
        Darwin*)    echo "macos" ;;
        CYGWIN*|MINGW*|MSYS*) echo "windows" ;;
        *)          echo "unknown" ;;
    esac
}

# Professional dependency check
check_dependencies() {
    log_section "Professional Dependency Verification"

    local missing_deps=()

    # Essential build tools
    if ! command -v cmake >/dev/null 2>&1; then
        missing_deps+=("cmake")
    fi

    if ! command -v git >/dev/null 2>&1; then
        missing_deps+=("git")
    fi

    # Platform-specific dependencies
    local platform=$(detect_platform)
    case "$platform" in
        linux)
            if ! command -v pkg-config >/dev/null 2>&1; then
                missing_deps+=("pkg-config")
            fi
            if ! dpkg -l | grep -q qt6-base-dev && ! rpm -qa | grep -q qt6-qtbase-devel; then
                missing_deps+=("qt6-dev")
            fi
            ;;
        macos)
            if ! command -v brew >/dev/null 2>&1; then
                log_message "⚠️  Homebrew not found - some dependencies may be missing"
            fi
            ;;
        windows)
            if ! command -v cl.exe >/dev/null 2>&1 && ! command -v gcc >/dev/null 2>&1; then
                missing_deps+=("C++ compiler (MSVC or GCC)")
            fi
            ;;
    esac

    if [ ${#missing_deps[@]} -eq 0 ]; then
        log_message "✅ All essential dependencies available"
        return 0
    else
        log_message "❌ Missing dependencies: ${missing_deps[*]}"
        log_message "Please install missing dependencies and retry"
        return 1
    fi
}

# Professional build configuration
configure_build() {
    log_section "Professional Build Configuration"

    local platform=$(detect_platform)
    local cmake_args=(
        "-B" "${BUILD_DIR}"
        "-S" "${PROJECT_ROOT}"
        "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
        "-DBUILD_TESTING=ON"
        "-DENABLE_COVERAGE=OFF"
    )

    # Platform-specific configuration
    case "$platform" in
        linux)
            cmake_args+=("-G" "Unix Makefiles")
            if command -v ninja >/dev/null 2>&1; then
                cmake_args=("${cmake_args[@]/-G/}" "-G" "Ninja")
            fi
            ;;
        macos)
            cmake_args+=("-G" "Unix Makefiles")
            # Keep in step with packaging/macos/Info.plist.in
            # (LSMinimumSystemVersion 12.0): deployment target must not exceed
            # the documented minimum (review LOW).
            cmake_args+=("-DCMAKE_OSX_DEPLOYMENT_TARGET=${DABX_OSX_DEPLOYMENT_TARGET:-12.0}")
            if [ "$(uname -m)" = "arm64" ]; then
                cmake_args+=("-DCMAKE_OSX_ARCHITECTURES=arm64")
            else
                cmake_args+=("-DCMAKE_OSX_ARCHITECTURES=x86_64")
            fi
            ;;
        windows)
            cmake_args+=("-G" "Visual Studio 17 2022")
            cmake_args+=("-A" "x64")
            ;;
    esac

    log_message "CMake configuration: ${cmake_args[*]}"

    if cmake "${cmake_args[@]}"; then
        log_message "✅ Professional build configuration successful"
        return 0
    else
        log_message "❌ Professional build configuration failed"
        return 1
    fi
}

# Professional build execution
execute_build() {
    log_section "Professional Build Execution"

    local build_args=(
        "--build" "${BUILD_DIR}"
        "--config" "${BUILD_TYPE}"
        "--parallel" "${PARALLEL_JOBS}"
    )

    log_message "Building with ${PARALLEL_JOBS} parallel jobs"

    if cmake "${build_args[@]}"; then
        log_message "✅ Professional build execution successful"
        return 0
    else
        log_message "❌ Professional build execution failed"
        return 1
    fi
}

# Professional test execution
execute_tests() {
    log_section "Professional Test Execution"

    cd "${BUILD_DIR}"

    local test_args=(
        "--build-config" "${BUILD_TYPE}"
        "--output-on-failure"
        "--verbose"
    )

    if ctest "${test_args[@]}"; then
        log_message "✅ Professional test execution successful"
        cd "${PROJECT_ROOT}"
        return 0
    else
        log_message "❌ Professional test execution failed"
        cd "${PROJECT_ROOT}"
        return 1
    fi
}

# Professional package creation
create_packages() {
    log_section "Professional Package Creation"

    cd "${BUILD_DIR}"
    local platform=$(detect_platform)
    local packages_created=()

    # Create platform-specific packages
    case "$platform" in
        linux)
            log_message "Creating Linux professional packages..."

            # DEB package
            if cpack -G DEB; then
                packages_created+=("DEB")
                log_message "✅ Linux DEB package created"
            else
                log_message "❌ Linux DEB package creation failed"
            fi

            # RPM package (documented matrix is TGZ+DEB on Linux; RPM only when
            # explicitly wanted, so the unbranded generic RPM is never emitted
            # by default)
            if [ -n "${DABX_BUILD_RPM:-}" ] && cpack -G RPM; then
                packages_created+=("RPM")
                log_message "✅ Linux RPM package created (DABX_BUILD_RPM set)"
            else
                log_message "ℹ️  Linux RPM skipped (set DABX_BUILD_RPM=1 to build)"
            fi

            # AppImage
            if [ -x "${PROJECT_ROOT}/packaging/linux/create_appimage.sh" ]; then
                if CMAKE_SOURCE_DIR="${PROJECT_ROOT}" CMAKE_BINARY_DIR="${BUILD_DIR}" "${PROJECT_ROOT}/packaging/linux/create_appimage.sh"; then
                    packages_created+=("AppImage")
                    log_message "✅ Linux AppImage created"
                else
                    log_message "❌ Linux AppImage creation failed"
                fi
            fi
            ;;

        macos)
            log_message "Creating macOS professional packages..."

            # DMG package
            if cpack -G DragNDrop; then
                packages_created+=("DMG")
                log_message "✅ macOS DMG package created"
            else
                log_message "❌ macOS DMG package creation failed"
            fi
            ;;

        windows)
            log_message "Creating Windows professional packages..."

            # NSIS installer
            if cpack -G NSIS; then
                packages_created+=("NSIS")
                log_message "✅ Windows NSIS installer created"
            else
                log_message "❌ Windows NSIS installer creation failed"
            fi
            ;;
    esac

    # Create source packages (all platforms)
    log_message "Creating source packages..."
    if cpack --config CPackSourceConfig.cmake; then
        packages_created+=("Source")
        log_message "✅ Source packages created"
    else
        log_message "❌ Source package creation failed"
    fi

    cd "${PROJECT_ROOT}"

    if [ ${#packages_created[@]} -gt 0 ]; then
        log_message "✅ Professional packages created: ${packages_created[*]}"
        return 0
    else
        log_message "❌ No professional packages were created"
        return 1
    fi
}

# Professional package validation
validate_packages() {
    log_section "Professional Package Validation"

    if [ -x "${PROJECT_ROOT}/packaging/scripts/validate_packages.sh" ]; then
        if "${PROJECT_ROOT}/packaging/scripts/validate_packages.sh"; then
            log_message "✅ Professional package validation successful"
            return 0
        else
            log_message "❌ Professional package validation failed"
            return 1
        fi
    else
        log_message "⚠️  Package validation script not found"
        return 0
    fi
}

# Professional build summary
build_summary() {
    log_section "Professional Build Summary"

    log_message "Build directory: ${BUILD_DIR}"
    log_message "Build type: ${BUILD_TYPE}"
    log_message "Platform: $(detect_platform)"
    log_message "Parallel jobs: ${PARALLEL_JOBS}"

    # List created packages
    log_message ""
    log_message "Created packages:"
    find "${BUILD_DIR}" -name "*.exe" -o -name "*.deb" -o -name "*.rpm" -o -name "*.dmg" -o -name "*.AppImage" -o -name "*-source.tar.gz" -o -name "*-source.zip" | while read package; do
        if [ -f "$package" ]; then
            log_message "  - $(basename "$package") ($(du -h "$package" | cut -f1))"
        fi
    done

    log_message ""
    log_message "Build log saved to: ${BUILD_LOG}"
}

# Professional cleanup function
cleanup_on_exit() {
    local exit_code=$?
    if [ $exit_code -ne 0 ]; then
        log_message "❌ Professional build failed with exit code $exit_code"
    fi
    exit $exit_code
}

# Professional main build routine
main() {
    # Setup cleanup handler
    trap cleanup_on_exit EXIT

    log_section "StreamDAB Analyser Package Build System"
    log_message "Starting cross-platform package build"
    log_message "Target platform: $(detect_platform)"

    # Initialize build log
    mkdir -p "${BUILD_DIR}"
    echo "" > "${BUILD_LOG}"

    # Professional build pipeline
    check_dependencies || exit 1
    configure_build || exit 1
    execute_build || exit 1
    execute_tests || exit 1
    create_packages || exit 1
    validate_packages || exit 1

    # Professional completion
    build_summary
    log_section "Professional Build Completed Successfully"
    log_message "🎉 All professional packages are ready for deployment"
}

# Professional script execution with error handling
main "$@"
