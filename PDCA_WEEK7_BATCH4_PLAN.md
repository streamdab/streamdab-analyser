# PDCA WEEK 7 BATCH 4 PLAN - Final Push to 90%+ ETSI Compliance

**Date:** November 3, 2025
**Goal:** Fix FIG 0/7 + Implement remaining types to reach 90%+ ETSI compliance
**Methodology:** PDCA Multi-Agent Parallel Development (5 agents)
**Duration:** 4-6 hours (estimated)

---

## 🎯 OBJECTIVE

**Primary Goals:**
1. **Fix FIG 0/7** bit extraction issue (8 failing tests)
2. **Implement 4 remaining FIG types** (0/12, 0/15, 0/16, 0/20) to reach 90%+

**Current State:** 16-17/22 types (73-77% compliance)
**Target State:** 21/22 types (95% compliance)

**Success Criteria:**
- ✅ FIG 0/7 fixed: 15/15 tests passing (100%)
- ✅ All 4 new types implemented with comprehensive tests
- ✅ >90% test pass rate for each new type
- ✅ Performance <15µs per parse
- ✅ Zero compilation errors
- ✅ 95% ETSI compliance achieved

---

## 📊 AGENT ASSIGNMENTS

### **Agent 26: FIG 0/7 Debug & Fix (URGENT)**

**Priority:** CRITICAL (blocks Batch 3 completion)
**Complexity:** LOW (bit mask correction)
**Estimated Time:** 30 minutes

**Task:**
- Debug bit extraction in parseFig007_ServiceComponentStreamMode()
- Fix CA/DG flag bit positions per ETSI EN 300 401 Section 8.1.4
- Current issue: 8/15 tests failing due to incorrect bit masks
- Target: 15/15 tests passing

**ETSI Byte Structure (Corrected):**
```
Byte 1-2: [SCId 12-bit][CA 1-bit][DG 1-bit][RFA 2-bit]
          MSB----------------------------------------LSB
Byte 1:   [SCId bit 11-4]
Byte 2:   [SCId bit 3-0][CA][DG][RFA 2-bit]
```

**Current Implementation (INCORRECT):**
```cpp
result.ca_flag = (scid_byte2 & 0x04) != 0;  // bit 2
result.dg_flag = (scid_byte2 & 0x08) != 0;  // bit 3
```

**Correct Implementation (EXPECTED):**
```cpp
result.ca_flag = (scid_byte2 & 0x08) != 0;  // bit 3
result.dg_flag = (scid_byte2 & 0x04) != 0;  // bit 2
```

**Deliverables:**
- Fixed parseFig007_ServiceComponentStreamMode() in src/core/fig_parser.cpp
- Run tests to verify 15/15 passing
- Update FIG007 documentation

---

### **Agent 27: FIG 0/12 - MPEG-2 TS Streaming**

**ETSI Standard:** EN 300 401 Section 8.1.10
**Priority:** MEDIUM (rarely used, but specified)
**Complexity:** MEDIUM (MPEG-2 TS specific fields)
**Target Tests:** 10+ tests

**Deliverables:**
1. **Struct:** `Mpeg2TsStreaming` in fig_parser.hpp
   - Fields: service_id, msc_flag, sub_ch_id, mpeg_frame_size, is_valid
   - Helper methods: isMscTransport(), getFrameSize()

2. **Parser:** `parseFig012_Mpeg2TsStreaming()` in fig_parser.cpp
   - Extract 32-bit Service ID
   - Parse MSC flag
   - Extract SubChId if MSC
   - Extract MPEG-2 TS frame size

3. **Switch Case:** `case 12` in parseFigType0()

4. **Signal:** `mpeg2TsStreamingDiscovered()`

5. **Test Suite:** tests/test_fig012_mpeg2_ts.cpp (10+ tests)

**ETSI Byte Structure:**
```
Byte 0: Extension (0x0C)
Byte 1-4: Service ID (32-bit)
Byte 5: [MSC flag 1-bit][SubChId 6-bit][RFA]
Byte 6-7: MPEG-2 TS frame size (16-bit)
```

---

### **Agent 28: FIG 0/15 - Programme Number**

**ETSI Standard:** EN 300 401 Section 8.1.13
**Priority:** MEDIUM (programme identification)
**Complexity:** LOW (simple ID mapping)
**Target Tests:** 10+ tests

**Deliverables:**
1. **Struct:** `ProgrammeNumber` in fig_parser.hpp
   - Fields: service_id, programme_number, continuation_flag, update_flag, is_valid
   - Helper methods: isContinuation(), hasUpdate()

2. **Parser:** `parseFig015_ProgrammeNumber()` in fig_parser.cpp
   - Extract Service ID (16/32-bit)
   - Extract Programme Number (16-bit)
   - Parse continuation and update flags

3. **Switch Case:** `case 15` in parseFigType0()

4. **Signal:** `programmeNumberDiscovered()`

5. **Test Suite:** tests/test_fig015_programme_number.cpp (10+ tests)

**ETSI Byte Structure:**
```
Byte 0: Extension (0x0F)
Byte 1-2(or 1-4): Service ID (16 or 32-bit)
Byte N-N+1: Programme Number (16-bit)
Byte N+2: [Continuation][Update][RFA]
```

---

### **Agent 29: FIG 0/16 - Programme Type International**

**ETSI Standard:** EN 300 401 Section 8.1.14
**Priority:** LOW (international programme types)
**Complexity:** LOW (extends FIG 0/17)
**Target Tests:** 10+ tests

**Deliverables:**
1. **Struct:** `ProgrammeTypeInternational` in fig_parser.hpp
   - Fields: service_id, international_code, programme_type, language_code, is_valid
   - Helper methods: getInternationalTypeName()

2. **Parser:** `parseFig016_ProgrammeTypeInternational()` in fig_parser.cpp
   - Extract Service ID
   - Extract international programme type code
   - Parse language code

3. **Switch Case:** `case 16` in parseFigType0()

4. **Signal:** `programmeTypeInternationalDiscovered()`

5. **Test Suite:** tests/test_fig016_programme_type_intl.cpp (10+ tests)

**ETSI Byte Structure:**
```
Byte 0: Extension (0x10)
Byte 1-2: Service ID (16-bit)
Byte 3: [International code 8-bit]
Byte 4: [Programme type 5-bit][Language 8-bit]
```

---

### **Agent 30: FIG 0/20 - Service Component Information**

**ETSI Standard:** EN 300 401 Section 8.1.15
**Priority:** MEDIUM (component metadata)
**Complexity:** MEDIUM (multiple component types)
**Target Tests:** 12+ tests

**Deliverables:**
1. **Struct:** `ServiceComponentInfo` in fig_parser.hpp
   - Fields: service_id, sc_id, ca_flag, num_components, component_list, is_valid
   - Helper methods: getComponentCount(), isProtected()

2. **Parser:** `parseFig020_ServiceComponentInfo()` in fig_parser.cpp
   - Extract Service ID
   - Extract Service Component ID
   - Parse CA flag
   - Parse component list

3. **Switch Case:** `case 20` in parseFigType0()

4. **Signal:** `serviceComponentInfoDiscovered()`

5. **Test Suite:** tests/test_fig020_service_component_info.cpp (12+ tests)

**ETSI Byte Structure:**
```
Byte 0: Extension (0x14)
Byte 1-2: Service ID (16-bit)
Byte 3-4: SC ID (12-bit) + flags
Byte 5+: Component list (variable)
```

---

## 📋 PARALLEL EXECUTION STRATEGY

### **Phase 1: Launch All 5 Agents Simultaneously (5 min)**

**Agent Launch Order (parallel):**
1. Agent 26 (FIG 0/7 fix) - CRITICAL
2. Agent 27 (FIG 0/12)
3. Agent 28 (FIG 0/15)
4. Agent 29 (FIG 0/16)
5. Agent 30 (FIG 0/20)

**Note:** All agents work independently on separate FIG types (no file conflicts)

### **Phase 2: Monitor Progress (45-60 min)**

**Expected Completion Times:**
- Agent 26: 30 minutes (debug + test)
- Agent 27: 1 hour (implementation + tests)
- Agent 28: 45 minutes (simple implementation)
- Agent 29: 45 minutes (simple implementation)
- Agent 30: 1 hour (complex implementation)

### **Phase 3: Build and Test All (15 min)**

```bash
cd /home/seksan/workspace/streamdab-analyzer/build
cmake ..
make test_fig007_stream_mode test_fig012_mpeg2_ts test_fig015_programme_number \
     test_fig016_programme_type_intl test_fig020_service_component_info -j5
```

### **Phase 4: Verification (10 min)**

Run all tests and verify:
- FIG 0/7: 15/15 passing (100%)
- FIG 0/12: >9/10 passing (>90%)
- FIG 0/15: >9/10 passing (>90%)
- FIG 0/16: >9/10 passing (>90%)
- FIG 0/20: >11/12 passing (>90%)

### **Phase 5: Documentation and Commit (20 min)**

- Create PDCA_WEEK7_BATCH4_COMPLETE.md
- Commit all changes to git
- Update ETSI compliance status

---

## ✅ ACCEPTANCE CRITERIA

### **Batch 4 Overall:**

| **Criterion** | **Target** | **Status** |
|---------------|-----------|-----------|
| FIG 0/7 fixed | 15/15 tests (100%) | ⏳ |
| All 4 types implemented | Yes (0/12,15,16,20) | ⏳ |
| Total tests added | 42+ tests | ⏳ |
| Combined pass rate | >90% | ⏳ |
| ETSI compliance | 95% (21/22) | ⏳ |
| Zero compilation errors | Yes | ⏳ |
| Time | <6 hours | ⏳ |

### **Per-Agent Success:**

| **Agent** | **FIG Type** | **Tests** | **Pass Rate** | **Status** |
|-----------|--------------|-----------|---------------|-----------|
| Agent 26 | 0/7 Fix | 15/15 | 100% | ⏳ |
| Agent 27 | 0/12 MPEG-2 | 10+ | >90% | ⏳ |
| Agent 28 | 0/15 Prog# | 10+ | >90% | ⏳ |
| Agent 29 | 0/16 Intl | 10+ | >90% | ⏳ |
| Agent 30 | 0/20 Info | 12+ | >90% | ⏳ |

---

## 📈 EXPECTED COMPLIANCE PROGRESS

```
Before Batch 4: ████████████████░░░░░░░ 73% (16/22 types)
After Batch 4:  ████████████████████░░░ 95% (21/22 types)
Target 100%:    ████████████████████████ 100% (22/22 types)
```

**Remaining After Batch 4:** FIG 0/24 (OE Services) - 1 type for 100%

---

## 🚀 TIME ESTIMATES

**Parallel Execution:**
- Agent launch: 5 minutes
- Agent execution (parallel): 1 hour (longest agent)
- Build and test: 15 minutes
- Verification: 10 minutes
- Documentation: 20 minutes
- **Total: 1 hour 50 minutes**

**Sequential Approach (hypothetical):**
- Agent 26: 30 min
- Agent 27: 1 hour
- Agent 28: 45 min
- Agent 29: 45 min
- Agent 30: 1 hour
- Build/test: 30 min
- Docs: 30 min
- **Total: 5 hours**

**Time Savings: 63% reduction**

---

## 🎯 SUCCESS METRICS

**Minimum Success:**
- ✅ FIG 0/7 fixed (15/15 tests)
- ✅ 3/4 new types at >90% pass rate
- ✅ ETSI compliance ≥86% (19/22 types)
- ✅ Zero compilation errors

**Full Success:**
- ✅ FIG 0/7 fixed (15/15 tests)
- ✅ 4/4 new types at >90% pass rate
- ✅ ETSI compliance 95% (21/22 types)
- ✅ All tests passing >92% overall
- ✅ Time <2 hours

---

## 📚 FINAL ETSI COMPLIANCE STATUS

**After Batch 4 Completion:**
- **Implemented Types:** 21/22 (95%)
- **Total Tests:** ~500 tests
- **Pass Rate:** >92%
- **Missing:** Only FIG 0/24 (OE Services) - rarely used

**Production Readiness:** EXCELLENT
- All critical FIG types implemented
- Emergency Warning System complete
- Comprehensive test coverage
- Professional code quality

---

## ✅ READY TO LAUNCH BATCH 4

**Status:** ⏳ Awaiting agent launch
**Next:** Execute 5 parallel agents
**Expected Completion:** 2 hours
**Final Goal:** 95% ETSI EN 300 401 compliance

---

**Document Version:** 1.0
**Date:** November 3, 2025
**Method:** PDCA Multi-Agent Parallel Development
