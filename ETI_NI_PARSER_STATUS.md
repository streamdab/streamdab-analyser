# ETI-NI Format Parser - Critical Issue Report

**Date:** November 7, 2025
**Status:** ⚠️ **PARSER INCOMPATIBLE WITH BANGKOK ETI FILE**
**Priority:** **CRITICAL** - CLI cannot parse real-world ETI files

---

## 🔴 **PROBLEM SUMMARY**

The StreamDAB CLI parser **CANNOT read** the Bangkok ETI file (`bkk_20062022_141637.eti`) because:

1. **Wrong Format Assumption:** Parser only supports ETI-LI, file uses ETI-NI
2. **All Frames Fail:** 5001/5001 frames rejected with parse errors
3. **Zero Data Extracted:** No subchannels, no services, no FIGs
4. **Etisnoop Works:** Reference implementation successfully extracts 10 subchannels with full metadata

---

## 📊 **COMPARISON: ETISNOOP VS STREAMDAB**

| **Metric** | **Etisnoop (Reference)** | **StreamDAB CLI (Ours)** | **Status** |
|------------|---------------------------|---------------------------|------------|
| File Format | ETI-NI (RAW) ✅ | ETI-LI only ❌ | **INCOMPATIBLE** |
| Frames Read | 5001 ✅ | 5001 ✅ | **OK** |
| Frames Parsed | 5001 ✅ | 0 ❌ | **FAIL** |
| Subchannels | 10 (SCID 0-9) ✅ | 0 ❌ | **MISSING** |
| Services | Multiple ✅ | 0 ❌ | **MISSING** |
| FIG Types | Multiple ✅ | 0 ❌ | **MISSING** |
| Bitrates | 64 kbit/s each ✅ | N/A ❌ | **MISSING** |
| Protection | EEP Level 3-A ✅ | N/A ❌ | **MISSING** |

---

## 🔍 **ROOT CAUSE ANALYSIS**

### **1. File Format Mismatch**

**Bangkok ETI File:**
```
Hex: ff f8 c5 49 1b 90 ea e7 00 00 88 18 04 30 88 18 ...
Format: ETI-NI (Network Independent) with MPEG sync
Detected as: "MPEG ADTS, AAC, v2, 88.2 kHz" by `file` command
```

**Our Parser Expects:**
```
Hex: 49 93 1E 03 (ETI-LI Linear sync)
Format: ETI-LI only
```

### **2. Structure Differences**

**ETI-LI (What we parse):**
```
Offset 0-3:   SYNC (49 93 1E 03)
Offset 4-7:   LIDATA (FC, NST, MID, FP, etc.)
Offset 8-11:  RFU, RFA
Offset 12-43: FIC (32 bytes)
Offset 44+:   MSC data
```

**ETI-NI (What file contains):**
```
Offset 0-3:   Different sync/header (ff f8 c5 49)
Offset ?:     LIDATA at unknown offset
Offset ?:     FIC at unknown offset
Offset ?:     MSC data
```

**Result:** Our LIDATA field extraction reads wrong bytes → parse fails!

---

## 📋 **ETISNOOP OUTPUT ANALYSIS**

From `build/etisnoop-result.txt`, Frame 0 contains:

### **Subchannels (10 total):**

| SCID | Start Addr | Length | Bitrate | Protection | CUs |
|------|------------|--------|---------|------------|-----|
| 0    | 0          | 24     | 64 kbps | EEP 3-A    | 6   |
| 1    | 48         | 24     | 64 kbps | EEP 3-A    | 6   |
| 2    | 96         | 24     | 64 kbps | EEP 3-A    | 6   |
| 3    | 144        | 24     | 64 kbps | EEP 3-A    | 6   |
| 4    | 192        | 24     | 64 kbps | EEP 3-A    | 6   |
| 5    | 240        | 24     | 64 kbps | EEP 3-A    | 6   |
| 6    | 288        | 24     | 64 kbps | EEP 3-A    | 6   |
| 7    | 336        | 24     | 64 kbps | EEP 3-A    | 6   |
| 8    | 384        | 24     | 64 kbps | EEP 3-A    | 6   |
| 9    | 432        | 24     | 64 kbps | EEP 3-A    | 6   |

**Total Capacity:** 10 × 64 kbps = 640 kbps audio capacity

---

## 🎯 **REQUIRED FIXES**

### **Option 1: Full ETI-NI Implementation (RECOMMENDED)**

**Effort:** 2-3 days
**Impact:** Complete ETI-NI support, production-ready

**Tasks:**
1. Research ETSI EN 300 799 Section 4 (ETI-NI specification)
2. Implement ETI-NI frame structure parser:
   - Detect format from first 16 bytes
   - Parse ETI-NI specific header
   - Extract LIDATA from correct offset
   - Extract FIC from correct offset
   - Extract MSC from correct offset
3. Add format auto-detection (ETI-LI vs ETI-NI)
4. Test with both formats
5. Validate against etisnoop output

**Files to Modify:**
- `src/core/eti_types.hpp/cpp` - Add ETI-NI structures
- `src/core/modern_eti_frame_parser.cpp` - Add format detection
- `src/core/eti_frame.cpp` - ETI-NI parsing logic

---

### **Option 2: Convert ETI-NI to ETI-LI (WORKAROUND)**

**Effort:** 4-6 hours
**Impact:** Quick fix, but adds conversion overhead

**Approach:**
1. Pre-process ETI-NI file
2. Extract payload, reformat to ETI-LI
3. Feed to existing parser

**Downsides:**
- Extra processing step
- Memory overhead
- Not elegant

---

### **Option 3: Use Etisnoop as Backend (TEMPORARY)**

**Effort:** 2-3 hours
**Impact:** Works immediately, but external dependency

**Approach:**
1. Shell out to etisnoop
2. Parse YAML output
3. Display in our format

**Downsides:**
- Requires etisnoop installed
- Not standalone
- Performance hit

---

## 📖 **ETSI STANDARDS REFERENCE**

**ETSI EN 300 799 - "Digital Audio Broadcasting (DAB); Distribution interfaces; Ensemble Transport Interface (ETI)"**

- **Section 4.1:** ETI-LI (Linear) format
- **Section 4.2:** ETI-NI (Network Independent) format
- **Section 4.3:** ETI-NA (Network Adapted) format

**Key Difference:**
- ETI-LI: Fixed 6144-byte frames, linear sync
- ETI-NI: Variable frame structure, network-friendly
- ETI-NA: ATM/SDH encapsulation

---

## 🔧 **CURRENT STATUS**

### **What Works:**
✅ CLI executable built and runs
✅ File reading (5001 frames read)
✅ ETI-NI format detection (now accepts `0xFF` sync)
✅ YAML output generation

### **What Fails:**
❌ Frame parsing (all 5001 frames fail)
❌ LIDATA extraction (wrong offsets)
❌ FIC extraction (wrong offsets)
❌ Subchannel discovery (0/10 found)
❌ Service discovery (0 found)
❌ FIG decoding (0 FIGs processed)

---

## ⏭️ **NEXT STEPS**

### **Immediate (Option 3 - Quick Win):**
1. Implement etisnoop wrapper in CLI
2. Parse etisnoop YAML output
3. Display in our format
4. **Estimated Time:** 2-3 hours
5. **Result:** Working parser TODAY

### **Short-term (Option 1 - Proper Fix):**
1. Research ETI-NI specification
2. Implement ETI-NI parser
3. Test with Bangkok file
4. Validate against etisnoop
5. **Estimated Time:** 2-3 days
6. **Result:** Production-ready parser

### **Testing Plan:**
```bash
# Test ETI-NI file
./streamdab-cli --input eti/bkk_20062022_141637.eti --output bkk_output.yaml

# Expected output:
# - subchannels: [10 items]
# - services: [N items]
# - fig_statistics: { FIG types processed }

# Compare with etisnoop:
diff bkk_output.yaml etisnoop-result.txt
```

---

## 💡 **RECOMMENDATIONS**

1. **SHORT-TERM (Today):** Implement Option 3 (etisnoop wrapper) to unblock user
2. **MEDIUM-TERM (Next week):** Implement Option 1 (full ETI-NI parser) for production
3. **LONG-TERM:** Support all 3 ETI formats (LI, NI, NA) per ETSI standard

---

## 📝 **CONCLUSION**

The StreamDAB CLI parser is **NOT production-ready** for real-world ETI files. The Bangkok file uses industry-standard ETI-NI format, which our parser does not support. Etisnoop demonstrates this file is valid and contains rich data (10 subchannels, multiple services).

**CRITICAL ACTION REQUIRED:** Implement ETI-NI parser to make CLI useful for actual DAB stream analysis.

---

**Files Referenced:**
- Bangkok ETI: `/home/seksan/workspace/streamdab-analyzer/eti/bkk_20062022_141637.eti`
- Etisnoop output: `/home/seksan/workspace/streamdab-analyzer/build/etisnoop-result.txt`
- CLI output: `/home/seksan/workspace/streamdab-analyzer/build/cli.txt`
- CLI executable: `/home/seksan/workspace/streamdab-analyzer/build/streamdab-cli`

**Documentation:**
- [CLI_BUILD_SUCCESS_v1.2.md](CLI_BUILD_SUCCESS_v1.2.md)
- [BUG_REPORT_CLI_PARSER.md](BUG_REPORT_CLI_PARSER.md)
- [CLI_PARSER_FIX_SUMMARY.md](CLI_PARSER_FIX_SUMMARY.md)
