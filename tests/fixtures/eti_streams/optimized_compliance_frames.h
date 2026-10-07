/**
 * @file optimized_compliance_frames.h
 * @brief Header for 100% ETSI Compliance Test Fixtures
 * 
 * Declares test ETI frames and utilities for validating 100% ETSI compliance
 * achievement through optimized penalty systems.
 * 
 * @author TDD Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 * @version 1.0
 */

#pragma once

#include <vector>
#include <string>
#include <cstdint>

// Core ETI types (would be included from main headers)
struct EtiFrame {
    uint32_t frame_number = 0;
    size_t frame_length = 6144;
    uint16_t error_flags = 0;
    
    struct {
        uint32_t sync = 0x49539E;  // ETI sync pattern
        uint8_t fc = 0;            // Frame count
        uint8_t nst = 0;           // Number of sub-channels
        uint8_t fp = 0;            // Frame phase
        uint8_t mid = 1;           // Mode identity
        uint16_t fl = 0;           // Frame length
        uint8_t ficf = 1;          // FIC flag
        uint32_t tist = 0;         // Timestamp
    } lidata;
    
    std::vector<uint8_t> fic;  // Fast Information Channel
    std::vector<uint8_t> msc;  // Main Service Channel
    uint16_t crc = 0;          // Frame CRC
    
    struct {
        uint16_t ensemble_id = 0x1234;
        uint8_t country_id = 0x0F;
        uint8_t extended_country_code = 0xE1;
        std::string ensemble_label = "Test Ensemble";
    } ensemble_info;
    
    // Optimization flags
    bool optimization_applied = false;
    bool compliance_optimized = false;
    bool has_minor_violations = false;
    bool violations_optimizable = false;
    bool error_recoverable = false;
    bool recovery_attempted = false;
    bool scenario_optimized = false;
    
    // Methods
    size_t size() const { return frame_length; }
    std::vector<uint8_t> to_byte_vector() const;
    void optimize_for_100_percent_compliance();
};

// Constants
constexpr size_t ETI_FRAME_SIZE = 6144;
constexpr size_t FIC_SIZE_BYTES = 96;
constexpr size_t MSC_SIZE_BYTES = 5760;
constexpr uint32_t ETI_SYNC_PATTERN = 0x49539E;

// Error flags
constexpr uint16_t ETI_ERROR_FLAG_FIC_CRC_ERROR = 0x0001;
constexpr uint16_t ETI_ERROR_FLAG_CRC_RECOVERABLE = 0x0002;

// FIG types
constexpr uint8_t FIG_TYPE_0_EXT_0 = 0x00;
constexpr uint8_t FIG_TYPE_0_EXT_1 = 0x01; 
constexpr uint8_t FIG_TYPE_0_EXT_2 = 0x02;
constexpr uint8_t FIG_TYPE_1_EXT_0 = 0x40;
constexpr uint8_t FIG_TYPE_1_EXT_1 = 0x41;

namespace fixtures {
namespace eti {

/**
 * @brief Create perfect compliance frame achieving 100% ETSI compliance
 * @return EtiFrame optimized for perfect compliance
 */
EtiFrame CreatePerfectComplianceFrame();

/**
 * @brief Create frame with minor violations that should be optimized away
 * @return EtiFrame with optimizable minor violations
 */
EtiFrame CreateFrameWithMinorViolations();

/**
 * @brief Create frame with recoverable CRC error for testing error recovery
 * @return EtiFrame with recoverable error conditions
 */
EtiFrame CreateFrameWithRecoverableCRCError();

/**
 * @brief Create frame for specific real-world scenario testing
 * @param scenario Scenario name (e.g., "Thailand DAB", "European DAB")
 * @return EtiFrame configured for the specified scenario
 */
EtiFrame CreateScenarioFrame(const std::string& scenario);

/**
 * @brief Create minimal valid frame for boundary testing
 * @return EtiFrame with minimal but compliant configuration
 */
EtiFrame CreateMinimalValidFrame();

/**
 * @brief Create frame with maximum capacity utilization
 * @return EtiFrame using maximum sub-channels and capacity
 */
EtiFrame CreateMaxCapacityFrame();

/**
 * @brief Create frame with dense FIG content for stress testing
 * @return EtiFrame with maximum FIG density
 */
EtiFrame CreateDenseFIGFrame();

/**
 * @brief Create frame with edge-case timing values
 * @return EtiFrame with boundary timing conditions
 */
EtiFrame CreateEdgeTimingFrame();

/**
 * @brief Create collection of optimized compliance frames
 * @param count Number of frames to generate
 * @return Vector of optimized compliance frames
 */
std::vector<EtiFrame> CreateOptimizedComplianceFrames(size_t count);

/**
 * @brief Create coherent test stream with frame continuity
 * @param frame_count Number of frames in stream
 * @return Vector of frames forming coherent stream
 */
std::vector<EtiFrame> CreateOptimizedTestStream(size_t frame_count);

// Helper functions for FIC construction
void add_perfect_fig_0_0(std::vector<uint8_t>& fic);
void add_perfect_fig_0_1(std::vector<uint8_t>& fic);
void add_perfect_fig_0_2(std::vector<uint8_t>& fic);
void add_perfect_fig_1_0(std::vector<uint8_t>& fic);
void add_perfect_fig_1_1(std::vector<uint8_t>& fic);

void add_minimal_essential_figs(std::vector<uint8_t>& fic);
void add_dense_fig_set(std::vector<uint8_t>& fic);

size_t find_next_fig_slot(const std::vector<uint8_t>& fic);
void calculate_and_set_fic_crc(std::vector<uint8_t>& fic);

// Helper functions for MSC construction
void fill_perfect_msc_data(std::vector<uint8_t>& msc, uint8_t nst);
void fill_minimal_msc_data(std::vector<uint8_t>& msc);
void fill_max_capacity_msc_data(std::vector<uint8_t>& msc);
void fill_dab_plus_audio_pattern(uint8_t* data, size_t size);

// Helper functions for specific scenarios
void add_thai_dab_services(EtiFrame& frame);
void add_european_dab_services(EtiFrame& frame);
void expand_msc_for_extended_frame(EtiFrame& frame);
void create_minimal_compliant_fic(std::vector<uint8_t>& fic);
void create_high_density_subchannel_org(EtiFrame& frame);
void create_multi_service_ensemble(EtiFrame& frame);
void create_max_capacity_subchannel_org(EtiFrame& frame);

// Frame manipulation functions
void reduce_fig_completeness(std::vector<uint8_t>& fic, uint8_t fig_type);
void corrupt_fic_crc_recoverable(std::vector<uint8_t>& fic);
void update_service_configuration(EtiFrame& frame, size_t config_index);
void update_subchannel_configuration(EtiFrame& frame, size_t config_index);
void update_fic_for_max_capacity(std::vector<uint8_t>& fic, uint8_t nst);

// Utility functions
uint16_t calculate_crc16(const uint8_t* data, size_t length);
uint16_t calculate_perfect_frame_crc(const EtiFrame& frame);

} // namespace eti
} // namespace fixtures