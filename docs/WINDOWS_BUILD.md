# Building StreamDAB Analyser on Windows 10 / 11 (x64)

Detailed, copy-pasteable guide for a native **MSVC (Visual Studio 2022) + Qt 6 +
vcpkg** build. It reflects the current tree (branch `main`, Qt-ADS GUI, C++20)
and the CMake options that were added for portable, cross-platform
configuration:

| Option | Default | Meaning |
|---|---|---|
| `DABX_ENABLE_ZMQ` | `ON` | Build the ZeroMQ (`zmq+tcp://`) ETI transport when `libzmq` + `cppzmq` are found; otherwise it is compiled out cleanly. |
| `DABX_ENABLE_ALSA` | `ON` | Build the ALSA audio **output** backend (Linux only; not found on Windows → no-op). Audio **decode** is unaffected. |
| `DABX_ENABLE_QMULTIMEDIA` | `ON` | Build the Qt Multimedia audio **output** backend (QAudioSink → WASAPI) when the Qt kit ships the Multimedia module (the full `aqt` desktop Qt and the default vcpkg Qt do). If the module is absent the backend is compiled out cleanly. |

> GUI and CLI binaries: **`StreamDABAnalyser.exe`** (GUI, CMake target
> `DABAnalyser_Enhanced_FilteringBuild`) and **`streamdab-cli.exe`** (headless).

---

## 0. TL;DR (once the prerequisites in §1 are installed)

```powershell
# From a "x64 Native Tools Command Prompt for VS 2022" (or after vcvars64.bat)
cd C:\src\streamdab-analyser
$env:QT_QPA_PLATFORM = "offscreen"

cmake -B build -S . -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH=C:\Qt\6.11.0\msvc2022_64 `
  -DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows

cmake --build build --config Release --parallel 8
ctest --test-dir build -C Release --output-on-failure
```

---

## 1. Prerequisites

Install all of the following (default install paths are assumed below):

1. **Windows 10 or 11, x64.**
2. **Visual Studio 2022 Build Tools** (or full VS 2022) with the
   **Desktop development with C++** workload. This provides **MSVC v143** and a
   **Windows 10/11 SDK**. Verify with the *x64 Native Tools Command Prompt*.
   - The generator string is `"Visual Studio 17 2022"` and the platform `x64`.
3. **Git for Windows** — required for `vcpkg` *and* because the build fetches
   the Qt Advanced Docking System at configure time (see §5).
4. **Python 3** (for `aqtinstall`). Ensure `python`/`pip` are on `PATH`.
5. **CMake ≥ 3.21**; added to `PATH`. (The project declares CMake 3.16 as a
   minimum, but MSVC's C++20 mode needs a newer CMake to map
   `CMAKE_CXX_STANDARD 20` to `/std:c++20`.)
6. **vcpkg** (see §3).

> Do **not** mix toolchains: the Qt kit you install must match the compiler you
> configure with. This guide uses **MSVC** (`win64_msvc2022_64`). An MSVC vcpkg
> triplet plus a `win64_mingw` Qt install will not link (see §8).

---

## 2. Install Qt 6 (MSVC x64) with aqtinstall

```powershell
pip install aqtinstall

# Full desktop Qt (recommended; simplest):
aqt install-qt windows desktop 6.11.0 win64_msvc2022_64 -O C:\Qt
# Result: C:\Qt\6.11.0\msvc2022_64
```

The analyser needs the **qtbase** module (`Core`, `Widgets`, `Network`,
and `Test` for the unit-test suites) and, for the audio **output** backend,
the **Qt Multimedia** module (QAudioSink/WASAPI). The full desktop Qt from
aqt ships both; to fetch only what the build needs:

```powershell
aqt install-qt windows desktop 6.11.0 win64_msvc2022_64 -O C:\Qt --archives qtbase qtmultimedia
```

If you install **qtbase only**, Qt Multimedia is absent, `find_package(Qt6
COMPONENTS Multimedia QUIET)` fails, and the build still succeeds with the
audio output backend compiled out (the Audio tab reports "no backend"; DAB+
**decode** keeps working). To force that state explicitly when the module is
present, pass `-DDABX_ENABLE_QMULTIMEDIA=OFF`.

Either way, the prefix passed to CMake is `C:\Qt\6.11.0\msvc2022_64`.

---

## 3. Install the native dependencies with vcpkg

```powershell
git clone https://github.com/microsoft/vcpkg C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "User")
# Reopen the shell after setting VCPKG_ROOT.

C:\vcpkg\vcpkg install faad2 fftw3 zeromq cppzmq gtest --triplet x64-windows
```

Notes:

- **`zeromq` + `cppzmq`** are what enable `zmq+tcp://`. `cppzmq` supplies the
  `cppzmq::cppzmq` CMake target and `<zmq.hpp>`; `zeromq` supplies `libzmq`.
  If you omit them, the build still succeeds with the ZeroMQ transport
  compiled out (the GUI annotates/disables it and the validator rejects
  `zmq+tcp://` with a clear message).
- **`faad2`** enables DAB+ HE-AAC **decode** (real PCM in the CLI/audio tests).
- **`gtest`** is required because the top-level `CMakeLists.txt` always calls
  `find_package(GTest REQUIRED)` and adds the `tests/` subdirectory.
- **`fftw3`** is listed for parity with the Linux prerequisites; it is not
  currently linked by CMake.
- `pkg-config` is **not** required on Windows: the CMake detection ladder
  (config packages → `pkg-config` → `find_path`/`find_library`) never makes it
  mandatory.

---

## 4. Configure and build

Open the **x64 Native Tools Command Prompt for VS 2022** (so `cl.exe` and the
SDK are set up), then:

```powershell
cd C:\src\streamdab-analyser
$env:QT_QPA_PLATFORM = "offscreen"   # set BEFORE configure so GUI tests register

cmake -B build -S . -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH=C:\Qt\6.11.0\msvc2022_64 `
  -DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows

cmake --build build --config Release --parallel 8
```

If you do **not** want the ZeroMQ transport (or the vcpkg ports are
unavailable), add `-DDABX_ENABLE_ZMQ=OFF`. To force the (Linux-only) ALSA
backend off, add `-DDABX_ENABLE_ALSA=OFF` — it is already a no-op on Windows.

### Bounded parallelism — never `-j$(nproc)`

`--parallel 8` (or a similar explicit cap) is intentional. As on Linux, an
unbounded parallel build can thrash a 16-core machine: each `cl.exe` can hold
1–2 GB, and the Qt-ADS subproject adds its own targets. Pick a job count that
fits your RAM (roughly 2 GB per compiler process). `cmake --build ... --parallel`
without a number uses all cores — avoid it here.

The `scripts/build-windows.ps1` wrapper (§9) defaults to a sensible bounded
value.

---

## 5. Network access during configure (Qt-ADS FetchContent)

`CMakeLists.txt` pulls **Qt Advanced Docking System 4.5.0** via
`FetchContent` (`GIT_REPOSITORY https://github.com/githubuser0xFFFF/...`). A
first configure therefore needs **Git + internet access** and downloads the
source into `build/_deps/`.

For an offline / air-gapped machine, pre-populate the dependency and point
CMake at it (this is exactly how the scratch verification builds were made to
reuse an existing checkout):

```powershell
# One-time, on a machine with network:
git clone https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System.git C:\deps\qt-ads
# then, in the same checkout, reset to the pinned commit:
git -C C:\deps\qt-ads checkout 87cffe5d40307c61141c88b88c10c5e7d181d880

# When configuring offline:
cmake -B build -S . ... `
  -DFETCHCONTENT_SOURCE_DIR_QTADVANCEDDOCKINGSYSTEM=C:\deps\qt-ads
```

---

## 6. Run the tests

The test suites are registered only when a display environment is present.
Set the offscreen platform plugin **before** running `ctest`:

```powershell
$env:QT_QPA_PLATFORM = "offscreen"
ctest --test-dir build -C Release --output-on-failure
```

PowerShell sets the environment for the current session only. In a classic
`cmd.exe` shell use `set QT_QPA_PLATFORM=offscreen`; for a persistent value:
`[Environment]::SetEnvironmentVariable("QT_QPA_PLATFORM","offscreen","User")`.

### Tests that need the real ETI fixtures (not in git)

Several suites read `eti/bkk_20062022_141637.eti`, which is **gitignored** and
must be obtained separately (Thai L-band/S-band captures). Without it:

- `test_gui_dock_layout` and `test_dabplus_audio_decoder` **fail**: they assert
  the fixture exists in order to check exact parity (DLS+ = 1114 labels,
  MOT = 97/97 images, 0 MOT warnings).
- `test_cli_fig_trace` and `test_cli_fig_extended` **QSKIP**
  (`QSKIP("Real ETI fixture eti/bkk_20062022_141637.eti not found.")`).
- `test_network_stream_validation` **QSKIPs** only its TCP/ZMQ loopback cases
  (the URL-validator cases still run).

To run the suite without fixtures, exclude the two failing suites:

```powershell
ctest --test-dir build -C Release --output-on-failure `
  -E "test_gui_dock_layout|test_dabplus_audio_decoder"
```

Place the capture at `eti\bkk_20062022_141637.eti` (relative to the repository
root; the tests resolve it as `..\..\eti\...` from `build\tests`). To run a
single suite:

```powershell
ctest --test-dir build -C Release -R test_network_stream_validation --output-on-failure
.\build\tests\Release\test_dabplus_audio_decoder.exe
```

---

## 7. Running the applications

```powershell
# CLI (headless) on a capture:
.\build\Release\streamdab-cli.exe --input eti\bkk_20062022_141637.eti --output out.yaml

# GUI (needs a real desktop session, i.e. without QT_QPA_PLATFORM=offscreen):
.\build\Release\StreamDABAnalyser.exe
```

Build outputs are multi-config: binaries land in `build\Release\` (GUI/CLI)
and `build\tests\Release\` (test binaries).

---

## 7.5 Packaging — NSIS installer + ZIP (implemented)

`install()` rules + CPack are wired (`CMakeLists.txt` install section +
`cmake/packaging.cmake`), so Windows ships an NSIS installer plus a ZIP
fallback:

```powershell
# 1) Stage the install tree (bin\StreamDABAnalyser.exe, bin\streamdab-cli.exe,
#    share\doc\streamdab-analyser\...):
cmake --install build --prefix C:\staging

# 2) CPack artifacts (run inside build\ so they land there):
cd build
cpack -G NSIS      # StreamDAB_Analyser_<ver>_Setup.exe — needs makensis on PATH
cpack -G ZIP       # streamdab-analyser-<ver>-win64.zip (no tooling required)
```

The CPack NSIS generator is branded through the `CPACK_NSIS_*` variables
`cmake/packaging.cmake` (product name **StreamDAB Analyser**, homepage/contact
pointing at the repo). The standalone advanced template
`packaging/windows/installer.nsi` (with `installer_sections.nsh` and
`nsis_config.nsh`, all repo-relative via `${__FILEDIR__}`) is documented in
`packaging/README.md`; build it directly with:

```bat
makensis -DPRODUCT_VERSION=1.5.0 packaging\windows\installer.nsi
```

`PRODUCT_VERSION` is deliberately not hardcoded — read it from the build
(`build\CMakeCache.txt`, `CPACK_PACKAGE_VERSION:STRING=<ver>`).

## 8. Known gaps and limitations (honest)

- **Audio output now has a Windows backend when Qt Multimedia is installed.**
  The old statement "no Windows backend" is obsolete: the build auto-detects
  Qt Multimedia and compiles the `QAudioSink` (WASAPI) backend in
  (`src/core/qt_audio_output_sink.cpp`, `HAVE_QMULTIMEDIA`). With a **full**
  aqt Qt (qtbase + qtmultimedia) or the default vcpkg Qt, the Audio tab lists
  real output devices and playback works; with a qtbase-only install the
  backend is compiled out and `QtAudioOutputSink::open()` fails with *"Qt
  Multimedia backend not available in this build"*. Disable explicitly with
  `-DDABX_ENABLE_QMULTIMEDIA=OFF`. DAB+ **decode** (libfaad2) is independent
  of both choices and always works.
- **ZeroMQ requires the vcpkg ports.** Install `zeromq` + `cppzmq`; otherwise
  `zmq+tcp://` is refused with *"ZeroMQ support not built in"* (UDP multicast
  and TCP raw ETI are unaffected).
- **Qt-ADS is fetched at configure time** and needs Git/network once (§5).
- **macOS/Linux instructions are unchanged** — see `INSTALL.md` §1/§4.
- The top-level build always configures the test subdirectory, so
  `find_package(GTest REQUIRED)` runs even for a "GUI-only" configure. Install
  `gtest` (vcpkg) or the configure fails. (A `-DBUILD_TESTING=OFF` guard could
  be added later; it does not exist today.)

---

## 9. Helper script: `scripts/build-windows.ps1`

The script was rewritten to match reality. It:

- detects Qt (`-QtDir`, else `C:\Qt\6.11.0\msvc2022_64`, else `QTDIR`) and
  vcpkg (`VCPKG_ROOT` or `-VcpkgRoot`),
- configures with the VS 2022 generator, the vcpkg toolchain, and
  `-DVCPKG_TARGET_TRIPLET=x64-windows`,
- builds with a **bounded** `-Parallel` (default 8),
- sets `QT_QPA_PLATFORM=offscreen` **before** configure (so the GUI suites are
  registered) and runs `ctest` for you,
- supports `-DisableZmq` and `-EnableAlsa` (the real switch names),
  `-SkipTests`, `-Clean`, and `-Help`.

```powershell
.\scripts\build-windows.ps1                 # configure + build + tests (Release)
.\scripts\build-windows.ps1 -DisableZmq     # build without the ZeroMQ transport
.\scripts\build-windows.ps1 -Clean -Parallel 6
```

---

## 10. Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `Could not find a package configuration file provided by "Qt6"` | `CMAKE_PREFIX_PATH` wrong or Qt not installed. Point it at `C:\Qt\6.11.0\msvc2022_64` (the dir containing `lib\cmake\Qt6`). |
| `Could not find a package configuration file provided by "GTest"` | `gtest` not installed in the active vcpkg triplet. `vcpkg install gtest --triplet x64-windows`. |
| `unknown argument '-march=native'` / `'-O3'` from `cl.exe` | You are using a **stale** tree. Current `CMakeLists.txt` guards the GCC flags behind `if(MSVC)`. Re-pull/re-configure. |
| LNK errors about `Qt6Core.lib` vs `Qt6Cored.lib` | Mixed **Debug/Release** or mixed Qt kit. Build with `--config Release` and use the `msvc2022_64` kit. |
| `fatal error LNK1181: cannot open input file 'pthread.lib'` | Stale tree. Current CMake links `Threads::Threads` (empty on MSVC), never `-lpthread`. Re-configure. |
| Undefined references between Qt and the compiler | **MSVC vs MinGW mismatch**: a `win64_mingw` Qt kit cannot be used with MSVC (and vice versa). Install `win64_msvc2022_64` for this guide. |
| vcpkg complains about triplet `x64-windows-static` vs `x64-windows` | Keep the **same triplet** for `vcpkg install` and `-DVCPKG_TARGET_TRIPLET`. Do not mix static and dynamic triplets in one build tree. |
| vcpkg "is not a clean checkout" / rebuild loops | Delete the `build\` directory and re-configure from scratch with the correct toolchain file. |
| GUI tests are skipped / not registered | `QT_QPA_PLATFORM` was not set **before** running `cmake`. `$env:QT_QPA_PLATFORM = "offscreen"`, then delete `build\CMakeCache.txt` (or `build\`) and re-configure. |
| Thai/English labels show as boxes or mojibake | Use a UTF-8 aware console (`chcp 65001`) and a Thai-capable font (e.g. *TH Sarabun New* / *Leelawadee UI*). The analyser stores and processes UTF-8 internally; the fixtures carry Thai labels. |
| Build machine becomes unresponsive | Too many parallel compilers. Cap with `--parallel 8` (or lower); never use an unbounded `--parallel`. |

---

## 11. What was changed to make this work (for maintainers)

- `CMakeLists.txt`: `find_package(PkgConfig)` is now `QUIET` (never
  `REQUIRED`); ZeroMQ uses a three-step detection ladder and exports
  `HAVE_ZMQ`/include dirs/link target only when found; `DABX_ENABLE_ZMQ` and
  `DABX_ENABLE_ALSA` options added; GCC-only release flags guarded behind
  `if(MSVC)`; `pthread` replaced by `find_package(Threads)` +
  `Threads::Threads`.
- `src/core/network_stream_receiver.cpp`: `#include <zmq.hpp>` and the ZMQ
  implementation are guarded by `#ifdef HAVE_ZMQ`; a stub keeps the
  `unique_ptr<ZmqState>` valid and the validator rejects `zmq+tcp://` with a
  clear message when the transport is absent.
- `src/core/qt_audio_output_sink.{hpp,cpp}`: new Qt Multimedia (QAudioSink)
  output backend with the same public API as the ALSA `AudioOutputSink` and
  the same graceful-degradation stubs when `HAVE_QMULTIMEDIA` is unset. The
  playback controller picks ALSA on Linux / Qt Multimedia elsewhere
  (`preferredBackendName()`), so the Audio tab works on WASAPI out of the box
  when the Qt kit ships Multimedia.
- `src/main.cpp`: the ZeroMQ quick-example entry, the URL tooltip and the
  "Supported formats" dialog are conditional on `HAVE_ZMQ` (the entry is
  disabled **and** annotated when unavailable).
- `tests/test_network_stream_validation.cpp`: ZMQ include and loopback test
  guarded; without `HAVE_ZMQ` the validator-rejection branch is asserted and
  the loopback test QSKIPs.
