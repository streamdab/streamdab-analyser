# INSTALL — StreamDAB-Analyser

Build & install instructions for **StreamDAB Analyser** (Qt 6 / C++20 / CMake GUI + CLI).
Reference runtime: **Qt 6.11 / g++ 13 / Ubuntu** — see the version matrix at the end.

Test suite: the **PUBLIC subset** runs anywhere (`ctest -L PUBLIC`, which is what
GitHub CI runs); the **full suite** additionally needs the broadcast captures
under `eti/` — those are **World DAB licensed test material, not redistributed**
with this repository (obtain/place them locally, e.g. by using the private
origin). Parity gates on the Bangkok capture: DLS+ 1114, MOT 97/97, 0 CRC
warnings.

---

## 1. Native Linux build (canonical)

```bash
# Debian/Ubuntu prerequisites
sudo apt-get install -y cmake qt6-base-dev libfaad-dev libfftw3-dev libzmq3-dev \
    qt6-base-private-dev libxkbcommon-dev libxkbcommon-x11-dev \
    libgtest-dev build-essential ninja-build

# Configure — MUST export offscreen BEFORE configure so GUI tests register
export QT_QPA_PLATFORM=offscreen
cmake -B build -S .          # or: -G Ninja
JOBS="$(bash scripts/build_jobs.sh)"          # bounded: min(nproc, RAM_GB/2, 8)
export CMAKE_BUILD_PARALLEL_LEVEL="$JOBS"     # nested cmake/make honour this too
cmake --build build --parallel "$JOBS"        # NEVER a bare -j (that is unbounded)

# Test (all 58 targets)
cd build && ctest --output-on-failure

# Run
./build/StreamDABAnalyser                          # GUI (needs display)
./build/streamdab-cli --input eti/x.eti --output out.yaml   # CLI (headless ok)
./build/StreamDABAnalyser --cli --input eti/x.eti  # CLI mode via the GUI binary
```

### Build parallelism (avoid over-forking)

`-j$(nproc)` is **not** safe on many-core machines: each `g++` with the project's
`-O3 -march=native` flags can hold 1–2 GB, and recursive sub-makes (e.g. the
Qt-ADS FetchContent subproject) can add more, so a 16-core box can thrash.
A **bare `-j` / `--parallel` (no value) is worse still: make treats it as
unbounded and forks one compiler per target until the machine stalls.**

Use the bounded helper (CPU **and** RAM aware, default cap 8 jobs):

```bash
JOBS="$(bash scripts/build_jobs.sh)"       # e.g. min(nproc, RAM_GB/2, 8)
export CMAKE_BUILD_PARALLEL_LEVEL="$JOBS"  # nested cmake/make honour this
cmake --build build --parallel "$JOBS"
# or with presets:
cmake --preset linux-gcc && cmake --build --preset build-linux   # 8 jobs
cmake --build --preset low-memory                          # 4 jobs (small RAM)
# overrides: JOBS=8 bash scripts/build.sh --jobs 8 | STREAMDAB_MAX_JOBS=6 ...
```

`scripts/build.sh`, `build-linux-macos.sh`, `build_with_audio_validation.sh`,
`tdd_quality_gate.sh` and `verify_build_quality.sh` all use the helper now.

Binary names: **`StreamDABAnalyser`** (GUI) and **`streamdab-cli`** (headless).
Build requirements: **zero errors / zero warnings**.

## 2. Docker build (canonical container — `docker/Dockerfile`)

The **new** `docker/Dockerfile` matches the current tree. The legacy
root-level build Dockerfile was removed: it predated the Qt-ADS migration and
was known-broken (missing
`libzmq3-dev`, no CMake `install()` rules, legacy binary name, no Qt-for-Windows
toolchain — see §4). Two stages:

```bash
# Build only (binaries + optional ctest in the image)
docker build -t streamdab:build --target build -f docker/Dockerfile .

# Minimal runtime image
docker build -t streamdab:latest -f docker/Dockerfile .

# CLI on a capture (bkk_... is IN the repo; the volume still makes /data tidy):
docker run --rm -v $PWD/eti:/data streamdab:latest \
    streamdab-cli --input /data/bkk_20062022_141637.eti --output /data/out.yaml

# Full test suite inside the container (needs fixtures mounted at /src/eti):
docker run --rm -v $PWD:/src -w /src -e QT_QPA_PLATFORM=offscreen streamdab:build \
    sh -c "cmake -B /tmp/bld -S /src && cmake --build /tmp/bld --parallel 4 \
           && (cd /tmp/bld && ctest --output-on-failure)"
```

Notes:
- Base **ubuntu:24.04 → Qt 6.4.2** (newer than the historical 22.04/6.2); the
  reference build is Qt 6.11 — see matrix.
- The build stage skips `ctest` by default (`ARG RUN_CTEST=OFF`) because the
  build context has no `eti/` fixtures; set `--build-arg RUN_CTEST=ON` when the
  context includes them (CI).
- GUI in Docker needs a display; `QT_QPA_PLATFORM=offscreen` is set in the image
  for headless smoke.

## 3. Windows

Windows 10/11 x64 with **Visual Studio 2022 Build Tools**, **aqtinstall** Qt 6
(MSVC kit, incl. the `qtmultimedia` module) and **vcpkg** for the small native
deps. Build with the `windows-msvc` preset (needs `VCPKG_ROOT`):

```powershell
# PowerShell
aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 --modules qtmultimedia
$env:VCPKG_ROOT = "C:\dev\vcpkg"    # vcpkg install faad2 fftw3 zeromq cppzmq gtest
$env:QT_QPA_PLATFORM = "offscreen"  # before configure: registers the GUI tests
cmake --preset windows-msvc
cmake --build --preset build-windows
ctest --preset test-windows
.\build\StreamDABAnalyser.exe              # GUI
.\build\streamdab-cli.exe --input eti\bkk_20062022_141637.eti --output out.yaml
```

Never mix a `win64_mingw` Qt kit with the MSVC toolchain. The optional
components are detected portably; `-DDABX_ENABLE_ZMQ=OFF` drops the ZeroMQ
transport and `-DDABX_ENABLE_QMULTIMEDIA=OFF` the Qt Multimedia audio backend.

**Route A (recommended): MSVC + Qt with aqtinstall + vcpkg** — short version:

```powershell
# 1. Qt 6 desktop for MSVC (6.11 recommended; qtbase is all the app needs)
pip install aqtinstall
aqt install-qt windows desktop 6.11.0 win64_msvc2022_64 -O C:\Qt

# 2. Native deps via vcpkg. zeromq + cppzmq enable the zmq+tcp:// transport;
#    faad2 enables DAB+ decode; gtest is required by the test subdirectory.
git clone https://github.com/microsoft/vcpkg C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
C:\vcpkg\vcpkg install faad2 fftw3 zeromq cppzmq gtest --triplet x64-windows

# 3. Configure (VS 2022 generator, x64) and build with bounded parallelism:
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_PREFIX_PATH=C:\Qt\6.11.0\msvc2022_64 `
    -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake `
    -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build --config Release --parallel 8

# 4. Tests need QT_QPA_PLATFORM=offscreen (set BEFORE configure):
$env:QT_QPA_PLATFORM = "offscreen"
ctest --test-dir build -C Release --output-on-failure

# Run: .\build\Release\StreamDABAnalyser.exe
#      .\build\Release\streamdab-cli.exe --input <x.eti> --output out.yaml
# or use the wrapper: .\scripts\build-windows.ps1
```

Features are optional and detected portably (no `pkg-config` required):
`-DDABX_ENABLE_ZMQ=OFF` drops the ZeroMQ transport; the (Linux-only) ALSA
backend is off by default on Windows. **Windows has no audio output backend
yet** — DAB+ decode works, playback does not.

## 3.5 Cross-platform reference

| | Linux | Windows | macOS |
|---|---|---|---|
| Toolchain | gcc/clang + system Qt6 | MSVC 2022 + aqt Qt6 + vcpkg | Apple clang + Homebrew Qt6 |
| Configure preset | `linux-gcc` | `windows-msvc` | `macos-brew` |
| Build preset (8 jobs) | `build-linux` | `build-windows` | `build-macos` |
| Test preset (serial) | `test-linux` | `test-windows` | `test-macos` |
| Env vars | — | `VCPKG_ROOT` (vcpkg checkout) | `QT_DIR` (`brew --prefix qt@6`) |
| Docs | this file | §3 (Windows) | §4 (macOS) |
| Audio output | ALSA (default) | Qt Multimedia (QAudioSink) | Qt Multimedia (QAudioSink) |
| Packaging | **implemented** — TGZ/DEB via CPack + AppImage script (§5) | **implemented** — NSIS/ZIP via CPack (§5) | **implemented** — `.app`/`.dmg` via `scripts/bundle-macos.sh` (§5) |

## 4. macOS

**Route B: MinGW cross (experimental)** — the removed legacy build Dockerfile's
`windows-builder` stage is **unsupported**: it sets `CC=x86_64-w64-mingw32-gcc`
but never installs Qt-for-Windows, so `find_package(Qt6)` fails. To make it work
you must add an aqt Qt 6.11 `win64_mingw` install + matching mingw prefix into
that stage first. Never mix a `win64_mingw` Qt kit with the MSVC toolchain.

## 4. macOS

```bash
brew install cmake qt@6 faad2 fftw zeromq cppzmq googletest
export QT_QPA_PLATFORM=offscreen     # before configure, else GUI tests are not registered
export QT_DIR="$(brew --prefix qt@6)"  # qt@6 is keg-only
cmake --preset macos-brew            # configure into build/ (Release)
cmake --build --preset build-macos   # 8 jobs — never a bare -j (unbounded)
ctest --preset test-macos
```

Gaps: no audio output backend on macOS yet when the Qt kit lacks Multimedia
(ALSA is Linux-only; DAB+ decode works), and no code-signing/notarization yet.
`.app`/`.dmg` packaging is implemented — see §5. The legacy
`cmake/{Linux,MacOS,Windows}Config.cmake` files were dead (never included by
`CMakeLists.txt`) and have been removed; the portable detection ladder in the
top-level `CMakeLists.txt` is the only path.

## 5. Alternatives / other build routes

| Route | How | Status |
|---|---|---|
| **Reproducible container** | §2 above | ✅ canonical |
| **Script wrapper** | `scripts/build.sh` (Linux/macOS/Git-Bash) | ✅ exists, thin wrapper |
| **GitHub Actions CI** | `.github/workflows/ci.yml` (ubuntu-24.04 / windows-2022 / macos-14, Qt 6.8, `QT_QPA_PLATFORM=offscreen`, per-OS presets) | ✅ current — builds + **PUBLIC suite** (`ctest -L PUBLIC`) + CLI version/help smoke |
| **GitHub Releases** | `.github/workflows/release.yml` (tag `v*` → per-OS binary archives + release notes) | ✅ current — upgraded to CPack artifacts |
| **AppImage (Linux)** | `packaging/linux/create_appimage.sh` (linuxdeploy + linuxdeploy-plugin-qt) | ✅ implemented — see §6 |
| **DEB / TGZ (Linux)** | CPack (`cmake/packaging.cmake`) — `cmake --install build --prefix <p>` + `cpack --config build/CPackConfig.cmake -G TGZ\|DEB` | ✅ implemented — see §6 |
| **NSIS / ZIP (Windows)** | CPack NSIS generator via `CPACK_NSIS_*` vars (+ standalone template `packaging/windows/installer.nsi`); ZIP fallback | ✅ implemented — see §6 |
| **App / DMG (macOS)** | `scripts/bundle-macos.sh` (macdeployqt + hdiutil); `cpack -G BUNDLE\|DragNDrop` alternative | ✅ implemented — see §6 |
| **CMake presets** | per-OS: `linux-gcc` / `windows-msvc` / `macos-brew`, builds `build-linux` / `build-windows` / `build-macos` (8 jobs), tests `test-linux` / `test-windows` / `test-macos` | ✅ provided (`CMakePresets.json`) |

## 6. Production packaging (implemented)

`install()` rules + CPack are wired (`CMakeLists.txt` install section +
`cmake/packaging.cmake`). The version is single-sourced from `DABX_VERSION`
(git tag → `-DDABX_VERSION` → `PROJECT_VERSION`); CPack mirrors it into
`build/CMakeCache.txt` as `CPACK_PACKAGE_VERSION`.

```bash
# 1) Stage the install tree anywhere (no root needed):
cmake --install build --prefix /tmp/opencode/pack-stage
#    -> bin/{StreamDABAnalyser,streamdab-cli}
#       share/applications/eti-stream-analyser.desktop
#       share/mime/packages/eti-stream-analyser.xml
#       share/icons/hicolor/scalable/apps/eti-stream-analyser.svg
#       share/doc/streamdab-analyser/{LICENSE,README.md}

# 2) CPack artifacts (run inside build/ so they land in build/):
cd build
cpack --config CPackConfig.cmake            # default generator for this OS
cpack -G TGZ                                # Linux tarball
cpack -G DEB                                # Debian/Ubuntu package
cpack -G NSIS                               # Windows installer (on Windows)
cpack -G ZIP                                # Windows zip fallback
cpack -G BUNDLE                             # macOS .app (raw, no Qt)
cpack -G DragNDrop                          # macOS .dmg
#    -> streamdab-analyser-<DABX_VERSION>-Linux.tar.gz (TGZ)
#       streamdab-analyser_<DABX_VERSION>_amd64.deb        (DEB)
#       streamdab-analyser-<DABX_VERSION>-win64.zip etc.

# 3) AppImage (Linux) — packaging/linux/create_appimage.sh
./packaging/linux/create_appimage.sh         # needs linuxdeploy + plugin-qt

# 4) macOS .app + .dmg — scripts/bundle-macos.sh
./scripts/bundle-macos.sh                    # macdeployqt + hdiutil
```

Remaining follow-ups (not blocks): GitHub Actions release.yml already consumes
CPack artifacts; Windows NSIS icons and a macOS `.icns` are missing (no
`.ico`/`.icns` assets exist yet); code-signing/notarization is a later task.
`CMakePresets.json` exists. A vcpkg manifest (`vcpkg.json`) for Windows
reproducibility is still a follow-up.

## 7. Qt/compiler version matrix

| Platform | Qt | Compiler | Status |
|---|---|---|---|
| Ubuntu (host reference) | **6.11** | g++ 13 | ✅ full suite (58) with the private captures · PUBLIC subset (45) on CI |
| Docker (ubuntu:24.04) | 6.4.2 | gcc 13 | ✅ container build validated |
| Windows (MSVC / vcpkg) | 6.8+ (6.11 rec.) | MSVC 2022 | 🚧 plan in §3A |
| Windows (MinGW cross/Wine) | 6.11 win64_mingw | x86_64-w64-mingw32-g++ | 🚧 needs Qt toolchain added |
| macOS | 6.x (brew) | AppleClang | 🚧 plan in §4 |

Minimum supported: Qt 6.4 (for `QTimeZone`, C++20). Reference: Qt 6.11.