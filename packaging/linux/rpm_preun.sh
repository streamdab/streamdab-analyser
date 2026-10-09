#!/bin/bash
# Pre-uninstallation script for StreamDAB Analyser
# RPM-based distributions (Fedora/RHEL/SUSE) pre-uninstallation

# Stop any running instances gracefully
if pgrep -f StreamDABAnalyser >/dev/null 2>&1; then
    echo "Stopping running StreamDAB Analyser instances..."
    pkill -TERM -f StreamDABAnalyser 2>/dev/null || true
    sleep 2
    # Force kill if still running
    pkill -KILL -f StreamDABAnalyser 2>/dev/null || true
fi

# Clean up temporary files
if [ -d /tmp/streamdab-* ]; then
    rm -rf /tmp/streamdab-* 2>/dev/null || true
fi

exit 0
