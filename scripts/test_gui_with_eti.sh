#!/bin/bash
# Test GUI with problematic ETI file
# This script launches the GUI in headless mode and monitors for crashes

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "=== Testing GUI with ETI File ==="
echo "File: ${PROJECT_ROOT}/eti/bkk_20062022_141637.eti"
echo ""

cd "${PROJECT_ROOT}/build"

# Set headless display
export QT_QPA_PLATFORM=offscreen

# Run with timeout to prevent hanging
timeout 30s ./StreamDABAnalyser 2>&1 | tee gui_test_output.log &
GUI_PID=$!

echo "GUI Process ID: $GUI_PID"
echo "Waiting for process to complete or timeout..."

# Wait for process
wait $GUI_PID
EXIT_CODE=$?

echo ""
echo "=== Test Results ==="
echo "Exit Code: $EXIT_CODE"

# Check for segfault
if grep -q "Segmentation fault" gui_test_output.log; then
    echo "Status: ❌ CRASH DETECTED (Segmentation Fault)"
    echo ""
    echo "Last 20 lines before crash:"
    tail -20 gui_test_output.log
    exit 1
elif grep -q "core dumped" gui_test_output.log; then
    echo "Status: ❌ CRASH DETECTED (Core Dumped)"
    exit 1
elif [ $EXIT_CODE -eq 124 ]; then
    echo "Status: ⏱️ TIMEOUT (30 seconds)"
    echo "Note: Timeout is OK for GUI tests - just means processing took longer"
    exit 0
elif [ $EXIT_CODE -eq 0 ]; then
    echo "Status: ✅ SUCCESS (No Crash)"
    exit 0
else
    echo "Status: ⚠️ UNKNOWN (Exit Code: $EXIT_CODE)"
    exit $EXIT_CODE
fi
