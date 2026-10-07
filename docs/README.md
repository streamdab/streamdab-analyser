# Documentation Index

**Project:** StreamDAB Analyzer — Professional DAB/ETI Stream Analyzer (Qt6/C++17)
**Main plan:** [`PHASE_E_PLAN.md`](PHASE_E_PLAN.md) — the single operational plan. If any doc contradicts it, it wins.
**Reorganized:** 2026-09-27 (2nd reorganization + audit; 62 obsolete docs moved to `archive/`).
**Root docs:** [`README.md`](../README.md) (overview) · [`CLAUDE.md`](../CLAUDE.md) (historical) · [`AGENTS.md`](../AGENTS.md) (live instructions)

> Rule: keep the repo root clean — only `README.md` + `CLAUDE.md` + `AGENTS.md`. All new docs go into the
> subfolder below that fits best. Completed phase docs move to `archive/` when superseded.

## Folder map

| Folder | Contents | Files |
|--------|----------|-------|
| `PHASE_E_PLAN.md` (docs root) | **Main plan:** E-0 verify → E-1 EWS → E-2 EDI + backlog | 1 |
| `pdca/` | PDCA completion reports (Weeks 1–8, all finished) | 13 |
| `fig/` | Per-FIG guides, quickstarts, delivery reports (patches consumed → archive) | 17 |
| `agents/` | Individual agent deliverable / fix reports (Agents 1–48) | 18 |
| `fixes/` | Bug reports + fix summaries (segfault, parser, labels, services, ETI/FIG decode review) | 11 |
| `eti-ni/` | ETI-NI parser completion + validation (Nov 2025) | 4 |
| `releases/` | Live status: validation, test status, CLI build, verbose mode | 6 |
| `gui/` | Qt-ADS docking architecture + usage (v1.3 GUI migration) | 1 |
| `archive/` | Historical docs: all superseded plans, snapshots, consumed patches | 111 |
| `docs/` root | Live specs, guides, style, session docs, v1.3 task list | 21 |

## Start here

- **Main plan:** `PHASE_E_PLAN.md` (v1.3 GUI migration → v1.4 test closeout → E-1 EWS → E-2 EDI)
- **Current status:** `releases/QUICK_STATUS.md` (ETI-NI bug fixed in source `bb0389f`, runtime check pending) →
  `eti-ni/ETI_NI_PARSER_COMPLETE.md`
- **Test status:** `releases/TEST_STATUS_REPORT.md` (97.7% pass, 34.4% coverage)
- **Instructions:** `../AGENTS.md` (build/test commands, workflow, repo rules)
- **Cross-platform build plan (Linux/Windows/macOS):** [`CROSS_PLATFORM_BUILD_PLAN.md`](CROSS_PLATFORM_BUILD_PLAN.md)
- **Windows build guide (detailed):** [`WINDOWS_BUILD.md`](WINDOWS_BUILD.md) — VS 2022 + Qt 6 MSVC + vcpkg,
  `DABX_ENABLE_ZMQ`/`DABX_ENABLE_ALSA`, offscreen tests, fixture requirements, known gaps, troubleshooting
- **Configuration:** `CONFIGURATION.md` (store location per platform, key groups, reset/export, removed options)
- **v1.3 spec:** GUI migration to Qt-ADS — task list [`TASK_LIST.md`](TASK_LIST.md), design
  [`gui/QT_ADS_DOCKING.md`](gui/QT_ADS_DOCKING.md), plan `PHASE_E_PLAN.md` v1.3 section

## `pdca/` — PDCA completion reports (all weeks finished)

- `PDCA_WEEK1_*` (4 files) — Week 1: FIG 1/0, TS 101 756 tables, 72/72 tests
- `PDCA_WEEK4_COMPLETE.md`, `PDCA_WEEK5_COMPLETE.md`
- `PDCA_WEEK6_AGENTS_COMPLETE.md`, `PDCA_WEEK6_CHECK_COMPLETE.md` — FIG 0/8, 0/9, 0/14 (69/69)
- `PDCA_WEEK7_BATCH{1,2,3,4}_COMPLETE.md`, `PDCA_WEEK7_V1.1_COMPLETE.md` — 100% ETSI EN 300 401 (22/22)
- (All PLAN docs for executed work retired to `archive/`. No Week 9 plan — see `PHASE_E_PLAN.md`.)

## `fig/` — FIG implementation docs

- `FIG007_*` — FIG 0/7 delivery, guide, quickstart (patch consumed → archive)
- `FIG009_*` — FIG 0/9 guide, quick-ref (patches consumed → archive)
- `FIG010_IMPLEMENTATION_SUMMARY.md` (+ MJD fix → see `fixes/FIG010_MJD_FIX_REPORT.md`)
- `FIG011_IMPLEMENTATION_SUMMARY.md`, `FIG012_IMPLEMENTATION_SUMMARY.md`
- `FIG014_*` — FIG 0/14 guide, quickstart (patch consumed → archive; guides still referenced by `verify_fig014_integration.sh`)
- `FIG016_IMPLEMENTATION_REPORT.md`, `FIG017_*` (delivery, guide, quickstart), `FIG_018_IMPLEMENTATION_REPORT.md`, `FIG019_AGENT2_COMPLETION_REPORT.md`, `FIG024_COMPLETION_REPORT.md`

## `agents/` — Agent reports

Agents 1, 15, 16, 20, 21 (×3), 36 (×2), 37–43, 48. Key: `AGENT48_COMPLETION_SUMMARY.md`
(ETI-NI final validation), `AGENT40_YAML_TESTS_FIXED.md`, `AGENT41_COVERAGE_REPORT.md`,
`AGENT38_MOC_FIX_COMPLETE.md`, `AGENT42_COMPLETION_REPORT.md`, `AGENT43_ETI_FORMAT_DETECTION.md`.

## `fixes/` — Bug reports & fixes

`BUG_REPORT_CLI_PARSER.md`, `CLI_PARSER_FIX_SUMMARY.md`, `CRITICAL_FIX_SERVICES_UNNAMED.md`,
`ETI_FIC_DECODE_STANDARDS_REVIEW.md` (2026-10-02: EN 300 799/EN 300 401/TS 101 756 review of the
ETI/FIG decode fix + F1–F9 findings + etisnoop parity tables),
`FIG010_MJD_FIX_REPORT.md`, `FINAL_FIX_SUMMARY.md`, `FINAL_SEGFAULT_FIX.md`,
`OPTION_VARIANT_MATRIX.md` (2026-10-02: 14 decode decision points — standard options vs etisnoop
default vs ours; roadmap for the analyzer-settings feature),
`SEGFAULT_FIX_COMPLETE.md`, `SEGFAULT_FIX_SUMMARY.md`, `SERVICE_LABELS_FIX_SUMMARY.md`.

## `eti-ni/` — ETI-NI parser (Nov 2025)

`ETI_NI_PARSER_COMPLETE.md` (production-ready) · `ETI_NI_PARSER_STATUS.md` ·
`ETI_NI_PARSER_SUCCESS_SUMMARY.md` · `SUBCHANNEL_COMPARISON.md` (vs etisnoop reference).

## `releases/` — Live status docs

- Validation: `FINAL_VALIDATION_REPORT.md` (bannered), `QUICK_STATUS.md` (bannered), `VERBOSE_OUTPUT_COMPLETE.md`
- Tests/builds: `TEST_STATUS_REPORT.md` (bannered), `CLI_BUILD_SUCCESS_v1.2.md` (bannered)
- History of reorganizations: `DOCUMENTATION_REORGANIZATION_REPORT.md` (1st reorg, Oct 2025)

## `gui/` — Qt-ADS GUI docking (v1.3)

- `QT_ADS_DOCKING.md` — architecture + usage for the layout-preserving Qt Advanced Docking System 4.5.0
  migration: 4 per-scope `CDockManager`s, all-26-dock registry, QSettings persistence keys, out-of-scope
  decisions. Runnable is `./build/StreamDABAnalyzer`.

## Active specs (docs root)

ETI: `ETI_PROTOCOL_SPECIFICATIONS.md`, `ETI_LI_FORMAT_NOTES.md`,
`ETI_NI_FRAME_DIAGRAM.txt`, `ETI_FORMAT_DETECTION_USAGE.md`, `ETI_TEST_DATA.md` ·
ETSI: `ETSI_COMPLIANCE_FRAMEWORK.md` (header pre-dates v1.1 100% — minor) ·
Phases: `PHASE_E_PLAN.md` (**main plan**), `PHASE_3B_{TECHNICAL_NOTES,USER_GUIDE}.md`,
`PHASE_2A_WEEK6_DELIVERABLE_REPORT.txt` ·
Testing: `GUI_PATTERNS.md` ·
Qt6: `Qt6_CPP20_Style_Guide.md` (cited by `AGENTS.md`), `SERVICE_TREE_VISUAL_GUIDE.txt` ·
Tasks: `TASK_LIST.md` (living v1.3 checklist) ·
Other: `DATA_AVAILABILITY_MATRIX.md`, `DEPLOYMENT_GUIDE.md`, `LICENSE_COMPLIANCE_REPORT.md`,
`FUTURE_FEATURES.md` (v2.0 idea pool), `SESSION_RESUMPTION_GUIDE.md`, `PDCA_WORKFLOW.md`,
`README.md` (this index).
