# Test suite split: public vs private (World DAB captures) — PLAN

Date: 2026-10-08 · Status: **approved, planned** · Owner: pm-agent

## Why

The ETI captures under `eti/` (`bkk_20062022_141637.eti`, the Bayerischer
Rundfunk / Hessischer Rundfunk warning-day sets, the `EWSv1_1` set) are
**World DAB licensed test material and cannot be published**. The public
GitHub repository therefore must not contain any of them, and the test suite
must be split so:

- **Public suite** — tests that are self-contained (synthetic payloads,
  loopback sockets, in-repo data). Runs in GitHub CI.
- **Private suite** — tests that load the real `eti/` captures. Runs locally
  (and on any machine that has the captures placed in `eti/`).

## Classification (from the registered ctest targets — source-verified)

| Suite | Targets | Criterion |
|---|---|---|
| **PRIVATE (13)** | `test_advanced_fig_analyser`, `test_cli_fig_extended`, `test_cli_fig_trace`, `test_dabplus_audio_decoder`, `test_e2e_epg_journaline`, `test_eti_format_detection`, `test_eti_frame_parser`, `test_eti_li_crc_validation`, `test_eti_ni_parser`, `test_gui_dock_layout` (parity gate: DLS 1114 / MOT 97/97), `test_network_stream_validation`, `test_segfault_fixes`, `test_tpeg_decoder` | Source references a real fixture path (`../../eti/`, `../eti/`, `eti/bkk_20062022_141637.eti`, `hr_20240912…`, `EWS*.eti`) |
| **PUBLIC (45)** | everything else (FIG unit tests, DLS+/MOT/EPG/Journaline decoders, CLI parser/settings, dock interactions, qt audio output sink, network loopback, YAML generator, …) | No real fixture path in any compiled source |

> Note: `test_eti_frame_parser` mixes a `/tmp` synthetic *and* the real capture;
> it is labelled PRIVATE (needs the capture present to pass its real-path cases).

## Mechanisms

1. **CTest LABELS** — `tests/CMakeLists.txt` sets `LABELS "PRIVATE"` on the 13
   targets above and `LABELS "PUBLIC"` on every other registered target
   (applied in one loop at the end of the file).

   ```cmake
   set(DABX_PRIVATE_TESTS
       test_advanced_fig_analyser test_cli_fig_extended test_cli_fig_trace
       test_dabplus_audio_decoder test_e2e_epg_journaline
       test_eti_format_detection test_eti_frame_parser
       test_eti_li_crc_validation test_eti_ni_parser test_gui_dock_layout
       test_network_stream_validation test_segfault_fixes test_tpeg_decoder)
   # for each registered add_test target: PROPERTIES LABELS ...
   ```

2. **Selection commands** (dev runbook)
   ```bash
   ctest                           # default: EVERYTHING (58) — needs the captures
   ctest -L PUBLIC                 # public subset (45) — CI-safe, no captures needed
   ctest -L PRIVATE                # only the capture-dependent tests (13)
   ```
   `ci.yml` / `release.yml` use **`ctest --preset <os> -L PUBLIC`** so GitHub
   never needs the captures.

3. **Publication rules** — `scripts/publish-public.sh` EXCLUDES list gains
   `eti` (all captures, regardless of tracking status); `.gitattributes` gets
   a blanket `eti/ export-ignore`. Git-tracked status only matters for the
   private repo:
   - `eti/bkk_20062022_141637.eti` — **tracked** (30 MB) so the private suite
     is self-contained after a clone of the private repo.
   - `eti/*.txt` description files — **tracked** (small, useful).
   - `eti/HessischerRundfunk-…/` (293 MB), `eti/BayerischerRundfunk-…/`
     (118 MB), `eti/EWSv1_1/` (967 MB) — **gitignored** (obtain locally).

## Capture inventory (private)

| Capture | Content (from the `.txt` companions) | Used by |
|---|---|---|
| `eti/bkk_20062022_141637.eti` + `.txt` | Bangkok ensemble: **EPG and TPEG** carried, DLS+, MOT slideshow, Thai labels | parity gate, EPG/TPEG validation phase 1 |
| `eti/HessischerRundfunk-Warntag2024-…_journalline/hr_20240912T105801_EWS_Start.eti` / `…T114211_Entwarnung.eti` + `HR_20240912.txt` | Hessischer Rundfunk warning-day: **EWS start + all-clear, Journaline** | EWS (v2.0) + Journaline validation |
| `eti/BayerischerRundfunk-Warntag2024-…/` | BR warning-day | EWS validation |
| `eti/EWSv1_1/` (`EWS1..9.eti`) | EWS v1.1 suite | EWS validation |

None of these ever reach the public repository.

## Phase-1 validation work (EPG / Journaline / TPEG — before the GUI tabs)

The EPG and Journaline decoders exist and pass synthetic unit tests, but had
never been fed real ensemble data when this plan was written —
`test_e2e_epg_journaline` was an empty shell (it now runs the real captures
and **logs and documents** those exact figures: bkk EPG honest rejection = 0
events, TEPG inventory = 36 unique messages / 4998 frames, HR Journaline
clean-item count = 3 014 of 11 544 candidates, and DataServiceStore
aggregation; what it *asserts* is bounds — 0 events, ≥ 20 items, candidates >
items, a 5 % pass-rate floor and the message/frame floors, see
`tests/e2e/test_e2e_epg_journaline.cpp`). Phase 1 (private, uses the
captures above):

1. Extend the MOT/FIC inventory to report content subtypes from real captures
   (EPG via FIG 5.1/5.2 + MOT; TPEG via subchannel data; Journaline via MOT
   objects) — **no fake data**.
2. Feed `bkk` EPG/TPEG objects and the HR Journaline objects through the
   EPG/Journaline decoders; fix any spec gaps (TS 102 371, TS 102 979) and add
   capture-backed unit/E2E tests.
3. Only then build the GUI tabs (MOT dock → `Slideshow | EPG | Journaline`)
   and update README/ATTRIBUTIONS to the validated reality.
4. New EWS tests (FIG 9.x EWS, TS 104 089/090) land labelled **PRIVATE**.

## Doc updates (in this change)

- `README.md` Testing section: fixtures are private World DAB material; the
  table becomes "private captures (obtained separately)"; CI runs the PUBLIC
  subset.
- `INSTALL.md` test paragraph: full 58/58 requires the private captures;
  public builds run `ctest -L PUBLIC`.
- `ATTRIBUTIONS.md` test-data section: captures not redistributed (World DAB
  licence) and required for the private suite.
- `docs/ETI_TEST_DATA.md`: tracking/ignore table updated (bkk tracked, big
  sets ignored).
- `docs/GITHUB_PUBLISHING_PLAN.md`: `eti/` (re-)added to the private list;
  the earlier "bkk is public" note is superseded.
- `docs/TASK_LIST.md`: this plan + its implementation task.

## Verification for this change

- Build 0/0 (recreate `build/` — it was cleaned by the environment).
- Full `ctest` (captures present) 58/58 — unchanged public claims replaced by
  "full = 58 with private captures".
- `ctest -L PUBLIC` runs and matches the public classification count.
- Publish `--dry-run`: `eti/` absent from the filtered tree, file count
  drops by the tracked `eti/*` entries.