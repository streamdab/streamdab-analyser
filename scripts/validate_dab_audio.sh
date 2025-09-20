#!/bin/bash
# validate_dab_audio.sh
# Validation script for DAB+ audio decoder support
#
# This script validates that the system has proper DAB+ audio decoding
# capabilities using either fdk-aac (preferred) or FAAD2 2.9.2+

set -e

echo "=== ETI Stream Analyzer - DAB+ Audio Decoder Validation ==="
echo "Validating DAB+ audio decoder support..."
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Track validation status
VALIDATION_PASSED=true
HAS_FDK_AAC=false
HAS_FAAD2=false
FAAD2_VERSION=""
FDK_AAC_VERSION=""

# Function to print status
print_status() {
    local status=$1
    local message=$2

    if [ "$status" = "OK" ]; then
        echo -e "${GREEN}✅ $message${NC}"
    elif [ "$status" = "WARNING" ]; then
        echo -e "${YELLOW}⚠️  $message${NC}"
    elif [ "$status" = "ERROR" ]; then
        echo -e "${RED}❌ $message${NC}"
        VALIDATION_PASSED=false
    elif [ "$status" = "INFO" ]; then
        echo -e "${BLUE}ℹ️  $message${NC}"
    fi
}

# Check for pkg-config
if ! command -v pkg-config &> /dev/null; then
    print_status "ERROR" "pkg-config not found. Required for dependency detection."
    exit 1
fi

print_status "OK" "pkg-config found"

# Check for fdk-aac (preferred)
echo
echo "Checking for fdk-aac (preferred for professional DAB+ decoding)..."
if pkg-config --exists fdk-aac; then
    FDK_AAC_VERSION=$(pkg-config --modversion fdk-aac)
    print_status "OK" "fdk-aac found: v${FDK_AAC_VERSION}"
    HAS_FDK_AAC=true

    # Check if version is suitable
    if pkg-config --atleast-version=2.0.0 fdk-aac; then
        print_status "OK" "fdk-aac version is suitable for professional DAB+ decoding"
    else
        print_status "WARNING" "fdk-aac version may be too old, recommend 2.0.0+"
    fi

    # Check development files
    if pkg-config --exists fdk-aac; then
        FDK_CFLAGS=$(pkg-config --cflags fdk-aac)
        FDK_LIBS=$(pkg-config --libs fdk-aac)
        print_status "OK" "fdk-aac development files available"
        print_status "INFO" "CFLAGS: $FDK_CFLAGS"
        print_status "INFO" "LIBS: $FDK_LIBS"
    fi
else
    print_status "WARNING" "fdk-aac not found via pkg-config"
fi

# Check for FAAD2 (fallback)
echo
echo "Checking for FAAD2 (fallback audio decoder)..."
if pkg-config --exists faad2; then
    FAAD2_VERSION=$(pkg-config --modversion faad2)
    print_status "OK" "FAAD2 found: v${FAAD2_VERSION}"
    HAS_FAAD2=true

    # Check if version meets requirement (2.9.2+)
    if pkg-config --atleast-version=2.9.2 faad2; then
        print_status "OK" "FAAD2 version ${FAAD2_VERSION} meets requirement (>= 2.9.2)"
    else
        print_status "ERROR" "FAAD2 version ${FAAD2_VERSION} is too old (require >= 2.9.2 for DAB+ support)"
        print_status "INFO" "Older FAAD2 versions have known bugs with DAB+ HE-AAC v2 decoding"
    fi

    # Check development files
    if pkg-config --exists faad2; then
        FAAD2_CFLAGS=$(pkg-config --cflags faad2)
        FAAD2_LIBS=$(pkg-config --libs faad2)
        print_status "OK" "FAAD2 development files available"
        print_status "INFO" "CFLAGS: $FAAD2_CFLAGS"
        print_status "INFO" "LIBS: $FAAD2_LIBS"
    fi
else
    print_status "WARNING" "FAAD2 not found via pkg-config"
fi

# Overall assessment
echo
echo "=== Overall Assessment ==="

if [ "$HAS_FDK_AAC" = true ]; then
    print_status "OK" "Recommended configuration: Using fdk-aac v${FDK_AAC_VERSION} for professional DAB+ decoding"
    print_status "INFO" "fdk-aac provides superior DAB+ HE-AAC v2 decoding quality"
elif [ "$HAS_FAAD2" = true ] && pkg-config --atleast-version=2.9.2 faad2; then
    print_status "OK" "Acceptable configuration: Using FAAD2 v${FAAD2_VERSION} for DAB+ decoding"
    print_status "INFO" "Consider installing fdk-aac for better professional-grade decoding"
elif [ "$HAS_FAAD2" = true ]; then
    print_status "WARNING" "Suboptimal configuration: FAAD2 v${FAAD2_VERSION} may have DAB+ decoding issues"
    print_status "INFO" "Recommend upgrading to FAAD2 2.9.2+ or installing fdk-aac"
else
    print_status "ERROR" "No suitable DAB+ audio decoder found"
    print_status "INFO" "Install either fdk-aac (recommended) or FAAD2 2.9.2+"
fi

# Installation recommendations
echo
echo "=== Installation Recommendations ==="

# Platform-specific recommendations
if command -v apt-get &> /dev/null; then
    # Debian/Ubuntu
    echo "For Debian/Ubuntu systems:"
    echo "  sudo apt-get install libfdk-aac-dev  # Preferred"
    echo "  sudo apt-get install libfaad-dev     # Alternative (check version >= 2.9.2)"
elif command -v yum &> /dev/null; then
    # RedHat/CentOS/Fedora
    echo "For RedHat/CentOS/Fedora systems:"
    echo "  sudo yum install fdk-aac-devel       # Preferred"
    echo "  sudo yum install faad2-devel         # Alternative (check version >= 2.9.2)"
elif command -v brew &> /dev/null; then
    # macOS with Homebrew
    echo "For macOS with Homebrew:"
    echo "  brew install fdk-aac                 # Preferred"
    echo "  brew install faad2                   # Alternative"
elif command -v pacman &> /dev/null; then
    # Arch Linux
    echo "For Arch Linux:"
    echo "  sudo pacman -S libfdk-aac            # Preferred"
    echo "  sudo pacman -S faad2                 # Alternative"
else
    echo "Install development packages for either:"
    echo "  - fdk-aac (recommended for professional DAB+ decoding)"
    echo "  - FAAD2 version 2.9.2 or newer"
fi

# Build system integration
echo
echo "=== Build System Integration ==="
print_status "INFO" "CMake will automatically detect and prefer fdk-aac over FAAD2"
print_status "INFO" "To force FAAD2 usage: cmake -DUSE_FDK_AAC=OFF .."
print_status "INFO" "Build will fail if no suitable audio decoder is found"

echo
if [ "$VALIDATION_PASSED" = true ]; then
    print_status "OK" "DAB+ audio decoder validation PASSED"
    echo -e "${GREEN}System is ready for professional DAB+ audio decoding${NC}"
    exit 0
else
    print_status "ERROR" "DAB+ audio decoder validation FAILED"
    echo -e "${RED}Please install required audio decoder libraries before building${NC}"
    exit 1
fi
