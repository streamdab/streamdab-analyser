# Option-Variant Matrix — Decode Decision Points (Standards vs etisnoop default vs ours)

**Date:** 2026-10-02
**Purpose:** For every decode decision point where the ETSI standards (EN 300 799, EN 300 401, TS 101 756)
permit multiple valid options (or real-world streams vary), record the allowed options, what the real
reference implementation (etisnoop) uses **by default**, what we do today, and how to expose the
non-default variants as user-selectable **settings** (GUI settings dialog + QSettings + CLI flags),
grouped per case.

**Rule (user decision):** etisnoop's default = our default. The remaining variants become settings.

Legend — Recommendation: **KEEP** = etisnoop parity stays the default · **SETTING** = expose as
user option · **FIX** = implement correctly first (was deviating / absent).

| # | Decision point (code ref) | Standard clause & allowed options | etisnoop default (real impl) | Our code (after decode fix) | Recommendation |
|---|---------------------------|-----------------------------------|------------------------------|------------------------------|----------------|
| 1 | **Label charset** (FIG 1/0, 1/1, DLS) — `modern_eti_frame_parser.cpp` `processFIG10/11` | EN 300 401 8.1.13/8.1.14 + TS 101 756 Annex C: 0 = EBU Latin, 3 = UTF-8, 4 = UCS-2/UTF-16BE, 6 = Thai TIS-620, 15 = complete EBU Latin; others undefined | Charset 0 → EBU-Latin→UTF-8 table; 3 → UTF-8 passthrough; 6 → Thai TIS-620; others ignored/warned | 0/3/6 decoded (F4 fixed); unknown → raw + warn-once | **KEEP** default; **SETTING**: force-charset override + fallback policy (raw / force-UTF-8 / skip) for unknown charsets |
| 2 | **ETI format** (LI vs NI) — detection + decode path | EN 300 799 §5: RAW/NI, STREAMED, FRAMED/LI transports; same frame decoding | Auto-identify by sync/framing, then decode | Auto by sync byte; LI/NI share corrected decode (FIC at 12+4·NST) | **KEEP** auto; **SETTING**: forced mode (auto/li/ni) + legacy-LI assumption toggle for odd captures |
| 3 | **NST interpretation** — `parseETI_NI_Frame` | EN 300 799 §5.2: NST = stream count (0–64), **no −1**; some legacy muxes wrote count−1 | `nst = p[5] & 0x7F` — no −1 | Same (fixed); validated ≤64 | **KEEP**; **SETTING**: `--nst-offset-1` legacy-correction toggle |
| 4 | **FIC format** (ETI 96/128 B vs full on-air 768 B) | EN 300 799 §5.2 (ETI FIC = FIBs with per-FIB CRC) vs EN 300 401 §6 (on-air FIC, FEC-protected, no FIB CRC) | `ficl*4` from MID; FIB CRC gate with `ignore_error` escape | 96/128 by MID; CRC gate **without fallback** for CRC-less FIC dumps | **SETTING**: `--fic-ignore-crc` (etisnoop `ignore_error` equivalent) **+ FIX**: auto raw-mode fallback when all FIBs fail CRC |
| 5 | **FIB CRC strictness** — `fig_parser.cpp` `extractFigBlocks` | EN 300 799 §5.2 / EN 300 401 §5.2: CRC mandatory; damaged FIBs dropped | Skip bad FIB unless `--ignore-errors` | Skip bad FIB; `fib_crc_failures` counter surfaced | **KEEP** strict; **SETTING**: ignore-FIB-CRC mode (CLI + GUI + QSettings); keep counter as diagnostics |
| 6 | **ECC / country resolution** — `headless_eti_processor.cpp`, `yaml_output_generator.cpp` | TS 101 756 Table 1: (ECC, CId) → country; ECC from FIG 0/9 (ensemble + optional per-service subfield) | Reads ECC hex (no name table); ensemble + per-service | TS 101 756 table keyed (ECC, CId) ✓ (0xF3/0x02 → Thailand); service ECC propagates from ensemble (F3 fixed) | **KEEP** auto; **SETTING**: manual ECC/CId override + optional deep parse of FIG 0/9 extended subfield (per-service ECC precision) |
| 7 | **FIB padding tolerance** (0xFF vs 0x00) | EN 300 401 §5.2: padding = FIG type 7 (0xE0–0xFF) | Type-7 break + hard 30-byte area bound | Type-7 break + len-0 break (covers 0x00 padding) — tolerant of both | **KEEP** tolerant (both); no setting needed |
| 8 | **Label source priority + short label** — `processFIG10/11`, service merge | EN 300 401: FIG 1/1 label-only services, FIG 1/0 ensemble label, FIG 0/16 service-org label; short label from 16-char label + 16-bit mask (§8.1.14) | Label DB keyed by SId; short label computed from mask | FIG 1/1 discovery ✓; ensemble label ✓; **short-label mask ignored** (only full 16-char) | **KEEP** full-label default; **SETTING**: short-label display mode (derive from mask) + label-source priority (FIG 0/16 vs 1/1) for conflicting names |
| 9 | **EEP/UEP protection decode** — STC TPL + FIG 0/1 | EN 300 401 §6.2.1/8.1.2 + EN 300 799 §5.3.2: EEP option/level ↔ size tables (A: 16/8/6/4 CU; B: 27/21/18/15); UEP table switch+index | Full table lookup (`num_cu`); UEP table switch+index | F1/F2 fixed (EEP long form, level, size; UEP short flagged) — **UEP size table not loaded** | **KEEP** decode; **SETTING** (later): full EEP/UEP size tables for reporting + table-set selection |
| 10 | **Timestamp source + alarm flag** — `EtiFrame` TIST, FIG 0/0 alarm | EN 300 799 §5.3.6: TIST at frame tail (NI); legacy LI alternative; alarm = FIG 0/0 bit | TIST read at tail, printed as ms; alarm latched | TIST still read at bytes 8–11 (legacy LI assumption); alarm decoded + latched ✓ | **FIX** tail TIST for NI parity; **SETTING**: timestamp source (file mtime vs TIST); **KEEP** alarm latch |
| 11 | **EOH/EOF frame CRC validation** | EN 300 799 §5.3.4/5.3.7: header CRC (EOH gap) + frame CRC (FIC+MSC), CCITT-FALSE complemented | Validates both per frame, reports mismatch | Not implemented (validation passes unconditionally; decode independent) | **SETTING** (default ON per etisnoop): EOH/EOF CRC validation with strict/lenient toggle |
| 12 | **Data-service SId width/display** — FIG 0/2 PD form | EN 300 401 8.1.5.2: data SId = 32 bits (ECC 8 + CId 4 + SRef 20) | Prints full 32-bit SId | Full `sid32` stored + `%08X` in yaml (F8 fixed) | **KEEP**; **SETTING**: display mode (hex32 / ECC+CId+SRef decomposition / hex16) |
| 13 | **Mode Identity 0 = Mode IV** — MID bounds | EN 300 799 §5.2: MID 0–3 (0 = Mode IV); FIC length = 3 FIBs for MID≠3 | MID 0 accepted, displayed "4" | Accepted (F7 fixed) | **KEEP**; no setting |
| 14 | **FIG 0/9 per-service extended ECC subfield** | EN 300 401 8.1.3.2: optional extended field with per-service ECC/LTO entries | Parses full subfield | Ensemble ECC only | **KEEP** minimal default; **SETTING**: deep per-service ECC parse (per-service country precision) |

## Implementation roadmap (settings feature)

1. **Storage:** `AnalyzerSettings` (QSettings-backed, org `StreamDAB-Analyzer`, app `DABAnalyzer`,
   group `analyzer/...`) shared by GUI + CLI; CLI flags override for headless runs.
2. **GUI:** extend `src/gui/settings_dialog.cpp` with an "Analyzer / Decode options" section grouped
   by case (charset, ETI mode, FIC/FIB CRC strictness, country override, label display, timestamp,
   SId display) + optional quick toggles in the Decoder panel.
3. **CLI:** add flags with documented defaults matching this matrix's `KEEP` column:
   `--force-charset N`, `--charset-fallback raw|utf8|skip`, `--eti-mode auto|li|ni`,
   `--nst-offset-1`, `--fic-ignore-crc`, `--fib-ignore-crc`, `--strict-crc`,
   `--ecc-override 0xNN:cid`, `--label-source 0/16|1/1`, `--show-short-labels`,
   `--timestamp-source mtime|tist`, `--sid-display hex32|ecc-sid|hex16`,
   `--service-ecc-deep-parse`.
4. **Defaults:** every default equals the etisnoop-parity behavior (the `KEEP` column) — so a
   default-config run stays byte-identical to today's output.

## Status

- [x] Review + fix wave F1–F9 (see `ETI_FIC_DECODE_STANDARDS_REVIEW.md`)
- [ ] Matrix `FIX` leftovers: auto raw-FIC fallback (row 4), tail TIST for NI (row 10)
- [ ] Settings feature: storage + GUI + CLI per roadmap (§1–4)
- [ ] Commit decode fix, then settings feature