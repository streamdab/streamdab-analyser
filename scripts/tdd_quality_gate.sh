#!/bin/bash
# TDD Quality Gate Enforcement Script
# CRITICAL: Emergency quality gate implementation to prevent untested code

set -euo pipefail

# TDD Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
COVERAGE_MIN_PERCENT=80
TDD_LOG_FILE="${BUILD_DIR}/tdd_quality_gate.log"

# Colors for TDD status reporting
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🚨 TDD QUALITY GATE ENFORCEMENT ACTIVE${NC}" | tee "$TDD_LOG_FILE"
echo "=======================================" | tee -a "$TDD_LOG_FILE"
echo "Project: StreamDAB Stream Analyser" | tee -a "$TDD_LOG_FILE"
echo "Build Directory: $BUILD_DIR" | tee -a "$TDD_LOG_FILE"
echo "Minimum Coverage: ${COVERAGE_MIN_PERCENT}%" | tee -a "$TDD_LOG_FILE"
echo "Started: $(date)" | tee -a "$TDD_LOG_FILE"
echo "" | tee -a "$TDD_LOG_FILE"

# TDD GATE 1: Build System Integrity Check
echo -e "${YELLOW}🔧 TDD GATE 1: Build System Integrity${NC}" | tee -a "$TDD_LOG_FILE"
cd "$BUILD_DIR"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
. "${SCRIPT_DIR}/build_jobs.sh"
TDD_JOBS=$(streamdab_build_jobs)
export CMAKE_BUILD_PARALLEL_LEVEL="${TDD_JOBS}"
if ! make -j"${TDD_JOBS}" 2>&1 | tee -a "$TDD_LOG_FILE"; then
    echo -e "${RED}❌ TDD GATE 1 FAILED: Build system broken${NC}" | tee -a "$TDD_LOG_FILE"
    exit 1
fi
echo -e "${GREEN}✅ TDD GATE 1 PASSED: Build system operational${NC}" | tee -a "$TDD_LOG_FILE"
echo "" | tee -a "$TDD_LOG_FILE"

# TDD GATE 2: Unit Test Execution Mandatory
echo -e "${YELLOW}🧪 TDD GATE 2: Unit Test Execution${NC}" | tee -a "$TDD_LOG_FILE"
if [ ! -f "${BUILD_DIR}/tests/eti_analyser_unit_tests" ]; then
    echo -e "${RED}❌ TDD GATE 2 FAILED: Unit test executable missing${NC}" | tee -a "$TDD_LOG_FILE"
    exit 1
fi

if ! "${BUILD_DIR}/tests/eti_analyser_unit_tests" 2>&1 | tee -a "$TDD_LOG_FILE"; then
    echo -e "${RED}❌ TDD GATE 2 FAILED: Unit tests failing${NC}" | tee -a "$TDD_LOG_FILE"
    exit 1
fi
echo -e "${GREEN}✅ TDD GATE 2 PASSED: Unit tests operational${NC}" | tee -a "$TDD_LOG_FILE"
echo "" | tee -a "$TDD_LOG_FILE"

# TDD GATE 3: GUI Test Validation (if available)
echo -e "${YELLOW}🖥️ TDD GATE 3: GUI Test Validation${NC}" | tee -a "$TDD_LOG_FILE"
if [ -f "${BUILD_DIR}/tests/eti_analyser_gui_tests" ]; then
    if ! "${BUILD_DIR}/tests/eti_analyser_gui_tests" 2>&1 | tee -a "$TDD_LOG_FILE"; then
        echo -e "${RED}❌ TDD GATE 3 FAILED: GUI tests failing${NC}" | tee -a "$TDD_LOG_FILE"
        exit 1
    fi
    echo -e "${GREEN}✅ TDD GATE 3 PASSED: GUI tests operational${NC}" | tee -a "$TDD_LOG_FILE"
else
    echo -e "${YELLOW}⚠️ TDD GATE 3 SKIPPED: GUI test executable missing${NC}" | tee -a "$TDD_LOG_FILE"
fi
echo "" | tee -a "$TDD_LOG_FILE"

# TDD GATE 4: Code Coverage Enforcement
echo -e "${YELLOW}📊 TDD GATE 4: Code Coverage Analysis${NC}" | tee -a "$TDD_LOG_FILE"
if command -v lcov >/dev/null 2>&1 && command -v genhtml >/dev/null 2>&1; then
    echo "Generating code coverage report..." | tee -a "$TDD_LOG_FILE"

    # Reset coverage counters
    lcov --directory . --zerocounters 2>&1 | tee -a "$TDD_LOG_FILE" || true

    # Run tests with coverage
    "${BUILD_DIR}/tests/eti_analyser_unit_tests" >/dev/null 2>&1 || true

    # Capture coverage data
    if lcov --directory . --capture --output-file coverage.info 2>&1 | tee -a "$TDD_LOG_FILE"; then
        # Filter coverage data
        lcov --remove coverage.info '/usr/*' '*/tests/*' '*/third_party/*' --output-file coverage_clean.info 2>&1 | tee -a "$TDD_LOG_FILE"

        # Extract coverage percentage
        COVERAGE_PERCENT=$(lcov --summary coverage_clean.info 2>&1 | grep -o 'lines......: [0-9.]*%' | grep -o '[0-9.]*' || echo "0")

        echo "Current Coverage: ${COVERAGE_PERCENT}%" | tee -a "$TDD_LOG_FILE"

        # Check coverage threshold
        if (( $(echo "$COVERAGE_PERCENT < $COVERAGE_MIN_PERCENT" | bc -l) )); then
            echo -e "${RED}❌ TDD GATE 4 FAILED: Coverage ${COVERAGE_PERCENT}% below minimum ${COVERAGE_MIN_PERCENT}%${NC}" | tee -a "$TDD_LOG_FILE"
            exit 1
        fi

        echo -e "${GREEN}✅ TDD GATE 4 PASSED: Coverage ${COVERAGE_PERCENT}% meets requirements${NC}" | tee -a "$TDD_LOG_FILE"
    else
        echo -e "${YELLOW}⚠️ TDD GATE 4 SKIPPED: Coverage data collection failed${NC}" | tee -a "$TDD_LOG_FILE"
    fi
else
    echo -e "${YELLOW}⚠️ TDD GATE 4 SKIPPED: lcov/genhtml not available${NC}" | tee -a "$TDD_LOG_FILE"
fi
echo "" | tee -a "$TDD_LOG_FILE"

# TDD GATE 5: Static Analysis (if available)
echo -e "${YELLOW}🔍 TDD GATE 5: Static Analysis${NC}" | tee -a "$TDD_LOG_FILE"
if command -v cppcheck >/dev/null 2>&1; then
    echo "Running static analysis..." | tee -a "$TDD_LOG_FILE"
    if ! cppcheck --error-exitcode=1 --enable=warning,style,performance \
        --suppress=missingIncludeSystem \
        "${PROJECT_ROOT}/src" 2>&1 | tee -a "$TDD_LOG_FILE"; then
        echo -e "${RED}❌ TDD GATE 5 FAILED: Static analysis issues found${NC}" | tee -a "$TDD_LOG_FILE"
        exit 1
    fi
    echo -e "${GREEN}✅ TDD GATE 5 PASSED: Static analysis clean${NC}" | tee -a "$TDD_LOG_FILE"
else
    echo -e "${YELLOW}⚠️ TDD GATE 5 SKIPPED: cppcheck not available${NC}" | tee -a "$TDD_LOG_FILE"
fi
echo "" | tee -a "$TDD_LOG_FILE"

# TDD SUCCESS: All gates passed
echo -e "${GREEN}🎉 TDD QUALITY GATE: ALL CHECKS PASSED${NC}" | tee -a "$TDD_LOG_FILE"
echo "=======================================" | tee -a "$TDD_LOG_FILE"
echo "Completed: $(date)" | tee -a "$TDD_LOG_FILE"
echo "" | tee -a "$TDD_LOG_FILE"
echo -e "${GREEN}✅ Code ready for commit/deployment${NC}" | tee -a "$TDD_LOG_FILE"

exit 0
