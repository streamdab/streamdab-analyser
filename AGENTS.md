# StreamDAB Analyser — Project Instructions

Qt6 / C++20 DAB/ETI stream analyser. CMake 3.16+. ETSI EN 300 799 (ETI), EN 300 401 (DAB FIG).

**Current Phase:** v1.3 — GUI Migration to Qt-ADS (feature/gui-qtads branch)
**Next:** v1.4 test closeout → v2.0 EWS/EDI (see `docs/PHASE_E_PLAN.md`)

## Build / Test / Verify

```bash
sudo apt-get install -y cmake qt6-base-dev libfaad-dev libfftw3-dev libzmq3-dev build-essential
export QT_QPA_PLATFORM=offscreen                  # BEFORE configure: enables every GUI test target
JOBS="$(bash scripts/build_jobs.sh)"              # bounded: min(nproc, RAM_GB/2, 8) -> 8 here
export CMAKE_BUILD_PARALLEL_LEVEL="$JOBS"         # nested cmake/make (Qt-ADS) honour this too
cmake -B build -S . && cmake --build build --parallel "$JOBS"   # build (zero errors/warnings)
cd build && ctest --output-on-failure             # tests (must be 100% pass)
cd .. && ./build/streamdab-cli --input eti/bkk_20062022_141637.eti --output /tmp/opencode/smoke.yaml  # CLI smoke test
```

> Never use a bare `-j` / `--parallel`: with no value make runs **unbounded**
> (one compiler per target), which forks the machine to a standstill.

- Real ETI fixtures live in `eti/` — use them for E2E validation, never synthetic-only tests.
- Sanitizers/Valgrind expected clean on touched paths (memory- and thread-safe code).

## Conventions

- Follow `docs/Qt6_CPP20_Style_Guide.md` (Qt naming, RAII, const-correctness, Doxygen on public APIs).
- Thai UTF-8 handling is a first-class requirement (labels, DLS, EPG) — never break it silently.
- Session continuity: `docs/SESSION_RESUMPTION_GUIDE.md`; doc map: `docs/README.md`.

## Mandatory workflow (every feature/fix)

Coding (`cpp-qt-developer` subagent) → unit tests → build → E2E on real `eti/` data → code review (**must pass before commit**) → commit with What/Why/Testing. No shortcuts. Full definition: `.opencode/agents/pm-agent.md`.

## Repo rules

- Root stays clean: only `README.md`, `INSTALL.md`, `AGENTS.md`. New docs go to themed `docs/` folders (`pdca/`, `fig/`, `agents/`, `fixes/`, `eti-ni/`, `releases/`); completed-phase docs move to `docs/archive/`. `CLAUDE.md` is historical and lives at `docs/archive/CLAUDE.md`.
- Never commit build output, `output.yaml` churn, or test artifacts.

## Agents

- `cpp-qt-developer` (subagent): all C++/Qt6 implementation, fixes, tests, scripts.
- `pm-agent` (primary/subagent): phase planning + parallel worker orchestration.
- Reconnaissance: built-in `explore`; multi-step research: built-in `general`.
