# PDCA Week 5 Plan - 100% Unit Test Achievement + FIG Type Expansion

**Date:** November 2, 2025
**Status:** ✅ **PLANNING COMPLETE** - Ready for Do Phase
**Methodology:** PDCA (Plan-Do-Check-Act) Multi-Agent Parallel Development

---

## 🎯 Executive Summary

**PDCA Week 5 Goal:** Achieve **100% unit test pass rate** by fixing remaining FIG 0/13 failures, then expand ETSI EN 300 401 compliance with three additional FIG types.

**Starting Position (Week 4 Complete):**
- ✅ FIG 0/3: 22/22 tests (100%) - Production-ready
- ✅ FIG 0/6: 22/22 tests (100%) - Production-ready
- ⚡ FIG 0/13: 11/21 tests (52%) - Improved but incomplete
- **Overall: 55/65 tests (87%)** vs 100% target (-13% gap)

**Week 5 Target:**
- ✅ FIG 0/13: 21/21 tests (100%) - Fix remaining 10 failures
- ✅ Overall: 65/65 tests (100%) - Meet unit test target
- ✅ Verify E2E/GUI tests ≥85% pass rate
- ✅ All files in actual project directory

---

## 📋 PDCA Week 5 Scope

### **Phase 1: Fix FIG 0/13 to 100% (Priority 1)**

**Goal:** Fix 10 remaining test failures in FIG 0/13 parser

**Root Causes Identified from Week 4:**
1. **UAType Extraction for Large Values**
   - Small UAType (0x002): ✅ Works correctly
   - Large UAType (0x123): ❌ Gets 18 instead of 291
   - **Issue:** Bit packing formula incorrect for UAType > 0x0FF

2. **CA Flag Detection**
   - Test expects CA=1 from bytes 0x00, 0xC2
   - Current code gets CA=0
   - **Issue:** CA flag may be in different bit position

3. **Minimum Data Size Validation**
   - Rejecting 8-byte valid inputs
   - Error: "got 8 bytes, need at least 9"
   - **Issue:** Overly strict size validation

**Failing Tests (10 total):**
1. `test_uatype_epg` - UAType large value (0x123)
2. `test_ca_flag_set` - CA flag extraction
3. `test_scids_12bit` - SCIdS boundary
4. `test_uatype_11bit_max` - UAType maximum
5. `test_uatype_not_used` - Special UAType handling
6. `test_uatype_unknown` - Unknown UAType
7. `test_signal_emission` - Qt signal emission
8. `test_minimum_valid_size` - Size validation
9. `test_helper_methods` - Helper method logic
10. `test_all_uatype_coverage` - Comprehensive coverage

**Acceptance Criteria:**
- ✅ All 21 FIG 0/13 tests passing (100%)
- ✅ ETSI EN 300 401 Section 8.1.14 full compliance
- ✅ Performance maintained (<15µs per parse)
- ✅ Zero compilation errors
- ✅ Git commit with detailed analysis

**Estimated Effort:** 3-5 hours
**Deliverable:** Fixed `src/core/fig_parser.cpp` in actual project

---

### **Phase 2: E2E and GUI Test Verification (Priority 2)**

**Goal:** Verify E2E and GUI test suites meet 85% target

**E2E Tests to Verify:**
- `test_phase3a_complete_pipeline` (26 tests)
- Integration tests (audio, EPG, Journaline pipelines)
- Expected pass rate: ≥85% (22/26 tests)

**GUI Tests to Verify:**
- `test_gui_stream_info_panel` (10 tests)
- `test_gui_service_discovery_panel` (10 tests)
- `test_gui_fig_type_display_panel` (10 tests)
- `test_gui_main_window` (10+ tests)
- Expected pass rate: ≥85% (34/40 tests)

**Acceptance Criteria:**
- ✅ E2E tests: ≥85% pass rate
- ✅ GUI tests: ≥85% pass rate
- ✅ Document any failures with root cause analysis
- ✅ Report results in Week 5 completion document

**Estimated Effort:** 1-2 hours
**Deliverable:** Test verification report in actual project

---

### **Phase 3: Optional FIG Type Expansion (If Time Permits)**

**Goal:** Implement additional FIG types to increase ETSI compliance

**Target FIG Types (in priority order):**

**Option A: FIG 0/8 - Service Component Global Definition**
- ETSI EN 300 401 Section 8.1.6
- Complexity: Medium
- Estimated effort: 4-6 hours
- Test target: 20+ tests, 100% pass rate

**Option B: FIG 0/9 - Country/LTO/International Table**
- ETSI EN 300 401 Section 8.1.8
- Complexity: Low-Medium
- Estimated effort: 3-4 hours
- Test target: 15+ tests, 100% pass rate

**Option C: FIG 0/14 - FEC Sub-channel Organization**
- ETSI EN 300 401 Section 8.1.5
- Complexity: Medium-High
- Estimated effort: 5-7 hours
- Test target: 22+ tests, 100% pass rate

**Decision Criteria:**
- Only proceed if Phase 1 achieves 100% and takes <4 hours
- Must maintain 100% pass rate for all tests
- All files must be in actual project directory

**Acceptance Criteria (if implemented):**
- ✅ Parser implementation in `src/core/fig_parser.cpp`
- ✅ Comprehensive test suite (20+ tests)
- ✅ 100% test pass rate
- ✅ ETSI standard compliance
- ✅ Performance <15µs per parse
- ✅ Git commits with detailed messages

---

## 🔧 Multi-Agent Parallel Workflow

### **Agent Assignment (Flexible Based on Phase 1 Results)**

#### **Agent 15: FIG 0/13 Bug Fix Specialist**
**Responsibility:** Fix all 10 remaining FIG 0/13 test failures

**Deliverables:**
1. **Fixed Parser Code**
   - File: `src/core/fig_parser.cpp` (actual project)
   - Method: `parseFig013_UserApplicationInfo()`
   - Lines affected: ~20-30 lines

2. **Test Verification**
   - Run: `./tests/test_fig013_user_application`
   - Target: 21/21 tests passing (100%)
   - Performance: <15µs maintained

3. **Documentation**
   - File: `AGENT15_FIG013_FIX_REPORT.md` (actual project)
   - Content: Root cause analysis, fixes applied, ETSI compliance notes
   - Lines: ~150-200 lines

**Acceptance Criteria:**
- ✅ All 21 FIG 0/13 tests passing
- ✅ ETSI EN 300 401 §8.1.14 compliance verified
- ✅ Zero regressions in other tests
- ✅ Git commit with detailed message

**Estimated Time:** 3-5 hours

---

#### **Agent 16: E2E and GUI Test Verifier**
**Responsibility:** Run and verify E2E/GUI test suites

**Deliverables:**
1. **E2E Test Results**
   - Run: `./tests/e2e/test_phase3a_complete_pipeline`
   - Document: Pass/fail for all 26 tests
   - Target: ≥22/26 (85%)

2. **GUI Test Results**
   - Run: All GUI test executables
   - Document: Pass/fail for all ~40 tests
   - Target: ≥34/40 (85%)

3. **Verification Report**
   - File: `AGENT16_E2E_GUI_TEST_REPORT.md` (actual project)
   - Content: Test results, failure analysis, recommendations
   - Lines: ~100-150 lines

**Acceptance Criteria:**
- ✅ E2E tests: ≥85% pass rate
- ✅ GUI tests: ≥85% pass rate
- ✅ Failure analysis documented
- ✅ No test infrastructure issues

**Estimated Time:** 1-2 hours

---

#### **Agent 17: FIG Type Implementation (Conditional)**
**Responsibility:** Implement FIG 0/8, 0/9, or 0/14 if Phase 1 completes quickly

**Deliverables (if activated):**
1. **Parser Implementation**
   - File: `src/core/fig_parser.hpp` (struct definition)
   - File: `src/core/fig_parser.cpp` (parser method)
   - Lines: ~150-200 lines total

2. **Test Suite**
   - File: `tests/test_fig00X_*.cpp` (actual project)
   - Tests: 20+ comprehensive unit tests
   - Lines: ~400-500 lines

3. **CMake Integration**
   - File: `tests/CMakeLists.txt`
   - Add test executable configuration
   - Lines: ~20-25 lines

**Acceptance Criteria:**
- ✅ 100% test pass rate for new parser
- ✅ ETSI standard compliance
- ✅ Zero regressions in existing tests
- ✅ Git commit with detailed message

**Estimated Time:** 4-7 hours (depends on FIG type)

---

#### **Agent 18: Integration and Documentation Manager**
**Responsibility:** Coordinate all agents, create final Week 5 report

**Deliverables:**
1. **Week 5 Completion Report**
   - File: `PDCA_WEEK5_COMPLETE.md` (actual project)
   - Content: All deliverables, test results, achievements
   - Lines: ~300-400 lines

2. **Test Metrics Summary**
   - File: `WEEK5_TEST_METRICS.md` (actual project)
   - Content: Before/after comparison, targets vs actual
   - Lines: ~150-200 lines

3. **Git Commit Summary**
   - Verify all changes committed
   - Ensure no /tmp files
   - Document commit hashes

**Acceptance Criteria:**
- ✅ All agent deliverables integrated
- ✅ Comprehensive documentation
- ✅ All files in actual project
- ✅ Git history clean and detailed

**Estimated Time:** 2-3 hours

---

## 📊 Success Metrics

### **Primary Metrics (Must Achieve):**

| **Metric** | **Week 4 Actual** | **Week 5 Target** | **Gap** |
|------------|-------------------|-------------------|---------|
| **Unit Test Pass Rate** | 87% (55/65) | **100% (65/65)** | +13% |
| **FIG 0/13 Tests** | 52% (11/21) | **100% (21/21)** | +48% |
| **E2E Test Pass Rate** | TBD | **≥85%** | TBD |
| **GUI Test Pass Rate** | TBD | **≥85%** | TBD |

### **Secondary Metrics (Nice to Have):**

| **Metric** | **Current** | **Target** | **Status** |
|------------|-------------|------------|------------|
| **ETSI Compliance** | 92% | 96% | If Phase 3 completed |
| **Total Test Count** | 277+ | 297+ | If FIG type added |
| **Code Quality Grade** | A+ (98/100) | A+ (99/100) | Maintain |

---

## 🎯 Acceptance Criteria (Week 5 Complete)

### **Must Have (100% Required):**
1. ✅ **FIG 0/13: 21/21 tests passing (100%)**
2. ✅ **Overall unit tests: 65/65 passing (100%)**
3. ✅ **E2E tests: ≥85% pass rate**
4. ✅ **GUI tests: ≥85% pass rate**
5. ✅ **All files in actual project directory (no /tmp)**
6. ✅ **Git commits with detailed messages**
7. ✅ **Zero compilation errors**
8. ✅ **Performance maintained (<15µs)**

### **Should Have (80% Required):**
1. ✅ **Comprehensive documentation (300+ lines)**
2. ✅ **Root cause analysis for all fixes**
3. ✅ **ETSI compliance verification**
4. ✅ **Test failure analysis documented**

### **Nice to Have (Optional):**
1. ⚡ **Additional FIG type implementation (0/8, 0/9, or 0/14)**
2. ⚡ **ETSI compliance increased to 96%**
3. ⚡ **Total test count >297 tests**

---

## 📋 Timeline

### **Week 5 Schedule:**

**Day 1 (November 2, 2025):**
- ✅ Create PDCA Week 5 Plan (this document)
- 🔄 Agent 15: Begin FIG 0/13 bug analysis
- 🔄 Agent 16: Start E2E/GUI test runs

**Day 2-3:**
- 🔄 Agent 15: Fix FIG 0/13 failures, verify 100%
- 🔄 Agent 16: Complete E2E/GUI verification
- 🔄 Agent 18: Begin documentation

**Day 4 (Optional):**
- ⚡ Agent 17: Implement additional FIG type (if Phase 1 fast)

**Day 5:**
- 🔄 Agent 18: Complete Week 5 documentation
- ✅ Final verification and git commits

---

## 🔍 Risk Assessment

### **High Risk Items:**

1. **FIG 0/13 ETSI Compliance Ambiguity**
   - Risk: Bit packing specification unclear
   - Mitigation: Consult ETSI EN 300 401 §8.1.14 directly, test with known-good data
   - Impact if fails: Week 5 target missed

2. **E2E/GUI Test Infrastructure Issues**
   - Risk: Tests may fail due to environment issues, not code
   - Mitigation: Verify test harness, check dependencies
   - Impact if fails: Cannot verify 85% target

### **Medium Risk Items:**

1. **Time Constraint for Optional Phase 3**
   - Risk: FIG 0/13 fixes take >4 hours
   - Mitigation: Focus on Phase 1-2 completion first
   - Impact if fails: No new FIG types, but primary goal met

### **Low Risk Items:**

1. **Documentation Overhead**
   - Risk: Documentation takes longer than coding
   - Mitigation: Agent 18 works in parallel
   - Impact if fails: Documentation delayed but deliverables complete

---

## ✅ Week 5 Readiness Checklist

**Pre-Start Verification:**
- ✅ Week 4 complete at 87% (55/65 tests)
- ✅ FIG 0/3 and FIG 0/6 at 100% (44/44 tests)
- ✅ Build system working (zero errors)
- ✅ Git repository clean
- ✅ Week 5 plan created (this document)
- ✅ All Week 4 files in actual project (verified)

**Agent Resources:**
- ✅ Agent 15: FIG 0/13 parser code access
- ✅ Agent 16: E2E/GUI test executables
- ✅ Agent 17: ETSI standards documentation
- ✅ Agent 18: All agent outputs

**Technical Resources:**
- ✅ ETSI EN 300 401 Section 8.1.14 (FIG 0/13)
- ✅ Test harness operational
- ✅ CMake build system configured
- ✅ Git version control ready

---

## 📚 Reference Documents

**Week 4 Deliverables:**
- [PDCA_WEEK4_COMPLETE.md](PDCA_WEEK4_COMPLETE.md) - Week 4 completion report
- [WEEK4_COMPLETION_STATUS.md](WEEK4_COMPLETION_STATUS.md) - Final status
- [TEST_TARGETS_WEEK4.md](TEST_TARGETS_WEEK4.md) - Test targets vs actual

**ETSI Standards:**
- ETSI EN 300 401 Section 8.1.14: User Application Information (FIG 0/13)
- ETSI EN 300 401 Section 8.1.6: Service Component Global Definition (FIG 0/8)
- ETSI EN 300 401 Section 8.1.8: Country/LTO/International Table (FIG 0/9)
- ETSI EN 300 401 Section 8.1.5: FEC Sub-channel Organization (FIG 0/14)

**Implementation Files:**
- [src/core/fig_parser.cpp](src/core/fig_parser.cpp) - Parser implementations
- [src/core/fig_parser.hpp](src/core/fig_parser.hpp) - Struct definitions
- [tests/test_fig013_user_application.cpp](tests/test_fig013_user_application.cpp) - FIG 0/13 tests

---

## 🎯 Week 5 Success Definition

**PDCA Week 5 will be considered COMPLETE when:**

1. ✅ **Unit tests: 65/65 passing (100%)**
2. ✅ **E2E tests: ≥85% pass rate**
3. ✅ **GUI tests: ≥85% pass rate**
4. ✅ **All files in actual project directory**
5. ✅ **Comprehensive documentation created**
6. ✅ **Git commits with detailed messages**
7. ✅ **Zero compilation errors**
8. ✅ **ETSI compliance verified**

**Grade Target:** **A (95/100) minimum**, **A+ (98/100) preferred**

---

**Status:** ✅ **PLAN PHASE COMPLETE - READY FOR DO PHASE**

**Next Action:** Launch Agent 15 to fix FIG 0/13 failures

---

**Document Created:** November 2, 2025
**Author:** Claude Code (Sonnet 4.5)
**PDCA Cycle:** Week 5 - Plan Phase ✅
**Methodology:** Multi-Agent Parallel Development
