#!/bin/bash

# ============================================================================
# Phase 1 Unit Test Runner Script
# StreamDAB Analyser - ETI Processing and FIG Decoding Tests
# ============================================================================

set -e  # Exit on error

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="${SCRIPT_DIR}/../build"

echo "========================================"
echo "Phase 1: ETI Processing Unit Tests"
echo "========================================"
echo ""

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "ERROR: Build directory not found: $BUILD_DIR"
    echo "Please run: cd build && cmake .. && make"
    exit 1
fi

cd "$BUILD_DIR"

# Check if tests are built
TESTS=("test_eti_frame_parser" "test_fig0_processor" "test_fig1_processor")
MISSING_TESTS=()

for test in "${TESTS[@]}"; do
    if [ ! -f "$test" ]; then
        MISSING_TESTS+=("$test")
    fi
done

if [ ${#MISSING_TESTS[@]} -gt 0 ]; then
    echo "WARNING: Some tests are not built:"
    for test in "${MISSING_TESTS[@]}"; do
        echo "  - $test"
    done
    echo ""
    echo "Building Phase 1 tests..."
    make test_eti_frame_parser test_fig0_processor test_fig1_processor
    echo ""
fi

# Run tests with summary
echo "Running Phase 1 Tests..."
echo "----------------------------------------"
echo ""

TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

for test in "${TESTS[@]}"; do
    if [ -f "$test" ]; then
        echo "Running: $test"
        echo "----------------------------------------"

        if ./"$test"; then
            echo "✅ PASSED: $test"
            ((PASSED_TESTS++))
        else
            echo "❌ FAILED: $test"
            ((FAILED_TESTS++))
        fi

        ((TOTAL_TESTS++))
        echo ""
    fi
done

echo "========================================"
echo "Phase 1 Test Summary"
echo "========================================"
echo "Total tests:  $TOTAL_TESTS"
echo "Passed:       $PASSED_TESTS"
echo "Failed:       $FAILED_TESTS"
echo ""

if [ $FAILED_TESTS -eq 0 ]; then
    echo "✅ All Phase 1 tests PASSED!"
    echo ""
    echo "Coverage Target: >80% on new code"
    echo "Performance Target: >100 FPS parsing"
    echo "========================================"
    exit 0
else
    echo "❌ Some Phase 1 tests FAILED!"
    echo ""
    echo "Please fix failing tests before continuing."
    echo "========================================"
    exit 1
fi
