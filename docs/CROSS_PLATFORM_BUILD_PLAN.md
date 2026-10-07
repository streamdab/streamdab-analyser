# Cross-Platform Build Plan — Linux / Windows / macOS

**Date:** 2026-10-07 · **Branch:** `main` (HEAD `0f33f9a`) · **Status:** plan (Linux verified; Windows worker in progress; macOS to do)
**Goal:** one CMake project that configures, builds and tests on **Linux, Windows 10/11 and macOS** (x86_64 + Apple Silicon), with per-platform docs and presets.

---

## 1. Current state per platform

| Platform | Configure | Build | Tests | Notes |
|---|---|---|---|---|
| **Linux** (reference) | ✅ | ✅ 0 errors/0 warnings | ✅ 55/55 | Qt 6.11, GCC 13; the only verified platform |
| **Windows 10/11 (MSVC)** | ❌ | ❌ | — | Hard blockers below (pkg-config, flags, pthread) |
| **macOS (Clang, arm64/x86_64)** | ⚠️ | ⚠️ | — | Same class of blockers + `-march=native` on arm64; never attempted |

## 2. Blockers found (evidence)

| # | Blocker | Where | Why it breaks | Fix |
|---|---|---|---|---|
| B1 | `find_package(PkgConfig REQUIRED)` + `pkg_check_modules(ZMQ REQUIRED libzmq)` | `CMakeLists.txt:14-15` | Windows/macOS have no pkg-config by default (vcpkg/brew `.pc` not guaranteed) → **configure fails** | Portable detection ladder (B1 fix, in progress by the Windows worker): `find_package(ZeroMQ/cppzmq CONFIG)` → `pkg_check_modules(ZMQ QUIET)` (only if PkgConfig found, made QUIET) → `find_path`/`find_library` with vcpkg/brew hints → define `HAVE_ZMQ` only when found |
| B2 | Unguarded `#include <zmq.hpp>` + ZMQ code | `src/core/network_stream_receiver.cpp:21` | Without cppzmq the *compile* fails everywhere | `#ifdef HAVE_ZMQ` around include + ZMQ transport; validator rejects `zmq+tcp://` with "ZeroMQ support not built in"; quick example annotated; UDP/TCP unaffected |
| B3 | GCC-only Release flags set unconditionally | `CMakeLists.txt:63` | MSVC rejects `-march=native -mtune=native -ffast-math …`; **Apple Silicon rejects `-march=native`** | Branch per compiler/arch (see §3.1) |
| B4 | Raw `pthread` link | main + tests link lines | MSVC has no `pthread` | `find_package(Threads REQUIRED)` + `Threads::Threads` (portable) |
| B5 | Audio output backend is **ALSA only** | `src/core/audio_output_sink.cpp` (`HAVE_ALSA`) | Windows/macOS get no audio out (decode/analysis fine) | Keep the guarded no-backend path (verify no-ALSA build); **follow-up**: Qt Multimedia sink (WASAPI/CoreAudio). Document as a known gap, not a build blocker |
| B6 | No platform guards at all | `CMakeLists.txt` (`grep WIN32/APPLE/MSVC` = 0) | — | Add explicit, minimal `if(MSVC)/if(APPLE)/if(UNIX)` blocks where needed |
| B7 | Stale `scripts/build-windows.ps1` (options that no longer exist; version drift) | `scripts/` | Misleading | Rewrite to the real flow (vcpkg + Qt prefix + bounded `--parallel` + offscreen tests) |
| B8 | Dead platform config files, never included | `cmake/{Linux,MacOS,Windows}Config.cmake`, `cmake/packaging.cmake` | Confusing (looks like support that is not wired) | Decide: wire the useful parts (e.g. packaging) or delete; do **not** leave as-is |
| B9 | No presets / no per-OS Qt prefix hints | — | Users hand-craft commands | `CMakePresets.json` presets conditioned on `hostSystemName`, Qt prefix from `$env{QT_DIR}`, vcpkg toolchain hint for Windows |
| B10 | `find_package(GTest REQUIRED)` | `CMakeLists.txt:197` | Fine on all three (vcpkg/brew/git), but makes a GUI-only build impossible | Keep REQUIRED for the current always-build-tests setup; document the vcpkg/brew package names |

## 3. Planned changes

### 3.1 CMake → portable flags + threads
```cmake
# Release flags per toolchain (was: unconditional GCC flags)
if(MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG /DRELEASE_BUILD")
else()
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -ffast-math -funroll-loops -finline-functions -DRELEASE_BUILD")
  # -march/-mtune=native only where the target actually supports it (x86_64)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " -march=native -mtune=native")
  endif()
endif()

find_package(Threads REQUIRED)      # replaces the raw `pthread` link
# ... target_link_libraries(... Threads::Threads)
```

### 3.2 Dependencies per platform
| Dependency | Linux | Windows (vcpkg) | macOS (brew) |
|---|---|---|---|
| Qt 6 (Core/Widgets/Network/Test) | `qt6-base-dev` | `aqt install-qt windows desktop 6.11.0 win64_msvc2022_64` | `brew install qt@6` |
| Qt-ADS 4.5.0 | FetchContent | FetchContent | FetchContent |
| libfaad2 (DAB+ audio) | `libfaad-dev` | `vcpkg install faad2` | `brew install faad2` |
| libfftw3 | `libfftw3-dev` | `vcpkg install fftw3` | `brew install fftw` |
| ZeroMQ + cppzmq (ETI over ZMQ) | `libzmq3-dev` (+ `cppzmq-dev` optional) | `vcpkg install zeromq cppzmq` | `brew install zeromq cppzmq` |
| GTest | `libgtest-dev` | `vcpkg install gtest` | `brew install googletest` |
| ALSA (audio out) | `libasound2-dev` | n/a (no backend yet) | n/a (no backend yet) |
| Threads | `pthread` | `Threads::Threads` (empty) | `Threads::Threads` |

### 3.3 Presets (`CMakePresets.json`)
- configure presets conditioned on `hostSystemName` — `linux-gcc`, `windows-msvc` (adds `CMAKE_TOOLCHAIN_FILE=$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake` when set), `macos-brew` (`CMAKE_PREFIX_PATH=$env{QT_DIR}` or `$(brew --prefix qt@6)`).
- build presets keep the bounded job counts (12 default / 4 low-memory / 1 serial) — same rationale as the Linux work (`scripts/build_jobs.sh`).
- test presets: `$env{QT_QPA_PLATFORM="offscreen"}` on the offscreen-capable suites.

### 3.4 Scripts
- `scripts/build_jobs.sh` (done, Linux/macOS) — Windows: `build-windows.ps1` computes `[Math]::Max(1, [Math]::Min(8, (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors))` and never uses unbounded parallelism.
- Optional `scripts/build-macos.sh` wrapper (brew paths + presets).

### 3.5 Docs
- `docs/WINDOWS_BUILD.md` (detailed Windows 10/11 guide; in progress) — prerequisites (VS 2022 Build Tools, aqt, vcpkg), exact commands, run/test, troubleshooting (Qt vs toolchain mismatch: **never mix `win64_mingw` with MSVC**; triplet mismatch; bounded jobs).
- `docs/MACOS_BUILD.md` (new) — Homebrew deps, `cmake -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"`, arm64 vs x86_64 (`-march=native` removed for arm64), offscreen tests, codesign/notarize notes for shipping a `.app`, known gaps.
- `INSTALL.md` — a cross-platform matrix at the top (OS → packages → configure/build/test commands) linking both platform docs; `docs/README.md` updated.
- Honest statements: audio-out gap on Windows/macOS; ZMQ requires cppzmq; fixtures (`eti/*.eti`) are not in git so fixture-based suites skip without them.

## 4. Verification strategy

| Platform | How it is verified |
|---|---|
| **Linux** | Native: build 0 warnings, `ctest` 55/55, fixture parity, CLI smoke — **and** two proxy configurations that simulate the missing deps: `-DDABX_ENABLE_ZMQ=OFF` and `-DDABX_ENABLE_ALSA=OFF` (both must configure/build; ZMQ rejects `zmq+tcp://` clearly; audio falls back to "no backend") |
| **Windows** | Cannot be compiled in this environment → the CMake guards are written so an MSVC configure cannot hit GCC flags/pkg-config; a **manual checklist** is provided (expected output per step + which log to send on failure); recommended: a GitHub Actions `windows-2022` job (see §5) |
| **macOS** | Same as Windows: guards + `docs/MACOS_BUILD.md` checklist; recommended CI job `macos-14` |

## 5. CI (recommended, follow-up)
`.github/workflows/*` are stale. Recommended replacement: a 3-OS matrix job (`ubuntu-24.04`, `windows-2022`, `macos-14`) that installs the deps per §3.2, configures with the OS preset, builds with bounded jobs, and runs the offscreen suites (fixture-dependent suites skipped or fed a small fixture artifact).

## 6. Tasks / sequencing
1. **B1+B2 (ZMQ portability)** — in progress (Windows worker): detection ladder + `HAVE_ZMQ` guard + docs.
2. **B3+B4 (flags, Threads)** — cross-platform hardening in `CMakeLists.txt` + `tests/CMakeLists.txt` (do after step 1 lands to avoid CMakeLists conflicts).
3. **B7–B9 (script, dead configs, presets)** — same pass as step 2.
4. **B5-follow-up** — Windows/macOS audio backend (Qt Multimedia sink) as its own task after the builds are green.
5. **Docs (§3.5)** + CI matrix (§5).
6. Each step: build + `ctest` 55/55 + parity on Linux; commit separately.

## 7. Risks / open questions
- **Audio backend scope** (B5): defer vs implement now — a Qt Multimedia sink adds `qt6-multimedia-dev` (Linux) / Qt Multimedia kit (Windows/macOS) and a new backend path; recommend deferring until the three builds are green.
- **MSVC flag semantics**: replacing `-ffast-math -O3` with `/O2` changes numerics slightly; the perf-threshold tests are Linux-tuned (<50 µs), so Windows runs may need the same budget treatment as v1.4 (documented).
- **macOS deployment**: shipping a `.app` needs an install()/CPack step (currently absent — INSTALL.md §6 gap).
- **Fixtures in CI**: `eti/*.eti` (~30 MB) are gitignored; CI either skips fixture suites or downloads them.