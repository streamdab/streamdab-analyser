# PDCA Week 8 - MOC Compilation Fix Plan

**Date:** November 6, 2025
**Goal:** Fix MOC compilation errors and achieve 100% test pass rate (>85% coverage)
**Status:** PLAN Phase
**Methodology:** Multi-Agent PDCA Parallel Execution

## 🎯 **Problem Statement**

**Current Status:**
- 76/82 tests passing (93%)
- 39/41 test suites blocked by MOC compilation errors
- Qt MOC cannot process custom `eti::` types in member variables

**Target:**
- ✅ 100% test pass rate (ALL tests passing)
- ✅ >85% code coverage
- ✅ Zero compilation errors
- ✅ All 41 test suites buildable

## 📋 **PLAN Phase - Multi-Agent Strategy**

### **Agent Assignment (4 Parallel Agents)**

#### **Agent 38: MOC Error Fix - eti_processor.hpp**
**Specialization:** C++ Refactoring Agent
**Duration:** 2-3 hours
**Priority:** CRITICAL

**Task:**
1. Fix MOC compilation errors in `src/core/eti_processor.hpp`
2. Comment out or refactor member variables using `eti::` types:
   - `eti::Ensemble current_ensemble_`
   - `std::vector<eti::DabService> discovered_services_`
   - `std::vector<eti::SubChannelInfo> subchannels_`
3. Update `src/core/eti_processor.cpp` implementation accordingly
4. Verify compilation succeeds

**Deliverables:**
- ✅ `src/core/eti_processor.hpp` - Fixed (member variables commented/refactored)
- ✅ `src/core/eti_processor.cpp` - Updated implementation
- ✅ Compilation test: `cmake --build . --target DABAnalyzer_Enhanced_FilteringBuild`
- ✅ Report: `AGENT38_MOC_FIX_COMPLETE.md`

**Acceptance Criteria:**
- Zero MOC compilation errors
- `eti_processor.cpp` compiles successfully
- No regressions in existing code

---

#### **Agent 39: Rebuild All Test Suites**
**Specialization:** Build System Agent
**Duration:** 30-60 minutes
**Priority:** HIGH
**Depends On:** Agent 38 completion

**Task:**
1. Rebuild all 41 test suites after MOC fix
2. Verify all test executables build successfully
3. Document any remaining build failures
4. Create test inventory

**Deliverables:**
- ✅ Build command: `cmake --build . -j4`
- ✅ Test executable count: 41/41 built
- ✅ Build log: `build_all_tests.log`
- ✅ Report: `AGENT39_TEST_BUILD_COMPLETE.md`

**Acceptance Criteria:**
- All 41 test executables present in `build/tests/`
- Zero build errors
- Build time <10 minutes

---

#### **Agent 40: Fix YAML Generator Integration Tests**
**Specialization:** Test Engineering Agent
**Duration:** 1-2 hours
**Priority:** MEDIUM

**Task:**
1. Fix 6 failing YAML generator tests:
   - test02_EnsembleInfoOutput
   - test03_ServiceListOutput
   - test04_SubchannelOutput
   - test08_LargeServiceCounts
   - test10_MissingOptionalFields
   - test18_HexadecimalFormatting
2. Create mock `HeadlessETIProcessor` OR update test expectations
3. Verify all 22 tests pass

**Deliverables:**
- ✅ `tests/test_yaml_output_generator.cpp` - Fixed (22/22 tests passing)
- ✅ OR: Mock implementation created
- ✅ Test run: `./test_yaml_output_generator` (100% pass)
- ✅ Report: `AGENT40_YAML_TESTS_FIXED.md`

**Acceptance Criteria:**
- 22/22 YAML tests passing (100%)
- No mock data hardcoding (proper test design)
- Coverage >85% for yaml_output_generator.cpp

---

#### **Agent 41: Code Coverage Analysis & Report**
**Specialization:** QA Coverage Agent
**Duration:** 1-2 hours
**Priority:** MEDIUM
**Depends On:** Agent 39 completion

**Task:**
1. Build with coverage flags: `cmake .. -DCMAKE_CXX_FLAGS="--coverage"`
2. Run all 41 test suites: `ctest --output-on-failure`
3. Generate lcov coverage report
4. Verify >85% coverage for CLI mode code
5. Create comprehensive coverage report

**Deliverables:**
- ✅ Coverage build successful
- ✅ All tests run: `ctest` output
- ✅ Coverage report: `coverage_html/index.html`
- ✅ Coverage summary: `AGENT41_COVERAGE_REPORT.md`
- ✅ CLI coverage: >85% verified

**Acceptance Criteria:**
- Line coverage >85% for:
  - `src/cli/cli_argument_parser.cpp`
  - `src/cli/yaml_output_generator.cpp`
  - `src/cli/cli_main.cpp`
- Function coverage >85%
- Branch coverage >80%
- HTML report generated

---

## 📊 **Success Metrics**

| **Metric** | **Current** | **Target** | **Agent** |
|------------|-------------|------------|-----------|
| Test Pass Rate | 93% (76/82) | 100% (ALL) | 38, 39, 40 |
| Buildable Tests | 7% (3/41) | 100% (41/41) | 38, 39 |
| CLI Coverage | Unknown | >85% | 41 |
| MOC Errors | 39 suites | 0 errors | 38 |
| Build Errors | Multiple | 0 errors | 38, 39 |

## 🔄 **DO Phase - Execution Plan**

### **Step 1: Launch All Agents in Parallel**
```bash
# Launch Agent 38, 40 immediately (independent tasks)
# Launch Agent 39, 41 after Agent 38 completes (dependencies)
```

### **Step 2: Monitor Progress**
- Agent 38: MOC fix (CRITICAL PATH)
- Agent 40: YAML tests (parallel with Agent 38)
- Agent 39: Build all tests (waits for Agent 38)
- Agent 41: Coverage (waits for Agent 39)

### **Step 3: Validation**
- Verify all deliverables created
- Check acceptance criteria met
- Run final validation tests

## ✅ **CHECK Phase - Validation Criteria**

1. **Build Validation:**
   - `cmake --build . -j4` → Zero errors
   - All 41 test executables present

2. **Test Validation:**
   - `ctest --output-on-failure` → 100% pass rate
   - No skipped or disabled tests

3. **Coverage Validation:**
   - `lcov` report → >85% for CLI code
   - HTML report accessible

4. **Documentation Validation:**
   - All agent reports created
   - TEST_STATUS_REPORT.md updated

## 🎯 **ACT Phase - Completion Actions**

1. **Update Documentation:**
   - Update TEST_STATUS_REPORT.md with final results
   - Update CLAUDE.md with Week 8 completion
   - Create PDCA_WEEK8_COMPLETE.md

2. **Git Commit:**
   - Commit all fixes
   - Tag release: `v1.2-cli-complete`

3. **Final Validation:**
   - Run full test suite
   - Generate final coverage report
   - Document lessons learned

## 📋 **Acceptance Criteria (Overall)**

- ✅ 100% test pass rate (ALL tests passing)
- ✅ >85% code coverage for CLI mode
- ✅ Zero MOC compilation errors
- ✅ All 41 test suites buildable
- ✅ Zero build errors
- ✅ All agent reports complete
- ✅ Documentation updated

## 🚀 **Ready to Execute**

**Status:** ✅ PLAN COMPLETE - Ready for DO phase
**Estimated Time:** 4-6 hours total (with parallel execution)
**Critical Path:** Agent 38 → Agent 39 → Agent 41
**Parallel Track:** Agent 40 (independent)

---

**Next Step:** Launch agents using Task tool with this plan.
