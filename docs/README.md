# Documentation Index

**Project:** StreamDAB Analyzer — Professional DAB/ETI Stream Analyzer (Qt6/C++17)
**Reorganized:** 2026-09-27 (2nd reorganization; see `releases/DOCUMENTATION_REORGANIZATION_REPORT.md` for the 1st, Oct 2025)
**Root docs:** [`README.md`](../README.md) (overview) · [`CLAUDE.md`](../CLAUDE.md) (session guide)

> Rule: keep the repo root clean — only `README.md` + `CLAUDE.md`. All new docs go into the
> subfolder below that fits best. Completed phase docs move to `archive/` when superseded.

## Folder map

| Folder | Contents | Files |
|--------|----------|-------|
| `pdca/` | PDCA multi-agent plans + completion reports (Week 1–8, ETI-NI) | 23 |
| `fig/` | Per-FIG implementation guides, quickstarts, delivery reports, code patches | 23 |
| `agents/` | Individual agent deliverable / fix reports (Agents 1–48) | 18 |
| `fixes/` | Bug reports + fix summaries (segfault, parser, labels, services) | 9 |
| `eti-ni/` | ETI-NI parser completion + validation (Nov 2025) | 4 |
| `releases/` | Build summaries, test status, weekly summaries, roadmap, CLI guides | 18 |
| `archive/` | Historical docs from phases 0–3A (1st reorganization) | 49 |
| `docs/` root | Active specs: ETI/ETSI, phases A–D, live-stream, UI, testing | 52 |

## Start here

- **Current status:** `releases/QUICK_STATUS.md` (ETI-NI validation, 95% prod-ready) →
  `releases/FINAL_VALIDATION_REPORT.md` (the 1 remaining aggregation bug) →
  `eti-ni/ETI_NI_PARSER_COMPLETE.md`
- **Test status:** `releases/TEST_STATUS_REPORT.md` (93%, MOC-blocked suites listed)
- **Roadmap:** `releases/ROADMAP_SUMMARY.md` (v1.1 ✅ done → v2.0 EWS/EDI planned)
- **Builds:** `releases/PRODUCTION_BUILD_v1.2_FINAL.md`, `releases/CLI_BUILD_SUCCESS_v1.2.md`
- **ETSI compliance:** `ETSI_COMPLIANCE_FRAMEWORK.md`, `ETSI_COMPLIANCE_IMPLEMENTATION_PLAN.md`
- **PDCA workflow:** `PDCA_WORKFLOW.md`, `MULTI_AGENT_PDCA_WORKFLOW.md`

## `pdca/` — PDCA plans & completion reports

- `PDCA_WEEK1_*` (4 files) — Week 1: FIG 1/0, TS 101 756 tables, 72/72 tests
- `PDCA_WEEK4_COMPLETE.md`, `PDCA_WEEK5_PLAN.md`, `PDCA_WEEK5_COMPLETE.md`
- `PDCA_WEEK6_PLAN.md`, `PDCA_WEEK6_AGENTS_COMPLETE.md`, `PDCA_WEEK6_CHECK_COMPLETE.md` — FIG 0/8, 0/9, 0/14 (69/69)
- `PDCA_WEEK7_PLAN.md`, `PDCA_WEEK7_BATCH{1,2}_COMPLETE.md`, `PDCA_WEEK7_BATCH3_{PLAN,COMPLETE}.md`, `PDCA_WEEK7_BATCH4_{PLAN,COMPLETE}.md`, `PDCA_WEEK7_FINAL_100PCT_PLAN.md`, `PDCA_WEEK7_V1.1_COMPLETE.md` — 100% ETSI EN 300 401 (22/22)
- `PDCA_WEEK8_CLI_MODE_PLAN.md`, `PDCA_WEEK8_MOC_FIX_PLAN.md` — CLI mode + MOC fixes
- `PDCA_ETI_NI_PARSER_PLAN.md`, `PDCA_ETI_NI_COMPLETION_PLAN.md` — ETI-NI parser track

## `fig/` — FIG implementation docs

- `FIG007_*` — FIG 0/7 delivery, guide, patches, quickstart
- `FIG009_*` — FIG 0/9 guide, quick-ref, CMake/C++/header patches
- `FIG010_IMPLEMENTATION_SUMMARY.md` (+ MJD fix → see `fixes/FIG010_MJD_FIX_REPORT.md`)
- `FIG011_IMPLEMENTATION_SUMMARY.md`, `FIG012_IMPLEMENTATION_SUMMARY.md`
- `FIG014_*` — FIG 0/14 guide, quickstart, code patches (referenced by `verify_fig014_integration.sh`)
- `FIG016_IMPLEMENTATION_REPORT.md`, `FIG017_*` (delivery, guide, patches, quickstart), `FIG_018_IMPLEMENTATION_REPORT.md`, `FIG019_AGENT2_COMPLETION_REPORT.md`, `FIG024_COMPLETION_REPORT.md`

## `agents/` — Agent reports

Agents 1, 15, 16, 20, 21 (×3), 36 (×2), 37–43, 48. Key: `AGENT48_COMPLETION_SUMMARY.md`
(ETI-NI final validation), `AGENT40_YAML_TESTS_FIXED.md`, `AGENT41_COVERAGE_REPORT.md`,
`AGENT38_MOC_FIX_COMPLETE.md`, `AGENT42_COMPLETION_REPORT.md`, `AGENT43_ETI_FORMAT_DETECTION.md`.

## `fixes/` — Bug reports & fixes

`BUG_REPORT_CLI_PARSER.md`, `CLI_PARSER_FIX_SUMMARY.md`, `CRITICAL_FIX_SERVICES_UNNAMED.md`,
`FIG010_MJD_FIX_REPORT.md`, `FINAL_FIX_SUMMARY.md`, `FINAL_SEGFAULT_FIX.md`,
`SEGFAULT_FIX_COMPLETE.md`, `SEGFAULT_FIX_SUMMARY.md`, `SERVICE_LABELS_FIX_SUMMARY.md`.

## `eti-ni/` — ETI-NI parser (Nov 2025)

`ETI_NI_PARSER_COMPLETE.md` (production-ready) · `ETI_NI_PARSER_STATUS.md` ·
`ETI_NI_PARSER_SUCCESS_SUMMARY.md` · `SUBCHANNEL_COMPARISON.md` (vs etisnoop reference).

## `releases/` — Builds, tests, summaries

- Builds: `CLI_BUILD_SUCCESS_v1.2.md`, `PRE_RELEASE_BUILD_v1.2_SUMMARY.md`, `PRODUCTION_BUILD_v1.2_FINAL.md`
- Tests: `TEST_STATUS_REPORT.md`, `TEST_TARGETS_WEEK4.md`
- Validation: `FINAL_VALIDATION_REPORT.md`, `QUICK_STATUS.md`, `VERBOSE_OUTPUT_COMPLETE.md`, `ETSI_COMPLIANCE_UPDATE_SUMMARY.md`
- Weeks: `WEEK2_TEST_EXPANSION_REPORT.md`, `WEEK4_{COMPLETION_STATUS,EXECUTIVE_SUMMARY,FINAL_SUMMARY}.md`, `WEEK5_PHASE2_COMPLETE.md`
- Planning: `ROADMAP_SUMMARY.md`, `IMPLEMENTATION_DELIVERABLES_OCT28.md`, `CLI_MODE_INSTRUCTIONS.md`
- History of this task: `DOCUMENTATION_REORGANIZATION_REPORT.md` (1st reorg, Oct 2025)

## Active specs (docs root)

ETI: `ETI_PROTOCOL_SPECIFICATIONS.md`, `ETI_LI_FORMAT_NOTES.md`, `ETI_NI_SPECIFICATION.md`,
`ETI_NI_FRAME_DIAGRAM.txt`, `ETI_FORMAT_DETECTION_USAGE.md`, `ETI_TEST_DATA.md` ·
ETSI: `ETSI_COMPLIANCE_FRAMEWORK.md`, `ETSI_COMPLIANCE_IMPLEMENTATION_PLAN.md`,
`ETSI_TS_101_756_IMPLEMENTATION_SUMMARY.md`, `FIG_ANALYSIS_REQUIREMENTS.md` ·
Phases: `PHASE_3A_AUDIO_DATA_SERVICES.md`, `PHASE_3B_{DLS_PLUS_PLAN,TECHNICAL_NOTES,USER_GUIDE}.md`,
`PHASE_{A,B,C,D}_{PDCA_PLAN,COMPLETION_SUMMARY}.md` (B–D) ·
Live stream: `LIVE_STREAM_INPUT.md`, `LIVE_STREAM_QUICK_REFERENCE.md` ·
Testing: `TEST_COMPLETION_PLAN.md`, `GUI_TESTING_COMPLETION_REPORT.md`, `QTEST_GUI_TESTING_REPORT.md` ·
UI/Qt6: `QT6_INTEGRATION_ARCHITECTURE.md`, `Qt6_CPP20_Style_Guide.md`, `UI_RESTRUCTURE_PLAN.md`,
`UI_REFACTORING_PLAN.md`, `UI_DESIGN_CONFLICT_ANALYSIS.md`, `TAB3_*` (4 files),
`SIGNAL_SLOT_CONNECTIONS_PLAN.md`, `GUI_PATTERNS.md`, `SERVICE_TREE_VISUAL_GUIDE.txt` ·
Other: `COMPREHENSIVE_IMPLEMENTATION_PLAN.md`, `DATA_AVAILABILITY_MATRIX.md`, `DEPLOYMENT_GUIDE.md`,
`LICENSE_COMPLIANCE_REPORT.md`, `FUTURE_FEATURES.md`, `SESSION_RESUMPTION_GUIDE.md`,
`SESSION_SUMMARY_2025_10_25.md`, `PDCA_WORKFLOW.md`, `PDCA_WEEK2_PLAN.md`, `PDCA_WEEK4_PLAN.md`,
`MULTI_AGENT_PDCA_WORKFLOW.md`, `VERBOSE_YAML_IMPLEMENTATION_PLAN.md`,
`PHASE_2A_WEEK6_DELIVERABLE_REPORT.txt`, `CLI_MODE_FEATURE_PLAN.md`.
