#!/bin/bash
# Pre-uninstallation script for StreamDAB Stream Analyser
# RPM-based distributions (Fedora/RHEL/SUSE) pre-uninstallation

# Stop any running instances gracefully
if pgrep -f ETIStreamAnalyser >/dev/null 2>&1; then
    echo "Stopping running StreamDAB Stream Analyser instances..."
    pkill -TERM -f ETIStreamAnalyser 2>/dev/null || true
    sleep 2
    # Force kill if still running
    pkill -KILL -f ETIStreamAnalyser 2>/dev/null || true
fi

# Clean up temporary files
if [ -d /tmp/streamdab-* ]; then
    rm -rf /tmp/streamdab-* 2>/dev/null || true
fi

exit 0
