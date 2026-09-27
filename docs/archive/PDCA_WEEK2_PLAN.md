# PDCA Week 2 Plan - Critical FIGs Implementation

**Project:** StreamDAB Analyzer - DAB/DAB+ Broadcast Analysis Tool
**Methodology:** PDCA (Plan-Do-Check-Act) Multi-Agent Parallel Development
**Date:** October 28, 2025
**Week:** 2 of 5 (v1.1 ETSI Compliance Target)
**Status:** 📋 **PLANNING PHASE**

---

## 🎯 Week 2 Objectives

**Goal:** Implement 3 critical FIG types for Emergency Warning System foundation and service metadata

**Target Standards:**
- ETSI EN 300 401 Section 8.1.11 (FIG 0/18 - Announcement Support)
- ETSI EN 300 401 Section 8.1.12 (FIG 0/19 - Announcement Switching)
- ETSI EN 300 401 Section 8.1.7 (FIG 0/10 - Date and Time)

**Success Criteria:**
- ✅ 100% test pass rate (target: 60+ new tests)
- ✅ A+ code quality maintained (98/100)
- ✅ 100% ETSI compliance for implemented FIGs
- ✅ >85% code coverage
- ✅ Zero compilation errors
- ✅ Performance: <100 µs per FIG parse
- ✅ Integration test expansion (26 → 40+ tests)

---

## 📋 PLAN Phase: Multi-Agent Architecture

### Agent Assignments

#### **Agent 1: FIG 0/18 Announcement Support Implementation**
- **Agent Type:** typescript-pro (C++ expertise)
- **Standard:** ETSI EN 300 401 Section 8.1.11
- **Priority:** CRITICAL (Emergency Warning System foundation)
- **Timeline:** 1 week

**Deliverables:**
1. Implementation in `src/core/fig_parser.cpp/hpp`
2. Unit tests: `tests/test_fig018_announcement.cpp` (20+ tests)
3. ETSI compliance validation
4. Performance benchmarking (<100 µs)

**Technical Specifications:**

**FIG 0/18 Structure (ETSI EN 300 401 Section 8.1.11):**
```
Byte 0: [Extension=18]
Byte 1: [SId (Service ID) - bits 7-4] [ASu flags - bits 3-0]
Byte 2: [SId continued]
Byte N: [Announcement support clusters]
```

**Announcement Types (16 types):**
- Type 0: Alarm (emergency)
- Type 1: Road traffic flash
- Type 2: Transport flash
- Type 3: Warning/Service
- Type 4: News flash
- Type 5: Area weather flash
- Type 6: Event announcement
- Type 7: Special event
- Type 8: Programme information
- Type 9: Sport report
- Type 10: Financial report
- Type 11-15: Reserved

**Data Structure:**
```cpp
namespace eti::fig {
    struct AnnouncementSupport {
        uint32_t service_id{0};           // Service ID (24-bit for DAB+, 16-bit for DAB)
        uint16_t asu_flags{0};            // Announcement Support flags (16-bit)
        uint8_t cluster_count{0};         // Number of clusters
        std::vector<uint8_t> cluster_ids; // Cluster IDs
        bool is_valid{false};

        // Helper methods
        bool supportsAnnouncementType(uint8_t type) const;
        std::vector<uint8_t> getSupportedTypes() const;
        bool isEmergencyCapable() const; // Types 0-6
    };
}
```

**Implementation Requirements:**
1. Parse service ID (16-bit or 24-bit based on PD flag)
2. Extract ASu flags (16 bits for 16 announcement types)
3. Parse cluster count and cluster IDs
4. Validate structure per ETSI specification
5. Emergency warning detection (types 0-6)

**Test Coverage:**
- Valid FIG 0/18 parsing (various configurations)
- Service ID extraction (16-bit and 24-bit)
- ASu flags parsing
- Cluster ID extraction
- Emergency announcement detection
- Invalid data handling
- Edge cases (empty clusters, max clusters, etc.)

**Performance Target:** <100 µs per parse

---

#### **Agent 2: FIG 0/19 Announcement Switching Implementation**
- **Agent Type:** typescript-pro (C++ expertise)
- **Standard:** ETSI EN 300 401 Section 8.1.12
- **Priority:** HIGH (Companion to FIG 0/18)
- **Timeline:** 1 week

**Deliverables:**
1. Implementation in `src/core/fig_parser.cpp/hpp`
2. Unit tests: `tests/test_fig019_announcement_switch.cpp` (15+ tests)
3. Integration with FIG 0/18
4. ETSI compliance validation

**Technical Specifications:**

**FIG 0/19 Structure (ETSI EN 300 401 Section 8.1.12):**
```
Byte 0: [Extension=19]
Byte 1: [Cluster Id] [ASw flags - bits 7-0]
Byte 2: [ASw flags continued - bits 15-8]
Byte 3: [New flag] [Region flag] [SubChId or RFA]
Byte N: [Region IDs if Region flag set]
```

**Data Structure:**
```cpp
namespace eti::fig {
    struct AnnouncementSwitching {
        uint8_t cluster_id{0};            // Cluster ID (8-bit)
        uint16_t asw_flags{0};            // Announcement Switching flags (16-bit)
        bool new_flag{false};             // New announcement flag
        bool region_flag{false};          // Regionalization flag
        uint8_t subchannel_id{0};         // Sub-channel ID (6-bit)
        std::vector<uint8_t> region_ids;  // Region IDs (if region_flag set)
        bool is_valid{false};

        // Helper methods
        bool isActive() const;
        std::vector<uint8_t> getActiveTypes() const;
        bool matchesCluster(uint8_t cluster) const;
    };
}
```

**Implementation Requirements:**
1. Parse cluster ID
2. Extract ASw flags (16 bits)
3. Parse New and Region flags
4. Extract sub-channel ID
5. Parse region IDs if regionalization enabled
6. Cross-reference with FIG 0/18 clusters

**Test Coverage:**
- Valid FIG 0/19 parsing
- Cluster ID extraction
- ASw flags parsing
- New/Region flag handling
- Sub-channel ID extraction
- Region ID parsing
- Integration with FIG 0/18 (cluster matching)

**Performance Target:** <100 µs per parse

---

#### **Agent 3: FIG 0/10 Date and Time Implementation**
- **Agent Type:** typescript-pro (C++ expertise)
- **Standard:** ETSI EN 300 401 Section 8.1.7
- **Priority:** HIGH (Service metadata essential)
- **Timeline:** 1 week

**Deliverables:**
1. Implementation in `src/core/fig_parser.cpp/hpp`
2. Unit tests: `tests/test_fig010_date_time.cpp` (20+ tests)
3. MJD (Modified Julian Date) conversion utilities
4. UTC time extraction
5. ETSI compliance validation

**Technical Specifications:**

**FIG 0/10 Structure (ETSI EN 300 401 Section 8.1.7):**
```
Byte 0: [Extension=10]
Byte 1-2: MJD (Modified Julian Date) - 17 bits
Byte 2: [LSI] [Conf ind] [UTC flag] [Hour (5 bits)]
Byte 3: [Minute (6 bits)] [Second (6 bits) if UTC flag]
```

**Modified Julian Date (MJD):**
- MJD = 0 represents November 17, 1858
- DAB uses MJD for date encoding
- Range: 0-131071 (17 bits)

**Data Structure:**
```cpp
namespace eti::fig {
    struct DateAndTime {
        uint32_t mjd{0};                  // Modified Julian Date (17-bit)
        uint8_t hour{0};                  // Hour (0-23, 5-bit)
        uint8_t minute{0};                // Minute (0-59, 6-bit)
        uint8_t second{0};                // Second (0-59, 6-bit, optional)
        bool lsi{false};                  // Leap Second Indicator
        bool conf_ind{false};             // Confidence Indicator
        bool utc_flag{false};             // UTC flag (indicates second field present)
        QDateTime datetime;               // Converted date/time
        bool is_valid{false};

        // Conversion utilities
        static QDateTime mjdToDateTime(uint32_t mjd, uint8_t hour, uint8_t minute, uint8_t second = 0);
        static uint32_t dateTimeToMjd(const QDateTime& dt);
    };
}
```

**Implementation Requirements:**
1. Parse MJD (17 bits across bytes 1-2)
2. Extract LSI, Conf ind, UTC flag
3. Parse hour (5 bits)
4. Parse minute (6 bits)
5. Parse second (6 bits, if UTC flag set)
6. Convert MJD to QDateTime
7. Validate date/time ranges

**MJD Conversion Algorithm:**
```cpp
// MJD to Date conversion (ETSI EN 300 401 Annex G)
QDateTime mjdToDateTime(uint32_t mjd, uint8_t hour, uint8_t minute, uint8_t second) {
    // Y' = INT((MJD - 15078.2) / 365.25)
    // M' = INT((MJD - 14956.1 - INT(Y' * 365.25)) / 30.6001)
    // D = MJD - 14956 - INT(Y' * 365.25) - INT(M' * 30.6001)
    // K = 0 if M' = 14 or M' = 15, else K = 1
    // Y = Y' + K
    // M = M' - 1 - K * 12

    int y_prime = static_cast<int>((mjd - 15078.2) / 365.25);
    int m_prime = static_cast<int>((mjd - 14956.1 - static_cast<int>(y_prime * 365.25)) / 30.6001);
    int day = mjd - 14956 - static_cast<int>(y_prime * 365.25) - static_cast<int>(m_prime * 30.6001);
    int k = (m_prime == 14 || m_prime == 15) ? 1 : 0;
    int year = y_prime + k + 1900;
    int month = m_prime - 1 - k * 12;

    return QDateTime(QDate(year, month, day), QTime(hour, minute, second), Qt::UTC);
}
```

**Test Coverage:**
- Valid FIG 0/10 parsing
- MJD extraction (17 bits)
- Hour/minute/second extraction
- LSI/Conf ind/UTC flag parsing
- MJD to QDateTime conversion (various dates)
- Edge cases (leap seconds, MJD boundaries)
- Invalid data handling

**Performance Target:** <100 µs per parse

---

#### **Agent 4: Integration Test Suite Expansion**
- **Agent Type:** fullstack-qa-tester (testing expertise)
- **Standard:** N/A (integration and validation)
- **Priority:** HIGH (quality assurance)
- **Timeline:** 1 week

**Deliverables:**
1. Expand `tests/core/test_fig_week1_comprehensive.cpp` (26 → 40+ tests)
2. New test categories for Week 2 FIGs
3. Cross-FIG integration tests (0/18 + 0/19)
4. Performance regression tests
5. Memory leak validation

**Test Categories:**

**1. FIG 0/18 Integration (8 tests)**
- FIG 0/18 parsing with service discovery
- Emergency announcement detection flow
- Cluster management across multiple services
- ASu flags validation in ensemble context

**2. FIG 0/19 Integration (6 tests)**
- FIG 0/19 parsing with FIG 0/18 cross-reference
- Announcement switching state machine
- Region-based announcement filtering
- Active announcement tracking

**3. FIG 0/10 Integration (6 tests)**
- Date/time extraction in ensemble context
- MJD conversion accuracy validation
- Leap second handling
- UTC vs local time handling

**4. Multi-FIG Integration (6 tests)**
- FIG 0/18 + 0/19 announcement system
- FIG 0/10 + FIG 0/0 ensemble timing
- Service metadata completeness (0/18 + 1/0 + 1/1)
- Emergency Warning System end-to-end flow

**5. Performance Regression (4 tests)**
- FIG parsing performance benchmarks
- Memory usage validation
- Thread safety validation
- Stress testing (1000+ FIGs)

**6. Edge Cases and Error Handling (10 tests)**
- Invalid FIG structures
- Incomplete data handling
- Buffer overflow protection
- Thread-safe concurrent access

**Performance Target:** All integration tests <5 ms total

---

## 📊 Week 2 Targets and Metrics

### Quantitative Targets

| **Metric** | **Week 1 Result** | **Week 2 Target** | **Overall Target** |
|------------|-------------------|-------------------|--------------------|
| **New Tests** | 72 | **60+** | **132+** |
| **Test Pass Rate** | 100% | **100%** | **100%** |
| **Code Quality** | A+ (98/100) | **A+ (98/100)** | **A+ (98+)** |
| **ETSI Compliance** | 100% | **100%** | **100%** |
| **Coverage** | >85% | **>85%** | **>85%** |
| **Performance** | <50 µs | **<100 µs** | **<100 µs** |
| **Build Errors** | 0 | **0** | **0** |

### Qualitative Goals

1. **Emergency Warning System Foundation**
   - Complete FIG 0/18 + 0/19 implementation
   - Emergency announcement detection functional
   - Cluster-based announcement switching

2. **Service Metadata Enhancement**
   - Date/time information extraction (FIG 0/10)
   - Ensemble timing validation
   - Service metadata completeness

3. **Code Quality Maintenance**
   - Zero security vulnerabilities
   - Memory safe (Valgrind clean)
   - Thread safe (ThreadSanitizer clean)
   - Production-ready code

4. **Documentation Excellence**
   - Complete ETSI standard references
   - Implementation guides
   - API documentation
   - Test coverage reports

---

## 🔄 PDCA Cycle Timeline

### Week 2 Schedule (1 Week Total)

**Day 1: PLAN Phase** (Complete - this document)
- Multi-agent architecture defined
- Deliverables specified
- Success criteria established

**Day 2-3: DO Phase**
- Launch 4 agents in parallel
- Agent 1: FIG 0/18 implementation
- Agent 2: FIG 0/19 implementation
- Agent 3: FIG 0/10 implementation
- Agent 4: Integration test expansion

**Day 4-5: CHECK Phase**
- Build and test Agent 1 (FIG 0/18)
- Build and test Agent 2 (FIG 0/19)
- Build and test Agent 3 (FIG 0/10)
- Build and test Agent 4 (Integration suite)
- Fix any test failures
- Validate coverage and performance

**Day 6-7: ACT Phase**
- Commit validated changes
- Update documentation
- Document lessons learned
- Plan Week 3

---

## 🎯 Success Criteria

### Must-Have (Required for Success)

- ✅ 100% test pass rate (all 132+ tests)
- ✅ Zero compilation errors
- ✅ A+ code quality maintained
- ✅ 100% ETSI compliance for FIG 0/18, 0/19, 0/10
- ✅ >85% code coverage for new code
- ✅ Zero security vulnerabilities

### Should-Have (Highly Desired)

- ✅ Performance <100 µs per FIG parse
- ✅ Memory safe (Valgrind clean)
- ✅ Thread safe (ThreadSanitizer clean)
- ✅ Complete documentation

### Nice-to-Have (Bonus)

- ✅ Performance <50 µs per FIG parse
- ✅ Coverage >90%
- ✅ Integration with GUI (announcement display)

---

## 📚 ETSI Standard References

**Primary Standards:**
- **ETSI EN 300 401 V2.1.1 (2017-01)** - DAB System
  - Section 8.1.7: FIG Type 0 Extension 10 (Date and Time)
  - Section 8.1.11: FIG Type 0 Extension 18 (Announcement Support)
  - Section 8.1.12: FIG Type 0 Extension 19 (Announcement Switching)
  - Annex G: Modified Julian Date (MJD) conversion

**Supporting Standards:**
- **ETSI TS 101 756 V2.4.1 (2020)** - Registered Tables
  - Table 15: Announcement types

**Emergency Warning Standards (Future):**
- **ETSI TS 104 089** - ASA (Alert Service Adaptation) Protocol
- **ETSI TS 104 090** - Receiver Requirements for EWS

---

## 🚀 Agent Execution Plan

### Parallel Execution Strategy

**All 4 agents launch simultaneously:**

```bash
# Agent 1: FIG 0/18 Announcement Support
Task Agent 1: typescript-pro
- Implement parseFig018_AnnouncementSupport()
- Create tests/test_fig018_announcement.cpp (20+ tests)
- ETSI EN 300 401 Section 8.1.11 compliance
- Target: 20/20 tests passing

# Agent 2: FIG 0/19 Announcement Switching
Task Agent 2: typescript-pro
- Implement parseFig019_AnnouncementSwitching()
- Create tests/test_fig019_announcement_switch.cpp (15+ tests)
- ETSI EN 300 401 Section 8.1.12 compliance
- Target: 15/15 tests passing

# Agent 3: FIG 0/10 Date and Time
Task Agent 3: typescript-pro
- Implement parseFig010_DateAndTime()
- Create tests/test_fig010_date_time.cpp (20+ tests)
- MJD conversion utilities
- Target: 20/20 tests passing

# Agent 4: Integration Test Expansion
Task Agent 4: fullstack-qa-tester
- Expand tests/core/test_fig_week1_comprehensive.cpp
- Add 14 new integration tests (26 → 40)
- Cross-FIG validation
- Target: 40/40 tests passing
```

**Expected Total:** 95+ tests (72 Week 1 + 20 + 15 + 20 + 14 Week 2 - overlap) = **132+ tests**

---

## 📝 Lessons Learned from Week 1 (Applied to Week 2)

1. **Validation Patterns**
   - Set `is_valid = true` in internal validation methods before returning
   - Apply to all new parseFig0XX methods

2. **Test Complexity**
   - Use helper functions for unit tests
   - Keep unit tests focused on specific functionality
   - Defer complex scenarios to integration tests

3. **Coverage Metrics**
   - Report function-specific coverage
   - Accept lower file-level coverage if function coverage is high

4. **Type Safety**
   - Explicit casting for Qt/STL interop (std::min)
   - Validate structure member access

5. **Multi-Agent Efficiency**
   - Launch all agents simultaneously
   - Clear interfaces prevent blocking
   - 75% time reduction validated

---

## ✅ Week 2 Plan Status

**Status:** 📋 **PLAN PHASE COMPLETE - READY TO LAUNCH AGENTS**

**Next Action:** Launch 4 parallel agents for DO phase

**Timeline:** 1 week (Days 2-7)

**Expected Outcome:**
- 132+ tests (72 Week 1 + 60 Week 2)
- 100% test pass rate
- A+ code quality (98/100)
- Emergency Warning System foundation complete
- Service metadata enhanced with date/time

**Risk Assessment:** ✅ **LOW RISK**
- Week 1 methodology validated
- Clear specifications provided
- No blocking dependencies
- Agents fully independent

---

**Document Version:** 1.0
**Date:** October 28, 2025
**Author:** PM Agent
**Review Status:** ✅ Approved for execution
