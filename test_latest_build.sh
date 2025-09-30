#!/bin/bash

echo "🔄 CLEAN BUILD TEST - Latest Version"
echo "====================================="
echo ""
echo "🏗️ Build Information:"
echo "Build directory: $(pwd)/build"
echo "Executable: $(ls -la build/DABAnalyzer_Enhanced_FilteringBuild)"
echo "Build time: $(stat -c %y build/DABAnalyzer_Enhanced_FilteringBuild)"
echo ""
echo "🧪 Test Features Included:"
echo "✅ Emergency test buttons (red and green)"
echo "✅ Comprehensive signal debugging"
echo "✅ Frame processing debug output"
echo "✅ UI refresh and event processing fixes"
echo "✅ Widget visibility debugging"
echo ""
echo "📋 What to Test:"
echo "1. Click GREEN button (bypasses file dialog)"
echo "2. Watch for debug messages in System Messages panel"
echo "3. Check if frames appear in ETI Frame Navigator"
echo "4. Look for '📺 Frame list widget visibility' messages"
echo "5. Check for '🔄 Final refresh: Frame list contains X items'"
echo ""
echo "🚀 Starting latest build..."
echo ""

cd /home/seksan/workspace/streamdab-analyzer/build
exec ./DABAnalyzer_Enhanced_FilteringBuild
