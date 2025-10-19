# Phase 2A Week 6: FIG Type 0 Advanced Features Implementation Plan

**Document Version:** 1.0
**Date:** October 19, 2025
**Phase:** Phase 2A Week 6 - FIG Type 0 Advanced (0/13, 0/14, 0/17, 0/18, 0/19)
**Status:** Planning Complete, Ready for Implementation
**Timeline:** 1 week

---

## Executive Summary

Phase 2A Week 6 implements 5 critical FIG Type 0 extensions that provide advanced DAB features including user application discovery, FEC organization, programme type classification, and **Emergency Warning System (EWS)** announcement handling.

**Key Milestone:** FIG 0/19 (Announcement switching) is **EWS CRITICAL** for Thai NBTC emergency broadcast compliance.

---

## Implementation Scope

### **FIG Types to Implement:**

| FIG Type | Description | ETSI Reference | Priority | EWS Critical |
|----------|-------------|----------------|----------|--------------|
| **FIG 0/13** | User application information | EN 300 401 §6.3.6 | HIGH | No |
| **FIG 0/14** | FEC sub-channel organization | EN 300 401 §6.2.2 | MEDIUM | No |
| **FIG 0/17** | Programme Type (PTy) | EN 300 401 §8.1.5 | HIGH | No |
| **FIG 0/18** | Announcement support | EN 300 401 §8.1.6.1 | HIGH | Yes |
| **FIG 0/19** | Announcement switching | EN 300 401 §8.1.6.2 | **CRITICAL** | **YES** |

---

## ODR-DabMux Reference Files

**Base Path:** `/home/seksan/workspace/ODR-DabMux/src/fig/`

- `FIG0_13.cpp` - User application information encoding
- `FIG0_14.cpp` - FEC sub-channel organization encoding
- `FIG0_17.cpp` - Programme Type encoding
- `FIG0_18.cpp` - Announcement support encoding
- `FIG0_19.cpp` - Announcement switching encoding

**Reference Strategy:**
- Study ODR-DabMux **encoder** implementations
- Reverse-engineer for **parser** (analyzer) use case
- Maintain compatibility with existing FIG processing pipeline
- Follow ETSI EN 300 401 specifications precisely

---

## Detailed Implementation Specifications

### **FIG 0/13: User Application Information**

**ETSI Reference:** EN 300 401 Section 6.3.6
**Purpose:** Indicates user applications carried by service components (e.g., MOT Slideshow, SPI, EPG)

**Structure Definition:**
```cpp
struct FIG0_13_UserApp {
    uint16_t service_id;           // Service carrying the application
    uint8_t component_id;          // Service component identifier
    uint16_t user_app_type;        // Application type code
    std::vector<uint8_t> user_app_data;  // Application-specific data
};

// Common User Application Types (ETSI EN 300 401 Table 18)
constexpr uint16_t USER_APP_NOT_USED = 0x000;
constexpr uint16_t USER_APP_SPI = 0x001;           // Service Programme Information
constexpr uint16_t USER_APP_MOT_SLIDESHOW = 0x002; // MOT Slideshow
constexpr uint16_t USER_APP_MOT_BWS = 0x003;       // MOT Broadcast Web Site
constexpr uint16_t USER_APP_TPEG = 0x004;          // TPEG (Traffic & Travel)
constexpr uint16_t USER_APP_DGPS = 0x005;          // DGPS
constexpr uint16_t USER_APP_TMC = 0x006;           // TMC (Traffic Message Channel)
constexpr uint16_t USER_APP_EPG = 0x007;           // Electronic Programme Guide
constexpr uint16_t USER_APP_DAB_JAVA = 0x008;      // DAB Java
constexpr uint16_t USER_APP_JOURNALINE = 0x044;    // Journaline
```

**Parsing Logic:**
```cpp
void AdvancedFIGAnalyzer::parseFIG0_13(const std::vector<uint8_t>& fib) {
    // Structure per service component:
    // - Service ID: 16-bit or 32-bit (depends on P/D flag)
    // - SCIdS: 4-bit service component identifier
    // - Number of user applications: 4-bit
    // For each user application:
    //   - User app type: 11-bit
    //   - User app data length: 5-bit
    //   - User app data: variable length

    // Extract P/D flag from FIG header (byte 0, bit 2)
    bool pd_flag = (fib[0] & 0x04) != 0;

    // Parse service components with applications
    // Signal discovery of MOT Slideshow, EPG, etc.
}
```

**Key Use Cases:**
- Detect MOT Slideshow availability for GUI slideshow panel
- Identify EPG data streams for program guide
- Discover TPEG traffic information services

---

### **FIG 0/14: FEC Sub-channel Organization**

**ETSI Reference:** EN 300 401 Section 6.2.2
**Purpose:** Defines Forward Error Correction (FEC) scheme for sub-channels

**Structure Definition:**
```cpp
struct FIG0_14_FECScheme {
    uint8_t subchannel_id;         // Sub-channel identifier (6 bits)
    uint8_t fec_scheme;            // FEC scheme identifier (2 bits)
};

// FEC Scheme Values (ETSI EN 300 401 Table 33)
constexpr uint8_t FEC_SCHEME_NO_FEC = 0;
constexpr uint8_t FEC_SCHEME_RS = 1;     // Reed-Solomon
constexpr uint8_t FEC_SCHEME_RS_2 = 2;   // Reed-Solomon variant
constexpr uint8_t FEC_SCHEME_RESERVED = 3;
```

**Parsing Logic:**
```cpp
void AdvancedFIGAnalyzer::parseFIG0_14(const std::vector<uint8_t>& fib) {
    // Simple structure: 1 byte per sub-channel
    // Byte format: SubChId(6) + FEC scheme(2)

    for (size_t offset = 1; offset < fib.size(); ++offset) {
        FIG0_14_FECScheme fec;
        fec.subchannel_id = (fib[offset] >> 2) & 0x3F;
        fec.fec_scheme = fib[offset] & 0x03;

        m_fec_schemes[fec.subchannel_id] = fec;
    }
}
```

---

### **FIG 0/17: Programme Type**

**ETSI Reference:** EN 300 401 Section 8.1.5
**Purpose:** Classifies programmes by genre (news, music, sports, etc.)

**Structure Definition:**
```cpp
struct FIG0_17_ProgrammeType {
    uint16_t service_id;           // Service identifier
    bool international;            // International/national flag
    uint8_t language_code;         // ISO 639-2 language code (if international)
    uint8_t programme_type;        // PTy code (5 bits)
};

// Programme Type Codes (ETSI EN 300 401 Table 10)
constexpr uint8_t PTY_NO_PROGRAMME = 0;
constexpr uint8_t PTY_NEWS = 1;
constexpr uint8_t PTY_CURRENT_AFFAIRS = 2;
constexpr uint8_t PTY_INFORMATION = 3;
constexpr uint8_t PTY_SPORT = 4;
constexpr uint8_t PTY_EDUCATION = 5;
constexpr uint8_t PTY_DRAMA = 6;
constexpr uint8_t PTY_CULTURE = 7;
constexpr uint8_t PTY_SCIENCE = 8;
constexpr uint8_t PTY_VARIED = 9;
constexpr uint8_t PTY_POP_MUSIC = 10;
constexpr uint8_t PTY_ROCK_MUSIC = 11;
constexpr uint8_t PTY_EASY_LISTENING = 12;
constexpr uint8_t PTY_LIGHT_CLASSICAL = 13;
constexpr uint8_t PTY_SERIOUS_CLASSICAL = 14;
constexpr uint8_t PTY_OTHER_MUSIC = 15;
// ... (31 total PTy codes)
```

**Parsing Logic:**
```cpp
void AdvancedFIGAnalyzer::parseFIG0_17(const std::vector<uint8_t>& fib) {
    // Extract P/D flag and international flag
    bool pd_flag = (fib[0] & 0x04) != 0;

    // Variable-length structure depends on:
    // - Service ID size (16-bit or 32-bit from P/D flag)
    // - International flag (adds language code if set)

    // Parse programme type for each service
    // Used for EPG classification and search filtering
}
```

---

### **FIG 0/18: Announcement Support**

**ETSI Reference:** EN 300 401 Section 8.1.6.1
**Purpose:** Indicates which announcement types a service can carry

**Structure Definition:**
```cpp
struct FIG0_18_AnnouncementSupport {
    uint16_t service_id;           // Service identifier
    uint16_t announcement_support_flags;  // 16-bit flags
    uint8_t cluster_id;            // Cluster identifier (optional)
};

// Announcement Type Flags (ETSI EN 300 401 Table 14)
constexpr uint16_t ANNOUNCEMENT_ALARM = 0x0001;           // Emergency alarm (EWS)
constexpr uint16_t ANNOUNCEMENT_ROAD_TRAFFIC = 0x0002;    // Road traffic flash
constexpr uint16_t ANNOUNCEMENT_TRANSPORT_FLASH = 0x0004; // Transport flash
constexpr uint16_t ANNOUNCEMENT_WARNING = 0x0008;         // Warning/Service
constexpr uint16_t ANNOUNCEMENT_NEWS_FLASH = 0x0010;      // News flash
constexpr uint16_t ANNOUNCEMENT_AREA_WEATHER = 0x0020;    // Area weather flash
constexpr uint16_t ANNOUNCEMENT_EVENT = 0x0040;           // Event announcement
constexpr uint16_t ANNOUNCEMENT_SPECIAL_EVENT = 0x0080;   // Special event
constexpr uint16_t ANNOUNCEMENT_PROGRAMME_INFO = 0x0100;  // Programme information
constexpr uint16_t ANNOUNCEMENT_SPORT_REPORT = 0x0200;    // Sport report
constexpr uint16_t ANNOUNCEMENT_FINANCIAL_REPORT = 0x0400; // Financial report
```

**Parsing Logic:**
```cpp
void AdvancedFIGAnalyzer::parseFIG0_18(const std::vector<uint8_t>& fib) {
    // Extract announcement support flags for each service
    // Critical for EWS: detect if service supports ALARM announcements

    for (auto& support : announcement_supports) {
        if (support.announcement_support_flags & ANNOUNCEMENT_ALARM) {
            // This service can carry emergency alarms
            qInfo() << "Service" << Qt::hex << support.service_id
                    << "supports EMERGENCY ALARMS (EWS capable)";
        }
    }
}
```

**EWS Relevance:** Identifies which services are capable of carrying emergency warnings.

---

### **FIG 0/19: Announcement Switching** ⚠️ **EWS CRITICAL**

**ETSI Reference:** EN 300 401 Section 8.1.6.2
**Purpose:** Real-time announcement activation/deactivation (Emergency Warning System core)

**Structure Definition:**
```cpp
struct FIG0_19_AnnouncementSwitching {
    uint8_t cluster_id;            // Cluster identifier
    uint16_t announcement_flags;    // Active announcement types (16-bit flags)
    uint8_t subchannel_id;         // Sub-channel carrying announcement
    bool new_flag;                 // New announcement indicator
    uint8_t region_id;             // Geographic region (optional)
};

// Same announcement type flags as FIG 0/18
```

**Parsing Logic:**
```cpp
void AdvancedFIGAnalyzer::parseFIG0_19(const std::vector<uint8_t>& fib) {
    // CRITICAL: Real-time announcement activation

    for (auto& switching : announcement_switching) {
        if (switching.announcement_flags & ANNOUNCEMENT_ALARM) {
            // EMERGENCY ALARM ACTIVE NOW!
            qCritical() << "🚨 EMERGENCY ALARM ACTIVE on cluster"
                        << switching.cluster_id
                        << "subchannel" << switching.subchannel_id;

            // Trigger EWS alert UI
            emit emergencyAlarmActivated(switching);

            // Log for Thai NBTC compliance reporting
            logEWSActivation(switching);
        }

        if (switching.new_flag) {
            // New announcement just started
            qWarning() << "NEW announcement:" << switching.announcement_flags;
        }
    }
}
```

**EWS Critical Features:**
- Real-time detection of emergency alarm activation
- Geographic region targeting support
- New announcement flag for immediate UI updates
- Integration point for EWS Alert Manager (Phase 4A)

**Thai NBTC Compliance:**
- Required for emergency broadcast system validation
- Must log all EWS activations with timestamps
- Region-based filtering for Bangkok metropolitan area

---

## Implementation Tasks

### **Task 1: Structure Definitions** (30 minutes)
- [ ] Add 5 FIG structures to `advanced_fig_analyzer.h`
- [ ] Define announcement type constants
- [ ] Define programme type constants
- [ ] Define user application type constants

### **Task 2: Parser Implementations** (3 hours)
- [ ] Implement `parseFIG0_13()` - User applications
- [ ] Implement `parseFIG0_14()` - FEC schemes
- [ ] Implement `parseFIG0_17()` - Programme types
- [ ] Implement `parseFIG0_18()` - Announcement support
- [ ] Implement `parseFIG0_19()` - Announcement switching (EWS)

### **Task 3: Integration** (1 hour)
- [ ] Add case statements to `processFIG0()` dispatcher
- [ ] Update `initializeDefaultFIGTypes()` to enable new FIGs
- [ ] Add storage containers for parsed data
- [ ] Implement Qt signals for EWS events

### **Task 4: Testing** (2 hours)
- [ ] Create unit tests for each FIG type (minimum 10 tests)
- [ ] Test announcement switching with simulated EWS data
- [ ] Validate against ODR-DabMux generated streams
- [ ] Verify ETSI compliance for all parsers

### **Task 5: Documentation** (30 minutes)
- [ ] Add inline documentation for all parsers
- [ ] Document announcement type flags
- [ ] Create EWS integration guide
- [ ] Update ETSI compliance checklist

---

## Success Criteria

### **Functional Requirements:**
- ✅ All 5 FIG parsers implemented and tested
- ✅ FIG 0/19 correctly detects emergency alarm activation
- ✅ Programme type classification functional
- ✅ User application discovery working (MOT Slideshow, EPG)
- ✅ Build compiles with zero errors

### **ETSI Compliance:**
- ✅ EN 300 401 Section 6.3.6 (FIG 0/13) - 100%
- ✅ EN 300 401 Section 6.2.2 (FIG 0/14) - 100%
- ✅ EN 300 401 Section 8.1.5 (FIG 0/17) - 100%
- ✅ EN 300 401 Section 8.1.6.1 (FIG 0/18) - 100%
- ✅ EN 300 401 Section 8.1.6.2 (FIG 0/19) - 100%

### **Thai NBTC EWS Requirements:**
- ✅ Real-time emergency alarm detection (FIG 0/19)
- ✅ EWS event logging with timestamps
- ✅ Geographic region filtering support
- ✅ Integration ready for Phase 4A EWS Alert Manager

---

## Risk Assessment

| Risk | Impact | Mitigation |
|------|--------|------------|
| **FIG 0/19 complexity** | HIGH | Reference ODR-DabMux extensively, test with real EWS data |
| **Announcement flag interpretation** | MEDIUM | Cross-reference ETSI tables, validate against known streams |
| **P/D flag handling in FIG 0/13/0/17** | MEDIUM | Reuse proven logic from FIG 0/6, 0/8 |
| **Build integration issues** | LOW | Incremental commits, continuous testing |

---

## Timeline

**Total Effort:** 1 week (6-8 hours development)

- **Day 1:** Structure definitions + FIG 0/13, 0/14 parsers
- **Day 2:** FIG 0/17, 0/18 parsers
- **Day 3:** FIG 0/19 parser (EWS critical) + testing
- **Day 4:** Integration testing + code review
- **Day 5:** Documentation + commit

---

## Integration Points

### **For Phase 4A (EWS Alert Manager):**
```cpp
// FIG 0/19 parser will emit signals that EWS Manager will consume
connect(figAnalyzer, &AdvancedFIGAnalyzer::emergencyAlarmActivated,
        ewsManager, &EWSAlertManager::handleEmergencyAlarm);
```

### **For GUI Integration (Release 1B):**
```cpp
// Programme type used for service filtering
auto services = figAnalyzer->getServicesByProgrammeType(PTY_NEWS);

// User applications shown in service details panel
auto apps = figAnalyzer->getUserApplications(service_id);
if (apps.contains(USER_APP_MOT_SLIDESHOW)) {
    enableSlideshowPanel();
}
```

---

## References

**ETSI Standards:**
- EN 300 401 V2.1.1 (2017-01) - DAB Radio Broadcasting System

**ODR-DabMux Source:**
- `/home/seksan/workspace/ODR-DabMux/src/fig/FIG0_13.cpp`
- `/home/seksan/workspace/ODR-DabMux/src/fig/FIG0_14.cpp`
- `/home/seksan/workspace/ODR-DabMux/src/fig/FIG0_17.cpp`
- `/home/seksan/workspace/ODR-DabMux/src/fig/FIG0_18.cpp`
- `/home/seksan/workspace/ODR-DabMux/src/fig/FIG0_19.cpp`

**Related Documents:**
- `RELEASE_1A_CORE_PROCESSORS_PLAN.md` - Overall project plan
- `PHASE_2A_WEEK5_FIG_EXTENSIONS.md` - Previous week implementation
- Thai NBTC Emergency Broadcast Requirements (external)

---

**Document Status:** ✅ Ready for Implementation
**Next Step:** Launch backend-developer agent for implementation
