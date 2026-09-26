# StreamDAB Analyzer — Project Instructions

Qt6 / C++17 DAB/ETI stream analyzer. CMake 3.16+. ETSI EN 300 799 (ETI), EN 300 401 (DAB FIG).

## Build / Test / Verify

```bash
cmake -B build -S . && cmake --build build        # build (must be zero errors, zero warnings)
cd build && ctest --output-on-failure             # tests (must be 100% pass)
./build/streamdab-cli --input eti/bkk_20062022_141637.eti --output /tmp/opencode/smoke.yaml  # CLI smoke test
```

- Real ETI fixtures live in `eti/` — use them for E2E validation, never synthetic-only tests.
- Sanitizers/Valgrind expected clean on touched paths (memory- and thread-safe code).

## Conventions

- Follow `docs/Qt6_CPP20_Style_Guide.md` (Qt naming, RAII, const-correctness, Doxygen on public APIs).
- Thai UTF-8 handling is a first-class requirement (labels, DLS, EPG) — never break it silently.
- Session continuity: `docs/SESSION_RESUMPTION_GUIDE.md`; doc map: `docs/README.md`.

## Mandatory workflow (every feature/fix)

Coding (`cpp-qt-developer` subagent) → unit tests → build → E2E on real `eti/` data → code review (**must pass before commit**) → commit with What/Why/Testing. No shortcuts. Full definition: `.opencode/agents/pm-agent.md`.

## Repo rules

- Root stays clean: only `README.md`, `CLAUDE.md` (historical), `AGENTS.md`. New docs go to themed `docs/` folders (`pdca/`, `fig/`, `agents/`, `fixes/`, `eti-ni/`, `releases/`); completed-phase docs move to `docs/archive/`.
- Never commit build output, `output.yaml` churn, or test artifacts.

## Agents

- `cpp-qt-developer` (subagent): all C++/Qt6 implementation, fixes, tests, scripts.
- `pm-agent` (primary/subagent): phase planning + parallel worker orchestration.
- Reconnaissance: built-in `explore`; multi-step research: built-in `general`.
