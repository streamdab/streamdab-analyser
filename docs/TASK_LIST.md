# Task List

Living checklist for active work. Check off items when done (add commit ref).

## v1.3 — GUI migration to Qt-ADS (branch `feature/gui-qtads`)

- [x] **T1: Rename main executable output to `StreamDABAnalyzer`** *(requested 2026-10-01, from `docs/ETI_TEST_DATA.md` run instructions)*
  - CMake: keep target `DABAnalyzer_Enhanced_FilteringBuild` (tests reference it) but set `OUTPUT_NAME StreamDABAnalyzer`, so the runnable becomes `./build/StreamDABAnalyzer`.
  - Update live docs that reference the legacy binary name `./DABAnalyzer_Enhanced_FilteringBuild` (runnable is now `./build/StreamDABAnalyzer`): `README.md`, `docs/ETI_TEST_DATA.md`, `docs/SESSION_RESUMPTION_GUIDE.md`, `docs/PDCA_WORKFLOW.md`, `docs/fixes/*`, `docs/agents/*`, and other non-archive docs. **Skip `docs/archive/`** (historical record).
  - Verify: rebuild + run `./build/StreamDABAnalyzer` + `ctest`.

- [x] **T2: Qt-ADS 4.5.0 layout-preserving migration** *(revised 2026-10-02: placeholder-panel experiment rejected — restore original `DABAnalyzerWindow` layout, wrap existing panels in per-tab `CDockManager`s; 16 tab1 docks + 3 tab2 + 2 tab3 + 5 shared bottom strip; QSettings persistence; auto-hide enabled)* — committed `263bcab`.
  - View-menu dock toggles / per-tab bottom strips are NOT in v1.3 (documented decisions); rich panel content porting not needed (panels preserved from original).
  - Reference: `docs/gui/QT_ADS_DOCKING.md` (architecture + 26-dock registry, QSettings keys).

- [x] **T3: v1.3 code review + commit** — review PASSED (no CRITICAL/HIGH; 3 MEDIUM + 4 LOW all addressed), committed `263bcab` on `feature/gui-qtads`.
- [ ] **T4: v1.3.x QA — manual drag/float/auto-hide interaction pass on the ship layout** (headless CI covers only visibility + floating round-trip).

## ETI/FIG decode parity (2026-10-02, on `feature/gui-qtads`)

- [x] **T5: ETI-NI/FIG decode root-cause fix** (CLI vs etisnoop mismatch) — FC bit layout (NST=`p[5]&0x7F` no −1, FICF=`p[5]` bit7), FIC at `12+4*NST` (96/128 B, 3/4 FIBs × 30 B data + 2 B FIB CRC), MSC offset `12+4*NST+fic_len` — committed (see T7).
- [x] **T6: Standards review (EN 300 799/EN 300 401/TS 101 756 vs etisnoop)** — PASSED after F1–F9; record in `docs/fixes/ETI_FIC_DECODE_STANDARDS_REVIEW.md`; option-variant matrix (14 decision points) in `docs/fixes/OPTION_VARIANT_MATRIX.md`.
- [x] **T7: Commit decode fix + review fixes** — committed with the F1–F9 fix wave + review docs (see commit log; ctest 44/45, only known v1.4 perf failure).
- [x] **T8: ETI/FIG settings feature** — implement the matrix `SETTING` rows (rows 1,2,3,4,5,6,8,9,10,11,12,14): `AnalyzerSettings` (QSettings `analyzer/...`) + GUI section in `settings_dialog.cpp` + CLI flags; **etisnoop-parity defaults** (matrix `KEEP` column) — output stays byte-identical on default config. — committed `fe1c431` (ctest 45/46).
- [x] **T9: Matrix FIX leftovers** — auto raw-FIC fallback when all FIB CRCs fail (row 4, `DABX_FIC_MODE` env hook) + tail TIST for ETI-NI (row 10) — committed (see commit log).
- [x] **T10: UI redesign (v2) — Tab-1 regroup + Input toolbar + Messages→strip + layout v2** — plan `docs/gui/UI_REDESIGN_PLAN.md` (docs-only commit `2c4055a`); implementation: Input Source dock → `inputToolbar` (page top), grouped tabbed dock areas (Transport / Signal Analysis / Frame Analysis / Signal Info / Error & Statistics), CU Usage moved to center, Messages → `bottom_Messages` (6th strip tab), `docking/layout_version = 2` one-time reset, first-launch sizes ≈ mockup, test + `QT_ADS_DOCKING.md` updated (25 docks: 14 tab1 + 3 tab2 + 2 tab3 + 6 bottom; ctest 45/46 with only the known v1.4 perf failure; CLI parity byte-identical vs `fe1c431`; review PASSED — default active tab per group fixed + null-safety/test gap closed).
- [x] **T11: CLI FIG detail (P1)** — `--fig-trace` (per-frame FIG instances + readable label/ECC/ensemble values + reconfig flags) + `figs:` inventory + `fic_health:` (FIB CRC / erroneous FIG / MCI reconfig) + `--max-frames` enforced — committed `94126b4`; evaluation + P2/P3 backlog in `docs/fixes/CLI_FIG_COVERAGE.md`.
- [x] **T12: GUI panel real-data wiring** — per `docs/gui/PANEL_AUDIT.md`: GUI decode path fix (FIB-aware, charset, offsets) + simulator removed + Tab 2 real data + Tab 3 FIG tracer/details tree + FIC overview real + FIB-CRC error row + short-label etisnoop parity + reload reset — committed `bd47999` (GUI dock tests 14/14 on the real fixture; review findings 1–10 fixed).

## v1.4 close-out (2026-10-02)

- [x] **T13: Perf budgets retargeted** (week1/week2/fig015 → measured capability + margin, documented) `a5a1a56`; **FIG backlog P2/P3** (1/2+1/3, 0/21, 0/22, type 2, --figs-by-type, --fib-hex, EEP/UEP tables, CLI 1/1 P/D align) `16f948f` — **ctest 49/49 = 100%**.
- [x] **T14: Housekeeping** — QDateTime deprecations fixed (0 warnings), 7 tracked *.backup removed, LI-TIST note — `ea07065`.
- [ ] **T15: Coverage re-measure + merge feature→main + tag** (in progress) — then v2.0 EWS branch.

## Later (see `docs/PHASE_E_PLAN.md`)

- v1.4 — test closeout (fix `test_performance_fig019_parsing_speed` 7.6 µs vs 5 µs threshold and `test_fig10_basic_parsing` <50 µs flake (31–51 µs measured) → 100 % pass).
- v2.0 — EWS (TS 104 089/090) on its own branch; then E-2 EDI.
