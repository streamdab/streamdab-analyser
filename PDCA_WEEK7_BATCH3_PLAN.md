# PDCA WEEK 7 BATCH 3 PLAN - Parallel Implementation of FIG 0/7, 0/11, 0/21

**Date:** November 3, 2025
**Goal:** Implement 3 FIG types in parallel to reach 82% ETSI compliance
**Methodology:** PDCA Multi-Agent Parallel Development (3 agents)
**Duration:** 4-6 hours (estimated)

---

## 🎯 OBJECTIVE

**Goal:** Implement FIG 0/7 (Service Component Stream Mode), FIG 0/11 (Region Definition), and FIG 0/21 (Frequency Information) in parallel

**Current State:** 15/22 types (68% compliance)
**Target State:** 18/22 types (82% compliance)

**Success Criteria:**
- ✅ All 3 FIG types fully implemented (struct, parser, switch case, signal)
- ✅ Comprehensive test suites (10-15 tests each)
- ✅ >90% test pass rate for each
- ✅ Performance <15µs per parse
- ✅ Zero compilation errors
- ✅ ETSI standard compliance (100% per spec)

---

## 📊 CURRENT STATUS

**Completed in Week 7:**
- **Batch 1:** FIG 0/10 (Date/Time), FIG 0/18 (Announcement Support) ✅
  - Result: 47 tests, 87% pass rate
  - Compliance: 55% → 64%

- **Batch 2:** FIG 0/19 (Announcement Switching) ✅
  - Result: 22 tests, 100% pass rate
  - Compliance: 64% → 68%

**Remaining for 100%:**
- FIG 0/7: Service component in stream mode
- FIG 0/11: Region definition ← **Batch 3**
- FIG 0/12: MPEG-2 TS streaming
- FIG 0/15: Programme number
- FIG 0/16: Programme type international
- FIG 0/20: Service component information
- FIG 0/21: Frequency information ← **Batch 3**

---

## 🚀 BATCH 3 STRATEGY

### **Agent 23: FIG 0/7 - Service Component Stream Mode**

**ETSI Standard:** EN 300 401 Section 8.1.4
**Priority:** MEDIUM (complements existing 0/3 packet mode)
**Complexity:** MEDIUM (similar to FIG 0/3 structure)
**Target Tests:** 12+ tests

**Deliverables:**
1. **Struct:** `ServiceComponentStreamMode` in fig_parser.hpp
   - Fields: service_component_id, sub_ch_id, ca_flag, dg_flag, is_valid
   - Helper methods: isCaProtected(), hasDataGroups()

2. **Parser:** `parseFig007_ServiceComponentStreamMode()` in fig_parser.cpp
   - Extract 12-bit Service Component ID (SCId)
   - Extract 6-bit SubChId
   - Parse CA flag (1-bit)
   - Parse DG flag (1-bit)
   - Validate ranges

3. **Switch Case:** `case 7` in parseFigType0()

4. **Signal:** `serviceComponentStreamModeDiscovered()`

5. **Test Suite:** tests/test_fig007_stream_mode.cpp
   - Basic parsing (3 tests)
   - CA flag handling (2 tests)
   - DG flag handling (2 tests)
   - Multiple service components (2 tests)
   - Edge cases (2 tests)
   - Performance (1 test)

**ETSI Byte Structure:**
```
Byte 0: Extension (type 0/7)
Byte 1-2: [SCId 12-bit][CA 1-bit][DG 1-bit][RFA 2-bit]
Byte 3: [SubChId 6-bit][RFA 2-bit]
```

**Expected Time:** 2 hours

---

### **Agent 24: FIG 0/11 - Region Definition**

**ETSI Standard:** EN 300 401 Section 8.1.9
**Priority:** LOW (regional services support)
**Complexity:** LOW (simple ID and flag parsing)
**Target Tests:** 10+ tests

**Deliverables:**
1. **Struct:** `RegionDefinition` in fig_parser.hpp
   - Fields: region_id, region_flag, sub_ch_id, ensemble_id, is_valid
   - Helper methods: isActive(), getRegionScope()

2. **Parser:** `parseFig011_RegionDefinition()` in fig_parser.cpp
   - Extract 8-bit Region ID
   - Parse Region flag (1-bit)
   - Extract associated SubChId or Ensemble ID
   - Validate ranges

3. **Switch Case:** `case 11` in parseFigType0()

4. **Signal:** `regionDefinitionDiscovered()`

5. **Test Suite:** tests/test_fig011_region_definition.cpp
   - Basic parsing (3 tests)
   - Region ID extraction (2 tests)
   - Region scope (2 tests)
   - Edge cases (2 tests)
   - Performance (1 test)

**ETSI Byte Structure:**
```
Byte 0: Extension (type 0/11)
Byte 1: Region ID (8-bit)
Byte 2: [Region flag 1-bit][...SubChId or EId...]
```

**Expected Time:** 1.5 hours

---

### **Agent 25: FIG 0/21 - Frequency Information**

**ETSI Standard:** EN 300 401 Section 8.1.16
**Priority:** MEDIUM (alternate frequency support)
**Complexity:** MEDIUM (frequency list parsing)
**Target Tests:** 12+ tests

**Deliverables:**
1. **Struct:** `FrequencyInformation` in fig_parser.hpp
   - Fields: region_id, frequency_list (vector<uint32_t>), length_indicator, is_valid
   - Helper methods: getFrequencyCount(), getFrequencyAt(index)

2. **Parser:** `parseFig021_FrequencyInformation()` in fig_parser.cpp
   - Extract Region ID (if present)
   - Parse Length Indicator (LI)
   - Extract frequency list (variable length)
   - Convert frequency encoding to Hz
   - Validate ranges (174-240 MHz for Band III)

3. **Switch Case:** `case 21` in parseFigType0()

4. **Signal:** `frequencyInformationDiscovered()`

5. **Test Suite:** tests/test_fig021_frequency_information.cpp
   - Basic parsing (3 tests)
   - Single frequency (2 tests)
   - Multiple frequencies (2 tests)
   - Frequency conversion (2 tests)
   - Edge cases (2 tests)
   - Performance (1 test)

**ETSI Byte Structure:**
```
Byte 0: Extension (type 0/21)
Byte 1: [Region ID 8-bit] (optional)
Byte 2: [Length Indicator LI]
Byte 3-N: [Frequency list, 16-bit per frequency]
```

**Expected Time:** 2 hours

---

## 📋 PARALLEL EXECUTION PLAN

### **Phase 1: Launch All 3 Agents (Simultaneous)**

**Agent 23 Task:**
```
Implement FIG 0/7 (Service Component Stream Mode) per ETSI EN 300 401 Section 8.1.4.

Requirements:
1. Add ServiceComponentStreamMode struct to src/core/fig_parser.hpp
2. Implement parseFig007_ServiceComponentStreamMode() in src/core/fig_parser.cpp
3. Add case 7 to parseFigType0() switch statement
4. Add serviceComponentStreamModeDiscovered() signal
5. Create tests/test_fig007_stream_mode.cpp with 12+ tests
6. Update tests/CMakeLists.txt

Struct fields: service_component_id (12-bit), sub_ch_id (6-bit), ca_flag (bool), dg_flag (bool), is_valid (bool)

Target: 100% ETSI compliance, >90% test pass rate, <15µs performance
```

**Agent 24 Task:**
```
Implement FIG 0/11 (Region Definition) per ETSI EN 300 401 Section 8.1.9.

Requirements:
1. Add RegionDefinition struct to src/core/fig_parser.hpp
2. Implement parseFig011_RegionDefinition() in src/core/fig_parser.cpp
3. Add case 11 to parseFigType0() switch statement
4. Add regionDefinitionDiscovered() signal
5. Create tests/test_fig011_region_definition.cpp with 10+ tests
6. Update tests/CMakeLists.txt

Struct fields: region_id (8-bit), region_flag (bool), sub_ch_id (6-bit), ensemble_id (16-bit), is_valid (bool)

Target: 100% ETSI compliance, >90% test pass rate, <15µs performance
```

**Agent 25 Task:**
```
Implement FIG 0/21 (Frequency Information) per ETSI EN 300 401 Section 8.1.16.

Requirements:
1. Add FrequencyInformation struct to src/core/fig_parser.hpp
2. Implement parseFig021_FrequencyInformation() in src/core/fig_parser.cpp
3. Add case 21 to parseFigType0() switch statement
4. Add frequencyInformationDiscovered() signal
5. Create tests/test_fig021_frequency_information.cpp with 12+ tests
6. Update tests/CMakeLists.txt

Struct fields: region_id (8-bit), frequency_list (vector<uint32_t>), length_indicator (8-bit), is_valid (bool)

Target: 100% ETSI compliance, >90% test pass rate, <15µs performance
```

### **Phase 2: Build and Test Verification**

After all 3 agents complete:
1. Build all 3 test executables
2. Run all 3 test suites
3. Verify 34+ total new tests
4. Verify >90% pass rate for each
5. Verify zero compilation errors

### **Phase 3: Integration and Commit**

1. Create PDCA_WEEK7_BATCH3_COMPLETE.md report
2. Commit all changes to git
3. Update ETSI compliance status (68% → 82%)

---

## ✅ ACCEPTANCE CRITERIA

### **Per-Agent Criteria:**

| **Criterion** | **Target** | **Agent 23** | **Agent 24** | **Agent 25** |
|---------------|-----------|-------------|-------------|-------------|
| Struct defined | Yes | ⏳ | ⏳ | ⏳ |
| Parser implemented | Yes | ⏳ | ⏳ | ⏳ |
| Switch case added | Yes | ⏳ | ⏳ | ⏳ |
| Signal emitted | Yes | ⏳ | ⏳ | ⏳ |
| Tests created | 10-15 | ⏳ | ⏳ | ⏳ |
| Test pass rate | >90% | ⏳ | ⏳ | ⏳ |
| Performance | <15µs | ⏳ | ⏳ | ⏳ |
| Build errors | 0 | ⏳ | ⏳ | ⏳ |
| ETSI compliance | 100% | ⏳ | ⏳ | ⏳ |

### **Batch 3 Overall Criteria:**

| **Criterion** | **Target** | **Status** |
|---------------|-----------|-----------|
| All 3 types implemented | Yes | ⏳ |
| Total tests added | 34+ | ⏳ |
| Combined pass rate | >90% | ⏳ |
| Zero compilation errors | Yes | ⏳ |
| ETSI compliance gain | +3 types | ⏳ |
| Time efficiency | <6 hours | ⏳ |

---

## 📊 EXPECTED RESULTS

### **Test Count Projections:**

| **Agent** | **Tests Added** | **Pass Rate** |
|-----------|----------------|--------------|
| Agent 23 (0/7) | 12 tests | >90% |
| Agent 24 (0/11) | 10 tests | >90% |
| Agent 25 (0/21) | 12 tests | >90% |
| **Total** | **34 tests** | **>90%** |

### **ETSI Compliance Progress:**

```
Before Batch 3: ███████████████░░░░░░░ 68% (15/22 types)
After Batch 3:  ███████████████████░░░░ 82% (18/22 types)
Target:         ████████████████████████ 100% (22/22 types)
```

**Remaining for 100%:** FIG 0/12, 0/15, 0/16, 0/20 (4 types)

### **Time Estimates:**

| **Phase** | **Sequential** | **Parallel** | **Savings** |
|-----------|---------------|-------------|-------------|
| Agent 23 (0/7) | 2h | | |
| Agent 24 (0/11) | 1.5h | | |
| Agent 25 (0/21) | 2h | | |
| **Total Implementation** | **5.5h** | **2h** | **64%** |
| Build & Test | 0.5h | 0.5h | 0% |
| Documentation | 0.5h | 0.5h | 0% |
| **Grand Total** | **6.5h** | **3h** | **54%** |

---

## 🔧 TECHNICAL SPECIFICATIONS

### **Code Pattern Consistency**

**Switch Case Pattern:**
```cpp
case X: { // FIG 0/X - Description (PDCA Week 7 - Batch 3 / Agent Y)
    auto result = parseFig00X_Name(fig_block.data);
    if (result.is_valid) {
        emit signalNameDiscovered(result);
        success = true;
    }
    break;
}
```

**Parser Method Pattern:**
```cpp
StructName FigParser::parseFig00X_Name(const std::vector<uint8_t>& fig_data) {
    StructName result;
    result.is_valid = false;

    // Minimum size check
    if (fig_data.size() < min_bytes) {
        Logger::instance().log(Logger::Warning, "FigParser",
            QString("FIG 0/X: Insufficient data - got %1 bytes, need at least %2")
            .arg(fig_data.size()).arg(min_bytes));
        return result;
    }

    try {
        size_t offset = 1; // Skip extension field

        // Parse fields...

        result.is_valid = true;

        Logger::instance().log(Logger::Debug, "FigParser",
            QString("Parsed FIG 0/X - Field1=%1, Field2=%2")
            .arg(result.field1).arg(result.field2));

        emit signalNameDiscovered(result);

    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "FigParser",
            QString("Exception parsing FIG 0/X: %1").arg(e.what()));
        result.is_valid = false;
    }

    return result;
}
```

---

## 📚 DOCUMENTATION DELIVERABLES

**Batch 3 Documentation:**
1. PDCA_WEEK7_BATCH3_PLAN.md (this document)
2. PDCA_WEEK7_BATCH3_COMPLETE.md (after completion)
3. Test suites: test_fig007_stream_mode.cpp, test_fig011_region_definition.cpp, test_fig021_frequency_information.cpp

**Updated Files:**
- src/core/fig_parser.hpp (3 structs, 3 methods, 3 signals)
- src/core/fig_parser.cpp (3 parsers, 3 switch cases)
- tests/CMakeLists.txt (3 test executable configurations)

---

## 🎯 SUCCESS METRICS

**Minimum Success:**
- ✅ All 3 FIG types implemented and tested
- ✅ 82% ETSI compliance (18/22 types)
- ✅ >30 new tests passing (>90% each)
- ✅ Zero compilation errors
- ✅ Time <6 hours

**Full Success:**
- ✅ All minimum criteria met
- ✅ 100% test pass rate (no failures)
- ✅ Performance <10µs (exceed target)
- ✅ Time <4 hours (parallel efficiency)

---

## 🚦 RISK ASSESSMENT

**Low Risk:**
- FIG 0/11 - Simple structure, low complexity
- All agents independent (no blocking dependencies)

**Medium Risk:**
- FIG 0/7 - Similar to 0/3, may need careful CA/DG flag handling
- FIG 0/21 - Variable-length frequency list parsing

**Mitigation:**
- Incremental testing after each agent completion
- ETSI standard verification before integration
- Code review for complex parsers

---

## ✅ NEXT STEPS

1. ⏳ Launch Agent 23 (FIG 0/7)
2. ⏳ Launch Agent 24 (FIG 0/11)
3. ⏳ Launch Agent 25 (FIG 0/21)
4. ⏳ Build and test all 3 implementations
5. ⏳ Create Batch 3 completion report
6. ⏳ Commit to git

**Status:** Ready to commence PDCA Week 7 Batch 3 - Parallel Implementation

---

**Document Version:** 1.0
**Author:** PDCA Multi-Agent Planning Framework
**Date:** November 3, 2025
