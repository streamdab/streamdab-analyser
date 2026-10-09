#!/bin/bash

# FIG 0/14 Integration Verification Script
# Agent 21 - Python Pro | PDCA Week 5
# Date: November 3, 2025

echo "========================================================================"
echo "FIG 0/14 FEC Sub-channel Organization - Integration Verification"
echo "========================================================================"
echo ""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "$PROJECT_ROOT" || exit 1

# Color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

check_passed=0
check_failed=0

# Function to print check result
print_check() {
    local name="$1"
    local status="$2"
    if [ "$status" = "PASS" ]; then
        echo -e "[${GREEN}✓${NC}] $name"
        ((check_passed++))
    else
        echo -e "[${RED}✗${NC}] $name"
        ((check_failed++))
    fi
}

echo "Checking deliverables..."
echo "------------------------"

# Check test file
if [ -f "tests/test_fig014_fec_subchannel.cpp" ]; then
    lines=$(wc -l < "tests/test_fig014_fec_subchannel.cpp")
    if [ "$lines" -gt 500 ]; then
        print_check "Test file exists ($lines lines)" "PASS"
    else
        print_check "Test file exists (but only $lines lines - expected 563)" "FAIL"
    fi
else
    print_check "Test file exists" "FAIL"
fi

# Check documentation files (skipped when the private documentation set is not
# part of the checkout, e.g. the public tree — only the source/tests matter there)
if [ -d docs ]; then
    for doc in "docs/fig/FIG014_QUICKSTART.md" "docs/archive/FIG014_CODE_PATCHES.txt" "docs/fig/FIG014_IMPLEMENTATION_GUIDE.md" \
               "docs/agents/AGENT21_FIG014_IMPLEMENTATION_COMPLETE.md" "docs/agents/AGENT21_FINAL_REPORT.md" "docs/agents/AGENT21_DELIVERY_SUMMARY.txt"; do
        if [ -f "$doc" ]; then
            print_check "Documentation: $doc" "PASS"
        else
            print_check "Documentation: $doc" "FAIL"
        fi
    done
else
    echo "  (documentation checks skipped: no docs/ directory in this checkout)"
fi

echo ""
echo "Checking source file integration..."
echo "-----------------------------------"

# Check if struct is added to header
if grep -q "struct FECSubchannelOrganization" src/core/fig_parser.hpp 2>/dev/null; then
    print_check "FECSubchannelOrganization struct in header" "PASS"
else
    print_check "FECSubchannelOrganization struct in header" "FAIL"
    echo -e "  ${YELLOW}→ Add struct to src/core/fig_parser.hpp (see docs/archive/FIG014_CODE_PATCHES.txt PATCH 1)${NC}"
fi

# Check if method declaration exists
if grep -q "parseFig014_FECSubchannelOrganization" src/core/fig_parser.hpp 2>/dev/null; then
    print_check "Method declaration in header" "PASS"
else
    print_check "Method declaration in header" "FAIL"
    echo -e "  ${YELLOW}→ Add method declaration (see docs/archive/FIG014_CODE_PATCHES.txt PATCH 2)${NC}"
fi

# Check if signal declaration exists
if grep -q "fecSubchannelOrganizationDiscovered" src/core/fig_parser.hpp 2>/dev/null; then
    print_check "Signal declaration in header" "PASS"
else
    print_check "Signal declaration in header" "FAIL"
    echo -e "  ${YELLOW}→ Add signal declaration (see docs/archive/FIG014_CODE_PATCHES.txt PATCH 3)${NC}"
fi

# Check if implementation exists
if grep -q "FigParser::parseFig014_FECSubchannelOrganization" src/core/fig_parser.cpp 2>/dev/null; then
    print_check "Implementation in cpp file" "PASS"
else
    print_check "Implementation in cpp file" "FAIL"
    echo -e "  ${YELLOW}→ Add implementation (see docs/archive/FIG014_CODE_PATCHES.txt PATCH 5)${NC}"
fi

# Check if case 14 is added
if grep -A2 "case 14:" src/core/fig_parser.cpp 2>/dev/null | grep -q "FEC sub-channel"; then
    print_check "Case 14 in parseFigType0()" "PASS"
else
    print_check "Case 14 in parseFigType0()" "FAIL"
    echo -e "  ${YELLOW}→ Add case 14 (see docs/archive/FIG014_CODE_PATCHES.txt PATCH 6)${NC}"
fi

# Check if test is registered in CMakeLists.txt
if grep -q "test_fig014_fec_subchannel" tests/CMakeLists.txt 2>/dev/null; then
    print_check "Test registered in CMakeLists.txt" "PASS"
else
    print_check "Test registered in CMakeLists.txt" "FAIL"
    echo -e "  ${YELLOW}→ Add test registration (see docs/archive/FIG014_CODE_PATCHES.txt PATCH 7)${NC}"
fi

echo ""
echo "Checking build status..."
echo "------------------------"

# Check if build directory exists
if [ -d "build" ]; then
    print_check "Build directory exists" "PASS"
    
    # Check if test executable exists
    if [ -f "build/test_fig014_fec_subchannel" ]; then
        print_check "Test executable built" "PASS"
        
        # Try to run the test
        echo ""
        echo -e "${YELLOW}Running tests...${NC}"
        echo "----------------"
        if cd build && ./test_fig014_fec_subchannel 2>&1 | tee /tmp/fig014_test_output.txt; then
            passed=$(grep -o "passed" /tmp/fig014_test_output.txt | wc -l)
            failed=$(grep -o "failed" /tmp/fig014_test_output.txt | wc -l)
            
            echo ""
            if [ "$failed" -eq 0 ] && [ "$passed" -gt 20 ]; then
                print_check "All tests passed ($passed passed, $failed failed)" "PASS"
            else
                print_check "Tests passed ($passed passed, $failed failed)" "FAIL"
            fi
            
            # Check performance
            if grep -q "Average parsing time:" /tmp/fig014_test_output.txt; then
                avg_time=$(grep "Average parsing time:" /tmp/fig014_test_output.txt | grep -oE '[0-9]+\.[0-9]+')
                if [ -n "$avg_time" ]; then
                    if (( $(echo "$avg_time < 18" | bc -l) )); then
                        print_check "Performance target met (${avg_time}µs < 18µs)" "PASS"
                    else
                        print_check "Performance target met (${avg_time}µs < 18µs)" "FAIL"
                    fi
                fi
            fi
        else
            print_check "Test execution" "FAIL"
        fi
        cd "$PROJECT_ROOT"
    else
        print_check "Test executable built" "FAIL"
        echo -e "  ${YELLOW}→ Run: cd build && cmake --build .${NC}"
    fi
else
    print_check "Build directory exists" "FAIL"
    echo -e "  ${YELLOW}→ Create build directory and run cmake${NC}"
fi

echo ""
echo "========================================================================"
echo "Summary"
echo "========================================================================"
echo -e "Checks passed: ${GREEN}$check_passed${NC}"
echo -e "Checks failed: ${RED}$check_failed${NC}"
echo ""

if [ "$check_failed" -eq 0 ]; then
    echo -e "${GREEN}✓ ALL CHECKS PASSED - FIG 0/14 INTEGRATION COMPLETE!${NC}"
    echo ""
    echo "Next steps:"
    echo "  1. Run full test suite: cd build && ctest"
    echo "  2. Commit changes: git add . && git commit -m 'feat: Add FIG 0/14 parser (Agent 21)'"
    echo "  3. Update CLAUDE.md with FIG 0/14 completion status"
    exit 0
elif [ "$check_failed" -le 3 ]; then
    echo -e "${YELLOW}⚠ PARTIAL INTEGRATION - See recommendations above${NC}"
    echo ""
    echo "Quick fix guide:"
    echo "  1. Review docs/archive/FIG014_CODE_PATCHES.txt for missing patches"
    echo "  2. Apply missing patches to source files"
    echo "  3. Run: cd build && cmake --build ."
    echo "  4. Re-run this script to verify"
    exit 1
else
    echo -e "${RED}✗ INTEGRATION NOT STARTED - Follow integration guide${NC}"
    echo ""
    echo "Start here:"
    echo "  1. Read docs/fig/FIG014_QUICKSTART.md"
    echo "  2. Apply patches from docs/archive/FIG014_CODE_PATCHES.txt"
    echo "  3. Build: cd build && cmake --build ."
    echo "  4. Re-run this script to verify"
    exit 2
fi
