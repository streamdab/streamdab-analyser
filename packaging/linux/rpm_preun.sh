#!/bin/bash
# Pre-uninstallation script for StreamDAB Stream Analyzer
# RPM-based distributions (Fedora/RHEL/SUSE) pre-uninstallation

# Stop any running instances gracefully
if pgrep -f ETIStreamAnalyzer >/dev/null 2>&1; then
    echo "Stopping running StreamDAB Stream Analyzer instances..."
    pkill -TERM -f ETIStreamAnalyzer 2>/dev/null || true
    sleep 2
    # Force kill if still running
    pkill -KILL -f ETIStreamAnalyzer 2>/dev/null || true
fi

# Clean up temporary files
if [ -d /tmp/streamdab-* ]; then
    rm -rf /tmp/streamdab-* 2>/dev/null || true
fi

exit 0
