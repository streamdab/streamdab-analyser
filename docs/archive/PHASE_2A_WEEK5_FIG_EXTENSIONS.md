# Phase 2A Week 5: FIG Type 0 Extensions Implementation

**Date**: 2025-10-19
**Phase**: Release 1A - Week 5
**Goal**: Implement FIG 0/3 through 0/10 parsers
**Reference**: `/home/seksan/workspace/ODR-DabMux/src/fig/FIG0_*.cpp`

---

## Implementation Checklist

### FIG 0/3 - Service Component in Packet Mode
- [ ] Study `/home/seksan/workspace/ODR-DabMux/src/fig/FIG0_3.cpp`
- [ ] Create parser in `advanced_fig_analyzer.cpp`
- [ ] Add `parseFIG0_3()` method
- [ ] Create unit tests

**Structure**:
```cpp
struct FIG0_3_ServiceComponent {
    uint16_t service_id;      // 16-bit service ID
    uint8_t subchannel_id;    // Sub-channel ID
    uint16_t component_id;    // Component ID within service
    bool ca_flag;             // Conditional access flag
};
```

### FIG 0/5 - Service Component Language
- [ ] Study `/home/seksan/workspace/ODR-DabMux/src/fig/FIG0_5.cpp`
- [ ] Create parser in `advanced_fig_analyzer.cpp`
- [ ] Add `parseFIG0_5()` method
- [ ] Create unit tests

**Structure**:
```cpp
struct FIG0_5_Language {
    uint16_t service_id;      // Service ID (16 or 32-bit)
    uint8_t component_id;     // Component ID within service (SCIdS)
    uint8_t language_code;    // ISO 639-2 language code
};
```

### FIG 0/6 - Service Linking
- [ ] Study `/home/seksan/workspace/ODR-DabMux/src/fig/FIG0_6.cpp`
- [ ] Create parser in `advanced_fig_analyzer.cpp`
- [ ] Add `parseFIG0_6()` method
- [ ] Create unit tests

**Structure**:
```cpp
struct FIG0_6_ServiceLink {
    bool id_list_flag;        // Identifier list flag
    bool la_flag;             // Linkage actuator flag
    bool sh_flag;             // Soft/Hard link
    bool ioe_flag;            // International or Ensemble flag
    uint16_t link_service_id; // Service to link to
    uint8_t num_ids;          // Number of IDs in list
    std::vector<uint16_t> id_list;  // List of linked IDs
};
```

### FIG 0/7 - Configuration Information
- [ ] Study `/home/seksan/workspace/ODR-DabMux/src/fig/FIG0_7.cpp`
- [ ] Create parser in `advanced_fig_analyzer.cpp`
- [ ] Add `parseFIG0_7()` method
- [ ] Create unit tests

**Structure**:
```cpp
struct FIG0_7_Configuration {
    uint8_t reconfiguration_count;  // Reconfig counter
    bool count_services;            // Service count indication
    uint16_t num_services;          // Number of services (if flag set)
};
```

### FIG 0/8 - Service Component Global Definition
- [ ] Study `/home/seksan/workspace/ODR-DabMux/src/fig/FIG0_8.cpp`
- [ ] Create parser in `advanced_fig_analyzer.cpp`
- [ ] Add `parseFIG0_8()` method
- [ ] Create unit tests

**Structure**:
```cpp
struct FIG0_8_GlobalDefinition {
    uint16_t service_id;             // 16 or 32-bit service ID
    bool long_form;                  // Long/short form indicator
    uint8_t component_id_within_service;  // SCIdS
    uint8_t subchannel_id;           // Sub-channel ID
    bool is_dab_plus;                // DAB+ flag (from ASCTy/DSCTy)
    uint8_t service_component_type;  // Audio (0x00) or Data type
};
```

### FIG 0/9 - Country, LTO, International Table
- [ ] Study `/home/seksan/workspace/ODR-DabMux/src/fig/FIG0_9.cpp`
- [ ] Create parser in `advanced_fig_analyzer.cpp`
- [ ] Add `parseFIG0_9()` method
- [ ] Create unit tests

**Structure**:
```cpp
struct FIG0_9_CountryLTO {
    uint8_t ensemble_lto;          // Local Time Offset (half-hours from UTC)
    uint8_t ensemble_ecc;          // Extended Country Code
    uint8_t international_table_id; // Character table ID
};
```

### FIG 0/10 - Date and Time
- [ ] Study `/home/seksan/workspace/ODR-DabMux/src/fig/FIG0_10.cpp`
- [ ] Create parser in `advanced_fig_analyzer.cpp`
- [ ] Add `parseFIG0_10()` method
- [ ] Create unit tests

**Structure**:
```cpp
struct FIG0_10_DateTime {
    uint32_t mjd;              // Modified Julian Date (17 bits)
    bool lsi_flag;             // Leap second indicator
    bool conf_flag;            // Conformance flag
    bool utc_flag;             // UTC/local time flag
    uint8_t hours;             // Hours (0-23)
    uint8_t minutes;           // Minutes (0-59)
    uint8_t seconds;           // Seconds (0-59, optional)
    uint16_t milliseconds;     // Milliseconds (optional)
};
```

---

## Integration Points

### File Modifications Required

**1. src/core/advanced_fig_analyzer.h**
- Add new structure definitions
- Add new parser method declarations
- Add storage maps for new FIG types

**2. src/core/advanced_fig_analyzer.cpp**
- Implement 7 new parser methods
- Add dispatch logic in `processFIG0()` method
- Add data storage and retrieval methods

**3. tests/test_fig0_processor.cpp**
- Add test cases for each new FIG type
- Minimum 3 tests per FIG type (valid, invalid, edge cases)

---

## Implementation Strategy

### Phase 1: Study & Design (Day 1 Morning)
- Read all 7 ODR-DabMux encoder implementations
- Document bit-level structures
- Create test data generation plan

### Phase 2: Core Implementation (Day 1 Afternoon - Day 2)
- Implement parsers for FIG 0/3, 0/5, 0/6, 0/7
- Implement parsers for FIG 0/8, 0/9, 0/10
- Add dispatch logic and storage

### Phase 3: Testing (Day 2-3)
- Create unit tests for all 7 FIG types
- Test with Bangkok ETI file (if contains these FIGs)
- Validate against ETSI EN 300 401 specifications

---

## Success Criteria

- [ ] All 7 FIG parsers implemented and compiling
- [ ] Minimum 21 unit tests created (3 per FIG type)
- [ ] 100% test pass rate on new FIG tests
- [ ] Integration with existing FIG processing pipeline
- [ ] No regressions in existing FIG 0/0, 0/1, 0/2 tests

---

## ETSI Standards References

**ETSI EN 300 401 V2.1.1 (2017-01)**

- Section 6.3.1: FIG 0/3 (Service component in packet mode)
- Section 6.3.2: FIG 0/5 (Service component language)
- Section 8.1.9: FIG 0/6 (Service linking)
- Section 8.1.10: FIG 0/7 (Configuration information)
- Section 6.3.5: FIG 0/8 (Service component global definition)
- Section 5.2.2: FIG 0/9 (Country, LTO, International table)
- Section 8.1.3.2: FIG 0/10 (Date and time)

---

**Status**: Ready to implement
**Next**: Launch implementation agent or begin manual coding
