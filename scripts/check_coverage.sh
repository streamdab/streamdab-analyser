#!/bin/bash
# Coverage Quality Gate Script
# Enforces TDD coverage requirements for ETI Stream Analyser

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/build"

# Configuration
MINIMUM_COVERAGE=${MINIMUM_COVERAGE:-80}
COVERAGE_FILE=${COVERAGE_FILE:-"${BUILD_DIR}/coverage_clean.info"}
COVERAGE_HTML_DIR="${BUILD_DIR}/coverage_html"
REPORT_FILE="${BUILD_DIR}/coverage_report.json"

echo "=== TDD Coverage Quality Gate ==="
echo "Project: ETI Stream Analyser"
echo "Minimum Required Coverage: ${MINIMUM_COVERAGE}%"
echo "Coverage Data File: ${COVERAGE_FILE}"
echo "==============================="

# Check if coverage file exists
if [[ ! -f "$COVERAGE_FILE" ]]; then
    echo "❌ ERROR: Coverage file not found: $COVERAGE_FILE"
    echo "Run 'make tdd_coverage_generate' first to generate coverage data"
    exit 1
fi

# Extract coverage data using lcov
echo "📊 Analyzing coverage data..."

# Generate summary
COVERAGE_SUMMARY=$(lcov --summary "$COVERAGE_FILE" 2>/dev/null | grep -E "lines\.*:" | tail -1)

if [[ -z "$COVERAGE_SUMMARY" ]]; then
    echo "❌ ERROR: Failed to extract coverage summary from $COVERAGE_FILE"
    exit 1
fi

# Parse coverage percentage
COVERAGE_PERCENT=$(echo "$COVERAGE_SUMMARY" | grep -oE '[0-9]+\.[0-9]+%' | head -1 | sed 's/%//')

if [[ -z "$COVERAGE_PERCENT" ]]; then
    echo "❌ ERROR: Failed to parse coverage percentage"
    echo "Coverage summary: $COVERAGE_SUMMARY"
    exit 1
fi

# Parse line counts
LINES_COVERED=$(echo "$COVERAGE_SUMMARY" | grep -oE '[0-9]+ of [0-9]+' | head -1 | cut -d' ' -f1)
LINES_TOTAL=$(echo "$COVERAGE_SUMMARY" | grep -oE '[0-9]+ of [0-9]+' | head -1 | cut -d' ' -f3)

echo "📈 Coverage Results:"
echo "   Lines Covered: $LINES_COVERED"
echo "   Total Lines: $LINES_TOTAL"
echo "   Coverage Percentage: $COVERAGE_PERCENT%"
echo "   Required Minimum: $MINIMUM_COVERAGE%"

# Generate detailed report by component
echo "📋 Generating detailed coverage report..."

# Create JSON report
cat > "$REPORT_FILE" << EOF
{
    "timestamp": "$(date -Iseconds)",
    "project": "ETI Stream Analyser",
    "coverage_summary": {
        "lines_covered": $LINES_COVERED,
        "lines_total": $LINES_TOTAL,
        "percentage": $COVERAGE_PERCENT,
        "minimum_required": $MINIMUM_COVERAGE,
        "passed": $(awk "BEGIN {print ($COVERAGE_PERCENT >= $MINIMUM_COVERAGE) ? \"true\" : \"false\"}")
    },
    "component_coverage": {
EOF

# Extract per-file coverage data
echo "        \"files\": [" >> "$REPORT_FILE"

# Parse lcov file for individual file coverage
while IFS= read -r line; do
    if [[ $line == SF:* ]]; then
        # Source file line
        current_file=$(echo "$line" | sed 's/SF://')
        current_file_relative=$(echo "$current_file" | sed "s|${PROJECT_ROOT}/||")
    elif [[ $line == LH:* ]]; then
        # Lines hit
        lines_hit=$(echo "$line" | sed 's/LH://')
    elif [[ $line == LF:* ]]; then
        # Lines found
        lines_found=$(echo "$line" | sed 's/LF://')

        # Calculate percentage for this file
        if [[ $lines_found -gt 0 ]]; then
            file_percentage=$(awk "BEGIN {printf \"%.1f\", ($lines_hit / $lines_found) * 100}")
        else
            file_percentage="0.0"
        fi

        # Only include source files from our project (not dependencies)
        if [[ $current_file_relative == src/* ]]; then
            cat >> "$REPORT_FILE" << EOF
            {
                "file": "$current_file_relative",
                "lines_hit": $lines_hit,
                "lines_found": $lines_found,
                "percentage": $file_percentage
            },
EOF
        fi
    fi
done < "$COVERAGE_FILE"

# Remove trailing comma and close JSON
sed -i '$ s/,$//' "$REPORT_FILE"
cat >> "$REPORT_FILE" << EOF
        ]
    }
}
EOF

echo "💾 Coverage report saved to: $REPORT_FILE"

# Component-specific analysis
echo ""
echo "🔍 Component-specific Coverage Analysis:"

# Core components (critical)
core_coverage=$(lcov --extract "$COVERAGE_FILE" "*/src/core/*" --output-file /tmp/core_coverage.info 2>/dev/null && lcov --summary /tmp/core_coverage.info 2>/dev/null | grep -oE '[0-9]+\.[0-9]+%' | head -1 | sed 's/%//' || echo "0")
gui_coverage=$(lcov --extract "$COVERAGE_FILE" "*/src/gui/*" --output-file /tmp/gui_coverage.info 2>/dev/null && lcov --summary /tmp/gui_coverage.info 2>/dev/null | grep -oE '[0-9]+\.[0-9]+%' | head -1 | sed 's/%//' || echo "0")
utils_coverage=$(lcov --extract "$COVERAGE_FILE" "*/src/utils/*" --output-file /tmp/utils_coverage.info 2>/dev/null && lcov --summary /tmp/utils_coverage.info 2>/dev/null | grep -oE '[0-9]+\.[0-9]+%' | head -1 | sed 's/%//' || echo "0")

echo "   📁 Core Components: ${core_coverage}%"
echo "   🖥️  GUI Components: ${gui_coverage}%"
echo "   🔧 Utilities: ${utils_coverage}%"

# Quality gates by component
echo ""
echo "🚦 Quality Gate Analysis:"

# Overall coverage gate
if awk "BEGIN {exit !($COVERAGE_PERCENT >= $MINIMUM_COVERAGE)}"; then
    echo "   ✅ Overall Coverage: PASSED (${COVERAGE_PERCENT}% >= ${MINIMUM_COVERAGE}%)"
    overall_passed=true
else
    echo "   ❌ Overall Coverage: FAILED (${COVERAGE_PERCENT}% < ${MINIMUM_COVERAGE}%)"
    overall_passed=false
fi

# Core component gate (critical - requires 85%)
core_minimum=85
if awk "BEGIN {exit !($core_coverage >= $core_minimum)}"; then
    echo "   ✅ Core Components: PASSED (${core_coverage}% >= ${core_minimum}%)"
    core_passed=true
else
    echo "   ⚠️  Core Components: REVIEW NEEDED (${core_coverage}% < ${core_minimum}%)"
    core_passed=false
fi

# GUI component gate (requires 70%)
gui_minimum=70
if awk "BEGIN {exit !($gui_coverage >= $gui_minimum)}"; then
    echo "   ✅ GUI Components: PASSED (${gui_coverage}% >= ${gui_minimum}%)"
    gui_passed=true
else
    echo "   ⚠️  GUI Components: REVIEW NEEDED (${gui_coverage}% < ${gui_minimum}%)"
    gui_passed=false
fi

# Generate HTML report if genhtml is available
if command -v genhtml &> /dev/null; then
    echo ""
    echo "🌐 Generating HTML coverage report..."
    genhtml "$COVERAGE_FILE" --output-directory "$COVERAGE_HTML_DIR" --title "ETI Stream Analyser Coverage Report" --show-details --legend 2>/dev/null
    echo "   📄 HTML Report: file://${COVERAGE_HTML_DIR}/index.html"
fi

# Final quality gate decision
echo ""
echo "🏆 FINAL QUALITY GATE RESULT:"

if $overall_passed; then
    echo "   ✅ QUALITY GATE: PASSED"
    echo "   Coverage requirement satisfied: ${COVERAGE_PERCENT}% >= ${MINIMUM_COVERAGE}%"

    # Bonus points for high coverage
    if awk "BEGIN {exit !($COVERAGE_PERCENT >= 90)}"; then
        echo "   🌟 EXCELLENT: Coverage exceeds 90% - Outstanding quality!"
    elif awk "BEGIN {exit !($COVERAGE_PERCENT >= 85)}"; then
        echo "   ⭐ GOOD: Coverage exceeds 85% - High quality!"
    fi

    exit_code=0
else
    echo "   ❌ QUALITY GATE: FAILED"
    echo "   Coverage requirement not met: ${COVERAGE_PERCENT}% < ${MINIMUM_COVERAGE}%"
    echo ""
    echo "🔧 IMPROVEMENT RECOMMENDATIONS:"

    if ! $core_passed; then
        echo "   • Priority: Increase core component test coverage (currently ${core_coverage}%)"
        echo "     - Focus on ETI processor, FIG analyser, and ETSI compliance modules"
        echo "     - Add unit tests for error handling and edge cases"
    fi

    if ! $gui_passed; then
        echo "   • Add GUI component tests (currently ${gui_coverage}%)"
        echo "     - Implement Qt Test framework for UI components"
        echo "     - Test user interactions and signal/slot connections"
    fi

    echo "   • General improvements:"
    echo "     - Add more unit tests for uncovered functions"
    echo "     - Implement integration tests for component interactions"
    echo "     - Add negative test cases for error conditions"

    exit_code=1
fi

echo ""
echo "📊 Coverage analysis complete!"
echo "==============================="

# Cleanup temporary files
rm -f /tmp/core_coverage.info /tmp/gui_coverage.info /tmp/utils_coverage.info

exit $exit_code
