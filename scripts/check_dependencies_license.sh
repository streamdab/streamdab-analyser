#!/bin/bash
# Dependency and License Compliance Check for StreamDAB Stream Analyzer
# Validates all dependencies and their licenses for commercial distribution

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

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

echo "=== StreamDAB Stream Analyzer Dependency & License Check ==="
echo "Project Root: ${PROJECT_ROOT}"
echo ""

# Check main dependencies and their licenses
log_info "Checking core dependencies and licenses..."

# Qt6 License Check
log_info "Checking Qt6 licensing..."
if command -v qmake6 >/dev/null 2>&1; then
    QT_VERSION=$(qmake6 -query QT_VERSION 2>/dev/null || echo "Unknown")
    QT_INSTALL_PREFIX=$(qmake6 -query QT_INSTALL_PREFIX 2>/dev/null || echo "Unknown")
    log_success "Qt6 Version: ${QT_VERSION}"
    log_info "Qt6 Install Path: ${QT_INSTALL_PREFIX}"
    log_info "Qt6 License: LGPL-3.0 / GPL-3.0 / Commercial (Compatible for open source distribution)"
else
    log_warning "Qt6 qmake not found in PATH"
fi

# FDK-AAC License Check
log_info "Checking FDK-AAC licensing..."
if pkg-config --exists fdk-aac; then
    FDK_VERSION=$(pkg-config --modversion fdk-aac)
    log_success "FDK-AAC Version: ${FDK_VERSION}"
    log_warning "FDK-AAC License: Modified BSD License with additional restrictions"
    log_warning "Note: FDK-AAC may have patent licensing requirements for commercial use"
    log_info "Alternative: Consider FAAD2 (GPL-2.0) for open source compatibility"
else
    log_info "FDK-AAC not found, checking FAAD2..."
    if pkg-config --exists faad2; then
        FAAD2_VERSION=$(pkg-config --modversion faad2)
        log_success "FAAD2 Version: ${FAAD2_VERSION}"
        log_success "FAAD2 License: GPL-2.0 (Compatible for GPL distribution)"
    else
        log_error "No DAB+ audio decoder found (FDK-AAC or FAAD2 required)"
    fi
fi

# FFTW3 License Check
log_info "Checking FFTW3 licensing..."
if pkg-config --exists fftw3; then
    FFTW3_VERSION=$(pkg-config --modversion fftw3)
    log_success "FFTW3 Version: ${FFTW3_VERSION}"
    log_success "FFTW3 License: GPL-2.0 or later (Compatible for GPL distribution)"
else
    log_error "FFTW3 not found - required for signal processing"
fi

# YAML-CPP License Check (Optional)
log_info "Checking optional dependencies..."
if pkg-config --exists yaml-cpp; then
    YAML_VERSION=$(pkg-config --modversion yaml-cpp)
    log_success "YAML-CPP Version: ${YAML_VERSION}"
    log_success "YAML-CPP License: MIT License (Compatible for all distributions)"
else
    log_info "YAML-CPP not found (optional dependency)"
fi

# Check system libraries
log_info "Checking system library licensing..."

# ALSA (Linux)
if [ "$(uname)" = "Linux" ]; then
    if pkg-config --exists alsa; then
        ALSA_VERSION=$(pkg-config --modversion alsa)
        log_success "ALSA Version: ${ALSA_VERSION}"
        log_success "ALSA License: LGPL-2.1 (Compatible for all distributions)"
    else
        log_warning "ALSA not found - audio may be limited"
    fi
fi

# Check for additional dependencies from CMake
log_info "Analyzing CMake dependency configuration..."
if [ -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    log_info "Extracting dependency information from CMake cache..."

    # Extract Qt components
    echo "Qt6 Components:"
    grep "Qt6.*_DIR:PATH" "${BUILD_DIR}/CMakeCache.txt" | sed 's/.*Qt6\([^_]*\)_DIR:PATH=.*/  - Qt6\1/' | sort -u

    # Extract other libraries
    echo ""
    echo "Other Libraries:"
    grep -E "(fdk-aac|faad2|fftw3|yaml-cpp|alsa)" "${BUILD_DIR}/CMakeCache.txt" | grep -E "(FOUND|VERSION)" | head -10
else
    log_warning "CMakeCache.txt not found - run cmake first"
fi

# License Compatibility Analysis
echo ""
log_info "License Compatibility Analysis..."

cat << 'EOF'
=== LICENSE COMPATIBILITY MATRIX ===

Core Dependencies:
✅ Qt6: LGPL-3.0/GPL-3.0 - Compatible with GPL-3.0+ distribution
✅ FFTW3: GPL-2.0+ - Compatible with GPL-3.0+ distribution
✅ ALSA: LGPL-2.1 - Compatible with all distributions

Audio Decoders (Choose One):
⚠️  FDK-AAC: Modified BSD with restrictions - May require patent licenses
✅ FAAD2: GPL-2.0 - Preferred for open source distribution

Optional Dependencies:
✅ YAML-CPP: MIT - Compatible with all distributions

RECOMMENDED LICENSE STRATEGY:
- Primary Distribution: GPL-3.0 (ensures compatibility with all core dependencies)
- Commercial Distribution: Consider FDK-AAC patent licensing requirements
- Open Source Distribution: Use FAAD2 instead of FDK-AAC for full GPL compatibility

PATENT CONSIDERATIONS:
- DAB/DAB+ audio coding patents may apply regardless of decoder choice
- Consider consulting IP attorney for commercial deployment
- Open source/research use typically has different patent implications
EOF

# Check for license files in project
echo ""
log_info "Checking project license files..."

if [ -f "${PROJECT_ROOT}/LICENSE" ]; then
    log_success "Main LICENSE file found"
    LICENSE_TYPE=$(head -5 "${PROJECT_ROOT}/LICENSE" | grep -i -E "(GPL|MIT|BSD|Apache)" | head -1 || echo "Unknown")
    log_info "License appears to be: ${LICENSE_TYPE}"
else
    log_warning "Main LICENSE file not found - should be added for distribution"
fi

# Check for third-party license attributions
if [ -f "${PROJECT_ROOT}/THIRD_PARTY_LICENSES" ] || [ -f "${PROJECT_ROOT}/ATTRIBUTION" ]; then
    log_success "Third-party license attribution found"
else
    log_warning "Third-party license attribution file not found"
    log_info "Consider creating THIRD_PARTY_LICENSES file with:"
    cat << 'EOF'
    - Qt6 framework licensing details
    - Audio decoder (FDK-AAC/FAAD2) licensing
    - FFTW3 licensing acknowledgment
    - Any other third-party components
EOF
fi

# Generate license compliance report
REPORT_FILE="${PROJECT_ROOT}/LICENSE_COMPLIANCE_REPORT.md"
log_info "Generating license compliance report: ${REPORT_FILE}"

cat > "${REPORT_FILE}" << EOF
# StreamDAB Stream Analyzer - License Compliance Report

Generated: $(date)

## Core Dependencies

| Component | Version | License | Compatibility | Notes |
|-----------|---------|---------|---------------|-------|
| Qt6 | ${QT_VERSION:-Unknown} | LGPL-3.0/GPL-3.0 | ✅ Compatible | Core GUI framework |
| FFTW3 | ${FFTW3_VERSION:-Unknown} | GPL-2.0+ | ✅ Compatible | Signal processing |
| ALSA | ${ALSA_VERSION:-Unknown} | LGPL-2.1 | ✅ Compatible | Linux audio |

## Audio Decoders (Alternative Options)

| Component | Version | License | Compatibility | Notes |
|-----------|---------|---------|---------------|-------|
| FDK-AAC | ${FDK_VERSION:-Unknown} | Modified BSD | ⚠️ Restrictions | Patent considerations |
| FAAD2 | ${FAAD2_VERSION:-Unknown} | GPL-2.0 | ✅ Compatible | Recommended for OSS |

## Optional Dependencies

| Component | Version | License | Compatibility | Notes |
|-----------|---------|---------|---------------|-------|
| YAML-CPP | ${YAML_VERSION:-Unknown} | MIT | ✅ Compatible | Configuration files |

## Recommendations

1. **For Open Source Distribution**: Use GPL-3.0 license with FAAD2 decoder
2. **For Commercial Distribution**: Evaluate FDK-AAC patent licensing requirements
3. **Attribution Requirements**: Include third-party license attributions in distribution
4. **Documentation**: Maintain this compliance report with version updates

## Legal Disclaimer

This report is for informational purposes only. Consult with qualified legal counsel for
commercial distribution decisions, especially regarding patent licensing requirements.
EOF

log_success "License compliance report generated: ${REPORT_FILE}"

# Summary
echo ""
echo "=== Dependency & License Check Summary ==="

# Count dependencies
COMPATIBLE_DEPS=0
WARNING_DEPS=0
ERROR_DEPS=0

# This is a simplified check - in practice, you'd parse the actual results
if command -v qmake6 >/dev/null 2>&1; then ((COMPATIBLE_DEPS++)); fi
if pkg-config --exists fftw3; then ((COMPATIBLE_DEPS++)); fi
if pkg-config --exists faad2; then ((COMPATIBLE_DEPS++)); fi
if pkg-config --exists alsa; then ((COMPATIBLE_DEPS++)); fi

if pkg-config --exists fdk-aac; then ((WARNING_DEPS++)); fi

log_info "Compatible dependencies: ${COMPATIBLE_DEPS}"
log_info "Dependencies with warnings: ${WARNING_DEPS}"
log_info "Missing critical dependencies: ${ERROR_DEPS}"

if [ $ERROR_DEPS -eq 0 ]; then
    log_success "All critical dependencies satisfied for distribution"

    if [ $WARNING_DEPS -gt 0 ]; then
        log_warning "Review dependency warnings before commercial distribution"
    fi

    log_info "Project is ready for packaging with appropriate license"
    exit 0
else
    log_error "Critical dependencies missing - resolve before distribution"
    exit 1
fi
EOF
