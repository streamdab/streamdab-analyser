# CLI FIG Decode Coverage — Evaluation vs EN 300 401 & DAB-XPlorer FIC tools

**Date:** 2026-10-02
**Reference (design):** `ref/man-dabxp_start-en.pdf` — FIC-XPlorer (FIG database + MCI/SI tree/lists) and
FIC-XTractor (bit-level FIC parse; chronological or by-type views; FIG element tree; readable values:
text / date (FIG 0/10) / frequency (FIG 0/21) / geographic coordinates (FIG 0/22); FIB-hex with marked
FIG; flags for erroneous FIGs, FIB CRC errors, multiplex reconfigurations).
**Scope question (user):** should the CLI show more FIC-Analyser/FIC-Extractor-style detail? What is still not decoded?
**Standard:** ETSI EN 300 401 (full FIG inventory below).

## 1. FIG type/ext coverage inventory (decode capability)

Legend: ✅ decoded · 🔶 partial · ❌ not decoded · — n/a

| FIG | Content (EN 300 401) | fig_parser.cpp (CLI/core) | modern parser (CLI headless summary) | AdvancedFIGAnalyser (GUI) | etisnoop |
|---|---|---|---|---|---|
| 0/0 | Ensemble information | ✅ | ✅ (EId/CId/CIF/alarm) | ✅ (fixed) | ✅ |
| 0/1 | Sub-channel organisation | ✅ | ✅ (EEP/UEP) | ✅ (fixed) | ✅ |
| 0/2 | Service components | ✅ | ✅ (PD/SId/comp) | ✅ (fixed) | ✅ |
| 0/3 | Packet-mode (5-B entries) | ✅ | ✅ | 🔶 | ✅ |
| 0/4 | CA (conditional access) | ✅ | ❌ | 🔶 | ✅ |
| 0/5 | Language | ✅ | ❌ | 🔶 | ✅ |
| 0/6 | Clock date/time | ✅ | ❌ | 🔶 | ✅ |
| 0/7 | Signal quality | ✅ | ❌ | 🔶 | ✅ |
| 0/8 | Service linking | ✅ | ❌ | 🔶 | ✅ |
| 0/9 | Service-related app mode/ECC | ✅ | ✅ (ECC) | ✅ | ✅ |
| 0/10 | Date & time (MJD) | ✅ | ❌ | ✅ (fixed) | ✅ |
| 0/11 | Broadcast area ID | ✅ | ❌ | 🔶 | ✅ |
| 0/12 | Service component record | ✅ | ❌ | ❌ | — |
| 0/13 | User application | ✅ | ❌ | 🔶 | ✅ |
| 0/14 | Announcement support (CA) | ✅ | ❌ | 🔶 | ✅ |
| 0/15 | Announcement switching | ✅ | ❌ | ❌ | — |
| 0/16 | Service organisation (label) | ✅ | ❌ (services from 1/1) | 🔶 (Programme Number only) | ✅ |
| 0/17 | Service component global defn | ✅ | ❌ | 🔶 | ✅ |
| 0/18 | Service component local defn | ✅ | ❌ | 🔶 | ✅ |
| 0/19 | Frequency information (CA) | ✅ | ❌ | ❌ | ✅ |
| 0/20 | Unknown/other ensembles | ✅ | ❌ | ❌ | — |
| 0/21 | Frequency information (FIB) | ✅ | ❌ | ❌ | ✅ |
| 0/22 | Geographical location | ❌ | ❌ | ❌ | ✅ |
| 0/23 | ? | ❌ | ❌ | ❌ | — |
| 0/24 | Announcements list | ✅ | ❌ | ❌ | ✅ |
| 0/25–0/30 | (ens. change, service related...) | ❌ | ❌ | ❌ | 0/25-0/28 ✅ |
| 0/31 | ? | ❌ | ❌ | ❌ | ✅ |
| 1/0 | Ensemble label | ✅ | ✅ | ✅ (fixed) | ✅ |
| 1/1 | Programme service label | ✅ | ✅ | ✅ (fixed) | ✅ |
| 1/2 | Service component label | ❌ | ❌ | ❌ | ✅ |
| 1/3 | Data service label | ❌ | ❌ | ❌ | ✅ |
| 1/4 | Service component label (short/of) | ✅ | ❌ | ✅ (fixed) | ✅ |
| 1/5 | Data service label | ✅ | ❌ | ✅ (fixed) | ✅ |
| 2/x | OTH / FIG type 2 (FIC data) | ❌ | ❌ | 🔶 | ✅ (fig2) |

Sources: `grep FIG src/core/fig_parser.cpp` (0/0-0/21, 0/24, 1/0, 1/1, 1/4, 1/5), modern_eti_frame_parser (0/0,0/1,0/2,0/3,0/9,1/0,1/1),
advanced_fig_analyser (0/0,0/1,0/2,0/6,0/7,0/8,0/9,0/10,0/13,1/0,1/1,1/4,1/5,2/0-2/5), etisnoop src/ (0/0-0/3,0/5-0/11,0/13,0/14,0/16-0/19,0/21,0/22,0/24-0/28,0/31,1/x,2).

## 2. What the CLI currently exposes (summary path)

`streamdab-cli --input f.eti --output out.yaml`: ensemble (id/label/ECC/country/alarm), services (SId/label/type/country/components),
subchannels (id/SAD/size/EEP-UEP/bitrate), per-frame short info, frame CRC counters, tist_ms (verbose), FIG counts (headless stats).
Parity: 14/14 labels, ensemble 0x2000 "Bangkok DAB+", 16/16 EEP 3-A vs etisnoop — verified.

## 3. Gaps & recommendations (FIC-Analyser/FIC-Extractor-style detail)

Priority ordered. **P1 = high value / cheap, P2 = medium, P3 = future.**

- **P1 — Per-frame FIG instance trace (verbose mode)**: chronological list `frame N: FIG t/e len=L fib=fib_idx [crc=ok|bad] [reconfig]` —
  the backbone of FIC-XTractor's chronological view and of Tab-3 in the GUI. Data already exists in the parsing walk
  (fig blocks per FIB); surface it in `--verbose-frames` and a `--fig-trace` flag.
- **P1 — FIG inventory in YAML summary**: `figs:` section listing observed (type, ext, count, first/last frame, crc_failures)
  — enables the FIC-Analyser "FIG Content" view from the CLI; cheap aggregation of existing data.
- **P1 — FIB/FIC error flags**: per-FIG CRC-fail and FIB CRC-fail counts + MCI reconfiguration detection
  (ensemble id / service / subchannel changes across frames) in summary — mirrors the manual's error icons.
- **P2 — Readable value decoding for key FIGs in the trace**: FIG 0/10 (MJD → UTC date/time — fig_parser already decodes; add to trace),
  FIG 0/21 (frequency table), FIG 0/22 (geo coordinates), FIG 0/7 (signal quality), FIG 0/16 (service labels via 0/16 — cross-check vs 1/1
  per label-source setting). Most decoders exist in fig_parser; the trace/grouped views just need to call them.
- **P2 — Grouped-by-FIG view**: `--figs-by-type` that prints one section per FIG type/ext with instances — FIC-XTractor's second view.
- **P3 — Remaining decoders**: FIG 1/2+1/3 (component/data-service labels — note 1/4+1/5 cover the same payloads on this mux),
  0/22-0/31 family, FIG type 2 (OTH) — implement when a capture with them is available (this Bangkok capture carries 0/0,0/1,0/2,0/9,1/0,1/1 only).
- **P3 — Full on-air 768-B FIC mode** is already a matrix row-4 setting; FIB-hex dump with marked FIG bytes (`--fib-hex`) — same idea as
  the manual's FIB-hex panel.

## 4. Files to touch (CLI detail wave)

`src/core/fig_parser.cpp` (trace aggregation hooks), `src/core/modern_eti_frame_parser.cpp` (expose per-frame FIG list in parse result),
`src/core/headless_eti_processor.cpp` (FIG inventory + error flags + reconfig detection), `src/cli/yaml_output_generator.cpp`
(`figs:` section, flags), `src/cli/cli_argument_parser.*` (--fig-trace, --figs-by-type), tests. GUI tab2/tab3 wiring is a
separate wave (see `PANEL_AUDIT.md`).

## 5. Decision framing

The Bangkok fixture only carries FIG 0/0, 0/1, 0/2, 0/9, 1/0, 1/1 — so any P2/P3 decoder must be validated synthetically
or postponed. P1 items are fully verifiable on the real capture and provide the FIC-Analyser/FIC-Extractor value the
manual describes. Recommendation: implement P1 now (CLI), P2 when the GUI tab2/tab3 wave lands (shared trace), P3 on demand.

**UPDATE 2026-10-02 (P1 implemented, commit `94126b4`):** `--fig-trace` (per-frame FIG instances with readable
values: ensemble/labels/ECC/subch/components; reconfig flags), `figs:` inventory section (type/ext/count/frames/
crc_failures), `fic_health:` block (fib_crc_failures/total_fibs/erroneous_figs/reconfiguration_count),
`--max-frames` now enforced. Default output byte-identical + new sections only. GUI Tab-3 tracer + details sharing
the same decoded data landed in `bd47999` (see `docs/gui/PANEL_AUDIT.md`). 

**UPDATE 2026-10-02 (P2/P3 implemented, commit `16f948f`):** FIG 1/2 (component label) + 1/3 (data-service label)
charset-incl. TIS-620; FIG 0/21 frequency (DAB/FM/AM etisnoop parity); FIG 0/22 geographical location (TII walk,
2's complement); FIG type 2 (OTH) header/identifier/label; `--figs-by-type` grouped view; `--fib-hex` with FIG
byte-range markers; EEP/UEP full size tables (`protection_table` additive yaml field); CLI FIG 1/1 P/D aligned to
GUI (32-bit data SIds). ctest 49/49 = 100% (after v1.4 budget retarget `a5a1a56`). Remaining backlog: `--advanced-fig`
(CLI benchmark mode from PHASE_E_PLAN) — deferred.