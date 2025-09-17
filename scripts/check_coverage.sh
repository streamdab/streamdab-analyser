#!/bin/bash

# TDD Coverage Enforcement Script
# Enforces minimum code coverage requirement for TDD quality gates

set -e

# Configuration
COVERAGE_FILE=${COVERAGE_FILE:-"coverage_clean.info"}
MINIMUM_COVERAGE=${MINIMUM_COVERAGE:-80}
COVERAGE_HTML_DIR="coverage_html"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# TDD Status indicators
TDD_RED="🔴"
TDD_GREEN="🟢"
TDD_BLUE="🔵"

echo -e "${BLUE}=== TDD Coverage Enforcement ===${NC}"
echo -e "${BLUE}Minimum required coverage: ${MINIMUM_COVERAGE}%${NC}"
echo ""

# Check if coverage file exists
if [ ! -f "$COVERAGE_FILE" ]; then
    echo -e "${RED}${TDD_RED} ERROR: Coverage file '$COVERAGE_FILE' not found${NC}"
    echo -e "${RED}Run 'make tdd_coverage_generate' first${NC}"
    exit 1
fi

# Extract coverage percentage using lcov
echo -e "${BLUE}Analyzing coverage data...${NC}"
COVERAGE_OUTPUT=$(lcov --summary "$COVERAGE_FILE" 2>/dev/null)

# Parse coverage percentage from lcov output
# lcov outputs: "lines......: 85.5% (1234 of 1443 lines)"
COVERAGE_PERCENT=$(echo "$COVERAGE_OUTPUT" | grep -E "lines\.*:" | sed 's/.*: \([0-9.]*\)%.*/\1/')

if [ -z "$COVERAGE_PERCENT" ]; then
    echo -e "${RED}${TDD_RED} ERROR: Could not parse coverage percentage from lcov output${NC}"
    echo "lcov output:"
    echo "$COVERAGE_OUTPUT"
    exit 1
fi

# Convert to integer for comparison (remove decimal)
COVERAGE_INT=$(echo "$COVERAGE_PERCENT" | cut -d. -f1)

echo -e "${BLUE}Current coverage: ${COVERAGE_PERCENT}%${NC}"
echo ""

# TDD Quality Gate Enforcement
if (( $(echo "$COVERAGE_PERCENT >= $MINIMUM_COVERAGE" | bc -l) )); then
    echo -e "${GREEN}${TDD_GREEN} TDD QUALITY GATE: PASSED${NC}"
    echo -e "${GREEN}Coverage ${COVERAGE_PERCENT}% meets minimum requirement of ${MINIMUM_COVERAGE}%${NC}"

    # Generate detailed report
    if [ -d "$COVERAGE_HTML_DIR" ]; then
        echo -e "${GREEN}Coverage report available at: ${COVERAGE_HTML_DIR}/index.html${NC}"
    fi

    echo ""
    echo -e "${GREEN}${TDD_BLUE} TDD Status: Ready for REFACTOR phase${NC}"
    exit 0
else
    echo -e "${RED}${TDD_RED} TDD QUALITY GATE: FAILED${NC}"
    echo -e "${RED}Coverage ${COVERAGE_PERCENT}% is below minimum requirement of ${MINIMUM_COVERAGE}%${NC}"

    # Calculate how much more coverage is needed
    COVERAGE_DEFICIT=$(echo "$MINIMUM_COVERAGE - $COVERAGE_PERCENT" | bc -l)
    echo -e "${YELLOW}Additional coverage needed: ${COVERAGE_DEFICIT}%${NC}"

    echo ""
    echo -e "${RED}TDD REQUIREMENT: Write more tests to increase coverage${NC}"
    echo -e "${RED}Commit blocked until coverage requirement is met${NC}"

    # Show uncovered lines if available
    if command -v genhtml >/dev/null 2>&1 && [ -f "$COVERAGE_FILE" ]; then
        echo ""
        echo -e "${YELLOW}Generating detailed coverage report...${NC}"
        genhtml "$COVERAGE_FILE" --output-directory "$COVERAGE_HTML_DIR" --quiet
        echo -e "${YELLOW}Open ${COVERAGE_HTML_DIR}/index.html to see uncovered lines${NC}"
    fi

    echo ""
    echo -e "${RED}${TDD_RED} TDD Status: BLOCKED - Insufficient test coverage${NC}"
    exit 1
fi
