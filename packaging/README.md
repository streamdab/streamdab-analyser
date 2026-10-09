# StreamDAB Analyser — Cross-Platform Packaging

Packaging infrastructure for **StreamDAB Analyser** (Qt6 / C++20 ETI/DAB stream
analyser). The CMake-driven path lives in `cmake/packaging.cmake` (wired from
`CMakeLists.txt`); this directory holds the per-platform extras:

```
packaging/
├── debian/              # DEB maintainer scripts (postinst / prerm) — wired via
│                        # CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
├── linux/               # AppImage builder + DEB/RPM maintainer scripts
├── macos/               # Info.plist.in (bundle template), DMGSetup.scpt,
│                        # entitlements.plist
├── windows/             # standalone NSIS 3 installer template (CPack's NSIS
│                        # generator uses the CPACK_NSIS_* vars instead)
├── scripts/             # build_all_packages.sh / validate_*_packages.sh
└── README.md
```

## Supported artifacts

| Platform | Artifact | How |
|---|---|---|
| Linux | TGZ, DEB | `cpack --config build/CPackConfig.cmake -G TGZ\|DEB` (from `build/`) |
| Linux | AppImage | `./packaging/linux/create_appimage.sh` (linuxdeploy + plugin-qt — see the script header) |
| Windows | NSIS installer, ZIP | `cpack -G NSIS\|ZIP` (needs NSIS; `packaging/windows/installer.nsi` is the standalone advanced template) |
| macOS | `.app` + `.dmg` | `scripts/bundle-macos.sh` (macdeployqt + hdiutil) — run **after** `cmake --install build --prefix dist` |

## Quick start (Linux)

```bash
export QT_QPA_PLATFORM=offscreen          # before configure (GUI tests)
cmake -B build -S .
JOBS="$(bash scripts/build_jobs.sh)"      # bounded — never a bare -j
cmake --build build --parallel "$JOBS"

# 1) stage the install tree
cmake --install build --prefix /tmp/opencode/pack-stage

# 2) CPack packages (TGZ + DEB)
cd build
cpack -G TGZ
cpack -G DEB
# artifacts land in build/: streamdab-analyser-<DABX_VERSION>-Linux.tar.gz
# and streamdab-analyser_<DABX_VERSION>_amd64.deb (DEB-DEFAULT naming)
```

Version source of truth: `DABX_VERSION` (git tag → `-DDABX_VERSION` →
`PROJECT_VERSION`) resolved in `CMakeLists.txt`; CPack mirrors it into
`build/CMakeCache.txt` as `CPACK_PACKAGE_VERSION`. Packaging scripts read it
from there — never hardcode it.

## Windows NSIS (standalone advanced template)

The CPack NSIS generator (branded via `CPACK_NSIS_*` in
`cmake/packaging.cmake`) is the main path. For the advanced
`packaging/windows/installer.nsi` template:

```bat
:: from the repo root, with makensis 3.x on PATH
makensis -DPRODUCT_VERSION=1.5.0 packaging\windows\installer.nsi
```

`PRODUCT_VERSION` is intentionally **not** hardcoded in `nsis_config.nsh`;
in both cases copy the built `StreamDABAnalyser.exe` + Qt DLLs into
`build\Release` (default `${BUILD_DIR}`) before running.

## macOS

`scripts/bundle-macos.sh` is the canonical flow:

```bash
./scripts/bundle-macos.sh
# 1. cmake --install build --prefix dist
# 2. macdeployqt dist/StreamDAB-Analyser.app   (bundles Qt frameworks)
# 3. hdiutil create ... build/StreamDAB-Analyser-<ver>.dmg
```

Run it **before** any `cpack -G DragNDrop` (CPack alone would produce a `.dmg`
whose `.app` has no Qt frameworks).

## Validation

```bash
./packaging/scripts/validate_packages.sh       # quick tree check
./packaging/scripts/build_all_packages.sh      # full build + all CPack packages
```

See `INSTALL.md` §5/§6 (repo root) for the documented flow and known gaps
(e.g. no .icns/ICO assets, no code-signing yet).