# PDCA Plan: ETI-NI Parser - Final Completion

**Date:** November 8, 2025
**Priority:** CRITICAL
**Status:** Parser Working (99.9%), Final Integration Needed
**Estimated Time:** 2-4 hours with parallel agents

---

## 🎯 **CURRENT STATUS**

### **What Works (99.9%):**
✅ ETI-NI format detection
✅ Frame parsing (4998/5001 frames)
✅ NST extraction with NST-1 decoding
✅ Subchannel STC field extraction
✅ FIC extraction
✅ FIG processing (6 types detected)
✅ Fast performance (14,207 FPS)

### **What's Missing:**
❌ Result aggregation - subchannels not collected into final output
❌ Service aggregation - services not collected into final output
❌ YAML output incomplete - shows empty arrays
❌ Unit tests not integrated
❌ ETI-LI regression tests not run

---

## 📋 **PLAN PHASE - Multi-Agent Tasks**

### **Agent 45: Result Aggregation & YAML Output Fix**
**Priority:** CRITICAL
**Time:** 1-2 hours

**Problem:**
Parser extracts subchannels/services from frames but doesn't aggregate them for YAML output.

**Tasks:**
1. Modify `src/core/headless_eti_processor.cpp`:
   - Add subchannel collection from parsed frames
   - Add service collection from FIG analysis
   - Build unique lists (deduplicate by SCID/SID)
   - Store for YAML generator

2. Modify `src/cli/yaml_output_generator.cpp`:
   - Read aggregated subchannels
   - Read aggregated services
   - Output in etisnoop-compatible format

3. Test with Bangkok file:
   - Should show 10 subchannels (not empty array)
   - Should show services discovered
   - Should match etisnoop structure

**Deliverable:**
- Fixed `headless_eti_processor.cpp` with aggregation
- Updated `yaml_output_generator.cpp`
- YAML output with real data

---

### **Agent 46: Unit Tests for ETI-NI Parser**
**Priority:** HIGH
**Time:** 2-3 hours

**Tasks:**
1. Create `tests/test_eti_ni_parser.cpp`:
   - Test NST extraction (10, 26, 42, 58 streams)
   - Test NST-1 decoding
   - Test dynamic FIC offset calculation
   - Test subchannel STC extraction
   - Test format detection
   - Test Bangkok file parsing
   - 25+ comprehensive tests

2. Add to CMakeLists.txt

3. Run and verify 100% pass rate

**Deliverable:**
- `tests/test_eti_ni_parser.cpp` (25+ tests)
- All tests passing
- Coverage report showing >85%

---

### **Agent 47: ETI-LI Regression Testing**
**Priority:** HIGH
**Time:** 1-2 hours

**Tasks:**
1. Find or create ETI-LI test files
2. Test existing ETI-LI parsing still works
3. Verify no regressions from ETI-NI changes
4. Create regression test suite

**Deliverable:**
- Verified ETI-LI still works
- Regression test results
- Both formats working side-by-side

---

### **Agent 48: Integration & Final Validation**
**Priority:** HIGH
**Time:** 1-2 hours

**Tasks:**
1. Run complete test suite
2. Compare Bangkok output with etisnoop
3. Validate subchannel data matches
4. Create final validation report
5. Document any differences

**Deliverable:**
- Complete test run results
- Comparison report vs etisnoop
- Final validation documentation
- Production readiness assessment

---

## 🔄 **DO PHASE - Parallel Execution**

### **Launch All 4 Agents Simultaneously:**

```
Agent 45 (CRITICAL) → Result aggregation
Agent 46 (HIGH)     → Unit tests
Agent 47 (HIGH)     → Regression tests
Agent 48 (HIGH)     → Validation

All agents work independently, no dependencies
```

---

## ✅ **CHECK PHASE - Validation Criteria**

### **Agent 45 Success:**
- [ ] Bangkok file shows 10 subchannels in YAML
- [ ] Services array populated (not empty)
- [ ] FIG statistics show counts >0
- [ ] Output structure matches etisnoop

### **Agent 46 Success:**
- [ ] 25+ tests created
- [ ] 100% pass rate
- [ ] >85% code coverage
- [ ] All ETI-NI features tested

### **Agent 47 Success:**
- [ ] ETI-LI files still parse correctly
- [ ] Zero regressions detected
- [ ] Both formats coexist properly

### **Agent 48 Success:**
- [ ] Subchannel count matches etisnoop
- [ ] Bitrates match (64 kbps)
- [ ] Protection levels match (EEP 3-A)
- [ ] All tests passing

---

## 🎬 **ACT PHASE - Completion**

### **Final Deliverables:**

1. **Code:**
   - Fixed `headless_eti_processor.cpp`
   - Updated `yaml_output_generator.cpp`
   - New `test_eti_ni_parser.cpp`
   - Regression test suite

2. **Documentation:**
   - Agent completion reports (4 docs)
   - Final validation report
   - Comparison with etisnoop
   - Updated CLAUDE.md

3. **Testing:**
   - All unit tests passing
   - Integration tests passing
   - Bangkok file output validated
   - ETI-LI regression confirmed

4. **Git Commit:**
   ```
   feat: Complete ETI-NI parser implementation

   - Add ETI-NI format support (ETSI EN 300 799 Section 4.2)
   - Auto-detect ETI-LI vs ETI-NI
   - Parse Bangkok DAB file (99.9% success)
   - Extract 10 subchannels correctly
   - Aggregate results for YAML output
   - Add 25+ unit tests
   - Verify zero ETI-LI regressions
   - Performance: 14,207 FPS

   Agents 42-48 (PDCA multi-agent workflow)
   ```

---

## 📊 **SUCCESS METRICS**

| Metric | Current | Target | Status |
|--------|---------|--------|--------|
| Bangkok frames parsed | 4998/5001 | 5001/5001 | 99.9% |
| Subchannels in YAML | 0 | 10 | ⏳ Pending |
| Services in YAML | 0 | >0 | ⏳ Pending |
| Unit tests | 0 | 25+ | ⏳ Pending |
| Test pass rate | N/A | 100% | ⏳ Pending |
| ETI-LI regression | Unknown | 0 issues | ⏳ Pending |
| Code coverage | Unknown | >85% | ⏳ Pending |

---

## ⏱️ **TIME ESTIMATE**

**With 4 Parallel Agents:**
- Agent 45: 1-2 hours (CRITICAL PATH)
- Agent 46: 2-3 hours (parallel)
- Agent 47: 1-2 hours (parallel)
- Agent 48: 1-2 hours (parallel)

**Total Wall Time:** 2-3 hours (parallelized)
**Total Effort:** 5-9 hours (if sequential)

**Time Savings:** 60-70% with parallelization

---

## 🚀 **READY TO EXECUTE**

All agents have clear specifications and can work independently. No blocking dependencies.

**Execute Command:**
Launch all 4 agents simultaneously for maximum efficiency!
