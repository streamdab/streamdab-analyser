#!/bin/bash

# Final Test Analysis - Quick execution to identify patterns
echo "=== FINAL TEST ANALYSIS ==="
echo "Timestamp: $(date)"
echo ""

# Test executables
TESTS=(
    "test_comprehensive_workflows"
    "test_eti_file_processing_workflow"
    "test_file_opening_workflow"
    "test_file_operations"
    "test_frame_analysis_workflow"
    "test_gui_integration_workflow"
    "test_main_window_workflows"
    "test_mandatory_workflow_coverage"
    "test_menu_interactions"
    "test_service_discovery_workflow"
)

export QT_QPA_PLATFORM=offscreen

# Quick test execution with pattern detection
for test in "${TESTS[@]}"; do
    echo "=== Testing $test ==="

    # Run with 15s timeout and capture result
    timeout 15s "build/tests/e2e/$test" > "/tmp/${test}_output.log" 2>&1
    exit_code=$?

    # Analyze output quickly
    if [ $exit_code -eq 0 ]; then
        echo "  ✅ PASSED"
    elif [ $exit_code -eq 124 ]; then
        echo "  ⏰ TIMEOUT"
    elif [ $exit_code -eq 139 ]; then
        echo "  💥 SEGFAULT"
    else
        echo "  ❌ FAILED (exit: $exit_code)"
    fi

    # Check for specific error patterns
    if grep -q "Segmentation fault\|SIGSEGV" "/tmp/${test}_output.log" 2>/dev/null; then
        echo "  🔍 Contains segmentation fault"
    fi

    if grep -q "QObject::connect.*invalid nullptr" "/tmp/${test}_output.log" 2>/dev/null; then
        echo "  🔍 Qt nullptr connection warnings"
    fi

    if grep -q "nlohmann/json not available" "/tmp/${test}_output.log" 2>/dev/null; then
        echo "  🔍 Missing nlohmann/json dependency"
    fi

    # Count test results
    passed=$(grep -c "PASS" "/tmp/${test}_output.log" 2>/dev/null || echo 0)
    failed=$(grep -c "FAIL" "/tmp/${test}_output.log" 2>/dev/null || echo 0)

    if [ $passed -gt 0 ] || [ $failed -gt 0 ]; then
        echo "  📊 Tests: $passed passed, $failed failed"
    fi

    echo ""
done

echo "=== PATTERN ANALYSIS ==="

# Count issues across all tests
echo "Analyzing patterns across all test outputs..."

segfault_tests=$(grep -l "Segmentation fault\|SIGSEGV" /tmp/test_*_output.log 2>/dev/null | wc -l)
nullptr_tests=$(grep -l "QObject::connect.*invalid nullptr" /tmp/test_*_output.log 2>/dev/null | wc -l)
json_missing_tests=$(grep -l "nlohmann/json not available" /tmp/test_*_output.log 2>/dev/null | wc -l)

echo "📈 Summary of Issues:"
echo "  - Tests with segmentation faults: $segfault_tests"
echo "  - Tests with Qt nullptr warnings: $nullptr_tests"
echo "  - Tests with missing JSON dependency: $json_missing_tests"

echo ""
echo "🔍 Detailed Analysis:"

if [ $segfault_tests -gt 0 ]; then
    echo "SEGFAULT PATTERN:"
    grep -h -A5 -B5 "Segmentation fault\|SIGSEGV" /tmp/test_*_output.log 2>/dev/null | head -10
    echo ""
fi

if [ $nullptr_tests -gt 0 ]; then
    echo "NULLPTR CONNECTION PATTERN:"
    grep -h "QObject::connect.*invalid nullptr" /tmp/test_*_output.log 2>/dev/null | head -5
    echo ""
fi

echo "=== ANALYSIS COMPLETE ==="
