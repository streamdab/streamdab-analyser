/**
 * @file etsi_compliant_frames.h
 * @brief ETSI-Compliant Test Frame Data Generators
 * 
 * Provides test data generation for ETSI-compliant ETI frames, FIG blocks,
 * and service discovery scenarios for comprehensive testing.
 * 
 * @author TDD Agent
 * @date 2025
 */

#ifndef ETSI_COMPLIANT_FRAMES_H
#define ETSI_COMPLIANT_FRAMES_H

#include <vector>
#include <array>
#include <cstdint>
#include <string>
#include "core/eti_types.h"

namespace eti {
namespace test {

/**
 * @brief Generate ETSI-compliant ETI frame with valid sync pattern
 */
std::array<uint8_t, ETI_FRAME_SIZE> generate_valid_eti_frame(
    uint8_t frame_count = 0,
    uint16_t ensemble_id = 0x1001,
    const std::vector<ServiceInfo>& services = {}
);

/**
 * @brief Generate FIG 0/0 block for ensemble information
 */
std::vector<uint8_t> generate_fig_0_0_block(
    uint16_t ensemble_id,
    uint8_t country_id = 0xE0,
    uint8_t extended_country_code = 0x01
);

/**
 * @brief Generate FIG 0/1 block for subchannel organization
 */
std::vector<uint8_t> generate_fig_0_1_block(
    const std::vector<SubChannelInfo>& subchannels
);

/**
 * @brief Generate FIG 0/2 block for service organization
 */
std::vector<uint8_t> generate_fig_0_2_block(
    const std::vector<ServiceInfo>& services
);

/**
 * @brief Generate FIG 1/0 block for ensemble label
 */
std::vector<uint8_t> generate_fig_1_0_block(
    uint16_t ensemble_id,
    const std::string& ensemble_label
);

/**
 * @brief Generate FIG 1/1 block for service labels
 */
std::vector<uint8_t> generate_fig_1_1_block(
    const std::vector<ServiceInfo>& services
);

/**
 * @brief Generate complete FIC field with multiple FIG blocks
 */
std::array<uint8_t, FIC_SIZE_BYTES> generate_fic_field(
    const std::vector<std::vector<uint8_t>>& fig_blocks
);

/**
 * @brief Generate test service set for comprehensive testing
 */
std::vector<ServiceInfo> generate_test_services(size_t count = 10);

/**
 * @brief Generate test subchannel organization
 */
std::vector<SubChannelInfo> generate_test_subchannels(size_t count = 10);

/**
 * @brief Generate large service set for performance testing
 */
std::vector<ServiceInfo> generate_large_service_set(size_t count = 1000);

/**
 * @brief Generate corrupted ETI frame for error testing
 */
std::array<uint8_t, ETI_FRAME_SIZE> generate_corrupted_eti_frame(
    eti::test::ErrorType error_type
);

/**
 * @brief Error types for corrupted frame generation
 */
enum class ErrorType {
    INVALID_SYNC,
    CRC_MISMATCH,
    INVALID_FIG,
    SUBCHANNEL_OVERLAP,
    INVALID_SERVICE_ID
};

} // namespace test
} // namespace eti

#endif // ETSI_COMPLIANT_FRAMES_H