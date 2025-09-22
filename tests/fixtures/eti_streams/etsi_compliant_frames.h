#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <string>

/**
 * @file etsi_compliant_frames.h
 * @brief ETSI EN 300 799 compliant ETI test data for comprehensive testing
 * 
 * This file provides fully ETSI-compliant ETI frames with proper structure,
 * timing, and content for thorough TDD testing of ETI processing capabilities.
 */

namespace EtsiTestData {

/**
 * @brief ETSI-compliant ETI frame with proper structure
 * 
 * Frame Structure (ETSI EN 300 799):
 * - SYNC (4 bytes): 0x49, 0x93, 0x1E, 0x03
 * - LIDATA (8 bytes): Frame characterization
 * - FIC (32 bytes): Fast Information Channel
 * - MSC (6096 bytes): Main Service Channel  
 * - CRC (4 bytes): Reed-Solomon error check
 */
class EtsiEtiFrame {
public:
    static constexpr size_t FRAME_SIZE = 6144;
    static constexpr size_t SYNC_SIZE = 4;
    static constexpr size_t LIDATA_SIZE = 8;
    static constexpr size_t FIC_SIZE = 32;
    static constexpr size_t MSC_SIZE = 6096;
    static constexpr size_t CRC_SIZE = 4;
    
    static constexpr uint8_t SYNC_PATTERN[4] = {0x49, 0x93, 0x1E, 0x03};
    
    std::array<uint8_t, FRAME_SIZE> data;
    
    EtsiEtiFrame();
    
    // Structure validation - Public interface for ETSI EN 300 799 compliance
    bool validate_sync_pattern() const;
    bool validate_frame_structure() const;  // Public for network processing validation
    bool validate_crc() const;
    bool is_etsi_compliant() const;
    
    // Alternative public validation method for network integration
    bool isValid() const { return validate_frame_structure(); }
    
    // Field accessors
    uint8_t get_frame_count() const;
    uint8_t get_sub_channel_count() const;
    uint16_t get_ensemble_id() const;
    std::vector<uint8_t> get_fic_data() const;
    std::vector<uint8_t> get_msc_data() const;
    
    // Field setters
    void set_frame_count(uint8_t fc);
    void set_sub_channel_count(uint8_t nst);
    void set_ensemble_id(uint16_t eid);
    void update_crc();
    
private:
    uint16_t calculate_crc16(const uint8_t* data, size_t length) const;
};

/**
 * @brief German public radio ensemble (typical real-world example)
 */
extern const EtsiEtiFrame GERMAN_PUBLIC_RADIO_ENSEMBLE;

/**
 * @brief UK BBC Radio ensemble
 */
extern const EtsiEtiFrame UK_BBC_RADIO_ENSEMBLE;

/**
 * @brief French Radio France ensemble
 */
extern const EtsiEtiFrame FRENCH_RADIO_FRANCE_ENSEMBLE;

/**
 * @brief Multi-format ensemble (DAB + DAB+ + data services)
 */
extern const EtsiEtiFrame MULTI_FORMAT_ENSEMBLE;

/**
 * @brief Minimal valid ensemble (single service)
 */
extern const EtsiEtiFrame MINIMAL_VALID_ENSEMBLE;

/**
 * @brief Maximum capacity ensemble (63 sub-channels)
 */
extern const EtsiEtiFrame MAXIMUM_CAPACITY_ENSEMBLE;

/**
 * @brief Error injection test frames
 */
namespace ErrorFrames {
    extern const EtsiEtiFrame INVALID_SYNC_FRAME;
    extern const EtsiEtiFrame CORRUPTED_FIC_FRAME;
    extern const EtsiEtiFrame INVALID_CRC_FRAME;
    extern const EtsiEtiFrame TIMING_ERROR_FRAME;
    extern const EtsiEtiFrame MALFORMED_MSC_FRAME;
}

/**
 * @brief Real-time stream simulation
 */
class EtsiStreamSimulator {
public:
    struct StreamConfig {
        std::string ensemble_label;
        uint16_t ensemble_id;
        uint8_t service_count;
        std::vector<std::string> service_labels;
        uint32_t frame_rate = 250; // frames per second
        bool include_errors = false;
        double error_rate = 0.01; // 1% error rate
    };
    
    /**
     * @brief Generate a real-time compliant ETI stream
     * @param config Stream configuration
     * @param duration_seconds Stream duration in seconds
     * @return Vector of ETSI-compliant frames
     */
    static std::vector<EtsiEtiFrame> generate_stream(
        const StreamConfig& config, 
        double duration_seconds
    );
    
    /**
     * @brief Generate high-performance test stream (>900 frames/second capability)
     * @param frame_count Number of frames to generate
     * @return High-performance test stream
     */
    static std::vector<EtsiEtiFrame> generate_performance_stream(size_t frame_count);
    
    /**
     * @brief Generate stream with timing validation data
     * @param frame_count Number of frames
     * @return Stream with proper TIST timing
     */
    static std::vector<EtsiEtiFrame> generate_timing_test_stream(size_t frame_count);
};

/**
 * @brief FIG (Fast Information Group) test data with ETSI compliance
 */
namespace FigTestData {
    
    // FIG 0/0: Ensemble information
    struct Fig0_0_EnsembleInfo {
        uint16_t ensemble_id;
        uint8_t change_flags;
        uint16_t cif_count;
        
        std::vector<uint8_t> encode() const;
        static Fig0_0_EnsembleInfo decode(const std::vector<uint8_t>& data);
    };
    
    // FIG 0/1: Sub-channel organization
    struct Fig0_1_SubChannelOrg {
        struct SubChannel {
            uint8_t sub_channel_id;
            uint16_t start_address;
            uint16_t size;
            uint8_t protection_level;
            bool uep_flag;
        };
        
        std::vector<SubChannel> sub_channels;
        
        std::vector<uint8_t> encode() const;
        static Fig0_1_SubChannelOrg decode(const std::vector<uint8_t>& data);
    };
    
    // FIG 0/2: Service organization
    struct Fig0_2_ServiceOrg {
        struct Service {
            uint16_t service_id;
            uint8_t country_id;
            uint8_t extended_country_code;
            std::vector<uint8_t> components;
        };
        
        std::vector<Service> services;
        
        std::vector<uint8_t> encode() const;
        static Fig0_2_ServiceOrg decode(const std::vector<uint8_t>& data);
    };
    
    // FIG 1/0: Ensemble label
    struct Fig1_0_EnsembleLabel {
        uint16_t ensemble_id;
        std::string label; // UTF-8, max 16 characters
        uint16_t character_flag;
        
        std::vector<uint8_t> encode() const;
        static Fig1_0_EnsembleLabel decode(const std::vector<uint8_t>& data);
    };
    
    // FIG 1/1: Service labels
    struct Fig1_1_ServiceLabel {
        uint16_t service_id;
        std::string label; // UTF-8, max 16 characters
        uint16_t character_flag;
        
        std::vector<uint8_t> encode() const;
        static Fig1_1_ServiceLabel decode(const std::vector<uint8_t>& data);
    };
    
    // Complete FIC blocks for testing
    extern const std::vector<uint8_t> GERMAN_ENSEMBLE_FIC;
    extern const std::vector<uint8_t> UK_ENSEMBLE_FIC;
    extern const std::vector<uint8_t> FRENCH_ENSEMBLE_FIC;
    extern const std::vector<uint8_t> MULTI_SERVICE_FIC;
    extern const std::vector<uint8_t> MINIMAL_FIC;
}

/**
 * @brief Audio service test data (DAB and DAB+)
 */
namespace AudioServiceData {
    
    // DAB audio (MPEG-1 Layer 2) test frames
    struct DabAudioFrame {
        static constexpr size_t MP2_HEADER_SIZE = 4;
        
        uint16_t sync_word = 0xFFFC;
        uint8_t mpeg_header[MP2_HEADER_SIZE];
        std::vector<uint8_t> audio_data;
        
        std::vector<uint8_t> encode() const;
        static DabAudioFrame decode(const std::vector<uint8_t>& data);
        bool is_valid() const;
    };
    
    // DAB+ audio (HE-AAC) test frames
    struct DabPlusAudioFrame {
        static constexpr size_t AAC_HEADER_SIZE = 7;
        
        uint16_t sync_word = 0xFFF1;
        uint8_t aac_header[AAC_HEADER_SIZE];
        std::vector<uint8_t> aac_data;
        
        std::vector<uint8_t> encode() const;
        static DabPlusAudioFrame decode(const std::vector<uint8_t>& data);
        bool is_valid() const;
    };
    
    // Pre-generated audio test samples
    extern const std::vector<DabAudioFrame> SAMPLE_DAB_AUDIO_SEQUENCE;
    extern const std::vector<DabPlusAudioFrame> SAMPLE_DABPLUS_AUDIO_SEQUENCE;
    
    // Audio service configurations
    struct AudioServiceConfig {
        std::string service_label;
        uint16_t service_id;
        uint8_t sub_channel_id;
        bool is_dab_plus;
        uint16_t bit_rate;
        uint8_t protection_level;
    };
    
    extern const std::vector<AudioServiceConfig> TYPICAL_AUDIO_SERVICES;
}

/**
 * @brief Performance test data generators
 */
class PerformanceTestData {
public:
    /**
     * @brief Generate test data for >900 frames/second processing validation
     * @param frame_count Number of frames to generate
     * @return High-performance test data
     */
    static std::vector<EtsiEtiFrame> generate_high_speed_test_data(size_t frame_count);
    
    /**
     * @brief Generate stress test data with maximum complexity
     * @param frame_count Number of frames
     * @return Maximum complexity test frames
     */
    static std::vector<EtsiEtiFrame> generate_stress_test_data(size_t frame_count);
    
    /**
     * @brief Generate memory usage test data
     * @param size_mb Target memory usage in MB
     * @return Large dataset for memory testing
     */
    static std::vector<EtsiEtiFrame> generate_memory_test_data(size_t size_mb);
    
    /**
     * @brief Generate real-time timing validation data
     * @param duration_seconds Stream duration
     * @return Frames with precise timing information
     */
    static std::vector<EtsiEtiFrame> generate_timing_validation_data(double duration_seconds);
};

/**
 * @brief Network stream test data for integration testing
 */
namespace NetworkTestData {
    
    // Network packet simulation
    struct EtiNetworkPacket {
        static constexpr size_t UDP_HEADER_SIZE = 8;
        static constexpr size_t ETI_PACKET_SIZE = FRAME_SIZE + UDP_HEADER_SIZE;
        
        uint8_t udp_header[UDP_HEADER_SIZE];
        EtsiEtiFrame eti_frame;
        
        std::vector<uint8_t> encode() const;
        static EtiNetworkPacket decode(const std::vector<uint8_t>& data);
    };
    
    // Network stream configurations
    struct NetworkStreamConfig {
        std::string multicast_address = "239.192.0.1";
        uint16_t port = 9200;
        uint32_t packet_rate = 250; // packets per second
        bool include_jitter = false;
        bool include_packet_loss = false;
        double loss_rate = 0.001; // 0.1% packet loss
    };
    
    /**
     * @brief Generate network stream test data
     * @param config Network configuration
     * @param duration_seconds Stream duration
     * @return Network packet sequence
     */
    static std::vector<EtiNetworkPacket> generate_network_stream(
        const NetworkStreamConfig& config,
        double duration_seconds
    );
    
    /**
     * @brief Generate network error scenarios
     * @param base_stream Base stream to corrupt
     * @return Stream with network-specific errors
     */
    static std::vector<EtiNetworkPacket> generate_network_error_scenarios(
        const std::vector<EtiNetworkPacket>& base_stream
    );
}

/**
 * @brief Integration test support data
 */
namespace IntegrationTestData {
    
    /**
     * @brief Complete ensemble with all ETSI-required elements
     */
    struct CompleteEnsemble {
        EtsiEtiFrame frame;
        FigTestData::Fig0_0_EnsembleInfo ensemble_info;
        FigTestData::Fig0_1_SubChannelOrg subchannel_org;
        FigTestData::Fig0_2_ServiceOrg service_org;
        FigTestData::Fig1_0_EnsembleLabel ensemble_label;
        std::vector<FigTestData::Fig1_1_ServiceLabel> service_labels;
        std::vector<AudioServiceData::AudioServiceConfig> audio_services;
        
        bool validate_consistency() const;
        std::vector<std::string> get_validation_errors() const;
    };
    
    // Pre-configured test ensembles
    extern const CompleteEnsemble BBC_RADIO_1_ENSEMBLE;
    extern const CompleteEnsemble DEUTSCHLANDFUNK_ENSEMBLE;
    extern const CompleteEnsemble RADIO_FRANCE_ENSEMBLE;
    extern const CompleteEnsemble MULTI_COUNTRY_ENSEMBLE;
    
    /**
     * @brief Generate test data for UI/UX Agent GUI testing
     * @return GUI-specific test scenarios
     */
    static std::vector<CompleteEnsemble> generate_gui_test_scenarios();
    
    /**
     * @brief Generate test data for Build Manager validation
     * @return Build system integration test data
     */
    static std::vector<EtsiEtiFrame> generate_build_test_data();
    
    /**
     * @brief Generate test data for Standards Compliance validation
     * @return ETSI compliance test scenarios
     */
    static std::vector<CompleteEnsemble> generate_compliance_test_data();
}

} // namespace EtsiTestData