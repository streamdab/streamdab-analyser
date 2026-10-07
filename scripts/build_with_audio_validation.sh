#!/bin/bash
# build_with_audio_validation.sh
# Enhanced build script with DAB+ audio decoder validation
#
# This script validates audio decoder dependencies before building
# and provides detailed feedback on the audio decoding configuration

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "=== ETI Stream Analyser - Enhanced Build with Audio Validation ==="
echo "Project: $PROJECT_ROOT"
echo "Build Directory: $BUILD_DIR"
echo

# Step 1: DAB+ audio decoder support
# Decoder availability (fdk-aac / FAAD2) is detected during CMake configuration
# (Step 3) and reported in Step 4, so there is no separate pre-build validator.
echo "Step 1: DAB+ audio decoder support is detected during CMake configuration."
echo

# Step 2: Create/clean build directory
echo "Step 2: Preparing build directory..."
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo -e "${GREEN}✅ Build directory prepared${NC}"
echo

# Step 3: Configure with CMake
echo "Step 3: Configuring with CMake..."
echo "Detecting audio decoder configuration..."

# Try to determine which audio decoder will be used
USE_FDK_AAC="ON"
if ! pkg-config --exists fdk-aac; then
    USE_FDK_AAC="OFF"
    echo -e "${YELLOW}fdk-aac not found, will use FAAD2${NC}"
else
    echo -e "${GREEN}fdk-aac found, will use fdk-aac (recommended)${NC}"
fi

# Configure with appropriate options
CMAKE_OPTIONS=(
    -DCMAKE_BUILD_TYPE=Release
    -DUSE_FDK_AAC=$USE_FDK_AAC
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    -DENABLE_TESTING=ON
)

echo "CMake options: ${CMAKE_OPTIONS[*]}"
echo

if ! cmake "${CMAKE_OPTIONS[@]}" "$PROJECT_ROOT"; then
    echo -e "${RED}❌ CMake configuration failed${NC}"
    exit 1
fi

echo -e "${GREEN}✅ CMake configuration completed${NC}"
echo

# Step 4: Verify audio decoder configuration
echo "Step 4: Verifying audio decoder configuration..."

# Check CMake cache for audio decoder settings
if [ -f "CMakeCache.txt" ]; then
    echo "Audio decoder configuration from CMake cache:"

    # Check which audio decoder was selected
    if grep -q "USE_FDK_AAC:BOOL=ON" CMakeCache.txt; then
        echo -e "${GREEN}✅ Using fdk-aac for professional DAB+ decoding${NC}"

        # Show fdk-aac details
        if grep -q "FDK_AAC_VERSION" CMakeCache.txt; then
            FDK_VERSION=$(grep "FDK_AAC_VERSION" CMakeCache.txt | cut -d'=' -f2)
            echo "   fdk-aac version: $FDK_VERSION"
        fi
    elif grep -q "USE_FDK_AAC:BOOL=OFF" CMakeCache.txt; then
        echo -e "${YELLOW}⚠️  Using FAAD2 for DAB+ decoding${NC}"

        # Show FAAD2 details
        if grep -q "FAAD2_VERSION" CMakeCache.txt; then
            FAAD2_VERSION=$(grep "FAAD2_VERSION" CMakeCache.txt | cut -d'=' -f2)
            echo "   FAAD2 version: $FAAD2_VERSION"

            # Warn if version is questionable
            if ! pkg-config --atleast-version=2.9.2 faad2 2>/dev/null; then
                echo -e "${YELLOW}   Warning: FAAD2 version may have DAB+ compatibility issues${NC}"
            fi
        fi
    fi

    # Show additional audio decoder info
    echo "Audio decoder libraries:"
    grep "AUDIO_DECODER_LIBS" CMakeCache.txt | cut -d'=' -f2 || echo "   Not found in cache"

    echo "Audio decoder include directories:"
    grep "AUDIO_DECODER_INCLUDE_DIRS" CMakeCache.txt | cut -d'=' -f2 || echo "   Not found in cache"
fi

echo

# Step 5: Build the project
echo "Step 5: Building the project..."

# Determine number of parallel jobs
# RAM/CPU-bounded parallelism (see scripts/build_jobs.sh)
. "${SCRIPT_DIR}/build_jobs.sh"
NPROC=$(streamdab_build_jobs)
export CMAKE_BUILD_PARALLEL_LEVEL="${NPROC}"
echo "Building with $NPROC parallel jobs..."

if ! cmake --build . --parallel $NPROC; then
    echo -e "${RED}❌ Build failed${NC}"
    echo
    echo "Build failed. Check the error messages above."
    echo "Common issues:"
    echo "  - Missing audio decoder development headers"
    echo "  - Incompatible audio decoder version"
    echo "  - Missing other dependencies (Qt6, FFTW3, etc.)"
    exit 1
fi

echo -e "${GREEN}✅ Build completed successfully${NC}"
echo

# Step 6: Verify the built executable
echo "Step 6: Verifying built executable..."

EXECUTABLE="$BUILD_DIR/ETIStreamAnalyser"
if [ -f "$EXECUTABLE" ]; then
    echo -e "${GREEN}✅ Executable created: $EXECUTABLE${NC}"

    # Show executable info
    ls -lh "$EXECUTABLE"

    # Check dependencies (Linux/macOS)
    if command -v ldd &> /dev/null; then
        echo
        echo "Checking audio decoder dependencies (ldd):"
        ldd "$EXECUTABLE" | grep -E "(faad|fdk-aac)" || echo "   Audio decoder libraries not shown in ldd output"
    elif command -v otool &> /dev/null; then
        echo
        echo "Checking audio decoder dependencies (otool):"
        otool -L "$EXECUTABLE" | grep -E "(faad|fdk-aac)" || echo "   Audio decoder libraries not shown in otool output"
    fi

else
    echo -e "${RED}❌ Executable not found: $EXECUTABLE${NC}"
    exit 1
fi

# Step 7: Run basic tests if available
echo
echo "Step 7: Running basic tests..."

if [ -f "$BUILD_DIR/tests/core_tests" ]; then
    echo "Running core tests..."
    if ! "$BUILD_DIR/tests/core_tests"; then
        echo -e "${YELLOW}⚠️  Some tests failed, but build completed${NC}"
    else
        echo -e "${GREEN}✅ Core tests passed${NC}"
    fi
else
    echo "No test executable found, skipping tests"
fi

# Final summary
echo
echo "=== Build Summary ==="
echo -e "${GREEN}✅ ETI Stream Analyser build completed successfully${NC}"
echo
echo "Executable: $EXECUTABLE"

# Show final audio configuration
if grep -q "USE_FDK_AAC:BOOL=ON" "$BUILD_DIR/CMakeCache.txt" 2>/dev/null; then
    echo -e "Audio Decoder: ${GREEN}fdk-aac (professional grade)${NC}"
else
    echo -e "Audio Decoder: ${YELLOW}FAAD2 (standard)${NC}"
fi

echo
echo "Next steps:"
echo "  1. Test the application: $EXECUTABLE"
echo "  2. Install: make install (from build directory)"
echo "  3. Package: cpack (from build directory)"
echo
echo -e "${GREEN}Build completed successfully!${NC}"
