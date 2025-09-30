#!/bin/bash

echo "🔍 Monitoring debug output for ETI file loading..."
echo "📋 Please now test File->Open and select: eti/bkk_20062022_141637.eti"
echo "💡 Watch for debug messages below..."
echo "----------------------------------------"

# Monitor the log file for changes
tail -f /tmp/debug_eti_loading.log
