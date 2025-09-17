# ETI Protocol Specifications for StreamDAB Analyzer

**Document Version:** 1.0  
**Date:** September 17, 2025  
**Author:** Standards Compliance Agent  
**Reference Standards:** ETSI EN 300 799 v1.2.1, ETSI EN 300 401 v2.1.1  

## Table of Contents

1. [Protocol Overview](#protocol-overview)
2. [ETI Frame Structure Specification](#eti-frame-structure-specification)
3. [Field-by-Field Analysis](#field-by-field-analysis)
4. [FIC Data Structure](#fic-data-structure)
5. [MSC Data Organization](#msc-data-organization)
6. [Error Detection and Correction](#error-detection-and-correction)
7. [Timing and Synchronization](#timing-and-synchronization)
8. [Implementation Data Structures](#implementation-data-structures)

## Protocol Overview

The Ensemble Transport Interface (ETI) is the standardized distribution interface for DAB (Digital Audio Broadcasting) defined by ETSI EN 300 799. ETI provides a format-independent method for distributing DAB multiplex data between different equipment types in the broadcast chain.

### Key Protocol Characteristics

- **Frame Size**: Exactly 6144 bytes per frame
- **Frame Duration**: 24 milliseconds per frame
- **Frame Rate**: 250 frames per second
- **Data Rate**: 2.048 Mbit/s
- **Error Protection**: Reed-Solomon outer coding + convolutional inner coding
- **Synchronization**: 4-byte sync pattern per frame

### ETI Format Types

#### ETI(NI) - Network Independent Format
- Contains complete DAB multiplex information
- Includes FIC (Fast Information Channel) data
- Contains MSC (Main Service Channel) data
- Suitable for storage and network distribution

#### ETI(LI) - Linear Format  
- Optimized for direct modulator input
- Time-aligned with transmission requirements
- Includes precise timing information (TIST)
- Ready for OFDM modulation

## ETI Frame Structure Specification

### Complete Frame Layout (6144 bytes)

```
Byte Offset | Field Name | Size (bytes) | Description
------------|------------|--------------|------------------------------------------
0-3         | SYNC       | 4           | Synchronization pattern
4-11        | LIDATA     | 8           | Linear Data (frame characterization)
12-43       | FIC        | 32          | Fast Information Channel
44-6139     | MSC        | 6096        | Main Service Channel
6140-6143   | CRC        | 4           | Reed-Solomon error check
```

### Frame Timing Relationship

```
ETI Frame n    ETI Frame n+1   ETI Frame n+2
|<--24ms-->|  |<--24ms-->|   |<--24ms-->|
|          |  |          |   |          |
0ms       24ms 48ms      72ms 96ms     120ms

Each frame represents exactly 24ms of real-time audio
250 frames = 1 second of broadcast time
```

## Field-by-Field Analysis

### SYNC Field (4 bytes, offset 0-3)

The synchronization field provides frame alignment and format identification.

```cpp
struct EtiSyncField {
    static constexpr uint8_t SYNC_PATTERN[4] = {0x49, 0x93, 0x1E, 0x03};
    
    uint8_t sync_bytes[4];
    
    bool is_valid() const {
        return (sync_bytes[0] == 0x49) && (sync_bytes[1] == 0x93) &&
               (sync_bytes[2] == 0x1E) && (sync_bytes[3] == 0x03);
    }
    
    void set_sync_pattern() {
        std::memcpy(sync_bytes, SYNC_PATTERN, 4);
    }
};
```

**Validation Requirements:**
- Must match exact pattern: 0x49, 0x93, 0x1E, 0x03
- Deviation indicates frame corruption or loss of synchronization
- Missing sync pattern requires frame boundary re-acquisition

### LIDATA Field (8 bytes, offset 4-11)

Linear Data field contains frame characterization and timing information.

```cpp
struct EtiLidataField {
    uint8_t fc;          // Frame Count (0-249, wraps every 6 seconds)
    uint8_t ficf : 1;    // FIC Flag (1=FIC present, 0=no FIC)
    uint8_t nst : 7;     // Number of Sub-channels in MSC (0-63)
    uint8_t fp : 3;      // Frame Phase (0-7)
    uint8_t mid : 2;     // Mode Identity (1-4)
    uint8_t fl : 11;     // Frame Length in bytes
    uint16_t sad[64];    // Sub-channel Address Descriptors (when present)
    
    // TIST (Time Stamp) - 4 bytes at end of LIDATA
    uint32_t tist;       // Time stamp in units of 16.384 MHz clock periods
    
    bool validate_frame_count(uint8_t expected_fc) const {
        return fc == expected_fc || fc == ((expected_fc + 1) % 250);
    }
    
    bool validate_mode(uint8_t expected_mode) const {
        return mid == expected_mode;
    }
    
    std::chrono::microseconds get_timestamp_us() const {
        // Convert TIST to microseconds (16.384 MHz clock)
        return std::chrono::microseconds(tist * 1000000 / 16384000);
    }
};
```

**Frame Count (FC) Validation:**
- Increments from 0 to 249, then wraps to 0
- Discontinuity indicates missing or duplicated frames
- Used for frame sequence validation

**FIC Flag (FICF) Requirements:**
- Must be 1 for ETI(NI) format
- May be 0 in some ETI(LI) applications
- Indicates presence of FIC data at offset 12-43

**Number of Sub-channels (NST):**
- Range: 0-63 sub-channels per frame
- Defines MSC organization structure
- Must match FIG 0/1 sub-channel configuration

### FIC Field (32 bytes, offset 12-43)

Fast Information Channel contains ensemble configuration data organized as FIG blocks.

```cpp
struct EtiFicField {
    uint8_t fic_data[32];    // Raw FIC data (256 bits)
    
    struct FigHeader {
        uint8_t fig_type : 5;        // FIG type (0-31)
        uint8_t length : 3;          // Length field
        bool c : 1;                  // Continuation flag
        bool oe : 1;                 // Other Ensemble flag
        uint8_t data_field : 6;      // Data field
    };
    
    std::vector<FigBlock> decode_fig_blocks() const;
    bool validate_fic_crc() const;
    bool contains_fig_type(uint8_t fig_type) const;
};

struct FigBlock {
    uint8_t fig_type;
    uint8_t length;
    bool continuation_flag;
    bool other_ensemble_flag;
    std::vector<uint8_t> data;
    
    bool is_valid() const {
        return length <= 29;  // Maximum FIG data length
    }
};
```

**FIG Type Coverage Requirements:**
- FIG 0/0: Ensemble information (mandatory)
- FIG 0/1: Sub-channel organization (mandatory)
- FIG 0/2: Service organization (mandatory)
- FIG 1/0: Ensemble label (mandatory)
- FIG 1/1: Service labels (recommended)

### MSC Field (6096 bytes, offset 44-6139)

Main Service Channel contains the actual service data (audio, data services).

```cpp
struct EtiMscField {
    uint8_t msc_data[6096];
    
    struct SubChannel {
        uint16_t sub_channel_id;
        uint16_t start_address;      // Start address in CUs (Capacity Units)
        uint16_t size;               // Size in CUs
        uint8_t protection_level;    // Protection level (1-5)
        bool uep_flag;              // Unequal Error Protection flag
        
        uint16_t get_end_address() const {
            return start_address + size - 1;
        }
        
        bool validate_boundaries() const {
            return (start_address + size) <= 864;  // Max CUs per frame
        }
    };
    
    std::vector<uint8_t> extract_subchannel_data(const SubChannel& subchannel) const;
    bool validate_subchannel_organization(const std::vector<SubChannel>& subchannels) const;
};
```

**Capacity Unit (CU) Organization:**
- 1 CU = 8 bytes (64 bits)
- Maximum 864 CUs per MSC (6912 bytes)
- Actual MSC size: 6096 bytes = 762 CUs

**Sub-channel Protection Levels:**
- Level 1: Code rate 1/4 (highest protection)
- Level 2: Code rate 3/8
- Level 3: Code rate 1/2  
- Level 4: Code rate 3/4
- Level 5: Code rate 1 (lowest protection)

### CRC Field (4 bytes, offset 6140-6143)

Reed-Solomon error detection field for frame integrity validation.

```cpp
struct EtiCrcField {
    uint8_t crc_bytes[4];
    
    bool validate_frame(const EtiFrame& frame) const;
    void calculate_crc(const EtiFrame& frame);
    uint8_t count_correctable_errors(const EtiFrame& frame) const;
};
```

## FIC Data Structure

### FIG Block Organization

FIG (Fast Information Group) blocks within the FIC provide essential configuration information.

#### FIG 0/0 - Ensemble Information
```cpp
struct Fig0_0_EnsembleInfo {
    uint16_t ensemble_id;           // Ensemble identifier
    uint8_t change_flags : 2;       // Change event flags
    uint8_t alarm_flag : 1;         // Alarm announcement flag
    uint8_t cif_count_high : 5;     // CIF count upper bits
    uint8_t cif_count_low;          // CIF count lower bits
    uint8_t occurrence_change : 8;   // Occurrence change field
    
    uint16_t get_cif_count() const {
        return (static_cast<uint16_t>(cif_count_high) << 8) | cif_count_low;
    }
};
```

#### FIG 0/1 - Sub-channel Organization
```cpp
struct Fig0_1_SubchannelOrg {
    struct SubChannelDescriptor {
        uint8_t sub_channel_id : 6;      // Sub-channel identifier
        uint16_t start_address : 10;      // Start address in CUs
        uint8_t table_switch : 1;         // Table switch flag
        uint8_t table_index : 6;          // Table index
        uint16_t sub_channel_size : 10;   // Size in CUs
        uint8_t protection_level : 3;     // Protection level
        uint8_t uep_index : 6;           // UEP table index
        
        bool validate_address_bounds() const {
            return (start_address + sub_channel_size) <= 864;
        }
    };
    
    std::vector<SubChannelDescriptor> sub_channels;
};
```

#### FIG 0/2 - Service Organization
```cpp
struct Fig0_2_ServiceOrg {
    struct ServiceDescriptor {
        uint16_t service_id;             // Service identifier
        uint8_t country_id;              // Country identifier
        uint8_t extended_country_code;   // Extended country code
        bool programme_service;          // Programme service flag
        
        struct ComponentInfo {
            uint8_t component_id : 6;     // Service component identifier
            uint8_t sub_channel_id : 6;   // Associated sub-channel
            bool primary;                 // Primary component flag
        };
        
        std::vector<ComponentInfo> components;
    };
    
    std::vector<ServiceDescriptor> services;
};
```

#### FIG 1/0 - Ensemble Label
```cpp
struct Fig1_0_EnsembleLabel {
    uint16_t ensemble_id;        // Ensemble identifier
    char label[16];              // Ensemble label (UTF-8)
    uint16_t character_flag;     // Character definition bitmap
    
    std::string get_label_string() const {
        return std::string(label, strnlen(label, 16));
    }
    
    bool validate_character_encoding() const {
        // Validate UTF-8 encoding compliance
        return is_valid_utf8(label, 16);
    }
};
```

#### FIG 1/1 - Service Labels
```cpp
struct Fig1_1_ServiceLabel {
    uint16_t service_id;         // Service identifier
    char label[16];              // Service label (UTF-8)
    uint16_t character_flag;     // Character definition bitmap
    
    std::string get_label_string() const {
        return std::string(label, strnlen(label, 16));
    }
};
```

## MSC Data Organization

### Capacity Unit Structure

The MSC is organized in Capacity Units (CUs) of 64 bits each.

```cpp
struct CapacityUnit {
    uint8_t data[8];    // 64 bits = 8 bytes
    
    static constexpr size_t BITS_PER_CU = 64;
    static constexpr size_t BYTES_PER_CU = 8;
    static constexpr size_t MAX_CUS_PER_FRAME = 864;
};

class MscOrganizer {
private:
    std::array<CapacityUnit, 762> capacity_units_;  // Actual CUs in 6096-byte MSC
    
public:
    struct SubChannelMapping {
        uint8_t sub_channel_id;
        uint16_t start_cu;
        uint16_t size_cu;
        uint8_t protection_level;
        bool uep_flag;
    };
    
    bool map_subchannel(const SubChannelMapping& mapping);
    std::vector<uint8_t> extract_subchannel_data(uint8_t sub_channel_id) const;
    bool validate_cu_allocation(const std::vector<SubChannelMapping>& mappings) const;
};
```

### Protection Level Implementation

Each sub-channel uses convolutional coding with different protection levels:

```cpp
enum class ProtectionLevel : uint8_t {
    LEVEL_1 = 1,    // Code rate 1/4, highest protection
    LEVEL_2 = 2,    // Code rate 3/8
    LEVEL_3 = 3,    // Code rate 1/2
    LEVEL_4 = 4,    // Code rate 3/4
    LEVEL_5 = 5     // Code rate 1, lowest protection
};

struct ProtectionParameters {
    ProtectionLevel level;
    uint8_t code_rate_numerator;
    uint8_t code_rate_denominator;
    uint16_t viterbi_constraint_length;
    
    static ProtectionParameters get_parameters(ProtectionLevel level) {
        switch (level) {
            case ProtectionLevel::LEVEL_1:
                return {level, 1, 4, 7};
            case ProtectionLevel::LEVEL_2:
                return {level, 3, 8, 7};
            case ProtectionLevel::LEVEL_3:
                return {level, 1, 2, 7};
            case ProtectionLevel::LEVEL_4:
                return {level, 3, 4, 7};
            case ProtectionLevel::LEVEL_5:
                return {level, 1, 1, 7};
        }
    }
};
```

## Error Detection and Correction

### Reed-Solomon Outer Coding

ETI frames use Reed-Solomon (204,188) coding for error detection and correction.

```cpp
class ReedSolomonDecoder {
public:
    static constexpr uint8_t RS_CODE_LENGTH = 204;
    static constexpr uint8_t RS_DATA_LENGTH = 188;
    static constexpr uint8_t RS_PARITY_LENGTH = 16;
    
    struct DecodingResult {
        bool success;
        uint8_t errors_corrected;
        bool uncorrectable_error;
        std::vector<uint16_t> error_positions;
    };
    
    DecodingResult decode_frame(EtiFrame& frame);
    bool validate_frame(const EtiFrame& frame);
    uint8_t calculate_syndrome(const EtiFrame& frame);
    
private:
    static constexpr uint16_t GF_PRIMITIVE_POLYNOMIAL = 0x011D;  // x^8 + x^4 + x^3 + x^2 + 1
    
    uint8_t gf_multiply(uint8_t a, uint8_t b);
    uint8_t gf_divide(uint8_t a, uint8_t b);
    std::vector<uint8_t> calculate_syndromes(const EtiFrame& frame);
};
```

### CRC Validation

Frame-level CRC validation using CRC-16-CCITT:

```cpp
class CrcValidator {
public:
    static constexpr uint16_t CRC_POLYNOMIAL = 0x1021;    // x^16 + x^12 + x^5 + 1
    static constexpr uint16_t CRC_INITIAL_VALUE = 0xFFFF;
    
    uint16_t calculate_crc(const uint8_t* data, size_t length);
    bool validate_eti_frame(const EtiFrame& frame);
    void update_frame_crc(EtiFrame& frame);
    
private:
    static std::array<uint16_t, 256> crc_table_;
    static void initialize_crc_table();
};
```

## Timing and Synchronization

### Frame Timing Requirements

```cpp
class EtiTiming {
public:
    static constexpr std::chrono::microseconds FRAME_DURATION{24000};      // 24ms per frame
    static constexpr uint32_t FRAMES_PER_SECOND = 250;                     // Exact frame rate
    static constexpr uint32_t SAMPLES_PER_FRAME = 2048000 / FRAMES_PER_SECOND;  // 8192 samples
    
    struct TimingInfo {
        uint32_t frame_number;
        std::chrono::system_clock::time_point timestamp;
        std::chrono::microseconds frame_offset;
        bool timing_valid;
    };
    
    static TimingInfo calculate_frame_timing(uint32_t frame_count, uint32_t tist_value);
    static bool validate_frame_sequence(const std::vector<TimingInfo>& frames);
    static std::chrono::microseconds get_expected_frame_time(uint32_t frame_number);
};
```

### TIST (Time Stamp) Processing

```cpp
class TistProcessor {
public:
    static constexpr uint32_t TIST_CLOCK_FREQUENCY = 16384000;  // 16.384 MHz
    static constexpr double TIST_RESOLUTION_US = 1000000.0 / TIST_CLOCK_FREQUENCY;  // ~61.04 ns
    
    struct TistInfo {
        uint32_t tist_value;
        std::chrono::microseconds timestamp_us;
        bool valid;
        std::chrono::microseconds drift_from_expected;
    };
    
    static TistInfo decode_tist(uint32_t tist_raw);
    static bool validate_tist_sequence(const std::vector<TistInfo>& tist_values);
    static std::chrono::microseconds calculate_drift(const TistInfo& tist, uint32_t frame_number);
};
```

## Implementation Data Structures

### Complete ETI Frame Class

```cpp
class EtiFrame {
private:
    std::array<uint8_t, 6144> frame_data_;
    bool frame_valid_;
    std::chrono::system_clock::time_point receive_timestamp_;
    
public:
    // Construction and initialization
    EtiFrame();
    explicit EtiFrame(const std::array<uint8_t, 6144>& data);
    
    // Field accessors
    EtiSyncField get_sync_field() const;
    EtiLidataField get_lidata_field() const;
    EtiFicField get_fic_field() const;
    EtiMscField get_msc_field() const;
    EtiCrcField get_crc_field() const;
    
    // Validation methods
    bool validate_sync_pattern() const;
    bool validate_frame_structure() const;
    bool validate_crc() const;
    bool is_valid() const { return frame_valid_; }
    
    // Frame analysis
    std::vector<FigBlock> decode_fic_blocks() const;
    std::vector<SubChannelInfo> get_subchannel_info() const;
    Ensemble extract_ensemble_info() const;
    
    // Raw data access
    const uint8_t* data() const { return frame_data_.data(); }
    uint8_t* data() { return frame_data_.data(); }
    size_t size() const { return frame_data_.size(); }
    
    // Timing information
    void set_receive_timestamp(std::chrono::system_clock::time_point timestamp);
    std::chrono::system_clock::time_point get_receive_timestamp() const;
    std::chrono::microseconds get_frame_timestamp() const;
};
```

### ETI Stream Processor

```cpp
class EtiStreamProcessor {
public:
    struct ProcessingStatistics {
        uint32_t frames_processed;
        uint32_t valid_frames;
        uint32_t sync_errors;
        uint32_t crc_errors;
        uint32_t correctable_errors;
        uint32_t uncorrectable_errors;
        double frame_rate;
        std::chrono::microseconds avg_processing_time;
    };
    
    struct ProcessingResult {
        bool success;
        EtiFrame processed_frame;
        std::vector<std::string> warnings;
        std::vector<std::string> errors;
        ProcessingStatistics statistics;
    };
    
    ProcessingResult process_frame(const std::array<uint8_t, 6144>& raw_frame_data);
    ProcessingResult process_stream(std::istream& stream);
    
    void set_error_correction_enabled(bool enabled);
    void set_strict_validation_mode(bool strict);
    ProcessingStatistics get_current_statistics() const;
    
private:
    bool error_correction_enabled_;
    bool strict_validation_mode_;
    ProcessingStatistics statistics_;
    ReedSolomonDecoder rs_decoder_;
    CrcValidator crc_validator_;
};
```

### Ensemble Information Extractor

```cpp
class EnsembleExtractor {
public:
    struct ExtractionResult {
        bool success;
        Ensemble ensemble_info;
        std::vector<DabService> services;
        std::map<uint8_t, SubChannelInfo> sub_channels;
        std::vector<std::string> extraction_log;
    };
    
    ExtractionResult extract_from_frame(const EtiFrame& frame);
    ExtractionResult extract_from_stream(EtiStreamProcessor& processor);
    
    bool validate_ensemble_consistency(const Ensemble& ensemble);
    std::vector<std::string> check_service_completeness(const std::vector<DabService>& services);
    
private:
    FigDecoder fig_decoder_;
    std::map<uint8_t, std::unique_ptr<FigProcessor>> fig_processors_;
};
```

## Protocol Compliance Validation

### Validation Rules Engine

```cpp
class EtiProtocolValidator {
public:
    enum class ValidationLevel {
        BASIC_STRUCTURE,     // Frame structure and sync validation
        PROTOCOL_COMPLIANCE, // Full ETI protocol compliance
        BROADCAST_QUALITY   // Broadcast industry quality standards
    };
    
    struct ValidationResult {
        ValidationLevel level_achieved;
        bool protocol_compliant;
        std::vector<std::string> violations;
        std::vector<std::string> warnings;
        std::vector<std::string> recommendations;
        double compliance_score;  // 0.0 - 1.0
    };
    
    ValidationResult validate_frame(const EtiFrame& frame, ValidationLevel target_level);
    ValidationResult validate_stream(EtiStreamProcessor& processor, ValidationLevel target_level);
    ValidationResult validate_ensemble(const Ensemble& ensemble, ValidationLevel target_level);
    
private:
    bool validate_frame_structure(const EtiFrame& frame, std::vector<std::string>& violations);
    bool validate_fic_content(const EtiFrame& frame, std::vector<std::string>& violations);
    bool validate_msc_organization(const EtiFrame& frame, std::vector<std::string>& violations);
    bool validate_timing_constraints(const EtiFrame& frame, std::vector<std::string>& violations);
};
```

This comprehensive ETI protocol specification provides the foundation for implementing full ETSI EN 300 799 compliance in the StreamDAB Analyzer. The detailed data structures and validation methods ensure professional-grade ETI processing capability suitable for broadcast industry applications.

**Key Implementation Priorities:**
1. Frame synchronization and validation
2. FIC/FIG decoding and analysis
3. MSC sub-channel extraction
4. Error detection and correction
5. Real-time timing validation
6. Comprehensive compliance checking

**Next Steps:**
1. Implement core frame processing classes
2. Develop FIG decoder implementations
3. Create MSC data extraction methods
4. Establish error correction algorithms
5. Build timing validation framework
6. Integrate with ETISnoop for real-time processing