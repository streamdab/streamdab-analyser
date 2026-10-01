# PHASE E — Single-Path Plan (Main Plan)

**Status:** ACTIVE — the single operational plan. All prior phase/week plans are history in `docs/archive/`.
**Parent strategy:** v2.0 direction (EWS emergency warnings + EDI distribution), folded here from archived `docs/archive/ROADMAP_SUMMARY.md`.
**Process:** `AGENTS.md` mandatory workflow (code → test → build → E2E → review → commit), run via `pm-agent` + `cpp-qt-developer` workers.
**Last updated:** 2026-09-27

## Path overview

```
E-0 VERIFY BASELINE (runtime proof that the shipped code works)
 └─ E-0.1  Toolchain + clean rebuild + ctest baseline + CLI smoke test
 └─ E-0.2  Confirm ETI-NI YAML output at runtime (bb0389f numbers on bkk_20062022_141637.eti: 5 services / 4 ensembles / 64 subchannels; schema key is `subchannel_id:`, not `scid:`)
 └─ E-0.3  Confirm MOC resolution at build (workaround landed a56c8aa/538b9a0 — confirm all 43 suites build)
 └─ E-0.4  Push to 100% pass + >85% coverage, close out, declare production-ready
E-1 EWS EMERGENCY WARNINGS (v2.0a, ETSI TS 104 089/090)
 └─ E-1.1  ASA protocol parser + alert model
 └─ E-1.2  Alert management + Thai province support
 └─ E-1.3  GUI integration + tests
E-2 EDI DISTRIBUTION (v2.0b, ETSI TS 102 693)
 └─ E-2.1  EDI packet parser
 └─ E-2.2  PFT reassembly + FEC
 └─ E-2.3  Network integration + tests
BACKLOG (deferred, from archived Week-8 plan): CLI `--advanced-fig`, CLI live-stream support, CLI benchmark mode
```

Rules: one phase active at a time; each subphase ends in a commit + a dated status line added below; no new feature work until E-0.4 is green.

## Progress log

- 2026-09-27: Plan created. E-0 pending — hard blocker: `cmake` + Qt6 dev headers not installed in this environment.
- 2026-09-27: Toolchain installed by user. Clean rebuild: all targets green EXCEPT `test_pft_reassembler` (missing `libgtest-dev`, environmental — needs `sudo apt-get install -y libgtest-dev`).
- 2026-09-27: E-0.2 runtime CONFIRMED — Bangkok ETI: 5 services / 4 ensembles / 64 subchannels, 5001/5001 frames, ~16k FPS; Bavarian/EWS fixtures all exit 0.
- 2026-09-27: ctest baseline (serial, authoritative): **41/43 (95%)** — all 9 assertion failures fixed (fig0 27/27, fig1 42/42, eti-ni 28/28, yaml 22/22, cli-integration 12/12, cli-functional 27/27); remaining: pft unbuilt (needs libgtest-dev via sudo), 2 perf-threshold disputes in week2. Details: `docs/releases/TEST_STATUS_REPORT.md`.
- 2026-10-01: `libgtest-dev` installed → full rebuild: **43/43 targets build**. ctest: **42/43 pass (97.7%)** — sole failure is the week2 perf-threshold dispute (`test_performance_fig019_parsing_speed` ~7.8µs vs 5µs budget, parser correct). Coverage: needs `lcov` (sudo). E-0.4 gate criteria: 100% pass not met due to environmental perf dispute; coverage unmeasured.

## doc status (single source of truth)

- Live: this file, `AGENTS.md`, `docs/README.md` (index), `docs/releases/{QUICK_STATUS,TEST_STATUS_REPORT,FINAL_VALIDATION_REPORT}.md` (all bannered Sep 2026), `docs/releases/VERBOSE_OUTPUT_COMPLETE.md`, `docs/releases/{CLI_BUILD_SUCCESS_v1.2}.md` (bannered).
- History: everything under `docs/archive/` (111 files). Do not treat archived plans as actionable — contradictions found during the Sep 2026 audit (same-day 87% vs 100% test claims; pre-release outscoring "final" build) are why they were retired.
- `README.md`/`CLAUDE.md` (root): human overview / historical record. If they contradict this file, this file wins.
