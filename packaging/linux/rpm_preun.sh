#!/bin/bash
# ETI Stream Analyzer - RPM Pre-Uninstallation Script
# Professional cleanup for Red Hat/Fedora/SUSE systems

echo "Preparing ETI Stream Analyzer for professional removal..."

# Stop any running instances
if command -v pgrep >/dev/null 2>&1; then
    pgrep ETIStreamAnalyzer | xargs -r kill -TERM 2>/dev/null || true
    sleep 2
    pgrep ETIStreamAnalyzer | xargs -r kill -KILL 2>/dev/null || true
fi

# Clean professional temporary files
rm -rf /tmp/eti-stream-analyzer-* 2>/dev/null || true

# Backup important configuration files
if [ -d "/etc/eti-stream-analyzer" ]; then
    if [ -f "/etc/eti-stream-analyzer/global.conf" ]; then
        cp /etc/eti-stream-analyzer/global.conf /etc/eti-stream-analyzer/global.conf.backup 2>/dev/null || true
    fi
fi

echo "ETI Stream Analyzer professional removal preparation complete."

exit 0
