/**
 * @file eti_types.h
 * @brief ETSI-Compliant ETI data type definitions and constants
 * 
 * This file contains the fundamental data types, structures, and constants
 * used throughout the ETI Stream Analyzer for DAB/DAB+ processing.
 * Fully compliant with ETSI EN 300 799 and ETSI EN 300 401 specifications.
 * 
 * @author Standards Compliance Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyzer Team
 */

#ifndef ETI_TYPES_H
#define ETI_TYPES_H

#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <chrono>
#include <memory>
#include <map>
#include <cstring>
#include <algorithm>
#include <QTime>
#include <QByteArray>

namespace eti {

// ETSI EN 300 799 Constants
constexpr size_t ETI_FRAME_SIZE = 6144;                    // bytes (ETSI EN 300 799)
constexpr size_t ETI_SYNC_SIZE = 4;                        // bytes
constexpr size_t ETI_LIDATA_SIZE = 8;                      // bytes
constexpr size_t ETI_FIC_SIZE = 32;                        // bytes
constexpr size_t ETI_MSC_SIZE = 6096;                      // bytes (6144 - 4 - 8 - 32 - 4)
constexpr size_t ETI_CRC_SIZE = 4;                         // bytes

constexpr uint32_t ETI_SAMPLE_RATE = 2048000;              // 2.048 MHz
constexpr uint32_t ETI_FRAMES_PER_SECOND = 250;            // 24ms per frame
constexpr std::chrono::microseconds ETI_FRAME_DURATION{24000}; // 24ms

// ETSI EN 300 401 Constants
constexpr size_t FIC_SIZE_BITS = 256;                      // bits
constexpr size_t FIC_SIZE_BYTES = 32;                      // bytes
constexpr size_t MAX_SUBCHANNEL_COUNT = 64;                // Maximum sub-channels
constexpr size_t MAX_CAPACITY_UNITS = 864;                 // Maximum CUs per frame
constexpr size_t CAPACITY_UNIT_SIZE = 8;                   // bytes per CU

// FIG Constants
constexpr size_t MAX_FIG_LENGTH = 29;                      // Maximum FIG data length
constexpr size_t FIG_HEADER_SIZE = 1;                      // bytes

// Sync pattern for ETI frames (ETSI EN 300 799)
constexpr std::array<uint8_t, 4> ETI_SYNC_PATTERN = {0x49, 0x93, 0x1E, 0x03};

// Forward declarations
struct EtiFrame;
struct FigBlock;

/**
 * @brief ETSI-compliant ETI Frame Sync Field
 * Reference: ETSI EN 300 799 Section 5.1
 */
struct EtiSyncField {
    std::array<uint8_t, 4> sync_bytes;
    
    EtiSyncField() : sync_bytes{} {
        set_sync_pattern();
    }
    
    bool is_valid() const {
        return std::memcmp(sync_bytes.data(), ETI_SYNC_PATTERN.data(), 4) == 0;
    }
    
    void set_sync_pattern() {
        std::memcpy(sync_bytes.data(), ETI_SYNC_PATTERN.data(), 4);
    }
};

/**
 * @brief ETSI-compliant ETI Frame LIDATA Field
 * Reference: ETSI EN 300 799 Section 5.2
 */
struct EtiLidataField {
    uint8_t fc;                 // Frame Count (0-249)
    uint8_t ficf : 1;          // FIC Flag (1=FIC present)
    uint8_t nst : 7;           // Number of Sub-channels in MSC (0-63)
    uint8_t fp : 3;            // Frame Phase (0-7)
    uint8_t mid : 2;           // Mode Identity (1-4)
    uint16_t fl : 11;          // Frame Length (11 bits fits in uint16_t)
    uint32_t tist;             // Time Stamp (32-bit)
    
    bool validate_frame_count(uint8_t expected_fc) const {
        return fc == expected_fc || fc == ((expected_fc + 1) % 250);
    }
    
    bool validate_mode(uint8_t expected_mode) const {
        return mid == expected_mode;
    }
    
    std::chrono::microseconds get_timestamp_us() const {
        // TIST is in units of 16.384 MHz clock periods
        return std::chrono::microseconds(static_cast<uint64_t>(tist) * 1000000 / 16384000);
    }
    
    bool validate_nst() const {
        return nst <= MAX_SUBCHANNEL_COUNT;
    }
};

/**
 * @brief ETSI-compliant FIC Field with FIG support
 * Reference: ETSI EN 300 401 Section 6.4
 */
struct EtiFicField {
    std::array<uint8_t, 32> fic_data;
    
    EtiFicField() : fic_data{} {
        fic_data.fill(0);
    }
    
    explicit EtiFicField(const std::array<uint8_t, 32>& data) : fic_data(data) {
    }
    
    std::vector<FigBlock> decode_fig_blocks() const;
    bool validate_fic_crc() const;
    bool contains_fig_type(uint8_t fig_type) const;
};

/**
 * @brief FIG Block structure for FIC analysis
 * Reference: ETSI EN 300 401 Section 5.2
 */
struct FigBlock {
    uint8_t fig_type : 5;       // FIG type (0-31)
    uint8_t length : 3;         // Length field
    bool continuation_flag;     // C: Continuation flag
    bool other_ensemble_flag;   // OE: Other Ensemble flag
    std::vector<uint8_t> data;  // FIG data field
    
    bool is_valid() const {
        return length <= MAX_FIG_LENGTH && 
               data.size() == length;
    }
    
    uint8_t get_extension() const {
        return data.empty() ? 0 : (data[0] & 0x1F);
    }
};

/**
 * @brief MSC Sub-channel Information
 * Reference: ETSI EN 300 401 Section 6.2
 */
struct SubChannelInfo {
    uint8_t sub_channel_id;     // SubChId (0-63)
    uint16_t start_address;     // Start address in CUs
    uint16_t size;              // Size in CUs
    uint8_t protection_level;   // Protection level (1-5)
    bool uep_flag;             // Unequal Error Protection flag
    
    uint16_t get_end_address() const {
        return start_address + size - 1;
    }
    
    bool validate_boundaries() const {
        return (start_address + size) <= MAX_CAPACITY_UNITS;
    }
    
    bool validate_sub_channel_id() const {
        return sub_channel_id < MAX_SUBCHANNEL_COUNT;
    }
    
    bool validate_protection_level() const {
        return protection_level >= 1 && protection_level <= 5;
    }
};

/**
 * @brief ETSI-compliant ETI Frame MSC Field
 * Reference: ETSI EN 300 799 Section 5.3
 */
struct EtiMscField {
    std::array<uint8_t, ETI_MSC_SIZE> msc_data;
    
    EtiMscField() : msc_data{} {
        msc_data.fill(0);
    }
    
    explicit EtiMscField(const std::array<uint8_t, ETI_MSC_SIZE>& data) : msc_data(data) {
    }
    
    std::vector<uint8_t> extract_subchannel_data(const SubChannelInfo& subchannel) const;
    bool validate_subchannel_organization(const std::vector<SubChannelInfo>& subchannels) const;
    
    size_t get_capacity_units_count() const {
        return ETI_MSC_SIZE / CAPACITY_UNIT_SIZE;  // 762 CUs
    }
};

/**
 * @brief ETSI-compliant ETI Frame CRC Field
 * Reference: ETSI EN 300 799 Section 5.4
 */
struct EtiCrcField {
    std::array<uint8_t, 4> crc_bytes;
    
    EtiCrcField() : crc_bytes{} {
        crc_bytes.fill(0);
    }
    
    bool validate_frame(const EtiFrame& frame) const;
    void calculate_crc(const EtiFrame& frame);
    uint32_t get_crc_value() const {
        return (static_cast<uint32_t>(crc_bytes[0]) << 24) |
               (static_cast<uint32_t>(crc_bytes[1]) << 16) |
               (static_cast<uint32_t>(crc_bytes[2]) << 8) |
               static_cast<uint32_t>(crc_bytes[3]);
    }
    
private:
    uint32_t calculate_frame_crc(const EtiFrame& frame) const;
};

/**
 * @brief Enhanced Service Component with ETSI compliance
 * Reference: ETSI EN 300 401 Section 8.1
 */
struct ServiceComponent {
    uint8_t sub_channel_id;     // Associated sub-channel
    uint16_t service_id;        // Parent service ID
    uint8_t component_type;     // Component type (audio, data, etc.)
    uint8_t tmid : 2;          // Transport Mechanism ID
    uint8_t asc_ty : 6;        // Audio Service Component Type
    bool primary;               // Primary/Secondary component
    bool ca_flag;              // Conditional Access flag
    std::string label;          // Component label
    
    // Default constructor with proper initialization
    ServiceComponent() 
        : sub_channel_id(0)
        , service_id(0)
        , component_type(0)
        , tmid(0)
        , asc_ty(0)
        , primary(false)
        , ca_flag(false)
        , label()
    {
    }
    
    bool validate_tmid() const {
        return tmid <= 3;
    }
    
    bool validate_asc_ty() const {
        return asc_ty <= 63;
    }
    
    bool validate_sub_channel_id() const {
        return sub_channel_id < MAX_SUBCHANNEL_COUNT;
    }
};

/**
 * @brief Enhanced DAB Service with ETSI compliance
 * Reference: ETSI EN 300 401 Section 8.1
 */
struct DabService {
    uint16_t service_id;               // SId: Service identifier
    uint8_t country_id;                // Country ID
    uint8_t extended_country_code;     // ECC: Extended Country Code
    std::string label;                 // Service label
    std::vector<ServiceComponent> components;
    bool is_programme;                 // Programme/Data service flag
    uint16_t character_flag;           // Character definition field
    
    bool validate_service_id() const {
        return service_id != 0x0000 && service_id != 0xFFFF;
    }
    
    bool validate_country_code() const {
        return country_id != 0x00 && country_id != 0xFF;
    }
    
    bool has_primary_component() const {
        return std::any_of(components.begin(), components.end(),
                          [](const ServiceComponent& comp) { return comp.primary; });
    }
    
    std::vector<uint8_t> get_subchannel_ids() const {
        std::vector<uint8_t> subchannel_ids;
        subchannel_ids.reserve(components.size()); // Pre-allocate for performance
        for (const auto& component : components) {
            subchannel_ids.push_back(component.sub_channel_id);
        }
        return subchannel_ids;
    }
};

/**
 * @brief Enhanced Ensemble with ETSI compliance
 * Reference: ETSI EN 300 401 Section 6.4
 */
struct Ensemble {
    uint16_t ensemble_id;              // EId: Ensemble identifier
    std::string label;                 // Ensemble label
    std::vector<DabService> services;  // Services in ensemble
    std::vector<SubChannelInfo> sub_channels; // Sub-channel organization
    uint8_t country_id;                // Country ID
    uint8_t extended_country_code;     // ECC: Extended Country Code
    uint16_t character_flag;           // Character definition field
    uint16_t cif_count;               // Current CIF count
    uint8_t occurrence_change;         // Occurrence change field
    bool alarm_flag;                   // Alarm announcement flag
    
    bool validate_ensemble_id() const {
        return ensemble_id != 0x0000 && ensemble_id != 0xFFFF;
    }
    
    bool validate_country_code() const {
        return country_id != 0x00 && country_id != 0xFF;
    }
    
    bool validate_subchannel_organization() const {
        // Check for overlapping sub-channels
        for (size_t i = 0; i < sub_channels.size(); ++i) {
            for (size_t j = i + 1; j < sub_channels.size(); ++j) {
                const auto& a = sub_channels[i];
                const auto& b = sub_channels[j];
                if (!(a.start_address + a.size <= b.start_address ||
                      b.start_address + b.size <= a.start_address)) {
                    return false;  // Overlap detected
                }
            }
        }
        return true;
    }
    
    uint16_t calculate_total_capacity() const {
        uint16_t total = 0;
        for (const auto& sub_ch : sub_channels) {
            total += sub_ch.size;
        }
        return total;
    }
    
    std::vector<uint16_t> get_service_ids() const {
        std::vector<uint16_t> service_ids;
        service_ids.reserve(services.size()); // Pre-allocate for performance
        for (const auto& service : services) {
            service_ids.push_back(service.service_id);
        }
        return service_ids;
    }
};

/**
 * @brief Complete ETSI-compliant ETI Frame
 * Reference: ETSI EN 300 799
 */
struct EtiFrame {
    std::array<uint8_t, ETI_FRAME_SIZE> frame_data;
    bool frame_valid;
    std::chrono::system_clock::time_point receive_timestamp;
    
    EtiFrame() : frame_valid(false) {
        frame_data.fill(0);
        receive_timestamp = std::chrono::system_clock::now();
    }
    
    explicit EtiFrame(const std::array<uint8_t, ETI_FRAME_SIZE>& data) 
        : frame_data(data), frame_valid(false) {
        receive_timestamp = std::chrono::system_clock::now();
        initialize_frame_validation();
    }
    
    // Field accessors
    EtiSyncField get_sync_field() const {
        EtiSyncField sync;
        std::memcpy(sync.sync_bytes.data(), frame_data.data(), 4);
        return sync;
    }
    
    EtiLidataField get_lidata_field() const {
        EtiLidataField lidata{};
        const uint8_t* data = frame_data.data() + 4;
        
        // Parse LIDATA field according to ETSI EN 300 799 Section 5.2
        lidata.fc = data[0];                    // Frame Count
        lidata.ficf = (data[1] & 0x80) ? 1 : 0; // FIC Flag
        lidata.nst = data[1] & 0x7F;            // Number of Sub-channels
        lidata.fp = (data[2] >> 5) & 0x07;      // Frame Phase
        lidata.mid = (data[2] >> 3) & 0x03;     // Mode Identity
        lidata.fl = ((data[2] & 0x07) << 8) | data[3]; // Frame Length
        
        // Time Stamp (TIST) - 32-bit value
        lidata.tist = (static_cast<uint32_t>(data[4]) << 24) |
                      (static_cast<uint32_t>(data[5]) << 16) |
                      (static_cast<uint32_t>(data[6]) << 8) |
                      static_cast<uint32_t>(data[7]);
        
        return lidata;
    }
    
    EtiFicField get_fic_field() const {
        EtiFicField fic;
        std::memcpy(fic.fic_data.data(), frame_data.data() + 12, 32);
        return fic;
    }
    
    EtiMscField get_msc_field() const {
        EtiMscField msc;
        std::memcpy(msc.msc_data.data(), frame_data.data() + 44, ETI_MSC_SIZE);
        return msc;
    }
    
    EtiCrcField get_crc_field() const {
        EtiCrcField crc;
        std::memcpy(crc.crc_bytes.data(), frame_data.data() + ETI_FRAME_SIZE - 4, 4);
        return crc;
    }
    
    // Validation methods
    bool validate_sync_pattern() const {
        auto sync = get_sync_field();
        return sync.is_valid();
    }
    
    bool validate_frame_structure() const {
        bool valid = validate_sync_pattern();
        if (valid) {
            auto crc = get_crc_field();
            valid = crc.validate_frame(*this);
        }
        return valid;
    }
    
    bool is_valid() const { 
        return frame_valid; 
    }
    
    // Frame analysis
    std::vector<FigBlock> decode_fic_blocks() const {
        auto fic = get_fic_field();
        return fic.decode_fig_blocks();
    }
    
    std::vector<SubChannelInfo> get_subchannel_info() const;
    Ensemble extract_ensemble_info() const;
    
    // Raw data access
    const uint8_t* data() const { 
        return frame_data.data(); 
    }
    
    uint8_t* data() { 
        return frame_data.data(); 
    }
    
    size_t size() const { 
        return frame_data.size(); 
    }
    
    // Timing information
    void set_receive_timestamp(std::chrono::system_clock::time_point timestamp) {
        receive_timestamp = timestamp;
    }
    
    std::chrono::system_clock::time_point get_receive_timestamp() const {
        return receive_timestamp;
    }
    
    std::chrono::microseconds get_frame_timestamp() const {
        auto lidata = get_lidata_field();
        return lidata.get_timestamp_us();
    }
    
private:
    void initialize_frame_validation() {
        // Basic validation - check if frame has minimum required size
        frame_valid = (frame_data.size() >= ETI_SYNC_SIZE + ETI_LIDATA_SIZE);
    }
    
    // FIG parsing helper methods
    void parse_fig_0_0(const FigBlock& fig, Ensemble& ensemble) const;
    void parse_fig_0_1(const FigBlock& fig, std::vector<SubChannelInfo>& subchannels) const;
    void parse_fig_0_2(const FigBlock& fig, Ensemble& ensemble) const;
    void parse_fig_0_3(const FigBlock& fig, Ensemble& ensemble) const;
    void parse_fig_1_0(const FigBlock& fig, Ensemble& ensemble) const;
    void parse_fig_1_1(const FigBlock& fig, Ensemble& ensemble) const;
};

// Type aliases for compatibility and convenience
using EtiFrameArray = std::array<uint8_t, ETI_FRAME_SIZE>;
using FicBlockArray = std::array<uint8_t, FIC_SIZE_BYTES>;

/**
 * @brief Simple ETI frame data structure for decoder interfaces
 */
struct EtiFrameData {
    quint32 frameNumber;
    QTime timestamp;
    QByteArray data;
    bool isValid;
    
    EtiFrameData() : frameNumber(0), isValid(false) {}
};

/**
 * @brief ETI Frame validation result
 */
struct ValidationResult {
    bool valid;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    ValidationResult(bool v = true) : valid(v) {}
    
    void add_error(const std::string& error) {
        errors.push_back(error);
        valid = false;
    }
    
    void add_warning(const std::string& warning) {
        warnings.push_back(warning);
    }
    
    bool has_errors() const {
        return !errors.empty();
    }
    
    bool has_warnings() const {
        return !warnings.empty();
    }
};

/**
 * @brief Service Type enumeration for DAB services
 * Reference: ETSI EN 300 401 Section 8.1
 */
enum class ServiceType : uint8_t {
    DAB_AUDIO = 0x00,       // DAB audio service
    DAB_PLUS_AUDIO = 0x01,  // DAB+ audio service (HE-AAC)
    DATA_SERVICE = 0x02,    // Data service
    PACKET_MODE = 0x03,     // Packet mode data
    MSC_DATA = 0x04,        // MSC stream data
    FIDC = 0x05,           // Fast Information Data Channel
    UNKNOWN = 0xFF          // Unknown/invalid service type
};

/**
 * @brief Service Information structure for testing and analysis
 * This structure provides a simplified interface for service discovery tests
 * while maintaining compatibility with the full DabService structure.
 */
struct ServiceInfo {
    uint16_t service_id;        // Service identifier (SId)
    std::string label;          // Service name/label (changed from 'name' to match test expectations)
    ServiceType service_type;   // Type of service
    uint16_t bitrate;          // Service bitrate in kbps
    uint8_t protection_level;   // Protection level (1-5)
    
    // Constructor with default values
    ServiceInfo() 
        : service_id(0)
        , label()
        , service_type(ServiceType::UNKNOWN)
        , bitrate(0)
        , protection_level(1)
    {
    }
    
    // Constructor for easy initialization in tests
    ServiceInfo(uint16_t sid, const std::string& service_label, 
                ServiceType type, uint16_t br, uint8_t protection)
        : service_id(sid)
        , label(service_label)
        , service_type(type)
        , bitrate(br)
        , protection_level(protection)
    {
    }
    
    // Validation methods
    bool is_valid() const {
        return service_id != 0 && 
               service_type != ServiceType::UNKNOWN &&
               protection_level >= 1 && protection_level <= 5;
    }
    
    // Type checking methods
    bool is_audio_service() const {
        return service_type == ServiceType::DAB_AUDIO || 
               service_type == ServiceType::DAB_PLUS_AUDIO;
    }
    
    bool is_data_service() const {
        return service_type == ServiceType::DATA_SERVICE ||
               service_type == ServiceType::PACKET_MODE ||
               service_type == ServiceType::MSC_DATA ||
               service_type == ServiceType::FIDC;
    }
    
    bool is_dab_plus() const {
        return service_type == ServiceType::DAB_PLUS_AUDIO;
    }
    
    // Convert to full DabService structure
    DabService to_dab_service() const {
        DabService service;
        service.service_id = service_id;
        service.label = label;
        service.is_programme = is_audio_service();
        return service;
    }
};

/**
 * @brief Protection level enumeration
 * Reference: ETSI EN 300 401 Section 11.1
 */
enum class ProtectionLevel : uint8_t {
    LEVEL_1 = 1,    // Code rate 1/4, highest protection
    LEVEL_2 = 2,    // Code rate 3/8
    LEVEL_3 = 3,    // Code rate 1/2
    LEVEL_4 = 4,    // Code rate 3/4
    LEVEL_5 = 5     // Code rate 1, lowest protection
};

/**
 * @brief FIG type enumeration for type safety
 */
enum class FigType : uint8_t {
    FIG_0_0_ENSEMBLE_INFO = 0x00,
    FIG_0_1_SUBCHANNEL_ORG = 0x01,
    FIG_0_2_SERVICE_ORG = 0x02,
    FIG_0_3_SERVICE_COMPONENT = 0x03,
    FIG_1_0_ENSEMBLE_LABEL = 0x10,
    FIG_1_1_SERVICE_LABEL = 0x11,
    // Additional FIG types as needed
};

/**
 * @brief ETSI Compliance Test Result
 * Used for testing and validation of ETSI compliance features
 */
struct ETSIComplianceResult {
    bool is_compliant;                    // Overall compliance status
    double compliance_score;              // Compliance score (0.0-100.0)
    std::vector<std::string> errors;      // Compliance errors found
    std::vector<std::string> warnings;    // Compliance warnings
    std::string etsi_standard;           // ETSI standard reference (e.g., "EN 300 401")
    std::string test_date;               // ISO date string of test
    
    // Constructor
    ETSIComplianceResult() 
        : is_compliant(false)
        , compliance_score(0.0)
        , etsi_standard("EN 300 401")
        , test_date()
    {
    }
    
    // Add error with automatic compliance status update
    void add_error(const std::string& error) {
        errors.push_back(error);
        is_compliant = false;
        recalculate_score();
    }
    
    // Add warning
    void add_warning(const std::string& warning) {
        warnings.push_back(warning);
        recalculate_score();
    }
    
    // Set compliance with score
    void set_compliance(bool compliant, double score = 100.0) {
        is_compliant = compliant;
        compliance_score = score;
    }
    
    // Calculate compliance percentage
    double get_compliance_percentage() const {
        return compliance_score;
    }
    
    // Check if fully compliant (100%)
    bool is_fully_compliant() const {
        return is_compliant && compliance_score >= 100.0 && errors.empty();
    }
    
private:
    void recalculate_score() {
        // Simple scoring: start at 100, subtract for each error/warning
        compliance_score = 100.0 - (static_cast<double>(errors.size()) * 10.0) - (static_cast<double>(warnings.size()) * 2.0);
        compliance_score = std::max(0.0, compliance_score);
        
        if (compliance_score < 100.0) {
            is_compliant = false;
        }
    }
};

} // namespace eti

#endif // ETI_TYPES_H