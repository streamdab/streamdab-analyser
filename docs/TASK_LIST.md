# Task List

Living checklist for active work. Check off items when done (add commit ref).

## v1.3 — GUI migration to Qt-ADS (branch `feature/gui-qtads`)

- [x] **T1: Rename main executable output to `StreamDABAnalyzer`** *(requested 2026-10-01, from `docs/ETI_TEST_DATA.md` run instructions)*
  - CMake: keep target `DABAnalyzer_Enhanced_FilteringBuild` (tests reference it) but set `OUTPUT_NAME StreamDABAnalyzer`, so the runnable becomes `./build/StreamDABAnalyzer`.
  - Update live docs that reference the legacy binary name `./DABAnalyzer_Enhanced_FilteringBuild` (runnable is now `./build/StreamDABAnalyzer`): `README.md`, `docs/ETI_TEST_DATA.md`, `docs/SESSION_RESUMPTION_GUIDE.md`, `docs/PDCA_WORKFLOW.md`, `docs/fixes/*`, `docs/agents/*`, and other non-archive docs. **Skip `docs/archive/`** (historical record).
  - Verify: rebuild + run `./build/StreamDABAnalyzer` + `ctest`.

- [ ] **T2: Qt-ADS 4.5.0 layout-preserving migration** *(revised 2026-10-02: placeholder-panel experiment rejected — restore original `DABAnalyzerWindow` layout, wrap existing panels in per-tab `CDockManager`s; 16 tab1 docks + 3 tab2 + 2 tab3 + 5 shared bottom strip; QSettings persistence; auto-hide enabled)*
  - View-menu dock toggles / per-tab bottom strips are NOT in v1.3 (documented decisions); rich panel content porting not needed (panels preserved from original).
  - Reference: `docs/gui/QT_ADS_DOCKING.md` (architecture + 26-dock registry, QSettings keys).

- [ ] **T3: v1.3 code review + commit** — review must pass before commit (AGENTS.md mandatory workflow), then commit on `feature/gui-qtads`.
- [ ] **T4: v1.3.x QA — manual drag/float/auto-hide interaction pass on the ship layout** (headless CI covers only visibility + floating round-trip).

## Later (see `docs/PHASE_E_PLAN.md`)

- v1.4 — test closeout (fix `test_performance_fig019_parsing_speed` 7.6 µs vs 5 µs threshold and `test_fig10_basic_parsing` <50 µs flake (31–51 µs measured) → 100 % pass).
- v2.0 — EWS (TS 104 089/090) on its own branch; then E-2 EDI.
