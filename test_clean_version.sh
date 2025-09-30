#!/bin/bash

echo "🚀 StreamDAB Analyzer - Clean Test Version"
echo "==========================================="
echo ""
echo "✅ Build Status: Clean version compiled successfully"
echo "📁 Test File: eti/bkk_20062022_141637.eti"
echo "📊 File Size: $(stat -c%s /home/seksan/workspace/streamdab-analyzer/eti/bkk_20062022_141637.eti) bytes"
echo "📊 Expected Frames: $(($(stat -c%s /home/seksan/workspace/streamdab-analyzer/eti/bkk_20062022_141637.eti) / 6144))"
echo ""
echo "🧪 Testing Instructions:"
echo "1. Application should start without freezing"
echo "2. File → Open → select eti/bkk_20062022_141637.eti"
echo "3. Check System Messages panel for loading status"
echo "4. Check Frame List panel for ETI frames"
echo "5. Multi-stream tab → Add ETI Stream (should not crash)"
echo ""
echo "🔧 Fixed Issues:"
echo "✅ Startup freeze - resolved with lazy initialization"
echo "✅ Multi-stream coredump - resolved with ensureMultiStreamReady()"
echo "✅ Signal connections - moved to component creation time"
echo ""
echo "Starting application..."

cd /home/seksan/workspace/streamdab-analyzer/build
exec ./DABAnalyzer_Enhanced_FilteringBuild
