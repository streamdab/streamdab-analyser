# Phase 3A: Audio & Data Services Implementation Plan

**Document Version:** 1.0
**Date:** October 20, 2025
**Phase:** Phase 3A - Audio & Data Services (Weeks 8-9)
**Status:** Planning Complete, Ready for Implementation
**Timeline:** 2 weeks

---

## Executive Summary

Phase 3A implements DAB+ audio stream validation and MOT Protocol support for SlideShow decoding. This phase adds professional audio analysis capabilities and multimedia content extraction, completing the core processing foundation before EWS Alert Manager implementation.

**Key Achievement:** Professional audio quality validation and MOT SlideShow extraction for Thai DAB+ broadcasts.

---

## Phase 3A Scope

### **Week 8: DAB+ Audio Stream Validation**
- DAB+ audio superframe parsing (TS 102 563)
- Reed-Solomon & FireCode validation
- Audio quality metrics extraction
- Bitrate/protection level compliance checking

### **Week 9: MOT Protocol & SlideShow Decoder**
- MOT header/directory parsing (EN 301 234)
- Segment reassembly logic
- JPEG SlideShow extraction (TS 101 499)
- Object carousel management

---

## Architecture Decision

**Decision:** Create separate processors for audio and MOT to maintain separation of concerns.

**New Components:**
```
src/core/
├── dabplus_stream_validator.hpp  // NEW - Audio superframe validation
├── dabplus_stream_validator.cpp
├── mot_protocol.hpp               // NEW - MOT header/directory parsing
├── mot_protocol.cpp
├── slideshow_decoder.hpp          // NEW - SlideShow JPEG extraction
└── slideshow_decoder.cpp
```

**Integration Points:**
- MSC (Main Service Channel) data extraction from ETI frames
- Service discovery integration (FIG 0/2, 0/13 user applications)
- GUI signal emissions for SlideShow display

---

## Week 8: DAB+ Audio Stream Validation

### **Implementation Scope**

**IMPORTANT:** This is **STREAM VALIDATION**, not audio decoding. We validate TS 102 563 compliance without actually decoding audio to PCM.

### **1. Audio Superframe Structure**

**ETSI TS 102 563 Compliance:**
```cpp
// DAB+ Audio Superframe (120ms)
struct AudioSuperframe {
    // Superframe Header
    uint16_t firecode_status;        // FireCode CRC validation
    uint8_t num_aus;                 // Number of Audio Access Units (1-4)
    bool sbr_flag;                   // Spectral Band Replication
    bool ps_flag;                    // Parametric Stereo
    uint8_t dac_rate;                // DAC rate index

    // Reed-Solomon FEC
    struct RSCoding {
        uint8_t rs_k;                // Data bytes
        uint8_t rs_z;                // Parity bytes
        bool valid_crc;              // RS(120, 110) or RS(120, 96)
    } rs_coding;

    // Audio Access Units
    struct AudioAccessUnit {
        uint16_t au_size;            // AU size in bytes
        std::vector<uint8_t> au_data;
    };
    std::vector<AudioAccessUnit> access_units;

    // Protection Profile
    uint8_t protection_level;        // UEP: 1-5
    uint16_t bitrate_kbps;           // 8-192 kbps
};
```

### **2. Validation Functions**

```cpp
class DABPlusStreamValidator {
public:
    // Superframe validation
    struct ValidationResult {
        bool is_valid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    ValidationResult validateSuperframe(const AudioSuperframe& superframe);
    ValidationResult validateFireCode(const std::vector<uint8_t>& sf_data);
    ValidationResult validateReedSolomon(const RSCoding& rs);
    ValidationResult validateAUStructure(const std::vector<AudioAccessUnit>& aus);

    // Bitrate compliance
    bool isValidBitrate(uint16_t bitrate, uint8_t protection_level);
    bool isValidProtectionProfile(uint8_t level);

    // Quality metrics (no decoding required)
    struct AudioMetrics {
        uint16_t bitrate_kbps;       // From superframe header
        uint8_t sample_rate_khz;     // 24 or 48 kHz
        uint8_t channels;            // 1 (mono) or 2 (stereo)
        bool is_stereo_ps;           // Parametric Stereo
        bool is_heaac_v2;            // HE-AAC v2 (with SBR+PS)
        double quality_estimate;     // 0-100 based on bitrate/protection
    };
    AudioMetrics extractMetrics(const AudioSuperframe& superframe);

    // Qt signals for GUI integration
signals:
    void audioStreamDetected(uint32_t service_id, const AudioMetrics& metrics);
    void audioQualityWarning(uint32_t service_id, const QString& warning);
    void audioStreamError(uint32_t service_id, const QString& error);
};
```

### **3. FireCode Validation**

**TS 102 563 Section 6.2: FireCode Inner Coding**
```cpp
bool DABPlusStreamValidator::validateFireCode(const std::vector<uint8_t>& sf_data) {
    // FireCode polynomial: g(x) = x^16 + x^14 + x^13 + x^11 + x^10 + x^8 + x^6 + x^5 + x^2 + x + 1
    // 16-bit CRC for superframe header protection

    const uint16_t FIRECODE_POLY = 0x782F;  // FireCode polynomial

    uint16_t calculated_crc = calculateFireCodeCRC(sf_data);
    uint16_t received_crc = (sf_data[0] << 8) | sf_data[1];  // First 2 bytes

    return (calculated_crc == received_crc);
}

uint16_t DABPlusStreamValidator::calculateFireCodeCRC(const std::vector<uint8_t>& data) {
    // Implementation similar to CCITT CRC-16 but with FireCode polynomial
    uint16_t crc = 0xFFFF;
    for (uint8_t byte : data) {
        crc ^= (byte << 8);
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ FIRECODE_POLY;
            else
                crc <<= 1;
        }
    }
    return crc;
}
```

### **4. Reed-Solomon Validation**

**TS 102 563 Section 6.1: Outer Coding**
```cpp
ValidationResult DABPlusStreamValidator::validateReedSolomon(const RSCoding& rs) {
    ValidationResult result;
    result.is_valid = true;

    // TS 102 563 Table 10: Valid RS profiles
    // RS(120, 110): 110 data bytes, 10 parity bytes
    // RS(120, 96): 96 data bytes, 24 parity bytes

    if (rs.rs_k == 110 && rs.rs_z == 10) {
        // UEP-1/2 protection
        result.is_valid = rs.valid_crc;
    } else if (rs.rs_k == 96 && rs.rs_z == 24) {
        // UEP-3/4/5 protection
        result.is_valid = rs.valid_crc;
    } else {
        result.is_valid = false;
        result.errors.push_back(QString("Invalid RS profile: RS(%1, %2)")
                                .arg(rs.rs_k + rs.rs_z).arg(rs.rs_k));
    }

    return result;
}
```

### **5. Audio Quality Metrics**

```cpp
AudioMetrics DABPlusStreamValidator::extractMetrics(const AudioSuperframe& superframe) {
    AudioMetrics metrics;

    // Bitrate from superframe header
    metrics.bitrate_kbps = superframe.bitrate_kbps;

    // Sample rate from DAC rate
    metrics.sample_rate_khz = (superframe.dac_rate == 0) ? 48 : 24;

    // Channel configuration
    metrics.is_stereo_ps = superframe.ps_flag;
    metrics.channels = (superframe.ps_flag || superframe.sbr_flag) ? 2 : 1;

    // HE-AAC v2 detection (SBR + PS)
    metrics.is_heaac_v2 = (superframe.sbr_flag && superframe.ps_flag);

    // Quality estimate (0-100)
    // Based on: bitrate, protection level, SBR/PS usage
    if (metrics.is_heaac_v2) {
        // HE-AAC v2: 32 kbps = good quality, 48+ kbps = excellent
        metrics.quality_estimate = std::min(100.0, (metrics.bitrate_kbps / 48.0) * 100.0);
    } else if (superframe.sbr_flag) {
        // HE-AAC: 64 kbps = good, 96+ kbps = excellent
        metrics.quality_estimate = std::min(100.0, (metrics.bitrate_kbps / 96.0) * 100.0);
    } else {
        // LC-AAC: 128 kbps = good, 192+ kbps = excellent
        metrics.quality_estimate = std::min(100.0, (metrics.bitrate_kbps / 192.0) * 100.0);
    }

    // Protection level bonus
    metrics.quality_estimate *= (1.0 + (superframe.protection_level * 0.05));
    metrics.quality_estimate = std::min(100.0, metrics.quality_estimate);

    return metrics;
}
```

---

## Week 9: MOT Protocol & SlideShow Decoder

### **Implementation Scope**

**ETSI EN 301 234: MOT Protocol**
**ETSI TS 101 499: MOT SlideShow**

### **1. MOT Header Structure**

```cpp
struct MOTHeader {
    // Body Size (13 bits)
    uint16_t body_size;

    // Header Size (13 bits)
    uint16_t header_size;

    // Content Type (6 bits)
    enum ContentType {
        GENERAL_DATA = 0x00,
        TEXT = 0x01,
        IMAGE_JPEG = 0x02,        // SlideShow
        IMAGE_PNG = 0x03,
        HTML = 0x05,
        AUDIO_MPEG = 0x06,
        VIDEO_MPEG = 0x07
    } content_type;

    // Content Subtype (9 bits)
    uint16_t content_subtype;

    // Header Extension Parameters
    struct HeaderParameter {
        uint8_t param_id;
        std::vector<uint8_t> param_data;
    };
    std::map<uint8_t, HeaderParameter> parameters;

    // Common parameters
    std::string content_name;         // ParamId 12
    uint32_t trigger_time;            // ParamId 5
    std::string category_title;       // ParamId 25 (SlideShow)
    std::string click_through_url;    // ParamId 27 (SlideShow)
    uint8_t compression_type;         // ParamId 10
};
```

### **2. MOT Directory Structure**

```cpp
struct MOTDirectory {
    // Directory Header
    uint16_t directory_size;
    uint16_t num_objects;
    uint32_t carousel_period_ms;
    uint16_t segment_size;

    // Directory Entries
    struct DirectoryEntry {
        uint32_t transport_id;        // 16-bit or 32-bit
        uint16_t header_size;
        uint32_t body_size;
        uint8_t repetition_count;
    };
    std::vector<DirectoryEntry> entries;

    // Validity flags
    bool has_valid_crc;
    uint16_t crc_calculated;
    uint16_t crc_received;
};
```

### **3. MOT Segment Reassembly**

```cpp
class MOTSegmentReassembler {
public:
    // Add segment from MSC data
    enum ReassemblyStatus {
        INCOMPLETE,
        COMPLETE,
        ERROR_CRC,
        ERROR_MISSING_SEGMENTS
    };

    ReassemblyStatus addSegment(uint32_t transport_id,
                                uint16_t segment_number,
                                uint16_t total_segments,
                                const std::vector<uint8_t>& segment_data);

    // Retrieve complete object
    std::optional<std::vector<uint8_t>> getCompleteObject(uint32_t transport_id);

    // Progress tracking
    double getProgress(uint32_t transport_id) const;
    bool isComplete(uint32_t transport_id) const;

    // Segment management
    void clearObject(uint32_t transport_id);
    std::vector<uint32_t> getActiveObjects() const;

private:
    struct SegmentBuffer {
        uint16_t total_segments;
        std::map<uint16_t, std::vector<uint8_t>> segments;  // segment_number → data
        uint32_t last_update_time_ms;
        bool is_complete;
    };

    std::map<uint32_t, SegmentBuffer> m_buffers;  // transport_id → buffer

    // Timeout management (clear stale objects after 60 seconds)
    void cleanupStaleObjects();
};
```

### **4. SlideShow Decoder**

```cpp
class SlideShowDecoder {
public:
    // SlideShow Image Structure (TS 101 499)
    struct SlideShowImage {
        uint32_t transport_id;
        std::vector<uint8_t> jpeg_data;

        // MOT header parameters
        std::string content_name;      // Filename
        std::string category_title;    // "Artist - Song Title"
        std::string click_through_url; // Optional URL
        uint32_t trigger_time;         // Display trigger timestamp

        // Image metadata
        uint32_t body_size;
        uint16_t width;                // From JPEG header
        uint16_t height;               // From JPEG header

        // Reception metadata
        QDateTime received_time;
        uint16_t carousel_id;
    };

    // Process MOT object for SlideShow
    std::optional<SlideShowImage> decodeSlideShow(const MOTHeader& header,
                                                   const std::vector<uint8_t>& body);

    // JPEG validation (basic header check, not full decode)
    bool isValidJPEG(const std::vector<uint8_t>& jpeg_data);
    struct JPEGInfo {
        uint16_t width;
        uint16_t height;
        uint8_t bits_per_pixel;
        bool is_progressive;
    };
    JPEGInfo extractJPEGInfo(const std::vector<uint8_t>& jpeg_data);

    // SlideShow cache management
    void addSlideShow(const SlideShowImage& image);
    std::vector<SlideShowImage> getCurrentSlideShows() const;
    std::optional<SlideShowImage> getSlideShow(uint32_t transport_id) const;
    void clearCache();

    // Qt signals for GUI
signals:
    void slideShowReceived(const SlideShowImage& image);
    void slideShowUpdated(uint32_t transport_id);
    void slideShowExpired(uint32_t transport_id);

private:
    std::map<uint32_t, SlideShowImage> m_slideshows;  // transport_id → image
    static constexpr int MAX_SLIDESHOW_CACHE = 10;
};
```

### **5. Integration with MSC Data**

```cpp
// MSC (Main Service Channel) data extraction
class MSCDataExtractor {
public:
    // Extract MSC data for a specific subchannel from ETI frame
    std::vector<uint8_t> extractSubChannel(const ETIFrame& eti_frame,
                                           uint8_t subchannel_id);

    // Parse MSC data group (EN 300 401 Section 5.3.3)
    struct DataGroup {
        uint8_t extension_flag;
        uint8_t crc_flag;
        uint8_t segment_flag;
        uint8_t user_access_flag;
        uint8_t data_group_type;
        uint8_t continuity_index;
        uint8_t repetition_index;

        std::vector<uint8_t> data_field;
        uint16_t crc_received;
        bool crc_valid;
    };

    std::optional<DataGroup> parseDataGroup(const std::vector<uint8_t>& msc_data);

    // MOT segment extraction from data group
    struct MOTSegmentHeader {
        uint16_t repetition_count;
        uint16_t segment_number;
        uint32_t transport_id;
        bool last_segment;
    };

    std::optional<MOTSegmentHeader> parseMOTSegmentHeader(const DataGroup& dg);
};
```

---

## Implementation Tasks

### **Week 8 Tasks (DAB+ Audio):**

**Day 1-2: Audio Superframe Parsing** (6 hours)
- [ ] Implement AudioSuperframe structure
- [ ] Parse superframe header (num_aus, SBR, PS, DAC rate)
- [ ] Extract Audio Access Units
- [ ] Add bounds checking and validation

**Day 3: FireCode & Reed-Solomon** (4 hours)
- [ ] Implement FireCode CRC calculation
- [ ] Implement RS(120,110) and RS(120,96) validation
- [ ] Add protection profile validation

**Day 4: Quality Metrics** (3 hours)
- [ ] Implement AudioMetrics extraction
- [ ] Add bitrate compliance checking
- [ ] Implement quality estimation algorithm
- [ ] Add Qt signals for audio events

**Day 5: Integration & Testing** (3 hours)
- [ ] Integrate with MSC data extraction
- [ ] Connect to FIG 0/2 service discovery
- [ ] Add audio stream tracking per service
- [ ] Unit tests for audio validation

### **Week 9 Tasks (MOT Protocol):**

**Day 1-2: MOT Header/Directory** (6 hours)
- [ ] Implement MOTHeader parsing
- [ ] Implement header parameter extraction
- [ ] Implement MOTDirectory parsing
- [ ] Add CRC validation

**Day 3-4: Segment Reassembly** (6 hours)
- [ ] Implement MOTSegmentReassembler
- [ ] Add segment buffer management
- [ ] Implement progress tracking
- [ ] Add timeout cleanup for stale objects

**Day 5: SlideShow Decoder** (4 hours)
- [ ] Implement SlideShowDecoder
- [ ] Add JPEG validation (SOI/EOI markers)
- [ ] Extract JPEG dimensions
- [ ] Implement SlideShow cache

**Day 6: Integration & Testing** (4 hours)
- [ ] Integrate with FIG 0/13 user application detection
- [ ] Connect to MSC data extraction
- [ ] Add Qt signals for SlideShow events
- [ ] Unit tests for MOT/SlideShow

---

## Success Criteria

### **Week 8 (Audio Validation):**
- ✅ Parse DAB+ audio superframes correctly
- ✅ Validate FireCode and Reed-Solomon FEC
- ✅ Extract audio quality metrics (bitrate, sample rate, channels)
- ✅ Detect HE-AAC v2 profiles (SBR+PS)
- ✅ Qt signals emit for audio stream events
- ✅ Zero compilation errors

### **Week 9 (MOT Protocol):**
- ✅ Parse MOT headers and directories
- ✅ Reassemble segmented MOT objects
- ✅ Extract JPEG SlideShow images
- ✅ Extract SlideShow metadata (category, URL)
- ✅ Qt signals emit for SlideShow events
- ✅ Zero compilation errors

---

## ETSI Standards Compliance

| Standard | Description | Target Compliance |
|----------|-------------|-------------------|
| **TS 102 563** | DAB+ Audio Coding | 85% (validation only, no decoding) |
| **EN 301 234** | MOT Protocol | 80% (core features) |
| **TS 101 499** | MOT SlideShow | 75% (JPEG only, no PNG/BWS) |
| **EN 300 401** | MSC data groups | 100% (Section 5.3.3) |

---

## Thai DAB Broadcast Support

**Audio Quality for Thai Stations:**
- Most Bangkok DAB+ stations: 48-64 kbps HE-AAC v2
- Quality estimate will show "Good" (70-80%) for these bitrates
- Protection levels 3-4 typical

**SlideShow for Thai Stations:**
- Thai characters in category_title (UTF-8 support via QString)
- Artist/song titles in Thai: "ศิลปิน - ชื่อเพลง"
- Album art JPEG extraction

---

## Integration with Existing Code

### **Connection to FIG Parsers:**

```cpp
// FIG 0/2: Service Component Discovery
connect(figAnalyzer, &AdvancedFIGAnalyzer::serviceComponentDiscovered,
        audioValidator, &DABPlusStreamValidator::onServiceComponentDetected);

// FIG 0/13: User Application Detection (MOT Slideshow)
connect(figAnalyzer, &AdvancedFIGAnalyzer::userApplicationDetected,
        slideshowDecoder, &SlideShowDecoder::onUserApplicationDetected);

// When FIG 0/13 detects USER_APP_MOT_SLIDESHOW (0x002):
void SlideShowDecoder::onUserApplicationDetected(uint32_t service_id, uint16_t app_type) {
    if (app_type == 0x002) {  // MOT Slideshow
        qInfo() << "MOT SlideShow detected for service" << Qt::hex << service_id;
        // Start listening for MOT segments on this service's sub-channel
    }
}
```

### **Connection to ETI Frame Parser:**

```cpp
// Extract MSC data for audio validation
void ETIFrameProcessor::processFrame(const ETIFrame& frame) {
    // ... existing FIC processing ...

    // Extract MSC for each service component
    for (auto& component : discovered_components) {
        if (component.is_audio_service) {
            auto msc_data = extractMSCData(frame, component.subchannel_id);
            audioValidator->processAudioData(component.service_id, msc_data);
        }

        if (component.has_mot_slideshow) {
            auto msc_data = extractMSCData(frame, component.subchannel_id);
            motProtocol->processMOTData(component.service_id, msc_data);
        }
    }
}
```

---

## Timeline

**Total Effort:** 2 weeks (40 hours)

- **Week 8**: DAB+ Audio Stream Validation (16 hours)
- **Week 9**: MOT Protocol & SlideShow (20 hours)
- **Buffer**: 4 hours for testing and documentation

---

## References

**ETSI Standards:**
- TS 102 563 - DAB+ Audio Coding (HE-AAC v2)
- EN 301 234 - MOT Protocol Specification
- TS 101 499 - MOT SlideShow Application
- EN 300 401 - DAB System Specification (MSC data groups)

**Related Documents:**
- `RELEASE_1A_CORE_PROCESSORS_PLAN.md` - Overall project plan
- `PHASE_2A_WEEK7_FIG_LABELS.md` - Previous week implementation

---

**Document Status:** ✅ Ready for Implementation
**Next Step:** Launch specialized agents for Week 8 and Week 9 implementations
