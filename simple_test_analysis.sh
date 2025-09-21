#!/bin/bash

# Simple Test Analysis - Direct execution without valgrind/strace
echo "=== SIMPLE TEST ANALYSIS ==="
echo "Timestamp: $(date)"

# Create results directory
mkdir -p simple_test_results
cd simple_test_results

# Initialize result file
echo "Test Name,Exit Code,Duration,stdout_lines,stderr_lines,Has_QT_Error,Has_Segfault,Test_Framework" > test_results.csv

# Test executables
TESTS=(
    "build/tests/e2e/test_comprehensive_workflows"
    "build/tests/e2e/test_eti_file_processing_workflow"
    "build/tests/e2e/test_file_opening_workflow"
    "build/tests/e2e/test_file_operations"
    "build/tests/e2e/test_frame_analysis_workflow"
    "build/tests/e2e/test_gui_integration_workflow"
    "build/tests/e2e/test_main_window_workflows"
    "build/tests/e2e/test_mandatory_workflow_coverage"
    "build/tests/e2e/test_menu_interactions"
    "build/tests/e2e/test_service_discovery_workflow"
)

# Function to run single test
run_simple_test() {
    local test_path="$1"
    local test_name=$(basename "$test_path")

    echo ""
    echo "=== TESTING: $test_name ==="

    # Setup environment
    export QT_QPA_PLATFORM=offscreen
    export QT_LOGGING_RULES="*.debug=false"

    # Run test with timeout
    local start_time=$(date +%s)
    timeout 60s "../$test_path" > "${test_name}_stdout.log" 2> "${test_name}_stderr.log"
    local exit_code=$?
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))

    # Count output lines
    local stdout_lines=$(wc -l < "${test_name}_stdout.log" 2>/dev/null || echo 0)
    local stderr_lines=$(wc -l < "${test_name}_stderr.log" 2>/dev/null || echo 0)

    # Check for specific error patterns
    local has_qt_error="No"
    local has_segfault="No"
    local test_framework="Unknown"

    if grep -q -i "qt.*error\|qwidget\|qapplication" "${test_name}_stderr.log" 2>/dev/null; then
        has_qt_error="Yes"
    fi

    if [ $exit_code -eq 139 ] || grep -q "Segmentation fault" "${test_name}_stderr.log" 2>/dev/null; then
        has_segfault="Yes"
    fi

    if grep -q "QTest\|QCOMPARE\|QVERIFY" "../$test_path" 2>/dev/null; then
        test_framework="Qt Test"
    elif grep -q "TEST\|EXPECT_\|ASSERT_" "../$test_path" 2>/dev/null; then
        test_framework="GTest"
    fi

    # Save to CSV
    echo "$test_name,$exit_code,$duration,$stdout_lines,$stderr_lines,$has_qt_error,$has_segfault,$test_framework" >> test_results.csv

    echo "Exit Code: $exit_code, Duration: ${duration}s, Stdout: $stdout_lines lines, Stderr: $stderr_lines lines"

    # Show first few lines of error output if any
    if [ $stderr_lines -gt 0 ]; then
        echo "Error snippet:"
        head -5 "${test_name}_stderr.log"
    fi
}

# Execute all tests
for test in "${TESTS[@]}"; do
    if [ -f "../$test" ]; then
        run_simple_test "$test"
    else
        echo "Test not found: $test"
        echo "$(basename "$test"),NOT_FOUND,0,0,0,No,No,Unknown" >> test_results.csv
    fi
done

echo ""
echo "=== ANALYSIS COMPLETE ==="
echo "Results saved in simple_test_results/"

# Show summary
echo ""
echo "=== SUMMARY ==="
echo "Total tests: $(tail -n +2 test_results.csv | wc -l)"
echo "Successful tests: $(tail -n +2 test_results.csv | grep ',0,' | wc -l)"
echo "Failed tests: $(tail -n +2 test_results.csv | grep -v ',0,' | wc -l)"
echo "Qt errors: $(tail -n +2 test_results.csv | grep ',Yes,' | wc -l)"
echo "Segfaults: $(tail -n +2 test_results.csv | grep 'Yes$' | wc -l)"

cd ..
