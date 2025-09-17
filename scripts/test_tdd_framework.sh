#!/bin/bash

# TDD Framework Validation Script
# Validates that the TDD framework is properly installed and operational

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
SUCCESS="✅"
FAILURE="❌"
WARNING="⚠️"

echo ""
echo -e "${BLUE}===============================================${NC}"
echo -e "${BLUE}🧪 TDD FRAMEWORK VALIDATION 🧪${NC}"
echo -e "${BLUE}===============================================${NC}"
echo ""

# Initialize validation results
VALIDATION_PASSED=0
VALIDATION_FAILED=0
VALIDATION_WARNINGS=0

# Function to report test result
report_result() {
    local test_name="$1"
    local result="$2"
    local message="$3"

    if [[ "$result" == "PASS" ]]; then
        echo -e "${GREEN}${SUCCESS} ${test_name}: PASSED${NC}"
        if [[ -n "$message" ]]; then
            echo -e "${GREEN}   ${message}${NC}"
        fi
        ((VALIDATION_PASSED++))
    elif [[ "$result" == "FAIL" ]]; then
        echo -e "${RED}${FAILURE} ${test_name}: FAILED${NC}"
        if [[ -n "$message" ]]; then
            echo -e "${RED}   ${message}${NC}"
        fi
        ((VALIDATION_FAILED++))
    elif [[ "$result" == "WARN" ]]; then
        echo -e "${YELLOW}${WARNING} ${test_name}: WARNING${NC}"
        if [[ -n "$message" ]]; then
            echo -e "${YELLOW}   ${message}${NC}"
        fi
        ((VALIDATION_WARNINGS++))
    fi
}

# Check if we're in a git repository
echo -e "${BLUE}Checking project environment...${NC}"
if ! git rev-parse --git-dir > /dev/null 2>&1; then
    report_result "Git Repository" "FAIL" "Not in a git repository"
    exit 1
else
    report_result "Git Repository" "PASS" "Git repository detected"
fi

PROJECT_ROOT=$(git rev-parse --show-toplevel)
cd "$PROJECT_ROOT"

# Test 1: CMake Configuration
echo ""
echo -e "${BLUE}Testing CMake TDD configuration...${NC}"
if [[ -f "tests/CMakeLists.txt" ]]; then
    if grep -q "Google Test Framework" tests/CMakeLists.txt; then
        report_result "CMake TDD Config" "PASS" "TDD framework configured in CMake"
    else
        report_result "CMake TDD Config" "FAIL" "TDD framework not found in CMake"
    fi
else
    report_result "CMake TDD Config" "FAIL" "tests/CMakeLists.txt not found"
fi

# Test 2: Test Directory Structure
echo ""
echo -e "${BLUE}Validating test directory structure...${NC}"
required_dirs=("tests/fixtures" "tests/mocks" "tests/benchmarks" "tests/integration")
missing_dirs=()

for dir in "${required_dirs[@]}"; do
    if [[ -d "$dir" ]]; then
        report_result "Directory: $dir" "PASS"
    else
        report_result "Directory: $dir" "FAIL" "Required directory missing"
        missing_dirs+=("$dir")
    fi
done

if [[ ${#missing_dirs[@]} -eq 0 ]]; then
    report_result "Test Directory Structure" "PASS" "All required directories present"
else
    report_result "Test Directory Structure" "FAIL" "Missing directories: ${missing_dirs[*]}"
fi

# Test 3: TDD Scripts
echo ""
echo -e "${BLUE}Checking TDD scripts...${NC}"
required_scripts=("scripts/check_coverage.sh" "scripts/tdd_pre_commit.sh" "scripts/install_tdd_hooks.sh")
missing_scripts=()

for script in "${required_scripts[@]}"; do
    if [[ -f "$script" ]] && [[ -x "$script" ]]; then
        report_result "Script: $(basename $script)" "PASS" "Executable and present"
    elif [[ -f "$script" ]]; then
        report_result "Script: $(basename $script)" "WARN" "Present but not executable"
        chmod +x "$script"
    else
        report_result "Script: $(basename $script)" "FAIL" "Script missing"
        missing_scripts+=("$script")
    fi
done

# Test 4: Test Data and Fixtures
echo ""
echo -e "${BLUE}Validating test data and fixtures...${NC}"
if [[ -f "tests/fixtures/eti_streams/sample_eti_frame.h" ]]; then
    report_result "ETI Test Data" "PASS" "ETI test fixtures available"
else
    report_result "ETI Test Data" "FAIL" "ETI test fixtures missing"
fi

if [[ -f "tests/mocks/mock_eti_processor.h" ]]; then
    report_result "Mock Objects" "PASS" "Google Mock objects available"
else
    report_result "Mock Objects" "FAIL" "Mock objects missing"
fi

# Test 5: CI/CD Configuration
echo ""
echo -e "${BLUE}Checking CI/CD configuration...${NC}"
if [[ -f ".github/workflows/tdd_ci.yml" ]]; then
    if grep -q "TDD Continuous Integration" .github/workflows/tdd_ci.yml; then
        report_result "CI/CD Pipeline" "PASS" "TDD CI/CD workflow configured"
    else
        report_result "CI/CD Pipeline" "WARN" "CI workflow exists but may not be TDD-configured"
    fi
else
    report_result "CI/CD Pipeline" "WARN" "GitHub Actions workflow not found"
fi

if [[ -f ".github/workflows/tdd_nightly.yml" ]]; then
    report_result "Nightly Testing" "PASS" "Nightly TDD workflow configured"
else
    report_result "Nightly Testing" "WARN" "Nightly testing workflow not found"
fi

# Test 6: Build System Test
echo ""
echo -e "${BLUE}Testing build system...${NC}"
BUILD_DIR="build_test"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo -e "${BLUE}Running CMake configuration test...${NC}"
if cmake .. -DENABLE_COVERAGE=ON >/dev/null 2>&1; then
    report_result "CMake Configuration" "PASS" "CMake configures successfully with TDD options"

    # Test build
    echo -e "${BLUE}Testing build compilation...${NC}"
    if make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) >/dev/null 2>&1; then
        report_result "Build Compilation" "PASS" "Project builds successfully"

        # Test if test executables exist
        if [[ -f "tests/eti_analyzer_unit_tests" ]] || [[ -f "tests/eti_analyzer_unit_tests.exe" ]]; then
            report_result "Unit Test Executable" "PASS" "Unit test executable built"
        else
            report_result "Unit Test Executable" "FAIL" "Unit test executable not found"
        fi

        if [[ -f "tests/eti_analyzer_gui_tests" ]] || [[ -f "tests/eti_analyzer_gui_tests.exe" ]]; then
            report_result "GUI Test Executable" "PASS" "GUI test executable built"
        else
            report_result "GUI Test Executable" "FAIL" "GUI test executable not found"
        fi

    else
        report_result "Build Compilation" "FAIL" "Build failed"
    fi

else
    report_result "CMake Configuration" "FAIL" "CMake configuration failed"
fi

cd "$PROJECT_ROOT"
rm -rf "$BUILD_DIR"

# Test 7: Git Hooks
echo ""
echo -e "${BLUE}Checking Git hooks installation...${NC}"
if [[ -f ".git/hooks/pre-commit" ]]; then
    if grep -q "TDD" .git/hooks/pre-commit; then
        report_result "Pre-commit Hook" "PASS" "TDD pre-commit hook installed"
    else
        report_result "Pre-commit Hook" "WARN" "Pre-commit hook exists but may not be TDD-configured"
    fi
else
    report_result "Pre-commit Hook" "WARN" "TDD pre-commit hook not installed"
    echo -e "${YELLOW}   Run: ./scripts/install_tdd_hooks.sh${NC}"
fi

# Test 8: Dependencies Check
echo ""
echo -e "${BLUE}Checking TDD dependencies...${NC}"

# Check for Google Test
if pkg-config --exists gtest 2>/dev/null || find /usr -name "gtest.h" 2>/dev/null | head -1 | grep -q .; then
    report_result "Google Test" "PASS" "Google Test available"
else
    report_result "Google Test" "WARN" "Google Test may not be installed"
fi

# Check for lcov (coverage)
if command -v lcov >/dev/null 2>&1; then
    report_result "Coverage Tools" "PASS" "lcov available for coverage reporting"
else
    report_result "Coverage Tools" "WARN" "lcov not found - coverage reporting may not work"
fi

# Check for Qt Test
if pkg-config --exists Qt6Test 2>/dev/null; then
    report_result "Qt Test Framework" "PASS" "Qt Test framework available"
else
    report_result "Qt Test Framework" "WARN" "Qt Test framework may not be available"
fi

# Test 9: Documentation
echo ""
echo -e "${BLUE}Checking TDD documentation...${NC}"
if [[ -f "TDD_FRAMEWORK_COMPLETE.md" ]]; then
    report_result "TDD Documentation" "PASS" "TDD framework documentation available"
else
    report_result "TDD Documentation" "WARN" "TDD documentation not found"
fi

# Test 10: Validation Summary
echo ""
echo -e "${BLUE}===============================================${NC}"
echo -e "${BLUE}🧪 TDD FRAMEWORK VALIDATION SUMMARY 🧪${NC}"
echo -e "${BLUE}===============================================${NC}"
echo ""

total_tests=$((VALIDATION_PASSED + VALIDATION_FAILED + VALIDATION_WARNINGS))

echo -e "${GREEN}✅ Passed: ${VALIDATION_PASSED}${NC}"
echo -e "${RED}❌ Failed: ${VALIDATION_FAILED}${NC}"
echo -e "${YELLOW}⚠️  Warnings: ${VALIDATION_WARNINGS}${NC}"
echo -e "${BLUE}📊 Total Tests: ${total_tests}${NC}"

echo ""

# Calculate success percentage
if [[ $total_tests -gt 0 ]]; then
    success_percentage=$(( (VALIDATION_PASSED * 100) / total_tests ))
    echo -e "${BLUE}Success Rate: ${success_percentage}%${NC}"

    if [[ $VALIDATION_FAILED -eq 0 ]]; then
        if [[ $VALIDATION_WARNINGS -eq 0 ]]; then
            echo -e "${GREEN}🎉 TDD FRAMEWORK: FULLY OPERATIONAL${NC}"
            echo -e "${GREEN}Ready for Test-Driven Development!${NC}"
        else
            echo -e "${YELLOW}🟡 TDD FRAMEWORK: OPERATIONAL WITH WARNINGS${NC}"
            echo -e "${YELLOW}Address warnings for optimal TDD experience${NC}"
        fi
    else
        echo -e "${RED}🔴 TDD FRAMEWORK: INCOMPLETE${NC}"
        echo -e "${RED}Fix failed tests before proceeding with development${NC}"
    fi
else
    echo -e "${RED}❌ No tests executed${NC}"
fi

echo ""

# Provide next steps
if [[ $VALIDATION_FAILED -gt 0 ]]; then
    echo -e "${BLUE}Next Steps:${NC}"
    echo -e "${BLUE}1. Fix failed validation tests${NC}"
    echo -e "${BLUE}2. Install missing dependencies${NC}"
    echo -e "${BLUE}3. Re-run validation: ./scripts/test_tdd_framework.sh${NC}"
    exit 1
elif [[ $VALIDATION_WARNINGS -gt 0 ]]; then
    echo -e "${BLUE}Recommended Actions:${NC}"
    echo -e "${BLUE}1. Install TDD git hooks: ./scripts/install_tdd_hooks.sh${NC}"
    echo -e "${BLUE}2. Install missing optional dependencies${NC}"
    echo -e "${BLUE}3. Review warnings and optimize setup${NC}"
    exit 0
else
    echo -e "${GREEN}🚀 TDD FRAMEWORK READY FOR DEVELOPMENT!${NC}"
    echo ""
    echo -e "${GREEN}Start developing with TDD:${NC}"
    echo -e "${GREEN}1. Write failing tests first (RED phase)${NC}"
    echo -e "${GREEN}2. Implement minimal code to pass (GREEN phase)${NC}"
    echo -e "${GREEN}3. Refactor and improve (REFACTOR phase)${NC}"
    echo ""
    echo -e "${GREEN}Quality gates will automatically enforce TDD compliance!${NC}"
    exit 0
fi
