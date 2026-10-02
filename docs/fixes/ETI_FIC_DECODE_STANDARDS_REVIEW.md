# ETI/FIG Protocol Decode — Standards Review & Fix Record

**Date:** 2026-10-02
**Branch:** `feature/gui-qtads`
**Status:** ✅ Review PASSED (after F1–F9 fixes applied) — **changes still uncommitted** (pending final verification wave)
**Scope:** CLI + GUI decode paths — ETI framing (EN 300 799), FIG parsing (EN 300 401), ECC/country (TS 101 756)
**Reference implementation:** [etisnoop](https://github.com/Opendigitalradio/etisnoop) (behavioral ground truth — GPL, no code copied)
**Test fixture:** `eti/bkk_20062022_141637.eti` (Bangkok DAB+, ensemble 0x2000, ECC 0xF3 — 5001 × 6144 B RAW ETI-NI)

---

## 1. Why this exists

The CLI/`headless` decode did not match etisnoop: `ensemble_label "(no label)"`, wrong service SIds
(`0x020E`-style), inverted country names (`Belgium`/`Albania` for a Thailand ensemble), every
subchannel reported `UEP`. Compared against etisnoop byte-for-byte, three layered bugs were found
(see §2) and fixed; a standards-based code review then produced the finding set in §3 and the
**option-variant matrix** in [`OPTION_VARIANT_MATRIX.md`](OPTION_VARIANT_MATRIX.md), which drives the
upcoming analyzer-settings feature.

## 2. Root cause (original decode bugs)

1. **FC bit layout wrong** — parser read `FICF` from bit 5 of byte 4 and `NST` from nibbles with a
   spurious `+1`. Correct (EN 300 799, etisnoop parity): `FICF = (p[5]&0x80)>>7`, `NST = p[5]&0x7F`
   (**no −1 encoding**), `FP = (p[6]&0xE0)>>5`, `MID = (p[6]&0x18)>>3`, `FL = ((p[6]&0x07)<<8)|p[7]`.
   Frame 0 FC `1b 90 ea e7` → FCT 27, FICF 1, **NST 16**, MID 1, FL 743.
2. **FIC truncated to 32 bytes & read from the STC region** — `EtiFrame::get_fic_field()` was
   hard-coded to `frame+12, 32B` (the middle of the STC table) and the parser buffer was 32 B.
   Correct: FIC at `12 + 4*NST` (=76 here), length `96 B` (MID≠3) / `128 B` (MID=3) as **3/4 FIBs of
   32 B** (`30 B FIG area + 2 B FIB CRC`).
3. **FIB CRC never validated + FIGs parsed over the STC/gap bytes** → ensemble `0x2009` (real
   `0x2000`), garbage services, no labels.

### Corrected layout constants (validated by CRC on the real file)

```
RAW ETI-NI frame (6144 B):
  SYNC p[0..3] = FF F8 C5 49
  FC   p[4..7] (see field rules above)
  STC  p[8 .. 8+4*NST-1]    4 B/stream: SCID=(b0&0xFC)>>2, SAD=((b0&0x03)<<8)|b1, TPL=(b2&0xFC)>>2, STL=((b2&0x03)<<8)|b3
  gap  p[8+4*NST .. 11+4*NST]   MNSC(2)=0xFFFF + EOH CRC(2)
  FIC  p[12+4*NST ..]        96 B (MID≠3) / 128 B (MID=3); FIB = [30 B FIG data][2 B FIB CRC BE]
  MSC  p[12+4*NST+fic_len ..] subchannel i at Σ stl[j]*8 (j<i), size stl[i]*8
  EOF  last 8 B: CRC(2) + RFU(2) + TIST(4)

FIB CRC = complement of CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF, MSB-first) over the first 30 B,
          stored big-endian. → 14 994/14 994 FIBs pass on the fixture (100 %, 0 bad).

FIG header (EN 300 401 8.1.x): type = b7..b5 (3b, 7 = padding/end), length = b4..b0 (5b, data-after-header),
  advance = len+1, max 30 B/FIB, FIGs never span FIBs.
FIG 0/0: [ext][EId 2B] (CId = EId high nibble)...  FIG 0/2: [ext][SId 2/4B][PD form][components 2B each]
FIG 0/9: [ext][...][ECC] (ensemble ECC 0xF3 for Bangkok)
FIG 1/0 & 1/1: [charset/OE/ext byte][EId/SId 2B][16-char label][charset flag/mask 2B]
```

## 3. Standards review — findings & fixes (F1–F9)

Review method: compare each decode site against EN 300 799 / EN 300 401 / TS 101 756 clauses and
etisnoop source (`etianalyse.cpp`, `fig0_1.cpp`, `fig1.cpp`, `fig0_2.cpp`, `charset.cpp`, `tables.cpp`).

| ID | Sev | Finding (file:line @ review time) | Fix (applied) |
|----|-----|----------------------------------|---------------|
| F1 | HIGH | FIG 0/1 EEP/UEP form semantics **inverted**: bit7=1 (EEP long form: option `(b2>>4)&7`, level `(b2>>2)&3`, size `((b2&3)<<8)|b3`) was decoded as "UEP" with a 5-bit level (0x88 → level 34). Every subchannel of the fixture misreported. | `ModernETIFrameParser::processFIG01_SubchannelOrg` + the same branch in `EnhancedFIGAnalyzer` + `FigParser::parseFig01*` fixed to spec; UEP short form (bit7=0): `tsw=(b2>>6)&1`, `idx=b2&0x3F`, size from UEP table (not encoded). Real `0x88 0x18` → **EEP 3-A, size 24 CU**. |
| F2 | HIGH | STC TPL EEP/UEP test inverted: `is_eep=(tpl&0x38)==0x08`; correct = `(tpl&0x20)` (TPL = top 6 bits of STC byte 2; real TPL 0x22 = EEP 3-A). `test16_SubchannelTPL` baked the wrong convention. | `parseETI_NI_Frame` TPL decode fixed (verifies byte 10/11 fixture `0x88 0x18`); `test16` rewritten to real-stream fixture, strengthened (EEP, option 0, level 3, size 24). |
| F3 | MED-HIGH | Service-level ECC propagation frame-order dependent → programme services printed `country_name "Unknown"` while ensemble said "Thailand". FIG 0/9 is processed before same-frame 0/2 services. | After the FIG loop, stamp ensemble ECC onto services with ECC==0; headless merge falls back to ensemble ECC. → **15/15 services = "Thailand"** (ECC 0xF3, CId 2). |
| F4 | MEDIUM | Label **charset ignored** (raw byte passthrough). etisnoop decodes charset 0 (EBU Latin), 3 (UTF-8), 6 (Thai TIS-620). Raw TIS-620 in UTF-8 output = mojibake + violates the project's "Thai UTF-8 first-class" rule. | `processFIG10/11`: charset `(data[0]>>4)&0x0F`; 0 → EBU-Latin→UTF-8 (reused `charset_converter`), 3 → UTF-8, 6 → TIS-620→UTF-8 (0xA1→ก … 0xFB→U+0E5B), unknown → raw + one warning per charset. New `tests/test_charset_converter.cpp` (9 tests). |
| F5 | LOW-MED | MSC tail bound off by 4: EOF = CRC(2)+RFU(2)+**TIST(4)** → `msc_size = 6144 − msc_offset − 8`. | Fixed with comment `EOF(4) + TIST(4)`. |
| F6 | LOW | CRC-16/CCITT-FALSE duplicated 3× (drift risk); `fib_crc_failures_count_` plain counter (race risk). | New shared `src/core/crc16.hpp` (`eti::crc16ccitt_false`), all 3 callers routed through it; counter → `std::atomic<uint32_t>`. |
| F7 | LOW | MID=0 (Mode IV) rejected (`mid<1` checks). | Accept 0–3; FIC-length rule (mid==3?128:96) already handles 0. |
| F8 | INFO | Data-service SId truncated to 16 bits (real `0xF3200001` printed `0x00000001`). | `DabService::sid32`; yaml prints `%08X` when >0xFFFF. Also removed a wrong TMId==3 +2-byte skip in FIG 0/2 components that desynchronized the walk. |
| F9 | INFO | Verbose STC bitrate formula suspected wrong (`stl*8/3`). | Verified: CU-based `size*8` (24 CU → 192 kbps) is DAB-correct; yaml not regressed; comment documents the etisnoop `stl*8/3` unit difference. |

**Verified match (byte-for-byte etisnoop parity):** FC/NST/FP/MID/FL layout · FIC offset + 96/128 rule ·
FIB CRC algorithm (poly/init/reflection/complement/endianness) · FIG header semantics · FIG 0/0, 0/2, 0/9 ·
STC SCID/SAD/STL + STL×8 · MSC offset `12+4*NST+fic_len` · ensemble/labels on the fixture.

## 4. etisnoop parity — real-file cross-check (`-n 2000`)

Labels & SIds (ours == etisnoop, **14/14**):

| SId | Label | | SId | Label |
|---|---|---|---|---|
| 0x2500 | NBTCtest | | 0x2C61 | RROne FM 101 |
| 0x250A | RadioThailand | | 0x2C66 | 103.5 dab+ RADIO |
| 0x250B | COOLfahrenheit | | 0x2C6C | GreenWave 106.5 |
| 0x250D | FM 94(TV5) | | 0x2C6D | MET 107 |
| 0x250F | LTM FM 95 | | 0x2C79 | Thai PBS Digital |
| 0x2510 | HITZ 955 | | 0x2CBC | KU RADIO |
| 0x2519 | JS.100 | | 0x2CBD | Looktung Network |

Ensemble: **0x2000 "Bangkok DAB+"** (charset 0) · Country: **Thailand** (ECC 0xF3, CId 0x02)
· Data service: **0xF3200001** (full 32-bit) · Subchannels: 16/16 **EEP 3-A**, SAD/STL identical to
etisnoop (STL 24 ×14, 12, 3 → 192/96/24 kbps) · FIB CRC: 14 994/14 994 pass (100 %).

## 5. Files changed (fix wave, uncommitted)

`eti_types.hpp/.cpp`, `modern_eti_frame_parser.hpp/.cpp`, `fig_parser.hpp/.cpp`,
`enhanced_fig_analyzer.cpp`, `enhanced_eti_processor_qt.cpp/.h`, `eti_processor.cpp`,
`headless_eti_processor.cpp`, `comprehensive_etsi_validator.cpp`,
`standards_compliance_perfection.cpp`, `etsi_registered_tables.cpp` (ECC 0xE0→0xF3, TS 101 756),
`charset_converter.hpp/.cpp`, `yaml_output_generator.cpp` (country lookup keyed on (ECC, CId)),
new `src/core/crc16.hpp`, new `tests/test_charset_converter.cpp`,
tests `test_eti_ni_parser`, `test_etsi_registered_tables`, `test_yaml_output_generator`, `tests/CMakeLists.txt`.

## 6. Verification gates (post-fix)

- Build: 0 errors, 0 new warnings (offscreen, Qt 6.11).
- ctest: **44/45** — only the known v1.4 `test_fig_week2_comprehensive::test_performance_fig019_parsing_speed`
  HW-threshold failure (untouched FIG 0/19 parser; 7.06 µs vs 5 µs).
- CLI: `streamdab-cli --input eti/bkk_20062022_141637.eti` → ensemble/services/subchannels match §4.
- GUI smoke: `timeout 10 ./build/StreamDABAnalyzer eti/bkk_20062022_141637.eti` → exit 124 (no crash).

## 7. Status & next steps

- [x] Decode fix (root cause) — **committed** (see commit log for T5/T7)
- [x] Standards review (this record) — PASSED after F1–F9
- [x] Commit decode fix + review docs (T7)
- [x] **Analyzer settings feature** — matrix `SETTING` rows implemented (AnalyzerSettings + settings-dialog Analyzer tab + 13 CLI flags; etisnoop-parity defaults byte-identical)
- [x] Matrix FIX leftovers — auto raw-FIC fallback (row 4), tail TIST for ETI-NI (row 10)