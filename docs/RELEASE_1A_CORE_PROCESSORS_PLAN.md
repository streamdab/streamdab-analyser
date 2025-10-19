# Release 1A: Core Processors Implementation Plan (65% → 100%)

**Document Version:** 1.0
**Date:** October 19, 2025
**Phase:** Core Processing Implementation (Backend Focus)
**Current Status:** 65% Complete
**Target:** 100% Complete
**Timeline:** 10-12 weeks

---

## Executive Summary

Release 1A focuses exclusively on **completing core processing engines** from 65% to 100% compliance with Thai DAB standards. This phase prioritizes backend implementation over GUI features, establishing a solid foundation that can be integrated into the UI in Release 1B.

### Strategy: "Processors First, UI Later"

**Rationale:**
- ✅ Core processors can be developed and tested independently
- ✅ Multiple processors can be developed in parallel
- ✅ GUI integration becomes straightforward with complete backends
- ✅ Code can reference ODR-DabMux implementations where applicable
- ✅ Faster path to 100% functionality (defer UI polish to 1B)

### Key Reference: ODR-DabMux Codebase

Location: `/home/seksan/workspace/ODR-DabMux`

**Available Reference Implementations:**
- ✅ **FIG Processing:** `/src/fig/FIG0_*.cpp` - ALL FIG types implemented
- ✅ **EDI Output:** `/lib/edioutput/` - Complete EDI protocol stack
- ✅ **EWS System:** `/src/ews/` - AlertManager, EWSTypes, GeometryConverter
- ✅ **MUX Elements:** `/src/MuxElements.cpp` - Service and ensemble structures

**Reference Strategy:**
- 📖 Study ODR-DabMux structures and algorithms
- 🔄 Adapt for **analyzer** use case (parsing vs generating)
- ✅ Maintain compatibility with our existing architecture
- ⚠️ Note: ODR-DabMux generates ETI, we analyze/parse it

---

## Table of Contents

1. [Current State Assessment](#current-state-assessment)
2. [Phase 1A: Complete ETI/EDI Processing (Weeks 1-4)](#phase-1a-complete-etietsi-processing-weeks-1-4)
3. [Phase 2A: Complete FIG Processing (Weeks 5-7)](#phase-2a-complete-fig-processing-weeks-5-7)
4. [Phase 3A: Audio & Data Services (Weeks 8-9)](#phase-3a-audio--data-services-weeks-8-9)
5. [Phase 4A: Emergency Warning System (Week 10)](#phase-4a-emergency-warning-system-week-10)
6. [Phase 5A: Testing & Validation (Weeks 11-12)](#phase-5a-testing--validation-weeks-11-12)
7. [Success Criteria](#success-criteria)
8. [Integration Points for Release 1B](#integration-points-for-release-1b)

---

## Current State Assessment

### ✅ **What Works (65% - Strong Foundation)**

| Component | Completion | Files | Status |
|-----------|------------|-------|--------|
| **ETI-NI/LI Parsing** | 95% | `src/core/eti_frame_parser.cpp` | Excellent quality |
| **FIG 0/0, 0/1, 0/2** | 90% | `src/core/fig0_processor.cpp` | Functional |
| **FIG 1/0, 1/1, 1/4** | 85% | `src/core/fig1_processor.cpp` | Functional |
| **ETSI Compliance Engine** | 70% | `src/core/etsi_compliance_engine.cpp` | Good framework |
| **Service Discovery** | 80% | Integrated in FIG processors | Working |
| **CRC Validation** | 90% | ETI frame parser | Solid |

### ❌ **Critical Gaps (35% to Complete)**

| Gap | Priority | Completion | ODR-DabMux Reference | Effort |
|-----|----------|------------|---------------------|--------|
| **EDI Network Protocol** | P0 | 0% | `/lib/edioutput/*` | 3 weeks |
| **FIG 0/3-0/19 (missing)** | P0 | 0% | `/src/fig/FIG0_*.cpp` | 2 weeks |
| **FIG 1/5, FIG 2/x** | P0 | 0% | `/src/fig/FIG1.cpp`, `/src/fig/FIG2.h` | 1 week |
| **DAB+ Audio Validation** | P0 | 30% | - | 1 week |
| **MOT Protocol** | P1 | 10% | - | 2 weeks |
| **SlideShow Decoder** | P1 | 5% | - | 1 week |
| **EWS Alert Processing** | P1 | 0% | `/src/ews/*` | 1 week |

---

## Phase 1A: Complete ETI/EDI Processing (Weeks 1-4)

### **Objective:** 100% Distribution Network Standards Compliance

**Target Standards:**
- ✅ EN 300 799 (ETI) - 95% → 100%
- ✅ TS 102 693 (EDI) - 0% → 95%

### **Week 1: Complete EN 300 799 (ETI) - FINAL 5%**

**Tasks:**
- [ ] Fix remaining ETI-LI edge cases (Pattern A/B variations)
- [ ] Add ETI-NA (Network Adapted) format support (if needed for Thai DAB)
- [ ] Complete TIST timestamp processing
- [ ] Add comprehensive error recovery mechanisms
- [ ] Finalize CRC validation for all frame types

**New/Modified Files:**
```cpp
src/core/eti_frame_parser.cpp              // Enhance existing
include/eti_types.hpp                      // Add ETI-NA structures
tests/test_eti_frame_parser.cpp            // Add edge case tests
```

**Deliverables:**
- ✅ 100% EN 300 799 compliance score
- ✅ Pass all ETSI reference ETI streams
- ✅ Zero critical errors on Thai DAB test files

### **Weeks 2-4: Implement TS 102 693 (EDI Protocol)**

**Reference:** `/home/seksan/workspace/ODR-DabMux/lib/edioutput/`

**ODR-DabMux Components to Study:**
- `AFPacket.cpp` - AF (Application Format) packet structure
- `PFT.cpp` - Protection, Fragmentation, Transport layer
- `TagItems.cpp` - TAG item encoding/decoding
- `TagPacket.cpp` - TAG packet structure
- `Transport.cpp` - UDP/TCP transport layer
- `EDIConfig.h` - Configuration structures

**Implementation Strategy:**

**Week 2: EDI Protocol Parser (Reverse Engineering)**

Since ODR-DabMux *generates* EDI, we need to create *parser* for analyzer:

```cpp
// NEW: src/network/edi_packet_parser.hpp
class EDIPacketParser {
public:
    // Parse incoming EDI packet (reverse of ODR-DabMux output)
    bool parseAFPacket(const std::vector<uint8_t>& data);
    bool parsePFTPacket(const std::vector<uint8_t>& data);
    bool parseTAGPacket(const std::vector<uint8_t>& data);

    // Extract ETI from EDI
    std::optional<ETIFrame> extractETIFrame();

    // TAG item parsers
    struct TAGItem {
        std::string tag_name;  // e.g., "deti", "est*", "*ptr"
        std::vector<uint8_t> data;
    };
    std::vector<TAGItem> extractTAGItems(const std::vector<uint8_t>& tag_packet);
};
```

**Tasks Week 2:**
- [ ] Create EDI packet structure definitions (study ODR-DabMux headers)
- [ ] Implement AF packet parser (reverse of `AFPacket.cpp`)
- [ ] Implement TAG item decoder (study `TagItems.cpp` encoding logic)
- [ ] Add TAG packet reassembly
- [ ] Create unit tests with ODR-DabMux generated EDI samples

**Week 3: PFT Layer & Network Transport**

```cpp
// NEW: src/network/pft_reassembler.hpp
class PFTReassembler {
public:
    // Protection, Fragmentation, Transport (reverse of ODR-DabMux PFT.cpp)
    struct PFTFragment {
        uint16_t findex;      // Fragment index
        uint16_t fcount;      // Total fragments
        uint32_t pseq;        // PFT sequence number
        std::vector<uint8_t> payload;
    };

    bool addFragment(const PFTFragment& fragment);
    std::optional<std::vector<uint8_t>> getReassembledPacket();

    // Forward Error Correction (if PFT FEC enabled)
    bool performFECRecovery();
};

// NEW: src/network/edi_network_receiver.hpp
class EDINetworkReceiver {
public:
    // UDP Multicast receiver for EDI streams
    bool connectToMulticast(const std::string& address, uint16_t port);
    bool receivePacket(std::vector<uint8_t>& packet);

    // Network statistics
    struct NetworkStats {
        uint64_t packets_received;
        uint64_t packets_lost;
        uint64_t fec_corrections;
        double packet_loss_rate;
    };
    NetworkStats getStatistics() const;
};
```

**Tasks Week 3:**
- [ ] Implement PFT fragment reassembly (study `PFT.cpp` fragmentation)
- [ ] Add Reed-Solomon FEC decoder (if needed)
- [ ] Create UDP multicast receiver (use Qt QUdpSocket or raw sockets)
- [ ] Implement packet buffering and sequencing
- [ ] Add network statistics tracking

**Week 4: EDI-to-ETI Conversion & Integration**

```cpp
// NEW: src/core/edi_to_eti_converter.hpp
class EDIToETIConverter {
public:
    // Convert EDI stream to ETI frames for existing pipeline
    std::optional<ETIFrame> convertEDItoETI(const std::vector<TAGItem>& tag_items);

    // Extract components from EDI TAG items
    struct EDIComponents {
        std::vector<uint8_t> fic_data;   // From TAG *ptr
        std::vector<uint8_t> msc_data;   // From TAG deti
        uint32_t timestamp;              // From TAG est*
        uint16_t frame_count;
    };
    EDIComponents extractComponents(const std::vector<TAGItem>& tags);
};
```

**Tasks Week 4:**
- [ ] Implement EDI TAG item to ETI field mapping
- [ ] Create EDI-to-ETI converter
- [ ] Integrate with existing ETI processing pipeline
- [ ] Test end-to-end: Network → EDI parse → ETI → FIG analysis
- [ ] Add comprehensive EDI validation tests

**New Files Created (Weeks 2-4):**
```
src/network/
├── edi_packet_parser.hpp
├── edi_packet_parser.cpp
├── pft_reassembler.hpp
├── pft_reassembler.cpp
├── edi_network_receiver.hpp
├── edi_network_receiver.cpp
└── multicast_socket.cpp

src/core/
├── edi_to_eti_converter.hpp
└── edi_to_eti_converter.cpp

include/
└── edi_types.hpp

tests/
├── test_edi_packet_parser.cpp
├── test_pft_reassembler.cpp
├── test_edi_network.cpp
└── test_edi_to_eti_converter.cpp
```

**Phase 1A Deliverables:**
- ✅ 100% EN 300 799 (ETI) compliance
- ✅ 95% TS 102 693 (EDI) compliance
- ✅ Parse real EDI streams from Thai DAB network
- ✅ Convert EDI to ETI for downstream processing
- ✅ Network statistics and monitoring
- ⚠️ Note: GUI integration deferred to Release 1B

---

## Phase 2A: Complete FIG Processing (Weeks 5-7)

### **Objective:** 100% EN 300 401 FIG Coverage

**Target:** All FIG types used in Thai DAB broadcasts

**Reference:** `/home/seksan/workspace/ODR-DabMux/src/fig/`

### **Week 5: FIG Type 0 Extensions (0/3 through 0/10)**

**ODR-DabMux References:**
- `FIG0_3.cpp` - Service component in packet mode
- `FIG0_5.cpp` - Service component language
- `FIG0_6.cpp` - Service linking
- `FIG0_7.cpp` - Configuration information
- `FIG0_8.cpp` - Service component global definition
- `FIG0_9.cpp` - Country, LTO, International table
- `FIG0_10.cpp` - Date and time

**Implementation Approach:**

Study ODR-DabMux *encoding* logic → Create *parsing* logic

```cpp
// MODIFY: src/core/fig0_processor.cpp

// Add missing FIG 0 handlers
class FIG0Processor {
public:
    // NEW: FIG 0/3 - Service component in packet mode
    struct FIG0_3_ServiceComponent {
        uint16_t service_id;
        uint8_t subchannel_id;
        uint16_t component_id;
        bool ca_flag;
    };
    std::vector<FIG0_3_ServiceComponent> parseFIG0_3(const std::vector<uint8_t>& data);

    // NEW: FIG 0/5 - Service component language
    struct FIG0_5_Language {
        uint16_t service_id;
        uint8_t component_id;
        uint8_t language_code;  // ISO 639-2
    };
    std::vector<FIG0_5_Language> parseFIG0_5(const std::vector<uint8_t>& data);

    // NEW: FIG 0/8 - Service component global definition
    struct FIG0_8_GlobalDefinition {
        uint16_t service_id;
        uint8_t component_id_within_service;
        bool is_dab_plus;
        uint8_t subchannel_id;
    };
    std::vector<FIG0_8_GlobalDefinition> parseFIG0_8(const std::vector<uint8_t>& data);

    // NEW: FIG 0/9 - Country, LTO, International table
    struct FIG0_9_CountryLTO {
        uint8_t ensemble_lto;     // Local Time Offset
        uint8_t ensemble_ecc;     // Extended Country Code
        uint8_t international_table_id;
    };
    FIG0_9_CountryLTO parseFIG0_9(const std::vector<uint8_t>& data);

    // NEW: FIG 0/10 - Date and time (MJD format)
    struct FIG0_10_DateTime {
        uint32_t mjd;             // Modified Julian Date
        uint8_t hours;
        uint8_t minutes;
        uint8_t seconds;
        uint16_t milliseconds;
    };
    FIG0_10_DateTime parseFIG0_10(const std::vector<uint8_t>& data);
};
```

**Tasks Week 5:**
- [ ] Implement FIG 0/3 parser (study `FIG0_3.cpp`)
- [ ] Implement FIG 0/5 parser (study `FIG0_5.cpp`)
- [ ] Implement FIG 0/6 parser (service linking)
- [ ] Implement FIG 0/7 parser (configuration)
- [ ] Implement FIG 0/8 parser (study `FIG0_8.cpp`)
- [ ] Implement FIG 0/9 parser (study `FIG0_9.cpp`)
- [ ] Implement FIG 0/10 parser (study `FIG0_10.cpp`)
- [ ] Add unit tests for each FIG type

### **Week 6: FIG Type 0 Advanced (0/13, 0/14, 0/17, 0/18, 0/19)**

**ODR-DabMux References:**
- `FIG0_13.cpp` - User application information
- `FIG0_14.cpp` - FEC sub-channel organization
- `FIG0_17.cpp` - Programme Type
- `FIG0_18.cpp` - Announcement support
- `FIG0_19.cpp` - Announcement switching **← EWS CRITICAL**

```cpp
// MODIFY: src/core/fig0_processor.cpp

// NEW: FIG 0/13 - User application information
struct FIG0_13_UserApp {
    uint16_t service_id;
    uint8_t component_id;
    uint16_t user_app_type;  // e.g., 0x001 = SPI, 0x002 = MOT Slideshow
    std::vector<uint8_t> user_app_data;
};
std::vector<FIG0_13_UserApp> parseFIG0_13(const std::vector<uint8_t>& data);

// NEW: FIG 0/17 - Programme Type
struct FIG0_17_ProgrammeType {
    uint16_t service_id;
    bool static_dynamic;
    uint8_t international_code;
    std::vector<uint8_t> language_codes;
};
std::vector<FIG0_17_ProgrammeType> parseFIG0_17(const std::vector<uint8_t>& data);

// ENHANCE: FIG 0/18 - Announcement support (already exists, complete it)
struct FIG0_18_AnnouncementSupport {
    uint16_t service_id;
    uint16_t announcement_support_flags;  // 16 bits, each = announcement type
    uint8_t cluster_id;
};
std::vector<FIG0_18_AnnouncementSupport> parseFIG0_18(const std::vector<uint8_t>& data);

// NEW: FIG 0/19 - Announcement switching **CRITICAL FOR EWS**
struct FIG0_19_AnnouncementSwitching {
    uint8_t cluster_id;
    uint16_t announcement_flags;  // Active announcements
    bool new_flag;
    bool region_flag;
    uint8_t subchannel_id;
    uint16_t region_id;
};
std::vector<FIG0_19_AnnouncementSwitching> parseFIG0_19(const std::vector<uint8_t>& data);
```

**Tasks Week 6:**
- [ ] Implement FIG 0/13 parser (study `FIG0_13.cpp`)
- [ ] Implement FIG 0/14 parser (study `FIG0_14.cpp`)
- [ ] Implement FIG 0/17 parser (study `FIG0_17.cpp`)
- [ ] **Complete FIG 0/18** parser (study `FIG0_18.cpp`)
- [ ] **Implement FIG 0/19** parser (study `FIG0_19.cpp`) **← EWS FOUNDATION**
- [ ] Add comprehensive tests
- [ ] Validate against Thai DAB broadcasts

### **Week 7: FIG Type 1 & 2 (Extended Labels)**

**ODR-DabMux References:**
- `FIG1.cpp` - All FIG Type 1 implementations
- `FIG2.h` - FIG Type 2 definitions

```cpp
// MODIFY: src/core/fig1_processor.cpp

// NEW: FIG 1/5 - Data service label
struct FIG1_5_DataServiceLabel {
    uint32_t service_id;
    char label[16];
    uint16_t character_flag;
};
std::vector<FIG1_5_DataServiceLabel> parseFIG1_5(const std::vector<uint8_t>& data);

// NEW: src/core/fig2_processor.hpp
class FIG2Processor {
public:
    // FIG 2/0 - Ensemble extended label
    struct FIG2_0_EnsembleLabel {
        uint16_t ensemble_id;
        uint8_t toggle_flag;
        uint8_t segment_index;
        std::vector<uint8_t> label_segment;  // UTF-8 extended label
    };

    // FIG 2/1 - Service extended label
    struct FIG2_1_ServiceLabel {
        uint16_t service_id;
        uint8_t toggle_flag;
        uint8_t segment_index;
        std::vector<uint8_t> label_segment;
    };

    // FIG 2/4 - Service component extended label
    struct FIG2_4_ComponentLabel {
        uint16_t service_id;
        uint8_t component_id;
        uint8_t toggle_flag;
        uint8_t segment_index;
        std::vector<uint8_t> label_segment;
    };

    // FIG 2/5 - Data service extended label
    struct FIG2_5_DataLabel {
        uint32_t service_id;
        uint8_t toggle_flag;
        uint8_t segment_index;
        std::vector<uint8_t> label_segment;
    };

    // Reassemble segmented labels
    std::string reassembleExtendedLabel(const std::vector<std::vector<uint8_t>>& segments);
};
```

**Tasks Week 7:**
- [ ] Implement FIG 1/5 parser (study `FIG1.cpp`)
- [ ] Create FIG2Processor class
- [ ] Implement FIG 2/0, 2/1, 2/4, 2/5 parsers
- [ ] Add label segment reassembly logic
- [ ] Add UTF-8 extended label support (Thai characters!)
- [ ] Integrate with service discovery
- [ ] Test with Thai DAB extended labels

**Phase 2A Deliverables:**
- ✅ 95% EN 300 401 (DAB System) compliance
- ✅ All FIG types 0/0 through 0/24 parsed
- ✅ All FIG type 1 variants (1/0-1/5) complete
- ✅ All FIG type 2 variants (2/0, 2/1, 2/4, 2/5) complete
- ✅ Thai character support in extended labels
- ✅ Foundation for EWS (FIG 0/18-19)

---

## Phase 3A: Audio & Data Services (Weeks 8-9)

### **Week 8: DAB+ Audio Stream Validation**

**Target:** TS 102 563 (DAB+ Audio Coding) - 30% → 85%

**Important Note:** This is STREAM VALIDATION, not decoding (uses fdk-aac library)

```cpp
// NEW: src/core/dabplus_stream_validator.hpp
class DABPlusStreamValidator {
public:
    // TS 102 563 Audio Superframe Validation
    struct AudioSuperframe {
        std::vector<uint8_t> superframe_data;  // 120ms of audio
        uint16_t bitrate_kbps;
        uint8_t protection_level;
        bool sbr_flag;          // Spectral Band Replication
        bool ps_flag;           // Parametric Stereo
        uint8_t num_aus;        // Audio Access Units

        // Reed-Solomon coding
        struct RSCoding {
            uint8_t rs_k;       // Data length
            uint8_t rs_z;       // Parity length
            bool valid_crc;
        };
        RSCoding rs_coding;

        // FireCode inner coding
        struct FireCode {
            bool valid;
            uint16_t crc;
        };
        FireCode firecode;
    };

    // Validate audio stream parameters
    ValidationResult validateAudioSuperframe(const AudioSuperframe& superframe);
    ValidationResult validateHEAACProfile(const std::vector<uint8_t>& audio_data);
    ValidationResult validateBitrateCompliance(uint16_t bitrate, uint8_t protection);

    // Extract audio metrics (not decoding!)
    struct AudioMetrics {
        uint16_t bitrate_kbps;
        uint8_t sample_rate_khz;
        uint8_t channels;
        bool is_stereo_ps;
        double quality_estimate;  // Based on bitrate/protection
    };
    AudioMetrics extractMetrics(const AudioSuperframe& superframe);
};
```

**Tasks Week 8:**
- [ ] Implement DAB+ superframe structure parser
- [ ] Add HE-AAC v2 profile detection (from audio headers)
- [ ] Implement Reed-Solomon validation (RS(120,110))
- [ ] Implement FireCode CRC validation
- [ ] Add bitrate and protection level validation
- [ ] Extract audio quality metrics
- [ ] Create TS 102 563 compliance validator
- [ ] Test with Thai DAB+ audio streams

### **Week 9: MOT Protocol & SlideShow**

**Target:**
- EN 301 234 (MOT) - 10% → 80%
- TS 101 499 (SlideShow) - 5% → 75%

```cpp
// NEW: src/core/mot_protocol.hpp
class MOTProtocol {
public:
    // MOT Header parsing (EN 301 234)
    struct MOTHeader {
        uint16_t body_size;
        uint16_t header_size;
        uint8_t content_type;           // e.g., 0x02 = JPEG
        uint8_t content_subtype;
        std::string content_name;
        uint32_t trigger_time;
        std::map<uint8_t, std::vector<uint8_t>> parameters;
    };
    MOTHeader parseMOTHeader(const std::vector<uint8_t>& data);

    // MOT Directory parsing
    struct MOTDirectory {
        uint16_t num_objects;
        struct DirectoryEntry {
            uint32_t transport_id;
            uint16_t header_size;
            uint32_t body_size;
        };
        std::vector<DirectoryEntry> entries;
    };
    MOTDirectory parseMOTDirectory(const std::vector<uint8_t>& data);

    // MOT Segment reassembly
    class SegmentReassembler {
    public:
        bool addSegment(uint32_t transport_id, uint16_t segment_number,
                       const std::vector<uint8_t>& segment_data);
        std::optional<std::vector<uint8_t>> getCompleteObject(uint32_t transport_id);
        double getProgress(uint32_t transport_id) const;
    };

    // MOT Object carousel manager
    class ObjectCarousel {
    public:
        void addObject(uint32_t transport_id, const MOTHeader& header,
                      const std::vector<uint8_t>& body);
        std::vector<uint32_t> getActiveObjects() const;
        std::optional<MOTHeader> getObjectHeader(uint32_t transport_id) const;
    };
};

// NEW: src/core/slideshow_decoder.hpp
class SlideShowDecoder {
public:
    // TS 101 499 SlideShow specific
    struct SlideShowImage {
        std::vector<uint8_t> jpeg_data;
        std::string category_title;
        std::string click_through_url;
        std::string alt_location_url;
        uint32_t show_time_ms;
    };

    std::optional<SlideShowImage> decodeSlideShow(const MOTProtocol::MOTHeader& header,
                                                  const std::vector<uint8_t>& body);

    // Save JPEG to file for later GUI display
    bool saveJPEG(const SlideShowImage& image, const std::string& filename);
};
```

**Tasks Week 9:**
- [ ] Implement MOT header parser
- [ ] Implement MOT directory parser
- [ ] Create segment reassembly engine
- [ ] Implement object carousel manager
- [ ] Create SlideShow JPEG extractor
- [ ] Add MOT object storage/caching
- [ ] Test with Thai DAB SlideShow broadcasts
- [ ] **Output:** Extracted JPEG files ready for GUI display (Release 1B)

**Phase 3A Deliverables:**
- ✅ 85% TS 102 563 (DAB+ Audio) compliance
- ✅ 80% EN 301 234 (MOT) compliance
- ✅ 75% TS 101 499 (SlideShow) compliance
- ✅ Audio stream validation functional
- ✅ SlideShow JPEG extraction working
- ⚠️ GUI display deferred to Release 1B

---

## Phase 4A: Emergency Warning System (Week 10)

### **Objective:** EWS Backend Processing Complete

**Reference:** `/home/seksan/workspace/ODR-DabMux/src/ews/`

**ODR-DabMux Components:**
- `AlertManager.cpp` - Alert lifecycle management
- `EWSTypes.cpp` - Alert structure definitions
- `GeometryConverter.cpp` - Geographic area handling

**Target Standards:**
- FIG 0/18-19 (Announcement) - 40% → 90%
- TS 104 089 (EWS ASA) - 0% → 70%
- TS 104 090 (EWS Receiver) - 0% → 70%

```cpp
// NEW: src/core/ews_alert_processor.hpp
class EWSAlertProcessor {
public:
    // Based on ODR-DabMux AlertManager but for PARSING
    struct Alert {
        uint32_t alert_id;
        std::string alert_type;        // e.g., "Earthquake", "Flood"
        uint8_t severity;              // 0=Minor, 1=Moderate, 2=Severe, 3=Extreme
        uint8_t urgency;
        uint8_t certainty;

        std::string headline;
        std::string description;
        std::string instruction;

        std::chrono::system_clock::time_point onset_time;
        std::chrono::system_clock::time_point expires_time;

        // Geographic targeting (study GeometryConverter.cpp)
        struct GeographicArea {
            std::string area_desc;
            std::vector<std::pair<double, double>> polygon;  // lat/lon pairs
            std::vector<std::string> geocode_references;
        };
        std::vector<GeographicArea> areas;

        // Link to announcement switching
        uint8_t cluster_id;        // From FIG 0/19
        uint8_t subchannel_id;     // Emergency audio subchannel
    };

    // Parse TS 104 089 ASA protocol
    std::vector<Alert> parseASAMessage(const std::vector<uint8_t>& asa_data);

    // Integrate with FIG 0/18-19
    void linkAnnouncementToAlert(const FIG0_19_AnnouncementSwitching& announcement);

    // Alert state management (study AlertManager.cpp)
    enum class AlertState {
        ACTIVE,
        UPDATED,
        EXPIRED,
        CANCELLED
    };
    std::vector<Alert> getActiveAlerts() const;
    AlertState getAlertState(uint32_t alert_id) const;
};
```

**Tasks Week 10:**
- [ ] Study ODR-DabMux EWS implementation (`/src/ews/*`)
- [ ] Create Alert structure (adapt from `EWSTypes.cpp`)
- [ ] Implement TS 104 089 ASA protocol parser
- [ ] Integrate with FIG 0/18-19 announcement detection
- [ ] Add geographic area parsing (study `GeometryConverter.cpp`)
- [ ] Implement alert lifecycle state machine
- [ ] Add alert priority and filtering
- [ ] Test with Thai EWS test broadcasts
- [ ] **Output:** Alert data ready for GUI notification (Release 1B)

**Phase 4A Deliverables:**
- ✅ 90% Announcement processing (FIG 0/18-19)
- ✅ 70% TS 104 089 (ASA Protocol) compliance
- ✅ Alert parsing and state management functional
- ✅ Geographic targeting support
- ⚠️ GUI alerts/notifications deferred to Release 1B

---

## Phase 5A: Testing & Validation (Weeks 11-12)

### **Week 11: Comprehensive Unit Testing**

**Test Coverage Target:** 90%+

**Test Categories:**
1. **EDI Protocol Tests**
   - AF packet parsing
   - PFT reassembly with FEC
   - TAG item extraction
   - EDI-to-ETI conversion
   - Network packet loss handling

2. **FIG Processing Tests**
   - All FIG 0 types (0/0-0/24)
   - All FIG 1 types (1/0-1/5)
   - All FIG 2 types (2/0, 2/1, 2/4, 2/5)
   - Extended label reassembly
   - Character encoding (UTF-8, Thai)

3. **Audio Validation Tests**
   - DAB+ superframe parsing
   - Reed-Solomon validation
   - FireCode CRC checks
   - Bitrate compliance

4. **MOT/SlideShow Tests**
   - MOT header parsing
   - Segment reassembly
   - JPEG extraction
   - Object carousel

5. **EWS Alert Tests**
   - ASA protocol parsing
   - Alert lifecycle
   - FIG 0/19 integration
   - Geographic filtering

**Tasks Week 11:**
- [ ] Create comprehensive unit test suite
- [ ] Add ETSI reference data tests
- [ ] Add negative test cases (error handling)
- [ ] Add performance benchmarks
- [ ] Achieve 90%+ code coverage
- [ ] Fix all failing tests

### **Week 12: Integration Testing & Real Broadcast Validation**

**Test Data Sources:**
1. **ETSI Reference Streams** - Standard compliance
2. **Thai DAB Test Broadcasts** - Real-world validation
3. **ODR-DabMux Generated EDI** - EDI protocol validation
4. **Simulated Emergency Alerts** - EWS testing

**Integration Test Scenarios:**
1. **End-to-End EDI Stream:**
   - Network → EDI parse → ETI conversion → FIG analysis → Service discovery

2. **Complete FIG Analysis:**
   - Parse ETI → Extract all FIG types → Build ensemble structure → Validate services

3. **Audio Stream Validation:**
   - Extract MSC → Identify DAB+ streams → Validate audio parameters

4. **SlideShow Extraction:**
   - Parse MOT → Reassemble segments → Extract JPEG → Validate image

5. **EWS Alert Processing:**
   - Detect FIG 0/19 → Parse ASA → Generate alert → Check geographic targeting

**Tasks Week 12:**
- [ ] Run integration tests on real Thai DAB broadcasts
- [ ] Validate EDI network reception
- [ ] Test all FIG types with live data
- [ ] Validate audio stream processing
- [ ] Test SlideShow extraction
- [ ] Validate EWS alert detection
- [ ] Generate compliance reports
- [ ] Performance profiling (maintain >1000 FPS)
- [ ] Memory leak detection
- [ ] Stability testing (24-hour continuous operation)

**Phase 5A Deliverables:**
- ✅ 90%+ unit test coverage
- ✅ All integration tests passing
- ✅ Validated against Thai DAB broadcasts
- ✅ Performance >1000 FPS maintained
- ✅ Zero memory leaks
- ✅ Stable 24-hour operation
- ✅ Compliance reports generated

---

## Success Criteria

### **Release 1A Completion Requirements**

#### **Mandatory (Must Achieve 100%):**

| Component | Metric | Target | Validation Method |
|-----------|--------|--------|-------------------|
| **EN 300 799 (ETI)** | Compliance Score | 100% | ETSI reference streams |
| **TS 102 693 (EDI)** | Compliance Score | 95% | Thai DAB network streams |
| **EN 300 401 (FIG)** | Coverage | All FIG types | Real broadcast parsing |
| **TS 102 563 (Audio)** | Validation Accuracy | 85% | Audio parameter checks |
| **EN 301 234 (MOT)** | Object Extraction | 80% | SlideShow JPEGs |
| **FIG 0/18-19 (EWS)** | Detection Rate | 90% | Emergency alert tests |

#### **Quality Metrics:**

- ✅ **Build:** Zero compilation errors/warnings
- ✅ **Tests:** 90%+ code coverage, all tests passing
- ✅ **Performance:** >1000 FPS ETI processing
- ✅ **Stability:** 24-hour continuous operation without crashes
- ✅ **Memory:** No memory leaks (Valgrind clean)
- ✅ **Documentation:** All processors documented with examples

#### **Functional Requirements:**

- ✅ Parse EDI streams from Thai DAB network
- ✅ Convert EDI to ETI seamlessly
- ✅ Extract ALL FIG types from FIC
- ✅ Validate DAB+ audio parameters
- ✅ Extract SlideShow JPEG images
- ✅ Detect and parse emergency alerts
- ✅ Support Thai character encoding (UTF-8)
- ✅ Generate ETSI compliance reports

### **100% Target Breakdown**

**Starting Point:** 65% Complete

**Remaining 35% Distribution:**
- **10%** - EDI network protocol (TS 102 693)
- **10%** - Complete FIG processing (EN 300 401)
- **5%** - Audio validation (TS 102 563)
- **5%** - MOT/SlideShow (EN 301 234, TS 101 499)
- **3%** - EWS alerts (TS 104 089/090)
- **2%** - Final testing and validation

**Final State:** 100% Core Processing Complete

---

## Integration Points for Release 1B

Release 1A creates complete backend processors. Release 1B will add GUI integration.

### **Data Output Formats for GUI**

All processors will output data in formats ready for Qt GUI display:

```cpp
// Standardized output interfaces for Release 1B
namespace release_1b_interface {

// EDI Network Statistics (for network monitoring panel)
struct EDINetworkStats {
    std::string source_address;
    uint16_t source_port;
    uint64_t packets_received;
    uint64_t packets_lost;
    double packet_loss_rate;
    uint64_t fec_corrections;
    double bitrate_mbps;
};

// Service List (for service tree widget)
struct ServiceInfo {
    uint16_t service_id;
    std::string service_label;
    std::string extended_label;  // UTF-8 for Thai
    uint8_t service_type;        // Audio/Data
    bool is_dab_plus;
    std::vector<ComponentInfo> components;
};

// Audio Metrics (for audio quality panel)
struct AudioQualityMetrics {
    uint16_t bitrate_kbps;
    uint8_t protection_level;
    bool sbr_enabled;
    bool ps_enabled;
    double estimated_quality;
    bool rs_valid;
    bool firecode_valid;
};

// SlideShow Images (for slideshow display widget)
struct SlideShowImage {
    std::string jpeg_filepath;   // Saved JPEG path
    std::string title;
    std::string category;
    uint32_t display_duration_ms;
    QDateTime received_time;
};

// EWS Alerts (for emergency notification widget)
struct EmergencyAlert {
    uint32_t alert_id;
    std::string alert_type;
    uint8_t severity;           // 0-3
    std::string headline;
    std::string description;
    QDateTime onset_time;
    QDateTime expires_time;
    bool affects_current_location;
};

} // namespace release_1b_interface
```

### **Qt Signal/Slot Integration Points**

Processors will emit Qt signals for GUI updates:

```cpp
// Processors will inherit from QObject and emit signals
class EDINetworkReceiver : public QObject {
    Q_OBJECT
signals:
    void networkStatsUpdated(const EDINetworkStats& stats);
    void ediPacketReceived(const EDIPacket& packet);
};

class FIG0Processor : public QObject {
    Q_OBJECT
signals:
    void serviceDiscovered(const ServiceInfo& service);
    void ensembleInfoUpdated(const EnsembleInfo& ensemble);
};

class EWSAlertProcessor : public QObject {
    Q_OBJECT
signals:
    void emergencyAlertReceived(const EmergencyAlert& alert);
    void alertExpired(uint32_t alert_id);
};

class SlideShowDecoder : public QObject {
    Q_OBJECT
signals:
    void slideShowImageReady(const SlideShowImage& image);
};
```

### **Files Ready for Release 1B GUI Integration**

```
Backend Processors (1A) → GUI Widgets (1B)
├── edi_network_receiver.cpp → NetworkMonitorPanel
├── fig0_processor.cpp → ServiceTreeWidget
├── fig1_processor.cpp → LabelDisplayWidget
├── fig2_processor.cpp → ExtendedLabelWidget
├── dabplus_stream_validator.cpp → AudioQualityPanel
├── slideshow_decoder.cpp → SlideShowViewer
└── ews_alert_processor.cpp → EmergencyAlertWidget
```

---

## Timeline Summary

### **Release 1A: 10-12 Weeks Total**

| Phase | Duration | Deliverable | Completion % |
|-------|----------|-------------|--------------|
| **Phase 1A** | 4 weeks | ETI/EDI Complete | 65% → 80% |
| **Phase 2A** | 3 weeks | All FIG Types | 80% → 90% |
| **Phase 3A** | 2 weeks | Audio & MOT | 90% → 95% |
| **Phase 4A** | 1 week | EWS Alerts | 95% → 98% |
| **Phase 5A** | 2 weeks | Testing & Validation | 98% → **100%** |

**Parallel Development Opportunities:**
- Weeks 5-7 (FIG) can overlap with Weeks 8-9 (Audio/MOT) if 2 developers
- Week 10 (EWS) can be done in parallel with Week 11 (Testing)

**Accelerated Timeline:** 10 weeks with 2 developers working in parallel

---

## Appendix: ODR-DabMux Code Reference Guide

### **How to Use ODR-DabMux Code**

**Location:** `/home/seksan/workspace/ODR-DabMux`

**Study Methodology:**

1. **For FIG Processing:**
   ```bash
   # Study FIG generation to understand structure
   vim /home/seksan/workspace/ODR-DabMux/src/fig/FIG0_X.cpp

   # Reverse engineer: If ODR encodes like this:
   buf[0] = (service_id >> 8) & 0xFF;
   buf[1] = service_id & 0xFF;

   # We parse like this:
   service_id = (data[0] << 8) | data[1];
   ```

2. **For EDI Protocol:**
   ```bash
   # Study packet structure
   vim /home/seksan/workspace/ODR-DabMux/lib/edioutput/TagItems.cpp

   # ODR builds packets → We parse packets
   # Copy structure definitions, reverse encode/decode logic
   ```

3. **For EWS System:**
   ```bash
   # Study alert management
   vim /home/seksan/workspace/ODR-DabMux/src/ews/AlertManager.cpp

   # Adapt alert structures for analyzer use case
   # Keep same data models, change from "generate" to "parse"
   ```

**Important Notes:**
- ⚠️ ODR-DabMux is GPL licensed - our analyzer is GPL compatible
- ✅ Study algorithm and structure, don't copy verbatim
- ✅ Adapt for analyzer use case (parse vs generate)
- ✅ Maintain our existing architecture patterns
- ✅ Add comprehensive tests for our implementation

---

## Document Status

**Status:** READY FOR REVIEW
**Next Actions:**
1. Review Release 1A plan with team
2. Approve "Processors First" strategy
3. Assign developers to phases
4. Begin Phase 1A Week 1 implementation
5. Schedule bi-weekly progress reviews

**Related Documents:**
- [RELEASE_1B_GUI_INTEGRATION_PLAN.md](RELEASE_1B_GUI_INTEGRATION_PLAN.md) - GUI integration phase
- [THAIDAB_RELEASE_1_IMPLEMENTATION_PLAN.md](THAIDAB_RELEASE_1_IMPLEMENTATION_PLAN.md) - Overall Release 1 plan

---

**Last Updated:** October 19, 2025
**Version:** 1.0
