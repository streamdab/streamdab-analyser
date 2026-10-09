#!/usr/bin/env bash
# =============================================================================
# StreamDAB Analyser — Linux AppImage builder (linuxdeploy + plugin-qt)
# -----------------------------------------------------------------------------
# Builds a self-contained AppImage from the `cmake --install` prefix using the
# modern linuxdeploy toolchain. Requires a configured + built tree:
#
#   export QT_QPA_PLATFORM=offscreen
#   cmake -B build -S .
#   JOBS="$(bash scripts/build_jobs.sh)"        # bounded (never a bare -j)
#   cmake --build build --parallel "$JOBS"
#   ./packaging/linux/create_appimage.sh
#
# Output: build/StreamDAB-Analyser-<DABX_VERSION>-x86_64.AppImage
# (the staging AppDir is build/appimage-stage/).
#
# Prerequisites (tool binaries are NOT downloaded by this script):
#   - linuxdeploy            -> PATH, or $LINUXDEPLOY
#   - linuxdeploy-plugin-qt  -> PATH, or $LINUXDEPLOY_PLUGIN_QT
#   - (optional) appimagetool on PATH, or linuxdeploy-plugin-appimage, to produce
#     the final .AppImage; without one of them the script stops after leaving a
#     valid AppDir (build/appimage-stage/) and prints the manual command.
#
# Example install of the tools (one-time):
#   mkdir -p ~/bin
#   wget -qO ~/bin/linuxdeploy \
#     https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
#   wget -qO ~/bin/linuxdeploy-plugin-qt \
#     https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
#   chmod +x ~/bin/linuxdeploy ~/bin/linuxdeploy-plugin-qt
#   export PATH="$HOME/bin:$PATH"
#
# On headless CI, set the AppImage container environment first:
#   export APPIMAGE_EXTRACT_AND_RUN=1
#
# Env overrides:
#   BUILD_DIR   build tree (default: <repo>/build)
#   STAGE_DIR   AppDir staging prefix (default: <repo>/build/appimage-stage)
#   LINUXDEPLOY / LINUXDEPLOY_PLUGIN_QT  explicit tool paths
#   OUTPUT      AppImage file name (default: StreamDAB-Analyser-<ver>-x86_64.AppImage)
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT}/build}"
STAGE_DIR="${STAGE_DIR:-${BUILD_DIR}/appimage-stage}"
ARCH="$(uname -m)"                         # x86_64 / aarch64

die() { echo "error: $*" >&2; exit 1; }

# --- version (single source: DABX_VERSION -> CPACK_PACKAGE_VERSION in cache) --
resolve_version() {
    local v
    v="$(sed -n 's/^CPACK_PACKAGE_VERSION:STRING=//p' \
        "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null | head -1)"
    if [ -z "${v}" ]; then
        v="$(sed -n 's/^set(CPACK_PACKAGE_VERSION "\(.*\)")$/\1/p' \
            "${BUILD_DIR}/CPackConfig.cmake" 2>/dev/null | head -1)"
    fi
    if [ -z "${v}" ]; then
        v="$(git -C "${ROOT}" describe --tags --abbrev=0 2>/dev/null | sed 's/^v//' || true)"
    fi
    [ -n "${v}" ] || die "cannot resolve the version — configure build/ first (cmake -B ${BUILD_DIR} -S ${ROOT})"
    echo "${v}"
}

# --- tool discovery ----------------------------------------------------------
LINUXDEPLOY="${LINUXDEPLOY:-$(command -v linuxdeploy || true)}"
LINUXDEPLOY_PLUGIN_QT="${LINUXDEPLOY_PLUGIN_QT:-$(command -v linuxdeploy-plugin-qt || true)}"
[ -n "${LINUXDEPLOY}" ] || die "linuxdeploy not found — set LINUXDEPLOY or add it to PATH (see header)"
[ -n "${LINUXDEPLOY_PLUGIN_QT}" ] || die "linuxdeploy-plugin-qt not found — set LINUXDEPLOY_PLUGIN_QT or add it to PATH (see header)"

VERSION="$(resolve_version)"
OUTPUT="${OUTPUT:-StreamDAB-Analyser-${VERSION}-${ARCH}.AppImage}"

echo "=== StreamDAB Analyser AppImage (v${VERSION}, ${ARCH}) ==="
echo "Staging : ${STAGE_DIR}"
echo "linuxdeploy: ${LINUXDEPLOY}"
echo "plugin-qt : ${LINUXDEPLOY_PLUGIN_QT}"

# --- 1) fresh AppDir staged from cmake --install -------------------------------
# cmake --install --prefix <stage> produces exactly the AppDir layout the
# AppImage spec expects: usr/bin, usr/share/applications, usr/share/icons/...,
# usr/share/mime/packages, usr/share/doc — plus the two binaries.
rm -rf "${STAGE_DIR}"
cmake --install "${BUILD_DIR}" --prefix "${STAGE_DIR}"

[ -x "${STAGE_DIR}/usr/bin/StreamDABAnalyser" ] || die \
    "staged AppDir has no usr/bin/StreamDABAnalyser — did the build succeed?"
[ -f "${STAGE_DIR}/usr/share/applications/eti-stream-analyser.desktop" ] || die \
    "staged AppDir has no desktop file — check install() rules"

# --- 2) optional high-res raster icon (AppImage tooling prefers PNG) ----------
# The install tree ships hicolor/scalable/apps/eti-stream-analyser.svg. If a
# rasteriser is available, add a 256x256 PNG alongside it.
if command -v rsvg-convert >/dev/null 2>&1; then
    mkdir -p "${STAGE_DIR}/usr/share/icons/hicolor/256x256/apps"
    rsvg-convert -w 256 -h 256 \
        "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/eti-stream-analyser.svg" \
        -o "${STAGE_DIR}/usr/share/icons/hicolor/256x256/apps/eti-stream-analyser.png"
elif command -v convert >/dev/null 2>&1; then
    mkdir -p "${STAGE_DIR}/usr/share/icons/hicolor/256x256/apps"
    convert -background none \
        "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/eti-stream-analyser.svg" \
        -resize 256x256 \
        "${STAGE_DIR}/usr/share/icons/hicolor/256x256/apps/eti-stream-analyser.png"
else
    echo "note: no rsvg-convert/convert found — SVG icon stays scalable-only"
fi

# --- 3) bundle Qt + libs into the AppDir with linuxdeploy ----------------------
"${LINUXDEPLOY}" \
    --appdir "${STAGE_DIR}" \
    --executable "${STAGE_DIR}/usr/bin/StreamDABAnalyser" \
    --desktop-file "${STAGE_DIR}/usr/share/applications/eti-stream-analyser.desktop" \
    --icon-file "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/eti-stream-analyser.svg" \
    --plugin qt

# --- 4) produce the AppImage ---------------------------------------------------
if command -v appimagetool >/dev/null 2>&1; then
    echo "Creating ${OUTPUT} with appimagetool..."
    appimagetool "${STAGE_DIR}" "${OUTPUT}"
elif command -v linuxdeploy-plugin-appimage >/dev/null 2>&1; then
    echo "Creating ${OUTPUT} with linuxdeploy-plugin-appimage (OUTPUT=${OUTPUT})..."
    OUTPUT="${OUTPUT}" "${LINUXDEPLOY}" \
        --appdir "${STAGE_DIR}" \
        --output appimage
else
    echo "note: neither appimagetool nor linuxdeploy-plugin-appimage found."
    echo "A valid AppDir is ready at: ${STAGE_DIR}"
    echo "Create the AppImage manually, e.g.:"
    echo "  appimagetool ${STAGE_DIR} ${OUTPUT}"
    exit 0
fi

if [ -f "${OUTPUT}" ]; then
    echo "=== AppImage created successfully ==="
    echo "Artifact: ${OUTPUT}"
    echo "Size    : $(du -h "${OUTPUT}" | cut -f1)"
    echo "SHA256  : $(sha256sum "${OUTPUT}" | cut -d' ' -f1)"
else
    die "AppImage was not created (see tool output above)"
fi