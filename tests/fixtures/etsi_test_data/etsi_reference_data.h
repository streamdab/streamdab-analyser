#pragma once

#include <cstdint>
#include <vector>
#include <array>

/**
 * @brief ETSI Reference Test Data for Network Streaming Tests
 * Contains reference ETI frames and expected data for validation
 */

namespace eti::test {

// Reference ETI frame data (6144 bytes)
extern const std::array<uint8_t, 6144> ETSI_REFERENCE_ETI_FRAME;

// Expected service information from reference frame
struct ETSIReferenceService {
    uint16_t service_id;
    std::string service_label;
    bool is_dab_plus;
    uint16_t subchannel_id;
    uint16_t start_address;
    uint16_t length;
};

// Reference services expected in test data
extern const std::vector<ETSIReferenceService> ETSI_REFERENCE_SERVICES;

// Test multicast addresses for ETI-over-IP
extern const char* ETSI_TEST_MULTICAST_ADDRESS;
extern const uint16_t ETSI_TEST_MULTICAST_PORT;

} // namespace eti::test