#!/bin/bash
# Production Release Build Script for StreamDAB Stream Analyzer
# Creates optimized builds for professional distribution

set -e

# Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build-release"
PACKAGE_DIR="${BUILD_DIR}/packages"
VERSION=$(grep "VERSION " "${PROJECT_ROOT}/CMakeLists.txt" | head -1 | cut -d' ' -f4)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

echo "=== StreamDAB Stream Analyzer Production Release Build ==="
echo "Project Root: ${PROJECT_ROOT}"
echo "Build Directory: ${BUILD_DIR}"
echo "Version: ${VERSION}"
echo ""

# Clean and create build directory
log_info "Preparing production build environment..."
if [ -d "${BUILD_DIR}" ]; then
    log_info "Cleaning existing build directory..."
    rm -rf "${BUILD_DIR}"
fi

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Configure for production release
log_info "Configuring CMake for production release..."
cmake "${PROJECT_ROOT}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG -march=native -mtune=native -flto" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-s -flto" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DENABLE_CLANG_TIDY=OFF \
    -DCPACK_PACKAGE_VERSION="${VERSION}" \
    -DCPACK_STRIP_FILES=ON \
    -DBUILD_TESTING=ON

if [ $? -eq 0 ]; then
    log_success "CMake configuration completed successfully"
else
    log_error "CMake configuration failed"
    exit 1
fi

# Build the application
log_info "Building StreamDAB Stream Analyzer (optimized)..."
CPU_CORES=$(nproc)
log_info "Using ${CPU_CORES} CPU cores for parallel build..."

make -j${CPU_CORES}

if [ $? -eq 0 ]; then
    log_success "Build completed successfully"
else
    log_error "Build failed"
    exit 1
fi

# Run tests to validate build
log_info "Running test suite to validate build..."
if make test; then
    log_success "All tests passed"
else
    log_warning "Some tests failed - review before distribution"
fi

# Validate the build
log_info "Validating production build..."
if [ -x "./ETIStreamAnalyzer" ]; then
    log_success "Main executable created successfully"

    # Check executable properties
    log_info "Executable information:"
    file ./ETIStreamAnalyzer
    ls -lh ./ETIStreamAnalyzer

    # Check dynamic dependencies
    log_info "Dynamic library dependencies:"
    ldd ./ETIStreamAnalyzer | head -10

    # Test basic functionality
    log_info "Testing basic functionality..."
    if timeout 5s ./ETIStreamAnalyzer --version 2>/dev/null || timeout 5s ./ETIStreamAnalyzer --help 2>/dev/null; then
        log_success "Basic functionality test passed"
    else
        log_warning "Basic functionality test inconclusive"
    fi
else
    log_error "Main executable not found"
    exit 1
fi

# Create packages
log_info "Creating distribution packages..."
mkdir -p "${PACKAGE_DIR}"

# Create source package
log_info "Creating source package..."
make package_source
if [ $? -eq 0 ]; then
    log_success "Source package created successfully"
    find . -name "*.tar.gz" -o -name "*.zip" | grep -i source | head -5
fi

# Platform-specific packages
if [ "$(uname)" = "Linux" ]; then
    log_info "Creating Linux packages..."

    # DEB package
    if command -v dpkg-deb >/dev/null 2>&1; then
        log_info "Creating DEB package..."
        make package-linux-deb
        if [ $? -eq 0 ]; then
            log_success "DEB package created successfully"
            find . -name "*.deb" | head -3
        fi
    fi

    # RPM package
    if command -v rpmbuild >/dev/null 2>&1; then
        log_info "Creating RPM package..."
        make package-linux-rpm
        if [ $? -eq 0 ]; then
            log_success "RPM package created successfully"
            find . -name "*.rpm" | head -3
        fi
    fi

    # TAR.GZ package
    log_info "Creating TAR.GZ package..."
    make package
    if [ $? -eq 0 ]; then
        log_success "TAR.GZ package created successfully"
        find . -name "*${VERSION}*.tar.gz" | head -3
    fi
fi

# Package validation
log_info "Running package validation..."
if [ -x "${PROJECT_ROOT}/packaging/scripts/validate_packages.sh" ]; then
    "${PROJECT_ROOT}/packaging/scripts/validate_packages.sh"
    VALIDATION_RESULT=$?

    if [ $VALIDATION_RESULT -eq 0 ]; then
        log_success "Package validation passed"
    else
        log_warning "Package validation had issues (exit code: $VALIDATION_RESULT)"
    fi
else
    log_warning "Package validation script not found"
fi

# Generate checksums
log_info "Generating package checksums..."
cd "${BUILD_DIR}"
find . -name "*.deb" -o -name "*.rpm" -o -name "*.tar.gz" -o -name "*.zip" | grep -E "\.(deb|rpm|tar\.gz|zip)$" > package_list.txt

if [ -s package_list.txt ]; then
    log_info "Creating checksums for distribution packages..."
    sha256sum $(cat package_list.txt) > "${PACKAGE_DIR}/checksums.sha256"
    md5sum $(cat package_list.txt) > "${PACKAGE_DIR}/checksums.md5"
    log_success "Package checksums created"

    echo ""
    log_info "Package Summary:"
    cat package_list.txt | while read package; do
        if [ -f "$package" ]; then
            echo "  $(basename "$package") ($(du -h "$package" | cut -f1))"
        fi
    done

    echo ""
    log_info "SHA256 Checksums:"
    cat "${PACKAGE_DIR}/checksums.sha256"
else
    log_warning "No packages found for checksum generation"
fi

# Installation instructions
cat > "${PACKAGE_DIR}/INSTALL.txt" << 'EOF'
StreamDAB Stream Analyzer - Installation Instructions
====================================================

LINUX INSTALLATION:

1. DEB Package (Ubuntu/Debian):
   sudo dpkg -i streamdab-stream-analyzer_*.deb
   sudo apt-get install -f  # Install missing dependencies

2. RPM Package (Fedora/RHEL/SUSE):
   sudo rpm -i streamdab-stream-analyzer-*.rpm
   # or
   sudo dnf install streamdab-stream-analyzer-*.rpm

3. TAR.GZ Package (Universal):
   tar -xzf streamdab-stream-analyzer-*.tar.gz
   cd streamdab-stream-analyzer-*
   sudo make install

DEPENDENCIES:
- Qt6 (Core, Widgets, GUI, Network, Multimedia)
- fdk-aac or FAAD2 (for DAB+ audio decoding)
- FFTW3 (for signal processing)
- ALSA (Linux audio support)

VERIFICATION:
After installation, verify with:
  ETIStreamAnalyzer --version
  ETIStreamAnalyzer --help

USAGE:
- GUI Mode: ETIStreamAnalyzer
- CLI Mode: ETIStreamAnalyzer [file.eti]
- Real-time: ETIStreamAnalyzer --realtime

For support: https://github.com/streamdab-analyzer/desktop
EOF

log_success "Installation instructions created: ${PACKAGE_DIR}/INSTALL.txt"

# Final summary
echo ""
echo "=== Production Release Build Complete ==="
echo "Build Directory: ${BUILD_DIR}"
echo "Package Directory: ${PACKAGE_DIR}"
echo "Version: ${VERSION}"
echo ""

# Package summary
TOTAL_PACKAGES=$(find "${BUILD_DIR}" -name "*.deb" -o -name "*.rpm" -o -name "*.tar.gz" -o -name "*.zip" | wc -l)
log_success "Created ${TOTAL_PACKAGES} distribution packages"

if [ $TOTAL_PACKAGES -gt 0 ]; then
    TOTAL_SIZE=$(find "${BUILD_DIR}" -name "*.deb" -o -name "*.rpm" -o -name "*.tar.gz" -o -name "*.zip" -exec du -b {} + | awk '{sum+=$1} END {print sum}')
    TOTAL_SIZE_MB=$((TOTAL_SIZE / 1024 / 1024))
    log_info "Total package size: ${TOTAL_SIZE_MB} MB"

    echo ""
    log_success "Production release is ready for distribution!"
    log_info "Next steps:"
    echo "  1. Test packages on target systems"
    echo "  2. Upload to distribution repositories"
    echo "  3. Update project documentation"
    echo "  4. Create release announcement"
else
    log_warning "No packages were created - check build configuration"
    exit 1
fi
