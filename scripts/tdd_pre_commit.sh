#!/bin/bash

# TDD Pre-Commit Hook
# Enforces Test-Driven Development quality gates before allowing commits

set -e

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

echo ""
echo -e "${BLUE}======================================${NC}"
echo -e "${BLUE}🧪 TDD QUALITY GATE ENFORCEMENT 🧪${NC}"
echo -e "${BLUE}======================================${NC}"
echo ""

# Check if we're in a git repository
if ! git rev-parse --git-dir > /dev/null 2>&1; then
    echo -e "${RED}${TDD_RED} ERROR: Not in a git repository${NC}"
    exit 1
fi

# Get the project root directory
PROJECT_ROOT=$(git rev-parse --show-toplevel)
cd "$PROJECT_ROOT"

# Configuration
BUILD_DIR="build"
MINIMUM_COVERAGE=80
COVERAGE_FILE="coverage_clean.info"

echo -e "${BLUE}TDD Phase 1: Building project with tests...${NC}"

# Ensure build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Creating build directory...${NC}"
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure with testing enabled
echo -e "${BLUE}Configuring CMake with TDD framework...${NC}"
if ! cmake .. -DENABLE_COVERAGE=ON; then
    echo -e "${RED}${TDD_RED} TDD GATE FAILED: CMake configuration failed${NC}"
    echo -e "${RED}Fix build configuration before committing${NC}"
    exit 1
fi

# Build the project
echo -e "${BLUE}Building project...${NC}"
if ! make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4); then
    echo -e "${RED}${TDD_RED} TDD GATE FAILED: Build failed${NC}"
    echo -e "${RED}Fix compilation errors before committing${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}${TDD_GREEN} BUILD SUCCESSFUL${NC}"
echo ""

# TDD Phase 2: Run all tests
echo -e "${BLUE}TDD Phase 2: Running all tests...${NC}"

# Run unit tests
echo -e "${BLUE}Running unit tests...${NC}"
if ! make run_unit_tests_only; then
    echo -e "${RED}${TDD_RED} TDD GATE FAILED: Unit tests failed${NC}"
    echo -e "${RED}All tests must pass before committing${NC}"
    echo ""
    echo -e "${RED}TDD REQUIREMENT: Fix failing tests${NC}"
    echo -e "${RED}Remember: Red → Green → Refactor${NC}"
    exit 1
fi

# Run GUI tests
echo -e "${BLUE}Running GUI tests...${NC}"
if ! make run_gui_tests_only; then
    echo -e "${RED}${TDD_RED} TDD GATE FAILED: GUI tests failed${NC}"
    echo -e "${RED}All tests must pass before committing${NC}"
    echo ""
    echo -e "${RED}TDD REQUIREMENT: Fix failing GUI tests${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}${TDD_GREEN} ALL TESTS PASSED${NC}"
echo ""

# TDD Phase 3: Code coverage enforcement
echo -e "${BLUE}TDD Phase 3: Enforcing code coverage requirements...${NC}"

# Generate coverage report
echo -e "${BLUE}Generating coverage report...${NC}"
if ! make tdd_coverage_generate; then
    echo -e "${RED}${TDD_RED} TDD GATE FAILED: Coverage generation failed${NC}"
    echo -e "${RED}Ensure all tests ran successfully${NC}"
    exit 1
fi

# Check coverage requirements
echo -e "${BLUE}Checking coverage requirements...${NC}"
if ! make tdd_coverage_check; then
    echo -e "${RED}${TDD_RED} TDD GATE FAILED: Insufficient code coverage${NC}"
    echo -e "${RED}Minimum required: ${MINIMUM_COVERAGE}%${NC}"
    echo ""
    echo -e "${RED}TDD REQUIREMENT: Write more tests to increase coverage${NC}"
    echo -e "${RED}Open coverage_html/index.html to see uncovered lines${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}${TDD_GREEN} COVERAGE REQUIREMENTS MET${NC}"
echo ""

# TDD Phase 4: Code quality checks
echo -e "${BLUE}TDD Phase 4: Code quality validation...${NC}"

# Check for TODO/FIXME comments in committed code
TODOS=$(git diff --cached --name-only | xargs grep -l "TODO\|FIXME\|XXX" 2>/dev/null || true)
if [ ! -z "$TODOS" ]; then
    echo -e "${YELLOW}⚠️  WARNING: Found TODO/FIXME comments in staged files:${NC}"
    echo "$TODOS"
    echo ""
    echo -e "${YELLOW}Consider resolving these before committing${NC}"
fi

# Check for debug print statements
DEBUG_PRINTS=$(git diff --cached | grep -E "^\+.*std::cout|^\+.*printf|^\+.*qDebug" || true)
if [ ! -z "$DEBUG_PRINTS" ]; then
    echo -e "${YELLOW}⚠️  WARNING: Found debug print statements in staged changes${NC}"
    echo "$DEBUG_PRINTS"
    echo ""
    echo -e "${YELLOW}Consider removing debug prints before committing${NC}"
fi

# TDD Phase 5: Validate TDD cycle compliance
echo -e "${BLUE}TDD Phase 5: Validating TDD cycle compliance...${NC}"

# Check if tests were added/modified with the code changes
CODE_CHANGES=$(git diff --cached --name-only | grep -E '\.(cpp|h)$' | grep -v test | wc -l)
TEST_CHANGES=$(git diff --cached --name-only | grep -E 'test.*\.(cpp|h)$' | wc -l)

if [ "$CODE_CHANGES" -gt 0 ] && [ "$TEST_CHANGES" -eq 0 ]; then
    echo -e "${YELLOW}⚠️  TDD COMPLIANCE WARNING:${NC}"
    echo -e "${YELLOW}Code changes detected without corresponding test changes${NC}"
    echo -e "${YELLOW}TDD requires tests to be written first (Red phase)${NC}"
    echo ""
    echo -e "${YELLOW}Files with code changes: $CODE_CHANGES${NC}"
    echo -e "${YELLOW}Files with test changes: $TEST_CHANGES${NC}"
    echo ""
    # This is a warning, not a blocker, to allow for refactoring
fi

# Final TDD validation
echo ""
echo -e "${GREEN}======================================${NC}"
echo -e "${GREEN}${TDD_GREEN} TDD QUALITY GATES: ALL PASSED${NC}"
echo -e "${GREEN}======================================${NC}"
echo ""
echo -e "${GREEN}✅ Build successful${NC}"
echo -e "${GREEN}✅ All tests passing${NC}"
echo -e "${GREEN}✅ Code coverage ≥ ${MINIMUM_COVERAGE}%${NC}"
echo -e "${GREEN}✅ Code quality validated${NC}"
echo ""
echo -e "${GREEN}${TDD_BLUE} TDD Status: Ready for commit${NC}"
echo -e "${GREEN}Commit allowed - TDD cycle maintained${NC}"
echo ""

exit 0
