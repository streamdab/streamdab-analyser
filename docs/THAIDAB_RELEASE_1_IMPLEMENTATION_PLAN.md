# Thai DAB Standards Compliance - Release 1 Implementation Plan

**Document Version:** 1.0
**Date:** October 19, 2025
**Author:** ETI Stream Analyzer Development Team
**Based On:** THAIDAB_REFERENCE_COMPAEISON.md + Current Code Assessment

---

## Executive Summary

This document provides a comprehensive assessment of current ETSI standards implementation in the StreamDAB Analyzer and defines a clear Release 1 roadmap for full Thai DAB compliance. The plan prioritizes mandatory standards for Thai broadcast deployment while establishing a foundation for future feature releases.

> **📋 IMPORTANT UPDATE (October 19, 2025):**
> This master plan has been split into two focused implementation sub-plans:
>
> - **[RELEASE_1A_CORE_PROCESSORS_PLAN.md](RELEASE_1A_CORE_PROCESSORS_PLAN.md)** - Backend processors development (65% → 100%)
> - **[RELEASE_1B_GUI_INTEGRATION_PLAN.md](RELEASE_1B_GUI_INTEGRATION_PLAN.md)** - GUI integration and polish
>
> **Strategy:** "Processors First, UI Later" - Focus on completing robust backends before GUI integration
> **Code Reference:** `/home/seksan/workspace/ODR-DabMux` - Reference implementation for FIG, EDI, and EWS

### Current Implementation Status: **65% Complete** → Target: **100% Complete**

**Current Strengths (65%):**

- ✅ Core ETI processing engine (EN 300 799) - **95% complete**
- ✅ Basic FIG processing framework (EN 300 401) - **80% complete**
- ✅ ETSI compliance validation engine - **70% complete**
- ✅ Professional Qt6 GUI foundation - **100% complete**

**Critical Gaps for 100% Completion (35%):**

- ❌ **EDI network interface (TS 102 693) - 0%** ← **HIGHEST PRIORITY (3 weeks)**
  - Reference: `/home/seksan/workspace/ODR-DabMux/lib/edioutput/`
  - AF packets, PFT reassembly, TAG items, network transport
- ❌ **Complete FIG processing (EN 300 401) - 60%** ← **HIGH PRIORITY (3 weeks)**
  - Reference: `/home/seksan/workspace/ODR-DabMux/src/fig/FIG0_*.cpp`
  - Missing FIG 0/3-0/19, FIG 1/5, FIG 2/x (extended labels)
- ❌ **DAB+ audio validation (TS 102 563) - 30%** ← **MEDIUM PRIORITY (2 weeks)**
  - Stream analysis (NOT decoding - uses fdk-aac library)
  - Superframe validation, RS/FireCode checking
- ❌ **MOT/SlideShow (EN 301 234, TS 101 499) - 10%** ← **MEDIUM PRIORITY (3 weeks)**
  - MOT protocol, segment reassembly, JPEG extraction
- ❌ **Emergency Warning System (TS 104 089/090) - 0%** ← **HIGH PRIORITY (1 week)**
  - Reference: `/home/seksan/workspace/ODR-DabMux/src/ews/`
  - ASA protocol, alert management, FIG 0/18-19 integration
- ⚠️ **GUI Integration - NEW** ← **POST-PROCESSORS (3-4 weeks)**
  - Connect all processors to Qt6 interface
  - Add network monitor, audio quality, SlideShow viewer, alert widgets

### **Release 1 Timeline Breakdown**

| Phase | Focus | Duration | Completion |
|-------|-------|----------|------------|
| **Release 1A** | Core Processors (Backend) | 10-12 weeks | 65% → 100% |
| **Release 1B** | GUI Integration & Polish | 3-4 weeks | 100% → Production |
| **TOTAL** | Complete Thai DAB Analyzer | **13-16 weeks** | **100% Ready** |

---

## Table of Contents

1. [Release 1 Scope Definition](#release-1-scope-definition)
2. [Standards Gap Analysis](#standards-gap-analysis)
3. [Current Code Assessment](#current-code-assessment)
4. [Release 1 Implementation Roadmap](#release-1-implementation-roadmap)
5. [Future Releases Planning](#future-releases-planning)
6. [Testing & Validation Strategy](#testing--validation-strategy)
7. [Success Criteria](#success-criteria)

---

## Release 1 Scope Definition

### **Mandatory Standards (Must Implement)**

Based on THAIDAB_REFERENCE_COMPAEISON.md marking (✅ symbols), Release 1 MUST support:

#### **Distribution Networks (Project Major Focus)**
| Standard | Version | Status | Priority | Effort |
|----------|---------|--------|----------|--------|
| **ETS 300 799** | V1.0 (1997) | 95% Complete | **P0** | 1 week |
| **TS 102 693** | V1.1.2 (2011) | 0% Complete | **P0** | 3 weeks |

#### **DAB System (Core Functionality)**
| Standard | Version | Status | Priority | Effort |
|----------|---------|--------|----------|--------|
| **EN 300 401** | V2.1.1 (2017-01) | 80% Complete | **P0** | 2 weeks |
| **TS 101 756** | V2.4.1 (2020-08) | 70% Complete | **P1** | 1 week |
| **TS 103 176** | V2.4.1 (2020-08) | 60% Complete | **P1** | 2 weeks |

#### **Audio Coding (Essential for DAB+)**
| Standard | Version | Status | Priority | Effort |
|----------|---------|--------|----------|--------|
| **TS 102 563** | V2.1.1 (2017-01) | 30% Complete | **P0** | 2 weeks |

*Note: DAB+ decoding uses fdk-aac library - Release 1 focuses on stream analysis, NOT codec implementation*

#### **Emergency Warning System**
| Standard | Version | Status | Priority | Effort |
|----------|---------|--------|----------|--------|
| **EN 300 401 Announcement** | V2.1.1 (2017-01) | 40% Complete | **P1** | 1 week |
| **TR 101 496-3** | V1.1.2 (2001-05) | 0% Complete | **P2** | 1 week |
| **TS 104 089** | V1.1.1 (2024-09) | 0% Complete | **P2** | 2 weeks |
| **TS 104 090** | V1.1.2 (2024) | 0% Complete | **P2** | 1 week |

#### **Data Services (Optional but Recommended)**
| Standard | Version | Status | Priority | Effort |
|----------|---------|--------|----------|--------|
| **EN 301 234** | V2.1.1 (2017) | 10% Complete | **P1** | 2 weeks |
| **TS 101 499** | V3.2.1 (2023-07) | 5% Complete | **P1** | 1 week |
| **TS 102 818** | V3.3.1 (2020-08) | 0% Complete | **P2** | 2 weeks |
| **TS 102 371** | V3.3.1 (2023) | 0% Complete | **P3** | 1 week |
| **TS 103 551** | V1.1.1 (2017-08) | 0% Complete | **P2** | 2 weeks |
| **TS 102 980** | V2.1.2 (2024) | 0% Complete | **P3** | 1 week |

#### **Receiver Standards (Analysis Validation)**
| Standard | Version | Status | Priority | Effort |
|----------|---------|--------|----------|--------|
| **TS 103 461** | V1.2.2 (2020-10) | 20% Complete | **P2** | 1 week |

### **Total Release 1 Effort Estimate: 12-14 weeks**

---

## Standards Gap Analysis

### **Current Implementation - Detailed Assessment**

#### ✅ **EN 300 799 (ETI Distribution Interface) - 95% Complete**

**Implemented:**
- ✅ ETI-NI frame parsing (6144 bytes)
- ✅ ETI-LI format support (Pattern A/B)
- ✅ Sync pattern validation (multiple byte orders)
- ✅ Frame counter validation with wraparound
- ✅ FIC/MSC extraction
- ✅ CRC validation framework
- ✅ TIST timestamp extraction
- ✅ Real-time stream processing

**Missing (5%):**
- ❌ STI (Service Transport Interface) - EN 300 797 integration
- ❌ DIQ (Data Interface) - EN 300 798 support
- ⚠️ EDI (Enhanced Distribution Interface) - TS 102 693 **CRITICAL GAP**

**Code Location:**
```cpp
src/core/eti_frame_parser.cpp           // Main ETI parsing (lines 1-850)
src/core/etsi_compliance_engine.cpp     // EN 300 799 validation (lines 19-200)
tests/test_eti_frame_parser.cpp         // Comprehensive ETI tests
```

**Release 1 Actions:**
1. ✅ Complete remaining 5% ETI-LI edge cases
2. ✅ Implement TS 102 693 (EDI) network interface
3. ✅ Add EN 300 797 (STI) optional support

---

#### ⚠️ **EN 300 401 (DAB System Standard) - 80% Complete**

**Implemented:**
- ✅ FIG 0/0 (Ensemble information)
- ✅ FIG 0/1 (Sub-channel organization) - partial EEP/UEP
- ✅ FIG 0/2 (Service organization) - basic parsing
- ✅ FIG 1/0 (Ensemble label)
- ✅ FIG 1/1 (Service label)
- ✅ FIG 1/4 (Service component label)
- ✅ Transmission Mode I parameters
- ✅ OFDM symbol structure awareness

**Missing (20%):**
- ❌ FIG 0/3 (Service component in packet mode)
- ❌ FIG 0/5 (Service component language)
- ❌ FIG 0/8 (Service component global definition)
- ❌ FIG 0/9 (Country, LTO and International table)
- ❌ FIG 0/10 (Date and time)
- ❌ FIG 0/13 (User application information)
- ❌ FIG 0/17 (Programme Type)
- ❌ FIG 0/18 (Announcement support)
- ❌ FIG 0/19 (Announcement switching)
- ❌ FIG 1/5 (Data service label)
- ❌ FIG 2/x (Extended labels)
- ⚠️ Transmission Mode II/III/IV support

**Code Location:**
```cpp
src/core/fig0_processor.cpp             // FIG Type 0 processing (lines 1-580)
src/core/fig1_processor.cpp             // FIG Type 1 processing (lines 1-420)
include/fig_types.hpp                   // FIG structure definitions
```

**Release 1 Actions:**
1. ✅ Implement missing FIG 0 types (0/3, 0/5, 0/8, 0/9, 0/10, 0/13, 0/17, 0/18, 0/19)
2. ✅ Implement FIG 1/5 (Data service labels)
3. ✅ Add FIG 2 extended label support
4. ⚠️ Transmission Mode II/III/IV - Optional for Release 1 (Thai uses Mode I)

---

#### ⚠️ **TS 102 563 (DAB+ Audio Coding) - 30% Complete**

**Implemented:**
- ✅ HE-AAC v2 superframe structure awareness
- ✅ Reed-Solomon outer coding detection
- ✅ FireCode inner coding framework
- ⚠️ Basic audio parameter validation

**Missing (70%):**
- ❌ Complete HE-AAC v2 profile validation
- ❌ SBR (Spectral Band Replication) parameter checking
- ❌ PS (Parametric Stereo) validation
- ❌ Audio superframe CRC validation
- ❌ Bitrate/quality compliance checks
- ❌ Audio frame timing validation

**IMPORTANT NOTE:** Release 1 does NOT implement audio decoding (uses fdk-aac library). Focus is on STREAM ANALYSIS and VALIDATION.

**Code Location:**
```cpp
src/core/audio_parameter_validator.cpp  // Basic audio validation (partial)
include/audio_types.hpp                 // Audio structure definitions
```

**Release 1 Actions:**
1. ✅ Implement complete TS 102 563 stream validation (NOT decoding)
2. ✅ Add audio superframe structure validation
3. ✅ Validate DAB+ encoding parameters against spec
4. ✅ Quality metrics extraction (bitrate, protection level)

---

#### ❌ **TS 102 693 (EDI - Enhanced Distribution Interface) - 0% Complete**

**CRITICAL GAP FOR RELEASE 1**

This is a MAJOR missing component for professional broadcast deployment.

**Required Implementation:**
- ❌ EDI packet structure parsing
- ❌ EDI protocol tags (AF, PFT, TAG items)
- ❌ Network transport (UDP multicast)
- ❌ Forward Error Correction (FEC)
- ❌ EDI to ETI conversion
- ❌ Real-time network streaming support

**Code Location:** NEW FILES REQUIRED

**Release 1 Actions:**
1. ✅ Create EDI protocol parser framework
2. ✅ Implement UDP multicast receiver
3. ✅ Add EDI packet reassembly
4. ✅ EDI-to-ETI conversion layer
5. ✅ Network stream monitoring GUI

**Estimated Effort:** 3 weeks (HIGHEST PRIORITY)

---

#### ❌ **Emergency Warning System Standards - 0% Complete**

**Required for Thai DAB Compliance:**

**EN 300 401 Announcement Switching (40% complete):**
- ✅ FIG 0/18 framework exists
- ❌ FIG 0/19 announcement switching NOT implemented
- ❌ Alarm flag processing incomplete

**TS 104 089 (EWS Definition - NEW 2024 standard):**
- ❌ ASA (Alert Signal Application) protocol
- ❌ Emergency alert message structure
- ❌ Alert priority levels
- ❌ Geographic targeting

**TS 104 090 (EWS Receiver Requirements):**
- ❌ Receiver behavior validation
- ❌ Alert presentation requirements

**Code Location:** NEW FILES REQUIRED

**Release 1 Actions:**
1. ✅ Complete FIG 0/18 and 0/19 implementation
2. ✅ Add TS 104 089 ASA protocol support
3. ✅ Implement emergency alert detection
4. ✅ Add EWS compliance validation
5. ✅ GUI indicators for emergency alerts

**Estimated Effort:** 4 weeks

---

#### ⚠️ **Data Services (MOT/Slideshow/EPG) - 10% Complete**

**EN 301 234 (MOT Protocol):**
- ✅ Basic MOT header awareness
- ❌ MOT directory parsing
- ❌ MOT object carousel
- ❌ Segment reassembly

**TS 101 499 (SlideShow):**
- ❌ JPEG image extraction
- ❌ SlideShow metadata
- ❌ Image carousel management

**TS 102 818 (EPG XML):**
- ❌ EPG XML schema parsing
- ❌ Programme information extraction
- ❌ Schedule data management

**TS 102 371 (EPG Binary):**
- ❌ Binary EPG format

**TS 103 551 (TPEG Traffic Info):**
- ❌ TPEG message structure
- ❌ Traffic data extraction

**TS 102 980 (Dynamic Label Plus):**
- ❌ DL Plus tag parsing
- ❌ Structured text display

**Code Location:** PARTIAL IMPLEMENTATION

**Release 1 Actions (Priority Order):**
1. ✅ **P1:** Complete EN 301 234 (MOT) - 2 weeks
2. ✅ **P1:** Complete TS 101 499 (SlideShow) - 1 week
3. ⚠️ **P2:** Add TS 102 818 (EPG XML) - 2 weeks
4. ⚠️ **P3:** Add TS 102 371 (EPG Binary) - 1 week
5. ⚠️ **P2:** Add TS 103 551 (TPEG) - 2 weeks
6. ⚠️ **P3:** Add TS 102 980 (DL Plus) - 1 week

**Total Data Services Effort:** 5-9 weeks (based on priority selection)

---

### **Gap Summary Matrix**

| Category | Standards | Current % | Target % | Gap | Effort |
|----------|-----------|-----------|----------|-----|--------|
| **Distribution Networks** | ETS 300 799, TS 102 693 | 48% | 100% | 52% | 4 weeks |
| **DAB System** | EN 300 401, TS 101 756, TS 103 176 | 70% | 95% | 25% | 5 weeks |
| **Audio Coding** | TS 102 563 | 30% | 85% | 55% | 2 weeks |
| **Emergency Warnings** | FIG 0/18-19, TS 104 089/090 | 10% | 90% | 80% | 4 weeks |
| **Data Services** | MOT, Slideshow, EPG, TPEG, DL+ | 8% | 80% | 72% | 9 weeks |
| **Receiver Validation** | TS 103 461 | 20% | 70% | 50% | 1 week |

**Total Implementation Gap:** 35% → Estimated **12-14 weeks** to Release 1

---

## Current Code Assessment

### **Strengths of Current Implementation**

#### 1. **Solid ETI Processing Foundation**
```cpp
// src/core/eti_frame_parser.cpp (Lines 85-180)
class ETIFrameParser {
    // ✅ Robust ETI-NI and ETI-LI support
    // ✅ Multiple sync pattern detection
    // ✅ Frame counter validation with wraparound
    // ✅ Proper CRC validation
    // ✅ TIST timestamp extraction
};
```

**Code Quality:** A+ (99.5/100 from recent code review)
**ETSI EN 300 799 Compliance:** 95%

#### 2. **Professional ETSI Compliance Engine**
```cpp
// src/core/etsi_compliance_engine.cpp (Lines 1-400)
namespace etsi {
    class ETSIComplianceEngine : public QObject {
        // ✅ Validation result framework
        // ✅ Compliance reporting
        // ✅ Multi-standard support architecture
        // ✅ Real-time monitoring signals
    };
}
```

**Code Quality:** Excellent architecture, extensible design
**Current Standards Coverage:** 5/20+ standards

#### 3. **Comprehensive FIG Processing**
```cpp
// src/core/fig0_processor.cpp (Lines 1-580)
// src/core/fig1_processor.cpp (Lines 1-420)
// ✅ FIG 0/0, 0/1, 0/2 implemented
// ✅ FIG 1/0, 1/1, 1/4 implemented
// ✅ Service discovery working
// ✅ Ensemble information extraction
```

**Code Quality:** Well-structured, good test coverage
**FIG Coverage:** 6/30+ FIG types

### **Architecture Weaknesses Requiring Attention**

#### 1. **Missing EDI Network Stack**
```
CURRENT: File-based ETI-NI/LI only
NEEDED:  Network EDI receiver with UDP multicast
         └── TS 102 693 protocol parser
             ├── AF (Application Format) packets
             ├── PFT (Protection, Fragmentation, Transport)
             ├── TAG item decoding
             └── FEC (Forward Error Correction)
```

**Impact:** Cannot process real-time broadcast streams from Thai DAB network
**Priority:** P0 - CRITICAL for Release 1

#### 2. **Incomplete Data Service Stack**
```
CURRENT: Basic MOT awareness, no reassembly
NEEDED:  Complete data service pipeline
         └── EN 301 234 MOT protocol
             ├── MOT directory parsing
             ├── Segment reassembly
             ├── Object carousel management
             └── Data service specific parsers
                 ├── TS 101 499 (SlideShow JPEG)
                 ├── TS 102 818 (EPG XML)
                 ├── TS 103 551 (TPEG)
                 └── TS 102 980 (DL Plus)
```

**Impact:** Cannot analyze data services in Thai DAB broadcasts
**Priority:** P1-P2 depending on specific service

#### 3. **Emergency Warning System Gap**
```
CURRENT: FIG 0/18 framework exists, incomplete
NEEDED:  Complete EWS implementation
         ├── FIG 0/18 (Announcement support) - COMPLETE
         ├── FIG 0/19 (Announcement switching) - IMPLEMENT
         ├── TS 104 089 (ASA protocol) - NEW
         └── TS 104 090 (Receiver requirements) - NEW
```

**Impact:** Cannot validate emergency alert compliance (Thai safety requirement)
**Priority:** P1-P2 (Important for government compliance)

---

## Release 1 Implementation Roadmap

### **Phase 1: Critical Distribution Networks (Weeks 1-4)**

#### **Week 1: Complete EN 300 799 (ETI) - 1 week**
- [ ] Fix remaining ETI-LI edge cases
- [ ] Add comprehensive ETI validation tests
- [ ] Complete documentation for ETI processing
- [ ] Verify 100% ETSI EN 300 799 compliance

**Files to Modify:**
- `src/core/eti_frame_parser.cpp`
- `tests/test_eti_frame_parser.cpp`
- `docs/ETI_PROTOCOL_SPECIFICATIONS.md`

#### **Weeks 2-4: Implement TS 102 693 (EDI) - 3 weeks**

**Week 2: EDI Protocol Parser**
- [ ] Create EDI packet structure definitions
- [ ] Implement AF (Application Format) parser
- [ ] Implement PFT (Protection/Fragmentation/Transport) parser
- [ ] Add TAG item decoder

**New Files:**
```cpp
src/core/edi_packet_parser.hpp
src/core/edi_packet_parser.cpp
include/edi_types.hpp
tests/test_edi_packet_parser.cpp
```

**Week 3: Network Transport Layer**
- [ ] Implement UDP multicast receiver
- [ ] Add packet reassembly logic
- [ ] Implement Forward Error Correction (FEC)
- [ ] Create EDI-to-ETI conversion layer

**New Files:**
```cpp
src/network/edi_network_receiver.hpp
src/network/edi_network_receiver.cpp
src/network/multicast_socket.cpp
tests/test_edi_network.cpp
```

**Week 4: EDI GUI Integration**
- [ ] Add network stream source to GUI
- [ ] Implement EDI stream monitoring panel
- [ ] Add network statistics display
- [ ] Create EDI configuration dialog

**Modified Files:**
```cpp
src/main.cpp                           // Add EDI source option
src/gui/network_stream_panel.hpp      // NEW
src/gui/network_stream_panel.cpp      // NEW
```

**Success Criteria:**
- ✅ Parse real EDI streams from Thai DAB network
- ✅ Display network statistics (packet loss, FEC corrections)
- ✅ Convert EDI to ETI for existing analysis pipeline
- ✅ Pass TS 102 693 compliance tests

---

### **Phase 2: Complete DAB System Standards (Weeks 5-9)**

#### **Week 5: FIG Type 0 Extensions - 1.5 weeks**
- [ ] Implement FIG 0/3 (Service component in packet mode)
- [ ] Implement FIG 0/5 (Service component language)
- [ ] Implement FIG 0/8 (Service component global definition)
- [ ] Implement FIG 0/9 (Country, LTO, International table)
- [ ] Implement FIG 0/10 (Date and time)

**Files to Modify:**
```cpp
src/core/fig0_processor.cpp            // Add new FIG 0 handlers
include/fig_types.hpp                  // Add new structures
tests/test_fig0_processor.cpp          // Add comprehensive tests
```

#### **Week 6: FIG Type 0 Advanced Features - 1 week**
- [ ] Implement FIG 0/13 (User application information)
- [ ] Implement FIG 0/17 (Programme Type)
- [ ] Implement FIG 0/18 (Announcement support) - COMPLETE
- [ ] Implement FIG 0/19 (Announcement switching) - NEW

**Files to Modify:**
```cpp
src/core/fig0_processor.cpp
include/announcement_types.hpp         // NEW
tests/test_fig0_announcements.cpp      // NEW
```

#### **Week 7: FIG Type 1 and 2 Extensions - 1.5 weeks**
- [ ] Implement FIG 1/5 (Data service label)
- [ ] Implement FIG 2/0 (Ensemble extended label)
- [ ] Implement FIG 2/1 (Service extended label)
- [ ] Implement FIG 2/4 (Service component extended label)
- [ ] Implement FIG 2/5 (Data service extended label)

**New Files:**
```cpp
src/core/fig2_processor.hpp            // NEW
src/core/fig2_processor.cpp            // NEW
tests/test_fig2_processor.cpp          // NEW
```

#### **Week 8: TS 101 756 (Registered Tables) - 1 week**
- [ ] Implement country code validation (Thailand = 0xE1)
- [ ] Add character set support (Unicode UTF-8 = 0x0F)
- [ ] Implement service type definitions
- [ ] Add programme type classifications
- [ ] Create language code tables

**New Files:**
```cpp
src/core/registered_tables.hpp         // NEW
src/core/registered_tables.cpp         // NEW
include/country_codes.hpp              // NEW
tests/test_registered_tables.cpp       // NEW
```

#### **Week 9: TS 103 176 (Service Information Rules) - 1 week**
- [ ] Implement service label encoding rules
- [ ] Add Dynamic Label Segment (DLS) validation
- [ ] Implement Programme Associated Data (PAD) parsing
- [ ] Add service linking validation
- [ ] Implement ensemble reconfiguration detection

**New Files:**
```cpp
src/core/service_info_rules.hpp        // NEW
src/core/service_info_rules.cpp        // NEW
src/core/dls_processor.cpp             // NEW
tests/test_service_info.cpp            // NEW
```

**Success Criteria:**
- ✅ Parse ALL FIG types used in Thai DAB broadcasts
- ✅ 95%+ EN 300 401 compliance score
- ✅ Complete service discovery with extended labels
- ✅ Pass TS 101 756 and TS 103 176 validation tests

---

### **Phase 3: Audio Coding Validation (Weeks 10-11)**

#### **Week 10: TS 102 563 Stream Validation - 1.5 weeks**
- [ ] Implement DAB+ superframe structure validation
- [ ] Add HE-AAC v2 profile detection
- [ ] Implement Reed-Solomon outer coding validation
- [ ] Add FireCode inner coding checks
- [ ] Create audio parameter extraction

**New Files:**
```cpp
src/core/dabplus_stream_validator.hpp  // NEW
src/core/dabplus_stream_validator.cpp  // NEW
tests/test_dabplus_validation.cpp      // NEW
```

**IMPORTANT:** This is STREAM VALIDATION only, NOT audio decoding.
Audio decoding uses fdk-aac library (already integrated).

#### **Week 11: Audio Quality Metrics - 0.5 weeks**
- [ ] Implement bitrate validation
- [ ] Add protection level checking
- [ ] Create audio quality indicators
- [ ] Add compliance reporting for TS 102 563

**Modified Files:**
```cpp
src/core/audio_parameter_validator.cpp // Expand existing
src/gui/audio_quality_panel.hpp        // NEW (optional)
```

**Success Criteria:**
- ✅ Validate DAB+ audio stream parameters
- ✅ 85%+ TS 102 563 compliance score
- ✅ Detect invalid encoding parameters
- ✅ Pass comprehensive audio validation tests

---

### **Phase 4: Emergency Warning System (Weeks 12-13)**

#### **Week 12: Announcement Framework - 1 week**
- [ ] Complete FIG 0/18 implementation
- [ ] Complete FIG 0/19 implementation
- [ ] Add alarm flag processing
- [ ] Implement announcement type detection
- [ ] Create announcement clustering

**Modified Files:**
```cpp
src/core/fig0_processor.cpp            // Complete 0/18 and 0/19
include/announcement_types.hpp         // NEW
tests/test_announcement_framework.cpp  // NEW
```

#### **Week 13: TS 104 089/090 EWS Support - 1 week**
- [ ] Implement ASA (Alert Signal Application) protocol
- [ ] Add emergency alert message structure
- [ ] Implement alert priority levels
- [ ] Add geographic targeting support
- [ ] Create EWS compliance validator

**New Files:**
```cpp
src/core/ews_protocol.hpp              // NEW (TS 104 089)
src/core/ews_protocol.cpp              // NEW
src/core/ews_receiver_validator.cpp    // NEW (TS 104 090)
tests/test_ews_protocol.cpp            // NEW
```

#### **Week 13: EWS GUI Integration**
- [ ] Add emergency alert indicators to GUI
- [ ] Implement alert notification system
- [ ] Create EWS monitoring panel
- [ ] Add alert history log

**New Files:**
```cpp
src/gui/ews_monitor_panel.hpp          // NEW
src/gui/ews_monitor_panel.cpp          // NEW
```

**Success Criteria:**
- ✅ Detect and process emergency alerts
- ✅ 90%+ EWS compliance score
- ✅ GUI displays emergency notifications
- ✅ Pass TS 104 089/090 validation tests

---

### **Phase 5: Data Services (Weeks 14-18) - OPTIONAL PRIORITY**

**Priority P1 (Weeks 14-16):**

#### **Week 14-15: EN 301 234 (MOT Protocol) - 2 weeks**
- [ ] Implement MOT directory parsing
- [ ] Add MOT header decoder
- [ ] Implement segment reassembly
- [ ] Create object carousel manager
- [ ] Add MOT object extraction

**New Files:**
```cpp
src/core/mot_protocol.hpp              // NEW
src/core/mot_protocol.cpp              // NEW
src/core/mot_object_carousel.cpp       // NEW
tests/test_mot_protocol.cpp            // NEW
```

#### **Week 16: TS 101 499 (SlideShow) - 1 week**
- [ ] Implement SlideShow MOT application
- [ ] Add JPEG image extraction
- [ ] Create SlideShow metadata parser
- [ ] Implement image display in GUI

**New Files:**
```cpp
src/core/slideshow_decoder.hpp         // NEW
src/core/slideshow_decoder.cpp         // NEW
src/gui/slideshow_panel.cpp            // NEW (optional)
tests/test_slideshow.cpp               // NEW
```

**Priority P2 (Weeks 17-18) - IF TIME PERMITS:**

#### **Week 17: TS 102 818 (EPG XML) - 1 week (optional)**
- [ ] Implement EPG XML schema parser
- [ ] Add programme information extraction
- [ ] Create schedule data manager

#### **Week 18: TS 103 551 (TPEG) & TS 102 980 (DL+) - 1 week (optional)**
- [ ] Implement TPEG message decoder
- [ ] Add Dynamic Label Plus parser

**Success Criteria (P1 only):**
- ✅ Extract MOT objects from data services
- ✅ Display SlideShow images
- ✅ 80%+ data services compliance score

---

## Future Releases Planning

### **Release 2: Extended Data Services & Professional Features (8-10 weeks)**

**Focus Areas:**
- Complete all data service standards (EPG Binary, TPEG, DL+, Journaline, Filecasting)
- Advanced receiver validation (TS 103 461 complete implementation)
- Multi-transmission mode support (Mode II/III/IV)
- Professional reporting features
- Cloud integration capabilities

**Standards to Add:**
- TS 102 371 (EPG Binary Format)
- TS 103 551 (TPEG Traffic Info) - completion
- TS 102 980 (Dynamic Label Plus) - completion
- TS 102 979 (Journaline)
- TS 103 177 (Filecasting)
- TS 103 689 (Filtered Information Service)
- TS 102 367 (Conditional Access) - optional

### **Release 3: Advanced Features & AI Integration (6-8 weeks)**

**Focus Areas:**
- Multi-stream comparative analysis
- AI-powered signal intelligence
- Advanced analytics and reporting
- Thai NBTC-specific compliance validation
- Performance optimization (7,482+ FPS target)

**Standards to Add:**
- Complete TS 103 461 receiver testing framework
- Additional Thai regulatory requirements
- Custom NBTC compliance validators

---

## Testing & Validation Strategy

### **Unit Testing Requirements**

Each new standard implementation MUST include:
- [ ] Comprehensive unit tests (90%+ code coverage)
- [ ] ETSI reference data validation
- [ ] Negative test cases (invalid data handling)
- [ ] Performance benchmarks

**Test Framework:**
```cpp
// tests/test_STANDARD_NAME.cpp
TEST_CASE("StandardName - Basic Parsing", "[standard]") {
    // Test with ETSI reference data
}

TEST_CASE("StandardName - Error Handling", "[standard][negative]") {
    // Test invalid data handling
}

TEST_CASE("StandardName - Performance", "[standard][benchmark]") {
    // Benchmark processing speed
}
```

### **Integration Testing**

- [ ] End-to-end ETI stream processing
- [ ] Real Thai DAB broadcast validation
- [ ] Network EDI stream testing
- [ ] Multi-standard cross-validation

### **Compliance Validation**

Each Release 1 standard MUST achieve:
- **Mandatory (✅) Standards:** ≥ 95% compliance score
- **Recommended (⚪) Standards:** ≥ 85% compliance score

**Validation Process:**
1. Run ETSI reference stream tests
2. Validate against Thai DAB real broadcasts
3. Generate compliance reports
4. Fix any critical compliance issues
5. Re-test until compliance targets met

---

## Success Criteria

### **Release 1 Completion Criteria**

#### **Mandatory Requirements (MUST HAVE)**
- ✅ **100% implementation** of all ✅-marked standards from THAIDAB_REFERENCE_COMPAEISON.md
- ✅ **≥ 95% ETSI compliance** for mandatory standards
- ✅ **Zero critical compliance violations** on Thai DAB test streams
- ✅ **EDI network support** functional and tested
- ✅ **Emergency Warning System** detection and processing
- ✅ **All essential FIG types** (0/0-0/19, 1/0-1/5, 2/0-2/5) implemented
- ✅ **DAB+ audio stream validation** (NOT decoding, uses fdk-aac)
- ✅ **Comprehensive test suite** with 90%+ code coverage

#### **Recommended Requirements (SHOULD HAVE)**
- ✅ **MOT and SlideShow** support (EN 301 234, TS 101 499)
- ⚪ **EPG support** (TS 102 818) - if time permits
- ⚪ **TPEG support** (TS 103 551) - if time permits

#### **Quality Metrics**
- ✅ **Build:** Zero compilation errors/warnings
- ✅ **Performance:** >1000 FPS ETI processing maintained
- ✅ **Stability:** No crashes during 24-hour continuous operation
- ✅ **Documentation:** All new standards fully documented
- ✅ **Code Quality:** A+ grade (≥99/100) from code review

### **Release 1 Deliverables**

1. **Source Code:**
   - All new standard implementations
   - Comprehensive unit and integration tests
   - Updated build system (CMake)

2. **Documentation:**
   - Updated ETSI_COMPLIANCE_FRAMEWORK.md
   - Individual standard implementation guides
   - API documentation for new components
   - User guide for new features

3. **Testing:**
   - ETSI reference data test suite
   - Thai DAB real broadcast test results
   - Performance benchmark results
   - Compliance validation reports

4. **GUI Updates:**
   - EDI network stream source
   - Emergency warning indicators
   - Data service panels (MOT/SlideShow)
   - Enhanced compliance monitoring

---

## Timeline Summary

### **Release 1 Total Duration: 12-14 weeks**

| Phase | Duration | Standards | Priority |
|-------|----------|-----------|----------|
| **Phase 1: Distribution Networks** | 4 weeks | EN 300 799, TS 102 693 | **P0 CRITICAL** |
| **Phase 2: DAB System** | 5 weeks | EN 300 401, TS 101 756, TS 103 176 | **P0 CRITICAL** |
| **Phase 3: Audio Coding** | 2 weeks | TS 102 563 | **P0 CRITICAL** |
| **Phase 4: Emergency Warnings** | 2 weeks | EN 300 401, TS 104 089/090 | **P1 HIGH** |
| **Phase 5: Data Services (P1 only)** | 3 weeks | EN 301 234, TS 101 499 | **P1 HIGH** |

**Total:** 16 weeks (can be reduced to 12-14 weeks by parallel development)

### **Parallel Development Strategy**

To achieve 12-14 week timeline:
- **Weeks 1-4:** Phase 1 (EDI) - Developer A
- **Weeks 1-5:** Phase 2 (FIG extensions) - Developer B
- **Weeks 6-7:** Phase 3 (Audio validation) - Developer A
- **Weeks 6-8:** Phase 4 (EWS) - Developer B
- **Weeks 9-11:** Phase 5 (Data services P1) - Both developers

---

## Appendix A: Standards Reference Matrix

### **Mandatory Standards (✅) for Release 1**

| Standard | Version | Document | Mandatory For | Implementation Status |
|----------|---------|----------|---------------|----------------------|
| ETS 300 799 | V1.0 (1997) | ETI Distribution Interface | Transmitter + Receiver | 95% Complete |
| TS 102 693 | V1.1.2 (2011) | EDI Enhanced Distribution | Transmitter + Receiver | 0% - CRITICAL |
| EN 300 401 | V2.1.1 (2017-01) | DAB System Standard | Transmitter + Receiver | 80% Complete |
| TS 101 756 | V2.4.1 (2020-08) | Registered Tables | Transmitter + Receiver | 70% Complete |
| TS 103 176 | V2.4.1 (2020-08) | Service Information Rules | Transmitter + Receiver | 60% Complete |
| TS 102 563 | V2.1.1 (2017-01) | DAB+ Audio Coding | Transmitter + Receiver | 30% Complete |
| EN 302 077 | V2.3.1 (2022-09) | Transmitting Equipment | Transmitter ONLY | N/A (not analyzer focus) |

### **Optional Standards (⚪) for Release 1**

| Standard | Version | Implementation Priority | Effort |
|----------|---------|------------------------|--------|
| EN 301 234 | V2.1.1 (2017) | **P1 HIGH** | 2 weeks |
| TS 101 499 | V3.2.1 (2023-07) | **P1 HIGH** | 1 week |
| TR 101 496-3 | V1.1.2 (2001-05) | **P2 MEDIUM** | 1 week |
| TS 104 089 | V1.1.1 (2024-09) | **P2 MEDIUM** | 2 weeks |
| TS 104 090 | V1.1.2 (2024) | **P2 MEDIUM** | 1 week |
| TS 102 818 | V3.3.1 (2020-08) | **P2 MEDIUM** | 2 weeks |
| TS 102 371 | V3.3.1 (2023) | **P3 LOW** | 1 week |
| TS 103 551 | V1.1.1 (2017-08) | **P2 MEDIUM** | 2 weeks |
| TS 102 980 | V2.1.2 (2024) | **P3 LOW** | 1 week |
| TS 103 461 | V1.2.2 (2020-10) | **P2 MEDIUM** | 1 week |

---

## Appendix B: Code Organization

### **Proposed Directory Structure for Release 1**

```
src/
├── core/
│   ├── eti_frame_parser.cpp               # EXISTING - enhance
│   ├── fig0_processor.cpp                 # EXISTING - extend
│   ├── fig1_processor.cpp                 # EXISTING - extend
│   ├── fig2_processor.cpp                 # NEW - Release 1
│   ├── edi_packet_parser.cpp              # NEW - Release 1
│   ├── dabplus_stream_validator.cpp       # NEW - Release 1
│   ├── mot_protocol.cpp                   # NEW - Release 1
│   ├── slideshow_decoder.cpp              # NEW - Release 1
│   ├── ews_protocol.cpp                   # NEW - Release 1
│   ├── registered_tables.cpp              # NEW - Release 1
│   ├── service_info_rules.cpp             # NEW - Release 1
│   ├── dls_processor.cpp                  # NEW - Release 1
│   └── etsi_compliance_engine.cpp         # EXISTING - extend
│
├── network/
│   ├── edi_network_receiver.cpp           # NEW - Release 1
│   ├── multicast_socket.cpp               # NEW - Release 1
│   └── network_statistics.cpp             # NEW - Release 1
│
├── gui/
│   ├── network_stream_panel.cpp           # NEW - Release 1
│   ├── ews_monitor_panel.cpp              # NEW - Release 1
│   ├── slideshow_panel.cpp                # NEW - Release 1 (optional)
│   └── etsi_compliance_monitor.cpp        # EXISTING - extend
│
├── etsi/                                  # EXISTING directory
│   ├── compliance_engine.cpp              # EXISTING - extend
│   ├── standard_validators.cpp            # EXISTING - extend
│   └── compliance_reporter.cpp            # EXISTING - extend
│
└── utils/
    └── report_generator.cpp               # EXISTING - extend
```

---

## Document Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | October 19, 2025 | Development Team | Initial Release 1 implementation plan based on THAIDAB reference and code assessment |

---

**Next Steps:**
1. Review this plan with stakeholders
2. Prioritize Phase 1-4 (mandatory) vs Phase 5 (optional)
3. Assign developers to parallel work streams
4. Begin Phase 1: Week 1 implementation
5. Set up bi-weekly progress reviews

**Document Status:** READY FOR REVIEW
