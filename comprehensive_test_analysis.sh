#!/bin/bash

# Comprehensive Test Failure Analysis Script
# Executes all tests with detailed diagnostics

echo "=== COMPREHENSIVE TEST FAILURE ANALYSIS ==="
echo "Timestamp: $(date)"
echo "=============================================="

# Create analysis directory
mkdir -p test_analysis_results
cd test_analysis_results

# Initialize result files
echo "Test Name,Exit Code,Duration,Memory Peak,Errors,Segfault,Timeout" > test_results_summary.csv

# Test executables array
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

# Function to run individual test with comprehensive diagnostics
run_test_with_diagnostics() {
    local test_path="$1"
    local test_name=$(basename "$test_path")

    echo ""
    echo "=== TESTING: $test_name ==="
    echo "Test Path: $test_path"
    echo "Start Time: $(date)"

    # Create individual test log directory
    mkdir -p "$test_name"
    cd "$test_name"

    # Pre-test environment check
    echo "Environment:" > environment.log
    echo "Display: $DISPLAY" >> environment.log
    echo "Qt Version: $(qmake -version 2>/dev/null || echo 'Not available')" >> environment.log
    echo "Memory before test: $(free -m)" >> environment.log

    # Run test with timeout and memory monitoring
    local start_time=$(date +%s)

    # Setup virtual display for GUI tests
    export QT_QPA_PLATFORM=offscreen
    export DISPLAY=:99

    # Execute test with comprehensive monitoring
    timeout 120s strace -f -e trace=memory,signal -o strace.log \
        valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all \
        --track-origins=yes --log-file=valgrind.log \
        "../$test_path" > stdout.log 2> stderr.log

    local exit_code=$?
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))

    echo "Exit Code: $exit_code" > test_result.log
    echo "Duration: ${duration}s" >> test_result.log
    echo "End Time: $(date)" >> test_result.log

    # Analyze results
    local has_segfault="No"
    local has_timeout="No"
    local error_count=0
    local memory_peak="Unknown"

    # Check for segmentation fault
    if [ $exit_code -eq 139 ] || grep -q "Segmentation fault" stderr.log; then
        has_segfault="Yes"
        echo "SEGMENTATION FAULT DETECTED" >> test_result.log
    fi

    # Check for timeout
    if [ $exit_code -eq 124 ]; then
        has_timeout="Yes"
        echo "TIMEOUT DETECTED" >> test_result.log
    fi

    # Count errors in output
    error_count=$(grep -c -i "error\|failed\|fatal" stdout.log stderr.log 2>/dev/null || echo 0)

    # Extract memory peak from valgrind if available
    if [ -f valgrind.log ]; then
        memory_peak=$(grep "total heap usage" valgrind.log | tail -1 | awk '{print $9 $10}' || echo "Unknown")
    fi

    # Generate stack trace if segfault occurred
    if [ "$has_segfault" = "Yes" ]; then
        echo "=== STACK TRACE ANALYSIS ===" >> test_result.log
        gdb -batch -ex "run" -ex "bt full" -ex "quit" "../$test_path" >> gdb_stacktrace.log 2>&1 || echo "GDB analysis failed"
    fi

    # Summary for CSV
    echo "$test_name,$exit_code,$duration,$memory_peak,$error_count,$has_segfault,$has_timeout" >> ../test_results_summary.csv

    echo "Test completed: $test_name (Exit: $exit_code, Duration: ${duration}s)"

    cd ..
}

# Execute all tests
echo "Starting comprehensive test execution..."

for test in "${TESTS[@]}"; do
    if [ -f "../$test" ]; then
        run_test_with_diagnostics "$test"
    else
        echo "Test not found: $test"
        echo "$(basename "$test"),NOT_FOUND,0,Unknown,0,No,No" >> test_results_summary.csv
    fi
done

# Generate comprehensive analysis report
echo ""
echo "=== GENERATING ANALYSIS REPORT ==="

cat > analysis_report.md << 'EOF'
# Comprehensive Test Failure Analysis Report

## Executive Summary
This report provides detailed analysis of all test execution failures, grouped by patterns and root causes.

## Test Execution Summary
EOF

# Add summary statistics
echo "" >> analysis_report.md
echo "### Overall Results" >> analysis_report.md
echo "- Total Tests: $(cat test_results_summary.csv | wc -l | awk '{print $1-1}')" >> analysis_report.md
echo "- Successful Tests: $(tail -n +2 test_results_summary.csv | grep ',0,' | wc -l)" >> analysis_report.md
echo "- Failed Tests: $(tail -n +2 test_results_summary.csv | grep -v ',0,' | wc -l)" >> analysis_report.md
echo "- Segmentation Faults: $(tail -n +2 test_results_summary.csv | grep ',Yes,' | wc -l)" >> analysis_report.md
echo "- Timeouts: $(tail -n +2 test_results_summary.csv | grep 'Yes$' | wc -l)" >> analysis_report.md

echo "" >> analysis_report.md
echo "### Detailed Results" >> analysis_report.md
echo "\`\`\`" >> analysis_report.md
cat test_results_summary.csv >> analysis_report.md
echo "\`\`\`" >> analysis_report.md

echo ""
echo "Analysis complete. Results saved in test_analysis_results/"
echo "View summary: cat test_analysis_results/test_results_summary.csv"
echo "View full report: cat test_analysis_results/analysis_report.md"
