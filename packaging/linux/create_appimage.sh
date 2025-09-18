#!/bin/bash
# ETI Stream Analyzer - Professional Linux AppImage Creation Script
# Universal Linux distribution for all major distros

set -e

# Professional AppImage configuration
APP_NAME="ETI Stream Analyzer"
APP_VERSION="${PROJECT_VERSION:-1.0.0}"
APP_DIR="ETIStreamAnalyzer.AppDir"
APPIMAGE_NAME="ETI_Stream_Analyzer-${APP_VERSION}-x86_64.AppImage"

echo "=== Creating Professional ETI Stream Analyzer AppImage ==="
echo "Version: ${APP_VERSION}"
echo "Target: Universal Linux Distribution"

# Clean previous builds
rm -rf "${APP_DIR}"
rm -f "${APPIMAGE_NAME}"

# Create professional AppDir structure
mkdir -p "${APP_DIR}/usr/bin"
mkdir -p "${APP_DIR}/usr/lib"
mkdir -p "${APP_DIR}/usr/share/applications"
mkdir -p "${APP_DIR}/usr/share/icons/hicolor/256x256/apps"
mkdir -p "${APP_DIR}/usr/share/mime/packages"
mkdir -p "${APP_DIR}/usr/share/doc/eti-stream-analyzer"

# Copy main application
echo "Installing application executable..."
cp "${CMAKE_BINARY_DIR}/ETIStreamAnalyzer" "${APP_DIR}/usr/bin/"
chmod +x "${APP_DIR}/usr/bin/ETIStreamAnalyzer"

# Professional library bundling with linuxdeploy
echo "Bundling required libraries..."
if command -v linuxdeploy >/dev/null 2>&1; then
    linuxdeploy --appdir "${APP_DIR}" --executable "${APP_DIR}/usr/bin/ETIStreamAnalyzer"
else
    echo "Warning: linuxdeploy not found. Manual library copying required."
    # Manual library detection and copying
    ldd "${APP_DIR}/usr/bin/ETIStreamAnalyzer" | grep "=>" | awk '{print $3}' | while read lib; do
        if [[ "$lib" == /usr/lib* ]] || [[ "$lib" == /lib* ]]; then
            continue  # Skip system libraries
        fi
        cp "$lib" "${APP_DIR}/usr/lib/" 2>/dev/null || true
    done
fi

# Professional desktop integration
echo "Creating professional desktop entry..."
cat > "${APP_DIR}/usr/share/applications/eti-stream-analyzer.desktop" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=ETI Stream Analyzer
GenericName=Professional ETI/DAB Stream Analysis
Comment=Professional ETI and DAB/DAB+ stream analyzer for broadcast engineers
Keywords=ETI;DAB;DAB+;Broadcast;Audio;Stream;Analysis;ETSI;Professional;
Icon=eti-stream-analyzer
Exec=ETIStreamAnalyzer %F
Terminal=false
MimeType=application/x-eti-stream;application/x-eti-ni;application/x-eti-li;
Categories=AudioVideo;Audio;Professional;Engineering;
StartupNotify=true
StartupWMClass=ETIStreamAnalyzer
X-AppImage-Version=${APP_VERSION}
X-AppImage-Comment=Professional broadcast engineering tool
EOF

# Create AppImage desktop file (required by AppImage spec)
cp "${APP_DIR}/usr/share/applications/eti-stream-analyzer.desktop" "${APP_DIR}/"

# Professional MIME type support
echo "Registering professional broadcast file types..."
cat > "${APP_DIR}/usr/share/mime/packages/eti-stream-analyzer.xml" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
    <mime-type type="application/x-eti-stream">
        <comment>ETI Stream File</comment>
        <comment xml:lang="en">ETI (Ensemble Transport Interface) Stream File</comment>
        <icon name="eti-stream-analyzer"/>
        <glob pattern="*.eti"/>
        <magic priority="50">
            <match value="ETI" type="string" offset="0"/>
        </magic>
    </mime-type>
    <mime-type type="application/x-eti-ni">
        <comment>ETI-NI Stream File</comment>
        <icon name="eti-stream-analyzer"/>
        <glob pattern="*.eti"/>
        <glob pattern="*.etini"/>
    </mime-type>
    <mime-type type="application/x-eti-li">
        <comment>ETI-LI Stream File</comment>
        <icon name="eti-stream-analyzer"/>
        <glob pattern="*.eti"/>
        <glob pattern="*.etili"/>
    </mime-type>
</mime-info>
EOF

# Professional application icon
echo "Installing professional application icon..."
if [ -f "${CMAKE_SOURCE_DIR}/resources/icons/eti-analyzer-256.png" ]; then
    cp "${CMAKE_SOURCE_DIR}/resources/icons/eti-analyzer-256.png" "${APP_DIR}/usr/share/icons/hicolor/256x256/apps/eti-stream-analyzer.png"
    cp "${CMAKE_SOURCE_DIR}/resources/icons/eti-analyzer-256.png" "${APP_DIR}/eti-stream-analyzer.png"
else
    echo "Warning: Application icon not found. Using placeholder."
    # Create a simple placeholder icon
    convert -size 256x256 xc:lightblue -pointsize 24 -fill black -gravity center -annotate +0+0 "ETI\nAnalyzer" "${APP_DIR}/eti-stream-analyzer.png" 2>/dev/null || true
fi

# Professional AppRun script
echo "Creating professional AppRun launcher..."
cat > "${APP_DIR}/AppRun" << 'EOF'
#!/bin/bash
# ETI Stream Analyzer Professional AppImage Launcher

HERE="$(dirname "$(readlink -f "${0}")")"

# Professional environment setup
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH}"
export PATH="${HERE}/usr/bin:${PATH}"
export XDG_DATA_DIRS="${HERE}/usr/share:${XDG_DATA_DIRS}"

# Professional Qt application settings
export QT_PLUGIN_PATH="${HERE}/usr/lib/qt6/plugins:${QT_PLUGIN_PATH}"
export QML2_IMPORT_PATH="${HERE}/usr/lib/qt6/qml:${QML2_IMPORT_PATH}"

# Professional broadcast engineering environment
export ETI_ANALYZER_PORTABLE=1
export ETI_ANALYZER_CONFIG_DIR="${HOME}/.config/eti-stream-analyzer"

# Create professional config directory
mkdir -p "${ETI_ANALYZER_CONFIG_DIR}"

# Launch professional application
exec "${HERE}/usr/bin/ETIStreamAnalyzer" "$@"
EOF

chmod +x "${APP_DIR}/AppRun"

# Professional documentation
echo "Installing professional documentation..."
if [ -f "${CMAKE_SOURCE_DIR}/README.md" ]; then
    cp "${CMAKE_SOURCE_DIR}/README.md" "${APP_DIR}/usr/share/doc/eti-stream-analyzer/"
fi
if [ -f "${CMAKE_SOURCE_DIR}/LICENSE" ]; then
    cp "${CMAKE_SOURCE_DIR}/LICENSE" "${APP_DIR}/usr/share/doc/eti-stream-analyzer/"
fi

# Create professional AppImage
echo "Creating professional AppImage package..."
if command -v appimagetool >/dev/null 2>&1; then
    # Use official AppImage tool
    appimagetool "${APP_DIR}" "${APPIMAGE_NAME}"
else
    echo "Warning: appimagetool not found. Please install AppImageKit."
    echo "AppDir created at: ${APP_DIR}"
    echo "Manual AppImage creation required."
    exit 1
fi

# Professional package validation
if [ -f "${APPIMAGE_NAME}" ]; then
    echo "=== Professional AppImage Created Successfully ==="
    echo "File: ${APPIMAGE_NAME}"
    echo "Size: $(du -h "${APPIMAGE_NAME}" | cut -f1)"
    echo "Executable: chmod +x ${APPIMAGE_NAME} && ./${APPIMAGE_NAME}"

    # Professional integrity check
    if command -v sha256sum >/dev/null 2>&1; then
        echo "SHA256: $(sha256sum "${APPIMAGE_NAME}" | cut -d' ' -f1)"
    fi

    echo "Ready for professional broadcast engineering deployment."
else
    echo "Error: AppImage creation failed."
    exit 1
fi
