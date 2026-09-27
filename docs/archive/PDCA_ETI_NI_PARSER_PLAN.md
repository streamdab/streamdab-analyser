# PDCA Plan: ETI-NI Format Parser Implementation

**Date:** November 7, 2025
**Priority:** CRITICAL
**Estimated Time:** 2-3 days (accelerated with multi-agent approach)
**Target:** Full ETI-NI compliance per ETSI EN 300 799

---

## 🎯 **OBJECTIVE**

Implement complete ETI-NI (Network Independent) format parser to enable StreamDAB CLI to read real-world DAB ETI files like the Bangkok test file.

**Success Criteria:**
- ✅ Parse all 5001 frames successfully (0 errors)
- ✅ Extract all 10 subchannels with correct metadata
- ✅ Discover all services from FIG data
- ✅ Match etisnoop output structure
- ✅ Maintain ETI-LI compatibility (no regressions)

---

## 📋 **PLAN PHASE**

### **Multi-Agent Task Breakdown**

#### **Agent 42: ETI-NI Format Research & Specification**
**Deliverable:** ETI-NI structure documentation
**Time:** 2-3 hours

**Tasks:**
1. Research ETSI EN 300 799 Section 4.2 (ETI-NI specification)
2. Document byte-level frame structure
3. Identify field offsets vs ETI-LI
4. Create test data extraction plan
5. Document sync pattern variations

**Output:**
- `docs/ETI_NI_SPECIFICATION.md` - Complete structure reference
- Byte offset mapping table (ETI-LI vs ETI-NI)
- Field extraction formulas

---

#### **Agent 43: ETI Format Auto-Detection**
**Deliverable:** Format detection logic
**Time:** 2-3 hours

**Tasks:**
1. Implement `detectETIFormat()` function in `eti_types.cpp`
2. Check sync patterns:
   - ETI-LI: `0x49, 0x93, 0x1E, 0x03`
   - ETI-NI: `0xFF 0xF8` or `0xFF 0x07` (MPEG sync variants)
3. Add format enum: `ETIFormat { LI, NI, NA, UNKNOWN }`
4. Return detected format with confidence level
5. Add unit tests for format detection

**Output:**
- Modified `src/core/eti_types.hpp` - Add ETIFormat enum
- Modified `src/core/eti_types.cpp` - Add detectETIFormat()
- New test: `tests/test_eti_format_detection.cpp` (15 tests)

---

#### **Agent 44: ETI-NI Frame Parser Implementation**
**Deliverable:** ETI-NI parsing logic
**Time:** 6-8 hours

**Tasks:**
1. Create `parseETI_NI_Frame()` in `modern_eti_frame_parser.cpp`
2. Implement field extraction:
   - SYNC detection (handle `0xFF` variants)
   - LIDATA extraction (ETI-NI specific offsets)
   - FIC extraction (32 bytes at ETI-NI offset)
   - MSC extraction (payload data)
3. Parse subchannel information from STC (Stream Characterization)
4. Extract NST (Number of Streams) correctly
5. Handle ETI-NI specific padding

**Output:**
- Modified `src/core/modern_eti_frame_parser.cpp`
- Modified `src/core/modern_eti_frame_parser.hpp`
- Support for both ETI-LI and ETI-NI in same codebase

---

#### **Agent 45: ETI-NI Integration & Testing**
**Deliverable:** End-to-end validation
**Time:** 4-6 hours

**Tasks:**
1. Integrate format detection into `headless_eti_processor.cpp`
2. Route frames to correct parser (LI vs NI)
3. Test with Bangkok ETI file:
   - Verify 10 subchannels extracted
   - Verify bitrates (64 kbps each)
   - Verify protection levels (EEP 3-A)
4. Create comprehensive test suite
5. Compare output with etisnoop

**Output:**
- Modified `src/core/headless_eti_processor.cpp`
- New test: `tests/test_eti_ni_bangkok.cpp` (25 tests)
- Validation report comparing with etisnoop

---

## 🔄 **DO PHASE**

### **Execution Strategy**

**Day 1 (Agents 42 + 43 in parallel):**
- Morning: Agent 42 researches ETI-NI spec
- Morning: Agent 43 implements format detection
- Afternoon: Integration and cross-validation

**Day 2 (Agent 44):**
- Full day: ETI-NI parser implementation
- Evening: Internal testing with sample frames

**Day 3 (Agent 45):**
- Morning: Integration with headless processor
- Afternoon: Bangkok file testing
- Evening: Validation against etisnoop

---

## ✅ **CHECK PHASE**

### **Validation Tests**

1. **Format Detection Test:**
   ```bash
   # Test ETI-LI detection (existing files)
   ./test_eti_format_detection --format=LI

   # Test ETI-NI detection (Bangkok file)
   ./test_eti_format_detection --format=NI --file=bkk_20062022_141637.eti
   ```

2. **Frame Parsing Test:**
   ```bash
   # Parse Bangkok file
   ./streamdab-cli --input eti/bkk_20062022_141637.eti \
                   --output bkk_output.yaml \
                   --verbose
   ```

3. **Expected Results:**
   ```yaml
   subchannels:
     - scid: 0
       start_address: 0
       bitrate: 64
       protection: "EEP-3A"
     - scid: 1
       start_address: 48
       bitrate: 64
     # ... (10 total)

   services:
     - # Services from FIG 0/2

   fig_statistics:
     total_figs_processed: >0  # NOT zero!
   ```

4. **Comparison with Etisnoop:**
   ```bash
   # Compare subchannel counts
   grep "SCID" bkk_output.yaml | wc -l  # Should be 10
   grep "SCID" etisnoop-result.txt | wc -l  # Should be 10
   ```

---

## 🎬 **ACT PHASE**

### **Deliverables Checklist**

- [ ] Agent 42: ETI-NI specification document
- [ ] Agent 43: Format detection code + tests
- [ ] Agent 44: ETI-NI parser implementation
- [ ] Agent 45: Integration + validation tests
- [ ] All tests passing (100%)
- [ ] Bangkok file parsed successfully
- [ ] Output matches etisnoop structure
- [ ] Documentation updated
- [ ] Git commit with fixes

### **Success Metrics**

| **Metric** | **Target** | **Validation** |
|------------|------------|----------------|
| Frames parsed | 5001/5001 (100%) | No parse errors |
| Subchannels found | 10 | Match etisnoop |
| Services found | >0 | From FIG 0/2 |
| FIG types processed | >0 | From FIC data |
| Test pass rate | 100% | All tests green |
| Bitrate accuracy | 64 kbps | Per subchannel |
| Protection level | EEP-3A | Per subchannel |

---

## 📚 **TECHNICAL REFERENCE**

### **ETSI EN 300 799 Structure**

**ETI-LI Frame (6144 bytes):**
```
Offset 0-3:   SYNC (0x49 0x93 0x1E 0x03)
Offset 4:     FC (Frame Characterization)
Offset 5:     NST (Number of Streams) | RFU
Offset 6:     MID (Mode Identity)
Offset 7:     FP (Frame Phase)
Offset 8-11:  RFA, RFU
Offset 12-43: FIC (Fast Information Channel - 32 bytes)
Offset 44+:   MSC (Main Service Channel)
```

**ETI-NI Frame (6144 bytes):**
```
Offset 0-1:   SYNC (0xFF 0xF8 or 0xFF 0x07) - MPEG-like
Offset 2-3:   ETI-NI header continuation
Offset ?:     FC, NST, MID, FP (DIFFERENT OFFSETS!)
Offset ?:     FIC (32 bytes, DIFFERENT OFFSET!)
Offset ?:     MSC data
```

**Key Differences:**
- Sync pattern completely different
- LIDATA fields at different byte positions
- May have different padding/alignment
- MPEG-compatible framing

---

## 🚀 **EXECUTION PLAN**

### **Step 1: Launch Agents (Parallel)**
```bash
# Agent 42: Research
Task: Research ETI-NI ETSI specification

# Agent 43: Format detection
Task: Implement format auto-detection

# Wait for both to complete, then:

# Agent 44: Parser implementation
Task: Implement ETI-NI frame parser

# Agent 45: Integration
Task: Integrate and test with Bangkok file
```

### **Step 2: Build & Test**
```bash
cd /home/seksan/workspace/streamdab-analyzer/build
cmake ..
make streamdab-cli -j4

# Test
./streamdab-cli --input ../eti/bkk_20062022_141637.eti \
                --output test_ni.yaml \
                --verbose
```

### **Step 3: Validate**
```bash
# Check subchannel count
grep "scid:" test_ni.yaml | wc -l  # Should be 10

# Check FIG processing
grep "total_figs_processed" test_ni.yaml  # Should be >0

# Visual comparison
head -100 test_ni.yaml
head -100 etisnoop-result.txt
```

---

## 📝 **NOTES**

- **Backwards Compatibility:** ETI-LI parsing must continue to work
- **Test Coverage:** Both formats should have >85% coverage
- **Performance:** Target >1000 FPS for both formats
- **Standards:** 100% ETSI EN 300 799 compliance

---

**READY TO EXECUTE:** Yes
**Estimated Completion:** 2-3 days (with 4 agents in parallel)
**Success Probability:** High (95%+)
