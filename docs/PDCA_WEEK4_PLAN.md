# PDCA Week 4 Plan - Multi-Agent FIG Implementation

**Date:** October 30, 2025
**Phase:** PLAN (Week 4 of ETSI v1.1 Implementation)
**Status:** Ready to Execute
**Methodology:** Plan → Do → Check → Act (Multi-Agent Parallel Development)

---

## 🎯 Executive Summary

**Goal:** Implement 4 additional FIG types to advance EN 300 401 compliance from 92% → 96%

**Approach:** Deploy 4 specialized agents in parallel for maximum efficiency
**Timeline:** 1 week (November 4-8, 2025)
**Expected Outcome:** 4 FIG parsers + 80+ tests + zero regressions
**Success Criteria:** 95%+ test pass rate, <5µs performance, 100% ETSI compliance

---

## 📊 Week 4 Context

### Baseline Status (Week 3 Complete)

**Current State:**
- EN 300 401 Compliance: **92%**
- FIG Type 0 Complete: 5/13 (38%)
- FIG Type 1 Complete: 1/4 (25%)
- Total Tests: 285+ (97%+ pass rate)
- Code Quality: A+ (98/100)

**PDCA Weeks 1-3 Completions:**
- ✅ FIG 1/0: Ensemble label (16/16 tests)
- ✅ FIG 0/5: Service component language (17/18 tests)
- ✅ FIG 0/10: Date and time (MJD/UTC)
- ✅ FIG 0/17: Programme type (21/21 tests)
- ✅ FIG 0/18: Announcement support (20/20 tests)
- ✅ FIG 0/19: Announcement switching (integrated)
- ✅ TS 101 756: Registered tables (100%)

### Week 4 Targets

**Primary Goal:** Implement 4 High-Priority FIG Types

1. **FIG 0/3** - Service component in packet mode (High priority)
2. **FIG 0/6** - Service linking (Medium priority)
3. **FIG 0/13** - User application information (Medium priority)
4. **Agent 10 Integration** - 70 comprehensive tests (from Week 3)

**Secondary Goal:** Achieve 96% EN 300 401 compliance

---

## 🤖 Multi-Agent Task Assignment

### Agent 11: FIG 0/3 - Service Component in Packet Mode

**Mission:** Implement FIG 0/3 parser per ETSI EN 300 401 Section 8.1.3

**ETSI Standard:** EN 300 401 Section 8.1.3
**Priority:** High (required for DAB data services)
**Complexity:** Medium
**Estimated Effort:** 2 days → 1 day (parallel)

**Deliverables:**

1. **ServiceComponentPacketMode struct** in fig_parser.hpp:
   - uint32_t service_id (16 or 32-bit, SId flag dependent)
   - uint16_t sc_id (12-bit service component identifier)
   - uint8_t dg_flag (data group flag)
   - uint8_t dscty (data service component type)
   - uint16_t sub_ch_id (sub-channel ID)
   - uint16_t packet_address (10-bit packet address)
   - bool ca_flag (conditional access flag)
   - bool is_valid

2. **parseFig003_ServiceComponentPacketMode()** implementation:
   - Parse SId flag, DSCTy, SubChId, Packet Address
   - Handle CA flag (conditional access)
   - Support both 16-bit and 32-bit SId
   - Emit serviceComponentPacketModeDiscovered signal

3. **test_fig003_packet_mode.cpp** (20 tests minimum):
   - Valid packet mode parsing (16/32-bit SId)
   - DSCTy validation (all 63 types)
   - Packet address range (0-1023)
   - CA flag detection
   - Invalid data handling
   - Performance benchmark (<5µs)

**Acceptance Criteria:**
- 20/20 tests passing (100%)
- Performance: <5µs per parse
- ETSI EN 300 401 Section 8.1.3 compliant
- Zero regressions in existing code

---

### Agent 12: FIG 0/6 - Service Linking

**Mission:** Implement FIG 0/6 parser per ETSI EN 300 401 Section 8.1.15

**ETSI Standard:** EN 300 401 Section 8.1.15
**Priority:** Medium (multi-service DAB support)
**Complexity:** Medium
**Estimated Effort:** 2 days → 1 day (parallel)

**Deliverables:**

1. **ServiceLinking struct** in fig_parser.hpp:
   - uint32_t service_id (linking service)
   - bool id_list_flag (indicates list type)
   - bool la_flag (linkage actuator flag)
   - bool sh_flag (soft/hard link)
   - bool iis_flag (international/national)
   - uint8_t lsn (linkage set number)
   - std::vector<uint32_t> linked_service_ids
   - std::vector<uint16_t> ecc_ids (if international)
   - bool is_valid

2. **parseFig006_ServiceLinking()** implementation:
   - Parse LA, SH, IIS flags
   - Extract linkage set number (LSN)
   - Parse linked service IDs (IdLQ list)
   - Support international linking with ECC
   - Emit serviceLinkingDiscovered signal

3. **test_fig006_service_linking.cpp** (20 tests minimum):
   - Valid service linking (national)
   - International linking with ECC
   - LA/SH/IIS flag combinations
   - Multiple linked services
   - Linkage set number validation
   - Invalid data handling
   - Performance benchmark (<5µs)

**Acceptance Criteria:**
- 20/20 tests passing (100%)
- Performance: <5µs per parse
- ETSI EN 300 401 Section 8.1.15 compliant
- Zero regressions in existing code

---

### Agent 13: FIG 0/13 - User Application Information

**Mission:** Implement FIG 0/13 parser per ETSI EN 300 401 Section 8.1.14

**ETSI Standard:** EN 300 401 Section 8.1.14
**Priority:** Medium (user application support)
**Complexity:** High
**Estimated Effort:** 3 days → 1 day (parallel)

**Deliverables:**

1. **UserApplicationInfo struct** in fig_parser.hpp:
   - uint32_t service_id (16 or 32-bit)
   - uint16_t sc_ids (service component ID)
   - uint8_t ca_flag (conditional access)
   - uint16_t ua_type (user application type, 11-bit)
   - uint8_t ua_data_length
   - std::vector<uint8_t> ua_data (application-specific data)
   - bool is_valid

2. **parseFig013_UserApplicationInfo()** implementation:
   - Parse SId flag (16/32-bit)
   - Extract SCIdS (service component ID)
   - Parse UAType (user application type)
   - Extract UA data with length field
   - Support CA flag
   - Emit userApplicationInfoDiscovered signal

3. **test_fig013_user_application.cpp** (20 tests minimum):
   - Valid UA parsing (16/32-bit SId)
   - UAType range validation (0-2047)
   - UA data extraction (variable length)
   - CA flag detection
   - Known UA types (MOT, TPEG, EPG, Journaline)
   - Invalid data handling
   - Performance benchmark (<5µs)

**Acceptance Criteria:**
- 20/20 tests passing (100%)
- Performance: <5µs per parse
- ETSI EN 300 401 Section 8.1.14 compliant
- Zero regressions in existing code

---

### Agent 14: Comprehensive Integration Test Suite

**Mission:** Integrate Agent 10's 70 tests from Week 3 + add new Week 4 tests

**Priority:** High (validation and regression prevention)
**Complexity:** Medium
**Estimated Effort:** 1 day → 1 day (parallel)

**Deliverables:**

1. **Complete Agent 10 Integration** (70 tests from Week 3):
   - Category 1: ETI/FIC Integration (15 tests)
   - Category 2: Cross-FIG Validation (20 tests)
   - Category 3: Performance Profiling (15 tests)
   - Category 4: Edge Cases and Error Handling (10 tests)
   - Category 5: ETSI Compliance Proof (10 tests)

2. **Week 4 Integration Tests** (new):
   - test_fig_week4_comprehensive.cpp (30 tests)
   - FIG 0/3, 0/6, 0/13 cross-validation
   - Packet mode + service linking scenarios
   - User application + data services
   - Complete FIC frame with all Week 1-4 FIGs

3. **CMakeLists.txt Updates**:
   - Add all Week 4 test configurations
   - Update CORE_SOURCES if needed
   - Verify all dependencies

**Acceptance Criteria:**
- 100/100 tests passing (100%)
- All Week 3 Agent 10 tests integrated
- Week 4 comprehensive tests complete
- Zero regressions from Week 1-3

---

## 📋 Implementation Timeline

### Day 1-2: Agent Execution (Parallel)

**Monday, November 4:**
- Launch all 4 agents simultaneously
- Agent 11: FIG 0/3 implementation
- Agent 12: FIG 0/6 implementation
- Agent 13: FIG 0/13 implementation
- Agent 14: Agent 10 + Week 4 integration tests

**Tuesday, November 5:**
- Agents deliver specifications
- Review deliverables for completeness

### Day 3-4: Manual Integration

**Wednesday, November 6:**
- Integrate Agent 11 (FIG 0/3) into fig_parser.cpp/hpp
- Integrate Agent 12 (FIG 0/6) into fig_parser.cpp/hpp
- Build and test FIG 0/3: expect 20/20 (100%)
- Build and test FIG 0/6: expect 20/20 (100%)

**Thursday, November 7:**
- Integrate Agent 13 (FIG 0/13) into fig_parser.cpp/hpp
- Integrate Agent 14 (all integration tests) into CMakeLists.txt
- Build and test FIG 0/13: expect 20/20 (100%)
- Build and test integration suite: expect 100/100 (100%)

### Day 5: Validation & Git Commit

**Friday, November 8:**
- Run complete test suite (285+ → 365+ tests)
- Verify EN 300 401 compliance: 92% → 96%
- Performance profiling across all FIG parsers
- Git commits (one per agent + final documentation)
- Update CLAUDE.md with Week 4 status

---

## ✅ Success Criteria

### Must-Have (Blocking)

1. **Agent Completion:** All 4 agents deliver complete specifications
2. **Code Integration:** All code in actual project files (not /tmp)
3. **Test Pass Rate:** 95%+ across all 365+ tests
4. **Performance:** All parsers <5µs (or within 3x tolerance)
5. **ETSI Compliance:** 100% for FIG 0/3, 0/6, 0/13
6. **Build Status:** Zero compilation errors
7. **Zero Regressions:** All Week 1-3 tests still passing

### Nice-to-Have (Optional)

1. Test pass rate 98%+ (exceeds minimum)
2. Performance <3µs average (exceeds target)
3. Code coverage >85% for new parsers
4. Memory safety validated with Valgrind
5. Thread safety validated with ThreadSanitizer

---

## 📊 Expected Outcomes

### Code Statistics

**Lines of Code (Estimated):**
- Agent 11 (FIG 0/3): ~600 lines (struct + parser + tests)
- Agent 12 (FIG 0/6): ~700 lines (struct + parser + tests)
- Agent 13 (FIG 0/13): ~800 lines (struct + parser + tests)
- Agent 14 (Integration): ~1,200 lines (70 + 30 tests)
- **Total:** ~3,300 lines of production code

**Files Modified:**
- src/core/fig_parser.hpp: +250 lines (3 structs)
- src/core/fig_parser.cpp: +450 lines (3 parsers)
- tests/CMakeLists.txt: +40 lines (4 test configs)
- tests/test_fig003_packet_mode.cpp: 450 lines (NEW)
- tests/test_fig006_service_linking.cpp: 500 lines (NEW)
- tests/test_fig013_user_application.cpp: 550 lines (NEW)
- tests/test_fig_week4_comprehensive.cpp: 1,000 lines (NEW)

### Test Statistics

**Test Count:**
- Current: 285+ tests
- Week 4 Additions: 80+ tests (20 + 20 + 20 + 20 integration)
- Agent 10 Integration: 70 tests
- **Total After Week 4:** 365+ tests

**Expected Pass Rate:** 95%+ (347+/365 tests passing)

### Compliance Impact

**EN 300 401:**
- Before Week 4: 92% (8/13 FIG 0 complete)
- After Week 4: 96% (11/13 FIG 0 complete)
- **Improvement:** +4%

**Overall ETSI Compliance:**
- Before Week 4: 92%
- After Week 4: 96%
- **Target:** 100% (Week 5 goal)

---

## 🚀 PDCA Workflow

### Plan Phase (This Document) ✅

- ✅ Define 4 parallel agents
- ✅ Specify deliverables and acceptance criteria
- ✅ Establish timeline (5 days)
- ✅ Document success criteria

### Do Phase (Week 4 Execution)

**To Be Executed:**
- Launch 4 agents in parallel
- Monitor agent progress
- Collect deliverables

### Check Phase (Week 4 Testing)

**To Be Validated:**
- Verify 365+ tests pass at 95%+
- Validate ETSI EN 300 401 compliance 100%
- Confirm performance targets met
- Check zero regressions

### Act Phase (Week 4 Completion)

**To Be Completed:**
- Git commits (4 feature commits + 1 doc commit)
- Update CLAUDE.md with Week 4 status
- Update ETSI_COMPLIANCE_IMPLEMENTATION_PLAN.md
- Generate Week 4 final report
- Plan Week 5 (final push to 100%)

---

## 📋 Risk Assessment

### Low Risk

- **Multi-Agent Proven:** Weeks 1-3 validated 77.5% time savings
- **Incremental Approach:** Adding to existing stable codebase
- **Clear Specifications:** ETSI standards well-defined
- **Known Patterns:** FIG parsing patterns established

### Medium Risk

- **FIG 0/13 Complexity:** High complexity, may take longer
- **Integration Testing:** 70 Agent 10 tests pending validation
- **Performance Variance:** FIG 0/5 slower than target (acceptable)

### Mitigation

- **Time Buffer:** 5-day timeline allows flexibility
- **Incremental Testing:** Test each agent as integrated
- **Manual Review:** PM reviews all code before commit
- **Regression Suite:** Run all 285+ existing tests after each agent

---

## 📌 Week 5 Preview

**Remaining for 100% Compliance:**
- FIG 0/8: Service component global definition (Medium)
- FIG 0/9: Country/LTO/International table (Low)
- Optional FIG Type 1 extensions: 1/2, 1/3, 1/6
- Optional FIG Type 2 extensions: 2/0, 2/1, 2/3

**Expected Timeline:** Week 5 (November 11-15, 2025)
**Expected Outcome:** EN 300 401 100% compliance

---

## ✅ Week 4 Approval

**Status:** ✅ READY TO EXECUTE
**Approval Date:** October 30, 2025
**PM Agent:** Approved
**Next Step:** Launch 4 parallel agents on November 4, 2025

---

**Document Version:** 1.0
**Last Updated:** October 30, 2025
**Next Review:** November 8, 2025 (Week 4 completion)
