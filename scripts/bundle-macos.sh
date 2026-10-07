#!/usr/bin/env bash
# =============================================================================
# StreamDAB Analyser — macOS .app bundling + .dmg creation
# -----------------------------------------------------------------------------
# Production packaging for macOS. Requires a configured + built tree
# (cmake -B build -S . && cmake --build build) which produces the
# StreamDAB-Analyser.app bundle via the MACOSX_BUNDLE install() rules.
#
# Steps:
#   1. resolve the Qt 6 prefix from Homebrew (`brew --prefix qt@6`)
#   2. `cmake --install build --prefix dist`   (unstaged .app + CLI + docs)
#   3. `macdeployqt dist/StreamDAB-Analyser.app` (bundle the Qt frameworks)
#   4. `hdiutil create -volname "StreamDAB Analyser" -srcfolder dist ...`
#      -> a compressed UDZO .dmg, printed at the end.
#
# Qt frameworks are bundled by macdeployqt BEFORE any `cpack -G DragNDrop`
# would run — the CPack .dmg of an undeployed .app would carry no Qt libraries.
# You can run `cpack -G DragNDrop` inside build/ afterwards if you prefer the
# CPack route; this script is the canonical one-stop flow.
#
# Env overrides:
#   QT_DIR        Qt prefix (default: `brew --prefix qt@6`)
#   BUILD_DIR     build tree  (default: <repo>/build)
#   DIST_DIR      install/staging prefix (default: <repo>/dist)
#   BUILD_CONFIG  CMake --config value (default: Release)
#   DMG_OUT_DIR   where the .dmg lands (default: <repo>/build — NOT dist/, so
#                 the image is not nested inside its own source folder)
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT}/build}"
DIST_DIR="${DIST_DIR:-${ROOT}/dist}"
BUILD_CONFIG="${BUILD_CONFIG:-Release}"
DMG_OUT_DIR="${DMG_OUT_DIR:-${BUILD_DIR}}"

die() { echo "error: $*" >&2; exit 1; }

# --- 1) Qt 6 prefix -----------------------------------------------------------
QT_DIR="${QT_DIR:-$(brew --prefix qt@6 2>/dev/null || true)}"
[ -n "${QT_DIR}" ] || die \
    "brew --prefix qt@6 failed — install qt@6 (brew install qt@6) or set QT_DIR"
export PATH="${QT_DIR}/bin:${PATH}"

# --- version (single source: DABX_VERSION -> CPACK_PACKAGE_VERSION in cache) --
VERSION="$(sed -n 's/^CPACK_PACKAGE_VERSION:STRING=//p' \
    "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null | head -1)"
if [ -z "${VERSION}" ]; then
    VERSION="$(sed -n 's/^set(CPACK_PACKAGE_VERSION "\(.*\)")$/\1/p' \
        "${BUILD_DIR}/CPackConfig.cmake" 2>/dev/null | head -1)"
fi
[ -n "${VERSION}" ] || die \
    "cannot resolve the package version — configure first: cmake -B ${BUILD_DIR} -S ${ROOT}"

APP_NAME="StreamDAB-Analyser.app"
DMG_NAME="StreamDAB-Analyser-${VERSION}.dmg"

echo "=== StreamDAB Analyser macOS bundle (v${VERSION}) ==="
echo "Qt dir   : ${QT_DIR}"
echo "Build dir: ${BUILD_DIR}"

# --- 2) staged install tree ---------------------------------------------------
echo "[1/3] cmake --install ${BUILD_DIR} --prefix ${DIST_DIR} (--config ${BUILD_CONFIG})"
cmake --install "${BUILD_DIR}" --prefix "${DIST_DIR}" --config "${BUILD_CONFIG}"

[ -d "${DIST_DIR}/${APP_NAME}" ] || die "expected ${DIST_DIR}/${APP_NAME} — check the install step"

# --- 3) Qt framework bundling -------------------------------------------------
echo "[2/3] macdeployqt ${DIST_DIR}/${APP_NAME}"
macdeployqt "${DIST_DIR}/${APP_NAME}" -always-overwrite -verbose=1

# --- 4) compressed .dmg -------------------------------------------------------
mkdir -p "${DMG_OUT_DIR}"
echo "[3/3] hdiutil create -volname 'StreamDAB Analyser' -srcfolder ${DIST_DIR}"
echo "      -ov -format UDZO ${DMG_OUT_DIR}/${DMG_NAME}"
hdiutil create \
    -volname "StreamDAB Analyser" \
    -srcfolder "${DIST_DIR}" \
    -ov \
    -format UDZO \
    "${DMG_OUT_DIR}/${DMG_NAME}"

echo "=== macOS bundle complete ==="
echo "Artifact: ${DMG_OUT_DIR}/${DMG_NAME}"