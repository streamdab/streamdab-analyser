#!/bin/bash
# Package Validation Script for StreamDAB Analyser
# Validates created packages for distribution readiness

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
PACKAGE_DIR="${BUILD_DIR}"  # CPack artifacts land in build/ (CPACK_PACKAGE_DIRECTORY)

echo "=== StreamDAB Analyser Package Validation ==="
echo "Project Root: ${PROJECT_ROOT}"
echo "Build Directory: ${BUILD_DIR}"
echo "Package Directory: ${PACKAGE_DIR}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Validation results
VALIDATION_ERRORS=0
VALIDATION_WARNINGS=0

log_error() {
    echo -e "${RED}ERROR: $1${NC}"
    ((VALIDATION_ERRORS++))
}

log_warning() {
    echo -e "${YELLOW}WARNING: $1${NC}"
    ((VALIDATION_WARNINGS++))
}

log_success() {
    echo -e "${GREEN}SUCCESS: $1${NC}"
}

log_info() {
    echo -e "${BLUE}INFO: $1${NC}"
}

# Check if build directory exists
if [ ! -d "${BUILD_DIR}" ]; then
    log_error "Build directory not found: ${BUILD_DIR}"
    exit 1
fi

cd "${BUILD_DIR}"

# 1. Validate CMake Configuration
log_info "Validating CMake configuration..."
if [ ! -f "CMakeCache.txt" ]; then
    log_error "CMakeCache.txt not found - run cmake first"
else
    log_success "CMake configuration found"

    # Check for essential configurations
    if grep -q "CMAKE_BUILD_TYPE:STRING=Release" CMakeCache.txt; then
        log_success "Release build configuration detected"
    else
        log_warning "Not configured for Release build - recommended for production"
    fi

    if grep -q "Qt6_DIR" CMakeCache.txt; then
        log_success "Qt6 configuration found"
    else
        log_error "Qt6 not properly configured"
    fi
fi

# 2. Validate Main Executable
log_info "Validating main executable..."
EXECUTABLE_FOUND=false

# Check different possible locations
for executable_path in "StreamDABAnalyser" "bin/StreamDABAnalyser" "./StreamDABAnalyser"; do
    if [ -f "${executable_path}" ]; then
        log_success "Main executable found: ${executable_path}"
        EXECUTABLE_FOUND=true

        # Check if executable is properly linked
        if command -v ldd >/dev/null 2>&1; then
            log_info "Checking dynamic library dependencies..."
            if ldd "${executable_path}" | grep -q "not found"; then
                log_error "Missing dynamic library dependencies detected"
                ldd "${executable_path}" | grep "not found"
            else
                log_success "All dynamic library dependencies satisfied"
            fi
        fi

        # Check file permissions
        if [ -x "${executable_path}" ]; then
            log_success "Executable has proper permissions"
        else
            log_error "Executable lacks execute permissions"
        fi

        break
    fi
done

if [ "$EXECUTABLE_FOUND" = false ]; then
    log_error "Main executable not found - build may have failed"
fi

# 3. Validate Qt Integration
log_info "Validating Qt integration..."
if [ -d "DABAnalyser_Enhanced_FilteringBuild_autogen" ]; then
    log_success "Qt MOC/UIC integration found"

    # Check for generated UI headers
    UI_HEADERS_FOUND=0
    for ui_header in DABAnalyser_Enhanced_FilteringBuild_autogen/include/ui_*.h; do
        if [ -f "$ui_header" ]; then
            ((UI_HEADERS_FOUND++))
        fi
    done

    if [ $UI_HEADERS_FOUND -gt 0 ]; then
        log_success "Found $UI_HEADERS_FOUND generated UI headers"
    else
        log_warning "No UI headers found - may indicate missing .ui files"
    fi
else
    log_warning "Qt autogen directory not found"
fi

# 4. Validate Test Framework
log_info "Validating test framework..."
if [ -d "tests" ]; then
    log_success "Test directory found"

    # Count test executables
    TEST_EXECUTABLES=$(find tests -name "test_*" -type f -executable 2>/dev/null | wc -l)
    if [ $TEST_EXECUTABLES -gt 0 ]; then
        log_success "Found $TEST_EXECUTABLES test executables"
    else
        log_warning "No test executables found"
    fi
else
    log_warning "Test directory not found"
fi

# 5. Validate Documentation
log_info "Validating documentation..."
if [ -f "${PROJECT_ROOT}/README.md" ]; then
    log_success "README.md found"
else
    log_warning "README.md not found"
fi

if [ -f "${PROJECT_ROOT}/CLAUDE.md" ]; then
    log_success "Project documentation (CLAUDE.md) found"
else
    log_warning "Project documentation not found"
fi

# 6. Validate Packaging Files
log_info "Validating packaging configuration..."

# Check packaging configuration
if [ -f "CPackConfig.cmake" ]; then
    log_success "CPack configuration found"

    # Check for platform-specific packaging
    if grep -q "CPACK_GENERATOR" CPackConfig.cmake; then
        GENERATORS=$(grep "CPACK_GENERATOR" CPackConfig.cmake | cut -d'"' -f2)
        log_success "Package generators configured: $GENERATORS"
    fi
else
    log_error "CPack configuration not found"
fi

# Check for desktop integration files (Linux)
if [ -f "${PROJECT_ROOT}/resources/linux/eti-stream-analyser.desktop" ]; then
    log_success "Linux desktop integration file found"

    # Validate desktop file syntax
    if command -v desktop-file-validate >/dev/null 2>&1; then
        if desktop-file-validate "${PROJECT_ROOT}/resources/linux/eti-stream-analyser.desktop"; then
            log_success "Desktop file syntax validation passed"
        else
            log_error "Desktop file syntax validation failed"
        fi
    fi
else
    log_warning "Linux desktop integration file not found"
fi

# Check for MIME type definitions
if [ -f "${PROJECT_ROOT}/resources/linux/eti-stream-analyser.xml" ]; then
    log_success "MIME type definitions found"
else
    log_warning "MIME type definitions not found"
fi

# 7. Validate Dependencies
log_info "Validating build dependencies..."

# Check for essential libraries
REQUIRED_LIBS=("Qt6Core" "Qt6Widgets" "Qt6Gui" "fdk-aac" "fftw3")
for lib in "${REQUIRED_LIBS[@]}"; do
    if grep -q "$lib" CMakeCache.txt; then
        log_success "Dependency found: $lib"
    else
        log_warning "Dependency not found: $lib"
    fi
done

# 8. Check Package Creation Capability
log_info "Testing package creation capability..."

# Test DEB package creation (if on Debian/Ubuntu)
if command -v dpkg-deb >/dev/null 2>&1; then
    log_info "Testing DEB package creation..."
    if make package-linux-deb >/dev/null 2>&1; then
        log_success "DEB package creation test passed"
    else
        log_warning "DEB package creation test failed"
    fi
fi

# Test RPM package creation (if rpmbuild available)
if command -v rpmbuild >/dev/null 2>&1; then
    log_info "Testing RPM package creation..."
    if make package-linux-rpm >/dev/null 2>&1; then
        log_success "RPM package creation test passed"
    else
        log_warning "RPM package creation test failed"
    fi
fi

# 9. Performance Validation
log_info "Validating performance configuration..."

# Check for optimized build flags
if grep -q "\-O3\|CMAKE_BUILD_TYPE.*Release" CMakeCache.txt; then
    log_success "Optimized build configuration detected"
else
    log_warning "Build may not be optimized for performance"
fi

# Check for debug symbols in release build
if [ "$EXECUTABLE_FOUND" = true ]; then
    if command -v file >/dev/null 2>&1; then
        EXECUTABLE_PATH=$(find . -name "StreamDABAnalyser" -type f -executable | head -1)
        if [ -n "$EXECUTABLE_PATH" ]; then
            FILE_INFO=$(file "$EXECUTABLE_PATH")
            if echo "$FILE_INFO" | grep -q "not stripped"; then
                log_warning "Executable contains debug symbols - consider stripping for production"
            else
                log_success "Executable is stripped for production"
            fi
        fi
    fi
fi

# 10. Security Validation
log_info "Validating security configuration..."

# Check for security compilation flags
if grep -q "fPIC\|fstack-protector" CMakeCache.txt; then
    log_success "Security compilation flags detected"
else
    log_warning "Security compilation flags not detected"
fi

# Summary
echo ""
echo "=== Validation Summary ==="
echo -e "Total Errors: ${RED}${VALIDATION_ERRORS}${NC}"
echo -e "Total Warnings: ${YELLOW}${VALIDATION_WARNINGS}${NC}"

if [ $VALIDATION_ERRORS -eq 0 ]; then
    if [ $VALIDATION_WARNINGS -eq 0 ]; then
        log_success "All validations passed! Package is ready for distribution."
        exit 0
    else
        log_success "Package validation passed with warnings. Review warnings before distribution."
        exit 0
    fi
else
    log_error "Package validation failed. Fix errors before distribution."
    exit 1
fi
